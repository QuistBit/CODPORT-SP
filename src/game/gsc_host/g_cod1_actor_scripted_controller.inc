/* WORK ONLY: include after current normal-record, queue and generic/frame
 * owners. ROOT must wire member/lifecycle/private hooks described in README.
 * SAME canonical turretMount.currentStack/currentTop; no second live stack.
 * Only fresh normal4->9 source selection, then live-main normal update/pop.
 * Unsupported callbacks return refusal, never successful placeholders. */
#include "../g_cod1_actor_state_consumer_core.h"
#include "../g_cod1_actor_scripted_queue_core.h"
#include "../g_cod1_actor_scripted_controller.h"

enum {C1CTRL_INIT=1,C1CTRL_SELECT,C1CTRL_UPDATE};
typedef union {GscActorInitReturnRequest init;GscActorControllerRequest step;} C1ControllerRequest;
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
/* Fully private expected output, computed before the real provider. No
 * actual output bytes are accepted by a whitelist after current callbacks. */
typedef struct C1ControllerExpected {
 C1GARecord generic;C1FaceFrame frame;
 C1NormalRecord *recordPointer;C1NormalRecord record;
 cod1EntityNativeStaticRecord_t nativeStatic;int nativeStaticPresent;
 cod1ActorRecord_t actor;gentity_t entity;
 int committed,present;
} C1ControllerExpected;
typedef struct {
 int pending,parent,entered,frames;
 GscEntityRef owner;
 int now;
 int (*current)(void*);
 void *context;
} C1ControllerProviderGate;
static C1ControllerProviderGate cod1ControllerProviderGate;

static int ActorControllerProviderDeny(void) {
 if(cod1ControllerOperation.busy)cod1ControllerOperation.cancelled=1;
 cod1ControllerProviderGate.pending=C1CP_NONE;
 return 0;
}
/* Only the trusted controller direct-call sites call Arm. No callback,
 * allocation, validation or unrelated operation may occur between Arm and
 * the corresponding provider call. Nothing public exposes this capability. */
static int ActorControllerProviderArm(int kind,const GscEntityRef *owner,
 int now,int(*current)(void*),void *context) {
 C1ControllerOperation *o=&cod1ControllerOperation;
 C1ControllerProviderGate *g=&cod1ControllerProviderGate;
 if(!o->busy||o->cancelled||o->checking||!owner||!current||
    !ActorRefEqual(owner,&o->lease.entity)||now!=o->now||
    g->pending||g->parent||g->entered||g->frames||
    (kind!=C1CP_CLOCK_SELECT&&kind!=C1CP_RECORD_UPDATE))
  return ActorControllerProviderDeny();
 g->pending=kind;g->owner=*owner;g->now=now;
 g->current=current;g->context=context;
 return 1;
}
/* Actual provider entrance BEFORE its first callback or busy check. A
 * foreign call during controller.current cannot reuse an entered grant. */
static int ActorControllerProviderEnter(int kind,const GscEntityRef *owner,
 int now,int(*current)(void*),void *context) {
 C1ControllerOperation *o=&cod1ControllerOperation;
 C1ControllerProviderGate *g=&cod1ControllerProviderGate;
 if(kind==C1CP_FRAME_ENTER){if(!ActorControllerAnimationFrameEnter(owner,now,current,context))return ActorControllerProviderDeny();}
 else if(!ActorControllerAnimationMutationAllowed())return ActorControllerProviderDeny();
 if(!o->busy)return 1; /* Exact legacy/default behavior remains outside scope. */
 if(o->cancelled||o->checking||!owner||g->pending!=kind||
    !ActorRefEqual(owner,&g->owner)||now!=g->now||
    current!=g->current||context!=g->context)
  return ActorControllerProviderDeny();
 if(kind==C1CP_FRAME_ENTER) {
  if(!g->entered||!g->parent||g->frames)
   return ActorControllerProviderDeny();
  ++g->frames;
 } else {
  if(g->parent||g->entered||g->frames||
     (kind!=C1CP_CLOCK_SELECT&&kind!=C1CP_RECORD_UPDATE))
   return ActorControllerProviderDeny();
  g->parent=kind;g->entered=1;
 }
 g->pending=C1CP_NONE;g->current=NULL;g->context=NULL;
 return 1;
}
/* Called solely immediately before the legitimate nested FrameEnter in the
 * real ClockSelect or RecordUpdate implementation. No implicit "parent is
 * active => all frame entries allowed" state exists. Child current differs
 * for NormalUpdate, hence bind this exact immediate-call callback/context. */
static int ActorControllerProviderArmFrame(int parent,const GscEntityRef *owner,
 int now,int(*current)(void*),void *context) {
 C1ControllerOperation *o=&cod1ControllerOperation;
 C1ControllerProviderGate *g=&cod1ControllerProviderGate;
 if(!o->busy)return 1;
 if(o->cancelled||o->checking||!owner||!current||
    !g->entered||g->parent!=parent||g->pending||g->frames||
    !ActorRefEqual(owner,&g->owner)||now!=g->now)
  return ActorControllerProviderDeny();
 g->pending=C1CP_FRAME_ENTER;g->current=current;g->context=context;
 return 1;
}
static int ActorControllerProviderFinish(int parent,int expectedFrames) {
 C1ControllerOperation *o=&cod1ControllerOperation;
 C1ControllerProviderGate *g=&cod1ControllerProviderGate;
 int ok=o->busy&&!o->cancelled&&!o->checking&&g->entered==1&&
  g->parent==parent&&!g->pending&&g->frames==expectedFrames;
 memset(g,0,sizeof(*g));
 if(!ok&&o->busy)o->cancelled=1;
 return ok;
}
/* No legitimately required constructor or generic host transaction is
 * entered inside this controller transaction. Genuine VM mutation belongs
 * to root's SEPARATE AnimationStep owner, not to this scope. */
static int ActorControllerForeignMutationAllowed(void) {
 return ActorControllerAnimationGenericAllowed()&&(!cod1ControllerOperation.busy?1:ActorControllerProviderDeny());
}
static void ActorControllerProviderReset(void) {
 memset(&cod1ControllerProviderGate,0,sizeof(cod1ControllerProviderGate));
}

/* REQUIRED REAL service implemented by ROOT. No body/success stub here. */
static int ActorControllerAnimationStep(void*,const GscActorControllerRequest*,
 const GscActorControllerAnimationServices*,char*,size_t,int(*)(void*),void*);
static int ActorControllerInitReturn(void*,const GscActorInitReturnRequest*,char*,size_t,int(*)(void*),void*);
static int ActorControllerSelectMain(void*,const GscActorControllerRequest*,GscActorControllerSelection*,char*,size_t,int(*)(void*),void*);
static int ActorControllerNormalUpdate(void*,const GscActorControllerRequest*,int*,char*,size_t,int(*)(void*),void*);
static const GscActorScriptedControllerHost cod1ControllerHost={NULL,
 ActorControllerInitReturn,ActorControllerSelectMain,ActorControllerAnimationStep,ActorControllerNormalUpdate};

/* A dynamic grant exists only on the synchronous real INIT call stack.
 * No public request can supply a returned-task certificate or select it. */
typedef struct {
 int active,cancelled,now;GscSession *session;uint64_t id,program;
 GscEntityRef source;cod1ActorLease_t lease;GscEntityScriptRun run;
 GscActorInitReturnRequest seal;const GscActorInitReturnRequest *borrowed;
} C1SpawnControllerScope;
static C1SpawnControllerScope cod1SpawnControllerScope;
static int cod1PlacedInitBusy,cod1PlacedInitCancelled;
typedef struct {
 int active,popActive,cancelled,checking,completed,now;
 const GscActorBootstrapServices*input;GscActorBootstrapServices api;
 const cod1ActorLease_t*inputLease;cod1ActorLease_t lease;
 const GscPlacedInitReturnRequest*borrowed;GscPlacedInitReturnRequest seal;
 const GscActorInitReturnRequest*controllerBorrowed;GscActorInitReturnRequest controllerSeal;
 int(*vmCurrent)(void*);void*vmContext;
} C1PlacedControllerScope;
static C1PlacedControllerScope cod1PlacedControllerScope;
static int ActorPlacedControllerPure(C1PlacedControllerScope*f){
 return f==&cod1PlacedControllerScope&&f->active&&!f->cancelled&&cod1PlacedInitBusy&&!cod1PlacedInitCancelled&&
  f->now==level.time&&!level.spawning&&!level.restarted&&cod1EntityReady&&ActorMapAvailable()&&
  f->input&&f->inputLease&&!memcmp(f->input,&f->api,sizeof(f->api))&&!memcmp(f->inputLease,&f->lease,sizeof(f->lease))&&
  ActorLeaseValid(&f->lease)&&G_Cod1ActorBootstrapPostinitCurrent(&f->lease)&&ActorInheritedCertificate(&f->lease.entity,f->lease.serial)&&
  cod1Actors[f->lease.actorIndex].placedInitAttempted&&!cod1Actors[f->lease.actorIndex].initialized;
}
static int ActorPlacedControllerCurrent(void*ctx){C1PlacedControllerScope*f=ctx;int ok;
 if(!ActorPlacedControllerPure(f)||!f->popActive||!f->borrowed||!f->controllerBorrowed||!f->vmCurrent||
  memcmp(f->borrowed,&f->seal,sizeof(f->seal))||memcmp(f->controllerBorrowed,&f->controllerSeal,sizeof(f->controllerSeal)))return 0;
 if(f->checking){f->cancelled=1;return 0;}f->checking=1;
 ok=f->vmCurrent(f->vmContext)==1&&ActorPlacedControllerPure(f)&&
  f->api.placed_init_return_current(f->api.context,f->borrowed)==1&&ActorPlacedControllerPure(f)&&
  f->api.program_identity(f->api.context)==f->seal.program_identity&&f->api.current(f->api.context)==1;
 f->checking=0;return ok&&ActorPlacedControllerPure(f)&&!memcmp(f->borrowed,&f->seal,sizeof(f->seal))&&!memcmp(f->controllerBorrowed,&f->controllerSeal,sizeof(f->controllerSeal));
}
static int ActorPlacedControllerAdmission(const GscActorInitReturnRequest*q,const cod1ActorLease_t*l,int(*current)(void*),void*ctx){
 C1PlacedControllerScope*f=&cod1PlacedControllerScope;
 return q==f->controllerBorrowed&&current==ActorPlacedControllerCurrent&&ctx==f&&!memcmp(l,&f->lease,sizeof(*l))&&ActorPlacedControllerCurrent(f);
}
static int ActorSpawnControllerCurrent(void*context){
 C1SpawnControllerScope*f=&cod1SpawnControllerScope;
 return context==f&&f->active&&!f->cancelled&&f->borrowed&&
  !memcmp(f->borrowed,&f->seal,sizeof(f->seal))&&f->now==level.time&&
  !level.spawning&&!level.restarted&&cod1EntityReady&&ActorMapAvailable()&&
  ActorLeaseValid(&f->lease)&&EntityValidate(NULL,&f->source)&&
  G_Cod1SpawnerIntakeIdentity(&f->source)&&
  f->run.status==GSC_ENTITY_ENTRY_RETURNED&&f->run.task_serial==f->seal.task_serial&&
  ActorRefEqual(&f->run.entity,&f->lease.entity)&&
  gsc_session_spawn_issued_owner_program(f->session,f->id,&f->source,
   &f->lease.entity,f->lease.serial)==f->program;
}
static int ActorSpawnControllerAdmission(const GscActorInitReturnRequest*q,
 const cod1ActorLease_t*l,int(*current)(void*),void*ctx){
 C1SpawnControllerScope*f=&cod1SpawnControllerScope;
 return q==f->borrowed&&current==ActorSpawnControllerCurrent&&ctx==f&&
  !memcmp(l,&f->lease,sizeof(*l))&&ActorSpawnControllerCurrent(f);
}

static int ActorControllerPrivate(const void*p,size_t n){C1ControllerOperation*o=&cod1ControllerOperation;
 C1ControllerExpected*e=o->expected;
 return ActorOverlap(p,n,&g_cod1AIDebugThreat,sizeof(g_cod1AIDebugThreat))||
  ActorOverlap(p,n,&cod1SpawnControllerScope,sizeof(cod1SpawnControllerScope))||
  ActorOverlap(p,n,&cod1PlacedControllerScope,sizeof(cod1PlacedControllerScope))||
  ActorOverlap(p,n,&cod1PlacedInitBusy,sizeof(cod1PlacedInitBusy))||ActorOverlap(p,n,&cod1PlacedInitCancelled,sizeof(cod1PlacedInitCancelled))||
  ActorControllerAnimationPrivate(p,n)||ActorOverlap(p,n,&cod1ControllerHost,sizeof(cod1ControllerHost))||
  ActorOverlap(p,n,&cod1ControllerProviderGate,sizeof(cod1ControllerProviderGate))||
  ActorOverlap(p,n,o,sizeof(*o))||ActorOverlap(p,n,o->before,o->before?sizeof(*o->before):0)||
  ActorOverlap(p,n,o->stage,o->stage?sizeof(*o->stage):0)||
  ActorOverlap(p,n,o->entity,o->entity?sizeof(*o->entity):0)||
  ActorOverlap(p,n,o->genericDeepBefore.nodes,o->genericDeepBefore.nodes?C1GA_COUNT*sizeof(*o->genericDeepBefore.nodes):0)||
  ActorGenericNamesPrivate(p,n,&o->genericDeepBefore)||ActorOverlap(p,n,o->genericDeepBefore.notify,o->genericDeepBefore.notify?sizeof(*o->genericDeepBefore.notify):0)||
  ActorOverlap(p,n,e,e?sizeof(*e):0)||
  (e&&(ActorOverlap(p,n,e->generic.nodes,e->generic.nodes?C1GA_COUNT*sizeof(*e->generic.nodes):0)||
       ActorGenericNamesPrivate(p,n,&e->generic)||ActorOverlap(p,n,e->generic.notify,e->generic.notify?sizeof(*e->generic.notify):0)));
}
static void ActorControllerExpectedFree(C1ControllerOperation*o){
 if(o->expected){ActorGenericFreeOwned(&o->expected->generic);free(o->expected);o->expected=NULL;}
}
static void ActorControllerGrantPoison(void){
 if(cod1ControllerOperation.busy)cod1ControllerOperation.cancelled=1;
 cod1ControllerProviderGate.pending=C1CP_NONE;
}
/* Allocation hook only; not entered/ready. Called once after actual serial. */
static void ActorControllerFresh(uint32_t i,uint64_t serial){
 cod1ActorScriptedController_t*m;if(i>=COD1_ACTOR_CAPACITY||!serial)return;
 m=&cod1Actors[i].scriptedController;memset(m,0,sizeof(*m));m->initialized=1;m->actorSerial=serial;m->revision=1;
}
static void ActorControllerRetire(uint32_t i,const GscEntityRef*owner){
 if(!owner||i>=COD1_ACTOR_CAPACITY)return;ActorControllerAnimationCancelOwner(owner);
 if(cod1ControllerOperation.busy&&ActorRefEqual(&cod1ControllerOperation.lease.entity,owner))ActorControllerGrantPoison();
 if(ActorRefEqual(&cod1Actors[i].owner,owner))memset(&cod1Actors[i].scriptedController,0,sizeof(cod1ActorScriptedController_t));
}
static void ActorControllerReset(void){
 unsigned i;ActorControllerAnimationReset();ActorControllerGrantPoison();
 for(i=0;i<COD1_ACTOR_CAPACITY;i++)memset(&cod1Actors[i].scriptedController,0,sizeof(cod1ActorScriptedController_t));
}
/* This identity-only portion is also safe at the closed provider return
 * seam BEFORE legitimate output bytes have been rebased. No foreign callback. */
static int ActorControllerIdentity(const C1ControllerOperation*o){
 cod1ActorLease_t l;const C1GARecord*r;const cod1ActorScriptedController_t*m;
 const GscEntityRef*ref;uint64_t runtime,catalog;
 if(!o||o!=&cod1ControllerOperation||!o->busy||o->cancelled||!o->before||!o->stage||!o->entity||
    o->now!=level.time||o->epoch!=cod1EntityEpoch||o->epoch!=cod1GenericEpoch||
    !cod1EntityReady||level.spawning||level.restarted||!o->input||
    memcmp(o->input,&o->q,o->inputBytes)||!ActorGoalLease(&o->lease.entity,&l)||
    memcmp(&l,&o->lease,sizeof(l)))return 0;
 ref=o->kind==C1CTRL_INIT?&o->q.init.entity:&o->q.step.entity;
 runtime=o->kind==C1CTRL_INIT?o->q.init.runtime_identity:o->q.step.runtime_identity;
 catalog=o->kind==C1CTRL_INIT?o->q.init.catalog_identity:o->q.step.catalog_identity;
 if(!ActorRefEqual(ref,&l.entity)||!ActorInheritedCertificate(&l.entity,l.serial))return 0;
 r=cod1GenericActors+l.actorIndex;m=&cod1Actors[l.actorIndex].scriptedController;
 return m->initialized&&!m->invalid&&m->actorSerial==l.serial&&m->revision&&m->revision!=UINT64_MAX&&
  r->used&&r->runtime==runtime&&r->catalog==catalog&&!memcmp(&r->lease,&l,sizeof(l))&&
  o->resourceRevision==cod1ScriptedResources.revision&&ActorScriptedReady(o->epoch)&&
  o->faceResourceRevision==cod1FaceResources.revision&&ActorFaceReady(o->epoch);
}
static int ActorControllerSnapshotStable(const C1ControllerOperation*o){
 const C1GARecord*g=cod1GenericActors+o->lease.actorIndex;
 const C1NormalRecord*r=cod1NormalRecords[o->lease.actorIndex];
 const cod1EntityNativeStaticRecord_t*s=NativeStaticFind(&o->lease.entity);
 if(memcmp(g,&o->genericHeaderBefore,sizeof(*g))||!g->nodes||!g->notify||
    !o->genericDeepBefore.nodes||!o->genericDeepBefore.notify||
    !R242ExactATRBytesEqual(g->nodes,o->genericDeepBefore.nodes,C1GA_COUNT*sizeof(*g->nodes))||
    !R242ExactATRBytesEqual(g->notify,o->genericDeepBefore.notify,sizeof(*g->notify))||!ActorGenericNamesEqual(g,&o->genericDeepBefore)||
    !R242ExactATRBytesEqual(cod1FaceFrames+o->lease.actorIndex,&o->frameBefore,sizeof(o->frameBefore))||
    r!=o->recordPointerBefore||(r&&!R242ExactATRBytesEqual(r,&o->recordBefore,sizeof(*r)))||
    ((s!=NULL)!=o->nativeStaticPresent)||(s&&!R242ExactATRBytesEqual(s,&o->nativeStaticBefore,sizeof(*s))))return 0;
 return 1;
}
static int ActorControllerScope(const C1ControllerOperation*o){
 return ActorControllerIdentity(o)&&
  R242ExactATRBytesEqual(cod1Actors+o->lease.actorIndex,o->before,sizeof(*o->before))&&
  R242ExactATRBytesEqual(g_entities+o->lease.entity.slot,o->entity,sizeof(*o->entity))&&
  ActorControllerSnapshotStable(o);
}
static int ActorControllerCurrent(void*context){
 C1ControllerOperation*o=context;C1ControllerOperation saved;int ok;
 if(!ActorControllerScope(o))return 0;
 if(o->checking){o->cancelled=1;return 0;}o->checking=1;saved=*o;
 ok=o->current(o->context)==1;
 if(!R242ExactATRBytesEqual(o,&saved,sizeof(saved)))ok=0;
 if(!ActorControllerScope(o))ok=0;
 o->checking=0;return ok;
}
static void ActorControllerEnd(void){C1ControllerOperation*o=&cod1ControllerOperation;
 ActorControllerProviderReset();ActorControllerExpectedFree(o);
 ActorGenericFreeOwned(&o->genericDeepBefore);
 free(o->before);free(o->stage);free(o->entity);memset(o,0,sizeof(*o));
}
static int C1ObserveInit(const char *predicate,int accepted){if(!accepted)G_Printf("[Foley private] init rejected %s\n",predicate);return accepted;}
static int ActorControllerBegin(const void*input,size_t bytes,int kind,const GscEntityRef*ref,
 int now,int(*current)(void*),void*ctx){C1ControllerOperation*o=&cod1ControllerOperation;
 if(!ActorControllerAnimationMutationAllowed())return 0;
 if(o->busy){o->cancelled=1;return 0;}
 if(!input||!ref||!current||bytes>sizeof(o->q)||!ActorGoalBuffers(input,bytes,NULL,0,NULL,0)||
    ActorControllerPrivate(input,bytes)||now!=level.time)return 0;
 memset(o,0,sizeof(*o));o->input=input;o->inputBytes=bytes;memcpy(&o->q,input,bytes);
 o->kind=kind;o->now=now;o->current=current;o->context=ctx;o->epoch=cod1EntityEpoch;
 if(!C1ObserveInit("ActorGoalLease(ref,&o->lease)",ActorGoalLease(ref,&o->lease)))goto fail;
 o->before=malloc(sizeof(*o->before));o->stage=malloc(sizeof(*o->stage));o->entity=malloc(sizeof(*o->entity));
 if(!o->before||!o->stage||!o->entity)goto fail;
 *o->before=*o->stage=cod1Actors[o->lease.actorIndex];*o->entity=g_entities[o->lease.entity.slot];
 o->resourceRevision=cod1ScriptedResources.revision;o->faceResourceRevision=cod1FaceResources.revision;
 o->genericHeaderBefore=cod1GenericActors[o->lease.actorIndex];
 if(!C1ObserveInit("ActorGenericClone(&o->genericDeepBefore,&o->genericHeaderBefore)",ActorGenericClone(&o->genericDeepBefore,&o->genericHeaderBefore)))goto fail;
 o->frameBefore=cod1FaceFrames[o->lease.actorIndex];
 o->recordPointerBefore=cod1NormalRecords[o->lease.actorIndex];
 if(o->recordPointerBefore)o->recordBefore=*o->recordPointerBefore;
 {const cod1EntityNativeStaticRecord_t*s=NativeStaticFind(&o->lease.entity);
  o->nativeStaticPresent=s!=NULL;if(s)o->nativeStaticBefore=*s;}
 o->busy=1;
 if(!C1ObserveInit("ActorControllerCurrent(o)",ActorControllerCurrent(o)))goto fail;return 1;
 fail:ActorControllerEnd();return 0;
}
static int ActorControllerFreshDomain(const cod1ActorRecord_t*a){
 const cod1ActorTurretMount_t*m=&a->turretMount;
 /* Strict selected no-route/no-locomotion domain. Not a fabricated empty path. */
 return m->initialized&&m->serial==a->serial&&!m->mounted&&!m->requestedGun.epoch&&!m->occupiedGun.epoch&&
  !m->pendingOwnerNotify&&!m->pendingStateNotify&&!m->pendingStand&&
  m->blocker1164Known&&!m->blocker1164&&m->claimedNode40Known&&!m->claimedNode40&&
  !a->motion.active&&!a->goal.assigned&&a->dataInitialized&&a->nativeDataInitialized&&a->alive&&
  a->defaults.pathWords[(0x6f4-0x374)/4]==0&&
  (a->defaults.pathWords[(0x6f8-0x374)/4]>>16)==0;
}
/* After a grounded first goal, the empty native path and real script record still own row4/9 transitions. */
static int TrainingJumperLanding(const C1ControllerOperation *o,const cod1ActorRecord_t *a){
 const cod1ActorScriptedController_t*m=&a->scriptedController;
 return TrainingJumperExitNode(&o->lease.entity,a->goal.assignment,&a->goal.node)&&
    a->goal.assigned&&!a->motion.active&&!a->turretMount.mounted&&a->alive&&
    m->entered&&!m->invalid&&m->actorSerial==o->lease.serial&&m->currentSource==4&&
    a->turretMount.currentTop==1&&a->turretMount.currentStack[1]==9&&
    a->critical.top==1&&a->critical.stack[1]==9&&!a->critical.command_count;
}
static int FoleyArrivedGround(const C1ControllerOperation *o,const cod1ActorRecord_t *a){
 const cod1ActorTurretMount_t*m=&a->turretMount;
 const cod1ActorMotion_t*move=&a->motion;
 const gentity_t*e=o->entity;
 if((o->kind!=C1CTRL_SELECT&&o->kind!=C1CTRL_UPDATE) || o->stage!=a || !ActorLeaseValid(&o->lease) ||
    !ActorRefEqual(&a->owner,&o->lease.entity) || a->serial!=o->lease.serial ||
    !e->targetname || strcmp(e->targetname,"foley") ||
    !e->classname || strcmp(e->classname,"actor_ally_airborne_officer_thompson") ||
    !e->r.linked || !(e->r.svFlags&SVF_NOCLIENT) || e->client || e->r.bmodel ||
    !a->spatial.linked || !a->dobj.present || a->dobj.busy || a->spatial.busy ||
    !a->alive || !a->nativeDataInitialized || !a->dataInitialized)return 0;
 if(!m->initialized || m->serial!=a->serial || m->mounted || m->requestedGun.epoch ||
    m->occupiedGun.epoch || m->pendingOwnerNotify || m->pendingStateNotify || m->pendingStand ||
    !m->blocker1164Known || m->blocker1164 || !m->claimedNode40Known || m->claimedNode40 ||
    a->defaults.pathWords[(0x6f4-0x374)/4] || (a->defaults.pathWords[(0x6f8-0x374)/4]>>16))return 0;
 if(!a->goal.assigned || !G_Cod1FoleyCourseGoal(a->goal.assignment,&a->goal.node) || !move->active || move->model ||
    move->busy || move->moving || move->blocked || !move->arrived ||
    move->assignment!=a->goal.assignment || move->notified!=a->goal.assignment ||
    !(move->travelled>0.f) || move->lastTime>o->now ||
    memcmp(e->r.currentOrigin,a->nativeData.entity.origin,12) ||
    memcmp(e->r.currentOrigin,a->goal.position,2*sizeof(float)))return 0;
 return 1;
}
/* While an original animscripted gesture owns row 9, setgoalnode
 * changes the sentient's destination. Native GAME keeps applying the
 * scripted root until the normal record releases it, including mid-walk. */
static int InstructorCourseGoal(const C1ControllerOperation *o,const cod1ActorRecord_t *a){
 const gentity_t *e=o->entity;
 const cod1ActorTurretMount_t *m=&a->turretMount;
 cod1PlacedGoal_t placed;
 if((o->kind!=C1CTRL_SELECT && o->kind!=C1CTRL_UPDATE) || o->stage!=a ||
    !ActorLeaseValid(&o->lease) || !ActorRefEqual(&a->owner,&o->lease.entity) ||
    a->serial!=o->lease.serial || !e->targetname || (strcmp(e->targetname,"moody") && strcmp(e->targetname,"foley")) ||
    !e->classname || strcmp(e->classname,"actor_ally_airborne_officer_thompson") ||
    !e->r.linked || !(e->r.svFlags&SVF_NOCLIENT) || e->client || e->r.bmodel ||
    !a->spatial.linked || !a->dobj.present || a->spatial.busy || a->dobj.busy ||
    !a->alive || !a->dataInitialized || !a->nativeDataInitialized || !m->initialized ||
    m->serial!=a->serial || m->mounted || m->requestedGun.epoch || m->occupiedGun.epoch ||
    m->pendingOwnerNotify || m->pendingStateNotify || m->pendingStand ||
    !m->blocker1164Known || m->blocker1164 || !m->claimedNode40Known || m->claimedNode40 ||
    a->motion.busy || a->motion.model || !a->goal.assigned ||
    !(strcmp(e->targetname,"foley") ? G_Cod1MoodyCourseGoal(a->goal.assignment,&a->goal.node) : G_Cod1FoleyCourseGoal(a->goal.assignment,&a->goal.node)) ||
    a->defaults.pathWords[(0x6f4-0x374)/4] || (a->defaults.pathWords[(0x6f8-0x374)/4]>>16) ||
    memcmp(e->r.currentOrigin,a->nativeData.entity.origin,12))return 0;
 placed=G_Cod1TrainingPlacedGoal(&o->lease.entity,a->goal.assignment,&a->goal.node);
 return placed.status==1 &&
    !memcmp(placed.position,cod1Sentients[o->lease.sentientIndex].rawGoal.position,12);
}
static int TowerOrArrivedGround(const C1ControllerOperation *o,const cod1ActorRecord_t *a){
 return FoleyArrivedGround(o,a) || InstructorCourseGoal(o,a);
}
static int FoleyArrivedBody(const C1ControllerOperation *o,const cod1ActorRecord_t *a){
 const C1NormalRecord *n;
 if((o->kind!=C1CTRL_UPDATE && o->kind!=C1CTRL_SELECT)||!TowerOrArrivedGround(o,a)||
    !a->scriptedController.entered||a->scriptedController.invalid||a->scriptedController.resumed4||
    a->scriptedController.actorSerial!=o->lease.serial||!a->scriptedController.selectionSerial||
    a->scriptedController.currentSource!=4||a->scriptedController.force250||
    a->turretMount.currentTop!=1||a->turretMount.currentStack[0]!=4||a->turretMount.currentStack[1]!=9||
    a->critical.command_count||a->critical.top!=1||a->critical.stack[0]!=4||a->critical.stack[1]!=9)return 0;
 n=cod1NormalRecords[o->lease.actorIndex];
 if(!n)return a->scriptedAux.record==FRESH_AUX_EMPTY && !o->recordPointerBefore;
 return o->recordPointerBefore==n && n->present && n->revision && !memcmp(&n->lease,&o->lease,sizeof(n->lease)) &&
    n->runtime==a->scriptedController.runtime && n->catalog==a->scriptedController.catalog &&
    n->resources==cod1ScriptedResources.revision && a->scriptedAux.record==FRESH_AUX_OWNED &&
    ((InstructorCourseGoal(o,a) && C1ASB14_Body(ActorScriptedIndex(n->value.clip))) ||
     n->value.clip==893 || n->value.clip==895 || n->value.clip==896 || n->value.clip==897 || (!n->value.clip && n->value.firstUpdate && n->updated>=n->created));
}
static int FoleyArrivedSelect(const C1ControllerOperation *o,const cod1ActorRecord_t *a){
 const cod1ActorTurretMount_t*m=&a->turretMount;
 if(o->kind!=C1CTRL_SELECT || !TowerOrArrivedGround(o,a))return 0;
 return a->scriptedController.resumed4 && a->scriptedController.entered &&
    !a->scriptedController.invalid && a->scriptedController.selectionSerial &&
    a->scriptedController.actorSerial==o->lease.serial && a->scriptedController.currentSource==4 &&
    !m->currentTop && m->currentStack[0]==4 && !o->recordPointerBefore &&
    a->critical.command_count==1 && a->critical.top==1 && a->critical.stack[0]==4 &&
    a->critical.stack[1]==9 && a->critical.commands[0].operation==2 &&
    a->critical.commands[0].state==9 && a->scriptedAux.prefix==FRESH_PREFIX_QUEUE_COMMITTED &&
    a->scriptedAux.record==FRESH_AUX_EMPTY && a->scriptedAux.link==FRESH_LINK_UNATTACHED;
}

static void ActorControllerRouteEmptyEnter(cod1ActorRecord_t*a){
 /* Actual5F2E0 count0/index0 path has no effect. Exact row4/9 clears6F4/6F6,
  * NOT index6FA. Both words are this one actual canonical path DWORD. */
 a->defaults.pathWords[(0x6f4-0x374)/4]=0;a->defaults.cell0774=0;
}
static int ActorControllerClosedFrame(const C1ControllerOperation*o){
 const C1FaceFrame*f=cod1FaceFrames+o->lease.actorIndex;
 return o->q.step.frame_generation&&ActorScriptedClockSelected(o->lease.actorIndex)&&
  f->frame==o->q.step.frame_generation&&f->tick==o->now&&!f->open&&f->done&&!f->count&&!f->poisoned&&
  f->runtime==o->q.step.runtime_identity&&f->catalog==o->q.step.catalog_identity&&
  !memcmp(&f->lease,&o->lease,sizeof(o->lease));
}
/* Expected heap is protected by ActorControllerPrivate. Clone allocation
 * is completed before a provider grant or any irreversible effect. */
static int ActorControllerExpectedBase(C1ControllerOperation*o){
 C1ControllerExpected*e;if(o->expected||!ActorControllerScope(o))return 0;
 e=calloc(1,sizeof(*e));if(!e)return 0;o->expected=e;
 e->actor=*o->before;e->entity=*o->entity;e->frame=o->frameBefore;
 e->recordPointer=o->recordPointerBefore;e->record=o->recordBefore;
 e->nativeStaticPresent=o->nativeStaticPresent;e->nativeStatic=o->nativeStaticBefore;
 return ActorGenericClone(&e->generic,&o->genericDeepBefore);
}
/* Full callback-free output comparison. Non-null replacement owned arrays
 * may reuse an address; identity is not inferred from malloc address novelty. */
static int ActorControllerExpectedMatches(C1ControllerOperation*o){
 const C1ControllerExpected*e=o->expected;const C1GARecord*g;
 const C1NormalRecord*r;const cod1EntityNativeStaticRecord_t*s;C1GARecord h;
 if(!e||!ActorControllerIdentity(o)||o->checking||cod1GenericOperation.busy||
    cod1FaceFrameOperation||cod1NormalUpdate.busy||cod1NormalOperation.busy)return 0;
 g=cod1GenericActors+o->lease.actorIndex;r=cod1NormalRecords[o->lease.actorIndex];
 s=NativeStaticFind(&o->lease.entity);h=e->generic;h.nodes=g->nodes;h.notify=g->notify;h.names=g->names;
 return g->nodes&&g->notify&&e->generic.nodes&&e->generic.notify&&
  !memcmp(g,&h,sizeof(h))&&
  R242ExactATRBytesEqual(g->nodes,e->generic.nodes,C1GA_COUNT*sizeof(*g->nodes))&&
  R242ExactATRBytesEqual(g->notify,e->generic.notify,sizeof(*g->notify))&&ActorGenericNamesEqual(g,&e->generic)&&
  R242ExactATRBytesEqual(cod1FaceFrames+o->lease.actorIndex,&e->frame,sizeof(e->frame))&&
  r==e->recordPointer&&(!r||R242ExactATRBytesEqual(r,&e->record,sizeof(*r)))&&
  ((s!=NULL)==e->nativeStaticPresent)&&(!s||R242ExactATRBytesEqual(s,&e->nativeStatic,sizeof(*s)))&&
  R242ExactATRBytesEqual(cod1Actors+o->lease.actorIndex,&e->actor,sizeof(e->actor))&&
  R242ExactATRBytesEqual(g_entities+o->lease.entity.slot,&e->entity,sizeof(e->entity));
}
/* No callbacks/allocation after comparison before this transfer. The original
 * per-operation BEFORE clone is freed, never the already-replaced LIVE arrays. */
static void ActorControllerExpectedRebase(C1ControllerOperation*o,int actorStage){
 C1ControllerExpected*e=o->expected;
 ActorGenericFreeOwned(&o->genericDeepBefore);o->genericDeepBefore=e->generic;
 e->generic.nodes=NULL;e->generic.notify=NULL;e->generic.names=NULL;
 o->genericHeaderBefore=cod1GenericActors[o->lease.actorIndex];o->frameBefore=e->frame;
 o->recordPointerBefore=e->recordPointer;o->recordBefore=e->record;
 o->nativeStaticPresent=e->nativeStaticPresent;o->nativeStaticBefore=e->nativeStatic;
 *o->before=e->actor;*o->entity=e->entity;if(actorStage)*o->stage=e->actor;
 ActorControllerExpectedFree(o);
}
static int ActorControllerInitClock(C1ControllerOperation*o){
 C1ControllerExpected*e;GscEntityRef ref=o->lease.entity;int called,finished;
 if(!C1ObserveInit("ActorControllerExpectedBase(o)",ActorControllerExpectedBase(o)))return 0;e=o->expected;
 if(e->frame.selected||e->frame.count||e->generic.tick!=o->now||
    ActorGenericNamedActive(&e->generic)||e->generic.revision==UINT64_MAX)return 0;
 memset(&e->frame,0,sizeof(e->frame));e->frame.selected=e->frame.scripted=1;
 e->frame.tick=o->now;e->frame.gameFrame=level.framenum;e->frame.runtime=e->generic.runtime;e->frame.catalog=e->generic.catalog;
 e->frame.identity=e->generic.notify->ownerIdentity;e->frame.lease=o->lease;
 if(!ActorScriptedMap(&e->frame))return 0;++e->generic.revision;e->committed=1;
 if(!ActorControllerCurrent(o)||!ActorControllerProviderArm(C1CP_CLOCK_SELECT,&ref,o->now,ActorControllerCurrent,o))return 0;
 called=G_Cod1ActorScriptedClockSelect(&ref,o->now,ActorControllerCurrent,o);
 finished=ActorControllerProviderFinish(C1CP_CLOCK_SELECT,1);
 if(!called||!finished||!ActorControllerExpectedMatches(o))return 0;
 /* Preserve staged row4 entry/timers: those are not committed actor bytes yet. */
 ActorControllerExpectedRebase(o,0);return ActorControllerCurrent(o);
}
static int ActorControllerInitReturnSelected(void*u,const GscActorInitReturnRequest*input,char*error,size_t cap,
 int(*current)(void*),void*ctx,int dynamic){C1ControllerOperation*o=&cod1ControllerOperation;
 cod1ActorScriptedController_t*m;cod1ActorRecord_t*a;GscEntityRef ref;int ok=0;(void)u;
 if(!input||!ActorGoalBuffers(input,sizeof(*input),NULL,0,error,cap))return 0;ref=input->entity;
 /* Trust boundary: configured Session invokes ONLY after actual canonical
  * animscripts/init::main normal pop, with authenticated returningFrame.self.
  * task_serial is an identity supplied by that VM owner, never ready proof. */
 if(g_debugMove.integer&&ref.slot<MAX_GENTITIES && cod1ActorByEntity[ref.slot]){
 unsigned idx=cod1ActorByEntity[ref.slot]-1;
 if(idx<COD1_ACTOR_CAPACITY){C1GARecord *g=cod1GenericActors+idx;
 G_Printf("[Foley private] init source slot=%u used=%d nodes=%d notify=%d tick=%d now=%d controller=%d\n",ref.slot,g->used,g->nodes!=NULL,g->notify!=NULL,g->tick,input->now_ms,cod1Actors[idx].scriptedController.initialized);}}
 if(!input->task_serial||!input->runtime_identity||!input->catalog_identity||input->now_ms<0||
    input->now_ms>INT_MAX-2000||!ActorControllerBegin(input,sizeof(*input),C1CTRL_INIT,&ref,input->now_ms,current,ctx))return 0;
 a=o->stage;m=&a->scriptedController; if(g_debugMove.integer)G_Printf("[Foley private] stage top=%d row=%d criticalTop=%d criticalRow=%d commands=%d genericTick=%d selected=%d\n",a->turretMount.currentTop,a->turretMount.currentStack[0],a->critical.top,a->critical.stack[0],a->critical.command_count,o->genericHeaderBefore.tick,o->frameBefore.selected);
 if(m->entered||m->initTask||m->currentSource||!(dynamic==2?ActorPlacedControllerAdmission(input,&o->lease,current,ctx):dynamic==1?ActorSpawnControllerAdmission(input,&o->lease,current,ctx):dynamic==0?C1ObserveInit("FreshBootstrapTarget(&o->lease)",FreshBootstrapTarget(&o->lease)):0)||
    !C1ObserveInit("ActorControllerFreshDomain(a)",ActorControllerFreshDomain(a))||a->turretMount.currentTop||a->turretMount.currentStack[0]!=4||
    a->critical.command_count||a->critical.top||a->critical.stack[0]!=4)goto done;
 ActorControllerRouteEmptyEnter(a);m->runtime=input->runtime_identity;m->catalog=input->catalog_identity;
 m->initTask=input->task_serial;m->entered=1;m->entered_ms=input->now_ms;
 m->timer28=500;m->timer2c=input->now_ms+50;m->timer34=input->now_ms+2000;
 ++m->revision;
 if(!C1ObserveInit("ActorControllerInitClock(o)",ActorControllerInitClock(o))||!ActorControllerCurrent(o))goto done;
 cod1Actors[o->lease.actorIndex]=*a;ok=1;
 done:ActorControllerEnd();return ok;
}
static int ActorControllerInitReturn(void*u,const GscActorInitReturnRequest*q,char*e,size_t n,
 int(*current)(void*),void*ctx){return ActorControllerInitReturnSelected(u,q,e,n,current,ctx,0);}

/* 4D7E0 absent/numeric target, immediately-returning INIT branch. This does not
 * run deferred INIT children, Think2, perception, spawned or count/time tail.
 * Original INIT is executed here; caller-provided completion is never used. */
int G_Cod1ActorSpawnPostinitSession(GscSession*s,uint64_t id,const GscEntityRef*source,
 const cod1ActorLease_t*input,GscEntityScriptRun*out,char*error,size_t cap){
 C1SpawnControllerScope*f=&cod1SpawnControllerScope;GscEntityRef src;
 cod1ActorLease_t lease;GscEntityScriptRun run;GscActorInitReturnRequest q;
 const C1GARecord*g;uint64_t program;int ok=0;
 if(f->active){f->cancelled=1;return 0;}
 if(!s||!source||!input||!out||!ActorGoalBuffers(input,sizeof(*input),out,sizeof(*out),error,cap)||
    ActorGoalPrivate(source,sizeof(*source))||
    !gsc_session_spawn_entry_storage_separate(s,source,sizeof(*source))||
    !gsc_session_spawn_entry_storage_separate(s,input,sizeof(*input))||
    !gsc_session_spawn_entry_storage_separate(s,out,sizeof(*out))||
    !gsc_session_spawn_entry_storage_separate(s,error,cap)||
    ActorOverlap(source,sizeof(*source),out,sizeof(*out))||ActorOverlap(input,sizeof(*input),out,sizeof(*out))||
    ActorOverlap(error,cap,source,sizeof(*source)))return 0;
 src=*source;lease=*input;memset(&run,0,sizeof(run));
 if(!G_Cod1ActorSpawnInitSession(s,id,&src,&lease,&run,error,cap)){*out=run;return 0;}
 *out=run;
 if(run.status!=GSC_ENTITY_ENTRY_RETURNED||!run.task_serial||!ActorRefEqual(&run.entity,&lease.entity)){
  EntityError(error,cap,"spawn postinit requires its own returned INIT; deferred continuation remains open");return 0;}
 program=gsc_session_spawn_issued_owner_program(s,id,&src,&lease.entity,lease.serial);
 if(!program||!ActorLeaseValid(&lease)){EntityError(error,cap,"spawn postinit owner retired after INIT");return 0;}
 g=cod1GenericActors+lease.actorIndex;
 if(!g->used||!g->runtime||!g->catalog||memcmp(&g->lease,&lease,sizeof(lease))){
  EntityError(error,cap,"spawn postinit requires its actual INIT animation owner");return 0;}
 memset(&q,0,sizeof(q));q.entity=lease.entity;q.runtime_identity=g->runtime;
 q.catalog_identity=g->catalog;q.task_serial=run.task_serial;q.now_ms=level.time;
 memset(f,0,sizeof(*f));f->session=s;f->id=id;f->program=program;f->source=src;
 f->lease=lease;f->run=run;f->seal=q;f->borrowed=&q;f->now=level.time;f->active=1;
 if(ActorSpawnControllerCurrent(f)&&ActorControllerInitReturnSelected(NULL,&q,error,cap,
    ActorSpawnControllerCurrent,f,1)&&ActorSpawnControllerCurrent(f)){
  /* Callback-free original 4D89E/4D8A4 entity stores after the actual row4
   * handler. Do not mark Actor initialized or emit any script notification. */
  cod1Actors[lease.actorIndex].nativeData.entity.cell01a4=(uint32_t)f->now;
  cod1Actors[lease.actorIndex].nativeData.entity.cell01a8=2;
  /* Original6AF28 peer inheritance follows the completed4D7E0 stores.
   * This private scope still owns the actual returned INIT/controller. */
  if(ActorSpawnControllerCurrent(f)&&ActorObservationInheritPeers(&lease)){
   if(ActorSpawnControllerCurrent(f)&&ActorSpawnNative6AA70(&lease,error,cap)&&
      ActorSpawnControllerCurrent(f)&&ActorSpawnNative6BA60(&lease,error,cap)&&
      ActorSpawnControllerCurrent(f))ok=1;
  }else EntityError(error,cap,"spawn postinit peer inheritance requires current typed history provenance");
 }
 memset(f,0,sizeof(*f));
 if(!ok&&error&&cap&&!error[0])EntityError(error,cap,"spawn postinit returned INIT; current row4 controller or peer inheritance refused");
 return ok;
}

/* Actual callbacks ONLY. Source entry/controller Think is not a callback
 * substitute here. The selected initial push/pop each have exact fixed shape. */
/* DC40 consumes the genuine reduced replace command (operation 1, state 9).
 * The old session MAIN is separately released by the existing 57080 service. */
static int InstructorReplaceRow(const C1ControllerOperation *o,const cod1ActorRecord_t *a){
 const C1NormalRecord *n=o->recordPointerBefore;
 const cod1ActorLease_t *l=&o->lease;
 const gentity_t *e=o->entity;
 return o->kind==C1CTRL_SELECT && n && n==cod1NormalRecords[l->actorIndex] &&
   n->present && n->revision && !memcmp(&n->lease,l,sizeof(*l)) &&
   n->runtime==o->q.step.runtime_identity && n->catalog==o->q.step.catalog_identity &&
   n->resources==o->resourceRevision && e->targetname && (!strcmp(e->targetname,"moody")||!strcmp(e->targetname,"foley")) &&
   e->classname && !strcmp(e->classname,"actor_ally_airborne_officer_thompson") &&
   (ActorControllerFreshDomain(a)||InstructorCourseGoal(o,a)) &&
   a->scriptedController.entered && !a->scriptedController.invalid && !a->scriptedController.resumed4 &&
   a->scriptedController.actorSerial==l->serial && a->scriptedController.selectionSerial &&
   a->scriptedController.currentSource==4 && !a->scriptedController.force250 &&
   a->turretMount.currentTop==1 && a->turretMount.currentStack[0]==4 && a->turretMount.currentStack[1]==9 &&
   a->critical.top==1 && a->critical.stack[0]==4 && a->critical.stack[1]==9 &&
   a->critical.command_count==1 && a->critical.commands[0].operation==1 && a->critical.commands[0].state==9 &&
   a->scriptedAux.prefix==FRESH_PREFIX_QUEUE_COMMITTED && a->scriptedAux.record==FRESH_AUX_OWNED &&
   a->scriptedAux.link==FRESH_LINK_UNATTACHED && a->scriptedAux.revision<UINT64_MAX;
}
static int ActorControllerStateCallback(void*context,int phase,int state,int related,int*out){
 C1ControllerOperation*o=context;cod1ActorRecord_t*a=o->stage;(void)related;
 if(!out)return 0;
 if(related==9 && state==9 && InstructorReplaceRow(o,a)){
  if(phase==COD1_ACTOR_STATE_EXIT){*out=0;return 1;}
  if(phase==COD1_ACTOR_STATE_ENTER){ActorControllerRouteEmptyEnter(a);a->scriptedController.force250=1;*out=0;return 1;}
 }
 if(!ActorControllerFreshDomain(a)&&!TrainingJumperLanding(o,a)){
  if(!(((phase==COD1_ACTOR_STATE_SUSPEND&&state==4)||(phase==COD1_ACTOR_STATE_ENTER&&state==9))&&FoleyArrivedSelect(o,a)) &&
     !(((phase==COD1_ACTOR_STATE_EXIT&&state==9)||(phase==COD1_ACTOR_STATE_RESUME&&state==4))&&!cod1NormalRecords[o->lease.actorIndex]&&FoleyArrivedBody(o,a)))return 0;
 }
 if(phase==COD1_ACTOR_STATE_SUSPEND&&state==4){*out=0;return 1;}
 if(phase==COD1_ACTOR_STATE_ENTER&&state==9){
  ActorControllerRouteEmptyEnter(a);a->scriptedController.force250=1;*out=0;return 1;}
 if(phase==COD1_ACTOR_STATE_EXIT&&state==9){
  /* Exact native NULL-record branch. No notify/unref/NamedCancel or fakefree. */
  if(cod1NormalRecords[o->lease.actorIndex]||a->scriptedAux.record!=FRESH_AUX_EMPTY)return 0;
  *out=0;return 1;}
 if(phase==COD1_ACTOR_STATE_RESUME&&state==4){*out=0;return 1;}
 return 0;
}
static void ActorControllerActualRead(const cod1ActorRecord_t*a,cod1ActorActualState_t*out){
 memset(out,0,sizeof(*out));out->top=a->turretMount.currentTop;
 memcpy(out->stack,a->turretMount.currentStack,sizeof(out->stack));
 out->base_entered_ms=a->scriptedController.base_entered_ms;
}
static void ActorControllerActualCommit(cod1ActorRecord_t*a,const cod1ActorActualState_t*in){
 a->turretMount.currentTop=in->top;memcpy(a->turretMount.currentStack,in->stack,sizeof(in->stack));
 a->scriptedController.base_entered_ms=in->base_entered_ms;
}
#include "g_cod1_actor_locomotion_select.inc"
int G_Cod1ActorSelectWithCleanup(void*u,const GscActorControllerRequest*input,
 GscActorControllerSelection*out,char*error,size_t cap,int(*current)(void*),void*ctx,const GscMainReplacementService*service){
 C1ControllerOperation*o=&cod1ControllerOperation;cod1ActorRecord_t*a;cod1ActorScriptedController_t*m;
 cod1ActorActualState_t actual;cod1ActorCriticalState_t queue;cod1ActorStateCallbacks_t cb;
 GscActorControllerSelection result;GscEntityRef ref;int changed,replaced=0,ok=0;(void)u;
 if(input&&input->locomotion_source)return ActorLocomotionSelect(input,out,error,cap,current,ctx,service);
 if(!input||!out||!ActorGoalBuffers(input,sizeof(*input),out,sizeof(*out),error,cap)||
    ActorOverlap(input,sizeof(*input),out,sizeof(*out)))return 0;ref=input->entity;
 if(!ActorControllerBegin(input,sizeof(*input),C1CTRL_SELECT,&ref,input->now_ms,current,ctx))return 0;
 a=o->stage;m=&a->scriptedController;
 if(input->actor_serial!=o->lease.serial||!m->entered||m->runtime!=input->runtime_identity||
    m->catalog!=input->catalog_identity||(!ActorControllerFreshDomain(a)&&!TrainingJumperLanding(o,a)&&!FoleyArrivedSelect(o,a)&&!FoleyArrivedBody(o,a)&&!InstructorReplaceRow(o,a))||
    !ActorControllerClosedFrame(o)||a->scriptStateBusy||!C1AST14_State(&a->scriptState))goto done;
 ActorControllerActualRead(a,&actual);queue=ActorStateQueueKernel(a);
 if(m->resumed4 && (!m->selectionSerial||m->selectionSerial==UINT64_MAX||m->actorSerial!=o->lease.serial||
    m->currentSource!=4||m->force250||m->currentCategory!=4||m->lastSelectionFrame>=input->frame_generation||
    actual.top!=0||actual.stack[0]!=4||o->recordPointerBefore||a->scriptedAux.link!=FRESH_LINK_UNATTACHED))goto done;
 if(actual.top==0&&actual.stack[0]==4){
  if(queue.command_count!=1||queue.commands[0].operation!=2||queue.commands[0].state!=9||
     queue.top!=1||queue.stack[0]!=4||queue.stack[1]!=9||
     a->scriptedAux.prefix!=FRESH_PREFIX_QUEUE_COMMITTED||a->scriptedAux.record!=FRESH_AUX_EMPTY)goto done;
  cb.context=o;cb.invoke=ActorControllerStateCallback;
  if(!Cod1ActorStateConsume(&actual,&queue,o->now,&cb)||actual.top!=1||actual.stack[0]!=4||actual.stack[1]!=9)goto done;
  ActorControllerActualCommit(a,&actual);ActorStateQueueCommit(a,&queue);
 }else if(InstructorReplaceRow(o,a)){
  /* 57610..57634 frees only the old 0x68-byte record and clears its
   * entity pointer. No end or completion is sent for an interrupted clip. */
  if(!service || !service->run || m->selectionSerial==UINT64_MAX ||
     m->lastSelectionFrame>=input->frame_generation || !ActorControllerCurrent(o))goto done;
  cb.context=o;cb.invoke=ActorControllerStateCallback;
  if(!Cod1ActorStateConsume(&actual,&queue,o->now,&cb) || actual.top!=1 || actual.stack[1]!=9 ||
     !m->force250 || queue.command_count)goto done;
  ActorControllerActualCommit(a,&actual);ActorStateQueueCommit(a,&queue);
  cod1NormalRecords[o->lease.actorIndex]=NULL;free(o->recordPointerBefore);o->recordPointerBefore=NULL;
  a->scriptedAux.record=FRESH_AUX_EMPTY;++a->scriptedAux.revision;
  *o->before=cod1Actors[o->lease.actorIndex]=*a;replaced=1;
 }else if(actual.top!=1||actual.stack[0]!=4||actual.stack[1]!=9||queue.command_count||
          queue.top!=actual.top||queue.stack[0]!=4||queue.stack[1]!=9)goto done;
 changed=m->force250||m->currentSource!=4;
 /* On restart, native 57080 retires the retained MAIN before clearing tracking. */
 if(changed){
  int repeat=m->resumed4 || replaced;
  /* The service exists only for an actual retained reference. The initial
   * stop reference has selection zero; native57080 depends on that reference,
   * independently of the old source/category request producers. */
  int cleanup=repeat || (!m->currentSource&&!m->selectionSerial&&service&&service->run);
  uint64_t selection=m->selectionSerial;
  if(repeat){
   if(!service||!service->run||!m->force250||actual.top!=1||actual.stack[1]!=9||queue.command_count||
      m->revision>=UINT64_MAX-1||!service->run(service->context,0,selection,ActorControllerCurrent,o)||
      !ActorControllerCurrent(o))goto done;
  }else if(m->currentSource||selection)goto done;
  else if(cleanup && (m->revision>=UINT64_MAX-1||
          !service->run(service->context,0,selection,ActorControllerCurrent,o)||!ActorControllerCurrent(o)))goto done;
  m->currentCategory=4;m->force250=0;
  a->defaults.allocation.cell0134=0;a->nativeInteraction1170=0;a->nativeMode264=0;
  a->nativeData.entity.nativeEntityFlags&=~UINT32_C(0x10000);
  if(cleanup){
   /* Native 57127..57149 precede 57080. Seal the actual intermediate actor before calling the VM. */
   ++m->revision;
   cod1Actors[o->lease.actorIndex]=*a;*o->before=*a;
   if(!ActorControllerCurrent(o)||!service->run(service->context,1,selection,ActorControllerCurrent,o)||
      !ActorControllerCurrent(o))goto done;
   m->resumed4=0;
  }
  if(!C1AST14_MainSelected(&a->scriptState))goto done;
  m->currentSource=4;m->selectionSerial=selection+1;
  m->lastSelectionFrame=input->frame_generation;
 }else if(!m->selectionSerial)goto done;
 ++m->revision;memset(&result,0,sizeof(result));result.certificate.owner=o->lease.entity;
 result.certificate.serial=o->lease.serial;result.certificate.current=ActorInheritedCertificate;
 result.selection_serial=m->selectionSerial;result.source_changed=changed;
 if(!ActorControllerCurrent(o))goto done;
 cod1Actors[o->lease.actorIndex]=*a;*out=result;ok=1;
 done:ActorControllerEnd();return ok;
}
static int ActorControllerSelectMain(void*u,const GscActorControllerRequest*q,GscActorControllerSelection*out,char*error,size_t cap,int(*current)(void*),void*ctx){
 return G_Cod1ActorSelectWithCleanup(u,q,out,error,cap,current,ctx,NULL);
}
/* Precompute the entire genuine13590 output using the actual sampled leaf
 * and native-tested C1ASR14 math. This never mutates a live record/tree. */
static int ActorControllerPrepareRecordUpdate(C1ControllerOperation*o){
 C1ControllerExpected*e;C1NormalRecord*n;const c1atl14Node_t*node;
 c1arc14Result_t sample;c1asr14Pose_t pose;float rate;unsigned clip;int finished;
 if(!ActorControllerExpectedBase(o))return 0;e=o->expected;n=&e->record;
 if(!e->recordPointer||n->resources!=o->resourceRevision||n->runtime!=e->generic.runtime||
    n->catalog!=e->generic.catalog||memcmp(&n->lease,&o->lease,sizeof(o->lease))||
    o->now<n->created||o->now<n->updated||!e->nativeStaticPresent||!e->nativeStatic.extrasPresent||
    e->actor.scriptedAux.record!=FRESH_AUX_OWNED||e->actor.scriptedAux.link!=FRESH_LINK_UNATTACHED)return 0;
 clip=n->value.clip;
 if(!clip){
  if(e->actor.scriptedAux.revision==UINT64_MAX)return 0;
  e->recordPointer=NULL;e->present=0;e->committed=0;
  e->actor.scriptedAux.record=FRESH_AUX_EMPTY;++e->actor.scriptedAux.revision;
  return 1;
 }
 if(clip>=C1GA_COUNT||n->value.mode||!n->present||e->frame.count||
    e->generic.tick!=o->now||!(e->generic.nodes[clip].goalWeight>0)||
    !ActorNormalLeafSample(clip,n->clipNative,e->generic.nodes+clip,&sample))return 0;
 memcpy(&rate,&e->nativeStatic.stored.anglelerprateBits,4);
 if(!C1ASR14_UpdatePose(&n->value,sample.q2,sample.translation,rate,&pose))return 0;
 if(!n->value.firstUpdate)n->value.firstUpdate=1;
 else{
  node=e->generic.nodes+clip;
  finished=C1ASR14_Finished(1,node->phase,node->oldPhase,(uint16_t)node->cycle,(uint16_t)node->oldCycle);
  if(finished<0)return 0;
  if(finished){
   if(!C1AGT14_SetAnim(cod1GenericDefinitions,e->generic.nodes,C1GA_COUNT,clip,1,.2f,1,0))return 0;
   ActorGenericNamedCancelSubtree(&e->generic,clip);n->value.clip=0;
  }
 }
 if(n->revision==UINT64_MAX||e->generic.revision==UINT64_MAX)return 0;
 ++n->revision;n->updated=o->now;++e->generic.revision;e->present=1;e->committed=1;
 memcpy(e->entity.r.currentOrigin,pose.origin,12);memcpy(e->entity.r.currentAngles,pose.angles,12);
 memcpy(e->actor.nativeData.entity.origin,pose.origin,12);memcpy(e->actor.nativeData.entity.angles,pose.angles,12);
 e->nativeStatic.sourceCertified=0;e->nativeStatic.everChanged=1;
 return 1;
}
static int ActorControllerRealRecordUpdate(C1ControllerOperation*o,int*present,char*error,size_t cap){
 GscEntityRef owner=o->lease.entity;int returned=-1,called,finished,frames;
 if(!ActorControllerPrepareRecordUpdate(o))return 0;frames=o->expected->committed;
 if(!ActorControllerCurrent(o)||!ActorControllerProviderArm(C1CP_RECORD_UPDATE,&owner,o->now,ActorControllerCurrent,o))return 0;
 called=G_Cod1ActorScriptedRecordUpdate(&owner,o->now,&returned,error,cap,ActorControllerCurrent,o);
 finished=ActorControllerProviderFinish(C1CP_RECORD_UPDATE,frames);
 if(!called||!finished||returned!=o->expected->present||!ActorControllerExpectedMatches(o))return 0;
 *present=returned;ActorControllerExpectedRebase(o,1);return ActorControllerCurrent(o);
}
#include "foley_state9_tail.inc"
static int ActorControllerNormalUpdate(void*u,const GscActorControllerRequest*input,int*outFrontier,
 char*error,size_t cap,int(*current)(void*),void*ctx){
 C1ControllerOperation*o=&cod1ControllerOperation;cod1ActorRecord_t*a;cod1ActorScriptedController_t*m;
 cod1ActorActualState_t actual;cod1ActorCriticalState_t queue;cod1ActorStateCallbacks_t cb;
 GscEntityRef ref;int present,ok=0;(void)u;
 vec3_t oldOrigin,oldAngles;
 if(!input||!outFrontier||!ActorGoalBuffers(input,sizeof(*input),outFrontier,sizeof(*outFrontier),error,cap)||
    ActorOverlap(input,sizeof(*input),outFrontier,sizeof(*outFrontier)))return 0;ref=input->entity;
 if(!ActorControllerBegin(input,sizeof(*input),C1CTRL_UPDATE,&ref,input->now_ms,current,ctx))return 0;
 a=o->stage;m=&a->scriptedController;
 if(input->actor_serial!=o->lease.serial||!m->entered||m->runtime!=input->runtime_identity||
    m->catalog!=input->catalog_identity||m->currentSource!=4||m->force250||!m->selectionSerial||
    m->resumed4||(!ActorControllerFreshDomain(a)&&!TrainingJumperLanding(o,a)&&!FoleyArrivedBody(o,a))||!ActorControllerClosedFrame(o))goto done;
 ActorControllerActualRead(a,&actual);
 if(actual.top!=1||actual.stack[0]!=4||actual.stack[1]!=9||a->critical.command_count)goto done;
 VectorCopy(o->entity->r.currentOrigin,oldOrigin);VectorCopy(o->entity->r.currentAngles,oldAngles);
 present=cod1NormalRecords[o->lease.actorIndex]!=NULL;
 if(present){
  if(a->scriptedAux.record!=FRESH_AUX_OWNED||
     !ActorControllerRealRecordUpdate(o,&present,error,cap))goto done;
  a=o->stage;m=&a->scriptedController;
 }else if(a->scriptedAux.record!=FRESH_AUX_EMPTY||!ActorControllerCurrent(o))goto done;
 if(present){
  int changed=!VectorCompare(oldOrigin,o->entity->r.currentOrigin)||
              !VectorCompare(oldAngles,o->entity->r.currentAngles);
  int result=ActorState9Tail(o,changed,error,cap);
  *outFrontier=result?result:GSC_ACTOR_CONTROLLER_STATE9_TAIL_HELD;
  ok=1;goto done;
 }
 queue=ActorStateQueueKernel(a);
 if(queue.top!=1||queue.stack[0]!=4||queue.stack[1]!=9||queue.command_count||
    Cod1ActorScriptedQueuePop(&queue)!=1)goto done;
 cb.context=o;cb.invoke=ActorControllerStateCallback;
 if(!Cod1ActorStateConsume(&actual,&queue,o->now,&cb)||actual.top||actual.stack[0]!=4)goto done;
 ActorControllerActualCommit(a,&actual);ActorStateQueueCommit(a,&queue);
 m->resumed4=1;++m->revision;
 if(!ActorControllerCurrent(o))goto done;
 cod1Actors[o->lease.actorIndex]=*a;*outFrontier=GSC_ACTOR_CONTROLLER_RESUMED4_THINK_HELD;ok=1;
 done:ActorControllerEnd();return ok;
}
const GscActorScriptedControllerHost*G_Cod1ActorScriptedControllerHost(void){return &cod1ControllerHost;}
