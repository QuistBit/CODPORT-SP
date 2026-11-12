/* R232 full original generic_human hierarchy. Included after turret animation.
 * This owner retains source ordinals, never packs them as native handles.
 * Resource clock/publication admission is separate from hierarchy creation. */
#include "../g_cod1_actor_immediate.h"
#include "../../qcommon/cod1_actor_generic_tree14.h"
#include "../../qcommon/cod1_actor_ancestor_knob14.h"
#include "../../qcommon/cod1_actor_flagged_leaf14.h"
#include "../../qcommon/cod1_actor_flagged_clock14.h"
#include "../bg_cod1_actor_look14.h"
#include "../../qcommon/cod1_actor_look_bank14.h"
#include "../../qcommon/cod1_actor_look_tree14_defs.h"
#include "../../qcommon/cod1_actor_root_interval14.h"
#include "../../qcommon/cod1_actor_unmounted_tree14.h"
#define C1GA_COUNT 2078u
typedef struct {c1afl14State_t flags[C1GA_COUNT];char names[C1GA_COUNT][64];uint64_t generation[C1GA_COUNT],pending[C1GA_COUNT],serial,ownerIdentity;int32_t pendingTick[C1GA_COUNT];} C1GANotifyState;
typedef struct {
 int used,tick;cod1ActorLease_t lease;uint64_t runtime,catalog,revision;
 c1atl14Node_t *nodes;C1GANotifyState *notify;uint32_t lookCells[16];
} C1GARecord;
static int ActorFaceMutate(C1GARecord*,void*);
static int ActorScriptedMutate(C1GARecord*,void*);
static int ActorScriptedClockSelected(unsigned);
static int ActorScriptedIndex(unsigned);
static const c1ar14Root_t*ActorScriptedRoot(unsigned);
static int ActorFaceLeafIndex(unsigned);
static const c1ar14Root_t*ActorFaceRoot(unsigned);
static const c1ab14Clip_t*ActorFaceBody(unsigned);
static int ActorFaceFrameCanMutate(const C1GARecord*,int,int);
static int ActorRunnerFaceOwner(const C1GARecord*);
/* Main-thread GAME transactions clone the same two fixed-size arrays.
 * Keep two released pairs. Every checkout overwrites both arrays in full. */
typedef struct {
 c1atl14Node_t *nodes;
 C1GANotifyState *notify;
} ActorCloneStorage;
static ActorCloneStorage actorClonePool[2];
static void ActorClonePoolClear(void){
 unsigned i;
 for(i=0;i<2;i++){free(actorClonePool[i].nodes);free(actorClonePool[i].notify);}
 memset(actorClonePool,0,sizeof(actorClonePool));
}
static void ActorGenericFreeOwned(C1GARecord*r){
 unsigned i;
 if(r->nodes && r->notify){
  for(i=0;i<2;i++)if(!actorClonePool[i].nodes){
   actorClonePool[i].nodes=r->nodes;actorClonePool[i].notify=r->notify;
   r->nodes=NULL;r->notify=NULL;return;
  }
 }
 free(r->nodes);free(r->notify);r->nodes=NULL;r->notify=NULL;
}
static int ActorGenericClone(C1GARecord*stage,const C1GARecord*old){
 unsigned i;
 *stage=*old;stage->nodes=NULL;stage->notify=NULL;
 if(!old->nodes||!old->notify)return 0;
 for(i=0;i<2;i++)if(actorClonePool[i].nodes){
  stage->nodes=actorClonePool[i].nodes;stage->notify=actorClonePool[i].notify;
  actorClonePool[i].nodes=NULL;actorClonePool[i].notify=NULL;break;
 }
 if(!stage->nodes){
  stage->nodes=malloc(C1GA_COUNT*sizeof(*stage->nodes));stage->notify=malloc(sizeof(*stage->notify));
 }
 if(!stage->nodes||!stage->notify){ActorGenericFreeOwned(stage);return 0;}
 memcpy(stage->nodes,old->nodes,C1GA_COUNT*sizeof(*stage->nodes));memcpy(stage->notify,old->notify,sizeof(*stage->notify));return 1;
}
static int ActorGenericNamedActive(const C1GARecord*);
static int ActorGenericNamedPending(const C1GARecord*);
static int ActorGenericNamedAdvance(C1GARecord*,unsigned,const c1ar14Root_t*,float,int);
static C1GARecord cod1GenericActors[COD1_ACTOR_CAPACITY];
static c1agt14Definition_t cod1GenericDefinitions[C1GA_COUNT];
static GscAnimationCatalogEntry cod1GenericNames[C1GA_COUNT];
/* Names and parent links are immutable between catalog publication and reset. */
static struct {unsigned entries[2][25]; int ready;} cod1GenericMGMap;
static uint64_t cod1GenericEpoch,cod1GenericReceipt,cod1GenericNotifyIdentity;
/* One map-owned bank: the Look member remains the existing immutable view. */
static c1aut14Bank_t cod1GenericUnmountedBank;
#define cod1GenericLookBank (cod1GenericUnmountedBank.look)
static uint64_t cod1GenericLookEpoch,cod1GenericLookBusy,cod1GenericLookRevision;
static struct {int busy,cancelled;void *stage;size_t bytes;void *notifyStage;size_t notifyBytes;} cod1GenericOperation;
/* Same selected time conversion as C1ATT14_AdvanceInner, with the complete
 * caller x87/SSE environment retained even for hostile caller rounding. */
static int ActorGenericDt(int milliseconds,float *out){
 float value;if(!out||milliseconds<0||milliseconds>60000)return 0;
#if defined(_MSC_VER)&&defined(_M_IX86)
 unsigned char saved[108];uint32_t mx,nearest=0x1f80;const unsigned short cw=0x027f;const double scale=.001;
 __asm fnsave saved
 __asm stmxcsr mx
 __asm fldcw cw
 __asm ldmxcsr nearest
 __asm fild milliseconds
 __asm fmul scale
 __asm fstp value
 __asm ldmxcsr mx
 __asm frstor saved
#else
 value=(float)((double)milliseconds*.001);
#endif
 *out=value;return 1;
}
static TurretClipExtent cod1GenericClipExtent;
static int ActorGenericClipRanges(const void *p,size_t n,TurretClipExtent *extent){
 unsigned i;
 for(i=0;i<C1AL14_COUNT;i++){
  if(TurretBodyRanges(p,n,cod1GenericLookBank.body[i],extent)||
     TurretClipRange(p,n,cod1GenericLookBank.root[i],sizeof(c1ar14Root_t),extent))return 1;
 }
 for(i=0;i<2;i++){
  if(TurretBodyRanges(p,n,cod1GenericUnmountedBank.body[i],extent)||
     TurretClipRange(p,n,cod1GenericUnmountedBank.root[i],sizeof(c1ar14Root_t),extent))return 1;
 }
 return 0;
}
static int ActorGenericPrivate(const void *p,size_t n) {
 unsigned i;if(ActorFacePrivate(p,n)||ActorOverlap(p,n,actorClonePool,sizeof(actorClonePool)))return 1;
 for(i=0;i<2;i++)if(actorClonePool[i].nodes &&
   (ActorOverlap(p,n,actorClonePool[i].nodes,C1GA_COUNT*sizeof(c1atl14Node_t))||
    ActorOverlap(p,n,actorClonePool[i].notify,sizeof(C1GANotifyState))))return 1;
 if(ActorOverlap(p,n,&cod1GenericMGMap,sizeof(cod1GenericMGMap))||
    ActorOverlap(p,n,&cod1GenericClipExtent,sizeof(cod1GenericClipExtent))||
    ActorOverlap(p,n,cod1GenericActors,sizeof(cod1GenericActors))||
    ActorOverlap(p,n,&cod1GenericUnmountedBank,sizeof(cod1GenericUnmountedBank))||
    ActorOverlap(p,n,&cod1GenericLookEpoch,sizeof(cod1GenericLookEpoch))||ActorOverlap(p,n,&cod1GenericLookBusy,sizeof(cod1GenericLookBusy))||ActorOverlap(p,n,&cod1GenericLookRevision,sizeof(cod1GenericLookRevision))||
    ActorOverlap(p,n,cod1GenericDefinitions,sizeof(cod1GenericDefinitions))||
    ActorOverlap(p,n,cod1GenericNames,sizeof(cod1GenericNames))||
    ActorOverlap(p,n,&cod1GenericEpoch,sizeof(cod1GenericEpoch))||
    ActorOverlap(p,n,&cod1GenericReceipt,sizeof(cod1GenericReceipt))||ActorOverlap(p,n,&cod1GenericNotifyIdentity,sizeof(cod1GenericNotifyIdentity))||
    ActorOverlap(p,n,&cod1GenericOperation,sizeof(cod1GenericOperation))||
    ActorOverlap(p,n,cod1GenericOperation.stage,cod1GenericOperation.bytes)||ActorOverlap(p,n,cod1GenericOperation.notifyStage,cod1GenericOperation.notifyBytes))return 1;
 for(i=0;i<COD1_ACTOR_CAPACITY;i++)if(ActorOverlap(p,n,cod1GenericActors[i].nodes,
     cod1GenericActors[i].nodes?C1GA_COUNT*sizeof(c1atl14Node_t):0)||ActorOverlap(p,n,cod1GenericActors[i].notify,cod1GenericActors[i].notify?sizeof(C1GANotifyState):0))return 1;
 if(cod1GenericLookEpoch && p && n){
  const uintptr_t address=(uintptr_t)p;
  if(address>cod1GenericClipExtent.hi ||
     (address<cod1GenericClipExtent.lo && n<=cod1GenericClipExtent.lo-address))return 0;
 }
 return ActorGenericClipRanges(p,n,NULL);
}
static void ActorGenericRetire(uint32_t index,const GscEntityRef*owner){
 C1GARecord*r;if(!owner||index>=COD1_ACTOR_CAPACITY)return;r=cod1GenericActors+index;
 if(!r->used||r->lease.actorIndex!=index||!ActorRefEqual(&r->lease.entity,owner))return;
 ActorFaceFrameRetire(index,owner);
 if(cod1GenericOperation.busy)cod1GenericOperation.cancelled=1;
 ActorGenericFreeOwned(r);memset(r,0,sizeof(*r));
}
static void ActorGenericReset(void) {
 unsigned i;ActorFaceReset();if(cod1GenericOperation.busy)cod1GenericOperation.cancelled=1;
 for(i=0;i<COD1_ACTOR_CAPACITY;i++){ActorGenericFreeOwned(cod1GenericActors+i);memset(&cod1GenericActors[i],0,sizeof(C1GARecord));}
 ActorClonePoolClear();
 memset(&cod1GenericMGMap,0,sizeof(cod1GenericMGMap));
 memset(cod1GenericDefinitions,0,sizeof(cod1GenericDefinitions));
 memset(cod1GenericNames,0,sizeof(cod1GenericNames));cod1GenericEpoch=cod1GenericReceipt=0;
 C1AUT14_Free(&cod1GenericUnmountedBank);cod1GenericLookEpoch=0;
 if(cod1GenericLookRevision!=UINT64_MAX)cod1GenericLookRevision++;
}
static int ActorGenericLookLoad(void) {
 c1aut14Bank_t stage;uint64_t epoch=cod1EntityEpoch,revision=cod1GenericLookRevision;unsigned i;char path[128];int ok=0;
 if(epoch&&cod1GenericLookEpoch==epoch)return C1AUT14_Ready(&cod1GenericUnmountedBank);
 if(!epoch||!level.spawning||!cod1EntityActive||cod1GenericLookBusy||revision==UINT64_MAX)return 0;
 memset(&stage,0,sizeof(stage));cod1GenericLookBusy=epoch;
 for(i=0;i<C1AL14_COUNT;i++){
  snprintf(path,sizeof(path),"xanim/%s",c1al14Names[i]);
  if(!TurretBankRead(path,&stage.look.root[i],&stage.look.body[i],epoch)||revision!=cod1GenericLookRevision||!C1AL14_Valid(i,stage.look.root[i],stage.look.body[i]))goto done;
 }
 for(i=0;i<2;i++){
  snprintf(path,sizeof(path),"xanim/%s",c1aut14Names[i]);
  if(!TurretBankRead(path,&stage.root[i],&stage.body[i],epoch)||revision!=cod1GenericLookRevision||!C1AUT14_ValidShuffle(stage.root[i],stage.body[i]))goto done;
 }
 if(epoch!=cod1EntityEpoch||revision!=cod1GenericLookRevision||!level.spawning||!cod1EntityActive||cod1GenericLookBusy!=epoch||cod1GenericLookEpoch||!C1AUT14_Ready(&stage))goto done;
 cod1GenericUnmountedBank=stage;
 cod1GenericClipExtent.lo=UINTPTR_MAX;cod1GenericClipExtent.hi=0;
 ActorGenericClipRanges(NULL,0,&cod1GenericClipExtent);
 memset(&stage,0,sizeof(stage));cod1GenericLookEpoch=epoch;ok=1;
done:C1AUT14_Free(&stage);if(cod1GenericLookBusy==epoch)cod1GenericLookBusy=0;return ok;
}
/* Compile source flags as ENGINE47E030: explicit sync roots acquire bit4;
 * descendants inherit their mode, while COMPLETE remains bit8. Nested explicit
 * sync roots are rejected. Clip loop-mode validation belongs to resource load. */
static int ActorGenericBuild(void) {
 GscAnimationCatalog *catalog=NULL;GscAnimationCatalogEntry e;
 c1agt14Definition_t *d=NULL;c1atl14Node_t *nodes=NULL;
 GscAnimationCatalogEntry *names=NULL;size_t i;unsigned p,depth,mode;int ok=0;
 char why[256];uint64_t epoch=cod1EntityEpoch,receipt=cod1TurretAnimationReceipt;
 if(cod1GenericEpoch==epoch&&epoch)return cod1GenericReceipt==receipt&&TurretAnimationBanks();
 if(!epoch||!receipt||cod1GenericOperation.busy||!TurretAnimationBanks())return 0;
 cod1GenericOperation.busy=1;cod1GenericOperation.cancelled=0;
 d=calloc(C1GA_COUNT,sizeof(*d));names=calloc(C1GA_COUNT,sizeof(*names));nodes=calloc(C1GA_COUNT,sizeof(*nodes));
 if(!d||!names||!nodes)goto done;
 cod1GenericOperation.stage=d;cod1GenericOperation.bytes=C1GA_COUNT*sizeof(*d);
 catalog=gsc_animation_catalog_parse("generic_human",cod1TurretAnimationATR[0],cod1TurretAnimationATRBytes[0],why,sizeof(why));
 if(!catalog||gsc_animation_catalog_count(catalog)!=C1GA_COUNT-1)goto done;
 strcpy(names[0].name,"<implicit-root>");
 for(i=0;i<C1GA_COUNT-1;i++){
  if(!gsc_animation_catalog_entry(catalog,i,&e)||!e.ordinal||e.ordinal>=C1GA_COUNT||e.parent>=C1GA_COUNT||names[e.ordinal].ordinal||e.flags>15)goto done;
  names[e.ordinal]=e;d[e.ordinal].parent=e.parent;
  d[e.ordinal].flags=((e.flags&2)?8:0)|((e.flags&4)?1:0)|((e.flags&8)?2:0);
  if((d[e.ordinal].flags&3)==3||(!(e.flags&1)&&e.flags))goto done;
  d[e.parent].children++;
 }
 for(i=1;i<C1GA_COUNT;i++){
  if(!names[i].ordinal||((names[i].flags&1)!=0)!=(d[i].children!=0))goto done;
  mode=0;
  for(p=(unsigned)i,depth=0;p;p=d[p].parent){
   unsigned explicitMode;
   if(p>=C1GA_COUNT||++depth>64)goto done;
   explicitMode=(names[p].flags&4)?1:((names[p].flags&8)?2:0);
   if(explicitMode){if(mode)goto done;mode=explicitMode;}
  }
  if(d[i].flags&3)d[i].flags|=4;else if(d[i].children)d[i].flags|=mode;
 }
 for(i=0;i<C1GA_COUNT;i++)nodes[i].rate=1;
 if(!C1AGT14_Valid(d,nodes,C1GA_COUNT)||epoch!=cod1EntityEpoch||receipt!=cod1TurretAnimationReceipt||cod1GenericOperation.cancelled)goto done;
 memcpy(cod1GenericDefinitions,d,C1GA_COUNT*sizeof(*d));memcpy(cod1GenericNames,names,C1GA_COUNT*sizeof(*names));
 cod1GenericEpoch=epoch;cod1GenericReceipt=receipt;ok=1;
done:gsc_animation_catalog_free(catalog);free(d);free(names);free(nodes);cod1GenericOperation.stage=NULL;cod1GenericOperation.bytes=0;cod1GenericOperation.notifyStage=NULL;cod1GenericOperation.notifyBytes=0;cod1GenericOperation.busy=0;return ok;
}
static int ActorGenericFresh(C1GARecord *r,const cod1ActorLease_t *lease,uint64_t runtime,uint64_t catalog,int now) {
 cod1ActorLookAtDefaults_t *s=&cod1Actors[lease->actorIndex].nativeData.lookat;
 c1al14State_t look;uint32_t known[10];unsigned i;
 memset(r,0,sizeof(*r));memset(known,0,sizeof(known));
 known[0]=s->active;memcpy(known+1,s->target,12);known[4]=s->cell01ec;known[5]=s->cell01f0;
 memcpy(known+6,&s->parameter,4);known[7]=s->animnodesReady;memcpy(known+8,s->yawLimits,8);
 if(!C1AL14_Fresh(known,&look)||cod1GenericNotifyIdentity==UINT64_MAX)return 0;
 r->nodes=calloc(C1GA_COUNT,sizeof(*r->nodes));r->notify=calloc(1,sizeof(*r->notify));if(!r->nodes||!r->notify){ActorGenericFreeOwned(r);return 0;}
 for(i=0;i<C1GA_COUNT;i++)r->notify->flags[i].cursor=-1;
 r->notify->ownerIdentity=++cod1GenericNotifyIdentity;
 for(i=0;i<C1GA_COUNT;i++)r->nodes[i].rate=1;
 r->used=1;r->tick=now;r->lease=*lease;r->runtime=runtime;r->catalog=catalog;r->revision=1;
 memcpy(r->lookCells,&look,64);return 1;
}
static unsigned ActorGenericOrdinal(const char *name) {
 unsigned i,found=0;for(i=1;i<C1GA_COUNT;i++)if(!Q_stricmp(name,cod1GenericNames[i].name)){if(found)return 0;found=i;}return found;
}
static int ActorGenericShuffleIndex(const char *name){unsigned i;if(!name)return -1;for(i=0;i<2;i++)if(!strcmp(name,c1aut14Names[i]))return (int)i;return -1;}
static int ActorGenericMGMap(unsigned map[2][25]) {
 static const char *const role[2]={"aim","fire"};
 static const char *const height[3]={"15up","level","15down"};
 static const char *const yaw[7]={"45right","30right","15right","forward","15left","30left","45left"};
 unsigned t,r,k;char name[128];
 if(cod1GenericMGMap.ready){memcpy(map,cod1GenericMGMap.entries,sizeof(cod1GenericMGMap.entries));return 1;}
 for(t=0;t<2;t++){
  snprintf(name,sizeof(name),"standMG42gunner_%s",role[t]);map[t][0]=ActorGenericOrdinal(name);if(!map[t][0])return 0;
  for(r=0;r<3;r++){
   snprintf(name,sizeof(name),"standMG42gunner_%s_%s",role[t],height[r]);map[t][1+r]=ActorGenericOrdinal(name);
   if(!map[t][1+r]||cod1GenericDefinitions[map[t][1+r]].parent!=map[t][0])return 0;
   for(k=0;k<7;k++){
    snprintf(name,sizeof(name),"standMG42gunner_%s_%s_%s",role[t],yaw[k],height[r]);map[t][4+r*7+k]=ActorGenericOrdinal(name);
    if(!map[t][4+r*7+k]||cod1GenericDefinitions[map[t][4+r*7+k]].parent!=map[t][1+r])return 0;
   }
  }
 }
 memcpy(cod1GenericMGMap.entries,map,sizeof(cod1GenericMGMap.entries));
 cod1GenericMGMap.ready=1;
 return 1;
}
/* Mutators are pure node/Look arithmetic: they must never call the engine.
 * Publication and MG tree synchronization are added by the concrete caller.
 * Until then this transaction refuses changes to mapped MG nodes rather than
 * creating an independent animation authority. */
static int ActorGenericTransactionCore(const GscEntityRef *ref,uint64_t runtime,uint64_t catalog,int now,
 const void *borrowed,size_t bytes,char *error,size_t cap,int (*current)(void*),void *ctx,
 int (*mutate)(C1GARecord*,void*),void *user) {
 cod1ActorLease_t lease,after;C1GARecord stage,*old=NULL;GscEntityRef owner;
 cod1ActorRecord_t *actorCopy=NULL;gentity_t *entityCopy=NULL;
 void *copy=NULL;uint64_t revision=0,epoch;int ok=0;
 unsigned i,t,j,map[2][25];float dt;C1TRecord *gun=NULL;c1att14Tree_t mgBefore[2],mgAfter[2];int selected=-1,haveMG=0,faceRequested=mutate==ActorFaceMutate||mutate==ActorScriptedMutate;
 if(!ref||!runtime||!catalog||!current||!mutate||now!=level.time||now<0||!borrowed||!bytes||
    cod1GenericOperation.busy||!ActorGoalBuffers(borrowed,bytes,NULL,0,error,cap)||
    !ActorGoalBuffers(ref,sizeof(*ref),NULL,0,error,cap)||ActorGenericPrivate(ref,sizeof(*ref))||ActorGenericPrivate(borrowed,bytes)||ActorGenericPrivate(error,cap)||
    ActorOverlap(borrowed,bytes,error,cap)||!ActorGenericBuild())return 0;
 owner=*ref;memset(&stage,0,sizeof(stage));copy=malloc(bytes);if(!copy)return 0;memcpy(copy,borrowed,bytes);
 epoch=cod1GenericEpoch;
 cod1GenericOperation.busy=1;cod1GenericOperation.cancelled=0;
 if(!ActorGoalLease(&owner,&lease))goto done;
 actorCopy=malloc(sizeof(*actorCopy));entityCopy=malloc(sizeof(*entityCopy));if(!actorCopy||!entityCopy)goto done;
 memcpy(actorCopy,&cod1Actors[lease.actorIndex],sizeof(*actorCopy));memcpy(entityCopy,&g_entities[owner.slot],sizeof(*entityCopy));
 if(current(ctx)!=1||memcmp(ref,&owner,sizeof(owner))||memcmp(copy,borrowed,bytes)||
    memcmp(actorCopy,&cod1Actors[lease.actorIndex],sizeof(*actorCopy))||memcmp(entityCopy,&g_entities[owner.slot],sizeof(*entityCopy))||
    cod1GenericOperation.cancelled||epoch!=cod1GenericEpoch)goto done;
 old=&cod1GenericActors[lease.actorIndex];revision=old->revision;
 if(old->used&&(memcmp(&old->lease,&lease,sizeof(lease))||old->runtime!=runtime||old->catalog!=catalog))goto done;
 if(old->used){if(!ActorGenericClone(&stage,old))goto done;}
 else if(!ActorGenericFresh(&stage,&lease,runtime,catalog,now))goto done;
 cod1GenericOperation.stage=stage.nodes;cod1GenericOperation.bytes=C1GA_COUNT*sizeof(*stage.nodes);cod1GenericOperation.notifyStage=stage.notify;cod1GenericOperation.notifyBytes=sizeof(*stage.notify);
 if(stage.tick>now||stage.revision==UINT64_MAX||(now!=stage.tick&&ActorGenericNamedPending(&stage)))goto done;
 /* The selected phase owner prepares this row exactly once before callbacks.
  * A lazy legacy command must not advance it or replace that original order. */
 if(ActorFaceClockSelected(lease.actorIndex)&&stage.tick!=now)goto done;
 if(!ActorFaceFrameCanMutate(&stage,faceRequested,0))goto done;
 if(!ActorGenericMGMap(map))goto done;
 for(i=0;i<C1T_CAPACITY;i++)if(cod1Turrets[i].used&&ActorRefEqual(&cod1Turrets[i].actor,&owner)){
  if(gun)goto done;gun=cod1Turrets+i;
 }
 if(gun){haveMG=TurretAnimationActorRead(gun,now,mgBefore,&selected);if(haveMG<0)goto done;}
 if(faceRequested&&haveMG)goto done;
 if(!ActorGenericDt(now-stage.tick,&dt))goto done;
 /* Until real resources/clocks are attached, any active animated leaf is held.
  * Zero-frame Look leaves are admitted later through their actual C1AR bank. */
 for(i=1;i<C1GA_COUNT;i++)if(!cod1GenericDefinitions[i].children&&
    (stage.nodes[i].currentWeight||stage.nodes[i].goalWeight)&&now!=stage.tick){
   int mapped=0;for(t=0;t<2;t++)for(j=4;j<25;j++)if(i==map[t][j])mapped=1;
   if(!mapped||haveMG!=1){
    int clip=C1AL14_Index(cod1GenericNames[i].name),shuffle=ActorGenericShuffleIndex(cod1GenericNames[i].name);
    if(cod1GenericLookEpoch!=epoch)goto done;
    if(shuffle>=0){
     unsigned p;if(!C1AUT14_ValidShuffle(cod1GenericUnmountedBank.root[shuffle],cod1GenericUnmountedBank.body[shuffle]))goto done;
     /* This exact original branch has no sync ancestor. Never replace a
      * future sync-tree frequency contract with independent leaf clocks. */
     for(p=i;p;p=cod1GenericDefinitions[p].parent)if(cod1GenericDefinitions[p].flags&3)goto done;
    }else{
     if(clip<0||!C1AL14_Ready(&cod1GenericLookBank)||!C1AL14_Valid((unsigned)clip,cod1GenericLookBank.root[clip],cod1GenericLookBank.body[clip]))goto done;
     /* Genuine zero-interval, zero-note leaf: its phase cannot advance. */
     if(stage.nodes[i].phase||stage.nodes[i].oldPhase||stage.nodes[i].cycle||stage.nodes[i].oldCycle)goto done;
    }
   }
 }
 if(now!=stage.tick)for(i=0;i<C1GA_COUNT;i++){
  /* Native47B190+3D/+4A copies actualphase/cycle before server weight update. */
  stage.nodes[i].oldPhase=stage.nodes[i].phase;stage.nodes[i].oldCycle=stage.nodes[i].cycle;
  if(!C1ATL14_AdvanceWeight(stage.nodes+i,dt))goto done;
 }
 if(now!=stage.tick)for(i=0;i<2;i++){
  unsigned ordinal=ActorGenericOrdinal(c1aut14Names[i]);float delta;const c1ar14Root_t *root=cod1GenericUnmountedBank.root[i];
  if(!ordinal)goto done;if(stage.nodes[ordinal].currentWeight||stage.nodes[ordinal].goalWeight){
   if(!C1AUT14_ValidShuffle(root,cod1GenericUnmountedBank.body[i]))goto done;
   if(stage.notify->flags[ordinal].token){if(!ActorGenericNamedAdvance(&stage,ordinal,root,dt,now))goto done;}
   else if(!C1ATL14_AdvanceLeaf(stage.nodes+ordinal,root->intervals,root->fps,root->flags&1,root->noteCount,dt,&delta))goto done;
  }
 }
 if(haveMG==1)for(t=0;t<2;t++)for(j=0;j<25;j++)stage.nodes[map[t][j]]=mgBefore[t].node[j];
 stage.tick=now;
 if(!mutate(&stage,user)||!C1AGT14_Valid(cod1GenericDefinitions,stage.nodes,C1GA_COUNT))goto done;
 /* Refuse an unsupported active leaf before committing any goals. A successful
  * generic command may not defer missing resource/loop contracts to next frame. */
 for(i=1;i<C1GA_COUNT;i++)if(!cod1GenericDefinitions[i].children&&
    (stage.nodes[i].currentWeight||stage.nodes[i].goalWeight)){
  const c1ar14Root_t *root=NULL;int clip=C1AL14_Index(cod1GenericNames[i].name),shuffle=ActorGenericShuffleIndex(cod1GenericNames[i].name),face=ActorFaceLeafIndex(i),scripted=ActorScriptedIndex(i);unsigned p,mode=0;
  if(clip>=0){
   if(cod1GenericLookEpoch!=epoch||!C1AL14_Valid((unsigned)clip,cod1GenericLookBank.root[clip],cod1GenericLookBank.body[clip]))goto done;
   root=cod1GenericLookBank.root[clip];
  }else if(shuffle>=0){
   if(cod1GenericLookEpoch!=epoch||!C1AUT14_ValidShuffle(cod1GenericUnmountedBank.root[shuffle],cod1GenericUnmountedBank.body[shuffle]))goto done;
   root=cod1GenericUnmountedBank.root[shuffle];
  }else if(scripted>=0&&ActorScriptedClockSelected(lease.actorIndex)){root=ActorScriptedRoot(i);}
  else if(face>=0&&(faceRequested||ActorFaceClockSelected(lease.actorIndex))){root=ActorFaceRoot(i);}
  else for(t=0;t<2;t++)for(j=4;j<25;j++)if(i==map[t][j]){
   if(haveMG!=1)goto done;root=mgBefore[t].bank[j-4];
  }
  if(!root||(root->noteCount&&face<0&&!(scripted>=0&&ActorScriptedClockSelected(lease.actorIndex))))goto done;
  for(p=cod1GenericDefinitions[i].parent;p;p=cod1GenericDefinitions[p].parent)if(cod1GenericDefinitions[p].flags&3){mode=cod1GenericDefinitions[p].flags&3;break;}
  if(mode&&((root->flags&1)!=((mode==1)?1u:0u)))goto done;
 }
 if(cod1GenericOperation.cancelled||epoch!=cod1GenericEpoch||!cod1EntityReady||level.spawning||level.restarted||
    current(ctx)!=1||memcmp(copy,borrowed,bytes)||memcmp(ref,&owner,sizeof(owner))||
    !ActorGoalLease(&owner,&after)||memcmp(&lease,&after,sizeof(lease))||
    memcmp(actorCopy,&cod1Actors[lease.actorIndex],sizeof(*actorCopy))||memcmp(entityCopy,&g_entities[owner.slot],sizeof(*entityCopy))||
    cod1GenericOperation.cancelled||old->revision!=revision)goto done;
 if(haveMG==1){
  memcpy(mgAfter,mgBefore,sizeof(mgAfter));for(t=0;t<2;t++)for(j=0;j<25;j++)mgAfter[t].node[j]=stage.nodes[map[t][j]];
  if(!TurretAnimationActorCommit(gun,now,mgBefore,mgAfter))goto done;
 }else{
  const c1atl14Node_t fresh={0,0,0,0,0,1,0,0};
  for(t=0;t<2;t++)for(j=0;j<25;j++)if(memcmp(stage.nodes+map[t][j],old->used?old->nodes+map[t][j]:&fresh,sizeof(fresh)))goto done;
 }
 if(!ActorFaceFrameCanMutate(&stage,faceRequested,1))goto done;
 stage.revision++;ActorGenericFreeOwned(old);*old=stage;stage.nodes=NULL;stage.notify=NULL;ok=1;
done:ActorGenericFreeOwned(&stage);free(copy);free(actorCopy);free(entityCopy);cod1GenericOperation.stage=NULL;cod1GenericOperation.bytes=0;cod1GenericOperation.notifyStage=NULL;cod1GenericOperation.notifyBytes=0;cod1GenericOperation.busy=0;return ok;
}
static int ActorGenericMutate(C1GARecord*,void*);
static int ActorGenericTransaction(const GscEntityRef*ref,uint64_t runtime,uint64_t catalog,int now,
 const void*borrowed,size_t bytes,char*error,size_t cap,int(*current)(void*),void*ctx,
 int(*mutate)(C1GARecord*,void*),void*user){
 int owned,ok,kind=mutate==ActorGenericMutate?1:mutate==ActorFaceMutate?2:mutate==ActorScriptedMutate?3:0;
 owned=ActorControllerGenericSourceEnter(ref,runtime,catalog,now,borrowed,bytes,current,ctx,user,kind);
 if(owned<0)return 0;if(!owned&&!ActorControllerForeignMutationAllowed())return 0;
 ok=ActorGenericTransactionCore(ref,runtime,catalog,now,borrowed,bytes,error,cap,
   owned?ActorControllerGenericSourceCurrent:current,owned?ActorControllerGenericSourceContext():ctx,mutate,user);
 return ActorControllerGenericSourceFinish(owned,ok);
}
static int ActorGenericBinding(const GscAnimationLengthRequest *a,uint64_t runtime) {
 unsigned n=a->ordinal;
 if(!n)return a->runtime_identity==runtime&&memchr(a->tree,0,sizeof(a->tree))&&memchr(a->name,0,sizeof(a->name))&&!strcmp(a->tree,"generic_human")&&!strcmp(a->name,"root")&&a->flags==(GSC_ANIMATION_NODE_BRANCH|GSC_ANIMATION_NODE_IMPLICIT_ROOT)&&cod1GenericDefinitions&&cod1GenericDefinitions[0].children>0;
 return n&&n<C1GA_COUNT&&a->runtime_identity==runtime&&memchr(a->tree,0,sizeof(a->tree))&&
  memchr(a->name,0,sizeof(a->name))&&!strcmp(a->tree,"generic_human")&&
  !strcmp(a->name,cod1GenericNames[n].name)&&a->flags==cod1GenericNames[n].flags;
}
#include "g_cod1_actor_named_notify.inc"
/* ENGINE101 only writes below its exclusive stop ancestor. The ended
 * script's named idle face occupies a disjoint subtree on this same tree. */
static int ActorOwnedEyeResetBody(const C1GARecord*,unsigned*);
static int ActorSpeakingFaceLeaf(unsigned);
static int ActorIndependentIdleLook(const C1GARecord *stage,const GscActorAnimationRequest *q){
 unsigned i,target=q->animation[0].ordinal,stop=q->animation[1].ordinal;
 unsigned body=0;
 int look=C1AL14_Index(cod1GenericNames[target].name);
 if(!ActorScriptedClockSelected(stage->lease.actorIndex)||ActorGenericNamedPending(stage)||look<0)return 0;
 if(!cod1Actors[stage->lease.actorIndex].scriptedController.resumed4){
  /* The four Look eyes are independent leaves. The two sideways Look
   * selectors write below their exclusive stop. Neither may change an
   * ancestor clock or any named subtree belonging to the body or face. */
  if(q->method==GSC_ACTOR_ANIMATION_SET){
   float weight;memcpy(&weight,q->scalar_bits,sizeof(weight));
   if(look>=4||!(weight>=0 && weight<=1)||q->scalar_bits[1]!=0x3dcccccdu||
      q->scalar_bits[2]!=0x3f800000u)return 0;
  }else{
   unsigned p;
   if(q->method!=GSC_ACTOR_ANIMATION_KNOB_ALL||!ActorGenericBinding(q->animation+1,q->runtime_identity)||
      (strcmp(cod1GenericNames[stop].name,"look_left")&&strcmp(cod1GenericNames[stop].name,"look_right"))||
      !C1AGT14_Descendant(cod1GenericDefinitions,target,stop)||target==stop)return 0;
   for(p=target;;p=cod1GenericDefinitions[p].parent){
    if(cod1GenericDefinitions[p].flags&7)return 0;
    if(!p)break;
   }
  }
  if(!ActorRunnerFaceOwner(stage)&&!ActorOwnedEyeResetBody(stage,&body))return 0;
 }
 if(q->method==GSC_ACTOR_ANIMATION_SET){
  unsigned p;
  /* With every ancestor already weighted and unsynchronized, original 109
   * can touch only this Look leaf. Neither the named sibling nor its
   * inherited notification eligibility can change. */
  if(cod1GenericDefinitions[target].children)return 0;
  for(p=cod1GenericDefinitions[target].parent;;p=cod1GenericDefinitions[p].parent){
   if(!stage->nodes[p].goalWeight||(cod1GenericDefinitions[p].flags&7))return 0;
   if(!p)break;
  }
  stop=target;
 }else if(q->method!=GSC_ACTOR_ANIMATION_KNOB_ALL||!ActorGenericBinding(q->animation+1,q->runtime_identity)||!stop||
    (strcmp(cod1GenericNames[stop].name,"look_left")&&strcmp(cod1GenericNames[stop].name,"look_right")&&
     strcmp(cod1GenericNames[stop].name,"look_straight")))return 0;
 for(i=0;i<C1GA_COUNT;i++)if(stage->notify->flags[i].token){
    if((i!=body&&ActorFaceLeafIndex(i)<2&&!ActorSpeakingFaceLeaf(i)&&!(ActorRunnerFaceOwner(stage)&&ActorFaceLeafIndex(i)>=0))||stage->notify->flags[i].descendant||
       C1AGT14_Descendant(cod1GenericDefinitions,i,stop)||C1AGT14_Descendant(cod1GenericDefinitions,stop,i))return 0;
 }
 return 1;
}
/* look::lookAtTarget reasserts its straight-eye parent while a named
 * line plays below it. ENGINE 109 can only rewrite this node when every
 * ancestor is already weighted and the path is unsynchronized. Execute
 * the ordinary setter and require the parent clock to be byte-identical. */
static int ActorReassertSpeakingEyeParent(C1GARecord *stage,const GscActorAnimationRequest *q){
 const unsigned node=q->animation[0].ordinal;
 c1atl14Node_t before=stage->nodes[node];
 unsigned parent;
 /* The map uses the same original look::lookAtTarget on Foley during
  * his named gestures. Its straight-eye write has the same unchanged
  * parent clock and already-weighted ancestor requirements. */
 const gentity_t *e=g_entities+stage->lease.entity.slot;
 int foley=e->targetname && !strcmp(e->targetname,"foley") && e->classname &&
     !strcmp(e->classname,"actor_ally_airborne_officer_thompson");
 if((!foley&&!ActorRunnerFaceOwner(stage))||ActorGenericNamedPending(stage)||
    q->method!=GSC_ACTOR_ANIMATION_SET||strcmp(cod1GenericNames[node].name,"eyes_straight")||
    !cod1GenericDefinitions[node].children||q->scalar_bits[0]!=0x3f800000u||
    q->scalar_bits[1]!=0x3dcccccdu||q->scalar_bits[2]!=0x3f800000u||
    before.goalWeight!=1.0f||before.currentWeight!=1.0f||before.remaining!=0.0f||before.rate!=1.0f)return 0;
 for(parent=node;;parent=cod1GenericDefinitions[parent].parent){
  if(cod1GenericDefinitions[parent].flags&7||!stage->nodes[parent].goalWeight)return 0;
  if(!parent)break;
 }
 return C1AGT14_SetAnim(cod1GenericDefinitions,stage->nodes,C1GA_COUNT,node,1.0f,0.1f,1.0f,0)&&
    !memcmp(&before,stage->nodes+node,sizeof(before));
}
/* The original stop initializer writes body and shoot while a line is
 * still speaking. Accept the two stock writes on this stopped actor only
 * when every live name belongs to an independent facial leaf. */
static int MoodyStopped(const C1GARecord*);
static int MoodyStopSet(const C1GARecord *stage,const GscActorAnimationRequest *q){
 unsigned i,n=q->animation[0].ordinal,p;
 const char *name=cod1GenericNames[n].name;
 if(q->method!=GSC_ACTOR_ANIMATION_SET || !MoodyStopped(stage) || ActorGenericNamedPending(stage) ||
    q->scalar_bits[1]!=0x3e4ccccdu || q->scalar_bits[2]!=0x3f800000u ||
    !((!strcmp(name,"body") && q->scalar_bits[0]==0x3e4ccccdu) ||
      (!strcmp(name,"shoot") && !q->scalar_bits[0])))return 0;
 for(p=cod1GenericDefinitions[n].parent;;p=cod1GenericDefinitions[p].parent){
  if(!stage->nodes[p].goalWeight || (cod1GenericDefinitions[p].flags&7))return 0;
  if(!p)break;
 }
 for(i=0;i<C1GA_COUNT;i++)if(stage->notify->flags[i].token &&
    (!ActorSpeakingFaceLeaf(i) || stage->notify->flags[i].descendant ||
     C1AGT14_Descendant(cod1GenericDefinitions,i,n) || C1AGT14_Descendant(cod1GenericDefinitions,n,i)))return 0;
 return 1;
}
static int ActorGenericMutate(C1GARecord *stage,void *user) {
 const GscActorAnimationRequest *q=user;float f[3];unsigned n=q->animation[0].ordinal;
 if((q->method==GSC_ACTOR_ANIMATION_FLAGGED_ALL_RESTART||q->method==GSC_ACTOR_ANIMATION_FLAGGED_ALL))return ActorGenericNamedLeafStage(stage,user);
 if(!ActorGenericBinding(q->animation,q->runtime_identity)||q->notify[0])return 0;
 memcpy(f,q->scalar_bits,sizeof(f));
 if(q->method==GSC_ACTOR_ANIMATION_CLEAR){if(!C1AGT14_Clear(cod1GenericDefinitions,stage->nodes,C1GA_COUNT,n,f[0]))return 0;ActorGenericNamedCancelSubtree(stage,n);return 1;}
 if(ActorGenericNamedActive(stage)&&!MoodyStopSet(stage,q)&&!ActorIndependentIdleLook(stage,q))return ActorReassertSpeakingEyeParent(stage,q);
 if(q->method==GSC_ACTOR_ANIMATION_SET)return C1AGT14_SetAnim(cod1GenericDefinitions,stage->nodes,C1GA_COUNT,n,f[0],f[1],f[2],0);
 if(q->method==GSC_ACTOR_ANIMATION_KNOB_ALL&&ActorGenericBinding(q->animation+1,q->runtime_identity))
  return C1AKR14_Select(cod1GenericDefinitions,stage->nodes,C1GA_COUNT,n,q->animation[1].ordinal,f[0],f[1],f[2],0);
 return 0;
}
extern int R243CommandSessionTake(const GscActorAnimationRequest*,int(*)(void*),void*);
static int ActorGenericInvoke(void *unused,const GscActorAnimationRequest *input,char *error,size_t cap,int (*current)(void*),void *ctx) {
 GscActorAnimationRequest q;(void)unused;
 if(!input||!ActorGoalBuffers(input,sizeof(*input),NULL,0,error,cap)||ActorGenericPrivate(input,sizeof(*input)))return 0;
 q=*input;if(q.precision_bits!=53||q.rounding||
   (q.method!=GSC_ACTOR_ANIMATION_CLEAR&&q.method!=GSC_ACTOR_ANIMATION_SET&&q.method!=GSC_ACTOR_ANIMATION_KNOB_ALL&&q.method!=GSC_ACTOR_ANIMATION_FLAGGED_ALL_RESTART&&q.method!=GSC_ACTOR_ANIMATION_FLAGGED_ALL))return 0;
 if(q.method==GSC_ACTOR_ANIMATION_FLAGGED_ALL&&!R243CommandSessionTake(input,current,ctx))return 0;
 return ActorGenericTransaction(&input->entity,q.runtime_identity,q.catalog_identity,q.now_ms,input,sizeof(*input),error,cap,current,ctx,ActorGenericMutate,&q);
}
static int ActorGenericValidate(void *unused,const GscEntityRef *input,uint64_t runtime,int now,char *error,size_t cap,int (*current)(void*),void *ctx) {
 GscEntityRef owner;cod1ActorLease_t lease,after;int ok=0;(void)unused;
 if(!input||!runtime||now!=level.time||!current||cod1GenericOperation.busy||
    !ActorGoalBuffers(input,sizeof(*input),NULL,0,error,cap)||ActorGenericPrivate(input,sizeof(*input))||!ActorGenericBuild())return 0;
 owner=*input;cod1GenericOperation.busy=1;cod1GenericOperation.cancelled=0;
 if(ActorGoalLease(&owner,&lease)&&current(ctx)==1&&!memcmp(input,&owner,sizeof(owner))&&
    !cod1GenericOperation.cancelled&&cod1GenericEpoch==cod1EntityEpoch&&ActorGoalLease(&owner,&after)&&!memcmp(&lease,&after,sizeof(lease)))ok=1;
 cod1GenericOperation.busy=0;return ok;
}
const GscActorAnimationHost *G_Cod1ActorGenericAnimationHost(void) {
 static const GscActorAnimationHost host={NULL,ActorGenericValidate,ActorGenericInvoke};return &host;
}
/* Pure final half of the existing turret transaction. It runs AFTER that
 * owner's last external fence; neither owner may call out after this commit.
 * Legacy independently selected MG-only mechanisms have no generic record. */
static int ActorGenericTurretCommit(const GscTurretAnimationRequest *q,const C1TARecord *next) {
 C1GARecord *old,stage;unsigned map[2][25],i,t,j;float f[3],dt;int ok=0;
 if(!q||!next||!next->used||next->lease.actorIndex>=COD1_ACTOR_CAPACITY)return 0;
 old=cod1GenericActors+next->lease.actorIndex;
 if(!old->used)return 1;
 if(q->method!=GSC_ANIMATION_SET_KNOB_RESTART||!ActorRefEqual(&q->entity,&next->actor))return 1;
 if(ActorGenericNamedActive(old))return 0;
 if(cod1GenericOperation.busy||old->runtime!=q->animation.runtime_identity||old->catalog!=q->catalog_identity||
    old->revision==UINT64_MAX||old->tick>q->now_ms||q->now_ms!=level.time||
    cod1GenericEpoch!=cod1EntityEpoch||memcmp(&old->lease,&next->lease,sizeof(old->lease))||
    !ActorGenericBinding(&q->animation,old->runtime)||!ActorGenericMGMap(map))return 0;
 if(!ActorGenericClone(&stage,old))return 0;
 if(!ActorGenericDt(q->now_ms-old->tick,&dt))goto done;
 for(i=0;i<C1GA_COUNT;i++)if(!C1ATL14_AdvanceWeight(stage.nodes+i,dt))goto done;
 memcpy(f,&q->weight_bits,4);memcpy(f+1,&q->blend_bits,4);memcpy(f+2,&q->rate_bits,4);
 if(!C1AGT14_SetAnim(cod1GenericDefinitions,stage.nodes,C1GA_COUNT,q->animation.ordinal,f[0],f[1],f[2],1))goto done;
 for(t=0;t<2;t++)for(j=0;j<25;j++)stage.nodes[map[t][j]]=next->actorTree[t].node[j];
 if(!C1AGT14_Valid(cod1GenericDefinitions,stage.nodes,C1GA_COUNT))goto done;
 stage.tick=q->now_ms;stage.revision++;ActorGenericFreeOwned(old);*old=stage;stage.nodes=NULL;stage.notify=NULL;ok=1;
done:ActorGenericFreeOwned(&stage);return ok;
}
int G_Cod1ActorGenericPresent(const GscEntityRef *ref) {
 cod1ActorLease_t lease;C1GARecord *row;
 if(!ref||!ActorGoalLease(ref,&lease))return -1;row=cod1GenericActors+lease.actorIndex;
 if(!row->used)return 0;
 return cod1GenericEpoch==cod1EntityEpoch&&!memcmp(&row->lease,&lease,sizeof(lease))?1:-1;
}
static const c1al14Bank_t *ActorGenericLookBankView(const GscEntityRef *ref,int now) {
 if(now!=level.time||G_Cod1ActorGenericPresent(ref)!=1||cod1GenericLookEpoch!=cod1EntityEpoch||!C1AL14_Ready(&cod1GenericLookBank))return NULL;
 return &cod1GenericLookBank;
}
/* Read-only current pose sample. It must not increment the generic revision:
 * Look's eye query uses this very pose before its next state transaction. */
int G_Cod1ActorGenericSample(const GscEntityRef *input,int now,float output[98],int (*current)(void*),void *ctx) {
 C1GARecord saved;GscEntityRef owner;cod1ActorLease_t lease,after;
 c1atl14Node_t *nodes=NULL;float values[98],dt;C1TRecord *gun=NULL;c1att14Tree_t mg[2];
 cod1ActorRecord_t *actorCopy=NULL;gentity_t *entityCopy=NULL;
 unsigned i,t,j,map[2][25];uint64_t epoch;int selected=-1,have=0,ok=0;
 if(!input||!output||!current||now!=level.time||cod1GenericOperation.busy||
    !ActorGoalBuffers(input,sizeof(*input),output,sizeof(values),NULL,0)||ActorGenericPrivate(input,sizeof(*input))||ActorGenericPrivate(output,sizeof(values)))return 0;
 owner=*input;epoch=cod1EntityEpoch;cod1GenericOperation.busy=1;cod1GenericOperation.cancelled=0;
 if(!ActorGoalLease(&owner,&lease)||G_Cod1ActorGenericPresent(&owner)!=1)goto done;
 saved=cod1GenericActors[lease.actorIndex];if(saved.tick>now||!ActorGenericMGMap(map))goto done;
 actorCopy=malloc(sizeof(*actorCopy));entityCopy=malloc(sizeof(*entityCopy));if(!actorCopy||!entityCopy)goto done;
 memcpy(actorCopy,&cod1Actors[lease.actorIndex],sizeof(*actorCopy));memcpy(entityCopy,&g_entities[owner.slot],sizeof(*entityCopy));
 nodes=malloc(C1GA_COUNT*sizeof(*nodes));if(!nodes)goto done;memcpy(nodes,saved.nodes,C1GA_COUNT*sizeof(*nodes));
 cod1GenericOperation.stage=nodes;cod1GenericOperation.bytes=C1GA_COUNT*sizeof(*nodes);
 if(current(ctx)!=1||cod1GenericOperation.cancelled||epoch!=cod1EntityEpoch||memcmp(input,&owner,sizeof(owner))||
    memcmp(&saved,cod1GenericActors+lease.actorIndex,sizeof(saved))||memcmp(actorCopy,&cod1Actors[lease.actorIndex],sizeof(*actorCopy))||memcmp(entityCopy,&g_entities[owner.slot],sizeof(*entityCopy)))goto done;
 if(!ActorGenericDt(now-saved.tick,&dt))goto done;
 for(i=0;i<C1GA_COUNT;i++)if(!C1ATL14_AdvanceWeight(nodes+i,dt))goto done;
 for(i=0;i<C1T_CAPACITY;i++)if(cod1Turrets[i].used&&ActorRefEqual(&cod1Turrets[i].actor,&owner)){if(gun)goto done;gun=cod1Turrets+i;}
 if(gun){have=TurretAnimationActorRead(gun,now,mg,&selected);if(have<0)goto done;}
 if(have==1)for(t=0;t<2;t++)for(j=0;j<25;j++)nodes[map[t][j]]=mg[t].node[j];
 for(i=1;i<C1GA_COUNT;i++)if(!cod1GenericDefinitions[i].children&&(nodes[i].currentWeight||nodes[i].goalWeight)){
  int covered=0;for(j=0;j<C1ALT14_NODES;j++)if(c1alt14Definitions[j].ordinal==i&&c1alt14Definitions[j].leaf>=0)covered=1;
  if(!covered)goto done;
 }
 for(i=0;i<C1ALT14_NODES;i++){
  unsigned ordinal=c1alt14Definitions[i].ordinal;
  if(ordinal>=C1GA_COUNT||(i&&strcmp(c1alt14NodeNames[i],cod1GenericNames[ordinal].name)))goto done;
  values[i]=nodes[ordinal].currentWeight;
 }
 if(current(ctx)!=1||cod1GenericOperation.cancelled||epoch!=cod1EntityEpoch||memcmp(input,&owner,sizeof(owner))||
    memcmp(&saved,cod1GenericActors+lease.actorIndex,sizeof(saved))||!ActorGoalLease(&owner,&after)||memcmp(&lease,&after,sizeof(lease))||
    memcmp(actorCopy,&cod1Actors[lease.actorIndex],sizeof(*actorCopy))||memcmp(entityCopy,&g_entities[owner.slot],sizeof(*entityCopy)))goto done;
 memcpy(output,values,sizeof(values));ok=1;
done:free(nodes);free(actorCopy);free(entityCopy);cod1GenericOperation.stage=NULL;cod1GenericOperation.bytes=0;cod1GenericOperation.notifyStage=NULL;cod1GenericOperation.notifyBytes=0;cod1GenericOperation.busy=0;return ok;
}
/* Read only the actual already advanced unmounted tree. A future walk entry
 * must select and clock its genuine clips before this view can publish them. */
int G_Cod1ActorUnmountedSample(const GscEntityRef *input,int now,float output[98],float phases[2][21],int (*current)(void*),void *ctx){
 GscEntityRef owner;cod1ActorLease_t lease,after;C1GARecord saved;
 cod1ActorRecord_t *actorCopy=NULL;gentity_t *entityCopy=NULL;c1atl14Node_t *copy=NULL;
 float values[98]={0},times[2][21]={{0}};c1ast14Node_t checked[50];uint64_t epoch;unsigned i,j;int ok=0;
 if(!input||!output||!phases||!current||now!=level.time||cod1GenericOperation.busy||
    !ActorGoalBuffers(input,sizeof(*input),output,sizeof(values),NULL,0)||!ActorGoalBuffers(input,sizeof(*input),phases,sizeof(times),NULL,0)||
    ActorOverlap(output,sizeof(values),phases,sizeof(times))||ActorGenericPrivate(input,sizeof(*input))||ActorGenericPrivate(output,sizeof(values))||ActorGenericPrivate(phases,sizeof(times)))return 0;
 owner=*input;epoch=cod1EntityEpoch;cod1GenericOperation.busy=1;cod1GenericOperation.cancelled=0;
 if(!ActorGoalLease(&owner,&lease)||G_Cod1ActorGenericPresent(&owner)!=1||cod1Actors[lease.actorIndex].turretMount.mounted||cod1GenericLookEpoch!=epoch||!C1AUT14_Ready(&cod1GenericUnmountedBank))goto done;
 saved=cod1GenericActors[lease.actorIndex];if(saved.tick!=now)goto done;
 actorCopy=malloc(sizeof(*actorCopy));entityCopy=malloc(sizeof(*entityCopy));copy=malloc(C1GA_COUNT*sizeof(*copy));if(!actorCopy||!entityCopy||!copy)goto done;
 memcpy(actorCopy,cod1Actors+lease.actorIndex,sizeof(*actorCopy));memcpy(entityCopy,g_entities+owner.slot,sizeof(*entityCopy));memcpy(copy,saved.nodes,C1GA_COUNT*sizeof(*copy));
 cod1GenericOperation.stage=copy;cod1GenericOperation.bytes=C1GA_COUNT*sizeof(*copy);
 if(current(ctx)!=1||now!=level.time||epoch!=cod1EntityEpoch||cod1GenericOperation.cancelled||memcmp(input,&owner,sizeof(owner))||
    memcmp(&saved,cod1GenericActors+lease.actorIndex,sizeof(saved))||memcmp(actorCopy,cod1Actors+lease.actorIndex,sizeof(*actorCopy))||memcmp(entityCopy,g_entities+owner.slot,sizeof(*entityCopy)))goto done;
 for(i=1;i<C1GA_COUNT;i++)if(!cod1GenericDefinitions[i].children&&(copy[i].currentWeight||copy[i].goalWeight)){
  int covered=0;for(j=0;j<C1AUT14_NODES;j++)if(c1aut14Definitions[j].ordinal==i&&c1aut14Definitions[j].leaf>=0)covered=1;if(!covered)goto done;
 }
 for(i=0;i<C1AUT14_NODES;i++){
  const c1aut14Definition_t *d=c1aut14Definitions+i;
  if(d->ordinal>=C1GA_COUNT||(d->leaf>=0&&strcmp(c1aut14Names[d->leaf],cod1GenericNames[d->ordinal].name))||
     (i&&cod1GenericDefinitions[d->ordinal].parent!=c1aut14Definitions[d->parent].ordinal))goto done;
  values[i]=copy[d->ordinal].currentWeight;if(d->leaf>=0&&d->leaf<2)times[0][d->leaf]=copy[d->ordinal].phase;
 }
 if(!C1AUT14_Nodes(values,times,checked))goto done;
 if(current(ctx)!=1||now!=level.time||epoch!=cod1EntityEpoch||cod1GenericOperation.cancelled||memcmp(input,&owner,sizeof(owner))||
    memcmp(&saved,cod1GenericActors+lease.actorIndex,sizeof(saved))||!ActorGoalLease(&owner,&after)||memcmp(&lease,&after,sizeof(lease))||
    memcmp(actorCopy,cod1Actors+lease.actorIndex,sizeof(*actorCopy))||memcmp(entityCopy,g_entities+owner.slot,sizeof(*entityCopy))||
    !R242ExactATRBytesEqual(copy,saved.nodes,C1GA_COUNT*sizeof(*copy))||cod1GenericLookEpoch!=epoch||!C1AUT14_Ready(&cod1GenericUnmountedBank))goto done;
 memcpy(output,values,sizeof(values));memcpy(phases,times,sizeof(times));ok=1;
done:free(copy);free(actorCopy);free(entityCopy);cod1GenericOperation.stage=NULL;cod1GenericOperation.bytes=0;cod1GenericOperation.notifyStage=NULL;cod1GenericOperation.notifyBytes=0;cod1GenericOperation.busy=0;return ok;
}
static int ActorGenericNoChange(C1GARecord *record,void *unused){(void)record;(void)unused;return 1;}
/* ENGINE112 reads the interval of the already advanced authoritative tree.
 * The caller must provide the original script lifecycle's +268 byte. This
 * query neither selects that policy nor advances clocks a second time. */
static int ActorGenericRootMGCurrent(const GscEntityRef *owner,int index,const C1TRecord *gun,const C1TARecord *animation,int now){
 unsigned i;int found=-1;if(now!=level.time||cod1TurretAnimationOperation.busy)return 0;
 for(i=0;i<C1T_CAPACITY;i++)if(cod1Turrets[i].used&&ActorRefEqual(&cod1Turrets[i].actor,owner)){if(found>=0)return 0;found=(int)i;}
 if(found!=index)return 0;if(index<0)return 1;
 return !memcmp(cod1Turrets+index,gun,sizeof(*gun))&&!memcmp(cod1TurretAnimations+index,animation,sizeof(*animation))&&
  (!animation->used||(TurretAnimationLive(animation,cod1Turrets+index,now)&&animation->actorTree[0].sampleTick==now&&animation->actorTree[1].sampleTick==now));
}
int G_Cod1ActorRootInterval(const GscEntityRef *input,int now,int goalByte,c1ari14Result_t *output,int (*current)(void*),void *ctx){
 GscEntityRef owner;cod1ActorLease_t lease,after;C1GARecord saved;
 cod1ActorRecord_t *actorCopy=NULL;gentity_t *entityCopy=NULL;
 c1atl14Node_t *copy=NULL;c1ari14Node_t nodes[C1ALT14_NODES];
 const c1ar14Root_t *bank[C1ALT14_LEAVES];c1ari14Result_t value;
 C1TRecord gunSaved;C1TARecord mgSaved;unsigned map[2][25],t;int gunIndex=-1;
 uint64_t epoch;unsigned i,j,count,leaves;int unmounted,ok=0;
 if(!input||!output||!current||now!=level.time||(goalByte!=0&&goalByte!=1)||cod1GenericOperation.busy||
    !ActorGoalBuffers(input,sizeof(*input),output,sizeof(*output),NULL,0)||ActorGenericPrivate(input,sizeof(*input))||ActorGenericPrivate(output,sizeof(*output)))return 0;
 owner=*input;epoch=cod1EntityEpoch;cod1GenericOperation.busy=1;cod1GenericOperation.cancelled=0;
 if(!ActorGoalLease(&owner,&lease)||G_Cod1ActorGenericPresent(&owner)!=1)goto done;
 saved=cod1GenericActors[lease.actorIndex];if(saved.tick!=now||!ActorGenericMGMap(map)||!TurretAnimationBanks()||cod1GenericLookEpoch!=epoch||!C1AL14_Ready(&cod1GenericLookBank))goto done;
 memset(&gunSaved,0,sizeof(gunSaved));memset(&mgSaved,0,sizeof(mgSaved));
 for(i=0;i<C1T_CAPACITY;i++)if(cod1Turrets[i].used&&ActorRefEqual(&cod1Turrets[i].actor,&owner)){if(gunIndex>=0)goto done;gunIndex=(int)i;}
 if(gunIndex>=0){gunSaved=cod1Turrets[gunIndex];mgSaved=cod1TurretAnimations[gunIndex];}
 if(!ActorGenericRootMGCurrent(&owner,gunIndex,&gunSaved,&mgSaved,now))goto done;
 actorCopy=malloc(sizeof(*actorCopy));entityCopy=malloc(sizeof(*entityCopy));copy=malloc(2*C1GA_COUNT*sizeof(*copy));
 if(!actorCopy||!entityCopy||!copy)goto done;
 memcpy(actorCopy,cod1Actors+lease.actorIndex,sizeof(*actorCopy));memcpy(entityCopy,g_entities+owner.slot,sizeof(*entityCopy));memcpy(copy,saved.nodes,C1GA_COUNT*sizeof(*copy));memcpy(copy+C1GA_COUNT,copy,C1GA_COUNT*sizeof(*copy));
 unmounted=!actorCopy->turretMount.mounted;count=unmounted?C1AUT14_NODES:C1ALT14_NODES;leaves=unmounted?C1AUT14_LEAVES:C1ALT14_LEAVES;
 cod1GenericOperation.stage=copy;cod1GenericOperation.bytes=2*C1GA_COUNT*sizeof(*copy);
 if(current(ctx)!=1||now!=level.time||cod1GenericOperation.cancelled||epoch!=cod1EntityEpoch||memcmp(input,&owner,sizeof(owner))||!ActorGenericRootMGCurrent(&owner,gunIndex,&gunSaved,&mgSaved,now)||
    memcmp(&saved,cod1GenericActors+lease.actorIndex,sizeof(saved))||memcmp(actorCopy,cod1Actors+lease.actorIndex,sizeof(*actorCopy))||memcmp(entityCopy,g_entities+owner.slot,sizeof(*entityCopy)))goto done;
 for(i=1;i<C1GA_COUNT;i++)if(!cod1GenericDefinitions[i].children&&(copy[i].currentWeight||copy[i].goalWeight)){
  int covered=0;for(j=0;j<count;j++)if(unmounted?(c1aut14Definitions[j].ordinal==i&&c1aut14Definitions[j].leaf>=0):(c1alt14Definitions[j].ordinal==i&&c1alt14Definitions[j].leaf>=0))covered=1;if(!covered)goto done;
 }
 /* Copy an already advanced live MG owner, never ActorRead (which advances).
  * Same-tick mount selection may legitimately be newer than the generic copy. */
 if(!unmounted)for(t=0;t<2;t++)for(j=0;j<25;j++){
  if(mgSaved.used&&mgSaved.selected>=0)copy[map[t][j]]=mgSaved.actorTree[t].node[j];
  else if(copy[map[t][j]].currentWeight||copy[map[t][j]].goalWeight)goto done;
 }
 for(i=0;i<count;i++){
  const c1alt14Definition_t *d=c1alt14Definitions+i;const c1atl14Node_t *n;
  unsigned ordinal=unmounted?c1aut14Definitions[i].ordinal:d->ordinal;int parent=unmounted?c1aut14Definitions[i].parent:d->parent,leaf=unmounted?c1aut14Definitions[i].leaf:d->leaf;
  if(ordinal>=C1GA_COUNT||(!unmounted&&i&&strcmp(c1alt14NodeNames[i],cod1GenericNames[ordinal].name))||
     (unmounted&&leaf>=0&&strcmp(c1aut14Names[leaf],cod1GenericNames[ordinal].name)))goto done;n=copy+ordinal;
  nodes[i].ordinal=ordinal;nodes[i].parent=parent;nodes[i].leaf=leaf;
  nodes[i].currentWeight=n->currentWeight;nodes[i].goalWeight=n->goalWeight;nodes[i].oldPhase=n->oldPhase;nodes[i].phase=n->phase;
 }
 for(i=0;i<leaves;i++){
  const c1alt14Leaf_t *l=c1alt14Leaves+i;
  if(unmounted){bank[i]=i<2?cod1GenericUnmountedBank.root[i]:cod1GenericLookBank.root[i-2];continue;}
  /* Definitions use native runtime order, as TurretAnimationFresh's bank.
   * The immutable asset cache is loaded in the opposite yaw/height order. */
  if(l->mgGroup>=0){if(l->mgGroup>1||l->mgIndex<0||l->mgIndex>20)goto done;bank[i]=cod1TurretRoots[l->mgGroup][20-l->mgIndex];}
  else{if(l->lookIndex<0||l->lookIndex>=C1AL14_COUNT)goto done;bank[i]=cod1GenericLookBank.root[l->lookIndex];}
 }
 if(!C1ARI14_Interval(bank,leaves,nodes,count,goalByte,&value))goto done;
 if(current(ctx)!=1||now!=level.time||cod1GenericOperation.cancelled||epoch!=cod1EntityEpoch||memcmp(input,&owner,sizeof(owner))||!ActorGenericRootMGCurrent(&owner,gunIndex,&gunSaved,&mgSaved,now)||
    memcmp(&saved,cod1GenericActors+lease.actorIndex,sizeof(saved))||!ActorGoalLease(&owner,&after)||memcmp(&lease,&after,sizeof(lease))||
    memcmp(actorCopy,cod1Actors+lease.actorIndex,sizeof(*actorCopy))||memcmp(entityCopy,g_entities+owner.slot,sizeof(*entityCopy))||
    !R242ExactATRBytesEqual(copy+C1GA_COUNT,saved.nodes,C1GA_COUNT*sizeof(*copy))||!TurretAnimationBanks()||cod1GenericLookEpoch!=epoch||!C1AL14_Ready(&cod1GenericLookBank))goto done;
 memcpy(output,&value,sizeof(value));ok=1;
done:free(copy);free(actorCopy);free(entityCopy);cod1GenericOperation.stage=NULL;cod1GenericOperation.bytes=0;cod1GenericOperation.notifyStage=NULL;cod1GenericOperation.notifyBytes=0;cod1GenericOperation.busy=0;return ok;
}
int G_Cod1ActorGenericFrame(const GscEntityRef *ref,int now,int (*current)(void*),void *ctx){
 cod1ActorLease_t lease;C1GARecord *row;int present;
 if(!ref||!current||now!=level.time||!ActorGoalBuffers(ref,sizeof(*ref),NULL,0,NULL,0)||ActorGenericPrivate(ref,sizeof(*ref))||!ActorGoalLease(ref,&lease))return 0;
 present=G_Cod1ActorGenericPresent(ref);if(present<0)return 0;if(!present)return current(ctx)==1;
 if(ActorFaceClockSelected(lease.actorIndex))return 0;
 row=cod1GenericActors+lease.actorIndex;
 return ActorGenericTransaction(ref,row->runtime,row->catalog,now,ref,sizeof(*ref),NULL,0,current,ctx,ActorGenericNoChange,NULL);
}

/* R233 original GetAnimLength metadata. No allocation, file IO, tree mutation,
 * fallback duration, notify, or lazy clip loading occurs in this provider. */
static int ActorLoadedLengthScope(uint64_t epoch,uint64_t receipt) {
 return epoch&&receipt&&epoch==cod1EntityEpoch&&epoch==cod1GenericEpoch&&
  receipt==cod1GenericReceipt&&receipt==cod1TurretAnimationReceipt&&
  cod1EntityReady&&!level.spawning&&!level.restarted&&
  !cod1GenericOperation.busy&&!cod1TurretAnimationOperation.busy&&TurretAnimationBanks();
}
static int ActorLoadedLengthMetadata(const GscAnimationLengthRequest *q,GscAnimationLengthReply *out) {
 const c1ar14Root_t *root=NULL;const c1ab14Clip_t *body=NULL;
 static const char *const role[2]={"aim","fire"};
 static const char *const height[3]={"15down","level","15up"};
 static const char *const yaw[7]={"45left","30left","15left","forward","15right","30right","45right"};
 int i,j,k,index;char name[128];
 if(!q->runtime_identity||!ActorGenericBinding(q,q->runtime_identity)||
    !q->ordinal||(q->flags&GSC_ANIMATION_NODE_BRANCH))return 0;
 index=C1AL14_Index(q->name);
 if(index>=0){if(cod1GenericLookEpoch!=cod1EntityEpoch||!C1AL14_Valid((unsigned)index,cod1GenericLookBank.root[index],cod1GenericLookBank.body[index]))return 0;
  root=cod1GenericLookBank.root[index];body=cod1GenericLookBank.body[index];
 }else if((index=ActorGenericShuffleIndex(q->name))>=0){
  if(cod1GenericLookEpoch!=cod1EntityEpoch||!C1AUT14_ValidShuffle(cod1GenericUnmountedBank.root[index],cod1GenericUnmountedBank.body[index]))return 0;
  root=cod1GenericUnmountedBank.root[index];body=cod1GenericUnmountedBank.body[index];
 }else if(ActorFaceLeafIndex(q->ordinal)>=0){root=ActorFaceRoot(q->ordinal);body=ActorFaceBody(q->ordinal);
 }else{
  if(cod1TurretBankEpoch!=cod1EntityEpoch)return 0;
  for(i=0;i<2;i++)for(j=0;j<3;j++)for(k=0;k<7;k++){
   snprintf(name,sizeof(name),"standMG42gunner_%s_%s_%s",role[i],yaw[k],height[j]);
   if(!strcmp(q->name,name)){root=cod1TurretRoots[i][j*7+k];body=cod1TurretBodies[i][j*7+k];}
  }
 }
 if(!root||!body||root->magic!=C1AR14_MAGIC||body->magic!=C1AB14_MAGIC||
    root->frames!=body->frames||root->flags!=body->flags||root->fps!=body->fps||
    root->intervals!=body->intervals||root->intervals>4095||!root->fps||root->fps>120||
    root->intervals!=root->frames-1+(root->flags&1u))return 0;
 out->frames=root->intervals;out->fps=root->fps;return 1;
}
static int ActorLoadedLengthQuery(void *user,const GscAnimationLengthRequest *input,
 GscAnimationLengthReply *output,char *error,size_t cap,int(*current)(void*),void *ctx){
 GscAnimationLengthRequest q;GscAnimationLengthReply reply,again;uint64_t epoch,receipt;(void)user;
 if(!input||!output||!current||!ActorGoalBuffers(input,sizeof(*input),output,sizeof(*output),error,cap)||
    ActorGenericPrivate(input,sizeof(*input))||ActorGenericPrivate(output,sizeof(*output))||ActorGenericPrivate(error,cap)||
    TurretAnimationPrivate(input,sizeof(*input))||TurretAnimationPrivate(output,sizeof(*output))||TurretAnimationPrivate(error,cap)||
    TurretBankPrivate(input,sizeof(*input))||TurretBankPrivate(output,sizeof(*output))||TurretBankPrivate(error,cap))return 0;
 q=*input;epoch=cod1EntityEpoch;receipt=cod1TurretAnimationReceipt;
 if(!ActorLoadedLengthScope(epoch,receipt)||current(ctx)!=1||!ActorLoadedLengthScope(epoch,receipt)||
    memcmp(input,&q,sizeof(q))||!ActorLoadedLengthMetadata(&q,&reply))return 0;
 /* A foreign current callback may retire/reload the banks. Never retain a
  * resource pointer across it; re-resolve the original copied intent. */
 if(current(ctx)!=1||!ActorLoadedLengthScope(epoch,receipt)||memcmp(input,&q,sizeof(q))||
    !ActorLoadedLengthMetadata(&q,&again)||memcmp(&reply,&again,sizeof(reply)))return 0;
 *output=reply;return 1;
}
const GscAnimationLengthHost *G_Cod1LoadedAnimationLengthHost(void){
 static const GscAnimationLengthHost host={NULL,ActorLoadedLengthQuery};return &host;
}
