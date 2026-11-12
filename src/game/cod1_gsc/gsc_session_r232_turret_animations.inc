/* Immutable two-ATR configuration and real command provider. */
int gsc_session_select_r232_turret_animations(GscSession *s,const GscSessionTurretAnimationSelection *input,char *error,size_t cap) {
    static const char *const names[2]={"generic_human","mg42"};
    GscSessionTurretAnimationSelection copy;GscAnimationCatalog *catalogs[2]={NULL,NULL};
    char why[GSC_SESSION_ERROR_CAP];void *bytes=NULL;int n,ok=0;
    if((cap&&!error)||session_animation_namespace_private(s,error,cap)||
       session_animation_namespace_private(s,input,sizeof(*input))||
       session_animation_namespace_overlap(error,cap,input,sizeof(*input)))return 0;
    if(!s||!input)return session_reject(error,cap,"turret animations require Session and selection");
    copy=*input;
    for(n=0;n<2;n++)if(session_animation_namespace_private(s,copy.catalogs[n].atr_bytes,copy.catalogs[n].atr_size)||
        session_animation_namespace_overlap(error,cap,copy.catalogs[n].atr_bytes,copy.catalogs[n].atr_size)||
        session_animation_namespace_overlap(input,sizeof(*input),copy.catalogs[n].atr_bytes,copy.catalogs[n].atr_size))return 0;
    if(!session_enter(s,error,cap))return 0;
    if(!s->host.enabled||!s->dynamic_randomfloat_enabled||s->view.state!=GSC_SESSION_EMPTY||
       s->program||s->runtime||s->animation_namespace_enabled||s->start_origin_enabled||
       !copy.host.validate||!copy.host.invoke||!copy.map_certificate.receipt||!copy.map_certificate.current)
        return session_leave_rejected(s,"turret animations require exclusive idle EMPTY mode40 and real host/certificate",error,cap);
    for(n=0;n<2;n++)if(!copy.catalogs[n].atr_bytes||!copy.catalogs[n].atr_size||copy.catalogs[n].atr_size>GSC_SESSION_MAX_SOURCE)
        return session_leave_rejected(s,"turret animations require two bounded ATR byte inputs",error,cap);
    why[0]=0;
    for(n=0;n<2;n++) {
        bytes=GSC_SESSION_MALLOC(copy.catalogs[n].atr_size);if(!bytes)goto done;
        memcpy(bytes,copy.catalogs[n].atr_bytes,copy.catalogs[n].atr_size);
        catalogs[n]=gsc_animation_catalog_parse(names[n],bytes,copy.catalogs[n].atr_size,why,sizeof(why));
        GSC_SESSION_FREE(bytes);bytes=NULL;if(!catalogs[n]||!gsc_animation_catalog_count(catalogs[n]))goto done;
    }
    if(!session_animation_namespace_certificate(s,&copy.map_certificate)||
       s->view.state!=GSC_SESSION_EMPTY||s->program||s->runtime||s->animation_namespace_enabled||
       s->start_origin_enabled||memcmp(input,&copy,sizeof(copy)))goto done;
    for(n=0;n<2;n++){s->animation_namespace_catalogs[n]=catalogs[n];catalogs[n]=NULL;}
    s->animation_namespace_map=copy.map_certificate;s->animation_namespace_enabled=1;
    s->turret_animations=copy.host;s->turret_animations_enabled=1;ok=1;
done:
    GSC_SESSION_FREE(bytes);for(n=0;n<2;n++)gsc_animation_catalog_free(catalogs[n]);s->busy=0;
    return ok?1:session_reject(error,cap,why[0]?why:"turret animation selection expired/refused");
}
typedef struct {
    GscSession *session;GscRuntime *runtime;uint64_t id,instance;int now,checking,cancelled;
    GscTurretAnimationHost host;int (*current)(void *);void *context;
} SessionTurretAnimationLease;
static int session_turret_animation_scope(const SessionTurretAnimationLease *l) {
    GscSession *s=l->session;
    return !l->cancelled&&s&&s->busy&&!s->abort_requested&&!s->view.error[0]&&
        s->view.state==GSC_SESSION_RUNNING&&s->turret_animations_enabled&&s->animation_namespace_enabled&&
        s->runtime==l->runtime&&s->view.session_id==l->id&&s->view.logical_ms==l->now&&
        gsc_runtime_instance_identity(l->runtime)==l->instance&&
        !memcmp(&s->turret_animations,&l->host,sizeof(l->host));
}
static int session_turret_animation_current(void *context) {
    SessionTurretAnimationLease *l=context;int ok;
    if(!session_turret_animation_scope(l))return 0;
    if(l->checking){l->cancelled=1;return 0;}
    l->checking=1;ok=l->current&&l->current(l->context)==1&&session_turret_animation_scope(l);
    l->checking=0;return ok;
}
static void session_turret_animation_lease(GscSession *s,SessionTurretAnimationLease *l,int (*current)(void*),void *context) {
    memset(l,0,sizeof(*l));l->session=s;l->runtime=s->runtime;l->id=s->view.session_id;
    l->instance=gsc_runtime_instance_identity(s->runtime);l->now=s->view.logical_ms;
    l->host=s->turret_animations;l->current=current;l->context=context;
}
static int session_turret_animation_validate(void *user,const GscEntityRef *input,uint64_t identity,int32_t now,
    char *error,size_t cap,int (*current)(void*),void *context) {
    GscSession *s=user;SessionTurretAnimationLease l;GscEntityRef copy,original;int ok;
    if(!s||!input||!current)return 0;copy=original=*input;session_turret_animation_lease(s,&l,current,context);
    if(identity!=l.instance||now!=l.now||!session_turret_animation_current(&l))return 0;
    ok=l.host.validate(l.host.user,&copy,identity,now,error,cap,session_turret_animation_current,&l);
    return ok==1&&session_turret_animation_current(&l)&&!memcmp(&copy,&original,sizeof(copy))&&!memcmp(input,&original,sizeof(copy));
}
static int session_turret_animation_invoke(void *user,const GscTurretAnimationRequest *input,
    char *error,size_t cap,int (*current)(void*),void *context) {
    GscSession *s=user;SessionTurretAnimationLease l;GscTurretAnimationRequest copy,original;int ok;
    if(!s||!input||!current)return 0;copy=original=*input;session_turret_animation_lease(s,&l,current,context);
    if(copy.animation.runtime_identity!=l.instance||copy.now_ms!=l.now||!copy.catalog_identity||
       !session_turret_animation_current(&l))return 0;
    ok=l.host.invoke(l.host.user,&copy,error,cap,session_turret_animation_current,&l);
    return ok==1&&session_turret_animation_current(&l)&&!memcmp(&copy,&original,sizeof(copy))&&!memcmp(input,&original,sizeof(copy));
}

static int session_turret_animation_runtime(GscSession *s) {
    GscTurretAnimationSelection selection;memset(&selection,0,sizeof(selection));
    selection.catalogs[0]=s->animation_namespace_catalogs[0];selection.catalogs[1]=s->animation_namespace_catalogs[1];
    selection.map_certificate=s->animation_namespace_map;selection.host.user=s;
    selection.host.validate=session_turret_animation_validate;selection.host.invoke=session_turret_animation_invoke;
    if(s->training_rigs && !session_animation_namespace_certificate(s,&s->training_rig_map))return 0;
    return gsc_runtime_select_training_trees(s->runtime,&selection,s->animation_namespace_catalogs[2],s->animation_namespace_catalogs[3]);
}
