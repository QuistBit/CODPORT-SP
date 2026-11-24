/* R199 copied node services: a backend may retire the Session while a callback
 * runs, but no staged identity/value is published after that lease expires. */
int gsc_session_select_vehicle_nodes(GscSession *s,const GscVehicleNodeHost *host,char *error,size_t cap) {
    if(!s||!host||!host->query||!host->validate||!host->read||s->busy||s->program||s->runtime||
       !s->execution_extensions||s->vehicle_nodes_enabled)
        return session_reject(error,cap,"node queries require idle selected R198 execution and typed node host");
    s->vehicle_nodes=*host;s->vehicle_nodes_enabled=1;return 1;
}
static int session_vehicle_node_query(void *user,const GscVehicleNodeQuery *request,
    GscVehicleNodeRef *out,size_t capacity,size_t *count,int *defined,char *error,size_t cap,
    int (*current)(void *),void *context) {
    GscSession *s=user;GscVehicleNodeQuery copy,expected;GscVehicleNodeRef *staged;
    SessionPlayerLease lease;size_t used=SIZE_MAX;int ok,isDefined=-1;
    if(!s||!request||!out||!count||!defined||!capacity||capacity>GSC_VEHICLE_NODE_CAPACITY||
       !s->vehicle_nodes_enabled||!s->vehicle_nodes.query)
        return session_reject(error,cap,"node query outside selected session or capacity");
    lease.session=s;lease.id=s->view.session_id;lease.current=current;lease.context=context;
    if(!session_player_current(&lease))return session_reject(error,cap,"node query session lease expired");
    staged=GSC_SESSION_CALLOC(capacity,sizeof(*staged));
    if(!staged)return session_reject(error,cap,"node query staging allocation failed");
    copy=*request;memcpy(&expected,&copy,sizeof(copy));
    ok=s->vehicle_nodes.query(s->vehicle_nodes.user,&copy,staged,capacity,&used,&isDefined,error,cap,session_player_current,&lease);
    if(ok!=1||!session_player_current(&lease)){GSC_SESSION_FREE(staged);return 0;}
    if(memcmp(&copy,&expected,sizeof(copy))||used>capacity||(isDefined!=0&&isDefined!=1)||(!isDefined&&used)){
        GSC_SESSION_FREE(staged);return session_reject(error,cap,"node query host mutated its borrowed request or exceeded capacity");
    }
    memcpy(out,staged,used*sizeof(*out));*count=used;*defined=isDefined;GSC_SESSION_FREE(staged);return 1;
}
static int session_vehicle_node_validate(void *user,const GscVehicleNodeRef *node,int now,int *alive,
    char *error,size_t cap,int (*current)(void *),void *context) {
    GscSession *s=user;GscVehicleNodeRef copy,expected;SessionPlayerLease lease;int result=-1,ok;
    if(!s||!node||!alive||!s->vehicle_nodes_enabled||!s->vehicle_nodes.validate)
        return session_reject(error,cap,"node validation outside selected session");
    lease.session=s;lease.id=s->view.session_id;lease.current=current;lease.context=context;
    if(!session_player_current(&lease))return session_reject(error,cap,"node validation session lease expired");
    copy=*node;expected=copy;
    ok=s->vehicle_nodes.validate(s->vehicle_nodes.user,&copy,now,&result,error,cap,session_player_current,&lease);
    if(ok!=1||!session_player_current(&lease))return 0;
    if(copy.epoch!=expected.epoch||copy.index!=expected.index||copy.generation!=expected.generation||
       (result!=0&&result!=1))return session_reject(error,cap,"node validator mutated identity or returned invalid boolean");
    *alive=result;return 1;
}
static int session_vehicle_node_read(void *user,const GscVehicleNodeRef *node,const char *field,int now,
    GscVehicleNodeField *out,char *error,size_t cap,int (*current)(void *),void *context) {
    GscSession *s=user;GscVehicleNodeRef copy,expected;GscVehicleNodeField result;
    SessionPlayerLease lease;char name[128],expected_name[128];size_t n;int ok;
    if(!s||!node||!field||!out||!s->vehicle_nodes_enabled||!s->vehicle_nodes.read)
        return session_reject(error,cap,"node field read outside selected session");
    memset(name,0,sizeof(name));
    for(n=0;n<sizeof(name)&&field[n];++n)name[n]=field[n];
    if(!n||n==sizeof(name))return session_reject(error,cap,"node field name is empty or too long");
    memcpy(expected_name,name,sizeof(name));
    lease.session=s;lease.id=s->view.session_id;lease.current=current;lease.context=context;
    if(!session_player_current(&lease))return session_reject(error,cap,"node field session lease expired");
    copy=*node;expected=copy;memset(&result,0,sizeof(result));result.kind=-1;
    ok=s->vehicle_nodes.read(s->vehicle_nodes.user,&copy,name,now,&result,error,cap,session_player_current,&lease);
    if(ok!=1||!session_player_current(&lease))return 0;
    if(copy.epoch!=expected.epoch||copy.index!=expected.index||copy.generation!=expected.generation||
       memcmp(name,expected_name,sizeof(name)))
        return session_reject(error,cap,"node field host mutated its borrowed identity or name");
    *out=result;return 1;
}
