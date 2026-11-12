typedef struct {
 GscSession*s;GscRuntime*r;const GscProgram*g;uint64_t id;unsigned profile_mask;
 R243ActorFloatRequest q;const R243ActorFloatRequest*source;
 GscMapExecutionCertificate map;R243ActorFloatHost provider;
 int(*current)(void*);void*context;
} R243ActorFloatSessionIdentity;
typedef struct {R243ActorFloatSessionIdentity id,seal;R243ActorFloatReply reply;char error[384];} R243ActorFloatSessionFrame;
static R243ActorFloatSessionFrame*r243ActorFloatSessionActive;
typedef struct {GscSession*s;GscRuntime*r;const GscProgram*g;uint64_t id;int logical,origin;unsigned mask;R243ActorFloatHost provider;GscMapExecutionCertificate map;} R243ActorFloatValidationIdentity;
typedef struct {R243ActorFloatValidationIdentity id,seal;} R243ActorFloatValidationFrame;
static R243ActorFloatValidationFrame*r243ActorFloatValidationActive;
static R243ActorFloatValidationFrame*r243ActorFloatBindingActive;
static int r243_actor_float_session_private(const void*p,size_t n){
 return session_animation_namespace_overlap(p,n,&r243ActorFloatSessionActive,sizeof(r243ActorFloatSessionActive))||
  session_animation_namespace_overlap(p,n,&r243ActorFloatValidationActive,sizeof(r243ActorFloatValidationActive))||
  session_animation_namespace_overlap(p,n,&r243ActorFloatBindingActive,sizeof(r243ActorFloatBindingActive))||
  (r243ActorFloatBindingActive&&session_animation_namespace_overlap(p,n,r243ActorFloatBindingActive,sizeof(*r243ActorFloatBindingActive)))||
  (r243ActorFloatSessionActive&&session_animation_namespace_overlap(p,n,r243ActorFloatSessionActive,sizeof(*r243ActorFloatSessionActive)))||
  (r243ActorFloatValidationActive&&session_animation_namespace_overlap(p,n,r243ActorFloatValidationActive,sizeof(*r243ActorFloatValidationActive)));
}
static int r243_actor_float_validation_scope(R243ActorFloatValidationFrame*f){const R243ActorFloatValidationIdentity*i;GscSession*s;
 if(!f||r243ActorFloatValidationActive!=f||memcmp(&f->id,&f->seal,sizeof(f->id)))return 0;i=&f->seal;s=i->s;
 return s&&s->actor_float_query_enabled&&s->runtime==i->r&&s->program==i->g&&s->view.session_id==i->id&&
  s->view.logical_ms==i->logical&&s->view.origin_host_ms==i->origin&&s->actor_float_profile_mask==i->mask&&
  !memcmp(&s->actor_float_provider,&i->provider,sizeof(i->provider))&&!memcmp(&s->animation_namespace_map,&i->map,sizeof(i->map))&&
  R243ActorFloatRuntimeMaskCurrent(i->r,i->mask)&&s->busy&&!s->abort_requested&&s->view.state==GSC_SESSION_RUNNING&&!s->view.error[0];
}
static int r243_actor_float_session_scope(R243ActorFloatSessionFrame*f){
 const R243ActorFloatSessionIdentity*i;GscSession*s;
 if(!f||r243ActorFloatSessionActive!=f||memcmp(&f->id,&f->seal,sizeof(f->id)))return 0;i=&f->seal;s=i->s;
 return s&&s->actor_float_query_enabled&&s->actor_float_profile_mask==i->profile_mask&&
  (i->profile_mask&R243ActorFloatMask(i->q.method))&&s->busy&&!s->abort_requested&&!s->view.error[0]&&s->view.state==GSC_SESSION_RUNNING&&
  s->runtime==i->r&&s->program==i->g&&R243ActorFloatRuntimeMaskCurrent(i->r,i->profile_mask)&&s->view.session_id==i->id&&i->id&&s->view.logical_ms==i->q.now&&
  i->q.runtime==gsc_runtime_instance_identity(i->r)&&!memcmp(&s->animation_namespace_map,&i->map,sizeof(i->map))&&
  !memcmp(&s->actor_float_provider,&i->provider,sizeof(i->provider))&&!memcmp(i->source,&i->q,sizeof(i->q));
}
static int r243_actor_float_session_current(R243ActorFloatSessionFrame*f){
 return r243_actor_float_session_scope(f)&&R243ActorFloatExactCallCurrent(&f->seal.q,f->seal.current,f->seal.context)&&
  r243_actor_float_session_scope(f)&&session_animation_resources_current(f->seal.s)&&r243_actor_float_session_scope(f);
}
int R243ActorFloatSessionBuffersCurrent(const R243ActorFloatRequest*q,const R243ActorFloatReply*out,const char*error,size_t cap,int(*current)(void*),void*context){
 R243ActorFloatSessionFrame*f=r243ActorFloatSessionActive;
 return f&&q==&f->id.q&&out==&f->reply&&error==f->error&&cap==sizeof(f->error)&&
  f->seal.current==current&&f->seal.context==context&&r243_actor_float_session_current(f);
}
static int r243_actor_float_session_validate(void*user,const GscEntityRef*entity){
 GscSession*s=user;R243ActorFloatValidationFrame f;int ok=0;
 if(r243ActorFloatValidationActive||!s||!s->actor_float_query_enabled||!s->runtime||!s->busy||s->abort_requested||s->view.state!=GSC_SESSION_RUNNING||s->view.error[0])return 0;
 memset(&f,0,sizeof(f));f.id.s=s;f.id.r=s->runtime;f.id.g=s->program;f.id.id=s->view.session_id;f.id.logical=s->view.logical_ms;f.id.origin=s->view.origin_host_ms;f.id.mask=s->actor_float_profile_mask;f.id.provider=s->actor_float_provider;f.id.map=s->animation_namespace_map;f.seal=f.id;r243ActorFloatValidationActive=&f;
 ok=f.seal.provider.validate&&r243_actor_float_validation_scope(&f)&&session_animation_resources_current(s)&&r243_actor_float_validation_scope(&f)&&
  f.seal.provider.validate(f.seal.provider.user,entity)==1&&r243_actor_float_validation_scope(&f)&&session_animation_resources_current(s)&&r243_actor_float_validation_scope(&f);
 r243ActorFloatValidationActive=NULL;return ok;
}
static int r243_actor_float_session_query(void*user,const R243ActorFloatRequest*input,R243ActorFloatReply*out,char*error,size_t cap,int(*current)(void*),void*context){
 GscSession*s=user;R243ActorFloatSessionFrame f;int accepted=0,ok=0;
 if(!s||!input||!out||!current||r243ActorFloatSessionActive||!s->actor_float_query_enabled)return 0;
 memset(&f,0,sizeof(f));f.id.s=s;f.id.r=s->runtime;f.id.g=s->program;f.id.id=s->view.session_id;f.id.profile_mask=s->actor_float_profile_mask;f.id.q=*input;f.id.source=input;
 f.id.map=s->animation_namespace_map;f.id.provider=s->actor_float_provider;f.id.current=current;f.id.context=context;
 f.seal=f.id;f.reply.bits=0x7fc00000u;r243ActorFloatSessionActive=&f;
 if(!r243_actor_float_session_current(&f))goto done;
 /* Preserve the genuine VM Current and copied intent through the pure GAME
  * read. Session's own sealed context is checked before and after publication. */
 accepted=f.seal.provider.query(f.seal.provider.user,&f.id.q,&f.reply,f.error,sizeof(f.error),current,context);f.error[sizeof(f.error)-1]=0;
 if(!r243_actor_float_session_current(&f)||accepted!=1||((f.reply.bits&0x80000000u)||(f.reply.bits&0x7f800000u)==0x7f800000u))goto done;ok=1;
done:
 r243ActorFloatSessionActive=NULL;
 if(!ok){if(error&&cap)snprintf(error,cap,"actor float Session query refused: %s",f.error[0]?f.error:"caller/map/context retired");return 0;}
 *out=f.reply;return 1;
}
int gsc_session_select_r243_actor_float_methods(GscSession*s,const R243ActorFloatHost*input,unsigned methods,char*error,size_t cap){
 R243ActorFloatHost copy;
 if(!s||!input||!methods||(methods&~3u)||(cap&&!error)||session_animation_namespace_private(s,input,sizeof(*input))||session_animation_namespace_private(s,error,cap)||session_animation_namespace_overlap(input,sizeof(*input),error,cap))return 0;
 copy=*input;if(!copy.validate||!copy.query||!session_enter(s,error,cap))return 0;
 if(s->view.state!=GSC_SESSION_EMPTY||s->program||s->runtime||!s->animation_namespace_enabled||!s->execution_extensions||!s->actor_program_enabled||((methods&2u)&&!s->ai_arrays_enabled)||s->actor_float_query_enabled||!session_animation_resources_current(s)||!session_actor_program_current(s))return session_leave_rejected(s,"N0087 needs EMPTY declared Actor/AI/namespace Session",error,cap);
 s->actor_float_provider=copy;s->actor_float_profile_mask=methods;s->actor_float_query_enabled=1;s->busy=0;return 1;
}
int gsc_session_select_r243_actor_float(GscSession*s,const R243ActorFloatHost*host,char*error,size_t cap){return gsc_session_select_r243_actor_float_methods(s,host,2u,error,cap);}
static int r243_actor_float_program_select(GscSession*s){return !s->actor_float_query_enabled||R243ProgramSelectActorFloat(s->program,s->actor_float_profile_mask);}
int R243ActorFloatSessionHostTimeCurrent(const R243ActorFloatRequest*q,int hostNow){
 R243ActorFloatSessionFrame*f=r243ActorFloatSessionActive;GscSession*s;
 if(!f||!q||memcmp(q,&f->seal.q,sizeof(*q))||!r243_actor_float_session_current(f))return 0;s=f->seal.s;
 return hostNow>=0&&s->view.origin_host_ms>=0&&f->seal.q.now>=0&&s->view.origin_host_ms<=INT_MAX-f->seal.q.now&&hostNow==s->view.origin_host_ms+f->seal.q.now&&r243_actor_float_session_current(f);
}
static int r243_actor_float_binding_scope(R243ActorFloatValidationFrame*f){const R243ActorFloatValidationIdentity*i;GscSession*s;
 if(!f||r243ActorFloatBindingActive!=f||memcmp(&f->id,&f->seal,sizeof(f->id)))return 0;i=&f->seal;s=i->s;
 return s&&s->actor_float_query_enabled&&s->runtime==i->r&&s->program==i->g&&s->view.session_id==i->id&&
  s->view.logical_ms==i->logical&&s->view.origin_host_ms==i->origin&&s->actor_float_profile_mask==i->mask&&i->mask&&!(i->mask&~3u)&&
  !memcmp(&s->actor_float_provider,&i->provider,sizeof(i->provider))&&!memcmp(&s->animation_namespace_map,&i->map,sizeof(i->map))&&
  s->busy&&!s->abort_requested&&!s->view.error[0];
}
static int r243_actor_float_runtime_bind(GscSession*s){
 R243ActorFloatHost host;R243ActorFloatValidationFrame f;int ok;
 if(!s->actor_float_query_enabled)return 1;if(r243ActorFloatBindingActive)return 0;
 memset(&f,0,sizeof(f));f.id.s=s;f.id.r=s->runtime;f.id.g=s->program;f.id.id=s->view.session_id;f.id.logical=s->view.logical_ms;f.id.origin=s->view.origin_host_ms;f.id.mask=s->actor_float_profile_mask;f.id.provider=s->actor_float_provider;f.id.map=s->animation_namespace_map;f.seal=f.id;r243ActorFloatBindingActive=&f;
 memset(&host,0,sizeof(host));host.user=s;host.validate=r243_actor_float_session_validate;host.query=r243_actor_float_session_query;
 ok=r243_actor_float_binding_scope(&f)&&session_animation_resources_current(s)&&r243_actor_float_binding_scope(&f)&&
  R243RuntimeSelectActorFloat(f.seal.r,&host)&&r243_actor_float_binding_scope(&f)&&session_animation_resources_current(s)&&r243_actor_float_binding_scope(&f)&&R243ActorFloatRuntimeMaskCurrent(f.seal.r,f.seal.mask);
 r243ActorFloatBindingActive=NULL;if(!ok)return 0;
 puts("R243 ACTOR_FLOAT_SESSION_BOUND selectedZeroArgTypedFloatQueries1 nonemptyPathAndObservationMatrixStillHOLD1 noMAINSourceEntry1");return 1;
}
