/* R106 task storage policy. Legacy modes keep their embedded 32 monotonic
 * slots. Lifetime mode allocates at most 128 stable slots lazily: a live task's
 * Task, Frame and continuation pointers never move while spawning another.
 * Physical slot order is unrelated to scheduling; serial is the birth order.
 * This bounded allocation policy is not a claim about the original VM cap. */
static Task *task_at(const GscRuntime *r,int n) {
    if(!r->lifetime_enabled)return (Task *)&r->tasks[n];
    if(n<GSC_LIFETIME_TASK_CAPACITY)return r->lifetime_tasks[n];
    return r->extra_tasks[n-GSC_LIFETIME_TASK_CAPACITY];
}
static void task_results_clear(Task *task) {
    if(task->results)GSC_RUNTIME_FREE(task->results);
    task->results=NULL;
}
/* Grow only the directory. Tasks, frames and controller ownership stay
 * at stable addresses. The legacy lifetime selection retains its ceiling. */
static int task_reserve_next(GscRuntime *r,int line){
    int count=r->extra_capacity;
    Task **directory;
    if(r->task_count<task_directory_limit(r))return 1;
    if(!r->expand_map_tasks)return rfail(r,line,"task limit");
    if(count>(INT_MAX-GSC_LIFETIME_TASK_CAPACITY)/2)return rfail(r,line,"task directory overflow");
    count=count?count*2:GSC_LIFETIME_TASK_CAPACITY;
    if((size_t)count>SIZE_MAX/sizeof(*directory))return rfail(r,line,"task directory overflow");
    directory=GSC_RUNTIME_CALLOC((size_t)count,sizeof(*directory));
    if(!directory)return rfail(r,line,"task directory allocation failed");
    if(r->extra_capacity)memcpy(directory,r->extra_tasks,(size_t)r->extra_capacity*sizeof(*directory));
    GSC_RUNTIME_FREE(r->extra_tasks);r->extra_tasks=directory;r->extra_capacity=count;
    return 1;
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
            if(!task_reserve_next(r,line))return NULL;
            task=GSC_RUNTIME_CALLOC(1,sizeof(*task));
            if(!task) {rfail(r,line,"task allocation failed");return NULL;}
            if(r->task_count<GSC_LIFETIME_TASK_CAPACITY)r->lifetime_tasks[r->task_count]=task;
            else r->extra_tasks[r->task_count-GSC_LIFETIME_TASK_CAPACITY]=task;
            ++r->task_count;
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
