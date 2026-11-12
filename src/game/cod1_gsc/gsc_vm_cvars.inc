/* R116 selected ordinary-string getter domain and shared localized builder.
 * No generic native import. Raw 0x1000 describes original registration only;
 * the game host owns target flags/name policy. Undefined is port result policy. */
static int cvar_runtime_preflight(const GscProgram *g,const GscCvarHost *host,char *error,size_t size) {
    int n,kind;
    for(n=0;n<g->code_count;n++) {
        const Instr *i=&g->code[n];
        if(i->kind!=I_CALL||i->target>=-1-CVAR_TARGET_BASE||i->target<-1-SHELLSHOCK_TARGET_BASE)continue;
        kind=-1-CVAR_TARGET_BASE-i->target;
        if(!g->cvars_mode||i->module[0]||cvar_descriptor(g,i->name)!=kind||
           find_func(g,i->owner_module,i->name)>=0||cvar_static_issues(g,kind,i->args,i->argc)) {
            if(error&&size)snprintf(error,size,"line %d: invalid cvar binding",i->line);
            return 0;
        }
        if(!host||!host->publish) {
            if(error&&size)snprintf(error,size,"line %d: cvar requires selected publish host capability",i->line);
            return 0;
        }
    }
    for(n=0;n<g->expr_count;n++) {
        const Expr *e=&g->expr[n];
        if(e->call_binding!=GSC_EXPR_TARGET_CVAR_NATIVE)continue;
        kind=e->target;
        if(!g->cvars_mode||e->kind!=E_NATIVE_CALL||e->module[0]||
           cvar_descriptor(g,e->name)!=kind||find_func(g,e->owner_module,e->name)>=0||
           cvar_static_issues(g,kind,e->args,e->argc)) {
            if(error&&size)snprintf(error,size,"line %d: invalid cvar expression binding",e->line);
            return 0;
        }
        if(!host||!host->publish) {
            if(error&&size)snprintf(error,size,"line %d: cvar expression requires selected publish host capability",e->line);
            return 0;
        }
    }
    return 1;
}
static int cvar_call(GscRuntime *r,int kind,const Value *args,int argc,int line,Value *out) {
    GscCvarEvent event,copy;char error[256];const char *end;size_t length;int accepted,context,logical_ms;
    if(!r->cvars_enabled||!r->names_enabled||r->error[0]||!out||
       cvar_static_issues(r->program,kind,NULL,argc)||(!args&&argc))
        return rfail(r,line,"invalid cvar structural invocation");
    /* Intrinsic getter order is observable even for missing arguments. */
    if(argc<1)return rfail(r,line,"setcvar missing GetString argument 0");
    if(args[0].kind!=V_STRING)return rfail(r,line,"setcvar GetString argument 0 requires selected ordinary string domain");
    end=memchr(args[0].str,0,sizeof(args[0].str));
    if(!end)return rfail(r,line,"setcvar unterminated name");
    length=(size_t)(end-args[0].str);
    if(length>=sizeof(event.name))return rfail(r,line,"setcvar oversized name");
    memset(&event,0,sizeof(event));memcpy(event.name,args[0].str,length+1);
    if(argc<2)return rfail(r,line,"setcvar missing GetType argument 1");
    if(args[1].kind==V_LOCALIZED_STRING) {
        if(!hud_text_compose(r,args+1,argc-1,line,event.value))return 0;
    } else {
        if(args[1].kind!=V_STRING)return rfail(r,line,"setcvar GetString argument 1 requires selected ordinary string domain");
        end=memchr(args[1].str,0,sizeof(args[1].str));
        if(!end)return rfail(r,line,"setcvar unterminated value");
        length=(size_t)(end-args[1].str);
        memcpy(event.value,args[1].str,length+1);
    }
    event.length=strlen(event.value);event.native_flags=GSC_CVAR_NATIVE_FLAGS;
    if(!r->cvars.publish)return rfail(r,line,"cvar publish host capability is unavailable");
    if(!r->next_cvar_operation)return rfail(r,line,"cvar operation token exhausted");
    event.operation_id=r->next_cvar_operation;
    r->next_cvar_operation=event.operation_id==UINT64_MAX?0:event.operation_id+1;
    logical_ms=r->now_ms;event.phase=GSC_CVAR_REGISTER;copy=event;
    error[0]=0;accepted=r->cvars.publish(r->cvars.user,logical_ms,&copy,error,sizeof(error));error[sizeof(error)-1]=0;
    if(r->error[0])return 0;
    if(accepted!=1)return rfail(r,line,"cvar register host rejected publication: %s",error[0]?error:"unspecified host failure");
    context=console_context(r);if(context<0)return 0;
    /* Receiver retirement cancels the task; no SET after a committed prefix. */
    if(!context){memset(out,0,sizeof(*out));return 1;}
    event.phase=GSC_CVAR_SET;copy=event;
    error[0]=0;accepted=r->cvars.publish(r->cvars.user,logical_ms,&copy,error,sizeof(error));error[sizeof(error)-1]=0;
    if(r->error[0])return 0;
    if(accepted!=1)return rfail(r,line,"cvar set host rejected publication: %s",error[0]?error:"unspecified host failure");
    context=console_context(r);if(context<0)return 0;
    memset(out,0,sizeof(*out));return 1;
}
