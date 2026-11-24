/* Explicit map lifetime, native host clock, copied immutable requests. */
int gsc_session_select_map_actor_look(GscSession*s,const GscActorLookHost*input,char*error,size_t cap){
 GscActorLookHost h;GscMapExecutionCertificate map;
 if(!s||!input||(cap&&!error)||session_animation_namespace_private(s,input,sizeof(*input))||
    session_animation_namespace_private(s,error,cap)||session_animation_namespace_overlap(input,sizeof(*input),error,cap))return 0;
 h=*input;map=s->root_map;
 if(s->busy||s->view.state!=GSC_SESSION_EMPTY||s->program||s->runtime||s->view.error[0]||s->abort_requested||
    !s->root_map_enabled||!s->map_actor_program_enabled||!s->actor_program_enabled||s->actor_bootstrap_enabled||
    !s->map_actor_animations_enabled||!s->animation_namespace_enabled||!s->map_animation_names.count||strcmp(s->map_animation_names.names[0],"generic_human")||
    s->turret_animations_enabled||s->actor_look_enabled||s->map_actor_look_enabled||!h.validate||!h.invoke||
    !map.receipt||!map.current||map.receipt!=s->actor_program.epoch)return session_reject(error,cap,"map commands require idle same-map Actor program and generic namespace");
 if(!session_enter(s,error,cap))return 0;
 if(!session_animation_resources_current(s)||!session_actor_program_current(s)||!session_animation_resources_current(s)||
    memcmp(input,&h,sizeof(h))||memcmp(&s->root_map,&map,sizeof(map)))return session_failed(s,"map Actor animation resources retired",error,cap);
 s->actor_look=h;s->actor_look_enabled=s->map_actor_look_enabled=1;s->busy=0;return 1;
}
typedef struct {
 GscSession*s;GscRuntime*r;uint64_t id,instance;int now,hostNow,checking,cancelled;int64_t origin;
 GscMapExecutionCertificate map;GscActorLookHost host;int(*current)(void*);void*context;
 GscEntityRef owner,forwardedOwner;const GscEntityRef*input;
 GscActorLookRequest q,original,expected;const GscActorLookRequest*borrowed;
} SessionMapActorLookLease;
static int session_map_actor_look_scope(const SessionMapActorLookLease*l){
 GscSession*s=l->s;
 return !l->cancelled&&s&&s->busy&&!s->abort_requested&&!s->view.error[0]&&s->view.state==GSC_SESSION_RUNNING&&
  s->runtime==l->r&&l->r&&s->view.session_id==l->id&&l->id&&s->view.logical_ms==l->now&&
  s->view.origin_host_ms==l->origin&&l->origin>=0&&l->origin<=INT32_MAX&&l->now>=0&&l->now<=INT32_MAX-l->origin&&
  l->hostNow==l->origin+l->now&&s->view.last_host_ms==l->hostNow&&
  gsc_runtime_instance_identity(l->r)==l->instance&&l->instance&&s->map_actor_look_enabled&&s->actor_look_enabled&&
  s->map_actor_program_enabled&&s->actor_program_enabled&&s->actor_bootstrap_enabled&&s->root_map_enabled&&
  !memcmp(&s->root_map,&l->map,sizeof(l->map))&&!memcmp(&s->actor_look,&l->host,sizeof(l->host))&&
  !memcmp(l->input,&l->owner,sizeof(l->owner))&&!memcmp(&l->forwardedOwner,&l->owner,sizeof(l->owner))&&
  (!l->borrowed||(!memcmp(l->borrowed,&l->original,sizeof(l->original))&&!memcmp(&l->q,&l->expected,sizeof(l->q))));
}
static int session_map_actor_look_resources(const SessionMapActorLookLease*l){
 return session_map_actor_look_scope(l)&&session_r237_frame_resources_current(l->s)&&
  session_map_actor_look_scope(l)&&session_actor_program_current(l->s)&&
  session_map_actor_look_scope(l)&&session_r237_frame_resources_current(l->s)&&session_map_actor_look_scope(l);
}
static int session_map_actor_look_current(void*u){
 SessionMapActorLookLease*l=u;int ok;
 if(!session_map_actor_look_scope(l))return 0;
 if(l->checking){l->cancelled=1;return 0;}l->checking=1;
 ok=session_map_actor_look_resources(l)&&l->current(l->context)==1&&session_map_actor_look_resources(l);
 l->checking=0;return ok;
}
static int session_map_actor_look_begin(GscSession*s,const GscEntityRef*input,uint64_t identity,int now,
 SessionMapActorLookLease*l,char*error,size_t cap,int(*current)(void*),void*ctx){
 if(!s||!input||!current||(cap&&!error)||session_animation_namespace_private(s,input,sizeof(*input))||
    session_animation_namespace_private(s,error,cap)||session_animation_namespace_overlap(input,sizeof(*input),error,cap))return 0;
 memset(l,0,sizeof(*l));l->s=s;l->r=s->runtime;l->id=s->view.session_id;l->instance=gsc_runtime_instance_identity(l->r);l->now=s->view.logical_ms;
 l->origin=s->view.origin_host_ms;if(l->origin<0||l->origin>INT32_MAX||now<0||now>INT32_MAX-l->origin)return 0;
 l->hostNow=(int)(l->origin+now);l->map=s->root_map;l->host=s->actor_look;l->current=current;l->context=ctx;
 l->input=input;l->owner=l->forwardedOwner=*input;
 return identity==l->instance&&now==l->now&&session_map_actor_look_scope(l);
}
static int session_map_actor_look_validate(void*u,const GscEntityRef*input,uint64_t identity,int32_t now,
 char*error,size_t cap,int(*current)(void*),void*ctx){
 SessionMapActorLookLease l;int ok;
 if(!session_map_actor_look_begin(u,input,identity,now,&l,error,cap,current,ctx)||!session_map_actor_look_current(&l))return 0;
 ok=l.host.validate(l.host.user,&l.forwardedOwner,identity,l.hostNow,error,cap,session_map_actor_look_current,&l);
 return ok==1&&session_map_actor_look_current(&l);
}
static int session_map_actor_look_invoke(void*u,const GscActorLookRequest*input,GscActorLookReply*out,char*error,size_t cap,int(*current)(void*),void*ctx){
 SessionMapActorLookLease l;GscActorLookReply reply;int ok;GscSession*s=u;
 if(!s||!input||!out||(cap&&!error)||session_animation_namespace_private(s,out,sizeof(*out))||session_animation_namespace_overlap(input,sizeof(*input),out,sizeof(*out))||session_animation_namespace_overlap(out,sizeof(*out),error,cap)||session_animation_namespace_private(s,input,sizeof(*input))||
    session_animation_namespace_overlap(input,sizeof(*input),error,cap)||
    !session_map_actor_look_begin(s,&input->entity,input->runtime_identity,input->now_ms,&l,error,cap,current,ctx))return 0;
 l.borrowed=input;l.original=*input;l.q=l.original;l.q.now_ms=l.hostNow;l.expected=l.q;
 if(!l.q.catalog_identity||!session_map_actor_look_current(&l))return 0;
 memset(&reply,0xff,sizeof(reply));ok=l.host.invoke(l.host.user,&l.q,&reply,error,cap,session_map_actor_look_current,&l);
 if(ok!=1||!session_map_actor_look_current(&l))return 0;*out=reply;return 1;
}
