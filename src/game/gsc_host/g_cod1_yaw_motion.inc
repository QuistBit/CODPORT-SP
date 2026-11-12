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
int G_Cod1YawMotionStorageProtected(const void *p,size_t n) {
    return !AmmoInventoryRange(p,n)||CAR_Overlap(p,n,cod1YawOwners,sizeof(cod1YawOwners))||
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
    if(cod1YawGuard.active)cod1YawGuard.cancelled=1;
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
