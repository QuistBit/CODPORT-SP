/* Independent real inline-mover angular operation, selected explicitly by the
 * typed host. No trap, fabricated movement event or actor controller. */
#include "../g_cod1_yaw_motion.h"
typedef struct {
    GscEntityRef entity;
    uint64_t serial;
    trajectory_t angular,position;
    vec3_t origin,mins,maxs,endpoint;
    int lastAngularTime,lastPositionTime,contents,serverFlags,modelIndex;
    char model[MAX_QPATH];
} Cod1YawOwner;
static Cod1YawOwner cod1YawOwners[MAX_GENTITIES];
static uint64_t cod1YawSerial;
static struct {int active,cancelled;uint64_t serial;} cod1YawGuard;
static int YawFinite(float value) {
    uint32_t bits;memcpy(&bits,&value,sizeof(bits));return (bits&UINT32_C(0x7f800000))!=UINT32_C(0x7f800000);
}
static int YawVector(const float v[3],float limit) {
    int i;for(i=0;i<3;++i)if(!YawFinite(v[i])||v[i]<-limit||v[i]>limit)return 0;return 1;
}
/* GAME 39E40 / 39A20, two-operand Training calls. A slot has
 * independent angular and linear trajectories on the same allocation. */
#include "../g_cod1_gsc_test.h"
static int YawMap(void);
typedef struct {
    GscEntityRef owner;
    trajectory_t trajectory;
    vec3_t endpoint;
    trajectory_t phases[3];
    int phase,phaseCount;
} ScriptMotionAxis;
static ScriptMotionAxis scriptMotion[MAX_GENTITIES][2];
static int ScriptMotionParent(const GscEntityRef *);
/* GAME 39960, 38E80 and 38D00. A phase starts on the server frame
 * finishing the previous phase. Only the final phase notifies movedone. */
static int ScriptMoveToPlan(const GscYawMotionRequest *q,const gentity_t *entity,ScriptMotionAxis *next){
    vec3_t start,base,target,delta,decelDirection;
    float seconds[3],speed;
    double denominator,length;
    int i,j;vec3_t direction;
    if(!q->operandsValid||!YawVector(q->velocity,10000000.0f)||!YawFinite(q->seconds)||!YawFinite(q->accel)||!YawFinite(q->decel)||
       q->seconds<=0||q->accel<0||q->decel<0||(double)q->accel+q->decel>q->seconds||
       (double)q->seconds*1000>INT_MAX-level.time||!YawVector(entity->r.currentOrigin,10000000.0f))return 0;
    VectorCopy(q->velocity,target);
    if(!YawVector(target,10000000.0f))return 0;
    VectorSubtract(target,entity->r.currentOrigin,delta);
    VectorCopy(entity->r.currentOrigin,start);
    if(entity->s.pos.trType!=TR_STATIONARY)BG_EvaluateTrajectory(&entity->s.pos,level.time,start);
    if(!YawVector(start,10000000.0f))return 0;
    VectorCopy(start,base);VectorCopy(target,next->endpoint);
    seconds[0]=q->accel;seconds[1]=(float)((double)q->seconds-q->accel-q->decel);seconds[2]=q->decel;
    denominator=2.0*q->seconds-q->accel-q->decel;
    length=sqrt((double)delta[0]*delta[0]+(double)delta[1]*delta[1]+(double)delta[2]*delta[2]);speed=(float)(2.0*length/denominator);
    for(j=0;j<3;j++)direction[j]=length?(float)(delta[j]/length):0;
    if(!YawFinite(speed))return 0;
    for(i=0;i<3;i++){
        trajectory_t *phase;int duration;
        if(seconds[i]==0)continue;
        duration=(int)((double)seconds[i]*1000.0);if(duration<1)return 0;
        VectorClear(decelDirection);
        if(i==2&&next->phaseCount){
            double magnitude;
            VectorSubtract(target,base,decelDirection);
            magnitude=sqrt((double)decelDirection[0]*decelDirection[0]+(double)decelDirection[1]*decelDirection[1]+(double)decelDirection[2]*decelDirection[2]);
            if(magnitude)for(j=0;j<3;j++)decelDirection[j]=(float)(decelDirection[j]/magnitude);
        }
        phase=&next->phases[next->phaseCount++];phase->trType=i==0?TR_ACCELERATE:i==1?TR_LINEAR_STOP:TR_DECCELERATE;
        phase->trTime=level.time;phase->trDuration=duration;
        for(j=0;j<3;j++){
            float velocity=(i==2&&next->phaseCount>1)?decelDirection[j]*speed:direction[j]*speed;
            phase->trBase[j]=base[j];
            if(!q->accel&&!q->decel)phase->trDelta[j]=(float)((double)delta[j]*1000.0/duration);
            else phase->trDelta[j]=i==1?(float)((double)velocity*seconds[i]*1000.0/duration):velocity;
            if(!YawFinite(phase->trDelta[j]))return 0;
        }
        BG_EvaluateTrajectory(phase,level.time+duration,base);
    }
    if(!next->phaseCount)return 0;
    next->owner=q->entity;next->trajectory=next->phases[0];return 1;
}

#include "g_cod1_roll_motion.inc"
#include "g_cod1_script_launch.inc"
static int ScriptMoveToInvoke(const GscYawMotionRequest *input,int (*current)(void *),void *context){
    GscYawMotionRequest request=*input;ScriptMotionAxis next={0},axisBefore;
    gentity_t before,*e;uint64_t epoch=cod1EntityEpoch;int now=level.time,count=level.num_entities,linked;
    if(request.now_ms<0||!YawMap()||!ActorOwnerAllowed(&request.entity,0))return 0;
    e=g_entities+request.entity.slot;before=*e;axisBefore=scriptMotion[request.entity.slot][0];
    if(!e->classname||(strcmp(e->classname,"script_origin")&&strcmp(e->classname,"script_model")&&strcmp(e->classname,"script_brushmodel")))return 0;
    linked=ScriptMotionParent(&request.entity);
    if(!linked&&!ScriptMoveToPlan(&request,&before,&next))return 0;
    if(current(context)!=1||!cod1YawGuard.active||cod1YawGuard.cancelled||!YawMap()||
       cod1EntityEpoch!=epoch||level.time!=now||level.num_entities!=count||
       memcmp(input,&request,sizeof(request))||!ActorOwnerAllowed(&request.entity,0)||
       memcmp(e,&before,sizeof(before))||memcmp(&axisBefore,&scriptMotion[request.entity.slot][0],sizeof(axisBefore))||
       ScriptMotionParent(&request.entity)!=linked)return 0;
    if(linked)return 1;
    scriptMotion[request.entity.slot][0]=next;e->s.pos=next.trajectory;
    trap_LinkEntity(e);
    return current(context)==1&&cod1YawGuard.active&&!cod1YawGuard.cancelled&&YawMap()&&
        level.time==now&&cod1EntityEpoch==epoch&&level.num_entities==count&&
        !memcmp(input,&request,sizeof(request))&&ActorOwnerAllowed(&request.entity,0)&&
        e->classname==before.classname&&ScriptMotionParent(&request.entity)==linked&&
        !memcmp(&e->s.pos,&next.trajectory,sizeof(next.trajectory))&&
        !memcmp(&scriptMotion[request.entity.slot][0],&next,sizeof(next));
}

static int ScriptMotionInvoke(const GscYawMotionRequest *input,int (*current)(void *),void *context){
    GscYawMotionRequest q=*input;
    ScriptMotionAxis next={0};
    gentity_t *entity;
    const float *base;
    int angular=q.method==COD1_ROTATE_VELOCITY,component;
    double duration;
    if(q.method==COD1_MOVE_TO)return ScriptMoveToInvoke(input,current,context);
    if(!angular && q.method!=COD1_MOVE_GRAVITY && q.method!=COD1_MOVE_TO)return 0;
    if(current(context)!=1 || cod1YawGuard.cancelled || memcmp(&q,input,sizeof(q)) || !YawMap() ||
       !ActorOwnerAllowed(&q.entity,0))return 0;
    entity=g_entities+q.entity.slot;
    if(!entity->classname || (strcmp(entity->classname,"script_origin") &&
       strcmp(entity->classname,"script_model") && strcmp(entity->classname,"script_brushmodel")))return 0;
    if(ScriptMotionParent(&q.entity))return 1;
    duration=(double)q.seconds*1000.0;
    if(!q.operandsValid || !isfinite(duration) || duration<1 || duration>INT_MAX-level.time)return 0;
    next.owner=q.entity;
    next.trajectory.trType=angular?TR_LINEAR_STOP:TR_GRAVITY;
    next.trajectory.trTime=level.time;
    next.trajectory.trDuration=(int)duration;
    base=angular?entity->r.currentAngles:entity->r.currentOrigin;
    for(component=0;component<3;component++){
        next.trajectory.trBase[component]=base[component];
        next.trajectory.trDelta[component]=q.velocity[component];
        next.endpoint[component]=(float)(base[component]+(double)(float)(next.trajectory.trDuration*0.001f)*q.velocity[component]);
        if(!isfinite(next.endpoint[component]))return 0;
        if(angular && fabs((double)next.endpoint[component])>11000000.0)return 0;
    }
    scriptMotion[q.entity.slot][angular]=next;
    if(angular)entity->s.apos=next.trajectory;else entity->s.pos=next.trajectory;
    trap_LinkEntity(entity);
    return 1;
}
/* Independent update and completion order in GAME 38D00. A completion
 * may retire the entity or replace either trajectory through its waiter. */
int G_Cod1ScriptMotionFrame(gentity_t *entity){
    GscEntityRef owner;
    int angular,active=0;
    if(!entity || entity->s.number<0 || entity->s.number>=MAX_GENTITIES)return 0;
    if(!scriptMotion[entity->s.number][0].owner.epoch && !scriptMotion[entity->s.number][1].owner.epoch)return 0;
    if(!G_Cod1EntityCapture(entity,&owner))return 0;
    for(angular=0;angular<2;angular++){
        ScriptMotionAxis *axis=&scriptMotion[owner.slot][angular];
        trajectory_t *tr=angular?&entity->s.apos:&entity->s.pos;
        float *world=angular?entity->r.currentAngles:entity->r.currentOrigin;
        int done,i;
        if(!axis->owner.epoch)continue;
        if(entity->s.eType==ET_MOVER && (int64_t)tr->trTime-axis->trajectory.trTime==
           (int64_t)level.time-level.previousTime)axis->trajectory.trTime=tr->trTime;
        if(!ActorRefEqual(&owner,&axis->owner) || memcmp(tr,&axis->trajectory,sizeof(*tr)) ||
           ScriptMotionParent(&owner)){memset(axis,0,sizeof(*axis));continue;}
        active=1;
        BG_EvaluateTrajectory(tr,level.time,world);
        done=level.time>=tr->trTime+tr->trDuration;
        if(done && axis->phase+1<axis->phaseCount){
            *tr=axis->phases[++axis->phase];
            tr->trTime=level.time;
            axis->trajectory=*tr;
            VectorCopy(tr->trBase,world);
            done=0;
        }
        if(done){
            if(angular || axis->phaseCount)VectorCopy(axis->endpoint,world);
            VectorCopy(world,tr->trBase);
            tr->trType=TR_STATIONARY;tr->trTime=level.time;
            if(angular)for(i=0;i<3;i++){
                int units=(int)((double)world[i]*(float)(65536.0/360.0));
                world[i]=(units&65535)*(360.0f/65536.0f);
                if(i!=YAW && world[i]>180)world[i]-=360;
            }
            memset(axis,0,sizeof(*axis));
        }
        if(angular)VectorCopy(world,entity->s.angles);else VectorCopy(world,entity->s.origin);
        trap_LinkEntity(entity);
        if(!G_Cod1EntitySameAllocation(&owner))return 2;
        if(g_debugMove.integer>1)
            G_Printf("[script motion] owner=%u/%u tick=%d axis=%d done=%d xyz=%.9g %.9g %.9g\n",
                     owner.slot,owner.generation,level.time,angular,done,world[0],world[1],world[2]);
        if(done){
            G_Cod1GscTestEntityEvent(entity,angular?"rotatedone":"movedone");
            if(!G_Cod1EntitySameAllocation(&owner))return 2;
        }
    }
    return active;
}

int G_Cod1YawMotionStorageProtected(const void *p,size_t n) {
    return CAR_Overlap(p,n,scriptLaunch,sizeof(scriptLaunch))||CAR_Overlap(p,n,&scriptLaunchCall,sizeof(scriptLaunchCall))||!AmmoInventoryRange(p,n)||CAR_Overlap(p,n,scriptMotion,sizeof(scriptMotion))||CAR_Overlap(p,n,cod1YawOwners,sizeof(cod1YawOwners))||
        CAR_Overlap(p,n,&cod1YawSerial,sizeof(cod1YawSerial))||
        CAR_Overlap(p,n,&cod1YawGuard,sizeof(cod1YawGuard))||
        CAR_Overlap(p,n,G_Cod1YawMotionHost(),sizeof(GscYawMotionHost));
}
static int YawBuffers(const GscYawMotionRequest *input,char *error,size_t size) {
    return AmmoInventoryPoolSafe()&&input&&!AmmoInventoryPrivate(input,sizeof(*input))&&
        !AmmoInventoryPrivate(error,size)&&!CAR_Overlap(input,sizeof(*input),error,size);
}
static int YawMap(void) {
    return cod1EntityReady&&ActorMapAvailable()&&!level.spawning&&!level.restarted&&
        cod1EntityEpoch!=UINT64_MAX&&level.time>=0&&level.gentities==g_entities&&
        level.num_entities<=ENTITYNUM_MAX_NORMAL&&AmmoInventoryPoolSafe();
}
static int YawBody(const GscEntityRef *ref,int stationary) {
    const gentity_t *e;int i;
    if(!YawMap()||!ActorOwnerAllowed(ref,0))return 0;e=&g_entities[ref->slot];
    if(!e->classname||strcmp(e->classname,"script_brushmodel")||e->s.eType!=ET_MOVER||
       !e->r.bmodel||!e->r.linked||e->s.solid!=SOLID_BMODEL||!EntityControlInlineModel(e)||
       e->r.contents!=CONTENTS_SOLID||e->r.svFlags!=SVF_USE_CURRENT_ORIGIN||
       e->physicsObject||e->neverFree||(e->flags&FL_TEAMSLAVE)||e->teamchain||e->teammaster||
       !YawVector(e->r.currentOrigin,16777216.0f)||!YawVector(e->r.currentAngles,36000.0f)||
       !YawVector(e->r.mins,16777216.0f)||!YawVector(e->r.maxs,16777216.0f)||
       e->s.pos.trType!=TR_STATIONARY||e->s.pos.trTime<0||e->s.pos.trTime>level.time||e->s.pos.trDuration||
       memcmp(e->s.pos.trDelta,vec3_origin,sizeof(vec3_t)))return 0;
    for(i=0;i<3;++i)if(e->r.mins[i]>e->r.maxs[i]||
       e->r.currentOrigin[i]!=e->s.pos.trBase[i]||e->r.currentOrigin[i]!=e->s.origin[i])return 0;
    if(stationary && (e->s.apos.trType!=TR_STATIONARY||e->s.apos.trDuration||
       memcmp(e->s.apos.trDelta,vec3_origin,sizeof(vec3_t))||
       memcmp(e->s.apos.trBase,e->r.currentAngles,sizeof(vec3_t))))return 0;
    return 1;
}
static int YawMilliseconds(float seconds,int *out) {
    double value;if(!YawFinite(seconds)||seconds<0||seconds>600)return 0;
    value=(double)seconds*1000.0+0.5;
    if(value<0||value>600000.5)return 0;*out=(int)value;return *out>=0&&*out<=600000;
}
void G_Cod1YawMotionReset(void) {
    if(scriptLaunchCall.active)scriptLaunchCall.cancelled=1;
    memset(scriptLaunch,0,sizeof(scriptLaunch));
    if(cod1YawGuard.active)cod1YawGuard.cancelled=1;
    memset(scriptMotion,0,sizeof(scriptMotion));
    memset(cod1YawOwners,0,sizeof(cod1YawOwners));
}
static int YawInvoke(void *user,const GscYawMotionRequest *input,char *error,size_t size,
    int (*current)(void *),void *context) {
    GscYawMotionRequest request;Cod1YawOwner staged;gentity_t before;gentity_t *e;
    uint64_t serial,epoch;gentity_t *entities;gclient_t *clients;
    int total,accel,decel,now,count,maxclients,ok=0;(void)user;
    if(!YawBuffers(input,error,size)||!current)return 0;
    if(cod1YawGuard.active){cod1YawGuard.cancelled=1;return 0;}
    request=*input;
    if(request.method){
        cod1YawGuard.active=1;cod1YawGuard.cancelled=0;
        ok=request.method==COD1_LAUNCH?ScriptLaunchInvoke(input,current,context):(request.method==502||request.method==COD1_ROTATE_ROLL||request.method==COD1_ROTATE_TO)?ScriptRotationInvoke(input,current,context):ScriptMotionInvoke(input,current,context);
        cod1YawGuard.active=0;cod1YawGuard.cancelled=0;
        if(!ok)EntityError(error,size,"script motion requires a live script entity, vector and duration");
        return ok;
    }
    if(cod1YawSerial==UINT64_MAX||request.now_ms<0||!YawFinite(request.yaw)||
       request.yaw < -3600||request.yaw>3600||!YawMilliseconds(request.seconds,&total)||total<1||
       !YawMilliseconds(request.accel,&accel)||!YawMilliseconds(request.decel,&decel)||
       accel>total||decel>total||accel+decel>total||!YawBody(&request.entity,1)||
       level.time>INT_MAX-total)goto refuse;
    e=&g_entities[request.entity.slot];before=*e;memset(&staged,0,sizeof(staged));
    staged.entity=request.entity;staged.serial=serial=++cod1YawSerial;
    staged.angular.trType=TR_COD1_YAW_R216;staged.angular.trTime=level.time;
    staged.angular.trDuration=total;memcpy(staged.angular.trBase,e->r.currentAngles,sizeof(vec3_t));
    staged.angular.trDelta[0]=(float)accel;staged.angular.trDelta[1]=request.yaw;
    staged.angular.trDelta[2]=(float)decel;staged.position=e->s.pos;
    memcpy(staged.origin,e->r.currentOrigin,sizeof(vec3_t));memcpy(staged.mins,e->r.mins,sizeof(vec3_t));
    memcpy(staged.maxs,e->r.maxs,sizeof(vec3_t));memcpy(staged.endpoint,e->r.currentAngles,sizeof(vec3_t));
    staged.endpoint[YAW]=staged.endpoint[YAW]+request.yaw;
    if(!YawVector(staged.endpoint,36000.0f)||strlen(e->model)>=sizeof(staged.model))goto refuse;
    strcpy(staged.model,e->model);staged.modelIndex=e->s.modelindex;
    staged.contents=e->r.contents;staged.serverFlags=e->r.svFlags;
    staged.lastAngularTime=staged.angular.trTime;staged.lastPositionTime=staged.position.trTime;
    epoch=cod1EntityEpoch;now=level.time;count=level.num_entities;maxclients=level.maxclients;
    entities=level.gentities;clients=level.clients;
    cod1YawGuard.active=1;cod1YawGuard.cancelled=0;cod1YawGuard.serial=serial;
    if(current(context)!=1||!cod1YawGuard.active||cod1YawGuard.cancelled||
       cod1YawGuard.serial!=serial||!YawMap()||cod1EntityEpoch!=epoch||level.time!=now||
       level.num_entities!=count||level.maxclients!=maxclients||level.gentities!=entities||
       level.clients!=clients||!YawBuffers(input,error,size)||memcmp(input,&request,sizeof(request))||
       !YawBody(&request.entity,1)||memcmp(&before,e,sizeof(before)))goto done;
    /* The callback is over. Publish the genuine angular trajectory; the real
     * frame mover performs all collision/push/link work asynchronously. */
    e->s.apos=staged.angular;cod1YawOwners[request.entity.slot]=staged;ok=1;
done:cod1YawGuard.active=0;cod1YawGuard.cancelled=0;
refuse:if(!ok&&YawBuffers(input,error,size))EntityError(error,size,"yaw motion requires a current independent stationary inline mover and finite ramp");
    return ok;
}
const GscYawMotionHost *G_Cod1YawMotionHost(void) {
    static const GscYawMotionHost host={NULL,YawInvoke};return &host;
}
static int YawTimeShift(int previous,int next,int frame) {
    return next==previous||(frame>=0&&previous<=INT_MAX-frame&&next==previous+frame);
}
void G_Cod1YawMotionFinish(gentity_t *e) {
    Cod1YawOwner *owner;trajectory_t expected,pos;vec3_t endpoint;int frame,i;int64_t elapsed;GscEntityRef ref;
    uintptr_t address=(uintptr_t)e,begin=(uintptr_t)g_entities;
    if(!e||address<begin||address-begin>=sizeof(g_entities)||
       (address-begin)%sizeof(gentity_t)||!G_Cod1EntityCapture(e,&ref))return;
    owner=&cod1YawOwners[ref.slot];
    if(!owner->serial||!LiveItemRefEqual(&owner->entity,&ref))return;
    if(!YawBody(&ref,0)||e->s.apos.trType!=TR_COD1_YAW_R216||
       e->s.modelindex!=owner->modelIndex||strcmp(e->model,owner->model)||
       e->r.contents!=owner->contents||e->r.svFlags!=owner->serverFlags||
       memcmp(e->r.mins,owner->mins,sizeof(vec3_t))||memcmp(e->r.maxs,owner->maxs,sizeof(vec3_t)))goto retire;
    elapsed=(int64_t)level.time-(int64_t)level.previousTime;
    frame=elapsed>=0&&elapsed<=INT_MAX?(int)elapsed:-1;
    expected=owner->angular;pos=owner->position;
    if(!YawTimeShift(owner->lastAngularTime,e->s.apos.trTime,frame)||
       !YawTimeShift(owner->lastPositionTime,e->s.pos.trTime,frame))goto retire;
    expected.trTime=e->s.apos.trTime;pos.trTime=e->s.pos.trTime;
    if(memcmp(&expected,&e->s.apos,sizeof(expected))||memcmp(&pos,&e->s.pos,sizeof(pos))||
       e->s.apos.trTime<owner->angular.trTime||e->s.apos.trTime>INT_MAX-e->s.apos.trDuration)goto retire;
    for(i=0;i<3;++i)if(e->r.currentOrigin[i]!=owner->origin[i]||e->s.origin[i]!=owner->origin[i])goto retire;
    owner->lastAngularTime=e->s.apos.trTime;owner->lastPositionTime=e->s.pos.trTime;
    if(level.time<e->s.apos.trTime+e->s.apos.trDuration)return;
    BG_EvaluateTrajectory(&e->s.apos,level.time,endpoint);
    for(i=0;i<3;++i)if(endpoint[i]!=owner->endpoint[i]||e->r.currentAngles[i]!=endpoint[i])return;
    /* No boundary remains: settle the reached pose, not a fabricated event. */
    memset(&e->s.apos,0,sizeof(e->s.apos));e->s.apos.trType=TR_STATIONARY;
    memcpy(e->s.apos.trBase,owner->endpoint,sizeof(vec3_t));
    memcpy(e->s.angles,owner->endpoint,sizeof(vec3_t));memcpy(e->r.currentAngles,owner->endpoint,sizeof(vec3_t));
retire:memset(owner,0,sizeof(*owner));
}
