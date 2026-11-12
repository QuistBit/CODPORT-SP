/* R117 original server-name registration wrapper; client .shock acquisition
 * and effect application are separate. Selected ordinary-string transport
 * domain, initial ready-drain scope and undefined result are port policy. */
static int shellshock_runtime_preflight(const GscProgram *g,const GscShellshockHost *host,char *error,size_t size) {
    int n,kind;
    for(n=0;n<g->code_count;n++) {
        const Instr *i=&g->code[n];
        if(i->kind!=I_CALL||i->target>=-1-SHELLSHOCK_TARGET_BASE||i->target<-1-DEBUG_TEXT_TARGET_BASE)continue;
        kind=-1-SHELLSHOCK_TARGET_BASE-i->target;
        if(!g->shellshock_mode||i->module[0]||shellshock_descriptor(g,i->name)!=kind||
           find_func(g,i->owner_module,i->name)>=0||shellshock_static_issues(g,kind,i->args,i->argc)) {
            if(error&&size)snprintf(error,size,"line %d: invalid shellshock binding",i->line);
            return 0;
        }
        if(!host||!host->publish) {
            if(error&&size)snprintf(error,size,"line %d: shellshock requires selected publish host capability",i->line);
            return 0;
        }
    }
    for(n=0;n<g->expr_count;n++) {
        const Expr *e=&g->expr[n];
        if(e->call_binding!=GSC_EXPR_TARGET_SHELLSHOCK_NATIVE)continue;
        kind=e->target;
        if(!g->shellshock_mode||e->kind!=E_NATIVE_CALL||e->module[0]||
           shellshock_descriptor(g,e->name)!=kind||find_func(g,e->owner_module,e->name)>=0||
           shellshock_static_issues(g,kind,e->args,e->argc)) {
            if(error&&size)snprintf(error,size,"line %d: invalid shellshock expression binding",e->line);
            return 0;
        }
        if(!host||!host->publish) {
            if(error&&size)snprintf(error,size,"line %d: shellshock expression requires selected publish host capability",e->line);
            return 0;
        }
    }
    return 1;
}
static int shellshock_call(GscRuntime *r,int kind,const Value *args,int argc,int line,Value *out) {
    GscShellshockEvent event,copy;char error[256];const char *end;size_t length,n;int accepted,context;
    if(!r->shellshock_enabled||!r->names_enabled||r->error[0]||!out||
       shellshock_static_issues(r->program,kind,NULL,argc)||(!args&&argc))
        return rfail(r,line,"invalid shellshock structural invocation");
    /* Original phase guard precedes GetString0, including missing/wrong type. */
    if(!r->shellshock_initial_window)
        return rfail(r,line,"precacheShellshock must be called during initial synchronous start before later advances");
    if(argc<1)return rfail(r,line,"precacheShellshock missing GetString argument 0");
    if(args[0].kind!=V_STRING)
        return rfail(r,line,"precacheShellshock GetString argument 0 requires selected ordinary string domain");
    end=memchr(args[0].str,0,sizeof(args[0].str));
    if(!end)return rfail(r,line,"precacheShellshock unterminated name");
    length=(size_t)(end-args[0].str);
    if(length>=sizeof(event.name))return rfail(r,line,"precacheShellshock oversized name");
    for(n=0;n<length;n++) {
        unsigned char c=(unsigned char)args[0].str[n];
        if(c<32||c>126||c=='"')return rfail(r,line,"precacheShellshock name outside selected configstring transport domain");
    }
    if(!length){memset(out,0,sizeof(*out));return 1;}
    if(!r->shellshock.publish)return rfail(r,line,"shellshock publish host capability is unavailable");
    if(!r->next_shellshock_operation)return rfail(r,line,"shellshock operation token exhausted");
    memset(&event,0,sizeof(event));memcpy(event.name,args[0].str,length+1);
    event.operation_id=r->next_shellshock_operation;
    r->next_shellshock_operation=event.operation_id==UINT64_MAX?0:event.operation_id+1;
    copy=event;error[0]=0;
    accepted=r->shellshock.publish(r->shellshock.user,r->now_ms,&copy,error,sizeof(error));error[sizeof(error)-1]=0;
    if(r->error[0])return 0;
    if(accepted!=1)return rfail(r,line,"shellshock host rejected registration: %s",error[0]?error:"unspecified host failure");
    context=console_context(r);if(context<0)return 0;
    /* Retirement keeps the accepted external prefix and cancels its task. */
    memset(out,0,sizeof(*out));return 1;
}
