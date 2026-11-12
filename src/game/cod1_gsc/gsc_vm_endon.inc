/* R198 opt-in task cancellation subscriptions. Their storage is independent of
 * the task's current sleep or payload wait. A notification first retires every
 * matching task, then delivers values only to the remaining ordinary waiters.
 * Native VM object/thread handles and unbounded original capacities are not
 * represented here. Complete host identities prevent slot reuse retargeting. */
int gsc_program_select_endon(GscProgram *g) {
    if(!g||g->invalid||g->linked||g->endon_selected||!g->names_syntax||
       !g->scalar_switch_syntax||!g->lifetime_syntax)return 0;
    g->endon_selected=1;return 1;
}
int gsc_runtime_select_endon(GscRuntime *r) {
    if(!r||!r->program->endon_selected||r->endon_enabled||r->started||
       r->advancing||r->executing_task||r->task_count||r->error[0]||
       !r->events_enabled||!r->state_enabled||!r->lifetime_enabled||
       !r->results_enabled||r->entity_prepare_busy||r->entity_certificate_busy)return 0;
    r->endon_enabled=1;return 1;
}
/* Object numbers are unique and never reused within a runtime. Validation of
 * the runtime token precedes registration or delivery in run_payload_event. */
static int event_scope_for(const GscEntityRef *owner,uint32_t object) {
    return owner ? EVENT_ENTITY : (object ? EVENT_STRUCT : EVENT_LEVEL);
}
static int endon_register(GscRuntime *r,Task *task,const GscEntityRef *owner,
                          const char *event,int line,uint32_t object) {
    int n;EndonSubscription *s;
    if(!r->endon_enabled||!task||task!=r->executing_task||task->status!=TASK_READY||
       !event||!event[0]||strlen(event)>=128)return rfail(r,line,"endon capability unavailable or invalid subscription");
    for(n=0;n<task->endon_count;n++) {
        s=&task->endon[n];
        if(s->scope==event_scope_for(owner,object)&&s->struct_id==object&&!strcmp(s->event,event)&&
           (!owner||entity_ref_equal(&s->owner,owner)))return 1;
    }
    if(task->endon_count>=GSC_ENDON_CAPACITY)return rfail(r,line,"endon subscription capacity exceeded");
    s=&task->endon[task->endon_count++];memset(s,0,sizeof(*s));
    s->scope=event_scope_for(owner,object);s->struct_id=object;if(owner)s->owner=*owner;
    str_copy(s->event,sizeof(s->event),event);return 1;
}
static int endon_matches_owner(const GscRuntime *r,const Task *task,
                         const GscEntityRef *owner,const char *event,uint32_t object) {
    int k;if(!r->endon_enabled||task->status==TASK_DEAD)return 0;
    for(k=0;k<task->endon_count;k++) {
        const EndonSubscription *s=&task->endon[k];
        if(s->scope==event_scope_for(owner,object)&&s->struct_id==object&&!strcmp(s->event,event)&&
           (!owner||entity_ref_equal(&s->owner,owner)))return 1;
    }
    return 0;
}
static int endon_matches(const GscRuntime *r,const Task *task,const GscEntityRef *owner,const char *event) {
    return endon_matches_owner(r,task,owner,event,0);
}
static void endon_cancel_owner(GscRuntime *r,const GscEntityRef *owner,const char *event,uint32_t object) {
    int n;if(!r->endon_enabled)return;
    for(n=0;n<r->task_count;n++) {
        Task *task=task_at(r,n);
        if(endon_matches_owner(r,task,owner,event,object)) {
            task->status=TASK_DEAD;task->depth=0;task->context_retired=1;
            clear_wait_owner(task);task->blast_wait_serial=0;
            memset(task->endon,0,sizeof(task->endon));task->endon_count=0;
            /* The notifying task or a suspended outer entity-entry caller may
             * still have pointers into ResultsTask. Safe scheduler/entry
             * teardown and slot reuse release it after unwinding. */
        }
    }
}
static void endon_cancel(GscRuntime *r,const GscEntityRef *owner,const char *event) {
    endon_cancel_owner(r,owner,event,0);
}
static void endon_collect_retired(GscRuntime *r) {
    int n;
    if(!r->endon_enabled||r->executing_task||r->entity_entry_depth)return;
    for(n=0;n<r->task_count;n++) {
        Task *task=task_at(r,n);
        if(task->status==TASK_DEAD&&task->context_retired&&!task->host_entry_pins)
            task_results_clear(task);
    }
}
