/* Exact common transaction entry. No parent-active permission survives the
 * first claim, and no new callback occurs after Core's commit fence. */
static int ActorControllerLookSourceLocalScope(C1ControllerAnimationOperation*o){
 return o==&cod1ControllerAnimation&&o->lookSourceActive&&!o->cancelled&&ActorControllerAnimationScope(o)&&
  o->lookSourceBorrowed&&o->lookSourceUser&&o->lookSourceCurrent&&
  !memcmp(o->lookSourceBorrowed,&o->lookSourceRequest,sizeof(o->lookSourceRequest))&&
  !memcmp(o->lookSourceUser,&o->lookSourceRequest,sizeof(o->lookSourceRequest))&&
  o->lookSourceReceipt.dispatch&&o->lookSourceReceipt.task_serial&&
  o->lookSourceReceipt.runtime_identity==o->q.runtime_identity&&o->lookSourceReceipt.catalog_identity==o->q.catalog_identity&&
  o->lookSourceReceipt.frame_generation==o->q.frame_generation&&ActorRefEqual(&o->lookSourceReceipt.entity,&o->q.entity);
}
static int ActorControllerLookSourceScope(C1ControllerAnimationOperation*o){
 GscActorSourceLookReceipt receipt;GscActorLookRequest request;
 const void*borrowed,*user;int(*current)(void*);void*context;int ok;
 if(!ActorControllerLookSourceLocalScope(o)||!o->api.sourceLook.current)return 0;
 receipt=o->lookSourceReceipt;request=o->lookSourceRequest;borrowed=o->lookSourceBorrowed;user=o->lookSourceUser;
 current=o->lookSourceCurrent;context=o->lookSourceContext;
 ok=o->api.sourceLook.current(o->api.sourceLook.context,&o->lookSourceReceipt,borrowed,current,context)==1;
 return ok&&ActorControllerLookSourceLocalScope(o)&&
  !memcmp(&o->lookSourceReceipt,&receipt,sizeof(receipt))&&!memcmp(&o->lookSourceRequest,&request,sizeof(request))&&
  o->lookSourceBorrowed==borrowed&&o->lookSourceUser==user&&o->lookSourceCurrent==current&&o->lookSourceContext==context;
}
/* A refused entry must not retain pointers into the caller's stack,
 * including refusal after a consumed claim. Cancellation stays sticky. */
static int ActorControllerLookSourceEntryFailure(void){
 C1ControllerAnimationOperation*o=&cod1ControllerAnimation;
 ActorControllerAnimationDeny();o->lookSourceActive=0;
 o->lookSourceBorrowed=o->lookSourceUser=NULL;o->lookSourceCurrent=NULL;o->lookSourceContext=NULL;
 memset(&o->lookSourceReceipt,0,sizeof(o->lookSourceReceipt));return -1;
}
static int ActorControllerLookSourceEnter(const GscEntityRef*ref,uint64_t runtime,uint64_t catalog,int now,
 const void*borrowed,size_t bytes,int(*current)(void*),void*context,const void*user){
 C1ControllerAnimationOperation*o=&cod1ControllerAnimation;GscActorLookRequest q;int ok;
 if(!o->busy)return 0;
 if(cod1ControllerOperation.busy||o->cancelled||o->checking||o->sourceChecking||o->sourceActive||o->lookSourceChecking||o->lookSourceActive||!o->vm_running||
    !ref||!borrowed||!user||!current||bytes!=sizeof(q)||
    !ActorGoalBuffers(borrowed,bytes,NULL,0,NULL,0)||!ActorGoalBuffers(user,sizeof(q),NULL,0,NULL,0)||
    !o->api.sourceLook.claim||!o->api.sourceLook.current){return ActorControllerLookSourceEntryFailure();}
 memcpy(&q,borrowed,sizeof(q));
 if(memcmp(user,&q,sizeof(q))||!ActorRefEqual(ref,&q.entity)||!ActorRefEqual(ref,&o->q.entity)||
    q.runtime_identity!=runtime||q.catalog_identity!=catalog||q.now_ms!=now||
    runtime!=o->q.runtime_identity||catalog!=o->q.catalog_identity||now!=o->now||
    q.method!=GSC_ACTOR_LOOK_AIM_AT_POS){
  return ActorControllerLookSourceEntryFailure();}
 o->lookSourceRequest=q;o->lookSourceBorrowed=borrowed;o->lookSourceUser=user;o->lookSourceCurrent=current;o->lookSourceContext=context;
 o->checking=1;ok=o->api.sourceLook.claim(o->api.sourceLook.context,borrowed,current,context,&o->lookSourceReceipt)==1;o->checking=0;
 if(!ok||o->cancelled||memcmp(borrowed,&q,sizeof(q))||memcmp(user,&q,sizeof(q))||!ActorControllerAnimationScope(o)){
  return ActorControllerLookSourceEntryFailure();}
 o->lookSourceActive=1;o->lookSourceChecking=1;ok=ActorControllerLookSourceScope(o);o->lookSourceChecking=0;
 if(!ok){return ActorControllerLookSourceEntryFailure();}
 if(g_debugMove.integer>1)G_Printf("[402 source look] owner=%u tick=%d method=%d dispatch=%llu claimed=1\n",q.entity.slot,now,q.method,(unsigned long long)o->lookSourceReceipt.dispatch);
 return 1;
}
static int ActorControllerLookSourceCurrent(void*context){
 C1ControllerAnimationOperation*o=context;int ok;
 if(o!=&cod1ControllerAnimation||!o->lookSourceActive||o->lookSourceChecking){ActorControllerAnimationDeny();return 0;}
 o->lookSourceChecking=1;
 ok=ActorControllerLookSourceScope(o)&&o->lookSourceCurrent(o->lookSourceContext)==1&&ActorControllerLookSourceScope(o);
 o->lookSourceChecking=0;if(!ok)ActorControllerAnimationDeny();return ok;
}
static void*ActorControllerLookSourceContext(void){return &cod1ControllerAnimation;}
static int ActorControllerLookSourceFinish(int owned,int accepted){
 C1ControllerAnimationOperation*o=&cod1ControllerAnimation;int ok;
 if(!owned)return accepted;
 /* Broker equality is pure: no external current/resources after commit. */
 o->lookSourceChecking=1;ok=accepted&&ActorControllerLookSourceScope(o);o->lookSourceChecking=0;
 o->lookSourceActive=0;o->lookSourceBorrowed=o->lookSourceUser=NULL;o->lookSourceCurrent=NULL;o->lookSourceContext=NULL;
 memset(&o->lookSourceReceipt,0,sizeof(o->lookSourceReceipt));
 if(!ok)ActorControllerAnimationDeny();return ok;
}
static int ActorControllerLookSourcePure(void){return ActorControllerLookSourceScope(&cod1ControllerAnimation);}
static int ActorControllerLookSourceMatch(const void*q,size_t n,int(*f)(void*),void*x,const GscActorLookRequest*copy){
 C1ControllerAnimationOperation*o=&cod1ControllerAnimation;
 return n==sizeof(*copy)&&q==o->lookSourceBorrowed&&f==ActorControllerLookSourceCurrent&&x==o&&copy&&
  !memcmp(copy,&o->lookSourceRequest,sizeof(*copy))&&ActorControllerLookSourceScope(o);
}
