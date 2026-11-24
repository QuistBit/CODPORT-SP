/* Actual1EEB0 service: invoke122 again on SAME prepared tree. No repeated
 * cmd123/weights/oldPhase preparation, no extra50ms frame. */
typedef struct {
 int busy,cancelled,checking,vm_running,now,framePermit;uint64_t epoch,resources,controllerRevision;
 GscActorControllerRequest q;const GscActorControllerRequest*input;
 GscActorControllerAnimationServices api;const GscActorControllerAnimationServices*borrowed;
 cod1ActorLease_t lease;cod1ActorDObjState_t dobj;int(*current)(void*);void*context;
 int sourceActive,sourceChecking;GscActorAnimationRequest sourceRequest;GscActorSourceAnimationReceipt sourceReceipt;
 const void*sourceBorrowed,*sourceUser;int(*sourceCurrent)(void*);void*sourceContext;
 int lookSourceActive,lookSourceChecking;GscActorLookRequest lookSourceRequest;GscActorSourceLookReceipt lookSourceReceipt;
 const void*lookSourceBorrowed,*lookSourceUser;int(*lookSourceCurrent)(void*);void*lookSourceContext;
 GscEntityRef frameOwner;int(*frameCurrent)(void*);void*frameContext;
} C1ControllerAnimationOperation;
static C1ControllerAnimationOperation cod1ControllerAnimation;
static int ActorControllerAnimationDeny(void){if(cod1ControllerAnimation.busy)cod1ControllerAnimation.cancelled=1;cod1ControllerAnimation.framePermit=0;return 0;}
static int ActorControllerAnimationMutationAllowed(void){return !cod1ControllerAnimation.busy?1:ActorControllerAnimationDeny();}
/* run_current includes foreign certificate/resource callbacks. Its dynamic
 * extent is NOT authority for a source-owned animation mutation. The fresh
 * normal-body prefix requires none here; such source branches remain HELD
 * until an exact VM->Session->provider one-shot owner is supplied. */
static int ActorControllerAnimationGenericAllowed(void){return !cod1ControllerAnimation.busy?1:ActorControllerAnimationDeny();}
static int ActorControllerAnimationFrameEnter(const GscEntityRef*owner,int now,int(*current)(void*),void*context){
 C1ControllerAnimationOperation*o=&cod1ControllerAnimation;if(!o->busy)return 1;
 if(o->cancelled||o->checking||!o->framePermit||!owner||!ActorRefEqual(owner,&o->frameOwner)||
    now!=o->now||current!=o->frameCurrent||context!=o->frameContext)return ActorControllerAnimationDeny();
 o->framePermit=0;return 1;
}
static int ActorControllerAnimationArmFrame(C1ControllerAnimationOperation*o,const GscEntityRef*owner,int(*current)(void*),void*context){
 if(!o->busy||o->cancelled||o->checking||o->framePermit||!owner||!ActorRefEqual(owner,&o->q.entity))return ActorControllerAnimationDeny();
 o->framePermit=1;o->frameOwner=*owner;o->frameCurrent=current;o->frameContext=context;return 1;
}
static int ActorControllerAnimationPrivate(const void*p,size_t n){return ActorOverlap(p,n,&cod1ControllerAnimation,sizeof(cod1ControllerAnimation));}
static void ActorControllerAnimationCancelOwner(const GscEntityRef*owner){
 if(cod1ControllerAnimation.busy&&owner&&ActorRefEqual(owner,&cod1ControllerAnimation.q.entity))ActorControllerAnimationDeny();
}
static void ActorControllerAnimationReset(void){if(cod1ControllerAnimation.busy)ActorControllerAnimationDeny();else memset(&cod1ControllerAnimation,0,sizeof(cod1ControllerAnimation));}
static int ActorControllerAnimationScope(const C1ControllerAnimationOperation*o){
 cod1ActorLease_t lease;const cod1ActorRecord_t*a;const C1GARecord*r;const C1FaceFrame*f;
 if(!o||o!=&cod1ControllerAnimation||!o->busy||o->cancelled||o->now!=level.time||o->epoch!=cod1EntityEpoch||
    !o->input||memcmp(o->input,&o->q,sizeof(o->q))||!o->borrowed||memcmp(o->borrowed,&o->api,sizeof(o->api))||
    !ActorGoalLease(&o->q.entity,&lease)||memcmp(&lease,&o->lease,sizeof(lease))||
    o->resources!=cod1ScriptedResources.revision||!ActorScriptedReady(o->epoch))return 0;
 a=cod1Actors+lease.actorIndex;r=cod1GenericActors+lease.actorIndex;f=cod1FaceFrames+lease.actorIndex;
 return a->scriptedController.entered&&!a->scriptedController.invalid&&a->scriptedController.revision==o->controllerRevision&&
  a->scriptedController.actorSerial==o->q.actor_serial&&
  (o->q.locomotion_source?ActorLocomotionAnimationScope(a,&o->q):
   (a->turretMount.currentTop==1&&a->turretMount.currentStack[1]==9))&&
  !a->turretMount.mounted&&a->dobj.present&&!a->dobj.busy&&!memcmp(&a->dobj,&o->dobj,sizeof(o->dobj))&&r->used&&r->runtime==o->q.runtime_identity&&
  r->catalog==o->q.catalog_identity&&r->tick==o->now&&!memcmp(&r->lease,&lease,sizeof(lease))&&
  f->scripted&&f->frame==o->q.frame_generation&&f->tick==o->now&&!f->poisoned&&!memcmp(&f->lease,&lease,sizeof(lease));
}
static int ActorControllerAnimationCurrent(void*context){
 C1ControllerAnimationOperation*o=context;int ok;if(!ActorControllerAnimationScope(o))return 0;
 if(o->checking){o->cancelled=1;return 0;}o->checking=1;
 ok=o->current(o->context)==1&&ActorControllerAnimationScope(o)&&o->api.current(o->api.context)==1&&ActorControllerAnimationScope(o);
 o->checking=0;return ok;
}
static int ActorControllerPreparedInvoke(C1ControllerAnimationOperation*o){
 C1FaceFrameCall*c;C1FaceFrame next;GscEntityRef owner=o->q.entity;int ok=0;
 if(!ActorControllerAnimationArmFrame(o,&owner,ActorControllerAnimationCurrent,o))return 0;
 c=ActorFaceFrameEnter(&owner,o->now,ActorControllerAnimationCurrent,o);if(!c)return 0;
 if(!c->frame.scripted||c->frame.open||!c->frame.done||c->frame.count||
    c->frame.frame!=o->q.frame_generation||c->frame.tick!=o->now||c->saved.tick!=o->now)goto done;
 next=c->frame;next.open=1;next.done=0;
 ok=ActorFaceFrameCommit(c,&next);
 done:ActorFaceFrameLeave(c);return ok;
}
static int ActorControllerAnimationDeliver(void*context,const cod1ActorFaceNote_t*note){
 C1ControllerAnimationOperation*o=context;cod1ActorFaceNote_t copied;int ok;
 if(!note||!ActorControllerAnimationCurrent(o))return 0;copied=*note;
 if(!ActorRefEqual(&note->entity,&o->q.entity)||note->produced_ms!=o->now||
    note->frame_generation!=o->q.frame_generation||note->runtime!=o->q.runtime_identity||note->catalog!=o->q.catalog_identity)return 0;
 ok=o->api.notify(o->api.context,&note->entity,note->event,note->payload);
 return ok==1&&!memcmp(note,&copied,sizeof(copied))&&ActorControllerAnimationCurrent(o);
}
static int ActorControllerAnimationStep(void*u,const GscActorControllerRequest*input,
 const GscActorControllerAnimationServices*services,char*error,size_t cap,int(*current)(void*),void*context){
 C1ControllerAnimationOperation*o=&cod1ControllerAnimation;GscEntityRef owner;int resume,iterations=0,ok=0;(void)u;
 if(o->busy){o->cancelled=1;return 0;}
 if(cod1ControllerOperation.busy){cod1ControllerOperation.cancelled=1;return 0;}
 if(!input||!services||!current||!ActorGoalBuffers(input,sizeof(*input),NULL,0,error,cap)||
    !ActorGoalBuffers(services,sizeof(*services),NULL,0,error,cap))return 0;
 memset(o,0,sizeof(*o));o->q=*input;o->input=input;o->api=*services;o->borrowed=services;
 if(!o->api.current||!o->api.run_current||!o->api.notify||input->now_ms!=level.time||
    !ActorGoalLease(&input->entity,&o->lease))goto done;
 o->now=input->now_ms;o->epoch=cod1EntityEpoch;o->resources=cod1ScriptedResources.revision;
 o->controllerRevision=cod1Actors[o->lease.actorIndex].scriptedController.revision;o->dobj=cod1Actors[o->lease.actorIndex].dobj;
 o->current=current;o->context=context;o->busy=1;owner=o->q.entity;
 if(!ActorControllerAnimationCurrent(o))goto done;
 if(cod1Actors[o->lease.actorIndex].nativeData.entity.nativeEntityFlags&0x10000u){ok=1;goto done;}
 if(!ActorControllerPreparedInvoke(o))goto done;
 do{
  if(++iterations>512||!ActorControllerAnimationCurrent(o)||
     !ActorControllerAnimationArmFrame(o,&owner,ActorControllerAnimationCurrent,o)||
     !G_Cod1ActorFaceFrameStep(&owner,o->now,o->q.frame_generation,&resume,ActorControllerAnimationCurrent,o))goto done;
  if(cod1FaceFrames[o->lease.actorIndex].count&&
     (!ActorControllerAnimationArmFrame(o,&owner,ActorControllerAnimationCurrent,o)||
      !G_Cod1ActorFaceFrameDrain(&owner,o->now,o->q.frame_generation,ActorControllerAnimationCurrent,ActorControllerAnimationDeliver,o)))goto done;
  if(resume){
   o->vm_running=1;ok=o->api.run_current(o->api.context)==1;o->vm_running=0;
   if(!ok||!ActorControllerAnimationCurrent(o))goto done;
   if(cod1Actors[o->lease.actorIndex].nativeData.entity.nativeEntityFlags&0x10000u){ok=0;goto done;} /* mid-yield suppression receipt held */
  }
 }while(resume);
 ok=ActorControllerAnimationCurrent(o);
done:memset(o,0,sizeof(*o));return ok;
}

#include "g_cod1_actor_source_dispatch.inc"

#include "g_cod1_actor_source_look.inc"
