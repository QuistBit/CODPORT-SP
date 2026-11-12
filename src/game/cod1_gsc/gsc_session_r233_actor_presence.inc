/* R233 transports the existing R191 native isAI contract. It does not infer
 * AI readiness, health, classname, or successful animation initialization. */
int gsc_session_select_r233_actor_presence(GscSession *s,const GscActorPresenceHost *input,char *error,size_t cap){
 GscActorPresenceHost copy;
 if((cap&&!error)||session_animation_namespace_private(s,input,sizeof(*input))||
    session_animation_namespace_private(s,error,cap)||session_animation_namespace_overlap(input,sizeof(*input),error,cap))return 0;
 if(!s||!input)return session_reject(error,cap,"actor presence selection requires a real host");copy=*input;
 if(!session_enter(s,error,cap))return 0;
 if(s->view.state!=GSC_SESSION_EMPTY||s->program||s->runtime||!s->turret_animations_enabled||
    s->actor_presence_enabled||!copy.read||!copy.current)
  return session_leave_rejected(s,"actor presence requires exclusive idle selected turret resources",error,cap);
 if(!session_animation_resources_current(s)||memcmp(input,&copy,sizeof(copy)))
  return session_leave_rejected(s,"actor presence selection expired",error,cap);
 s->actor_presence=copy;s->actor_presence_enabled=1;s->busy=0;return 1;
}
typedef struct {GscSession*s;GscRuntime*r;GscActorPresenceHost host;uint64_t id,instance;int now;} SessionActorPresenceLease;
static int session_actor_presence_scope(const SessionActorPresenceLease*l){
 GscSession*s=l->s;return s&&s->busy&&!s->abort_requested&&s->view.state==GSC_SESSION_RUNNING&&
 s->actor_presence_enabled&&s->runtime==l->r&&s->view.session_id==l->id&&s->view.logical_ms==l->now&&
 l->instance&&gsc_runtime_instance_identity(l->r)==l->instance&&
 !memcmp(&s->actor_presence,&l->host,sizeof(l->host));
}
static int session_actor_presence_lease(GscSession*s,SessionActorPresenceLease*l){
 memset(l,0,sizeof(*l));if(!s)return 0;l->s=s;l->r=s->runtime;l->host=s->actor_presence;
 l->id=s->view.session_id;l->now=s->view.logical_ms;l->instance=gsc_runtime_instance_identity(l->r);
 return session_actor_presence_scope(l)&&session_animation_resources_current(s)&&session_actor_presence_scope(l);
}
static int session_actor_presence_read(void*user,const GscEntityRef*input,uint64_t*out,char*error,size_t cap){
 GscSession*s=user;SessionActorPresenceLease l;GscEntityRef ref,original;uint64_t serial=0;int ok;
 if(!input||!out||session_animation_namespace_private(s,input,sizeof(*input))||
    session_animation_namespace_private(s,out,sizeof(*out))||session_animation_namespace_private(s,error,cap)||
    session_animation_namespace_overlap(input,sizeof(*input),out,sizeof(*out))||
    session_animation_namespace_overlap(error,cap,input,sizeof(*input))||session_animation_namespace_overlap(error,cap,out,sizeof(*out)))return 0;
 ref=original=*input;if(!session_actor_presence_lease(s,&l))return 0;
 ok=l.host.read(l.host.user,&ref,&serial,error,cap);
 if(ok!=1||!session_actor_presence_scope(&l)||!session_animation_resources_current(s)||
    !session_actor_presence_scope(&l)||memcmp(&ref,&original,sizeof(ref))||memcmp(input,&original,sizeof(ref)))return 0;
 *out=serial;return 1;
}
static int session_actor_presence_current(void*user,const GscEntityRef*input,uint64_t serial){
 GscSession*s=user;SessionActorPresenceLease l;GscEntityRef ref,original;int ok;
 if(!input||session_animation_namespace_private(s,input,sizeof(*input)))return 0;
 ref=original=*input;if(!session_actor_presence_lease(s,&l))return 0;
 ok=l.host.current(l.host.user,&ref,serial);
 if((ok!=0&&ok!=1)||!session_actor_presence_scope(&l)||!session_animation_resources_current(s)||
    !session_actor_presence_scope(&l)||memcmp(&ref,&original,sizeof(ref))||memcmp(input,&original,sizeof(ref)))return -1;
 return ok;
}
