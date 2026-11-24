/* Include after mount host. Selected native fit arithmetic plus actual bounded
 * port capsule/floor physics; not equivalence to complete original AI51180. */
#include "../g_cod1_turret_mount_fit.h"
#include "../bg_cod1_turret_height_limit.h"
#include "../bg_cod1_turret_mount.h"
#include "../bg_cod1_turret_mount_motion.h"
typedef struct {uint64_t serial;GscEntityRef gun;int lastTime,initialized,blocked;c1tmmState_t smooth;} C1TMFRecord;
static C1TMFRecord cod1TurretFits[COD1_ACTOR_CAPACITY];
static uint64_t cod1TurretFitBusyEpoch;
static int TurretFitPrivate(const void*p,size_t n){return ActorOverlap(p,n,cod1TurretFits,sizeof(cod1TurretFits))||ActorOverlap(p,n,&cod1TurretFitBusyEpoch,sizeof(cod1TurretFitBusyEpoch));}
static void TurretFitReset(void){memset(cod1TurretFits,0,sizeof(cod1TurretFits));cod1TurretFitBusyEpoch=0;}
static void TurretFitRetire(const GscEntityRef*a){unsigned i;for(i=0;i<COD1_ACTOR_CAPACITY;i++)if(ActorRefEqual(&cod1Actors[i].owner,a))memset(&cod1TurretFits[i],0,sizeof(C1TMFRecord));}
typedef struct {const GscEntityRef*borrowedInput;GscEntityRef gun,actor;cod1ActorLease_t lease;cod1TurretMountFitSample_t sample;C1TRecord before;cod1ActorTurretMount_t mount;float mins[3],maxs[3];int contents,clipmask,linked;int now;uint64_t epoch;int(*provider)(void*,const GscEntityRef*,cod1TurretMountFitSample_t*);int(*current)(void*);void*ctx;} C1TMFGuard;
static int TurretFitCapsuleCurrent(const C1TMFGuard*g){const gentity_t*a;if(!G_Cod1EntitySameAllocation(&g->actor))return 0;a=&g_entities[g->actor.slot];return !memcmp(a->r.mins,g->mins,12)&&!memcmp(a->r.maxs,g->maxs,12)&&a->r.contents==g->contents&&a->clipmask==g->clipmask&&a->r.linked==g->linked;}
static int TurretFitCurrent(C1TMFGuard*g){C1TRecord*r;GscEntityRef a;cod1TurretMountFitSample_t fresh;
 if(!TurretFitCapsuleCurrent(g)||!ActorGoalBuffers(g->borrowedInput,sizeof(*g->borrowedInput),NULL,0,NULL,0)||memcmp(g->borrowedInput,&g->gun,sizeof(g->gun))||g->epoch!=cod1EntityEpoch||g->now!=level.time||!ActorLeaseValid(&g->lease)||(r=TurretFind(&g->gun))==NULL||memcmp(r,&g->before,sizeof(*r))||!TurretMountOwnerCurrent(r,g->now,&a)||!ActorRefEqual(&a,&g->actor)||memcmp(&cod1Actors[g->lease.actorIndex].turretMount,&g->mount,sizeof(g->mount)))return 0;
 if(g->current(g->ctx)!=1)return 0;
 if(!TurretFitCapsuleCurrent(g)||!ActorGoalBuffers(g->borrowedInput,sizeof(*g->borrowedInput),NULL,0,NULL,0)||memcmp(g->borrowedInput,&g->gun,sizeof(g->gun))||g->epoch!=cod1EntityEpoch||!ActorLeaseValid(&g->lease)||(r=TurretFind(&g->gun))==NULL||memcmp(r,&g->before,sizeof(*r)))return 0;
 memset(&fresh,0,sizeof(fresh));if(g->provider(g->ctx,&g->gun,&fresh)!=1||memcmp(&fresh,&g->sample,sizeof(fresh)))return 0;
 return TurretFitCapsuleCurrent(g)&&ActorGoalBuffers(g->borrowedInput,sizeof(*g->borrowedInput),NULL,0,NULL,0)&&!memcmp(g->borrowedInput,&g->gun,sizeof(g->gun))&&g->epoch==cod1EntityEpoch&&g->now==level.time&&ActorLeaseValid(&g->lease)&&(r=TurretFind(&g->gun))!=NULL&&!memcmp(r,&g->before,sizeof(*r))&&TurretMountOwnerCurrent(r,g->now,&a)&&ActorRefEqual(&a,&g->actor)&&!memcmp(&cod1Actors[g->lease.actorIndex].turretMount,&g->mount,sizeof(g->mount));
}
static int TurretFitArithmetic(C1TMFGuard*g,C1TMFRecord*n,uint32_t*flags,c1tmGoal_t*goal,c1tmmMotion_t*motion){int ok;
#if defined(_MSC_VER)&&defined(_M_IX86)
 unsigned char outer[108];uint32_t savedMx,nearestMx=0x1f80u;const unsigned short nearest=0x027f;
 __asm fnsave outer
 __asm stmxcsr savedMx
 __asm fldcw nearest
 __asm ldmxcsr nearestMx
 ok=C1TM_SolveGoal(g->mount.parentOrigin,g->mount.parentAngles,g->sample.tagWeapon,g->sample.rootRotation,g->sample.rootTranslation,g->mount.relativeOrigin[2],goal)&&C1TMM_Step50(&n->smooth,flags,g->mount.actorOrigin,g->mount.actorAngles,goal->origin,goal->angles,motion);
 __asm ldmxcsr savedMx
 __asm frstor outer
#else
 ok=0;
#endif
 return ok;
}
static int TurretFitFloor(C1TMFGuard*g,const vec3_t at,vec3_t floor){trace_t tr;vec3_t start,end;gentity_t*a=&g_entities[g->actor.slot];VectorCopy(at,start);VectorCopy(at,end);start[2]+=18;end[2]-=128;trap_TraceCapsule(&tr,start,a->r.mins,a->r.maxs,end,g->actor.slot,MASK_PLAYERSOLID);if(!TurretFitCurrent(g))return 0;if(tr.startsolid||tr.allsolid||!isfinite(tr.fraction)||tr.fraction<0||tr.fraction>=1||!isfinite(tr.plane.normal[2])||tr.plane.normal[2]<.7f)return -1;for(unsigned k=0;k<3;k++)if(!isfinite(tr.endpos[k]))return -1;VectorCopy(tr.endpos,floor);return 1;}
static int TurretFitSweep(C1TMFGuard*g,const vec3_t start,const vec3_t end,int down){trace_t tr;gentity_t*a=&g_entities[g->actor.slot];trap_TraceCapsule(&tr,start,a->r.mins,a->r.maxs,end,g->actor.slot,MASK_PLAYERSOLID);if(!TurretFitCurrent(g))return 0;if(tr.startsolid||tr.allsolid||!isfinite(tr.fraction)||tr.fraction<0||tr.fraction>1)return -1;if(down)return fabsf(tr.endpos[2]-end[2])<=.1f?1:-1;return tr.fraction==1?1:-1;}
int G_Cod1TurretMountFit(const GscEntityRef*gun,int now,int(*provider)(void*,const GscEntityRef*,cod1TurretMountFitSample_t*),int(*current)(void*),void*ctx){
 C1TMFGuard guard;C1TMFRecord next;C1TRecord*r;GscEntityRef actor;cod1ActorLease_t lease;c1tmGoal_t goal;c1tmmMotion_t motion;c1thlState_t limit;vec3_t origin,desired,ground,up,over;int result;unsigned k;uint32_t flags;
 if(!gun||!provider||!current||cod1TurretFitBusyEpoch||!ActorGoalBuffers(gun,sizeof(*gun),NULL,0,NULL,0)||!(r=TurretFind(gun))||!TurretMountOwnerCurrent(r,now,&actor)||!TurretMountLease(&actor,&lease))return 0;
 memset(&guard,0,sizeof(guard));guard.borrowedInput=gun;guard.gun=*gun;guard.actor=actor;guard.lease=lease;guard.before=*r;guard.mount=cod1Actors[lease.actorIndex].turretMount;guard.now=now;guard.epoch=cod1EntityEpoch;guard.provider=provider;guard.current=current;guard.ctx=ctx;cod1TurretFitBusyEpoch=guard.epoch;
 VectorCopy(g_entities[actor.slot].r.mins,guard.mins);VectorCopy(g_entities[actor.slot].r.maxs,guard.maxs);guard.contents=g_entities[actor.slot].r.contents;guard.clipmask=g_entities[actor.slot].clipmask;guard.linked=g_entities[actor.slot].r.linked;
 if(!guard.linked||g_entities[actor.slot].client||g_entities[actor.slot].r.bmodel)goto lost;
 for(k=0;k<3;k++)if(!isfinite(guard.mins[k])||!isfinite(guard.maxs[k])||guard.mins[k]>=guard.maxs[k])goto lost;

 if(provider(ctx,&guard.gun,&guard.sample)!=1||!guard.sample.revision||guard.sample.tick!=now||!ActorRefEqual(&guard.sample.gun,&guard.gun)||!ActorRefEqual(&guard.sample.actor,&actor)||!TurretFitCurrent(&guard))goto lost;
 next=cod1TurretFits[lease.actorIndex];if(!next.initialized||next.serial!=lease.serial||!ActorRefEqual(&next.gun,&guard.gun)){memset(&next,0,sizeof(next));next.initialized=1;next.serial=lease.serial;next.gun=guard.gun;next.lastTime=now-50;}
 if(now<next.lastTime)goto lost;if(now-next.lastTime<50)goto done;
 flags=guard.before.tracking.flags;limit.flags=flags;limit.pitchLimit=guard.before.tracking.pitchLimit;
 /* Original45BF0 supplies model-local tags here. World-space shot/muzzle
  * composition is separate and must not enter the relative-height limiter. */
 if(guard.sample.endpoint){if(!C1THL_Update(&limit,guard.sample.endpoint,guard.before.controls[0],next.smooth.positionOffset,&guard.sample.tagWeapon[12],&guard.sample.tagAim[12],guard.mount.relativeOrigin[2],guard.sample.endpointRootZ))goto lost;}
 else if(!C1THL_Interior(&limit,guard.before.config.minPitch,guard.before.config.maxPitch,next.smooth.positionOffset))goto lost;
 flags=limit.flags;
 if(!TurretFitArithmetic(&guard,&next,&flags,&goal,&motion))goto lost;
 VectorCopy(guard.mount.actorOrigin,origin);VectorCopy(origin,desired);desired[0]+=motion.movementDelta[0];desired[1]+=motion.movementDelta[1];result=TurretFitFloor(&guard,desired,ground);if(!result)goto lost;if(result<0||fabsf(ground[2]-origin[2])>18.1f)goto blocked;
 result=TurretFitSweep(&guard,origin,ground,0);if(!result)goto lost;if(result<0){VectorCopy(origin,up);up[2]+=18;result=TurretFitSweep(&guard,origin,up,0);if(!result)goto lost;if(result<0)goto blocked;VectorCopy(ground,over);over[2]+=18;result=TurretFitSweep(&guard,up,over,0);if(!result)goto lost;if(result<0)goto blocked;result=TurretFitSweep(&guard,over,ground,1);if(!result)goto lost;if(result<0)goto blocked;}
 if(!TurretFitCurrent(&guard))goto lost;r=TurretFind(&guard.gun);next.blocked=0;next.lastTime=now;cod1TurretFits[lease.actorIndex]=next;r->tracking.flags=flags;r->tracking.pitchLimit=limit.pitchLimit;
 G_SetOrigin(&g_entities[actor.slot],ground);G_SetAngle(&g_entities[actor.slot],motion.angles);g_entities[actor.slot].s.pos.trType=TR_INTERPOLATE;g_entities[actor.slot].s.apos.trType=TR_INTERPOLATE;
 VectorCopy(ground,cod1Actors[lease.actorIndex].nativeData.entity.origin);VectorCopy(ground,cod1Actors[lease.actorIndex].nativeData.entity.originTrajectory);VectorCopy(motion.angles,cod1Actors[lease.actorIndex].nativeData.entity.angles);VectorCopy(motion.angles,cod1Actors[lease.actorIndex].nativeData.entity.angleTrajectory);cod1Actors[lease.actorIndex].nativeData.orientation.yaw=motion.angles[1];cod1Actors[lease.actorIndex].nativeData.orientation.yawCopy=motion.angles[1];
 if(!TurretMountRefreshRoot(r,now))goto lost;
 /* Ordinary SV_LinkEntity unlinks first and discards the keyed actor's
  * spatial registration. Keep the current DObj across both mount states. */
 if(cod1Actors[lease.actorIndex].motion.active && !cod1Actors[lease.actorIndex].motion.model){
  if(!MotionLink(&lease))goto lost;
 }else trap_LinkEntity(&g_entities[actor.slot]);
 if(current(ctx)!=1||!TurretFitCapsuleCurrent(&guard)||!ActorGoalBuffers(gun,sizeof(*gun),NULL,0,NULL,0)||memcmp(gun,&guard.gun,sizeof(guard.gun))||guard.epoch!=cod1EntityEpoch||now!=level.time||!ActorLeaseValid(&lease)||!(r=TurretFind(&guard.gun))||!TurretMountOwnerCurrent(r,now,&actor))goto lost;goto done;
 blocked:if(!TurretFitCurrent(&guard))goto lost;next.blocked=1;next.lastTime=now;cod1TurretFits[lease.actorIndex]=next;r=TurretFind(&guard.gun);r->tracking.flags=flags;r->tracking.pitchLimit=limit.pitchLimit;
 done:if(cod1TurretFitBusyEpoch==guard.epoch)cod1TurretFitBusyEpoch=0;return 1;
 lost:if(cod1TurretFitBusyEpoch==guard.epoch)cod1TurretFitBusyEpoch=0;return 0;
}
