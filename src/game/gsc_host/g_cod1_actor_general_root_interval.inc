#include "../../qcommon/cod1_actor_general_root_interval14.h"
typedef struct {C1ActorCurrentRoot base;c1agr14Projection_t projection;const c1ar14Root_t*bank[C1GA_COUNT];uint64_t locoRevision;} C1GeneralInterval;
static int ActorGeneralIntervalFence(const C1GeneralInterval*v,const GscEntityRef*input){
 return ActorCurrentRootFence(&v->base,input)&&ActorLocomotionReady(v->base.epoch)&&
  v->locoRevision==cod1LocoResources.revision&&ActorGeneralClockActive(&v->base.generic)&&
  ActorLocomotionClosure(&v->base.generic)&&!v->base.actor.turretMount.mounted&&!v->base.generic.detachedMG;
}
int G_Cod1ActorGeneralRootInterval(const GscEntityRef*input,int now,int goal,c1ari14Result_t*out,int(*current)(void*),void*ctx){
 C1GeneralInterval*v=NULL;C1ActorCurrentRoot*b;cod1ActorLease_t lease;c1ari14Result_t result;unsigned i;int ok=0;
 if(!input||!out||!current||now!=level.time||(goal!=0&&goal!=1)||cod1GenericOperation.busy||
    !ActorGoalBuffers(input,sizeof(*input),out,sizeof(*out),NULL,0)||ActorGenericPrivate(input,sizeof(*input))||
    ActorGenericPrivate(out,sizeof(*out))||ActorLocomotionPrivate(input,sizeof(*input))||ActorLocomotionPrivate(out,sizeof(*out))||
    TurretAnimationPrivate(input,sizeof(*input))||TurretAnimationPrivate(out,sizeof(*out))||
    TurretBankPrivate(input,sizeof(*input))||TurretBankPrivate(out,sizeof(*out))||
    !ActorGoalLease(input,&lease)||G_Cod1ActorGenericPresent(input)!=1)return 0;
 v=calloc(1,sizeof(*v));if(!v)return 0;b=&v->base;
 b->owner=*input;b->lease=lease;b->now=now;b->epoch=cod1EntityEpoch;b->receipt=cod1GenericReceipt;
 b->lookRevision=cod1GenericLookRevision;b->faceRevision=cod1FaceResources.revision;b->scriptedRevision=cod1ScriptedResources.revision;b->gunIndex=-1;
 v->locoRevision=cod1LocoResources.revision;b->generic=cod1GenericActors[lease.actorIndex];
 if(b->generic.tick!=now||!b->generic.nodes||!b->generic.runtime||!b->generic.catalog||!ActorLocomotionReady(b->epoch)||
    !ActorGeneralClockActive(&b->generic)||!ActorLocomotionClosure(&b->generic))goto done;
 b->actor=cod1Actors[lease.actorIndex];b->entity=g_entities[b->owner.slot];
 if(b->actor.turretMount.mounted||b->generic.detachedMG)return free(v),0;
 memcpy(b->original,b->generic.nodes,sizeof(b->original));memcpy(b->working,b->original,sizeof(b->working));
 for(i=0;i<C1T_CAPACITY;i++)if(cod1Turrets[i].used&&ActorRefEqual(&cod1Turrets[i].actor,&b->owner)){
  if(b->gunIndex>=0)goto done;b->gunIndex=(int)i;b->gun=cod1Turrets[i];b->mg=cod1TurretAnimations[i];
 }
 for(i=0;i<C1GA_COUNT;i++)v->bank[i]=ActorLocomotionRoot(i);
 cod1GenericOperation.busy=1;cod1GenericOperation.cancelled=0;cod1GenericOperation.stage=v;cod1GenericOperation.bytes=sizeof(*v);
 if(current(ctx)!=1||!ActorGeneralIntervalFence(v,input)||
    !C1AGR14_Project(cod1GenericDefinitions,b->working,v->bank,C1GA_COUNT,goal,&v->projection)||
    !C1ARI14_Interval(v->projection.roots,v->projection.leaves,v->projection.nodes,v->projection.count,goal,&result))goto unlock;
 if(current(ctx)!=1||!ActorGeneralIntervalFence(v,input))goto unlock;
 for(i=0;i<v->projection.leaves;i++)if(ActorLocomotionRoot(v->projection.leafSource[i])!=v->projection.roots[i])goto unlock;
 *out=result;ok=1;
unlock:
 cod1GenericOperation.stage=NULL;cod1GenericOperation.bytes=0;cod1GenericOperation.busy=0;
done:free(v);return ok;
}
