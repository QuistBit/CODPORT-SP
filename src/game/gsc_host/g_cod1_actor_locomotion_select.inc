/* Shared canonical native MAIN source and actual state queue. The selected
 * ordinary domain has only base4 and critical8; no fabricated queue flags. */
static int ActorLocomotionControllerCurrent(const cod1ActorRecord_t*a,uint64_t serial){
 const cod1ActorScriptedController_t*m=&a->scriptedController;
 return a->alive&&a->dataInitialized&&a->nativeDataInitialized&&m->entered&&!m->invalid&&
  m->actorSerial==serial&&!a->turretMount.mounted&&!a->motion.busy&&
  a->turretMount.currentStack[0]==4&&(a->turretMount.currentTop==0||
   (a->turretMount.currentTop==1&&a->turretMount.currentStack[1]==8))&&
  !cod1NormalRecords[a-cod1Actors]&&a->scriptedAux.record==FRESH_AUX_EMPTY&&
  a->scriptedAux.link==FRESH_LINK_UNATTACHED;
}
static int ActorOrdinaryClockOwner(const C1GARecord*r){
 const cod1ActorRecord_t*a;const cod1ActorScriptedController_t*m;
 if(!r||!r->used||!r->nodes||!r->notify||r->lease.actorIndex>=COD1_ACTOR_CAPACITY||
    !ActorLeaseValid(&r->lease)||!ActorScriptedClockSelected(r->lease.actorIndex)||
    !ActorLocomotionReady(cod1EntityEpoch))return 0;
 a=cod1Actors+r->lease.actorIndex;m=&a->scriptedController;
 return ActorLocomotionControllerCurrent(a,r->lease.serial)&&m->selectionSerial&&m->nativeClass268==1&&
  (m->currentSource==1||m->currentSource==2)&&m->runtime==r->runtime&&m->catalog==r->catalog;
}
static int ActorLocomotionAnimationScope(const cod1ActorRecord_t*a,const GscActorControllerRequest*q){
 const cod1ActorScriptedController_t*m=&a->scriptedController;
 return (q->locomotion_source==1||q->locomotion_source==2)&&
  ActorLocomotionControllerCurrent(a,q->actor_serial)&&m->currentSource==q->locomotion_source&&
  m->selectionSerial&&m->lastSelectionFrame==q->frame_generation&&m->nativeClass268==1;
}
static int ActorLocomotionState(void*ctx,int phase,int state,int related,int*out){
 C1ControllerOperation*o=ctx;cod1ActorRecord_t*a=o->stage;(void)related;
 if(!out)return 0;
 if(state==4&&(phase==COD1_ACTOR_STATE_SUSPEND||phase==COD1_ACTOR_STATE_RESUME)){*out=0;return 1;}
 if(state==8){
  if(phase==COD1_ACTOR_STATE_ENTER||phase==COD1_ACTOR_STATE_RESUME)a->scriptedController.criticalEnteredMs=o->now;
  else if(phase!=COD1_ACTOR_STATE_EXIT&&phase!=COD1_ACTOR_STATE_SUSPEND)return 0;
  *out=0;return 1;
 }
 return 0;
}
static int ActorLocomotionSelect(const GscActorControllerRequest*input,GscActorControllerSelection*out,
 char*error,size_t cap,int(*current)(void*),void*ctx,const GscMainReplacementService*service){
 C1ControllerOperation*o=&cod1ControllerOperation;cod1ActorRecord_t*a;cod1ActorScriptedController_t*m;
 cod1ActorActualState_t actual;cod1ActorCriticalState_t queue;cod1ActorStateCallbacks_t cb;
 GscActorControllerSelection result;int source,changed,critical,category,ok=0;uint64_t selection;
 if(!input||!out||!current||(input->locomotion_source!=-1&&input->locomotion_source!=1&&input->locomotion_source!=2)||
    !ActorGoalBuffers(input,sizeof(*input),out,sizeof(*out),error,cap)||ActorOverlap(input,sizeof(*input),out,sizeof(*out)))return 0;
 if(!ActorControllerBegin(input,sizeof(*input),C1CTRL_SELECT,&input->entity,input->now_ms,current,ctx))return 0;
 a=o->stage;m=&a->scriptedController;
 /* The staged copy cannot index allocation tables. Validate that part on the
  * captured canonical owner before consuming any queued command. */
 if(!ActorLocomotionControllerCurrent(cod1Actors+o->lease.actorIndex,input->actor_serial)||
    input->actor_serial!=o->lease.serial||m->runtime!=input->runtime_identity||m->catalog!=input->catalog_identity||
    !ActorControllerClosedFrame(o)||a->scriptStateBusy||!C1AST14_State(&a->scriptState)||
    m->currentSource<0||m->currentSource>2||m->force250||m->selectionSerial==UINT64_MAX||
    m->lastSelectionFrame>=input->frame_generation||m->revision>=UINT64_MAX-2)goto done;
 ActorControllerActualRead(a,&actual);queue=ActorStateQueueKernel(a);
 cb.context=o;cb.invoke=ActorLocomotionState;
 if(!Cod1ActorStateConsume(&actual,&queue,o->now,&cb)||actual.stack[0]!=4||
    (actual.top!=0&&(actual.top!=1||actual.stack[1]!=8)))goto done;
 ActorControllerActualCommit(a,&actual);ActorStateQueueCommit(a,&queue);critical=actual.top==1;
 source=input->locomotion_source;
 if(source==-1){if(!critical||!m->currentSource)goto done;source=m->currentSource;}
 else if(critical&&m->currentSource&&source!=m->currentSource)goto done;
 category=critical?m->currentCategory:(source==1?7:1);if(!category)goto done;
 changed=m->currentSource!=source;selection=m->selectionSerial;
 if(changed){
  if((selection!=0)!=!!(service&&service->run))goto done;
  if(service&&(!service->run(service->context,0,selection,ActorControllerCurrent,o)||!ActorControllerCurrent(o)))goto done;
  m->nativeClass268=1;m->force250=0;a->defaults.allocation.cell0134=0;
  a->nativeInteraction1170=0;a->nativeMode264=0;a->nativeData.entity.nativeEntityFlags&=~UINT32_C(0x10000);
  ++m->revision;cod1Actors[o->lease.actorIndex]=*a;*o->before=*a;
  if(service&&(!service->run(service->context,1,selection,ActorControllerCurrent,o)||!ActorControllerCurrent(o)))goto done;
  if(!C1AST14_MainSelected(&a->scriptState))goto done;
  m->currentSource=source;m->selectionSerial=selection+1;
 }else if(!selection||!service||!service->run)goto done;
 m->currentCategory=a->nativeMode264?(int)a->nativeMode264:category;m->lastSelectionFrame=input->frame_generation;++m->revision;
 memset(&result,0,sizeof(result));result.certificate.owner=o->lease.entity;result.certificate.serial=o->lease.serial;
 result.certificate.current=ActorInheritedCertificate;result.selection_serial=m->selectionSerial;result.source_changed=changed;
 if(!ActorControllerCurrent(o))goto done;cod1Actors[o->lease.actorIndex]=*a;*out=result;ok=1;
done:ActorControllerEnd();return ok;
}
