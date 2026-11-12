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
static int ActorImmediatePrivate(const void*p,size_t n){return MoveDeltaGamePrivate(p,n)||ActorFloatPrivate(p,n)||ActorStancePrivate(p,n)||ActorAnimationMotionPrivate(p,n)||ActorOverlap(p,n,&cod1ActorImmediateOperation,sizeof(cod1ActorImmediateOperation))||ActorOverlap(p,n,cod1ActorImmediate,sizeof(cod1ActorImmediate))||ActorOverlap(p,n,&cod1ActorImmediateFacade,sizeof(cod1ActorImmediateFacade))||(cod1ActorImmediateOperation.stage&&ActorOverlap(p,n,cod1ActorImmediateOperation.stage,cod1ActorImmediateOperation.bytes));}
static void ActorImmediateReset(void){ActorAnimationMotionReset();if(cod1ActorImmediateOperation.busy)cod1ActorImmediateOperation.cancelled=1;if(cod1ActorImmediateOperation.serial!=UINT64_MAX)cod1ActorImmediateOperation.serial++;memset(cod1ActorImmediate,0,sizeof(cod1ActorImmediate));}
typedef struct {GscActorAnimationRequest q;cod1ActorLease_t lease;uint64_t serial,epoch;cod1ActorRecord_t actor;gentity_t body;C1AIRecord previous;const GscActorAnimationRequest*source;int(*current)(void*);void*ctx;} C1AICall;
static int ActorImmediateScope(C1AICall*c){cod1ActorLease_t after;
 return c&&cod1ActorImmediateOperation.busy&&!cod1ActorImmediateOperation.cancelled&&c->serial==cod1ActorImmediateOperation.serial&&c->epoch==cod1EntityEpoch&&
  c->q.now_ms==level.time&&cod1EntityReady&&!level.spawning&&!level.restarted&&ActorMapAvailable()&&TurretAnimationMapCurrent(cod1TurretAnimationReceipt)&&
  ActorGoalBuffers(c->source,sizeof(*c->source),NULL,0,NULL,0)&&!ActorImmediatePrivate(c->source,sizeof(*c->source))&&!memcmp(c->source,&c->q,sizeof(c->q))&&
  ActorGoalLease(&c->q.entity,&after)&&!memcmp(&after,&c->lease,sizeof(after))&&
  !memcmp(&c->actor,&cod1Actors[c->lease.actorIndex],sizeof(c->actor))&&!memcmp(&c->body,g_entities+c->q.entity.slot,sizeof(c->body))&&!memcmp(&c->previous,cod1ActorImmediate+c->lease.actorIndex,sizeof(c->previous));
}
static int ActorImmediateFence(C1AICall*c){return ActorImmediateScope(c)&&c->current(c->ctx)==1&&ActorImmediateScope(c);}
static int ActorImmediateInvoke(void*unused,const GscActorAnimationRequest*input,char*error,size_t cap,int(*current)(void*),void*ctx){
 C1AICall*c=NULL;GscActorAnimationRequest q;cod1ActorLease_t lease;C1AIRecord record;uint32_t cells[5]={0},staged[5];int ok=0;(void)unused;
 if(!input||!current||!ActorGoalBuffers(input,sizeof(*input),NULL,0,error,cap)||ActorImmediatePrivate(input,sizeof(*input))||ActorImmediatePrivate(error,cap))return 0;
 q=*input;
 if(q.method==GSC_ACTOR_ANIMATION_ANIM_MODE)return ActorAnimationMotionInvoke(input,error,cap,current,ctx);
 if(q.method!=GSC_ACTOR_ANIMATION_EXIT_PRONE&&q.method!=GSC_ACTOR_ANIMATION_UPDATE_PRONE&&q.method!=GSC_ACTOR_ANIMATION_ORIENT_MODE&&q.method!=GSC_ACTOR_ANIMATION_ORIENT_ERROR_PREFIX)return ActorGenericInvoke(NULL,input,error,cap,current,ctx);
 if(!ActorControllerForeignMutationAllowed())return 0;
 if(cod1ActorImmediateOperation.busy){cod1ActorImmediateOperation.cancelled=1;return 0;}
 if(q.precision_bits!=53||q.rounding||!q.runtime_identity||!q.catalog_identity||q.now_ms!=level.time||
    cod1ActorImmediateOperation.serial==UINT64_MAX||!ActorGoalLease(&q.entity,&lease))return 0;
 if(q.method==GSC_ACTOR_ANIMATION_ORIENT_ERROR_PREFIX){
  static const GscAnimationLengthRequest emptyAnimation[3]={{0}};
  if(!memchr(q.notify,0,sizeof(q.notify))||strcmp(q.notify,"face angle")||q.scalar_bits[0]||q.scalar_bits[1]||q.scalar_bits[2]||
     memcmp(q.animation,emptyAnimation,sizeof(emptyAnimation)))return 0;
 }
 else if(q.method==GSC_ACTOR_ANIMATION_ORIENT_MODE){
  if(!memchr(q.notify,0,sizeof(q.notify))||(strcmp(q.notify,"face default")&&strcmp(q.notify,"face angle")))return 0;
  if(!strcmp(q.notify,"face angle")&&!C1AI14_OrientAngle(q.scalar_bits[0],staged))return 0;
 }
 else if(q.method==GSC_ACTOR_ANIMATION_UPDATE_PRONE){unsigned i;for(i=0;i<2;i++)if(!ActorGenericBinding(q.animation+i,q.runtime_identity))return 0;for(i=0;i<3;i++){uint32_t bits=q.scalar_bits[i];if(bits>>31||(bits&0x7fffffff)>0x44160000u||((bits&0x7fffffff)&&!(bits&0x7f800000u)))return 0;}if(!C1AI14_ExitFresh(cod1Actors[lease.actorIndex].nativeData.actor.zeroCells))return 0;}
 else {uint32_t bits=q.scalar_bits[0];if(bits>>31||(bits&0x7fffffff)>0x44160000u||((bits&0x7fffffff)&&!(bits&0x7f800000u)))return 0;if(!C1AI14_ExitFresh(cod1Actors[lease.actorIndex].nativeData.actor.zeroCells))return 0;}
 c=malloc(sizeof(*c));if(!c)return 0;memset(c,0,sizeof(*c));c->q=q;c->lease=lease;c->source=input;c->current=current;c->ctx=ctx;c->epoch=cod1EntityEpoch;c->previous=cod1ActorImmediate[lease.actorIndex];
 memcpy(&c->actor,cod1Actors+lease.actorIndex,sizeof(c->actor));memcpy(&c->body,g_entities+q.entity.slot,sizeof(c->body));
 cod1ActorImmediateOperation.stage=c;cod1ActorImmediateOperation.bytes=sizeof(*c);cod1ActorImmediateOperation.busy=1;cod1ActorImmediateOperation.cancelled=0;c->serial=++cod1ActorImmediateOperation.serial;
 if(!ActorImmediateFence(c)||!ActorImmediateFence(c)||!ActorGoalBuffers(input,sizeof(*input),NULL,0,error,cap)||ActorImmediatePrivate(error,cap))goto done;
 if(q.method==GSC_ACTOR_ANIMATION_ORIENT_ERROR_PREFIX){
  memset(&record,0,sizeof(record));
  if(c->previous.runtime==q.runtime_identity&&c->previous.catalog==q.catalog_identity&&!memcmp(&c->previous.lease,&lease,sizeof(lease))){record.knownAngle=c->previous.knownAngle;record.cell0138=c->previous.cell0138;record.cell013c=c->previous.cell013c;record.cell0140=c->previous.cell0140;}
  record.known144=1;record.cell0144=0;record.tick=q.now_ms;record.lease=lease;record.runtime=q.runtime_identity;record.catalog=q.catalog_identity;
  /* Native order: +144=0 before +134=1. No angle kernel, rollback or
   * successful orientation publication; retained angle knowledge stays owned. */
  cod1ActorImmediate[lease.actorIndex]=record;
  cod1Actors[lease.actorIndex].defaults.allocation.cell0134=1;
 }
 else if(q.method==GSC_ACTOR_ANIMATION_ORIENT_MODE){
  /* Only these two cells are touched by the selected native branch. The
   * omitted intermediates are never read as initialized native state. */
  cells[0]=c->actor.defaults.allocation.cell0134;
  if(!strcmp(q.notify,"face angle")){if(!C1AI14_OrientAngle(q.scalar_bits[0],staged))goto done;}
  else if(!C1AI14_OrientDefault(cells,staged))goto done;
  memset(&record,0,sizeof(record));
  if(c->previous.runtime==q.runtime_identity&&c->previous.catalog==q.catalog_identity&&!memcmp(&c->previous.lease,&lease,sizeof(lease))){record.knownAngle=c->previous.knownAngle;record.cell0138=c->previous.cell0138;record.cell013c=c->previous.cell013c;record.cell0140=c->previous.cell0140;}
  record.known144=1;record.cell0144=staged[4];record.tick=q.now_ms;record.lease=lease;record.runtime=q.runtime_identity;record.catalog=q.catalog_identity;
  if(!strcmp(q.notify,"face angle")){record.knownAngle=1;record.cell0138=staged[1];record.cell013c=staged[2];record.cell0140=staged[3];}
  cod1Actors[lease.actorIndex].defaults.allocation.cell0134=staged[0];cod1ActorImmediate[lease.actorIndex]=record;
 }
 /* Fresh ExitProne is an actual native early return, with every byte retained.
  * There is no fabricated transition, successful active-prone fallback/event. */
 ok=1;
done:cod1ActorImmediateOperation.stage=NULL;cod1ActorImmediateOperation.bytes=0;cod1ActorImmediateOperation.busy=0;free(c);return ok;
}
const GscActorAnimationHost *G_Cod1ActorComposedAnimationHost(void){cod1ActorImmediateFacade.user=NULL;cod1ActorImmediateFacade.validate=ActorGenericValidate;cod1ActorImmediateFacade.invoke=ActorImmediateInvoke;return &cod1ActorImmediateFacade;}
#include "g_cod1_actor_animation_motion.inc"
