/* R98 deliberately bounded predicate bridge. The original native body uses
 * argument 0; exact one-argument admission is our port policy. The host alone
 * represents real actor/player state. No allocator or classname creates it. */
static int predicate_expression(GscRuntime *r,int descriptor,const Value *args,
                                int argc,int line,Value *out) {
    GscEntityRef identity,ref;Value result;char error[256];int alive,got=-1,ok;
    int (*callback)(void *,const GscEntityRef *,int *,char *,size_t);
    if(!r->predicates_enabled||!args||argc!=1||!out||
       (descriptor!=ENTITY_GLOBAL_ISALIVE&&descriptor!=ENTITY_GLOBAL_ISSENTIENT))
        return rfail(r,line,"invalid predicate binding or arity");
    memset(&result,0,sizeof(result));result.kind=V_NUMBER;result.integer=1;
    /* Original nonentity values are false. No host callback is needed. */
    if(args[0].kind!=V_ENTITY) {*out=result;return 1;}
    identity=args[0].entity;alive=state_entity_live(r,&identity);
    if(alive<0)return 0;
    if(r->executing_task&&!refresh_state_context(r,r->executing_task))return 0;
    if(r->executing_task&&r->executing_task->status==TASK_DEAD)return 0;
    if(!alive) {*out=result;return 1;}
    callback=descriptor==ENTITY_GLOBAL_ISALIVE?r->predicates.isalive:r->predicates.issentient;
    if(!callback)return rfail(r,line,"predicate host capability is unavailable");
    ref=identity;error[0]=0;ok=callback(r->predicates.user,&ref,&got,error,sizeof(error));
    error[sizeof(error)-1]=0;
    if(!entity_task_current(r))return 0;
    if(r->error[0])return 0;
    if(!ok)return rfail(r,line,"predicate host: %s",error);
    if(got!=0&&got!=1)return rfail(r,line,"predicate host returned invalid boolean");
    if(r->executing_task&&!refresh_state_context(r,r->executing_task))return 0;
    if(r->executing_task&&r->executing_task->status==TASK_DEAD)return 0;
    /* A true answer may not publish after its object is retired/replaced.
     * Stale=false is explicit port lifetime policy, not original handle proof. */
    alive=state_entity_live(r,&identity);
    if(alive<0)return 0;
    if(r->executing_task&&!refresh_state_context(r,r->executing_task))return 0;
    if(r->executing_task&&r->executing_task->status==TASK_DEAD)return 0;
    result.number=(float)(alive&&got);*out=result;return 1;
}
static int predicate_runtime_preflight(const GscProgram *g,
    const GscEntityPredicateHost *host,char *error,size_t size) {
    int n;
    for(n=0;n<g->expr_count;n++) {
        const Expr *e=&g->expr[n];
        if(e->kind!=E_NATIVE_CALL||e->call_binding!=GSC_EXPR_TARGET_ENTITY_NATIVE)continue;
        if((e->target==ENTITY_GLOBAL_ISALIVE&&(!host||!host->isalive))||
           (e->target==ENTITY_GLOBAL_ISSENTIENT&&(!host||!host->issentient))) {
            if(error&&size)snprintf(error,size,"line %d: %s requires an enabled predicate capability",e->line,e->name);
            return 0;
        }
    }
    return 1;
}
