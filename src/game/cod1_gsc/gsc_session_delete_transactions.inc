/* R204 Session keeps the borrowed transaction facade internal to the active
 * native call. Public Session start/advance/notify/unload still reject reentry. */
static int session_delete_overlap(const void *a,size_t na,const void *b,size_t nb) {
    uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;return a&&b&&na&&nb&&(x<=y?y-x<na:x-y<nb);
}
int gsc_session_select_r204_delete_transactions(GscSession *s,const GscDeleteTransactionHost *input,char *error,size_t cap) {
    if(!s||!input||session_delete_overlap(input,sizeof(*input),s,sizeof(*s))||
       session_delete_overlap(error,cap,s,sizeof(*s))||session_delete_overlap(input,sizeof(*input),error,cap))return 0;
    if(!input->invoke||s->busy||s->program||s->runtime||!s->execution_extensions||
       !s->entity_operations_enabled||s->delete_transactions_enabled)
        return session_reject(error,cap,"delete transaction requires idle selected R198 entity operations");
    s->delete_transactions=*input;s->delete_transactions_enabled=1;return 1;
}
typedef struct {
    GscSession *session;GscRuntime *runtime;uint64_t id;int now_ms,active;
    GscDeleteTransactionServices backend;
    const GscDeleteTransactionRequest *request;
    const GscDeleteTransactionServices *facade;
} SessionDeleteTransactionLease;
static int session_delete_current(void *context) {
    SessionDeleteTransactionLease *l=context;GscSession *s=l?l->session:NULL;
    return l&&l->active&&s&&s->busy&&!s->abort_requested&&s->runtime==l->runtime&&
        s->view.session_id==l->id&&s->view.state==GSC_SESSION_RUNNING&&s->view.logical_ms==l->now_ms&&
        l->backend.current&&l->backend.current(l->backend.context);
}
static int session_delete_death(void *context,char *error,size_t cap) {
    SessionDeleteTransactionLease *l=context;
    if(!l||!gsc_runtime_storage_separate(l->runtime,error,cap)||
       session_delete_overlap(error,cap,l,sizeof(*l))||
       session_delete_overlap(error,cap,l->session,sizeof(*l->session))||
       session_delete_overlap(error,cap,l->request,sizeof(*l->request))||
       session_delete_overlap(error,cap,l->facade,sizeof(*l->facade)))return 0;
    if(!session_delete_current(l))return session_reject(error,cap,"delete death outside current Session transaction");
    if(!l->backend.notify_death(l->backend.context,error,cap))return 0;
    return session_delete_current(l);
}
static int session_delete_transaction(void *user,const GscDeleteTransactionRequest *input,
    const GscDeleteTransactionServices *services,char *error,size_t cap) {
    GscSession *s=user;SessionDeleteTransactionLease lease;
    GscDeleteTransactionRequest request,expected;GscDeleteTransactionServices facade,original;int ok;
    if(!s||!input||!services||!services->current||!services->notify_death||!s->delete_transactions_enabled||
       !s->delete_transactions.invoke||!s->runtime)return session_reject(error,cap,"delete outside selected Session transaction");
    memset(&lease,0,sizeof(lease));memset(&facade,0,sizeof(facade));request=*input;expected=request;
    lease.session=s;lease.runtime=s->runtime;lease.id=s->view.session_id;lease.now_ms=request.now_ms;
    lease.active=1;lease.backend=*services;lease.request=&request;lease.facade=&facade;
    facade.context=&lease;facade.current=session_delete_current;facade.notify_death=session_delete_death;original=facade;
    if(!session_delete_current(&lease)){lease.active=0;return session_reject(error,cap,"delete Session transaction lease expired");}
    ok=s->delete_transactions.invoke(s->delete_transactions.user,&request,&facade,error,cap);
    if(memcmp(&request,&expected,sizeof(request))||memcmp(&facade,&original,sizeof(facade))) {
        lease.active=0;return session_reject(error,cap,"delete host mutated borrowed Session request or services");
    }
    if(!session_delete_current(&lease))ok=0;
    lease.active=0;return ok==1;
}
