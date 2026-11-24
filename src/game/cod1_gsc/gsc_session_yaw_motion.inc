int gsc_session_select_r216_yaw(GscSession *s,const GscYawMotionHost *host,
    char *error,size_t size) {
    if(!s||!host||!host->invoke||s->busy||s->program||s->runtime||s->yaw_motion_enabled||
       !s->execution_extensions||!s->state_enabled||!s->scalar_switch_enabled||!s->names_enabled)
        return session_reject(error,size,"yaw requires idle selected execution Session");
    s->yaw_motion=*host;s->yaw_motion_enabled=1;return 1;
}
typedef struct {
    GscSession *session;uint64_t id;int logical_ms;GscRuntime *runtime;
    GscYawMotionHost host;int (*current)(void *);void *context;
} SessionYawMotionLease;
static int session_yaw_motion_scope(const SessionYawMotionLease *l) {
    GscSession *s=l->session;
    return s&&s->busy&&!s->abort_requested&&s->yaw_motion_enabled&&
        s->view.state==GSC_SESSION_RUNNING&&s->view.session_id==l->id&&
        s->view.logical_ms==l->logical_ms&&s->runtime==l->runtime&&
        !memcmp(&s->yaw_motion,&l->host,sizeof(l->host));
}
static int session_yaw_motion_current(void *context) {
    SessionYawMotionLease *l=context;
    return session_yaw_motion_scope(l)&&l->current&&l->current(l->context)==1&&
        session_yaw_motion_scope(l);
}
static int session_yaw_motion_invoke(void *user,const GscYawMotionRequest *input,
    char *error,size_t size,int (*current)(void *),void *context) {
    GscSession *s=user;SessionYawMotionLease l;GscYawMotionRequest request,original;int ok;
    if(!s||!input||!current)return 0;
    l.session=s;l.id=s->view.session_id;l.logical_ms=s->view.logical_ms;l.runtime=s->runtime;
    l.host=s->yaw_motion;l.current=current;l.context=context;
    if(!session_yaw_motion_scope(&l))return 0;
    request=original=*input;
    ok=l.host.invoke(l.host.user,&request,error,size,session_yaw_motion_current,&l);
    return ok==1&&session_yaw_motion_scope(&l)&&!memcmp(&request,&original,sizeof(request));
}

int gsc_session_select_map_moveto(GscSession*s,char*error,size_t cap){
 if((cap&&!error)||!s||session_animation_namespace_private(s,error,cap))return 0;
 if(!session_enter(s,error,cap))return 0;
 if(s->view.state!=GSC_SESSION_EMPTY||s->program||s->runtime||!s->root_map_enabled||!s->root_map.receipt||!s->root_map.current||s->actor_bootstrap_enabled||!s->yaw_motion_enabled||s->map_moveto_enabled)
  return session_leave_rejected(s,"map moveto requires idle map authority and real motion host",error,cap);
 s->map_moveto_enabled=1;s->busy=0;return 1;
}
