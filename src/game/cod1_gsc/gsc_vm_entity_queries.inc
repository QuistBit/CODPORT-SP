/* R149 shared selector/preflight. Host layouts of earlier modes are frozen. */
static int entity_query_host_ready(const GscEntityQueryHost *host,int kind) {
    return host&&(kind==GSC_ENTITY_QUERY_BY_NUMBER?host->by_number!=NULL:
        kind==GSC_ENTITY_QUERY_SPAWNER_TEAM?host->collect_team!=NULL:0);
}
static int entity_queries_runtime_preflight(const GscProgram *g,
    const GscEntityQueryHost *host,char *error,size_t size) {
    int n,kind;
    for(n=0;n<g->code_count;n++) {
        const Instr *i=&g->code[n];
        if(i->kind!=I_CALL||i->target>=-1-ENTITY_QUERY_TARGET_BASE||
           i->target<-1-ENTITY_QUERY_TARGET_BASE-GSC_ENTITY_QUERY_SPAWNER_TEAM)continue;
        kind=-1-ENTITY_QUERY_TARGET_BASE-i->target;
        if(!g->entity_queries_mode||i->module[0]||
           find_func(g,i->owner_module,i->name)>=0||entity_query_descriptor(g,i->name)!=kind||
           entity_query_static_issues(g,kind,i->args,i->argc)||!entity_query_host_ready(host,kind)) {
            if(error&&size)snprintf(error,size,"line %d: entity query requires its selected host capability",i->line);
            return 0;
        }
    }
    for(n=0;n<g->expr_count;n++) {
        const Expr *e=&g->expr[n];
        if(e->call_binding!=GSC_EXPR_TARGET_ENTITY_QUERY_NATIVE)continue;
        if(!g->entity_queries_mode||e->kind!=E_NATIVE_CALL||e->module[0]||
           find_func(g,e->owner_module,e->name)>=0||entity_query_descriptor(g,e->name)!=e->target||
           entity_query_static_issues(g,e->target,e->args,e->argc)||!entity_query_host_ready(host,e->target)) {
            if(error&&size)snprintf(error,size,"line %d: entity query expression requires its selected host capability",e->line);
            return 0;
        }
    }
    return 1;
}
static int entity_query_call(GscRuntime *r,int kind,const Value *args,int argc,
    int line,Value *out) {
    if(!r||!r->entity_queries_enabled||
       entity_query_static_issues(r->program,kind,NULL,argc))
        return r?rfail(r,line,"invalid selected entity query invocation"):0;
    if(kind==GSC_ENTITY_QUERY_BY_NUMBER)return entity_number_call(r,args,argc,line,out);
    return spawner_team_array_call(r,args,argc,line,out);
}
