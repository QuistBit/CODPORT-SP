/* R199 copied node services: a backend may retire the Session while a callback
 * runs, but no staged identity/value is published after that lease expires. */
int gsc_session_select_r199_node_queries(GscSession *s,const GscNodeQueryHost *host,char *error,size_t cap) {
    if(!s||!host||!host->query||!host->validate||!host->read||s->busy||s->program||s->runtime||
       !s->execution_extensions||s->node_queries_enabled)
        return session_reject(error,cap,"node queries require idle selected R198 execution and typed node host");
    s->node_queries=*host;s->node_queries_enabled=1;return 1;
}
static int session_node_query(void *user,const GscNodeQueryRequest *request,
    GscNodeRef *out,size_t capacity,size_t *count,char *error,size_t cap,
    int (*current)(void *),void *context) {
    GscSession *s=user;GscNodeQueryRequest copy,expected;GscNodeRef *staged;
    SessionPlayerLease lease;size_t used=SIZE_MAX;int ok;
    if(!s||!request||!out||!count||!capacity||capacity>GSC_NODE_QUERY_CAPACITY||
       !s->node_queries_enabled||!s->node_queries.query)
        return session_reject(error,cap,"node query outside selected session or capacity");
    lease.session=s;lease.id=s->view.session_id;lease.current=current;lease.context=context;
    if(!session_player_current(&lease))return session_reject(error,cap,"node query session lease expired");
    staged=GSC_SESSION_CALLOC(capacity,sizeof(*staged));
    if(!staged)return session_reject(error,cap,"node query staging allocation failed");
    copy=*request;memcpy(&expected,&copy,sizeof(copy));
    ok=s->node_queries.query(s->node_queries.user,&copy,staged,capacity,&used,error,cap,session_player_current,&lease);
    if(ok!=1||!session_player_current(&lease)){GSC_SESSION_FREE(staged);return 0;}
    if(memcmp(&copy,&expected,sizeof(copy))||used>capacity){
        GSC_SESSION_FREE(staged);return session_reject(error,cap,"node query host mutated its borrowed request or exceeded capacity");
    }
    memcpy(out,staged,used*sizeof(*out));*count=used;GSC_SESSION_FREE(staged);return 1;
}
static int session_node_validate(void *user,const GscNodeRef *node,int now,int *alive,
    char *error,size_t cap,int (*current)(void *),void *context) {
    GscSession *s=user;GscNodeRef copy,expected;SessionPlayerLease lease;int result=-1,ok;
    if(!s||!node||!alive||!s->node_queries_enabled||!s->node_queries.validate)
        return session_reject(error,cap,"node validation outside selected session");
    lease.session=s;lease.id=s->view.session_id;lease.current=current;lease.context=context;
    if(!session_player_current(&lease))return session_reject(error,cap,"node validation session lease expired");
    copy=*node;expected=copy;
    ok=s->node_queries.validate(s->node_queries.user,&copy,now,&result,error,cap,session_player_current,&lease);
    if(ok!=1||!session_player_current(&lease))return 0;
    if(copy.epoch!=expected.epoch||copy.index!=expected.index||copy.generation!=expected.generation||
       (result!=0&&result!=1))return session_reject(error,cap,"node validator mutated identity or returned invalid boolean");
    *alive=result;return 1;
}
static int session_node_read(void *user,const GscNodeRef *node,const char *field,int now,
    GscNodeFieldValue *out,char *error,size_t cap,int (*current)(void *),void *context) {
    GscSession *s=user;GscNodeRef copy,expected;GscNodeFieldValue result;
    SessionPlayerLease lease;char name[128],expected_name[128];size_t n;int ok;
    if(!s||!node||!field||!out||!s->node_queries_enabled||!s->node_queries.read)
        return session_reject(error,cap,"node field read outside selected session");
    memset(name,0,sizeof(name));
    for(n=0;n<sizeof(name)&&field[n];++n)name[n]=field[n];
    if(!n||n==sizeof(name))return session_reject(error,cap,"node field name is empty or too long");
    memcpy(expected_name,name,sizeof(name));
    lease.session=s;lease.id=s->view.session_id;lease.current=current;lease.context=context;
    if(!session_player_current(&lease))return session_reject(error,cap,"node field session lease expired");
    copy=*node;expected=copy;memset(&result,0,sizeof(result));result.kind=-1;
    ok=s->node_queries.read(s->node_queries.user,&copy,name,now,&result,error,cap,session_player_current,&lease);
    if(ok!=1||!session_player_current(&lease))return 0;
    if(copy.epoch!=expected.epoch||copy.index!=expected.index||copy.generation!=expected.generation||
       memcmp(name,expected_name,sizeof(name)))
        return session_reject(error,cap,"node field host mutated its borrowed identity or name");
    *out=result;return 1;
}
