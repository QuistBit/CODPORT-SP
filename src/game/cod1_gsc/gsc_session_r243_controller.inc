/* EMPTY-only binding; runtime pointer/task Values never enter GAME. */
int gsc_session_select_r243_scripted_controller(GscSession*s,const GscActorScriptedControllerHost*h,char*error,size_t cap){
 return gsc_session_select_r243_controller_with_cleanup(s,h,NULL,error,cap);
}
int gsc_session_select_r243_controller_with_cleanup(GscSession*s,const GscActorScriptedControllerHost*input,GscControllerSelectWithCleanup select,char*error,size_t cap){
 GscActorScriptedControllerHost h;
 if(!s||!input||(cap&&!error)||session_animation_namespace_private(s,input,sizeof(*input))||
    session_animation_namespace_private(s,error,cap)||session_animation_namespace_overlap(input,sizeof(*input),error,cap))return 0;
 h=*input;if(!session_enter(s,error,cap))return 0;
 if(s->view.state!=GSC_SESSION_EMPTY||s->runtime||s->program||s->r243_controller_enabled||
    !s->r243_scripted_start_enabled||!s->r243_thread_entities_enabled||!s->r237_frame_buckets_enabled||
    !h.init_return||!h.select_main||!h.animation_step||!h.normal_update)
  return session_leave_rejected(s,"scripted controller requires fresh selected prefix/start/thread/frame profile",error,cap);
 if(!session_r237_frame_resources_current(s)||memcmp(input,&h,sizeof(h)))
  return session_leave_rejected(s,"controller binding resources retired",error,cap);
 s->r243_controller=h;s->r243_select_with_cleanup=select;s->r243_controller_enabled=1;s->busy=0;return 1;
}
typedef union {GscActorInitReturnRequest init;GscActorControllerRequest step;} SessionControllerForwardedQ;
typedef struct {
 SessionTurretAnimationLease base;GscActorScriptedControllerHost host;GscControllerSelectWithCleanup select;GscMapExecutionCertificate bootstrapMap;
 const void *originalQ,*forwardedQ;size_t qBytes;SessionControllerForwardedQ expectedQ,originalCopy;int hostNow,checking;
 const GscActorControllerAnimationServices *originalApi,*forwardedApi;
 GscActorControllerAnimationServices expectedOriginalApi,expectedApi;
} SessionControllerLease;
static int session_controller_forward(SessionControllerLease*l,const void*original,const void*forwarded,
 size_t bytes,const GscActorControllerAnimationServices*originalApi,const GscActorControllerAnimationServices*forwardedApi){
 if(!l||!original||!forwarded||(bytes!=sizeof(GscActorInitReturnRequest)&&bytes!=sizeof(GscActorControllerRequest))||
    ((originalApi!=NULL)!=(forwardedApi!=NULL)))return 0;
 l->originalQ=original;l->forwardedQ=forwarded;l->qBytes=bytes;
 memcpy(&l->originalCopy,original,bytes);
 memcpy(&l->expectedQ,forwarded,bytes);
 if(originalApi){l->originalApi=originalApi;l->forwardedApi=forwardedApi;l->expectedOriginalApi=*originalApi;l->expectedApi=*forwardedApi;}
 return 1;
}
static int session_controller_scope(SessionControllerLease*l){
 int ok,hostNow;if(!l)return 0;
 ok=r243_command_host_time(l->base.session,l->base.now,&hostNow)&&hostNow==l->hostNow&&
  session_turret_animation_scope(&l->base)&&l->base.session->r243_controller_enabled&&
  l->originalQ&&l->forwardedQ&&(l->qBytes==sizeof(GscActorInitReturnRequest)||l->qBytes==sizeof(GscActorControllerRequest))&&
  !memcmp(l->originalQ,&l->originalCopy,l->qBytes)&&!memcmp(l->forwardedQ,&l->expectedQ,l->qBytes)&&
  ((l->originalApi==NULL)==(l->forwardedApi==NULL))&&
  (!l->forwardedApi||(!memcmp(l->originalApi,&l->expectedOriginalApi,sizeof(l->expectedOriginalApi))&&
                     !memcmp(l->forwardedApi,&l->expectedApi,sizeof(l->expectedApi))))&&
  !memcmp(&l->base.session->actor_bootstrap.map_certificate,&l->bootstrapMap,sizeof(l->bootstrapMap))&&
  l->select==l->base.session->r243_select_with_cleanup&&!memcmp(&l->base.session->r243_controller,&l->host,sizeof(l->host));
 if(!ok)l->base.cancelled=1;return ok;
}
static int session_controller_current(void*context){
 SessionControllerLease*l=context;int ok;
 if(!l)return 0;
 if(l->checking){l->base.cancelled=1;return 0;}
 if(!session_controller_scope(l))return 0;
 l->checking=1;
 ok=session_r237_frame_resources_current(l->base.session)&&session_controller_scope(l)&&
    session_turret_animation_current(&l->base)&&session_controller_scope(l);
 l->checking=0;if(!ok)l->base.cancelled=1;return ok;
}
static void session_controller_lease(GscSession*s,SessionControllerLease*l,int(*current)(void*),void*context){
 memset(l,0,sizeof(*l));session_turret_animation_lease(s,&l->base,current,context);l->host=s->r243_controller;l->select=s->r243_select_with_cleanup;l->bootstrapMap=s->actor_bootstrap.map_certificate;
}
/* Animation callbacks run at GAME time. Timers and ownership checks
 * in the VM retain their logical session tick throughout the callback. */
static int session_controller_host_tick(SessionControllerLease *l,uint64_t runtime,int *tick){
 if(runtime!=l->base.instance || *tick!=l->base.now ||
    !r243_command_host_time(l->base.session,l->base.now,&l->hostNow))return 0;
 *tick=l->hostNow;
 return 1;
}
static int session_controller_buffers_bound(GscSession*s,const void*input,size_t in,const void*out,size_t on,char*error,size_t cap,
 int(*current)(void*),void*context){
 return s&&input&&(!on||out)&&(!cap||error)&&!session_animation_namespace_private(s,input,in)&&
  !session_animation_namespace_private(s,out,on)&&!session_animation_namespace_private(s,error,cap)&&
  !session_animation_namespace_overlap(input,in,out,on)&&!session_animation_namespace_overlap(input,in,error,cap)&&
  !session_animation_namespace_overlap(out,on,error,cap)&&(!s->runtime||
   ((gsc_runtime_r243_controller_storage_separate(s->runtime,input,in)||
     gsc_runtime_r243_controller_request_input(s->runtime,input,in,current,context))&&
    gsc_runtime_r243_controller_storage_separate(s->runtime,out,on)&&gsc_runtime_r243_controller_storage_separate(s->runtime,error,cap)));
}
static int session_controller_buffers(GscSession*s,const void*input,size_t in,const void*out,size_t on,char*error,size_t cap){
 return session_controller_buffers_bound(s,input,in,out,on,error,cap,NULL,NULL);
}
static int session_controller_init(void*user,const GscActorInitReturnRequest*input,char*error,size_t cap,int(*current)(void*),void*context){
 GscSession*s=user;SessionControllerLease l;GscActorInitReturnRequest q,original;int ok;
 if(!current||!session_controller_buffers(s,input,sizeof(*input),NULL,0,error,cap))return 0;
 q=original=*input;session_controller_lease(s,&l,current,context);
 if(!session_controller_host_tick(&l,q.runtime_identity,&q.now_ms))return 0;
 if(!session_controller_forward(&l,input,&q,sizeof(q),NULL,NULL))return 0;
 if(!session_controller_current(&l))return 0;
 ok=l.host.init_return(l.host.user,&q,error,cap,session_controller_current,&l);
 return ok==1&&session_controller_current(&l)&&!memcmp(&q,&l.expectedQ,sizeof(q))&&!memcmp(input,&original,sizeof(q));
}
typedef struct {
 SessionControllerLease *lease;
 const GscMainReplacementService *source,*offer;
 GscMainReplacementService seal,offerSeal;
 int active,next,busy,failed;
} SessionMainReplacement;
static int session_replace_main(void*ctx,int phase,uint64_t selection,int(*fence)(void*),void*game){
 SessionMainReplacement*x=ctx;int ok;
 if(!x)return 0;
 if(!x->active||x->busy||x->failed||!x->source||phase!=x->next||
    phase<0||phase>1||!fence||memcmp(x->source,&x->seal,sizeof(x->seal))||
    memcmp(x->offer,&x->offerSeal,sizeof(x->offerSeal))){x->failed=1;return 0;}
 x->busy=1;
 ok=session_controller_current(x->lease)&&!x->failed&&x->seal.run(x->seal.context,phase,selection,fence,game)&&
    !x->failed&&session_controller_current(x->lease)&&!x->failed&&
    !memcmp(x->source,&x->seal,sizeof(x->seal))&&!memcmp(x->offer,&x->offerSeal,sizeof(x->offerSeal));
 x->busy=0;if(!ok)x->failed=1;else ++x->next;return ok;
}

static int session_controller_select_with_cleanup(void*user,const GscActorControllerRequest*input,GscActorControllerSelection*out,
 char*error,size_t cap,int(*current)(void*),void*context,const GscMainReplacementService*service){
 GscSession*s=user;SessionControllerLease l;GscActorControllerRequest q,original;GscActorControllerSelection result;int ok;
 SessionMainReplacement replacement={0};GscMainReplacementService offer={0};
 if(!current||!session_controller_buffers_bound(s,input,sizeof(*input),out,sizeof(*out),error,cap,current,context))return 0;
 q=original=*input;memset(&result,0,sizeof(result));session_controller_lease(s,&l,current,context);
 if(!session_controller_host_tick(&l,q.runtime_identity,&q.now_ms))return 0;
 if(!session_controller_forward(&l,input,&q,sizeof(q),NULL,NULL))return 0;
 if(!session_controller_current(&l))return 0;
 if(service){
  if(!service->run||!session_controller_buffers(s,service,sizeof(*service),out,sizeof(*out),error,cap))return 0;
  replacement.lease=&l;replacement.source=service;replacement.seal=*service;replacement.offer=&offer;
  offer.context=&replacement;offer.run=session_replace_main;replacement.offerSeal=offer;replacement.active=1;
 }
 ok=l.select?l.select(l.host.user,&q,&result,error,cap,session_controller_current,&l,service?&offer:NULL):
    l.host.select_main(l.host.user,&q,&result,error,cap,session_controller_current,&l);
 replacement.active=0;if(replacement.failed||replacement.busy)return 0;
 if(ok!=1||!session_controller_current(&l)||memcmp(&q,&l.expectedQ,sizeof(q))||memcmp(input,&original,sizeof(q)))return 0;
 *out=result;return 1;
}
static int session_controller_select(void*u,const GscActorControllerRequest*q,GscActorControllerSelection*out,char*error,size_t cap,int(*current)(void*),void*ctx){
 return session_controller_select_with_cleanup(u,q,out,error,cap,current,ctx,NULL);
}
static int session_controller_step(void*user,const GscActorControllerRequest*input,const GscActorControllerAnimationServices*services,
 char*error,size_t cap,int(*current)(void*),void*context){
 GscSession*s=user;SessionControllerLease l;GscActorControllerRequest q,original;GscActorControllerAnimationServices api,sealed;int ok;
 if(!current||!services||!session_controller_buffers_bound(s,input,sizeof(*input),NULL,0,error,cap,current,context)||
    !session_controller_buffers(s,services,sizeof(*services),NULL,0,error,cap))return 0;
 q=original=*input;api=sealed=*services;session_controller_lease(s,&l,current,context);
 api.source.context=&l;api.source.take=session_source_take_unavailable;api.source.claim=session_source_claim;api.source.current=session_source_current;
 api.sourceLook.context=&l;api.sourceLook.take=session_source_look_take_unavailable;api.sourceLook.claim=session_source_look_claim;api.sourceLook.current=session_source_look_current;
 if(!session_controller_host_tick(&l,q.runtime_identity,&q.now_ms))return 0;
 if(!session_controller_forward(&l,input,&q,sizeof(q),services,&api))return 0;
 if(!session_controller_current(&l))return 0;
 if(s->r243_source_controller||s->r243_source_active||s->r402_source_active)return 0;
 s->r243_source_controller=&l;
 ok=l.host.animation_step(l.host.user,&q,&api,error,cap,session_controller_current,&l);
 if(s->r243_source_active){l.base.cancelled=1;s->r243_source_active=NULL;}
 if(s->r402_source_active){l.base.cancelled=1;s->r402_source_active=NULL;}
 s->r243_source_controller=NULL;
 return ok==1&&session_controller_current(&l)&&!memcmp(&q,&l.expectedQ,sizeof(q))&&!memcmp(input,&original,sizeof(q))&&
  !memcmp(&api,&l.expectedApi,sizeof(api))&&!memcmp(services,&sealed,sizeof(api));
}
static int session_controller_update(void*user,const GscActorControllerRequest*input,int*outFrontier,
 char*error,size_t cap,int(*current)(void*),void*context){
 GscSession*s=user;SessionControllerLease l;GscActorControllerRequest q,original;int ok,frontier=-1;
 if(!current||!session_controller_buffers_bound(s,input,sizeof(*input),outFrontier,sizeof(*outFrontier),error,cap,current,context))return 0;
 q=original=*input;session_controller_lease(s,&l,current,context);
 if(!session_controller_host_tick(&l,q.runtime_identity,&q.now_ms))return 0;
 if(!session_controller_forward(&l,input,&q,sizeof(q),NULL,NULL))return 0;
 if(!session_controller_current(&l))return 0;
 ok=l.host.normal_update(l.host.user,&q,&frontier,error,cap,session_controller_current,&l);
 if(ok!=1||!session_controller_current(&l)||memcmp(&q,&l.expectedQ,sizeof(q))||memcmp(input,&original,sizeof(q)))return 0;
 *outFrontier=frontier;return 1;
}
int gsc_session_r243_controller_prefix(GscSession*s,uint64_t id,const GscEntityRef*input,uint64_t serial,uint64_t generation,
 GscActorControllerPrefixRun*out,char*error,size_t cap){
 GscEntityRef owner;GscActorControllerPrefixRun result;int ok;
 if(!session_controller_buffers(s,input,sizeof(*input),out,sizeof(*out),error,cap)||!session_enter(s,error,cap))return 0;
 owner=*input;
 if(!session_id_matches(s,id)||!s->r243_controller_enabled||!s->actor_bootstrap_enabled||!s->runtime||
    s->view.state!=GSC_SESSION_RUNNING||!session_r237_frame_resources_current(s))
  return session_leave_rejected(s,"scripted controller prefix Session unavailable",error,cap);
 memset(&result,0,sizeof(result));
 ok=gsc_runtime_r243_controller_prefix(s->runtime,&owner,serial,generation,&s->actor_bootstrap.map_certificate,&result);
 if(ok&&(!session_r237_frame_resources_current(s)||memcmp(input,&owner,sizeof(owner))))
  return session_failed(s,"controller prefix resources retired",error,cap);
 if(!session_after_vm(s,ok,error,cap))return 0;*out=result;return 1;
}

int gsc_session_initial_stop_main(GscSession*s,uint64_t id,const GscEntityExecutionCertificate*input,char*error,size_t cap){
 GscEntityExecutionCertificate cert;GscEntityScriptRun result={0};int ok;
 if(!session_controller_buffers(s,input,sizeof(*input),NULL,0,error,cap)||!session_enter(s,error,cap))return 0;
 cert=*input;
 if(!session_id_matches(s,id)||!s->r243_controller_enabled||!s->actor_bootstrap_enabled||
    s->view.state!=GSC_SESSION_RUNNING||!s->runtime||!s->script_roots_ready||
    !session_script_roots_cache_current(s)||!session_r237_frame_resources_current(s)||
    !cert.current||!cert.serial||!cert.current(&cert.owner,cert.serial)||memcmp(input,&cert,sizeof(cert)))
  return session_leave_rejected(s,"initial MAIN lacks live actor, map or animation root",error,cap);
 ok=gsc_runtime_initial_stop_main(s->runtime,&cert,&s->actor_bootstrap.map_certificate,&result);
 if(ok&&(!session_r237_frame_resources_current(s)||memcmp(input,&cert,sizeof(cert))))
  return session_failed(s,"initial MAIN resources retired",error,cap);
 if(!session_after_vm(s,ok,error,cap))return 0;
 return result.task_serial&&(result.status==GSC_ENTITY_ENTRY_WAITING||result.status==GSC_ENTITY_ENTRY_SLEEPING);
}

int gsc_session_locomotion_main(GscSession*s,uint64_t id,const GscEntityExecutionCertificate*input,int source,char*error,size_t cap){
 GscEntityExecutionCertificate cert;GscEntityScriptRun result={0};int ok;
 if((source<0||source>2)||!session_controller_buffers(s,input,sizeof(*input),NULL,0,error,cap)||!session_enter(s,error,cap))return 0;
 cert=*input;
 if(!session_id_matches(s,id)||!s->r243_controller_enabled||!s->actor_bootstrap_enabled||
    s->view.state!=GSC_SESSION_RUNNING||!s->runtime||!s->script_roots_ready||
    !session_script_roots_cache_current(s)||!session_r237_frame_resources_current(s)||
    !cert.current||!cert.serial||!cert.current(&cert.owner,cert.serial)||memcmp(input,&cert,sizeof(cert)))
  return session_leave_rejected(s,"locomotion MAIN lacks live actor, map or animation root",error,cap);
 ok=gsc_runtime_locomotion_main(s->runtime,&cert,&s->actor_bootstrap.map_certificate,source,&result);
 if(ok&&(!session_r237_frame_resources_current(s)||memcmp(input,&cert,sizeof(cert))))
  return session_failed(s,"locomotion MAIN resources retired",error,cap);
 if(!session_after_vm(s,ok,error,cap))return 0;
 return result.task_serial&&(result.status==GSC_ENTITY_ENTRY_WAITING||result.status==GSC_ENTITY_ENTRY_SLEEPING);
}
