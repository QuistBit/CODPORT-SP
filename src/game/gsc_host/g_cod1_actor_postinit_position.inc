#include "../g_cod1_actor_bootstrap.h"
#include "../g_cod1_spawner.h"
#include "../g_cod1_actor_postinit_position.h"
#include "../g_cod1_actor_postinit_target.h"
/* No callbacks from row admission through commit. Session current was checked
 * by the actual bootstrap owner immediately before this synchronous handoff. */
static int ActorPostinitPositionSelected(const cod1ActorLease_t*input,
 GscSession*session,uint64_t id,const GscEntityRef*source){
 cod1ActorLease_t lease;cod1ActorRecord_t*a;cod1SentientRecord_t*s;
 C1PostinitPosition v;uint32_t cursor;unsigned i;
 if(!input||ActorGoalPrivate(input,sizeof(*input)))return 0;lease=*input;
 if(!ActorMapAvailable()||!cod1EntityReady||level.spawning||level.restarted||
    lease.kind!=COD1_ACTOR_LEASE_ACTOR||!ActorLeaseValid(&lease)||
    (session?(!source||!EntityValidate(NULL,source)||!G_Cod1SpawnerIntakeIdentity(source)||
     !gsc_session_spawn_issued_owner_program(session,id,source,&lease.entity,lease.serial)):
     !G_Cod1ActorBootstrapPostinitCurrent(&lease)))return 0;
 a=cod1Actors+lease.actorIndex;s=cod1Sentients+lease.sentientIndex;
 if(a->positionPostinit||!a->alive||a->initialized||!a->dataInitialized||!a->nativeDataInitialized||
    a->spatial.busy||!a->spatial.linked||a->dobj.busy||a->modelBusy||a->modelMethodBusy||
    a->motion.active||a->goal.assigned||a->criticalBusy||a->scriptStateBusy||
    !s->enemyKnown||s->enemy.epoch||s->enemy.slot||s->enemy.generation)return 0;
 memset(&v,0,sizeof(v));
 for(i=0;i<3;i++){
  if(a->nativeData.entity.origin[i]!=g_entities[lease.entity.slot].r.currentOrigin[i]||
     a->nativeData.entity.angles[i]!=g_entities[lease.entity.slot].r.currentAngles[i])return 0;
 }
 memcpy(v.position,a->nativeData.entity.origin,12);memcpy(v.goal,s->rawGoal.position,12);
 v.yaw=a->nativeData.entity.angles[1];v.radius=s->defaults.goalradius;v.cell20=s->defaults.cell0020;
 v.node8=s->rawGoal.entityGoal.epoch||s->rawGoal.entityGoal.slot||s->rawGoal.entityGoal.generation;v.node40=s->rawGoal.node0040;v.node4c=s->rawGoal.node004c;
 v.path6f4=a->defaults.pathWords[(0x6f4-0x374)/4];
 cursor=a->defaults.pathWords[(0x6f8-0x374)/4];v.cursor6fa=(int16_t)(cursor>>16);
 v.cell774=a->defaults.cell0774;v.cell7e4=a->rawGoalCell07e4;v.cell78f=a->rawGoalCell078f;
 if(!C1PostinitPositionApply(&v))return 0;
 memcpy(s->rawGoal.position,v.goal,12);memset(&s->rawGoal.entityGoal,0,sizeof(s->rawGoal.entityGoal));s->defaults.cell0020=v.cell20;
 a->defaults.pathWords[(0x6f4-0x374)/4]=v.path6f4;a->defaults.cell0774=v.cell774;
 a->rawGoalCell07e4=v.cell7e4;a->rawGoalCell078f=v.cell78f;a->positionPostinit=1;
 return 1;
}

int G_Cod1ActorPostinitPosition(const cod1ActorLease_t*lease){
 return ActorPostinitPositionSelected(lease,NULL,0,NULL);
}
int G_Cod1ActorSpawnPostinitPositionSession(GscSession*s,uint64_t id,
 const GscEntityRef*source,const cod1ActorLease_t*input){
 GscEntityRef copied;cod1ActorLease_t lease;
 if(!s||!source||!input||ActorGoalPrivate(source,sizeof(*source))||
    ActorGoalPrivate(input,sizeof(*input))||
    !gsc_session_spawn_entry_storage_separate(s,source,sizeof(*source))||
    !gsc_session_spawn_entry_storage_separate(s,input,sizeof(*input)))return 0;
 copied=*source;lease=*input;
 return ActorPostinitPositionSelected(&lease,s,id,&copied);
}

/* Factory/source/lease checks below precede this callback-free live commit.
 * Only the native numeric empty-path lane is supported; named/empty targets
 * refuse before any stores. The consumed bit prevents replay after INIT fails. */
static int ActorSpawnNumericTarget(cod1ActorRecord_t*a,const cod1ActorLease_t*l){
 cod1SentientRecord_t*s=cod1Sentients+l->sentientIndex;C1PostinitPosition v;
 float target[3];unsigned i;uint32_t cursor;
 if(a->numericTargetApplied||s->rawGoal.entityGoal.epoch||s->rawGoal.entityGoal.slot||s->rawGoal.entityGoal.generation||
    !C1TargetParse(g_entities[l->entity.slot].target,target)||
    !s->enemyKnown||s->enemy.epoch||s->enemy.slot||s->enemy.generation)return 0;
 memset(&v,0,sizeof(v));
 for(i=0;i<3;i++){
  if(a->nativeData.entity.origin[i]!=g_entities[l->entity.slot].r.currentOrigin[i])return 0;
  v.position[i]=g_entities[l->entity.slot].r.currentOrigin[i];
 }
 memcpy(v.goal,s->rawGoal.position,12);v.radius=s->defaults.goalradius;v.cell20=s->defaults.cell0020;
 v.node8=s->rawGoal.entityGoal.epoch||s->rawGoal.entityGoal.slot||s->rawGoal.entityGoal.generation;v.node40=s->rawGoal.node0040;v.node4c=s->rawGoal.node004c;
 v.path6f4=a->defaults.pathWords[(0x6f4-0x374)/4];
 cursor=a->defaults.pathWords[(0x6f8-0x374)/4];v.cursor6fa=(int16_t)(cursor>>16);
 v.cell774=a->defaults.cell0774;v.cell7e4=a->rawGoalCell07e4;v.cell78f=a->rawGoalCell078f;
 if(!C1PostinitTargetApply(&v,target))return 0;
 memcpy(s->rawGoal.position,v.goal,12);memset(&s->rawGoal.entityGoal,0,sizeof(s->rawGoal.entityGoal));s->defaults.cell0020=v.cell20;
 a->defaults.pathWords[(0x6f4-0x374)/4]=v.path6f4;a->defaults.cell0774=v.cell774;
 a->rawGoalCell07e4=v.cell7e4;a->rawGoalCell078f=v.cell78f;a->numericTargetApplied=1;
 return 1;
}
/* Original INIT follows the live optional target prefix synchronously. */
int G_Cod1ActorSpawnInitSession(GscSession*s,uint64_t id,const GscEntityRef*source,
 const cod1ActorLease_t*input,GscEntityScriptRun*out,char*error,size_t cap){
 GscEntityRef src;cod1ActorLease_t lease;cod1ActorRecord_t*a;int ok;
 if(!s||!source||!input||!out||!ActorArguments(input,sizeof(*input),out,sizeof(*out),error,cap)||
    ActorGoalPrivate(source,sizeof(*source))||
    !gsc_session_spawn_entry_storage_separate(s,source,sizeof(*source))||
    !gsc_session_spawn_entry_storage_separate(s,input,sizeof(*input))||
    !gsc_session_spawn_entry_storage_separate(s,out,sizeof(*out))||
    !gsc_session_spawn_entry_storage_separate(s,error,cap)||
    ActorOverlap(source,sizeof(*source),out,sizeof(*out))||ActorOverlap(input,sizeof(*input),out,sizeof(*out))||
    ActorOverlap(error,cap,source,sizeof(*source)))return 0;
 src=*source;lease=*input;
 if(!ActorMapAvailable()||!cod1EntityReady||level.spawning||level.restarted||
    lease.kind!=COD1_ACTOR_LEASE_ACTOR||!ActorLeaseValid(&lease)||
    !EntityValidate(NULL,&src)||!G_Cod1SpawnerIntakeIdentity(&src)||
    !gsc_session_spawn_issued_owner_program(s,id,&src,&lease.entity,lease.serial)){
    EntityError(error,cap,"spawn INIT requires its current factory/source/Actor lease");return 0;}
 a=cod1Actors+lease.actorIndex;
 if(!a->positionPostinit||!a->alive||a->initialized||!a->nativeDataInitialized||
    a->spatial.busy||!a->spatial.linked||a->dobj.busy||a->modelBusy||a->modelMethodBusy||
    a->motion.active||a->goal.assigned||a->criticalBusy||a->scriptStateBusy||
    a->numericTargetApplied){
    EntityError(error,cap,"spawn INIT requires completed position and unused numeric target prefix");return 0;}
 if(g_entities[lease.entity.slot].target&&!ActorSpawnNumericTarget(a,&lease)){
    EntityError(error,cap,"spawn target736A0 requires supported finite numeric empty-path target; named target remains open");return 0;}
 ok=gsc_session_execute_spawn_init(s,id,&src,&lease.entity,lease.serial,out,error,cap);
 if(ok&&(!ActorLeaseValid(&lease)||!EntityValidate(NULL,&src)||
    !gsc_session_spawn_issued_owner_program(s,id,&src,&lease.entity,lease.serial))){
    EntityError(error,cap,"spawn INIT owner retired during script");return 0;}
 return ok;
}
