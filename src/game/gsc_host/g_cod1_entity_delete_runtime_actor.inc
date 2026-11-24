/* R240 separate explicit exact-prefix runtime actor retirement. No default
 * permission change. A real saved hidden/nonsolid motion origin is required;
 * real resource retirement restores it before the unchanged R204 free gate. */
#include "../g_cod1_actor_bootstrap.h"
static struct {uint64_t epoch,receipt;uint32_t row;cod1ActorLease_t lease;} cod1RuntimeDeleteOwner;
static int RuntimeActorDeletePrivate(const void*p,size_t n){return ActorOverlap(p,n,&cod1RuntimeDeleteOwner,sizeof(cod1RuntimeDeleteOwner));}
static void RuntimeActorDeleteReset(void){memset(&cod1RuntimeDeleteOwner,0,sizeof(cod1RuntimeDeleteOwner));}
static int RuntimeActorDeleteIdle(void){
 return !cod1ActorModelRegistryBusyEpoch&&!cod1TurretBankBusyEpoch&&!cod1ActorGoalBusy&&!cod1ActorCriticalBusy&&!cod1ActorEngineSceneBusyEpoch&&!cod1ActorDObjSceneBusyEpoch&&
  !cod1TurretBusy&&!cod1TurretFrameBusyEpoch&&!cod1TurretMountedBusyEpoch&&!cod1TurretMountBusyEpoch&&!cod1TurretFitBusyEpoch&&
  !cod1TurretAnimationOperation.busy&&!cod1GenericOperation.busy&&!cod1ActorLookOperation.busy&&
  !cod1ActorImmediateOperation.busy&&!cod1ActorAnimationMotionOperation.busy&&!cod1TurretShotBusyEpoch&&
  !cod1TurretActorConfigBusy&&!cod1UnmountedPublishBusy&&!cod1TurretActorTagBusy&&!cod1UnmountedTagBusy&&!cod1FaceTagBusy&&!cod1FaceTagOperation;
}
static int RuntimeActorDeletePrefix(uint64_t epoch,uint64_t receipt,const cod1ActorLease_t*lease,uint32_t*outRow){
 cod1ActorBootstrapSnapshot_t p;uint32_t i;
 if(!epoch||epoch!=cod1EntityEpoch||!receipt||!lease||!ActorLeaseValid(lease)||!cod1EntityReady||level.spawning||level.restarted||
  !G_Cod1ActorBootstrapRead(&p,NULL,0)||p.epoch!=epoch||p.receipt!=receipt||!p.enabled||!p.declarationsReady||!p.sealed||
  !p.selected||!p.runComplete||p.closed||p.invalid||p.placedRows>COD1_ACTOR_CAPACITY)return 0;
 for(i=0;i<p.placedRows;i++)if(ActorRefEqual(&p.rows[i].entity,&lease->entity)){
  const cod1ActorBootstrapRow_t*r=p.rows+i;
  if(r->retired||!r->accounted||!r->startupConsumed||r->stage!=COD1_ACTOR_ENTRY_ENTERED||r->entryStatus!=GSC_ENTITY_ENTRY_RETURNED||memcmp(&r->lease,lease,sizeof(*lease)))return 0;
  if(outRow)*outRow=i;return 1;
 }return 0;
}
int G_Cod1EntitySelectRuntimeActorDeleteOwner(uint64_t epoch,uint64_t receipt,const cod1ActorLease_t*input){
 cod1ActorLease_t lease,candidate;uint32_t row;
 if(cod1EntityDeleteBusy){cod1EntityDeleteCancelled=1;return 0;}
 if(!input||!ActorGoalBuffers(input,sizeof(*input),NULL,0,NULL,0)||DeleteTransactionPrivate(input,sizeof(*input))||!RuntimeActorDeleteIdle())return 0;
 lease=*input;
 if(!RuntimeActorDeletePrefix(epoch,receipt,&lease,&row)||!G_Cod1ActorBootstrapDeleteCandidate(&lease.entity,&candidate)||memcmp(&candidate,&lease,sizeof(lease)))return 0;
 if(cod1RuntimeDeleteOwner.epoch)return cod1RuntimeDeleteOwner.epoch==epoch&&cod1RuntimeDeleteOwner.receipt==receipt&&cod1RuntimeDeleteOwner.row==row&&!memcmp(&cod1RuntimeDeleteOwner.lease,&lease,sizeof(lease));
 cod1RuntimeDeleteOwner.epoch=epoch;cod1RuntimeDeleteOwner.receipt=receipt;cod1RuntimeDeleteOwner.row=row;cod1RuntimeDeleteOwner.lease=lease;return 1;
}
int G_Cod1EntityRuntimeActorDeleteFrameReady(uint64_t epoch){
 if(cod1EntityDeleteBusy){cod1EntityDeleteCancelled=1;return 0;}
 return epoch&&epoch==cod1EntityEpoch&&cod1RuntimeDeleteOwner.epoch==epoch;
}
/* The returned bootstrap row remains the death/free owner. Unlike a
 * mounted Avery, Foley has an independent unmounted pose and no gun. */
static int CourseWalkingDeleteEntry(const GscEntityRef *ref,const cod1ActorLease_t *lease){
 const cod1ActorRecord_t *a;const cod1ActorMotion_t *m;const gentity_t *e;
 const C1TActorPublished *p;const C1GARecord *g;
 int foley,runner;
 if(!RuntimeActorDeleteIdle()||!DeleteTransactionShape(ref,lease)||!ActorModelTopologyStageValid(lease,0))return 0;
 a=cod1Actors+lease->actorIndex;m=&a->motion;e=&g_entities[ref->slot];p=cod1UnmountedPublished+lease->actorIndex;g=cod1GenericActors+lease->actorIndex;
 foley=e->targetname && !strcmp(e->targetname,"foley") && e->classname &&
  !strcmp(e->classname,"actor_ally_airborne_officer_thompson") && G_Cod1FoleyCourseGoal(a->goal.assignment,&a->goal.node);
 runner=e->targetname && !strcmp(e->targetname,"runner") && e->classname &&
  !strcmp(e->classname,"actor_ally_airborne_garand") && G_Cod1RunnerCourseGoal(ref,a->goal.assignment,&a->goal.node);
 return (runner || foley) && !a->modelBusy && !a->modelMethodBusy &&
  a->dobj.present && a->spatial.linked && a->goal.assigned &&
  m->active && !m->model && !m->busy && m->assignment==a->goal.assignment &&
  !m->oldContents && !m->oldState.solid && (m->oldSvFlags&SVF_NOCLIENT) &&
  !(m->oldSvFlags&(SVF_BOT|SVF_CASTAI)) && !a->turretMount.mounted &&
  !a->turretMount.requestedGun.epoch && !a->turretMount.occupiedGun.epoch &&
  p->used && ActorRefEqual(ref,&p->owner) && g->used && !memcmp(&g->lease,lease,sizeof(*lease)) &&
  g->nodes && g->notify && (C1ScriptedWireValid(&e->s,level.time) ||
   (ActorLocomotionReady(ref->epoch) && C1SourcePoseWireValid(&e->s,level.time)) || (runner && TrainingElderCurrent(ref) && C1UAW_Valid(&e->s,level.time))) &&
  e->s.cod1TurretActorGeneration==ref->generation &&
  (((uint64_t)(uint32_t)e->s.cod1TurretEpochHi<<32)|(uint32_t)e->s.cod1TurretEpochLo)==ref->epoch;
}
/* A dynamically spawned jumper has no placed-map bootstrap row. Bind its
 * transaction to the factory operation and the specific notified exit goal. */
static int JumperRetireCandidate(const GscEntityRef *ref,cod1ActorLease_t *lease){
 if(!TrainingJumperAllocation(ref))return 0;
 *lease=trainingJumperOwner;return 1;
}
static int JumperRetireEntry(const GscEntityRef *ref,const cod1ActorLease_t *l){
 const cod1ActorRecord_t *a;const cod1ActorMotion_t *m;
 const C1TActorPublished *p;
 if(!TrainingJumperAllocation(ref)||memcmp(l,&trainingJumperOwner,sizeof(*l))||
    !RuntimeActorDeleteIdle()||!DeleteTransactionShape(ref,l)||!ActorModelTopologyStageValid(l,0))return 0;
 a=cod1Actors+l->actorIndex;m=&a->motion;p=cod1UnmountedPublished+l->actorIndex;
 /* close_door() removes the current jumper at carbine pickup, including
  * during flight or on the return run. The caller's genuine death
  * notification and normal free own cancellation and component release. */
 if(!m->active)return a->alive&&a->dobj.present&&!a->modelBusy&&!a->modelMethodBusy&&
   !a->turretMount.mounted&&!a->turretMount.requestedGun.epoch&&!a->turretMount.occupiedGun.epoch&&
   DeleteTransactionTopology(ref,l)?4:0;
 return a->alive&&a->dobj.present&&!a->modelBusy&&!a->modelMethodBusy&&
   a->goal.assigned&&TrainingJumperExitNode(ref,a->goal.assignment,&a->goal.node)&&
   m->active&&!m->model&&!m->busy&&m->assignment==a->goal.assignment&&
   !m->oldContents&&!m->oldState.solid&&(m->oldSvFlags&SVF_NOCLIENT)&&
   !(m->oldSvFlags&(SVF_BOT|SVF_CASTAI))&&!a->turretMount.mounted&&
   !a->turretMount.requestedGun.epoch&&!a->turretMount.occupiedGun.epoch&&
   p->used&&ActorRefEqual(ref,&p->owner)?3:0;
}
static int JumperRetireAdmit(const GscEntityRef *ref,const cod1ActorLease_t *l,cod1ActorDeleteReceipt_t *out){
 const cod1ActorRecord_t *a;
 if(!JumperRetireEntry(ref,l))return 0;
 a=cod1Actors+l->actorIndex;memset(out,0,sizeof(*out));
 out->lease=*l;out->entity=*ref;out->epoch=ref->epoch;
 out->receipt=a->spawnCompletion.operation;out->transaction=l->serial;
 return 1;
}
static int JumperRetireCurrent(const cod1ActorDeleteReceipt_t *r){
 const cod1ActorRecord_t *a;
 if(!cod1EntityDeleteBusy||r->epoch!=cod1EntityEpoch||!ActorRefEqual(&r->lease.entity,&r->entity)||
    memcmp(&r->lease,&trainingJumperOwner,sizeof(r->lease))||r->transaction!=r->lease.serial||
    !ActorLeaseValid(&r->lease))return 0;
 a=cod1Actors+r->lease.actorIndex;
 return a->spawnCompletion.serial==r->lease.serial&&r->receipt&&r->receipt==a->spawnCompletion.operation;
}
static int JumperRetireCommit(const cod1ActorDeleteReceipt_t *r){
 return r->receipt&&r->transaction==r->lease.serial&&
   !memcmp(&r->lease,&trainingJumperOwner,sizeof(r->lease))&&
   G_Cod1EntityDeleteRetiredIdentity(&r->entity,&r->lease);
}
/* The factory jumper has no placed-actor row. Its exact allocation
 * carries the normal delete transaction tombstone after G_FreeEntity. */
int G_Cod1TrainingJumperDeleteCommitted(const GscEntityRef *ref,const cod1ActorLease_t *lease){
 const Cod1DeleteAttempt *attempt;
 if(!ref||!lease||ref->slot>=MAX_GENTITIES||cod1EntityDeleteBusy||
    !ActorRefEqual(ref,&lease->entity)||memcmp(lease,&trainingJumperOwner,sizeof(*lease)))return 0;
 attempt=&cod1DeleteAttempts[ref->slot];
 return ActorRefEqual(&attempt->entity,ref)&&attempt->attempted&&attempt->notified&&attempt->retired&&
    G_Cod1EntityDeleteRetiredIdentity(ref,lease);
}
/* The shared transaction stores attempted/notified/retired per allocation. */
static void JumperRetireFail(const cod1ActorDeleteReceipt_t *r){(void)r;}
static const cod1ActorDeleteOwnerHooks_t jumperRetireHooks={
 JumperRetireCandidate,JumperRetireAdmit,JumperRetireCurrent,JumperRetireCurrent,JumperRetireCommit,JumperRetireFail
};

static int RuntimeActorDeleteEntry(const GscEntityRef*ref,const cod1ActorLease_t*lease){
 const cod1ActorRecord_t*a;const gentity_t*e;const cod1ActorMotion_t*m;C1TRecord*gun;uint32_t row;int i,found=0;
 if(TrainingJumperAllocation(ref))return JumperRetireEntry(ref,lease);
 if(CourseWalkingDeleteEntry(ref,lease))return 2;
 if(!ref||!lease||!ActorRefEqual(ref,&lease->entity)||!cod1RuntimeDeleteOwner.epoch||
  memcmp(lease,&cod1RuntimeDeleteOwner.lease,sizeof(*lease))||
  !RuntimeActorDeletePrefix(cod1RuntimeDeleteOwner.epoch,cod1RuntimeDeleteOwner.receipt,lease,&row)||row!=cod1RuntimeDeleteOwner.row||!RuntimeActorDeleteIdle()||AmmoCandidateOwned(ref))return 0;
 a=cod1Actors+lease->actorIndex;e=g_entities+ref->slot;m=&a->motion;
 if(!a->alive||!a->nativeDataInitialized||!a->dataInitialized||a->initialized||a->criticalBusy||a->spatial.busy||!a->spatial.linked||
  !a->dobj.present||a->dobj.busy||a->modelBusy||a->modelMethodBusy||!m->active||m->model||m->busy||
  m->oldContents||m->oldState.solid||!(m->oldSvFlags&SVF_NOCLIENT)||(m->oldSvFlags&(SVF_BOT|SVF_CASTAI))||
  !e->r.linked||e->r.contents!=CONTENTS_BODY||e->clipmask!=MASK_PLAYERSOLID||!e->s.solid||
  (e->r.svFlags&(SVF_BOT|SVF_CASTAI))||e->r.bmodel||e->client||e->physicsObject||e->neverFree||e->teamchain||e->teammaster||e->tagParent||e->tagName||
  (e->team&&e->team[0])||(e->spawnflags&1)||e->aiName||e->aiCharacter||e->aiAttributes||e->AIScript_AlertEntity||e->aiInactive||e->item||
  !a->turretMount.initialized||a->turretMount.serial!=lease->serial||!a->turretMount.mounted||a->turretMount.currentTop||a->turretMount.currentStack[0]!=3||
  !(gun=TurretFind(&a->turretMount.occupiedGun))||!ActorRefEqual(&gun->actor,ref))return 0;
 for(i=0;i<level.num_entities;i++)if(g_entities[i].inuse&&g_entities[i].tagParent==e)return 0;
 for(i=0;i<C1T_CAPACITY;i++)if(cod1TurretPublished[i].used&&ActorRefEqual(ref,&cod1TurretPublished[i].owner)){
  if(i!=(int)(gun-cod1Turrets))return 0;found++;
 }
 if(found!=1||!e->s.cod1TurretActorVersion||e->s.cod1TurretActorGeneration!=ref->generation)return 0;
 /* Keyed DObj links retain SVF_NOCLIENT. They are visible through the
  * mounted actor snapshot. Check the published gun and actor identities
  * before accepting this body for the existing death/free transaction. */
 if((e->r.svFlags&SVF_NOCLIENT) &&
    (!C1TAW_Valid(&e->s,level.time) ||
     (uint32_t)e->s.cod1TurretActorGun!=gun->owner.slot ||
     (uint32_t)e->s.cod1TurretActorGunGeneration!=gun->owner.generation ||
     (((uint64_t)(uint32_t)e->s.cod1TurretEpochHi<<32)|(uint32_t)e->s.cod1TurretEpochLo)!=ref->epoch))return 0;
 /* The owned graph must correspond to this exact current native allocation. */
 return cod1GenericActors[lease->actorIndex].used&&cod1GenericActors[lease->actorIndex].nodes&&cod1GenericActors[lease->actorIndex].notify&&
  !memcmp(&cod1GenericActors[lease->actorIndex].lease,lease,sizeof(*lease));
}
static int RuntimeActorDeleteQuiesce(const GscEntityRef*ref,const cod1ActorLease_t*lease,int(*current)(void*),void*ctx){
 cod1ActorRecord_t a;gentity_t e;
 int lane=RuntimeActorDeleteEntry(ref,lease);
 if(!lane||!current)return 0;
 if(lane==4)return current(ctx)==1&&JumperRetireEntry(ref,lease)==4;
 if(lane==3)return current(ctx)==1&&JumperRetireEntry(ref,lease)&&
    ActorMotionQuiesceForFree(lease,current,ctx)&&DeleteTransactionTopology(ref,lease);
 if(lane==2){
  /* Only after real death notification on the exact transaction, release
   * movement's publication. Normal actor teardown owns graph/pose release. */
  return current(ctx)==1 && CourseWalkingDeleteEntry(ref,lease) &&
    ActorMotionQuiesceForFree(lease,current,ctx) && DeleteTransactionTopology(ref,lease);
 }
a=cod1Actors[lease->actorIndex];e=g_entities[ref->slot];
 if(current(ctx)!=1||!RuntimeActorDeleteEntry(ref,lease)||memcmp(&a,&cod1Actors[lease->actorIndex],sizeof(a))||memcmp(&e,&g_entities[ref->slot],sizeof(e)))return 0;
 /* Same pure ownership retirement order as ActorRetireActor, but preserve
  * actual actor/sentient allocation, life, DObj and lease until G_FreeEntity. */
 ActorGenericRetire(lease->actorIndex,ref);TurretMountRetire(ref);
 if(!ActorMotionQuiesceForFree(lease,current,ctx))return 0;
 return DeleteTransactionTopology(ref,lease);
}
