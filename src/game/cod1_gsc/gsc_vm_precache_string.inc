/* Original 0x200355a0 checks startup before GetLocalizedString(0), returns
 * without a pool call for empty text, and has no argc guard or Add* result.
 * This optional port admits ordinary/localized printable strings only. The
 * engine GetLocalizedString coercion and native argument evaluation order are
 * still boundaries. The existing deferred child scheduler is unchanged. */
static int precache_string_runtime_preflight(const GscProgram *g,
    const GscPrecacheStringHost *host,char *error,size_t size) {
    int n;
    if(!g || !g->precache_string_mode)return 0;
    for(n=0;n<g->code_count;++n) {
        const Instr *i=&g->code[n];
        if(i->kind!=I_CALL ||
           i->target!=-1-NATIVE_QUERY_TARGET_BASE-GSC_QUERY_PRECACHE_STRING)continue;
        if(i->module[0] || native_query_descriptor(g,i->name)!=GSC_QUERY_PRECACHE_STRING ||
           find_func(g,i->owner_module,i->name)>=0 ||
           native_query_static_issues(g,GSC_QUERY_PRECACHE_STRING,i->args,i->argc) ||
           !host || !host->startup || !host->publish) {
            if(error&&size)snprintf(error,size,"precacheString requires selected startup and public pool host capabilities");
            return 0;
        }
    }
    for(n=0;n<g->expr_count;++n) {
        const Expr *e=&g->expr[n];
        if(e->call_binding!=GSC_EXPR_TARGET_NATIVE_QUERY ||
           e->target!=GSC_QUERY_PRECACHE_STRING)continue;
        if(e->kind!=E_NATIVE_CALL || e->module[0] ||
           native_query_descriptor(g,e->name)!=GSC_QUERY_PRECACHE_STRING ||
           find_func(g,e->owner_module,e->name)>=0 ||
           native_query_static_issues(g,GSC_QUERY_PRECACHE_STRING,e->args,e->argc) ||
           !host || !host->startup || !host->publish) {
            if(error&&size)snprintf(error,size,"precacheString expression requires selected startup and public pool host capabilities");
            return 0;
        }
    }
    return 1;
}
int gsc_runtime_precache_string_startup(const GscRuntime *r,int now) {
    return r && r->started && r->advancing && r->precache_string_enabled &&
        (r->configured_precache_interval_enabled?configured_precache_active(r):r->precache_string_initial_window) && !r->error[0] &&
        now>=0 && r->now_ms==now;
}
static int precache_string_call(GscRuntime *r,const Value *args,int argc,
    int line,Value *out) {
    GscPrecacheStringEvent event,copy;char error[256];int accepted,now,context;
    if(!r || !r->precache_string_enabled || !r->names_enabled || !out ||
       argc<0 || argc>MAX_PARAMS || r->error[0])
        return rfail(r,line,"invalid precacheString structural invocation");
    now=r->now_ms;
    /* Value expressions already evaluated. Native gate precedes arg0 access,
     * including missing arguments, bad types and the empty-string no-op. */
    if(!configured_precache_current(r) || !gsc_runtime_precache_string_startup(r,now) || !r->precache_string.startup ||
       r->precache_string.startup(r->precache_string.user,now)!=1 ||
       !configured_precache_current(r) || !gsc_runtime_precache_string_startup(r,now))
        return rfail(r,line,"precacheString must be called before any wait statements in the gametype or level script");
    if(argc<1 || !args)return rfail(r,line,"precacheString GetLocalizedString argument 0 is missing");
    if(args[0].kind!=V_STRING && args[0].kind!=V_LOCALIZED_STRING)
        return rfail(r,line,"precacheString argument 0 requires admitted ordinary or localized string");
    memset(&event,0,sizeof(event));memcpy(event.name,args[0].str,sizeof(event.name));
    if(!precache_string_name_valid(&event))
        return rfail(r,line,"precacheString name exceeds admitted printable 127-byte string domain");
    if(!event.name[0]) {memset(out,0,sizeof(*out));return 1;}
    if(!r->precache_string.publish)return rfail(r,line,"precacheString public pool host unavailable");
    if(!r->next_precache_string_operation)return rfail(r,line,"precacheString operation token exhausted");
    event.operation_id=r->next_precache_string_operation;
    r->next_precache_string_operation=event.operation_id==UINT64_MAX?0:event.operation_id+1;
    copy=event;error[0]=0;
    accepted=r->precache_string.publish(r->precache_string.user,now,&copy,error,sizeof(error));
    error[sizeof(error)-1]=0;
    if(r->error[0])return 0;
    if(accepted!=1)return rfail(r,line,"precacheString public pool host refused: %s",error[0]?error:"unspecified host failure");
    if(memcmp(&event,&copy,sizeof(event)) || !configured_precache_current(r) || !gsc_runtime_precache_string_startup(r,now))
        return rfail(r,line,"precacheString publication changed intent or startup lease");
    context=console_context(r);if(context<=0)return 0;
    if(!configured_precache_current(r)||!gsc_runtime_precache_string_startup(r,now))return rfail(r,line,"precacheString context changed startup lease");
    memset(out,0,sizeof(*out));return 1;
}
