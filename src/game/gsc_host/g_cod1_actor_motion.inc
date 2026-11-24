int TrainingJumperExitNode(const GscEntityRef *,uint64_t,const GscNodeRef *);
/* Selected ground locomotion for a real lease-owned original Actor.
 * This is not the RTCW cast-AI controller or a completed CoD combat brain.
 * Direct reachable goals are supported; a blocked route remains pending.
 * Original goal node Z is projected to a walkable floor (nodes sit above it).
 * Every engine callback is followed by lease/goal/time/Session revalidation. */
#include "../bg_cod1_actor_runtime.h"
#include "../scripted_wire.h"
static int MotionLease(const GscEntityRef *ref,cod1ActorLease_t *lease) {
    return cod1EntityReady&&!level.spawning&&!level.restarted&&ActorGoalLease(ref,lease);
}
static int MotionCurrent(const cod1ActorLease_t *lease,uint64_t assignment,int now) {
    return ActorLeaseValid(lease)&&cod1EntityReady&&!level.spawning&&!level.restarted&&
        level.time==now&&cod1Actors[lease->actorIndex].alive&&
        cod1Actors[lease->actorIndex].motion.active&&
        cod1Actors[lease->actorIndex].goal.assignment==assignment;
}
static int MotionLink(const cod1ActorLease_t *lease){
 cod1SpatialActorLink_t q;const cod1ActorRecord_t*a;gentity_t*e;
 if(!ActorLeaseValid(lease))return 0;a=cod1Actors+lease->actorIndex;e=g_entities+lease->entity.slot;
 if(a->motion.model){trap_LinkEntity(e);return ActorLeaseValid(lease)&&e->r.linked;}
 memset(&q,0,sizeof(q));q.version=COD1_SPATIAL_VERSION;q.bytes=sizeof(q);q.key=ActorSpatialKey(lease);
 q.nativeContents=a->nativeData.entity.nativeContents;q.nativeServerFlags=a->nativeData.entity.nativeServerFlags;q.hasDObj=a->dobj.present;
 return trap_Cod1SpatialActorLink(&q)==COD1_SPATIAL_OK;
}
/* The Training adapter uses the port's checked 24-unit prone capsule height.
 * Preserve the actual actor radius, foot origin, mask and stationary-body link.
 * A blocked stand keeps the current goal pending until the player moves clear. */
static int MotionCourseHull(const cod1ActorLease_t *lease,uint64_t assignment,
                            int now,int mask,int prone){
    cod1ActorMotion_t *m=&cod1Actors[lease->actorIndex].motion;
    gentity_t *e=g_entities+lease->entity.slot;
    vec3_t hi;
    trace_t tr;
    float old=e->r.maxs[2];
    if(prone && m->standingTop)return old==e->r.mins[2]+24.f?1:-1;
    if(!prone && !m->standingTop)return 1;
    VectorCopy(e->r.maxs,hi);
    hi[2]=prone?e->r.mins[2]+24.f:m->standingTop;
    if((prone && hi[2]>=old) || (!prone && hi[2]<=old))return -1;
    trap_TraceCapsule(&tr,e->r.currentOrigin,e->r.mins,hi,e->r.currentOrigin,lease->entity.slot,mask);
    if(!MotionCurrent(lease,assignment,now))return -1;
    if(tr.startsolid || tr.allsolid)return 0;
    e->r.maxs[2]=hi[2];
    if(!MotionLink(lease)){
        if(MotionCurrent(lease,assignment,now)){e->r.maxs[2]=old;MotionLink(lease);}
        return -1;
    }
    if(!MotionCurrent(lease,assignment,now))return -1;
    m->standingTop=prone?old:0;
    if(g_debugMove.integer)G_Printf("[%s hull] owner=%u/%u assignment=%llu height=%.1f\n",G_Cod1RunnerName(&lease->entity),lease->entity.slot,lease->entity.generation,(unsigned long long)assignment,hi[2]-e->r.mins[2]);
    return 1;
}
static void MotionClip(gentity_t *e,int moving,int now) {
    int frame=moving?COD1_ACTOR_MOODY_WALK_FRAME:0;
    if(e->s.density!=COD1_ACTOR_NET_MARKER)return;
    if(e->s.frame!=frame){e->s.frame=frame;e->s.time=now;}
}
int G_Cod1ActorReadEnemy(const GscEntityRef *ref,GscEntityRef *out,char *error,size_t size){
 cod1ActorLease_t lease;const cod1SentientRecord_t *s;
 if(!ref||!out||!ActorGoalBuffers(ref,sizeof(*ref),out,sizeof(*out),error,size)||!MotionLease(ref,&lease))return 0;
 s=&cod1Sentients[lease.sentientIndex];
 if(!s->enemyKnown)return 0;
 if(s->enemy.epoch&&(!G_Cod1EntitySameAllocation(&s->enemy)||!cod1SentientByEntity[s->enemy.slot]))return 0;
 *out=s->enemy;return 1;
}
/* Native sentient table20089200 row7: enemy+34 type8, read-only.
 * Generic getter20041B60 case8 leaves undefined only for a real NULL pointer.
 * The legacy string-cast field wire cannot carry a present entity: refuse it. */
int G_Cod1ActorReadAbsentEnemyField(const GscEntityRef*ref,GscStringCastValue*out,char*error,size_t size){
 GscEntityRef enemy;GscStringCastValue value;
 if(!ref||!out||!ActorGoalBuffers(ref,sizeof(*ref),out,sizeof(*out),error,size)||!G_Cod1ActorReadEnemy(ref,&enemy,error,size))return 0;
 if(enemy.epoch){EntityError(error,size,"present native enemy requires typed entity field transport; absence cannot be inferred");return 0;}
 memset(&value,0,sizeof(value));*out=value;return 1;
}
int G_Cod1ActorMotionRead(const GscEntityRef *ref,cod1ActorMotionSnapshot_t *out) {
    cod1ActorLease_t lease;cod1ActorMotion_t *m;cod1ActorMotionSnapshot_t copy;
    if(!ref||!out||!ActorGoalBuffers(ref,sizeof(*ref),out,sizeof(*out),NULL,0)||!MotionLease(ref,&lease))return 0;
    m=&cod1Actors[lease.actorIndex].motion;memset(&copy,0,sizeof(copy));
    copy.active=m->active;copy.blocked=m->blocked;copy.moving=m->moving;
    copy.arrived=m->arrived;copy.assignment=m->assignment;copy.notified=m->notified;
    copy.travelled=m->travelled;VectorCopy(g_entities[ref->slot].r.currentOrigin,copy.position);*out=copy;return 1;
}
int G_Cod1ActorGoalRadius(const GscEntityRef *ref,float *radius,int write) {
    cod1ActorLease_t lease;float value;
    if(!ref||!radius||!ActorGoalBuffers(ref,sizeof(*ref),radius,sizeof(*radius),NULL,0)||!MotionLease(ref,&lease))return 0;
    if(!write){*radius=cod1Sentients[lease.sentientIndex].defaults.goalradius;return 1;}
    value=*radius;if(!isfinite(value)||value<0||value>1000000)return 0;
    cod1Sentients[lease.sentientIndex].defaults.goalradius=value;
    cod1Sentients[lease.sentientIndex].defaults.goalradiusSquared=value*value;return 1;
}
int G_Cod1ActorMotionBegin(const GscEntityRef *ref,int model,int now) {
    cod1ActorLease_t lease;cod1ActorMotion_t *m;gentity_t *e;
    char name[MAX_QPATH];
    if(!ref||!ActorGoalBuffers(ref,sizeof(*ref),NULL,0,NULL,0)||now!=level.time||!MotionLease(ref,&lease)||
       model<=0||model>=MAX_MODELS)return 0;
    e=&g_entities[ref->slot];m=&cod1Actors[lease.actorIndex].motion;
    if(m->active)return m->model==model;
    if(!e->targetname||strcmp(e->targetname,"moody")||!cod1Actors[lease.actorIndex].spatial.linked||
       !e->r.linked||!(e->r.svFlags&SVF_NOCLIENT)||e->client||e->r.bmodel||e->r.contents)return 0;
    trap_GetConfigstring(CS_MODELS+model,name,sizeof(name));
    if(!MotionLease(ref,&lease)||now!=level.time||strcmp(name,COD1_ACTOR_MOODY_ALIAS))return 0;
    e=&g_entities[ref->slot];m=&cod1Actors[lease.actorIndex].motion;memset(m,0,sizeof(*m));
    m->model=model;m->lastTime=now;m->active=1;m->oldState=e->s;
    m->oldFlags=e->flags;m->oldSvFlags=e->r.svFlags;m->oldContents=e->r.contents;
    e->s.eType=ET_GENERAL;e->s.density=COD1_ACTOR_NET_MARKER;e->s.modelindex=model;
    e->s.frame=0;e->s.time=now;e->s.eFlags&=~EF_NODRAW;e->flags&=~FL_NODRAW;
    e->r.svFlags&=~SVF_NOCLIENT;e->r.contents=CONTENTS_BODY;e->clipmask=MASK_PLAYERSOLID;
    trap_LinkEntity(e);
    return MotionCurrent(&lease,cod1Actors[lease.actorIndex].goal.assignment,now);
}
int G_Cod1ActorMotionBeginUnpublished(const GscEntityRef *ref,int now) {
 cod1ActorLease_t lease;cod1ActorMotion_t *m;gentity_t *e;int oldMask;
 if(!ref||now!=level.time||!ActorGoalBuffers(ref,sizeof(*ref),NULL,0,NULL,0)||!MotionLease(ref,&lease))return 0;
 m=&cod1Actors[lease.actorIndex].motion;e=&g_entities[ref->slot];
 if(m->active)return m->model==0;
 if(!cod1Actors[lease.actorIndex].spatial.linked||!e->r.linked||e->client||e->r.bmodel||
    !(e->r.svFlags&SVF_NOCLIENT)||e->r.contents||e->s.modelindex)return 0;
 oldMask=e->clipmask;memset(m,0,sizeof(*m));m->active=1;m->lastTime=now;m->oldState=e->s;
 /* Run the displacement accumulated since the real goal was assigned. */
 if(cod1Actors[lease.actorIndex].towerGoalTick>0 && cod1Actors[lease.actorIndex].towerGoalTick==level.previousTime &&
    TrainingJumperExitNode(ref,cod1Actors[lease.actorIndex].goal.assignment,&cod1Actors[lease.actorIndex].goal.node))m->lastTime=cod1Actors[lease.actorIndex].towerGoalTick;
 m->oldFlags=e->flags;m->oldSvFlags=e->r.svFlags;m->oldContents=e->r.contents;
 e->r.contents=CONTENTS_BODY;e->clipmask=MASK_PLAYERSOLID;if(!MotionLink(&lease)){if(ActorLeaseValid(&lease)){m=&cod1Actors[lease.actorIndex].motion;e=&g_entities[ref->slot];e->r.contents=m->oldContents;e->s.solid=m->oldState.solid;e->clipmask=oldMask;m->active=0;}return 0;}
 return MotionCurrent(&lease,cod1Actors[lease.actorIndex].goal.assignment,now);
}
/* Quiesce before DObj/spatial retirement, death, Session close or map teardown.
 * Never writes a replacement allocation. It preserves the actual walked pose. */
static void ActorMotionStopOwner(const GscEntityRef *ref) {
    cod1ActorLease_t lease;cod1ActorMotion_t old;gentity_t *e;trajectory_t pos,apos;
    if(!ref||!ActorGoalLease(ref,&lease))return;
    old=cod1Actors[lease.actorIndex].motion;if(!old.active)return;
    cod1Actors[lease.actorIndex].motion.active=0;
    e=&g_entities[ref->slot];pos=e->s.pos;apos=e->s.apos;e->s=old.oldState;e->s.pos=pos;e->s.apos=apos;
    e->flags=(e->flags&~FL_NODRAW)|(old.oldFlags&FL_NODRAW);
    e->r.svFlags=(e->r.svFlags&~SVF_NOCLIENT)|(old.oldSvFlags&SVF_NOCLIENT);
    e->r.contents=old.oldContents;e->s.eFlags|=EF_NODRAW;e->flags|=FL_NODRAW;
    if(old.standingTop)e->r.maxs[2]=old.standingTop;
    (void)MotionLink(&lease);
}
/* R240 SINGLE free-specific path. The published actor owns an ordinary
 * world link; publication has already dropped the older checked model lane.
 * Restore the real saved motion component, but never relink an allocation
 * immediately entering G_FreeEntity. Its genuine final unlink owns removal.
 * Legacy StopOwner and its normal motion behavior remain unchanged. */
static int ActorMotionQuiesceForFree(const cod1ActorLease_t*lease,int(*current)(void*),void*ctx){
 cod1ActorRecord_t before,expected;gentity_t body,expectedBody;cod1ActorMotion_t old;gentity_t*e;trajectory_t pos,apos;
 if(!lease||!current||!ActorLeaseValid(lease))return 0;
 before=cod1Actors[lease->actorIndex];body=g_entities[lease->entity.slot];old=before.motion;
 if(!old.active||old.model||old.busy||old.oldContents||old.oldState.solid||
    !(old.oldSvFlags&SVF_NOCLIENT)||(old.oldSvFlags&(SVF_BOT|SVF_CASTAI)))return 0;
 if(current(ctx)!=1||!ActorLeaseValid(lease)||memcmp(&before,&cod1Actors[lease->actorIndex],sizeof(before))||memcmp(&body,&g_entities[lease->entity.slot],sizeof(body)))return 0;
 expected=before;expected.motion.active=0;expectedBody=body;pos=body.s.pos;apos=body.s.apos;
 expectedBody.s=old.oldState;expectedBody.s.pos=pos;expectedBody.s.apos=apos;
 expectedBody.flags=(body.flags&~FL_NODRAW)|(old.oldFlags&FL_NODRAW);
 expectedBody.r.svFlags=(body.r.svFlags&~SVF_NOCLIENT)|(old.oldSvFlags&SVF_NOCLIENT);
 expectedBody.r.contents=old.oldContents;expectedBody.s.eFlags|=EF_NODRAW;expectedBody.flags|=FL_NODRAW;
 /* Exactly StopOwner's actual recorded restoration, omitting solely the
  * relink. No callback occurs from here through strict final admission. */
 cod1Actors[lease->actorIndex].motion.active=0;e=g_entities+lease->entity.slot;
 e->s=old.oldState;e->s.pos=pos;e->s.apos=apos;e->flags=(e->flags&~FL_NODRAW)|(old.oldFlags&FL_NODRAW);
 e->r.svFlags=(e->r.svFlags&~SVF_NOCLIENT)|(old.oldSvFlags&SVF_NOCLIENT);e->r.contents=old.oldContents;e->s.eFlags|=EF_NODRAW;e->flags|=FL_NODRAW;
 return ActorLeaseValid(lease)&&!memcmp(&expected,&cod1Actors[lease->actorIndex],sizeof(expected))&&!memcmp(&expectedBody,&g_entities[lease->entity.slot],sizeof(expectedBody));
}
void G_Cod1ActorMotionStop(uint64_t epoch) {
    unsigned i;
    for(i=0;i<COD1_ACTOR_CAPACITY;i++)if(cod1Actors[i].motion.active&&cod1Actors[i].owner.epoch==epoch){
        GscEntityRef ref=cod1Actors[i].owner;ActorMotionStopOwner(&ref);
    }
}
static int MotionSharedTerminal(const cod1ActorRecord_t *a,const GscEntityRef *ref){
    return a->goal.assigned && G_Cod1RunnerStep(&a->owner,&a->goal.node)==22 && a->motion.assignment==a->goal.assignment &&
        a->motion.route.phase==2 && a->motion.route.cursor==0 &&
        G_Cod1RunnerSharesNode(ref,22) &&
        G_Cod1RunnerCourseGoal(ref,a->goal.assignment,&a->goal.node)==1;
}

/* Retail actor locomotion reads its own movement mask (GAME+66a30).
 * The same registered runner has native contents 0x4000 and an RTCW
 * presentation body. World geometry, player and owner exclusions still
 * participate in the native query. No entity is temporarily unlinked. */
static int MotionCourseSweep(const cod1ActorLease_t *lease,uint64_t assignment,int now,
                            const vec3_t start,const vec3_t end,int mask,trace_t *tr){
    const cod1ActorRecord_t *a=&cod1Actors[lease->actorIndex];
    const gentity_t *e=&g_entities[lease->entity.slot];
    unsigned courseStep=G_Cod1RunnerStep(&lease->entity,&a->goal.node);
    if(MotionSharedTerminal(a,&lease->entity) ||
       (a->motion.route.entryPeer.epoch && (courseStep==1 || courseStep==2) &&
        G_Cod1RunnerCourseGoal(&lease->entity,assignment,&a->goal.node)==1)){
        cod1SpatialTraceQuery_t q={0};
        if((uint32_t)mask!=COD1_SPATIAL_FLOOR_MASK || !e->r.linked)return 0;
        q.version=COD1_SPATIAL_VERSION;q.bytes=sizeof(q);q.contentsMask=mask;
        q.passEntity[0]=lease->entity.slot;q.passEntity[1]=ENTITYNUM_NONE;
        VectorCopy(start,q.start);VectorCopy(end,q.end);
        VectorCopy(e->r.mins,q.mins);VectorCopy(e->r.maxs,q.maxs);
        if(trap_Cod1SpatialCapsuleTrace(&q,tr)!=COD1_SPATIAL_OK)return 0;
    }else trap_TraceCapsule(tr,start,e->r.mins,e->r.maxs,end,lease->entity.slot,mask);
    return MotionCurrent(lease,assignment,now);
}
/* PM clips linked character bodies as axis-aligned boxes
 * (CM_TraceInternal, ALWAYS_BBOX_VS_BBOX). An actor course sweep
 * can clear the player's rounded corners while PM still starts solid.
 * Reserve the same hull PM will use before publishing the destination. */
static int MotionOccupiesPlayer(const gentity_t *actor,const vec3_t next){
    const gentity_t *player=&g_entities[0];
    int axis;
    if(!player->inuse || player->client!=level.clients || !player->r.linked ||
       player->client->ps.pm_type!=PM_NORMAL || player->client->noclip ||
       !(actor->r.contents & player->clipmask))return 0;
    for(axis=0;axis<3;axis++){
        float offset=player->client->ps.origin[axis]-next[axis];
        if(offset+player->r.maxs[axis]<=actor->r.mins[axis] ||
           offset+player->r.mins[axis]>=actor->r.maxs[axis])return 0;
    }
    return 1;
}
/* Steer the next ground step towards the near free side of a player's
 * body. Keep the route, root distance and all ordinary floor/obstacle sweeps.
 * The side follows the actor's approach; the diagonal is bounded by the
 * same per-tick travel budget as the unobstructed step. */
static void MotionPassPlayer(const gentity_t *actor,const vec3_t origin,
                             const vec3_t target,float step,vec3_t next){
    const gentity_t *player=&g_entities[0];
    const float *p;
    float low[2],high[2],nearSide,farSide,dx,dy,dist,sideDest,forwardDest;
    int fwd,side,i;
    if(!player->inuse || player->client!=level.clients || !player->r.linked ||
       player->client->ps.pm_type!=PM_NORMAL || player->client->noclip ||
       !(actor->r.contents & player->clipmask) || step<=0)return;
    p=player->client->ps.origin;
    if(origin[2]+actor->r.maxs[2]<=p[2]+player->r.mins[2] ||
       origin[2]+actor->r.mins[2]>=p[2]+player->r.maxs[2])return;
    for(i=0;i<2;i++){
        low[i]=p[i]+player->r.mins[i]-actor->r.maxs[i]-.125f;
        high[i]=p[i]+player->r.maxs[i]-actor->r.mins[i]+.125f;
    }
    fwd=fabsf(target[0]-origin[0])>fabsf(target[1]-origin[1])?0:1;
    side=1-fwd;
    nearSide=target[fwd]>origin[fwd]?low[fwd]:high[fwd];
    farSide=target[fwd]>origin[fwd]?high[fwd]:low[fwd];
    if((target[fwd]-origin[fwd])*(p[fwd]-origin[fwd])<0 &&
       fabsf(p[fwd]-origin[fwd])>=.5f*(high[fwd]-low[fwd]))return;
    if(fabsf(origin[fwd]-p[fwd])>high[fwd]-low[fwd] ||
       (target[fwd]-nearSide)*(target[fwd]-origin[fwd])<=0)return;
    if(origin[side]<low[side]-step || origin[side]>high[side]+step)return;
    sideDest=origin[side]<p[side]?low[side]:high[side];
    forwardDest=nearSide;
    if(fabsf(origin[side]-p[side])>=fabsf(sideDest-p[side])-.01f)
        forwardDest=farSide;
    else if((origin[fwd]-nearSide)*(farSide-nearSide)>0)
        forwardDest=origin[fwd];
    dx=forwardDest-origin[fwd];dy=sideDest-origin[side];dist=hypotf(dx,dy);
    if(dist<=.01f)return;
    if(step>dist)step=dist;
    next[fwd]=origin[fwd]+dx*step/dist;next[side]=origin[side]+dy*step/dist;
}
/* Returns 1 for a completed floor query, 0 for lost ownership, -1 for no
 * walkable ground. Being blocked is ordinary controller state, not success. */
static int MotionFloor(const cod1ActorLease_t *lease,uint64_t assignment,int now,const vec3_t at,
                       vec3_t floor,int mask,int runnerGoal) {
    trace_t tr;vec3_t start,end;
    VectorCopy(at,start);VectorCopy(at,end);start[2]+=18;end[2]-=128;
    if(!MotionCourseSweep(lease,assignment,now,start,end,mask,&tr))return 0;
    if(tr.startsolid && runnerGoal){
        /* Under a low ceiling the elevated start can intersect overhead geometry.
         * Retry from the current foot height with the same full hull and mask. */
        VectorCopy(at,start);
        if(!MotionCourseSweep(lease,assignment,now,start,end,mask,&tr))return 0;
    }
    if(tr.startsolid||tr.allsolid||!isfinite(tr.fraction)||tr.fraction<0||tr.fraction>=1||
       !isfinite(tr.plane.normal[2])||tr.plane.normal[2]<.7f){
        if(g_debugMove.integer&&now%500==0)G_Printf("[472 floor refusal] owner=%u tick=%d assignment=%llu at=%.5f,%.5f,%.5f trace=%d/%d/%.6f hit=%d normal=%.5f,%.5f,%.5f hullZ=%.3f,%.3f\n",lease->entity.slot,now,(unsigned long long)assignment,at[0],at[1],at[2],tr.startsolid,tr.allsolid,tr.fraction,tr.entityNum,tr.plane.normal[0],tr.plane.normal[1],tr.plane.normal[2],g_entities[lease->entity.slot].r.mins[2],g_entities[lease->entity.slot].r.maxs[2]);
        return -1;
    }
    if(!isfinite(tr.endpos[0])||!isfinite(tr.endpos[1])||!isfinite(tr.endpos[2]))return -1;
    VectorCopy(tr.endpos,floor);return 1;
}
static int MotionCrossesEntryPeer(const vec3_t start,const vec3_t end,
                                  const vec3_t center,float clearance){
    float dx=end[0]-start[0],dy=end[1]-start[1];
    float square=dx*dx+dy*dy;
    float t;
    if(!(clearance>0) || square<.001f)return 0;
    t=((center[0]-start[0])*dx+(center[1]-start[1])*dy)/square;
    if(t<=0)return 0;
    if(t>1)t=1;
    dx=start[0]+t*dx-center[0];dy=start[1]+t*dy-center[1];
    return fabsf(start[2]+t*(end[2]-start[2])-center[2])<80 &&
        dx*dx+dy*dy<clearance*clearance;
}
static int MotionBindEntryPeer(const cod1ActorLease_t *lease,const cod1ActorGoalSnapshot_t *goal,
                                int now,int mask,const vec3_t start,const vec3_t target,cod1LocalRoute_t *r){
    const gentity_t *self=&g_entities[lease->entity.slot];
    int i;
    if(!self->target || !G_Cod1RunnerSharesNode(&lease->entity,1) ||
       G_Cod1RunnerCourseGoal(&lease->entity,goal->assignment,&goal->node)!=1)return 0;
    for(i=0;i<COD1_ACTOR_CAPACITY;i++){
        const cod1ActorRecord_t *p=&cod1Actors[i];
        cod1ActorLease_t other;
        const gentity_t *body;
        float clearance;
        if(i==(int)lease->actorIndex || !p->alive || !MotionLease(&p->owner,&other) ||
           other.actorIndex!=(unsigned)i || !strcmp(G_Cod1RunnerName(&p->owner),"unbound"))continue;
        body=&g_entities[p->owner.slot];
        if(!body->r.linked || !body->target || strcmp(self->target,body->target))continue;
        r->entryPeer=other.entity;
        /* The second goal leaves the shared entry. Bind the current peer again
         * for this assignment; the spawn detour belongs to the first goal. */
        if(G_Cod1RunnerStep(&lease->entity,&goal->node)==2)return 0;
        clearance=self->r.maxs[0]+body->r.maxs[0];
        if(!MotionCrossesEntryPeer(start,target,body->r.currentOrigin,clearance))continue;
        /* Two observed route points bracket the peer longitudinally, both
         * 37.5 units on the passing side of the spawn-to-goal line. */
        if(clearance!=30.f)continue;
        {
            vec3_t direction,side,offset,at,from,to,corner[2];
            int k,status;
            trace_t tr;
            VectorSubtract(target,start,direction);
            direction[2]=0;
            if(VectorNormalize(direction)==0)return 0;
            VectorSet(side,direction[1],-direction[0],0);
            VectorSubtract(start,body->r.currentOrigin,offset);
            if(DotProduct(offset,side)<0)VectorInverse(side);
            VectorCopy(start,from);
            for(k=0;k<3;k++){
                if(k<2){
                    VectorMA(body->r.currentOrigin,k?37.5f:-37.5f,direction,at);
                    VectorMA(at,37.5f,side,at);
                    status=MotionFloor(lease,goal->assignment,now,at,corner[k],mask,1);
                    if(!status)return -1;
                    if(status<0)return 0;
                    VectorCopy(corner[k],to);
                }else VectorCopy(target,to);
                from[2]+=.125f;to[2]+=.125f;
                if(!MotionCourseSweep(lease,goal->assignment,now,from,to,mask,&tr))return -1;
                if(!ActorLeaseValid(&other))return -1;
                if(tr.startsolid || tr.allsolid || tr.fraction<1)return 0;
                VectorCopy(to,from);from[2]-=.125f;
            }
            VectorCopy(corner[0],r->points[2]);
            VectorCopy(corner[1],r->points[3]);
            r->path[2]=2;r->path[1]=3;r->path[0]=1;
            r->length=3;r->cursor=2;r->phase=2;
            if(g_debugMove.integer)
                G_Printf("[Training entry] owner=%u/%u peer=%u/%u corners=%.2f,%.2f %.2f,%.2f\n",
                         lease->entity.slot,lease->entity.generation,other.entity.slot,other.entity.generation,
                         corner[0][0],corner[0][1],corner[1][0],corner[1][1]);
            return 1;
        }
    }
    return 0;
}
/* Search once per script assignment, with at most six capsule queries per
 * tick. Transient bodies still block the real walking controller each step. */
#include "g_cod1_tower_path.inc"
static int MotionLocalRoute(const cod1ActorLease_t *lease,const cod1ActorGoalSnapshot_t *goal,
                           int now,int mask,const vec3_t start,vec3_t target) {
    cod1LocalRoute_t *r=&cod1Actors[lease->actorIndex].motion.route;
    gentity_t *e=&g_entities[lease->entity.slot];
    int i,budget=6;
    if(!r->phase && (G_Cod1RunnerStep(&lease->entity,&goal->node)==1 || G_Cod1RunnerStep(&lease->entity,&goal->node)==2) &&
       MotionBindEntryPeer(lease,goal,now,mask,start,target,r)<0)return -1;
    while (r->phase<2 && budget>0) {
        vec3_t a,b;
        trace_t tr;
        int to;
        if (!r->phase) {
            VectorCopy(start,a);VectorCopy(target,b);
        } else {
            if (r->from<0) {
                int best=-1;
                for (i=0;i<r->count;i++)
                    if (!r->visited[i] && r->cost[i]>=0 &&
                        (best<0 || r->cost[i]<r->cost[best])) best=i;
                if (best<0) {r->phase=3;break;}
                if (best==1) {
                    for (i=1;i!=0;i=r->parent[i]) r->path[r->length++]=i;
                    r->cursor=r->length-1;r->phase=2;
                    G_Printf("[CoD1 navigation] actor=%d assignment=%llu segments=%d\n",
                             lease->entity.slot,(unsigned long long)goal->assignment,r->length);
                    break;
                }
                r->from=best;r->visited[best]=1;r->next=0;
            }
            if (r->next==r->count) {r->from=-1;continue;}
            to=r->next++;
            if (r->visited[to]) continue;
            VectorCopy(r->points[r->from],a);VectorCopy(r->points[to],b);
            if (DistanceSquared(a,b)>160.0f*160.0f) continue;
        }
        a[2]+=.125f;b[2]+=.125f;
        trap_TraceCapsule(&tr,a,e->r.mins,e->r.maxs,b,lease->entity.slot,mask&~CONTENTS_BODY);
        if (!MotionCurrent(lease,goal->assignment,now)) return -1;
        budget--;
        if (!r->phase) {
            if(g_debugMove.integer && G_Cod1RunnerStep(&lease->entity,&goal->node)>=14)
                G_Printf("[Elder crawl route] assignment=%llu fromZ=%.2f targetZ=%.2f trace=%.4f normalZ=%.4f solid=%d\n",(unsigned long long)goal->assignment,a[2],b[2],tr.fraction,tr.plane.normal[2],tr.startsolid);
            /* A straight chord across the wire trench can run into its sloped
             * floor. Keep following the authored leg using the ordinary short
             * capsule sweeps and ground projection at each actual move. */
            int support=(r->approachingLadder || G_Cod1ElderStandingRun(G_Cod1RunnerStep(&lease->entity,&goal->node)) || (G_Cod1RunnerStep(&lease->entity,&goal->node)>=14 && cod1Actors[lease->actorIndex].motion.standingTop)) &&
                tr.fraction>0 && tr.fraction<1 && tr.plane.normal[2]>=.7f;
            if (!tr.startsolid && !tr.allsolid && (tr.fraction==1 || support)) {r->phase=2;break;}
            i=G_Cod1LocalWaypoints(&lease->entity,goal->assignment,&goal->node,start,target,r->points+2,COD1_LOCAL_POINTS-2);
            if (i<=0) {r->phase=3;break;}
            r->count=i+2;VectorCopy(start,r->points[0]);VectorCopy(target,r->points[1]);
            for (i=1;i<r->count;i++) r->cost[i]=-1;
            r->from=-1;r->phase=1;
        } else if (!tr.startsolid && !tr.allsolid && tr.fraction==1) {
            float cost=r->cost[r->from]+Distance(a,b);
            if (r->cost[to]<0 || cost<r->cost[to]) {r->cost[to]=cost;r->parent[to]=r->from;}
        }
    }
    if (r->phase!=2) return 0;
    if (r->cursor>0) VectorCopy(r->points[r->path[r->cursor]],target);
    return 1;
}

/* Local traversal for an authored wall_dive Begin/End pair. The server owns
 * the phase and the grounded handoff, including when a body blocks the path. */
#include "../../qcommon/cod1_actor_root14.h"
static struct {
    uint64_t epoch;
    vec3_t root[39];
    int gravityFrame;
} wallDiveClip;

static int MotionLoadWallDive(const cod1ActorLease_t *lease,uint64_t assignment,int now){
    fileHandle_t file=0;
    c1ar14Root_t *clip=NULL;
    c1ar14Sample_t sample;
    vec3_t samples[39];
    void *raw=NULL;
    int i,gravity=0,ok=0;
    int bytes;
    if(wallDiveClip.epoch==lease->entity.epoch)return 1;
    bytes=trap_FS_FOpenFile("xanim/jump_over_low_wall",&file,FS_READ);
    if(!file || bytes<=0 || bytes>1024*1024 || !MotionCurrent(lease,assignment,now))goto done;
    raw=malloc(bytes);
    if(!raw)goto done;
    trap_FS_Read(raw,bytes,file);
    if(!MotionCurrent(lease,assignment,now) || !C1AR14_Decode(raw,bytes,&clip) ||
       clip->frames!=39 || clip->intervals!=38 || clip->fps!=30 || clip->flags!=2 ||
       clip->noteCount!=1 || strcmp(clip->notes[0].name,"gravity on"))goto done;
    gravity=clip->notes[0].frame;
    if(gravity<1 || gravity>=38)goto done;
    for(i=0;i<=38;i++){
        if(!C1AR14_SampleRaw(clip,i/38.f,&sample))goto done;
        VectorCopy(sample.translation,samples[i]);
    }
    ok=samples[38][0]-samples[0][0]>1;
done:
    if(file)trap_FS_FCloseFile(file);
    free(raw);C1AR14_Free(clip);
    if(!ok || !MotionCurrent(lease,assignment,now))return 0;
    memcpy(wallDiveClip.root,samples,sizeof(samples));
    wallDiveClip.gravityFrame=gravity;wallDiveClip.epoch=lease->entity.epoch;
    return 1;
}

/* 1 commits the sampled position, 0 waits for clearance, -1 loses ownership. */
static int MotionDiveStep(const cod1ActorLease_t *lease,const cod1ActorGoalSnapshot_t *goal,
                          int now,int dt,int mask,const vec3_t origin,vec3_t ground,vec3_t facing,const char *runner){
    cod1LocalRoute_t *r=&cod1Actors[lease->actorIndex].motion.route;
    gentity_t *e=g_entities+lease->entity.slot;
    trace_t tr;
    vec3_t offset,span,probe,bottom,landing;
    float frame,forward,side,remaining,z,drop;
    int i,j,note;
    if(!r->diveArmed || !MotionLoadWallDive(lease,goal->assignment,now))return -1;
    if(!r->diveTime)VectorCopy(origin,r->diveStart);
    frame=fminf(38,(r->diveTime+dt)*.030f);i=(int)frame;j=i<38?i+1:i;
    for(int axis=0;axis<3;axis++){
        offset[axis]=wallDiveClip.root[i][axis]+(frame-i)*(wallDiveClip.root[j][axis]-wallDiveClip.root[i][axis])-wallDiveClip.root[0][axis];
    }
    VectorSubtract(goal->position,r->diveStart,span);span[2]=0;
    if(VectorLengthSquared(span)<1 || VectorLengthSquared(span)>128*128)return -1;
    forward=offset[0]/(wallDiveClip.root[38][0]-wallDiveClip.root[0][0]);
    side=(offset[1]-forward*(wallDiveClip.root[38][1]-wallDiveClip.root[0][1]))/VectorLength(span);
    ground[0]=r->diveStart[0]+forward*span[0]-side*span[1];
    ground[1]=r->diveStart[1]+forward*span[1]+side*span[0];
    ground[2]=r->diveStart[2]+offset[2];
    note=wallDiveClip.gravityFrame;
    if(frame>=note){
        remaining=(frame-note)/30.f;
        z=r->diveStart[2]+wallDiveClip.root[note][2]-wallDiveClip.root[0][2];
        drop=30.f*(wallDiveClip.root[note][2]-wallDiveClip.root[note-1][2])*remaining-.5f*g_gravity.value*remaining*remaining;
        VectorCopy(ground,probe);probe[2]=fmaxf(origin[2],z)+48.f;
        VectorCopy(probe,bottom);bottom[2]=r->diveStart[2]-128.f;
        /* Original traversal permits overlap with the rail. Gravity still
         * requires real walkable support under the actor's feet. */
        trap_Trace(&tr,probe,NULL,NULL,bottom,lease->entity.slot,mask&~CONTENTS_BODY);
        if(!MotionCurrent(lease,goal->assignment,now))return -1;
        if(tr.startsolid || tr.allsolid || tr.fraction>=1 || tr.plane.normal[2]<.7f)return 0;
        ground[2]=fmaxf(z+drop,tr.endpos[2]+.125f);
    }
    if(frame==38){
        int result=MotionFloor(lease,goal->assignment,now,ground,landing,mask,1);
        if(!result)return -1;
        if(result<0 || fabsf(landing[2]-ground[2])>2.f)return 0;
        VectorCopy(landing,ground);
    }
    trap_TraceCapsule(&tr,origin,e->r.mins,e->r.maxs,ground,lease->entity.slot,CONTENTS_BODY);
    if(!MotionCurrent(lease,goal->assignment,now))return -1;
    if(tr.startsolid || tr.allsolid || tr.fraction<1)return 0;
    if(!r->diveTime)G_Printf("[%s dive] start owner=%u/%u assignment=%llu z=%.2f\n",runner,lease->entity.slot,lease->entity.generation,(unsigned long long)goal->assignment,origin[2]);
    if(r->diveTime*.030f<note && frame>=note)G_Printf("[%s dive] gravity on owner=%u/%u note=%d z=%.2f\n",runner,lease->entity.slot,lease->entity.generation,note,ground[2]);
    r->diveTime+=dt;r->divePhase=frame/38.f;
    VectorAdd(origin,span,facing);
    return 1;
}

typedef struct {
    uint64_t epoch;
    vec3_t loop[21],transition[30];
    vec2_t mountRotation[30];
} cod1LadderRoots_t;
static cod1LadderRoots_t ladderRoots[2];

static int MotionLoadRootTrack(const cod1ActorLease_t *lease,uint64_t assignment,int now,
                           const char *path,int intervals,int flags,int fps,int bones,vec3_t *samples,vec2_t *rotations,const int *footsteps){
    fileHandle_t file=0;
    c1ar14Root_t *clip=NULL;
    c1ar14Sample_t value;
    void *raw=NULL;
    int i,ok=0,bytes=trap_FS_FOpenFile(path,&file,FS_READ);
    if(!file || bytes<1 || bytes>1024*1024 || !MotionCurrent(lease,assignment,now))goto done;
    raw=malloc(bytes);
    if(!raw)goto done;
    trap_FS_Read(raw,bytes,file);
    if(!MotionCurrent(lease,assignment,now) || !C1AR14_Decode(raw,bytes,&clip) ||
       clip->intervals!=intervals || clip->fps!=fps || clip->bones!=bones ||
       clip->flags!=flags || clip->noteCount!=(footsteps?2:0))goto done;
    if(footsteps){
        for(i=0;i<2;i++)if(clip->notes[i].frame!=footsteps[i] || strcmp(clip->notes[i].name,"footstep"))goto done;
    }
    for(i=0;i<=intervals;i++){
        if(!C1AR14_SampleRaw(clip,(float)i/intervals,&value))goto done;
        VectorCopy(value.translation,samples[i]);
        if(rotations){
            float z=value.q2[0],w=value.q2[1];
            if(!isfinite(z)||!isfinite(w)||z*z+w*w<.25f)goto done;
            /* Equivalent quaternion signs must interpolate on the same hemisphere. */
            if(i && z*rotations[i-1][0]+w*rotations[i-1][1]<0){z=-z;w=-w;}
            rotations[i][0]=z;rotations[i][1]=w;
        }
    }
    for(i=intervals;i>=0;i--)VectorSubtract(samples[i],samples[0],samples[i]);
    ok=1;
done:
    if(file)trap_FS_FCloseFile(file);
    free(raw);C1AR14_Free(clip);
    return ok && MotionCurrent(lease,assignment,now);
}
static int MotionLoadRootSamples(const cod1ActorLease_t *lease,uint64_t assignment,int now,
                           const char *path,int intervals,int flags,int fps,int bones,vec3_t *samples,vec2_t *rotations){
    return MotionLoadRootTrack(lease,assignment,now,path,intervals,flags,fps,bones,samples,rotations,NULL);
}

/* Each vector comes from the mounted retail root track. The movement pose
 * is chosen from the actor's actual GSC idle set at the end of the ladder. */
static c1CourseEntryRoots_t courseEntryRoots;
static uint64_t courseEntryEpoch;

static int MotionLoadEntryRoots(const cod1ActorLease_t *lease,uint64_t assignment,int now){
    static const char *const paths[C1_ENTRY_CLIPS]={
        "xanim/combatrun_right","xanim/combatrun_left","xanim/combatrun_back",
        "xanim/precombatrun1","xanim/stand_alerta_shuffle_forward","xanim/stand_alertb_shuffle_forward"
    };
    static const int footsteps[4][2]={{2,5},{4,10},{5,12},{7,16}};
    int i;
    if(courseEntryEpoch==lease->entity.epoch)return 1;
    for(i=0;i<C1_ENTRY_CLIPS;i++){
        if(!MotionLoadRootTrack(lease,assignment,now,paths[i],c1EntryIntervals[i],3,30,55,
            courseEntryRoots.position[i],NULL,i<4?footsteps[i]:NULL))return 0;
    }
    courseEntryEpoch=lease->entity.epoch;
    return 1;
}
int G_Cod1ActorSetCourseRunRate(const GscEntityRef *ref,float rate){
    cod1ActorLease_t lease;
    cod1ActorRecord_t *actor;
    if(!ref || !(rate>=.8f && rate<=1.2f) ||
       !ActorGoalBuffers(ref,sizeof(*ref),NULL,0,NULL,0) || !MotionLease(ref,&lease))return 0;
    actor=&cod1Actors[lease.actorIndex];
    if(!actor->alive || !actor->goal.assigned || G_Cod1RunnerStep(ref,&actor->goal.node)!=1 ||
       G_Cod1RunnerCourseGoal(ref,actor->goal.assignment,&actor->goal.node)!=1 ||
       actor->turretMount.mounted || actor->motion.busy || actor->courseRunRate)return 0;
    actor->courseRunRate=rate;
    if(g_debugMove.integer)G_Printf("[Training crouch rate] owner=%u/%u rate=%.8f\n",ref->slot,ref->generation,rate);
    return 1;
}

int G_Cod1ActorSetCourseEntryIdle(const GscEntityRef *ref,const char *idle){
    cod1ActorLease_t lease;
    cod1ActorRecord_t *actor;
    cod1ActorMotion_t *m;
    int selected;
    if(!idle||!ref||!ActorGoalBuffers(ref,sizeof(*ref),NULL,0,NULL,0)||!MotionLease(ref,&lease))return 0;
    if(!strcmp(idle,"none")||!strcmp(idle,"w")||!strcmp(idle,"a"))selected=C1_SHUFFLE_A;
    else if(!strcmp(idle,"b"))selected=C1_SHUFFLE_B;
    else return 0;
    actor=&cod1Actors[lease.actorIndex];m=&actor->motion;
    if(!actor->goal.assigned || G_Cod1RunnerStep(ref,&actor->goal.node)!=21 ||
       G_Cod1RunnerCourseGoal(ref,actor->goal.assignment,&actor->goal.node)!=4 || !m->active || m->model ||
       m->busy || !m->arrived || m->moving || m->blocked || m->assignment!=actor->goal.assignment ||
       m->notified!=actor->goal.assignment || !m->route.ladderReady || m->route.entry.shuffle)return 0;
    m->route.entry.shuffle=selected;
    return 1;
}

static void MotionRootSample(const vec3_t *samples,int intervals,float phase,vec3_t out){
    float frame=fminf(1,fmaxf(0,phase))*intervals;
    int i=(int)frame,j=i<intervals?i+1:i,k;
    for(k=0;k<3;k++)out[k]=samples[i][k]+(frame-i)*(samples[j][k]-samples[i][k]);
}
/* Both authored traversals have one non-looping clip and a vertical cycle.
 * Ascending, the actor cycles first. Descending, it mounts the ladder first. */
/* GAME+52d72..52da6 uses the remaining braking interval to scale
 * the live path-point delta before the movement sweep. */
static float MotionBrakeFraction(int dt,int remaining){
    float fraction=fminf(1,(float)dt/remaining);
    return fraction*(2-fraction);
}
static int MotionLadderStep(const cod1ActorLease_t *lease,const cod1ActorGoalSnapshot_t *goal,
                            int now,int dt,int mask,const vec3_t origin,vec3_t pos,vec3_t facing,const char *runner){
    cod1LocalRoute_t *r=&cod1Actors[lease->actorIndex].motion.route;
    gentity_t *e=g_entities+lease->entity.slot;
    int down=G_Cod1RunnerStep(&lease->entity,&goal->node)==21,sign=down?-1:1;
    cod1LadderRoots_t *roots=&ladderRoots[down];
    const float transitionTime=29.f/30.f;
    float spanX,spanY,length,height,cycleZ,transitionZ,seconds,cycles,phase;
    vec3_t delta,landing,clipEnd;
    int clip,result,finished;
    trace_t tr;
    if(!r->diveArmed)return -1;
    if(roots->epoch!=lease->entity.epoch){
        if(!MotionLoadRootSamples(lease,goal->assignment,now,down?"xanim/ladder_climbdown":"xanim/ladder_climbup",20,3,30,55,roots->loop,NULL) ||
           !MotionLoadRootSamples(lease,goal->assignment,now,down?"xanim/ladder_climbon":"xanim/ladder_climboff",29,2,30,55,roots->transition,down?roots->mountRotation:NULL))return -1;
        roots->epoch=lease->entity.epoch;
    }
    cycleZ=roots->loop[20][2];transitionZ=roots->transition[29][2];
    if(sign*cycleZ<=1 || sign*transitionZ<=1 || roots->transition[29][0]<=1)return -1;
    if(!r->ladderReady){
        result=MotionFloor(lease,goal->assignment,now,goal->position,landing,mask&~CONTENTS_BODY,1);
        if(result<=0)return result==0?-1:0;
        height=landing[2]-origin[2];
        if(sign*(height-transitionZ)<=0 || sign*height>256 || fabsf(landing[2]-goal->position[2])>48)return 0;
        VectorCopy(origin,r->diveStart);VectorCopy(landing,r->ladderExit);
        r->climbSeconds=(height-transitionZ+1.f)/(cycleZ*30.f/20.f);
        r->ladderReady=1;
    }
    spanX=r->ladderExit[0]-r->diveStart[0];spanY=r->ladderExit[1]-r->diveStart[1];
    length=sqrtf(spanX*spanX+spanY*spanY);
    if(length<1 || length>96)return -1;
    seconds=(r->diveTime+dt)*.001f;
    finished=seconds>=r->climbSeconds+transitionTime;
    cycles=fminf(r->climbSeconds,down?fmaxf(0,seconds-transitionTime):seconds)*30.f/20.f;
    phase=cycles-floorf(cycles);
    MotionRootSample(roots->loop,20,phase,delta);
    height=r->diveStart[2]+floorf(cycles)*cycleZ+delta[2];
    VectorCopy(r->diveStart,pos);pos[2]=height;
    clip=down?COD1_FRAME_LADDER_DOWN:COD1_FRAME_LADDER_UP;
    VectorCopy(r->ladderExit,clipEnd);
    if(down){
        /* The exit node defines the ladder axis. The mount's root track
         * determines where the actor's feet actually meet the rungs. */
        clipEnd[0]=r->diveStart[0]+(spanX*roots->transition[29][0]-spanY*roots->transition[29][1])/length;
        clipEnd[1]=r->diveStart[1]+(spanY*roots->transition[29][0]+spanX*roots->transition[29][1])/length;
    }
    if(down && seconds>=transitionTime){
        pos[0]=clipEnd[0];pos[1]=clipEnd[1];
        pos[2]=fmaxf(r->ladderExit[2],height+transitionZ);
    }else if(down || seconds>=r->climbSeconds){
        float along,side;
        clip=down?COD1_FRAME_LADDER_ON:COD1_FRAME_LADDER_OFF;
        phase=fminf(1,(down?seconds:seconds-r->climbSeconds)/transitionTime);
        MotionRootSample(roots->transition,29,phase,delta);
        if(down){
            along=delta[0]/length;
            side=delta[1]/length;
        }else{
            along=delta[0]/roots->transition[29][0];
            side=(delta[1]-along*roots->transition[29][1])/length;
        }
        pos[0]+=along*spanX-side*spanY;
        pos[1]+=along*spanY+side*spanX;
        pos[2]+=down?delta[2]:delta[2]/transitionZ*(r->ladderExit[2]+1-height);
    }
    if(finished){
        result=MotionFloor(lease,goal->assignment,now,pos,landing,mask,1);
        if(!result)return -1;
        if(result<0 || DistanceSquared(landing,clipEnd)>1)return 0;
        if(down){
            const cod1SentientRecord_t *s=&cod1Sentients[lease->sentientIndex];
            float radius=s->defaults.goalradius;
            vec3_t wish,step;
            VectorSubtract(r->ladderExit,origin,wish);
            VectorMA(origin,MotionBrakeFraction(dt,500),wish,step);
            if(!MotionCourseSweep(lease,goal->assignment,now,origin,step,mask,&tr))return -1;
            if(tr.startsolid || tr.allsolid || !isfinite(tr.fraction) || tr.fraction<1)return 0;
            result=MotionFloor(lease,goal->assignment,now,step,landing,mask,1);
            if(!result)return -1;
            if(result<0 || fabsf(landing[2]-r->ladderExit[2])>1)return 0;
            if(g_debugMove.integer)
                G_Printf("[Training ladder foot] owner=%u/%u tick=%d dt=%d old=%.5f,%.5f,%.5f goal=%.5f,%.5f,%.5f ground=%.5f,%.5f,%.5f\n",
                    lease->entity.slot,lease->entity.generation,now,dt,origin[0],origin[1],origin[2],
                    r->ladderExit[0],r->ladderExit[1],r->ladderExit[2],landing[0],landing[1],landing[2]);
            if(!(radius>0) || !(DistanceSquared(landing,r->ladderExit)<=radius*radius))return 0;
        }
        VectorCopy(landing,pos);
    }
    trap_TraceCapsule(&tr,origin,e->r.mins,e->r.maxs,pos,lease->entity.slot,CONTENTS_BODY);
    if(!MotionCurrent(lease,goal->assignment,now))return -1;
    if(tr.startsolid || tr.allsolid || tr.fraction<1)return 0;
    if(r->ladderFrame!=clip && g_debugMove.integer)
        G_Printf("[%s ladder] owner=%u/%u frame=%d height=%.2f exitZ=%.2f climbSeconds=%.4f\n",
            runner,lease->entity.slot,lease->entity.generation,clip,pos[2],r->ladderExit[2],r->climbSeconds);
    r->diveTime+=dt;r->divePhase=phase;r->ladderFrame=clip;
    /* Face the authored ladder direction plus the mount clip's root rotation.
     * The descending loop keeps the completed mount orientation. */
    if(down){
        float frame=fminf(1,seconds/transitionTime)*29.f;
        int i=(int)frame,j=i<29?i+1:i;
        float t=frame-i,z=roots->mountRotation[i][0]+t*(roots->mountRotation[j][0]-roots->mountRotation[i][0]);
        float w=roots->mountRotation[i][1]+t*(roots->mountRotation[j][1]-roots->mountRotation[i][1]);
        float norm=z*z+w*w,cosine=(w*w-z*z)/norm,sine=2*z*w/norm;
        VectorCopy(origin,facing);
        facing[0]+=cosine*spanX-sine*spanY;
        facing[1]+=sine*spanX+cosine*spanY;
    }else{
        VectorCopy(origin,facing);facing[0]+=spanX;facing[1]+=spanY;
    }
    return finished?2:1;
}

/* Local presentation of Training's living runner after the original terminal
 * stance write. Keep the chosen family and clock on the actor's assignment. */
/* At 20051EB0 the scripted goal can be reached on a live path. On this
 * final direct leg the remaining path is the segment to the placed goal.
 * 200619D0 also bounds remaining travel at twice the effective radius. */
static int MotionSharedGoalReached(const cod1ActorLease_t *lease,const vec3_t target){
    const cod1ActorRecord_t *a=&cod1Actors[lease->actorIndex];
    const cod1SentientRecord_t *s=&cod1Sentients[lease->sentientIndex];
    C1PostinitPosition state;
    vec3_t d;
    if(!MotionSharedTerminal(a,&lease->entity) || !a->motion.moving || a->motion.blocked ||
       a->defaults.allocation.cell0120==5 || a->defaults.allocation.cell0134 ||
       !ActorGoalPosition(lease,&state) || state.radius!=128 ||
       s->defaults.goalradiusSquared!=state.radius*state.radius ||
       memcmp(target,state.goal,sizeof(vec3_t)))return 0;
    VectorSubtract(state.position,target,d);
    return d[2]*d[2]<=6400 && d[0]*d[0]+d[1]*d[1]<=s->defaults.goalradiusSquared &&
        VectorLengthSquared(d)<=4*s->defaults.goalradiusSquared;
}
static int MotionRestRequestOwner(const cod1ActorLease_t *lease){
    const cod1ActorRecord_t *a=cod1Actors+lease->actorIndex;
    const cod1ActorMotion_t *m=&a->motion;
    const cod1SentientRecord_t *s=cod1Sentients+lease->sentientIndex;
    const gentity_t *e=g_entities+lease->entity.slot;
    return a->alive && !a->turretMount.mounted && a->goal.assigned && G_Cod1RunnerStep(&a->owner,&a->goal.node)==22 &&
        m->active && !m->model && m->assignment==a->goal.assignment && m->notified==a->goal.assignment &&
        !m->blocked && a->nativeData.actor.cell0234==2 &&
        a->defaults.allocation.interval==64 && e->target &&
        e->targetname && !strcmp(e->targetname,"runner") && a->scriptStrings.weaponPresent &&
        !Q_stricmp(a->scriptStrings.weapon,"m1garand") && s->enemyKnown &&
        !s->enemy.epoch && !s->enemy.slot && !s->enemy.generation &&
        G_Cod1RunnerCourseGoal(&lease->entity,a->goal.assignment,&a->goal.node)==1;
}
/* Original GAME 54F50 sums ten movement directions; 55040 ages their weights
 * from 1.0 to 0.1. The selected yaw goes through 54DE0's bounded body turn. */
static float MotionDepartureYaw(cod1LocalRoute_t *r,const vec3_t direction,float previous,int dt){
    vec3_t sum={0,0,0};
    float length=sqrtf(direction[0]*direction[0]+direction[1]*direction[1]),desired,delta,limited,yaw;
    int age,slot;
    if(length<=.001f)return previous;
    slot=r->turnNext;
    r->turnDirections[slot][0]=direction[0]/length;
    r->turnDirections[slot][1]=direction[1]/length;
    if(r->turnCount<10)r->turnCount++;
    r->turnNext=(slot+1)%10;
    for(age=0;age<r->turnCount;age++){
        float weight=(10-age)*.1f;
        sum[0]+=weight*r->turnDirections[slot][0];
        sum[1]+=weight*r->turnDirections[slot][1];
        slot=(slot+9)%10;
    }
    if(sum[0]==0 && sum[1]==0)return previous;
    if(!ActorQuantizeAngle(vectoyaw(sum),&desired))return previous;
    delta=AngleSubtract(desired,previous);
    limited=fmaxf(-.4f*dt,fminf(.4f*dt,delta));
    if(!ActorQuantizeAngle(previous+limited,&yaw))return previous;
    return yaw;
}
static void MotionStartRestClip(cod1LocalRoute_t *r){
    r->restSeed=rand();
    r->restFamily=Q_random(&r->restSeed)<.5f?0:2;
    r->restFrame=COD1_FRAME_REST_A+r->restFamily+(Q_random(&r->restSeed)>=.7f);
    r->restPhase=0;
    r->restStamp=level.time;
}
int G_Cod1ActorStartRest(const GscEntityRef *ref,float rate){
    cod1ActorLease_t lease;
    cod1ActorMotion_t *m;
    if(!isfinite(rate)||rate<=0||!ref||!ActorGoalBuffers(ref,sizeof(*ref),NULL,0,NULL,0)||
       !MotionLease(ref,&lease)||!MotionRestRequestOwner(&lease))return 0;
    m=&cod1Actors[lease.actorIndex].motion;
    if(m->busy || m->route.restRate || m->route.restFrame)return 0;
    if(!m->arrived && !(m->moving && MotionSharedTerminal(&cod1Actors[lease.actorIndex],ref)))return 0;
    if(m->arrived && m->moving)return 0;
    m->route.restRate=rate;
    if(m->arrived)MotionStartRestClip(&m->route);
    return 1;
}
int G_Cod1TrainingRunnerAppearance(const GscEntityRef *ref,int after,
 int (*read)(const GscEntityRef*,int,GscTrainingAppearance*)){
 cod1ActorLease_t lease;
 cod1ActorRecord_t *a;cod1ActorMotion_t *m;
 GscTrainingAppearance input={0};
 int kind=0;
 if(!read || !ref || !MotionLease(ref,&lease))return 0;
 a=cod1Actors+lease.actorIndex;m=&a->motion;
 if(!a->goal.assigned || m->busy || m->model)return 0;
 if(!after && G_Cod1RunnerStep(&a->owner,&a->goal.node)==1 && !a->courseRunRate)kind=1;
 if(after && G_Cod1RunnerStep(&a->owner,&a->goal.node)==21 && m->assignment==a->goal.assignment && m->arrived && m->notified==a->goal.assignment &&
    m->route.ladderReady && !m->route.entry.shuffle)kind=2;
 if(after && G_Cod1RunnerStep(&a->owner,&a->goal.node)==22 && !a->courseRestSampled && !m->route.restRate &&
    MotionRestRequestOwner(&lease))kind=3;
 if(!kind)return 1;
 if(!read(ref,kind,&input) || !ActorLeaseValid(&lease))return 0;
 if(kind==1)return G_Cod1ActorSetCourseRunRate(ref,input.rate);
 if(kind==2)return G_Cod1ActorSetCourseEntryIdle(ref,input.idle);
 if(input.rest && !G_Cod1ActorStartRest(ref,input.rate))return 0;
 a->courseRestSampled=1;return 1;
}
static void MotionRestAdvance(const cod1ActorLease_t *lease,int now){
    static const unsigned intervals[]={137,69,100,110};
    cod1LocalRoute_t *r=&cod1Actors[lease->actorIndex].motion.route;
    if(!r->restRate)return;
    if(!MotionRestRequestOwner(lease) || cod1Actors[lease->actorIndex].motion.moving || now<r->restStamp){
        r->restRate=0;
        r->restFrame=0;
        return;
    }
    if(!r->restFrame)MotionStartRestClip(r);
    /* A finished note-free clip yields its next selection on this game tick.
     * Calling twice at the same sim time never advances the animation twice. */
    r->restPhase+=(now-r->restStamp)*(.030f*r->restRate)/intervals[r->restFrame-COD1_FRAME_REST_A];
    r->restStamp=now;
    if(r->restPhase>=1){
        r->restFrame=COD1_FRAME_REST_A+r->restFamily+(Q_random(&r->restSeed)>=.7f);
        r->restPhase=0;
    }
}

/* GAME+53330 acquires a nearby actor by horizontal and vertical separation;
 * GAME+4e187 holds that same entity until 37.5 units. This direct-leg
 * subset handles a settled runner yielding to the approaching runner.
 * Each actor changes only its own linked pose. */
/* Cumulative translation across the authored start and the repeating gait. */
static struct {uint64_t epoch;vec3_t start[14],loop[19];} courseWalkRoots;
static void MotionWalkSample(int elapsed,vec3_t out,int *frame,float *phase){
    float frames=elapsed*.015f;
    if(frames<13){
        *frame=COD1_FRAME_CROUCH_WALK_START;*phase=frames/13;
        MotionRootSample(courseWalkRoots.start,13,*phase,out);
    }else{
        int cycles=(int)((frames-13)/18);
        *frame=COD1_FRAME_CROUCH_WALK_LOOP;*phase=(frames-13-18*cycles)/18;
        MotionRootSample(courseWalkRoots.loop,18,*phase,out);
        VectorMA(out,(float)cycles,courseWalkRoots.loop[18],out);
        VectorAdd(out,courseWalkRoots.start[13],out);
    }
}
static int MotionWalkDistance(const cod1ActorLease_t *lease,int now,cod1LocalRoute_t *route,int dt,float *distance){
    vec3_t before,after;
    int frame;
    float phase;
    uint64_t assignment=cod1Actors[lease->actorIndex].goal.assignment;
    if(courseWalkRoots.epoch!=lease->entity.epoch){
        if(!MotionLoadRootSamples(lease,assignment,now,"xanim/crouch2crouchwalk_straight",13,2,15,54,courseWalkRoots.start,NULL) ||
           !MotionLoadRootSamples(lease,assignment,now,"xanim/crouchwalk_loop",18,3,15,54,courseWalkRoots.loop,NULL))return 0;
        courseWalkRoots.epoch=lease->entity.epoch;
    }
    /* After the start clip, 18 frames at 15 fps repeat every 1200 ms. */
    if(route->walkTime>=2067)route->walkTime-=1200;
    MotionWalkSample(route->walkTime,before,&frame,&phase);
    route->walkTime+=dt;
    MotionWalkSample(route->walkTime,after,&route->walkFrame,&route->walkPhase);
    *distance=hypotf(after[0]-before[0],after[1]-before[1]);
    return isfinite(*distance);
}
static int MotionCrouchCycle(int gait,float *stride,float *frequency){
    switch(gait){
    case COD1_FRAME_RIFLE_CROUCHRUN_1:*stride=COD1_CROUCHRUN1_FORWARD;*frequency=30.f/20;break;
    case COD1_FRAME_RIFLE_CROUCHRUN_2:*stride=73.89181f;*frequency=30.f/16;break;
    case COD1_FRAME_RIFLE_CROUCHRUN_3:*stride=COD1_CROUCHRUN3_FORWARD;*frequency=30.f/14;break;
    case COD1_FRAME_RIFLE_CROUCHRUN_4:*stride=COD1_CROUCHRUN4_FORWARD;*frequency=30.f/20;break;
    default:return 0;
    }
    return 1;
}

/* The standing/crouched forward branches share a clock. Each branch has
 * its own cycle length and rate; normalize their weights before applying
 * the blended stride. The stance request supplies this actor's script rate. */
static float MotionApproachStep(const cod1ActorMotion_t *m,int gait,int dt,
                                int *elapsed,float *phase){
    float stride,frequency,blend,stand,crouch,advance;
    float previous=m->route.approachTime?m->route.approachPhase:
        (m->route.entry.elapsedMs?m->route.entry.phase:m->travelled/COD1_PRECOMBAT_RUN_FORWARD);
    if(!MotionCrouchCycle(gait,&stride,&frequency))return 0;
    *elapsed=m->route.approachTime+dt;
    if(*elapsed>600)*elapsed=600;
    blend=*elapsed/600.f;
    stand=1-blend;
    crouch=fminf(1,.001f+blend);
    crouch/=stand+crouch;
    stand=1-crouch;
    advance=(stand*1.5f*m->route.restRate+crouch*frequency)*dt*.001f;
    *phase=previous+advance;
    *phase-=floorf(*phase);
    return (stand*COD1_PRECOMBAT_RUN_FORWARD+crouch*stride)*advance;
}

/* GAME+52c94..52da6. Only the final grounded path segment is eligible.
 * The caller advances the remaining duration after a successful swept step. */
static float MotionBrakeStep(const cod1LocalRoute_t *route,int dt,float distance,float gaitStep,int *remaining){
    int left=route->brakeRemaining;
    *remaining=0;
    if(!left && route->terminalSpeed>0 && distance<=.25f*route->terminalSpeed)left=500;
    if(!left)return gaitStep;
    *remaining=left;
    return distance*MotionBrakeFraction(dt,left);
}
static void MotionBrakeCommit(const cod1ActorLease_t *lease,cod1LocalRoute_t *route,int left,int dt,
                              const vec3_t origin,const vec3_t end,int now){
    float speed=hypotf(end[0]-origin[0],end[1]-origin[1])*1000.f/dt;
    if(left && g_debugMove.integer)
        G_Printf("[Training brake] owner=%u/%u return=%d tick=%d remaining=%d dt=%d speed=%.3f from=%.4f,%.4f to=%.4f,%.4f\n",
                 lease->entity.slot,lease->entity.generation,route->returning,now,left,dt,route->terminalSpeed,origin[0],origin[1],end[0],end[1]);
    if(left==500)route->stopStarted=0;
    if(left>0 && left<200 && !route->stopStarted){
        route->stopStarted=1;
        route->stopTick=now;
        route->stopFirstStep=dt;
        route->stopFadeTick=0;
    }
    route->terminalSpeed=speed;
    route->brakeRemaining=left>dt?left-dt:0;
}
static int MotionYieldTerminal(const cod1ActorLease_t *lease,int now,int dt,
                              int (*current)(void *),void *context){
    cod1ActorRecord_t *a=&cod1Actors[lease->actorIndex];
    cod1LocalRoute_t *route=&a->motion.route;
    GscEntityRef *held=&route->separationPeer;
    cod1ActorLease_t other;
    const gentity_t *body=&g_entities[lease->entity.slot];
    vec3_t away,wanted,ground;
    int i,result,remaining=0,mask=a->nativeData.entity.nativeClipMask;
    float xy2,step,rootDistance=0;
    trace_t tr;
    uint64_t assignment=a->goal.assignment,peerAssignment;
    if(!a->motion.arrived || a->motion.notified!=a->goal.assignment || !MotionSharedTerminal(a,&lease->entity))return 1;
    for(i=0;i<COD1_ACTOR_CAPACITY;i++){
        const cod1ActorRecord_t *p=&cod1Actors[i];
        if(i==(int)lease->actorIndex || !p->alive || !p->motion.active || p->motion.notified!=p->goal.assignment ||
           p->turretMount.mounted || !MotionLease(&p->owner,&other) || other.actorIndex!=(unsigned)i ||
           !MotionSharedTerminal(p,&p->owner) ||
           memcmp(&a->goal.node,&p->goal.node,sizeof(a->goal.node)))continue;
        if(held->epoch && memcmp(held,&p->owner,sizeof(*held)))continue;
        peerAssignment=p->goal.assignment;
        VectorSubtract(body->r.currentOrigin,g_entities[p->owner.slot].r.currentOrigin,away);
        xy2=away[0]*away[0]+away[1]*away[1];
        if(!isfinite(xy2) || !isfinite(away[2]))return 0;
        if(!held->epoch && (xy2>=900.f || away[2]*away[2]>=6400.f))continue;
        break;
    }
    if(i==COD1_ACTOR_CAPACITY){
        memset(held,0,sizeof(*held));route->walkFrame=route->reciprocal=route->returning=0;
        return 1;
    }
    if(!held->epoch){
        route->reciprocal=!cod1Actors[i].motion.route.returning &&
            !memcmp(&cod1Actors[i].motion.route.separationPeer,&lease->entity,sizeof(*held));
        *held=other.entity;route->walkTime=0;
        if(route->stopTick && !route->stopFadeTick && now-route->stopTick<600)
            route->stopFadeTick=now;
        if(g_debugMove.integer)
            G_Printf("[Training separation] owner=%u/%u peer=%u/%u beginDistance=%.3f reciprocal=%d tick=%d\n",
                     lease->entity.slot,lease->entity.generation,other.entity.slot,other.entity.generation,sqrtf(xy2),route->reciprocal,now);
    }else if(!route->returning && (xy2>=1406.25f || away[2]*away[2]>=6400.f)){
        route->walkTime=route->walkFrame=0;
        if(!route->reciprocal){memset(held,0,sizeof(*held));return 1;}
        route->returning=1;route->brakeRemaining=0;route->terminalSpeed=0;
    }
    if(!dt)return 1;
    if(route->returning){
        VectorSubtract(a->goal.position,body->r.currentOrigin,away);
        xy2=away[0]*away[0]+away[1]*away[1];
        if(!isfinite(xy2))return 0;
        if(xy2==0){
            if(g_debugMove.integer)
                G_Printf("[Training return] owner=%u/%u reached goal tick=%d\n",lease->entity.slot,lease->entity.generation,now);
            memset(held,0,sizeof(*held));route->returning=route->reciprocal=route->walkFrame=0;
            return 1;
        }
    }
    if(xy2==0){
        /* GAME+50bb5 draws two signed components when origins coincide. */
        uint32_t x,y;
        if(!G_Cod1RngDrawPair(&x,&y))return 0;
        away[0]=2.f*((float)x/32767.f)-1.f;
        away[1]=2.f*((float)y/32767.f)-1.f;
        xy2=away[0]*away[0]+away[1]*away[1];
        if(xy2==0)return 1;
    }
    /* In retail the late actor is still blending out of its stop during
     * reciprocal separation. Both measured blends remained below the floor. */
    if(!route->reciprocal || route->returning){
        if(!MotionWalkDistance(lease,now,route,dt,&rootDistance))return 0;
    }
    step=route->returning?fminf(sqrtf(xy2),rootDistance):fmaxf(.03f*dt,rootDistance);
    if(route->returning)step=MotionBrakeStep(route,dt,sqrtf(xy2),step,&remaining);
    step/=sqrtf(xy2);
    VectorCopy(body->r.currentOrigin,wanted);
    wanted[0]+=away[0]*step;wanted[1]+=away[1]*step;
    if(route->returning && step>=1){wanted[0]=a->goal.position[0];wanted[1]=a->goal.position[1];}
    result=MotionFloor(lease,assignment,now,wanted,ground,mask,1);
    if(!result)return 0;
    if(result<0 || fabsf(ground[2]-body->r.currentOrigin[2])>18.1f)return 1;
    if(!MotionCourseSweep(lease,assignment,now,body->r.currentOrigin,ground,mask,&tr))return 0;
    if(tr.startsolid || tr.allsolid || tr.fraction<1)return 1;
    if(current(context)!=1 || !MotionCurrent(lease,assignment,now) || !MotionCurrent(&other,peerAssignment,now))return 0;
    if(route->returning)MotionBrakeCommit(lease,route,remaining,dt,body->r.currentOrigin,ground,now);
    G_SetOrigin(&g_entities[lease->entity.slot],ground);
    g_entities[lease->entity.slot].s.pos.trType=TR_INTERPOLATE;
    VectorCopy(ground,a->nativeData.entity.origin);
    VectorCopy(ground,a->nativeData.entity.originTrajectory);
    return MotionLink(lease) && MotionCurrent(lease,assignment,now) && MotionCurrent(&other,peerAssignment,now);
}

/* GAME+51eb0/+619d0 on a live, final, direct segment. The physical
 * negotiation endpoint remains distinct from the scripted goal radius. */
static int MotionWireGoalReached(const cod1ActorLease_t *lease,const vec3_t target){
    const cod1ActorRecord_t *actor=cod1Actors+lease->actorIndex;
    const cod1SentientDefaults_t *s=&cod1Sentients[lease->sentientIndex].defaults;
    vec3_t d;
    if(actor->motion.route.phase!=2 || actor->motion.route.cursor || !actor->motion.moving ||
       actor->motion.blocked || actor->defaults.allocation.cell0120==5 || actor->defaults.allocation.cell0134)return 0;
    VectorSubtract(actor->nativeData.entity.origin,target,d);
    return d[2]*d[2]<=6400.f && d[0]*d[0]+d[1]*d[1]<=s->goalradiusSquared &&
        VectorLengthSquared(d)<=4.f*s->goalradiusSquared;
}
static int MotionWireExitDistance(const cod1ActorLease_t *,int,float *);

int G_Cod1ActorMotionFrame(const GscEntityRef *ref,int now,
    int (*current)(void *),int (*notify)(void *,const GscEntityRef *),void *context) {
    cod1ActorLease_t lease;cod1ActorMotion_t *m;cod1ActorGoalSnapshot_t goal;
    gentity_t *e;vec3_t origin,target,desired,delta,ground,angles,up,over;
    float distance,step,travelled,approachPhase=0,approachStep=0;
    int approachTime=0,dt,result,mask=MASK_PLAYERSOLID,runnerGoal,scriptGoalReached=0,terminalPath=0,remaining=0;trace_t tr;
    int sharedRun=0,holdStanding=0,entryStep=0;int jumperExit=0;unsigned courseStep;
    int blockLine=0,approachingLadder=0,walkingToStand=0;
    vec3_t scriptedTarget;
    c1CourseEntry_t entry={0};
    if(!ref||!current||!notify||!ActorGoalBuffers(ref,sizeof(*ref),NULL,0,NULL,0)||
       current(context)!=1||!MotionLease(ref,&lease)||now!=level.time)return 0;
    m=&cod1Actors[lease.actorIndex].motion;
    if(!m->active||m->busy||now<m->lastTime)return 0;
    goal=cod1Actors[lease.actorIndex].goal;m->busy=1;
    courseStep=goal.assigned?G_Cod1RunnerStep(ref,&goal.node):0;
    dt=now-m->lastTime;m->lastTime=now;
    /* Publish this frame's committed movement to script readers after
     * the mover leaves its busy scope. Waiting and obstruction have zero velocity. */
    if(dt>0)VectorClear(m->velocity);

    /* Split frame hitches by dropping excess simulation time, never by a long
     * collision-free position jump. At most 6.2 units are attempted per frame. */
    if(dt>100)dt=100;
    if(m->assignment!=goal.assignment)ActorMotionAdoptGoal(m,goal.assignment,courseStep);
    /* Mounted native state3 owns displacement through the fit controller.
     * Keep a new path goal pending until that state releases its ROOT link. */
    if(cod1Actors[lease.actorIndex].turretMount.mounted){m->moving=0;goto done;}
    /* The real row-9 body record alone writes the actor pose. Keep the
     * pending destination and last movement tick so a resumed walk uses
     * only its own frame time. The actual goal waiter remains attached. */
    if(goal.assigned && g_entities[ref->slot].targetname &&
       ((!strcmp(g_entities[ref->slot].targetname,"moody") && G_Cod1MoodyCourseGoal(goal.assignment,&goal.node)) ||
        (!strcmp(g_entities[ref->slot].targetname,"foley") && G_Cod1FoleyCourseGoal(goal.assignment,&goal.node))) &&
       cod1Actors[lease.actorIndex].turretMount.currentTop==1 &&
       cod1Actors[lease.actorIndex].turretMount.currentStack[1]==9){
        m->moving=0;
        goto done;
    }
    if(m->arrived){
        MotionRestAdvance(&lease,now);
        if(courseStep==22 && !MotionYieldTerminal(&lease,now,dt,current,context))goto lost;
    }
    if(!goal.assigned||m->arrived||!dt)goto done;
    e=&g_entities[ref->slot];VectorCopy(e->r.currentOrigin,origin);
    runnerGoal=G_Cod1RunnerCourseGoal(ref,goal.assignment,&goal.node);
    jumperExit=TrainingJumperExitNode(ref,goal.assignment,&goal.node);
    if(jumperExit)mask=cod1Actors[lease.actorIndex].nativeData.entity.nativeClipMask;
    if(runnerGoal){
        cod1PlacedGoal_t placed=G_Cod1TrainingPlacedGoal(ref,goal.assignment,&goal.node);
        if(placed.status!=(runnerGoal==1?1:0))goto lost;
        /* The node already has a ground position from the map's placement trace.
         * Collision belongs to the current step, including the final step. */
        /* The CoD actor uses its movement mask for every sweep. Playerclip
         * at the pipes blocks players; the original actor mask includes
         * monsterclip, bodies and solid geometry. */
        mask=cod1Actors[lease.actorIndex].nativeData.entity.nativeClipMask;
        if((uint32_t)mask!=COD1_SPATIAL_FLOOR_MASK)goto lost;
        if(courseStep>=14 && courseStep<=17 && m->notified!=goal.assignment){
            if(cod1Actors[lease.actorIndex].nativeData.actor.cell0234!=4)goto lost;
            result=MotionCourseHull(&lease,goal.assignment,now,mask,1);
            if(result<0)goto lost;
            if(!result){blockLine=__LINE__;goto blocked;}
        }
        approachingLadder=courseStep==19 && runnerGoal==4 && m->route.approachingLadder;
        if(runnerGoal==3 || (runnerGoal==4 && !approachingLadder)){
            result=runnerGoal==4?MotionLadderStep(&lease,&goal,now,dt,mask,origin,ground,target,G_Cod1RunnerName(ref)):
                MotionDiveStep(&lease,&goal,now,dt,mask,origin,ground,target,G_Cod1RunnerName(ref));
            if(result<0)goto lost;
            if(!result){blockLine=__LINE__;goto blocked;}
            distance=1;goto commit_motion;
        }
        if(approachingLadder){
            VectorCopy(m->route.diveStart,target);
        }else if(runnerGoal==2){
            /* A negotiation Begin remains a script-owned traversal. Approach
             * its authored XY on foot while its goal waiter remains pending. */
            if(m->route.atTraversalStart)goto done;
            VectorCopy(goal.position,target);
            target[2]=origin[2];
            result=MotionFloor(&lease,goal.assignment,now,target,ground,mask,runnerGoal);
            if(!result)goto lost;
            if(result<0){blockLine=__LINE__;goto blocked;}
            VectorCopy(ground,target);
        }else VectorCopy(placed.position,target);
        VectorCopy(target,scriptedTarget);
        walkingToStand=m->route.approachingStand &&
            ((courseStep==17 && m->notified==goal.assignment) || courseStep==18 || approachingLadder);
        if(walkingToStand)VectorCopy(m->route.standPoint,target);
        result=MotionLocalRoute(&lease,&goal,now,mask,origin,target);
        if(result<0 || current(context)!=1 || !MotionCurrent(&lease,goal.assignment,now))goto lost;
        if(!result){blockLine=__LINE__;goto blocked;}
        m->route.headingValid=1;
        m->route.heading[0]=target[0]-origin[0];
        m->route.heading[1]=target[1]-origin[1];
    }else if(jumperExit){
        VectorCopy(goal.position,target);
        result=MotionJumperRoute(&lease,&goal,now,mask,origin,target);
        if(result<0||!MotionCurrent(&lease,goal.assignment,now))goto lost;
        if(!result){blockLine=__LINE__;goto blocked;}
    }else{
        result=MotionFloor(&lease,goal.assignment,now,goal.position,target,mask,runnerGoal);
        if(!result)goto lost;
    }
    if(result<0){m=&cod1Actors[lease.actorIndex].motion;m->blocked=1;m->moving=0;MotionClip(e,0,now);goto done;}
    VectorSubtract(target,origin,delta);distance=sqrtf(delta[0]*delta[0]+delta[1]*delta[1]);
    /* Authored leaderwalk root delta: 63.4768 units / (31 frames /30fps).
     * Controller applies displacement; the renderer samples bone tracks only. */
    step=(jumperExit?COD1_PRECOMBAT_RUN_FORWARD*1.5f:61.435f)*dt*.001f;
    sharedRun=runnerGoal==1 && courseStep==22 && MotionSharedTerminal(&cod1Actors[lease.actorIndex],ref);
    terminalPath=sharedRun && m->notified==goal.assignment;
    holdStanding=terminalPath && cod1Actors[lease.actorIndex].nativeData.actor.cell0234==2 &&
        m->route.nextRunPoseReview>now && !m->route.approachTime;
    if(holdStanding){
        step=COD1_PRECOMBAT_RUN_FORWARD*1.5f*dt*.001f;
    }else if(runnerGoal && (courseStep<=13 || (courseStep==22 && m->notified==goal.assignment &&
       cod1Actors[lease.actorIndex].nativeData.actor.cell0234==2 &&
       MotionSharedTerminal(&cod1Actors[lease.actorIndex],ref)))){
        int gait=Cod1CourseCrouchFrame(ref->slot);
        if(cod1Actors[lease.actorIndex].nativeData.actor.cell0234!=2)goto lost;
        float stride,frequency;
        const float runRate=cod1Actors[lease.actorIndex].courseRunRate;
        if(!MotionCrouchCycle(gait,&stride,&frequency))goto lost;
        if(runRate && courseStep<=13){
            step=stride*frequency*runRate*dt*.001f;
            if(g_debugMove.integer && courseStep<=2 && !(now%200))
                G_Printf("[Training crouch stride] owner=%u/%u tick=%d goal=%llu gait=%d rate=%.8f dt=%d step=%.7f\n",
                    ref->slot,ref->generation,now,(unsigned long long)goal.assignment,gait,runRate,dt,step);
        }else if(gait!=COD1_FRAME_RIFLE_CROUCHRUN_2){
            step=stride*frequency*dt*.001f;
        }
    }else if(approachingLadder || (runnerGoal && G_Cod1ElderStandingRun(courseStep))){
        if((m->standingTop && !walkingToStand) || cod1Actors[lease.actorIndex].nativeData.actor.cell0234!=1)goto lost;
        step=COD1_PRECOMBAT_RUN_FORWARD*(30.f/20.f)*dt*.001f;
        if((courseStep==18 || approachingLadder) && !MotionWireExitDistance(&lease,now,&step))goto lost;
    }else if(runnerGoal && courseStep>=14){
        step=COD1_PRONE_CRAWL_FORWARD*(30.f/67.f)*dt*.001f;
    }
    if(jumperExit){
        float relative=vectoyaw(delta)-e->r.currentAngles[YAW];
        entry=m->route.entry;entry.shuffle=C1_ENTRY_FROM_LANDING;entry.parentRate=cod1Actors[lease.actorIndex].courseRunRate;
        if(!MotionLoadEntryRoots(&lease,goal.assignment,now)||!C1EntryStep(&entry,&courseEntryRoots,relative,dt,&step))goto lost;
        entryStep=1;
    }
    if(runnerGoal==1 && courseStep==22 && m->route.entry.shuffle &&
       (cod1Actors[lease.actorIndex].nativeData.actor.cell0234==1 || holdStanding) &&
       !m->route.approachTime){
        float relative=vectoyaw(delta)-e->r.currentAngles[YAW];
        entry=m->route.entry;
        if(!MotionLoadEntryRoots(&lease,goal.assignment,now) || !C1EntryStep(&entry,&courseEntryRoots,relative,dt,&step))goto lost;
        entryStep=1;
    }
    if(terminalPath && !holdStanding && m->route.restRate>0 && cod1Actors[lease.actorIndex].nativeData.actor.cell0234==2){
        approachStep=MotionApproachStep(m,Cod1CourseCrouchFrame(ref->slot),dt,&approachTime,&approachPhase);
        if(!isfinite(approachStep)||!isfinite(approachPhase)||approachStep<=0)goto lost;
        step=approachStep;
    }
    if(terminalPath)step=MotionBrakeStep(&m->route,dt,distance,step,&remaining);
    if(step>distance)step=distance;
    /* The scripted Foley handoff reads the exact node XY. Complete the last
     * fraction with the same floor and capsule sweeps as every walking step. */
    VectorCopy(origin,desired);
    if((terminalPath || (e->targetname && !strcmp(e->targetname,"foley"))) && distance<=step){
        desired[0]=target[0];desired[1]=target[1];
    }else if(distance>.001f){desired[0]+=delta[0]*step/distance;desired[1]+=delta[1]*step/distance;}
    if(runnerGoal==1)MotionPassPlayer(e,origin,target,step,desired);
    result=MotionFloor(&lease,goal.assignment,now,desired,ground,mask,runnerGoal||jumperExit);
    if(!result)goto lost;
    if(jumperExit&&g_debugMove.integer&&!(now%500)&&result<0)G_Printf("[tower obstruction] floor refused target=%.1f %.1f tick=%d\n",desired[0],desired[1],now);
    if(result<0||fabsf(ground[2]-origin[2])>18.1f){m=&cod1Actors[lease.actorIndex].motion;m->blocked=1;m->moving=0;MotionClip(e,0,now);goto done;}
    if(!MotionCourseSweep(&lease,goal.assignment,now,origin,ground,mask,&tr))goto lost;
    if(tr.startsolid||tr.allsolid||tr.fraction<1){
        /* An actual bounded step-up sweep, followed by the validated floor.
         * Final descending sweep prevents stepping through a solid ledge. */
        VectorCopy(origin,up);up[2]+=18;
        if(!MotionCourseSweep(&lease,goal.assignment,now,origin,up,mask,&tr))goto lost;
        if(tr.startsolid||tr.allsolid||tr.fraction<1){blockLine=__LINE__;goto blocked;}
        VectorCopy(ground,over);over[2]+=18;
        if(!MotionCourseSweep(&lease,goal.assignment,now,up,over,mask,&tr))goto lost;
        if(tr.startsolid||tr.allsolid||tr.fraction<1){blockLine=__LINE__;goto blocked;}
        if(!MotionCourseSweep(&lease,goal.assignment,now,over,ground,mask,&tr))goto lost;
        if(tr.startsolid||tr.allsolid||VectorLengthSquared(tr.endpos)==0||fabsf(tr.endpos[2]-ground[2])>.1f){blockLine=__LINE__;goto blocked;}
    }
commit_motion:
    if(current(context)!=1||!MotionCurrent(&lease,goal.assignment,now))goto lost;
    e=&g_entities[ref->slot];m=&cod1Actors[lease.actorIndex].motion;
    if(MotionOccupiesPlayer(e,ground)){blockLine=__LINE__;goto blocked;}
    if(runnerGoal && (courseStep==1 || (courseStep==2 && m->route.entryPeer.epoch))){
        int peer=G_Cod1WaitingRunnerContact(ref,ground);
        /* The actual actor mask excludes 0x4000 bodies. Preserve space
         * between two moving capsules while letting a separating step pass.
         * A waiting runner and the player's capsule still block normally. */
        if(peer<0 && m->route.entryPeer.epoch){
            cod1ActorLease_t other;
            const gentity_t *body;
            vec3_t before,after;
            float oldGap,newGap,radius;
            if(!MotionLease(&m->route.entryPeer,&other))goto lost;
            body=&g_entities[other.entity.slot];
            if(body->r.linked && body->r.contents){
                VectorSubtract(origin,body->r.currentOrigin,before);
                VectorSubtract(ground,body->r.currentOrigin,after);
                oldGap=before[0]*before[0]+before[1]*before[1];
                newGap=after[0]*after[0]+after[1]*after[1];
                radius=e->r.maxs[0]+body->r.maxs[0];
                if(fabsf(after[2])<80 && newGap<radius*radius && newGap<oldGap)
                    peer=other.entity.slot;
            }
        }
        if(!MotionCurrent(&lease,goal.assignment,now))goto lost;
        if(peer>=0){
            if(g_debugMove.integer && !m->blocked)
                G_Printf("[Training contact] moving=%u/%u waitingSlot=%d tick=%d\n",ref->slot,ref->generation,peer,now);
            {blockLine=__LINE__;goto blocked;}
        }
    }
    /* run.gsc resumes after BlendIntoStandRun (1 s), then after each
     * standing runanim wait (0.2 s). The assignment owns this deadline.
     * Start it with the first swept standing step out of the ladder. */
    if(sharedRun && cod1Actors[lease.actorIndex].nativeData.actor.cell0234==1 &&
       now>=m->route.nextRunPoseReview){
        int wait=m->route.nextRunPoseReview?200:1000;
        m->route.nextRunPoseReview=(int64_t)now+wait;
        if(g_debugMove.integer)
            G_Printf("[Runner pose review] owner=%u/%u tick=%d wait=%d\n",ref->slot,ref->generation,now,wait);
    }
    if(entryStep){
        m->route.entry=entry;
        if(g_debugMove.integer && entry.elapsedMs<=1100)
            G_Printf("[Training run entry] owner=%u/%u tick=%d age=%d shuffle=%d phase=%.6f distance=%.6f xy=%.5f,%.5f\n",
                ref->slot,ref->generation,now,entry.elapsedMs,entry.shuffle,entry.phase,step,ground[0],ground[1]);
    }
    if(approachTime){
        m->route.approachTime=approachTime;
        m->route.approachPhase=approachPhase;
        if(g_debugMove.integer)
            G_Printf("[Training approach] owner=%u/%u tick=%d dt=%d elapsed=%d rate=%.6f phase=%.6f wanted=%.6f brake=%d xy=%.5f\n",ref->slot,ref->generation,now,dt,approachTime,m->route.restRate,approachPhase,approachStep,remaining,hypotf(ground[0]-origin[0],ground[1]-origin[1]));
    }
    if(terminalPath)MotionBrakeCommit(&lease,&m->route,remaining,dt,origin,ground,now);
    VectorSubtract(ground,origin,delta);travelled=VectorLength(delta);
    VectorScale(delta,1000.0f/dt,m->velocity);
    m->travelled+=travelled;m->blocked=0;m->moving=travelled>.001f;MotionClip(e,m->moving,now);
    G_SetOrigin(e,ground);e->s.pos.trType=TR_INTERPOLATE;
    if(distance>.001f){VectorSubtract(target,origin,delta);vectoangles(delta,angles);angles[0]=angles[2]=0;
        if(jumperExit){
            /* Native GAME 550D0 feeds the ten-sample heading history with a
             * blend of the current path direction and the forward sample.
             * Its argument is the root displacement for this AI tick. */
            vec3_t facing;
            float ahead=fminf(1.f,fmaxf(0.f,(step*20.f-95.f)/95.f));
            VectorScale(m->route.tower.aim,1.f-ahead,facing);
            VectorMA(facing,ahead,m->route.tower.face,facing);
            angles[YAW]=MotionDepartureYaw(&m->route,facing,e->r.currentAngles[YAW],dt);
        }else if(runnerGoal==1 && courseStep==22 && !m->notified && m->route.turnCount){
            angles[YAW]=MotionDepartureYaw(&m->route,delta,e->r.currentAngles[YAW],dt);
        }
        /* Keep the path's default target, while native override owns the
         * actual body rotation in ActorOrientationFrame. */
        cod1Actors[lease.actorIndex].nativeData.orientation.yawCopy=angles[1];
        if(!cod1Actors[lease.actorIndex].defaults.allocation.cell0134){
        G_SetAngle(e,angles);e->s.apos.trType=TR_INTERPOLATE;
        VectorCopy(angles,cod1Actors[lease.actorIndex].nativeData.entity.angles);
        VectorCopy(angles,cod1Actors[lease.actorIndex].nativeData.entity.angleTrajectory);
        cod1Actors[lease.actorIndex].nativeData.orientation.yaw=angles[1];
        cod1Actors[lease.actorIndex].nativeData.orientation.yawCopy=angles[1];}}
    VectorCopy(ground,cod1Actors[lease.actorIndex].nativeData.entity.origin);
    VectorCopy(ground,cod1Actors[lease.actorIndex].nativeData.entity.originTrajectory);
    if(!MotionLink(&lease))goto lost;
    if(current(context)!=1||!MotionCurrent(&lease,goal.assignment,now))goto lost;
    m=&cod1Actors[lease.actorIndex].motion;e=&g_entities[ref->slot];
    if(runnerGoal==4 && !approachingLadder){
        if(result==2){
            if(courseStep==21){
                /* The last path direction aims from committed feet back to the
                 * placed node; it survives this completed assignment only. */
                float dx=m->route.ladderExit[0]-ground[0],dy=m->route.ladderExit[1]-ground[1];
                float gap=sqrtf(dx*dx+dy*dy);
                if(gap>.001f){
                    m->route.turnDirections[0][0]=dx/gap;
                    m->route.turnDirections[0][1]=dy/gap;
                    m->route.turnCount=1;
                }
            }
            m->arrived=1;m->moving=0;
            G_Printf("[%s ladder] grounded owner=%u/%u assignment=%llu xyz=%.2f,%.2f,%.2f\n",
                G_Cod1RunnerName(ref),ref->slot,ref->generation,(unsigned long long)goal.assignment,ground[0],ground[1],ground[2]);
        }
        goto done;
    }
    if(runnerGoal==3){
        if(m->route.divePhase>=1){
            m->arrived=1;m->moving=0;
            G_Printf("[%s dive] landed owner=%u/%u assignment=%llu xyz=%.2f,%.2f,%.2f\n",G_Cod1RunnerName(ref),ref->slot,ref->generation,(unsigned long long)goal.assignment,ground[0],ground[1],ground[2]);
        }
        goto done;
    }
    VectorSubtract(target,e->r.currentOrigin,delta);
    if((runnerGoal||jumperExit) && m->route.cursor>0 && VectorLengthSquared(delta)<=.25f){
        if(jumperExit){m->route.cursor--;goto done;}
        G_Printf("[CoD1 navigation] waypoint actor=%d position=%.1f,%.1f,%.1f\n",
                 ref->slot,ground[0],ground[1],ground[2]);
        m->route.cursor--;goto done;
    }
    /* The animation waiter is independent of the port's actual stand-hull
     * transition. Check and relink the hull only at the authored stand point. */
    if(walkingToStand && VectorLengthSquared(delta)<=.25f){
        result=MotionCourseHull(&lease,goal.assignment,now,mask,0);
        if(result<0)goto lost;
        if(!result){blockLine=__LINE__;goto blocked;}
        m->route.approachingStand=0;
        if(courseStep==17){m->moving=0;m->arrived=1;MotionClip(e,0,now);}
        /* Plan the next physical segment against the restored full hull. */
        m->route.phase=m->route.count=m->route.length=m->route.cursor=0;
        if(g_debugMove.integer)G_Printf("[Training stand clearance] owner=%u tick=%d at=%.3f,%.3f,%.3f target=%.3f,%.3f,%.3f height=%.1f\n",
            ref->slot,now,ground[0],ground[1],ground[2],target[0],target[1],target[2],e->r.maxs[2]-e->r.mins[2]);
        goto done;
    }
    if(approachingLadder && VectorLengthSquared(delta)<=.25f){
        m->route.approachingLadder=0;m->route.diveArmed=1;
        goto done;
    }
    if(runnerGoal==2 && VectorLengthSquared(delta)<=.25f){
        m->moving=0;m->route.atTraversalStart=1;m->arrived=1;MotionClip(e,0,now);
        G_Printf("[Training] %s at %s Begin owner=%u/%u xy=%.2f,%.2f\n",G_Cod1RunnerName(ref),courseStep==18?"ladder_up":courseStep==20?"ladder_down":"wall_dive",
                 ref->slot,ref->generation,ground[0],ground[1]);
        goto done;
    }
    if(runnerGoal && courseStep>=16 && courseStep<=18 && !m->notified){
        int exact=VectorLengthSquared(delta)<=.25f;
        scriptGoalReached=MotionWireGoalReached(&lease,scriptedTarget);
        if(courseStep==17 && scriptGoalReached && !exact){
            m->route.approachingStand=1;
            VectorCopy(scriptedTarget,m->route.standPoint);
        }
        if(courseStep==17 && exact){
            result=MotionCourseHull(&lease,goal.assignment,now,mask,0);
            if(result<0)goto lost;
            if(!result){
                scriptGoalReached=0;
                if(exact){blockLine=__LINE__;goto blocked;}
            }
        }
        if(courseStep==18 && scriptGoalReached){
            m->route.approachingLadder=1;
            VectorCopy(scriptedTarget,m->route.diveStart);
        }
    }
    if(jumperExit&&!m->notified){
        const cod1SentientDefaults_t *sent=&cod1Sentients[lease.sentientIndex].defaults;
        vec3_t d;VectorSubtract(ground,goal.position,d);
        if(sent->goalradiusSquared!=32.f*32.f)goto lost;
        scriptGoalReached=d[2]*d[2]<=6400.f && d[0]*d[0]+d[1]*d[1]<=sent->goalradiusSquared &&
            VectorLengthSquared(d)<=4*sent->goalradiusSquared;
    }
    if(runnerGoal==1 && courseStep==22 && !m->notified)
        scriptGoalReached=MotionSharedGoalReached(&lease,target);
    if((!terminalPath || (delta[0]==0 && delta[1]==0)) && VectorLengthSquared(delta)<=.25f &&
       (!e->targetname || strcmp(e->targetname,"foley") ||
        !memcmp(e->r.currentOrigin,goal.position,2*sizeof(float)))){m->arrived=1;m->moving=0;MotionClip(e,0,now);}
    goto done;
blocked:
    if(g_debugMove.integer&&now%500==0)G_Printf("[472 blocked] owner=%u tick=%d assignment=%llu step=%u line=%d origin=%.5f,%.5f,%.5f target=%.5f,%.5f,%.5f\n",ref->slot,now,(unsigned long long)goal.assignment,courseStep,blockLine,origin[0],origin[1],origin[2],target[0],target[1],target[2]);
    if(jumperExit&&g_debugMove.integer&&!(now%500))G_Printf("[tower obstruction] travel refused cursor=%d target=%.1f %.1f tick=%d\n",m->route.cursor,target[0],target[1],now);
    m=&cod1Actors[lease.actorIndex].motion;m->route.terminalSpeed=0;m->blocked=1;m->moving=0;MotionClip(e,0,now);
done:
    if(!MotionCurrent(&lease,goal.assignment,now))goto lost;
    m=&cod1Actors[lease.actorIndex].motion;m->busy=0;
    if((m->arrived||scriptGoalReached)&&!m->notified){
        /* Commit before notify. A continuation may replace goal or delete self;
         * never write old pending state after this boundary. */
        m->notified=goal.assignment;
        if(current(context)!=1||!MotionCurrent(&lease,goal.assignment,now))return 0;
        return notify(context,ref)==1;
    }
    return 1;
lost:
    if(ActorLeaseValid(&lease))cod1Actors[lease.actorIndex].motion.busy=0;
    return 0;
}

/* Prepare the existing real graph path before the animation script asks
 * about its stances. This performs no displacement or goal notification. */
int G_Cod1TrainingJumperPreparePath(const GscEntityRef*ref,int now,int(*current)(void*),void*ctx){
 cod1ActorLease_t lease;cod1ActorGoalSnapshot_t goal;cod1ActorMotion_t*m;vec3_t target,origin;int ok;
 if(!ref||!current||current(ctx)!=1||!MotionLease(ref,&lease)||now!=level.time)return 0;
 goal=cod1Actors[lease.actorIndex].goal;
 if(!goal.assigned||!TrainingJumperExitNode(ref,goal.assignment,&goal.node)||
    !G_Cod1ActorMotionBeginUnpublished(ref,now)||!MotionCurrent(&lease,goal.assignment,now))return 0;
 m=&cod1Actors[lease.actorIndex].motion;if(m->busy||m->model||now<=m->lastTime||m->route.tower.prepared)return 0;
 if(m->assignment!=goal.assignment)ActorMotionAdoptGoal(m,goal.assignment,0);
 m->busy=1;VectorCopy(g_entities[ref->slot].r.currentOrigin,origin);
 ok=MotionJumperRoute(&lease,&goal,now,cod1Actors[lease.actorIndex].nativeData.entity.nativeClipMask,origin,target);
 if(ok==1&&MotionCurrent(&lease,goal.assignment,now)){
  cod1TowerPath_t*p=&cod1Actors[lease.actorIndex].motion.route.tower;
  p->prepared=1;p->preparedTick=now;VectorCopy(origin,p->preparedOrigin);
 }
 if(!MotionCurrent(&lease,goal.assignment,now))return 0;
 m=&cod1Actors[lease.actorIndex].motion;m->busy=0;
 return ok==1&&current(ctx)==1&&MotionCurrent(&lease,goal.assignment,now);
}
