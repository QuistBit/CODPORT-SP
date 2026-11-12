/* R112 wrapper effects are phased so accepted int1 survives a later map getter
 * refusal. They record diagnostic intent; no engine transition or serializer is
 * substituted. Original success/failure markers are independent of task life. */
static GscMissionCallback mission_callback(const GscMissionHost *host,int kind) {
    if(!host)return NULL;
    return kind==GSC_MISSION_SUCCESS?host->success:kind==GSC_MISSION_FAILED?host->failed:NULL;
}
static int mission_runtime_preflight(const GscProgram *g,const GscMissionHost *host,char *error,size_t size) {
    int n,kind;
    for(n=0;n<g->code_count;n++) {
        const Instr *i=&g->code[n];
        if(i->kind!=I_CALL||i->target>=-1-MISSION_TARGET_BASE||i->target<-1-BLAST_TARGET_BASE)continue;
        kind=-1-MISSION_TARGET_BASE-i->target;
        if(!g->mission_mode||i->module[0]||mission_descriptor(g,i->name)!=kind||
           find_func(g,i->owner_module,i->name)>=0||mission_static_issues(g,kind,i->args,i->argc)) {
            if(error&&size)snprintf(error,size,"line %d: invalid mission binding",i->line);
            return 0;
        }
        if(!host||!host->query_gate||!mission_callback(host,kind)) {
            if(error&&size)snprintf(error,size,"line %d: mission requires gate and selected host capability",i->line);
            return 0;
        }
    }
    for(n=0;n<g->expr_count;n++) {
        const Expr *e=&g->expr[n];
        if(e->call_binding!=GSC_EXPR_TARGET_MISSION_NATIVE)continue;
        kind=e->target;
        if(!g->mission_mode||e->kind!=E_NATIVE_CALL||e->module[0]||
           mission_descriptor(g,e->name)!=kind||find_func(g,e->owner_module,e->name)>=0||
           mission_static_issues(g,kind,e->args,e->argc)) {
            if(error&&size)snprintf(error,size,"line %d: invalid mission expression binding",e->line);
            return 0;
        }
        if(!host||!host->query_gate||!mission_callback(host,kind)) {
            if(error&&size)snprintf(error,size,"line %d: mission expression requires gate and selected host capability",e->line);
            return 0;
        }
    }
    return 1;
}
/* -1 failure, 0 retired context, 1 continuing context. Accepted prior effects
 * remain accepted when a later callback or bounded getter refuses. */
static int mission_emit(GscRuntime *r,GscMissionCallback callback,const GscMissionEvent *event,int line) {
    char error[256];int accepted;
    error[0]=0;accepted=callback(r->mission.user,r->now_ms,event,error,sizeof(error));error[sizeof(error)-1]=0;
    if(r->error[0])return -1;
    if(accepted!=1) {
        rfail(r,line,"mission host rejected effect: %s",error[0]?error:"unspecified host failure");return -1;
    }
    return console_context(r);
}
static int mission_statement(GscRuntime *r,int kind,const Value *args,int argc,int line) {
    GscMissionEvent event;GscMissionCallback callback;char error[256];int accepted,enabled=-1,context;const char *end;
    if(!r->mission_enabled||!r->names_enabled||r->error[0]||
       mission_static_issues(r->program,kind,NULL,argc)||(!args&&argc))
        return rfail(r,line,"invalid mission invocation");
    callback=mission_callback(&r->mission,kind);
    if(!r->mission.query_gate||!callback)return rfail(r,line,"mission host capability is unavailable");
    /* >2 success is a wrapper arity refusal before gate; zero arguments must
     * reach the shared native gate before its missing GetFloat(2) boundary. */
    error[0]=0;accepted=r->mission.query_gate(r->mission.user,r->now_ms,kind,&enabled,error,sizeof(error));error[sizeof(error)-1]=0;
    if(r->error[0])return 0;
    if(accepted!=1)return rfail(r,line,"mission host rejected gate: %s",error[0]?error:"unspecified host failure");
    context=console_context(r);if(context<0)return 0;
        if(!context)return 1;
    if(enabled!=0&&enabled!=1)return rfail(r,line,"mission gate returned invalid enabled state");
    memset(&event,0,sizeof(event));event.kind=kind;
    if(kind==GSC_MISSION_FAILED) {
        if(!enabled)return 1;
        event.phase=GSC_MISSION_DEATH_FADE;context=mission_emit(r,callback,&event,line);
        if(context<0)return 0;
        if(!context)return 1;
        event.phase=GSC_MISSION_FAILURE_MARK;context=mission_emit(r,callback,&event,line);
        return context>=0;
    }
    if(enabled) {
        if(!argc)return rfail(r,line,"missionsuccess requires unavailable missing GetFloat argument 2");
        if(argc==2) {
            if(!scalar_get_integer(r,&args[1],line,&event.arg1))return 0;
            event.has_arg1=1;event.phase=GSC_MISSION_ARGUMENT;
            context=mission_emit(r,callback,&event,line);if(context<0)return 0;
        if(!context)return 1;
        }
        /* Native GetInt(1) and its persistent write precede GetString(0).
         * Unproved engine string coercions are rejected only after the gate. */
        if(args[0].kind!=V_STRING)return rfail(r,line,"mission map getter admits ordinary VM strings only");
        end=memchr(args[0].str,0,sizeof(args[0].str));
        if(!end)return rfail(r,line,"unterminated mission map string");
        event.length=(size_t)(end-args[0].str);memcpy(event.map,args[0].str,event.length+1);
        event.phase=GSC_MISSION_REQUEST;context=mission_emit(r,callback,&event,line);
        if(context<0)return 0;
        if(!context)return 1;
    }
    memset(&event,0,sizeof(event));event.kind=GSC_MISSION_SUCCESS;event.phase=GSC_MISSION_SUCCESS_MARK;
    context=mission_emit(r,callback,&event,line);return context>=0;
}
