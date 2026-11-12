/* EMPTY-only tracking declaration may precede BSP cohort admission. An exact
 * Actor program plan owns that early declaration; runtime preparation still
 * requires fresh cohort/resource admission. No execution authority is created. */
int gsc_session_select_r243_script_state(GscSession*s,const GscActorScriptStateHost*input,char*error,size_t cap){
 GscActorScriptStateHost host;int planned,cohort;
 if((cap&&!error)||session_animation_namespace_private(s,input,sizeof(*input))||
    session_animation_namespace_private(s,error,cap)||session_animation_namespace_overlap(input,sizeof(*input),error,cap))return 0;
 if(!s||!input)return session_reject(error,cap,"tracking requires a genuine Actor host");
 if(s->view.error[0]||s->abort_requested)return session_reject(error,cap,"tracking selection retains terminal Session error");
 host=*input;if(!session_enter(s,error,cap))return 0;
 if(s->view.state!=GSC_SESSION_EMPTY||s->program||s->runtime||(!s->actor_bootstrap_enabled&&!s->actor_program_enabled)||
    !s->actor_animations_enabled||s->r243_script_state_enabled||!host.invoke||!host.read)
  return session_leave_rejected(s,"tracking requires idle EMPTY Actor-animation profile",error,cap);
 planned=s->actor_program_enabled;cohort=s->actor_bootstrap_enabled;
 if(!session_actor_program_current(s)||planned!=s->actor_program_enabled||cohort!=s->actor_bootstrap_enabled||!session_r237_frame_resources_current(s)||
    !session_actor_program_current(s)||planned!=s->actor_program_enabled||cohort!=s->actor_bootstrap_enabled||memcmp(input,&host,sizeof(host)))
  return session_leave_rejected(s,"tracking selection resources retired",error,cap);
 s->r243_script_state=host;s->r243_script_state_enabled=1;s->busy=0;return 1;
}
typedef struct {
 SessionTurretAnimationLease base;GscActorScriptStateHost host;
 const GscActorScriptStateCall*input;GscActorScriptStateCall call;
 const GscActorScriptStateCall*forwarded;GscActorScriptStateCall expectedForwarded;
 GscEntityRef owner;const GscEntityRef*borrowed;
} SessionScriptStateLease;
static int session_script_state_scope(const SessionScriptStateLease*l){
 return session_turret_animation_scope(&l->base)&&l->base.session->r243_script_state_enabled&&
  !memcmp(&l->base.session->r243_script_state,&l->host,sizeof(l->host))&&
  !memcmp(l->input,&l->call,sizeof(l->call))&&!memcmp(l->borrowed,&l->owner,sizeof(l->owner))&&
  (!l->forwarded||!memcmp(l->forwarded,&l->expectedForwarded,sizeof(l->expectedForwarded)));
}
static int session_script_state_current(void*opaque){
 SessionScriptStateLease*l=opaque;
 return session_script_state_scope(l)&&session_r237_frame_resources_current(l->base.session)&&
  session_script_state_scope(l)&&session_turret_animation_current(&l->base)&&session_script_state_scope(l);
}
static int session_script_state_read(void*opaque,unsigned index,char*out,size_t cap,char*error,size_t size){
 SessionScriptStateLease*l=opaque;int ok;
 if(!session_script_state_current(l))return 0;
 ok=l->call.read(l->call.context,index,out,cap,error,size);
 return ok==1&&session_script_state_current(l);
}
static int session_script_state_invoke(void*user,const GscEntityRef*input,
 const GscActorScriptStateCall*request,char*error,size_t cap){
 GscSession*s=user;SessionScriptStateLease l;GscActorScriptStateCall forwarded;GscEntityRef ref;int ok;
 if(!s||!input||!request||!request->current||!request->read||(cap&&!error)||
    session_animation_namespace_private(s,input,sizeof(*input))||
    session_animation_namespace_private(s,request,sizeof(*request))||
    session_animation_namespace_private(s,error,cap)||
    session_animation_namespace_overlap(input,sizeof(*input),request,sizeof(*request))||
    session_animation_namespace_overlap(input,sizeof(*input),error,cap)||
    session_animation_namespace_overlap(request,sizeof(*request),error,cap))return 0;
 memset(&l,0,sizeof(l));ref=*input;l.owner=ref;l.borrowed=input;l.input=request;l.call=*request;
 l.host=s->r243_script_state;
 session_turret_animation_lease(s,&l.base,l.call.current,l.call.context);
 if(!session_script_state_current(&l))return 0;
 forwarded=l.call;forwarded.context=&l;forwarded.read=session_script_state_read;forwarded.current=session_script_state_current;
 l.expectedForwarded=forwarded;l.forwarded=&forwarded;
 ok=l.host.invoke(l.host.user,&ref,&forwarded,error,cap);
 return ok==1&&session_script_state_current(&l)&&!memcmp(&ref,&l.owner,sizeof(ref));
}
