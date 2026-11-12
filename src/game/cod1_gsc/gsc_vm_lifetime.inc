/* R106 task storage policy. Legacy modes keep their embedded 32 monotonic
 * slots. Lifetime mode allocates at most 128 stable slots lazily: a live task's
 * Task, Frame and continuation pointers never move while spawning another.
 * Physical slot order is unrelated to scheduling; serial is the birth order.
 * This bounded allocation policy is not a claim about the original VM cap. */
static Task *task_at(const GscRuntime *r,int n) {
    return r->lifetime_enabled?r->lifetime_tasks[n]:(Task *)&r->tasks[n];
}
static void task_results_clear(Task *task) {
    if(task->results)GSC_RUNTIME_FREE(task->results);
    task->results=NULL;
}
static Task *task_acquire(GscRuntime *r,int line) {
    Task *task=NULL;int n;
    if(!r->lifetime_enabled) {
        if(r->task_count>=MAX_THREADS) {rfail(r,line,"task limit");return NULL;}
        task=&r->tasks[r->task_count++];
    } else {
        /* Reserve serial UINT64_MAX as the deterministic exhaustion sentinel.
         * Never wrap to an earlier birth order, including after a slot dies. */
        if(r->next_serial==UINT64_MAX) {rfail(r,line,"task serial exhausted");return NULL;}
        for(n=0;n<r->task_count;n++) {
            Task *candidate=task_at(r,n);
            if(candidate->status==TASK_DEAD&&candidate!=r->executing_task&&!candidate->host_entry_pins&&!candidate->retained_owner_refs) {
                task=candidate;break;
            }
        }
        if(!task) {
            if(r->task_count>=GSC_LIFETIME_TASK_CAPACITY) {rfail(r,line,"task limit");return NULL;}
            task=GSC_RUNTIME_CALLOC(1,sizeof(*task));
            if(!task) {rfail(r,line,"task allocation failed");return NULL;}
            r->lifetime_tasks[r->task_count++]=task;
        }
        /* Clear ResultsTask first, then every owner/payload/frame/self/flow
         * field. DEAD task memory cannot deliver a prior wait into a new birth. */
        task_results_clear(task);
    }
    memset(task,0,sizeof(*task));
    if(r->frame_cadence_enabled){task->wake_ms=r->executing_task?r->cadence_drain_ms:r->now_ms;
        if(r->executing_task)task->cadence_event_resume=r->executing_task->cadence_event_resume;}
    return task;
}
