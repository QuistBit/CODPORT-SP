/* Explicit EMPTY-only selector. Early program+factory profile declares this
 * capability before compile; target admission still needs its own completed
 * allocation, never an invented placed bootstrap row. */
static int session_r237_frame_resources_current(GscSession*);
int gsc_session_select_r243_scripted_prefix(GscSession*s,const GscActorScriptedPrefixHost*input,char*error,size_t cap){
 GscActorScriptedPrefixHost host;
 if((cap&&!error)||session_animation_namespace_private(s,input,sizeof(*input))||
    session_animation_namespace_private(s,error,cap)||session_animation_namespace_overlap(input,sizeof(*input),error,cap))return 0;
 if(!s||!input)return session_reject(error,cap,"scripted prefix requires a genuine Session native host");
 host=*input;if(!session_enter(s,error,cap))return 0;
 if(s->view.state!=GSC_SESSION_EMPTY||s->program||s->runtime||!(s->actor_bootstrap_enabled||(s->actor_program_enabled&&s->actor_spawners_enabled))||
    !s->r243_scripted_init_dependency_enabled||!s->r243_thread_start_enabled||
    !s->actor_animations_enabled||s->r243_scripted_prefix_enabled||!host.invoke)
  return session_leave_rejected(s,"scripted prefix requires idle EMPTY explicit init/Actor/bootstrap profile",error,cap);
 if(!session_r237_frame_resources_current(s)||memcmp(input,&host,sizeof(host)))
  return session_leave_rejected(s,"scripted prefix selection resources retired",error,cap);
 s->r243_scripted_prefix=host;s->r243_scripted_prefix_enabled=1;s->busy=0;return 1;
}
typedef struct {
 SessionTurretAnimationLease base;GscActorScriptedPrefixHost host;
 const GscActorScriptedPrefixRequest *input,*upstream;
 GscActorScriptedPrefixRequest copy,logical;
} SessionScriptedPrefixLease;
static int session_scripted_prefix_scope(const SessionScriptedPrefixLease*l){
 int hostNow;
 return r243_command_host_time(l->base.session,l->base.now,&hostNow) &&
  hostNow==l->copy.now_ms && !memcmp(l->upstream,&l->logical,sizeof(l->logical)) &&
  session_turret_animation_scope(&l->base)&&l->base.session->r243_scripted_prefix_enabled&&
  !memcmp(&l->base.session->r243_scripted_prefix,&l->host,sizeof(l->host))&&
  !memcmp(l->input,&l->copy,sizeof(l->copy));
}
static int session_scripted_prefix_current(void*opaque){
 SessionScriptedPrefixLease*l=opaque;
 return session_scripted_prefix_scope(l)&&session_r237_frame_resources_current(l->base.session)&&
  session_scripted_prefix_scope(l)&&session_turret_animation_current(&l->base)&&
  session_scripted_prefix_scope(l);
}
static int session_scripted_prefix_invoke(void*user,const GscActorScriptedPrefixRequest*input,
 GscEntityExecutionCertificate*out,int*admitted,char*error,size_t cap,int(*current)(void*),void*context){
 GscSession*s=user;SessionScriptedPrefixLease l;GscActorScriptedPrefixRequest q;
 GscEntityExecutionCertificate certificate;int result=-1,ok;
 if(!s||!input||!out||!admitted||!current||(cap&&!error)||
    session_animation_namespace_private(s,input,sizeof(*input))||
    session_animation_namespace_private(s,out,sizeof(*out))||
    session_animation_namespace_private(s,admitted,sizeof(*admitted))||
    session_animation_namespace_private(s,error,cap)||
    session_animation_namespace_overlap(input,sizeof(*input),out,sizeof(*out))||
    session_animation_namespace_overlap(input,sizeof(*input),admitted,sizeof(*admitted))||
    session_animation_namespace_overlap(input,sizeof(*input),error,cap)||
    session_animation_namespace_overlap(out,sizeof(*out),admitted,sizeof(*admitted))||
    session_animation_namespace_overlap(out,sizeof(*out),error,cap)||
    session_animation_namespace_overlap(admitted,sizeof(*admitted),error,cap))return 0;
 memset(&l,0,sizeof(l));q=*input;l.input=&q;l.upstream=input;l.logical=q;
 session_turret_animation_lease(s,&l.base,current,context);l.host=s->r243_scripted_prefix;
 /* VM timers start at map entry. GAME owns the pose and queued AI
  * operation at the corresponding absolute server tick. */
 if(q.runtime_identity!=l.base.instance || q.now_ms!=l.base.now ||
    !r243_command_host_time(s,l.base.now,&q.now_ms))return 0;
 l.copy=q;
 if(!session_scripted_prefix_current(&l))return 0;
 memset(&certificate,0,sizeof(certificate));
 ok=l.host.invoke(l.host.user,&q,&certificate,&result,error,cap,session_scripted_prefix_current,&l);
 if(ok!=1||!session_scripted_prefix_current(&l)||memcmp(input,&l.logical,sizeof(l.logical)))return 0;
 *out=certificate;*admitted=result;return 1;
}
