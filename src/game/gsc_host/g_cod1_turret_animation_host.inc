#include "../../qcommon/cod1_exact_bytes_r242.h"
#include "g_cod1_atr_write_watch.h"
/* Real selected MG42 resource/tree owner, included after the mount owner.
 * No event queue, native tree ID, guessed clock or completion notification. */
#include "../../qcommon/cod1_actor_turret_tree14.h"
typedef struct {
 int used,selected,gunActive,gunTick;GscEntityRef gun,actor;cod1ActorLease_t lease;
 uint64_t runtime,catalog,treeSerial;c1att14Tree_t actorTree[2];c1atl14Node_t gunNode[2];
} C1TARecord;
static int ActorGenericTurretCommit(const GscTurretAnimationRequest *,const C1TARecord *);
static C1TARecord cod1TurretAnimations[C1T_CAPACITY];
static GscAnimationCatalogEntry cod1TurretAnimationDeclarations[2][2];
static Cod1ATRWatch cod1TurretWatches[2];
static unsigned char *cod1TurretAnimationReference[2];
static unsigned char *cod1TurretAnimationATR[2];static size_t cod1TurretAnimationATRBytes[2];
static uint64_t cod1TurretAnimationHash[2],cod1TurretAnimationEpoch,cod1TurretAnimationReceipt,cod1TurretAnimationSerial;
static GscTurretAnimationHost cod1TurretAnimationFacade;
static struct {uint64_t epoch,serial;int busy,cancelled;void *bytes;size_t size;} cod1TurretAnimationOperation;
static uint64_t TurretAnimationHash(const void *bytes,size_t size){const unsigned char*p=bytes;uint64_t h=UINT64_C(1469598103934665603);size_t n;for(n=0;n<size;n++){h^=p[n];h*=UINT64_C(1099511628211);}return h;}
static int TurretAnimationPrivate(const void *p,size_t n) {
 int i;
 if(ActorOverlap(p,n,cod1TurretWatches,sizeof(cod1TurretWatches))||
    ActorOverlap(p,n,cod1TurretAnimations,sizeof(cod1TurretAnimations))||
    ActorOverlap(p,n,cod1TurretAnimationDeclarations,sizeof(cod1TurretAnimationDeclarations))||
    ActorOverlap(p,n,cod1TurretAnimationReference,sizeof(cod1TurretAnimationReference))||
    ActorOverlap(p,n,cod1TurretAnimationATR,sizeof(cod1TurretAnimationATR))||
    ActorOverlap(p,n,cod1TurretAnimationATRBytes,sizeof(cod1TurretAnimationATRBytes))||
    ActorOverlap(p,n,cod1TurretAnimationHash,sizeof(cod1TurretAnimationHash))||
    ActorOverlap(p,n,&cod1TurretAnimationEpoch,sizeof(cod1TurretAnimationEpoch))||
    ActorOverlap(p,n,&cod1TurretAnimationReceipt,sizeof(cod1TurretAnimationReceipt))||
    ActorOverlap(p,n,&cod1TurretAnimationSerial,sizeof(cod1TurretAnimationSerial))||
    ActorOverlap(p,n,&cod1TurretAnimationFacade,sizeof(cod1TurretAnimationFacade))||
    ActorOverlap(p,n,&cod1TurretAnimationOperation,sizeof(cod1TurretAnimationOperation))||
    ActorOverlap(p,n,cod1TurretAnimationOperation.bytes,cod1TurretAnimationOperation.size))return 1;
 for(i=0;i<2;i++)if(ActorOverlap(p,n,cod1TurretAnimationATR[i],cod1TurretAnimationATRBytes[i])||ActorOverlap(p,n,cod1TurretAnimationReference[i],cod1TurretAnimationATRBytes[i]))return 1;
 return 0;
}
static void TurretAnimationReset(void) {
 int i;if(cod1TurretAnimationOperation.busy)cod1TurretAnimationOperation.cancelled=1;
 for(i=0;i<2;i++){
  if(cod1TurretWatches[i].base) Cod1ATRWatchFree(&cod1TurretWatches[i]);
  else {free(cod1TurretAnimationReference[i]);free(cod1TurretAnimationATR[i]);}
  cod1TurretAnimationReference[i]=NULL;cod1TurretAnimationATR[i]=NULL;cod1TurretAnimationATRBytes[i]=0;}
 memset(cod1TurretAnimations,0,sizeof(cod1TurretAnimations));
 memset(cod1TurretAnimationDeclarations,0,sizeof(cod1TurretAnimationDeclarations));
 memset(cod1TurretAnimationHash,0,sizeof(cod1TurretAnimationHash));
 cod1TurretAnimationEpoch=cod1TurretAnimationReceipt=0;
}
static int TurretAnimationBanks(void) {
 unsigned i,j;
 if(!cod1TurretAnimationEpoch||cod1TurretAnimationEpoch!=cod1EntityEpoch||
    cod1TurretBankEpoch!=cod1EntityEpoch||cod1TurretResourceEpoch!=cod1EntityEpoch)return 0;
 for(i=0;i<2;i++){
  if(!cod1TurretAnimationATR[i]||!cod1TurretAnimationATRBytes[i]||
     !cod1TurretAnimationReference[i]||(!Cod1ATRWatchClean(&cod1TurretWatches[i],cod1TurretAnimationATR[i],cod1TurretAnimationReference[i],cod1TurretAnimationATRBytes[i]) &&
      !R242ExactATRBytesEqual(cod1TurretAnimationATR[i],cod1TurretAnimationReference[i],cod1TurretAnimationATRBytes[i]))||
     !cod1TurretGunRoots[i]||cod1TurretGunRoots[i]->magic!=C1AR14_MAGIC||cod1TurretGunRoots[i]->noteCount||
     !cod1TurretGunBodies[i]||cod1TurretGunBodies[i]->magic!=C1AB14_MAGIC)return 0;
  for(j=0;j<21;j++)if(!cod1TurretRoots[i][j]||cod1TurretRoots[i][j]->magic!=C1AR14_MAGIC||cod1TurretRoots[i][j]->noteCount||
      !cod1TurretBodies[i][j]||cod1TurretBodies[i][j]->magic!=C1AB14_MAGIC)return 0;
 }return 1;
}
static int TurretAnimationMapCurrent(uint64_t receipt) {
 return receipt&&receipt==cod1TurretAnimationReceipt&&cod1EntityReady&&ActorMapAvailable()&&
  !level.spawning&&!level.restarted&&TurretAnimationBanks();
}
static int TurretAnimationDeclaration(const GscAnimationCatalog *c,const char *name,GscAnimationCatalogEntry *out) {
 size_t n;GscAnimationCatalogEntry e;int found=0;
 for(n=0;n<gsc_animation_catalog_count(c);n++)if(gsc_animation_catalog_entry(c,n,&e)&&!Q_stricmp(e.name,name)){
  if(found)return 0;*out=e;found=1;
 }return found;
}
static int TurretAnimationLoad(void) {
 static const char *const trees[2]={"generic_human","mg42"};
 static const char *const groups[2]={"standMG42gunner_aim","standMG42gunner_fire"};
 static const char *const guns[2]={"standMG42gun_aim_foward","standMG42gun_fire_foward"};
 static const char *const yaw[7]={"45left","30left","15left","forward","15right","30right","45right"};
 static const char *const height[3]={"15down","level","15up"};
 static const char *const roles[2]={"aim","fire"};
 unsigned char *bytes[2]={NULL,NULL},*reference[2]={NULL,NULL};size_t sizes[2]={0,0};GscAnimationCatalog *catalog=NULL;
 GscAnimationCatalogEntry declarations[2][2],row,leaf;char path[128],why[256];fileHandle_t file=0;
 uint64_t epoch=cod1EntityEpoch;int t,i,j,k,length,ok=0;
 if(cod1TurretAnimationEpoch==epoch&&epoch)return TurretAnimationBanks();
 if(!epoch||!cod1EntityActive||!level.spawning||cod1TurretAnimationOperation.busy||cod1TurretAnimationSerial==UINT64_MAX)return 0;
 cod1TurretAnimationOperation.busy=1;cod1TurretAnimationOperation.cancelled=0;cod1TurretAnimationOperation.epoch=epoch;
 for(t=0;t<2;t++){
  snprintf(path,sizeof(path),"animtrees/%s.atr",trees[t]);length=trap_FS_FOpenFile(path,&file,FS_READ);
  if(length<=0||length>131072||!file)goto done;
  bytes[t]=malloc(length);if(!bytes[t])goto done;sizes[t]=(size_t)length;
  cod1TurretAnimationOperation.bytes=bytes[t];cod1TurretAnimationOperation.size=sizes[t];
  trap_FS_Read(bytes[t],length,file);trap_FS_FCloseFile(file);file=0;
  if(epoch!=cod1EntityEpoch||!level.spawning||!cod1EntityActive||cod1TurretAnimationOperation.cancelled)goto done;
  catalog=gsc_animation_catalog_parse(trees[t],bytes[t],sizes[t],why,sizeof(why));if(!catalog)goto done;
  for(i=0;i<2;i++){
   if(!TurretAnimationDeclaration(catalog,t?guns[i]:groups[i],&declarations[t][i]))goto done;
   if(t){if(declarations[t][i].flags)goto done;}
   else {
    if(declarations[t][i].flags!=(uint32_t)(GSC_ANIMATION_NODE_BRANCH|GSC_ANIMATION_NODE_COMPLETE|
        (i?GSC_ANIMATION_NODE_LOOPSYNC:GSC_ANIMATION_NODE_NONLOOPSYNC)))goto done;
    for(j=0;j<3;j++){
     snprintf(path,sizeof(path),"standMG42gunner_%s_%s",roles[i],height[j]);
     if(!TurretAnimationDeclaration(catalog,path,&row)||row.parent!=declarations[t][i].ordinal||row.flags!=GSC_ANIMATION_NODE_BRANCH)goto done;
     for(k=0;k<7;k++){
      snprintf(path,sizeof(path),"standMG42gunner_%s_%s_%s",roles[i],yaw[k],height[j]);
      if(!TurretAnimationDeclaration(catalog,path,&leaf)||leaf.parent!=row.ordinal||leaf.flags)goto done;
     }
    }
   }
  }
  gsc_animation_catalog_free(catalog);catalog=NULL;
 }
 if(epoch!=cod1EntityEpoch||!level.spawning||!cod1EntityActive||cod1TurretAnimationOperation.cancelled||cod1TurretAnimationEpoch)goto done;
 for(t=0;t<2;t++){reference[t]=malloc(sizes[t]);if(!reference[t])goto done;memcpy(reference[t],bytes[t],sizes[t]);}
 for(t=0;t<2;t++){cod1TurretAnimationReference[t]=reference[t];reference[t]=NULL;cod1TurretAnimationATR[t]=bytes[t];bytes[t]=NULL;cod1TurretAnimationATRBytes[t]=sizes[t];cod1TurretAnimationHash[t]=TurretAnimationHash(cod1TurretAnimationATR[t],sizes[t]);}
 /* No callback occurs between committing these allocations and publishing the receipt. */
 for(t=0;t<2;t++){
  if(Cod1ATRWatchCreate(&cod1TurretWatches[t],cod1TurretAnimationATR[t],sizes[t])){
   free(cod1TurretAnimationATR[t]);free(cod1TurretAnimationReference[t]);
   cod1TurretAnimationATR[t]=cod1TurretWatches[t].base;
   cod1TurretAnimationReference[t]=cod1TurretWatches[t].base+sizes[t];
  }
 }
 memcpy(cod1TurretAnimationDeclarations,declarations,sizeof(declarations));cod1TurretAnimationEpoch=epoch;
 cod1TurretAnimationReceipt=++cod1TurretAnimationSerial;ok=TurretAnimationBanks();
done:
 if(file)trap_FS_FCloseFile(file);gsc_animation_catalog_free(catalog);for(t=0;t<2;t++){free(reference[t]);free(bytes[t]);}
 cod1TurretAnimationOperation.bytes=NULL;cod1TurretAnimationOperation.size=0;cod1TurretAnimationOperation.busy=0;
 return ok;
}
int G_Cod1TurretAnimationCatalogs(const void *outBytes[2],size_t outSizes[2],GscMapExecutionCertificate *out) {
 GscMapExecutionCertificate certificate;int i;
 if(!outBytes||!outSizes||!out||TurretAnimationPrivate(outBytes,2*sizeof(*outBytes))||TurretAnimationPrivate(outSizes,2*sizeof(*outSizes))||
    TurretAnimationPrivate(out,sizeof(*out))||!ActorGoalBuffers(NULL,0,outBytes,2*sizeof(*outBytes),NULL,0)||
    !ActorGoalBuffers(NULL,0,outSizes,2*sizeof(*outSizes),NULL,0)||!ActorGoalBuffers(NULL,0,out,sizeof(*out),NULL,0)||
    ActorOverlap(outBytes,2*sizeof(*outBytes),outSizes,2*sizeof(*outSizes))||ActorOverlap(outBytes,2*sizeof(*outBytes),out,sizeof(*out))||
    ActorOverlap(outSizes,2*sizeof(*outSizes),out,sizeof(*out))||!TurretAnimationMapCurrent(cod1TurretAnimationReceipt))return 0;
 certificate.receipt=cod1TurretAnimationReceipt;certificate.current=TurretAnimationMapCurrent;
 for(i=0;i<2;i++){outBytes[i]=cod1TurretAnimationATR[i];outSizes[i]=cod1TurretAnimationATRBytes[i];}*out=certificate;return 1;
}
static int TurretAnimationRow(const GscEntityRef *receiver,C1TRecord **gun,int *actor) {
 int i;if(!receiver||!G_Cod1EntitySameAllocation(receiver))return -1;
 for(i=0;i<C1T_CAPACITY;i++)if(cod1Turrets[i].used){
  if(ActorRefEqual(receiver,&cod1Turrets[i].owner)){*gun=&cod1Turrets[i];*actor=0;return i;}
  if(ActorRefEqual(receiver,&cod1Turrets[i].actor)){*gun=&cod1Turrets[i];*actor=1;return i;}
 }return -1;
}
static int TurretAnimationLive(const C1TARecord *row,C1TRecord *r,int now) {
 GscEntityRef actor;gentity_t *gun;
 if(!r||!G_Cod1EntitySameAllocation(&r->owner))return 0;gun=&g_entities[r->owner.slot];
 return row&&row->used&&r&&row->runtime&&row->catalog&&TurretAnimationMapCurrent(cod1TurretAnimationReceipt)&&
  gun->classname&&!strcmp(gun->classname,"misc_mg42")&&gun->s.modelindex==r->modelIndex&&gun->s.eType==ET_GENERAL&&
  !gun->client&&!gun->r.bmodel&&!gun->physicsObject&&!gun->neverFree&&ActorOwnerAllowed(&r->owner,0)&&
  now==level.time&&ActorRefEqual(&row->gun,&r->owner)&&ActorRefEqual(&row->actor,&r->actor)&&
  ActorLeaseValid(&row->lease)&&ActorRefEqual(&row->lease.entity,&row->actor)&&
  TurretMountOwnerCurrent(r,now,&actor)&&ActorRefEqual(&actor,&row->actor);
}
static int TurretAnimationAdvanceInner(C1TARecord *row,int now) {
 int i;float dt,delta;
 if(now<row->gunTick||now-row->gunTick>60000)return 0;
 for(i=0;i<2;i++)if(now>row->actorTree[i].sampleTick&&!C1ATT14_Advance(&row->actorTree[i],now))return 0;
 if(row->gunActive&&now>row->gunTick){dt=(float)((double)(now-row->gunTick)*.001);
  for(i=0;i<2;i++){
   if(!C1ATL14_AdvanceWeight(&row->gunNode[i],dt))return 0;
   if((row->gunNode[i].currentWeight>0||row->gunNode[i].goalWeight>0)&&
      !C1ATL14_AdvanceLeaf(&row->gunNode[i],cod1TurretGunRoots[i]->intervals,cod1TurretGunRoots[i]->fps,
        cod1TurretGunRoots[i]->flags&1,cod1TurretGunRoots[i]->noteCount,dt,&delta))return 0;
  }
 }row->gunTick=now;return 1;
}
static int TurretAnimationAdvance(C1TARecord *row,int now) {
 C1TARecord next;int ok;if(!row)return 0;next=*row;
#if defined(_MSC_VER)&&defined(_M_IX86)
 {unsigned char outer[108];uint32_t mx,nmx=0x1f80;const unsigned short cw=0x027f;
 __asm fnsave outer
 __asm stmxcsr mx
 __asm fldcw cw
 __asm ldmxcsr nmx
 ok=TurretAnimationAdvanceInner(&next,now);
 __asm ldmxcsr mx
 __asm frstor outer
 }
#else
 ok=TurretAnimationAdvanceInner(&next,now);
#endif
 if(!ok)return 0;*row=next;return 1;
}
static int TurretAnimationGunRestartInner(c1atl14Node_t nodes[2],int which,float weight,float blend,float rate) {
 float eps,difference,maximum,fade;uint32_t u=0x3a83126fu;
 memcpy(&eps,&u,4);if(weight<eps)weight=0;
 difference=(float)((double)weight-nodes[which].currentWeight);if(difference<0)difference=-difference;maximum=difference;
 if(nodes[1-which].currentWeight>maximum)maximum=nodes[1-which].currentWeight;fade=(float)((double)maximum*blend);
 return C1ATL14_ClearGoal(&nodes[1-which],fade,nodes[1-which].currentWeight>0)&&
  C1ATL14_SetGoal(&nodes[which],weight,blend,rate,0)&&C1ATL14_RestartNodes(&nodes[which],1);
}
static int TurretAnimationGunRestart(c1atl14Node_t nodes[2],int which,float weight,float blend,float rate) {
 c1atl14Node_t next[2];int ok;memcpy(next,nodes,sizeof(next));
#if defined(_MSC_VER)&&defined(_M_IX86)
 {unsigned char outer[108];uint32_t mx,nmx=0x1f80;const unsigned short cw=0x027f;
 __asm fnsave outer
 __asm stmxcsr mx
 __asm fldcw cw
 __asm ldmxcsr nmx
 ok=TurretAnimationGunRestartInner(next,which,weight,blend,rate);
 __asm ldmxcsr mx
 __asm frstor outer
 }
#else
 ok=TurretAnimationGunRestartInner(next,which,weight,blend,rate);
#endif
 if(!ok)return 0;memcpy(nodes,next,sizeof(next));return 1;
}
static int TurretAnimationInitialize(C1TARecord *row,C1TRecord *r,int now,uint64_t runtime,uint64_t catalog) {
 unsigned notes[21],i,j;const c1ar14Root_t *runtimeBank[21];cod1ActorLease_t lease;GscEntityRef actor;
 if(!runtime||!catalog||!TurretMountOwnerCurrent(r,now,&actor)||!TurretMountLease(&actor,&lease))return 0;
 memset(row,0,sizeof(*row));row->used=1;row->selected=-1;row->gun=r->owner;row->actor=actor;row->lease=lease;
 row->runtime=runtime;row->catalog=catalog;row->gunTick=now;
 /* Actual ATR builder prepends each child (45E21B..45E24D); the native
  * selector traverses child.next. Runtime order reverses rows AND leaves,
  * independently of the public declared ordinal/source-bank order. */
 for(i=0;i<2;i++){for(j=0;j<21;j++){runtimeBank[j]=cod1TurretRoots[i][20-j];notes[j]=runtimeBank[j]->noteCount;}
  if(!C1ATT14_Init(&row->actorTree[i],runtimeBank,notes,now))return 0;row->gunNode[i].rate=1;
 }return 1;
}
static int TurretAnimationEnter(void) {
 if(cod1TurretAnimationOperation.busy){cod1TurretAnimationOperation.cancelled=1;return 0;}
 if(cod1TurretAnimationSerial==UINT64_MAX)return 0;
 cod1TurretAnimationOperation.busy=1;cod1TurretAnimationOperation.cancelled=0;
 cod1TurretAnimationOperation.epoch=cod1EntityEpoch;cod1TurretAnimationOperation.serial=++cod1TurretAnimationSerial;return 1;
}
static int TurretAnimationOpCurrent(uint64_t epoch,uint64_t operation,int now) {
 return cod1TurretAnimationOperation.busy&&!cod1TurretAnimationOperation.cancelled&&
  cod1TurretAnimationOperation.epoch==epoch&&cod1TurretAnimationOperation.serial==operation&&
  epoch==cod1EntityEpoch&&now==level.time&&TurretAnimationMapCurrent(cod1TurretAnimationReceipt);
}
/* A script_model has an independent tree even before its first clip starts. */
typedef struct {
 GscEntityRef owner;
 cod1StartOriginRef_t source;
 uint64_t runtime,catalog;
 int modelIndex,assignedTick;
 c1atl14Node_t nodes[2];
} TrainingRigTree;
static TrainingRigTree trainingRigTrees[2];
static const char *const trainingRigModels[2]={"xmodel/training_jumprig_gear","xmodel/training_jumprig_wire"};
static const char *const trainingRigNames[2]={"animation_rig_gear_trainingjump","animation_rig_wire_trainingjump"};
static const char *const trainingRigClips[2]={"training_gear_practicejump","training_wire_practicejump"};
#include "../../qcommon/cod1_actor_scripted_record14.h"
typedef struct {
 c1ar14Root_t *root;
 c1asr14Record_t record;
 uint64_t epoch;
 int running,startTick,lastTick,initialStep;
 char event[128];
} TrainingRigPlayback;
static TrainingRigPlayback trainingRigPlayback[2];
static void TrainingRigPlaybackReset(void){
 int i;for(i=0;i<2;i++)C1AR14_Free(trainingRigPlayback[i].root);
 memset(trainingRigPlayback,0,sizeof(trainingRigPlayback));
 memset(trainingRigTrees,0,sizeof(trainingRigTrees));
}
static int TrainingRigPlaybackLoad(void){
 int i;char path[MAX_QPATH];c1ar14Root_t *roots[2]={0};c1ab14Clip_t *body=NULL;
 if(!level.spawning||!cod1EntityEpoch)return 0;
 if(trainingRigPlayback[0].epoch==cod1EntityEpoch&&trainingRigPlayback[1].epoch==cod1EntityEpoch)return 1;
 for(i=0;i<2;i++){
  snprintf(path,sizeof(path),"xanim/%s",trainingRigClips[i]);
  if(!TurretBankRead(path,roots+i,&body,cod1EntityEpoch)||roots[i]->fps!=30||roots[i]->intervals!=175||
     roots[i]->bones!=(i?7:29)||roots[i]->flags!=(i?0:2))break;
  C1AB14_Free(body);body=NULL;
 }
 C1AB14_Free(body);
 if(i!=2){C1AR14_Free(roots[0]);C1AR14_Free(roots[1]);return 0;}
 TrainingRigPlaybackReset();
 for(i=0;i<2;i++){trainingRigPlayback[i].root=roots[i];trainingRigPlayback[i].epoch=cod1EntityEpoch;}
 return 1;
}
static int TrainingRigKind(const GscEntityRef *ref){
 const gentity_t *e;int i;
 if(!ref||!G_Cod1EntitySameAllocation(ref)||ref->slot>=MAX_GENTITIES)return -1;
 e=g_entities+ref->slot;
 if(!e->classname||strcmp(e->classname,"script_model")||!e->model||!e->s.modelindex||
    e->s.density!=COD1_SCRIPT_MODEL_NET_MARKER||e->s.eType!=ET_GENERAL||e->r.contents||e->r.bmodel)return -1;
 for(i=0;i<2;i++)if(!strcmp(e->model,trainingRigModels[i]))return i;
 return -1;
}
static int TrainingRigReceiverCurrent(const GscEntityRef *ref,uint64_t serial){
 int kind=TrainingRigKind(ref);
 return kind>=0 && serial==ref->generation && trainingRigTrees[kind].runtime &&
   ActorRefEqual(ref,&trainingRigTrees[kind].owner)&&trainingRigTrees[kind].source.epoch==ref->epoch;
}
static int TrainingRigAssociate(const GscTurretAnimationRequest *q,int kind,uint64_t op,
 int(*current)(void*),void *context,char *error,size_t cap){
 TrainingRigTree next={0};GscTurretAnimationRequest copy=*q;
 if(kind<0||kind>1||copy.method!=GSC_ANIMATION_USE_TREE||!copy.catalog_identity||!copy.animation.runtime_identity||
    copy.animation.ordinal||copy.animation.flags||copy.animation.name[0]||copy.animation.tree[0]||
    copy.weight_bits||copy.blend_bits||copy.rate_bits||copy.precision_bits!=53||copy.rounding||
    !memchr(copy.tree,0,sizeof(copy.tree))||strcmp(copy.tree,trainingRigNames[kind]))return 0;
 if(!G_Cod1StartOriginResourceRef(copy.tree,trainingRigClips[kind],&next.source,error,cap)||
    next.source.epoch!=copy.entity.epoch||next.source.flags||next.source.ordinal!=1||
    !current(context)||memcmp(&copy,q,sizeof(copy))||TrainingRigKind(&copy.entity)!=kind||
    !TurretAnimationOpCurrent(copy.entity.epoch,op,copy.now_ms))return 0;
 if(trainingRigTrees[kind].runtime&&G_Cod1EntitySameAllocation(&trainingRigTrees[kind].owner)&&
    !ActorRefEqual(&copy.entity,&trainingRigTrees[kind].owner))return 0;
 next.owner=copy.entity;next.runtime=copy.animation.runtime_identity;next.catalog=copy.catalog_identity;
 next.modelIndex=g_entities[copy.entity.slot].s.modelindex;next.assignedTick=copy.now_ms;
 next.nodes[0].rate=next.nodes[1].rate=1;
 trainingRigTrees[kind]=next;
 G_Printf("[Training rig tree] owner=%u/%u tree=%s model=%d clip=%s ordinal=%u\n",
   next.owner.slot,next.owner.generation,copy.tree,next.modelIndex,next.source.name,next.source.ordinal);
 return 1;
}
static int TurretAnimationBuffers(const void *input,size_t n,char *error,size_t cap) {
 return !TurretAnimationPrivate(input,n)&&!TurretAnimationPrivate(error,cap)&&ActorGoalBuffers(input,n,NULL,0,error,cap);
}
static int TurretAnimationValidate(void *user,const GscEntityRef *input,uint64_t runtime,int32_t now,
 char *error,size_t cap,int(*current)(void*),void *context) {
 GscEntityRef copy,actor;C1TRecord before,*r;cod1ActorLease_t lease;gentity_t gunBody,actorBody;
 cod1ActorTurretMount_t mount;uint64_t epoch,op;int index,isActor,ok=0;(void)user;
 if(!input||!runtime||!current||!TurretAnimationBuffers(input,sizeof(*input),error,cap)||!TurretAnimationEnter())return 0;
 copy=*input;epoch=cod1EntityEpoch;op=cod1TurretAnimationOperation.serial;
 {int kind=TrainingRigKind(&copy);
  if(kind>=0){
   ok=current(context)==1&&!memcmp(input,&copy,sizeof(copy))&&TrainingRigKind(&copy)==kind&&
      TurretAnimationOpCurrent(epoch,op,now);
   goto done;
  }}
 index=TurretAnimationRow(&copy,&r,&isActor);if(index<0||!TurretMountOwnerCurrent(r,now,&actor)||!TurretMountLease(&actor,&lease)||!TurretAnimationMapCurrent(cod1TurretAnimationReceipt))goto done;before=*r;
 gunBody=g_entities[r->owner.slot];actorBody=g_entities[actor.slot];mount=cod1Actors[lease.actorIndex].turretMount;
 if(current(context)!=1||!TurretAnimationOpCurrent(epoch,op,now)||!ActorLeaseValid(&lease)||
    !TurretAnimationBuffers(input,sizeof(*input),error,cap)||memcmp(input,&copy,sizeof(copy))||
    !(r=TurretFind(&before.owner))||memcmp(r,&before,sizeof(before))||!TurretMountOwnerCurrent(r,now,&actor)||
    memcmp(&g_entities[r->owner.slot],&gunBody,sizeof(gunBody))||memcmp(&g_entities[actor.slot],&actorBody,sizeof(actorBody))||
    memcmp(&cod1Actors[lease.actorIndex].turretMount,&mount,sizeof(mount)))goto done;
 ok=1;
done:cod1TurretAnimationOperation.busy=0;return ok;
}
static int TurretAnimationInvoke(void *user,const GscTurretAnimationRequest *input,char *error,size_t cap,int(*current)(void*),void *context) {
 GscTurretAnimationRequest q;C1TRecord before,*r;C1TARecord saved,next;GscAnimationCatalogEntry declaration;
 gentity_t gunBody,actorBody;cod1ActorTurretMount_t mount;
 uint64_t epoch,op;float weight,blend,rate;int index,isActor,which=-1,i,ok=0;(void)user;
 if(!input||!current||!TurretAnimationBuffers(input,sizeof(*input),error,cap)||!TurretAnimationEnter())return 0;
 q=*input;epoch=cod1EntityEpoch;op=cod1TurretAnimationOperation.serial;
 {int kind=TrainingRigKind(&q.entity);if(kind>=0){ok=TrainingRigAssociate(input,kind,op,current,context,error,cap);goto done;}}
 index=TurretAnimationRow(&q.entity,&r,&isActor);if(index<0||!q.catalog_identity||!q.animation.runtime_identity||q.now_ms!=level.time||q.precision_bits!=53||q.rounding!=0||!TurretAnimationMapCurrent(cod1TurretAnimationReceipt))goto done;
 before=*r;saved=next=cod1TurretAnimations[index];
 if(!next.used||!ActorRefEqual(&next.gun,&r->owner)||!ActorRefEqual(&next.actor,&r->actor)||!ActorLeaseValid(&next.lease)){
  if(!TurretAnimationInitialize(&next,r,q.now_ms,q.animation.runtime_identity,q.catalog_identity))goto done;
 }
 if(next.runtime!=q.animation.runtime_identity||next.catalog!=q.catalog_identity||!TurretAnimationLive(&next,r,q.now_ms))goto done;
 gunBody=g_entities[r->owner.slot];actorBody=g_entities[next.actor.slot];mount=cod1Actors[next.lease.actorIndex].turretMount;
 if(current(context)!=1||!TurretAnimationOpCurrent(epoch,op,q.now_ms)||memcmp(input,&q,sizeof(q))||
    memcmp(&cod1TurretAnimations[index],&saved,sizeof(saved))||!(r=TurretFind(&before.owner))||memcmp(r,&before,sizeof(before))||
    !TurretAnimationLive(&next,r,q.now_ms)||memcmp(&g_entities[r->owner.slot],&gunBody,sizeof(gunBody))||
    memcmp(&g_entities[next.actor.slot],&actorBody,sizeof(actorBody))||memcmp(&cod1Actors[next.lease.actorIndex].turretMount,&mount,sizeof(mount)))goto done;
 if(!TurretAnimationAdvance(&next,q.now_ms))goto done;
 if(!memchr(q.tree,0,sizeof(q.tree))||!memchr(q.animation.tree,0,sizeof(q.animation.tree))||!memchr(q.animation.name,0,sizeof(q.animation.name)))goto done;
 if(q.method==GSC_ANIMATION_USE_TREE){
  if(isActor||strcmp(q.tree,"mg42")||q.animation.tree[0]||q.animation.name[0]||q.animation.ordinal||q.animation.flags||q.weight_bits||q.blend_bits||q.rate_bits)goto done;
  memset(next.gunNode,0,sizeof(next.gunNode));for(i=0;i<2;i++)next.gunNode[i].rate=1;next.gunActive=1;next.treeSerial=op;
 }else if(q.method==GSC_ANIMATION_STOP_USE_TREE){
  if(isActor||!next.gunActive||q.tree[0]||q.animation.tree[0]||q.animation.name[0]||q.animation.ordinal||q.animation.flags||q.weight_bits||q.blend_bits||q.rate_bits)goto done;
  memset(next.gunNode,0,sizeof(next.gunNode));next.gunActive=0;next.treeSerial=0;
 }else {
  if(strcmp(q.tree,isActor?"generic_human":"mg42")||strcmp(q.tree,q.animation.tree))goto done;
  for(i=0;i<2;i++){declaration=cod1TurretAnimationDeclarations[isActor?0:1][i];
   if(q.animation.ordinal==declaration.ordinal&&q.animation.flags==declaration.flags&&!strcmp(q.animation.name,declaration.name))which=i;}
  if(which<0)goto done;
  if(q.method==GSC_ANIMATION_SET_TURRET){
   if(!isActor||q.weight_bits||q.blend_bits||q.rate_bits)goto done;next.selected=which;
  }else if(q.method==GSC_ANIMATION_SET_KNOB_RESTART){
   memcpy(&weight,&q.weight_bits,4);memcpy(&blend,&q.blend_bits,4);memcpy(&rate,&q.rate_bits,4);
   if(!C1ATL14_Finite(weight)||weight<0||weight>1||!C1ATL14_Finite(blend)||blend<0||blend>600||!C1ATL14_Finite(rate)||rate<0||rate>100)goto done;
   if(isActor){if(!C1ATT14_KnobRestart(&next.actorTree[which],&next.actorTree[1-which],weight,blend,rate))goto done;}
   else {
    if(!next.gunActive)goto done;
    if(!TurretAnimationGunRestart(next.gunNode,which,weight,blend,rate))goto done;
   }
  }else goto done;
 }
 if(current(context)!=1||!TurretAnimationOpCurrent(epoch,op,q.now_ms)||
    !TurretAnimationBuffers(input,sizeof(*input),error,cap)||memcmp(input,&q,sizeof(q))||
    memcmp(&cod1TurretAnimations[index],&saved,sizeof(saved))||!(r=TurretFind(&before.owner))||
    memcmp(r,&before,sizeof(before))||!TurretAnimationLive(&next,r,q.now_ms)||
    memcmp(&g_entities[r->owner.slot],&gunBody,sizeof(gunBody))||memcmp(&g_entities[next.actor.slot],&actorBody,sizeof(actorBody))||
    memcmp(&cod1Actors[next.lease.actorIndex].turretMount,&mount,sizeof(mount)))goto done;
 if(!ActorGenericTurretCommit(&q,&next))goto done;
 cod1TurretAnimations[index]=next;ok=1;
done:cod1TurretAnimationOperation.busy=0;return ok;
}
const GscTurretAnimationHost *G_Cod1TurretAnimationHost(void) {
 cod1TurretAnimationFacade.user=NULL;cod1TurretAnimationFacade.validate=TurretAnimationValidate;cod1TurretAnimationFacade.invoke=TurretAnimationInvoke;
 return &cod1TurretAnimationFacade;
}
static void TurretAnimationRetire(const GscEntityRef *actor) {
 int i;if(!actor)return;
 if(cod1TurretAnimationOperation.busy)cod1TurretAnimationOperation.cancelled=1;
 for(i=0;i<C1T_CAPACITY;i++)if(cod1TurretAnimations[i].used&&ActorRefEqual(actor,&cod1TurretAnimations[i].actor))
  memset(&cod1TurretAnimations[i],0,sizeof(cod1TurretAnimations[i]));
}
static int TurretAnimationGunSamples(const C1TRecord *input,int now,float phases[2],float weights[2]) {
 C1TRecord *r;C1TARecord value;int i,index;
 if(!input||!phases||!weights||cod1TurretAnimationOperation.busy)return -1;
 r=TurretFind(&input->owner);if(r!=input)return -1;index=(int)(r-cod1Turrets);value=cod1TurretAnimations[index];
 if(!value.used||!value.gunActive)return 0;
 if(!TurretAnimationLive(&value,r,now)||!TurretAnimationAdvance(&value,now))return -1;
 cod1TurretAnimations[index]=value;for(i=0;i<2;i++){phases[i]=value.gunNode[i].phase;weights[i]=value.gunNode[i].currentWeight;}return 1;
}
static int TurretAnimationActorRead(const C1TRecord *input,int now,c1att14Tree_t out[2],int *selected) {
 C1TRecord *r;C1TARecord value;int index;
 if(!input||!out||!selected||cod1TurretAnimationOperation.busy)return -1;
 r=TurretFind(&input->owner);if(r!=input)return -1;index=(int)(r-cod1Turrets);value=cod1TurretAnimations[index];
 if(!value.used||value.selected<0)return 0;
 if(!TurretAnimationLive(&value,r,now)||!TurretAnimationAdvance(&value,now))return -1;
 cod1TurretAnimations[index]=value;memcpy(out,value.actorTree,sizeof(value.actorTree));*selected=value.selected;return 1;
}
static int TurretAnimationActorCommit(const C1TRecord *input,int now,const c1att14Tree_t before[2],const c1att14Tree_t after[2]) {
 C1TRecord *r;C1TARecord *row;int i,index;
 if(!input||!before||!after||cod1TurretAnimationOperation.busy)return 0;
 r=TurretFind(&input->owner);if(r!=input)return 0;index=(int)(r-cod1Turrets);row=&cod1TurretAnimations[index];
 if(!TurretAnimationLive(row,r,now)||memcmp(row->actorTree,before,sizeof(row->actorTree)))return 0;
 for(i=0;i<2;i++)if(after[i].magic!=C1ATT14_MAGIC||after[i].sampleTick!=now||memcmp(after[i].bank,before[i].bank,sizeof(after[i].bank))||
    memcmp(after[i].frequency,before[i].frequency,sizeof(after[i].frequency))||after[i].loop!=before[i].loop)return 0;
 memcpy(row->actorTree,after,sizeof(row->actorTree));return 1;
}
