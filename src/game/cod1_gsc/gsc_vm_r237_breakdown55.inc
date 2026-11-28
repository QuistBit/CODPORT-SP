/* Same-owner, synchronous continuation scope. Nested immediate script calls
 * inherit this observer; never carry it across a yield or GAME phase return. */
static int r55_run_source_breakdown(GscRuntime *r,Task *task,int *budget,GscR237Profile53 *p){
    int ok;
    if(r->r55_breakdown_scope)return r55_run_source(r,task,budget,p);
    if(!r->results_enabled){
        ++p->breakdown.legacy_sources;
        return r55_run_source(r,task,budget,p);
    }
    p->breakdown.clock=p->clock;
    ++p->breakdown.sources;
    r->r55_breakdown_scope=&p->breakdown;
    ok=r55_run_source(r,task,budget,p);
    r->r55_breakdown_scope=NULL;
    return ok;
}
