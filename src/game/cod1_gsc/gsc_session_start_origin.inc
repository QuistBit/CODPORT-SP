/* R209 immutable resource selection and synchronous typed query facade.
 * The certificate is the actual GAME table owner, never a Session pointer
 * encoded in a receipt. Only this newly selected lane adds prepare callbacks. */
static int session_start_origin_overlap(const void *a,size_t na,const void *b,size_t nb) {
    uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
    return a&&b&&na&&nb&&(x<=y?y-x<na:x-y<nb);
}
static int session_start_origin_private(GscSession *s,const void *p,size_t n) {
    return s&&session_start_origin_overlap(p,n,s,sizeof(*s));
}
static int session_start_origin_certificate(GscSession *s,const GscMapExecutionCertificate *input) {
    GscMapExecutionCertificate copy;GscRuntime *runtime;uint64_t id;
    int state,now,ok;
    if(!s||!s->busy||s->abort_requested||s->view.error[0]||!input)return 0;
    if(s->start_origin_map_busy) {
        s->abort_requested=1;session_terminal(s,"start-origin certificate reentered");return 0;
    }
    copy=*input;
    if(!copy.receipt||!copy.current)return 0;
    runtime=s->runtime;id=s->view.session_id;state=s->view.state;now=s->view.logical_ms;
    s->start_origin_map_busy=1;ok=copy.current(copy.receipt);s->start_origin_map_busy=0;
    return ok==1&&s->busy&&!s->abort_requested&&!s->view.error[0]&&
        s->runtime==runtime&&s->view.session_id==id&&s->view.state==state&&
        s->view.logical_ms==now&&input->receipt==copy.receipt&&input->current==copy.current;
}
static int session_start_origin_map_current(GscSession *s) {
    return s&&(!s->start_origin_enabled||
        (s->start_origin_catalog&&session_start_origin_certificate(s,&s->start_origin_map)));
}
static int session_start_origin_float3(const float values[3],int angles) {
    uint32_t bits[3];int n;memcpy(bits,values,sizeof(bits));
    for(n=0;n<3;n++)if((bits[n]&0x7f800000u)==0x7f800000u||
        (angles&&(bits[n]&0x7fffffffu)>0x45610000u))return 0;
    return 1;
}
int gsc_session_select_r209_start_origin(GscSession *s,
    const GscSessionStartOriginSelection *input,char *error,size_t cap) {
    GscSessionStartOriginSelection copy;GscAnimationCatalog *catalog=NULL;
    GscAnimationCatalogEntry entry;char why[GSC_SESSION_ERROR_CAP];void *bytes=NULL;
    size_t n;uint32_t ordinal=0;int ok=0;
    /* Refuse bounded diagnostic aliases before session_enter clears error. */
    if((cap&&!error)||session_start_origin_private(s,error,cap)||
       session_start_origin_private(s,input,sizeof(*input))||
       session_start_origin_overlap(error,cap,input,sizeof(*input)))return 0;
    if(!s||!input)return session_reject(error,cap,"start-origin selection requires Session and resource input");
    memcpy(&copy,input,sizeof(copy));
    if(session_start_origin_private(s,copy.generic_human_atr,copy.generic_human_atr_size)||
       session_start_origin_overlap(error,cap,copy.generic_human_atr,copy.generic_human_atr_size)||
       session_start_origin_overlap(input,sizeof(*input),copy.generic_human_atr,copy.generic_human_atr_size))return 0;
    if(!session_enter(s,error,cap))return 0;
    if(!s->host.enabled||!s->dynamic_randomfloat_enabled||s->view.state!=GSC_SESSION_EMPTY||
       s->program||s->runtime||s->start_origin_enabled||s->animation_namespace_enabled) {
        return session_leave_rejected(s,"start-origin requires idle EMPTY mode40 and one immutable selection",error,cap);
    }
    if(!copy.generic_human_atr||!copy.generic_human_atr_size||
       copy.generic_human_atr_size>GSC_SESSION_MAX_SOURCE||
       !copy.map_certificate.receipt||!copy.map_certificate.current||!copy.host.query) {
        return session_leave_rejected(s,"start-origin requires bounded generic_human bytes, real certificate and typed host",error,cap);
    }
    why[0]=0;bytes=GSC_SESSION_MALLOC(copy.generic_human_atr_size);
    if(!bytes)goto done;
    memcpy(bytes,copy.generic_human_atr,copy.generic_human_atr_size);
    catalog=gsc_animation_catalog_parse("generic_human",bytes,copy.generic_human_atr_size,why,sizeof(why));
    GSC_SESSION_FREE(bytes);bytes=NULL;
    if(!catalog)goto done;
    for(n=0;n<gsc_animation_catalog_count(catalog);n++) {
        if(!gsc_animation_catalog_entry(catalog,n,&entry))goto done;
        if(!strcmp(entry.name,"training_guy_practicejump")) {
            if(!entry.ordinal||entry.flags)goto done;
            ordinal=entry.ordinal;break;
        }
    }
    if(!ordinal||!session_start_origin_certificate(s,&copy.map_certificate)||
       s->view.state!=GSC_SESSION_EMPTY||s->program||s->runtime||s->start_origin_enabled||
       memcmp(input,&copy,sizeof(copy)))goto done;
    s->start_origin_catalog=catalog;catalog=NULL;
    s->start_origin_map=copy.map_certificate;s->start_origin_host=copy.host;
    s->start_origin_ordinal=ordinal;s->start_origin_enabled=1;ok=1;
 done:
    GSC_SESSION_FREE(bytes);gsc_animation_catalog_free(catalog);s->busy=0;
    if(!ok)return session_reject(error,cap,why[0]?why:"start-origin resource selection refused");
    return 1;
}
typedef struct {
    GscSession *session;GscRuntime *runtime;uint64_t id,instance;
    int now,active;int (*current)(void *);void *context;GscStartOriginHost provider;
} SessionStartOriginLease;
static int session_start_origin_scope(const SessionStartOriginLease *lease) {
    const GscSession *s=lease?lease->session:NULL;
    return lease&&lease->active&&s&&s->busy&&!s->abort_requested&&!s->view.error[0]&&
        s->start_origin_enabled&&s->view.state==GSC_SESSION_RUNNING&&
        s->runtime==lease->runtime&&s->view.session_id==lease->id&&lease->id&&
        s->view.logical_ms==lease->now&&gsc_runtime_instance_identity(s->runtime)==lease->instance&&
        s->start_origin_host.user==lease->provider.user&&s->start_origin_host.query==lease->provider.query;
}
static int session_start_origin_current(void *context) {
    SessionStartOriginLease *lease=context;int ok;
    if(!session_start_origin_scope(lease)||!lease->current)return 0;
    ok=lease->current(lease->context);
    return ok==1&&session_start_origin_scope(lease);
}
static int session_start_origin_query(void *user,const GscStartOriginRequest *request,
    GscStartOriginReply *out,char *error,size_t cap,int (*current)(void *),void *context) {
    GscSession *s=user;SessionStartOriginLease lease;
    GscStartOriginRequest copy,expected;GscStartOriginReply reply;
    const uint32_t poison[3]={0x7fc00000u,0x7fc00000u,0x7fc00000u};int ok;
    if(!s||!request||!out||!current||!s->start_origin_enabled||!s->start_origin_host.query)
        return session_reject(error,cap,"start-origin outside selected Session");
    /* Copy the complete request before any Runtime/map current callback; a
     * retired task may clear its expression-result arena during that call. */
    memcpy(&copy,request,sizeof(copy));memcpy(&expected,&copy,sizeof(expected));
    memset(&lease,0,sizeof(lease));lease.session=s;lease.runtime=s->runtime;
    lease.id=s->view.session_id;lease.instance=gsc_runtime_instance_identity(s->runtime);
    lease.now=s->view.logical_ms;lease.active=1;lease.current=current;lease.context=context;
    lease.provider=s->start_origin_host;
    if(!memchr(copy.tree,0,sizeof(copy.tree))||!memchr(copy.name,0,sizeof(copy.name))||
       strcmp(copy.tree,"generic_human")||strcmp(copy.name,"training_guy_practicejump")||
       copy.flags||copy.ordinal!=s->start_origin_ordinal||copy.runtime_identity!=lease.instance||
       !copy.catalog_identity||copy.now_ms!=lease.now||
       !session_start_origin_float3(copy.origin,0)||!session_start_origin_float3(copy.angles,1)||
       !session_start_origin_current(&lease))return session_reject(error,cap,"start-origin Session lease or typed request expired");
    memcpy(reply.vector,poison,sizeof(poison));
    ok=lease.provider.query(lease.provider.user,&copy,&reply,error,cap,session_start_origin_current,&lease);
    if(ok!=1||!session_start_origin_current(&lease)) {lease.active=0;return 0;}
    lease.active=0;
    if(memcmp(&copy,&expected,sizeof(copy))||!session_start_origin_float3(reply.vector,0))
        return session_reject(error,cap,"start-origin host mutated request or returned nonfinite vector");
    memcpy(out,&reply,sizeof(reply));return 1;
}
