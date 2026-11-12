/* R202 Session-owned staging and retirement fences for typed AI snapshots. */
int gsc_session_select_r202_ai_arrays(GscSession *s,const GscAiArrayHost *host,char *error,size_t cap) {
    if(!s||!host||!host->query||!host->validate||s->busy||s->program||s->runtime||
       !s->execution_extensions||s->ai_arrays_enabled)
        return session_reject(error,cap,"AI arrays require idle selected R198 execution and typed AI host");
    s->ai_arrays=*host;s->ai_arrays_enabled=1;return 1;
}
static int session_ai_array_query(void *user,const GscAiArrayRequest *request,
    GscAiArraySnapshot *out,char *error,size_t cap,int (*current)(void *),void *context) {
    GscSession *s=user;GscAiArrayRequest copy,expected;GscAiArraySnapshot result;SessionPlayerLease lease;int ok;
    if(!s||!request||!out||!s->ai_arrays_enabled||!s->ai_arrays.query)
        return session_reject(error,cap,"AI query outside selected Session");
    lease.session=s;lease.id=s->view.session_id;lease.current=current;lease.context=context;
    if(!session_player_current(&lease))return session_reject(error,cap,"AI query Session lease expired");
    copy=*request;expected=copy;memset(&result,0,sizeof(result));result.count=UINT32_MAX;
    ok=s->ai_arrays.query(s->ai_arrays.user,&copy,&result,error,cap,session_player_current,&lease);
    if(ok!=1||!session_player_current(&lease))return 0;
    if(memcmp(&copy,&expected,sizeof(copy))||result.count>GSC_AI_ARRAY_CAPACITY)
        return session_reject(error,cap,"AI query mutated its borrowed request or exceeded capacity");
    *out=result;return 1;
}
static int session_ai_array_validate(void *user,const GscAiArrayRequest *request,
    const GscAiArraySnapshot *snapshot,char *error,size_t cap,int (*current)(void *),void *context) {
    GscSession *s=user;GscAiArrayRequest copy,expected;GscAiArraySnapshot borrowed,original;SessionPlayerLease lease;int ok;
    if(!s||!request||!snapshot||!s->ai_arrays_enabled||!s->ai_arrays.validate)
        return session_reject(error,cap,"AI validation outside selected Session");
    lease.session=s;lease.id=s->view.session_id;lease.current=current;lease.context=context;
    if(!session_player_current(&lease))return session_reject(error,cap,"AI validation Session lease expired");
    copy=*request;expected=copy;borrowed=*snapshot;original=borrowed;
    ok=s->ai_arrays.validate(s->ai_arrays.user,&copy,&borrowed,error,cap,session_player_current,&lease);
    if(ok!=1||!session_player_current(&lease))return 0;
    if(memcmp(&copy,&expected,sizeof(copy))||memcmp(&borrowed,&original,sizeof(borrowed)))
        return session_reject(error,cap,"AI validator mutated its borrowed request or snapshot");
    return 1;
}
