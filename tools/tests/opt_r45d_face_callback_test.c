/* Actual FaceFrame fence/acquire/commit/drain extracted by the driver.
 * Synthetic GAME state, lease, closure and provider admission; no GSC VM. */
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define C1GA_COUNT 2078u
#define C1CP_FRAME_ENTER 3
#define COUNT 2

typedef struct {uint64_t epoch,generation;unsigned slot,reserved;} GscEntityRef;
typedef struct {GscEntityRef entity;uint64_t serial;unsigned actorIndex,sentientIndex;} cod1ActorLease_t;
typedef struct {float phase,oldPhase,remaining,goalWeight,currentWeight,rate;unsigned cycle,oldCycle;} c1atl14Node_t;
typedef struct {uint16_t descendant;int16_t cursor;uint16_t token,clientType;} c1afl14State_t;
typedef struct {c1afl14State_t flags[C1GA_COUNT];unsigned nameCount;uint64_t generation[C1GA_COUNT],pending[C1GA_COUNT],serial,ownerIdentity;int32_t pendingTick[C1GA_COUNT];} C1GANotifyState;
typedef struct {unsigned ordinal;char text[64];} C1GANameEntry;
typedef struct {int used,tick,detachedMG,mgTick;cod1ActorLease_t lease;uint64_t runtime,catalog,revision;c1atl14Node_t *nodes;C1GANotifyState *notify;C1GANameEntry *names;uint32_t lookCells[16];} C1GARecord;
typedef struct {unsigned ordinal;char event[64];} cod1ActorFaceNote_t;
typedef int (*cod1ActorFaceNoteCallback)(void*,cod1ActorFaceNote_t*);
typedef struct {int selected,open,done,tick,poisoned,scripted,gameFrame;uint64_t runtime,catalog,identity,frame;cod1ActorLease_t lease;unsigned count,nextNote;cod1ActorFaceNote_t notes[COUNT];} C1FaceFrame;
typedef struct {struct {int mounted;} turretMount;unsigned value;} cod1ActorRecord_t;
typedef struct {unsigned value;} gentity_t;
typedef struct {GscEntityRef owner;cod1ActorLease_t lease;C1GARecord saved,stage,baseline;C1FaceFrame frame;cod1ActorRecord_t*actor;gentity_t*entity;uint64_t epoch,faceRevision,scriptedRevision;int now;const GscEntityRef*borrowed;int(*current)(void*);void*ctx;} C1FaceFrameCall;
static C1GARecord cod1GenericActors[1];
static C1FaceFrame cod1FaceFrames[1];
static cod1ActorRecord_t cod1Actors[1];
static gentity_t g_entities[1];
static struct {int busy,cancelled;void *stage;size_t bytes;void *notifyStage;size_t notifyBytes;void *nameStage;size_t nameBytes;} cod1GenericOperation;
static C1FaceFrameCall *cod1FaceFrameOperation;
static struct {int time,spawning,restarted;} level;
static struct {uint64_t revision;} cod1FaceResources,cod1ScriptedResources;
static uint64_t cod1EntityEpoch,cod1GenericEpoch;
static int cod1EntityReady,leaseOK,faceOK,scriptedOK,closureOK,providerOK;
static unsigned allocations,live,failAt,providerCalls,callbackCalls,deliveryCalls,accepted[COUNT];
static GscEntityRef input;
static void *tmalloc(size_t n){void*p;++allocations;if(allocations==failAt)return NULL;p=malloc(n);assert(p);++live;memset(p,0xab,n);return p;}
static void tfree(void*p){if(p){assert(live);--live;free(p);}}
static int ActorOverlap(const void*p,size_t n,const void*q,size_t m){uintptr_t a=(uintptr_t)p,b=(uintptr_t)q;return n&&m&&p&&q&&a<=UINTPTR_MAX-n&&b<=UINTPTR_MAX-m&&a<b+m&&b<a+n;}
#define malloc tmalloc
#define free tfree
#ifdef REFERENCE_R40
#include "reference.inc"
#include "g_cod1_face_frame_storage35.h"
#else
#include "g_cod1_actor_clone_storage45.h"
#include "g_cod1_face_frame_workspace45.h"
#endif
#undef malloc
#undef free
static int ActorGenericPrivate(const void*p,size_t n){
#ifdef REFERENCE_R40
 return cod1FaceFrameOperation&&ActorOverlap(p,n,cod1FaceFrameOperation,sizeof(C1FaceFrameAllocation));
#else
 return ActorFaceFrameWorkspacePrivate(p,n);
#endif
}
static int ActorGoalBuffers(const void*p,size_t n,void*out,size_t bytes,char*error,size_t cap){(void)error;(void)cap;return !ActorOverlap(p,n,out,bytes)&&!ActorGenericPrivate(p,n)&&!ActorGenericPrivate(out,bytes);}
static int ActorControllerProviderEnter(int kind,const GscEntityRef*owner,int now,int(*current)(void*),void*ctx){(void)kind;(void)owner;(void)now;(void)current;(void)ctx;++providerCalls;return providerOK;}
static int ActorGoalLease(const GscEntityRef*owner,cod1ActorLease_t*out){if(!leaseOK||memcmp(owner,&cod1GenericActors[0].lease.entity,sizeof(*owner)))return 0;*out=cod1GenericActors[0].lease;return 1;}
static int ActorFaceReady(uint64_t e){return faceOK&&e==cod1EntityEpoch;}
static int ActorScriptedReady(uint64_t e){return scriptedOK&&e==cod1EntityEpoch;}
static int ActorFaceFrameClosure(const C1GARecord*r){return closureOK&&r->nodes&&r->notify&&r->names;}
static int R242ExactATRBytesEqual(const void*a,const void*b,size_t n){return a&&b&&!memcmp(a,b,n);}
static int ActorGenericNamesEqual(const C1GARecord*a,const C1GARecord*b){return a&&b&&a->notify&&b->notify&&a->names&&b->names&&a->notify->nameCount<=C1GA_COUNT&&a->notify->nameCount==b->notify->nameCount&&R242ExactATRBytesEqual(a->names,b->names,(size_t)a->notify->nameCount*sizeof(*a->names));}
#include "face_core.inc"
#include "face_drain.inc"
static void clearIdle(void){ActorClonePoolClear();
#ifndef REFERENCE_R40
 ActorFaceFrameWorkspaceClear();
#endif
}
static void idle(void){assert(!cod1GenericOperation.busy&&!cod1FaceFrameOperation&&!cod1GenericOperation.stage&&!cod1GenericOperation.notifyStage&&!cod1GenericOperation.nameStage);}
static void destroy(void){idle();ActorGenericFreeOwned(cod1GenericActors);clearIdle();assert(!live);}
static void fresh(unsigned generation){C1GARecord*r=cod1GenericActors;C1FaceFrame*f=cod1FaceFrames;
 memset(r,0,sizeof(*r));memset(f,0,sizeof(*f));
#ifdef REFERENCE_R40
 r->nodes=tmalloc(C1GA_COUNT*sizeof(*r->nodes));r->notify=tmalloc(sizeof(*r->notify));r->names=tmalloc(C1GA_COUNT*sizeof(*r->names));assert(r->nodes&&r->notify&&r->names);memset(r->nodes,0,C1GA_COUNT*sizeof(*r->nodes));memset(r->notify,0,sizeof(*r->notify));
#else
 assert(ActorGenericAllocateBlock45(r,1));
#endif
 r->used=1;r->tick=100;r->revision=10;r->runtime=5;r->catalog=6;r->lease.entity.epoch=cod1EntityEpoch;r->lease.entity.generation=generation;r->lease.serial=generation;r->notify->ownerIdentity=generation;r->notify->nameCount=1;r->names[0].ordinal=3;memset(r->names[0].text,0,64);strcpy(r->names[0].text,"note");
 f->selected=f->open=f->done=f->scripted=1;f->count=COUNT;f->tick=100;f->frame=9;f->runtime=r->runtime;f->catalog=r->catalog;f->identity=r->notify->ownerIdentity;f->lease=r->lease;f->notes[0].ordinal=1;f->notes[1].ordinal=2;
}
static void setup(void){assert(!live);memset(&cod1GenericOperation,0,sizeof(cod1GenericOperation));memset(&level,0,sizeof(level));memset(cod1Actors,0,sizeof(cod1Actors));memset(g_entities,0,sizeof(g_entities));level.time=100;cod1EntityEpoch=cod1GenericEpoch=1;cod1FaceResources.revision=cod1ScriptedResources.revision=1;cod1EntityReady=leaseOK=faceOK=scriptedOK=closureOK=providerOK=1;failAt=0;providerCalls=callbackCalls=deliveryCalls=0;memset(accepted,0,sizeof(accepted));fresh(1);input=cod1GenericActors[0].lease.entity;}
enum Action { NONE,REFUSE,NODE,NOTIFY,NAME,FRAME,ACTOR,ENTITY,BORROWED,LEASE,REVISION,FACE_REV,SCRIPT_REV,EPOCH,TIME,SPAWN,RESTART,FACE_READY,SCRIPT_READY,CANCEL,RESET,REPLACE,REENTER,CLOSURE };
typedef struct {enum Action action;unsigned on;unsigned refuseDelivery,editPacket;enum Action deliveryAction;} Control;
static int current(void*ctx);
static int act(enum Action a,void*ctx){C1GARecord*r=cod1GenericActors;switch(a){
 case NONE:return 1;case REFUSE:return 0;case NODE:r->nodes[17].cycle++;break;case NOTIFY:r->notify->pending[23]++;break;case NAME:r->names[0].text[0]^=1;break;case FRAME:cod1FaceFrames[0].gameFrame++;break;case ACTOR:cod1Actors[0].value++;break;case ENTITY:g_entities[0].value++;break;case BORROWED:input.generation++;break;case LEASE:leaseOK=0;break;case REVISION:r->revision++;break;case FACE_REV:cod1FaceResources.revision++;break;case SCRIPT_REV:cod1ScriptedResources.revision++;break;case EPOCH:cod1GenericEpoch++;break;case TIME:level.time++;break;case SPAWN:level.spawning=1;break;case RESTART:level.restarted=1;break;case FACE_READY:faceOK=0;break;case SCRIPT_READY:scriptedOK=0;break;case CANCEL:cod1GenericOperation.cancelled=1;break;case RESET:case REPLACE:cod1GenericOperation.cancelled=1;ActorGenericFreeOwned(r);memset(r,0,sizeof(*r));memset(cod1FaceFrames,0,sizeof(cod1FaceFrames));clearIdle();if(a==REPLACE){++cod1FaceResources.revision;fresh(2);}break;case REENTER:assert(!ActorFaceFrameAcquire(&input,100,current,ctx,1));break;case CLOSURE:closureOK=0;break;
 }return 1;}
static int current(void*ctx){Control*c=ctx;++callbackCalls;return callbackCalls==c->on?act(c->action,ctx):1;}
static int deliver(void*ctx,cod1ActorFaceNote_t*p){Control*c=ctx;unsigned ordinal=p->ordinal;assert(ordinal>=1&&ordinal<=COUNT);++deliveryCalls;if(c->editPacket==deliveryCalls)p->event[0]^=1;if(c->deliveryAction&&deliveryCalls==1)(void)act(c->deliveryAction,ctx);if(c->refuseDelivery==deliveryCalls)return 0;++accepted[ordinal-1];return 1;}
int main(void){unsigned a,w,checks=0;C1FaceFrameCall*c;C1FaceFrame next;Control q;
 /* Actual entry and fence under every independent mutation, read and write. */
 for(w=0;w<=1;w++)for(a=REFUSE;a<=REPLACE;a++){
  setup();q=(Control){(enum Action)a,1,0,0,NONE};assert(!ActorFaceFrameAcquire(&input,100,current,&q,(int)w));assert(callbackCalls==1&&providerCalls==1);idle();destroy();checks++;
 }
 /* Successful entry followed by mutation at actual Commit callback. */
 for(a=REFUSE;a<=REPLACE;a++){
  setup();q=(Control){(enum Action)a,2,0,0,NONE};c=ActorFaceFrameEnter(&input,100,current,&q);assert(c);next=c->frame;c->stage.nodes[5].cycle=99;assert(!ActorFaceFrameCommit(c,&next));if(a!=RESET&&a!=REPLACE)assert(cod1GenericActors[0].nodes[5].cycle==0);ActorFaceFrameLeave(c);destroy();checks++;
 }
 setup();q=(Control){REENTER,1,0,0,NONE};c=ActorFaceFrameEnter(&input,100,current,&q);assert(c&&providerCalls==2&&callbackCalls==1);next=c->frame;c->stage.nodes[5].cycle=99;assert(ActorFaceFrameCommit(c,&next));assert(cod1GenericActors[0].nodes[5].cycle==99&&cod1GenericActors[0].revision==11);ActorFaceFrameLeave(c);destroy();checks++;
 setup();q=(Control){CLOSURE,2,0,0,NONE};c=ActorFaceFrameEnter(&input,100,current,&q);assert(c);next=c->frame;assert(!ActorFaceFrameCommit(c,&next));ActorFaceFrameLeave(c);destroy();checks++;
 setup();q=(Control){0};providerOK=0;assert(!ActorFaceFrameEnter(&input,100,current,&q)&&providerCalls==1&&!callbackCalls);destroy();checks++;
 setup();q=(Control){0};failAt=allocations+1;assert(!ActorFaceFrameEnter(&input,100,current,&q)&&!callbackCalls);destroy();checks++;
 setup();q=(Control){0};c=ActorFaceFrameAcquire(&input,100,current,&q,0);assert(c&&!c->stage.nodes&&c->baseline.nodes);ActorFaceFrameLeave(c);destroy();checks++;
 /* Real receipt loop: second delivery or current refusal must not repeat first. */
 for(a=0;a<2;a++){
  setup();q=(Control){a?REFUSE:NONE,a?3:0,a?0:2,0,NONE};assert(!G_Cod1ActorFaceFrameDrain(&input,100,9,current,deliver,&q));assert(accepted[0]==1&&!accepted[1]&&cod1FaceFrames[0].nextNote==1&&!cod1FaceFrames[0].poisoned);q=(Control){0};assert(G_Cod1ActorFaceFrameDrain(&input,100,9,current,deliver,&q));assert(accepted[0]==1&&accepted[1]==1&&!cod1FaceFrames[0].count&&!cod1FaceFrames[0].open);destroy();checks++;
 }
 setup();q=(Control){REFUSE,4,0,0,NONE};assert(!G_Cod1ActorFaceFrameDrain(&input,100,9,current,deliver,&q));assert(accepted[0]==1&&accepted[1]==1&&cod1FaceFrames[0].nextNote==2);q=(Control){0};assert(G_Cod1ActorFaceFrameDrain(&input,100,9,current,deliver,&q)&&deliveryCalls==2);destroy();checks++;
 for(a=0;a<2;a++){
  setup();q=(Control){NONE,0,a?1:0,1,NONE};assert(!G_Cod1ActorFaceFrameDrain(&input,100,9,current,deliver,&q));assert(cod1FaceFrames[0].poisoned&&!cod1FaceFrames[0].open&&!cod1FaceFrames[0].count);q=(Control){0};assert(!G_Cod1ActorFaceFrameDrain(&input,100,9,current,deliver,&q)&&deliveryCalls==1);destroy();checks++;
 }
 setup();q=(Control){NONE,0,0,0,NODE};assert(!G_Cod1ActorFaceFrameDrain(&input,100,9,current,deliver,&q)&&cod1FaceFrames[0].poisoned);destroy();checks++;
 setup();q=(Control){NONE,0,0,0,REPLACE};assert(!G_Cod1ActorFaceFrameDrain(&input,100,9,current,deliver,&q));assert(cod1FaceFrames[0].identity==2&&!cod1FaceFrames[0].poisoned);destroy();checks++;
 setup();q=(Control){0};assert(!G_Cod1ActorFaceFrameDrain(&input,100,8,current,deliver,&q)&&!deliveryCalls);destroy();checks++;
 printf("R45D1: %u callback/receipt scenarios PASS\n",checks);return 0;
}
