int gsc_session_select_r243_scripted_start(GscSession*s,const GscActorScriptedStartHost*input,char*error,size_t cap){
 GscActorScriptedStartHost host;
 if((cap&&!error)||session_animation_namespace_private(s,input,sizeof(*input))||
    session_animation_namespace_private(s,error,cap)||session_animation_namespace_overlap(input,sizeof(*input),error,cap))return 0;
 if(!s||!input)return session_reject(error,cap,"normal scripted start requires native Actor host");
 host=*input;if(!session_enter(s,error,cap))return 0;
 if(s->view.state!=GSC_SESSION_EMPTY||s->program||s->runtime||!s->r243_scripted_prefix_enabled||
    !s->r243_script_state_enabled||s->r243_scripted_start_enabled||!host.invoke)
  return session_leave_rejected(s,"normal scripted start requires idle EMPTY prefix/tracking profile",error,cap);
 if(!session_r237_frame_resources_current(s)||memcmp(input,&host,sizeof(host)))
  return session_leave_rejected(s,"normal scripted start resources retired",error,cap);
 s->r243_scripted_start=host;s->r243_scripted_start_enabled=1;s->busy=0;return 1;
}
typedef struct {
 SessionTurretAnimationLease base;GscActorScriptedStartHost host;
 const GscActorScriptedPrefixRequest *input,*upstream;
 GscActorScriptedPrefixRequest q,logical;
} SessionScriptedStartLease;
static int session_scripted_start_scope(const SessionScriptedStartLease*l){
 int hostNow;
 return r243_command_host_time(l->base.session,l->base.now,&hostNow) &&
  hostNow==l->q.now_ms && !memcmp(l->upstream,&l->logical,sizeof(l->logical)) &&
  session_turret_animation_scope(&l->base)&&l->base.session->r243_scripted_start_enabled&&
  !memcmp(&l->base.session->r243_scripted_start,&l->host,sizeof(l->host))&&!memcmp(l->input,&l->q,sizeof(l->q));
}
static int session_scripted_start_current(void*context){
 SessionScriptedStartLease*l=context;
 return session_scripted_start_scope(l)&&session_r237_frame_resources_current(l->base.session)&&
  session_scripted_start_scope(l)&&session_turret_animation_current(&l->base)&&session_scripted_start_scope(l);
}
static int session_scripted_start_invoke(void*user,const GscActorScriptedPrefixRequest*input,
 char*error,size_t cap,int(*current)(void*),void*context){
 GscSession*s=user;SessionScriptedStartLease l;GscActorScriptedPrefixRequest q;int ok;
 if(!s||!input||!current||(cap&&!error)||
    session_animation_namespace_private(s,input,sizeof(*input))||session_animation_namespace_private(s,error,cap)||
    session_animation_namespace_overlap(input,sizeof(*input),error,cap))return 0;
 memset(&l,0,sizeof(l));q=*input;l.input=&q;l.upstream=input;l.logical=q;l.host=s->r243_scripted_start;
 session_turret_animation_lease(s,&l.base,current,context);
 /* VM timers start at map entry. GAME owns the pose and queued AI
  * operation at the corresponding absolute server tick. */
 if(q.runtime_identity!=l.base.instance || q.now_ms!=l.base.now ||
    !r243_command_host_time(s,l.base.now,&q.now_ms))return 0;
 l.q=q;
 if(!session_scripted_start_current(&l))return 0;
 ok=l.host.invoke(l.host.user,&q,error,cap,session_scripted_start_current,&l);
 return ok==1&&session_scripted_start_current(&l)&&!memcmp(input,&l.logical,sizeof(l.logical));
}
