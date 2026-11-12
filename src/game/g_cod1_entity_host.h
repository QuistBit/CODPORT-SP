/* R84: checked CoD script-entity references. No game/save/wire struct changes. */
#ifndef G_COD1_ENTITY_HOST_H
#define G_COD1_ENTITY_HOST_H
#if defined(COD1_GSC_TEST_WIP) && COD1_GSC_TEST_WIP
#include "cod1_gsc/gsc_fixture.h"
typedef int (*cod1CarbineSpawnCallback_t)(const char *,const float[3],int32_t,GscEntityRef *,char *,size_t);
int G_Cod1EntitySelectCarbineSpawn(cod1CarbineSpawnCallback_t);
int G_Cod1SpawnTrainingWeapon(const char *,const float[3],int32_t,GscEntityRef *,char *,size_t);
#include "cod1_dobj.h"
#include "g_cod1_actor.h"
void G_Cod1EntityMapBegin( void );
void G_Cod1EntityMapReady( void );
void G_Cod1EntityMapEnd( void );
void G_Cod1EntityBorn( gentity_t *ent );
void G_Cod1EntityLinkOriginBorn(const GscEntityRef*);
int G_Cod1EntityLinksBeforeFree(const GscEntityRef*);
int G_Cod1EntityLinkRunEntity(gentity_t*);
const GscEntityLinkHost*G_Cod1EntityLinkHost(void);
const GscWeaponClassnameHost*G_Cod1WeaponClassnameHost(void);
/* R104 checked DObj cleanup before pure retirement. Failure retains the
 * allocation/lease; callers must not clear it. A trap can change ownership. */
int G_Cod1EntityFree( const GscEntityRef *entity );
/* Private loop owner: retirement precedes memset; no-op for unowned entities. */
int G_Cod1LoopFxBeforeFree(const GscEntityRef *);
void G_Cod1LoopFxAfterFree(const GscEntityRef *);
int G_Cod1LoopFxMapCleanup(void);
int G_Cod1LoopSoundBeforeFree(const GscEntityRef *);
int G_Cod1LoopSoundMapCleanup(void);
/* R86 allocation identity is available during brush spawn, before MapReady.
 * No host traps or pointer retention; MapEnd immediately invalidates it. */
uint64_t G_Cod1EntityMapEpoch( void );
int G_Cod1EntityCapture( const gentity_t *ent, GscEntityRef *out );
int G_Cod1TeleportEffectCurrent(const GscEntityRef *);
int G_Cod1EntitySameAllocation( const GscEntityRef *ref );
/* R87 bounded USE: eligibility is pure; the callback only posts an event.
 * Cmd_Activate_f supplies the separately checked first-hit ray policy. */
int G_Cod1TriggerUseEligible( const gentity_t *ent );
void G_Cod1TriggerMultipleTouch( gentity_t *self, gentity_t *other, trace_t *trace );
void G_Cod1TriggerUse( gentity_t *self, gentity_t *other, gentity_t *activator );
const GscEntityHost *G_Cod1EntityHost( void );
const GscEntityOriginMethodHost *G_Cod1OriginMethodHost(void);
int G_Cod1EntityReadClassnameField(const GscEntityRef *,GscStringCastValue *,char *,size_t);
/* Selected native player entity fields. Maximum is separate from normal-health
 * client maximum. Unknown/unassigned maximum is refused, never fabricated. */
int G_Cod1PlayerHealthFieldKnown(const GscEntityRef *);
int G_Cod1PlayerHealthFieldRead(const GscEntityRef *,const char *,GscStringCastValue *,char *,size_t,int(*)(void*),void*);
int G_Cod1PlayerHealthFieldWrite(const GscEntityRef *,const char *,const GscStringCastValue *,char *,size_t,int(*)(void*),void*);
/* R139 internal partial ammo initialization on a ready CoD SP map. Success
 * copies descriptor/flags/pose/bounds only: no allocation, model registration,
 * ET_ITEM publication or link. The VM spawn capability is not widened. */
struct cod1AmmoSpawnInit_s;
int G_Cod1PrepareAmmoSpawn( const char *classname, const float origin[3],
    int32_t flags, struct cod1AmmoSpawnInit_s *out, char *error, size_t size );
/* R140 strict server XModel registration before allocation. Returned slots
 * belong to the server map cache, never to RTCW ET_ITEM or a client renderer.
 * Failure leaves out unchanged; a committed resource prefix may remain in
 * the map cache. No entity allocation, configstring publication or link. */
struct cod1AmmoSpawnModel_s;
int G_Cod1RegisterAmmoSpawnModel( const char *classname, const float origin[3],
    int32_t flags, struct cod1AmmoSpawnModel_s *out, char *error, size_t size );
/* Pure GAME map/descriptor admission; not a query of the server bank. */
int G_Cod1AmmoSpawnModelCurrent( const struct cod1AmmoSpawnModel_s *model );
/* R141 internal owned allocation candidate, deliberately absent from VM spawn.
 * Copies confirmed flags/origin/bounds and keeps the native descriptor/model in
 * a generation-keyed sidecar. Reserve creates an unlinked, nonsolid SVF_NOCLIENT
 * body; R146 may publish its static visual. Read accepts the staged or exact
 * owned world body and copies the binding. Discard never frees a
 * replacement allocation. Normal GAME free/birth/map hooks retire sidecars. */
int G_Cod1ReserveAmmoSpawn( const char *classname, const float origin[3],
    int32_t flags, GscEntityRef *out, char *error, size_t size );
int G_Cod1ReadAmmoSpawnCandidate( const GscEntityRef *entity,
    struct cod1AmmoSpawnModel_s *out );
int G_Cod1DiscardAmmoSpawn( const GscEntityRef *entity, char *error, size_t size );
/* R143 internal static ammo topology: one descriptor base, empty tag and
 * explicitly selected zero animation tree. This is not live-item publication
 * or a claim that native kind 2 always has a zero tree. Read checks the actual
 * server instance/revision/model before copying its summary. Free preserves
 * the allocation candidate; entity free/birth/map cleanup release ownership.
 * Failure leaves output unchanged. No actor/sentient lease or VM admission. */
int G_Cod1BuildAmmoSpawnDObj( const GscEntityRef *entity,
    cod1DObjSummary_t *out, char *error, size_t size );
int G_Cod1ReadAmmoSpawnDObj( const GscEntityRef *entity, cod1DObjSummary_t *out );
int G_Cod1FreeAmmoSpawnDObj( const GscEntityRef *entity, char *error, size_t size );
/* R144 one-time unpublished native state staging for a current ammo DObj.
 * Copied explicit native seeds preserve prior flags/ground/angles; they are
 * not Q3 fields. A successful DObj rebuild/free invalidates the staged state.
 * Read verifies the actual server bank/revision. No entity/network writes,
 * world link, handler invocation, pickup, physics or VM admission. */
struct cod1AmmoSpawnNativeSeed_s;
struct cod1AmmoSpawnNativeState_s;
int G_Cod1InitializeAmmoSpawnNativeState( const GscEntityRef *entity,
    const struct cod1AmmoSpawnNativeSeed_s *seed,
    struct cod1AmmoSpawnNativeState_s *out, char *error, size_t size );
int G_Cod1ReadAmmoSpawnNativeState( const GscEntityRef *entity,
    struct cod1AmmoSpawnNativeState_s *out );
/* R145 copied static network proposal, not publication. Requires native ground
 * NONE2047, an actual current DObj/state and an independently registered
 * CS_MODELS binding. Call again before a later publisher's action; any further
 * callback can invalidate this snapshot. No gentity writes, link or resource
 * registration. The client recognizes only the strict private ammo marker. */
int G_Cod1ProjectAmmoSpawnPacket( const GscEntityRef *entity,
    int networkModelSlot, entityState_t *out, char *error, size_t size );
/* R146 selected static visual publication. Applies the checked private packet
 * and uses the ordinary engine link, with no RTCW item/physics/pickup admission.
 * Unpublish hides/unlinks and restores the original staged body while retaining
 * native state/DObj. Failed cleanup retains hidden ownership for retry. Entity
 * free/birth/map hooks unpublish before keyed DObj cleanup. World read copies
 * only a committed, currently linked body after actual bank/model revalidation. */
int G_Cod1PublishAmmoSpawn( const GscEntityRef *entity,
    int networkModelSlot, char *error, size_t size );
int G_Cod1UnpublishAmmoSpawn( const GscEntityRef *entity, char *error, size_t size );
int G_Cod1ReadAmmoSpawnWorld( const GscEntityRef *entity, entityState_t *out );
/* R206 internal owned ammo pose only. Updates the verified native angular
 * state and staged/published portable pose together, including the hidden
 * restoration image. No allocation, DObj rebuild, link or VM item admission;
 * the existing raw-ground2047 publication restriction remains unchanged.
 * Engine MODEL/configstring readers are the actual read-only providers; no
 * atomic certificate for a provider returning stale data while mutating its
 * backing bank/configstring table is promised by this internal helper. */
int G_Cod1SetAmmoSpawnAngles( const GscEntityRef *entity, const float angles[3],
    char *error, size_t size );
/* R85 opt-in access contract, separate from the accepted R84 host layout. */
const GscEntityAccessHost *G_Cod1EntityAccessHost( void );
/* R91 separately selected metadata, visibility and admitted trigger pose
 * control. Earlier host layouts and origin admission remain unchanged. */
const GscEntityControlHost *G_Cod1EntityControlHost( void );
/* R93 separately selected, bounded copied entity collections. Lookup supports
 * exact targetname/classname and never truncates or publishes stale members. */
const GscEntityCollectionHost *G_Cod1EntityCollectionHost( void );
/* R131 pure exact ready-map lease; no callbacks or pointer retention. */
int G_Cod1EntityCollectionReady( uint64_t epoch, int entityCount );
/* R149 pure numeric pool read. Missing/out-of-native-range numbers return a
 * copied zero identity with found0. Actual active allocations require exact
 * current epoch/slot/generation and the requested slot. The port's smaller
 * entity capacity is explicit policy, not native pool-state parity. Outputs
 * are separate; failure preserves them. No callbacks, traps or allocation. */
int G_Cod1EntityByNumber( int32_t slot, GscEntityRef *out, int *found,
    char *error, size_t errorSize );
/* R98 separately selected read-only life/sentience predicates. Only the
 * verified nonactor health path and canonical native SP player are admitted;
 * RTCW cast AI is not substituted for a CoD actor/sentient lifecycle. */
const GscEntityPredicateHost *G_Cod1EntityPredicateHost( void );
/* R198 explicitly selected connected-player inventory/input adapter. */
const GscPlayerNativeHost *G_Cod1PlayerNativeHost( void );
void G_Cod1PlayerNativeReset( void );
const GscEntityDeleteHost *G_Cod1EntityDeleteHost( void );
/* R204 separately selected death-notify/free transaction. The legacy R198
 * host and the general ActorOwnerAllowed reservation predicate are unchanged. */
const GscDeleteTransactionHost *G_Cod1EntityDeleteTransactionHost( void );
enum { COD1_ACTOR_DELETE_LIVE=0, COD1_ACTOR_DELETE_ADMITTED=1,
    COD1_ACTOR_DELETE_NOTIFIED=2, COD1_ACTOR_DELETE_RETIRED=3,
    COD1_ACTOR_DELETE_FAILED=4 };
typedef struct {
    uint64_t epoch,receipt,transaction;
    uint32_t row;
    GscEntityRef entity;
    cod1ActorLease_t lease;
} cod1ActorDeleteReceipt_t;
typedef struct {
    GscEntityRef entity;
    cod1ActorLease_t lease;
    uint64_t transaction;
    int state;
} cod1ActorDeleteRowSnapshot_t;
typedef struct {
    uint64_t epoch,receipt;
    uint32_t count,notifiedRows,retiredRows,failedRows;
    cod1ActorDeleteRowSnapshot_t rows[COD1_ACTOR_CAPACITY];
} cod1ActorDeleteSnapshot_t;
typedef struct {
    int (*candidate)(const GscEntityRef *,cod1ActorLease_t *);
    int (*admit)(const GscEntityRef *,const cod1ActorLease_t *,cod1ActorDeleteReceipt_t *);
    int (*notified)(const cod1ActorDeleteReceipt_t *);
    int (*current)(const cod1ActorDeleteReceipt_t *);
    int (*commit)(const cod1ActorDeleteReceipt_t *);
    void (*fail)(const cod1ActorDeleteReceipt_t *);
} cod1ActorDeleteOwnerHooks_t;
/* Actual BSP owner selects its copied operations for this exact map. Keeping
 * this optional table empty never creates an actor deletion authority. */
int G_Cod1EntitySelectActorDeleteOwner(uint64_t,const cod1ActorDeleteOwnerHooks_t *);
/* R240 separate exact original-prefix runtime owner selection. Never enables
 * the VM transaction or broadens ordinary/constructor deletion. */
int G_Cod1EntitySelectRuntimeActorDeleteOwner(uint64_t,uint64_t,const cod1ActorLease_t*);
/* Cheap frame/reentry fence; never admits deletion or proves retirement. */
int G_Cod1EntityRuntimeActorDeleteFrameReady(uint64_t);
int G_Cod1ActorBootstrapDeleteRead(cod1ActorDeleteSnapshot_t *,char *,size_t);
/* Internal synchronous GAME receipt operations. No public completion setter:
 * Commit proves the actual exact-generation free and component retirement. */
int G_Cod1ActorBootstrapDeleteCandidate(const GscEntityRef *,cod1ActorLease_t *);
int G_Cod1ActorBootstrapDeleteAdmit(const GscEntityRef *,const cod1ActorLease_t *,cod1ActorDeleteReceipt_t *);
int G_Cod1ActorBootstrapDeleteNotified(const cod1ActorDeleteReceipt_t *);
int G_Cod1ActorBootstrapDeleteCurrent(const cod1ActorDeleteReceipt_t *);
int G_Cod1ActorBootstrapDeleteCommit(const cod1ActorDeleteReceipt_t *);
void G_Cod1ActorBootstrapDeleteFail(const cod1ActorDeleteReceipt_t *);
int G_Cod1EntityDeleteRetiredIdentity(const GscEntityRef *,const cod1ActorLease_t *);
const GscSpatialNativeHost *G_Cod1SpatialNativeHost( void );
/* R202 read-only actor query. A sealed census with a skipped constructor is
 * deliberately incomplete; reservations alone never prove an empty AI world.
 * This diagnostic has no setter and does not grant actor initialization. */
typedef struct {
    uint64_t epoch,serial;
    uint32_t constructorRows,spawnerRows,unaccountedRows;
    int sealed,complete,invalid;
} cod1AiDomainSnapshot_t;
int G_Cod1AiDomainRead( cod1AiDomainSnapshot_t *out,char *error,size_t size );
const GscAiArrayHost *G_Cod1AiArrayHost( void );
struct GscSessionActorBootstrapHost;
const struct GscSessionActorBootstrapHost *G_Cod1ActorBootstrapHost( void );
/* Exact old epoch only. Refusal retains hidden prefix ownership for retry;
 * a new map's intake/catalog is never retired by an old Session cleanup. */
int G_Cod1ActorBootstrapRetire( uint64_t epoch,char *error,size_t size );
/* R126 VM-shaped read provider for the R124 tagged-FX contract. MODEL may
 * report no model for an ordinary current entity with no model assignment or
 * skeletal reservation, or an eligible provisional actor with no topology
 * and no RTCW model assignment. Unsupported players/brushes/cast entities and
 * inconsistent ownership are refused. BONE requires the captured serial/name.
 * The VM/session owner still guards operation order, logical time and reentry.
 * There is no publisher/constructor selection or client pose in this API. */
int G_Cod1EntityTagFxQuery( void *user, int logicalMs, GscTagFxQuery *query,
    char *error, size_t errorSize );
/* Repeat exact entity/model/bone validation immediately before the eventual
 * publisher's effect. Success is a read preflight, not FX publication. Any
 * intervening engine callback requires a new validation. */
int G_Cod1EntityTagFxValidate( const GscTagFxEvent *event,
    char *error, size_t errorSize );
/* R210 pure memory-range admission for separately selected registry hosts.
 * Includes actual current entities/clients and owned entity/resource banks;
 * invokes no engine callback and grants no setter or ownership capability. */
int G_Cod1EntityStorageProtected(const void *,size_t);
/* Shared bounded model registration implemented with the BSP spawn path.
 * stablePath refers to a map-scoped cache entry, never a transient buffer. */
int G_Cod1ScriptModelResolve( const char *path, const char **stablePath );
#else
#define G_Cod1EntityMapBegin() ((void)0)
#define G_Cod1EntityMapReady() ((void)0)
#define G_Cod1EntityMapEnd() ((void)0)
#define G_Cod1EntityBorn(ent) ((void)0)
#endif
#endif

int G_Cod1GscTestFollowupActive(void);
void G_Cod1HintOwnersReset(void);
int G_Cod1HintIndex(const struct gentity_s *);
int G_Cod1HintWrite(const GscEntityRef *,const GscStringCastValue *,char *,size_t);
int G_Cod1HintRead(const GscEntityRef *,GscStringCastValue *,char *,size_t);
