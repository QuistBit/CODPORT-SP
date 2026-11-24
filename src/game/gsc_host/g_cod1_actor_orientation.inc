#include "../bg_cod1_actor_orientation14.h"
#include "../bg_cod1_actor_body_turn14.h"
/* Native look approach and override body turn. Default/path target
 * production remains separate; no synthetic GSC completion event. */
typedef struct {cod1ActorLease_t lease;uint64_t runtime,catalog;int tick,active;} C1AOClock;
typedef struct {GscEntityRef ref;cod1ActorLease_t lease;cod1ActorRecord_t actor;gentity_t body;C1GARecord graph;C1AIRecord immediate;C1AOClock clock;c1atl14Node_t nodes[C1GA_COUNT];const GscEntityRef*source;int now;uint64_t serial,locoRevision;int locoReady;int(*current)(void*);void*ctx;} C1AOCall;
static C1AOClock cod1OrientationClocks[COD1_ACTOR_CAPACITY];
static C1AOCall*cod1OrientationCall;
static uint64_t cod1OrientationSerial;
static int cod1OrientationCancelled;
static int ActorOrientationPrivate(const void*p,size_t n){return ActorOverlap(p,n,cod1OrientationClocks,sizeof(cod1OrientationClocks))||ActorOverlap(p,n,&cod1OrientationCall,sizeof(cod1OrientationCall))||ActorOverlap(p,n,&cod1OrientationSerial,sizeof(cod1OrientationSerial))||ActorOverlap(p,n,&cod1OrientationCancelled,sizeof(cod1OrientationCancelled))||(cod1OrientationCall&&ActorOverlap(p,n,cod1OrientationCall,sizeof(*cod1OrientationCall)));}
static void ActorOrientationReset(void){if(cod1OrientationCall)cod1OrientationCancelled=1;if(cod1OrientationSerial!=UINT64_MAX)cod1OrientationSerial++;memset(cod1OrientationClocks,0,sizeof(cod1OrientationClocks));}
static int ActorOrientationScope(C1AOCall*c){cod1ActorLease_t lease;C1GARecord*g;
 if(!c||cod1OrientationCall!=c||cod1OrientationCancelled||c->serial!=cod1OrientationSerial||c->now!=level.time||!cod1EntityReady||level.spawning||level.restarted||c->ref.epoch!=cod1EntityEpoch||c->locoRevision!=cod1LocoResources.revision||c->locoReady!=ActorLocomotionReady(c->ref.epoch)||!TurretAnimationMapCurrent(cod1TurretAnimationReceipt)||
    !ActorGoalBuffers(c->source,sizeof(*c->source),NULL,0,NULL,0)||memcmp(c->source,&c->ref,sizeof(c->ref))||!ActorGoalLease(&c->ref,&lease)||memcmp(&lease,&c->lease,sizeof(lease)))return 0;
 g=cod1GenericActors+c->lease.actorIndex;
 return !memcmp(&c->actor,cod1Actors+c->lease.actorIndex,sizeof(c->actor))&&!memcmp(&c->body,g_entities+c->ref.slot,sizeof(c->body))&&!memcmp(&c->immediate,cod1ActorImmediate+c->lease.actorIndex,sizeof(c->immediate))&&
  !memcmp(&c->clock,cod1OrientationClocks+c->lease.actorIndex,sizeof(c->clock))&&!memcmp(&c->graph,g,sizeof(*g))&&(!g->nodes?!g->used:!memcmp(c->nodes,g->nodes,sizeof(c->nodes)));
}
static int ActorOrientationFence(C1AOCall*c){return ActorOrientationScope(c)&&c->current(c->ctx)==1&&ActorOrientationScope(c);}
/* Rotation changes the checked engine spatial record as well as the GAME
 * entity. An already-linked non-brush actor retains its origin/bounds/solid;
 * SV_LinkEntityArea changes only linkcount in addition to the staged angle.
 * Predict that exact body before the callback, then validate the complete
 * actor/tree/body/resource snapshot. Never refresh it from borrowed output. */
static int ActorOrientationBodyLink(C1AOCall*c,const c1abt14Result_t*next){
 gentity_t*e=g_entities+c->ref.slot;vec3_t angles={0,0,0};
 if(!c->body.r.linked||c->body.r.bmodel||c->body.r.linkcount<0||c->body.r.linkcount==INT_MAX)return 0;
 angles[1]=next->yaw;
 G_SetAngle(&c->body,angles);c->body.s.apos.trType=TR_INTERPOLATE;c->body.r.linkcount++;
 G_SetAngle(e,angles);e->s.apos.trType=TR_INTERPOLATE;
 if(!MotionLink(&c->lease)||!ActorOrientationFence(c))return 0;
 return 1;
}
/* Pure numeric publication follows the successful spatial callback fence. */
static void ActorOrientationBodyCommit(C1AOCall*c,const c1abt14Result_t*next){
 cod1ActorRecord_t*a=cod1Actors+c->lease.actorIndex;vec3_t angles={0,0,0};
 angles[1]=next->yaw;
 memcpy(a->nativeData.entity.angles,angles,12);memcpy(a->nativeData.entity.angleTrajectory,angles,12);
 a->nativeData.orientation.yaw=next->yaw;a->orientBodyTick=c->now;
 a->nativeData.actor.zeroCells[4]=(uint32_t)next->start;a->nativeData.actor.zeroCells[5]=(uint32_t)next->duration;
}
static int ActorOrientationFrame(const GscEntityRef*input,int now,int(*current)(void*),void*ctx){
 C1AOCall*c=NULL;cod1ActorLease_t lease;C1AOClock next;GscEntityRef ref;float target[2],look[2];c1ao14Result_t result;unsigned index;int active,ok=0,desiredKnown;uint32_t desiredBase,desired;c1abt14Result_t bodyTurn;int turnBody=0;float bodyTarget;
 if(cod1OrientationCall){cod1OrientationCancelled=1;return 0;}
 if(!input||!current||now!=level.time||!ActorControllerForeignMutationAllowed()||cod1OrientationSerial==UINT64_MAX||!ActorGoalBuffers(input,sizeof(*input),NULL,0,NULL,0)||ActorOrientationPrivate(input,sizeof(*input)))return 0;
 ref=*input;if(!ActorGoalLease(&ref,&lease))return 0;index=lease.actorIndex;
 if(cod1GenericActors[index].used&&!cod1GenericActors[index].nodes)return 0;
 c=calloc(1,sizeof(*c));if(!c)return 0;c->ref=ref;c->locoRevision=cod1LocoResources.revision;c->locoReady=ActorLocomotionReady(ref.epoch);c->lease=lease;c->source=input;c->now=now;c->current=current;c->ctx=ctx;c->actor=cod1Actors[index];c->body=g_entities[ref.slot];c->graph=cod1GenericActors[index];c->immediate=cod1ActorImmediate[index];c->clock=cod1OrientationClocks[index];if(c->graph.nodes)memcpy(c->nodes,c->graph.nodes,sizeof(c->nodes));
 cod1OrientationCall=c;cod1OrientationCancelled=0;c->serial=++cod1OrientationSerial;
 if(!ActorOrientationFence(c))goto done;
 /* Publish the native selected target in the orientation frame, under the
  * same complete actor/graph/body fence. Its lifetime is independent of a
  * later queued state change or a read from a suspended animation script. */
 memcpy(&desiredBase,&c->actor.nativeData.orientation.yawCopy,4);
 desired=ActorDesiredAngleSelect(c->actor.defaults.allocation.cell0134,desiredBase,c->immediate.cell0140);
 desiredKnown=c->actor.nativeDataInitialized;
 if(c->actor.defaults.allocation.cell0134){
  desiredKnown=desiredKnown&&c->actor.defaults.allocation.cell0134==1&&c->immediate.knownAngle&&
   !memcmp(&c->immediate.lease,&lease,sizeof(lease))&&c->immediate.runtime==c->graph.runtime&&c->immediate.catalog==c->graph.catalog;
 }else desiredKnown=desiredKnown&&c->actor.defaults.allocation.cell0120==1;
 if(c->actor.defaults.allocation.cell0134&&c->actor.orientBodyTick!=now){
  if(!desiredKnown||c->actor.orientBodyTick>now)goto done;
  memcpy(&bodyTarget,&desired,4);
  if(!C1ABT14_Apply(c->body.r.currentAngles[1],bodyTarget,c->actor.nativeData.actor.zeroCells[3],
      (int)c->actor.nativeData.actor.zeroCells[4],(int)c->actor.nativeData.actor.zeroCells[5],now,&bodyTurn))goto done;
  turnBody=1;
 }
 active=c->graph.used&&ActorLocomotionActive(&c->graph)&&!TrainingJumperAllocation(&ref)&&!c->actor.turretMount.mounted;
 next=c->clock;
 if(!active){memset(&next,0,sizeof(next));if(!ActorOrientationFence(c)||(turnBody&&!ActorOrientationBodyLink(c,&bodyTurn)))goto done;cod1OrientationClocks[index]=next;ActorDesiredAngleCommit(cod1Actors+index,desiredKnown,desired);if(turnBody)ActorOrientationBodyCommit(c,&bodyTurn);ok=1;goto done;}
 if(c->graph.tick!=now||!c->actor.nativeDataInitialized)goto done;
 if(!next.active||memcmp(&next.lease,&lease,sizeof(lease))||next.runtime!=c->graph.runtime||next.catalog!=c->graph.catalog){memset(&next,0,sizeof(next));next.lease=lease;next.runtime=c->graph.runtime;next.catalog=c->graph.catalog;next.active=1;next.tick=now-50;}
 if(now<next.tick)goto done;if(now==next.tick){ok=1;goto done;}if(now-next.tick!=50)goto done;
 if(c->actor.defaults.allocation.cell0134){
  if(c->actor.defaults.allocation.cell0134!=1||!c->immediate.knownAngle||memcmp(&c->immediate.lease,&lease,sizeof(lease))||c->immediate.runtime!=c->graph.runtime||c->immediate.catalog!=c->graph.catalog)goto done;
  memcpy(target,&c->immediate.cell0138,4);memcpy(target+1,&c->immediate.cell013c,4);
 }else{
  if(c->actor.defaults.allocation.cell0120!=1)goto done;
  target[0]=c->actor.nativeData.orientation.pitch;target[1]=c->actor.nativeData.orientation.yaw;
 }
 look[0]=c->actor.nativeData.orientation.lookPitch;look[1]=c->actor.nativeData.orientation.lookYaw;
 if(!C1AO14_Apply(look,target,1,&result)||!ActorOrientationFence(c))goto done;
 if(turnBody&&!ActorOrientationBodyLink(c,&bodyTurn))goto done;
 next.tick=now;cod1OrientationClocks[index]=next;ActorDesiredAngleCommit(cod1Actors+index,desiredKnown,desired);if(turnBody)ActorOrientationBodyCommit(c,&bodyTurn);
 cod1Actors[index].nativeData.orientation.lookPitch=result.look[0];cod1Actors[index].nativeData.orientation.lookYaw=result.look[1];
 memcpy(cod1Actors[index].nativeData.orientation.lookforward,result.forward,12);memcpy(cod1Actors[index].nativeData.orientation.lookright,result.right,12);memcpy(cod1Actors[index].nativeData.orientation.lookup,result.up,12);ok=1;
 if(g_debugMove.integer>1)G_Printf("[380 look approach] owner=%u tick=%d override=%u target=%.6f,%.6f before=%.6f,%.6f after=%.6f,%.6f\n",ref.slot,now,c->actor.defaults.allocation.cell0134,target[0],target[1],look[0],look[1],result.look[0],result.look[1]);
done:if(cod1OrientationCall==c)cod1OrientationCall=NULL;free(c);return ok;
}
