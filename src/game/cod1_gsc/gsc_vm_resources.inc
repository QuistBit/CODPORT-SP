/* R108 typed resource effects. The original DLL permits these natives only
 * around its synchronous initial level ExecuteThread call. The port separately
 * selects an initial-start admission window; later advances cannot reopen it.
 * Original helpers compare resource identity without ASCII case sensitivity;
 * this VM preserves spelling and the genuine host owns identity/index policy. */
static int resource_runtime_preflight(const GscProgram *g,const GscResourceHost *host,
    char *error,size_t size) {
    int n,kind;
    for(n=0;n<g->code_count;n++) {
        const Instr *i=&g->code[n];
        if(i->kind!=I_CALL||i->target>=-1-RESOURCE_TARGET_BASE||i->target<-1-OBJECTIVE_TARGET_BASE)continue;
        kind=-1-RESOURCE_TARGET_BASE-i->target;
        if(!g->resources_mode||i->module[0]||resource_descriptor(g,i->name)!=kind||
           find_func(g,i->owner_module,i->name)>=0||!resource_argc_valid(kind,i->argc)) {
            if(error&&size)snprintf(error,size,"line %d: invalid resource binding",i->line);
            return 0;
        }
        if(!host||!(kind==GSC_RESOURCE_SHADER?host->shader:host->model)) {
            if(error&&size)snprintf(error,size,"line %d: precache %s requires enabled host capability",
                i->line,kind==GSC_RESOURCE_SHADER?"shader":"model");
            return 0;
        }
    }
    for(n=0;n<g->expr_count;n++) {
        const Expr *e=&g->expr[n];
        if(e->call_binding!=GSC_EXPR_TARGET_RESOURCE_NATIVE)continue;
        kind=e->target;
        if(!g->resources_mode||e->kind!=E_NATIVE_CALL||e->module[0]||
           resource_descriptor(g,e->name)!=kind||find_func(g,e->owner_module,e->name)>=0||
           resource_static_issues(g,kind,e->args,e->argc)) {
            if(error&&size)snprintf(error,size,"line %d: invalid resource expression binding",e->line);
            return 0;
        }
        if(!host||!(kind==GSC_RESOURCE_SHADER?host->shader:host->model)) {
            if(error&&size)snprintf(error,size,"line %d: precache %s expression requires enabled host capability",
                e->line,kind==GSC_RESOURCE_SHADER?"shader":"model");
            return 0;
        }
    }
    return 1;
}
static int resource_statement(GscRuntime *r,int kind,const Value *args,int argc,int line) {
    GscResourceEvent event;GscResourceCallback callback;char error[256];
    const char *end;size_t length;int ok;
    if(!r->resources_enabled||!r->names_enabled||r->error[0]||!resource_argc_valid(kind,argc))
        return rfail(r,line,"invalid resource invocation");
    /* The phase gate precedes native GetString, as in the original wrappers.
     * Argument expressions still finish before entering the native call. */
    if(r->resources_closed)return rfail(r,line,"precache must occur during initial level startup before a wait");
    if(!args||args[0].kind!=V_STRING)return rfail(r,line,"precache admitted argument must be a string");
    end=memchr(args[0].str,0,sizeof(args[0].str));
    if(!end)return rfail(r,line,"unterminated resource name");
    length=(size_t)(end-args[0].str);
    if(length>=sizeof(event.name))return rfail(r,line,"oversized resource name");
    if(!length) {
        if(kind==GSC_RESOURCE_MODEL)return 1;
        return rfail(r,line,"shader name cannot be empty");
    }
    memset(&event,0,sizeof(event));event.kind=kind;memcpy(event.name,args[0].str,length+1);
    callback=kind==GSC_RESOURCE_SHADER?r->resources.shader:r->resources.model;
    if(!callback)return rfail(r,line,"resource host capability is unavailable");
    error[0]=0;ok=callback(r->resources.user,r->now_ms,&event,error,sizeof(error));error[sizeof(error)-1]=0;
    if(r->error[0])return 0;
    if(!entity_task_current(r))return 0;
    if(ok!=1)return rfail(r,line,"resource host rejected effect: %s",error[0]?error:"unspecified host failure");
    return 1;
}
