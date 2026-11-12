/* Exact common transaction entry. No parent-active permission survives the
 * first claim, and no new callback occurs after Core's commit fence. */
static int ActorControllerSourceLocalScope(C1ControllerAnimationOperation*o){
 return o==&cod1ControllerAnimation&&o->sourceActive&&!o->cancelled&&ActorControllerAnimationScope(o)&&
  o->sourceBorrowed&&o->sourceUser&&o->sourceCurrent&&
  !memcmp(o->sourceBorrowed,&o->sourceRequest,sizeof(o->sourceRequest))&&
  !memcmp(o->sourceUser,&o->sourceRequest,sizeof(o->sourceRequest))&&
  o->sourceReceipt.dispatch&&o->sourceReceipt.task_serial&&
  o->sourceReceipt.runtime_identity==o->q.runtime_identity&&o->sourceReceipt.catalog_identity==o->q.catalog_identity&&
  o->sourceReceipt.frame_generation==o->q.frame_generation&&ActorRefEqual(&o->sourceReceipt.entity,&o->q.entity);
}
static int ActorControllerSourceScope(C1ControllerAnimationOperation*o){
 GscActorSourceAnimationReceipt receipt;GscActorAnimationRequest request;
 const void*borrowed,*user;int(*current)(void*);void*context;int ok;
 if(!ActorControllerSourceLocalScope(o)||!o->api.source.current)return 0;
 receipt=o->sourceReceipt;request=o->sourceRequest;borrowed=o->sourceBorrowed;user=o->sourceUser;
 current=o->sourceCurrent;context=o->sourceContext;
 ok=o->api.source.current(o->api.source.context,&o->sourceReceipt,borrowed,current,context)==1;
 return ok&&ActorControllerSourceLocalScope(o)&&
  !memcmp(&o->sourceReceipt,&receipt,sizeof(receipt))&&!memcmp(&o->sourceRequest,&request,sizeof(request))&&
  o->sourceBorrowed==borrowed&&o->sourceUser==user&&o->sourceCurrent==current&&o->sourceContext==context;
}
static int ActorControllerGenericSourceEnter(const GscEntityRef*ref,uint64_t runtime,uint64_t catalog,int now,
 const void*borrowed,size_t bytes,int(*current)(void*),void*context,const void*user,int kind){
 C1ControllerAnimationOperation*o=&cod1ControllerAnimation;GscActorAnimationRequest q;int ok;
 if(!o->busy)return 0;
 if(cod1ControllerOperation.busy||o->cancelled||o->checking||o->sourceChecking||o->sourceActive||!o->vm_running||
    !ref||!borrowed||!user||!current||bytes!=sizeof(q)||!kind||
    !ActorGoalBuffers(borrowed,bytes,NULL,0,NULL,0)||!ActorGoalBuffers(user,sizeof(q),NULL,0,NULL,0)||
    !o->api.source.claim||!o->api.source.current){ActorControllerAnimationDeny();return -1;}
 memcpy(&q,borrowed,sizeof(q));
 if(memcmp(user,&q,sizeof(q))||!ActorRefEqual(ref,&q.entity)||!ActorRefEqual(ref,&o->q.entity)||
    q.runtime_identity!=runtime||q.catalog_identity!=catalog||q.now_ms!=now||
    runtime!=o->q.runtime_identity||catalog!=o->q.catalog_identity||now!=o->now||
    (kind==1&&(q.method!=GSC_ACTOR_ANIMATION_SET&&q.method!=GSC_ACTOR_ANIMATION_CLEAR&&
               q.method!=GSC_ACTOR_ANIMATION_KNOB_ALL&&q.method!=GSC_ACTOR_ANIMATION_FLAGGED_ALL_RESTART))||
    (kind==2&&q.method!=GSC_ACTOR_ANIMATION_FLAGGED_RESTART)||
    (kind==3&&q.method!=GSC_ACTOR_ANIMATION_SET&&q.method!=GSC_ACTOR_ANIMATION_FLAGGED_RESTART)||kind>3){
  ActorControllerAnimationDeny();return -1;}
 o->sourceRequest=q;o->sourceBorrowed=borrowed;o->sourceUser=user;o->sourceCurrent=current;o->sourceContext=context;
 o->checking=1;ok=o->api.source.claim(o->api.source.context,borrowed,current,context,&o->sourceReceipt)==1;o->checking=0;
 if(!ok||o->cancelled||memcmp(borrowed,&q,sizeof(q))||memcmp(user,&q,sizeof(q))||!ActorControllerAnimationScope(o)){
  ActorControllerAnimationDeny();return -1;}
 o->sourceActive=1;o->sourceChecking=1;ok=ActorControllerSourceScope(o);o->sourceChecking=0;
 if(!ok){ActorControllerAnimationDeny();return -1;}return 1;
}
static int ActorControllerGenericSourceCurrent(void*context){
 C1ControllerAnimationOperation*o=context;int ok;
 if(o!=&cod1ControllerAnimation||!o->sourceActive||o->sourceChecking){ActorControllerAnimationDeny();return 0;}
 o->sourceChecking=1;
 ok=ActorControllerSourceScope(o)&&o->sourceCurrent(o->sourceContext)==1&&ActorControllerSourceScope(o);
 o->sourceChecking=0;if(!ok)ActorControllerAnimationDeny();return ok;
}
static void*ActorControllerGenericSourceContext(void){return &cod1ControllerAnimation;}
static int ActorControllerGenericSourceFinish(int owned,int accepted){
 C1ControllerAnimationOperation*o=&cod1ControllerAnimation;int ok;
 if(!owned)return accepted;
 /* Broker equality is pure: no external current/resources after commit. */
 o->sourceChecking=1;ok=accepted&&ActorControllerSourceScope(o);o->sourceChecking=0;
 o->sourceActive=0;o->sourceBorrowed=o->sourceUser=NULL;o->sourceCurrent=NULL;o->sourceContext=NULL;
 memset(&o->sourceReceipt,0,sizeof(o->sourceReceipt));
 if(!ok)ActorControllerAnimationDeny();return ok;
}
