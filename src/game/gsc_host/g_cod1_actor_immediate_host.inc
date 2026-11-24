static int ActorOrientationPrivate(const void*,size_t);
static void ActorOrientationReset(void);
/* Included after the genuine full catalog generic owner. */
#include "../g_cod1_actor_immediate.h"
#include "../bg_cod1_actor_immediate14.h"
static int ActorAnimationMotionPrivate(const void*,size_t);
static void ActorAnimationMotionReset(void);
static int ActorAnimationMotionInvoke(const GscActorAnimationRequest*,char*,size_t,int(*)(void*),void*);
static struct {int busy,cancelled;uint64_t serial;void*stage;size_t bytes;} cod1ActorImmediateOperation;
typedef struct {int known144,tick;uint32_t cell0144;int knownAngle;uint32_t cell0138,cell013c,cell0140;cod1ActorLease_t lease;uint64_t runtime,catalog;} C1AIRecord;
static C1AIRecord cod1ActorImmediate[COD1_ACTOR_CAPACITY];
static GscActorAnimationHost cod1ActorImmediateFacade;
static int ActorImmediatePrivate(const void*p,size_t n){return ActorOrientationPrivate(p,n)||MoveDeltaGamePrivate(p,n)||ActorFloatPrivate(p,n)||ActorStancePrivate(p,n)||ActorAnimationMotionPrivate(p,n)||ActorOverlap(p,n,&cod1ActorImmediateOperation,sizeof(cod1ActorImmediateOperation))||ActorOverlap(p,n,cod1ActorImmediate,sizeof(cod1ActorImmediate))||ActorOverlap(p,n,&cod1ActorImmediateFacade,sizeof(cod1ActorImmediateFacade))||(cod1ActorImmediateOperation.stage&&ActorOverlap(p,n,cod1ActorImmediateOperation.stage,cod1ActorImmediateOperation.bytes));}
static void ActorImmediateReset(void){ActorOrientationReset();ActorAnimationMotionReset();if(cod1ActorImmediateOperation.busy)cod1ActorImmediateOperation.cancelled=1;if(cod1ActorImmediateOperation.serial!=UINT64_MAX)cod1ActorImmediateOperation.serial++;memset(cod1ActorImmediate,0,sizeof(cod1ActorImmediate));}
typedef struct {GscActorAnimationRequest q;cod1ActorLease_t lease;uint64_t serial,epoch;cod1ActorRecord_t actor;gentity_t body;C1AIRecord previous;C1GARecord aimTree;const GscActorAnimationRequest*source;int(*current)(void*);void*ctx;} C1AICall;
static int ActorImmediateScope(C1AICall*c){cod1ActorLease_t after;
 return c&&cod1ActorImmediateOperation.busy&&!cod1ActorImmediateOperation.cancelled&&c->serial==cod1ActorImmediateOperation.serial&&c->epoch==cod1EntityEpoch&&
  (c->q.method!=GSC_ACTOR_ANIMATION_AIM_NODES||
   (cod1GenericEpoch==c->epoch&&!cod1GenericOperation.busy&&
    !memcmp(&c->aimTree,cod1GenericActors+c->lease.actorIndex,sizeof(c->aimTree))))&&
  c->q.now_ms==level.time&&cod1EntityReady&&!level.spawning&&!level.restarted&&ActorMapAvailable()&&TurretAnimationMapCurrent(cod1TurretAnimationReceipt)&&
  ActorGoalBuffers(c->source,sizeof(*c->source),NULL,0,NULL,0)&&!ActorImmediatePrivate(c->source,sizeof(*c->source))&&!memcmp(c->source,&c->q,sizeof(c->q))&&
  ActorGoalLease(&c->q.entity,&after)&&!memcmp(&after,&c->lease,sizeof(after))&&
  !memcmp(&c->actor,&cod1Actors[c->lease.actorIndex],sizeof(c->actor))&&!memcmp(&c->body,g_entities+c->q.entity.slot,sizeof(c->body))&&!memcmp(&c->previous,cod1ActorImmediate+c->lease.actorIndex,sizeof(c->previous));
}
static int ActorImmediateFence(C1AICall*c){return ActorImmediateScope(c)&&c->current(c->ctx)==1&&ActorImmediateScope(c);}
static int ActorImmediateInvokeCore(void*unused,const GscActorAnimationRequest*input,char*error,size_t cap,int(*current)(void*),void*ctx,int sourceOwned){
 C1AICall*c=NULL;GscActorAnimationRequest q;cod1ActorLease_t lease;C1AIRecord record;uint32_t cells[5]={0},staged[5];int ok=0;(void)unused;
 if(!input||!current||!ActorGoalBuffers(input,sizeof(*input),NULL,0,error,cap)||ActorImmediatePrivate(input,sizeof(*input))||ActorImmediatePrivate(error,cap))return 0;
 q=*input;
 if(q.method==GSC_ACTOR_ANIMATION_PRONE_NODES||q.method==GSC_ACTOR_ANIMATION_ENTER_PRONE||q.method==GSC_ACTOR_ANIMATION_EXIT_PRONE||q.method==GSC_ACTOR_ANIMATION_UPDATE_PRONE)return ActorProneInvoke(input,error,cap,current,ctx);
 if(q.method==GSC_ACTOR_ANIMATION_FLAGGED_ALL && q.animation[0].ordinal==131 &&
    !strcmp(q.animation[0].tree,"generic_human"))
  return ActorScriptedInvoke(NULL,input,error,cap,current,ctx);
 if(q.method==GSC_ACTOR_ANIMATION_ANIM_MODE)return ActorAnimationMotionInvoke(input,error,cap,current,ctx);
 if(q.method!=GSC_ACTOR_ANIMATION_AIM_NODES&&q.method!=GSC_ACTOR_ANIMATION_EXIT_PRONE&&q.method!=GSC_ACTOR_ANIMATION_UPDATE_PRONE&&q.method!=GSC_ACTOR_ANIMATION_ORIENT_MODE&&q.method!=GSC_ACTOR_ANIMATION_ORIENT_ERROR_PREFIX)return ActorGenericInvoke(NULL,input,error,cap,current,ctx);
 if(!sourceOwned&&!ActorControllerForeignMutationAllowed())return 0;
 if(cod1ActorImmediateOperation.busy){cod1ActorImmediateOperation.cancelled=1;return 0;}
 if(q.precision_bits!=53||q.rounding||!q.runtime_identity||!q.catalog_identity||q.now_ms!=level.time||
    cod1ActorImmediateOperation.serial==UINT64_MAX||!ActorGoalLease(&q.entity,&lease))return 0;
 if(q.method==GSC_ACTOR_ANIMATION_AIM_NODES){
  const C1GARecord*t=cod1GenericActors+lease.actorIndex;unsigned i;
  /* Resolve against this actor's owned generic_human tree, as native66CF0
   * resolves202DFCF0[actorIndex]. Global catalog membership is insufficient. */
  if(cod1GenericEpoch!=cod1EntityEpoch||cod1GenericOperation.busy||!t->used||!t->nodes||!t->notify||!t->names||
     t->runtime!=q.runtime_identity||t->catalog!=q.catalog_identity||memcmp(&t->lease,&lease,sizeof(lease)))return 0;
  for(i=0;i<6;i++)if(!ActorGenericBinding(q.animation+i,q.runtime_identity))return 0;
  if(q.scalar_bits[0]||q.scalar_bits[1]||q.scalar_bits[2]||q.notify[0])return 0;
 }
 else if(q.method==GSC_ACTOR_ANIMATION_ORIENT_ERROR_PREFIX){
  static const GscAnimationLengthRequest emptyAnimation[6]={{0}};
  if(!memchr(q.notify,0,sizeof(q.notify))||strcmp(q.notify,"face angle")||q.scalar_bits[0]||q.scalar_bits[1]||q.scalar_bits[2]||
     memcmp(q.animation,emptyAnimation,sizeof(emptyAnimation)))return 0;
 }
 else if(q.method==GSC_ACTOR_ANIMATION_ORIENT_MODE){
  if(!memchr(q.notify,0,sizeof(q.notify))||(strcmp(q.notify,"face default")&&strcmp(q.notify,"face angle")&&strcmp(q.notify,"face current")))return 0;
  if(!strcmp(q.notify,"face angle")&&!C1AI14_OrientAngle(q.scalar_bits[0],staged))return 0;
 }
 else if(q.method==GSC_ACTOR_ANIMATION_UPDATE_PRONE){unsigned i;for(i=0;i<2;i++)if(!ActorGenericBinding(q.animation+i,q.runtime_identity))return 0;for(i=0;i<3;i++){uint32_t bits=q.scalar_bits[i];if(bits>>31||(bits&0x7fffffff)>0x44160000u||((bits&0x7fffffff)&&!(bits&0x7f800000u)))return 0;}if(!C1AI14_ExitFresh(cod1Actors[lease.actorIndex].nativeData.actor.zeroCells))return 0;}
 else {uint32_t bits=q.scalar_bits[0];if(bits>>31||(bits&0x7fffffff)>0x44160000u||((bits&0x7fffffff)&&!(bits&0x7f800000u)))return 0;if(!C1AI14_ExitFresh(cod1Actors[lease.actorIndex].nativeData.actor.zeroCells))return 0;}
 c=malloc(sizeof(*c));if(!c)return 0;memset(c,0,sizeof(*c));c->q=q;c->lease=lease;c->source=input;c->current=current;c->ctx=ctx;c->epoch=cod1EntityEpoch;c->previous=cod1ActorImmediate[lease.actorIndex];
 if(q.method==GSC_ACTOR_ANIMATION_AIM_NODES)c->aimTree=cod1GenericActors[lease.actorIndex];
 memcpy(&c->actor,cod1Actors+lease.actorIndex,sizeof(c->actor));memcpy(&c->body,g_entities+q.entity.slot,sizeof(c->body));
 cod1ActorImmediateOperation.stage=c;cod1ActorImmediateOperation.bytes=sizeof(*c);cod1ActorImmediateOperation.busy=1;cod1ActorImmediateOperation.cancelled=0;c->serial=++cod1ActorImmediateOperation.serial;
 if(!ActorImmediateFence(c)||!ActorImmediateFence(c)||!ActorGoalBuffers(input,sizeof(*input),NULL,0,error,cap)||ActorImmediatePrivate(error,cap))goto done;
 if(q.method==GSC_ACTOR_ANIMATION_AIM_NODES){
  unsigned i;cod1ActorRecord_t*a=cod1Actors+lease.actorIndex;
  /* No borrowed code after the closing fences; nodes retain the actor's
   * actual allocation lifetime. No playback, state event or fake aim. */
  for(i=0;i<6;i++)a->aimNodes154[i]=q.animation[i].ordinal;
  a->aimNodesRuntime=q.runtime_identity;a->aimNodesCatalog=q.catalog_identity;a->aimNodesSet=1;
  if(g_debugMove.integer>1)G_Printf("[397 aim nodes] owner=%u/%u tick=%d nodes=%u,%u,%u,%u,%u,%u\n",q.entity.slot,q.entity.generation,q.now_ms,a->aimNodes154[0],a->aimNodes154[1],a->aimNodes154[2],a->aimNodes154[3],a->aimNodes154[4],a->aimNodes154[5]);
 }
 else if(q.method==GSC_ACTOR_ANIMATION_ORIENT_ERROR_PREFIX){
  memset(&record,0,sizeof(record));
  if(c->previous.runtime==q.runtime_identity&&c->previous.catalog==q.catalog_identity&&!memcmp(&c->previous.lease,&lease,sizeof(lease))){record.knownAngle=c->previous.knownAngle;record.cell0138=c->previous.cell0138;record.cell013c=c->previous.cell013c;record.cell0140=c->previous.cell0140;}
  record.known144=1;record.cell0144=0;record.tick=q.now_ms;record.lease=lease;record.runtime=q.runtime_identity;record.catalog=q.catalog_identity;
  /* Native order: +144=0 before +134=1. No angle kernel, rollback or
   * successful orientation publication; retained angle knowledge stays owned. */
  cod1ActorImmediate[lease.actorIndex]=record;
  cod1Actors[lease.actorIndex].defaults.allocation.cell0134=1;
 }
 else if(q.method==GSC_ACTOR_ANIMATION_ORIENT_MODE){
  /* Canonical allocation-owned angles preserve native untouched cells. */
  cells[0]=c->actor.defaults.allocation.cell0134;
  memcpy(cells+1,c->actor.orientAngles138,12);
  if(!strcmp(q.notify,"face angle")){if(!C1AI14_OrientAngle(q.scalar_bits[0],staged))goto done;}
  else if(!strcmp(q.notify,"face current")){
   if(c->actor.desiredAngleUnavailable||(c->actor.desiredAngle148&0x7fffffff)>0x49742400u||
      !C1AI14_OrientCurrent(cells,c->actor.desiredAngle148,staged))goto done;
  }else if(!C1AI14_OrientDefault(cells,staged))goto done;
  memset(&record,0,sizeof(record));
  if(c->previous.runtime==q.runtime_identity&&c->previous.catalog==q.catalog_identity&&!memcmp(&c->previous.lease,&lease,sizeof(lease))){record.knownAngle=c->previous.knownAngle;record.cell0138=c->previous.cell0138;record.cell013c=c->previous.cell013c;record.cell0140=c->previous.cell0140;}
  record.known144=1;record.cell0144=staged[4];record.tick=q.now_ms;record.lease=lease;record.runtime=q.runtime_identity;record.catalog=q.catalog_identity;
  record.knownAngle=1;record.cell0138=staged[1];record.cell013c=staged[2];record.cell0140=staged[3];
  memcpy(cod1Actors[lease.actorIndex].orientAngles138,staged+1,12);
  cod1Actors[lease.actorIndex].defaults.allocation.cell0134=staged[0];cod1ActorImmediate[lease.actorIndex]=record;
 }
 /* Fresh ExitProne is an actual native early return, with every byte retained.
  * There is no fabricated transition, successful active-prone fallback/event. */
 ok=1;
done:cod1ActorImmediateOperation.stage=NULL;cod1ActorImmediateOperation.bytes=0;cod1ActorImmediateOperation.busy=0;free(c);return ok;
}
/* Six-node AIM uses kind4; actual fresh ExitProne uses separate kind6.
 * Consume the exact
 * VM/Session offer before Core callbacks; retain the ordinary owner fences.
 * Every early Core refusal closes and poisons the once-only source receipt. */
static int ActorImmediateInvoke(void*unused,const GscActorAnimationRequest*input,char*error,size_t cap,int(*current)(void*),void*ctx){
 GscActorAnimationRequest q;int owned,ok,kind=4;
 if(!input||!current||!ActorGoalBuffers(input,sizeof(*input),NULL,0,error,cap)||ActorImmediatePrivate(input,sizeof(*input))||ActorImmediatePrivate(error,cap))return 0;
 q=*input;
 if(q.method==GSC_ACTOR_ANIMATION_PRONE_NODES||q.method==GSC_ACTOR_ANIMATION_ENTER_PRONE||q.method==GSC_ACTOR_ANIMATION_EXIT_PRONE||q.method==GSC_ACTOR_ANIMATION_UPDATE_PRONE)return ActorProneInvoke(input,error,cap,current,ctx);
 if(q.method==GSC_ACTOR_ANIMATION_ORIENT_MODE||q.method==GSC_ACTOR_ANIMATION_ORIENT_ERROR_PREFIX){kind=8;}
 else if(q.method==GSC_ACTOR_ANIMATION_EXIT_PRONE&&cod1ControllerAnimation.busy){
  static const GscAnimationLengthRequest empty[6]={{0}};static const char emptyNotify[64]={0};
  if(q.scalar_bits[1]||q.scalar_bits[2]||memcmp(q.animation,empty,sizeof(empty))||memcmp(q.notify,emptyNotify,sizeof(emptyNotify)))return 0;
  kind=6;
 }else if(q.method!=GSC_ACTOR_ANIMATION_AIM_NODES)return ActorImmediateInvokeCore(unused,input,error,cap,current,ctx,0);
 owned=ActorControllerGenericSourceEnter(&input->entity,q.runtime_identity,q.catalog_identity,q.now_ms,input,sizeof(*input),current,ctx,&q,kind);
 if(owned<0)return 0;
 ok=ActorImmediateInvokeCore(unused,input,error,cap,owned?ActorControllerGenericSourceCurrent:current,owned?ActorControllerGenericSourceContext():ctx,owned);
 return ActorControllerGenericSourceFinish(owned,ok);
}
/* Native-time source provider: the Session owns time conversion and the
 * exact GAME_ENTRY pointer. Never pass a source grant through a clock copy. */
/* Source method12 has a separate typed once-only owner. The ordinary
 * R243CommandSessionTake frontdoor remains unchanged and cannot use this.
 * Only the already supplied generic run/aim/face resource domain is admitted. */
static int ActorSourceFlaggedAllInvoke(const GscActorAnimationRequest*input,char*error,size_t cap,int(*current)(void*),void*ctx){
 GscActorAnimationRequest q;int owned,ok;unsigned n;
 if(!input||!current||!ActorGoalBuffers(input,sizeof(*input),NULL,0,error,cap)||ActorGenericPrivate(input,sizeof(*input))||ActorGenericPrivate(error,cap))return 0;
 q=*input;n=q.animation[0].ordinal;
 if(q.method!=GSC_ACTOR_ANIMATION_FLAGGED_ALL||q.precision_bits!=53||q.rounding||
    !(ActorLocomotionTarget(n)||ActorAimTarget(n)||ActorAimFaceLeaf(n)))return 0;
 owned=ActorControllerGenericSourceEnter(&input->entity,q.runtime_identity,q.catalog_identity,q.now_ms,input,sizeof(*input),current,ctx,&q,5);
 if(owned!=1)return 0;
 ok=ActorGenericTransactionCore(&input->entity,q.runtime_identity,q.catalog_identity,q.now_ms,input,sizeof(*input),error,cap,
    ActorControllerGenericSourceCurrent,ActorControllerGenericSourceContext(),ActorGenericMutate,&q);
 return ActorControllerGenericSourceFinish(owned,ok);
}
static int ActorSourceAnimationInvoke(void*u,const GscActorAnimationRequest*q,char*error,size_t cap,int(*current)(void*),void*ctx){
 if(!q||!cod1ControllerAnimation.busy)return 0;
 if(q->method==GSC_ACTOR_ANIMATION_PRONE_NODES||q->method==GSC_ACTOR_ANIMATION_ENTER_PRONE||q->method==GSC_ACTOR_ANIMATION_EXIT_PRONE||q->method==GSC_ACTOR_ANIMATION_UPDATE_PRONE)return ActorProneInvoke(q,error,cap,current,ctx);
 if(q->method==GSC_ACTOR_ANIMATION_ORIENT_MODE||q->method==GSC_ACTOR_ANIMATION_ORIENT_ERROR_PREFIX||q->method==GSC_ACTOR_ANIMATION_AIM_NODES||q->method==GSC_ACTOR_ANIMATION_EXIT_PRONE)return ActorImmediateInvoke(u,q,error,cap,current,ctx);
 if(q->method==GSC_ACTOR_ANIMATION_FLAGGED_ALL)return ActorSourceFlaggedAllInvoke(q,error,cap,current,ctx);
 return ActorScriptedInvoke(u,q,error,cap,current,ctx);
}
const GscActorAnimationHost *G_Cod1ActorSourceAnimationHost(void){
 static const GscActorAnimationHost h={NULL,ActorGenericValidate,ActorSourceAnimationInvoke};return &h;
}
const GscActorAnimationHost *G_Cod1ActorComposedAnimationHost(void){cod1ActorImmediateFacade.user=NULL;cod1ActorImmediateFacade.validate=ActorGenericValidate;cod1ActorImmediateFacade.invoke=ActorImmediateInvoke;return &cod1ActorImmediateFacade;}
#include "g_cod1_actor_animation_motion.inc"

#include "g_cod1_actor_orientation.inc"
