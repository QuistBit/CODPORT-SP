/* Owned native-CRT bullet stage, real raw-mask CM and guarded damage commit.
 * Original GSC owns cadence. No implicit fire timer or player-homing target. */
#include "../bg_cod1_turret_shot.h"
#include "../bg_cod1_impact.h"
#include <stdlib.h>
#include <limits.h>
#include <errno.h>
typedef struct {int used,damage,rifle,loopRemaining,lastFrame;GscEntityRef owner;float accuracy;uint32_t serial;} C1TSRecord;
static C1TSRecord cod1TurretShots[C1T_CAPACITY];
static c1tsRandom_t cod1TurretShotRandom={1,0};
static uint64_t cod1TurretShotEpoch,cod1TurretShotBusyEpoch;
static int cod1TurretShotRifle;
static int TurretShotPrivate(const void*p,size_t n){return ActorOverlap(p,n,cod1TurretShots,sizeof(cod1TurretShots))||ActorOverlap(p,n,&cod1TurretShotRandom,sizeof(cod1TurretShotRandom))||ActorOverlap(p,n,&cod1TurretShotEpoch,sizeof(cod1TurretShotEpoch))||ActorOverlap(p,n,&cod1TurretShotBusyEpoch,sizeof(cod1TurretShotBusyEpoch))||ActorOverlap(p,n,&cod1TurretShotRifle,sizeof(cod1TurretShotRifle));}
static void TurretShotReset(void){memset(cod1TurretShots,0,sizeof(cod1TurretShots));cod1TurretShotRandom.crt=1;cod1TurretShotRandom.qrand=0;cod1TurretShotEpoch=cod1TurretShotBusyEpoch=0;cod1TurretShotRifle=0;}
static int TurretShotSpawn(C1TRecord*r){C1TSRecord v;cwDef_t d;C1TRWeapon actual;char weapon[16384],*text,*end;int bytes,index;long damage;double accuracy;
 if(!r||!level.spawning||!G_Cod1EntitySameAllocation(&r->owner)||cod1TurretShotBusyEpoch)return 0;index=(int)(r-cod1Turrets);if(index<0||index>=C1T_CAPACITY)return 0;
 if(cod1TurretShotEpoch!=cod1EntityEpoch){bytes=TurretBlob("weapons/sp/mg42_bipod_stand",weapon,sizeof(weapon));if(bytes<=0||CW_Parse(&d,"mg42_bipod_stand",weapon,bytes)<0||strcmp(CW_String(&d,"weaponType"),"bullet")||CW_Number(&d,"rifleBullet")!=1||!C1TR_Weapon(&d,&actual)||memcmp(&actual,&cod1TurretWeapon,sizeof(actual)))return 0;cod1TurretShotRifle=1;cod1TurretShotEpoch=cod1EntityEpoch;}
 memset(&v,0,sizeof(v));v.owner=r->owner;v.used=1;v.damage=cod1TurretWeapon.damage;v.accuracy=cod1TurretWeapon.accuracy;v.rifle=cod1TurretShotRifle;v.lastFrame=level.time-50;
 if(G_SpawnString("damage","",&text)){errno=0;damage=strtol(text,&end,10);if(errno==ERANGE||!*text||*end||damage>INT_MAX||damage<INT_MIN)return 0;v.damage=damage<0?0:(int)damage;}
 if(G_SpawnString("accuracy","",&text)){errno=0;accuracy=strtod(text,&end);if(errno==ERANGE||!*text||*end||!isfinite(accuracy))return 0;if(accuracy<0)accuracy=0;if(accuracy>1)accuracy=1;v.accuracy=(float)accuracy;}
 cod1TurretShots[index]=v;return 1;
}
/* Original playerinit14800: actual72E90 allocation, entity+10C,
 * team2 and threatbias1. Called only from actual selected ClientSpawn. */
int G_Cod1TurretShotPlayerBegin(const GscEntityRef*input){GscEntityRef ref;cod1ActorLease_t lease;cod1SentientRecord_t*s;unsigned index;
 if(!input||!ActorGoalBuffers(input,sizeof(*input),NULL,0,NULL,0)||input->slot!=0||!cod1EntityReady||level.spawning||level.restarted||!G_Cod1EntitySameAllocation(input))return 0;ref=*input;
 if(cod1SentientByEntity[ref.slot]){index=cod1SentientByEntity[ref.slot]-1;if(index>=COD1_SENTIENT_CAPACITY)return 0;s=&cod1Sentients[index];memset(&lease,0,sizeof(lease));lease.entity=ref;lease.serial=s->serial;lease.actorIndex=s->actorIndex;lease.sentientIndex=index;lease.kind=s->kind;return ActorLeaseValid(&lease)&&lease.kind==COD1_ACTOR_LEASE_PLAYER&&s->defaults.team==2&&s->defaults.threatbias==0x3f800000u;}
 if(!G_Cod1PlayerSentientReserve(&ref,&lease,NULL,0)||!ActorLeaseValid(&lease))return 0;s=&cod1Sentients[lease.sentientIndex];s->defaults.team=2;s->defaults.threatbias=0x3f800000u;return ActorLeaseValid(&lease)&&!memcmp(input,&ref,sizeof(ref));
}
static int TurretShotSentient(const GscEntityRef*ref,cod1ActorLease_t*lease,int*team){const cod1SentientRecord_t*s;unsigned index;if(!G_Cod1EntitySameAllocation(ref)||!cod1SentientByEntity[ref->slot])return 0;index=cod1SentientByEntity[ref->slot]-1;if(index>=COD1_SENTIENT_CAPACITY)return 0;s=&cod1Sentients[index];memset(lease,0,sizeof(*lease));lease->entity=*ref;lease->serial=s->serial;lease->actorIndex=s->actorIndex;lease->sentientIndex=index;lease->kind=s->kind;if(!ActorLeaseValid(lease)||s->defaults.team<1||s->defaults.team>3)return 0;*team=s->defaults.team;return 1;}
typedef struct {int inuse,linked,linkcount,contents,bmodel,svFlags,ownerNum,eType,model;uint32_t generation;float mins[3],maxs[3],absmin[3],absmax[3],origin[3],angles[3];} C1TSSceneRow;
static void TurretShotSceneRow(unsigned i,C1TSSceneRow*out){gentity_t*e=&g_entities[i];memset(out,0,sizeof(*out));out->inuse=e->inuse;out->generation=cod1EntityGeneration[i];if(!e->inuse)return;out->linked=e->r.linked;out->linkcount=e->r.linkcount;out->contents=e->r.contents;out->bmodel=e->r.bmodel;out->svFlags=e->r.svFlags;out->ownerNum=e->r.ownerNum;out->eType=e->s.eType;out->model=e->s.modelindex;memcpy(out->mins,e->r.mins,12);memcpy(out->maxs,e->r.maxs,12);memcpy(out->absmin,e->r.absmin,12);memcpy(out->absmax,e->r.absmax,12);memcpy(out->origin,e->r.currentOrigin,12);memcpy(out->angles,e->r.currentAngles,12);}
typedef struct {
 const GscTurretRequest*input;GscTurretRequest request;GscTurretReply*out;char*error;size_t capacity;int(*current)(void*);void*context;
 uint64_t epoch;cod1ActorLease_t lease;int attackerTeam,skipCount,victimTeam,victimHasSentient;cod1ActorLease_t skipped[13],victimLease;int skippedTeam[13];C1TRecord gun;cod1ActorTurretMount_t mount;C1TSRecord state;c1tsRandom_t random;
 int sceneCount,sceneReady,hasVictim;C1TSSceneRow scene[MAX_GENTITIES];GscEntityRef victim;int victimHealth,victimDamage;gclient_t*victimClient;
} C1TSTicket;
static int TurretShotTicketCurrent(C1TSTicket*t,int callback){C1TRecord*r;GscEntityRef actor;C1TSSceneRow row;unsigned i;int pass;
 for(pass=0;pass<(callback?2:1);pass++){
  if(t->epoch!=cod1EntityEpoch||t->request.now_ms!=level.time||!ActorLeaseValid(&t->lease)||!ActorGoalBuffers(t->input,sizeof(*t->input),t->out,sizeof(*t->out),t->error,t->capacity)||memcmp(t->input,&t->request,sizeof(t->request))||(r=TurretFind(&t->request.entity))==NULL||memcmp(r,&t->gun,sizeof(*r))||!r->animated||r->poseTick!=level.time||!TurretMountOwnerCurrent(r,level.time,&actor)||!ActorRefEqual(&actor,&t->lease.entity)||memcmp(&cod1Actors[t->lease.actorIndex].turretMount,&t->mount,sizeof(t->mount))||memcmp(&cod1TurretShots[r-cod1Turrets],&t->state,sizeof(t->state))||memcmp(&cod1TurretShotRandom,&t->random,sizeof(t->random)))return 0;
  if(cod1Sentients[t->lease.sentientIndex].defaults.team!=(unsigned)t->attackerTeam)return 0;
  for(i=0;i<(unsigned)t->skipCount;i++)if(!ActorLeaseValid(&t->skipped[i])||cod1Sentients[t->skipped[i].sentientIndex].defaults.team!=(unsigned)t->skippedTeam[i])return 0;
  if(t->victimHasSentient&&(!ActorLeaseValid(&t->victimLease)||cod1Sentients[t->victimLease.sentientIndex].defaults.team!=(unsigned)t->victimTeam))return 0;
  if(t->sceneReady){if(level.num_entities!=t->sceneCount)return 0;for(i=0;i<(unsigned)t->sceneCount;i++){TurretShotSceneRow(i,&row);if(memcmp(&row,&t->scene[i],sizeof(row)))return 0;}}
  if(t->hasVictim&&(!G_Cod1EntitySameAllocation(&t->victim)||g_entities[t->victim.slot].health!=t->victimHealth||g_entities[t->victim.slot].takedamage!=t->victimDamage||g_entities[t->victim.slot].client!=t->victimClient))return 0;
  if(pass==0&&callback&&t->current(t->context)!=1)return 0;
 }return 1;
}
static gentity_t*TurretShotAllocate(const float point[3],GscEntityRef*ref){gentity_t*e;if(G_Cod1RngOwnerPolicy()!=COD1_RNG_POLICY_FRESH_SP)return NULL;e=G_Spawn();if(!e||!G_Cod1EntityCapture(e,ref))return NULL;e->classname="tempEntity";e->eventTime=e->r.eventTime=level.time;e->freeAfterEvent=qtrue;e->r.svFlags|=SVF_NOCLIENT;G_SetOrigin(e,(float*)point);trap_LinkEntity(e);return G_Cod1EntitySameAllocation(ref)?e:NULL;}
static void TurretShotDrop(const GscEntityRef*ref){if(ref->epoch&&G_Cod1EntitySameAllocation(ref))G_FreeEntity(&g_entities[ref->slot]);}
static void TurretShotPacket(gentity_t*event,const C1TSRecord*s,int now,unsigned kind,const float point[3],const float forward[3],const float up[3]){
 event->s.eType=ET_EVENTS+EV_COD1_TURRET_SHOT;event->s.otherEntityNum=s->owner.slot;event->s.cod1TurretEpochLo=(int)(uint32_t)s->owner.epoch;event->s.cod1TurretEpochHi=(int)(uint32_t)(s->owner.epoch>>32);event->s.cod1TurretGeneration=s->owner.generation;event->s.effect1Time=s->serial;event->s.time=now;event->s.time2=(int)(C1TS_WIRE_MAGIC|kind);VectorCopy(point,event->s.origin);VectorCopy(forward,event->s.origin2);VectorCopy(up,event->s.angles2);event->r.svFlags&=~SVF_NOCLIENT;
}
static int TurretShotInvoke(const GscTurretRequest*input,GscTurretReply*out,char*error,size_t size,int(*current)(void*),void*context){
 C1TSTicket t;C1TRecord*r;C1TSRecord next;GscEntityRef actor,events[2]={{0}};gentity_t*shotEvent=NULL,*impactEvent=NULL;trace_t trace;cod1SpatialTraceQuery_t query;c1tsPlan_t plan;float fx[3][3],bullet[3][3],angles[3],axis[3][3],zero[3]={0,0,0},halfWidth=15;unsigned i,k;int status=-1,damage=0,committed=0,index,refusalLine=0;
 memset(&trace,0,sizeof(trace));
 if(!input||!out||!current||cod1TurretShotBusyEpoch||!ActorGoalBuffers(input,sizeof(*input),out,sizeof(*out),error,size)){G_Printf("[R234] MG42 early refusal line=%d time=%d\n",__LINE__,level.time);return 0;}memset(&t,0,sizeof(t));t.input=input;t.request=*input;t.out=out;t.error=error;t.capacity=size;t.current=current;t.context=context;t.epoch=cod1EntityEpoch;
 if(t.request.kind!=GSC_TURRET_SHOOT||t.request.now_ms!=level.time||!(r=TurretFind(&t.request.entity))||!TurretMountOwnerCurrent(r,level.time,&actor)||!TurretMountLease(&actor,&t.lease)||!TurretPrepare(r,level.time)||!r->animated||r->poseTick!=level.time||cod1TurretShotEpoch!=t.epoch){G_Printf("[R234] MG42 early refusal line=%d time=%d\n",__LINE__,level.time);return 0;}
 index=(int)(r-cod1Turrets);t.state=cod1TurretShots[index];if(!t.state.used||!ActorRefEqual(&t.state.owner,&t.request.entity)||t.state.serial>=INT_MAX){G_Printf("[R234] MG42 early refusal line=%d time=%d\n",__LINE__,level.time);return 0;}t.attackerTeam=cod1Sentients[t.lease.sentientIndex].defaults.team;if(t.attackerTeam<1||t.attackerTeam>3){G_Printf("[R234] MG42 early refusal line=%d time=%d\n",__LINE__,level.time);return 0;}t.gun=*r;t.mount=cod1Actors[t.lease.actorIndex].turretMount;t.random=cod1TurretShotRandom;cod1TurretShotBusyEpoch=t.epoch;
 if(!TurretShotTicketCurrent(&t,1)||!C1TP_Axis(r->pose.q[r->pose.flash],axis)){refusalLine=__LINE__;goto done;}
 for(i=0;i<3;i++)if(!C1NSO_Transform(zero,g_entities[r->owner.slot].r.currentAngles,axis[i],fx[i])){refusalLine=__LINE__;goto done;}
 for(i=0;i<3;i++)angles[i]=g_entities[r->owner.slot].r.currentAngles[i];angles[0]+=r->controls[0]+r->controls[2];angles[1]+=r->controls[1];C1TM_Axes(angles,bullet);for(k=0;k<3;k++)bullet[1][k]=-bullet[1][k];
 if(level.clients&&level.maxclients>0&&g_entities[0].inuse&&g_entities[0].client){if(g_entities[0].r.mins[0]!=-15||g_entities[0].r.maxs[0]!=15){refusalLine=__LINE__;goto done;}halfWidth=g_entities[0].r.maxs[0];}
 if(!C1TS_Plan(bullet,t.gun.flash.position,t.gun.tracking.trackedPoint,t.state.accuracy,halfWidth,t.gun.tracking.flags,&t.random,&plan)){refusalLine=__LINE__;goto done;}
 shotEvent=TurretShotAllocate(t.gun.flash.position,&events[0]);if(!shotEvent||!TurretShotTicketCurrent(&t,1)){refusalLine=__LINE__;goto done;}impactEvent=TurretShotAllocate(plan.end,&events[1]);if(!impactEvent||!TurretShotTicketCurrent(&t,1)){refusalLine=__LINE__;goto done;}
 if(level.num_entities<0||level.num_entities>MAX_GENTITIES){refusalLine=__LINE__;goto done;}t.sceneCount=level.num_entities;for(i=0;i<(unsigned)t.sceneCount;i++)TurretShotSceneRow(i,&t.scene[i]);t.sceneReady=1;
 memset(&query,0,sizeof(query));query.version=COD1_SPATIAL_VERSION;query.bytes=sizeof(query);query.contentsMask=C1TS_TRACE_MASK;query.passEntity[0]=t.request.entity.slot;query.passEntity[1]=t.lease.entity.slot;VectorCopy(t.gun.flash.position,query.start);VectorCopy(plan.end,query.end);memset(&trace,0,sizeof(trace));for(;;){GscEntityRef victim;cod1ActorLease_t victimLease;int victimTeam=0;
 status=trap_Cod1SpatialPointTrace(&query,&trace);
 if(status!=COD1_SPATIAL_OK||!isfinite(trace.fraction)||trace.fraction<0||trace.fraction>1||trace.startsolid||trace.allsolid){refusalLine=__LINE__;goto done;}for(i=0;i<3;i++)if(!C1TS_Finite(trace.endpos[i])||!isfinite(trace.plane.normal[i])){refusalLine=__LINE__;goto done;}
 /* Native contents bit0x10 enters its separate recursive response. Not
  * silently reduced to a first-surface successful shot in this domain. */
 if(trace.contents&0x10){refusalLine=__LINE__;goto done;}
 if(trace.entityNum>=0&&trace.entityNum<ENTITYNUM_WORLD){gentity_t*v=&g_entities[trace.entityNum];if(cod1ActorByEntity[trace.entityNum]){refusalLine=__LINE__;goto done;}
  if(!G_Cod1EntityCapture(v,&victim)){refusalLine=__LINE__;goto done;}
  if(v->client){if(v!=&g_entities[0]||!TurretShotSentient(&victim,&victimLease,&victimTeam)||victimLease.kind!=COD1_ACTOR_LEASE_PLAYER){refusalLine=__LINE__;goto done;}}
  if(v->takedamage&&C1TS_FriendlyPass(1,t.attackerTeam,victimTeam!=0,victimTeam)){
   if(t.skipCount>=13){refusalLine=__LINE__;goto done;}t.skipped[t.skipCount]=victimLease;t.skippedTeam[t.skipCount++]=victimTeam;
   if(!TurretShotTicketCurrent(&t,1)){refusalLine=__LINE__;goto done;}VectorCopy(trace.endpos,query.start);query.passEntity[0]=trace.entityNum;query.passEntity[1]=ENTITYNUM_NONE;continue;
  }
  if(v->takedamage){t.victim=victim;t.hasVictim=1;t.victimHealth=v->health;t.victimDamage=v->takedamage;t.victimClient=v->client;damage=t.state.damage;if(victimTeam){t.victimHasSentient=1;t.victimLease=victimLease;t.victimTeam=victimTeam;}}
 }
 break;
 }
 if(!TurretShotTicketCurrent(&t,1)){refusalLine=__LINE__;goto done;}
 next=t.state;next.serial++;next.loopRemaining=3*cod1TurretWeapon.fireMs;next.lastFrame=t.request.now_ms;cod1TurretShots[index]=next;cod1TurretShotRandom=plan.random;r=TurretFind(&t.request.entity);r->tracking.flags=plan.nativeFlags;g_entities[t.request.entity.slot].s.loopSound=1;memset(out,0,sizeof(*out));out->kind=GSC_TURRET_RESULT_VOID;committed=1;
 /* Native bullet damage precedes eventA8. No callback is inserted between
  * final preflight and the irreversible G_Damage call. */
 if(t.hasVictim){vec3_t direction;VectorCopy(plan.forward,direction);G_Damage(&g_entities[t.victim.slot],&g_entities[t.request.entity.slot],&g_entities[t.lease.entity.slot],direction,trace.endpos,damage,DAMAGE_NO_ARMOR,MOD_MACHINEGUN);}
 if(t.epoch==cod1EntityEpoch&&G_Cod1EntitySameAllocation(&events[0])&&G_Cod1EntitySameAllocation(&t.request.entity)){shotEvent=&g_entities[events[0].slot];TurretShotPacket(shotEvent,&next,t.request.now_ms,C1TS_EVENT_SHOT,t.gun.flash.position,fx[0],fx[2]);shotEvent->s.otherEntityNum2=t.lease.entity.slot;trap_LinkEntity(shotEvent);}
 if(t.epoch==cod1EntityEpoch&&G_Cod1EntitySameAllocation(&events[1])){if(trace.fraction<1&&!t.hasVictim&&G_Cod1EntitySameAllocation(&t.request.entity)){impactEvent=&g_entities[events[1].slot];impactEvent->s.eType=ET_EVENTS+EV_COD1_TURRET_IMPACT;G_SetOrigin(impactEvent,trace.endpos);CI_WriteEvent(&impactEvent->s,&trace,trace.endpos,t.gun.flash.position,plan.end,C1TS_WEAPON_CLASS,t.state.rifle);impactEvent->s.otherEntityNum=t.request.entity.slot;impactEvent->r.svFlags&=~SVF_NOCLIENT;trap_LinkEntity(impactEvent);}else TurretShotDrop(&events[1]);}

 done:if(!committed){G_Printf("[R234] MG42 shot refusal line=%d time=%d priorSerial=%u traceStatus=%d fraction=%g contents=%08x victim=%d startsolid=%d allsolid=%d skips=%d\n",refusalLine,level.time,t.state.serial,status,(double)trace.fraction,trace.contents,trace.entityNum,trace.startsolid,trace.allsolid,t.skipCount);TurretShotDrop(&events[0]);TurretShotDrop(&events[1]);}if(cod1TurretShotBusyEpoch==t.epoch)cod1TurretShotBusyEpoch=0;return committed;
}

/* Native20B10 sound-tail counter: one decrement50 per unique50ms frame,
 * initialized/refreshed by actual shots to3*WeaponDef.fireTime, not a cooldown. */
static int TurretShotFrame(int now,int(*current)(void*),void*context){uint64_t epoch=cod1EntityEpoch;unsigned i;C1TSRecord before,next;C1TRecord*r;GscEntityRef eventRef={0};gentity_t*event;float zero[3]={0,0,0},point[3];
 if(!epoch||!current||cod1TurretShotBusyEpoch||now!=level.time||!cod1EntityReady||level.spawning||level.restarted)return 0;cod1TurretShotBusyEpoch=epoch;
 for(i=0;i<C1T_CAPACITY;i++){
  before=next=cod1TurretShots[i];if(!before.used)continue;if(!(r=TurretFind(&before.owner))){memset(&cod1TurretShots[i],0,sizeof(C1TSRecord));continue;}
  if(now<before.lastFrame)goto refused;if(now-before.lastFrame<50)continue;next.lastFrame=now;
  if(before.loopRemaining>0){next.loopRemaining=before.loopRemaining-50;if(next.loopRemaining<=0){next.loopRemaining=0;VectorCopy(g_entities[before.owner.slot].r.currentOrigin,point);event=TurretShotAllocate(point,&eventRef);if(!event)goto refused;}else event=NULL;}else event=NULL;
  if(current(context)!=1||epoch!=cod1EntityEpoch||now!=level.time||!(r=TurretFind(&before.owner))||memcmp(&cod1TurretShots[i],&before,sizeof(before))){TurretShotDrop(&eventRef);goto refused;}
  cod1TurretShots[i]=next;g_entities[before.owner.slot].s.loopSound=next.loopRemaining>0?1:0;
  if(event&&G_Cod1EntitySameAllocation(&eventRef)){TurretShotPacket(&g_entities[eventRef.slot],&next,now,C1TS_EVENT_STOP,point,zero,zero);trap_LinkEntity(&g_entities[eventRef.slot]);if(epoch!=cod1EntityEpoch)goto refused;}
  memset(&eventRef,0,sizeof(eventRef));
 }
 if(cod1TurretShotBusyEpoch==epoch)cod1TurretShotBusyEpoch=0;return 1;
 refused:if(cod1TurretShotBusyEpoch==epoch)cod1TurretShotBusyEpoch=0;return 0;
}

