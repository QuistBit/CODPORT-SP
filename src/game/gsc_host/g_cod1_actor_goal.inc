int TrainingJumperExitNode(const GscEntityRef *,uint64_t,const GscNodeRef *);
#include "../g_cod1_node_host.h"
static int cod1ActorGoalBusy,cod1ActorGoalCancelled;
static int ActorGoalPrivate(const void *p,size_t n) {
    return (n&&(!p||n>UINTPTR_MAX-(uintptr_t)p))||
        ActorOverlap(p,n,cod1Actors,sizeof(cod1Actors))||ActorOverlap(p,n,cod1Sentients,sizeof(cod1Sentients))||
        ActorOverlap(p,n,g_entities,sizeof(g_entities))||ActorOverlap(p,n,g_clients,sizeof(g_clients))||
        ActorOverlap(p,n,&level,sizeof(level))||ActorOverlap(p,n,&cod1ActorGoalBusy,sizeof(cod1ActorGoalBusy))||
        ActorOverlap(p,n,&cod1ActorGoalCancelled,sizeof(cod1ActorGoalCancelled))||
        ActorOverlap(p,n,G_Cod1ActorGoalHost(),sizeof(GscActorGoalHost))||G_Cod1EntityStorageProtected(p,n);
}
static int ActorGoalBuffers(const void *p,size_t n,void *out,size_t bytes,char *error,size_t size) {
    return !ActorGoalPrivate(p,n)&&!ActorGoalPrivate(out,bytes)&&!ActorGoalPrivate(error,size)&&
        !ActorOverlap(p,n,out,bytes)&&ActorArguments(p,n,out,bytes,error,size);
}
static int ActorGoalLease(const GscEntityRef *ref,cod1ActorLease_t *lease) {
    unsigned i;cod1ActorRecord_t *a;
    if(!ref||!G_Cod1EntitySameAllocation(ref)||ref->slot>=MAX_GENTITIES||!cod1ActorByEntity[ref->slot])return 0;
    i=cod1ActorByEntity[ref->slot]-1;if(i>=COD1_ACTOR_CAPACITY)return 0;a=&cod1Actors[i];
    memset(lease,0,sizeof(*lease));lease->entity=*ref;lease->actorIndex=i;lease->sentientIndex=a->sentientIndex;
    lease->serial=a->serial;lease->kind=COD1_ACTOR_LEASE_ACTOR;
    return ActorLeaseValid(lease)&&a->alive&&a->nativeDataInitialized&&!a->initialized&&
        !a->spatial.busy&&!a->dobj.busy&&!a->modelBusy&&!a->modelMethodBusy;
}
#include "../g_cod1_actor_postinit_target.h"
/* Read the supported empty-path, unclaimed, no-enemy branch of 739A0.
 * A goal command can replace a moving actor's previous scripted assignment. */
static int ActorGoalPosition(const cod1ActorLease_t *l,C1PostinitPosition *v) {
    const cod1ActorRecord_t *a=cod1Actors+l->actorIndex;
    const cod1SentientRecord_t *s=cod1Sentients+l->sentientIndex;
    GscEntityRef empty={0};
    if (!s->enemyKnown || memcmp(&s->enemy,&empty,sizeof(empty)) ||
        memcmp(&s->rawGoal.entityGoal,&empty,sizeof(empty)) || s->rawGoal.node0040 || s->rawGoal.node004c ||
        memcmp(a->nativeData.entity.origin,g_entities[l->entity.slot].r.currentOrigin,12)) return 0;
    memset(v,0,sizeof(*v));
    memcpy(v->position,a->nativeData.entity.origin,12);memcpy(v->goal,s->rawGoal.position,12);
    v->cell20=s->defaults.cell0020;v->radius=s->defaults.goalradius;
    v->path6f4=a->defaults.pathWords[(0x6f4-0x374)/4];
    v->cursor6fa=(int16_t)(a->defaults.pathWords[(0x6f8-0x374)/4]>>16);
    v->cell774=a->defaults.cell0774;v->cell7e4=a->rawGoalCell07e4;v->cell78f=a->rawGoalCell078f;
    return 1;
}
/* A completed waiter can select the next node and retire the actor in one
 * VM frame. The goal and its motion owner must agree before either is observable. */
static void ActorMotionAdoptGoal(cod1ActorMotion_t *motion,uint64_t assignment,unsigned nextStep) {
    int traversal=motion->route.atTraversalStart && motion->arrived &&
        motion->notified==motion->assignment && assignment==motion->assignment+1;
    int leavingLadder=motion->arrived && motion->notified==motion->assignment &&
        assignment==motion->assignment+1 && motion->route.turnCount==1 && motion->route.ladderReady;
    int shuffle=leavingLadder?motion->route.entry.shuffle:0;
    int carryBegin=nextStep==19 && motion->route.approachingLadder &&
        !motion->arrived && motion->notified==motion->assignment && assignment==motion->assignment+1;
    int carryStand=(nextStep==18 || carryBegin) && motion->route.approachingStand &&
        !motion->arrived && motion->notified==motion->assignment && assignment==motion->assignment+1;
    vec3_t begin,stand;
    if(carryStand)VectorCopy(motion->route.standPoint,stand);
    vec2_t residual;
    if(carryBegin)VectorCopy(motion->route.diveStart,begin);
    if(leavingLadder)memcpy(residual,motion->route.turnDirections[0],sizeof(residual));
    memset(&motion->route,0,sizeof(motion->route));
    motion->route.diveArmed=traversal;
    if(carryStand){
        motion->route.approachingStand=1;
        VectorCopy(stand,motion->route.standPoint);
    }
    if(carryBegin){
        motion->route.approachingLadder=1;
        VectorCopy(begin,motion->route.diveStart);
    }
    if(leavingLadder){
        motion->route.entry.shuffle=shuffle;
        memcpy(motion->route.turnDirections[0],residual,sizeof(residual));
        motion->route.turnCount=motion->route.turnNext=1;
    }
    motion->assignment=assignment;
    motion->arrived=0;
    motion->blocked=0;
    motion->notified=0;
}
/* GAME 74490..7451b: GetVector(0), 739a0, optional yaw. The
 * obstacle task uses the no-yaw form at the seven stationary recruits.
 * Preserve the separate last authored-node receipt: a vector has no node. */
static int ActorGoalSetStationary(const GscActorGoalRequest *borrowed,const GscActorGoalRequest *q,
 const cod1ActorLease_t *lease,int now,int(*current)(void*),void *context){
 const gentity_t *e=g_entities+lease->entity.slot;
 cod1ActorRecord_t *a=cod1Actors+lease->actorIndex;
 cod1SentientRecord_t *s=cod1Sentients+lease->sentientIndex;
 cod1ActorLease_t checked;
 C1PostinitPosition old,after,staged;
 cod1ActorGoalSnapshot_t previous=a->goal;
 int ok=0;
 if(q->positional!=1 ||
    (previous.assigned && (previous.assignment==UINT64_MAX || a->turretMount.mounted)) ||
    a->motion.active || a->motion.busy || !a->spatial.linked || !e->r.linked ||
    !ActorGoalPosition(lease,&old) || memcmp(q->position,old.position,12))return 0;
 staged=old;if(!C1PostinitTargetApply(&staged,q->position))return 0;
 cod1ActorGoalBusy=1;cod1ActorGoalCancelled=0;
 if(current(context)!=1 || cod1ActorGoalCancelled || now!=level.time || level.spawning || level.restarted ||
    memcmp(borrowed,q,sizeof(*q)) || !ActorGoalLease(&q->entity,&checked) || memcmp(lease,&checked,sizeof(checked)) ||
    memcmp(&previous,&a->goal,sizeof(previous)) || a->motion.active || a->motion.busy ||
    !ActorGoalPosition(lease,&after) || memcmp(&old,&after,sizeof(old)))goto done;
 memcpy(s->rawGoal.position,staged.goal,12);s->defaults.cell0020=staged.cell20;
 a->defaults.pathWords[(0x6f4-0x374)/4]=staged.path6f4;a->defaults.cell0774=staged.cell774;
 a->rawGoalCell07e4=staged.cell7e4;a->rawGoalCell078f=staged.cell78f;
 /* The position command replaces an unstarted node command immediately.
  * Keep assignment identities monotonic for readers retaining an old node. */
 if(previous.assigned){
     memset(&a->goal,0,sizeof(a->goal));
     a->goal.assignment=previous.assignment+1;
 }
 a->stationaryGoal=1;a->stationaryGoalTick=now;
 if(g_debugMove.integer)G_Printf("[Training stationary goal] owner=%u/%u tick=%d xyz=%.3f,%.3f,%.3f inrange=%u\n",
      q->entity.slot,q->entity.generation,now,staged.goal[0],staged.goal[1],staged.goal[2],staged.cell78f);
 ok=1;
done:cod1ActorGoalBusy=0;cod1ActorGoalCancelled=0;return ok;
}
#include "g_cod1_actor_entity_goal.inc"
static int ActorGoalInvoke(void *user,const GscActorGoalRequest *input,char *error,size_t size,int (*current)(void *),void *context) {
    GscActorGoalRequest q;cod1ActorLease_t lease,after;cod1ActorGoalSnapshot_t goal,old;
    C1PostinitPosition prior,check,applied;
    cod1PlacedGoal_t placed,last;
    GscNodeFieldValue value;const GscNodeQueryHost *nodes=G_Cod1NodeQueryHost();int now,alive,i,ok=0;
    cod1ActorNativeData_t native;cod1SentientDefaults_t sentient;(void)user;
    if(!input||!current||!ActorGoalBuffers(input,sizeof(*input),NULL,0,error,size))return 0;
    if(cod1ActorGoalBusy){cod1ActorGoalCancelled=1;return 0;}
    q=*input;now=level.time;if(q.now_ms<0||!cod1EntityReady||level.spawning||level.restarted||!ActorMapAvailable())return 0;
    if(q.positional==2)return ActorSetEntityGoal(input,&q,now,error,size,current,context);
    if(!ActorGoalLease(&q.entity,&lease)||!nodes)return 0;
    if(q.positional)return ActorGoalSetStationary(input,&q,&lease,now,current,context);
    old=cod1Actors[lease.actorIndex].goal;native=cod1Actors[lease.actorIndex].nativeData;sentient=cod1Sentients[lease.sentientIndex].defaults;
    if(old.assignment==UINT64_MAX)return 0;
    placed=G_Cod1TrainingPlacedGoal(&q.entity,old.assignment+1,&q.node);
    if(placed.status<0)return 0;
    if(placed.status==1){
        if(!ActorGoalPosition(&lease,&prior))return 0;
        applied=prior;
        if(!C1PostinitTargetApply(&applied,placed.position))return 0;
    }
    cod1ActorGoalBusy=1;cod1ActorGoalCancelled=0;
    if(current(context)!=1||!nodes->validate(nodes->user,&q.node,q.now_ms,&alive,error,size,current,context)||alive!=1||
       !nodes->read(nodes->user,&q.node,"origin",q.now_ms,&value,error,size,current,context)||value.kind!=GSC_NODE_VALUE_VECTOR)goto done;
    memset(&goal,0,sizeof(goal));goal.assigned=1;goal.node=q.node;goal.assignment=old.assignment+1;
    memcpy(goal.position,value.vector,sizeof(goal.position));goal.changed=!old.assigned||memcmp(old.position,goal.position,sizeof(goal.position))!=0;
    for(i=0;i<3;i++)if(!isfinite(goal.position[i]))goto done;
    if(current(context)!=1||cod1ActorGoalCancelled||level.time!=now||level.spawning||level.restarted||!ActorGoalBuffers(input,sizeof(*input),NULL,0,error,size)||memcmp(input,&q,sizeof(q))||
       !ActorGoalLease(&q.entity,&after)||memcmp(&after,&lease,sizeof(lease))||
       memcmp(&old,&cod1Actors[lease.actorIndex].goal,sizeof(old))||memcmp(&native,&cod1Actors[lease.actorIndex].nativeData,sizeof(native))||
       memcmp(&sentient,&cod1Sentients[lease.sentientIndex].defaults,sizeof(sentient)))goto done;
    /* Pure command commit. No movement completion, pose or initialized flag
     * is invented. The existing prefix has no active path to release. */
    last=G_Cod1TrainingPlacedGoal(&q.entity,old.assignment+1,&q.node);
    if(memcmp(&placed,&last,sizeof(placed)))goto done;
    if(placed.status==1){
        cod1ActorRecord_t *a=cod1Actors+lease.actorIndex;
        cod1SentientRecord_t *s=cod1Sentients+lease.sentientIndex;
        if(!ActorGoalPosition(&lease,&check)||memcmp(&prior,&check,sizeof(prior)))goto done;
        memcpy(s->rawGoal.position,applied.goal,12);
        a->defaults.pathWords[(0x6f4-0x374)/4]=applied.path6f4;
        a->defaults.cell0774=applied.cell774;a->rawGoalCell07e4=applied.cell7e4;a->rawGoalCell078f=applied.cell78f;
    }
    cod1Sentients[lease.sentientIndex].defaults.cell0020=placed.status==1?placed.yaw:-1.0f;
    cod1Actors[lease.actorIndex].stationaryGoal=0;
    cod1Actors[lease.actorIndex].goal=goal;
    if(g_debugMove.integer && g_entities[q.entity.slot].targetname &&
       !strcmp(g_entities[q.entity.slot].targetname,"runner")){
        unsigned step=G_Cod1RunnerStep(&q.entity,&goal.node);
        if(step)G_Printf("[runner course goal] owner=%u/%u tick=%d serial=%llu step=%u node=%u\n",
            q.entity.slot,q.entity.generation,now,(unsigned long long)goal.assignment,step,goal.node.index);
    }
    if(TrainingJumperExitNode(&q.entity,goal.assignment,&goal.node)){
        cod1Actors[lease.actorIndex].towerGoalTick=now;
        if(g_debugMove.integer)G_Printf("[tower goal assigned] owner=%u/%u host=%d logical=%d\n",
            q.entity.slot,q.entity.generation,now,q.now_ms);
    }
    {
        cod1ActorMotion_t *motion=&cod1Actors[lease.actorIndex].motion;
        if(old.assigned && motion->active && !motion->model && !motion->busy &&
           !cod1Actors[lease.actorIndex].turretMount.mounted && motion->arrived &&
           motion->assignment==old.assignment && motion->notified==old.assignment){
            ActorMotionAdoptGoal(motion,goal.assignment,0);
            if(g_debugMove.integer)G_Printf("[actor goal] owner=%u/%u tick=%d handoff=%llu->%llu\n",
                q.entity.slot,q.entity.generation,now,(unsigned long long)old.assignment,(unsigned long long)goal.assignment);
        }
    }
    ok=1;
done:cod1ActorGoalBusy=0;cod1ActorGoalCancelled=0;return ok;
}
/* GAME 4E2D0 tests 51EB0 each AI step. In the empty path, unclaimed
 * non-facing branch, every in-range step posts the real sentient goal.
 * There is no one-shot latch: scripts can re-register between steps. */
int G_Cod1TrainingStationaryGoals(int now,int (*current)(void*),
 int (*notify)(void*,const GscEntityRef*),void *context){
 int slot;
 if(!current || !notify || now!=level.time || !ActorMapAvailable())return 0;
 for(slot=MAX_CLIENTS;slot<level.num_entities;slot++){
  unsigned index;cod1ActorRecord_t *a;cod1ActorLease_t lease;C1PostinitPosition state;GscEntityRef ref;
  if(!cod1ActorByEntity[slot])continue;
  index=cod1ActorByEntity[slot]-1;if(index>=COD1_ACTOR_CAPACITY)return 0;
  a=cod1Actors+index;if(!a->stationaryGoal || a->stationaryGoalTick==now)continue;
  ref=a->owner;
  if(ref.slot!=(unsigned)slot || current(context)!=1 || !ActorGoalLease(&ref,&lease) ||
     lease.actorIndex!=index || a->stationaryGoalTick>now || a->goal.assigned ||
     a->motion.active || !ActorGoalPosition(&lease,&state) || (a->defaults.allocation.cell0120==5 && !a->defaults.allocation.cell0134) ||
     !g_entities[slot].r.linked || !isfinite(state.radius) || state.radius<0 ||
     (int16_t)state.path6f4 || state.cursor6fa)return 0;
  a->stationaryGoalTick=now;
  a->rawGoalCell078f=(unsigned char)C1TargetRange(state.position,state.goal,state.radius);
  if(a->rawGoalCell078f && notify(context,&ref)!=1)return 0;
 }
 return 1;
}
int G_Cod1ActorReadGoal(const GscEntityRef *input,cod1ActorGoalSnapshot_t *out,char *error,size_t size) {
    cod1ActorLease_t lease;
    if(!input||!out||!ActorGoalBuffers(input,sizeof(*input),out,sizeof(*out),error,size)||!ActorGoalLease(input,&lease))return 0;
    *out=cod1Actors[lease.actorIndex].goal;return 1;
}
const GscActorGoalHost *G_Cod1ActorGoalHost(void){static const GscActorGoalHost h={NULL,ActorGoalInvoke};return &h;}
