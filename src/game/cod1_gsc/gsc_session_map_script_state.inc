/* Native tracking data is owned by Actor allocation, independent of a
 * mounted weapon. Explicit map composition uses genuine root/Actor resources;
 * the older exclusive turret selector and its callback scope stay unchanged. */
int gsc_session_select_map_script_state(GscSession*s,const GscActorScriptStateHost*input,char*error,size_t cap){
 GscActorScriptStateHost copy;GscMapExecutionCertificate map;
 if(!s||!input||(cap&&!error)||session_animation_namespace_private(s,input,sizeof(*input))||
    session_animation_namespace_private(s,error,cap)||session_animation_namespace_overlap(input,sizeof(*input),error,cap))return 0;
 copy=*input;map=s->root_map;
 if(s->busy||s->view.state!=GSC_SESSION_EMPTY||s->program||s->runtime||s->view.error[0]||s->abort_requested||
    !s->root_map_enabled||!s->map_actor_program_enabled||!s->actor_program_enabled||s->actor_bootstrap_enabled||
    s->turret_animations_enabled||s->r243_script_state_enabled||s->map_script_state_enabled||!copy.invoke||!copy.read||
    !map.receipt||!map.current||map.receipt!=s->actor_program.epoch)return session_reject(error,cap,"map tracking requires idle same-map Actor program");
 if(!session_enter(s,error,cap))return 0;
 if(!session_animation_resources_current(s)||!session_actor_program_current(s)||!session_animation_resources_current(s)||
    memcmp(input,&copy,sizeof(copy))||memcmp(&s->root_map,&map,sizeof(map)))return session_failed(s,"map tracking resources retired during selection",error,cap);
 s->r243_script_state=copy;s->r243_script_state_enabled=1;s->map_script_state_enabled=1;s->busy=0;return 1;
}
typedef struct {
 GscSession*s;GscRuntime*r;uint64_t id,instance;int now,checking,cancelled;
 GscMapExecutionCertificate map;GscActorScriptStateHost host;
 GscEntityRef owner,forwardedOwner;const GscEntityRef*input;
 GscActorScriptStateCall call,forwarded,expected;const GscActorScriptStateCall*borrowed;
} SessionMapScriptStateLease;
static int session_map_script_state_scope(const SessionMapScriptStateLease*l){
 GscSession*s=l->s;
 return !l->cancelled&&s&&s->busy&&!s->abort_requested&&!s->view.error[0]&&s->view.state==GSC_SESSION_RUNNING&&
  s->runtime==l->r&&l->r&&s->view.session_id==l->id&&l->id&&s->view.logical_ms==l->now&&
  gsc_runtime_instance_identity(l->r)==l->instance&&l->instance&&s->map_script_state_enabled&&s->r243_script_state_enabled&&
  s->map_actor_program_enabled&&s->actor_program_enabled&&s->actor_bootstrap_enabled&&s->root_map_enabled&&
  !memcmp(&s->root_map,&l->map,sizeof(l->map))&&!memcmp(&s->r243_script_state,&l->host,sizeof(l->host))&&
  !memcmp(l->input,&l->owner,sizeof(l->owner))&&!memcmp(&l->forwardedOwner,&l->owner,sizeof(l->owner))&&
  !memcmp(l->borrowed,&l->call,sizeof(l->call))&&!memcmp(&l->forwarded,&l->expected,sizeof(l->expected));
}
static int session_map_script_state_resources(const SessionMapScriptStateLease*l){
 return session_map_script_state_scope(l)&&session_r237_frame_resources_current(l->s)&&
  session_map_script_state_scope(l)&&session_actor_program_current(l->s)&&
  session_map_script_state_scope(l)&&session_r237_frame_resources_current(l->s)&&session_map_script_state_scope(l);
}
static int session_map_script_state_current(void*u){
 SessionMapScriptStateLease*l=u;int ok;
 if(!session_map_script_state_scope(l))return 0;
 if(l->checking){l->cancelled=1;return 0;}l->checking=1;
 ok=session_map_script_state_resources(l)&&l->call.current(l->call.context)==1&&session_map_script_state_resources(l);
 l->checking=0;return ok;
}
static int session_map_script_state_argument(void*u,unsigned index,char*out,size_t cap,char*error,size_t size){
 SessionMapScriptStateLease*l=u;int ok;
 if(!session_map_script_state_current(l))return 0;
 ok=l->call.read(l->call.context,index,out,cap,error,size);
 return ok==1&&session_map_script_state_current(l);
}
static int session_map_script_state_invoke(void*u,const GscEntityRef*input,const GscActorScriptStateCall*request,char*error,size_t cap){
 GscSession*s=u;SessionMapScriptStateLease l;int ok;
 if(!s||!input||!request||!request->current||!request->read||(cap&&!error)||
    session_animation_namespace_private(s,input,sizeof(*input))||session_animation_namespace_private(s,request,sizeof(*request))||
    session_animation_namespace_private(s,error,cap)||session_animation_namespace_overlap(input,sizeof(*input),request,sizeof(*request))||
    session_animation_namespace_overlap(input,sizeof(*input),error,cap)||session_animation_namespace_overlap(request,sizeof(*request),error,cap))return 0;
 memset(&l,0,sizeof(l));l.s=s;l.r=s->runtime;l.id=s->view.session_id;l.instance=gsc_runtime_instance_identity(l.r);l.now=s->view.logical_ms;
 l.map=s->root_map;l.host=s->r243_script_state;l.input=input;l.owner=l.forwardedOwner=*input;l.borrowed=request;l.call=*request;
 l.forwarded=l.call;l.forwarded.context=&l;l.forwarded.current=session_map_script_state_current;l.forwarded.read=session_map_script_state_argument;l.expected=l.forwarded;
 if(!session_map_script_state_current(&l))return 0;
 ok=l.host.invoke(l.host.user,&l.forwardedOwner,&l.forwarded,error,cap);
 return ok==1&&session_map_script_state_current(&l);
}
