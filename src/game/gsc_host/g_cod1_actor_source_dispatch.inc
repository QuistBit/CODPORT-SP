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
/* A refused entry must not retain pointers into the caller's stack,
 * including refusal after a consumed claim. Cancellation stays sticky. */
static int ActorControllerSourceEntryFailure(void){
 C1ControllerAnimationOperation*o=&cod1ControllerAnimation;
 ActorControllerAnimationDeny();o->sourceActive=0;
 o->sourceBorrowed=o->sourceUser=NULL;o->sourceCurrent=NULL;o->sourceContext=NULL;
 memset(&o->sourceReceipt,0,sizeof(o->sourceReceipt));return -1;
}
static int ActorControllerGenericSourceEnter(const GscEntityRef*ref,uint64_t runtime,uint64_t catalog,int now,
 const void*borrowed,size_t bytes,int(*current)(void*),void*context,const void*user,int kind){
 C1ControllerAnimationOperation*o=&cod1ControllerAnimation;GscActorAnimationRequest q;int ok;
 if(!o->busy)return 0;
 if(cod1ControllerOperation.busy||o->cancelled||o->checking||o->lookSourceChecking||o->lookSourceActive||o->sourceChecking||o->sourceActive||!o->vm_running||
    !ref||!borrowed||!user||!current||bytes!=sizeof(q)||!kind||
    !ActorGoalBuffers(borrowed,bytes,NULL,0,NULL,0)||!ActorGoalBuffers(user,sizeof(q),NULL,0,NULL,0)||
    !o->api.source.claim||!o->api.source.current){return ActorControllerSourceEntryFailure();}
 memcpy(&q,borrowed,sizeof(q));
 if(memcmp(user,&q,sizeof(q))||!ActorRefEqual(ref,&q.entity)||!ActorRefEqual(ref,&o->q.entity)||
    q.runtime_identity!=runtime||q.catalog_identity!=catalog||q.now_ms!=now||
    runtime!=o->q.runtime_identity||catalog!=o->q.catalog_identity||now!=o->now||
    (kind==1&&(q.method!=GSC_ACTOR_ANIMATION_SET&&q.method!=GSC_ACTOR_ANIMATION_CLEAR&&q.method!=GSC_ACTOR_ANIMATION_KNOB&&
               q.method!=GSC_ACTOR_ANIMATION_KNOB_ALL&&q.method!=GSC_ACTOR_ANIMATION_KNOB_ALL_RESTART&&q.method!=GSC_ACTOR_ANIMATION_FLAGGED_ALL_RESTART&&
               !(q.method==GSC_ACTOR_ANIMATION_FLAGGED_RESTART&&ActorAimFaceLeaf(q.animation[0].ordinal))))||
    (kind==2&&q.method!=GSC_ACTOR_ANIMATION_FLAGGED_RESTART)||
    (kind==3&&q.method!=GSC_ACTOR_ANIMATION_SET&&q.method!=GSC_ACTOR_ANIMATION_FLAGGED_RESTART)||
    (kind==4&&q.method!=GSC_ACTOR_ANIMATION_AIM_NODES)||(kind==5&&q.method!=GSC_ACTOR_ANIMATION_FLAGGED_ALL)||(kind==6&&q.method!=GSC_ACTOR_ANIMATION_EXIT_PRONE)||(kind==7&&q.method!=GSC_ACTOR_ANIMATION_PRONE_NODES&&q.method!=GSC_ACTOR_ANIMATION_ENTER_PRONE&&q.method!=GSC_ACTOR_ANIMATION_EXIT_PRONE&&q.method!=GSC_ACTOR_ANIMATION_UPDATE_PRONE)||(kind==8&&q.method!=GSC_ACTOR_ANIMATION_ORIENT_MODE&&q.method!=GSC_ACTOR_ANIMATION_ORIENT_ERROR_PREFIX)||kind>8){
  return ActorControllerSourceEntryFailure();}
 o->sourceRequest=q;o->sourceBorrowed=borrowed;o->sourceUser=user;o->sourceCurrent=current;o->sourceContext=context;
 o->checking=1;ok=o->api.source.claim(o->api.source.context,borrowed,current,context,&o->sourceReceipt)==1;o->checking=0;
 if(!ok||o->cancelled||memcmp(borrowed,&q,sizeof(q))||memcmp(user,&q,sizeof(q))||!ActorControllerAnimationScope(o)){
  return ActorControllerSourceEntryFailure();}
 o->sourceActive=1;o->sourceChecking=1;ok=ActorControllerSourceScope(o);o->sourceChecking=0;
 if(!ok){return ActorControllerSourceEntryFailure();}
 if(g_debugMove.integer>1&&(q.method==GSC_ACTOR_ANIMATION_KNOB||q.method==GSC_ACTOR_ANIMATION_AIM_NODES))
  G_Printf("[400 source command] owner=%u tick=%d kind=%d method=%d dispatch=%llu claimed=1\n",q.entity.slot,now,kind,q.method,(unsigned long long)o->sourceReceipt.dispatch);
 if(g_debugMove.integer>1&&kind==1&&q.method==GSC_ACTOR_ANIMATION_FLAGGED_RESTART)
  G_Printf("[403 source face] owner=%u tick=%d node=%u dispatch=%llu claimed=1\n",q.entity.slot,now,q.animation[0].ordinal,(unsigned long long)o->sourceReceipt.dispatch);
 if(g_debugMove.integer>1&&kind==5)
  G_Printf("[404 source all] owner=%u tick=%d node=%u stop=%u dispatch=%llu claimed=1\n",q.entity.slot,now,q.animation[0].ordinal,q.animation[1].ordinal,(unsigned long long)o->sourceReceipt.dispatch);
 if(g_debugMove.integer>1&&kind==6)
  G_Printf("[407 source exit] owner=%u tick=%d dispatch=%llu claimed=1\n",q.entity.slot,now,(unsigned long long)o->sourceReceipt.dispatch);
 return 1;
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
