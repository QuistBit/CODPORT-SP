#include "../scripted_wire.h"
#include "../g_cod1_actor_scripted_frame.h"
/* Separate current unmounted publication; no synthetic gun or pose clock.
 * Merely publishing a selected tree never starts a locomotion GSC script. */
#include "../bg_cod1_unmounted_actor_wire.h"
static C1TActorPublished cod1UnmountedPublished[COD1_ACTOR_CAPACITY];
static uint64_t cod1UnmountedPublishBusy,cod1UnmountedPublishSerial;
static int ActorUnmountedPublisherPrivate(const void*p,size_t n){return ActorOverlap(p,n,cod1UnmountedPublished,sizeof(cod1UnmountedPublished))||ActorOverlap(p,n,&cod1UnmountedPublishBusy,sizeof(cod1UnmountedPublishBusy))||ActorOverlap(p,n,&cod1UnmountedPublishSerial,sizeof(cod1UnmountedPublishSerial));}
static void ActorUnmountedPublisherReset(void){memset(cod1UnmountedPublished,0,sizeof(cod1UnmountedPublished));if(cod1UnmountedPublishSerial!=UINT64_MAX)cod1UnmountedPublishSerial++;/* Retain a callback's busy lease until unwind. */}
static void ActorUnmountedPublisherRetire(const GscEntityRef*actor){unsigned i;gentity_t*e;C1TActorPublished old;
 if(!actor)return;for(i=0;i<COD1_ACTOR_CAPACITY;i++)if(cod1UnmountedPublished[i].used&&ActorRefEqual(actor,&cod1UnmountedPublished[i].owner)){
  old=cod1UnmountedPublished[i];memset(cod1UnmountedPublished+i,0,sizeof(old));if(!G_Cod1EntitySameAllocation(actor))continue;
  e=g_entities+actor->slot;TurretActorPublisherRestore(&e->s,&old.previous);e->flags=(e->flags&~FL_NODRAW)|(old.oldFlags&FL_NODRAW);e->r.svFlags=(e->r.svFlags&~SVF_NOCLIENT)|(old.oldSvFlags&SVF_NOCLIENT);
 }
}
static int TrainingJumperAllocation(const GscEntityRef *);
typedef struct {const GscEntityRef*borrowed;GscEntityRef actor;cod1ActorLease_t lease;cod1ActorRecord_t record;gentity_t body;C1GARecord generic;uint64_t epoch,serial;int now;int(*current)(void*);void*ctx;} C1UAPCall;
static int ActorUnmountedPublishCurrent(C1UAPCall*c){cod1ActorLease_t lease;
 if(!c||c->now!=level.time||c->epoch!=cod1EntityEpoch||c->serial!=cod1UnmountedPublishSerial||cod1UnmountedPublishBusy!=c->epoch||
    !cod1EntityReady||level.spawning||level.restarted||cod1TurretActorConfigEpoch!=c->epoch||memcmp(c->borrowed,&c->actor,sizeof(c->actor))||
    !ActorGoalLease(&c->actor,&lease)||memcmp(&lease,&c->lease,sizeof(lease))||memcmp(cod1Actors+c->lease.actorIndex,&c->record,sizeof(c->record))||
    memcmp(g_entities+c->actor.slot,&c->body,sizeof(c->body))||memcmp(cod1GenericActors+c->lease.actorIndex,&c->generic,sizeof(c->generic)))return 0;
 return !c->record.turretMount.mounted&&(c->body.r.svFlags&SVF_NOCLIENT)&&c->body.r.linked&&c->record.spatial.linked&&c->record.dobj.present&&!c->record.dobj.busy&&!c->record.dobj.summary.skippedCount&&
  c->record.dobj.summary.modelCount==c->record.modelComposition.descriptorCount;
}
static int ActorUnmountedPublishFence(void*ctx){C1UAPCall*c=ctx;return ActorUnmountedPublishCurrent(c)&&c->current(c->ctx)==1&&ActorUnmountedPublishCurrent(c);}
static int TrainingCoursePoseActive(const cod1ActorRecord_t *actor){
    return (((actor->goal.assignment==19 || actor->goal.assignment==21) && actor->motion.route.ladderFrame && !actor->motion.arrived) || (G_Cod1ElderStandingRun(actor->goal.assignment) && actor->nativeData.actor.cell0234==1 && actor->motion.moving && !actor->motion.arrived) || actor->nativeData.actor.cell0234==2 || actor->nativeData.actor.cell0234==4);
}
static void TrainingStandingPose(entityState_t *wire,const cod1ActorMotion_t *motion){
    float cycles=motion->travelled/COD1_PRECOMBAT_RUN_FORWARD;
    wire->frame=COD1_FRAME_PRECOMBAT_RUN;
    wire->cod1TurretActorPhases[1][10]=cycles-floorf(cycles);
}
static void TrainingEntryPose(entityState_t *wire,const c1CourseEntry_t *entry){
    float *phase=wire->cod1TurretActorPhases[1];
    unsigned i;
    wire->cod1TurretActorFlags|=COD1_COURSE_ENTRY_BIT;
    if(entry->shuffle==C1_SHUFFLE_B)wire->cod1TurretActorFlags|=COD1_ENTRY_SHUFFLE_B;
    phase[10]=entry->phase;
    for(i=0;i<4;i++)phase[14+i]=entry->current[i];
    phase[18]=entry->runWeight;phase[19]=entry->shuffleWeight;phase[20]=entry->shufflePhase;
}
static void TrainingCrawlPose(entityState_t *wire,const cod1ActorMotion_t *motion){
    float cycles=motion->travelled/COD1_PRONE_CRAWL_FORWARD;
    wire->frame=COD1_FRAME_PRONE_CRAWL;
    wire->cod1TurretActorPhases[1][10]=cycles-floorf(cycles);
}
static void TrainingFinishTrace(const char *name,const cod1ActorRecord_t *actor,const GscEntityRef *owner,int now){
    if(g_debugMove.integer && actor->goal.assignment==22 && actor->motion.arrived && now%500==0)
        G_Printf("[%s finish] owner=%u/%u notified=%llu interval=%.1f stance=%u\n",name,
            owner->slot,owner->generation,(unsigned long long)actor->motion.notified,
            actor->defaults.allocation.interval,(unsigned)actor->nativeData.actor.cell0234);
}
/* Nine intervals at 15 fps, with the original 50 ms incoming blend.
 * The same owner can leave the stop for its peer before the clip ends. */
static void TrainingStopPose(entityState_t *wire,const cod1LocalRoute_t *route,int now){
    int age,elapsed,fade;
    float weight;
    if(!route->stopTick || now<route->stopTick)return;
    age=now-route->stopTick;
    if(age>600)return;
    elapsed=age+route->stopFirstStep;
    if(elapsed>600)return;
    weight=fmaxf(.001f,fminf(1,age/50.f));
    if(route->stopFadeTick){
        fade=now-route->stopFadeTick;
        if(fade>=350)return;
        if(fade>50)weight*=1-(fade-50)/300.f;
        if(route->reciprocal && !route->returning && fade>=50){
            wire->frame=COD1_FRAME_CROUCH_WALK_LOOP;
            wire->cod1TurretActorPhases[1][10]=fade/1200.f;
        }
    }
    wire->cod1TurretActorFlags=COD1_COURSE_STOP_FLAGS;
    wire->cod1TurretActorPhases[1][11]=elapsed/600.f;
    wire->cod1TurretActorPhases[1][12]=weight;
}
static void TrainingTraversalPose(entityState_t *wire,const cod1ActorMotion_t *motion){
 if(motion->arrived && COD1_IS_COURSE_WALK_FRAME(motion->route.walkFrame)){
    wire->frame=motion->route.walkFrame;
    wire->cod1TurretActorPhases[1][10]=motion->route.walkPhase;
 }else if(motion->arrived && COD1_IS_REST_FRAME(motion->route.restFrame)){
    wire->frame=motion->route.restFrame;
    wire->cod1TurretActorPhases[1][10]=motion->route.restPhase;
 }
 if(motion->route.diveTime && !motion->arrived){
    wire->frame=motion->route.ladderFrame?motion->route.ladderFrame:COD1_FRAME_WALL_DIVE;
    wire->cod1TurretActorPhases[1][10]=motion->route.divePhase;
 }
}
int G_Cod1ActorUnmountedPublish(const GscEntityRef*input,int now,int(*current)(void*),void*ctx){
 C1UAPCall*c=NULL;C1TActorPublished saved,next;entityState_t wire;gentity_t*e;char text[1024],actual[1024];unsigned i;int slot=-1,ok=0,scripted;
 if(!input||!current||now!=level.time||!cod1EntityEpoch||cod1UnmountedPublishBusy||cod1UnmountedPublishSerial==UINT64_MAX||cod1TurretActorConfigBusy||
    !ActorGoalBuffers(input,sizeof(*input),NULL,0,NULL,0)||ActorUnmountedPublisherPrivate(input,sizeof(*input)))return 0;
 c=calloc(1,sizeof(*c));if(!c)return 0;c->borrowed=input;c->actor=*input;c->epoch=cod1EntityEpoch;c->serial=++cod1UnmountedPublishSerial;c->now=now;c->current=current;c->ctx=ctx;cod1UnmountedPublishBusy=c->epoch;
 if(!ActorGoalLease(&c->actor,&c->lease)||G_Cod1ActorGenericPresent(&c->actor)!=1)goto done;
 c->record=cod1Actors[c->lease.actorIndex];c->body=g_entities[c->actor.slot];c->generic=cod1GenericActors[c->lease.actorIndex];
 if(!ActorUnmountedPublishFence(c)||!TurretActorCompositionText(&c->record.modelComposition,text))goto done;
 if(TrainingJumperAllocation(&c->actor)){
    static GscEntityRef owner;
    char bound[sizeof(actual)];
    int count=snprintf(bound,sizeof(bound),"j:%u:%u:%u:%u|%s",
        (unsigned)c->epoch,(unsigned)(c->epoch>>32),c->actor.slot,c->actor.generation,text);
    if(count<0||(size_t)count>=sizeof(bound))goto done;
    slot=CS_COD1_JUMPER-CS_COD1_TURRET_ACTORS;
    trap_GetConfigstring(CS_COD1_JUMPER,actual,sizeof(actual));
    if(!ActorUnmountedPublishFence(c))goto done;
    if(!ActorRefEqual(&owner,&c->actor)){
        if(owner.epoch==c->epoch&&G_Cod1EntitySameAllocation(&owner))goto done;
        cod1TurretActorConfigBusy=c->epoch;
        trap_SetConfigstring(CS_COD1_JUMPER,bound);
        if(!ActorUnmountedPublishFence(c))goto done;
        trap_GetConfigstring(CS_COD1_JUMPER,actual,sizeof(actual));
        if(!ActorUnmountedPublishFence(c)||strcmp(actual,bound))goto done;
        owner=c->actor;cod1TurretActorConfigBusy=0;
    }
    if(strcmp(actual,bound))goto done;
 }else{
 for(i=0;i<MAX_COD1_TURRET_ACTORS;i++)if(!strcmp(cod1TurretActorConfigs[i],text)){slot=(int)i;break;}
 if(slot<0)for(i=0;i<MAX_COD1_TURRET_ACTORS;i++)if(!cod1TurretActorConfigs[i][0]){slot=(int)i;break;}
 if(slot<0)goto done;trap_GetConfigstring(CS_COD1_TURRET_ACTORS+slot,actual,sizeof(actual));if(!ActorUnmountedPublishFence(c))goto done;
 if(cod1TurretActorConfigs[slot][0]){if(strcmp(actual,text))goto done;}
 else{
  if(actual[0]&&strcmp(actual,text))goto done;cod1TurretActorConfigBusy=c->epoch;
  if(!actual[0])trap_SetConfigstring(CS_COD1_TURRET_ACTORS+slot,text);if(!ActorUnmountedPublishFence(c))goto done;
  trap_GetConfigstring(CS_COD1_TURRET_ACTORS+slot,actual,sizeof(actual));if(!ActorUnmountedPublishFence(c)||strcmp(actual,text))goto done;
  strcpy(cod1TurretActorConfigs[slot],text);cod1TurretActorConfigBusy=0;
 }
 }
 saved=next=cod1UnmountedPublished[c->lease.actorIndex];if(next.used&&!ActorRefEqual(&next.owner,&c->actor))goto done;
 if(!next.used){next.used=1;next.owner=c->actor;next.previous=c->body.s;next.oldFlags=c->body.flags;next.oldSvFlags=c->body.r.svFlags;}
 wire=c->body.s;wire.eType=ET_GENERAL;wire.cod1TurretVersion=0;wire.cod1TurretEpochLo=(int)(uint32_t)c->epoch;wire.cod1TurretEpochHi=(int)(uint32_t)(c->epoch>>32);
 scripted=ActorScriptedClockSelected(c->lease.actorIndex);
 wire.cod1TurretActorVersion=scripted?COD1_SCRIPTED_ACTOR_WIRE_VERSION:COD1_UNMOUNTED_ACTOR_WIRE_VERSION;wire.cod1TurretActorGeneration=c->actor.generation;wire.cod1TurretActorGun=-1;wire.cod1TurretActorGunGeneration=0;
 wire.cod1TurretActorTick=now;wire.cod1TurretActorConfig=CS_COD1_TURRET_ACTORS+slot;wire.cod1TurretActorFlags=1;wire.pos.trType=wire.apos.trType=TR_STATIONARY;wire.eFlags&=~EF_NODRAW;
 VectorCopy(c->body.r.currentOrigin,wire.pos.trBase);VectorCopy(c->body.r.currentAngles,wire.apos.trBase);memset(wire.cod1TurretActorCurrent,0,sizeof(wire.cod1TurretActorCurrent));
 if(scripted){
  if(!G_Cod1ActorScriptedSampleFull(&c->actor,now,wire.cod1TurretActorGenericCurrent,
      wire.cod1TurretActorPhases,wire.cod1TurretActorCurrent,ActorUnmountedPublishFence,c))goto done;
 }else if(!G_Cod1ActorUnmountedSample(&c->actor,now,wire.cod1TurretActorGenericCurrent,
      wire.cod1TurretActorPhases,ActorUnmountedPublishFence,c))goto done;
 /* The mover owns world translation. Wire phase follows only swept distance;
  * waiting for a goal, blocked motion and retirement never advance feet. */
 if(scripted && c->record.motion.active && c->record.motion.moving && !c->record.motion.blocked &&
    !c->record.motion.arrived && !c->record.motion.model && !c->record.motion.busy &&
    c->record.goal.assigned && c->record.motion.assignment==c->record.goal.assignment &&
    c->body.targetname && (!strcmp(c->body.targetname,"foley") ||
    (!strcmp(c->body.targetname,"moody") && G_Cod1MoodyCourseGoal(c->record.goal.assignment,&c->record.goal.node)))){
  float cycles=c->record.motion.travelled/63.4768f;
  if(!isfinite(cycles)||cycles<0)goto done;
  wire.cod1TurretActorFlags=3;wire.cod1TurretActorPhases[1][10]=cycles-floorf(cycles);
 }
 if(COD1_IS_RIFLE_CROUCHRUN(wire.frame) || wire.frame==COD1_FRAME_WALL_DIVE || wire.frame==COD1_FRAME_PRONE_CRAWL || wire.frame==COD1_FRAME_PRECOMBAT_RUN || COD1_IS_LADDER_FRAME(wire.frame) || COD1_IS_COURSE_WALK_FRAME(wire.frame) || COD1_IS_REST_FRAME(wire.frame))wire.frame=0;
 if(scripted && c->record.goal.assigned && c->record.motion.active && !c->record.motion.busy &&
    !c->record.motion.model && c->record.motion.assignment==c->record.goal.assignment &&
    c->body.classname && !strcmp(c->body.classname,"actor_ally_airborne_garand") &&
    c->body.targetname && !strcmp(c->body.targetname,"runner") && TrainingCoursePoseActive(&c->record) &&
    G_Cod1RunnerCourseGoal(&c->actor,c->record.goal.assignment,&c->record.goal.node)){
    float stride,cycles;
    wire.frame=Cod1CourseCrouchFrame(c->actor.slot);
    switch(wire.frame){
    case COD1_FRAME_RIFLE_CROUCHRUN_1: stride=COD1_CROUCHRUN1_FORWARD;break;
    case COD1_FRAME_RIFLE_CROUCHRUN_2: stride=73.89181f;break;
    case COD1_FRAME_RIFLE_CROUCHRUN_3: stride=COD1_CROUCHRUN3_FORWARD;break;
    case COD1_FRAME_RIFLE_CROUCHRUN_4: stride=COD1_CROUCHRUN4_FORWARD;break;
    default: goto done;
    }
    cycles=c->record.motion.travelled/stride;
    if(!isfinite(cycles)||cycles<0)goto done;
    if(g_debugMove.integer)TrainingFinishTrace(G_Cod1RunnerName(&c->actor),&c->record,&c->actor,now);
    wire.cod1TurretActorFlags=3;
    wire.cod1TurretActorPhases[1][10]=cycles-floorf(cycles);
    if(c->record.motion.route.approachTime && !c->record.motion.arrived && c->record.nativeData.actor.cell0234==2)
        wire.cod1TurretActorPhases[1][10]=c->record.motion.route.approachPhase;
    if(c->record.nativeData.actor.cell0234==4)TrainingCrawlPose(&wire,&c->record.motion);
    if(c->record.nativeData.actor.cell0234==1 ||
       (c->record.motion.route.nextRunPoseReview && !c->record.motion.route.approachTime &&
        c->record.motion.moving && !c->record.motion.arrived && c->record.nativeData.actor.cell0234==2))
        TrainingStandingPose(&wire,&c->record.motion);
    TrainingTraversalPose(&wire,&c->record.motion);
    if(wire.frame==COD1_FRAME_PRECOMBAT_RUN && c->record.motion.route.entry.elapsedMs>0 &&
       !c->record.motion.route.approachTime && c->record.motion.moving && !c->record.motion.arrived)
        TrainingEntryPose(&wire,&c->record.motion.route.entry);
    /* An allowed stance can change during the standing script's wait.
     * A crouch stop first needs a crouch pose to blend out of. */
    if((COD1_IS_RIFLE_CROUCHRUN(wire.frame) || COD1_IS_COURSE_WALK_FRAME(wire.frame) || COD1_IS_REST_FRAME(wire.frame)) &&
       c->record.motion.route.stopTick && now-c->record.motion.route.stopTick<=600 && c->record.goal.assignment==22 && c->record.nativeData.actor.cell0234==2 &&
       c->record.motion.notified==22 && MotionSharedTerminal(&c->record,&c->actor))
        TrainingStopPose(&wire,&c->record.motion.route,now);
    if(c->record.motion.route.approachTime>0 && c->record.motion.route.approachTime<600 &&
       !c->record.motion.arrived && c->record.nativeData.actor.cell0234==2 && COD1_IS_RIFLE_CROUCHRUN(wire.frame)){
        wire.cod1TurretActorFlags|=COD1_COURSE_APPROACH_BIT;
        wire.cod1TurretActorPhases[1][13]=c->record.motion.route.approachTime/600.f;
    }
 }
 if(scripted && TrainingJumperAllocation(&c->actor)){
    const c1atl14Node_t *leaf=c->generic.nodes+892;
    if(!c->generic.nodes || !C1ATL14_Finite(leaf->currentWeight)||!C1ATL14_Finite(leaf->phase)||
       leaf->currentWeight<0||leaf->currentWeight>1||leaf->phase<0||leaf->phase>1)goto done;
    if(c->record.motion.active&&c->record.motion.moving&&!c->record.motion.blocked&&
       c->record.goal.assigned&&TrainingJumperExitNode(&c->actor,c->record.goal.assignment,&c->record.goal.node)){
        wire.cod1TurretActorFlags=3;
        const c1CourseEntry_t *entry=&c->record.motion.route.entry;
        TrainingStandingPose(&wire,&c->record.motion);
        if(entry->shuffle==C1_ENTRY_FROM_LANDING&&entry->elapsedMs){
            TrainingEntryPose(&wire,entry);
            /* CGFA4_Jumper adds the actual outgoing body after the run layers. */
            wire.cod1TurretActorPhases[1][19]=0;
        }
    }
    if(g_debugMove.integer>1)
        G_Printf("[tower actor pose] tick=%d owner=%u/%u xyz=%.4f %.4f %.4f yaw=%.3f weight=%.5f phase=%.6f frame=%d gait=%.6f\n",
            now,c->actor.slot,c->actor.generation,wire.pos.trBase[0],wire.pos.trBase[1],wire.pos.trBase[2],
            wire.apos.trBase[1],leaf->currentWeight,leaf->phase,wire.frame,wire.cod1TurretActorPhases[1][10]);
    wire.effect1Time=0x4a5202;
    memcpy(&wire.effect2Time,&leaf->currentWeight,4);memcpy(&wire.effect3Time,&leaf->phase,4);
 }
 ActorHudPublish(&wire,&next,&c->lease);
 if(!(scripted?C1ScriptedWireValid(&wire,now):C1UAW_Valid(&wire,now))||!ActorUnmountedPublishCurrent(c)||memcmp(cod1UnmountedPublished+c->lease.actorIndex,&saved,sizeof(saved)))goto done;
 e=g_entities+c->actor.slot;e->s=wire;e->flags&=~FL_NODRAW;cod1UnmountedPublished[c->lease.actorIndex]=next;
 /* Origin, bounds and contents stay at the already linked actual pose.
  * The checked spatial lane remains NOCLIENT. The server admits only the
  * exact current owned unmounted wire to snapshot output; no relink is needed. */
 c->body=*e;ok=ActorUnmountedPublishCurrent(c)&&!memcmp(cod1UnmountedPublished+c->lease.actorIndex,&next,sizeof(next));
done:if(c){if(cod1TurretActorConfigBusy==c->epoch)cod1TurretActorConfigBusy=0;if(cod1UnmountedPublishBusy==c->epoch)cod1UnmountedPublishBusy=0;}free(c);return ok;
}
