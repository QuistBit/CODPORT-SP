static int FoleyNormalIdleOwner(const C1GARecord*);
static int ActorHasNormalRecord(unsigned index);
/* GAME phase2/3 dependency. Original GAME2001F327/3BF and ENGINE47B190,
 *47CCD0: old time/weight once; repeat the SAME full dt after a VM resumption.
 * Deliberately bounded to the actual unmounted Look/shuffle/five-face closure,
 * identity-rate unsynchronized ancestors and one named face leaf. It supplies
 * no actor FSM, source entry, trigger, sound or VM scheduling implementation. */
#include "../g_cod1_actor_face_frame.h"
#include "../../qcommon/cod1_actor_named_clock14.h"
#include "../../qcommon/cod1_actor_flagged_knob14.h"
typedef struct {
 int selected,open,done,tick,poisoned,scripted,gameFrame;
 uint64_t runtime,catalog,identity,frame;
 cod1ActorLease_t lease;
 unsigned count,nextNote;
 unsigned pairNative[C1ASB14_COUNT],idleNative[3];
 unsigned scriptedCount,scriptedSource[32],scriptedNative[32];int scriptedParent[32];
 cod1ActorFaceNote_t notes[C1ANC14_MAX_NOTES];
} C1FaceFrame;
static C1FaceFrame cod1FaceFrames[COD1_ACTOR_CAPACITY];
#if defined(_MSC_VER) && defined(_M_IX86) && _M_IX86_FP >= 2
#include <emmintrin.h>
static struct {
 int valid;
 c1agt14Definition_t definitions[C1GA_COUNT];
} faceTopology;

/* Only the hierarchy can be reused. Every live phase, weight, rate and cycle
 * is checked each time. The fast path accepts nonnegative IEEE encodings in
 * the scalar validator's inclusive ranges; anything else uses that validator
 * with the caller's current FP environment, including negative zero. */
static int ActorFaceTreeValid(const c1atl14Node_t *nodes){
 const __m128i zero=_mm_setzero_si128();
 const __m128i firstNormal=_mm_set1_epi32(0x00800000);
 const __m128i lastNormal=_mm_setr_epi32(0x00800000,0x00800000,0,0);
 const __m128i firstMax=_mm_setr_epi32(0x3f800000,0x3f800000,0x7f7fffff,0x3f800000);
 const __m128i lastMax=_mm_setr_epi32(0x3f800000,0x42c80000,65535,65535);
 unsigned i;
 _Static_assert(sizeof(c1atl14Node_t)==32 && offsetof(c1atl14Node_t,oldCycle)==28 &&
   offsetof(c1atl14Node_t,rate)==20 && offsetof(c1atl14Node_t,goalWeight)==12,"animation node layout");
 if(!faceTopology.valid || memcmp(faceTopology.definitions,cod1GenericDefinitions,sizeof(faceTopology.definitions))){
  if(!C1AGT14_Valid(cod1GenericDefinitions,nodes,C1GA_COUNT))return 0;
  memcpy(faceTopology.definitions,cod1GenericDefinitions,sizeof(faceTopology.definitions));
  faceTopology.valid=1;return 1;
 }
 if(!nodes || ActorOverlap(nodes,C1GA_COUNT*sizeof(*nodes),cod1GenericDefinitions,sizeof(cod1GenericDefinitions)))return 0;
 for(i=0;i<C1GA_COUNT;i++){
  const __m128i first=_mm_loadu_si128((const __m128i *)&nodes[i].phase);
  const __m128i last=_mm_loadu_si128((const __m128i *)&nodes[i].currentWeight);
  const __m128i negative=_mm_cmpgt_epi32(zero,_mm_or_si128(first,last));
  const __m128i high=_mm_or_si128(_mm_cmpgt_epi32(first,firstMax),_mm_cmpgt_epi32(last,lastMax));
  /* Scalar comparisons on a subnormal may set the MXCSR denormal flag
   * or trap when unmasked. Run the original comparisons for these nodes. */
  const __m128i lowFirst=_mm_andnot_si128(_mm_cmpeq_epi32(first,zero),_mm_cmpgt_epi32(firstNormal,first));
  const __m128i lowLast=_mm_andnot_si128(_mm_cmpeq_epi32(last,zero),_mm_cmpgt_epi32(lastNormal,last));
  const __m128i exceptional=_mm_or_si128(_mm_or_si128(lowFirst,lowLast),_mm_or_si128(negative,high));
  if(_mm_movemask_epi8(exceptional))
   return C1AGT14_Valid(cod1GenericDefinitions,nodes,C1GA_COUNT);
 }
 return 1;
}
#else
#define ActorFaceTreeValid(nodes) C1AGT14_Valid(cod1GenericDefinitions,(nodes),C1GA_COUNT)
#endif

static void ActorFaceFrameReset(void){
#if defined(_MSC_VER) && defined(_M_IX86) && _M_IX86_FP >= 2
 faceTopology.valid=0;
#endif
 memset(cod1FaceFrames,0,sizeof(cod1FaceFrames));}
static void ActorFaceFrameRetire(uint32_t index,const GscEntityRef*owner){
 if(index<COD1_ACTOR_CAPACITY&&owner&&cod1FaceFrames[index].selected&&ActorRefEqual(&cod1FaceFrames[index].lease.entity,owner))memset(cod1FaceFrames+index,0,sizeof(C1FaceFrame));
}
static int ActorFaceClockSelected(unsigned index){return index<COD1_ACTOR_CAPACITY&&cod1FaceFrames[index].selected;}
static int ActorScriptedClockSelected(unsigned index){return ActorFaceClockSelected(index)&&cod1FaceFrames[index].scripted;}
static int ActorScriptedFrameAdvance(C1GARecord*,C1FaceFrame*,float);
static int MoodyIdleTree(const C1GARecord*);
static int MoodyNamedIdle(const C1GARecord*);
static int MoodyStopped(const C1GARecord*);
static int ActorFaceFrameClosure(const C1GARecord*);
/* Native actor field dispatch20041A6F/87 and generic type4 getter20041B73:
 * a live actor owns WORD+178 independently of collision/motion/publication.
 * A zero WORD pushes nothing (undefined), never an invented standing pose.
 * This wider read is explicitly limited to the selected current facial owner;
 * legacy constructor/motion admission and all setter predicates stay intact.
 * -1 means this profile is unselected, 0 an expired selected owner. */
static int ActorFacePoseRead(const GscEntityRef*ref,GscStringCastValue*out,char*error,size_t size){
 cod1ActorLease_t lease;const C1FaceFrame*f;const C1GARecord*r;const cod1ActorScriptStrings_t*s;GscStringCastValue value;
 if(!ref||!out||!ActorGoalLease(ref,&lease))return -1;
 f=cod1FaceFrames+lease.actorIndex;if(!f->selected)return -1;
 if(!ActorGoalBuffers(ref,sizeof(*ref),out,sizeof(*out),error,size)||ActorGenericPrivate(ref,sizeof(*ref))||ActorGenericPrivate(out,sizeof(*out))||ActorGenericPrivate(error,size))return 0;
 if(!cod1EntityReady||!ActorMapAvailable()||level.spawning||level.restarted||cod1ActorEngineSceneBusyEpoch==ref->epoch||!EntityValidate(NULL,ref)||memcmp(&f->lease,&lease,sizeof(lease)))return 0;
 r=cod1GenericActors+lease.actorIndex;
 if(!r->used||!r->notify||memcmp(&r->lease,&lease,sizeof(lease))||f->runtime!=r->runtime||f->catalog!=r->catalog||f->identity!=r->notify->ownerIdentity)return 0;
 s=&cod1Actors[lease.actorIndex].scriptStrings;memset(&value,0,sizeof(value));
 if(s->posePresent){if(!memchr(s->pose,0,sizeof(value.string)))return 0;value.type=1;strcpy(value.string,s->pose);}
 *out=value;return 1;
}
/* Definitions are checked independently of a presumed ordering of the compact
 * transport. The authored source ordinal is the authoritative lookup key. */
static int ActorFaceLeafIndex(unsigned ordinal){unsigned i;for(i=0;i<C1AFU14_NODES;i++)if(c1afu14Definitions[i].ordinal==ordinal&&c1afu14Definitions[i].leaf>=42)return c1afu14Definitions[i].leaf-42;return -1;}
static const c1ar14Root_t*ActorFaceRoot(unsigned ordinal){int face=ActorFaceLeafIndex(ordinal);return face>=0&&ActorFaceReady(cod1EntityEpoch)?cod1FaceResources.root[face]:NULL;}
static const c1ab14Clip_t*ActorFaceBody(unsigned ordinal){int face=ActorFaceLeafIndex(ordinal);return face>=0&&ActorFaceReady(cod1EntityEpoch)?cod1FaceResources.body[face]:NULL;}
static int ActorFaceFrameCanMutate(const C1GARecord*r,int requested,int commit){
 C1FaceFrame*f;if(!r||r->lease.actorIndex>=COD1_ACTOR_CAPACITY)return 0;f=cod1FaceFrames+r->lease.actorIndex;
 if(!f->selected&&!requested)return 1;
 if(!r->notify||!ActorFaceReady(cod1EntityEpoch)||cod1Actors[r->lease.actorIndex].turretMount.mounted)return 0;
 if(f->selected)return !f->poisoned&&!f->count&&f->tick==r->tick&&f->runtime==r->runtime&&f->catalog==r->catalog&&f->identity==r->notify->ownerIdentity&&!memcmp(&f->lease,&r->lease,sizeof(r->lease))&&(!f->scripted||ActorFaceFrameClosure(r));
 if(commit){memset(f,0,sizeof(*f));f->selected=1;f->tick=r->tick;f->runtime=r->runtime;f->catalog=r->catalog;f->identity=r->notify->ownerIdentity;f->lease=r->lease;}
 return 1;
}
static int ActorFaceMutate(C1GARecord*r,void*user){
 const GscActorAnimationRequest*q=user;unsigned n=q->animation[0].ordinal,i;int face=ActorFaceLeafIndex(n);const char*end;float f[3];const c1anm14Metadata_t*m;
 if(q->method!=GSC_ACTOR_ANIMATION_FLAGGED_RESTART||face<0||!r->notify||r->notify->serial==UINT64_MAX||!ActorGenericBinding(q->animation,q->runtime_identity)||!ActorFaceReady(cod1EntityEpoch)||!(end=memchr(q->notify,0,sizeof(q->notify)))||end==q->notify)return 0;
 memcpy(f,q->scalar_bits,sizeof(f));m=cod1FaceResources.metadata+face;
 if(!C1AFK14_Select(cod1GenericDefinitions,r->nodes,r->notify->flags,C1GA_COUNT,n,n+1,f[0],f[1],f[2],1,m->notes,m->count))return 0;
 for(i=0;i<C1GA_COUNT;i++)if(!r->notify->flags[i].token){memset(r->notify->names[i],0,64);r->notify->generation[i]=r->notify->pending[i]=0;r->notify->pendingTick[i]=0;}
 memset(r->notify->names[n],0,64);memcpy(r->notify->names[n],q->notify,(size_t)(end-q->notify));r->notify->pending[n]=0;r->notify->pendingTick[n]=0;r->notify->generation[n]=++r->notify->serial;return 1;
}
static int ActorFaceInvoke(void*unused,const GscActorAnimationRequest*input,char*error,size_t cap,int(*current)(void*),void*ctx){
 GscActorAnimationRequest q;(void)unused;
 if(!input||!ActorGoalBuffers(input,sizeof(*input),NULL,0,error,cap)||ActorGenericPrivate(input,sizeof(*input)))return 0;
 q=*input;if(q.method!=GSC_ACTOR_ANIMATION_FLAGGED_RESTART)return ActorGenericInvoke(NULL,input,error,cap,current,ctx);
 if(q.precision_bits!=53||q.rounding)return 0;
 return ActorGenericTransaction(&input->entity,q.runtime_identity,q.catalog_identity,q.now_ms,input,sizeof(*input),error,cap,current,ctx,ActorFaceMutate,&q);
}
const GscActorAnimationHost*G_Cod1ActorFacialAnimationHost(void){static const GscActorAnimationHost host={NULL,ActorGenericValidate,ActorFaceInvoke};return &host;}
extern int TrainingElderCurrent(const GscEntityRef*);
/* This placed runner has already entered its scripted clock. Until it has a
 * body selection, its one facial channel has the original R237 single-leaf
 * ordering. Keep the clock, lease, notetrack receipts and wire identity. */
static int ActorRunnerFaceOwner(const C1GARecord*r){
 const cod1ActorRecord_t *a;
 if(!r||r->lease.actorIndex>=COD1_ACTOR_CAPACITY||!TrainingElderCurrent(&r->lease.entity)||
    !ActorLeaseValid(&r->lease)||!ActorScriptedClockSelected(r->lease.actorIndex))return 0;
 a=cod1Actors+r->lease.actorIndex;
 return a->scriptedController.entered&&!a->scriptedController.invalid&&!a->scriptedController.selectionSerial&&
    !a->turretMount.mounted&&!a->turretMount.currentTop&&a->turretMount.currentStack[0]==4&&
    !ActorHasNormalRecord(r->lease.actorIndex)&&a->scriptedAux.record==FRESH_AUX_EMPTY&&
    a->scriptedAux.prefix==FRESH_PREFIX_UNUSED;
}
static int ActorFaceFrameClosure(const C1GARecord*r){
 unsigned i,p;int named=0,idleNamed=0,scripted=r&&r->lease.actorIndex<COD1_ACTOR_CAPACITY&&ActorScriptedClockSelected(r->lease.actorIndex);
 if(!r||!r->nodes||!r->notify||!ActorFaceReady(cod1EntityEpoch))return 0;
 int runnerFace=scripted&&ActorRunnerFaceOwner(r);
 if(runnerFace)scripted=0;
 for(i=0;i<C1GA_COUNT;i++){
  int face=-1,look,shuffle,extra;
  if(r->notify->pending[i])return 0;
  if(!r->notify->flags[i].token && (cod1GenericDefinitions[i].children ||
      (!r->nodes[i].currentWeight && !r->nodes[i].goalWeight)))continue;
  extra=scripted?ActorScriptedIndex(i):-1;
  /* The factory jump leaf belongs to the verified moving body bank. In
   * closure this is a membership test; the regular bank index stays local. */
  if(scripted&&i==892&&TrainingJumperAllocation(&r->lease.entity))extra=0;
  if(r->notify->flags[i].token){face=ActorFaceLeafIndex(i);
   if(runnerFace&&(face<0||strcmp(r->notify->names[i],"animscript faceanim")))return 0;
   /* The original idle expression replaces the ended facial leaf. A mixed
    * idle/gesture notify set needs a different multi-leaf order contract. */
   if(scripted&&face>=2&&face<5){
    if(!(cod1Actors[r->lease.actorIndex].scriptedController.resumed4&&!ActorHasNormalRecord(r->lease.actorIndex))&&
       !FoleyNormalIdleOwner(r) && !(MoodyStopped(r)&&MoodyNamedIdle(r)))return 0;
    idleNamed++;
   }
   if((scripted?(extra<0&&(face<2||face>=5)):face<0)||++named>(scripted?2:1)||r->notify->flags[i].token!=i+1||!r->notify->generation[i]||!r->notify->names[i][0])return 0;}
  if(cod1GenericDefinitions[i].children||(!r->nodes[i].currentWeight&&!r->nodes[i].goalWeight))continue;
  /* Resource classification is relevant only for a live leaf. The private
   * source hierarchy has 2078 nodes; do not rescan 40 names for every dormant
   * branch on every phase transaction. Pending/token guards still cover all. */
  if(face<0)face=ActorFaceLeafIndex(i);look=C1AL14_Index(cod1GenericNames[i].name);shuffle=ActorGenericShuffleIndex(cod1GenericNames[i].name);
  if((scripted?(extra<0&&(face<2||face>=5)):face<0)&&look<0&&shuffle<0)return 0;
  if(extra>=0&&!ActorScriptedReady(cod1EntityEpoch))return 0;
  if(look>=0&&(r->nodes[i].phase||r->nodes[i].oldPhase||r->nodes[i].cycle||r->nodes[i].oldCycle))return 0;
  for(p=cod1GenericDefinitions[i].parent;p;p=cod1GenericDefinitions[p].parent)if((cod1GenericDefinitions[p].flags&3)||r->nodes[p].rate!=1)return 0;
 }
 if(idleNamed){
  if(idleNamed!=1)return 0;
  if(named!=1){
   if(named!=2||(!FoleyNormalIdleOwner(r)&&!MoodyNamedIdle(r)))return 0;
   for(i=0;i<C1GA_COUNT;i++)if(r->notify->flags[i].token&&ActorFaceLeafIndex(i)<0&&!C1ASB14_Body(ActorScriptedIndex(i)))return 0;
  }
 }
 return ActorFaceTreeValid(r->nodes);
}
typedef struct {
 GscEntityRef owner;cod1ActorLease_t lease;C1GARecord saved,stage,baseline;
 C1FaceFrame frame;cod1ActorRecord_t*actor;gentity_t*entity;
 uint64_t epoch,faceRevision,scriptedRevision;int now;const GscEntityRef*borrowed;int(*current)(void*);void*ctx;
} C1FaceFrameCall;
static C1FaceFrameCall*cod1FaceFrameOperation;
static int ActorFaceFramePrivate(const void*p,size_t n){C1FaceFrameCall*c=cod1FaceFrameOperation;
#if defined(_MSC_VER) && defined(_M_IX86) && _M_IX86_FP >= 2
 if(ActorOverlap(p,n,&faceTopology,sizeof(faceTopology)))return 1;
#endif
 if(ActorOverlap(p,n,cod1FaceFrames,sizeof(cod1FaceFrames))||ActorOverlap(p,n,&cod1FaceFrameOperation,sizeof(cod1FaceFrameOperation)))return 1;
 return c&&(ActorOverlap(p,n,c,sizeof(*c))||ActorOverlap(p,n,c->actor,c->actor?sizeof(*c->actor):0)||ActorOverlap(p,n,c->entity,c->entity?sizeof(*c->entity):0)||ActorOverlap(p,n,c->baseline.nodes,c->baseline.nodes?C1GA_COUNT*sizeof(*c->baseline.nodes):0)||ActorOverlap(p,n,c->baseline.notify,c->baseline.notify?sizeof(*c->baseline.notify):0));
}
static int ActorFaceFrameFence(C1FaceFrameCall*c){
 cod1ActorLease_t after;C1GARecord*r=cod1GenericActors+c->lease.actorIndex;
 return !cod1GenericOperation.cancelled&&c->now==level.time&&c->epoch==cod1EntityEpoch&&c->epoch==cod1GenericEpoch&&cod1EntityReady&&!level.spawning&&!level.restarted&&ActorFaceReady(c->epoch)&&c->faceRevision==cod1FaceResources.revision&&c->scriptedRevision==cod1ScriptedResources.revision&&(!c->frame.scripted||ActorScriptedReady(c->epoch))&&
  !memcmp(c->borrowed,&c->owner,sizeof(c->owner))&&ActorGoalLease(&c->owner,&after)&&!memcmp(&c->lease,&after,sizeof(after))&&
  !memcmp(r,&c->saved,sizeof(c->saved))&&R242ExactATRBytesEqual(r->nodes,c->baseline.nodes,C1GA_COUNT*sizeof(*r->nodes))&&R242ExactATRBytesEqual(r->notify,c->baseline.notify,sizeof(*r->notify))&&
  !memcmp(cod1FaceFrames+c->lease.actorIndex,&c->frame,sizeof(c->frame))&&
  !memcmp(cod1Actors+c->lease.actorIndex,c->actor,sizeof(*c->actor))&&!memcmp(g_entities+c->owner.slot,c->entity,sizeof(*c->entity));
}
static C1FaceFrameCall*ActorFaceFrameAcquire(const GscEntityRef*input,int now,int(*current)(void*),void*ctx,int writable){
 C1FaceFrameCall*c=NULL;C1GARecord*r;
 if(!ActorControllerProviderEnter(C1CP_FRAME_ENTER,input,now,current,ctx))return NULL;
 if(!input||!current||now<0||now!=level.time||cod1GenericOperation.busy||!ActorGoalBuffers(input,sizeof(*input),NULL,0,NULL,0)||ActorGenericPrivate(input,sizeof(*input)))return NULL;
 c=calloc(1,sizeof(*c));if(!c)return NULL;c->owner=*input;c->borrowed=input;c->now=now;c->current=current;c->ctx=ctx;c->epoch=cod1EntityEpoch;c->faceRevision=cod1FaceResources.revision;c->scriptedRevision=cod1ScriptedResources.revision;
 if(!ActorGoalLease(&c->owner,&c->lease)||cod1Actors[c->lease.actorIndex].turretMount.mounted)goto rejected;
 r=cod1GenericActors+c->lease.actorIndex;if(!r->used||!r->runtime||!r->catalog||!r->notify||memcmp(&r->lease,&c->lease,sizeof(c->lease))||r->revision==UINT64_MAX||!ActorFaceFrameClosure(r))goto rejected;
 c->saved=*r;c->frame=cod1FaceFrames[c->lease.actorIndex];
 if(c->frame.poisoned||(c->frame.selected&&(memcmp(&c->frame.lease,&c->lease,sizeof(c->lease))||c->frame.runtime!=r->runtime||c->frame.catalog!=r->catalog||c->frame.identity!=r->notify->ownerIdentity)))goto rejected;
 c->actor=malloc(sizeof(*c->actor));c->entity=malloc(sizeof(*c->entity));
 if(!c->actor||!c->entity||(writable&&!ActorGenericClone(&c->stage,r))||!ActorGenericClone(&c->baseline,r))goto rejected;
 memcpy(c->actor,cod1Actors+c->lease.actorIndex,sizeof(*c->actor));memcpy(c->entity,g_entities+c->owner.slot,sizeof(*c->entity));
 cod1FaceFrameOperation=c;cod1GenericOperation.busy=1;cod1GenericOperation.cancelled=0;
 cod1GenericOperation.stage=c->stage.nodes;
 cod1GenericOperation.bytes=writable?C1GA_COUNT*sizeof(*c->stage.nodes):0;
 cod1GenericOperation.notifyStage=c->stage.notify;
 cod1GenericOperation.notifyBytes=writable?sizeof(*c->stage.notify):0;
 if(current(ctx)!=1||!ActorFaceFrameFence(c)){cod1FaceFrameOperation=NULL;cod1GenericOperation.busy=0;cod1GenericOperation.stage=NULL;cod1GenericOperation.bytes=0;cod1GenericOperation.notifyStage=NULL;cod1GenericOperation.notifyBytes=0;goto rejected;}
 return c;
 rejected:if(c){ActorGenericFreeOwned(&c->stage);ActorGenericFreeOwned(&c->baseline);free(c->actor);free(c->entity);free(c);}return NULL;
}
static C1FaceFrameCall*ActorFaceFrameEnter(const GscEntityRef*input,int now,int(*current)(void*),void*ctx){
 return ActorFaceFrameAcquire(input,now,current,ctx,1);
}
static void ActorFaceFrameLeave(C1FaceFrameCall*c){ActorGenericFreeOwned(&c->stage);ActorGenericFreeOwned(&c->baseline);free(c->actor);free(c->entity);free(c);cod1FaceFrameOperation=NULL;cod1GenericOperation.stage=NULL;cod1GenericOperation.bytes=0;cod1GenericOperation.notifyStage=NULL;cod1GenericOperation.notifyBytes=0;cod1GenericOperation.busy=0;}
static int ActorFaceFrameCommit(C1FaceFrameCall*c,const C1FaceFrame*next){
 C1GARecord*r=cod1GenericActors+c->lease.actorIndex;
 if(c->current(c->ctx)!=1||!ActorFaceFrameFence(c)||!ActorFaceFrameClosure(&c->stage))return 0;
 c->stage.revision++;ActorGenericFreeOwned(r);*r=c->stage;c->stage.nodes=NULL;c->stage.notify=NULL;cod1FaceFrames[c->lease.actorIndex]=*next;return 1;
}
int G_Cod1ActorFaceClockSelect(const GscEntityRef*input,int now,int(*current)(void*),void*ctx){
 C1FaceFrameCall*c=ActorFaceFrameEnter(input,now,current,ctx);C1FaceFrame next;int ok=0;if(!c)return 0;
 if(c->frame.selected||c->saved.tick!=now)goto done;memset(&next,0,sizeof(next));next.selected=1;next.tick=now;next.runtime=c->saved.runtime;next.catalog=c->saved.catalog;next.identity=c->saved.notify->ownerIdentity;next.lease=c->lease;ok=ActorFaceFrameCommit(c,&next);
 done:ActorFaceFrameLeave(c);return ok;
}
/* Native 2001F34F and 2001F400 pass the same float 0.05 for each dispatched
 * body. SV_SpawnServer advances host time by 100 between settling frames.
 * A missed GAME frame, stale Session generation, or other cadence is held. */
static int ActorFrameCadence(const C1FaceFrame *frame,int now,uint64_t generation){
 if(!frame->scripted)return frame->tick<=INT_MAX-50&&now==frame->tick+50;
 if(now!=level.time||level.previousTime!=frame->tick||frame->gameFrame<0||
    frame->gameFrame==INT_MAX||level.framenum!=frame->gameFrame+1||
    (frame->frame&&(frame->frame==UINT64_MAX||generation!=frame->frame+1)))return 0;
 const int64_t delta=(int64_t)now-frame->tick;
 /* At map handoff the last settling call increments svs.time by 100;
  * SV_Frame increments it by another frameMsec before GAME_RUN_FRAME. */
 if(delta==150)G_Printf("[Foley cadence] actual adjacent GAME frames %d..%d times %d..%d generation=%llu\n",frame->gameFrame,level.framenum,frame->tick,now,(unsigned long long)generation);
 return delta==50||delta==100||delta==150;
}
int G_Cod1ActorFaceFrameBegin(const GscEntityRef*input,int now,uint64_t generation,int(*current)(void*),void*ctx){
 C1FaceFrameCall*c=ActorFaceFrameEnter(input,now,current,ctx);C1FaceFrame next;unsigned i;float dt;int ok=0;if(!c)return 0;
 if(!c->frame.selected||c->frame.open||c->frame.count||!generation||generation<=c->frame.frame||c->saved.tick!=c->frame.tick||!ActorFrameCadence(&c->frame,now,generation)||!ActorGenericDt(50,&dt))goto done;
 next=c->frame;next.open=1;next.done=0;next.tick=now;next.frame=generation;if(next.scripted)next.gameFrame=level.framenum;
 for(i=0;i<C1GA_COUNT;i++){c->stage.nodes[i].oldPhase=c->stage.nodes[i].phase;c->stage.nodes[i].oldCycle=c->stage.nodes[i].cycle;if(!C1ATL14_AdvanceWeight(c->stage.nodes+i,dt))goto done;}
 c->stage.tick=now;ok=ActorFaceFrameCommit(c,&next);
 done:ActorFaceFrameLeave(c);return ok;
}
static int ActorFaceFrameAdvanceNode(C1GARecord*r,unsigned ordinal,float dt,int notify,c1anc14Step_t*out){
 int face=ActorFaceLeafIndex(ordinal),shuffle=ActorGenericShuffleIndex(cod1GenericNames[ordinal].name);const c1anm14Metadata_t*m;unsigned event;
 if(face>=0){m=cod1FaceResources.metadata+face;return m->loop?C1ANC14_AdvanceLoopEnd(r->nodes+ordinal,r->notify->flags+ordinal,m->notes,m->frequency,dt,1,notify,out):C1ANC14_Advance(r->nodes+ordinal,r->notify->flags+ordinal,m->notes,m->count,m->frequency,0,dt,1,notify,out);}
 if(shuffle>=0){const c1ar14Root_t*root=cod1GenericUnmountedBank.root[shuffle];c1afl14State_t flags=r->notify->flags[ordinal];float frequency;
  if(flags.token||root->noteCount)return 0;
  /* Precomputed validated resource frequency, same PC53 checkpoint as loader. */
  {c1anm14Metadata_t metadata;if(!C1ANM14_Build(root,&metadata))return 0;frequency=metadata.frequency;}
  return C1AFC14_Advance(r->nodes+ordinal,&flags,frequency,root->flags&1,dt,1,0,&event);
 }
 return C1AL14_Index(cod1GenericNames[ordinal].name)>=0;
}
int G_Cod1ActorFaceFrameStep(const GscEntityRef*input,int now,uint64_t generation,int*resume,int(*current)(void*),void*ctx){
 C1FaceFrameCall*c;C1FaceFrame next;c1anc14Step_t step,other;unsigned i,j,lead=0;float dt;int ok=0;
 if(!resume||!ActorGoalBuffers(input,input?sizeof(*input):0,resume,sizeof(*resume),NULL,0)||ActorGenericPrivate(resume,sizeof(*resume))||ActorOverlap(input,input?sizeof(*input):0,resume,sizeof(*resume)))return 0;
 c=ActorFaceFrameEnter(input,now,current,ctx);if(!c)return 0;
 if(!c->frame.selected||!c->frame.open||c->frame.done||c->frame.count||!generation||generation!=c->frame.frame||c->saved.tick!=now||!ActorGenericDt(50,&dt))goto done;
 next=c->frame;memset(&step,0,sizeof(step));step.usedDt=dt;
 if(c->frame.scripted&&!ActorRunnerFaceOwner(&c->stage)){
  if(!ActorScriptedFrameAdvance(&c->stage,&next,dt))goto done;
  if(!ActorFaceFrameCommit(c,&next))goto done;*resume=next.done?0:1;ok=1;goto done;
 }
 for(i=1;i<C1GA_COUNT;i++)if(c->stage.notify->flags[i].token){if(lead||ActorFaceLeafIndex(i)<0)goto done;lead=i;}
 if(lead&&!ActorFaceFrameAdvanceNode(&c->stage,lead,dt,1,&step))goto done;
 for(i=1;i<C1GA_COUNT;i++)if(i!=lead&&!cod1GenericDefinitions[i].children&&(c->stage.nodes[i].currentWeight||c->stage.nodes[i].goalWeight))if(!ActorFaceFrameAdvanceNode(&c->stage,i,step.usedDt,0,&other))goto done;
 if(step.count){const c1anm14Metadata_t*m=cod1FaceResources.metadata+ActorFaceLeafIndex(lead);
  for(j=0;j<step.count;j++){cod1ActorFaceNote_t*q=next.notes+j;memset(q,0,sizeof(*q));q->entity=c->owner;q->runtime=c->saved.runtime;q->catalog=c->saved.catalog;q->owner_identity=c->saved.notify->ownerIdentity;q->generation=c->saved.notify->generation[lead];q->frame_generation=generation;q->ordinal=lead;q->produced_ms=now;memcpy(q->event,c->saved.notify->names[lead],64);if(!C1ANM14_Resolve(m,step.payload[j],q->payload))goto done;}
 }
 next.count=step.count;next.done=step.resume?0:1;if(next.done&&!next.count)next.open=0;
 if(!ActorFaceFrameCommit(c,&next))goto done;*resume=(int)step.resume;ok=1;
 done:ActorFaceFrameLeave(c);return ok;
}
static void ActorFaceFramePoison(C1FaceFrameCall*c){C1FaceFrame*f=cod1FaceFrames+c->lease.actorIndex;
 /* An external recipient may already have accepted an effect. Never retry it
  * after an invalid callback, and never poison a newly allocated actor owner. */
 if(f->selected&&f->identity==c->frame.identity&&f->frame==c->frame.frame&&f->runtime==c->frame.runtime&&f->catalog==c->frame.catalog&&!memcmp(&f->lease,&c->lease,sizeof(c->lease))){f->poisoned=1;f->open=f->count=f->nextNote=0;memset(f->notes,0,sizeof(f->notes));}
}
int G_Cod1ActorFaceFrameDrain(const GscEntityRef*input,int now,uint64_t generation,int(*current)(void*),cod1ActorFaceNoteCallback deliver,void*ctx){
 C1FaceFrameCall*c;C1FaceFrame next;cod1ActorFaceNote_t packet,copy;unsigned i;int accepted,ok=0;
 if(!deliver)return 0;c=ActorFaceFrameEnter(input,now,current,ctx);if(!c)return 0;
 if(!c->frame.selected||!c->frame.open||!generation||generation!=c->frame.frame||c->saved.tick!=now)goto done;
 next=c->frame;
 for(i=next.nextNote;i<next.count;i++){packet=next.notes[i];copy=packet;
  if(current(ctx)!=1||!ActorFaceFrameFence(c))goto done;
  accepted=deliver(ctx,&packet);
  if(accepted!=1){if(memcmp(&packet,&copy,sizeof(packet))||!ActorFaceFrameFence(c))ActorFaceFramePoison(c);goto done;}
  if(memcmp(&packet,&copy,sizeof(packet))||!ActorFaceFrameFence(c)){ActorFaceFramePoison(c);goto done;}
  /* Accepted receipts are consumed immediately. A later external refusal must
   * never cause this already delivered notification to be delivered twice. */
  memset(cod1FaceFrames[c->lease.actorIndex].notes+i,0,sizeof(packet));c->frame.notes[i]=cod1FaceFrames[c->lease.actorIndex].notes[i];cod1FaceFrames[c->lease.actorIndex].nextNote=i+1;c->frame.nextNote=i+1;
 }
 memset(next.notes,0,sizeof(next.notes));next.count=next.nextNote=0;if(next.done)next.open=0;
 ok=ActorFaceFrameCommit(c,&next);
 done:ActorFaceFrameLeave(c);return ok;
}
