/* Called only in the opt-in detailed profile path. Capture entry identity
 * before source can pop frames, retire the task or execute nested scripts. */
static void r55_source_capture(const GscRuntime *r,const Task *task,GscSourceSample55 *s){
    const GscProgram *g=r->program;const Frame *frame;const Func *fun;
    memset(s,0,sizeof(*s));s->serial=task->serial;s->depth=task->depth;s->fn=-1;s->pc=-1;
    if(!g||!g->funcs||!g->modules||task->depth<=0||task->depth>MAX_FRAMES)return;
    frame=&task->frames[task->depth-1];s->fn=frame->fn;s->pc=frame->pc;
    if(frame->fn<0||frame->fn>=g->func_count)return;
    fun=&g->funcs[frame->fn];if(fun->module<0||fun->module>=g->module_count)return;
    memcpy(s->module,g->modules[fun->module],sizeof(s->module)-1);
    memcpy(s->function,fun->name,sizeof(s->function)-1);s->known=1;
    if(g->code&&frame->pc>=0&&frame->pc<g->code_count&&frame->pc>=fun->start&&frame->pc<fun->end)
        s->line=g->code[frame->pc].line;
}
static int r55_run_source(GscRuntime *r,Task *task,int *budget,GscR237Profile53 *p){
    GscSourceSample55 sample;int before=*budget,ok;
    uint64_t prior=p->ticks[GSC53_SOURCE],failures=p->clock_failures,t;
    r55_source_capture(r,task,&sample);
    t=r237_profile_begin53(p);
    ok=r->results_enabled?run_task_results(r,task,budget):run_task(r,task,budget);
    r237_profile_end53(p,GSC53_SOURCE,t);
    if(p->clock_failures==failures){
        sample.ticks=p->ticks[GSC53_SOURCE]-prior;
        if(before>*budget)sample.budget_spent=(uint64_t)((int64_t)before-*budget);
        if(sample.ticks>p->source_worst.ticks)p->source_worst=sample;
    }
    return ok;
}
