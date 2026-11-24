/* R204 explicit death-before-free transaction. No scheduler entry, Task-status
 * bypass for unrelated methods, or public Session notification is introduced. */
int gsc_program_select_delete_transactions(GscProgram *g) {
    if(!g||g->invalid||g->linked||g->delete_transactions_selected||
       !g->entity_operations_selected||!g->endon_selected||!g->event_payload_syntax)return 0;
    g->delete_transactions_selected=1;return 1;
}
int gsc_runtime_select_delete_transactions(GscRuntime *r,const GscDeleteTransactionHost *host) {
    if(!r||!host||!host->invoke||r->started||r->advancing||r->executing_task||r->task_count||r->error[0]||
       !r->program->delete_transactions_selected||r->delete_transactions_enabled||
       !r->entity_operations_enabled||!r->endon_enabled||!r->event_payloads_enabled||
       !r->entities_enabled||!r->results_enabled||!r->lifetime_enabled||
       !gsc_runtime_storage_separate(r,host,sizeof(*host)))return 0;
    r->delete_transactions=*host;r->delete_transactions_enabled=1;return 1;
}
typedef struct {
    GscRuntime *runtime;Task *task;uint64_t serial;int now_ms,line,active,notified,current_busy;
    GscEntityRef owner;GscMapExecutionCertificate map;
    const GscDeleteTransactionRequest *request;
    const GscDeleteTransactionServices *services;
} DeleteTransactionLease;
static int delete_transaction_current(void *context) {
    DeleteTransactionLease *l=context;GscRuntime *r=l?l->runtime:NULL;int ok;
    if(!l||!l->active||!r||r->delete_transaction_scope!=l||!r->entity_method_busy||r->error[0]||
       r->executing_task!=l->task||!l->task||l->task->serial!=l->serial||r->now_ms!=l->now_ms)return 0;
    if(l->current_busy||r->entity_prepare_busy||r->entity_certificate_busy)
        return rfail(r,l->line,"delete transaction map callback reentered");
    if(!l->map.receipt)return 1;
    l->current_busy=1;r->entity_certificate_busy=1;ok=l->map.current(l->map.receipt);
    r->entity_certificate_busy=0;l->current_busy=0;
    if(r->error[0])return 0;
    if(ok!=0&&ok!=1)return rfail(r,l->line,"invalid delete transaction map certificate result");
    return ok;
}
/* The ordinary validator consults the caller's execution certificate. That is
 * deliberately unsuitable after death endon canceled the caller: the actual
 * host result must still be proved against the exact receiver allocation. */
static int delete_transaction_owner_live(DeleteTransactionLease *l) {
    GscRuntime *r=l->runtime;GscEntityRef owner=l->owner;int alive;
    if(!delete_transaction_current(l))return -1;
    if(r->entity_certificate_busy||r->entity_prepare_busy){rfail(r,l->line,"delete identity validation reentered");return -1;}
    r->entity_certificate_busy=1;alive=r->entities.validate(r->entities.user,&owner);r->entity_certificate_busy=0;
    if(r->error[0])return -1;
    if(!entity_ref_equal(&owner,&l->owner)){rfail(r,l->line,"delete validator mutated borrowed owner");return -1;}
    if(alive!=0&&alive!=1){rfail(r,l->line,"invalid delete identity validation result");return -1;}
    return delete_transaction_current(l)?alive:-1;
}
static int delete_transaction_external(DeleteTransactionLease *l,const void *p,size_t n) {
    return gsc_runtime_storage_separate(l->runtime,p,n)&&
        !Cod1ModelStringOverlap(p,n,l,sizeof(*l))&&
        !Cod1ModelStringOverlap(p,n,l->request,sizeof(*l->request))&&
        !Cod1ModelStringOverlap(p,n,l->services,sizeof(*l->services));
}
static int delete_transaction_death(void *context,char *error,size_t cap) {
    DeleteTransactionLease *l=context;GscRuntime *r=l?l->runtime:NULL;
    /* Deletion selects lifetime storage. A waiter can occupy any of its
     * stable slots, including slots beyond the legacy embedded task array. */
    uint64_t local_waiters[GSC_LIFETIME_TASK_CAPACITY], *waiters=local_waiters;
    int n,matched,slots;
    if(!l||!r||!delete_transaction_external(l,error,cap))return 0;
    if(!delete_transaction_current(l))return 0;
    if(l->notified)return rfail(r,l->line,"delete death notification already attempted");
    slots=r->task_count;
    if(slots<0||slots>task_directory_limit(r))return rfail(r,l->line,"invalid delete task count");
    l->notified=1;
    if(delete_transaction_owner_live(l)!=1)return rfail(r,l->line,"delete death owner is stale or missing");
    if(slots>GSC_LIFETIME_TASK_CAPACITY){
        if((size_t)slots>SIZE_MAX/sizeof(*waiters))return rfail(r,l->line,"death waiter count overflow");
        waiters=GSC_RUNTIME_CALLOC((size_t)slots,sizeof(*waiters));
        if(!waiters)return rfail(r,l->line,"death waiter allocation failed");
    }else memset(waiters,0,(size_t)slots*sizeof(*waiters));
    for(n=0;n<slots;n++) {
        Task *t=task_at(r,n);
        if(payload_matches(t,&l->owner,"death")&&!endon_matches(r,t,&l->owner,"death")&&
           waittillmatch_matches(t,NULL,0))waiters[n]=t->serial;
    }
    matched=entity_notify_internal(r,&l->owner,"death");
    if(matched<0||r->error[0]){
        if(waiters!=local_waiters)GSC_RUNTIME_FREE(waiters);
        return 0;
    }
    /* Delivery has copied its payload and ended the wait. Consume only these
     * matched registrations, preserving each Task execution certificate. A
     * level observer may resume after free; owner-bound tasks still retire. */
    for(n=0;n<slots;n++) {
        Task *t=task_at(r,n);
        if(waiters[n]&&waiters[n]==t->serial&&t->status==TASK_READY&&
           t->event_scope==EVENT_ENTITY&&entity_ref_equal(&t->event_owner,&l->owner)&&
           !strcmp(t->event,"death"))clear_wait_owner(t);
    }
    if(waiters!=local_waiters)GSC_RUNTIME_FREE(waiters);
    if(!delete_transaction_current(l))return 0;
    if(error&&cap)error[0]=0;return 1;
}
/* The host has notified death and proved free of this exact allocation.
 * A controller MAIN waiting for killanimscript cannot wake on a destroyed
 * owner. Its transferred VM reference must die with the GAME actor. */
static int r243_main_task_owned(const GscRuntime *,const Task *,uint64_t);
static int delete_transaction_release_main(GscRuntime *r,const GscEntityRef *entity,int line){
    unsigned i;
    for(i=0;i<sizeof(r->r243_controller_main)/sizeof(r->r243_controller_main[0]);i++){
        R243ControllerMainOwner *owner=&r->r243_controller_main[i];
        Task *task;
        if(!owner->used||!state_ref_equal(&owner->entity,entity))continue;
        task=owner->task;
        if(!owner->receipt||owner->runtime!=r->array_token||
           !r243_main_task_owned(r,task,owner->task_serial)||task->retained_owner_refs!=1||
           !state_ref_equal(&owner->certificate.owner,entity)||
           memcmp(&owner->certificate,&task->certificate,sizeof(owner->certificate)))
            return rfail(r,line,"freed actor MAIN receipt or task identity changed");
        entity_task_retire(task);
        task->retained_owner_refs--;
        memset(owner,0,sizeof(*owner));
    }
    return 1;
}
static int delete_transaction_invoke(GscRuntime *r,const Value *receiver,int line,Value *out) {
    DeleteTransactionLease lease;GscDeleteTransactionRequest request,expected;
    GscDeleteTransactionServices services,original;char why[384];int ok=0,accepted,alive,finished;
    if(!r->delete_transactions_enabled||!receiver||receiver->kind!=V_ENTITY||!r->executing_task)
        return rfail(r,line,"delete transaction capability or entity receiver unavailable");
    if(r->delete_transaction_scope||r->entity_method_busy||r->entity_field_busy||r->entity_inheritance_busy||
       r->entity_prepare_busy||r->entity_certificate_busy)return rfail(r,line,"delete transaction callback reentered");
    memset(&lease,0,sizeof(lease));memset(&request,0,sizeof(request));memset(&services,0,sizeof(services));
    request.entity=receiver->entity;request.now_ms=r->now_ms;expected=request;
    r->entity_method_busy=1;finished=entity_method_finish(r,&request.entity,line);
    if(finished<0){r->entity_method_busy=0;return finished==-2;}
    lease.runtime=r;lease.task=r->executing_task;lease.serial=lease.task->serial;lease.now_ms=r->now_ms;
    lease.line=line;lease.active=1;lease.owner=request.entity;lease.map=lease.task->map_certificate;
    lease.request=&request;lease.services=&services;
    services.context=&lease;services.current=delete_transaction_current;services.notify_death=delete_transaction_death;original=services;
    r->delete_transaction_scope=&lease;why[0]=0;
    accepted=r->delete_transactions.invoke(r->delete_transactions.user,&request,&services,why,sizeof(why));why[sizeof(why)-1]=0;
    if(r->error[0])goto done;
    if(memcmp(&request,&expected,sizeof(request))||memcmp(&services,&original,sizeof(services))) {
        rfail(r,line,"delete transaction host mutated borrowed request or services");goto done;
    }
    if(!delete_transaction_current(&lease)) {
        if(!r->error[0]){entity_task_retire(lease.task);ok=1;}goto done;
    }
    if(accepted!=1){rfail(r,line,"delete transaction refused: %s",why[0]?why:"host callback failed");goto done;}
    if(!lease.notified){rfail(r,line,"delete transaction omitted death notification");goto done;}
    alive=delete_transaction_owner_live(&lease);
    if(alive<0)goto done;
    if(alive){rfail(r,line,"delete transaction host left the receiver alive");goto done;}
    ok=delete_transaction_release_main(r,&lease.owner,line);
done:
    lease.active=0;r->delete_transaction_scope=NULL;r->entity_method_busy=0;
    if(ok&&r->executing_task&&!refresh_state_context(r,r->executing_task))return 0;
    if(ok&&out)memset(out,0,sizeof(*out));return ok;
}
