/* R102: bounded ownership of provisional CoD actor/sentient resources.
 * This internal fresh-map sidecar is not a native actor constructor or SAVE24
 * payload. A reservation never grants VM actor/sentient membership. */
#ifndef G_COD1_ACTOR_H
#define G_COD1_ACTOR_H
#if defined(COD1_GSC_TEST_WIP) && COD1_GSC_TEST_WIP
#include "cod1_gsc/gsc_fixture.h"
#include "cod1_gsc/gsc_session.h"
#include "cod1_spatial.h"
#include "cod1_dobj.h"
#include "cod1_precache.h"
enum {
    COD1_ACTOR_CAPACITY = 32,
    COD1_SENTIENT_CAPACITY = 36,
    COD1_ACTOR_LEASE_ACTOR = 1,
    COD1_ACTOR_LEASE_PLAYER = 2
};
/* Original sentient team values, separate from RTCW/Q3 team enums. */
enum {
    COD1_ACTOR_TEAM_AXIS = 1,
    COD1_ACTOR_TEAM_ALLIES = 2,
    COD1_ACTOR_TEAM_NEUTRAL = 3
};
#define COD1_ACTOR_NO_INDEX UINT32_MAX
typedef struct {int assigned;GscNodeRef node;float position[3];uint64_t assignment;int changed;} cod1ActorGoalSnapshot_t;
/* R223 selected locomotion is a separate activation certificate, not a claim
 * that every native postinit/AI subsystem is present. */
typedef struct {int active,moving,blocked,arrived;uint64_t assignment,notified;float travelled,position[3];} cod1ActorMotionSnapshot_t;
int G_Cod1ActorMotionBegin(const GscEntityRef *,int,int);
/* R232 real collision movement without inventing a visible model/clip. The
 * caller must separately provide the actual actor rendering/animation owner. */
int G_Cod1ActorMotionBeginUnpublished(const GscEntityRef *,int);
int G_Cod1ActorSetCourseRunRate(const GscEntityRef *,float);
int G_Cod1TakeTowerTouch(const GscEntityRef *,uint64_t,uint64_t,int,GscEntityRef *);
int G_Cod1ActorSetCourseEntryIdle(const GscEntityRef *,const char *);
int G_Cod1ActorStartRest(const GscEntityRef *,float);
int G_Cod1ActorReadEnemy(const GscEntityRef *,GscEntityRef *,char *,size_t);
int G_Cod1TrainingRunnerAppearance(const GscEntityRef*,int,int (*)(const GscEntityRef*,int,GscTrainingAppearance*));
int G_Cod1TrainingStationaryGoals(int,int (*)(void*),int (*)(void*,const GscEntityRef*),void*);
int G_Cod1ActorMotionFrame(const GscEntityRef *,int,int (*)(void *),int (*)(void *,const GscEntityRef *),void *);
void G_Cod1ActorMotionStop(uint64_t);
int G_Cod1ActorMotionRead(const GscEntityRef *,cod1ActorMotionSnapshot_t *);
int G_Cod1ActorGoalRadius(const GscEntityRef *,float *,int);
const GscActorGoalHost *G_Cod1ActorGoalHost(void);
int G_Cod1ActorReadGoal(const GscEntityRef *,cod1ActorGoalSnapshot_t *,char *,size_t);
typedef struct {
    GscEntityRef entity;
    uint64_t serial;
    uint32_t actorIndex, sentientIndex, kind;
} cod1ActorLease_t;
/* R232 selected fresh native-state4 to owned ROOT-linked turret state3.
 * currentStack is native +8/+20; critical.stack remains prospective+A4/+BC. */
typedef struct {
 int initialized,currentTop,currentStack[6],mounted,pendingOwnerNotify,pendingStateNotify,pendingStand;
 int blocker1164Known,blocker1164,claimedNode40Known,claimedNode40;
 uint64_t serial;GscEntityRef requestedGun,occupiedGun;
 float relativeOrigin[3],relativeAxis[3][3];
 float parentOrigin[3],parentAngles[3],actorOrigin[3],actorAngles[3];
} cod1ActorTurretMount_t;
/* Exact closed-helper data, not a memory overlay of the original 0x122c
 * actor. Named fields are proved by the primary native field table. Unknown
 * cells retain their numeric contract instead of invented gameplay names. */
typedef struct {
    uint32_t cell0008, cell00a4, cell0120, cell0134, cell014c, cell0150, cell024c;
    float walkdist, interval, personalspace, visibilitythreshold;
    float fovcosine, maxsightdistsqrd, grenadeawareness;
    int32_t followmin, followmax, pacifistwait, defaultsightlatency, suppressionwait;
} cod1ActorAllocatorDefaults_t;
typedef struct {
    cod1ActorAllocatorDefaults_t allocation;
    /* Native zero ranges [0x26c,0x374) and [0x374,0x74c).
     * Each word is a numeric little-endian native DWORD, not an address.
     * WORD sentinels are inserted into the appropriate DWORD half. */
    uint32_t lifeWords[66], pathWords[246], cell0774;
} cod1ActorLocalDefaults_t;
typedef struct {
    float origin[3], originTrajectory[3], angles[3], angleTrajectory[3];
    float originDelta[3], angleDelta[3], mins[3], maxs[3];
    uint32_t originType, originTime, originDuration;
    uint32_t angleType, angleTime, angleDuration;
    uint32_t nativeKind, nativeClipMask, nativeContents;
    uint32_t nativeServerFlags, nativeEntityFlags;
    uint32_t cell0010, cell01a4, cell01a8, cell01b4, cell01bc;
    uint32_t cell01c0, cell01c4, cell01c8;
    int32_t health, maxhealth, takedamage;
} cod1ActorNativeEntityDefaults_t;
typedef struct {
    float accuracy, accuracystationarymod, cell00e4, cell074c, cell0750;
    uint32_t cell00dc, cell0234, dropweapon, drawoncompass, cell11c4;
    /* +17c,+180,+184,+18c,+190,+194,+198,+19c,+1a0. */
    uint32_t zeroCells[9];
    uint32_t proneNodes16c[3];int proneGround188,proneTick;
    uint64_t proneRuntime,proneCatalog;
    /* Native allocation-zero Actor+0x788, type0 INT32; not readiness flags. */
    int32_t dontavoidplayer;
    /* Native allocation-zero Actor+0x798, plain type0 INT32; not pacifistwait. */
    int32_t pacifist;
    /* Native allocation-zero Actor+0x78c, type1 signed WORD raw bits. */
    uint16_t chainfallback;
    /* Actor+0x11b8, plain type2 BYTE; unsigned integer getter. */
    uint8_t useable;
} cod1ActorInlineDefaults_t;
typedef struct {
    float pitch, yaw, yawCopy, lookPitch, lookYaw;
    float lookforward[3], lookright[3], lookup[3];
    uint32_t history[21];
} cod1ActorOrientationDefaults_t;
typedef struct {
    uint32_t active, cell01ec, cell01f0, animnodesReady;
    float target[3], parameter, yawLimits[2];
} cod1ActorLookAtDefaults_t;
typedef struct {
    uint32_t team, threatbias;
    float goalradius, goalradiusSquared, cell0020, goalangletolerance, scariness;
} cod1SentientDefaults_t;
typedef struct {
    cod1ActorNativeEntityDefaults_t entity;
    cod1ActorInlineDefaults_t actor;
    cod1ActorOrientationDefaults_t orientation;
    cod1ActorLookAtDefaults_t lookat;
} cod1ActorNativeData_t;
/* R170 lease-owned text for native actor WORD fields name+ec/weapon+ee.
 * Presence distinguishes a zero native cell from an interned empty string.
 * These are semantic copies, not native pool IDs/refcounts or SAVE24 data. */
typedef struct {
    int namePresent, weaponPresent, posePresent;
    char name[128], weapon[128], pose[128];
} cod1ActorScriptStrings_t;
/* R176 selected training fields, independent of inventory/AI. The weapon
 * cell is a BYTE lookup result stored as DWORD, not a WORD/string or WP enum. */
typedef struct {
    uint32_t grenadeWeaponIndex;
    float bravery;
    int32_t grenadeAmmo;
} cod1ActorTrainingFields_t;
#define COD1_ACTOR_REGISTERED_WEAPONS_MAX 256
typedef struct {
    int definitionPresent,namePresent;
    char name[128];
    uint32_t resourceReceipt;
} cod1ActorRegisteredWeaponRow_t;
typedef struct {
    uint32_t rowCount;
    cod1ActorRegisteredWeaponRow_t rows[COD1_ACTOR_REGISTERED_WEAPONS_MAX];
} cod1ActorRegisteredWeaponTable_t;
typedef struct {
    cod1ActorRegisteredWeaponTable_t table;
    uint64_t receipt;
    int (*current)(uint64_t);
} cod1ActorWeaponFieldHost_t;
/* Explicit once-per-ready-map copied authority. No resource loading or table
 * selection is automatic. The static current function owns its registry
 * lifetime and validates the scalar receipt; it must retain no request/table
 * pointers. Every grenade read/write revalidates it and exact actor ownership.
 * rowCount is the number of rows, so256 rows admits indices0..255. Setter
 * requires nonnull definitions throughout its scan; names may be absent and
 * are skipped. Getter returns undefined for a NULL definition; NULL name on a
 * nonnull definition is outside its admitted domain. Canonical empty names
 * stay distinct. Fresh original registry row0 is named none; explicit existing
 * table authority is not inferred from CW/CI/WP indices or requested text. */
int G_Cod1ActorSelectWeaponFieldTable(const cod1ActorWeaponFieldHost_t *,char *,size_t);
int G_Cod1ActorSpawnPostinitSession(GscSession *,uint64_t,const GscEntityRef *,const cod1ActorLease_t *,GscEntityScriptRun *,char *,size_t);
int G_Cod1ActorSpawnInitSession(GscSession *,uint64_t,const GscEntityRef *,const cod1ActorLease_t *,GscEntityScriptRun *,char *,size_t);
int G_Cod1ActorSpawnPostinitPositionSession(GscSession *,uint64_t,const GscEntityRef *,const cod1ActorLease_t *);
int G_Cod1ActorReadTrainingFields(const cod1ActorLease_t *,cod1ActorTrainingFields_t *,char *,size_t);
/* Separate opt-in host: adds grenadeWeapon/bravery/grenadeAmmo while retaining
 * all existing checked field providers. ConstructorFieldHost is unchanged.
 * Numeric admission is finite float/exact integer for bravery, and int32 or
 * finite in-range float truncated toward zero for grenadeAmmo. GetString uses
 * the existing bounded typed conversion. A lookup miss warns then stores0;
 * registered canonical spelling is returned by the getter. No grant/precache,
 * projectile, inventory, aitype selection or readiness is performed. */
const GscEntityFieldHost *G_Cod1ActorTrainingFieldHost(void);
/* R103: a real floor query/link is still a provisional constructor stage.
 * No model, character script, AI membership or initialized actor is implied. */
typedef struct {
    GscEntityRef entity;
    float originalOrigin[3], placedOrigin[3];
    float fraction;
    int startsolid, allsolid, hitSlot;
} cod1ActorFloorPlacement_t;
typedef struct {
    int linked, busy;
    cod1ActorFloorPlacement_t floor;
} cod1ActorSpatialState_t;
/* A skeletal topology has no current animated pose and grants no actor/AI
 * membership. The engine owns all resource bytes and instance references. */
typedef struct {
    int present, busy;
    cod1DObjSummary_t summary;
} cod1ActorDObjState_t;
/* R156 GAME-owned authored composition, distinct from accepted DObj models.
 * Descriptor0 is the untagged base; up to seven attachments keep authored
 * order, including descriptors skipped by a missing current base tag.
 * Count0 means no saved request. An empty base can retain attachments after
 * checked topology removal. This is neither a live pose nor network state. */
typedef struct {
    uint32_t descriptorCount;
    cod1DObjModel_t descriptors[COD1_DOBJ_MODELS_MAX];
} cod1ActorModelComposition_t;
/* R160 lease-bound snapshot of a map-owned registration. Native modelIndex
 * (CS_MODELS+index) and server resource.slot are independent namespaces.
 * A copied reference owns no engine pointer and pins no resource. */
typedef struct {
    cod1ActorLease_t lease;
    uint32_t modelIndex, reserved;
    cod1ModelResourceReference_t resource;
    char nativeName[MAX_QPATH];
} cod1ActorModelRegistration_t;
/* R161 copied native base-byte assignment plus its owned topology revision.
 * This is unpublished constructor data, not a renderer/network packet. */
typedef struct {
    cod1ActorModelRegistration_t registration;
    cod1DObjSummary_t topology;
} cod1ActorAssignedModel_t;
typedef struct {
    uint32_t count;
    cod1ActorModelRegistration_t models[COD1_DOBJ_MODELS_MAX-1];
} cod1ActorRegisteredAttachments_t;
int G_Cod1ActorApplyRegisteredModel(const cod1ActorModelRegistration_t *model,
    cod1ActorAssignedModel_t *out, char *error, size_t errorSize);
int G_Cod1ActorClearRegisteredModel(const cod1ActorLease_t *lease,
    cod1ActorAssignedModel_t *out, char *error, size_t errorSize);
/* Pure copied snapshot. Registration may subsequently expire; it is no pin. */
int G_Cod1ActorReadAssignedModel(const cod1ActorLease_t *lease,
    cod1ActorAssignedModel_t *out, char *error, size_t errorSize);
/* R162 internal nonempty registered attachment operands, not GSC methods.
 * Append copies registration and a complete bounded128-byte tag buffer;
 * printable ASCII is lowered with the proved C-locale policy, empty is valid.
 * Native empty-model tag holes/string IDs, duplicate lookup and ScriptError
 * belong to the pending adapter. Raw append therefore allows duplicates.
 * Removal uses a current dense authored descriptor index1..7 and compacts;
 * clear keeps the base. A cleared base retains registrations without a DObj.
 * Requires the checked hidden placement and either all registered attachments
 * or none; mixing existing raw descriptors is refused. Every participating
 * CS/resource/owner is rechecked under both locks before/after the trap.
 * Success advances DObj/lane revision, without ordinary unlink/link, PVS or
 * linkcount writes. setModel/clear later preserves these registrations and
 * uses their decoded bytes, even when current FS files are missing. Normal
 * backend refusal preserves state/output; post-success invalidation uses
 * exact-old-key cleanup. Raw Build/Free/raw mutation clears the certificates.
 * No initialized, AI/network identity, live pose or GSC binding. ABI59. */
int G_Cod1ActorAppendRegisteredAttachment(const cod1ActorModelRegistration_t *model,
    const char tag[COD1_DOBJ_NAME_MAX],cod1ActorAssignedModel_t *out,char *error,size_t errorSize);
int G_Cod1ActorRemoveRegisteredAttachment(const cod1ActorLease_t *lease,
    uint32_t descriptorIndex,cod1ActorAssignedModel_t *out,char *error,size_t errorSize);
int G_Cod1ActorClearRegisteredAttachments(const cod1ActorLease_t *lease,
    cod1ActorAssignedModel_t *out,char *error,size_t errorSize);
/* Pure copied snapshot of the saved attachment registrations, not a pin. */
int G_Cod1ActorReadRegisteredAttachments(const cod1ActorLease_t *lease,
    cod1ActorRegisteredAttachments_t *out,char *error,size_t errorSize);
#include "cod1_gsc/gsc_native_imports.h"
enum { COD1_ACTOR_METHOD_SETMODEL=1, COD1_ACTOR_METHOD_ATTACH,
    COD1_ACTOR_METHOD_DETACH, COD1_ACTOR_METHOD_DETACH_ALL };
enum { COD1_ACTOR_METHOD_UNAVAILABLE=0, COD1_ACTOR_METHOD_VOID=1,
    COD1_ACTOR_METHOD_SCRIPT_ERROR=2, COD1_ACTOR_METHOD_PARAMETER_ERROR=3 };
enum { COD1_ACTOR_METHOD_ERROR_NONE, COD1_ACTOR_METHOD_ERROR_DUPLICATE,
    COD1_ACTOR_METHOD_ERROR_CAPACITY, COD1_ACTOR_METHOD_ERROR_NOT_ATTACHED,
    COD1_ACTOR_METHOD_ERROR_MISSING_PARAMETER, COD1_ACTOR_METHOD_ERROR_TYPE_CAST };
/* R165 copied raw native Value tags 0..18, not a script Value/pool pointer.
 * string1, vector3, float4 and int5 are accepted. integer is a full int32;
 * number/vector are IEEE float32. Localized string2 remains a distinct type.
 * Ordinary entity/array/struct references use raw object7; subtype13/15/14
 * is object-table metadata and is not their stack Value.type.
 * Malformed/overlong/nonfinite inputs are unavailable, not native type errors.
 * No shared EXE import record, serialization or native ABI is changed. */
#include "cod1_gsc/gsc_string_cast.h"
typedef GscStringCastValue cod1ActorModelValue_t;
typedef GscStringCastResult cod1ActorModelString_t;
/* 1=converted/copied text, -1=known native cast error in error buffer,
 * 0=unavailable with output unchanged. C-locale finite numeric domain only;
 * no mutation/refcount cleanup of the caller's VM stack is performed. */
int G_Cod1ActorConvertModelValue(const cod1ActorModelValue_t *value,
    cod1ActorModelString_t *out,char *error,size_t errorSize);
typedef struct {
    uint32_t disposition, errorKind, mutation, descriptorIndex;
    uint32_t parameterIndex, diagnosticCount;
    uint32_t consumedParameterCount, coercedParameterMask;
    char parameterText[2][128];
    cod1ActorAssignedModel_t assigned;
    cod1DObjModel_t diagnostics[COD1_DOBJ_MODELS_MAX-1];
    char message[384];
} cod1ActorModelMethodResult_t;
/* R164 host evaluator for copied string/int/float/vector operands on the
 * checked hidden/provisional actor stage. No VM name is bound by this API.
 * VOID, native SCRIPT_ERROR and missing-parameter outcomes are completed
 * evaluations with a copied result. Cast errors use one-based parameterIndex;
 * successful preceding conversions are recorded before a later cast error.
 * UNAVAILABLE preserves output. Global VM ownership, native nonfinite values,
 * raw mix and empty-model attachment holes remain outside the domain. Extras are ignored
 * locally after evaluation; detachAll consumes none. Caller supplies argc
 * 0..16 and complete value records, with input/output/error ranges separate.
 * Tag identity uses copied ASCII lowercase strings, not fabricated WORD IDs.
 * Lookup compares current exact CS model names using C-locale ASCII equality;
 * prefix/slash normalization never participates in duplicate/detach matching.
 * Duplicate attach commits removal/rebuild then reports SCRIPT_ERROR. A miss
 * in detach reports copied native-style diagnostics without rebuilding. Full
 * append refuses before resource registration. Other backend failures retain
 * the checked candidate/rollback policy, not retail free-first error parity.
 * A per-owner method lock spans lookup/registration/update/diagnostics, with
 * genuine retirement allowed only through the existing exact-key route.
 * EXE/GAME59/imports unchanged; no actor initialized/spawned/AI publication. */
int G_Cod1ActorInvokeModelMethod(const cod1ActorLease_t *lease,uint32_t method,
    const GscNativeValue *args,int argc,cod1ActorModelMethodResult_t *out,
    char *error,size_t errorSize);
int G_Cod1ActorInvokeTypedModelMethod(const cod1ActorLease_t *lease,uint32_t method,
    const cod1ActorModelValue_t *args,int argc,cod1ActorModelMethodResult_t *out,
    char *error,size_t errorSize);
typedef struct {
    cod1ActorLease_t lease;
    int32_t actorActivation, sentientActivation;
    int alive, initialized, dataInitialized, nativeDataInitialized;
    cod1ActorLocalDefaults_t defaults;
    cod1ActorNativeData_t nativeData;
    cod1ActorScriptStrings_t scriptStrings;
    cod1SentientDefaults_t sentientDefaults;
    cod1ActorSpatialState_t spatial;
    cod1ActorDObjState_t dobj;
} cod1ActorLeaseSnapshot_t;
typedef struct {
    int32_t actorActivation[COD1_ACTOR_CAPACITY];
    int32_t sentientActivation[COD1_SENTIENT_CAPACITY];
    int32_t actorRetirementCounter, sentientRetirementCounter;
    unsigned int actorReserved, sentientReserved, playerReserved;
} cod1ActorPoolSnapshot_t;

/* R154 complete internal reservation view, not a script AI array. These
 * records include provisional/dead owners and retain their readiness flags.
 * Entity-slot order must not replace native actor-pool order. */
typedef struct {
    cod1ActorLease_t lease;
    int32_t actorActivation, sentientActivation;
    uint32_t team;
    int alive, initialized, dataInitialized, nativeDataInitialized;
} cod1ActorReservationView_t;

/* Actor allocation precedes sentient allocation. On failure only provisional
 * resources are rolled back; the caller still owns the requested entity.
 * The eventual constructor adapter must implement native entity-free failure.
 * Allocation requires nonnegative level.time: time 0 is occupied, not unused.
 * Forced eviction is not implemented, and occupied records are never stolen. */
int G_Cod1ActorReserve( const GscEntityRef *entity, cod1ActorLease_t *out,
    char *error, size_t errorSize );
/* Canonical connected SP players consume the same 36-slot sentient pool,
 * without an actor slot. No read/predicate silently creates this reservation. */
int G_Cod1PlayerSentientReserve( const GscEntityRef *entity, cod1ActorLease_t *out,
    char *error, size_t errorSize );
int G_Cod1ActorLinkedAndAlive(const cod1ActorLease_t *lease);
int G_Cod1ActorLeaseCurrent( const cod1ActorLease_t *lease );
int G_Cod1ActorReadLease( const cod1ActorLease_t *lease,
    cod1ActorLeaseSnapshot_t *out, char *error, size_t errorSize );
/* Allocation-backed native entity134/C4 view. Includes provisional/dead
 * actors with initialized native data; unknown/player-only owners refuse.
 * No initialized/AI/graph capability is granted. Output is untouched on
 * refusal. Contents exchange changes the native record and, when linked,
 * the exact matching SERVER lane. No relink, retirement or sticky reset. */
typedef struct {
    cod1ActorLease_t lease;
    uint32_t flags, contents;
    int linked;
} cod1ActorPathView_t;
int G_Cod1ActorPathRead(const GscEntityRef *entity,cod1ActorPathView_t *out);
/* Positive proof that a previously captured actor record retired/replaced.
 * Malformed or inconsistent surviving records are not treated as gone. */
int G_Cod1ActorPathRetired(const cod1ActorLease_t *lease);
int G_Cod1ActorPathContents(const cod1ActorLease_t *lease,
    uint32_t expected,uint32_t replacement);
int G_Cod1ActorReadPool( cod1ActorPoolSnapshot_t *out,
    char *error, size_t errorSize );
/* Copies every owned actor reservation by ascending actor index. Player-only
 * sentients are not actor records. No health, team or initialized filtering,
 * truncation, traps, allocation, publication or constructor is performed.
 * A stale/malformed occupied record refuses the entire copy. Capacity may be
 * 0..32; NULL output is allowed only at capacity0 with an empty actor pool.
 * Output/count/error ranges must be separate; refusal preserves outputs.
 * A successful empty reservation view does not establish an empty game AI
 * domain. getaiarray additionally requires authoritative map producer/census
 * coverage; initialized/publication are not native per-record filters. */
int G_Cod1ActorCollectReservations( cod1ActorReservationView_t *out,
    size_t capacity, size_t *count, char *error, size_t errorSize );
/* Executes only the proved closed allocation/life/path default contracts.
 * Bbox is the native constructor's (-15,-15,0)..(15,15,72). This sets the
 * internal life cell to 1, while the lease remains provisional/unpublished.
 * It neither writes shared entity flags nor performs CM/link, orientation,
 * sentient/character/script initialization. initialized remains 0. */
int G_Cod1ActorInitializeLocalDefaults( const cod1ActorLease_t *lease,
    char *error, size_t errorSize );
/* Proved allocated-constructor local continuation, STOPPED before engine link
 * and every subsequent character/script callback. Copies current engine pose;
 * the flags arguments belong to the original native namespace, never the port
 * r.svFlags/flags namespace. They are retained as typed data, not shared writes.
 * Composes local defaults, quantized orientation and real AngleVectors,
 * sentient parameters and look-at data. initialized remains 0. Nonfinite pose
 * and angle products outside int32 conversion range fail before mutation. */
int G_Cod1ActorInitializeNativeData( const cod1ActorLease_t *lease,
    uint32_t nativeServerFlags, uint32_t nativeEntityFlags,
    char *error, size_t errorSize );
/* R174 explicitly invoked, ready-map middle segment. Caller supplies a raw,
 * hidden nonsolid unlinked ordinary entity with no actor/sentient reservation.
 * Copies dynamic fields before Reserve -> owned NativeData -> PlaceAndLink ->
 * the named, already linked zero-argument script entry. The VM is quiescent at
 * admission; the component certificate persists across waits and descendants.
 * Stage reports the last fully completed prefix even when this call returns0;
 * the refused current stage may also have effects. No rollback is implied.
 * A current acquired lease is retained for explicit caller Release; a lost
 * lease never permits cleanup of a replacement. The caller always owns the
 * entity. Argument alias failures preserve output. Upstream static copying,
 * explicit targetname replacement/spawnflags, native allocation-failure entity
 * free, aitype/postinit/init and actor readiness/publication remain outside.
 * Existing sidecar initialization does not project every native entity cell. */
typedef struct {
    GscEntityRef source,destination;
    uint32_t nativeServerFlags,nativeEntityFlags;
    char module[128],function[64];
} cod1ActorInheritedEntryRequest_t;
enum { COD1_ACTOR_ENTRY_RAW=0,COD1_ACTOR_ENTRY_FIELDS,COD1_ACTOR_ENTRY_RESERVED,
       COD1_ACTOR_ENTRY_NATIVE_DATA,COD1_ACTOR_ENTRY_LINKED,COD1_ACTOR_ENTRY_ENTERED };
typedef struct {
    int stage,leaseRetained;
    size_t inheritedFields;
    cod1ActorLease_t lease;
    GscEntityScriptRun entry;
} cod1ActorInheritedEntryResult_t;
int G_Cod1ActorRunInheritedEntryStage(GscRuntime *,
    const cod1ActorInheritedEntryRequest_t *,cod1ActorInheritedEntryResult_t *,char *,size_t);
/* R175 explicit native-static source authority. Text owns at most127 bytes;
 * absent and defined empty are distinct. Floating cells carry raw float32
 * bits; this copy does not invoke custom setters or coerce native values.
 * The six missing semantic cells and raw model BYTE require complete explicit
 * host provenance. A model token/port configstring is never a native BYTE.
 * Publication does not select a VM host or create an actor/spawner. */
typedef struct { int present; char text[128]; } cod1NativeStaticText_t;
typedef struct {
    cod1NativeStaticText_t teamname,groupname,exportName,scriptNoteworthy;
    int32_t maxhealth;
    uint32_t anglelerprateBits,modelByte;
    int modelByteAuthoritative;
} cod1EntityNativeStaticExtras_t;
typedef struct {
    GscEntityRef entity;
    cod1NativeStaticText_t classname,target,targetname,message,teamname;
    cod1NativeStaticText_t spawnitem,track,groupname,exportName,scriptNoteworthy;
    uint32_t modelByte,spawnflags;
    int32_t count,health,damage,key,radius,start_size,end_size,maxhealth;
    uint32_t originBits[3],anglesBits[3],rotateBits[3],colorBits[3];
    uint32_t speedBits,closespeedBits,waitBits,randomBits,durationBits;
    uint32_t degreesBits,delayBits,anglelerprateBits;
    int modelByteAuthoritative;
} cod1EntityNativeStaticSnapshot_t;
int G_Cod1EntityReadNoteworthy(const GscEntityRef *ref,GscStringCastValue *out,char *error,size_t size);
int G_Cod1EntityPublishNativeStaticExtras(const GscEntityRef *,
    const cod1EntityNativeStaticExtras_t *,char *,size_t);
/* Initial BSP authority is one-time per allocation, including bootstrap.
 * Unlike complete explicit republication, it never reasserts old defaults
 * after reservation, model mutation, publication or projection. */
int G_Cod1EntityNativeStaticInitialAuthorityAvailable(const GscEntityRef *);
int G_Cod1EntityCaptureNativeStaticSource(const GscEntityRef *,
    cod1EntityNativeStaticSnapshot_t *,char *,size_t);
/* Current compatible cells (including pose/count) are read live. Source
 * authority is invalidated on reservation/model mutation; owned text survives
 * ActorRelease and is retired only with its exact allocation/map. No fallback
 * to stale native-only fields occurs after component retirement.
 * New path: preflight -> static38/32unique cells (light skipped) -> inherited
 * dynamic fields -> caller targetname replacement, including absent clears,
 * spawnflags&~1 -> unpublished modelBYTE0 -> Reserve -> NativeData with live
 * health100/maxhealth100 -> link -> explicit known entry. Static projection
 * copies only compatible current pose, never trajectories or setter effects.
 * Caller retains entity/current partial lease on refusal. Flags report last
 * completed prefixes; a failed stage may also have effects. No native failure
 * entity-free, aitype/postinit/init, model registration or readiness is implied.
 * The old R174 entry and old initializer behavior remain unchanged. */
typedef struct {
    cod1ActorInheritedEntryRequest_t inherited;
    cod1NativeStaticText_t targetname;
} cod1ActorProjectedEntryRequest_t;
typedef struct {
    int staticProjected,overrideApplied;
    cod1EntityNativeStaticSnapshot_t copiedStatic;
    cod1ActorInheritedEntryResult_t inherited;
} cod1ActorProjectedEntryResult_t;
int G_Cod1ActorRunProjectedEntryStage(GscRuntime *,
    const cod1ActorProjectedEntryRequest_t *,cod1ActorProjectedEntryResult_t *,char *,size_t);
/* Same genuine native prefix, restricted by VM to its active spawner prepare
 * callback and source. Partial destination/lease remains caller-owned on
 * failure. Does not publish a completed factory or supply failure-free. */
int G_Cod1ActorRunProjectedSpawnEntryStage(GscRuntime *,
    const cod1ActorProjectedEntryRequest_t *,cod1ActorProjectedEntryResult_t *,char *,size_t);
/* Same prefix through a current public Session factory callback. */
int G_Cod1ActorRunProjectedSpawnEntrySession(GscSession *,uint64_t,
    const cod1ActorProjectedEntryRequest_t *,cod1ActorProjectedEntryResult_t *,char *,size_t);
/* Allocated prefix only: no aitype resolution, postinit, ready/spawned, count or
 * time commit. Success retains the exact allocation/lease for the caller.
 * Failure attempts exact-old-key G_FreeEntity once; cleanupPending means the
 * caller still owns the reported allocation and must recover cleanup. */
typedef struct {
    GscEntityRef spawner;
    cod1NativeStaticText_t targetname;
    char module[128],function[128];
    uint32_t nativeServerFlags,nativeEntityFlags;
} cod1ActorSpawnPrefixRequest_t;
typedef struct {
    GscEntityRef allocation;
    int allocated,retired,cleanupPending;
    cod1ActorProjectedEntryResult_t prefix;
} cod1ActorSpawnPrefixResult_t;
int G_Cod1ActorAllocateSpawnPrefixSession(GscSession *,uint64_t,
    const cod1ActorSpawnPrefixRequest_t *,cod1ActorSpawnPrefixResult_t *,char *,size_t);
/* Copied cached names/receipt only, not an execution certificate. */
typedef struct {char module[128],function[128];uint64_t resourceReceipt;} cod1ActorSpawnScript_t;
typedef struct {
 GscEntityRef spawner;cod1NativeStaticText_t targetname;
 uint32_t nativeServerFlags,nativeEntityFlags;
} cod1ActorDynamicPrefixRequest_t;
int G_Cod1ActorResolveSpawnAitypeSession(GscSession *,uint64_t,const GscEntityRef *,
 cod1ActorSpawnScript_t *,char *,size_t);
int G_Cod1ActorFactoryPrepareSession(GscSession *,uint64_t,const GscActorSpawnRequest *,GscActorSpawnTicket *,int *,char *,size_t);
int G_Cod1ActorFactoryFinishSession(GscSession *,uint64_t,const GscActorSpawnTicket *,int,char *,size_t);
int G_Cod1ActorFactoryCommitCurrent(const GscActorSpawnTicket *,uint64_t intake);
int G_Cod1ActorAllocateAitypePrefixSession(GscSession *,uint64_t,
 const cod1ActorDynamicPrefixRequest_t *,cod1ActorSpawnPrefixResult_t *,char *,size_t);



/* R177 declaration-driven BSP private strings. Unknown keys are not inferred
 * from script_ spelling. Caller supplies copied registered declarations and
 * a lifetime-stable pure scalar receipt validator. Only declared string type1
 * is admitted for BSP import here; declared float4/int5 remain explicit holds.
 * Names are canonical lowercase ASCII; lookup is ASCII case-insensitive. */
#define COD1_BSP_FIELD_DECLARATIONS_MAX 256
typedef struct { char name[128];uint32_t type; } cod1BspFieldDeclaration_t;
typedef struct {
    uint32_t count;
    cod1BspFieldDeclaration_t fields[COD1_BSP_FIELD_DECLARATIONS_MAX];
    uint64_t receipt;
    int (*current)(uint64_t);
} cod1BspFieldDeclarationHost_t;
int G_Cod1EntitySelectBspFieldDeclarations(const cod1BspFieldDeclarationHost_t *,char *,size_t);
/* Explicit-length declaration text, one selected file rather than directory
 * discovery. Token pairs use exact lowercase type keywords, ASCII folded
 * identifier names, boundary comments and quoted identifiers. Refuses tokens
 * over127 bytes, embedded NUL and malformed comments/quotes without truncation.
 * Parser output is atomic; input/output/error storage must be disjoint.
 * Text selection owns the complete copied table with a map-scoped receipt;
 * it does not retain source bytes or change the string-only BSP import gate. */
#define COD1_BSP_DECLARATION_TEXT_MAX 65536
typedef struct {
    uint32_t count;
    cod1BspFieldDeclaration_t fields[COD1_BSP_FIELD_DECLARATIONS_MAX];
} cod1BspFieldDeclarationTable_t;
int G_Cod1ParseBspFieldDeclarations(const char *,size_t,cod1BspFieldDeclarationTable_t *,char *,size_t);
int G_Cod1EntitySelectBspFieldDeclarationText(const char *,size_t,char *,size_t);
typedef struct {
    GscEntityRef entity;
    uint32_t rawFieldCount,rawBytes,privateFieldCount;
    int privateFieldsAvailable,startupConsumed,nativeStaticAvailable;
} cod1PlacedActorSnapshot_t;
struct gentity_s;
/* Explicit BSP-bootstrap intake, same original allocation, actor_ and bit0
 * clear only. Copies raw ordered pairs and owns static text; retains a hidden
 * provisional entity without reserve/AI/entry or automatic G_CallSpawn wiring.
 * Raw capture can succeed while unsupported typed fields keep startup held.
 * Bounded ASCII strings exclude escapes/control bytes; no truncation. */
int G_Cod1PlacedActorIntake(struct gentity_s *,char *,size_t);
int G_Cod1PlacedActorCapture(const struct gentity_s *,GscEntityRef *);
int G_Cod1PlacedActorRead(const GscEntityRef *,cod1PlacedActorSnapshot_t *,char *,size_t);
int G_Cod1PlacedActorMatchesFields(const GscEntityRef *,const char *,size_t,size_t,const unsigned short *,const unsigned short *);
int G_Cod1PlacedActorRawField(const GscEntityRef *,size_t,char *,size_t,char *,size_t,char *,size_t);
typedef struct {
    GscEntityRef entity;
    uint32_t nativeServerFlags,nativeEntityFlags;
    char module[128],function[64];
} cod1ActorPlacedEntryRequest_t;
typedef struct {
    int stage,leaseRetained,startupConsumed;
    size_t initialFields;
    cod1ActorLease_t lease;
    GscEntityScriptRun entry;
} cod1ActorPlacedEntryResult_t;
/* Stages use COD1_ACTOR_ENTRY_*. Pure preflight refusal may retry. Admission
 * of the first private-field commit, including an empty batch, permanently
 * consumes startup for this allocation, also after ActorRelease/runtime free.
 * Native model0 precedes Reserve; health/maxhealth100 follows NativeData.
 * Targetname/spawnflags and allocation identity are preserved. Caller retains
 * entity/current partial lease on refusal; failed stages may have effects.
 * Existing prepared ownership certificate protects resumes/children. No
 * inheritance, arg2 replacement, aitype resolution/postinit or readiness. */
int G_Cod1ActorRunPlacedEntryStage(GscRuntime *,const cod1ActorPlacedEntryRequest_t *,
    cod1ActorPlacedEntryResult_t *,char *,size_t);
/* R179 opt-in semantic resource subset: main/precache must resolve in one
 * linked program. The native third spawner handle/execution remains held.
 * Names are lowercase aitype suffixes, copied once per map; the existing
 * nonreviving program token prevents substitution by another program.
 * Shared hooks run before the selected derived-main route, never replay main.
 * An attempted hook is consumed after its initial return/wait/error, even when
 * a failed phase leaves a prefix. A partial phase cannot retry in this map.
 * Receipt lifetime ignores consumption and survives VM teardown; map reset
 * retires all continuations. No automatic Session/postinit/AI publication. */
#define COD1_AITYPE_RESOURCES_MAX 32
typedef struct { uint32_t count;char names[COD1_AITYPE_RESOURCES_MAX][121]; } cod1AitypeResourceSelection_t;
typedef struct {
    uint32_t count,attempted;
    int phaseInvoked,allReturned;
    GscResourceBootstrapRun hooks[COD1_AITYPE_RESOURCES_MAX];
} cod1AitypeResourcePhaseResult_t;
typedef struct { GscEntityRef entity;uint32_t nativeServerFlags,nativeEntityFlags; } cod1ActorPlacedAitypeEntryRequest_t;
int G_Cod1ActorSelectAitypeResources(const GscRuntime *,const cod1AitypeResourceSelection_t *,char *,size_t);
int G_Cod1ActorRunAitypeResourcePhase(GscRuntime *,cod1AitypeResourcePhaseResult_t *,char *,size_t);
int G_Cod1ActorRunPlacedAitypeEntryStage(GscRuntime *,const cod1ActorPlacedAitypeEntryRequest_t *,cod1ActorPlacedEntryResult_t *,char *,size_t);
/* R180 full three-label catalog and complete typed entity initial hook pass.
 * This mode is exclusive with the old two-label phase. The copied complete
 * current placed/spawner collection must be in ascending entity-slot order.
 * Per entity, a typed spawner invokes spawner before its resource's first
 * shared precache; shared roots have independent map/resource lifetime.
 * Results name the trigger entity for shared hooks, without owning it.
 * Initial invocation completion includes waits, as in the R179 contract.
 * No animation init, native postinit tail, readiness or AI is supplied. */
#define COD1_ACTOR_ENTITY_HOOKS_MAX 32
enum { COD1_ACTOR_HOOK_SPAWNER=1,COD1_ACTOR_HOOK_PRECACHE=2 };
typedef struct { uint32_t count;GscEntityRef entities[COD1_ACTOR_ENTITY_HOOKS_MAX]; } cod1ActorEntityHookPhaseRequest_t;
typedef struct { uint32_t kind,resourceIndex;GscEntityRef entity;GscResourceBootstrapRun run; } cod1ActorEntityHookRun_t;
typedef struct {
    uint32_t entityCount,hookCount,attempted;int phaseInvoked,allReturned;
    cod1ActorEntityHookRun_t hooks[GSC_ENTITY_HOOK_BOOTSTRAP_MAX];
} cod1ActorEntityHookPhaseResult_t;
int G_Cod1ActorSelectAitypeResourcesWithSpawnerHooks(const GscRuntime *,const cod1AitypeResourceSelection_t *,char *,size_t);
int G_Cod1ActorRunEntityHookPhase(GscRuntime *,const cod1ActorEntityHookPhaseRequest_t *,cod1ActorEntityHookPhaseResult_t *,char *,size_t);
/* R203 restricted Session services share the same one-shot orchestration.
 * Service/context pointers are borrowed only for the synchronous call. */
int G_Cod1ActorSelectAitypeResourcesViaServices(const GscActorBootstrapServices *,const cod1AitypeResourceSelection_t *,char *,size_t);
int G_Cod1ActorRunEntityHookPhaseViaServices(const GscActorBootstrapServices *,const cod1ActorEntityHookPhaseRequest_t *,cod1ActorEntityHookPhaseResult_t *,char *,size_t);
int G_Cod1ActorRunPlacedEntryStageViaServices(const GscActorBootstrapServices *,const cod1ActorPlacedEntryRequest_t *,cod1ActorPlacedEntryResult_t *,char *,size_t);
int G_Cod1ActorConstructPlacedAitype(const GscActorBootstrapServices *,const cod1ActorPlacedAitypeEntryRequest_t *,cod1ActorPlacedEntryResult_t *,char *,size_t);
int G_Cod1ActorEnterConstructedAitype(const GscActorBootstrapServices *,const cod1ActorLease_t *,GscEntityScriptRun *,char *,size_t);
int G_Cod1ActorRunPlacedAitypeEntryStageViaServices(const GscActorBootstrapServices *,const cod1ActorPlacedAitypeEntryRequest_t *,cod1ActorPlacedEntryResult_t *,char *,size_t);
const GscSpawnerTeamMethodHost *G_Cod1SpawnerTeamMethodHost(void);
/* R155 closed data mutation required by aitype's native self.team write.
 * Accepts only the proved original sentient codes 1/2/3, not a port team enum
 * or a GSC value/string. Original 0x20074130 maps canonical axis/allies/neutral
 * to these codes and writes only sentient+4. Its getter, CRT locale-dependent
 * comparison and ScriptError boundary remain outside this internal API.
 * Requires a current native-data-initialized, unpublished, nonbusy actor
 * lease. A dead owned reservation can change team without becoming alive.
 * No shared entity flags, player team, AI identity, publication, callbacks or
 * allocation are changed. Refusal preserves all owned data. */
int G_Cod1ActorSetTeam( const cod1ActorLease_t *lease, uint32_t nativeTeam,
    char *error, size_t errorSize );
/* R168 explicitly selected constructor-side VM adapters. Native field names
 * from the primary actor/sentient/entity tables are reserved, so unsupported
 * fields cannot become private script dictionaries. Team, accuracy,
 * accuracystationarymod, scariness, health, maxhealth, name and weapon are readable/writable. Requires
 * a current, native-data-initialized hidden provisional actor; no AI admission.
 * Float32 inputs are finite; int32->float accepts the exact range +/-2^24.
 * Team uses owned string conversion and the observed ASCII compare branch.
 * R169 health reads live gentity.health; maxhealth owns the native sidecar
 * cell. Successful positive health writes set both and mirror native health;
 * old health<=0 with maxhealth!=0 warns/no-ops, while maxhealth0 permits the
 * original initialization exception. Nonpositive health errors. Maxhealth
 * accepts any int32. Numeric float->int truncates in finite int32 range;
 * native string parsing, out-of-range wrap and actual death are not admitted.
 * R170 name/weapon own bounded127-byte plain text after the R165 string cast;
 * unassigned reads are undefined, assigned empty strings remain present.
 * Field writes do not grant or remove inventory/AI resources. Exact native
 * string pool IDs/refcounts are not stored. Reservation/retirement/map reset
 * own their lifetime, without borrowing caller strings.
 * The separate access host routes actor setModel through actual registered
 * v14/DObj methods, and delegates ordinary script_model/pose operations to the
 * earlier host. Neither host is selected by GAME startup automatically. */
const GscEntityFieldHost *G_Cod1ActorConstructorFieldHost(void);
/* R233 selected native anim_pose WORD+178 provider; no initialization side effects. */
const GscEntityFieldHost *G_Cod1ActorPoseFieldHost(void);
int G_Cod1ActorReadRuntimeStringField(const GscEntityRef*,const char*,GscStringCastValue*,char*,size_t);
int G_Cod1ActorReadAbsentEnemyField(const GscEntityRef*,GscStringCastValue*,char*,size_t);
int G_Cod1MapModelStarting(uint64_t);
int G_Cod1EntityReadModelField(const GscEntityRef *input,GscStringCastValue *out,char *error,size_t size);
int G_Cod1ActorReadModelField(const GscEntityRef *,GscStringCastValue *,char *,size_t);
int G_Cod1ActorReadRuntimeField(const GscEntityRef *,const char *,GscStringCastValue *,char *,size_t);
const GscEntityAccessHost *G_Cod1ActorConstructorAccessHost(void);
/* Explicit optional VM method host for attach/detach/detachAll. Requires the
 * program's private method selector and runtime host selection before start;
 * it delegates to the checked R164 typed registered-model method pipeline.
 * VOID succeeds; completed native script/parameter errors stop the VM after
 * any already committed duplicate removal. Backend diagnostics remain copied
 * result data; native formatted warning I/O/pool behavior are not claimed. */
const GscEntityMethodHost *G_Cod1ActorConstructorMethodHost(void);
const GscEntityMethodHost *G_Cod1ActorRuntimeMethodHost(void);
/* R203 real registered attachment snapshots, selected independently by the
 * Session actor-prefix lane. No readiness or native tag-hole fabrication. */
const GscAttachmentQueryHost *G_Cod1ActorAttachmentQueryHost(void);
/* Checked native capsule floor query: original origin+z1 to original z-128,
 * height30, raw native mask. startsolid keeps the raised start; every other
 * valid trace uses endpos, including fraction1. No shared fields are written. */
int G_Cod1ActorProbeFloor( const GscEntityRef *entity,
    cod1ActorFloorPlacement_t *out, char *error, size_t errorSize );
/* First placement/link of an alive, native-data-initialized provisional lease.
 * Caller must already hide its ordinary unlinked, nonclient entity with
 * SVF_NOCLIENT; this function never writes flags/contents/health/type. Commits
 * only origin trajectory/current origin and exact native bbox. The no-DObj
 * engine lane preserves native collision bits. initialized remains0. */
int G_Cod1ActorPlaceAndLink( const cod1ActorLease_t *lease,
    char *error, size_t errorSize );
/* Explicit provisional topology stage; not the script setModel/attach API.
 * Copies all authored descriptors before the trap; validates allocation/lease
 * again afterward. The base descriptor has an empty tag. A native missing
 * attachment tag skips its descriptor, reported in the summary. No animation
 * tree, live pose, model configstring, renderer, collision flag or spawned
 * notification is published. Engine and game DLL require the paired ABI48. */
int G_Cod1ActorBuildDObj( const cod1ActorLease_t *lease,
    const cod1DObjModel_t *models, uint32_t modelCount,
    cod1DObjSummary_t *out, char *error, size_t errorSize );
int G_Cod1ActorFreeDObj( const cod1ActorLease_t *lease,
    char *error, size_t errorSize );
/* Pure copy of the last successful authored Build/base-change request.
 * Retirement/reset remove its owner. Explicit DObj free retains the saved
 * descriptors; the separate DObj state determines live topology readiness.
 * Requires a current actor lease; output/lease/error ranges are separate. */
int G_Cod1ActorReadModelComposition( const cod1ActorLease_t *lease,
    cod1ActorModelComposition_t *out, char *error, size_t errorSize );
/* Internal topology stage of model replacement, not GSC setModel. Copies a
 * complete bounded base descriptor (tag must be empty), preserves every saved
 * attachment and rebuilds through the existing checked engine DObj backend.
 * Empty base performs keyed free and retains attachments; output is then all
 * zero. Stage/lease ownership is checked again after callbacks. Failed engine
 * builds retain the previous composition/topology; invalidation after a
 * successful trap can remove topology during its existing keyed cleanup.
 * No model index/configstring, animtree, pose, network/collision flag, link,
 * script field binding or actor publication is performed. All input/output/
 * error ranges are separate. Requires the existing hidden native-data stage. */
int G_Cod1ActorReplaceDObjBase( const cod1ActorLease_t *lease,
    const cod1DObjModel_t *base, cod1DObjSummary_t *out,
    char *error, size_t errorSize );
/* R157 raw topology/data mutations AFTER any native lookup has selected an
 * attachment. These are not GSC attach/detach/detachAll and perform no string
 * ID lookup, CRT name comparison or ScriptError. Dense authored slots include
 * missing-tag descriptors. Append corresponds to the first free native slot,
 * requires a nonempty saved base, and deliberately has no duplicate check:
 * original raw20044a40 has none; wrapper20031210 calls removal before error.
 * Remove takes CURRENT descriptor index1..7 (native slot=index-1), compacts
 * remaining descriptors and clears the tail. An index is not a saved identity.
 * Remove/clear also work with a cleared base: checked keyed free, zero summary
 * and owned authored data commit. Clear preserves descriptor0. With a base,
 * all successful mutations rebuild through real SV DObj. Normal backend
 * refusal retains prior data/topology; callback invalidation can trigger the
 * existing keyed cleanup. Failed output remains unchanged. Input/output/error
 * ranges are separate; stage/lifetime gates equal ReplaceDObjBase. No native
 * registration/index/refcount parity, animation/pose/link, GSC or AI admission. */
int G_Cod1ActorAppendDObjAttachment( const cod1ActorLease_t *lease,
    const cod1DObjModel_t *attachment, cod1DObjSummary_t *out,
    char *error, size_t errorSize );
int G_Cod1ActorRemoveDObjAttachment( const cod1ActorLease_t *lease,
    uint32_t descriptorIndex, cod1DObjSummary_t *out, char *error, size_t errorSize );
int G_Cod1ActorClearDObjAttachments( const cod1ActorLease_t *lease,
    cod1DObjSummary_t *out, char *error, size_t errorSize );
/* Internal hidden/native-data constructor stage, not GSC setModel/attach.
 * Copies a versioned bounded nonempty precache request. New CS registration
 * or missing resource acquisition is admitted only while level.spawning;
 * outside that port gate an existing CS and actually loaded bank entry are
 * required. This is not a claim about the live original CRT/loadgate state.
 * Admitted name identity follows the existing bounded ASCII backend policy.
 * First matching CS before first empty wins; indices1..255. Full original
 * spelling remains separate from the normalized FS path/resource bank slot.
 * Backend refusal leaves old certificates/output unchanged. New publication
 * occurs only after genuine acquisition; callback invalidation after Set CS
 * can retain its external effect, never rolled back across another owner/map.
 * Inputs/output/error ranges are separate. No actor/native model byte, DObj,
 * pose/link/AI readiness or GSC binding is committed. EXE/GAME ABI57 required. */
int G_Cod1ActorAcquireModelRegistration( const cod1ActorLease_t *lease,
    const cod1ModelPrecache_t *model, cod1ActorModelRegistration_t *out,
    char *error, size_t errorSize );
/* Rechecks exact lease, saved map certificate, actual CS spelling and current
 * server resource generation. Requires the same hidden/native-data stage.
 * May cross a CS-read callback; rejects reentry/stale state. A success is a
 * snapshot and must be repeated at the next consuming operation. */
int G_Cod1ActorModelRegistrationCurrent( const cod1ActorModelRegistration_t *model,
    char *error, size_t errorSize );
/* R125 read of the existing provisional topology, with caller's exact Build
 * serial. The same hidden/native-data lease is checked before/after the trap;
 * absent or unknown revision is refused. BONE missing is success with -1.
 * tag points to a bounded 128-byte C string. MODEL requires it empty.
 * Input bytes are copied; rejected callbacks never publish output or dispose
 * another topology. No script/AI admission, tag pose or FX event is implied. */
int G_Cod1ActorQueryDObj( const cod1ActorLease_t *lease,
    uint64_t topologySerial, uint32_t phase, const char *tag,
    cod1DObjQueryResult_t *out, char *error, size_t errorSize );
/* R126 pure capture of an existing provisional topology by script reference.
 * The reverse actor and sentient ownership links are checked, never created.
 * This copied lease is a snapshot, not a pin; the query must revalidate it.
 * Missing/unavailable topology leaves output unchanged. No traps are called. */
typedef struct {
    cod1ActorLease_t lease;
    uint64_t topologySerial;
} cod1ActorDObjLease_t;
int G_Cod1ActorCaptureDObj( const GscEntityRef *entity,
    cod1ActorDObjLease_t *out, char *error, size_t errorSize );
/* Exact owned reservation retirement, not entity free. A spatially linked
 * lease first performs checked keyed unlink; failure retains its resources.
 * Retirement also removes the sentient and every reverse link. */
int G_Cod1ActorRelease( const cod1ActorLease_t *lease,
    char *error, size_t errorSize );
/* R190 lease-owned selected critical state. Array limits match R189's proven
 * kernel, not a native Actor/SAVE overlay. No completed AI admission implied. */
typedef struct {
    cod1ActorLease_t lease;
    int32_t command_operations[13],command_states[13],command_count;
    int32_t stack[6],top,native_cell02a4;
    int has_label,has_script_label;
    char label[128],script_label[128];
} cod1ActorCriticalSnapshot_t;
int G_Cod1ActorLocomotionSource(const GscEntityRef*,uint64_t,int,int,int*,int(*)(void*),void*);
int G_Cod1ActorCriticalRead(const cod1ActorLease_t *,cod1ActorCriticalSnapshot_t *,char *,size_t);
/* Explicit current-script metadata binding, never invented by the VM. NULL
 * clears this metadata. label is a bounded128-byte C string when non-NULL. */
int G_Cod1ActorCriticalSetScriptLabel(const cod1ActorLease_t *,const char *,char *,size_t);
/* Selected GAME representation of the original map-global disable flag.
 * Entity/Actor validity is always checked before this flag by invoke. */
int G_Cod1ActorCriticalSetDisabled(int32_t,char *,size_t);
int G_Cod1ActorCriticalInvoke(const cod1ActorLease_t *,uint32_t,
    const GscActorCriticalCall *,int32_t *,char *,size_t);
const GscActorCriticalHost *G_Cod1ActorCriticalHost(void);
/* Death and resource retirement are different. This does not grant alive or
 * initialized status; a still-provisional record starts with alive 0. */
int G_Cod1ActorDeath( const cod1ActorLease_t *lease,
    char *error, size_t errorSize );
/* R191 selected Actor presence independent of alive/initialized/model state.
 * A successful read returns serial0 for absence, otherwise the owned serial.
 * No allocation or mutation; stale references are false in the bounded port. */
int G_Cod1ActorPresenceRead(const GscEntityRef *,uint64_t *,char *,size_t);
const GscActorPresenceHost *G_Cod1ActorPresenceHost(void);

#endif
#endif

const GscActorCriticalHost*G_Cod1ActorCriticalSessionHost(void);
