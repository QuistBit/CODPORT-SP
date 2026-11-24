
#include "../cod1_gsc/animtime-profile.h"
static int r243_animtime_resources_current(void){
 return cod1GenericDefinitions&&cod1GenericEpoch==cod1EntityEpoch&&cod1GenericReceipt==cod1TurretAnimationReceipt&&
  cod1GenericLookEpoch==cod1EntityEpoch&&C1AUT14_Ready(&cod1GenericUnmountedBank);
}
/* ENGINE47E0E0 accepts synchronized branches. Their stored phase is owned by
 * the prepared ordinary tree clock, not by a leaf's notetrack token. A
 * positive current weight proves this selected domain has live node info;
 * absent-info handling remains separate and is never guessed as zero. */
static int r243_animtime_sync_current(const C1GARecord*r,unsigned n){
 return cod1GenericDefinitions[n].children&&(cod1GenericDefinitions[n].flags&3)&&
  ActorLocomotionReady(cod1EntityEpoch)&&ActorLocomotionTarget(n)&&ActorOrdinaryClockOwner(r)&&
  r->nodes[n].currentWeight>0&&r->nodes[n].currentWeight<=1;
}
static int r243_animtime_validate(void*u,const GscEntityRef*e){cod1ActorLease_t l;GscEntityRef copy;(void)u;if(!e||!R243AnimTimeValidationRefCurrent(e))return 0;copy=*e;return ActorGoalLease(&copy,&l);}
static int r243_animtime_query(void*u,const R243AnimTimeRequest*input,R243AnimTimeReply*out,char*error,size_t cap,int(*current)(void*),void*ctx){
 R243AnimTimeRequest q;cod1ActorLease_t l,after;C1GARecord saved;C1GARecord*r;c1atl14Node_t node;c1agt14Definition_t definition;uint64_t epoch,resourceRevision;uint32_t bits;int ok=0,group;unsigned n,p;(void)u;
 if(!input||!out||cod1GenericOperation.busy||!R243AnimTimeNativeBuffersCurrent(input,out,error,cap,current,ctx)||!R243AnimTimeSessionHostTimeCurrent(input,level.time))return 0;
 q=*input;n=q.animation.ordinal;
 if(!r243_animtime_resources_current())goto hold;
 if(!ActorGoalLease(&q.entity,&l)||!ActorGenericBinding(&q.animation,q.runtime)||!n||n>=C1GA_COUNT)goto hold;
 r=cod1GenericActors+l.actorIndex;
 if(!r->used||!r->nodes||!r->notify||memcmp(&r->lease,&l,sizeof(l))||r->runtime!=q.runtime||r->catalog!=q.catalog||(r->tick<0||r->tick>level.time))goto hold;
 group=cod1GenericDefinitions[n].children!=0;
 if(group){if(!r243_animtime_sync_current(r,n))goto hold;}
 else{
  if(ActorGenericShuffleIndex(q.animation.name)<0||!r->notify->flags[n].token||!r->notify->generation[n]||
     !(r->nodes[n].currentWeight>0)||!(r->nodes[n].goalWeight>0))goto hold;
  for(p=n;p;p=cod1GenericDefinitions[p].parent)if(cod1GenericDefinitions[p].flags&3)goto hold;
 }
 /* Original 47CB30 reads stored phase without advancing. VM executes before the course animation frame; the owned row may still carry the previous tick. */
 saved=*r;node=r->nodes[n];definition=cod1GenericDefinitions[n];epoch=cod1EntityEpoch;resourceRevision=cod1LocoResources.revision;
 memcpy(&bits,&node.phase,4);if(bits>0x3f800000u)goto hold;
 cod1GenericOperation.busy=1;cod1GenericOperation.cancelled=0;
 if(current(ctx)!=1||cod1GenericOperation.cancelled||!ActorGoalLease(&q.entity,&after)||memcmp(&after,&l,sizeof(l))||
 memcmp(input,&q,sizeof(q))||memcmp(r,&saved,sizeof(saved))||memcmp(r->nodes+n,&node,sizeof(node))||!R243AnimTimeSessionHostTimeCurrent(&q,level.time)||
 !R243AnimTimeNativeBuffersCurrent(input,out,error,cap,current,ctx)||cod1GenericOperation.cancelled||memcmp(r,&saved,sizeof(saved))||memcmp(r->nodes+n,&node,sizeof(node))||
 cod1EntityEpoch!=epoch||!r243_animtime_resources_current()||memcmp(cod1GenericDefinitions+n,&definition,sizeof(definition))||
 (group&&(cod1LocoResources.revision!=resourceRevision||!r243_animtime_sync_current(r,n))))goto done;
 out->bits=bits;ok=1;
 if(group)G_Printf("[498 getanimtime] owner=%u tick=%d node=%u bits=%08x stored_tick=%d\n",q.entity.slot,level.time,n,bits,r->tick);
done:cod1GenericOperation.busy=0;return ok;
hold:if(error&&cap)snprintf(error,cap,"getAnimTime requires owned shuffle leaf or active ordinary synchronized group; absent-info domain HOLD");return 0;
}
const R243AnimTimeHost*R243GameAnimTimeHost(void){static const R243AnimTimeHost h={NULL,r243_animtime_validate,r243_animtime_query};return &h;}
