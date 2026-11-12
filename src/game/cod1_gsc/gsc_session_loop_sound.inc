int gsc_session_select_local_identifier_case(GscSession *s,char *err,size_t cap){
 if(!s||s->busy||s->program||s->runtime||!s->dynamic_randomfloat_enabled||s->local_identifier_case_enabled)
  return session_reject(err,cap,"local identifiers require idle mode40 session");
 s->local_identifier_case_enabled=1;return 1;
}
int gsc_session_select_loop_sound(GscSession *s,const GscLoopSoundHost *h,char *err,size_t cap){
 if(!s||!h||!h->invoke||!h->finish||s->busy||s->program||s->runtime||s->loop_sound_enabled||
    !s->dynamic_randomfloat_enabled||
    ((uintptr_t)h<=(uintptr_t)s?(uintptr_t)s-(uintptr_t)h<sizeof(*h):(uintptr_t)h-(uintptr_t)s<sizeof(*s)))
  return session_reject(err,cap,"loop sound requires idle selected mode40 session");
 s->loop_sound=*h;s->loop_sound_enabled=1;return 1;
}
typedef struct {
 GscSession *s;GscLoopSoundRequest request,sealed;const GscLoopSoundRequest *upstream;
 GscLoopSoundHost host;uint64_t id;uintptr_t token;int now,checking,cancelled,active;
 int (*current)(void *);void *context;
} SessionLoopSoundCall;
static SessionLoopSoundCall *sessionLoopSoundActive;static uintptr_t sessionLoopSoundToken;
static int session_loop_sound_scope(SessionLoopSoundCall *c){
 return c&&c==sessionLoopSoundActive&&c->active&&!c->cancelled&&c->s&&c->s->busy&&!c->s->abort_requested&&
  c->s->loop_sound_enabled&&c->s->view.state==GSC_SESSION_RUNNING&&
  !c->s->view.error[0]&&c->s->view.session_id==c->id&&c->s->view.logical_ms==c->now&&
  c->s->loop_sound.user==c->host.user&&c->s->loop_sound.invoke==c->host.invoke&&c->s->loop_sound.finish==c->host.finish&&
  !memcmp(&c->request,&c->sealed,sizeof(c->sealed))&&
  !memcmp(c->upstream,&c->sealed,sizeof(c->sealed));
}
static int session_loop_sound_current(void *p){
 SessionLoopSoundCall *c=sessionLoopSoundActive;int ok;
 if(!c||(uintptr_t)p!=c->token)return 0;
 if(!session_loop_sound_scope(c)||c->checking){if(c)c->cancelled=1;return 0;}
 c->checking=1;ok=c->current(c->context)==1&&session_loop_sound_scope(c);
 c->checking=0;if(!ok)c->cancelled=1;return ok;
}
static int session_loop_sound(void *user,const GscLoopSoundRequest *q,char *error,size_t cap,
 int (*current)(void *),void *context){
 GscSession *s=user;SessionLoopSoundCall c;int ok,valid;
 if(!s||!s->busy||s->abort_requested||s->view.state!=GSC_SESSION_RUNNING||
    !s->loop_sound_enabled||!q||!current||sessionLoopSoundActive||sessionLoopSoundToken==UINTPTR_MAX)
  return session_reject(error,cap,"loop sound outside running session");
 memset(&c,0,sizeof(c));c.s=s;c.request=*q;c.sealed=*q;c.upstream=q;c.host=s->loop_sound;
 c.id=s->view.session_id;c.now=s->view.logical_ms;c.current=current;c.context=context;c.token=++sessionLoopSoundToken;c.active=1;sessionLoopSoundActive=&c;
 ok=session_loop_sound_current((void*)c.token)&&c.host.invoke(c.host.user,&c.request,error,cap,session_loop_sound_current,(void*)c.token)==1;
 valid=session_loop_sound_current((void*)c.token);c.active=0;sessionLoopSoundActive=NULL;
 return ok&&valid;
}

static int session_loop_sound_finish(void *user,const GscLoopSoundRequest *q,int commit,
 char *error,size_t cap,int (*current)(void*),void *context){
 GscSession *s=user;SessionLoopSoundCall c;int ok,valid;
 if(!s||!q||!q->runtime||!q->operation||!s->loop_sound_enabled||!s->loop_sound.finish||
    !current||sessionLoopSoundActive||sessionLoopSoundToken==UINTPTR_MAX)return 0;
 memset(&c,0,sizeof(c));c.s=s;c.request=*q;c.sealed=*q;c.upstream=q;c.host=s->loop_sound;
 c.id=s->view.session_id;c.now=s->view.logical_ms;c.current=current;c.context=context;
 c.token=++sessionLoopSoundToken;c.active=1;sessionLoopSoundActive=&c;
 if(!commit){
  /* Abort may already be sticky. The provider's immutable receipt is the
   * authority to cancel; a running Session is deliberately not required. */
  ok=c.host.finish(c.host.user,&c.sealed,0,error,cap,session_loop_sound_current,(void*)c.token)==1;
 }else{
  ok=session_loop_sound_current((void*)c.token)&&
   c.host.finish(c.host.user,&c.request,1,error,cap,session_loop_sound_current,(void*)c.token)==1;
  valid=session_loop_sound_current((void*)c.token);ok=ok&&valid;
 }
 c.active=0;sessionLoopSoundActive=NULL;return ok;
}
