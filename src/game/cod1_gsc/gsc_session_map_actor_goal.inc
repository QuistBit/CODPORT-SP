/* Map goal composition retains VM entity checks and native Actor leases. */
int gsc_session_select_map_actor_goal(GscSession*s,char*error,size_t cap){
 GscMapExecutionCertificate map;
 if(!s||(cap&&!error)||session_animation_namespace_private(s,error,cap))return 0;
 map=s->root_map;
 if(s->busy||s->view.state!=GSC_SESSION_EMPTY||s->program||s->runtime||s->view.error[0]||s->abort_requested||
    !s->root_map_enabled||!s->map_actor_program_enabled||!s->actor_program_enabled||s->actor_bootstrap_enabled||
    !s->map_actor_animations_enabled||s->actor_goal_enabled!=2||s->map_actor_goal_enabled||!s->actor_goal.invoke||
    !map.receipt||!map.current||map.receipt!=s->actor_program.epoch)return session_reject(error,cap,"map vector goals require idle same-map Actor program and goal host");
 if(!session_enter(s,error,cap))return 0;
 if(!session_animation_resources_current(s)||!session_actor_program_current(s)||!session_animation_resources_current(s)||
    memcmp(&s->root_map,&map,sizeof(map)))return session_failed(s,"map goal resources retired",error,cap);
 s->map_actor_goal_enabled=1;s->busy=0;return 1;
}
typedef struct {
 GscSession*s;GscRuntime*r;uint64_t id,instance;int now,hostNow,checking,cancelled;int64_t origin;
 GscMapExecutionCertificate map;GscActorGoalHost host;int(*current)(void*);void*context;
 GscActorGoalRequest q,original,expected;const GscActorGoalRequest*borrowed;
} SessionMapActorGoalLease;
static int session_map_actor_goal_scope(const SessionMapActorGoalLease*l){
 GscSession*s=l->s;
 return !l->cancelled&&s&&s->busy&&!s->abort_requested&&!s->view.error[0]&&s->view.state==GSC_SESSION_RUNNING&&
  s->runtime==l->r&&l->r&&s->view.session_id==l->id&&l->id&&s->view.logical_ms==l->now&&
  s->view.origin_host_ms==l->origin&&l->origin>=0&&l->origin<=INT32_MAX&&l->now>=0&&l->now<=INT32_MAX-l->origin&&
  l->hostNow==l->origin+l->now&&s->view.last_host_ms==l->hostNow&&
  gsc_runtime_instance_identity(l->r)==l->instance&&l->instance&&s->map_actor_goal_enabled&&s->actor_goal_enabled==2&&
  s->map_actor_program_enabled&&s->actor_program_enabled&&s->actor_bootstrap_enabled&&s->root_map_enabled&&
  !memcmp(&s->root_map,&l->map,sizeof(l->map))&&!memcmp(&s->actor_goal,&l->host,sizeof(l->host))&&
  l->borrowed&&!memcmp(l->borrowed,&l->original,sizeof(l->original))&&!memcmp(&l->q,&l->expected,sizeof(l->q));
}
static int session_map_actor_goal_resources(const SessionMapActorGoalLease*l){
 return session_map_actor_goal_scope(l)&&session_r237_frame_resources_current(l->s)&&
  session_map_actor_goal_scope(l)&&session_actor_program_current(l->s)&&
  session_map_actor_goal_scope(l)&&session_r237_frame_resources_current(l->s)&&session_map_actor_goal_scope(l);
}
static int session_map_actor_goal_current(void*u){
 SessionMapActorGoalLease*l=u;int ok;
 if(!session_map_actor_goal_scope(l))return 0;
 if(l->checking){l->cancelled=1;return 0;}l->checking=1;
 ok=session_map_actor_goal_resources(l)&&l->current(l->context)==1&&session_map_actor_goal_resources(l);
 l->checking=0;return ok;
}
static int session_map_actor_goal_invoke(void*u,const GscActorGoalRequest*input,char*error,size_t cap,int(*current)(void*),void*ctx){
 SessionMapActorGoalLease l;GscSession*s=u;int ok;
 if(!s||!input||!current||(cap&&!error)||session_animation_namespace_private(s,input,sizeof(*input))||
    session_animation_namespace_private(s,error,cap)||session_animation_namespace_overlap(input,sizeof(*input),error,cap))return 0;
 memset(&l,0,sizeof(l));l.s=s;l.r=s->runtime;l.id=s->view.session_id;l.instance=gsc_runtime_instance_identity(l.r);l.now=s->view.logical_ms;
 l.origin=s->view.origin_host_ms;if(l.origin<0||l.origin>INT32_MAX||l.now<0||l.now>INT32_MAX-l.origin)return 0;
 l.hostNow=(int)(l.origin+l.now);l.map=s->root_map;l.host=s->actor_goal;l.current=current;l.context=ctx;
 l.borrowed=input;l.original=*input;l.q=l.original;l.q.now_ms=l.hostNow;l.expected=l.q;
 if(input->now_ms!=l.now||!session_map_actor_goal_current(&l))return 0;
 ok=l.host.invoke(l.host.user,&l.q,error,cap,session_map_actor_goal_current,&l);
 return ok==1&&session_map_actor_goal_current(&l);
}
