/* Real native-CoD allocation and authoritative current pose. No donor spawn,
 * no renderer pose readback. Firing admission is a separate guarded contract. */
#include "../bg_cod1_turret_pose.h"
#include "../bg_cod1_turret_wire.h"
#include "../../qcommon/cod1_turret_animated_pose14.h"
#include "../bg_cod1_turret_target.h"
#include "../g_cod1_turret.h"
#include "g_cod1_turret_animation_bank.inc"
#define C1T_CAPACITY 8
typedef struct {
 GscEntityRef owner,actor,target;
 int used,modelIndex,poseTick,firingRequested,pendingStateNotify;unsigned mode;
 float controls[3],arc[4];
 int animated;float phases[2],weights[2];
 c1ttConfig_t config;c1ttState_t tracking;
 c1tpStaticPose_t pose;c1tpWorldFlash_t flash;unsigned nativeContents;
} C1TRecord;
static int TurretAnimationGunSamples(const C1TRecord *,int,float[2],float[2]);
static C1TRecord cod1Turrets[C1T_CAPACITY];
/* Initial idle mask proof only. A control handoff consumes this snapshot;
 * returning to the old mode/body never enrolls the allocation again. */
typedef struct {int valid;C1TRecord owner;gentity_t body;} C1TMaskRecord;
static C1TMaskRecord cod1TurretMasks[C1T_CAPACITY];
static c1mrResource_t cod1TurretModel;
static C1TRWeapon cod1TurretWeapon;
static c1aw14Skeleton_t *cod1TurretAnimatedSkeleton;
static uint64_t cod1TurretResourceEpoch;
static int cod1TurretBusy;
static uint64_t cod1TurretFrameBusyEpoch;
static int TurretStoragePrivate(const void *p,size_t n) {
 return ActorOverlap(p,n,cod1TurretMasks,sizeof(cod1TurretMasks))||ActorUnmountedTagPrivate(p,n)||ActorImmediatePrivate(p,n)||ActorGenericPrivate(p,n)||ActorLookPrivate(p,n)||TurretActorTagPrivate(p,n)||TurretShotPrivate(p,n)||TurretActorPublisherPrivate(p,n)||TurretMountedPrivate(p,n)||TurretFitPrivate(p,n)||TurretAnimationPrivate(p,n)||TurretMountPrivate(p,n)||ActorOverlap(p,n,&cod1TurretAnimatedSkeleton,sizeof(cod1TurretAnimatedSkeleton))||ActorOverlap(p,n,cod1TurretAnimatedSkeleton,sizeof(c1aw14Skeleton_t))||TurretBankPrivate(p,n)||ActorOverlap(p,n,cod1Turrets,sizeof(cod1Turrets))||ActorOverlap(p,n,&cod1TurretModel,sizeof(cod1TurretModel))||
  ActorOverlap(p,n,&cod1TurretWeapon,sizeof(cod1TurretWeapon))||ActorOverlap(p,n,&cod1TurretResourceEpoch,sizeof(cod1TurretResourceEpoch))||
  ActorOverlap(p,n,&cod1TurretBusy,sizeof(cod1TurretBusy))||
  ActorOverlap(p,n,&cod1TurretFrameBusyEpoch,sizeof(cod1TurretFrameBusyEpoch))||
  ActorOverlap(p,n,G_Cod1TurretControlHost(),sizeof(GscTurretHost));
}
static void TurretReset(void) {
 memset(cod1TurretMasks,0,sizeof(cod1TurretMasks));
 ActorUnmountedTagReset();ActorImmediateReset();ActorLookReset();ActorGenericReset();TurretActorTagReset();TurretShotReset();TurretActorPublisherReset();TurretMountedReset();TurretFitReset();TurretAnimationReset();TurretMountReset();TurretBankReset();C1AW14_Free(cod1TurretAnimatedSkeleton);cod1TurretAnimatedSkeleton=NULL;
 memset(cod1Turrets,0,sizeof(cod1Turrets));memset(&cod1TurretModel,0,sizeof(cod1TurretModel));
 memset(&cod1TurretWeapon,0,sizeof(cod1TurretWeapon));cod1TurretResourceEpoch=0;cod1TurretBusy=0;cod1TurretFrameBusyEpoch=0;
}
static C1TRecord *TurretFind(const GscEntityRef *ref) {
 int i;if(!ref||!G_Cod1EntitySameAllocation(ref))return NULL;
 for(i=0;i<C1T_CAPACITY;i++)if(cod1Turrets[i].used&&
    !memcmp(&cod1Turrets[i].owner,ref,sizeof(*ref)))return &cod1Turrets[i];
 return NULL;
}
int G_Cod1TurretInitialMaskCurrent(const GscEntityRef *ref) {
 C1TRecord *r,copy;C1TMaskRecord*m;gentity_t body;uint64_t presence;char error[128];
 if(!ref||!cod1EntityActive||!cod1EntityReady||level.spawning||level.restarted||
    cod1TurretBusy||cod1TurretFrameBusyEpoch||!(r=TurretFind(ref))||
    cod1TurretResourceEpoch!=ref->epoch||!cod1TurretAnimatedSkeleton||
    !G_Cod1ActorPresenceRead(ref,&presence,error,sizeof(error))||presence)return 0;
 m=cod1TurretMasks+(r-cod1Turrets);if(!m->valid)return 0;
 copy=*r;body=g_entities[ref->slot];
 if(r->poseTick<m->owner.poseTick||r->poseTick>level.time||
    body.s.cod1TurretTick!=r->poseTick)return 0;
 copy.poseTick=m->owner.poseTick;body.s.cod1TurretTick=m->body.s.cod1TurretTick;
 /* G_RunThink copies inactive RTCW script status, no script is admitted. */
 if(!memcmp(&body.scriptStatusCurrent,&body.scriptStatus,sizeof(body.scriptStatus)))
  body.scriptStatusCurrent=m->body.scriptStatusCurrent;
 return !memcmp(&copy,&m->owner,sizeof(copy))&&!memcmp(&body,&m->body,sizeof(body));
}
#include "g_cod1_turret_navigation_mask.inc"
static int TurretBlob(const char *path,void *data,int capacity) {
 fileHandle_t file=0;int bytes=trap_FS_FOpenFile(path,&file,FS_READ);
 if(bytes<=0||bytes>capacity||!file){if(file)trap_FS_FCloseFile(file);return 0;}
 memset(data,0,capacity);trap_FS_Read(data,bytes,file);trap_FS_FCloseFile(file);return bytes;
}
static int TurretResources(void) {
 unsigned char model[4096],parts[8192];char weapon[16384];
 int a,b,c;cwDef_t def;c1mrResource_t resource;C1TRWeapon plan;c1tpStaticPose_t pose;float zero[3]={0,0,0};
 c1aw14Skeleton_t *animated=NULL;uint64_t epoch=cod1EntityEpoch;
 if(cod1TurretResourceEpoch==epoch)return epoch!=0;
 if(!epoch||!cod1EntityActive||!level.spawning||cod1TurretBusy)return 0;
 cod1TurretBusy=1;
 a=TurretBlob("xmodel/mg42_bipod",model,sizeof(model));
 b=TurretBlob("xmodelparts/MG42_bipod1",parts,sizeof(parts));
 c=TurretBlob("weapons/sp/mg42_bipod_stand",weapon,sizeof(weapon));
 if(epoch!=cod1EntityEpoch||!cod1EntityActive||!level.spawning)goto fail;
 if(!a||!b||!c||CW_Parse(&def,"mg42_bipod_stand",weapon,c)<0||!C1TR_Weapon(&def,&plan)||
    !C1MR_DecodeResource("mg42_bipod",model,a,"MG42_bipod1",parts,b,&resource)||
    resource.skeleton.boneCount!=13||!C1TP_StaticResource(&resource,zero,&pose)||!C1AW14_Decode(parts,b,&animated))goto fail;
 cod1TurretModel=resource;cod1TurretWeapon=plan;cod1TurretAnimatedSkeleton=animated;animated=NULL;cod1TurretResourceEpoch=epoch;cod1TurretBusy=0;return 1;
fail:C1AW14_Free(animated);cod1TurretBusy=0;return 0;
}
/* Native 20022EE0 -> 2002FDB0 preloads animscripts/<WeaponDef.script>::main.
 * Read only resources already decoded during this map's real turret spawn. */
int G_Cod1TurretScriptModule(char *out,size_t cap) {
 char staged[128];size_t n,i;int live=0;
 if(!out||!cap||TurretStoragePrivate(out,cap)||!cod1EntityActive||
    !cod1TurretResourceEpoch||cod1TurretResourceEpoch!=G_Cod1EntityMapEpoch()||
    level.spawning||level.restarted||cod1TurretBusy)return 0;
 for(i=0;i<C1T_CAPACITY;i++)if(cod1Turrets[i].used&&
    G_Cod1EntitySameAllocation(&cod1Turrets[i].owner))live=1;
 if(!live||!memchr(cod1TurretWeapon.script,0,sizeof(cod1TurretWeapon.script)))return 0;
 n=strlen(cod1TurretWeapon.script);if(!n||n+sizeof("animscripts/")>sizeof(staged)||n+sizeof("animscripts/")>cap)return 0;
 for(i=0;i<n;i++){unsigned char c=(unsigned char)cod1TurretWeapon.script[i];
  if(!((c>='a'&&c<='z')||(c>='0'&&c<='9')||c=='_'||c=='/'))return 0;}
 if(cod1TurretWeapon.script[0]=='/'||cod1TurretWeapon.script[n-1]=='/'||strstr(cod1TurretWeapon.script,"//"))return 0;
 memset(staged,0,sizeof(staged));strcpy(staged,"animscripts/");strcat(staged,cod1TurretWeapon.script);
 memcpy(out,staged,n+sizeof("animscripts/"));return 1;
}
static int TurretPrepare(C1TRecord *r,int now) {
 gentity_t *e;c1tpStaticPose_t pose;c1tpWorldFlash_t flash;unsigned nativeContents;c1aw14Pose_t animatedPose;C1TRModel tags;
 float phases[2]={0,0},weights[2]={0,0};int animated,i;
 if(!r||!r->used||now!=level.time||now<0||!G_Cod1EntitySameAllocation(&r->owner)||
    cod1TurretResourceEpoch!=r->owner.epoch)return 0;
 e=&g_entities[r->owner.slot];
 if(!e->classname||strcmp(e->classname,"misc_mg42")||e->s.modelindex!=r->modelIndex)return 0;
 animated=TurretAnimationGunSamples(r,now,phases,weights);if(animated<0)return 0;
 if(animated){
  if(!cod1TurretAnimatedSkeleton||!C1TR_Model(&cod1TurretModel,&tags)||
     !C1TAP14_SampleGun(cod1TurretAnimatedSkeleton,(const c1ab14Clip_t*const*)cod1TurretGunBodies,phases,weights,r->controls,&animatedPose))return 0;
  memset(&pose,0,sizeof(pose));pose.bones=animatedPose.bones;pose.flash=tags.flash;
  for(i=0;i<13;i++){memcpy(pose.q[i],animatedPose.q[i],16);memcpy(pose.p[i],animatedPose.matrix[i]+12,12);}
 }else if(!C1TP_StaticResource(&cod1TurretModel,r->controls,&pose))return 0;
 if(!C1TP_WorldFlash(&pose,e->r.currentOrigin,e->r.currentAngles,&flash))return 0;
 r->pose=pose;r->flash=flash;r->poseTick=now;r->animated=animated;memcpy(r->phases,phases,8);memcpy(r->weights,weights,8);
 e->s.cod1TurretVersion=COD1_TURRET_WIRE_VERSION;e->s.cod1TurretEpochLo=(int)(uint32_t)r->owner.epoch;
 e->s.cod1TurretEpochHi=(int)(uint32_t)(r->owner.epoch>>32);e->s.cod1TurretGeneration=(int)r->owner.generation;
 e->s.cod1TurretTick=now;VectorCopy(r->controls,e->s.cod1TurretAngles);
 e->s.cod1TurretAnimated=animated;memcpy(e->s.cod1TurretPhases,phases,8);memcpy(e->s.cod1TurretWeights,weights,8);return 1;
}
#include "g_cod1_turret_mount_host.inc"
#include "g_cod1_turret_mount_fit.inc"
#include "g_cod1_turret_animation_host.inc"
#include "g_cod1_actor_generic_host.inc"
#include "g_cod1_turret_mounted_frame.inc"
#include "g_cod1_turret_actor_publisher.inc"
#include "g_cod1_actor_unmounted_publisher.inc"
#include "g_cod1_actor_face_host.inc"
#include "g_cod1_actor_scripted_resources.inc"
#include "g_cod1_actor_face_frame.inc"
#include "g_cod1_actor_face_tags.inc"
#include "g_cod1_actor_current_root.inc"
#include "g_cod1_actor_locomotion_resources.inc"
#include "g_cod1_actor_general_root_interval.inc"
#include "g_cod1_actor_scripted_frame.inc"
#include "g_cod1_actor_scripted_prefix.inc"
#include "g_cod1_actor_scripted_record.inc"
#include "g_cod1_actor_scripted_controller.inc"
#include "g_cod1_actor_controller_animation.inc"
#include "g_cod1_turret_actor_tags.inc"
#include "g_cod1_actor_unmounted_tags.inc"
#include "g_cod1_turret_shot_host.inc"
int G_Cod1TurretSpawn(gentity_t *ent) {
 GscEntityRef ref;C1TRecord record;char *value;int i,j,model;float arcs[4];
 const char *keys[4]={"leftarc","rightarc","toparc","bottomarc"};
 if(!ent||!level.spawning||!G_Cod1MapEntities()||!ent->classname||strcmp(ent->classname,"misc_mg42")||
    !G_Cod1EntityCapture(ent,&ref)||TurretFind(&ref)||!TurretResources()||!TurretBankLoad()||!TurretAnimationLoad()||!TurretActorPublisherBegin()||!G_Cod1EntitySameAllocation(&ref))return 0;
 if(trap_Cvar_VariableIntegerValue("g_cod1GscR232Turret")==1&&
    (!ActorGenericLookLoad()||!G_Cod1EntitySameAllocation(&ref)||!level.spawning))return 0;
 G_SpawnString("weaponinfo","",&value);if(strcmp(value,"mg42_bipod_stand"))return 0;
 G_SpawnString("model","",&value);if(Q_stricmp(value,"xmodel/mg42_bipod"))return 0;
 for(i=0;i<4;i++){
  arcs[i]=cod1TurretWeapon.arc[i];
  if(G_SpawnString(keys[i],"",&value)){
   char *end;double f=strtod(value,&end);if(!value[0]||*end||!(f>=0&&f<=180))return 0;arcs[i]=(float)f;
  }
 }
 for(i=0;i<C1T_CAPACITY;i++)if(!cod1Turrets[i].used||!G_Cod1EntitySameAllocation(&cod1Turrets[i].owner))break;
 if(i==C1T_CAPACITY)return 0;
 for(j=0;j<3;j++)if(!C1TP_Finite(ent->s.origin[j],1000000)||!C1TP_Finite(ent->s.angles[j],3600))return 0;
 model=G_ModelIndex(COD1_TURRET_MODEL_ALIAS);
 if(model<=0||model>=MAX_MODELS||!G_Cod1EntitySameAllocation(&ref))return 0;
 memset(&record,0,sizeof(record));record.owner=ref;record.used=1;record.modelIndex=model;record.mode=3;record.nativeContents=0x200004;
 memcpy(record.arc,arcs,sizeof(arcs));
 record.config.minPitch=-arcs[2];record.config.maxPitch=arcs[3];
 record.config.minYaw=-arcs[1];record.config.maxYaw=arcs[0];
 record.config.pitchRate=cod1TurretWeapon.turnSpeed[1];record.config.yawRate=cod1TurretWeapon.turnSpeed[0];
 record.tracking.lastTickMs=level.time-50;
 ent->s.eType=ET_GENERAL;ent->s.modelindex=model;ent->s.groundEntityNum=ENTITYNUM_NONE;
 ent->s.pos.trType=ent->s.apos.trType=TR_STATIONARY;
 G_SetOrigin(ent,ent->s.origin);G_SetAngle(ent,ent->s.angles);
 ent->r.contents=0;ent->r.svFlags|=SVF_USE_CURRENT_ORIGIN;ent->r.svFlags&=~SVF_NOCLIENT;
 ent->flags&=~FL_NODRAW;ent->s.eFlags&=~EF_NODRAW;ent->takedamage=qfalse;
 cod1Turrets[i]=record;
 if(!TurretShotSpawn(&cod1Turrets[i])||!TurretPrepare(&cod1Turrets[i],level.time)){memset(&cod1Turrets[i],0,sizeof(record));return 0;}
 trap_LinkEntity(ent);
 if(!G_Cod1EntitySameAllocation(&ref)||!TurretFind(&ref))return 0;
 memset(cod1TurretMasks+i,0,sizeof(cod1TurretMasks[i]));
 if(!ent->client&&!ent->think&&!ent->touch&&!ent->use&&!ent->die&&!ent->nextthink&&
    !ent->scriptEvents&&!ent->numScriptEvents&&ent->scriptStatus.scriptEventIndex==-1&&
    !ent->r.bmodel&&ent->r.linked&&!ent->r.contents){
  cod1TurretMasks[i].owner=cod1Turrets[i];cod1TurretMasks[i].body=*ent;cod1TurretMasks[i].valid=1;
 }
 return 1;
}
int G_Cod1TurretReadFlash(const GscEntityRef *ref,int now,float point[3],float forward[3]) {
 C1TRecord *r=TurretFind(ref);
 if(!point||!forward||!ActorGoalBuffers(ref,ref?sizeof(*ref):0,point,3*sizeof(float),NULL,0)||
    !ActorGoalBuffers(ref,sizeof(*ref),forward,3*sizeof(float),NULL,0)||ActorOverlap(point,12,forward,12)||
    !cod1EntityReady||!r||!TurretPrepare(r,now))return 0;
 VectorCopy(r->flash.position,point);VectorCopy(r->flash.forward,forward);return 1;
}
/* Preserve the complete caller request across MountRequest's borrowed fence.
 * Native UseTurret only records a request; GetTurret needs committed ownership. */
typedef struct {const GscTurretRequest *input;GscTurretRequest saved;GscTurretReply *out;char *error;size_t size;cod1ActorLease_t lease;int(*current)(void*);void *context;} C1TActorCall;
static int TurretActorCallCurrent(void *opaque){
 C1TActorCall*c=opaque;
 if(!c||!ActorLeaseValid(&c->lease)||memcmp(c->input,&c->saved,sizeof(c->saved))||
    !ActorGoalBuffers(c->input,sizeof(*c->input),c->out,sizeof(*c->out),c->error,c->size)||c->current(c->context)!=1)return 0;
 return cod1EntityReady&&!level.spawning&&!level.restarted&&ActorLeaseValid(&c->lease)&&!memcmp(c->input,&c->saved,sizeof(c->saved))&&c->saved.now_ms==level.time&&
  ActorGoalBuffers(c->input,sizeof(*c->input),c->out,sizeof(*c->out),c->error,c->size);
}
/* 68AC0 clears the request even when another actor owns the gun. Only the
 * matching occupant runs 6BE40 and wakes turretownerchange on the gun. */
/* State 3 Think (6CF97), queued operation 1 (4DCDC) and the
 * no-enemy, empty-route state 4 enter (65270). State 3 exit is empty
 * once the original request has been cleared at the script boundary. */
static int ActorDismountThink(cod1ActorRecord_t *actor,int now){
    cod1ActorLease_t lease;
    cod1ActorCriticalState_t queue;
    cod1ActorTurretMount_t *mount=&actor->turretMount;
    GscEntityRef owner=actor->owner;
    if(!TurretMountLease(&owner,&lease) || !TurretMountNoEnemy(&owner) ||
       now>INT_MAX-2000 || mount->occupiedGun.epoch ||
       actor->defaults.pathWords[(0x6f4-0x374)/4] ||
       actor->defaults.pathWords[(0x6f8-0x374)/4]>>16 ||
       actor->scriptedController.revision==UINT64_MAX)return 0;
    queue=ActorStateQueueKernel(actor);
    if(queue.command_count || queue.top || queue.stack[0]!=3 ||
       !Cod1ActorCriticalAppend(&queue,1,4))return 0;
    queue.stack[0]=4;
    ActorControllerRouteEmptyEnter(actor);
    actor->scriptedController.timer28=500;
    actor->scriptedController.timer2c=now+50;
    actor->scriptedController.timer34=now+2000;
    actor->scriptedController.base_entered_ms=now;
    actor->scriptedController.revision++;
    mount->currentStack[0]=4;
    queue.command_count=0;
    ActorStateQueueCommit(actor,&queue);
    if(g_debugMove.integer)G_Printf("[turret] dismounted actor=%u/%u time=%d state=4\n",
                                   lease.entity.slot,lease.entity.generation,now);
    return 1;
}
static int TurretActorStop(const cod1ActorLease_t *lease,int now,GscTurretReply *reply){
 cod1ActorTurretMount_t *mount=&cod1Actors[lease->actorIndex].turretMount;
 C1TRecord *gun;
 GscEntityRef occupant;
 if(cod1TurretMountBusyEpoch)return 0;
 reply->kind=GSC_TURRET_RESULT_VOID;
 if(mount->requestedGun.epoch){
    gun=TurretFind(&mount->requestedGun);
    if(!gun)return 0;
    if(ActorRefEqual(&gun->actor,&lease->entity)){
        if(!TurretMountOwnerCurrent(gun,now,&occupant) || !ActorRefEqual(&occupant,&lease->entity))return 0;
        reply->kind=GSC_TURRET_RESULT_OWNER_RELEASED;
        reply->entity=gun->owner;
        if(!ActorDismountTransfer(lease,gun,now))return 0;
        gun->tracking.flags &= ~0x200u;
        memset(&gun->actor,0,sizeof(gun->actor));
        /* Release just the mounted pose/parent. The actor, its VM threads,
         * state stack, allocation serial and claimed-node state stay live. */
        TurretActorPublisherRetire(&lease->entity);
        TurretMountedRetire(&lease->entity);
        TurretFitRetire(&lease->entity);
        TurretAnimationRetire(&lease->entity);
        memset(&mount->occupiedGun,0,sizeof(mount->occupiedGun));
        mount->mounted=0;
        mount->pendingOwnerNotify=mount->pendingStateNotify=mount->pendingStand=0;
    }
 }
 memset(&mount->requestedGun,0,sizeof(mount->requestedGun));
 return 1;
}
static int TurretActorControl(const GscTurretRequest*input,GscTurretReply*out,char*error,size_t size,int(*current)(void*),void*context){
 C1TActorCall call;GscTurretReply reply;cod1ActorTurretMount_t mount;C1TRecord*r;GscEntityRef owner;int accepted;
 if(!input||!out||!current||!ActorGoalBuffers(input,sizeof(*input),out,sizeof(*out),error,size))return 0;
 memset(&call,0,sizeof(call));call.input=input;call.saved=*input;call.out=out;call.error=error;call.size=size;call.current=current;call.context=context;
 if(!TurretMountLease(&call.saved.entity,&call.lease)||!TurretActorCallCurrent(&call))return 0;
 memset(&reply,0,sizeof(reply));
 if(call.saved.kind==GSC_TURRET_USE){
  if(!G_Cod1TurretMountRequest(&call.saved.entity,&call.saved.argument,call.saved.now_ms,TurretActorCallCurrent,&call,&accepted))return 0;
  reply.kind=GSC_TURRET_RESULT_BOOL;reply.boolean=accepted;
 }else if(call.saved.kind==GSC_TURRET_STOP_USE){
  if(!TurretActorStop(&call.lease,call.saved.now_ms,&reply))return 0;
 }else if(call.saved.kind==GSC_TURRET_GET){
  if(!G_Cod1TurretMountRead(&call.saved.entity,&mount))return 0;
  reply.kind=GSC_TURRET_RESULT_UNDEFINED;
  if(mount.mounted){r=TurretFind(&mount.occupiedGun);if(!r||!TurretMountOwnerCurrent(r,call.saved.now_ms,&owner)||!ActorRefEqual(&owner,&call.saved.entity))return 0;reply.kind=GSC_TURRET_RESULT_ENTITY;reply.entity=r->owner;}
 }else return 0;
 /* There are no callbacks after MountRequest commits. GET's second check is
  * pure ownership/time: querying a callback here could duplicate a USE commit. */
 if(!cod1EntityReady||level.spawning||level.restarted||!ActorLeaseValid(&call.lease)||memcmp(input,&call.saved,sizeof(call.saved))||call.saved.now_ms!=level.time||
    !ActorGoalBuffers(input,sizeof(*input),out,sizeof(*out),error,size))return 0;
 *out=reply;return 1;
}
static int TurretControlInvoke(void *unused,const GscTurretRequest *input,GscTurretReply *out,
 char *error,size_t size,int (*current)(void *),void *context) {
 GscTurretRequest q;GscTurretReply reply;C1TRecord before,next,*r;unsigned bits;(void)unused;
 if(!input||!out||!current||!ActorGoalBuffers(input,sizeof(*input),out,sizeof(*out),error,size))return 0;
 q=*input;
 /* Consume before any borrowed callback, even when a mutating request later
  * fails. Read-only queries do not extend or refresh initial provenance. */
 if(q.kind!=GSC_TURRET_GET&&q.kind!=GSC_TURRET_IS_FIRING&&(r=TurretFind(&q.entity)))
  cod1TurretMasks[r-cod1Turrets].valid=0;
 if(q.kind==GSC_TURRET_SHOOT)return TurretShotInvoke(input,out,error,size,current,context);
 if(q.kind==GSC_TURRET_USE||q.kind==GSC_TURRET_GET||q.kind==GSC_TURRET_STOP_USE)return TurretActorControl(input,out,error,size,current,context);
 if(!cod1EntityReady||level.spawning||level.restarted||q.now_ms!=level.time||current(context)!=1||
    !(r=TurretFind(&q.entity)))goto refused;
 before=*r;next=before;memset(&reply,0,sizeof(reply));reply.kind=GSC_TURRET_RESULT_VOID;
 switch(q.kind){
 case GSC_TURRET_SET_MODE:
  if(!memchr(q.mode,0,sizeof(q.mode))||!C1TR_ModeBits(q.mode,&bits))goto refused;
  next.mode=bits;break;
 case GSC_TURRET_START_FIRING:next.firingRequested=1;break;
 case GSC_TURRET_STOP_FIRING:next.firingRequested=0;break;
 /* Original 33669 clears turret+10 only. Tracking state and the request
  * to fire belong to separate native cells. */
 case GSC_TURRET_CLEAR_TARGET:memset(&next.target,0,sizeof(next.target));break;
 case GSC_TURRET_SET_TARGET_ENTITY:
  if(!G_Cod1EntitySameAllocation(&q.argument)||!g_entities[q.argument.slot].classname||
     strcmp(g_entities[q.argument.slot].classname,"script_origin"))goto refused;
  next.target=q.argument;
  if(!C1TT_BeginTarget(&next.tracking,((uint64_t)q.argument.generation<<32)|((uint64_t)q.argument.slot+1),q.now_ms))goto refused;
  break;
 case GSC_TURRET_IS_FIRING:reply.kind=GSC_TURRET_RESULT_BOOL;reply.boolean=C1TT_IsFiring(&next.tracking);break;
 default:
  goto refused;
 }
 if(current(context)!=1||level.time!=q.now_ms||!cod1EntityReady||level.spawning||level.restarted||
    !ActorGoalBuffers(input,sizeof(*input),out,sizeof(*out),error,size)||
    !(r=TurretFind(&q.entity))||memcmp(r,&before,sizeof(before))||memcmp(input,&q,sizeof(q))||
    (q.kind==GSC_TURRET_SET_TARGET_ENTITY&&!G_Cod1EntitySameAllocation(&q.argument)))goto refused;
 *r=next;*out=reply;return 1;
refused:EntityError(error,size,"native turret control requires a current owned allocation and a supported real operation");return 0;
}
/* Host capabilities are separately preflighted before admitting an original
 * function. Method availability alone does not certify stand/common/training. */
const GscTurretHost *G_Cod1TurretControlHost(void) {
 static const GscTurretHost host={NULL,TurretControlInvoke};return &host;
}
static int TurretFrameCore(int now,int (*current)(void *),int (*notify)(void *,const GscEntityRef *,const char *),void *context) {
 int i;uint64_t epoch=cod1EntityEpoch;
 if(!current||!notify||!cod1EntityReady||level.spawning||level.restarted||now!=level.time||current(context)!=1)return 0;
 for(i=0;i<C1T_CAPACITY;i++){
  C1TRecord *r=&cod1Turrets[i],next,before;c1ttTransition_t transition;float desired[2];GscEntityRef owner;
  if(!r->used)continue;
  if(!G_Cod1EntitySameAllocation(&r->owner)){memset(r,0,sizeof(*r));continue;}
  if(!TurretPrepare(r,now))return 0;
  before=next=*r;owner=r->owner;
  if(next.target.epoch&&now-next.tracking.lastTickMs>=50){
   if(next.mode!=0)return 0; /* Original auto_ai needs actual actor/enemy driver. */
   if(!G_Cod1EntitySameAllocation(&next.target)||
      !C1TT_Aim(&next.config,next.flash.position,g_entities[next.target.slot].r.currentOrigin,
               g_entities[owner.slot].r.currentAngles,desired)){
    if(!C1TT_RejectTarget(&next.tracking))return 0;
   }else{
    if(!C1TT_Tick(&next.config,&next.tracking,desired,g_entities[next.target.slot].r.currentOrigin,
                  next.firingRequested,now,&transition))return 0;
    next.pendingStateNotify|=transition.turretStateChange;
    next.controls[0]=next.tracking.pitch;next.controls[1]=next.tracking.yaw;next.controls[2]=next.tracking.pitchCorrection;
   }
   if(current(context)!=1||cod1EntityEpoch!=epoch||!cod1EntityReady||level.spawning||level.restarted||
      !G_Cod1EntitySameAllocation(&owner)||now!=level.time||memcmp(r,&before,sizeof(before)))return 0;
   *r=next;if(!TurretPrepare(r,now))return 0;
  }
  if(r->pendingStateNotify){
   before=*r;
   if(current(context)!=1||cod1EntityEpoch!=epoch||!cod1EntityReady||level.spawning||level.restarted||
      !G_Cod1EntitySameAllocation(&owner)||now!=level.time||memcmp(r,&before,sizeof(before)))return 0;
   r->pendingStateNotify=0;
   /* Outside Session invoke; continuation may delete/unload. Never touch r
    * after notify without recapturing full current allocation. */
   if(!notify(context,&owner,"turretstatechange"))return 0;
   if(current(context)!=1||cod1EntityEpoch!=epoch||!cod1EntityReady||now!=level.time)return 0;
  }
 }
 return 1;
}
int G_Cod1TurretFrame(int now,int (*current)(void *),int (*notify)(void *,const GscEntityRef *,const char *),void *context) {
 uint64_t epoch=cod1EntityEpoch;int ok;
 if(!epoch||cod1TurretFrameBusyEpoch)return 0;
 cod1TurretFrameBusyEpoch=epoch;ok=TurretFrameCore(now,current,notify,context);
 if(ok)ok=TurretShotFrame(now,current,context);
 if(cod1EntityEpoch!=epoch||!cod1EntityReady||level.spawning||level.restarted||now!=level.time)ok=0;
 if(cod1TurretFrameBusyEpoch==epoch)cod1TurretFrameBusyEpoch=0;
 return ok;
}

/* Optional read-only diagnostic of the committed live frame. Disabled by
 * default; it never advances a clock, selects a pose or produces an event. */
void G_Cod1TurretPoseTrace(const GscEntityRef*gun,int now){
 C1TRecord*r;C1TARecord*a;C1TFitProducer*p;C1TMFRecord*f;cod1ActorTurretMount_t*m;int index;
 if(trap_Cvar_VariableIntegerValue("g_cod1GscPoseTrace")!=1||now!=level.time||!gun||!(r=TurretFind(gun)))return;
 index=(int)(r-cod1Turrets);a=cod1TurretAnimations+index;p=cod1TurretFitProducers+index;
 if(!a->used||a->lease.actorIndex>=COD1_ACTOR_CAPACITY||!ActorLeaseValid(&a->lease)||!p->used||p->sample.tick!=now)return;
 m=&cod1Actors[a->lease.actorIndex].turretMount;f=cod1TurretFits+a->lease.actorIndex;
 G_Printf("[R236 pose] t=%d req=%d state=%d target=%u selected=%d pitch=%.7g yaw=%.7g corr=%.7g limit=%.7g flags=%x endpoint=%d rowZ=%.7g relZ=%.7g parentZ=%.7g modelWZ=%.7g modelAZ=%.7g offZ=%.7g actorZ=%.7g\n",now,r->firingRequested,r->tracking.trackingState,r->target.slot,a->selected,r->controls[0],r->controls[1],r->controls[2],r->tracking.pitchLimit,r->tracking.flags,p->sample.endpoint,p->sample.endpointRootZ,m->relativeOrigin[2],m->parentOrigin[2],p->sample.tagWeapon[14],p->sample.tagAim[14],f->smooth.positionOffset[2],m->actorOrigin[2]);
 G_Printf("[R236 anim] t=%d gunAim=%.7g gunFire=%.7g gunPhase=%.7g actorAim=%.7g actorFire=%.7g actorPhase=%.7g aimRow0=%.7g aimRow1=%.7g aimRow2=%.7g fireRow0=%.7g fireRow1=%.7g fireRow2=%.7g\n",now,a->gunNode[0].currentWeight,a->gunNode[1].currentWeight,a->gunNode[1].phase,a->actorTree[0].node[0].currentWeight,a->actorTree[1].node[0].currentWeight,a->actorTree[1].node[0].phase,a->actorTree[0].node[1].currentWeight,a->actorTree[0].node[2].currentWeight,a->actorTree[0].node[3].currentWeight,a->actorTree[1].node[1].currentWeight,a->actorTree[1].node[2].currentWeight,a->actorTree[1].node[3].currentWeight);
}

int FoleySessionQueueFrameNote(const cod1ActorFaceNote_t *note);
int FoleyRunNoteBucket(const GscEntityRef*,int,uint64_t,const cod1ActorLease_t*,int*);
#include "g_cod1_course_profile46.h"
static const cod1ActorFaceNote_t *foleyDelivering;
int FoleyFrameNoteCurrent(const cod1ActorFaceNote_t *note){
 C1FaceFrameCall *c=cod1FaceFrameOperation;
 const C1FaceFrame *f;
 if(!note||foleyDelivering!=note||!c||!ActorFaceFrameFence(c))return 0;
 f=&c->frame;
 return f->selected&&f->scripted&&f->open&&!f->poisoned&&f->nextNote<f->count&&
     f->count<=C1ANC14_MAX_NOTES&&note->ordinal<C1GA_COUNT&&note->generation&&
     note->generation==c->saved.notify->generation[note->ordinal]&&
     ActorRefEqual(&note->entity,&c->owner)&&c->now==note->produced_ms&&
     note->frame_generation==f->frame&&note->owner_identity==f->identity&&
     note->catalog==f->catalog&&note->runtime==f->runtime&&
     !memcmp(note,&f->notes[f->nextNote],sizeof(*note));
}
static int FoleyPrivateFrameNote(void *ctx,const cod1ActorFaceNote_t *note){
 int ok;
 if(foleyDelivering)return 0;
 foleyDelivering=note;
 ok=FoleyFrameNoteCurrent(note)&&CourseClockQueue46(note)&&FoleyFrameNoteCurrent(note);
 foleyDelivering=NULL;
 return ok;
}
static int MoodyIdleTree(const C1GARecord *);
static int FoleyDispatchFrame(const GscEntityRef *ref,int now,uint64_t generation,int (*current)(void*),void *ctx,int beforeVm,const cod1ActorLease_t *prepared){
 cod1ActorLease_t lease;
 C1FaceFrame *f;
 cod1ActorScriptedController_t *owner;
 int resume=0;
 if(!current || current(ctx)!=1 || !ActorGoalLease(ref,&lease) || now!=level.time)return 0;
 if(!ActorScriptedClockSelected(lease.actorIndex))return G_Cod1ActorGenericFrame(ref,now,current,ctx);
 f=cod1FaceFrames+lease.actorIndex; owner=&cod1Actors[lease.actorIndex].scriptedController;
 if(g_debugMove.integer&&now<=250)G_Printf("[Foley private] scripted frame entry slot=%u tick=%d generic=%d gen=%llu frame=%llu entered=%d entered_ms=%d done=%d open=%d count=%d poisoned=%d\n",ref->slot,f->tick,cod1GenericActors[lease.actorIndex].tick,(unsigned long long)generation,(unsigned long long)f->frame,owner->entered,owner->entered_ms,f->done,f->open,f->count,f->poisoned);
 if(!generation || !owner->entered || owner->invalid || !owner->initTask || owner->actorSerial!=lease.serial ||
    memcmp(&lease,&f->lease,sizeof(lease)) || owner->runtime!=f->runtime || owner->catalog!=f->catalog ||
    f->poisoned || f->count)return 0;
 if(prepared){
    if(!beforeVm||memcmp(prepared,&lease,sizeof(lease))||(!ActorStandingBody(cod1GenericActors+lease.actorIndex)&&!ActorRunnerFaceOwner(cod1GenericActors+lease.actorIndex)&&!ActorOrdinaryClockOwner(cod1GenericActors+lease.actorIndex)&&!TrainingJumperAllocation(ref)&&!MoodyIdleTree(cod1GenericActors+lease.actorIndex))||
       !f->open||f->done||f->frame!=generation||f->tick!=now||cod1GenericActors[lease.actorIndex].tick!=now)return 0;
 }else if(f->open)return 0;
 if(f->frame==generation && f->tick==now && f->done && cod1GenericActors[lease.actorIndex].tick==now){
    float weight[98]={0},phase[2][21]={{0}},facial[2][25];
    C1GARecord *record=&cod1GenericActors[lease.actorIndex];
    if(!CourseClockSample46(ref,now,weight,phase,facial,current,ctx))return 0;
    if(g_debugMove.integer&&now<2200)G_Printf("[Foley pose] owner=%u/%u tick=%d body880Goal=%g bodyWeight=%g bodyPhase=%g bodyToken=%u face1647Goal=%g faceWeight=%g facePhase=%g faceToken=%u\n",
        ref->slot,ref->generation,now,record->nodes[880].goalWeight,weight[58],phase[0][7],record->notify->flags[880].token,
        record->nodes[1647].goalWeight,weight[59],phase[0][8],record->notify->flags[1647].token);
    return current(ctx)==1;
 }
 if(!f->frame && f->tick==now && owner->entered_ms==now && cod1GenericActors[lease.actorIndex].tick==now)return 1;
 if(!prepared&&!CourseClockBegin46(ref,now,generation,current,ctx)){G_Printf("[Foley private] frame begin refused slot=%u now=%d\n",ref->slot,now);return 0;}
 do{
   cod1ActorLease_t after;
   if(!CourseClockStep46(ref,now,generation,&resume,current,ctx))return 0;
   if(f->open&&!CourseClockDrain46(ref,now,generation,current,FoleyPrivateFrameNote,ctx))return 0;
   if(resume){
    int retired=0;
    if(!CourseClockBucket46(ref,now,generation,beforeVm?&lease:NULL,&retired)||current(ctx)!=1)return 0;
    if(retired)return 1; /* Only the actor work ended. The VM still owns this frame. */
    if(!ActorGoalLease(ref,&after)||
       memcmp(&after,&lease,sizeof(after))||!owner->entered||owner->invalid||owner->actorSerial!=lease.serial||
       owner->runtime!=f->runtime||owner->catalog!=f->catalog||f->poisoned||!f->open||f->count||
       f->frame!=generation||f->tick!=now||memcmp(&lease,&f->lease,sizeof(lease))||
       (cod1Actors[lease.actorIndex].nativeData.entity.nativeEntityFlags&0x10000u))return 0;
   }
 }while(resume);
 if(g_debugMove.integer&&(generation<8 || (ref->slot==199 && now<=1500)))G_Printf("[Foley private] selected frame owner=%u now=%d gen=%llu closed=%d\n",ref->slot,now,(unsigned long long)generation,!f->open);
 return f->done && !f->open && !f->count && !f->poisoned && current(ctx)==1;
}

int FoleyPrivateDispatch(const GscEntityRef *ref,int now,uint64_t generation,int (*current)(void*),void *ctx){
 return FoleyDispatchFrame(ref,now,generation,current,ctx,0,NULL);
}

/* The original run::main now owns body clearing and animation selection.
 * This retains only the existing motion controller's rate receipt, with the
 * same closed-frame lease. It does not mutate the animation tree. */
int TrainingJumperRunRate(const GscEntityRef *ref,int now,float rate,int(*current)(void*),void *ctx){
 C1FaceFrameCall *c;int ok=0;
 if(!(rate>=.8f&&rate<=1.2f)||!TrainingJumperAllocation(ref)||cod1Actors[trainingJumperOwner.actorIndex].courseRunRate)return 0;
 c=ActorFaceFrameAcquire(ref,now,current,ctx,0);if(!c)return 0;
 if(c->frame.open||!c->frame.done||c->saved.tick!=now||c->baseline.nodes[892].phase<1)goto done;
 if(current(ctx)!=1||!ActorFaceFrameFence(c))goto done;
 cod1Actors[c->lease.actorIndex].courseRunRate=rate;ok=1;
 done:ActorFaceFrameLeave(c);return ok;
}
int TrainingJumperClock(int now,uint64_t generation,int prepare,int(*current)(void*),void *ctx){
 GscEntityRef ref;cod1ActorLease_t l;const C1FaceFrame *f;
 if(!TrainingJumperRead(&ref))return 1;
 if(!ActorGoalLease(&ref,&l)||!ActorScriptedClockSelected(l.actorIndex)||!current(ctx))return 0;
 f=cod1FaceFrames+l.actorIndex;
 if(f->tick==now&&!f->frame&&!f->open)return 1;
 if(prepare)return CourseClockBegin46(&ref,now,generation,current,ctx);
 if(f->tick!=now||f->frame!=generation||!f->open||f->count||!ActorRefEqual(&f->lease.entity,&ref))return 0;
 {int ok=FoleyDispatchFrame(&ref,now,generation,current,ctx,1,&l);
 if(!ok)G_Printf("[Training jump exit] actor clock refused now=%d frame=%llu open=%d done=%d count=%u\n",now,generation,f->open,f->done,f->count);return ok;}
}
int TrainingMoodyFrame(const GscEntityRef *ref,int now,uint64_t generation,int prepare,int(*current)(void*),void *ctx){
 cod1ActorLease_t lease;const C1FaceFrame *f;
 if(!ActorGoalLease(ref,&lease)||now!=level.time||current(ctx)!=1)return 0;
 if(!ActorScriptedClockSelected(lease.actorIndex))return 1;
 if(!MoodyIdleTree(cod1GenericActors+lease.actorIndex)){G_Printf("[382 frame] MoodyIdleTree refused tick=%d prepare=%d\n",now,prepare);return 0;}
 f=cod1FaceFrames+lease.actorIndex;
 if(f->tick==now&&!f->frame&&!f->open)return 1;
 if(prepare)return CourseClockBegin46(ref,now,generation,current,ctx);
 {int ok=FoleyDispatchFrame(ref,now,generation,current,ctx,1,&lease);if(!ok)G_Printf("[382 frame] dispatch refused tick=%d prepare=%d\n",now,prepare);return ok;}
}
static int FoleyPrivateSelectedBeforeVm(const GscEntityRef *ref,int now,uint64_t generation,int (*current)(void*),void *ctx,cod1ActorLease_t *lease){
 if(!ActorGoalLease(ref,lease) || current(ctx)!=1 || now!=level.time)return 0;
 if(!ActorScriptedClockSelected(lease->actorIndex))return 1;
 return FoleyDispatchFrame(ref,now,generation,current,ctx,1,NULL);
}

int FoleyPrivateQueuedController(const GscEntityRef *ref,int now,uint64_t frame,uint64_t *serial){
 cod1ActorLease_t lease;
 C1FaceFrame *clock;
 if(!serial || !ref || now!=level.time || !ActorGoalLease(ref,&lease))return -1;
 if(!ActorScriptedClockSelected(lease.actorIndex))return 0;
 if(cod1Actors[lease.actorIndex].scriptedAux.prefix!=FRESH_PREFIX_QUEUE_COMMITTED){
  const cod1ActorRecord_t *a=cod1Actors+lease.actorIndex;
  if(!a->scriptedController.selectionSerial)return 0;
  if(a->scriptedController.currentSource==1||a->scriptedController.currentSource==2)
   return ActorLocomotionControllerCurrent(a,lease.serial)?0:-1;
  if(!a->scriptedController.entered||a->scriptedController.invalid||a->scriptedController.resumed4||
     a->scriptedController.actorSerial!=lease.serial||a->scriptedController.currentSource!=4||
     a->turretMount.currentTop!=1||a->turretMount.currentStack[0]!=4||a->turretMount.currentStack[1]!=9)return -1;
 }
 clock=cod1FaceFrames+lease.actorIndex;
 if(clock->frame!=frame || clock->tick!=now || !clock->done || clock->open || clock->count || clock->poisoned ||
    memcmp(&lease,&clock->lease,sizeof(lease)))return -1;
 *serial=lease.serial;return 1;
}

/* Transfer only the update responsibility. The canonical tree, phase, goal
 * weights, notify owner, runtime, catalog and actor allocation stay in place. */
static int ActorTransferClockAtMount(const cod1ActorLease_t *l,int now){
 unsigned i;C1FaceFrame *f;C1GARecord *tree;cod1ActorRecord_t *a;
 if(!l||!ActorLeaseValid(l))return 0;
 f=cod1FaceFrames+l->actorIndex;
 if(!f->selected)return 1;
 a=cod1Actors+l->actorIndex;tree=cod1GenericActors+l->actorIndex;
 if(!f->scripted||!f->done||f->open||f->count||f->poisoned||!f->frame||
    f->gameFrame!=level.framenum||f->tick!=now||tree->tick!=now||!tree->used||!tree->nodes||!tree->notify||
    cod1FaceFrameOperation||cod1GenericOperation.busy||cod1ControllerOperation.busy||
    f->runtime!=tree->runtime||f->catalog!=tree->catalog||f->identity!=tree->notify->ownerIdentity||
    memcmp(l,&tree->lease,sizeof(*l))||memcmp(l,&f->lease,sizeof(*l))||cod1NormalRecords[l->actorIndex]||
    a->scriptedController.actorSerial!=l->serial||!a->scriptedController.entered||a->scriptedController.invalid||
    a->scriptedController.selectionSerial||a->scriptedAux.record!=FRESH_AUX_EMPTY||
    a->scriptedAux.link!=FRESH_LINK_UNATTACHED||a->scriptedAux.prefix!=FRESH_PREFIX_UNUSED||
    a->turretMount.currentTop||a->turretMount.currentStack[0]!=4||a->turretMount.mounted)return 0;
 for(i=0;i<C1GA_COUNT;i++){
  if(tree->notify->flags[i].token||tree->notify->pending[i])return 0;
  if(!cod1GenericDefinitions[i].children && (tree->nodes[i].currentWeight||tree->nodes[i].goalWeight) &&
     ActorGenericShuffleIndex(cod1GenericNames[i].name)<0&&C1AL14_Index(cod1GenericNames[i].name)<0)return 0;
 }
 G_Printf("[Foley course] closed unmounted clock owner=%u/%u tick=%d treeRevision=%llu\n",l->entity.slot,l->entity.generation,now,(unsigned long long)tree->revision);
 memset(f,0,sizeof(*f));return 1;
}

/* AI row4 Think has no qualified provider yet. Only the already selected
 * presentation clock can run independently while this exact row stays put. */
static int FoleyReturned4Selection(const GscEntityRef *ref,uint64_t serial,uint64_t selection,int queued,cod1ActorLease_t *selected){
 cod1ActorLease_t lease;
 const cod1ActorRecord_t *actor;
 const cod1ActorScriptedController_t *ctl;
 if(!selection||!ActorGoalLease(ref,&lease)||lease.serial!=serial)return 0;
 actor=cod1Actors+lease.actorIndex;ctl=&actor->scriptedController;
 *selected=lease;
 return ActorScriptedClockSelected(lease.actorIndex)&&ctl->entered&&!ctl->invalid&&ctl->resumed4&&
    ctl->actorSerial==serial&&ctl->selectionSerial==selection&&ctl->currentSource==4&&!ctl->force250&&
    !actor->turretMount.currentTop&&actor->turretMount.currentStack[0]==4&&!actor->turretMount.mounted&&
    (queued ? (actor->critical.command_count==1&&actor->critical.top==1&&actor->critical.stack[0]==4&&
      actor->critical.stack[1]==9&&actor->critical.commands[0].operation==2&&actor->critical.commands[0].state==9) :
      !actor->critical.command_count)&&!cod1NormalRecords[lease.actorIndex]&&
    actor->scriptedAux.record==FRESH_AUX_EMPTY&&actor->scriptedAux.link==FRESH_LINK_UNATTACHED&&
    actor->scriptedAux.prefix==(queued?FRESH_PREFIX_QUEUE_COMMITTED:FRESH_PREFIX_UNUSED)&&!cod1ControllerOperation.busy;
}

int FoleyReturnedBodyReleased(const GscEntityRef *ref,uint64_t serial,uint64_t selection){
 cod1ActorLease_t lease;
 if(!FoleyReturned4Selection(ref,serial,selection,0,&lease))return 0;
 return cod1Actors[lease.actorIndex].critical.top==0 && cod1Actors[lease.actorIndex].critical.stack[0]==4;
}

static int FoleyReturned4Owner(const GscEntityRef *ref,uint64_t serial,uint64_t selection,int now,uint64_t frame,int queued){
 cod1ActorLease_t lease;
 const C1FaceFrame *clock;
 const C1GARecord *tree;
 const cod1ActorScriptedController_t *ctl;
 if(!frame||now!=level.time||!FoleyReturned4Selection(ref,serial,selection,queued,&lease))return 0;
 ctl=&cod1Actors[lease.actorIndex].scriptedController;
 clock=cod1FaceFrames+lease.actorIndex;tree=cod1GenericActors+lease.actorIndex;
 return tree->used&&tree->tick==now&&tree->runtime==ctl->runtime&&tree->catalog==ctl->catalog&&
    !memcmp(&tree->lease,&lease,sizeof(lease))&&!memcmp(&clock->lease,&lease,sizeof(lease))&&
    clock->runtime==tree->runtime&&clock->catalog==tree->catalog&&clock->frame==frame&&clock->tick==now&&
    clock->done&&!clock->open&&!clock->count&&!clock->poisoned;
}
int FoleyReturned4PoseCurrent(const GscEntityRef*ref,uint64_t serial,uint64_t selection,int now,uint64_t frame){
 return FoleyReturned4Owner(ref,serial,selection,now,frame,0);
}
int FoleyReturned4RepeatQueued(const GscEntityRef*ref,uint64_t serial,uint64_t selection,int now,uint64_t frame){
 return FoleyReturned4Owner(ref,serial,selection,now,frame,1);
}

static int ActorHasNormalRecord(unsigned index){return index<COD1_ACTOR_CAPACITY && cod1NormalRecords[index]!=NULL;}

/* Only the unmounted runner's real INIT can install this clock. A GetEye
 * request may then arrive every VM bucket. Tick once before that bucket,
 * under the live level/Session lease, without advancing AI state or notes. */
typedef struct {
 unsigned next,count,stage;
 GscEntityRef instructor;
 cod1ActorLease_t instructorLease,speaking[COD1_ACTOR_CAPACITY];
} CourseClockCursor;

static int CourseClockRunner(CourseClockCursor *cursor,unsigned i,int now,uint64_t frame,
                             int (*current)(void*),void *ctx){
  const C1FaceFrame *f=cod1FaceFrames+i;
  cod1ActorLease_t lease;
  const cod1ActorRecord_t *a;
  lease=f->lease;
  if(lease.actorIndex!=i||!ActorLeaseValid(&lease)){G_Printf("[474C course boundary] tick=%d line=%d\n",now,__LINE__);return 0;}
  if(!g_entities[lease.entity.slot].targetname||strcmp(g_entities[lease.entity.slot].targetname,"runner"))return 1;
  a=cod1Actors+i;
  /* Native ordinary MAIN now has an explicit retained selection. Advance
   * its canonical prepared tree before VM just as the initial standing
   * owner, including a genuine critical8 transition. No second clock. */
  if((!ActorOrdinaryClockOwner(cod1GenericActors+i)&&
     (!a->scriptedController.entered||a->scriptedController.invalid||a->scriptedController.selectionSerial||
      a->turretMount.mounted||a->turretMount.currentTop||a->turretMount.currentStack[0]!=4||
      cod1NormalRecords[i]||a->scriptedAux.record!=FRESH_AUX_EMPTY||a->scriptedAux.prefix!=FRESH_PREFIX_UNUSED))||
     !cod1GenericActors[i].notify||f->count||f->open||f->poisoned){G_Printf("[474C course boundary] tick=%d line=%d\n",now,__LINE__);return 0;}
  if(ActorGenericNamedActive(cod1GenericActors+i)){
   if((!ActorStandingBody(cod1GenericActors+i)&&!ActorRunnerFaceOwner(cod1GenericActors+i)&&!ActorOrdinaryClockOwner(cod1GenericActors+i))||
      !ActorFaceFrameClosure(cod1GenericActors+i)){G_Printf("[474C course boundary] tick=%d line=%d\n",now,__LINE__);return 0;}
   cursor->speaking[cursor->count++]=lease;
  }else if(!FoleyPrivateDispatch(&lease.entity,now,frame,current,ctx)||!ActorLeaseValid(&lease)){G_Printf("[474C course boundary] tick=%d line=%d\n",now,__LINE__);return 0;}
 return 1;
}
static int TrainingCourseClockSteps(CourseClockCursor *cursor,const GscEntityRef *instructor,
    int now,uint64_t frame,int (*current)(void*),void *ctx,int (*expired)(void)){
 if(!frame||now!=level.time||!current||current(ctx)!=1)return 0;
 if(cursor->stage==0){
  if(instructor)cursor->instructor=*instructor;
  cursor->stage=1;
 }else if(!instructor||instructor->epoch!=cursor->instructor.epoch||
          instructor->slot!=cursor->instructor.slot||instructor->generation!=cursor->instructor.generation)return 0;
 while(cursor->stage==1 && cursor->next<COD1_ACTOR_CAPACITY){
  unsigned i=cursor->next++;
  if(!cod1FaceFrames[i].selected||!cod1FaceFrames[i].scripted)continue;
  if(!CourseClockRunner(cursor,i,now,frame,current,ctx))return 0;
  if(expired && expired())return 2;
 }
 if(cursor->stage==1){cursor->stage=2;cursor->next=0;}
 /* An actual notetrack can wake scripts that read either actor's eye tag.
  * Make Elder's current tick available before Foley can wake a waiter. Then
  * finish Elder's already-open frame after the other actor poses are current. */
 while(cursor->stage==2 && cursor->next<cursor->count){
  const cod1ActorLease_t *l=&cursor->speaking[cursor->next++];
  if(!CourseClockBegin46(&l->entity,now,frame,current,ctx))return 0;
  if(expired && expired())return 2;
 }
 if(cursor->stage==2){
  if(!FoleyPrivateSelectedBeforeVm(instructor,now,frame,current,ctx,&cursor->instructorLease)||current(ctx)!=1){G_Printf("[474C course boundary] tick=%d line=%d\n",now,__LINE__);return 0;}
  cursor->stage=3;cursor->next=0;
  if(cursor->count && expired && expired())return 2;
 }
 while(cursor->next<cursor->count){
  const cod1ActorLease_t *l=&cursor->speaking[cursor->next++];
  if(!ActorLeaseValid(l)){if(!G_Cod1EntityDeleteRetiredIdentity(&l->entity,l)){G_Printf("[474C course boundary] tick=%d line=%d\n",now,__LINE__);return 0;}continue;}
  if(!FoleyDispatchFrame(&l->entity,now,frame,current,ctx,1,l)||current(ctx)!=1){G_Printf("[474C course boundary] tick=%d line=%d\n",now,__LINE__);return 0;}
  /* A note can retire the whole course. Once the last actor finishes,
   * return completion before the outer scheduler checks its deadline. */
  if(cursor->next < cursor->count && expired && expired())return 2;
 }
 return 1;
}
int TrainingCourseClocksBeforeVm(const GscEntityRef *instructor,int now,uint64_t frame,
                                 int (*current)(void*),void *ctx){
 CourseClockCursor cursor;
 cursor.stage=cursor.next=cursor.count=0;
 return TrainingCourseClockSteps(&cursor,instructor,now,frame,current,ctx,NULL);
}
int FoleySameBucketRetirement(const GscEntityRef *,const cod1ActorLease_t *);
int TrainingCourseClocksBeforeVmSlice(const GscEntityRef *instructor,int now,uint64_t frame,
    int (*current)(void*),void *ctx,int resume,int (*expired)(void)){
 static CourseClockCursor cursor;
 if(!resume)cursor.stage=cursor.next=cursor.count=0;
 /* After Foley's step, the remaining runners own their individual leases.
  * A note can commit the original instructor's deletion before we resume. */
 if(!G_Cod1EntitySameAllocation(instructor)){
  if(!resume || cursor.stage!=3 || !FoleySameBucketRetirement(instructor,&cursor.instructorLease))return 0;
  if(g_debugMove.integer)G_Printf("[Training course] resumed after committed instructor retirement tick=%d remaining=%u\n",
                                  now,cursor.count-cursor.next);
 }
 return TrainingCourseClockSteps(&cursor,instructor,now,frame,current,ctx,expired);
}

/* A real idle face may span a new body request. Before the normal record
 * commits, only its live input tree or its own staged tree has authority. */
/* Named standing animation on the placed, unmounted instructor. The same
 * allocation may enter a normal-record gesture after this selection. */
static int ActorSpeakingFaceLeaf(unsigned source){
 int index=ActorScriptedIndex(source);
 return ActorFaceLeafIndex(source)>=0 || (index>=0 && !C1ASB14_Body(index));
}
static int MoodyIdleTree(const C1GARecord *tree){
 const cod1ActorLease_t *lease=&tree->lease;
 const cod1ActorRecord_t *a;
 const gentity_t *e;
 if(lease->actorIndex>=COD1_ACTOR_CAPACITY || !ActorLeaseValid(lease) || !tree->notify ||
    !ActorScriptedClockSelected(lease->actorIndex))return 0;
 a=cod1Actors+lease->actorIndex;e=g_entities+lease->entity.slot;
 return e->targetname && !strcmp(e->targetname,"moody") && e->classname &&
    !strcmp(e->classname,"actor_ally_airborne_officer_thompson") && e->health>0 &&
    a->scriptedController.entered && !a->scriptedController.invalid && a->scriptedController.actorSerial==lease->serial &&
    a->scriptedController.runtime==tree->runtime && a->scriptedController.catalog==tree->catalog &&
    !a->turretMount.mounted && a->scriptedAux.link==FRESH_LINK_UNATTACHED;
}
static int MoodyStopped(const C1GARecord *tree){
 const cod1ActorRecord_t *a;
 if(!MoodyIdleTree(tree))return 0;a=cod1Actors+tree->lease.actorIndex;
 return !a->turretMount.currentTop && a->turretMount.currentStack[0]==4 &&
    !a->critical.top && !a->critical.command_count && !cod1NormalRecords[tree->lease.actorIndex] &&
    a->scriptedAux.record==FRESH_AUX_EMPTY && a->scriptedAux.prefix==FRESH_PREFIX_UNUSED &&
    (!a->goal.assigned || (a->motion.arrived && !a->motion.moving && !a->motion.blocked &&
       a->motion.notified==a->goal.assignment && a->motion.assignment==a->goal.assignment));
}
static int MoodyNamedIdle(const C1GARecord *tree){
 return MoodyIdleTree(tree) && tree->notify->flags[889].token==890 && tree->notify->generation[889] &&
    !strcmp(ActorGenericName(tree,889),"idleanim");
}
/* The original scripted prefix queues row9 before constructing its body
 * record. The existing idle tree remains owned during that exact queue state.
 * This predicate permits observation/clock closure, never queue consumption. */
static int MoodyQueuedIdle(const C1GARecord *tree){
 const cod1ActorRecord_t *a;
 if(!MoodyNamedIdle(tree))return 0;a=cod1Actors+tree->lease.actorIndex;
 return ActorControllerFreshDomain(a)&&!a->scriptedController.selectionSerial&&
  !a->scriptedController.resumed4&&!a->turretMount.currentTop&&a->turretMount.currentStack[0]==4&&
  !cod1NormalRecords[tree->lease.actorIndex]&&a->critical.command_count==1&&
  a->critical.top==1&&a->critical.stack[0]==4&&a->critical.stack[1]==9&&
  a->critical.commands[0].operation==2&&a->critical.commands[0].state==9&&
  a->scriptedAux.actorSerial==tree->lease.serial&&a->scriptedAux.revision&&
  a->scriptedAux.revision<UINT64_MAX&&a->scriptedAux.admitted==1&&
  a->scriptedAux.record==FRESH_AUX_EMPTY&&a->scriptedAux.prefix==FRESH_PREFIX_QUEUE_COMMITTED;
}
int G_Cod1MoodyIdleNeeded(const GscEntityRef *ref,int *needed){
 cod1ActorLease_t lease;
 const C1GARecord *tree;
 if(!needed || !ActorGoalBuffers(ref,sizeof(*ref),needed,sizeof(*needed),NULL,0) || !ActorGoalLease(ref,&lease))return 0;
 tree=cod1GenericActors+lease.actorIndex;
 if(!MoodyIdleTree(tree) || !ActorRefEqual(ref,&tree->lease.entity))return 0;
 *needed=MoodyStopped(tree) ? (MoodyNamedIdle(tree)?2:1) : 0;
 return 1;
}
static int FoleyConstructingIdleOwner(const C1GARecord *tree){
 const C1NormalOperation *op=&cod1NormalOperation;
 const cod1ActorRecord_t *actor;
 const C1GARecord *live;
 unsigned index=tree->lease.actorIndex;
 unsigned clip,j;
 int staged;
 if(index>=COD1_ACTOR_CAPACITY||op->checking||!ActorNormalScope(op)||
    memcmp(&op->record->lease,&tree->lease,sizeof(tree->lease))||!ActorLeaseValid(&tree->lease))return 0;
 actor=cod1Actors+index;live=cod1GenericActors+index;clip=op->q.animation.ordinal;
 if(!actor->scriptedController.entered||actor->scriptedController.invalid||
    actor->scriptedController.resumed4||actor->scriptedController.currentSource!=4||
    !actor->scriptedController.selectionSerial||actor->turretMount.mounted||
    actor->turretMount.currentTop!=1||actor->turretMount.currentStack[0]!=4||
    actor->turretMount.currentStack[1]!=9||!C1ASB14_Body(ActorScriptedIndex(clip)))return 0;
 staged=op->frame!=NULL;
 if(staged){
  if(op->frame!=cod1FaceFrameOperation||tree!=&op->frame->stage||
     !op->record->present||op->record->revision!=1||op->record->value.clip!=clip||
     !tree->notify||tree->notify->flags[clip].token!=clip+1||
     strcmp(ActorGenericName(tree,clip),op->q.notify))return 0;
 }else if(tree!=live||op->record->present||op->record->revision)return 0;
 for(j=0;j<C1ASB14_COUNT;j++){
  unsigned n=c1asb14Additional[2+j].ordinal;
  if(C1ASB14_Body(j)&&tree->notify->flags[n].token&&(!staged||n!=clip) &&
     !(n==889 && !staged && MoodyNamedIdle(tree)))return 0;
 }
 return 1;
}
static int FoleyNormalIdleOwner(const C1GARecord*r){
 unsigned i=r->lease.actorIndex;
 const cod1ActorRecord_t*a=cod1Actors+i;
 const C1NormalRecord*normal=cod1NormalRecords[i];
 unsigned j;
 if(!normal)return FoleyConstructingIdleOwner(r);
 if(!normal->present||!normal->revision||normal->runtime!=r->runtime||
    normal->catalog!=r->catalog||normal->resources!=cod1ScriptedResources.revision||
    memcmp(&normal->lease,&r->lease,sizeof(r->lease))||!ActorLeaseValid(&r->lease)||
    a->scriptedAux.record!=FRESH_AUX_OWNED||!a->scriptedController.entered||
    a->scriptedController.invalid||a->scriptedController.resumed4||
    a->turretMount.currentTop!=1||a->turretMount.currentStack[0]!=4||
    a->turretMount.currentStack[1]!=9)return 0;
 if(normal->value.clip)return C1ASB14_Body(ActorScriptedIndex(normal->value.clip));
 for(j=0;j<C1ASB14_COUNT;j++)
  if(C1ASB14_Body(j)&&r->notify->flags[c1asb14Additional[2+j].ordinal].token)return 0;
 return 1;
}

/* The eye reset is outside the start constructor and runs only on a committed scripted body. */
static int ActorOwnedEyeResetBody(const C1GARecord *tree,unsigned *body){
 unsigned i=tree->lease.actorIndex,clip;
 const C1NormalRecord *record;
 if(i>=COD1_ACTOR_CAPACITY||!tree->notify||tree->tick!=level.time||
    !(record=cod1NormalRecords[i])||!FoleyNormalIdleOwner(tree))return 0;
 clip=record->value.clip;
 /* 431B0 cleared the clip and its body token on natural completion.
  * The same leased record still owns this frame until its next update.
  * FoleyNormalIdleOwner already excluded every named body in this case. */
 if(!clip){
  if(!record->value.firstUpdate||record->updated<record->created)return 0;
  *body=0;
  return 1;
 }
 /* 36E38 has removed the old name, but the original record remains
  * owned until the DC40 replacement runs. A disjoint Look subtree can
  * change during this interval, with all named clocks still fenced. */
 if(clip<C1GA_COUNT && !tree->notify->flags[clip].token && !tree->notify->pending[clip]){
  const cod1ActorRecord_t *a=cod1Actors+i;
  const gentity_t *e=g_entities+tree->lease.entity.slot;
  if(e->targetname && !strcmp(e->targetname,"foley") && e->classname &&
     !strcmp(e->classname,"actor_ally_airborne_officer_thompson") &&
     C1ASB14_Body(ActorScriptedIndex(clip)) && a->critical.command_count==1 &&
     a->critical.commands[0].operation==1 && a->critical.commands[0].state==9 &&
     a->scriptedAux.prefix==FRESH_PREFIX_QUEUE_COMMITTED && a->scriptedAux.link==FRESH_LINK_UNATTACHED){
   *body=0;return 1;
  }
 }
 if(clip>=C1GA_COUNT||tree->notify->flags[clip].token!=clip+1||
    !tree->notify->generation[clip]||strcmp(ActorGenericName(tree,clip),"single anim"))return 0;
 *body=clip;
 return 1;
}
