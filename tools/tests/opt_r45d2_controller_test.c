/* Actual controller functions extracted from the pinned GAME sources.
 * Synthetic actors, callbacks and resource predicates; no actual VM. */
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define C1GA_COUNT 2078u
#define COUNT 2
#define C1CP_NONE 0
#define C1CP_CLOCK_SELECT 1
#define C1CP_RECORD_UPDATE 2
#define C1CP_FRAME_ENTER 3
#define C1CTRL_INIT 1
#define C1CTRL_UPDATE 3
typedef struct {uint64_t epoch,generation;unsigned slot,reserved;} GscEntityRef;
typedef struct {GscEntityRef entity;uint64_t serial;unsigned actorIndex,sentientIndex;} cod1ActorLease_t;
typedef struct {float phase,oldPhase,remaining,goalWeight,currentWeight,rate;unsigned cycle,oldCycle;} c1atl14Node_t;
typedef struct {uint16_t descendant;int16_t cursor;uint16_t token,clientType;} c1afl14State_t;
typedef struct {c1afl14State_t flags[C1GA_COUNT];unsigned nameCount;uint64_t generation[C1GA_COUNT],pending[C1GA_COUNT],serial,ownerIdentity;int32_t pendingTick[C1GA_COUNT];} C1GANotifyState;
typedef struct {unsigned ordinal;char text[64];} C1GANameEntry;
typedef struct {int used,tick,detachedMG,mgTick;cod1ActorLease_t lease;uint64_t runtime,catalog,revision;c1atl14Node_t *nodes;C1GANotifyState *notify;C1GANameEntry *names;uint32_t lookCells[16];} C1GARecord;
typedef struct {unsigned value;} C1FaceFrame;
typedef struct {unsigned value;} C1NormalRecord;
typedef struct {unsigned value;} cod1EntityNativeStaticRecord_t;
typedef struct {int initialized,invalid;uint64_t actorSerial,revision;} cod1ActorScriptedController_t;
typedef struct {cod1ActorScriptedController_t scriptedController;unsigned value;} cod1ActorRecord_t;
typedef struct {unsigned value;} gentity_t;
typedef struct {GscEntityRef entity;uint64_t runtime_identity,catalog_identity;} Request;
typedef union {Request init,step;} C1ControllerRequest;
typedef struct {
 int busy,cancelled,checking,kind,now;uint64_t epoch,resourceRevision,faceResourceRevision;
 C1GARecord genericHeaderBefore,genericDeepBefore;C1FaceFrame frameBefore;
 C1NormalRecord *recordPointerBefore;C1NormalRecord recordBefore;
 cod1EntityNativeStaticRecord_t nativeStaticBefore;int nativeStaticPresent;
 struct C1ControllerExpected *expected;
 cod1ActorLease_t lease;const void *input;size_t inputBytes;C1ControllerRequest q;
 cod1ActorRecord_t *before,*stage;gentity_t *entity;
 int(*current)(void*);void*context;
} C1ControllerOperation;
static C1ControllerOperation cod1ControllerOperation;
typedef struct C1ControllerExpected {
 C1GARecord generic;C1FaceFrame frame;
 C1NormalRecord *recordPointer;C1NormalRecord record;
 cod1EntityNativeStaticRecord_t nativeStatic;int nativeStaticPresent;
 cod1ActorRecord_t actor;gentity_t entity;
 int committed,present;
} C1ControllerExpected;
typedef struct {int pending,parent,entered,frames;GscEntityRef owner;int now;int(*current)(void*);void*context;} C1ControllerProviderGate;
static C1ControllerProviderGate cod1ControllerProviderGate;
static C1GARecord cod1GenericActors[1];static C1FaceFrame cod1FaceFrames[1];static cod1ActorRecord_t cod1Actors[1],before,stage;static gentity_t g_entities[1],entityBefore;
static C1NormalRecord normal,other,*cod1NormalRecords[1];static cod1EntityNativeStaticRecord_t nativeStatic;
static struct {int busy;} cod1GenericOperation,cod1NormalUpdate,cod1NormalOperation;
static void *cod1FaceFrameOperation;
static struct {int time,spawning,restarted;} level;
static struct {uint64_t revision;} cod1ScriptedResources,cod1FaceResources;
static uint64_t cod1EntityEpoch,cod1GenericEpoch;
static int cod1EntityReady,leaseOK,certificateOK,scriptOK,faceOK,staticPresent,animAllowed;
static unsigned live,allocs,failAt,calls,animationCalls,checks;
static Request request;static GscEntityRef owner;
static void *tmalloc(size_t n){void*p;++allocs;if(allocs==failAt)return NULL;p=malloc(n);assert(p);++live;memset(p,0xab,n);return p;}
static void *tcalloc(size_t n,size_t s){void*p=tmalloc(n*s);if(p)memset(p,0,n*s);return p;}
static void tfree(void*p){if(p){assert(live);--live;free(p);}}
#define malloc tmalloc
#define calloc tcalloc
#define free tfree
#ifdef REFERENCE_R40
#include "reference.inc"
#else
#include "g_cod1_actor_clone_storage45.h"
#endif
static int ActorRefEqual(const GscEntityRef*a,const GscEntityRef*b){return a&&b&&!memcmp(a,b,sizeof(*a));}
static int ActorControllerAnimationFrameEnter(const GscEntityRef*r,int now,int(*cb)(void*),void*ctx){(void)r;(void)now;(void)cb;(void)ctx;++animationCalls;return animAllowed;}
static int ActorControllerAnimationMutationAllowed(void){++animationCalls;return animAllowed;}
static int ActorControllerAnimationGenericAllowed(void){++animationCalls;return animAllowed;}
static int ActorGoalLease(const GscEntityRef*r,cod1ActorLease_t*l){if(!leaseOK||!ActorRefEqual(r,&cod1GenericActors[0].lease.entity))return 0;*l=cod1GenericActors[0].lease;return 1;}
static int ActorInheritedCertificate(const GscEntityRef*r,uint64_t serial){return certificateOK&&ActorRefEqual(r,&owner)&&serial==9;}
static int ActorScriptedReady(uint64_t e){return scriptOK&&e==cod1EntityEpoch;}
static int ActorFaceReady(uint64_t e){return faceOK&&e==cod1EntityEpoch;}
static const cod1EntityNativeStaticRecord_t *NativeStaticFind(const GscEntityRef*r){return staticPresent&&ActorRefEqual(r,&owner)?&nativeStatic:NULL;}
static int R242ExactATRBytesEqual(const void*a,const void*b,size_t n){return a&&b&&!memcmp(a,b,n);}
static int ActorGenericNamesEqual(const C1GARecord*a,const C1GARecord*b){return a&&b&&a->notify&&b->notify&&a->names&&b->names&&a->notify->nameCount<=C1GA_COUNT&&a->notify->nameCount==b->notify->nameCount&&!memcmp(a->names,b->names,(size_t)a->notify->nameCount*sizeof(*a->names));}
static void ActorControllerExpectedFree(C1ControllerOperation*o){if(o->expected){ActorGenericFreeOwned(&o->expected->generic);free(o->expected);o->expected=NULL;}}
#include "gate.inc"
#include "scope.inc"
#include "expected.inc"
#undef malloc
#undef calloc
#undef free
static C1ControllerOperation *o=&cod1ControllerOperation;
static int parentCallback(void*ctx);
static int childCallback(void*ctx){return ctx!=NULL;}
static int childContext;
static void setup(void){C1GARecord*g=cod1GenericActors;assert(!live);memset(o,0,sizeof(*o));ActorControllerProviderReset();memset(g,0,sizeof(*g));memset(&owner,0,sizeof(owner));memset(&request,0,sizeof(request));memset(cod1Actors,0,sizeof(cod1Actors));memset(&level,0,sizeof(level));
 owner.epoch=1;owner.generation=2;request.entity=owner;request.runtime_identity=3;request.catalog_identity=4;level.time=100;cod1EntityEpoch=cod1GenericEpoch=1;cod1ScriptedResources.revision=cod1FaceResources.revision=1;
 cod1EntityReady=leaseOK=certificateOK=scriptOK=faceOK=staticPresent=animAllowed=1;cod1GenericOperation.busy=cod1NormalUpdate.busy=cod1NormalOperation.busy=0;cod1FaceFrameOperation=NULL;calls=animationCalls=0;failAt=0;
#ifdef REFERENCE_R40
 g->nodes=tmalloc(C1GA_COUNT*sizeof(*g->nodes));g->notify=tmalloc(sizeof(*g->notify));g->names=tmalloc(C1GA_COUNT*sizeof(*g->names));assert(g->nodes&&g->notify&&g->names);memset(g->nodes,0,C1GA_COUNT*sizeof(*g->nodes));memset(g->notify,0,sizeof(*g->notify));
#else
 assert(ActorGenericAllocateBlock45(g,1));
#endif
 g->used=1;g->runtime=3;g->catalog=4;g->tick=100;g->revision=2;g->lease.entity=owner;g->lease.serial=9;g->notify->nameCount=1;g->names[0].ordinal=7;memset(g->names[0].text,0,64);strcpy(g->names[0].text,"notify");
 cod1Actors[0].scriptedController.initialized=1;cod1Actors[0].scriptedController.actorSerial=9;cod1Actors[0].scriptedController.revision=1;cod1Actors[0].value=17;g_entities[0].value=18;cod1FaceFrames[0].value=19;normal.value=20;other=normal;nativeStatic.value=21;cod1NormalRecords[0]=&normal;
 o->busy=1;o->kind=C1CTRL_UPDATE;o->now=100;o->epoch=1;o->resourceRevision=o->faceResourceRevision=1;o->lease=g->lease;o->input=&request;o->inputBytes=sizeof(request);o->q.step=request;o->before=&before;o->stage=&stage;o->entity=&entityBefore;before=stage=cod1Actors[0];entityBefore=g_entities[0];o->frameBefore=cod1FaceFrames[0];o->recordPointerBefore=&normal;o->recordBefore=normal;o->nativeStaticPresent=1;o->nativeStaticBefore=nativeStatic;o->genericHeaderBefore=*g;assert(ActorGenericClone(&o->genericDeepBefore,g));o->current=parentCallback;
}
static void done(void){ActorControllerExpectedFree(o);ActorGenericFreeOwned(&o->genericDeepBefore);ActorGenericFreeOwned(cod1GenericActors);ActorClonePoolClear();ActorControllerProviderReset();assert(!live);++checks;}
/* callback mutations independent of revisions, and identity/scope failures */
enum {OK,REFUSE,NODE,NOTIFY,NAME,HEADER,FRAME,ACTOR,ENTITY,RECORD,RECORDPTR,STATIC,STATICABSENT,INPUT,LEASE,CERT,EPOCH,RESOURCE,READY,TIME,SPAWN,RESTART,OPHEADER,REENTER,FOREIGN};
static int action;
static int parentCallback(void*ctx){(void)ctx;++calls;switch(action){case OK:break;case REFUSE:return 0;case NODE:cod1GenericActors[0].nodes[17].cycle++;break;case NOTIFY:cod1GenericActors[0].notify->pending[19]++;break;case NAME:cod1GenericActors[0].names[0].text[0]^=1;break;case HEADER:cod1GenericActors[0].revision++;break;case FRAME:cod1FaceFrames[0].value++;break;case ACTOR:cod1Actors[0].value++;break;case ENTITY:g_entities[0].value++;break;case RECORD:normal.value++;break;case RECORDPTR:cod1NormalRecords[0]=&other;break;case STATIC:nativeStatic.value++;break;case STATICABSENT:staticPresent=0;break;case INPUT:request.catalog_identity++;break;case LEASE:leaseOK=0;break;case CERT:certificateOK=0;break;case EPOCH:cod1GenericEpoch++;break;case RESOURCE:cod1FaceResources.revision++;break;case READY:scriptOK=0;break;case TIME:level.time++;break;case SPAWN:level.spawning=1;break;case RESTART:level.restarted=1;break;case OPHEADER:o->kind=C1CTRL_INIT;break;case REENTER:assert(!ActorControllerCurrent(o));break;case FOREIGN:assert(!ActorControllerForeignMutationAllowed());break;}return 1;}
static void arm(int p){assert(ActorControllerProviderArm(p,&owner,100,parentCallback,o));assert(ActorControllerProviderEnter(p,&owner,100,parentCallback,o));}
static void armChild(int p){arm(p);assert(ActorControllerProviderArmFrame(p,&owner,100,childCallback,&childContext));}
static void expectedPublish(void){C1ControllerExpected*e=o->expected;C1GARecord tmp,*g=cod1GenericActors;assert(ActorGenericClone(&tmp,&e->generic));ActorGenericFreeOwned(g);*g=tmp;cod1FaceFrames[0]=e->frame;cod1Actors[0]=e->actor;g_entities[0]=e->entity;normal=e->record;nativeStatic=e->nativeStatic;cod1NormalRecords[0]=e->recordPointer;staticPresent=e->nativeStaticPresent;}
int main(void){int p,i;GscEntityRef wrong;unsigned beforeCalls;action=OK;
 for(p=C1CP_CLOCK_SELECT;p<=C1CP_RECORD_UPDATE;p++){
  setup();armChild(p);assert(ActorControllerProviderEnter(C1CP_FRAME_ENTER,&owner,100,childCallback,&childContext));assert(ActorControllerProviderFinish(p,1)&&!o->cancelled&&!cod1ControllerProviderGate.parent);done();
  setup();arm(p);assert(ActorControllerProviderFinish(p,0));done();
  for(i=0;i<8;i++){setup();armChild(p);wrong=owner;wrong.generation++;switch(i){case 0:assert(!ActorControllerProviderEnter(C1CP_FRAME_ENTER,&wrong,100,childCallback,&childContext));break;case 1:assert(!ActorControllerProviderEnter(C1CP_FRAME_ENTER,&owner,101,childCallback,&childContext));break;case 2:assert(!ActorControllerProviderEnter(C1CP_FRAME_ENTER,&owner,100,parentCallback,&childContext));break;case 3:assert(!ActorControllerProviderEnter(C1CP_FRAME_ENTER,&owner,100,childCallback,o));break;case 4:o->checking=1;assert(!ActorControllerProviderEnter(C1CP_FRAME_ENTER,&owner,100,childCallback,&childContext));break;case 5:assert(!ActorControllerProviderEnter(p,&owner,100,parentCallback,o));break;case 6:animAllowed=0;assert(!ActorControllerProviderEnter(C1CP_FRAME_ENTER,&owner,100,childCallback,&childContext));break;case 7:assert(ActorControllerProviderEnter(C1CP_FRAME_ENTER,&owner,100,childCallback,&childContext));assert(!ActorControllerProviderEnter(C1CP_FRAME_ENTER,&owner,100,childCallback,&childContext));break;}assert(o->cancelled&&!cod1ControllerProviderGate.pending&&!ActorControllerProviderFinish(p,1));done();}
  setup();arm(p);assert(!ActorControllerProviderFinish(p,1)&&o->cancelled);done();
 }
 setup();o->busy=0;assert(ActorControllerProviderEnter(C1CP_FRAME_ENTER,&owner,100,childCallback,&childContext)&&ActorControllerProviderArmFrame(C1CP_RECORD_UPDATE,&owner,100,childCallback,&childContext));assert(ActorControllerForeignMutationAllowed());done();
 setup();assert(!ActorControllerProviderEnter(C1CP_FRAME_ENTER,&owner,100,childCallback,&childContext)&&o->cancelled&&animationCalls==1);done();
 setup();action=OK;assert(ActorControllerCurrent(o)&&calls==1);done();
 for(i=REFUSE;i<=FOREIGN;i++){setup();action=i;assert(!ActorControllerCurrent(o)&&!o->checking&&calls==1);done();}action=OK;
 setup();cod1GenericActors[0].nodes[0].cycle++;assert(!ActorControllerCurrent(o)&&!calls);done();
 for(i=0;i<2;i++){setup();assert(ActorControllerExpectedBase(o));o->expected->generic.nodes[8].cycle=77;o->expected->generic.revision++;o->expected->actor.value=42;o->expected->entity.value=43;assert(!ActorControllerExpectedMatches(o));expectedPublish();assert(ActorControllerExpectedMatches(o));o->stage->value=99;beforeCalls=allocs;ActorControllerExpectedRebase(o,i);assert(allocs==beforeCalls&&!o->expected&&o->before->value==42&&o->entity->value==43&&o->stage->value==(i?42u:99u));assert(o->genericDeepBefore.nodes!=cod1GenericActors[0].nodes&&ActorControllerCurrent(o));done();}
 for(i=NODE;i<=STATICABSENT;i++){setup();assert(ActorControllerExpectedBase(o));expectedPublish();action=i;assert(parentCallback(NULL));assert(!ActorControllerExpectedMatches(o));done();}action=OK;
 for(i=0;i<4;i++){setup();assert(ActorControllerExpectedBase(o));if(i==0)cod1GenericOperation.busy=1;if(i==1)cod1NormalUpdate.busy=1;if(i==2)cod1NormalOperation.busy=1;if(i==3)cod1FaceFrameOperation=o;assert(!ActorControllerExpectedMatches(o));done();}
 setup();failAt=allocs+1;assert(!ActorControllerExpectedBase(o)&&!o->expected);done();
 setup();failAt=allocs+2;assert(!ActorControllerExpectedBase(o)&&o->expected&&!o->expected->generic.nodes);done();
 printf("R45D2 controller: %u scenarios PASS\n",checks);return 0;
}
