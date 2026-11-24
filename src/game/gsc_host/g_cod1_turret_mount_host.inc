static int ActorDismountThink(cod1ActorRecord_t *,int);
static int ActorTransferClockAtMount(const cod1ActorLease_t*,int);
/* Included after TurretPrepare. Fresh selected native FSM branch, not a
 * replacement combat brain. Real allocation/lease, queue and ROOT link. */
#include "../g_cod1_turret_mount.h"
#include "../../qcommon/cod1_native_start_origin.h"
static uint64_t cod1TurretMountBusyEpoch;
static int TurretMountPrivate(const void*p,size_t n){return ActorOverlap(p,n,&cod1TurretMountBusyEpoch,sizeof(cod1TurretMountBusyEpoch));}
static void TurretMountReset(void){cod1TurretMountBusyEpoch=0;}
/* Called ONLY by real ActorReserve after its fresh zero-allocation/defaults
 * and serial assignment. Unsupported later AI/state/claimed-node writers must
 * invalidate these narrow no-writer certificates before selection. */
static void TurretMountFresh(uint32_t actorIndex,uint64_t serial){
 cod1ActorTurretMount_t*m;if(actorIndex>=COD1_ACTOR_CAPACITY||!serial)return;m=&cod1Actors[actorIndex].turretMount;memset(m,0,sizeof(*m));
 m->initialized=1;m->serial=serial;m->currentStack[0]=4;m->blocker1164Known=1;m->claimedNode40Known=1;
}
static void TurretMountRetire(const GscEntityRef*actor){
 unsigned i;if(!actor)return;TurretActorPublisherRetire(actor);TurretMountedRetire(actor);TurretFitRetire(actor);TurretAnimationRetire(actor);for(i=0;i<C1T_CAPACITY;i++)if(cod1Turrets[i].used&&ActorRefEqual(&cod1Turrets[i].actor,actor))memset(&cod1Turrets[i].actor,0,sizeof(GscEntityRef));
 for(i=0;i<COD1_ACTOR_CAPACITY;i++)if(ActorRefEqual(&cod1Actors[i].owner,actor))memset(&cod1Actors[i].turretMount,0,sizeof(cod1ActorTurretMount_t));
}
static int TurretMountLease(const GscEntityRef*actor,cod1ActorLease_t*lease){
 cod1ActorRecord_t*a;if(!ActorGoalLease(actor,lease)||!cod1EntityReady||level.spawning||level.restarted)return 0;a=&cod1Actors[lease->actorIndex];
 return a->turretMount.initialized&&a->turretMount.serial==lease->serial&&!a->criticalBusy&&a->alive&&a->nativeDataInitialized;
}
static int TurretMountNoEnemy(const GscEntityRef*actor){GscEntityRef enemy;return G_Cod1ActorReadEnemy(actor,&enemy,NULL,0)&&!enemy.epoch&&!enemy.slot&&!enemy.generation;}
static int TurretMountRelative(cod1ActorTurretMount_t*m,const GscEntityRef*actor,const GscEntityRef*gun){
 float basis[3][3],actorBasis[3][3],zero[3]={0,0,0},unit[3],delta[3];unsigned i,k;gentity_t*a,*g;
 if(!G_Cod1EntitySameAllocation(actor)||!G_Cod1EntitySameAllocation(gun))return 0;a=&g_entities[actor->slot];g=&g_entities[gun->slot];
 for(i=0;i<3;i++){if(!C1TP_Finite(a->r.currentOrigin[i],1000000)||!C1TP_Finite(g->r.currentOrigin[i],1000000)||!C1TP_Finite(a->r.currentAngles[i],3600)||!C1TP_Finite(g->r.currentAngles[i],3600))return 0;delta[i]=a->r.currentOrigin[i]-g->r.currentOrigin[i];}
 for(i=0;i<3;i++){memset(unit,0,sizeof(unit));unit[i]=1;if(!C1NSO_Transform(zero,g->r.currentAngles,unit,basis[i])||!C1NSO_Transform(zero,a->r.currentAngles,unit,actorBasis[i]))return 0;}
 for(i=0;i<3;i++){m->relativeOrigin[i]=(float)((double)basis[i][0]*delta[0]+(double)basis[i][1]*delta[1]+(double)basis[i][2]*delta[2]);for(k=0;k<3;k++)m->relativeAxis[k][i]=(float)((double)basis[i][0]*actorBasis[k][0]+(double)basis[i][1]*actorBasis[k][1]+(double)basis[i][2]*actorBasis[k][2]);}
 VectorCopy(g->r.currentOrigin,m->parentOrigin);VectorCopy(g->r.currentAngles,m->parentAngles);VectorCopy(a->r.currentOrigin,m->actorOrigin);VectorCopy(a->r.currentAngles,m->actorAngles);return 1;
}
static int TurretMountOwnerCurrent(C1TRecord*r,int now,GscEntityRef*outActor){
 cod1ActorLease_t lease;cod1ActorTurretMount_t*m;gentity_t*a,*g;
 if(!r||!outActor||now!=level.time||!cod1EntityReady||level.spawning||level.restarted||!G_Cod1EntitySameAllocation(&r->owner)||!TurretMountLease(&r->actor,&lease))return 0;
 m=&cod1Actors[lease.actorIndex].turretMount;if(!m->mounted||m->currentTop!=0||m->currentStack[0]!=3||!ActorRefEqual(&m->occupiedGun,&r->owner)||!ActorRefEqual(&m->requestedGun,&r->owner))return 0;
 a=&g_entities[lease.entity.slot];g=&g_entities[r->owner.slot];
 if(memcmp(a->r.currentOrigin,m->actorOrigin,12)||memcmp(a->r.currentAngles,m->actorAngles,12)||memcmp(g->r.currentOrigin,m->parentOrigin,12)||memcmp(g->r.currentAngles,m->parentAngles,12))return 0;
 *outActor=lease.entity;return 1;
}
/* Real CM movement/fitting caller refreshes the root-local offset AFTER its
 * committed collision-valid actor pose, as native45280 does. */
static int TurretMountRefreshRoot(C1TRecord*r,int now){cod1ActorLease_t lease;cod1ActorTurretMount_t value;if(!r||now!=level.time||!TurretMountLease(&r->actor,&lease))return 0;value=cod1Actors[lease.actorIndex].turretMount;if(!value.mounted||!ActorRefEqual(&value.occupiedGun,&r->owner)||value.currentStack[0]!=3||!TurretMountRelative(&value,&r->actor,&r->owner))return 0;cod1Actors[lease.actorIndex].turretMount=value;return 1;}
int G_Cod1TurretMountRequest(const GscEntityRef*actor,const GscEntityRef*gun,int now,int(*current)(void*),void*context,int*outResult){
 cod1ActorLease_t lease;cod1ActorTurretMount_t before,next;C1TRecord*r;GscEntityRef a,g;uint64_t epoch=cod1EntityEpoch;int accepted;
 if(!actor||!gun||!outResult||!current||!ActorGoalBuffers(actor,sizeof(*actor),outResult,sizeof(*outResult),NULL,0)||!ActorGoalBuffers(gun,sizeof(*gun),outResult,sizeof(*outResult),NULL,0)||cod1TurretMountBusyEpoch||now!=level.time||!TurretMountLease(actor,&lease))return 0;
 a=*actor;g=*gun;before=next=cod1Actors[lease.actorIndex].turretMount;r=TurretFind(&g);if(!r||!TurretMountNoEnemy(&a))return 0;
 cod1TurretMountBusyEpoch=epoch;accepted=!r->actor.epoch||ActorRefEqual(&r->actor,&a);
 if(next.mounted&&!ActorRefEqual(&next.occupiedGun,&g))goto fail;
 if(accepted)next.requestedGun=g;else memset(&next.requestedGun,0,sizeof(next.requestedGun));
 if(current(context)!=1||epoch!=cod1EntityEpoch||now!=level.time||!TurretMountLease(&a,&lease)||!(r=TurretFind(&g))||memcmp(&cod1Actors[lease.actorIndex].turretMount,&before,sizeof(before))||
    accepted!=(!r->actor.epoch||ActorRefEqual(&r->actor,&a))||!TurretMountNoEnemy(&a)||memcmp(actor,&a,sizeof(a))||memcmp(gun,&g,sizeof(g))||!ActorGoalBuffers(actor,sizeof(*actor),outResult,sizeof(*outResult),NULL,0)||!ActorGoalBuffers(gun,sizeof(*gun),outResult,sizeof(*outResult),NULL,0))goto fail;
 cod1Actors[lease.actorIndex].turretMount=next;*outResult=accepted;if(cod1TurretMountBusyEpoch==epoch)cod1TurretMountBusyEpoch=0;return 1;
 fail:if(cod1TurretMountBusyEpoch==epoch)cod1TurretMountBusyEpoch=0;return 0;
}
int G_Cod1TurretMountRead(const GscEntityRef*actor,cod1ActorTurretMount_t*out){cod1ActorLease_t lease;if(!actor||!out||!ActorGoalBuffers(actor,sizeof(*actor),out,sizeof(*out),NULL,0)||!TurretMountLease(actor,&lease))return 0;*out=cod1Actors[lease.actorIndex].turretMount;return 1;}
int G_Cod1TurretMountFrame(int now,int(*current)(void*),int(*notify)(void*,const GscEntityRef*,const char*),int(*startStand)(void*,const GscEntityRef*,const GscEntityRef*,const char*,const char*),void*context){
 uint64_t epoch=cod1EntityEpoch;unsigned i;int ok=0;
 if(!current||!notify||!startStand||!epoch||cod1TurretMountBusyEpoch||now!=level.time||!cod1EntityReady||level.spawning||level.restarted)return 0;cod1TurretMountBusyEpoch=epoch;
 for(i=0;i<COD1_ACTOR_CAPACITY;i++){
  cod1ActorRecord_t*a=&cod1Actors[i];cod1ActorLease_t lease;cod1ActorTurretMount_t before,next;cod1ActorCriticalState_t critical;cod1ActorQueuedState_t oldCritical;C1TRecord*r,oldGun,newGun;GscEntityRef actor,gun;int delivered;
  if(a->turretMount.initialized && !a->turretMount.mounted &&
     a->turretMount.currentTop==0 && a->turretMount.currentStack[0]==3){
   if(current(context)!=1 || epoch!=cod1EntityEpoch || now!=level.time ||
      !ActorDismountThink(a,now))goto done;
  }
  if(!a->turretMount.initialized||!a->turretMount.requestedGun.epoch)continue;actor=a->owner;
  if(!TurretMountLease(&actor,&lease)){TurretMountRetire(&actor);continue;}
  before=next=a->turretMount;gun=next.requestedGun;r=TurretFind(&gun);if(!r){if(next.mounted)TurretMountRetire(&actor);memset(&a->turretMount.requestedGun,0,sizeof(GscEntityRef));continue;}
  if(!next.mounted){
   if(r->actor.epoch){memset(&a->turretMount.requestedGun,0,sizeof(GscEntityRef));continue;}
   if(next.currentTop!=0||next.currentStack[0]!=4||!next.blocker1164Known||next.blocker1164||!next.claimedNode40Known||next.claimedNode40||!TurretMountNoEnemy(&actor))goto done;
   oldCritical=a->critical;critical=ActorStateQueueKernel(a);if(critical.top!=0||critical.stack[0]!=4||critical.command_count)goto done;
   /* native65387→4E020→4DEA0: real command1/state3 and prospective stack. */
   if(!Cod1ActorCriticalAppend(&critical,1,3))goto done;critical.stack[critical.top]=3;
   if(critical.command_count!=1||critical.commands[0].operation!=1||critical.commands[0].state!=3)goto done;
   /* native4DC40 operation1: state4 exit is a real no-op only while current
    * sentient+40 has the fresh known NULL certificate. DB80 starts state3. */
   next.currentStack[0]=3;critical.command_count=0;next.mounted=1;next.occupiedGun=gun;next.pendingOwnerNotify=1;next.pendingStand=1;
   if(!TurretMountRelative(&next,&actor,&gun))goto done;oldGun=newGun=*r;newGun.actor=actor;
   next.pendingStateNotify=newGun.tracking.trackingState!=0;newGun.tracking.trackingState=0;newGun.tracking.flags=(newGun.tracking.flags&~0xf8u)|0x100u;
   newGun.config.minYaw=-newGun.arc[1];newGun.config.maxYaw=newGun.arc[0];
   if(current(context)!=1||epoch!=cod1EntityEpoch||now!=level.time||!TurretMountLease(&actor,&lease)||!(r=TurretFind(&gun))||memcmp(r,&oldGun,sizeof(oldGun))||memcmp(&cod1Actors[i].turretMount,&before,sizeof(before))||memcmp(&cod1Actors[i].critical,&oldCritical,sizeof(oldCritical))||!TurretMountNoEnemy(&actor))goto done;
   if(!ActorTransferClockAtMount(&lease,now))goto done;
   ActorStateQueueCommit(cod1Actors+i,&critical);cod1Actors[i].turretMount=next;*r=newGun;
  }
  /* Pending effects are claimed before callback to prevent nested duplicate
   * scheduling. A 0 callback guarantees nondelivery and retries while owned. */
  for(;;){
   GscEntityRef owned;int kind;before=cod1Actors[i].turretMount;if(!TurretMountOwnerCurrent(r,now,&owned))goto done;
   kind=before.pendingOwnerNotify?1:before.pendingStateNotify?2:before.pendingStand?3:0;if(!kind)break;
   if(current(context)!=1||epoch!=cod1EntityEpoch||now!=level.time||!TurretMountOwnerCurrent(r,now,&owned)||memcmp(&cod1Actors[i].turretMount,&before,sizeof(before)))goto done;
   if(kind==1)cod1Actors[i].turretMount.pendingOwnerNotify=0;else if(kind==2)cod1Actors[i].turretMount.pendingStateNotify=0;else cod1Actors[i].turretMount.pendingStand=0;
   cod1TurretMountBusyEpoch=0;
   delivered=kind==3?startStand(context,&actor,&gun,"animscripts/mg42/stand","main"):notify(context,&gun,kind==1?"turretownerchange":"turretstatechange");
   if(current(context)!=1||epoch!=cod1EntityEpoch||now!=level.time||!TurretMountLease(&actor,&lease)||!(r=TurretFind(&gun))||!TurretMountOwnerCurrent(r,now,&owned))return 0;
   if(cod1TurretMountBusyEpoch)return 0;cod1TurretMountBusyEpoch=epoch;
   if(!delivered){if(kind==1)cod1Actors[i].turretMount.pendingOwnerNotify=1;else if(kind==2)cod1Actors[i].turretMount.pendingStateNotify=1;else cod1Actors[i].turretMount.pendingStand=1;goto done;}
  }
 }
 ok=1;
 done:if(cod1TurretMountBusyEpoch==epoch)cod1TurretMountBusyEpoch=0;return ok;
}
