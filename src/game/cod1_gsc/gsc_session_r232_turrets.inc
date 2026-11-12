int gsc_session_select_r232_turrets(GscSession *s,const GscTurretHost *host,
    char *error,size_t size) {
    if(s&&error&&size&&((uintptr_t)error<=(uintptr_t)s?
       (uintptr_t)s-(uintptr_t)error<size:(uintptr_t)error-(uintptr_t)s<sizeof(*s)))return 0;
    if(host&&error&&size&&((uintptr_t)error<=(uintptr_t)host?
       (uintptr_t)host-(uintptr_t)error<size:(uintptr_t)error-(uintptr_t)host<sizeof(*host)))return 0;
    if(!s||!host||!host->invoke||s->busy||s->program||s->runtime||s->turrets_enabled||
       !s->dynamic_randomfloat_enabled||!s->state_enabled||!s->results_enabled||
       !s->entities_enabled||
       ((uintptr_t)host<=(uintptr_t)s?(uintptr_t)s-(uintptr_t)host<sizeof(*host):
        (uintptr_t)host-(uintptr_t)s<sizeof(*s)))
        return session_reject(error,size,"turrets require an idle selected entity/results Session and real host");
    s->turrets=*host;s->turrets_enabled=1;return 1;
}
typedef struct {
    GscSession *session;uint64_t id;int logical_ms;GscRuntime *runtime;
    GscTurretHost host;int (*current)(void *);void *context;int checking,cancelled;
} SessionR232TurretLease;
static int session_turret_scope(const SessionR232TurretLease *l) {
    GscSession *s=l->session;
    return !l->cancelled&&s&&s->busy&&!s->abort_requested&&!s->view.error[0]&&s->turrets_enabled&&
        s->view.state==GSC_SESSION_RUNNING&&s->view.session_id==l->id&&
        s->view.logical_ms==l->logical_ms&&s->runtime==l->runtime&&l->runtime&&
        !memcmp(&s->turrets,&l->host,sizeof(l->host));
}
static int session_turret_current(void *p) {
    SessionR232TurretLease *l=p;int ok;
    if(!session_turret_scope(l))return 0;
    if(l->checking){l->cancelled=1;return 0;}
    l->checking=1;ok=l->current&&l->current(l->context)==1&&session_turret_scope(l);
    l->checking=0;return ok;
}
static int session_turret_invoke(void *user,const GscTurretRequest *input,
    GscTurretReply *out,char *error,size_t size,int (*current)(void *),void *context) {
    GscSession *s=user;SessionR232TurretLease l;GscTurretRequest copy,original;
    GscTurretReply reply;int ok;
    if(!s||!input||!out||!current)return 0;
    memset(&l,0,sizeof(l));l.session=s;l.id=s->view.session_id;
    l.logical_ms=s->view.logical_ms;l.runtime=s->runtime;l.host=s->turrets;
    l.current=current;l.context=context;copy=original=*input;
    if(copy.now_ms!=l.logical_ms||copy.kind<GSC_TURRET_SET_MODE||copy.kind>GSC_TURRET_SHOOT||
       !session_turret_current(&l))return 0;
    memset(&reply,0,sizeof(reply));reply.kind=-1;
    ok=l.host.invoke(l.host.user,&copy,&reply,error,size,session_turret_current,&l);
    if(ok!=1||!session_turret_current(&l)||memcmp(&copy,&original,sizeof(copy))||
       memcmp(input,&original,sizeof(original)))return 0;
    *out=reply;return 1;
}
