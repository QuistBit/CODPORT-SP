#ifndef GSC_SESSION_H
#define GSC_SESSION_H
#include <stddef.h>
#include <stdint.h>
#include "gsc_fixture.h"

/* R75: single-threaded, explicitly enabled lifecycle for ONE selected fixture
 * module. No automatic map/dependency discovery, game hooks or original VM ABI.
 * Compile/link/runtime preflight is complete before load returns; load emits no
 * game effects. start must follow the host's real map bootstrap. All callbacks
 * run synchronously. User/config data must outlive the adapter.
 *
 * A successful read transfers a lease, NOT ownership: release is called exactly
 * once, even for invalid bytes or cancellation. On a failed read the host owns
 * cleanup of any partial allocation. Source need not be NUL terminated. It must
 * remain readable for size bytes until release; token is opaque. The adapter
 * copies at most MAX_SOURCE bytes, rejects embedded NUL and releases the lease
 * BEFORE compile. Never return an invalid pointer for a claimed byte range.
 */
#define GSC_SESSION_PATH_CAP 128
#define GSC_SESSION_ERROR_CAP 256
#define GSC_SESSION_MAX_SOURCE 131072u
#define GSC_SESSION_MAX_MODULES 32u
#define GSC_SESSION_LIFETIME_MAX_MODULES 128u
/* Existing R198 execution opt-in only; file/total byte budgets unchanged. */
#define GSC_SESSION_EXTENDED_MAX_MODULES 256u
#define GSC_SESSION_MAX_TOTAL_SOURCE 1048576u
#define GSC_SESSION_EXTENDED_MAX_SOURCE 1048576u
#define GSC_SESSION_EXTENDED_MAX_TOTAL_SOURCE 8388608u
typedef struct GscSession GscSession;
/* Pure identity query for a current compiled/prepared program. DATA only:
 * no runtime, execution certificate, callbacks or lifetime extension. */
uint64_t gsc_session_prepared_program_identity(const GscSession *,uint64_t);
/* Selected synchronous factory-prepare service. No runtime pointer escapes;
 * caller/session ID and the VM's exact source/task scope must both be current.
 * Partial native owners remain caller-owned on refusal. No readiness/publication. */
/* One actual allocated BSP Actor, same compiled program, no script task.
 * Host callbacks are borrowed for this call only. Runtime survives until
 * explicit Session retirement; ordinary prepare/start cannot recreate it. */
typedef struct {
 void *user;int (*current)(void *,const GscEntityRef *);
 GscPreparedEntityEntryHost native;
} GscSessionBspConstructionHost;
int gsc_session_construct_bsp_actor(GscSession *,uint64_t,const GscPreparedEntityEntry *,const GscSessionBspConstructionHost *,GscPreparedEntityEntryRun *,char *,size_t);
int gsc_session_bsp_fields_current(const GscSession *,uint64_t,const GscPreparedEntityEntry *);
int gsc_session_spawn_entry_storage_separate(const GscSession *,const void *,size_t);
int gsc_session_execute_spawn_init(GscSession *,uint64_t,const GscEntityRef *,const GscEntityRef *,uint64_t,GscEntityScriptRun *,char *,size_t);
uint64_t gsc_session_spawn_prepare_program(const GscSession *,uint64_t,const GscActorSpawnRequest *);
uint64_t gsc_session_spawn_commit_program(const GscSession *,uint64_t,const GscActorSpawnTicket *);
uint64_t gsc_session_spawn_finish_program(const GscSession *,uint64_t,const GscActorSpawnTicket *,int published);
uint64_t gsc_session_spawn_entry_program(const GscSession *,uint64_t,const GscEntityRef *);
uint64_t gsc_session_spawn_issued_owner_program(const GscSession *,uint64_t,const GscEntityRef *,const GscEntityRef *,uint64_t);
int gsc_session_execute_projected_spawn_entry(GscSession *,uint64_t,
 const GscInheritedEntityEntry *,const GscProjectedEntityEntryHost *,
 GscProjectedEntityEntryRun *,char *,size_t);

/* Preparation only: acquire the native implicit original scripted::init edge
 * and require whole-function runtime admission before PREPARED. No native
 * method/controller admission, actor mutation or init execution is implied. */
int gsc_session_select_r243_scripted_init_dependency(GscSession *,char *,size_t);
int gsc_session_r243_scripted_init_dependency_selected(const GscSession *);
int gsc_session_select_map_script_state(GscSession*,const GscActorScriptStateHost*,char*,size_t);
int gsc_session_select_r243_script_state(GscSession*,const GscActorScriptStateHost*,char*,size_t);
int gsc_session_select_r243_scripted_start(GscSession*,const GscActorScriptedStartHost*,char*,size_t);
int gsc_session_select_r243_scripted_prefix(GscSession *,const GscActorScriptedPrefixHost *,char *,size_t);
/* Idle array-capable Session only; ordinary private arrays, no persistence or
 * indexed undefined deletion extension. Runtime numeric key bounds remain. */
int gsc_session_select_sparse_numeric_arrays(GscSession *,char *,size_t);
int gsc_session_select_r233_owned_numeric_delete(GscSession *,char *,size_t);
int gsc_session_select_r238_array_value_cow(GscSession *,char *,size_t);
int gsc_session_r238_array_value_cow_selected(const GscSession *);
int gsc_session_select_r243_same_bucket_wait(GscSession *,char *,size_t);
int gsc_session_r243_same_bucket_wait_selected(const GscSession *);
int gsc_session_select_configured_precache_interval(GscSession *,char *,size_t);
int gsc_session_select_r243_thread_start(GscSession *,char *,size_t);
int gsc_session_r243_thread_start_selected(const GscSession *);
int gsc_session_select_r243_thread_entities(GscSession *,const GscThreadEntityAuthorityHost *,char *,size_t);
int gsc_session_select_r239_mixed_scalar_equality(GscSession *,char *,size_t);
int gsc_session_r239_mixed_scalar_equality_selected(const GscSession *);
int gsc_session_sound_notify_native(GscSession *,uint64_t,const GscEntityRef *,const char *,int *,char *,size_t);
int gsc_session_select_sound_notify(GscSession *,const GscSoundNotifyHost *,char *,size_t);
int gsc_session_select_local_identifier_case(GscSession *,char *,size_t);
int gsc_session_select_loop_sound(GscSession *,const GscLoopSoundHost *,char *,size_t);
int gsc_session_select_script_sound(GscSession *,const GscScriptSoundHost *,char *,size_t);
/* R198 cumulative opt-in: vectors, endon, int32 console display, large source
 * files and typed player methods. Select on an idle mode40 session before load. */
int gsc_session_select_owned_wait_delivery(GscSession *,char *,size_t);
int gsc_session_select_r198_execution(GscSession *,const GscPlayerNativeHost *,char *,size_t);
int gsc_session_select_entity_links(GscSession*,const GscEntityLinkHost*,char*,size_t);
int gsc_session_select_r198_entity_operations(GscSession *,const GscEntityDeleteHost *,char *,size_t);
int gsc_session_select_origin_method(GscSession *,const GscEntityOriginMethodHost *,char *,size_t);
int gsc_session_select_r198_spatial_natives(GscSession *,const GscSpatialNativeHost *,char *,size_t);
int gsc_session_select_vehicle_nodes(GscSession*,const GscVehicleNodeHost*,char*,size_t);
int gsc_session_select_r199_node_queries(GscSession *,const GscNodeQueryHost *,char *,size_t);
/* Startup HUD does not require path-node queries. Same typed owner/retirement service. */
int gsc_session_select_startup_hud(GscSession *,const GscHudElemHost *,char *,size_t);
int gsc_session_select_r200_hud_elements(GscSession *,const GscHudElemHost *,char *,size_t);
int gsc_session_select_r200_entity_collision(GscSession *,const GscEntityCollisionHost *,char *,size_t);
typedef struct { const char *data; size_t size; void *token; } GscSessionSource;
typedef int (*GscSessionRead)(void *, const char *path, GscSessionSource *out,
                               char *error, size_t error_size);
typedef void (*GscSessionRelease)(void *, const GscSessionSource *source);
/* Event and transition_ms are copied unchanged. Pointer valid only during call.
 * No rollback/restoration of events already accepted by the host is promised. */
typedef int (*GscSessionFog)(void *, uint64_t session_id, int logical_ms,
                              const GscFogEvent *event);
typedef struct {
    int enabled; /* 0 disables all reads/start; 1 explicitly opts in. Immutable. */
    void *user;
    GscSessionRead read;
    GscSessionRelease release;
    GscSessionFog fog;
} GscSessionHost;
enum { GSC_SESSION_DISABLED=0, GSC_SESSION_EMPTY, GSC_SESSION_PREPARED,
       GSC_SESSION_RUNNING, GSC_SESSION_COMPLETED, GSC_SESSION_FAILED, GSC_SESSION_COMPILED };
typedef struct {
    int state, busy, has_program, has_runtime, active_threads;
    uint64_t session_id;
    int64_t origin_host_ms, last_host_ms; /* -1 before start */
    int logical_ms;                    /* -1 before start */
    char path[GSC_SESSION_PATH_CAP], module[GSC_SESSION_PATH_CAP];
    char error[GSC_SESSION_ERROR_CAP]; /* first fatal error; not stale requests */
    GscRuntimeNativeStats native;
    GscRuntimeFogNativeStats fog;
} GscSessionSnapshot;

int gsc_session_select_player_normal_health(GscSession *,char *,size_t);
int gsc_session_select_player_viewmodel(GscSession *,char *,size_t);
int gsc_session_select_player_max_ammo(GscSession *,char *,size_t);
int gsc_session_select_player_stance_permissions(GscSession *,char *,size_t);
/* Explicit map-local game array for startup scripts without Actor bootstrap.
 * A fresh runtime starts empty; no cross-map carry or SAVE claim. */
int gsc_session_select_map_game_array(GscSession *,char *,size_t);
int gsc_session_select_loop_fx(GscSession *,const GscLoopFxHost *,char *,size_t);
GscSession *gsc_session_create(const GscSessionHost *, char *error, size_t size);
/* Explicit R84 constructor. The old host layout and constructor stay scalar.
 * Entity callbacks are never entered during create/load/preflight. */
GscSession *gsc_session_create_with_entities(const GscSessionHost *,
    const GscEntityHost *, char *error, size_t size);
/* R85 selects optional model/property capabilities separately. Retained
 * source nodes determine which callbacks are required during load preflight. */
GscSession *gsc_session_create_with_access(const GscSessionHost *,
    const GscEntityHost *, const GscEntityAccessHost *, char *error, size_t size);
/* Explicit R86 superset of access mode, using the existing host layouts.
 * Earlier constructors leave entity events unavailable. */
GscSession *gsc_session_create_with_events(const GscSessionHost *,
    const GscEntityHost *, const GscEntityAccessHost *, char *error, size_t size);
/* R88 explicit state superset. Bags and receiver contexts are private to this
 * session's runtime and are retired on replacement, failure, unload/destroy.
 * Create/load/preflight never enter entity callbacks. Older modes are frozen. */
GscSession *gsc_session_create_with_state(const GscSessionHost *,
    const GscEntityHost *, const GscEntityAccessHost *, char *error, size_t size);
/* R91 separate controls capability selection. Load/preflight never invoke it.
 * The prior constructors and host layouts remain unchanged. */
GscSession *gsc_session_create_with_controls(const GscSessionHost *,
    const GscEntityHost *, const GscEntityAccessHost *,
    const GscEntityControlHost *, char *error, size_t size);
/* R92 flow is selected explicitly; earlier sessions retain prior guards. */
GscSession *gsc_session_create_with_flow(const GscSessionHost *,
    const GscEntityHost *, const GscEntityAccessHost *,
    const GscEntityControlHost *, char *error, size_t size);
/* R93 explicit arrays constructor; collection capability is required only
 * for retained getentarray nodes and is never entered during prepare/load. */
GscSession *gsc_session_create_with_arrays(const GscSessionHost *,
    const GscEntityHost *, const GscEntityAccessHost *,
    const GscEntityControlHost *, const GscEntityCollectionHost *,
    char *error, size_t size);
/* R94 explicit resumable result superset of arrays mode. */
GscSession *gsc_session_create_with_results(const GscSessionHost *,
    const GscEntityHost *, const GscEntityAccessHost *,
    const GscEntityControlHost *, const GscEntityCollectionHost *,
    char *error, size_t error_size);
/* R95 explicit dependency-loader superset. Whole modules are discovered from
 * retained typed qualified calls/threads/function references, including dead
 * functions, then linked/preflighted together before any execution. BFS order
 * follows the existing instruction then expression audit order; cycles and
 * duplicate paths read once. Bounds: 32 modules, 131072 bytes each, 1 MiB total.
 * Module paths use an explicit ASCII lowercase/slash VFS identity policy;
 * functions remain case-sensitive. #include/computed module discovery and
 * missing-native substitution remain unsupported. Earlier constructors read
 * exactly one selected file under their previous path and capability policy. */
GscSession *gsc_session_create_with_modules(const GscSessionHost *,
    const GscEntityHost *, const GscEntityAccessHost *,
    const GscEntityControlHost *, const GscEntityCollectionHost *,
    char *error, size_t error_size);
/* R96 explicit payload superset of module loading. The payload transport and
 * local-output rules are documented in gsc_fixture.h. Older sessions never
 * enable this grammar or payload APIs; USE/TOUCH may still deliver no values. */
GscSession *gsc_session_create_with_event_payloads(const GscSessionHost *,
    const GscEntityHost *, const GscEntityAccessHost *,
    const GscEntityControlHost *, const GscEntityCollectionHost *,
    char *error, size_t error_size);
/* R98 predicate superset. Retained calls require their corresponding callback,
 * including unreachable bodies. Load/preflight never invoke predicate hosts. */
GscSession *gsc_session_create_with_predicates(const GscSessionHost *,
    const GscEntityHost *, const GscEntityAccessHost *,
    const GscEntityControlHost *, const GscEntityCollectionHost *,
    const GscEntityPredicateHost *, char *error, size_t error_size);
/* R101 explicitly selected actor-spawner superset; load never enters spawn
 * callbacks. No production actor constructor is supplied by this adapter. */
GscSession *gsc_session_create_with_actor_spawners(const GscSessionHost *,
    const GscEntityHost *, const GscEntityAccessHost *,
    const GscEntityControlHost *, const GscEntityCollectionHost *,
    const GscEntityPredicateHost *, const GscActorSpawnHost *, char *, size_t);
/* R102 internal script function-reference/computed-call superset. All hosts
 * retain their earlier layouts. Expression thread results remain unsupported. */
GscSession *gsc_session_create_with_function_refs(const GscSessionHost *,
    const GscEntityHost *, const GscEntityAccessHost *,
    const GscEntityControlHost *, const GscEntityCollectionHost *,
    const GscEntityPredicateHost *, const GscActorSpawnHost *, char *, size_t);
/* R105 scalar/switch module session; whole-module loader and guards retained. */
GscSession *gsc_session_create_with_scalar_switch(const GscSessionHost *,
    const GscEntityHost *, const GscEntityAccessHost *,
    const GscEntityControlHost *, const GscEntityCollectionHost *,
    const GscEntityPredicateHost *, const GscActorSpawnHost *, char *, size_t);
/* R106 explicitly selects the lifetime VM and a 128-whole-module loader.
 * This is a bounded port policy, not a recovered original module limit.
 * Per-source 131072-byte and 1 MiB total limits, discovery order, canonical
 * identity, lease cleanup and full preflight remain unchanged. All earlier
 * constructors, including scalar/switch, retain their 32-module limit.
 * No additional native, animation or mission capability is implied. */
GscSession *gsc_session_create_with_lifetime(const GscSessionHost *,
    const GscEntityHost *, const GscEntityAccessHost *,
    const GscEntityControlHost *, const GscEntityCollectionHost *,
    const GscEntityPredicateHost *, const GscActorSpawnHost *, char *, size_t);
/* R107 selected callable identity and real optional music effects. Session IDs
 * are forwarded only while RUNNING. Load/preflight never call music hosts.
 * Earlier constructor and host layouts remain unchanged. */
typedef int (*GscSessionMusicCallback)(void *, uint64_t session_id, int logical_ms,
    const GscMusicEvent *, char *error, size_t error_size);
typedef struct { void *user; GscSessionMusicCallback play, stop; } GscSessionMusicHost;
GscSession *gsc_session_create_with_names(const GscSessionHost *,
    const GscEntityHost *, const GscEntityAccessHost *,
    const GscEntityControlHost *, const GscEntityCollectionHost *,
    const GscEntityPredicateHost *, const GscActorSpawnHost *, char *, size_t);
GscSession *gsc_session_create_with_music(const GscSessionHost *,
    const GscEntityHost *, const GscEntityAccessHost *,
    const GscEntityControlHost *, const GscEntityCollectionHost *,
    const GscEntityPredicateHost *, const GscActorSpawnHost *, const GscSessionMusicHost *, char *, size_t);
/* Resource callbacks carry the current session ID and run only during start
 * or advance. Earlier constructor and host layouts remain unchanged. */
typedef int (*GscSessionResourceCallback)(void *, uint64_t session_id, int logical_ms,
    const GscResourceEvent *, char *error, size_t error_size);
typedef struct { void *user; GscSessionResourceCallback shader, model; } GscSessionResourceHost;
GscSession *gsc_session_create_with_resources(const GscSessionHost *,
    const GscEntityHost *, const GscEntityAccessHost *,
    const GscEntityControlHost *, const GscEntityCollectionHost *,
    const GscEntityPredicateHost *, const GscActorSpawnHost *, const GscSessionMusicHost *,
    const GscSessionResourceHost *, char *, size_t);
/* R109 separate optional objective effects. Every retained native requires its
 * individual callback. Events are borrowed, copied and carry the current ID;
 * failed/stale/reentrant effects do not authorize further script execution. */
typedef int (*GscSessionObjectiveCallback)(void *, uint64_t session_id, int logical_ms,
    const GscObjectiveEvent *, char *error, size_t error_size);
typedef struct { void *user; GscSessionObjectiveCallback add, delete_record, state,
    string, string_nomessage, position, current, ring; } GscSessionObjectiveHost;
GscSession *gsc_session_create_with_objectives(const GscSessionHost *,
    const GscEntityHost *, const GscEntityAccessHost *,
    const GscEntityControlHost *, const GscEntityCollectionHost *,
    const GscEntityPredicateHost *, const GscActorSpawnHost *, const GscSessionMusicHost *,
    const GscSessionResourceHost *, const GscSessionObjectiveHost *, char *, size_t);
/* R110 console callbacks carry the active session ID. Gate and sink callbacks
 * are synchronous and copied; rejected reentrant mutation aborts the current
 * call before any later argument or newline sink can execute. */
typedef int (*GscSessionConsoleGateCallback)(void *, uint64_t session_id, int logical_ms,
    int kind, int phase, int *enabled, char *error, size_t error_size);
typedef int (*GscSessionConsoleCallback)(void *, uint64_t session_id, int logical_ms,
    const GscConsoleEvent *, char *error, size_t error_size);
typedef struct { void *user; GscSessionConsoleGateCallback query_gate;
    GscSessionConsoleCallback print, println; } GscSessionConsoleHost;
GscSession *gsc_session_create_with_console(const GscSessionHost *,
    const GscEntityHost *, const GscEntityAccessHost *,
    const GscEntityControlHost *, const GscEntityCollectionHost *,
    const GscEntityPredicateHost *, const GscActorSpawnHost *, const GscSessionMusicHost *,
    const GscSessionResourceHost *, const GscSessionObjectiveHost *, const GscSessionConsoleHost *,
    char *, size_t);
/* R111 typed HUD callbacks carry the active ID. No console gate is queried.
 * Borrowed events are copied; failed/reentrant effects abort the outer call. */
typedef int (*GscSessionHudCallback)(void *, uint64_t session_id, int logical_ms,
    const GscHudEvent *, char *error, size_t error_size);
typedef struct { void *user; GscSessionHudCallback print, bold; } GscSessionHudHost;
GscSession *gsc_session_create_with_hud(const GscSessionHost *,
    const GscEntityHost *, const GscEntityAccessHost *,
    const GscEntityControlHost *, const GscEntityCollectionHost *,
    const GscEntityPredicateHost *, const GscActorSpawnHost *, const GscSessionMusicHost *,
    const GscSessionResourceHost *, const GscSessionObjectiveHost *, const GscSessionConsoleHost *,
    const GscSessionHudHost *, char *, size_t);
/* R112 phased diagnostic mission effects carry the active session ID. Gates
 * and accepted prefixes are synchronous; failed/reentrant callbacks cancel the
 * remaining phases, while accepted prior diagnostic fields remain retained. */
typedef int (*GscSessionMissionGateCallback)(void *, uint64_t session_id, int logical_ms,
    int kind, int *enabled, char *error, size_t error_size);
typedef int (*GscSessionMissionCallback)(void *, uint64_t session_id, int logical_ms,
    const GscMissionEvent *, char *error, size_t error_size);
typedef struct { void *user; GscSessionMissionGateCallback query_gate;
    GscSessionMissionCallback success, failed; } GscSessionMissionHost;
GscSession *gsc_session_create_with_mission(const GscSessionHost *,
    const GscEntityHost *, const GscEntityAccessHost *,
    const GscEntityControlHost *, const GscEntityCollectionHost *,
    const GscEntityPredicateHost *, const GscActorSpawnHost *, const GscSessionMusicHost *,
    const GscSessionResourceHost *, const GscSessionObjectiveHost *, const GscSessionConsoleHost *,
    const GscSessionHudHost *, const GscSessionMissionHost *, char *, size_t);
/* R113 radius effects carry the active ID and an independent copied payload.
 * Callback failure or reentrant mutation aborts remaining VM statements. */
typedef int (*GscSessionBlastCallback)(void *, uint64_t session_id, int logical_ms,
    const GscBlastEvent *, char *error, size_t error_size);
typedef struct { void *user; GscSessionBlastCallback radius; } GscSessionBlastHost;
GscSession *gsc_session_create_with_blast(const GscSessionHost *,
    const GscEntityHost *, const GscEntityAccessHost *,
    const GscEntityControlHost *, const GscEntityCollectionHost *,
    const GscEntityPredicateHost *, const GscActorSpawnHost *, const GscSessionMusicHost *,
    const GscSessionResourceHost *, const GscSessionObjectiveHost *, const GscSessionConsoleHost *,
    const GscSessionHudHost *, const GscSessionMissionHost *, const GscSessionBlastHost *, char *, size_t);
/* R114 earthquake is selected independently; host receives a copied typed
 * event with current session ID. No emit during create/load/preflight. */
typedef int (*GscSessionQuakeCallback)(void *, uint64_t session_id, int logical_ms,
    const GscQuakeEvent *, char *error, size_t error_size);
typedef struct { void *user; GscSessionQuakeCallback emit; } GscSessionQuakeHost;
typedef int (*GscSessionFxLoadCallback)(void *, uint64_t session_id, int logical_ms,
    const GscFxLoadEvent *, int32_t *index_out, char *error, size_t error_size);
typedef int (*GscSessionFxPlayCallback)(void *, uint64_t session_id, int logical_ms,
    const GscFxPlayEvent *, char *error, size_t error_size);
typedef struct { void *user; GscSessionFxLoadCallback load;
    GscSessionFxPlayCallback play; } GscSessionFxHost;
typedef int (*GscSessionCvarCallback)(void *, uint64_t session_id, int logical_ms,
    const GscCvarEvent *, char *error, size_t error_size);
typedef struct { void *user; GscSessionCvarCallback publish; } GscSessionCvarHost;
typedef int (*GscSessionShellshockCallback)(void *, uint64_t session_id, int logical_ms,
    const GscShellshockEvent *, char *error, size_t error_size);
typedef struct { void *user; GscSessionShellshockCallback publish; } GscSessionShellshockHost;
typedef int (*GscSessionDebugTextCallback)(void *, uint64_t session_id, int logical_ms,
    const GscDebugTextEvent *, char *error, size_t error_size);
typedef struct { void *user; GscSessionDebugTextCallback publish; } GscSessionDebugTextHost;
typedef int (*GscSessionDebugLineCallback)(void *, uint64_t session_id, int logical_ms,
    const GscDebugLineEvent *, char *error, size_t error_size);
typedef struct { void *user; GscSessionDebugLineCallback publish; } GscSessionDebugLineHost;
typedef int (*GscSessionAmbientCallback)(void *, uint64_t session_id, int logical_ms,
    const GscAmbientEvent *, char *error, size_t error_size);
typedef struct { void *user; GscSessionAmbientCallback publish; } GscSessionAmbientHost;
/* Item events are copied and forwarded only for the exact running ID/time. */
typedef int (*GscSessionPrecacheItemCallback)(void *, uint64_t session_id, int logical_ms,
    const GscPrecacheItemEvent *, char *error, size_t error_size);
typedef struct { void *user; GscSessionPrecacheItemCallback publish; } GscSessionPrecacheItemHost;
typedef int (*GscSessionSaveGameQueryCallback)(void *, uint64_t, int, GscSaveGameQuery *, char *, size_t);
typedef int (*GscSessionSaveGameCallback)(void *, uint64_t, int, const GscSaveGameEvent *, char *, size_t);
typedef struct { void *user; GscSessionSaveGameQueryCallback query; GscSessionSaveGameCallback publish; } GscSessionSaveGameHost;
GscSession *gsc_session_create_with_quake(const GscSessionHost *,
    const GscEntityHost *, const GscEntityAccessHost *,
    const GscEntityControlHost *, const GscEntityCollectionHost *,
    const GscEntityPredicateHost *, const GscActorSpawnHost *, const GscSessionMusicHost *,
    const GscSessionResourceHost *, const GscSessionObjectiveHost *, const GscSessionConsoleHost *,
    const GscSessionHudHost *, const GscSessionMissionHost *, const GscSessionBlastHost *, const GscSessionQuakeHost *, char *, size_t);
GscSession *gsc_session_create_with_fx(const GscSessionHost *,
    const GscEntityHost *, const GscEntityAccessHost *, const GscEntityControlHost *,
    const GscEntityCollectionHost *, const GscEntityPredicateHost *,
    const GscActorSpawnHost *, const GscSessionMusicHost *,
    const GscSessionResourceHost *, const GscSessionObjectiveHost *, const GscSessionConsoleHost *,
    const GscSessionHudHost *, const GscSessionMissionHost *, const GscSessionBlastHost *, const GscSessionQuakeHost *, const GscSessionFxHost *, char *, size_t);
GscSession *gsc_session_create_with_cvars(const GscSessionHost *,
    const GscEntityHost *, const GscEntityAccessHost *, const GscEntityControlHost *,
    const GscEntityCollectionHost *, const GscEntityPredicateHost *,
    const GscActorSpawnHost *, const GscSessionMusicHost *,
    const GscSessionResourceHost *, const GscSessionObjectiveHost *, const GscSessionConsoleHost *,
    const GscSessionHudHost *, const GscSessionMissionHost *, const GscSessionBlastHost *, const GscSessionQuakeHost *, const GscSessionFxHost *, const GscSessionCvarHost *, char *, size_t);
GscSession *gsc_session_create_with_shellshock(const GscSessionHost *,
    const GscEntityHost *, const GscEntityAccessHost *, const GscEntityControlHost *,
    const GscEntityCollectionHost *, const GscEntityPredicateHost *,
    const GscActorSpawnHost *, const GscSessionMusicHost *,
    const GscSessionResourceHost *, const GscSessionObjectiveHost *, const GscSessionConsoleHost *,
    const GscSessionHudHost *, const GscSessionMissionHost *, const GscSessionBlastHost *, const GscSessionQuakeHost *, const GscSessionFxHost *, const GscSessionCvarHost *, const GscSessionShellshockHost *, char *, size_t);
GscSession *gsc_session_create_with_debug_text(const GscSessionHost *,
    const GscEntityHost *, const GscEntityAccessHost *, const GscEntityControlHost *,
    const GscEntityCollectionHost *, const GscEntityPredicateHost *,
    const GscActorSpawnHost *, const GscSessionMusicHost *,
    const GscSessionResourceHost *, const GscSessionObjectiveHost *, const GscSessionConsoleHost *,
    const GscSessionHudHost *, const GscSessionMissionHost *, const GscSessionBlastHost *, const GscSessionQuakeHost *, const GscSessionFxHost *, const GscSessionCvarHost *, const GscSessionShellshockHost *, const GscSessionDebugTextHost *, char *, size_t);
GscSession *gsc_session_create_with_debug_line(const GscSessionHost *,
    const GscEntityHost *, const GscEntityAccessHost *, const GscEntityControlHost *,
    const GscEntityCollectionHost *, const GscEntityPredicateHost *,
    const GscActorSpawnHost *, const GscSessionMusicHost *,
    const GscSessionResourceHost *, const GscSessionObjectiveHost *, const GscSessionConsoleHost *,
    const GscSessionHudHost *, const GscSessionMissionHost *, const GscSessionBlastHost *, const GscSessionQuakeHost *, const GscSessionFxHost *, const GscSessionCvarHost *, const GscSessionShellshockHost *, const GscSessionDebugTextHost *, const GscSessionDebugLineHost *, char *, size_t);
GscSession *gsc_session_create_with_ambient(const GscSessionHost *,
    const GscEntityHost *, const GscEntityAccessHost *, const GscEntityControlHost *,
    const GscEntityCollectionHost *, const GscEntityPredicateHost *,
    const GscActorSpawnHost *, const GscSessionMusicHost *,
    const GscSessionResourceHost *, const GscSessionObjectiveHost *, const GscSessionConsoleHost *,
    const GscSessionHudHost *, const GscSessionMissionHost *, const GscSessionBlastHost *, const GscSessionQuakeHost *, const GscSessionFxHost *, const GscSessionCvarHost *, const GscSessionShellshockHost *, const GscSessionDebugTextHost *, const GscSessionDebugLineHost *, const GscSessionAmbientHost *, char *, size_t);
GscSession *gsc_session_create_with_precache_item(const GscSessionHost *,
    const GscEntityHost *, const GscEntityAccessHost *, const GscEntityControlHost *,
    const GscEntityCollectionHost *, const GscEntityPredicateHost *,
    const GscActorSpawnHost *, const GscSessionMusicHost *,
    const GscSessionResourceHost *, const GscSessionObjectiveHost *, const GscSessionConsoleHost *,
    const GscSessionHudHost *, const GscSessionMissionHost *, const GscSessionBlastHost *, const GscSessionQuakeHost *, const GscSessionFxHost *, const GscSessionCvarHost *, const GscSessionShellshockHost *, const GscSessionDebugTextHost *, const GscSessionDebugLineHost *, const GscSessionAmbientHost *, const GscSessionPrecacheItemHost *, char *, size_t);
GscSession *gsc_session_create_with_save_game(const GscSessionHost *,
    const GscEntityHost *, const GscEntityAccessHost *, const GscEntityControlHost *,
    const GscEntityCollectionHost *, const GscEntityPredicateHost *,
    const GscActorSpawnHost *, const GscSessionMusicHost *,
    const GscSessionResourceHost *, const GscSessionObjectiveHost *, const GscSessionConsoleHost *,
    const GscSessionHudHost *, const GscSessionMissionHost *, const GscSessionBlastHost *, const GscSessionQuakeHost *, const GscSessionFxHost *, const GscSessionCvarHost *, const GscSessionShellshockHost *, const GscSessionDebugTextHost *, const GscSessionDebugLineHost *, const GscSessionAmbientHost *, const GscSessionPrecacheItemHost *, const GscSessionSaveGameHost *, char *, size_t);
/* Destruction from any callback is refused WITHOUT freeing. A rejected
 * reentrant mutation poisons the outer operation; cleanup waits for VM unwind.
 * Caller may query from callbacks, but may not mutate, destroy or concurrently
 * access the adapter. destroy(NULL) succeeds. On return 0 keep the pointer. */
int gsc_session_destroy(GscSession *, char *error, size_t size);

/* Every admitted load first retires the previous VM, even if replacement fails.
 * IDs never wrap; no previous VM is resumed on failure. Path is a canonical
 * relative ASCII identifier segments (letter/underscore first, then digits also),
 * slash separator, exact .gsc
 * suffix; no dots in the stem, absolute paths, traversal or backslashes.
 * Host must ALSO enforce its own VFS/root policy. The R95 modules constructor
 * additionally accepts backslashes and suffix case, then calls host.read only
 * with ASCII lowercase forward-slash paths and exact .gsc suffix. Dependencies
 * follow that same policy; no implicit filesystem-root permission is granted.
 * bootstrap_ready must be 1.
 * out_session is required, set to 0 on failure. Even a failed load has an ID in
 * snapshot for diagnostics/unload. No implicit main execution on successful load.
 */
/* Explicit split preparation. COMPILED owns one linked program and no runtime,
 * tasks or effects. Existing selected owner/real-cohort guards still apply;
 * this does not itself permit pre-BSP selection. Runtime preparation rechecks
 * owners and never reloads/relinks the program. Unload/destroy retire either
 * stage. Selectors remain closed while the program exists. */
int gsc_session_prepare_program(GscSession *,const char *,int,uint64_t *,char *,size_t);
int gsc_session_prepare_runtime(GscSession *,uint64_t,char *,size_t);
void gsc_session_bind_readonly_context(GscSession *, uint64_t, const GscEntityHost *,
    const GscReadOnlyContext *);
int gsc_session_load(GscSession *, const char *path, int bootstrap_ready,
                       uint64_t *out_session, char *error, size_t size);
int gsc_session_start(GscSession *, uint64_t session_id, int64_t host_now_ms,
                        char *error, size_t size);
/* Time is host_now - start_time; checked before conversion to VM's signed int.
 * Negative, backward or >INT_MAX elapsed time fails/stops the current session.
 * Equal times allowed. No synthetic catch-up frames: existing VM timing rules.
 * Wrong ID/state only rejects the request; it cannot kill a newer valid VM. */
int gsc_session_advance(GscSession *, uint64_t session_id, int64_t host_now_ms,
                          char *error, size_t size);
/* Queue a level event at the last logical clock. No execution until advance. */
int gsc_session_notify(GscSession *, uint64_t session_id, const char *event,
                         char *error, size_t size);
/* Stale ID/owner or invalid event rejects only the request. No queued event
 * survives replacement. Matched tasks remain deferred until advance; a
 * retired owner cancels its pending waiter task, never retargets its slot.
 * out_awakened is required, reset before all validation, and counts only newly
 * READY matching tasks. A reentrant call still poisons the outer operation. */
int gsc_session_notify_entity(GscSession *, uint64_t session_id,
    const GscEntityRef *owner, const char *event, int *out_awakened,
    char *error, size_t size);
/* Copies borrowed values before callbacks; invalid/stale requests reject only
 * that request. out_awakened is required and reset before validation. Delivery
 * binds existing current waiters atomically and execution remains deferred. */
int gsc_session_notify_with_payload(GscSession *, uint64_t session_id,
    const char *event, const GscEventValue *values, size_t count,
    int *out_awakened, char *error, size_t size);
/* R233 external animation receipt lane. GAME owns clip crossing, cancellation
 * and receipt generation. Session verifies current VM/catalog and queues the
 * typed end payload for existing entity waiters without executing a scheduler.
 * produced_ms is the GAME clock and is not Session logical time. */
typedef struct {GscEntityRef entity;uint64_t runtime,catalog,owner_identity,generation;uint32_t ordinal;int32_t produced_ms;char event[64],payload[4];} GscActorAnimationEnd;
int gsc_session_select_r233_actor_animation_end(GscSession*,char*,size_t);
int gsc_session_notify_actor_animation_end(GscSession*,uint64_t,const GscActorAnimationEnd*,int*,char*,size_t);
int gsc_session_notify_entity_with_payload(GscSession *, uint64_t session_id,
    const GscEntityRef *owner, const char *event,
    const GscEventValue *values, size_t count, int *out_awakened,
    char *error, size_t size);
/* R113 independently selected deferred combat lane. RUNNING uses the genuine
 * typed runtime notifier; COMPLETED admits only a started zero-active-task
 * blast VM and succeeds with zero recipients, after ordinary owner/payload
 * validation. READY/SLEEP/EVENT tasks all count as active. No scheduler executes
 * here, no completed script is restarted, and old notify APIs retain their
 * RUNNING requirement. Only damage(number,entity-or-undefined) and
 * death(entity-or-undefined) payload grammar is admitted; inputs copy before
 * owner validation. Stale ID/ref rejects only this request; callback reentry
 * still poisons the outer operation and resets out_awakened. */
int gsc_session_blast_notify_entity_with_payload(GscSession *, uint64_t session_id,
    const GscEntityRef *owner, const char *event,
    const GscEventValue *values, size_t count, int *out_awakened,
    char *error, size_t size);
/* Read-only matching-registration capture is safe while the blast callback is
 * active/busy. It invokes no callbacks and does not poison a running session on
 * a stale capture request. Drain copies the set before validation, binds only
 * the same birth+wait generation, and rejects replaced runtime/session IDs. */
int gsc_session_blast_capture_waiters(const GscSession *, uint64_t session_id,
    const GscEntityRef *owner, const char *event, GscBlastWaiterSet *, char *, size_t);
int gsc_session_blast_notify_captured(GscSession *, uint64_t session_id,
    const GscBlastWaiterSet *, const GscEventValue *, size_t, int *out_awakened,
    char *, size_t);
int gsc_session_unload(GscSession *, uint64_t session_id, char *error, size_t size);
/* Allocation-free copy, allowed during callbacks. Stats remain available after
 * fatal cleanup, until new load/unload. Does not expose VM or program pointers. */
int gsc_session_model_starting(const GscSession *,uint64_t,int);
int gsc_session_running_at(const GscSession *,uint64_t id,int logical,int host);
int gsc_session_is_running(const GscSession *, uint64_t id);
int gsc_session_snapshot(const GscSession *, GscSessionSnapshot *out);
/* Live control fields only. No task census, statistics, callbacks, cache or
 * execution authority. Callers retain their existing identity/time checks. */
typedef struct {
    uint64_t session_id;
    int64_t origin_host_ms,last_host_ms;
    int state,busy,logical_ms,has_error;
} GscSessionTiming;
int gsc_session_timing(const GscSession *,GscSessionTiming *out);
/* R124 copied, session/time fenced tagged-FX capability. The real game
 * provider/client tag-pose consumer are separate from this adapter contract. */
typedef int (*GscSessionTagFxQueryCallback)(void *, uint64_t, int, GscTagFxQuery *, char *, size_t);
typedef int (*GscSessionTagFxCallback)(void *, uint64_t, int, const GscTagFxEvent *, char *, size_t);
typedef struct { void *user; GscSessionTagFxQueryCallback query;
    GscSessionTagFxCallback publish; } GscSessionTagFxHost;
GscSession *gsc_session_create_with_tag_fx(const GscSessionHost *,
    const GscEntityHost *, const GscEntityAccessHost *, const GscEntityControlHost *,
    const GscEntityCollectionHost *, const GscEntityPredicateHost *,
    const GscActorSpawnHost *, const GscSessionMusicHost *,
    const GscSessionResourceHost *, const GscSessionObjectiveHost *, const GscSessionConsoleHost *,
    const GscSessionHudHost *, const GscSessionMissionHost *, const GscSessionBlastHost *, const GscSessionQuakeHost *, const GscSessionFxHost *, const GscSessionCvarHost *, const GscSessionShellshockHost *, const GscSessionDebugTextHost *, const GscSessionDebugLineHost *, const GscSessionAmbientHost *, const GscSessionPrecacheItemHost *, const GscSessionSaveGameHost *, const GscSessionTagFxHost *, char *, size_t);
/* R128 synchronous copied native query reply, fenced by ID/logical time.
 * Reentry, unload and modified request identity invalidate publication. */
typedef int (*GscSessionNativeQueryCallback)(void *, uint64_t, int, GscNativeQuery *, char *, size_t);
typedef struct { void *user; GscSessionNativeQueryCallback query; } GscSessionNativeQueryHost;
GscSession *gsc_session_create_with_native_queries(const GscSessionHost *,
    const GscEntityHost *, const GscEntityAccessHost *, const GscEntityControlHost *,
    const GscEntityCollectionHost *, const GscEntityPredicateHost *,
    const GscActorSpawnHost *, const GscSessionMusicHost *,
    const GscSessionResourceHost *, const GscSessionObjectiveHost *, const GscSessionConsoleHost *,
    const GscSessionHudHost *, const GscSessionMissionHost *, const GscSessionBlastHost *, const GscSessionQuakeHost *, const GscSessionFxHost *, const GscSessionCvarHost *, const GscSessionShellshockHost *, const GscSessionDebugTextHost *, const GscSessionDebugLineHost *, const GscSessionAmbientHost *, const GscSessionPrecacheItemHost *, const GscSessionSaveGameHost *, const GscSessionTagFxHost *, const GscSessionNativeQueryHost *, char *, size_t);

/* R129 startup is pure and synchronous; no callback is invoked by the public
 * lease inspection below. Session ID/logical clock and runtime first-wait latch
 * are checked before and after host callbacks. */
typedef int (*GscSessionPrecacheStringStartupCallback)(void *,uint64_t,int);
typedef int (*GscSessionPrecacheStringCallback)(void *,uint64_t,int,const GscPrecacheStringEvent *,char *,size_t);
typedef struct { void *user; GscSessionPrecacheStringStartupCallback startup;
    GscSessionPrecacheStringCallback publish; } GscSessionPrecacheStringHost;
int gsc_session_precache_string_startup(const GscSession *,uint64_t,int);
GscSession *gsc_session_create_with_precache_string(const GscSessionHost *,
    const GscEntityHost *, const GscEntityAccessHost *, const GscEntityControlHost *,
    const GscEntityCollectionHost *, const GscEntityPredicateHost *,
    const GscActorSpawnHost *, const GscSessionMusicHost *,
    const GscSessionResourceHost *, const GscSessionObjectiveHost *, const GscSessionConsoleHost *,
    const GscSessionHudHost *, const GscSessionMissionHost *, const GscSessionBlastHost *, const GscSessionQuakeHost *, const GscSessionFxHost *, const GscSessionCvarHost *, const GscSessionShellshockHost *, const GscSessionDebugTextHost *, const GscSessionDebugLineHost *, const GscSessionAmbientHost *, const GscSessionPrecacheItemHost *, const GscSessionSaveGameHost *, const GscSessionTagFxHost *, const GscSessionNativeQueryHost *, const GscSessionPrecacheStringHost *, char *, size_t);

/* R131 opt-in complete typed spawner collection. The callback belongs to
 * the current running session and logical clock; old host layouts are frozen.
 * It copies the whole ascending identity set, including empty success, and
 * rejects insufficient capacity without publishing a prefix. */
typedef int (*GscSessionSpawnerArrayCallback)(void *,uint64_t,int,GscEntityRef *,
    size_t,size_t *,char *,size_t);
typedef struct { void *user; GscSessionSpawnerArrayCallback collect_all; } GscSessionSpawnerArrayHost;
GscSession *gsc_session_create_with_spawner_arrays(const GscSessionHost *,
    const GscEntityHost *, const GscEntityAccessHost *, const GscEntityControlHost *,
    const GscEntityCollectionHost *, const GscEntityPredicateHost *,
    const GscActorSpawnHost *, const GscSessionMusicHost *,
    const GscSessionResourceHost *, const GscSessionObjectiveHost *, const GscSessionConsoleHost *,
    const GscSessionHudHost *, const GscSessionMissionHost *, const GscSessionBlastHost *, const GscSessionQuakeHost *, const GscSessionFxHost *, const GscSessionCvarHost *, const GscSessionShellshockHost *, const GscSessionDebugTextHost *, const GscSessionDebugLineHost *, const GscSessionAmbientHost *, const GscSessionPrecacheItemHost *, const GscSessionSaveGameHost *, const GscSessionTagFxHost *, const GscSessionNativeQueryHost *, const GscSessionPrecacheStringHost *, const GscSessionSpawnerArrayHost *, char *, size_t);

/* R133 selected exact-integer draw callback; callbacks receive a private
 * request copy and must preserve operation_id/bound while replacing draw15.
 * A draw already consumed in GAME remains consumed on late session refusal. */
typedef int (*GscSessionRandomIntDrawCallback)(void *,uint64_t,int,GscRandomIntRequest *,char *,size_t);
typedef struct { void *user; GscSessionRandomIntDrawCallback draw; } GscSessionRandomIntHost;
GscSession *gsc_session_create_with_randomint(const GscSessionHost *,
    const GscEntityHost *, const GscEntityAccessHost *, const GscEntityControlHost *,
    const GscEntityCollectionHost *, const GscEntityPredicateHost *,
    const GscActorSpawnHost *, const GscSessionMusicHost *,
    const GscSessionResourceHost *, const GscSessionObjectiveHost *, const GscSessionConsoleHost *,
    const GscSessionHudHost *, const GscSessionMissionHost *, const GscSessionBlastHost *, const GscSessionQuakeHost *, const GscSessionFxHost *, const GscSessionCvarHost *, const GscSessionShellshockHost *, const GscSessionDebugTextHost *, const GscSessionDebugLineHost *, const GscSessionAmbientHost *, const GscSessionPrecacheItemHost *, const GscSessionSaveGameHost *, const GscSessionTagFxHost *, const GscSessionNativeQueryHost *, const GscSessionPrecacheStringHost *, const GscSessionSpawnerArrayHost *, const GscSessionRandomIntHost *, char *, size_t);

/* R134 private exact float draw request copy; no reseed or rollback. */
typedef int (*GscSessionRandomFloatDrawCallback)(void *,uint64_t,int,GscRandomFloatRequest *,char *,size_t);
typedef struct { void *user; GscSessionRandomFloatDrawCallback draw; } GscSessionRandomFloatHost;
GscSession *gsc_session_create_with_randomfloat(const GscSessionHost *,
    const GscEntityHost *, const GscEntityAccessHost *, const GscEntityControlHost *,
    const GscEntityCollectionHost *, const GscEntityPredicateHost *,
    const GscActorSpawnHost *, const GscSessionMusicHost *,
    const GscSessionResourceHost *, const GscSessionObjectiveHost *, const GscSessionConsoleHost *,
    const GscSessionHudHost *, const GscSessionMissionHost *, const GscSessionBlastHost *, const GscSessionQuakeHost *, const GscSessionFxHost *, const GscSessionCvarHost *, const GscSessionShellshockHost *, const GscSessionDebugTextHost *, const GscSessionDebugLineHost *, const GscSessionAmbientHost *, const GscSessionPrecacheItemHost *, const GscSessionSaveGameHost *, const GscSessionTagFxHost *, const GscSessionNativeQueryHost *, const GscSessionPrecacheStringHost *, const GscSessionSpawnerArrayHost *, const GscSessionRandomIntHost *, const GscSessionRandomFloatHost *, char *, size_t);

/* R135 private request copies; late refusals keep preceding CG cache effects. */
typedef int (*GscSessionKeyBindingQueryCallback)(void *,uint64_t,int,GscKeyBindingRequest *,char *,size_t);
typedef struct { void *user; GscSessionKeyBindingQueryCallback query; } GscSessionKeyBindingHost;
GscSession *gsc_session_create_with_keybinding(const GscSessionHost *,
    const GscEntityHost *, const GscEntityAccessHost *, const GscEntityControlHost *,
    const GscEntityCollectionHost *, const GscEntityPredicateHost *,
    const GscActorSpawnHost *, const GscSessionMusicHost *,
    const GscSessionResourceHost *, const GscSessionObjectiveHost *, const GscSessionConsoleHost *,
    const GscSessionHudHost *, const GscSessionMissionHost *, const GscSessionBlastHost *, const GscSessionQuakeHost *, const GscSessionFxHost *, const GscSessionCvarHost *, const GscSessionShellshockHost *, const GscSessionDebugTextHost *, const GscSessionDebugLineHost *, const GscSessionAmbientHost *, const GscSessionPrecacheItemHost *, const GscSessionSaveGameHost *, const GscSessionTagFxHost *, const GscSessionNativeQueryHost *, const GscSessionPrecacheStringHost *, const GscSessionSpawnerArrayHost *, const GscSessionRandomIntHost *, const GscSessionRandomFloatHost *, const GscSessionKeyBindingHost *, char *, size_t);

/* R136 opt-in proved local/dense-array integer bounds; same session hosts. */
GscSession *gsc_session_create_with_computed_randomint(const GscSessionHost *,
    const GscEntityHost *, const GscEntityAccessHost *, const GscEntityControlHost *,
    const GscEntityCollectionHost *, const GscEntityPredicateHost *,
    const GscActorSpawnHost *, const GscSessionMusicHost *,
    const GscSessionResourceHost *, const GscSessionObjectiveHost *, const GscSessionConsoleHost *,
    const GscSessionHudHost *, const GscSessionMissionHost *, const GscSessionBlastHost *, const GscSessionQuakeHost *, const GscSessionFxHost *, const GscSessionCvarHost *, const GscSessionShellshockHost *, const GscSessionDebugTextHost *, const GscSessionDebugLineHost *, const GscSessionAmbientHost *, const GscSessionPrecacheItemHost *, const GscSessionSaveGameHost *, const GscSessionTagFxHost *, const GscSessionNativeQueryHost *, const GscSessionPrecacheStringHost *, const GscSessionSpawnerArrayHost *, const GscSessionRandomIntHost *, const GscSessionRandomFloatHost *, const GscSessionKeyBindingHost *, char *, size_t);

/* R148 native structure allocation and cached difficulty, same session hosts. */
GscSession *gsc_session_create_with_structs(const GscSessionHost *,
    const GscEntityHost *, const GscEntityAccessHost *, const GscEntityControlHost *,
    const GscEntityCollectionHost *, const GscEntityPredicateHost *,
    const GscActorSpawnHost *, const GscSessionMusicHost *,
    const GscSessionResourceHost *, const GscSessionObjectiveHost *, const GscSessionConsoleHost *,
    const GscSessionHudHost *, const GscSessionMissionHost *, const GscSessionBlastHost *, const GscSessionQuakeHost *, const GscSessionFxHost *, const GscSessionCvarHost *, const GscSessionShellshockHost *, const GscSessionDebugTextHost *, const GscSessionDebugLineHost *, const GscSessionAmbientHost *, const GscSessionPrecacheItemHost *, const GscSessionSaveGameHost *, const GscSessionTagFxHost *, const GscSessionNativeQueryHost *, const GscSessionPrecacheStringHost *, const GscSessionSpawnerArrayHost *, const GscSessionRandomIntHost *, const GscSessionRandomFloatHost *, const GscSessionKeyBindingHost *, char *, size_t);

/* R149 same-session copied entity and team collection replies. */
typedef int (*GscSessionEntityNumberCallback)(void *,uint64_t,int,int32_t,GscEntityRef *,int *,char *,size_t);
typedef int (*GscSessionSpawnerTeamArrayCallback)(void *,uint64_t,int,unsigned int,GscEntityRef *,size_t,size_t *,char *,size_t);
typedef struct { void *user; GscSessionEntityNumberCallback by_number; GscSessionSpawnerTeamArrayCallback collect_team; } GscSessionEntityQueryHost;
GscSession *gsc_session_create_with_entity_queries(const GscSessionHost *,
    const GscEntityHost *, const GscEntityAccessHost *, const GscEntityControlHost *,
    const GscEntityCollectionHost *, const GscEntityPredicateHost *,
    const GscActorSpawnHost *, const GscSessionMusicHost *,
    const GscSessionResourceHost *, const GscSessionObjectiveHost *, const GscSessionConsoleHost *,
    const GscSessionHudHost *, const GscSessionMissionHost *, const GscSessionBlastHost *, const GscSessionQuakeHost *, const GscSessionFxHost *, const GscSessionCvarHost *, const GscSessionShellshockHost *, const GscSessionDebugTextHost *, const GscSessionDebugLineHost *, const GscSessionAmbientHost *, const GscSessionPrecacheItemHost *, const GscSessionSaveGameHost *, const GscSessionTagFxHost *, const GscSessionNativeQueryHost *, const GscSessionPrecacheStringHost *, const GscSessionSpawnerArrayHost *, const GscSessionRandomIntHost *, const GscSessionRandomFloatHost *, const GscSessionKeyBindingHost *, const GscSessionEntityQueryHost *, char *, size_t);

/* R150 runtime-checked integer bounds, same session hosts. */
GscSession *gsc_session_create_with_dynamic_randomint(const GscSessionHost *,
    const GscEntityHost *, const GscEntityAccessHost *, const GscEntityControlHost *,
    const GscEntityCollectionHost *, const GscEntityPredicateHost *,
    const GscActorSpawnHost *, const GscSessionMusicHost *,
    const GscSessionResourceHost *, const GscSessionObjectiveHost *, const GscSessionConsoleHost *,
    const GscSessionHudHost *, const GscSessionMissionHost *, const GscSessionBlastHost *, const GscSessionQuakeHost *, const GscSessionFxHost *, const GscSessionCvarHost *, const GscSessionShellshockHost *, const GscSessionDebugTextHost *, const GscSessionDebugLineHost *, const GscSessionAmbientHost *, const GscSessionPrecacheItemHost *, const GscSessionSaveGameHost *, const GscSessionTagFxHost *, const GscSessionNativeQueryHost *, const GscSessionPrecacheStringHost *, const GscSessionSpawnerArrayHost *, const GscSessionRandomIntHost *, const GscSessionRandomFloatHost *, const GscSessionKeyBindingHost *, const GscSessionEntityQueryHost *, char *, size_t);

/* R151 bounded original FLOAT GetInt conversion, same session hosts. */
GscSession *gsc_session_create_with_numeric_randomint(const GscSessionHost *,
    const GscEntityHost *, const GscEntityAccessHost *, const GscEntityControlHost *,
    const GscEntityCollectionHost *, const GscEntityPredicateHost *,
    const GscActorSpawnHost *, const GscSessionMusicHost *,
    const GscSessionResourceHost *, const GscSessionObjectiveHost *, const GscSessionConsoleHost *,
    const GscSessionHudHost *, const GscSessionMissionHost *, const GscSessionBlastHost *, const GscSessionQuakeHost *, const GscSessionFxHost *, const GscSessionCvarHost *, const GscSessionShellshockHost *, const GscSessionDebugTextHost *, const GscSessionDebugLineHost *, const GscSessionAmbientHost *, const GscSessionPrecacheItemHost *, const GscSessionSaveGameHost *, const GscSessionTagFxHost *, const GscSessionNativeQueryHost *, const GscSessionPrecacheStringHost *, const GscSessionSpawnerArrayHost *, const GscSessionRandomIntHost *, const GscSessionRandomFloatHost *, const GscSessionKeyBindingHost *, const GscSessionEntityQueryHost *, char *, size_t);

/* R152 conditional exact-domain dynamic randomfloat, same session hosts. */
GscSession *gsc_session_create_with_dynamic_randomfloat(const GscSessionHost *,
    const GscEntityHost *, const GscEntityAccessHost *, const GscEntityControlHost *,
    const GscEntityCollectionHost *, const GscEntityPredicateHost *,
    const GscActorSpawnHost *, const GscSessionMusicHost *,
    const GscSessionResourceHost *, const GscSessionObjectiveHost *, const GscSessionConsoleHost *,
    const GscSessionHudHost *, const GscSessionMissionHost *, const GscSessionBlastHost *, const GscSessionQuakeHost *, const GscSessionFxHost *, const GscSessionCvarHost *, const GscSessionShellshockHost *, const GscSessionDebugTextHost *, const GscSessionDebugLineHost *, const GscSessionAmbientHost *, const GscSessionPrecacheItemHost *, const GscSessionSaveGameHost *, const GscSessionTagFxHost *, const GscSessionNativeQueryHost *, const GscSessionPrecacheStringHost *, const GscSessionSpawnerArrayHost *, const GscSessionRandomIntHost *, const GscSessionRandomFloatHost *, const GscSessionKeyBindingHost *, const GscSessionEntityQueryHost *, char *, size_t);

/* Optional R202 getaiarray, selected before loading the first module. */
int gsc_session_select_r202_ai_arrays(GscSession *,const GscAiArrayHost *,char *,size_t);

/* R203 explicitly selected constructor-prefix prelude. */
enum {GSC_ACTOR_BOOTSTRAP_MODULE_CAPACITY=32};
/* Copied compile capabilities and map declaration identity only. No entity
 * refs, runtime/provider bindings or execution certificate are present. */
typedef struct {
 uint64_t epoch,receipt;void *user;
 int (*current)(void *,uint64_t,uint64_t);
 uint32_t module_count;char module_roots[GSC_ACTOR_BOOTSTRAP_MODULE_CAPACITY][128];
} GscSessionActorProgramPlan;
int gsc_session_select_map_actor_program(GscSession *,const GscSessionActorProgramPlan *,char *,size_t);
int gsc_session_select_actor_program(GscSession *,const GscSessionActorProgramPlan *,char *,size_t);
typedef struct GscSessionActorBootstrapHost {
    void *user;
    /* Additional graph roots, copied/canonicalized before load. Complete
     * module graph compilation and strict linking precede every stage. */
    uint32_t module_count;
    char module_roots[GSC_ACTOR_BOOTSTRAP_MODULE_CAPACITY][128];
    /* Genuine constructor encounter order, independent of entity-slot and
     * separate entity-hook ordering. Each full identity is prepared once. */
    uint32_t placed_count;
    GscEntityRef placed[GSC_AI_ARRAY_CAPACITY];
    GscMapExecutionCertificate map_certificate;
    GscEntityAccessHost access;
    GscEntityFieldHost fields;
    GscEntityMethodHost methods;
    GscSpawnerTeamMethodHost spawner_team;
    GscAttachmentQueryHost attachments;
    int (*run)(void *,const GscActorBootstrapServices *,char *,size_t);
    /* Optional explicit construction phase after load and before start.
     * Only copied entry identities survive it; no callback-local certificate
     * or service facade may be retained. run then owns hooks/late entry only. */
    int (*construct)(void *,const GscActorBootstrapServices *,char *,size_t);
    int (*node_target)(void *,GscSession *,uint64_t,uint64_t,const GscEntityRef *,const char *,GscActorBootstrapNodeTarget *);
    /* Matching finish is callback-free; the GAME graph stays pinned until it
     * checks cancellation after ALL Session/provider callbacks. */
    int (*node_target_end)(void *,GscSession *,uint64_t,uint64_t,const GscEntityRef *,int);
    int placed_init_initial; /* Explicit selection before bootstrap admission. */
    int placed_cohort;
    int (*placed_init_return)(void *,const GscPlacedInitReturnRequest *,char *,size_t,int(*)(void*),void*);
} GscSessionActorBootstrapHost;

/* New selected lane only; existing Session constructors/start stay intact.
 * Select while idle before loading, alongside R198 execution. */
int gsc_session_select_r203_actor_bootstrap(GscSession *,
    const GscSessionActorBootstrapHost *,char *,size_t);

/* One-shot late admission of real placed identities/providers into the exact
 * COMPILED plan. Failure is terminal; no relink or runtime creation occurs. */
int gsc_session_admit_actor_bootstrap(GscSession *,uint64_t,const GscSessionActorBootstrapHost *,char *,size_t);

/* One-shot constructor phase; Session remains PREPARED with no root/hook task.
 * A selected construct callback requires this call before start. */
int gsc_session_construct_actors(GscSession *,uint64_t,char *,size_t);

/* R204 additive delete transaction; select after R198 entity operations,
 * while idle before loading. Public notify reentry remains forbidden. */
int gsc_session_select_r204_delete_transactions(GscSession *,
    const GscDeleteTransactionHost *,char *,size_t);

/* R209 fixed generic_human/training_guy_practicejump query. Select once on an
 * idle EMPTY enabled mode40 Session before load. The complete ATR is copied and
 * parsed; no source pointer survives selection. The immutable selected catalog
 * persists across unload/failure/reload until destroy. The real map/resource
 * receipt is never retargeted: retirement requires a fresh Session/selection.
 * Read-only resource certificate calls during this selected prepare/load path
 * are explicit; no query/effect callback runs before script execution. Older
 * constructors and unselected Sessions retain their existing callback policy.
 * Selection/ATR bytes must remain readable during the call; host.user must
 * outlive the Session. Catalog ordinals are declared symbols, never native IDs.
 */
typedef struct {
    const void *generic_human_atr;
    size_t generic_human_atr_size;
    GscMapExecutionCertificate map_certificate;
    GscStartOriginHost host;
} GscSessionStartOriginSelection;
int gsc_session_select_r209_start_origin(GscSession *,
    const GscSessionStartOriginSelection *,char *,size_t);

/* R211 metadata-only namespaces, in canonical order generic_human,
 * animation_rig_gear_trainingjump, animation_rig_wire_trainingjump.
 * Selection requires idle EMPTY mode40 and is exclusive with R209. All three
 * ATR payloads are parsed into owned immutable catalogs until destroy;
 * unload/reload binds fresh Runtime identities to the same genuine map lease.
 * This supplies no UseAnimTree, animation playback or query-host widening. */
typedef struct {const void *atr_bytes;size_t atr_size;} GscSessionAnimationNamespaceCatalogInput;
typedef struct {
    GscSessionAnimationNamespaceCatalogInput catalogs[3];
    GscMapExecutionCertificate map_certificate;
} GscSessionAnimationNamespaceSelection;
typedef struct {GscMapAnimationNames names;GscSessionAnimationNamespaceCatalogInput catalogs[GSC_MAP_ANIMATION_CATALOGS];GscMapExecutionCertificate map_certificate;} GscSessionMapAnimationSelection;
int gsc_session_select_map_animation_namespace(GscSession*,const GscSessionMapAnimationSelection*,char*,size_t);
/* New immutable generic_human/mg42 ATR namespace and actual command provider.
 * Select idle EMPTY mode40, mutually exclusive with R209/R211 selections. */
typedef struct {GscSessionAnimationNamespaceCatalogInput catalogs[2];
    GscMapExecutionCertificate map_certificate;GscTurretAnimationHost host;
} GscSessionTurretAnimationSelection;
/* DATA-only early ATR/settings plan. No live entity or execution certificate.
 * Bytes are copied until destroy. Every new program needs fresh late admission. */
typedef struct {uint64_t epoch,receipt;void *user;int (*current)(void*,uint64_t,uint64_t);
 GscSessionAnimationNamespaceCatalogInput catalogs[2];int developer_enabled;
} GscSessionTurretProgramPlan;
typedef struct {uint64_t epoch;GscSessionTurretAnimationSelection animations;
 GscDeveloperScriptSelection developer;
} GscSessionTurretProgramAdmission;
int gsc_session_select_turret_program(GscSession*,const GscSessionTurretProgramPlan*,char*,size_t);
int gsc_session_admit_turret_program(GscSession*,uint64_t,const GscSessionTurretProgramAdmission*,char*,size_t);
int gsc_session_select_r232_turret_animations(GscSession *,const GscSessionTurretAnimationSelection *,char *,size_t);
/* Exact linked stand::main entry at a quiescent RUNNING selected Session.
 * Waiting/sleeping/cancellation remain real Runtime task results. */
int gsc_session_execute_r232_turret_entry(GscSession *,uint64_t,const GscEntityScriptEntry *,GscEntityScriptRun *,char *,size_t);
int gsc_session_training_rigs(GscSession*,const GscSessionAnimationNamespaceSelection*,int,char*,size_t);
int gsc_session_select_r211_animation_namespace(GscSession *,
    const GscSessionAnimationNamespaceSelection *,char *,size_t);


/* R212 optional typed Guy/gear query after R211 namespace configuration.
 * Select once while EMPTY with no Program/Runtime; first selection after a
 * metadata-only unload is allowed. The copied provider and captured catalog
 * ordinals remain immutable until destroy, including reload. host.user must
 * outlive the Session. Namespace selection alone grants no query capability.
 * This supplies no UseAnimTree, playback, wire query or native tree identity. */
int gsc_session_select_map_start_pose(GscSession*,const GscStartOriginHost*,char*,size_t);
int gsc_session_select_map_animation_length(GscSession*,const GscAnimationLengthHost*,char*,size_t);
int gsc_session_map_animation_metadata_request(const GscSession*,uint64_t,unsigned,GscAnimationLengthRequest*);
int gsc_session_select_r212_namespace_start_origin(GscSession *,
    const GscStartOriginHost *,char *,size_t);

/* Selected real item fields and spawn admission; configure while EMPTY.
 * The provider is copied. Actor-bootstrap fields remain a separate profile. */
int gsc_session_select_map_items(GscSession *,const GscEntityFieldHost *,char *,size_t);
int gsc_session_select_r216_items(GscSession *,const GscEntityFieldHost *,char *,size_t);
int gsc_session_select_r222_carbine_items(GscSession *,char *,size_t);
int gsc_session_select_r228_training_opening(GscSession *,char *,size_t);
int gsc_session_select_r225_training_followup(GscSession *,char *,size_t);
int gsc_session_initial_stop_main(GscSession*,uint64_t,const GscEntityExecutionCertificate*,char*,size_t);
int gsc_session_locomotion_main(GscSession*,uint64_t,const GscEntityExecutionCertificate*,int,char*,size_t);
int gsc_session_course_anim_entry(GscSession *,uint64_t,const GscEntityExecutionCertificate *,int,char *,size_t);
int gsc_session_training_appearance(GscSession*,uint64_t,const GscEntityRef*,int,GscTrainingAppearance*);
int gsc_session_select_map_actor_goal(GscSession*,char*,size_t);
int gsc_session_select_entity_goal(GscSession *);
int gsc_session_select_r222_actor_goal(GscSession *,const GscActorGoalHost *,char *,size_t);
int gsc_session_select_r232_turrets(GscSession *,const GscTurretHost *,char *,size_t);
/* Independent real immediate commands after the owned two-ATR selection. */
int gsc_session_select_map_flagged_all(GscSession*,char*,size_t);
int gsc_session_select_map_actor_animations(GscSession*,const GscActorAnimationHost*,char*,size_t);
int gsc_session_select_r232_actor_animations(GscSession *,const GscActorAnimationHost *,char *,size_t);
int gsc_session_select_r232_randomfloat_range(GscSession *,char *,size_t);
int gsc_session_select_r232_source_float_arithmetic(GscSession *,char *,size_t);
/* Idle two-ATR profile only; retained for reloads until destroy. */
int gsc_session_select_r232_quoted_strings(GscSession *,char *,size_t);
int gsc_session_select_map_quoted_strings(GscSession *,char *,size_t);
int gsc_session_select_map_actor_look(GscSession*,const GscActorLookHost*,char*,size_t);
int gsc_session_select_r232_actor_look(GscSession *,const GscActorLookHost *,char *,size_t);
int gsc_session_select_r233_namespace_animation_length(GscSession *,const GscAnimationLengthHost *,char *,size_t);
int gsc_session_select_r233_native_animation_roots(GscSession*,char*,size_t);
int gsc_session_select_r233_actor_drop(GscSession*,const GscActorDropHost*,char*,size_t);
int gsc_session_select_r233_actor_presence(GscSession *,const GscActorPresenceHost *,char *,size_t);
int gsc_session_select_map_moveto(GscSession *session,char *error,size_t size);
int gsc_session_select_map_source_float(GscSession*,char*,size_t);
int gsc_session_select_root_map(GscSession*,const GscMapExecutionCertificate*,char*,size_t);
int gsc_session_select_vehicle_methods(GscSession*,const GscVehicleMethodHost*,char*,size_t);
int gsc_session_select_vehicle_spawn(GscSession*,const GscVehicleSpawnHost*,char*,size_t);
int gsc_session_select_turret_precache(GscSession*,const GscTurretPrecacheHost*,char*,size_t);
int gsc_session_select_turret_spawn(GscSession*,const GscTurretSpawnHost*,char*,size_t);
int gsc_session_select_model_part(GscSession*,const GscModelPartHost*,char*,size_t);
int gsc_session_select_weapon_classname(GscSession*,const GscWeaponClassnameHost*,char*,size_t);
int gsc_session_select_r232_weapon_model(GscSession *,const GscWeaponModelHost *,char *,size_t);
int gsc_session_select_r232_function_admission(GscSession *,char *,size_t);
int gsc_session_select_map_developer_script(GscSession *,const GscDeveloperScriptSelection *,char *,size_t);
int gsc_session_select_r232_developer_script(GscSession *,const GscDeveloperScriptSelection *,char *,size_t);
int gsc_session_select_r216_yaw(GscSession *,const GscYawMotionHost *,char *,size_t);

int gsc_session_select_r233_flagged_all_restart(GscSession*,char*,size_t);

/* R237 opt-in native GAME frame bridge. Select once while idle EMPTY, before
 * prepare/load. This separates the physical host callback time from the VM
 * bucket cursor. Legacy advance is refused for selected Sessions. Begin emits
 * no script callbacks and commits its generation output only on success.
 * Current-bucket drains may repeat while the same generation is open;
 * exclusive advance drains [cursor, target) and closes that generation.
 * Explicit phases require the exact current Session ID and frame generation.
 * Selection persists over unload; an old Session ID never authorizes reload.
 * The read-only view is available during callbacks; it never pumps a VM.
 */
int gsc_session_select_r237_frame_buckets(GscSession *,char *,size_t);
int gsc_session_r237_frame_buckets_selected(const GscSession *);
int gsc_session_r237_frame_view(const GscSession *,uint64_t,GscR237FrameView *);
int gsc_session_r237_frame_begin(GscSession *,uint64_t,int64_t,uint64_t *,char *,size_t);
int gsc_session_r237_run_current_bucket(GscSession *,uint64_t,uint64_t,char *,size_t);
int gsc_session_r237_advance_exclusive(GscSession *,uint64_t,uint64_t,char *,size_t);

int gsc_session_select_r243_controller_with_cleanup(GscSession*,const GscActorScriptedControllerHost*,GscControllerSelectWithCleanup,char*,size_t);
int gsc_session_select_r243_scripted_controller(GscSession*,const GscActorScriptedControllerHost*,char*,size_t);
int gsc_session_r243_controller_prefix(GscSession*,uint64_t,const GscEntityRef*,uint64_t,uint64_t,GscActorControllerPrefixRun*,char*,size_t);

typedef struct {GscNodeScriptNames scripts;GscMapExecutionCertificate map_certificate;} GscSessionNodeScriptSelection;
int gsc_session_select_node_scripts(GscSession*,const GscSessionNodeScriptSelection*,char*,size_t);
/* Names owned before registry allocation. Never an execution certificate. */
typedef struct {uint64_t epoch,receipt;void *user;int (*current)(void*,uint64_t,uint64_t);GscNodeScriptNames scripts;} GscSessionNodeProgramPlan;
typedef struct {uint64_t epoch;GscSessionNodeScriptSelection selection;} GscSessionNodeScriptAdmission;
int gsc_session_select_node_program(GscSession*,const GscSessionNodeProgramPlan*,char*,size_t);
int gsc_session_admit_node_scripts(GscSession*,uint64_t,const GscSessionNodeScriptAdmission*,char*,size_t);
GscNodeScriptRef gsc_session_node_script_lookup(GscSession*,uint64_t,const char*);
int gsc_session_node_script_current(GscSession*,uint64_t,GscNodeScriptRef);
int gsc_session_node_script_map_matches(GscSession*,uint64_t,const GscMapExecutionCertificate*);
/* One exact GAME graph dependency per Session. Begin pins this Session object
 * until End, which must run even after a failed Begin. Drop/unload/replacement
 * retire the lease before node-script cache destruction. Destroy during a pin
 * cancels the lease and refuses to free the object; retry after End. Retire
 * must be a no-fail exact-owner release/cancel, without Session reentry.
 * No runtime/VM graph admission or SAVE semantics are granted. */
int gsc_session_node_graph_begin(GscSession*,uint64_t,uint64_t,void(*)(uint64_t));
int gsc_session_select_disconnectpaths(GscSession *,GscEntityWorldAction);
int gsc_session_select_connectpaths(GscSession *,GscEntityWorldAction);
int gsc_session_running_path_graph(const GscSession *,uint64_t,uint64_t);
int gsc_session_node_graph_current(const GscSession*,uint64_t,uint64_t);
int gsc_session_node_graph_check_begin(GscSession*,uint64_t,uint64_t);
int gsc_session_node_graph_end(GscSession*,uint64_t,uint64_t,int);
/* Separate synchronous placed-postinit graph query, never generic busy bypass.
 * Only the current Session service establishes its exact row/program pin. */
int gsc_session_actor_graph_current(GscSession *,uint64_t,uint64_t,const GscEntityRef *);
int gsc_session_actor_graph_map_matches(GscSession *,uint64_t,uint64_t,const GscEntityRef *,const GscMapExecutionCertificate *);
int gsc_session_actor_graph_script_current(GscSession *,uint64_t,uint64_t,const GscEntityRef *,GscNodeScriptRef);

/* Early immutable map/program DATA; not a runtime execution certificate. */
typedef struct {
 uint64_t epoch,receipt;void*user;int(*current)(void*,uint64_t,uint64_t);
 char mapname[64];
} GscSessionScriptRootsPlan;
int gsc_session_select_script_roots(GscSession*,const GscSessionScriptRootsPlan*,char*,size_t);

#endif


int gsc_session_select_r243_flagged_all(GscSession*,const GscActorAnimationHost*,char*,size_t);
int gsc_session_select_r402_source_look(GscSession*,const GscActorLookHost*,char*,size_t);
int gsc_session_select_r400_source_animations(GscSession*,const GscActorAnimationHost*,char*,size_t);

int gsc_session_select_r243_critical(GscSession*,const GscActorCriticalHost*,char*,size_t);


int gsc_session_select_bullet_trace(GscSession*,const GscBulletTraceHost*,char*,size_t);

int gsc_session_select_map_weapon_model(GscSession*,const GscWeaponModelHost*,char*,size_t);

int gsc_session_select_map_actor_presence(GscSession*,const GscActorPresenceHost*,char*,size_t);
