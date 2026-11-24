/* Map queries use the same copied typed transport and lifetime fences. */
int gsc_session_select_map_start_pose(GscSession*s,const GscStartOriginHost*input,char*error,size_t cap){
 GscStartOriginHost copy;
 if(!s||!input||(cap&&!error)||session_animation_namespace_private(s,input,sizeof(*input))||session_animation_namespace_private(s,error,cap)||session_animation_namespace_overlap(input,sizeof(*input),error,cap))return 0;
 copy=*input;if(!session_enter(s,error,cap))return 0;
 if(!copy.query||s->view.state!=GSC_SESSION_EMPTY||s->program||s->runtime||!s->animation_namespace_enabled||!s->map_animation_names.count||s->namespace_start_origin_enabled||s->start_origin_enabled||!s->root_map_enabled)return session_leave_rejected(s,"map pose requires idle map namespace and real provider",error,cap);
 if(!session_animation_namespace_map_current(s)||s->view.state!=GSC_SESSION_EMPTY||s->program||s->runtime||s->namespace_start_origin_enabled||memcmp(input,&copy,sizeof(copy)))return session_leave_rejected(s,"map pose selection retired",error,cap);
 s->namespace_start_origin_host=copy;s->namespace_start_origin_enabled=1;s->map_start_pose_enabled=1;s->busy=0;return 1;
}
/* R212 optional query after immutable R211 namespace configuration. */
int gsc_session_select_r212_namespace_start_origin(GscSession *s,
    const GscStartOriginHost *input,char *error,size_t cap) {
    static const char *const names[2]={"training_guy_practicejump",
        "training_gear_practicejump"};
    GscStartOriginHost copy;uint32_t ordinals[2]={0,0};
    GscAnimationCatalogEntry entry;size_t n;int i;
    if((cap&&!error)||session_animation_namespace_private(s,error,cap)||
       session_animation_namespace_private(s,input,sizeof(*input))||
       session_animation_namespace_overlap(error,cap,input,sizeof(*input)))return 0;
    if(!s||!input)return session_reject(error,cap,"namespace query requires Session and host");
    memcpy(&copy,input,sizeof(copy));
    if(!session_enter(s,error,cap))return 0;
    if(!s->host.enabled||!s->dynamic_randomfloat_enabled||
       !s->animation_namespace_enabled||(s->turret_animations_enabled&&!s->training_rigs)||s->start_origin_enabled||
       s->namespace_start_origin_enabled||s->view.state!=GSC_SESSION_EMPTY||
       s->program||s->runtime||!copy.query)
        return session_leave_rejected(s,"namespace query requires one immutable host after idle EMPTY namespace selection",error,cap);
    for(i=0;i<2;++i) {
        for(n=0;n<gsc_animation_catalog_count(s->animation_namespace_catalogs[i&&s->training_rigs?2:i]);++n) {
            if(!gsc_animation_catalog_entry(s->animation_namespace_catalogs[i&&s->training_rigs?2:i],n,&entry))
                return session_leave_rejected(s,"namespace query declaration invalid",error,cap);
            if(!strcmp(entry.name,names[i])) {
                if(ordinals[i]||!entry.ordinal||entry.flags)
                    return session_leave_rejected(s,"namespace query requires canonical Guy and gear leaves",error,cap);
                ordinals[i]=entry.ordinal;
            }
        }
        if(!ordinals[i])return session_leave_rejected(s,"namespace query declaration missing",error,cap);
    }
    if(!session_animation_namespace_map_current(s)||
       s->view.state!=GSC_SESSION_EMPTY||s->program||s->runtime||
       !s->animation_namespace_enabled||s->start_origin_enabled||
       s->namespace_start_origin_enabled||memcmp(input,&copy,sizeof(copy)))
        return session_leave_rejected(s,"namespace query selection expired",error,cap);
    s->namespace_start_origin_host=copy;
    memcpy(s->namespace_start_origin_ordinals,ordinals,sizeof(ordinals));
    s->namespace_start_origin_enabled=1;s->busy=0;return 1;
}
typedef struct {
    GscSession *session;GscRuntime *runtime;uint64_t id,instance;
    int now,active;int (*current)(void *);void *context;
    GscStartOriginHost provider;uint32_t ordinals[2];
    GscMapExecutionCertificate map;
} SessionNamespaceStartOriginLease;
static int session_namespace_start_origin_scope(const SessionNamespaceStartOriginLease *lease) {
    const GscSession *s=lease?lease->session:NULL;
    return lease&&lease->active&&s&&s->busy&&!s->abort_requested&&!s->view.error[0]&&
        s->animation_namespace_enabled&&s->namespace_start_origin_enabled&&!s->start_origin_enabled&&
        s->view.state==GSC_SESSION_RUNNING&&s->runtime==lease->runtime&&
        s->view.session_id==lease->id&&lease->id&&s->view.logical_ms==lease->now&&
        gsc_runtime_instance_identity(s->runtime)==lease->instance&&
        s->namespace_start_origin_host.user==lease->provider.user&&
        s->namespace_start_origin_host.query==lease->provider.query&&
        !memcmp(s->namespace_start_origin_ordinals,lease->ordinals,sizeof(lease->ordinals))&&
        s->animation_namespace_map.receipt==lease->map.receipt&&
        s->animation_namespace_map.current==lease->map.current;
}
static int session_namespace_start_origin_current(void *context) {
    SessionNamespaceStartOriginLease *lease=context;int ok;
    if(!session_namespace_start_origin_scope(lease)||!lease->current)return 0;
    ok=lease->current(lease->context);
    return ok==1&&session_namespace_start_origin_scope(lease);
}
static int session_namespace_start_origin_query(void *user,const GscStartOriginRequest *request,
    GscStartOriginReply *out,char *error,size_t cap,int (*current)(void *),void *context) {
    static const char *const trees[2]={"generic_human","animation_rig_gear_trainingjump"};
    static const char *const names[2]={"training_guy_practicejump","training_gear_practicejump"};
    const uint32_t poison[3]={0x7fc00000u,0x7fc00000u,0x7fc00000u};
    GscSession *s=user;SessionNamespaceStartOriginLease lease;
    GscStartOriginRequest copy,expected;GscStartOriginReply reply;int i,match=-1,ok;
    if(!s||!request||!out||!current||!s->namespace_start_origin_enabled||
       !s->namespace_start_origin_host.query)
        return session_reject(error,cap,"namespace query outside selected Session");
    /* Runtime current may retire the result arena. Copy first. */
    memcpy(&copy,request,sizeof(copy));memcpy(&expected,&copy,sizeof(expected));
    memset(&lease,0,sizeof(lease));lease.session=s;lease.runtime=s->runtime;
    lease.id=s->view.session_id;lease.instance=gsc_runtime_instance_identity(s->runtime);
    lease.now=s->view.logical_ms;lease.active=1;lease.current=current;lease.context=context;
    lease.provider=s->namespace_start_origin_host;lease.map=s->animation_namespace_map;
    memcpy(lease.ordinals,s->namespace_start_origin_ordinals,sizeof(lease.ordinals));
    if(!memchr(copy.tree,0,sizeof(copy.tree))||!memchr(copy.name,0,sizeof(copy.name)))
        return session_reject(error,cap,"namespace query name is not bounded");
    if(s->map_start_pose_enabled){GscAnimationCatalogEntry entry;
        if((copy.query_kind==1||copy.query_kind==2)&&copy.ordinal)for(i=0;i<(int)s->map_animation_names.count;i++)if(!strcmp(copy.tree,s->map_animation_names.names[i])&&gsc_animation_catalog_entry(s->animation_namespace_catalogs[i],copy.ordinal-1,&entry)&&!entry.flags&&entry.ordinal==copy.ordinal&&!strcmp(entry.name,copy.name))match=i;
    }else{if(copy.query_kind)return session_reject(error,cap,"legacy pose kind refused");for(i=0;i<2;++i)if(!strcmp(copy.tree,trees[i])&&!strcmp(copy.name,names[i]))match=i;}
    if(match<0||copy.flags||(!s->map_start_pose_enabled&&copy.ordinal!=lease.ordinals[match])||
       copy.runtime_identity!=lease.instance||!copy.catalog_identity||copy.now_ms!=lease.now||
       !session_start_origin_float3(copy.origin,0)||!session_start_origin_float3(copy.angles,1)||
       !session_namespace_start_origin_current(&lease))
        return session_reject(error,cap,"namespace query typed request or lease expired");
    memcpy(reply.vector,poison,sizeof(poison));
    ok=lease.provider.query(lease.provider.user,&copy,&reply,error,cap,session_namespace_start_origin_current,&lease);
    if(ok!=1||!session_namespace_start_origin_current(&lease)){lease.active=0;return 0;}
    lease.active=0;
    if(memcmp(&copy,&expected,sizeof(copy))||!session_start_origin_float3(reply.vector,0))
        return session_reject(error,cap,"namespace query provider mutated request or returned nonfinite vector");
    memcpy(out,&reply,sizeof(reply));return 1;
}
