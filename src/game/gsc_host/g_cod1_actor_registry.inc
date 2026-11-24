/* Included by g_cod1_entity_host.c, inside its WIP guard. No engine pointers
 * or borrowed strings are retained. Pure pool operations have no callbacks;
 * public release performs the separate R103 checked spatial cleanup first. */
#include "../g_cod1_actor.h"
#include "../g_cod1_actor_critical_core.h"
#include "../g_cod1_actor_scripted_prefix.h"
#include "../g_cod1_actor_script_state.h"
#include "../g_cod1_actor_scripted_controller.h"
typedef char cod1ActorFloatMustBe32[sizeof(float) == sizeof(uint32_t) ? 1 : -1];

#include "cod1_training_run_entry.h"
#define COD1_LOCAL_POINTS 24
typedef struct {
    int length,lastIndex,lastClear;
    int prepared,preparedTick;vec3_t preparedOrigin;
    vec3_t edge[6],base,aim,face;
    float edgeLength[6],area,back,remaining,distance;
} cod1TowerPath_t;
typedef struct {
    cod1TowerPath_t tower;
    int headingValid;vec2_t heading;
    vec3_t points[COD1_LOCAL_POINTS];
    float cost[COD1_LOCAL_POINTS];
    int parent[COD1_LOCAL_POINTS],visited[COD1_LOCAL_POINTS],path[COD1_LOCAL_POINTS];
    int phase,count,from,next,length,cursor,atTraversalStart;
    int diveArmed,diveTime;
    /* The Begin waiter may complete while the runner is still on the ground path. */
    int approachingLadder,approachingStand;
    vec3_t standPoint;
    vec3_t diveStart;
    float divePhase;
    int ladderReady,ladderFrame;
    vec3_t ladderExit;
    float climbSeconds;
    c1CourseEntry_t entry;
    vec2_t turnDirections[10];
    int turnCount,turnNext;
    int restFrame,restFamily,restSeed,restStamp;
    GscEntityRef entryPeer, separationPeer;
    float restRate,restPhase;
    int reciprocal,returning,walkTime,walkFrame;
    float walkPhase;
    int brakeRemaining,stopStarted,stopTick,stopFirstStep,stopFadeTick;
    float terminalSpeed,approachPhase;
    int approachTime;
    int64_t nextRunPoseReview;
} cod1LocalRoute_t;
typedef struct {
    cod1LocalRoute_t route;
    vec3_t velocity;
    int active,busy,model,lastTime,moving,blocked,arrived;
    uint64_t assignment,notified;float travelled,standingTop;
    entityState_t oldState;int oldFlags,oldSvFlags,oldContents;
} cod1ActorMotion_t;
static void ActorMotionStopOwner(const GscEntityRef *);

/* Prospective queue storage only. Original actor+02a4 belongs to the canonical
 * life span below, not to a second independently initialized critical flag. */
typedef struct {
    cod1ActorStateCommand_t commands[COD1_CRITICAL_COMMAND_CAPACITY];
    int32_t command_count,stack[COD1_CRITICAL_STACK_CAPACITY],top;
} cod1ActorQueuedState_t;
#include "../../qcommon/cod1_actor_perception14.h"
typedef struct { uint64_t serial;GscEntityRef owner; } Cod1ObservationTarget;
typedef struct {
    int32_t activation;
    uint64_t serial;
    GscEntityRef owner;
    uint32_t sentientIndex;
    int alive, initialized, dataInitialized, nativeDataInitialized, modelBusy, modelMethodBusy;
    cod1ActorGoalSnapshot_t goal;
    /* Native observation rows belong to the actual allocation lifetime.
     * These cells do not authorize perception callbacks or MAIN selection. */
    c1per14Memory_t observations;
    Cod1ObservationTarget observationTargets[C1PER14_SLOTS];
    cod1ActorMotion_t motion;
    GscNodeRef standingNode;
    float courseRunRate;
    int towerGoalTick;
    struct {GscEntityRef brush;uint64_t frame;int tick;} towerTouch;
    cod1ActorLocalDefaults_t defaults;
    /* One allocation-owned projection of native DWORD264/1170. Original
     * C490 clears all122C bytes before4D710; neither is a ready flag. The
     * actual reserve/reset/retire whole-record clear owns their lifetime.
     * Only selected mode4/6 and570D0-clear producers are admitted; general
     * interaction/path/import/restore producers remain unsupported. */
    uint32_t nativeMode264,nativeInteraction1170;
    /* Actual4C4FC clears122C bytes: F0 is the visibility-query counter,
     * 7C8 the native threat-entry count. Never interpret either as readiness.
     * General visibility/threat writers remain unsupported in this profile. */
    /* Actor field table899A4: type6, pointer1164. C4FC allocation clears
     * all122C bytes. The Actor record's existing reserve/reset/retire owns
     * the null cell. No grenade acquisition/throwback writer is admitted. */
    /* Native allocator-owned readonly damageDir. Unknown input never falls back to0. */
    uint32_t desiredAngle148;int desiredAngleUnavailable;
    /* Native C4C4FC allocation-zero cells138/13C/140. Only admitted
     * OrientMode writers update them; never reconstruct from body angles. */
    uint32_t orientAngles138[3];int orientBodyTick;
    /* GAME66CF0 stores six GetAnim results at154..168. Real allocation
     * reserve/reset clears this record. Catalog identity is port ownership,
     * not native aim readiness; aimAtPos remains a separate consumer. */
    uint32_t aimNodes154[6];uint64_t aimNodesRuntime,aimNodesCatalog;
    int aimNodesSet;
    int32_t damageTaken21c,damageYaw220;uint32_t damageDirection224[3];int damageStateUnavailable;
    /* Native allocator C4FC clears actor+778. Own an Actor reference, not a
     * reusable slot/pointer. General avoidance-chain writers53420/53E.. are
     * not implemented; this profile retains the actual allocation-null cell. */
    Cod1ObservationTarget avoidance778;
    GscEntityRef grenade1164;
    GscEntityRef favoriteEnemy10f8;
    uint32_t nativeVisibilityQueriesF0,nativeThreatCount7c8;
    cod1ActorNativeData_t nativeData;
    cod1ActorScriptStrings_t scriptStrings;
    c1ast14State_t scriptState;int scriptStateBusy;
    cod1ActorTrainingFields_t trainingFields;
    cod1ActorSpatialState_t spatial;
    cod1ActorDObjState_t dobj;
    cod1ActorModelComposition_t modelComposition;
    cod1ActorAssignedModel_t assignedModel;
    cod1ActorRegisteredAttachments_t registeredAttachments;
    cod1ActorQueuedState_t critical;
    cod1ActorTurretMount_t turretMount;
    int criticalBusy,criticalHasLabel,criticalHasScriptLabel;
    char criticalLabel[128],criticalScriptLabel[128];
    cod1ActorScriptedAux_t scriptedAux;
    cod1ActorScriptedController_t scriptedController;
    /* Native allocation cells cleared by the existing whole-record lifetime.
     * positionPostinit records only739A0+107D0, never full postinit readiness. */
    uint32_t rawGoalCell07e4,goalObservationTail[4];unsigned char rawGoalCell078f;
    /* Vector goals do not occupy a path-node receipt. Cleared with this
     * allocation or by the next successful path-node assignment. */
    int stationaryGoal,stationaryGoalTick,courseRestSampled;
    int positionPostinit;
    /* One-shot numeric target prefix consumed before INIT; never readiness.
     * Cleared only with this actual Actor allocation, including failed INIT. */
    int numericTargetApplied;
    int placedInitAttempted; /* One initial invocation, never readiness. */
    int placedTargetApplied; /* Exact allocation one-shot prefix; not readiness. */
    /* Allocation-owned provenance of an ACTUAL healthy source98 factory
     * finish. Not native initialized/render readiness; never caller-writable.
     * Whole-record reserve/retire clears it with this exact Actor lease. */
    struct {uint64_t operation,program,runtime,catalog,serial;} spawnCompletion;
} cod1ActorRecord_t;

static cod1ActorCriticalState_t ActorStateQueueKernel(const cod1ActorRecord_t *a) {
    cod1ActorCriticalState_t state;
    memset(&state,0,sizeof(state));
    memcpy(state.commands,a->critical.commands,sizeof(state.commands));
    state.command_count=a->critical.command_count;
    memcpy(state.stack,a->critical.stack,sizeof(state.stack));state.top=a->critical.top;
    /* Native4FD90/init,307C0/isalive,4DEA0/reducer and4E950/death all use
     * the SAME actual cell. Preserve allocation-before-init0 and death0. */
    memcpy(&state.native_cell02a4,&a->defaults.lifeWords[(0x2a4-0x26c)/4],sizeof(state.native_cell02a4));
    return state;
}
static void ActorStateQueueCommit(cod1ActorRecord_t *a,const cod1ActorCriticalState_t *state) {
    memcpy(a->critical.commands,state->commands,sizeof(a->critical.commands));
    a->critical.command_count=state->command_count;
    memcpy(a->critical.stack,state->stack,sizeof(a->critical.stack));a->critical.top=state->top;
    /* The queue kernel reads life state; it cannot overwrite its owner. */
}
typedef struct {
    int32_t activation;
    uint64_t serial;
    GscEntityRef owner;
    uint32_t actorIndex, kind;
    cod1SentientDefaults_t defaults;
    /* Canonical native72E90 fresh-allocation projection, adopted from the
     * proved WORK owner. Whole-record reserve/reset clears this storage;
     * no native node/path setter or restore is admitted in this profile. */
    struct { GscEntityRef entityGoal;float position[3];uint32_t node0040,node004c; } rawGoal;
    /* Actual whole-record birth/reset/retire clears native54/58. */
    uint32_t observation54;
    unsigned char observation58;
    /* Native72E90 zeroes the complete 0x60 sentient, including +0x34.
     * This is explicit fresh enemy state, not inference from a missing field.
     * Any future enemy controller/setter must own/invalidate this component. */
    int enemyKnown;
    GscEntityRef enemy;
    /* Actual72EED clears all60 bytes, including enemy deadline38. Canonical
     * storage follows this same reserve/reset/retire lifetime and enemy owner.
     * No expired-event or enemy-transition producer is silently simulated. */
    int32_t nativeEnemyDeadline38;
} cod1SentientRecord_t;
static cod1ActorRecord_t cod1Actors[COD1_ACTOR_CAPACITY];
static cod1SentientRecord_t cod1Sentients[COD1_SENTIENT_CAPACITY];
/* Zero means no ownership; otherwise the stored pool index is index+1. */
static unsigned char cod1ActorByEntity[MAX_GENTITIES];
static unsigned char cod1SentientByEntity[MAX_GENTITIES];
static int32_t cod1ActorCounter, cod1SentientCounter;
static uint64_t cod1ActorSerial;
static int32_t cod1ActorCriticalDisabled;
static int cod1ActorCriticalBusy;
static int ActorSpatialUnlinkLease( const cod1ActorLease_t *lease,
    char *error, size_t size );
static int ActorDObjFreeLease( const cod1ActorLease_t *lease,
    char *error, size_t size );
static void ActorModelRegistryReset( void );
static void ActorNativeStaticInvalidateSource(const GscEntityRef *ref);
static void ActorWeaponFieldsReset(void);
static void WeaponModelsReset(void);
static void AnimDataReset(void);
static void AnimationRefsReset(void);
static void ActorPlacedFieldsReset(void);

static uint32_t ActorFloatBits( float value ) {
    uint32_t bits;
    memcpy( &bits, &value, sizeof( bits ) );
    return bits;
}
static float ActorFloatFromBits( uint32_t bits ) {
    float value;
    memcpy( &value, &bits, sizeof( value ) );
    return value;
}
/* Exact 0x2004d710 contract, independently executed in native proof. The
 * remaining native allocation record is zero; sidecar reverse links are
 * represented by full identities above, never truncated original pointers. */
static void ActorAllocationDefaults( cod1ActorAllocatorDefaults_t *d ) {
    memset( d, 0, sizeof( *d ) );
    d->cell0008 = 4; d->cell00a4 = 4; d->cell0120 = 1; d->cell024c = 2;
    d->walkdist = 128.0f; d->followmin = -5; d->followmax = -1;
    d->interval = 96.0f; d->personalspace = 40.0f; d->pacifistwait = 20000;
    d->visibilitythreshold = 0.5f;
    d->fovcosine = ActorFloatFromBits( UINT32_C(0x3c23d70a) );
    d->maxsightdistsqrd = 67108864.0f; d->defaultsightlatency = 250;
    d->suppressionwait = 5000;
    d->grenadeawareness = ActorFloatFromBits( UINT32_C(0x3ea8f5c3) );
}

static int ActorRefEqual( const GscEntityRef *a, const GscEntityRef *b ) {
    return a->epoch == b->epoch && a->slot == b->slot &&
        a->generation == b->generation;
}
static inline int ActorOverlap( const void *a, size_t an, const void *b, size_t bn ) {
    uintptr_t ap = (uintptr_t)a, bp = (uintptr_t)b;
    if ( !a || !b || !an || !bn ) return 0;
    return ap <= bp ? bp - ap < an : ap - bp < bn;
}
static int ActorArguments( const void *input, size_t inputSize,
    const void *output, size_t outputSize, char *error, size_t errorSize ) {
    return !ActorOverlap( input, inputSize, error, errorSize ) &&
        !ActorOverlap( output, outputSize, error, errorSize );
}
#include "g_cod1_actor_scripted_lifecycle.inc"
static void ActorPoolReset( void ) {
    int i;
    ActorFactoryReset();
    ActorControllerReset();
    FreshAuxReset();
    memset( cod1Actors, 0, sizeof( cod1Actors ) );
    memset( cod1Sentients, 0, sizeof( cod1Sentients ) );
    memset( cod1ActorByEntity, 0, sizeof( cod1ActorByEntity ) );
    memset( cod1SentientByEntity, 0, sizeof( cod1SentientByEntity ) );
    for ( i = 0; i < COD1_ACTOR_CAPACITY; ++i ) {
        cod1Actors[i].activation = -1;
        cod1Actors[i].sentientIndex = COD1_ACTOR_NO_INDEX;
    }
    for ( i = 0; i < COD1_SENTIENT_CAPACITY; ++i ) {
        cod1Sentients[i].activation = -1;
        cod1Sentients[i].actorIndex = COD1_ACTOR_NO_INDEX;
    }
    cod1ActorCounter = cod1SentientCounter = -1;
    cod1ActorSerial = 0;
    cod1ActorCriticalDisabled = 0;
    /* Keep the synchronous call guard across map resets during callbacks. */
    ActorModelRegistryReset();
    ActorWeaponFieldsReset();
    WeaponModelsReset();
    AnimDataReset();
    AnimationRefsReset();
    ActorPlacedFieldsReset();
}
/* Native decrement would overflow after enough retirements. The bounded port
 * saturates at INT32_MIN and never reuses that sentinel; it never wraps active. */
static int32_t ActorRetirementStamp( int32_t *counter ) {
    if ( *counter > INT32_MIN ) --*counter;
    return *counter;
}
static void ActorRetireSentient( uint32_t index ) {
    cod1SentientRecord_t *s = &cod1Sentients[index];
    unsigned i;
    /* 73031..73068. Teammates inherit the retiring Sentient's target.
     * Row retirement in 4D9D0 clears its first word, preserving the tail. */
    for(i=0;i<COD1_SENTIENT_CAPACITY;i++){
        cod1SentientRecord_t *other=cod1Sentients+i;
        if(i==index || other->activation<0)continue;
        if(ActorRefEqual(&other->rawGoal.entityGoal,&s->owner)){
            if(other->defaults.team==s->defaults.team)
                other->rawGoal.entityGoal=s->rawGoal.entityGoal;
            else memset(&other->rawGoal.entityGoal,0,sizeof(other->rawGoal.entityGoal));
        }
        if(other->kind==COD1_ACTOR_LEASE_ACTOR && other->actorIndex<COD1_ACTOR_CAPACITY){
            cod1ActorRecord_t *observer=cod1Actors+other->actorIndex;
            observer->observations.row[index].word[0]=0;
            memset(observer->observationTargets+index,0,sizeof(observer->observationTargets[index]));
        }
    }
    if ( s->owner.slot < MAX_GENTITIES &&
         cod1SentientByEntity[s->owner.slot] == index + 1 )
        cod1SentientByEntity[s->owner.slot] = 0;
    memset( s, 0, sizeof( *s ) );
    s->actorIndex = COD1_ACTOR_NO_INDEX;
    s->activation = ActorRetirementStamp( &cod1SentientCounter );
}
static void ActorRetireActor( uint32_t index ) {
    cod1ActorRecord_t *a = &cod1Actors[index];
    ActorControllerRetire(index,&a->owner);
    FreshAuxRetire(index,&a->owner);
    ActorGenericRetire(index,&a->owner);
    TurretMountRetire(&a->owner);
    if ( a->owner.slot < MAX_GENTITIES &&
         cod1ActorByEntity[a->owner.slot] == index + 1 )
        cod1ActorByEntity[a->owner.slot] = 0;
    if ( a->sentientIndex < COD1_SENTIENT_CAPACITY ) {
        cod1SentientRecord_t *s = &cod1Sentients[a->sentientIndex];
        if ( s->serial == a->serial && ActorRefEqual( &s->owner, &a->owner ) &&
             s->actorIndex == index ) ActorRetireSentient( a->sentientIndex );
    }
    memset( a, 0, sizeof( *a ) );
    a->sentientIndex = COD1_ACTOR_NO_INDEX;
    a->activation = ActorRetirementStamp( &cod1ActorCounter );
}
static void ActorRetireEntity( const GscEntityRef *owner ) {
    uint32_t index;
    if ( !owner || owner->slot >= MAX_GENTITIES ) return;
    if ( cod1ActorByEntity[owner->slot] ) {
        index = cod1ActorByEntity[owner->slot] - 1;
        if ( ActorRefEqual( owner, &cod1Actors[index].owner ) )
            ActorRetireActor( index );
    }
    if ( cod1SentientByEntity[owner->slot] ) {
        index = cod1SentientByEntity[owner->slot] - 1;
        if ( ActorRefEqual( owner, &cod1Sentients[index].owner ) )
            ActorRetireSentient( index );
    }
}
static int ActorMapAvailable( void ) {
    return cod1EntityActive && cod1EntityEpoch &&
        G_Cod1MapEntities() && g_gametype.integer == GT_SINGLE_PLAYER &&
        level.num_entities >= MAX_CLIENTS && level.num_entities <= MAX_GENTITIES;
}
static int ActorPickActor( void ) {
    int i, selected = -1;
    int32_t rank = INT32_MIN;
    for ( i = 0; i < COD1_ACTOR_CAPACITY; ++i ) {
        int32_t stamp = cod1Actors[i].activation;
        if ( stamp == -1 ) return i;
        if ( stamp < 0 && stamp > rank ) { rank = stamp; selected = i; }
    }
    return selected;
}
static int ActorPickSentient( void ) {
    int i, selected = -1;
    int32_t rank = INT32_MIN;
    for ( i = 0; i < COD1_SENTIENT_CAPACITY; ++i ) {
        int32_t stamp = cod1Sentients[i].activation;
        if ( stamp == -1 ) return i;
        if ( stamp < 0 && stamp > rank ) { rank = stamp; selected = i; }
    }
    return selected;
}
static int ActorOwnerAllowed( const GscEntityRef *ref, int player ) {
    const gentity_t *e;
    if ( !ActorMapAvailable() || !G_Cod1EntitySameAllocation( ref ) ||
         level.time < 0 || cod1ActorSerial == UINT64_MAX ) return 0;
    e = &g_entities[ref->slot];
    if ( AmmoCandidateOwned( ref ) || cod1ActorByEntity[ref->slot] || cod1SentientByEntity[ref->slot] ||
         ( e->r.svFlags & ( SVF_CASTAI | SVF_BOT ) ) || e->aiCharacter ||
         e->aiAttributes || e->AIScript_AlertEntity || e->aiInactive ) return 0;
    if ( !player ) return ref->slot >= MAX_CLIENTS &&
        ref->slot < ENTITYNUM_MAX_NORMAL && !e->client && !e->aiName;
    return ref->slot < MAX_CLIENTS && level.clients &&
        level.maxclients > 0 && level.maxclients <= MAX_CLIENTS &&
        ref->slot < (uint32_t)level.maxclients &&
        e->client == &level.clients[ref->slot] &&
        e->client->pers.connected == CON_CONNECTED && !e->client->ps.aiChar &&
        ( !e->aiName || !strcmp( e->aiName, "player" ) );
}
static int ActorReserve( const GscEntityRef *entity, cod1ActorLease_t *out,
    int player, char *error, size_t size ) {
    GscEntityRef ref;
    cod1ActorLease_t lease;
    int ai = -1, si;
    if ( !ActorArguments( entity, sizeof( *entity ), out, sizeof( *out ), error, size ) ) return 0;
    if ( !entity || !out ) { EntityError( error, size, "actor reservation requires identity and output" ); return 0; }
    ref = *entity;
    if ( !ActorOwnerAllowed( &ref, player ) ) {
        EntityError( error, size, "actor reservation requires an unowned current CoD SP allocation and nonnegative time" ); return 0;
    }
    if ( !player ) {
        ai = ActorPickActor();
        if ( ai < 0 ) { EntityError( error, size, "actor pool exhausted; native forced eviction is unsupported" ); return 0; }
        memset( &cod1Actors[ai], 0, sizeof( cod1Actors[ai] ) );
        cod1Actors[ai].activation = level.time;
        cod1Actors[ai].sentientIndex = COD1_ACTOR_NO_INDEX;
        ActorAllocationDefaults( &cod1Actors[ai].defaults.allocation );
        /* Native allocator zeroes queue/top and writes state4 at +a4. */
        cod1Actors[ai].critical.stack[0] = cod1Actors[ai].defaults.allocation.cell00a4;
    }
    si = ActorPickSentient();
    if ( si < 0 ) {
        if ( ai >= 0 ) ActorRetireActor( (uint32_t)ai );
        EntityError( error, size, "sentient pool exhausted; provisional actor rolled back" ); return 0;
    }
    memset( &lease, 0, sizeof( lease ) );
    lease.entity = ref;
    lease.serial = ++cod1ActorSerial;
    lease.actorIndex = ai < 0 ? COD1_ACTOR_NO_INDEX : (uint32_t)ai;
    lease.sentientIndex = (uint32_t)si;
    lease.kind = player ? COD1_ACTOR_LEASE_PLAYER : COD1_ACTOR_LEASE_ACTOR;
    memset( &cod1Sentients[si], 0, sizeof( cod1Sentients[si] ) );
    cod1Sentients[si].enemyKnown=1;
    cod1Sentients[si].activation = level.time;
    cod1Sentients[si].serial = lease.serial;
    cod1Sentients[si].owner = ref;
    cod1Sentients[si].actorIndex = lease.actorIndex;
    cod1Sentients[si].kind = lease.kind;
    cod1SentientByEntity[ref.slot] = (unsigned char)(si + 1);
    if ( ai >= 0 ) {
        cod1Actors[ai].serial = lease.serial;
        TurretMountFresh((uint32_t)ai,lease.serial);
        cod1Actors[ai].owner = ref;
        cod1Actors[ai].sentientIndex = (uint32_t)si;
        cod1ActorByEntity[ref.slot] = (unsigned char)(ai + 1);
        /* Exclusive canonical representation construction at genuine Actor
         * birth. This is not a read of original raw entity29C/294. */
        if(!FreshAuxConstruct(cod1Actors+ai,&lease)){
            ActorRetireActor((uint32_t)ai);
            EntityError(error,size,"fresh scripted auxiliary owner construction refused");return 0;
        }
        ActorControllerFresh((uint32_t)ai,lease.serial);
    }
    ActorNativeStaticInvalidateSource(&ref);
    *out = lease;
    return 1;
}
int G_Cod1ActorReserve( const GscEntityRef *entity, cod1ActorLease_t *out,
    char *error, size_t size ) { return ActorReserve( entity, out, 0, error, size ); }
int G_Cod1PlayerSentientReserve( const GscEntityRef *entity, cod1ActorLease_t *out,
    char *error, size_t size ) { return ActorReserve( entity, out, 1, error, size ); }
static int ActorLeaseRecordsValid( const cod1ActorLease_t *lease ) {
    const cod1SentientRecord_t *s;
    const cod1ActorRecord_t *a;
    if ( !lease->serial ||
         lease->sentientIndex >= COD1_SENTIENT_CAPACITY ) return 0;
    s = &cod1Sentients[lease->sentientIndex];
    if ( s->activation < 0 || s->serial != lease->serial ||
         !ActorRefEqual( &s->owner, &lease->entity ) ||
         s->kind != lease->kind || s->actorIndex != lease->actorIndex ||
         cod1SentientByEntity[lease->entity.slot] != lease->sentientIndex + 1 ) return 0;
    if ( lease->kind == COD1_ACTOR_LEASE_PLAYER )
        return lease->actorIndex == COD1_ACTOR_NO_INDEX;
    if ( lease->kind != COD1_ACTOR_LEASE_ACTOR ||
         lease->actorIndex >= COD1_ACTOR_CAPACITY ) return 0;
    a = &cod1Actors[lease->actorIndex];
    return a->activation >= 0 && a->serial == lease->serial &&
        ActorRefEqual( &a->owner, &lease->entity ) &&
        a->sentientIndex == lease->sentientIndex &&
        cod1ActorByEntity[lease->entity.slot] == lease->actorIndex + 1;
}
static int ActorLeaseValid( const cod1ActorLease_t *lease ) {
    return lease && ActorMapAvailable() && G_Cod1EntitySameAllocation(&lease->entity) &&
           ActorLeaseRecordsValid(lease);
}
#include "g_cod1_actor_observation.inc"

int G_Cod1ActorLeaseCurrent( const cod1ActorLease_t *lease ) {
    return ActorLeaseValid( lease );
}
int G_Cod1ActorLinkedAndAlive(const cod1ActorLease_t *lease){
    const cod1ActorRecord_t *actor;
    if(!ActorLeaseValid(lease) || lease->actorIndex>=COD1_ACTOR_CAPACITY)return 0;
    actor=&cod1Actors[lease->actorIndex];
    return actor->alive && actor->nativeDataInitialized && actor->spatial.linked;
}
int G_Cod1ActorReadLease( const cod1ActorLease_t *input,
    cod1ActorLeaseSnapshot_t *out, char *error, size_t size ) {
    cod1ActorLease_t lease;
    cod1ActorLeaseSnapshot_t copy;
    if ( !ActorArguments( input, sizeof( *input ), out, sizeof( *out ), error, size ) ) return 0;
    if ( !input || !out ) { EntityError( error, size, "actor lease read requires identity and output" ); return 0; }
    lease = *input;
    if ( !ActorLeaseValid( &lease ) ) { EntityError( error, size, "actor lease is stale or not owned" ); return 0; }
    memset( &copy, 0, sizeof( copy ) );
    copy.lease = lease;
    copy.actorActivation = lease.actorIndex < COD1_ACTOR_CAPACITY ?
        cod1Actors[lease.actorIndex].activation : -1;
    copy.sentientActivation = cod1Sentients[lease.sentientIndex].activation;
    if ( lease.actorIndex < COD1_ACTOR_CAPACITY ) {
        copy.alive = cod1Actors[lease.actorIndex].alive;
        copy.initialized = cod1Actors[lease.actorIndex].initialized;
        copy.dataInitialized = cod1Actors[lease.actorIndex].dataInitialized;
        copy.nativeDataInitialized = cod1Actors[lease.actorIndex].nativeDataInitialized;
        copy.defaults = cod1Actors[lease.actorIndex].defaults;
        copy.nativeData = cod1Actors[lease.actorIndex].nativeData;
        copy.scriptStrings = cod1Actors[lease.actorIndex].scriptStrings;
        copy.spatial = cod1Actors[lease.actorIndex].spatial;
        copy.dobj = cod1Actors[lease.actorIndex].dobj;
    }
    copy.sentientDefaults = cod1Sentients[lease.sentientIndex].defaults;
    *out = copy;
    return 1;
}
int G_Cod1ActorReadPool( cod1ActorPoolSnapshot_t *out, char *error, size_t size ) {
    cod1ActorPoolSnapshot_t copy;
    int i;
    if ( !ActorArguments( NULL, 0, out, sizeof( *out ), error, size ) ) return 0;
    if ( !out || !ActorMapAvailable() ) { EntityError( error, size, "actor pool requires a fresh CoD SP map" ); return 0; }
    memset( &copy, 0, sizeof( copy ) );
    for ( i = 0; i < COD1_ACTOR_CAPACITY; ++i ) {
        copy.actorActivation[i] = cod1Actors[i].activation;
        if ( copy.actorActivation[i] >= 0 ) ++copy.actorReserved;
    }
    for ( i = 0; i < COD1_SENTIENT_CAPACITY; ++i ) {
        copy.sentientActivation[i] = cod1Sentients[i].activation;
        if ( copy.sentientActivation[i] >= 0 ) {
            ++copy.sentientReserved;
            if ( cod1Sentients[i].kind == COD1_ACTOR_LEASE_PLAYER ) ++copy.playerReserved;
        }
    }
    copy.actorRetirementCounter = cod1ActorCounter;
    copy.sentientRetirementCounter = cod1SentientCounter;
    *out = copy;
    return 1;
}
int G_Cod1ActorCollectReservations( cod1ActorReservationView_t *out,
    size_t capacity, size_t *count, char *error, size_t size ) {
    cod1ActorReservationView_t copy[COD1_ACTOR_CAPACITY];
    size_t bytes, used = 0;
    uint64_t epoch;
    int i;
    /* Reject impossible sizes before calculating byte spans. Never write a
     * diagnostic through an aliased count/output buffer. */
    if ( capacity > COD1_ACTOR_CAPACITY ) return 0;
    bytes = capacity * sizeof(*out);
    if ( ActorOverlap(out,bytes,error,size) ||
         ActorOverlap(count,sizeof(*count),error,size) ) return 0;
    if ( !count || (!out && capacity) ||
         ActorOverlap(out,bytes,count,sizeof(*count)) ) {
        EntityError(error,size,"actor reservation collection requires separate bounded outputs"); return 0;
    }
    if ( !ActorMapAvailable() ) {
        EntityError(error,size,"actor reservation collection requires a current CoD SP map"); return 0;
    }
    epoch = cod1EntityEpoch;
    memset(copy,0,sizeof(copy));
    for ( i = 0; i < COD1_ACTOR_CAPACITY; ++i ) {
        const cod1ActorRecord_t *a = &cod1Actors[i];
        const cod1SentientRecord_t *s;
        cod1ActorReservationView_t *row;
        if ( a->activation < 0 ) continue;
        row = &copy[used];
        row->lease.entity = a->owner;
        row->lease.serial = a->serial;
        row->lease.actorIndex = (uint32_t)i;
        row->lease.sentientIndex = a->sentientIndex;
        row->lease.kind = COD1_ACTOR_LEASE_ACTOR;
        if ( !ActorLeaseValid(&row->lease) ) {
            EntityError(error,size,"actor reservation collection found a stale occupied owner"); return 0;
        }
        s = &cod1Sentients[a->sentientIndex];
        row->actorActivation = a->activation;
        row->sentientActivation = s->activation;
        row->team = s->defaults.team;
        row->alive = a->alive;
        row->initialized = a->initialized;
        row->dataInitialized = a->dataInitialized;
        row->nativeDataInitialized = a->nativeDataInitialized;
        ++used;
    }
    if ( used > capacity || epoch != cod1EntityEpoch || !ActorMapAvailable() ) {
        EntityError(error,size,"actor reservation collection requires complete current-pool capacity"); return 0;
    }
    if ( used ) memcpy(out,copy,used*sizeof(*out));
    *count = used;
    return 1;
}
int G_Cod1ActorInitializeLocalDefaults( const cod1ActorLease_t *input,
    char *error, size_t size ) {
    cod1ActorLease_t lease;
    cod1ActorRecord_t *a;
    uint32_t *life, *path;
    static const float bbox[6] = { -15.0f, -15.0f, 0.0f, 15.0f, 15.0f, 72.0f };
    int i;
    if ( !ActorArguments( input, sizeof( *input ), NULL, 0, error, size ) ) return 0;
    if ( !input ) { EntityError( error, size, "actor defaults require a lease" ); return 0; }
    lease = *input;
    if ( !ActorLeaseValid( &lease ) || lease.kind != COD1_ACTOR_LEASE_ACTOR ||
         cod1Actors[lease.actorIndex].dataInitialized ) {
        EntityError( error, size, "actor defaults require a current uninitialized provisional actor lease" ); return 0;
    }
    a = &cod1Actors[lease.actorIndex];
    life = a->defaults.lifeWords; path = a->defaults.pathWords;
    memset( life, 0, sizeof( a->defaults.lifeWords ) );
    /* 0x2004fd90: six bit-preserving bbox values, independently alive cell,
     * checked canonical engine slot as zero-extended native entity index. */
    life[(0x284-0x26c)/4] = UINT32_C(0x7fe);
    life[(0x2a4-0x26c)/4] = 1;
    life[(0x2a8-0x26c)/4] = lease.entity.slot;
    life[(0x2b4-0x26c)/4] = 50;
    for ( i = 0; i < 6; ++i ) life[(0x2b8-0x26c)/4+i] = ActorFloatBits( bbox[i] );
    life[(0x2d0-0x26c)/4] = UINT32_C(0x02820011);
    life[(0x35c-0x26c)/4] = UINT32_C(0x7ff);
    /* 0x20052a00: numeric WORD at +0x6fe is the upper half of its DWORD. */
    memset( path, 0, sizeof( a->defaults.pathWords ) );
    path[(0x6fc-0x374)/4] = UINT32_C(0x07ff0000);
    path[(0x724-0x374)/4] = UINT32_C(0x47000000);
    a->defaults.cell0774 = 0;
    a->alive = 1;
    a->dataInitialized = 1;
    return 1;
}
static int ActorQuantizeAngle( float input, float *out ) {
    /* The product of two finite float32 inputs is exact in double here.
     * Native x87 helper truncates before unsigned low16 wrapping. */
    double scaled = (double)input * (double)ActorFloatFromBits(UINT32_C(0x43360b61));
    int32_t truncated;
    if ( !isfinite(input) || !isfinite(scaled) ||
         scaled < (double)INT32_MIN || scaled > (double)INT32_MAX ) return 0;
    truncated = (int32_t)scaled;
    *out = (float)((uint32_t)truncated & UINT32_C(0xffff)) *
        ActorFloatFromBits(UINT32_C(0x3bb40000));
    return 1;
}
int G_Cod1ActorInitializeNativeData( const cod1ActorLease_t *input,
    uint32_t nativeServerFlags, uint32_t nativeEntityFlags,
    char *error, size_t size ) {
    cod1ActorLease_t lease;
    cod1ActorNativeData_t data;
    cod1SentientDefaults_t sentient;
    cod1ActorRecord_t *a;
    const gentity_t *ent;
    vec3_t lookAngles;
    int i;
    if ( !ActorArguments( input, sizeof(*input), NULL, 0, error, size ) ) return 0;
    if ( !input ) { EntityError(error,size,"native actor data requires a lease"); return 0; }
    lease = *input;
    if ( !ActorLeaseValid(&lease) || lease.kind != COD1_ACTOR_LEASE_ACTOR ) {
        EntityError(error,size,"native actor data requires a current provisional actor lease"); return 0;
    }
    a = &cod1Actors[lease.actorIndex];
    if ( a->nativeDataInitialized || (a->dataInitialized && !a->alive) ) {
        EntityError(error,size,"native actor data cannot reinitialize a completed or dead data stage"); return 0;
    }
    ent = &g_entities[lease.entity.slot];
    memset(&data,0,sizeof(data));memset(&sentient,0,sizeof(sentient));
    for ( i=0; i<3; ++i ) {
        if ( !isfinite(ent->r.currentOrigin[i]) || !isfinite(ent->r.currentAngles[i]) ) {
            EntityError(error,size,"native actor data requires a finite current pose"); return 0;
        }
        data.entity.origin[i] = data.entity.originTrajectory[i] = ent->r.currentOrigin[i];
        data.entity.angles[i] = data.entity.angleTrajectory[i] = ent->r.currentAngles[i];
    }
    if ( !ActorQuantizeAngle(data.entity.angles[0],&data.orientation.pitch) ||
         !ActorQuantizeAngle(data.entity.angles[1],&data.orientation.yaw) ) {
        EntityError(error,size,"native actor angle is outside the bounded int32 conversion range"); return 0;
    }
    data.orientation.yawCopy = data.orientation.yaw;
    data.orientation.lookPitch = data.orientation.pitch;
    data.orientation.lookYaw = data.orientation.yaw;
    lookAngles[0] = data.orientation.pitch;lookAngles[1] = data.orientation.yaw;lookAngles[2] = 0;
    AngleVectors(lookAngles,data.orientation.lookforward,data.orientation.lookright,data.orientation.lookup);
    data.entity.mins[0] = data.entity.mins[1] = -15.0f;
    data.entity.maxs[0] = data.entity.maxs[1] = 15.0f;data.entity.maxs[2] = 72.0f;
    data.entity.health = data.entity.maxhealth = 100;data.entity.takedamage = 1;
    data.entity.nativeKind = 11;data.entity.nativeClipMask = UINT32_C(0x02820011);
    data.entity.nativeContents = UINT32_C(0x4000);
    data.entity.nativeServerFlags = nativeServerFlags | UINT32_C(8);
    data.entity.nativeEntityFlags = nativeEntityFlags | UINT32_C(0x28000);
    data.entity.cell0010 = 16;
    data.entity.cell01b4 = data.entity.cell01bc = data.entity.cell01c0 = data.entity.cell01c4 = 1;
    data.entity.cell01c8 = 3;
    data.actor.accuracy = ActorFloatFromBits(UINT32_C(0x3e4ccccd));
    data.actor.accuracystationarymod = 1.0f;data.actor.cell00e4 = -6969.0f;
    data.actor.cell00dc = 4;data.actor.cell0234 = 7;
    data.actor.dropweapon = data.actor.drawoncompass = 1;data.actor.cell11c4 = UINT32_MAX;
    data.actor.useable = 0; /* Conditional original prelink Actor+11b8 BYTE. */
    data.actor.cell074c = 3.0f;data.actor.cell0750 = 9.0f;
    data.lookat.parameter = 120.0f;data.lookat.yawLimits[0] = data.lookat.yawLimits[1] = 90.0f;
    sentient.team = 3;sentient.goalradius = 384.0f;sentient.goalradiusSquared = 147456.0f;
    sentient.cell0020 = -1.0f;sentient.goalangletolerance = 20.0f;sentient.scariness = 1.0f;
    /* All fallible input validation precedes this pure commit. No callback,
     * shared state mutation, pointer adoption or actor VM admission occurs. */
    if ( !a->dataInitialized && !G_Cod1ActorInitializeLocalDefaults(&lease,error,size) ) return 0;
    a->nativeData = data;
    cod1Sentients[lease.sentientIndex].defaults = sentient;
    a->nativeDataInitialized = 1;
    return 1;
}
int G_Cod1ActorSetTeam( const cod1ActorLease_t *input, uint32_t nativeTeam,
    char *error, size_t size ) {
    cod1ActorLease_t lease;
    const cod1ActorRecord_t *a;
    if ( !ActorArguments( input, sizeof( *input ), NULL, 0, error, size ) ) return 0;
    if ( !input ) { EntityError( error, size, "actor team requires a lease" ); return 0; }
    lease = *input;
    if ( !ActorLeaseValid( &lease ) || lease.kind != COD1_ACTOR_LEASE_ACTOR ) {
        EntityError( error, size, "actor team requires a current actor and sentient owner" ); return 0;
    }
    a = &cod1Actors[lease.actorIndex];
    if ( !a->nativeDataInitialized || a->initialized || a->spatial.busy || a->dobj.busy || a->modelBusy || a->modelMethodBusy ) {
        EntityError( error, size, "actor team requires unpublished native data with no pending operation" ); return 0;
    }
    if ( nativeTeam != COD1_ACTOR_TEAM_AXIS && nativeTeam != COD1_ACTOR_TEAM_ALLIES &&
         nativeTeam != COD1_ACTOR_TEAM_NEUTRAL ) {
        EntityError( error, size, "actor team requires original axis, allies or neutral code" ); return 0;
    }
    /* Native branches 0x2007415d/184/1ab write only sentient+4. Preserve the
     * actor's separate life/readiness state and all other sentient fields. */
    cod1Sentients[lease.sentientIndex].defaults.team = nativeTeam;
    return 1;
}
int G_Cod1ActorRelease( const cod1ActorLease_t *input, char *error, size_t size ) {
    cod1ActorLease_t lease;
    if ( !ActorArguments( input, sizeof( *input ), NULL, 0, error, size ) ) return 0;
    if ( !input ) { EntityError( error, size, "actor retirement requires a lease" ); return 0; }
    lease = *input;
    if ( !ActorLeaseValid( &lease ) ) { EntityError( error, size, "actor retirement received a stale lease" ); return 0; }
    ActorMotionStopOwner(&lease.entity);
    if(!ActorLeaseValid(&lease))return 0;
    if ( lease.kind == COD1_ACTOR_LEASE_ACTOR &&
         ( !ActorDObjFreeLease( &lease, error, size ) ||
           !ActorSpatialUnlinkLease( &lease, error, size ) ) ) return 0;
    if ( !ActorLeaseValid( &lease ) ) {
        EntityError( error, size, "actor retired during spatial cleanup" ); return 0;
    }
    ActorRetireEntity( &lease.entity );
    return 1;
}
int G_Cod1ActorDeath( const cod1ActorLease_t *input, char *error, size_t size ) {
    cod1ActorLease_t lease;
    if ( !ActorArguments( input, sizeof( *input ), NULL, 0, error, size ) ) return 0;
    if ( !input ) { EntityError( error, size, "actor death requires a lease" ); return 0; }
    lease = *input;
    if ( !ActorLeaseValid( &lease ) || lease.kind != COD1_ACTOR_LEASE_ACTOR ) {
        EntityError( error, size, "actor death received a stale or player lease" ); return 0;
    }
    ActorMotionStopOwner(&lease.entity);
    if(!ActorLeaseValid(&lease))return 0;
    TurretMountRetire(&lease.entity);
    cod1Actors[lease.actorIndex].alive = 0;
    cod1Actors[lease.actorIndex].defaults.lifeWords[(0x2a4-0x26c)/4] = 0;
    return 1;
}
