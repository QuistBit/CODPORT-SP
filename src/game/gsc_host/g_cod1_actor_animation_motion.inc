/* R232 real selected root/capsule/gravity owner. Shipping code uses the
 * existing port CoD Step/Slide kernel and real engine traces; no PE execution.
 * Native command/mode/root interval authority is independent of a claim of
 * complete native669B0 physics equivalence. No active prone/path AI is added. */
#include "../g_cod1_actor_animation_motion.h"
#include "../cod_step.h"
/* Private compilation of the unchanged authored CoD movement kernel avoids
 * adding new optional-owner linker requirements to old thin GAME fixtures.
 * Canonical GAME may also compile the ordinary public player kernel. */
#define add C1AM14_Add
#define sub C1AM14_Sub
#define mul C1AM14_Mul
#define divf C1AM14_Div
#define copy C1AM14_Copy
#define zero C1AM14_Zero
#define dot C1AM14_Dot
#define normalize C1AM14_Normalize
#define clip C1AM14_Clip
#define touch C1AM14_Touch
#define player_trace C1AM14_Trace
#define event C1AM14_Event
#define step_footstep C1AM14_Footstep
#define angle180 C1AM14_Angle180
#define angle_difference C1AM14_AngleDifference
#define vector_pitch C1AM14_Pitch
#define prone_slope C1AM14_ProneSlope
#define cod_slide C1AM14_Slide
#define cod_step C1AM14_Step
#include "../cod_step.c"
#undef add
#undef sub
#undef mul
#undef divf
#undef copy
#undef zero
#undef dot
#undef normalize
#undef clip
#undef touch
#undef player_trace
#undef event
#undef step_footstep
#undef angle180
#undef angle_difference
#undef vector_pitch
#undef prone_slope
#undef cod_slide
#undef cod_step
typedef struct {
 int used,tick,goalByte,blocked;cod1ActorLease_t lease;GscEntityRef gun;
 uint64_t runtime,catalog;float velocity[3],lastRoot[3];
} C1AMRecord;
static C1AMRecord cod1ActorAnimationMotions[COD1_ACTOR_CAPACITY];
static struct {int busy,cancelled;uint64_t serial;void*stage;size_t bytes;} cod1ActorAnimationMotionOperation;
static int ActorAnimationMotionPrivate(const void*p,size_t n){return ActorOverlap(p,n,cod1ActorAnimationMotions,sizeof(cod1ActorAnimationMotions))||ActorOverlap(p,n,&cod1ActorAnimationMotionOperation,sizeof(cod1ActorAnimationMotionOperation))||ActorOverlap(p,n,cod1ActorAnimationMotionOperation.stage,cod1ActorAnimationMotionOperation.bytes);}
static void ActorAnimationMotionReset(void){if(cod1ActorAnimationMotionOperation.busy)cod1ActorAnimationMotionOperation.cancelled=1;if(cod1ActorAnimationMotionOperation.serial!=UINT64_MAX)cod1ActorAnimationMotionOperation.serial++;memset(cod1ActorAnimationMotions,0,sizeof(cod1ActorAnimationMotions));}
typedef struct {
 cod1ActorLease_t lease;cod1ActorRecord_t actor;gentity_t body;C1AMRecord before;
 uint64_t epoch,serial;int now;const void*source;const void*copy;size_t bytes;
 int(*current)(void*);void*ctx;vmCvar_t gravity;int traceFailed;
} C1AMCall;
static int ActorAnimationMotionScope(C1AMCall*c){cod1ActorLease_t lease;
 return c&&cod1ActorAnimationMotionOperation.busy&&!cod1ActorAnimationMotionOperation.cancelled&&c->serial==cod1ActorAnimationMotionOperation.serial&&c->epoch==cod1EntityEpoch&&c->now==level.time&&cod1EntityReady&&!level.spawning&&!level.restarted&&ActorMapAvailable()&&TurretAnimationMapCurrent(cod1TurretAnimationReceipt)&&
  ActorGoalBuffers(c->source,c->bytes,NULL,0,NULL,0)&&!ActorAnimationMotionPrivate(c->source,c->bytes)&&!memcmp(c->source,c->copy,c->bytes)&&
  ActorGoalLease(&c->lease.entity,&lease)&&!memcmp(&lease,&c->lease,sizeof(lease))&&!memcmp(cod1Actors+lease.actorIndex,&c->actor,sizeof(c->actor))&&!memcmp(g_entities+lease.entity.slot,&c->body,sizeof(c->body))&&!memcmp(cod1ActorAnimationMotions+lease.actorIndex,&c->before,sizeof(c->before))&&!memcmp(&g_gravity,&c->gravity,sizeof(c->gravity));
}
static int ActorAnimationMotionFence(void*u){C1AMCall*c=u;return ActorAnimationMotionScope(c)&&c->current(c->ctx)==1&&ActorAnimationMotionScope(c);}
static C1AMCall*ActorAnimationMotionBegin(const GscEntityRef*owner,int now,const void*source,const void*copy,size_t bytes,int(*current)(void*),void*ctx){
 C1AMCall*c;cod1ActorLease_t lease;
 if(!ActorControllerForeignMutationAllowed())return NULL;
 if(cod1ActorAnimationMotionOperation.busy){cod1ActorAnimationMotionOperation.cancelled=1;return NULL;}
 if(cod1ActorImmediateOperation.busy){cod1ActorImmediateOperation.cancelled=1;return NULL;}
 if(cod1GenericOperation.busy){cod1GenericOperation.cancelled=1;return NULL;}
 if(!owner||!source||!copy||!bytes||!current||now<0||now!=level.time||cod1ActorAnimationMotionOperation.serial==UINT64_MAX||!ActorGoalBuffers(source,bytes,NULL,0,NULL,0)||ActorAnimationMotionPrivate(source,bytes)||!ActorGoalLease(owner,&lease))return NULL;
 c=calloc(1,sizeof(*c));if(!c)return NULL;c->lease=lease;c->actor=cod1Actors[lease.actorIndex];c->body=g_entities[owner->slot];c->before=cod1ActorAnimationMotions[lease.actorIndex];c->epoch=cod1EntityEpoch;c->now=now;c->source=source;c->copy=copy;c->bytes=bytes;c->current=current;c->ctx=ctx;c->gravity=g_gravity;
 cod1ActorAnimationMotionOperation.stage=c;cod1ActorAnimationMotionOperation.bytes=sizeof(*c);cod1ActorAnimationMotionOperation.busy=1;cod1ActorAnimationMotionOperation.cancelled=0;c->serial=++cod1ActorAnimationMotionOperation.serial;
 if(ActorAnimationMotionFence(c))return c;cod1ActorAnimationMotionOperation.stage=NULL;cod1ActorAnimationMotionOperation.bytes=0;cod1ActorAnimationMotionOperation.busy=0;free(c);return NULL;
}
static void ActorAnimationMotionEnd(C1AMCall*c){cod1ActorAnimationMotionOperation.stage=NULL;cod1ActorAnimationMotionOperation.bytes=0;cod1ActorAnimationMotionOperation.busy=0;free(c);}
static int ActorAnimationMotionCapsule(const C1AMCall*c){unsigned i;
 const gentity_t*e=&c->body;
 if(!c->actor.nativeDataInitialized||!c->actor.dataInitialized||!c->actor.alive||!c->actor.motion.active||c->actor.motion.busy||e->client||e->r.bmodel||e->physicsObject||e->neverFree||!e->r.linked||e->r.contents!=CONTENTS_BODY||e->clipmask!=MASK_PLAYERSOLID)return 0;
 for(i=0;i<3;i++)if(!isfinite(e->r.currentOrigin[i])||fabsf(e->r.currentOrigin[i])>1000000||!isfinite(e->r.mins[i])||!isfinite(e->r.maxs[i])||e->r.maxs[i]<=e->r.mins[i])return 0;
 return e->r.maxs[0]-e->r.mins[0]==e->r.maxs[1]-e->r.mins[1];
}
int G_Cod1ActorAnimationMotionScriptBegin(const GscEntityRef*input,int now,int goalByte,int(*current)(void*),void*ctx){
 GscEntityRef ref;C1AMCall*c;C1AMRecord next;C1TRecord*gun;GscEntityRef actor;int ok=0;
 if(!ActorControllerForeignMutationAllowed())return 0;
 if(!input||goalByte!=0)return 0;ref=*input;c=ActorAnimationMotionBegin(&ref,now,input,&ref,sizeof(ref),current,ctx);if(!c)return 0;
 gun=TurretFind(&c->actor.turretMount.occupiedGun);
 if(!gun||!TurretMountOwnerCurrent(gun,now,&actor)||!ActorRefEqual(&actor,&ref)||strcmp(cod1TurretWeapon.script,"mg42/stand"))goto done;
 if(c->before.used&&!memcmp(&c->before.lease,&c->lease,sizeof(c->lease))&&ActorRefEqual(&c->before.gun,&gun->owner)){ok=1;goto done;}
 memset(&next,0,sizeof(next));next.used=1;next.lease=c->lease;next.gun=gun->owner;next.tick=now;next.goalByte=goalByte;memcpy(next.velocity,c->actor.defaults.lifeWords+(0x278-0x26c)/4,12);
 if(!ActorAnimationMotionFence(c))goto done;
 /* Native570D0 resets +264 only for an actual queued script change. */
 cod1ActorAnimationMotions[c->lease.actorIndex]=next;cod1Actors[c->lease.actorIndex].nativeMode264=0;cod1Actors[c->lease.actorIndex].nativeData.entity.nativeEntityFlags&=~UINT32_C(0x10000);ok=1;
done:ActorAnimationMotionEnd(c);return ok;
}
static int ActorAnimationMotionInvoke(const GscActorAnimationRequest*input,char*error,size_t cap,int(*current)(void*),void*ctx){
 GscActorAnimationRequest q;C1AMCall*c;C1AMRecord next;C1AIRecord orient;c1ari14Result_t root;GscEntityRef ref;int mode,ok=0;
 if(!input||!ActorGoalBuffers(input,sizeof(*input),NULL,0,error,cap)||ActorAnimationMotionPrivate(input,sizeof(*input))||ActorAnimationMotionPrivate(error,cap))return 0;
 if(!ActorControllerForeignMutationAllowed())return 0;
 q=*input;if(q.method!=GSC_ACTOR_ANIMATION_ANIM_MODE||q.precision_bits!=53||q.rounding||!q.runtime_identity||!q.catalog_identity||!memchr(q.notify,0,sizeof(q.notify)))return 0;
 mode=!strcmp(q.notify,"gravity")?4:!strcmp(q.notify,"nogravity")?6:0;if(!mode)return 0;ref=q.entity;
 c=ActorAnimationMotionBegin(&ref,q.now_ms,input,&q,sizeof(q),current,ctx);if(!c)return 0;next=c->before;orient=cod1ActorImmediate[c->lease.actorIndex];
 if(!ActorAnimationMotionCapsule(c)||c->actor.turretMount.mounted||!next.used||memcmp(&next.lease,&c->lease,sizeof(c->lease))||!orient.known144||orient.cell0144||orient.runtime!=q.runtime_identity||orient.catalog!=q.catalog_identity||memcmp(&orient.lease,&c->lease,sizeof(c->lease))||c->actor.defaults.allocation.cell0134||
    (next.runtime&&(next.runtime!=q.runtime_identity||next.catalog!=q.catalog_identity)))goto done;
 if(!G_Cod1ActorRootInterval(&ref,q.now_ms,next.goalByte,&root,ActorAnimationMotionFence,c)||!ActorAnimationMotionFence(c)||root.q2[0]!=0||root.q2[1]!=1||!ActorGoalBuffers(input,sizeof(*input),NULL,0,error,cap)||ActorAnimationMotionPrivate(error,cap))goto done;
 next.tick=q.now_ms;next.runtime=q.runtime_identity;next.catalog=q.catalog_identity;
 /* Exact native68C60 two admitted mode commands clear only the native
  * nophysics flag and assign override4/6. No guessed port flag bit is used. */
 cod1ActorAnimationMotions[c->lease.actorIndex]=next;cod1Actors[c->lease.actorIndex].nativeMode264=(uint32_t)mode;cod1Actors[c->lease.actorIndex].nativeData.entity.nativeEntityFlags&=~UINT32_C(0x10000);ok=1;
done:ActorAnimationMotionEnd(c);return ok;
}
static void ActorAnimationMotionTrace(void*u,cod_trace*out,const float*start,const float*mins,const float*maxs,const float*end,int entity,uint32_t mask){
 C1AMCall*c=u;trace_t tr;unsigned i;memset(out,0,sizeof(*out));out->allsolid=out->startsolid=1;
 if(c->traceFailed||entity!=(int)c->lease.entity.slot||mask!=MASK_PLAYERSOLID||!ActorAnimationMotionFence(c)){c->traceFailed=1;return;}
 trap_TraceCapsule(&tr,start,mins,maxs,end,entity,(int)mask);
 if(!ActorAnimationMotionFence(c)||!isfinite(tr.fraction)||tr.fraction<0||tr.fraction>1){c->traceFailed=1;return;}
 for(i=0;i<3;i++)if(!isfinite(tr.endpos[i])||!isfinite(tr.plane.normal[i])){c->traceFailed=1;return;}
 out->fraction=tr.fraction;memcpy(out->end,tr.endpos,12);memcpy(out->normal,tr.plane.normal,12);out->allsolid=tr.allsolid;out->startsolid=tr.startsolid;out->entity=tr.entityNum==ENTITYNUM_WORLD?2046:tr.entityNum==ENTITYNUM_NONE?2047:tr.entityNum;
 /* The selected port mask stays in its actual namespace. Native retry bits
  * 0xc000 are not interpreted as port mover/areaportal contents. */
 out->contents=(uint32_t)tr.contents&~0xc000u;out->surface_flags=tr.surfaceFlags;
}
int G_Cod1ActorAnimationMotionFrame(const GscEntityRef*input,int now,int(*current)(void*),void*ctx){
 GscEntityRef ref;cod1ActorLease_t lease;C1AMCall*c;C1AMRecord next;cod_move move;cod_trace ground;float start[3],end[3],angles[3]={0},zero[3]={0},local[3],world[3];c1ari14Result_t root;unsigned i;uint32_t mode;int ok=0;
 if(!ActorControllerForeignMutationAllowed())return 0;
 if(!input||!current||!ActorGoalBuffers(input,sizeof(*input),NULL,0,NULL,0))return 0;ref=*input;if(!ActorGoalLease(&ref,&lease))return 0;
 if(!cod1ActorAnimationMotions[lease.actorIndex].used)return 2;
 /* Optional binding cannot supply a native mode for another allocation. */
 if(memcmp(&cod1ActorAnimationMotions[lease.actorIndex].lease,&lease,sizeof(lease)))return 0;
 if(!cod1Actors[lease.actorIndex].nativeMode264)return 2;
 c=ActorAnimationMotionBegin(&ref,now,input,&ref,sizeof(ref),current,ctx);if(!c)return 0;next=c->before;mode=c->actor.nativeMode264;
 if(!ActorAnimationMotionCapsule(c)||c->actor.turretMount.mounted||memcmp(&next.lease,&c->lease,sizeof(c->lease))||now<next.tick)goto done;
 if(now==next.tick){ok=1;goto done;}
 if(now-next.tick!=50||(mode!=4&&mode!=6)||!isfinite(c->gravity.value)||c->gravity.value<0||c->gravity.value>10000||c->gravity.value!=(float)c->gravity.integer)goto done;
 if(!G_Cod1ActorRootInterval(&ref,now,next.goalByte,&root,ActorAnimationMotionFence,c)||!ActorAnimationMotionFence(c))goto done;
 /* Root yaw changes need their native orientation consumer, not silent
  * discard. This explicit straight-delta domain admits identity only. */
 if(root.q2[0]!=0||root.q2[1]!=1)goto done;
 memcpy(local,root.translation,12);if(mode==4)local[2]=0;angles[1]=c->actor.nativeData.orientation.yaw;
 if(!C1NSO_Transform(zero,angles,local,world))goto done;
 memset(&move,0,sizeof(move));memcpy(move.origin,c->body.r.currentOrigin,12);memcpy(move.velocity,next.velocity,12);memcpy(move.mins,c->body.r.mins,12);memcpy(move.maxs,c->body.r.maxs,12);move.frame_time=.05f;move.gravity=c->gravity.integer;move.client=ref.slot;move.trace_mask=MASK_PLAYERSOLID;move.ground_entity=2047;
 if(mode==4){
  memcpy(start,move.origin,12);memcpy(end,move.origin,12);start[2]+=.25f;end[2]-=.25f;
  ActorAnimationMotionTrace(c,&ground,start,move.mins,move.maxs,end,move.client,move.trace_mask);if(c->traceFailed||ground.allsolid||ground.startsolid)goto done;
  if(ground.fraction<1&&ground.normal[2]>=.7f){move.ground_plane=1;move.ground_entity=ground.entity;memcpy(move.ground_normal,ground.normal,12);move.velocity[0]=world[0]/move.frame_time;move.velocity[1]=world[1]/move.frame_time;move.velocity[2]=0;}
  C1AM14_Step(&move,1,ActorAnimationMotionTrace,c);
 }else{for(i=0;i<3;i++)move.velocity[i]=world[i]/move.frame_time;C1AM14_Slide(&move,0,ActorAnimationMotionTrace,c);}
 if(c->traceFailed||!ActorAnimationMotionFence(c))goto done;
 for(i=0;i<3;i++)if(!isfinite(move.origin[i])||fabsf(move.origin[i])>1000000||!isfinite(move.velocity[i])||fabsf(move.velocity[i])>100000)goto done;
 next.tick=now;next.blocked=move.touch_count!=0;memcpy(next.velocity,move.velocity,12);memcpy(next.lastRoot,root.translation,12);
 cod1ActorAnimationMotions[lease.actorIndex]=next;
 memcpy(g_entities[ref.slot].r.currentOrigin,move.origin,12);memcpy(g_entities[ref.slot].s.origin,move.origin,12);memcpy(g_entities[ref.slot].s.pos.trBase,move.origin,12);g_entities[ref.slot].s.pos.trType=TR_INTERPOLATE;g_entities[ref.slot].s.pos.trTime=now;g_entities[ref.slot].s.pos.trDuration=0;memset(g_entities[ref.slot].s.pos.trDelta,0,12);
 memcpy(cod1Actors[lease.actorIndex].nativeData.entity.origin,move.origin,12);memcpy(cod1Actors[lease.actorIndex].nativeData.entity.originTrajectory,move.origin,12);memcpy(cod1Actors[lease.actorIndex].defaults.lifeWords,move.origin,12);memcpy(cod1Actors[lease.actorIndex].defaults.lifeWords+(0x278-0x26c)/4,move.velocity,12);
 /* Publication is after the atomic GAME commit. Never mutate a replacement
  * after a real LinkEntity callback, including a retired Session/map. */
 trap_LinkEntity(g_entities+ref.slot);
 if(cod1ActorAnimationMotionOperation.cancelled||c->serial!=cod1ActorAnimationMotionOperation.serial||
    cod1EntityEpoch!=c->epoch||level.time!=now||!ActorLeaseValid(&lease)||
    !ActorGoalBuffers(input,sizeof(*input),NULL,0,NULL,0)||memcmp(input,&ref,sizeof(ref)))goto done;
 c->before=next;c->body=g_entities[ref.slot];c->actor=cod1Actors[lease.actorIndex];
 ok=ActorAnimationMotionFence(c);
done:ActorAnimationMotionEnd(c);return ok;
}
