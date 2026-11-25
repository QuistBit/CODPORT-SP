#ifndef GSC_FIXTURE_H
#define GSC_FIXTURE_H

#include <stddef.h>
#include <stdint.h>
#include "gsc_node_script_types.h"
#include "../bg_cod1_keybinding.h"
#include "gsc_string_cast.h"

/* Standalone research fixture, deliberately independent of RTCW. */
typedef struct GscProgram GscProgram;
typedef struct GscRuntime GscRuntime;

/* R84 fixture entity transport. These are host-owned identities, never game
 * pointers or original VM tags. An epoch and generation must be nonzero.
 * The host checks slot range, epoch and generation immediately before use. */
typedef struct { uint64_t epoch; uint32_t slot, generation; } GscEntityRef;
/* R194 explicitly selected one-argument sound. argc is passed to the host so
 * native resource/index/flags-before-arity ordering can be preserved. */
typedef struct {
 void *user;
 int (*invoke)(void *,const GscEntityRef *,const char *,int,char *,size_t);
} GscScriptSoundHost;
/* R243 explicitly selected persistent loop method boundary. This is not a
 * one-shot event or a sound completion acknowledgement. Providers must keep
 * their own allocation/Session/resource receipts and call current around any
 * callback before committing. current and its context are call-scoped. */
typedef struct {GscEntityRef owner;char alias[128];int stop;uint64_t runtime,operation;} GscLoopSoundRequest;
typedef struct {
 void *user;
 int (*invoke)(void *,const GscLoopSoundRequest *,char *,size_t,int (*current)(void *),void *);
 /* invoke prepares only. finish(commit=1) is called after VM post-validation;
  * finish(0) cancels even a failed prepare/commit. It must use exact operation
  * receipts, not a retained borrowed current callback. No playback in prepare. */
 int (*finish)(void *,const GscLoopSoundRequest *,int,char *,size_t,int (*current)(void *),void *);
} GscLoopSoundHost;
int gsc_program_select_local_identifier_case(GscProgram *);
int gsc_program_select_loop_sound(GscProgram *);
int gsc_runtime_select_loop_sound(GscRuntime *,const GscLoopSoundHost *);

typedef struct {
 GscEntityRef owner;char alias[128],notify[128];int argc,mode,notify_valid,mode_valid;
} GscSoundNotifyRequest;
typedef struct {void *user;int (*invoke)(void *,const GscSoundNotifyRequest *,char *,size_t);} GscSoundNotifyHost;
int gsc_runtime_sound_notify_native(GscRuntime *,const GscEntityRef *,const char *,int *);
int gsc_program_select_sound_notify(GscProgram *);
int gsc_runtime_select_sound_notify(GscRuntime *,const GscSoundNotifyHost *);
int gsc_program_select_script_sound(GscProgram *);
int gsc_runtime_select_script_sound(GscRuntime *,const GscScriptSoundHost *);

typedef struct {
    void *user;
    /* Return success separately from found; a miss writes found=0. A hit
     * writes found=1 and a valid ref. Output is copied, no pointer retained. */
    int (*lookup)(void *user, const char *name, const char *field,
                  GscEntityRef *out, int *found, char *error, size_t size);
    int (*setorigin)(void *user, const GscEntityRef *entity,
                     const float origin[3], char *error, size_t size);
    /* 1 means this identity is alive, 0 means stale/missing. No side effects. */
    int (*validate)(void *user, const GscEntityRef *entity);
    /* Optional capability, required by all retained spawn calls, including
     * unreachable ones. A successful creation returns a live identity. */
    int (*spawn)(void *user, const char *classname, const float origin[3],
                 GscEntityRef *out, char *error, size_t size);
    /* R138 optional exact-int32 flags transport. Three-argument script-class
     * calls require this capability even in unexecuted branches. The legacy
     * two-argument callback above retains its ABI and supplies flags=0.
     * Hosts selecting this new capability must initialize the entire struct. */
    int (*spawn_with_flags)(void *user, const char *classname,
                 const float origin[3], int32_t flags, GscEntityRef *out,
                 char *error, size_t size);
} GscEntityHost;

/* R85 optional, separately selected entity access capabilities. R138 extends
 * only GscEntityHost's optional spawn transport. Callbacks validate the identity at the actual
 * read/mutation boundary; no game pointers or generic field dictionary. */
typedef struct {
    void *user;
    int (*setmodel)(void *user, const GscEntityRef *entity,
                    const char *model, char *error, size_t size);
    int (*getorigin)(void *user, const GscEntityRef *entity,
                     float out[3], char *error, size_t size);
    int (*getangles)(void *user, const GscEntityRef *entity,
                     float out[3], char *error, size_t size);
    int (*setangles)(void *user, const GscEntityRef *entity,
                     const float angles[3], char *error, size_t size);
} GscEntityAccessHost;

/* Native-name GetOrigin is distinct from raw .origin property access.
 * Separately selected; the frozen R85 GscEntityAccessHost stays unchanged.
 * Returns exactly1 with a finite copied float[3], or refuses without output.
 * Identity and output pointers are borrowed only for the synchronous call. */
typedef struct {
    void *user;
    int (*read)(void *,const GscEntityRef *,float[3],char *,size_t);
} GscEntityOriginMethodHost;


/* R91 explicitly selected controls, separate from the frozen R84/R85 hosts.
 * getstring returns copied target/targetname metadata; found=0 is undefined,
 * found=1 includes an explicitly empty string. Callbacks validate full identity
 * at the actual boundary. No callback retains an input/output pointer. */
typedef struct {
    void *user;
    int (*getstring)(void *user, const GscEntityRef *entity, const char *name,
                     char *out, size_t capacity, int *found,
                     char *error, size_t size);
    int (*setvisible)(void *user, const GscEntityRef *entity, int visible,
                      char *error, size_t size);
    int (*setorigin)(void *user, const GscEntityRef *entity,
                     const float origin[3], char *error, size_t size);
} GscEntityControlHost;

/* R93 collection capability is separate from every earlier host ABI. Copies
 * all complete identities atomically; overflow fails, never truncates. Empty
 * success is a defined empty array. Callback retains no input/output pointers. */
typedef struct {
    void *user;
    int (*lookup_array)(void *user, const char *name, const char *field,
                        GscEntityRef *out, size_t capacity, size_t *count,
                        char *error, size_t size);
} GscEntityCollectionHost;
/* R98 separately selected predicates; prior host layouts remain unchanged.
 * Success and the exact boolean 0/1 are separate. Full identities are copied,
 * validated at the actual read boundary, and never retained or retargeted.
 * Unsupported actor representations must fail, not invent an actor result. */
typedef struct {
    void *user;
    int (*isalive)(void *user, const GscEntityRef *entity, int *out,
                   char *error, size_t size);
    int (*issentient)(void *user, const GscEntityRef *entity, int *out,
                      char *error, size_t size);
} GscEntityPredicateHost;
/* R191 separately selected isAI presence, not life/sentience/AI readiness.
 * read publishes a copied Actor allocation serial (zero = confirmed absence).
 * current returns exactly 0/1 for the same entity AND observation; no callback
 * may retain borrowed pointers. Both callbacks are synchronous and read-only.
 * Stale observations publish false in this port, not a native VM lifetime claim.
 * This does not change GscEntityPredicateHost or any prior host layout. */
typedef struct {
    void *user;
    int (*read)(void *,const GscEntityRef *,uint64_t *,char *,size_t);
    int (*current)(void *,const GscEntityRef *,uint64_t);
} GscActorPresenceHost;
int gsc_program_select_actor_presence(GscProgram *);
int gsc_runtime_select_actor_presence(GscRuntime *,const GscActorPresenceHost *);

/* R101 optional actor-spawner capability. All earlier host layouts are frozen.
 * Native dospawn is not spawn(classname, origin): the host must implement the
 * genuine actor constructor or fail explicitly. No synthetic actor is valid.
 * Arguments are evaluated normally (the existing maximum remains 16). Only
 * argc==1 carries single_kind/string. The host checks receiver type and its
 * same-frame timestamp BEFORE interpreting the optional string getter; argc
 * 0 or >1 ignores that getter. These tags are adapter tags, not original IDs. */
enum { GSC_ACTOR_SPAWN_UNDEFINED, GSC_ACTOR_SPAWN_NUMBER,
       GSC_ACTOR_SPAWN_STRING, GSC_ACTOR_SPAWN_VECTOR,
       GSC_ACTOR_SPAWN_ENTITY, GSC_ACTOR_SPAWN_LEVEL, GSC_ACTOR_SPAWN_ARRAY,
       GSC_ACTOR_SPAWN_FUNCTION };
enum { GSC_ACTOR_SPAWN_DOSPAWN=0, GSC_ACTOR_SPAWN_STALINGRAD=1 };
typedef struct { GscEntityRef spawner; int wrapper, arg_count, single_kind;
                 char single_string[128]; } GscActorSpawnRequest;
typedef struct { GscEntityRef spawner, actor; uint64_t operation; } GscActorSpawnTicket;
typedef struct {
    void *user;
    /* Success is separate from created (exactly 0/1). created=0 is a genuine
     * native no-spawn result with no reservation token; unsupported backends
     * must return failure. Any nonzero token transfers a cancellation duty,
     * including malformed created flags or callback failure. An
     * actor ticket owns a host reservation and contains a fully initialized,
     * live actor and the unchanged spawner identity. The real backend must cover
     * native constructor/post-init setup, including aitype::main and
     * animscripts/init::main invocation before publication; thread completion
     * semantics are not claimed here. Output is copied before
     * validation; callbacks must not retain or mutate borrowed pointers. */
    int (*prepare)(void *, const GscActorSpawnRequest *,
                   GscActorSpawnTicket *, int *created, char *, size_t);
    /* Exactly once for each created ticket, including invalid output/reentry:
     * published=0 cancels the reservation without count/timestamp commit;
     * published=1 follows private VM spawned(spawner,actor) delivery, then the
     * host decrements positive count and stamps time. Validate token and full
     * original identities; never adopt map/slot replacements. No VM reentry,
     * notification, or trap on ordinary commit. A failure is fatal; no full
     * game-effects rollback or retry of a failed finalization is promised. */
    int (*finish)(void *, const GscActorSpawnTicket *, int published, char *, size_t);
} GscActorSpawnHost;
enum { GSC_ARRAY_CONTAINER_CAPACITY=256, GSC_ARRAY_ENTRY_CAPACITY=4096,
       GSC_ARRAY_NUMERIC_KEY_MAX=255, GSC_ARRAY_KEY_CAPACITY=128 };

typedef struct {
    float near_dist, far_dist, density, red, green, blue;
    int transition_ms;
} GscFogEvent;

typedef int (*GscFogCallback)(void *user, const GscFogEvent *event);

/* The staged API compiles each whole module, then links ALL calls before a
 * runtime can be created. Paths use forward slashes in this API; GSC source
 * may use backslashes, e.g. maps\foo::bar(). */
GscProgram *gsc_program_create(void);
/* R95 opt-in VFS module identity policy: ASCII lowercase, slash separators.
 * Compile/link accept slash or backslash module IDs; retained requested-module
 * syntax spelling is not rewritten. Function names remain exact-case. This
 * does not enable additional VM instructions or #include symbol semantics. */
GscProgram *gsc_program_create_with_modules(void);
/* R96 explicit event payload syntax plus R95 module identity policy. */
GscProgram *gsc_program_create_with_event_payloads(void);
/* R98 explicit predicate policy, using the existing payload/module syntax. */
GscProgram *gsc_program_create_with_predicates(void);
GscProgram *gsc_program_create_with_actor_spawners(void);
/* R102 script function references and computed ordinary calls/statement
 * threads. Values stay private to this runtime/program; no string-to-function
 * conversion, native reference or external host/event serialization. */
GscProgram *gsc_program_create_with_function_refs(void);
/* R105 opt-in exact int32 scalars, numeric casts and int/string switch.
 * R171 also admits ordinary-string + and += in this lane: copied plain-string
 * operands, exact left/right byte order and a 127-byte result limit. Overflow
 * refuses without truncation or a destination write; mixed/coerced/localized
 * addition and native string-pool ownership are outside this selected domain.
 * Superset of function refs; earlier constructors retain their refusal policy.
 * Runtime cast bounds, modulo overflow and native switch-key range are checked. */
GscProgram *gsc_program_create_with_scalar_switch(void);
/* R106 explicit scalar/switch superset with reusable task storage. The 128
 * simultaneous task cap is a bounded port policy, not an original VM limit.
 * Legacy modes retain 32 monotonic admissions. Reuse never changes birth-order
 * scheduling or entity-generation ownership; exhausted uint64 serials refuse. */
enum { GSC_LIFETIME_TASK_CAPACITY=128 };
GscProgram *gsc_program_create_with_lifetime(void);
/* R107 explicit callable-name superset. Function declarations/calls/refs and
 * selected native callable names use ASCII-insensitive identity. Local scripts
 * precede native fallback; declaration spelling remains available to inspectors.
 * Variables, fields, event payloads and strings retain earlier mode semantics.
 * Keywords and event primitive spelling retain their exact grammar; local
 * waittill/notify declarations are refused to avoid primitive shadow ambiguity.
 * This bounded callable policy is not full original identifier/lexer parity. */
GscProgram *gsc_program_create_with_names(void);
/* Separate music superset. Its host is optional when no retained music calls
 * exist; a retained call requires its specific real callback at preflight. */
GscProgram *gsc_program_create_with_music(void);
enum { GSC_MUSIC_PLAY=1, GSC_MUSIC_STOP };
typedef struct { int kind, transition_ms; char alias[128]; } GscMusicEvent;
/* Synchronous borrowed event, copied before dispatch. Callback returns exactly
 * 1 for accepted effect, 0 for failure. No rollback of prior effects, no
 * synthetic completion notify and no callback during create/link/preflight. */
typedef int (*GscMusicCallback)(void *, int logical_ms, const GscMusicEvent *,
                                char *error, size_t error_size);
typedef struct { void *user; GscMusicCallback play, stop; } GscMusicHost;
/* R108 explicitly selected resources superset. Statement and expression calls to the
 * two proved precache natives are admitted; the native result is undefined.
 * References, threads and receiver forms remain unsupported. Strings are copied without changing resource spelling.
 * A shader requires a nonempty name; an empty model succeeds without effect.
 * Resource admission opens only for the synchronous initial entry-task prefix
 * in gsc_start, closes on its first suspension/retirement and before start
 * returns, and never reopens on later advances (including equal-time ones).
 * Deferred child scheduling is retained; child precache after root suspension
 * is refused. This conservative port window is not original child-order parity.
 * Hosts perform the genuine resource acquisition/publication and return 1 only
 * after acceptance. Create, link and runtime preflight never enter callbacks. */
GscProgram *gsc_program_create_with_resources(void);
/* R109 separately selected objectives superset; earlier public APIs and hosts
 * remain frozen. Objective calls require their genuine typed host capability. */
GscProgram *gsc_program_create_with_objectives(void);
/* R110 separately selected console superset. Original print/println emit each
 * display argument separately; println emits an additional newline. This stage
 * admits ordinary strings only when the queried host gate enables output.
 * Other display-getter conversions remain an explicit original-engine boundary.
 * All script argument expressions are evaluated before querying the gate.
 * print queries ARGUMENTS once. println queries ENTRY, then ARGUMENTS;
 * a suppressed inner ARGUMENTS gate still permits its final newline.
 * Suppression skips display conversion and sink callbacks. The bounded VM value
 * capacity is 127 bytes per string; no joining, locale markers or formatting. */
GscProgram *gsc_program_create_with_console(void);
/* R111 separately selected HUD message superset. Original global iprintln and
 * iprintlnbold compose one localized payload, without console gates/newlines.
 * The bounded port admits strings/localized tokens and exact int32 decimal
 * segments. Ordinary bytes are printable ASCII; localized tokens use ASCII
 * letters/digits/underscore (or one printable byte), each VM value<=127bytes.
 * A zero-argument call composes the original single 0x15 byte. Composition
 * permits literal percent, semicolon, backslash and quotes; the genuine host
 * applies its own safe quoted-command transport subset before publishing.
 * Original natives are global-only; receiver/thread/reference forms remain
 * unsupported. PLAYER is reserved and never emitted by this stage.
 * Engine GetString coercion for other types remains unavailable.
 * Hosts receive one borrowed, synchronous typed message; they publish it only
 * after validating current session, audience and transport limits. */
GscProgram *gsc_program_create_with_hud(void);
/* R112 separately selected mission diagnostic superset. Success checks at most
 * two arguments before the host gate, but zero arguments only fail at the
 * original missing getter boundary when the gate admits. Suppressed success
 * still emits SUCCESS_MARK. Argument expressions retain normal VM evaluation.
 * After the gate, explicit arg1 admits exact int32 and emits ARGUMENT before
 * converting ordinary-string arg0; REQUEST then preserves original spelling.
 * VM strings are bounded to 127 bytes, versus original nextmap copy cap1023.
 * The host retains omitted arg1 and refuses unknown persistent serialization
 * after preserving the accepted request prefix. No map transition is invented.
 * Failed ignores its argument values: player-found gate, DEATH_FADE, then
 * FAILURE_MARK. These are synchronous typed diagnostic effects; engine fade,
 * persistent serialization and later frame transitions require separate hosts.
 * Local script shadowing remains first; receivers, threads and refs unsupported.
 * Both wrappers have undefined script result; they do not retire VM tasks. */
GscProgram *gsc_program_create_with_mission(void);
/* changelevel shares native359B0 request semantics, without success mark.
 * Selected domain: one/two arguments, no third-argument fade override. */
enum { GSC_MISSION_SUCCESS=1, GSC_MISSION_FAILED, GSC_MISSION_CHANGELEVEL };
enum { GSC_MISSION_ARGUMENT=1, GSC_MISSION_REQUEST, GSC_MISSION_SUCCESS_MARK,
       GSC_MISSION_DEATH_FADE, GSC_MISSION_FAILURE_MARK };
typedef struct { int kind, phase, has_arg1; int32_t arg1; size_t length;
    char map[1024]; } GscMissionEvent;
typedef int (*GscMissionGateCallback)(void *, int logical_ms, int kind,
    int *enabled, char *error, size_t error_size);
typedef int (*GscMissionCallback)(void *, int logical_ms, const GscMissionEvent *,
    char *error, size_t error_size);
typedef struct { void *user; GscMissionGateCallback query_gate;
    GscMissionCallback success, failed; } GscMissionHost;
/* R113 independently selected blast superset. radiusDamage reads vector0,
 * float1(radius), float2(inner), float3(outer), in that order; missing getters
 * refuse at use, with no invented native argc guard. Extra argument expressions
 * retain ordinary VM evaluation, then their values are ignored. Numeric int32
 * promotion and finite float32 vectors/scalars are the bounded port domain.
 * Local script shadowing is first; native receivers, threads and refs remain
 * unsupported. No script result, event index, player-ignore or LOS policy is
 * invented here: the copied typed host callback owns the actual game effect. */
GscProgram *gsc_program_create_with_blast(void);
enum { GSC_BLAST_RADIUS=1 };
typedef struct { int kind; float origin[3], radius, inner_damage, outer_damage; } GscBlastEvent;
typedef int (*GscBlastCallback)(void *, int logical_ms, const GscBlastEvent *,
    char *error, size_t error_size);
typedef struct { void *user; GscBlastCallback radius; } GscBlastHost;
/* R114 independently selected earthquake superset. Ordered GetFloat0,
 * GetFloat1, GetVector2, GetFloat3 precede the bounded host domain check.
 * Extra arguments are ordinarily evaluated then ignored; result is undefined.
 * Duration admission is finite nonnegative seconds whose float32(seconds*1000)
 * is exactly integral int32 milliseconds. Fractional-ms x87 rounding remains
 * unproven; positive finite radius and finite float32 scale/origin are required.
 * Local script shadowing is first; native receivers/threads/refs unsupported. */
GscProgram *gsc_program_create_with_quake(void);
enum { GSC_QUAKE_EARTHQUAKE=1 };
typedef struct { float scale; int32_t duration_ms; float origin[3], radius; } GscQuakeEvent;
typedef int (*GscQuakeCallback)(void *, int logical_ms, const GscQuakeEvent *,
    char *error, size_t error_size);
typedef struct { void *user; GscQuakeCallback emit; } GscQuakeHost;
/* R115 independently selected FX superset. loadfx evaluates all arguments but
 * obtains only GetString0, then delegates case-insensitive resource identity
 * and initial registration admission to its host. The initial-start flag is
 * the port's one-way resource window; existing identities may load afterwards.
 * playfx has the original 2..3 argument structural guard and ordered GetInt0,
 * GetVector1, optional GetVector2. Raw direction is retained: original axis
 * construction is owned by the genuine game host, never guessed by the VM.
 * Finite vectors, 128-byte copied names and successful indices 1..63 (or zero
 * only for an initial empty name) are the
 * bounded port domain. Earlier APIs, public structs and native imports remain
 * unchanged. Native receiver/thread/function-ref forms remain unsupported. */
GscProgram *gsc_program_create_with_fx(void);
enum { GSC_FX_LOAD=1, GSC_FX_PLAY, GSC_FX_LOOP };
#include "gsc_loop_fx.h"
enum { GSC_FX_NAME_CAPACITY=128, GSC_FX_INDEX_CAPACITY=64 };
typedef struct { char name[GSC_FX_NAME_CAPACITY]; int initial_window; } GscFxLoadEvent;
typedef struct { int32_t index; float origin[3]; int has_direction;
    float direction[3]; } GscFxPlayEvent;
typedef int (*GscFxLoadCallback)(void *, int logical_ms, const GscFxLoadEvent *,
    int32_t *index_out, char *error, size_t error_size);
typedef int (*GscFxPlayCallback)(void *, int logical_ms, const GscFxPlayEvent *,
    char *error, size_t error_size);
typedef struct { void *user; GscFxLoadCallback load; GscFxPlayCallback play; } GscFxHost;
/* R116 independently selected setcvar superset. Original ordered GetString0,
 * GetType1 and either ordinary GetString1 or the shared localized text builder
 * precede publication. Ordinary strings are a selected bounded subset: no
 * unproven scalar GetString coercion, no quote/@ rewriting, extras ignored after
 * normal evaluation. Localized input consumes all arguments from index 1.
 * REGISTER then SET carry identical copied data and raw native flag metadata;
 * this flag is not a target-engine flag mapping. The nonzero operation_id is a
 * nonwrapping per-runtime port token, not a native argument. Undefined is port result policy.
 * Host owns name safety and actual target registration policy. */
GscProgram *gsc_program_create_with_cvars(void);
enum { GSC_CVAR_SETVAR=1 };
enum { GSC_CVAR_REGISTER=1, GSC_CVAR_SET=2 };
enum { GSC_CVAR_NAME_CAPACITY=128, GSC_CVAR_VALUE_CAPACITY=1024,
       GSC_CVAR_NATIVE_FLAGS=0x1000 };
typedef struct { int phase; uint32_t native_flags;
    char name[GSC_CVAR_NAME_CAPACITY]; size_t length;
    char value[GSC_CVAR_VALUE_CAPACITY];
    uint64_t operation_id; } GscCvarEvent;
typedef int (*GscCvarCallback)(void *, int logical_ms, const GscCvarEvent *,
    char *error, size_t error_size);
typedef struct { void *user; GscCvarCallback publish; } GscCvarHost;
/* R117 independently selected server-name registration. The original startup
 * guard precedes GetString0; extras keep normal expression evaluation. The
 * selected port window spans the synchronous initial start, including ready
 * children, but does not claim original child scheduling parity. Printable
 * ASCII <=127 bytes excluding double quote is transport policy. Empty names
 * publish nothing. Undefined is the port result; registration is not client
 * resource acquisition or shellshock application. */
GscProgram *gsc_program_create_with_shellshock(void);
enum { GSC_SHELLSHOCK_PRECACHE=1, GSC_SHELLSHOCK_NAME_CAPACITY=128 };
typedef struct { char name[GSC_SHELLSHOCK_NAME_CAPACITY];
    uint64_t operation_id; } GscShellshockEvent;
typedef int (*GscShellshockCallback)(void *, int logical_ms,
    const GscShellshockEvent *, char *error, size_t error_size);
typedef struct { void *user; GscShellshockCallback publish; } GscShellshockHost;
/* R118 selected print3d world debug text. Native getter order and white/1
 * defaults are reproduced; ordinary printable ASCII strings retain the VM's
 * <=127-byte bound. Finite numeric text conversion (%ld/%.9g), undefined result
 * and 256-byte transport capacity are explicit port policy. RGB/alpha/scale
 * are finite float data, copied without clamping. No original render parity. */
GscProgram *gsc_program_create_with_debug_text(void);
enum { GSC_DEBUG_TEXT_PRINT3D=1, GSC_DEBUG_TEXT_CAPACITY=256 };
typedef struct { float origin[3], color[4], scale;
    char text[GSC_DEBUG_TEXT_CAPACITY]; size_t length;
    uint64_t operation_id; } GscDebugTextEvent;
typedef int (*GscDebugTextCallback)(void *, int logical_ms,
    const GscDebugTextEvent *, char *error, size_t error_size);
typedef struct { void *user; GscDebugTextCallback publish; } GscDebugTextHost;
/* R119 selected line wrapper: endpoint/RGBA copies, integer argument4 retained
 * as an opaque native word (its original engine meaning is unproved). Existing
 * scalar/vector finite policy and undefined result are explicit port choices.
 * No original debug geometry persistence/render parity is implied. */
GscProgram *gsc_program_create_with_debug_line(void);
enum { GSC_DEBUG_LINE_DRAW=1 };
typedef struct { float start[3], end[3], color[4];
    int32_t optional_integer; uint64_t operation_id; } GscDebugLineEvent;
typedef int (*GscDebugLineCallback)(void *, int logical_ms,
    const GscDebugLineEvent *, char *error, size_t error_size);
typedef struct { void *user; GscDebugLineCallback publish; } GscDebugLineHost;
/* R120 selected ambient singleton publication. Transition conversion uses the
 * declared nearest-rounding profile; retail FPU state is unproved. Alias bytes
 * and finite/bounded time admission, undefined result are explicit port policy.
 * Host validates actual sound alias and owns engine ambience lifecycle. */
GscProgram *gsc_program_create_with_ambient(void);
enum { GSC_AMBIENT_PLAY=1, GSC_AMBIENT_STOP };
typedef struct { int kind, transition_ms; char alias[128];
    uint64_t operation_id; } GscAmbientEvent;
typedef int (*GscAmbientCallback)(void *, int logical_ms,
    const GscAmbientEvent *, char *error, size_t error_size);
typedef struct { void *user; GscAmbientCallback publish; } GscAmbientHost;
/* R121 selected item precache publication. Native initialization gate and
 * GetString scalar/vector formats are source-backed; the 127-byte printable
 * name domain and initial deferred-ready-drain admission are selected port
 * policies. Genuine host owns descriptor lookup and idempotent registration. */
GscProgram *gsc_program_create_with_precache_item(void);
enum { GSC_PRECACHE_ITEM_REGISTER=1 };
typedef struct { char name[128]; uint64_t operation_id; } GscPrecacheItemEvent;
typedef int (*GscPrecacheItemCallback)(void *, int logical_ms,
    const GscPrecacheItemEvent *, char *error, size_t error_size);
typedef struct { void *user; GscPrecacheItemCallback publish; } GscPrecacheItemHost;
/* R123 original saveGame request wrapper. A successful publish admits an
 * append-console request only, not save IO or VM serialization/restoration.
 * Old host layouts remain unchanged. Finite 127-byte getter values selected. */
GscProgram *gsc_program_create_with_save_game(void);
enum { GSC_SAVE_GAME_REQUEST=1 };
enum { GSC_SAVE_GAME_PLAYER_HEALTH=1, GSC_SAVE_GAME_MAPNAME, GSC_SAVE_GAME_CAMPAIGN };
typedef struct { int kind, player_health; char text[128]; uint64_t operation_id; } GscSaveGameQuery;
typedef struct { char filename[64], description[128], image[128]; uint64_t operation_id; } GscSaveGameEvent;
typedef int (*GscSaveGameQueryCallback)(void *, int logical_ms, GscSaveGameQuery *, char *, size_t);
typedef int (*GscSaveGameCallback)(void *, int logical_ms, const GscSaveGameEvent *, char *, size_t);
typedef struct { void *user; GscSaveGameQueryCallback query; GscSaveGameCallback publish; } GscSaveGameHost;
/* Immutable actual matching waiter registrations, captured before native
 * combat returns to script. Both task birth and each wait registration are
 * stamped, so reused slots and a rearmed wait cannot receive an older event.
 * Runtime token is the existing unique array-runtime identity. No VM pointers,
 * host callback or scheduler execution is involved in capture. Endon is still
 * unsupported by this VM and therefore has no admitted recipients to capture. */
enum { GSC_BLAST_WAITER_CAPACITY=128 };
typedef struct { uint64_t task_serial, registration_serial; } GscBlastWaiterRegistration;
typedef struct { uint64_t runtime_token; GscEntityRef owner; char event[128];
    size_t count; GscBlastWaiterRegistration registrations[GSC_BLAST_WAITER_CAPACITY]; } GscBlastWaiterSet;
int gsc_blast_capture_waiters(const GscRuntime *, const GscEntityRef *, const char *,
    GscBlastWaiterSet *, char *, size_t);

enum { GSC_HUD_PRINT=1, GSC_HUD_BOLD };
enum { GSC_HUD_GLOBAL=1, GSC_HUD_PLAYER };
typedef struct { int kind, audience; GscEntityRef receiver; size_t length;
    char text[1024]; } GscHudEvent;
typedef int (*GscHudCallback)(void *, int logical_ms, const GscHudEvent *,
    char *error, size_t error_size);
typedef struct { void *user; GscHudCallback print, bold; } GscHudHost;
enum { GSC_CONSOLE_PRINT=1, GSC_CONSOLE_PRINTLN };
enum { GSC_CONSOLE_ARGUMENT=1, GSC_CONSOLE_NEWLINE };
enum { GSC_CONSOLE_GATE_ENTRY=1, GSC_CONSOLE_GATE_ARGUMENTS };
typedef struct { int kind, phase, argument_index; size_t length; char text[128]; } GscConsoleEvent;
typedef int (*GscConsoleGateCallback)(void *, int logical_ms, int kind, int phase,
    int *enabled, char *error, size_t error_size);
typedef int (*GscConsoleCallback)(void *, int logical_ms, const GscConsoleEvent *,
    char *error, size_t error_size);
typedef struct { void *user; GscConsoleGateCallback query_gate;
    GscConsoleCallback print, println; } GscConsoleHost;
enum { GSC_RESOURCE_SHADER=1, GSC_RESOURCE_MODEL };
typedef struct { int kind; char name[128]; } GscResourceEvent;
typedef int (*GscResourceCallback)(void *, int logical_ms, const GscResourceEvent *,
                                  char *error, size_t error_size);
typedef struct { void *user; GscResourceCallback shader, model; } GscResourceHost;
/* R109 bounded original objective contracts. Native integers are admitted only
 * as exact int32 values; index range is 0..15. Text arguments admit ordinary
 * or localized strings without erasing their type, plus exact int32 decimal
 * segments. This bounded numeric spelling is not full native GetString coercion. Public earlier hosts and
 * value layouts are unchanged. Statement/expression calls return undefined;
 * references, threads and native receivers remain unsupported. */
enum { GSC_OBJECTIVE_ADD=1, GSC_OBJECTIVE_DELETE, GSC_OBJECTIVE_STATE,
       GSC_OBJECTIVE_STRING, GSC_OBJECTIVE_STRING_NOMESSAGE,
       GSC_OBJECTIVE_POSITION, GSC_OBJECTIVE_CURRENT, GSC_OBJECTIVE_RING };
typedef struct {
    int kind, index, state, has_text, has_origin;
    int origin[3]; char text[1024]; unsigned current_mask;
} GscObjectiveEvent;
typedef int (*GscObjectiveCallback)(void *, int logical_ms, const GscObjectiveEvent *,
                                   char *error, size_t error_size);
typedef struct { void *user; GscObjectiveCallback add, delete_record, state,
    string, string_nomessage, position, current, ring; } GscObjectiveHost;
int gsc_module_compile(GscProgram *program, const char *module_path,
                       const char *source, char *error, size_t error_size);
int gsc_program_link(GscProgram *program, const char *entry_module,
                     char *error, size_t error_size);
/* Opt-in fixture policy: exact local scripts precede getent expression and
 * spawn expressions and setorigin ordinary-method fallbacks. The legacy
 * linker never enables them.
 * All nodes are preflighted before a host-aware runtime can be created.
 * R137: getent with zero/one supplied arguments may link; a missing required
 * parameter is reported only when that call executes, before host lookup.
 * No default field, extra-argument support or wider string coercion is added. */
int gsc_program_link_with_entities(GscProgram *program, const char *entry_module,
                                   char *error, size_t error_size);
/* R85 additionally selects exact .origin/.angles and the authored method
 * spellings setmodel/setModel. Exact local script lookup still precedes native
 * fallback; no global script/module/field name folding is introduced. */
int gsc_program_link_with_access(GscProgram *program, const char *entry_module,
                                 char *error, size_t error_size);
/* R86 adds exact lowercase entity-owned waittill/notify statements after
 * exact local-script lookup. One nonempty string of at most 127 bytes, no
 * payload/out parameters; distinct from native imports. Scheduler cadence
 * and owner-retirement policy are bounded fixture rules, not original ABI. */
int gsc_program_link_with_events(GscProgram *program, const char *entry_module,
                                 char *error, size_t error_size);
/* R88 separately selects VM-owned state and named receiver script statements.
 * Earlier modes remain unchanged. Exact field names are case-sensitive; one
 * runtime has at most 256 level/entity field records, including stale owners.
 * Entity keys use the complete host reference. Entity origin/angles retain
 * their host-backed meaning; level fields with those names are ordinary state.
 * Arrays, expression threads and script expression results remain unsupported. */
/* Legacy default/initial allocation quantum. The separately selected owned
 * arena ceiling is a portable memory policy, not an original native pool size. */
enum { GSC_STATE_FIELD_CAPACITY=256, GSC_STATE_FIELD_OWNED_LIMIT=8192 };
int gsc_runtime_select_owned_state_arena(GscRuntime *,uint32_t max_fields);
int gsc_program_link_with_state(GscProgram *program, const char *entry_module,
                                char *error, size_t error_size);
/* R91 controls superset only: hide/show, read-only entity target/targetname,
 * ASCII-insensitive field keys, vector addition/equality and origin += vector.
 * Earlier linkers retain exact-case fields and their prior refusal policies. */
int gsc_program_link_with_controls(GscProgram *program, const char *entry_module,
                                   char *error, size_t error_size);
/* R92 explicit flow superset: for/while/do, break/continue, statement return,
 * numeric local/state mutations and readonly true/false/undefined constants.
 * Return expressions are evaluated once and discarded; script expression
 * results, arrays and switch remain unavailable. Bounded per-frame loop state
 * survives waits, events and statement calls without changing syntax IR. */
int gsc_program_link_with_flow(GscProgram *program, const char *entry_module,
                               char *error, size_t error_size);

/* R93 explicit arrays superset. Runtime-owned shared handles, exact string
 * keys and dense nonnegative integer indexes; .size is read-only entry count.
 * No sparse numeric writes, indexed undefined deletion, cycles, array truth or
 * equality. Vector indexes 0..2 are read-only. These are bounded port policies,
 * not reconstructed original VM tags/ownership rules. */
int gsc_program_link_with_arrays(GscProgram *, const char *entry_module,
                                char *error, size_t error_size);

/* R94 explicit resumable script expression results, superset of arrays.
 * Runtime-owned pending statements and bounded expression stack survive waits
 * and events. Bare return/fallthrough produce undefined; thread expressions,
 * computed calls, unbound natives and switch remain refused. */
int gsc_program_link_with_results(GscProgram *, const char *entry_module,
                                 char *error, size_t error_size);
int gsc_program_link_with_event_payloads(GscProgram *, const char *entry_module,
                                        char *error, size_t error_size);
/* Receiverless isalive/issentient, plus exact authored aliases isAlive/isSentient.
 * Exact local scripts win. One argument is a bounded port arity policy;
 * nonentities and stale identities yield false, without dereferencing them. */
int gsc_program_link_with_predicates(GscProgram *, const char *entry_module,
                                    char *error, size_t error_size);

/* R101 selected predicate/payload superset. Exact unqualified dospawn method
 * results may be entity or undefined; local scripts retain priority. No static
 * argc<=1/string preflight: timestamp gating belongs to the typed host. */
int gsc_program_link_with_actor_spawners(GscProgram *, const char *entry_module,
                                        char *error, size_t error_size);

int gsc_program_link_with_function_refs(GscProgram *, const char *, char *, size_t);
int gsc_program_link_with_scalar_switch(GscProgram *, const char *, char *, size_t);
int gsc_program_link_with_lifetime(GscProgram *, const char *, char *, size_t);
int gsc_program_link_with_names(GscProgram *, const char *, char *, size_t);
int gsc_program_link_with_music(GscProgram *, const char *, char *, size_t);
int gsc_program_link_with_resources(GscProgram *, const char *, char *, size_t);
int gsc_program_link_with_objectives(GscProgram *, const char *, char *, size_t);
int gsc_program_link_with_console(GscProgram *, const char *, char *, size_t);
int gsc_program_link_with_hud(GscProgram *, const char *, char *, size_t);
int gsc_program_link_with_mission(GscProgram *, const char *, char *, size_t);
int gsc_program_link_with_blast(GscProgram *, const char *, char *, size_t);
int gsc_program_link_with_quake(GscProgram *, const char *, char *, size_t);
int gsc_program_link_with_fx(GscProgram *, const char *, char *, size_t);
int gsc_program_link_with_cvars(GscProgram *, const char *, char *, size_t);
int gsc_program_link_with_shellshock(GscProgram *, const char *, char *, size_t);
int gsc_program_link_with_debug_text(GscProgram *, const char *, char *, size_t);
int gsc_program_link_with_debug_line(GscProgram *, const char *, char *, size_t);
int gsc_program_link_with_ambient(GscProgram *, const char *, char *, size_t);
int gsc_program_link_with_precache_item(GscProgram *, const char *, char *, size_t);
int gsc_program_link_with_save_game(GscProgram *, const char *, char *, size_t);
/* Exact int literal accessor for this mode. The older syntax.number field is
 * a float32 diagnostic view; it does not promise an exact integer payload.
 * Invalid access leaves output untouched. INT_MIN magnitude lexing remains
 * the earlier parser limit; it can be computed exactly at runtime. */
int gsc_program_scalar_integer_literal_get(const GscProgram *, int expr, int32_t *);

/* R66 read-only diagnostic audit of EVERY retained call/reference, including
 * unreachable functions and modules. This is NOT a successful link and never
 * writes target indexes, linked/invalid flags, or runtime guards. Exact-case
 * module/name matching uses the current fixture rules, NOT proven CoD1 rules.
 * Unqualified misses remain UNKNOWN_UNQUALIFIED, not confirmed native names.
 * R69 qualified named receiver calls use a separate static script binding.
 * Unqualified receiver and computed dispatch remain DEFERRED. Resolving a
 * receiver's script target does NOT implement receiver evaluation or execution.
 * R67 bare expression calls resolve to exact local scripts or fixture builtins.
 * CHECKED means the static rule passes, NOT that the program is linked or can
 * execute. Script expression execution depends on the selected runtime's
 * result/continuation capabilities; older modes retain their refusal.
 * R122 script formal counts are runtime parameter boundaries. Well-formed
 * script calls within the parser argument capacity are admitted statically;
 * an executed excess-argument call faults before its callee body. Builtin
 * exact-arity and literal preflight policies remain unchanged.
 *
 * SITE coordinates: 1-based byte line/column, zero-based module byte offset.
 * Statements point at statement start (including receiver/thread), expression
 * methods at their target name, expression threads at the thread keyword.
 * Source buffers may be released immediately after successful compile.
 * Each AST expression and instruction contributes at most one record; nested
 * calls are separate records, no AST recursion/duplication is used by audit.
 * Records arrive in instruction order then expression allocation order, NOT
 * execution order. Function references are not invocations (argc=-1).
 * wait / level waittill / level notify are reported as fixture primitives,
 * not name-bound script functions or verified original native entry points.
 *
 * Valid compiled programs can be audited before/after link or a failed link.
 * An invalid/compile-failed/empty program is refused. Missing entry is recorded
 * in summary.entry_function=-1; it does not suppress the remaining records.
 * No memory allocation is performed by the audit itself. Callback returns
 * nonzero to continue, zero to cancel; it must not mutate/free the program.
 * NULL callback performs summary-only audit. out_summary is required and is
 * zeroed on failure/cancellation; return 1 means the AUDIT completed, never
 * that link or execution succeeded. Returned records/functions are copies.
 */
enum { GSC_LINK_STATEMENT_CALL=1, GSC_LINK_STATEMENT_THREAD,
       GSC_LINK_RECEIVER_CALL, GSC_LINK_RECEIVER_THREAD,
       GSC_LINK_EXPRESSION_CALL, GSC_LINK_RECEIVER_EXPRESSION,
       GSC_LINK_SCOPED_EXPRESSION, GSC_LINK_FUNCTION_REFERENCE,
       GSC_LINK_THREAD_EXPRESSION, GSC_LINK_COMPUTED_CALL,
       GSC_LINK_COMPUTED_THREAD, GSC_LINK_COMPUTED_EXPRESSION,
       GSC_LINK_COMPUTED_THREAD_EXPRESSION, GSC_LINK_WAIT,
       GSC_LINK_LEVEL_WAITTILL, GSC_LINK_LEVEL_NOTIFY };
enum { GSC_LINK_SCRIPT_LOCAL=1, GSC_LINK_SCRIPT_QUALIFIED,
       GSC_LINK_FIXTURE_BUILTIN, GSC_LINK_UNKNOWN_UNQUALIFIED,
       GSC_LINK_MISSING_MODULE, GSC_LINK_MISSING_FUNCTION,
       GSC_LINK_COMPUTED, GSC_LINK_FIXTURE_PRIMITIVE, GSC_LINK_ENTITY_NATIVE,
       GSC_LINK_MUSIC_NATIVE, GSC_LINK_RESOURCE_NATIVE, GSC_LINK_OBJECTIVE_NATIVE,
       GSC_LINK_CONSOLE_NATIVE, GSC_LINK_HUD_NATIVE, GSC_LINK_MISSION_NATIVE, GSC_LINK_BLAST_NATIVE, GSC_LINK_QUAKE_NATIVE, GSC_LINK_FX_NATIVE, GSC_LINK_CVAR_NATIVE };
enum { GSC_LINK_SHELLSHOCK_NATIVE=GSC_LINK_CVAR_NATIVE+1 };
enum { GSC_LINK_DEBUG_TEXT_NATIVE=GSC_LINK_SHELLSHOCK_NATIVE+1 };
enum { GSC_LINK_DEBUG_LINE_NATIVE=GSC_LINK_DEBUG_TEXT_NATIVE+1 };
enum { GSC_LINK_AMBIENT_NATIVE=GSC_LINK_DEBUG_LINE_NATIVE+1 };
enum { GSC_LINK_PRECACHE_ITEM_NATIVE=GSC_LINK_AMBIENT_NATIVE+1 };
enum { GSC_LINK_SAVE_GAME_NATIVE=GSC_LINK_PRECACHE_ITEM_NATIVE+1 };
enum { GSC_LINK_CHECKED=1, GSC_LINK_BLOCKED, GSC_LINK_DEFERRED };
enum { GSC_LINK_ISSUE_UNKNOWN=1u, GSC_LINK_ISSUE_MISSING_MODULE=2u,
       GSC_LINK_ISSUE_MISSING_FUNCTION=4u, GSC_LINK_ISSUE_SCRIPT_ARITY=8u,
       GSC_LINK_ISSUE_BUILTIN_ARITY=16u, GSC_LINK_ISSUE_BUILTIN_LITERAL=32u,
       GSC_LINK_ISSUE_BUILTIN_CONTEXT=64u, GSC_LINK_ISSUE_LOCAL_EXPR=128u,
       GSC_LINK_ISSUE_RECEIVER=256u, GSC_LINK_ISSUE_COMPUTED=512u };
typedef struct {
    int site_kind, binding, outcome;
    unsigned issues;
    int instruction, expr, function_index, module_index;
    int line, col;
    size_t source_offset;
    char source_module[128], source_function[64];
    char requested_module[128], name[128];
    int receiver_expr, target_expr, argc, args[16];
    int candidate_function, candidate_module, expected_arity, builtin_index;
    char candidate_module_path[128];
    int entity_native_index; /* separate fixed R84 descriptor namespace */
    int entity_event_index;  /* separate fixed R86 primitive namespace */
} GscLinkAuditSite;
typedef struct {
    int modules, functions, instructions, expressions, entry_function;
    int sites, checked, blocked, deferred;
    int local_scripts, qualified_scripts, fixture_builtins, unknown_unqualified;
    int missing_modules, missing_functions, computed, fixture_primitives;
    int arity_issues, literal_issues, context_issues, unsupported_local_expressions;
    int entity_natives;
} GscLinkAuditSummary;
typedef struct {
    int index, module_index, argc, code_start, code_end, expr_start, expr_end;
    char module[128], name[64];
} GscLinkFunctionInfo;
typedef int (*GscLinkAuditCallback)(void *user, const GscLinkAuditSite *site);
int gsc_program_audit_links(const GscProgram *program, const char *entry_module,
                            GscLinkAuditCallback callback, void *user,
                            GscLinkAuditSummary *out_summary,
                            char *error, size_t error_size);
int gsc_program_link_function_count(const GscProgram *program);
int gsc_program_link_function_get(const GscProgram *program, int index,
                                  GscLinkFunctionInfo *out);

/* R67 read-only resolution of bare expression calls (historical syntax kind
 * GSC_SYNTAX_NATIVE_CALL), plus isdefined. Takes an expression handle, not an
 * ordinal call number. A return of 1 means a valid inspection record, NOT a
 * successful resolution/link. issues uses GSC_LINK_ISSUE_*; zero means this
 * site's static checks pass. Methods/scoped/thread/computed calls are excluded.
 * The candidate uses exact names in the caller's module only. No case folding,
 * foreign-module fallback, receiver semantics or runtime Value is introduced.
 * target_kind explicitly separates a builtin index from a script index;
 * unused indexes are -1. committed is true only after a successful whole link.
 * Failed link leaves inspection available but never exposes a committed tag.
 * Failed compile hides everything. No allocation, execution or mutation.
 * Source coordinates refer to retained call syntax. Structs are copies.
 * Existing GSC_LINK_ISSUE_LOCAL_EXPR stays reserved with its old numeric value;
 * R67 no longer emits it for exact local calls that pass static validation. */
enum { GSC_EXPR_TARGET_NONE=0, GSC_EXPR_TARGET_BUILTIN, GSC_EXPR_TARGET_LOCAL_SCRIPT,
       GSC_EXPR_TARGET_ENTITY_NATIVE, GSC_EXPR_TARGET_RESOURCE_NATIVE,
       GSC_EXPR_TARGET_OBJECTIVE_NATIVE, GSC_EXPR_TARGET_CONSOLE_NATIVE, GSC_EXPR_TARGET_HUD_NATIVE, GSC_EXPR_TARGET_MISSION_NATIVE, GSC_EXPR_TARGET_BLAST_NATIVE, GSC_EXPR_TARGET_QUAKE_NATIVE, GSC_EXPR_TARGET_FX_NATIVE, GSC_EXPR_TARGET_CVAR_NATIVE };
enum { GSC_EXPR_TARGET_SHELLSHOCK_NATIVE=GSC_EXPR_TARGET_CVAR_NATIVE+1 };
enum { GSC_EXPR_TARGET_DEBUG_TEXT_NATIVE=GSC_EXPR_TARGET_SHELLSHOCK_NATIVE+1 };
enum { GSC_EXPR_TARGET_DEBUG_LINE_NATIVE=GSC_EXPR_TARGET_DEBUG_TEXT_NATIVE+1 };
enum { GSC_EXPR_TARGET_AMBIENT_NATIVE=GSC_EXPR_TARGET_DEBUG_LINE_NATIVE+1 };
enum { GSC_EXPR_TARGET_PRECACHE_ITEM_NATIVE=GSC_EXPR_TARGET_AMBIENT_NATIVE+1 };
enum { GSC_EXPR_TARGET_SAVE_GAME_NATIVE=GSC_EXPR_TARGET_PRECACHE_ITEM_NATIVE+1 };
typedef struct {
    int expr, owner_module, line, col, argc, args[16];
    size_t source_offset;
    char module[128], name[128];
    int target_kind, builtin_index, script_function, expected_arity, committed;
    unsigned issues;
    int entity_native_index;
} GscExpressionCallInfo;
int gsc_program_expression_call_get(const GscProgram *program,int expr,
                                    GscExpressionCallInfo *out);

/* R71 read-only named receiver binding (statement call/thread, method expression,
 * named thread expression with a receiver). Exactly one handle is nonnegative;
 * the unused instruction/expr argument must be -1. Computed/implicit targets
 * and non-call nodes are excluded. Uses stored IR, not source buffer pointers.
 *
 * Qualified names: exact module + exact function, no builtin fallback/casefold.
 * R70 also binds named unqualified receiver-thread targets to the exact
 * enclosing module's script function. There is NO builtin/global/case fallback.
 * R71 additionally selects an ordinary unqualified method's exact local
 * declaration, if present; without it the method stays deferred. The
 * receiverless builtin table is NOT a receiver-native registry. The existing
 * parser ban on declarations using those three fixture builtin names stays.
 * This is an explicit fixture-only exact-local-candidate policy, NOT evidence
 * of original CoD1 script/native precedence or absence of native collisions.
 * Computed dispatch stays deferred. No fallback after selecting a script.
 * issues is zero only when the applicable structural script check passes.
 * The parser argument capacity applies independently of the formal count;
 * excess actuals are rejected by the executed callee parameter boundary.
 * target_kind/script_function describe the selected script candidate.
 * committed becomes true only after a successful WHOLE link. New receiver tags
 * and indexes are never written on failed link (legacy other tags are not made
 * transactional by this change). This does not relax ANY runtime guard.
 *
 * Source module/function are derived from the enclosing function's spans, not
 * a method expression's legacy owner_module field. Coordinates use audit rules.
 * Available pre/post link and after failed link; failed compile hides records.
 * Returns a copy, no allocations/mutations/evaluation. 0 leaves out unchanged.
 * builtin_index is always -1; receiver script indexes never use that namespace.
 * Returned kind uses GSC_LINK_RECEIVER_CALL/THREAD/EXPRESSION or
 * GSC_LINK_THREAD_EXPRESSION; it is not an original opcode namespace.
 */
enum { GSC_RECEIVER_TARGET_NONE=0, GSC_RECEIVER_TARGET_QUALIFIED_SCRIPT,
       GSC_RECEIVER_TARGET_LOCAL_SCRIPT, GSC_RECEIVER_TARGET_ENTITY_NATIVE,
       GSC_RECEIVER_TARGET_ENTITY_EVENT };
typedef struct {
    int instruction, expr, site_kind, owner_module, owner_function;
    int line, col, receiver_expr, argc, args[16];
    size_t source_offset;
    char source_module[128], source_function[64];
    char requested_module[128], name[128];
    int target_kind, script_module, script_function, builtin_index;
    int expected_arity, committed;
    unsigned issues;
    int entity_native_index;
    int entity_event_index;
} GscReceiverBindingInfo;
int gsc_program_receiver_binding_get(const GscProgram *program,
                                     int instruction, int expr,
                                     GscReceiverBindingInfo *out);

/* R68 diagnostic-only script-name query. Independent ASCII A-Z folding can
 * be requested for module paths and function names. These flags are NOT
 * linker policies and NOT a claim about original CoD1 name canonicalization.
 * No native lookup, path/slash rewriting, Unicode/locale folding, allocation,
 * binding, execution, guard modification or program mutation is performed.
 * Query all modes to distinguish path from function-name discrepancies.
 *
 * An empty/NULL requested_module means EXACT owner-module identity, even with
 * MODULE_ASCII enabled: another similarly spelled module is never local.
 * Explicit module paths must use canonical '/' segments; ASCII identifiers
 * and paths are required here. Function query length may be up to 127 bytes
 * even though declarations have the existing 63-byte cap. Empty/invalid names,
 * unknown flags, invalid owners and failed-compile programs are refused.
 * Compiled programs, including empty modules, can be inspected pre/post link
 * and after a failed link. Lookup misses are valid completed queries.
 *
 * Callback receives copies in declaration order and must not mutate/free the
 * program or input strings. NULL callback counts only. Cancellation/failure
 * zeros the required summary. Success means QUERY completed, never link.
 * module_matches includes modules without a matching function. Two matched
 * modules are ambiguous even if only one contains that function. An exact
 * hit does not hide additional folded candidates. unique_* indices are -1
 * unless unique; exact_* are diagnostic. No "first candidate wins" rule.
 */
enum { GSC_NAME_MODULE_ASCII=1u, GSC_NAME_FUNCTION_ASCII=2u };
enum { GSC_NAME_NO_MODULE=1, GSC_NAME_NO_FUNCTION, GSC_NAME_UNIQUE,
       GSC_NAME_AMBIGUOUS_MODULE, GSC_NAME_AMBIGUOUS_FUNCTION };
typedef struct {
    int index;
    char path[128];
} GscLinkModuleInfo;
int gsc_program_link_module_count(const GscProgram *program);
int gsc_program_link_module_get(const GscProgram *program,int index,
                                GscLinkModuleInfo *out);
typedef struct {
    int module_index, function_index, argc;
    int module_case_changed, function_case_changed;
    char module[128], name[64];
} GscNameCandidate;
typedef struct {
    unsigned match_flags;
    int local_scope, status, module_matches, function_matches;
    int exact_module, exact_function, unique_module, unique_function;
} GscNameCandidateSummary;
typedef int (*GscNameCandidateCallback)(void *user,const GscNameCandidate *candidate);
int gsc_program_name_candidates(const GscProgram *program,int owner_module,
                                const char *requested_module,const char *name,
                                unsigned match_flags,GscNameCandidateCallback callback,
                                void *user,GscNameCandidateSummary *out,
                                char *error,size_t error_size);

/* Single-source compatibility wrapper: compiles and links module "fixture". */
GscProgram *gsc_compile(const char *source, char *error, size_t error_size);
void gsc_program_free(GscProgram *program);

/* String spelling is retained for EVERY consumed string literal, including
 * expression values, case labels and #using_animtree names. Lexer lookahead
 * must not allocate records. raw[] excludes quotes and the localized prefix;
 * offsets refer to the original module bytes (not normalized lines).
 * FIXTURE_DECODED preserves R60's \n / \t / \" / \\ behavior; it is NOT a
 * claim that the original CoD1 decoder has been verified. With any unknown
 * escape, the WHOLE literal is RAW_UNRESOLVED and decoded[] is empty, not a
 * partly converted value. first_unknown_offset is relative to raw[], or -1.
 * Unknown sequences consume two spelling bytes; raw CR/LF, a dangling slash
 * and a missing closing quote still fail. Both kinds are limited to 127
 * lexical units and 254 spelling bytes. Localized literals remain distinct.
 * These accessors work after successful module compile, BEFORE linking, to
 * support read-only corpus audits. Compile-failed/invalid programs expose no records.
 * A failed link does not invalidate syntax records; execution is still refused.
 * Returned structs are copies; no pointers into caller-owned source survive. */
enum { GSC_STRING_VALUE_CAPACITY=128, GSC_STRING_RAW_CAPACITY=255 };
enum { GSC_STRING_FIXTURE_DECODED=1, GSC_STRING_RAW_UNRESOLVED, GSC_STRING_NATIVE_DECODED };
/* Explicit precompile selection of proved original ENGINE46BDE0 behavior:
 * n/r/t -> LF/CR/TAB, every other escaped printable byte copies without the
 * slash. Ordinary/localized kinds, raw spelling and offsets remain distinct.
 * The default parser and malformed-string refusal are unchanged. */
int gsc_program_select_r232_quoted_strings(GscProgram *);
enum { GSC_STRING_OWNER_EXPR=1, GSC_STRING_OWNER_CASE, GSC_STRING_OWNER_ANIMTREE };
typedef struct {
    int module, line, col, owner_kind, owner_index, localized, decode_status;
    int raw_length, unknown_escape_count, first_unknown_offset;
    size_t source_offset, raw_offset;
    char raw[GSC_STRING_RAW_CAPACITY], decoded[GSC_STRING_VALUE_CAPACITY];
} GscStringLiteralInfo;
int gsc_program_string_literal_count(const GscProgram *program);
int gsc_program_string_literal_get(const GscProgram *program, int index,
                                   GscStringLiteralInfo *out);

/* Explicit empty statement `;`: one instruction per statement, source line
 * and byte column are 1-based. A linked, valid program is required for this
 * inspection API. `for` header separators / comments / string contents are
 * not statements. Empty loop bodies `;` have one instruction; `{}` has none.
 * The standalone VM executes I_EMPTY as a budgeted no-op; this does not enable
 * surrounding unsupported loops/switches/developer regions. */
typedef struct { int instruction, module, line, col; } GscEmptyStmtInfo;
int gsc_program_empty_stmt_count(const GscProgram *program);
int gsc_program_empty_stmt_get(const GscProgram *program, int index,
                              GscEmptyStmtInfo *out);

/* Research-only syntax IR inspection; indexes are in compiled instruction
 * order. A body's half-open [body_start, body_end) includes nested code. */
enum { GSC_FLOW_FOR=1, GSC_FLOW_WHILE, GSC_FLOW_RETURN, GSC_FLOW_BREAK,
       GSC_FLOW_CONTINUE, GSC_FLOW_SWITCH, GSC_FLOW_DO_WHILE };
typedef struct {
    int kind, line, expr, body_start, body_end;
    /* target_instruction identifies the enclosing owner; it is not a VM PC.
     * Continue targets the for step / while condition / trailing do-while
     * condition in future lowering. No executable target is inferred. */
    int instruction, target_instruction, target_kind;
} GscFlowInfo;
int gsc_program_flow_count(const GscProgram *program);
int gsc_program_flow_get(const GscProgram *program, int index, GscFlowInfo *out);

/* Syntax-only post-test loop: the body and trailing condition are distinct.
 * Indexes are in declaration order, and the half-open body span may contain
 * nested code. instruction is an I_DO_WHILE owner, NOT an executable jump.
 * Continue binds to this owner; future lowering must reach cond_expr after
 * the body, not re-enter the body without a test. No execution is enabled.
 * line/col locate do; while_line/while_col locate the trailing while;
 * end_line/end_col locate its required terminating semicolon (1-based byte
 * columns). That semicolon is syntax, not I_EMPTY; `do ; while(x);` retains
 * exactly one I_EMPTY for its body, whereas `do {} while(x);` retains none.
 * These read-only copies work BEFORE link; source may be freed after compile.
 * Failed link keeps syntax visible; any failed compile hides all records. */
typedef struct {
    int module, line, col, while_line, while_col, end_line, end_col;
    int instruction, cond_expr, body_start, body_end;
} GscDoWhileInfo;
int gsc_program_do_while_count(const GscProgram *program);
int gsc_program_do_while_get(const GscProgram *program, int index,
                            GscDoWhileInfo *out);

/* Syntax-only switch and labels. Switch indexes are in declaration order;
 * global case indexes can interleave across nested switches. Follow next_case
 * for siblings, never assume a contiguous case array per switch. Each span is
 * [body_start, body_end); an empty span can fall through to the next label.
 * default_case is -1 if absent. No matching or VM execution is implemented. */
typedef struct { int module, line, selector_expr, instruction, body_start, body_end,
                 first_case, default_case, case_count; } GscSwitchInfo;
/* RAW_STRING carries no usable string[] value. Consult string_literal in
 * the raw table. Equal raw spellings are rejected as duplicate spellings;
 * equality between distinct unresolved/decoded spellings is NOT inferred. */
enum { GSC_CASE_INTEGER=1, GSC_CASE_STRING, GSC_CASE_DEFAULT, GSC_CASE_RAW_STRING };
typedef struct { int owner_switch, line, kind, integer, body_start, body_end,
                 next_case; char string[128]; int string_literal; } GscCaseInfo;
int gsc_program_switch_count(const GscProgram *program);
int gsc_program_switch_get(const GscProgram *program, int index, GscSwitchInfo *out);
int gsc_program_case_count(const GscProgram *program);
int gsc_program_case_get(const GscProgram *program, int index, GscCaseInfo *out);

/* Syntax-only binary remainder and bitwise-and, distinct from %animation,
 * && and &"localized". Types/coercions and VM evaluation remain unsupported. */
enum { GSC_INTEGER_MOD=1, GSC_INTEGER_BITAND };
typedef struct { int expr, line, kind, left_expr, right_expr; } GscIntegerOpInfo;
int gsc_program_integer_op_count(const GscProgram *program);
int gsc_program_integer_op_get(const GscProgram *program, int index,
                                GscIntegerOpInfo *out);

/* Research-only structural inspection of vector AST nodes. The component
 * indexes refer to complete expression trees in the program's internal IR. */
typedef struct { int expr, components[3]; } GscVectorInfo;
int gsc_program_vector_count(const GscProgram *program);
int gsc_program_vector_get(const GscProgram *program, int index, GscVectorInfo *out);

/* Syntax-only (float)expr / (int)expr casts. operand_expr is the complete
 * unary operand AST root, and kind is retained without a numeric conversion.
 * The whole linked program is refused by the standalone VM if any cast occurs. */
enum { GSC_CAST_FLOAT=1, GSC_CAST_INT };
typedef struct { int expr, line, kind, operand_expr; } GscCastInfo;
int gsc_program_cast_count(const GscProgram *program);
int gsc_program_cast_get(const GscProgram *program, int index, GscCastInfo *out);

/* Syntax-only %animation_name references, in compiled expression order.
 * Names are preserved without the % prefix; no animtree lookup is performed. */
typedef struct { int expr; char name[128]; } GscAnimRefInfo;
int gsc_program_anim_ref_count(const GscProgram *program);
int gsc_program_anim_ref_get(const GscProgram *program, int index, GscAnimRefInfo *out);

/* Syntax-only #animtree expression atoms. They are separate from the
 * #using_animtree declaration and have no invented tree identity/value.
 * Execution of any linked program containing one is refused globally. */
typedef struct { int expr, line; } GscAnimtreeValueInfo;
int gsc_program_animtree_value_count(const GscProgram *program);
int gsc_program_animtree_value_get(const GscProgram *program, int index,
                                   GscAnimtreeValueInfo *out);

/* Syntax-only [] value nodes, in compiled expression order. No array is
 * allocated by the VM and an entire linked program using one is rejected. */
typedef struct { int expr, line; } GscEmptyArrayInfo;
int gsc_program_empty_array_count(const GscProgram *program);
int gsc_program_empty_array_get(const GscProgram *program, int index,
                                GscEmptyArrayInfo *out);

/* Syntax-only computed calls. target_expr and optional receiver are roots of
 * retained ASTs, args[] retain their expression roots. No function lookup or
 * execution is inferred from these nodes. */
enum { GSC_DYNAMIC_CALL=1, GSC_DYNAMIC_THREAD,
       GSC_DYNAMIC_EXPR_CALL };
/* Public capacity matches the parser's 16-argument maximum. */
enum { GSC_DYNAMIC_INFO_MAX_ARGS=16 };
typedef struct { int kind, line, target_expr, receiver, argc;
                 int args[GSC_DYNAMIC_INFO_MAX_ARGS]; } GscDynamicCallInfo;
int gsc_program_dynamic_call_count(const GscProgram *program);
int gsc_program_dynamic_call_get(const GscProgram *program, int index,
                                 GscDynamicCallInfo *out);

/* A direct path-qualified expression call is distinct from an unscoped
 * builtin, receiver method, or bare ::function reference. Linker validates
 * its named target in every module; execution is still unsupported. */
typedef struct { int expr, line, target, argc;
                 char module[128], name[128];
                 int args[GSC_DYNAMIC_INFO_MAX_ARGS]; } GscScopedExprCallInfo;
int gsc_program_scoped_expr_call_count(const GscProgram *program);
int gsc_program_scoped_expr_call_get(const GscProgram *program, int index,
                                    GscScopedExprCallInfo *out);

/* A first-class function reference is an expression value, not an invocation.
 * `local` marks ::fn; `module` then names the module where it appeared.
 * Qualified path references are normalized to / in `module`. Every reference
 * is resolved to a function index across all modules before runtime creation.
 * No callable value, dynamic dispatch or function-pointer save format exists
 * in this fixture, so any reference blocks the entire linked VM at runtime. */
typedef struct { int expr, line, target, local;
                 char module[128], name[128]; } GscFuncRefInfo;
int gsc_program_func_ref_count(const GscProgram *program);
int gsc_program_func_ref_get(const GscProgram *program, int index,
                            GscFuncRefInfo *out);

/* Research-only developer regions /# ... #/. Contents remain ordinary
 * compiled IR/declarations, with both spans half-open. Execution is blocked
 * for the entire linked program until the native modes are understood. */
enum { GSC_DEV_STATEMENT=1, GSC_DEV_FILE };
typedef struct { int scope, module, line, col, code_start, code_end,
                 func_start, func_end; } GscDevBlockInfo;
int gsc_program_dev_block_count(const GscProgram *program);
int gsc_program_dev_block_get(const GscProgram *program, int index,
                              GscDevBlockInfo *out);

/* Statement-level compound assignment and postfix ++/-- retain an lvalue
 * expression tree and an optional RHS expression. They are syntax-only:
 * no game value or local is read or written by these instructions. */
enum { GSC_MUT_ADD=1, GSC_MUT_SUB, GSC_MUT_MUL, GSC_MUT_DIV,
       GSC_MUT_INC, GSC_MUT_DEC, GSC_MUT_BIT_OR, GSC_MUT_BIT_XOR, GSC_MUT_BIT_AND };
typedef struct { int line, op, lvalue_expr, rhs_expr; } GscMutationInfo;
int gsc_program_mutation_count(const GscProgram *program);
int gsc_program_mutation_get(const GscProgram *program, int index,
                             GscMutationInfo *out);
/* R63 pre-link read-only for-header inspection, in loop declaration order.
 * Init remains the existing optional '=' clause. The step keeps its lvalue
 * and full RHS exactly once: no synthetic read/add/write or body instruction.
 * For compound steps, step_op is GSC_MUT_ADD..GSC_MUT_DIV; otherwise it is 0.
 * Absent expression/target indexes are -1. body is [body_start, body_end).
 * An empty ';' body occupies one instruction; an empty '{}' occupies none.
 * continue owns 'instruction', NOT a VM jump address: later lowering must
 * reach this header step before its next condition. No lowering exists yet.
 * FOR_NO_STEP/ASSIGN/INCREMENT/DECREMENT keep the fixture's previous values.
 * Successful module compile suffices; failed link preserves syntax, failed
 * compile hides it. Returns copies. This is not an original opcode/value ABI.
 * Header mutations are NOT counted by the statement-level mutation API. */
enum { GSC_FOR_NO_STEP=0, GSC_FOR_ASSIGN, GSC_FOR_INCREMENT,
       GSC_FOR_DECREMENT, GSC_FOR_COMPOUND_ASSIGN };
typedef struct {
    int index, instruction, owner_module, line;
    int init_target, init_expr, cond_expr, step_target, step_expr;
    int step_kind, step_op, body_start, body_end;
} GscForInfo;
int gsc_program_for_count(const GscProgram *program);
int gsc_program_for_get(const GscProgram *program, int index, GscForInfo *out);

/* Walk a mutation's lvalue from its root. A field/index has a base_expr;
 * an index also has an expression for its subscript. */
enum { GSC_LVALUE_VAR=1, GSC_LVALUE_FIELD, GSC_LVALUE_INDEX };
typedef struct { int kind, base_expr, index_expr; char name[128]; } GscLvalueInfo;
int gsc_program_lvalue_get(const GscProgram *program, int expr,
                           GscLvalueInfo *out);

/* Read-only syntax inspection, available BEFORE linking. This includes
 * receiver statement threads in every successfully compiled module, including
 * unreachable bodies. No runtime receiver, function value, scheduling result,
 * or game entity is invented. Failed compiles hide all syntax records; failed
 * links leave successful syntax available, like the raw-string API.
 * A named target's module[] is empty when unqualified, normalized otherwise.
 * A computed target has target_expr >= 0 and empty name/module. owner_module
 * refers to compilation order. The location is the START OF THE STATEMENT.
 * All structs are copies and expression indexes are opaque syntax handles. */
enum { GSC_RECEIVER_THREAD_NAMED=1, GSC_RECEIVER_THREAD_COMPUTED };
typedef struct {
    int instruction, owner_module, line, col, kind, receiver_expr, target_expr;
    int argc, args[GSC_DYNAMIC_INFO_MAX_ARGS];
    char module[128], name[128];
} GscReceiverThreadInfo;
int gsc_program_receiver_thread_count(const GscProgram *program);
int gsc_program_receiver_thread_get(const GscProgram *program, int index,
                                    GscReceiverThreadInfo *out);

/* R64 syntax-only thread invocation AS AN EXPRESSION (not an instruction).
 * a named/computed target, optional receiver, and argument trees are each
 * retained once. receiver_expr == -1 records absence, NOT a synthesized self.
 * Coordinates locate the thread KEYWORD. Records are available pre-link and
 * after failed link, hidden after failed compile. Counts exclude statements.
 * Named implicit targets are script-link-checked like statement I_THREAD;
 * R69 qualified named receiver targets receive static script checks, while
 * unqualified receiver targets remain deferred. No native is
 * invented, and no return type/value/scheduling behavior is asserted.
 * Every program containing this kind is globally refused by the fixture VM. */
enum { GSC_THREAD_EXPR_NAMED=1, GSC_THREAD_EXPR_COMPUTED };
typedef struct {
    int expr, owner_module, line, col, kind, receiver_expr, target_expr;
    int argc, args[GSC_DYNAMIC_INFO_MAX_ARGS];
    char module[128], name[128];
} GscThreadExprInfo;
int gsc_program_thread_expr_count(const GscProgram *program);
int gsc_program_thread_expr_get(const GscProgram *program,int index,GscThreadExprInfo *out);

/* Syntax expression walk (not a Value API). left/right are normalized:
 * field/unary/cast -> left; binary/index -> left/right; method -> left is
 * receiver; computed call -> left is target, optional right is receiver.
 * Thread expression -> left is computed target (or -1 for named), right is
 * optional receiver, operator_token is GSC_THREAD_EXPR_NAMED/COMPUTED.
 * Function-call and vector arguments live in args[]; isdefined has one arg.
 * NATIVE_CALL is the legacy spelling for a bare call; it is NOT proof of a
 * native target. Use expression_call_get for R67's explicit target namespace.
 * Strings have string_literal, NOT a child expression. Unused child indexes
 * and arguments are -1. name/module preserve syntax, NOT resolved identities.
 * operator_token is the internal fixture token, not a native CoD1 opcode.
 * raw strings have no decoded name value; inspect their retained literal.
 * This reuses the fixture expression grammar; extra accepted fixture forms
 * are NOT evidence of original receiver typing, precedence or evaluation order. */
enum { GSC_SYNTAX_NUMBER=1, GSC_SYNTAX_STRING, GSC_SYNTAX_LOCALIZED_STRING,
       GSC_SYNTAX_VAR, GSC_SYNTAX_UNARY, GSC_SYNTAX_BINARY, GSC_SYNTAX_ISDEFINED,
       GSC_SYNTAX_FIELD, GSC_SYNTAX_INDEX, GSC_SYNTAX_NATIVE_CALL,
       GSC_SYNTAX_METHOD_CALL, GSC_SYNTAX_VECTOR, GSC_SYNTAX_ANIM_REF,
       GSC_SYNTAX_DYNAMIC_CALL, GSC_SYNTAX_EMPTY_ARRAY, GSC_SYNTAX_SCOPED_CALL,
       GSC_SYNTAX_CAST, GSC_SYNTAX_FUNC_REF, GSC_SYNTAX_ANIMTREE_VALUE,
       GSC_SYNTAX_RAW_STRING, GSC_SYNTAX_RAW_LOCALIZED_STRING, GSC_SYNTAX_THREAD_CALL };
typedef struct {
    int expr, kind, operator_token, left_expr, right_expr, string_literal;
    int argc, args[GSC_DYNAMIC_INFO_MAX_ARGS], integer;
    float number;
    char name[128], module[128];
} GscSyntaxExprInfo;
int gsc_program_syntax_expr_count(const GscProgram *program);
int gsc_program_syntax_expr_get(const GscProgram *program, int expr,
                               GscSyntaxExprInfo *out);

/* R73 read-only counters from the private fixture isdefined migration.
 * Not original VM/native IDs, not a host-registration or mutation interface.
 * Counters saturate at ULLONG_MAX and are independent for each runtime.
 * callbacks increments inside the actual registered C callback. completed
 * additionally requires checked result conversion. Query never invokes code.
 * Both IDs refer to DIFFERENT namespaces; do not compare/use interchangeably.
 * On invalid input, query returns 0 and leaves the caller's output untouched.
 */
typedef struct {
    unsigned long long isdefined_dispatches, isdefined_callbacks, isdefined_completed;
    int isdefined_builtin_index, isdefined_import_index;
} GscRuntimeNativeStats;
int gsc_runtime_native_stats(const GscRuntime *runtime,GscRuntimeNativeStats *out);

/* R74: independent read-only fog diagnostics; the R73 stats layout is unchanged.
 * callbacks increments INSIDE the native handler, before domain validation;
 * completed requires a successful VOID result and no runtime error.
 * output_calls counts actual user fog callback entries; output_accepted counts
 * nonzero callback returns (including ones that also poison the scheduler).
 * A rejected callback's effects cannot be rolled back. Indices are private
 * registry-local IDs, not game ABI IDs. All counters saturate and are per-VM. */
typedef struct {
    unsigned long long exp_dispatches, exp_callbacks, exp_completed;
    unsigned long long cull_dispatches, cull_callbacks, cull_completed;
    unsigned long long output_calls, output_accepted;
    int exp_builtin_index, exp_import_index, cull_builtin_index, cull_import_index;
} GscRuntimeFogNativeStats;
int gsc_runtime_fog_native_stats(const GscRuntime *runtime,GscRuntimeFogNativeStats *out);

/* Runtime owns its private native registry and callback context. Program must
 * outlive runtime; destruction is not allowed from callbacks/during advance.
 * No externally supplied registry is accepted. Global unsupported-code guards
 * execute before any runtime/native allocation, let alone execution. */
GscRuntime *gsc_runtime_new(const GscProgram *program, GscFogCallback fog,
                            void *user, char *error, size_t error_size);
GscRuntime *gsc_runtime_new_with_entities(const GscProgram *program,
    GscFogCallback fog, void *user, const GscEntityHost *entities,
    char *error, size_t error_size);
GscRuntime *gsc_runtime_new_with_access(const GscProgram *program,
    GscFogCallback fog, void *user, const GscEntityHost *entities,
    const GscEntityAccessHost *access, char *error, size_t error_size);
/* Requires the events linker; earlier constructors never enable events.
 * Existing host layouts and optional access capabilities remain unchanged. */
GscRuntime *gsc_runtime_new_with_events(const GscProgram *program,
    GscFogCallback fog, void *user, const GscEntityHost *entities,
    const GscEntityAccessHost *access, char *error, size_t error_size);
/* State-mode main has undefined self. Bare calls/threads inherit frame self;
 * explicit entity/level script receivers replace it only in the new frame or
 * child task. Entity initial owners and all active frame self identities are
 * checked at continuation boundaries; retirement cancels only that task.
 * This is bounded restoration policy, not original thread ownership ABI.
 * State lasts until runtime free. Whole level values cannot be exported to
 * native calls, including isdefined; field values use the existing bridge. */
GscRuntime *gsc_runtime_new_with_state(const GscProgram *program,
    GscFogCallback fog, void *user, const GscEntityHost *entities,
    const GscEntityAccessHost *access, char *error, size_t error_size);
GscRuntime *gsc_runtime_new_with_controls(const GscProgram *program,
    GscFogCallback fog, void *user, const GscEntityHost *entities,
    const GscEntityAccessHost *access, const GscEntityControlHost *controls,
    char *error, size_t error_size);
GscRuntime *gsc_runtime_new_with_flow(const GscProgram *program,
    GscFogCallback fog, void *user, const GscEntityHost *entities,
    const GscEntityAccessHost *access, const GscEntityControlHost *controls,
    char *error, size_t error_size);
GscRuntime *gsc_runtime_new_with_arrays(const GscProgram *, GscFogCallback,
    void *user, const GscEntityHost *, const GscEntityAccessHost *,
    const GscEntityControlHost *, const GscEntityCollectionHost *,
    char *error, size_t error_size);
GscRuntime *gsc_runtime_new_with_results(const GscProgram *, GscFogCallback,
    void *user, const GscEntityHost *, const GscEntityAccessHost *,
    const GscEntityControlHost *, const GscEntityCollectionHost *,
    char *error, size_t error_size);
GscRuntime *gsc_runtime_new_with_event_payloads(const GscProgram *, GscFogCallback,
    void *user, const GscEntityHost *, const GscEntityAccessHost *,
    const GscEntityControlHost *, const GscEntityCollectionHost *,
    char *error, size_t error_size);
GscRuntime *gsc_runtime_new_with_predicates(const GscProgram *, GscFogCallback,
    void *user, const GscEntityHost *, const GscEntityAccessHost *,
    const GscEntityControlHost *, const GscEntityCollectionHost *,
    const GscEntityPredicateHost *, char *error, size_t error_size);
GscRuntime *gsc_runtime_new_with_actor_spawners(const GscProgram *, GscFogCallback,
    void *user, const GscEntityHost *, const GscEntityAccessHost *,
    const GscEntityControlHost *, const GscEntityCollectionHost *,
    const GscEntityPredicateHost *, const GscActorSpawnHost *, char *, size_t);
GscRuntime *gsc_runtime_new_with_function_refs(const GscProgram *, GscFogCallback,
    void *user, const GscEntityHost *, const GscEntityAccessHost *,
    const GscEntityControlHost *, const GscEntityCollectionHost *,
    const GscEntityPredicateHost *, const GscActorSpawnHost *, char *, size_t);
GscRuntime *gsc_runtime_new_with_scalar_switch(const GscProgram *, GscFogCallback,
    void *user, const GscEntityHost *, const GscEntityAccessHost *,
    const GscEntityControlHost *, const GscEntityCollectionHost *,
    const GscEntityPredicateHost *, const GscActorSpawnHost *, char *, size_t);
GscRuntime *gsc_runtime_new_with_lifetime(const GscProgram *, GscFogCallback,
    void *user, const GscEntityHost *, const GscEntityAccessHost *,
    const GscEntityControlHost *, const GscEntityCollectionHost *,
    const GscEntityPredicateHost *, const GscActorSpawnHost *, char *, size_t);
GscRuntime *gsc_runtime_new_with_names(const GscProgram *, GscFogCallback,
    void *user, const GscEntityHost *, const GscEntityAccessHost *,
    const GscEntityControlHost *, const GscEntityCollectionHost *,
    const GscEntityPredicateHost *, const GscActorSpawnHost *, char *, size_t);
GscRuntime *gsc_runtime_new_with_music(const GscProgram *, GscFogCallback,
    void *user, const GscEntityHost *, const GscEntityAccessHost *,
    const GscEntityControlHost *, const GscEntityCollectionHost *,
    const GscEntityPredicateHost *, const GscActorSpawnHost *, const GscMusicHost *, char *, size_t);
GscRuntime *gsc_runtime_new_with_resources(const GscProgram *, GscFogCallback,
    void *user, const GscEntityHost *, const GscEntityAccessHost *,
    const GscEntityControlHost *, const GscEntityCollectionHost *,
    const GscEntityPredicateHost *, const GscActorSpawnHost *, const GscMusicHost *,
    const GscResourceHost *, char *, size_t);
GscRuntime *gsc_runtime_new_with_objectives(const GscProgram *, GscFogCallback,
    void *user, const GscEntityHost *, const GscEntityAccessHost *,
    const GscEntityControlHost *, const GscEntityCollectionHost *,
    const GscEntityPredicateHost *, const GscActorSpawnHost *, const GscMusicHost *,
    const GscResourceHost *, const GscObjectiveHost *, char *, size_t);
GscRuntime *gsc_runtime_new_with_console(const GscProgram *, GscFogCallback,
    void *user, const GscEntityHost *, const GscEntityAccessHost *,
    const GscEntityControlHost *, const GscEntityCollectionHost *,
    const GscEntityPredicateHost *, const GscActorSpawnHost *, const GscMusicHost *,
    const GscResourceHost *, const GscObjectiveHost *, const GscConsoleHost *, char *, size_t);
GscRuntime *gsc_runtime_new_with_hud(const GscProgram *, GscFogCallback,
    void *user, const GscEntityHost *, const GscEntityAccessHost *,
    const GscEntityControlHost *, const GscEntityCollectionHost *,
    const GscEntityPredicateHost *, const GscActorSpawnHost *, const GscMusicHost *,
    const GscResourceHost *, const GscObjectiveHost *, const GscConsoleHost *, const GscHudHost *, char *, size_t);
GscRuntime *gsc_runtime_new_with_mission(const GscProgram *, GscFogCallback,
    void *user, const GscEntityHost *, const GscEntityAccessHost *,
    const GscEntityControlHost *, const GscEntityCollectionHost *,
    const GscEntityPredicateHost *, const GscActorSpawnHost *, const GscMusicHost *,
    const GscResourceHost *, const GscObjectiveHost *, const GscConsoleHost *,
    const GscHudHost *, const GscMissionHost *, char *, size_t);
GscRuntime *gsc_runtime_new_with_blast(const GscProgram *, GscFogCallback,
    void *user, const GscEntityHost *, const GscEntityAccessHost *,
    const GscEntityControlHost *, const GscEntityCollectionHost *,
    const GscEntityPredicateHost *, const GscActorSpawnHost *, const GscMusicHost *,
    const GscResourceHost *, const GscObjectiveHost *, const GscConsoleHost *,
    const GscHudHost *, const GscMissionHost *, const GscBlastHost *, char *, size_t);
GscRuntime *gsc_runtime_new_with_quake(const GscProgram *, GscFogCallback,
    void *user, const GscEntityHost *, const GscEntityAccessHost *,
    const GscEntityControlHost *, const GscEntityCollectionHost *,
    const GscEntityPredicateHost *, const GscActorSpawnHost *, const GscMusicHost *,
    const GscResourceHost *, const GscObjectiveHost *, const GscConsoleHost *,
    const GscHudHost *, const GscMissionHost *, const GscBlastHost *, const GscQuakeHost *, char *, size_t);
GscRuntime *gsc_runtime_new_with_fx(const GscProgram *, GscFogCallback,
    void *user, const GscEntityHost *, const GscEntityAccessHost *,
    const GscEntityControlHost *, const GscEntityCollectionHost *,
    const GscEntityPredicateHost *, const GscActorSpawnHost *, const GscMusicHost *,
    const GscResourceHost *, const GscObjectiveHost *, const GscConsoleHost *,
    const GscHudHost *, const GscMissionHost *, const GscBlastHost *, const GscQuakeHost *, const GscFxHost *, char *, size_t);
GscRuntime *gsc_runtime_new_with_cvars(const GscProgram *, GscFogCallback,
    void *user, const GscEntityHost *, const GscEntityAccessHost *,
    const GscEntityControlHost *, const GscEntityCollectionHost *,
    const GscEntityPredicateHost *, const GscActorSpawnHost *, const GscMusicHost *,
    const GscResourceHost *, const GscObjectiveHost *, const GscConsoleHost *,
    const GscHudHost *, const GscMissionHost *, const GscBlastHost *, const GscQuakeHost *, const GscFxHost *, const GscCvarHost *, char *, size_t);
GscRuntime *gsc_runtime_new_with_shellshock(const GscProgram *, GscFogCallback,
    void *user, const GscEntityHost *, const GscEntityAccessHost *,
    const GscEntityControlHost *, const GscEntityCollectionHost *,
    const GscEntityPredicateHost *, const GscActorSpawnHost *, const GscMusicHost *,
    const GscResourceHost *, const GscObjectiveHost *, const GscConsoleHost *,
    const GscHudHost *, const GscMissionHost *, const GscBlastHost *, const GscQuakeHost *, const GscFxHost *, const GscCvarHost *, const GscShellshockHost *, char *, size_t);
GscRuntime *gsc_runtime_new_with_debug_text(const GscProgram *, GscFogCallback,
    void *user, const GscEntityHost *, const GscEntityAccessHost *,
    const GscEntityControlHost *, const GscEntityCollectionHost *,
    const GscEntityPredicateHost *, const GscActorSpawnHost *, const GscMusicHost *,
    const GscResourceHost *, const GscObjectiveHost *, const GscConsoleHost *,
    const GscHudHost *, const GscMissionHost *, const GscBlastHost *, const GscQuakeHost *, const GscFxHost *, const GscCvarHost *, const GscShellshockHost *, const GscDebugTextHost *, char *, size_t);
GscRuntime *gsc_runtime_new_with_debug_line(const GscProgram *, GscFogCallback,
    void *user, const GscEntityHost *, const GscEntityAccessHost *,
    const GscEntityControlHost *, const GscEntityCollectionHost *,
    const GscEntityPredicateHost *, const GscActorSpawnHost *, const GscMusicHost *,
    const GscResourceHost *, const GscObjectiveHost *, const GscConsoleHost *,
    const GscHudHost *, const GscMissionHost *, const GscBlastHost *, const GscQuakeHost *, const GscFxHost *, const GscCvarHost *, const GscShellshockHost *, const GscDebugTextHost *, const GscDebugLineHost *, char *, size_t);
GscRuntime *gsc_runtime_new_with_ambient(const GscProgram *, GscFogCallback,
    void *user, const GscEntityHost *, const GscEntityAccessHost *,
    const GscEntityControlHost *, const GscEntityCollectionHost *,
    const GscEntityPredicateHost *, const GscActorSpawnHost *, const GscMusicHost *,
    const GscResourceHost *, const GscObjectiveHost *, const GscConsoleHost *,
    const GscHudHost *, const GscMissionHost *, const GscBlastHost *, const GscQuakeHost *, const GscFxHost *, const GscCvarHost *, const GscShellshockHost *, const GscDebugTextHost *, const GscDebugLineHost *, const GscAmbientHost *, char *, size_t);
GscRuntime *gsc_runtime_new_with_precache_item(const GscProgram *, GscFogCallback,
    void *user, const GscEntityHost *, const GscEntityAccessHost *,
    const GscEntityControlHost *, const GscEntityCollectionHost *,
    const GscEntityPredicateHost *, const GscActorSpawnHost *, const GscMusicHost *,
    const GscResourceHost *, const GscObjectiveHost *, const GscConsoleHost *,
    const GscHudHost *, const GscMissionHost *, const GscBlastHost *, const GscQuakeHost *, const GscFxHost *, const GscCvarHost *, const GscShellshockHost *, const GscDebugTextHost *, const GscDebugLineHost *, const GscAmbientHost *, const GscPrecacheItemHost *, char *, size_t);
GscRuntime *gsc_runtime_new_with_save_game(const GscProgram *, GscFogCallback,
    void *user, const GscEntityHost *, const GscEntityAccessHost *,
    const GscEntityControlHost *, const GscEntityCollectionHost *,
    const GscEntityPredicateHost *, const GscActorSpawnHost *, const GscMusicHost *,
    const GscResourceHost *, const GscObjectiveHost *, const GscConsoleHost *,
    const GscHudHost *, const GscMissionHost *, const GscBlastHost *, const GscQuakeHost *, const GscFxHost *, const GscCvarHost *, const GscShellshockHost *, const GscDebugTextHost *, const GscDebugLineHost *, const GscAmbientHost *, const GscPrecacheItemHost *, const GscSaveGameHost *, char *, size_t);
/* Bounded port policy: current waiters only, broadcast by exact scope/name;
 * missing values become undefined, extra values are ignored. waittill outputs
 * must be simple nonreserved local names; duplicate names bind left to right
 * (last wins). Internal array payloads share the existing VM-owned alias.
 * External inputs are copied before any callback. Arrays and whole-level values
 * have no external representation. Entity payload identities must be complete,
 * but may be stale opaque values; dereference validates at use as with returns. */
enum { GSC_EVENT_UNDEFINED, GSC_EVENT_NUMBER, GSC_EVENT_STRING,
       GSC_EVENT_VECTOR, GSC_EVENT_ENTITY, GSC_EVENT_PAYLOAD_CAPACITY=15 };
typedef struct { int kind; float number; char string[128]; float vector[3];
                 GscEntityRef entity; } GscEventValue;
int gsc_notify_with_payload(GscRuntime *, const char *event,
    const GscEventValue *values, size_t count, int *out_awakened);
int gsc_notify_entity_with_payload(GscRuntime *, const GscEntityRef *owner,
    const char *event, const GscEventValue *values, size_t count,
    int *out_awakened);
int gsc_blast_notify_captured(GscRuntime *, const GscBlastWaiterSet *,
    const GscEventValue *, size_t, int *out_awakened);
void gsc_runtime_free(GscRuntime *runtime);
int gsc_start(GscRuntime *runtime);                 /* starts main at logical t=0 */
/* R168 explicitly selected entity field namespace, separate from the frozen
 * pose/control hosts. classify is pure: 0=dynamic, 1=native, -1=unavailable.
 * A native field NEVER falls through to private StateFields after refusal.
 * Semantic scalar/string/vector copies use raw Value type tags, not native
 * addresses, object IDs or argument stack cells. Read supports 0/1/2/3/4/5;
 * write also labels object/function inputs 7/9 for explicit type rejection.
 * read/write return exactly 1 on success, 0 on refusal. They own no borrowed
 * pointers. user must remain valid until runtime teardown. Reentry during
 * any field callback is fatal; successful owner retirement cancels its task.
 * Select once before start in a state/scalar/names profile. This does not add
 * builtin names, prove every field, or admit completed actors/getaiarray. */
typedef struct {
    void *user;
    int (*classify)(void *,const GscEntityRef *,const char *,char *,size_t);
    int (*read)(void *,const GscEntityRef *,const char *,GscStringCastValue *,char *,size_t);
    int (*write)(void *,const GscEntityRef *,const char *,const GscStringCastValue *,char *,size_t);
} GscEntityFieldHost;
int gsc_runtime_select_entity_fields(GscRuntime *,const GscEntityFieldHost *);
/* R170 explicit private attach/detach/detachAll gateway. Select the program
 * before linking, then select the host once before start in a state/scalar/
 * names profile. Default selectors and the native import registry are frozen.
 * Exact local script candidates retain precedence; these native methods are
 * ordinary statement-only VOID calls, never threads or function references.
 * Receiver and operands are copied, with full epoch/slot/generation identity.
 * Native arguments evaluate left to right, including ignored extras. args is
 * a complete argc-cell frame (0..16); attach/detach export only the first two
 * consumed cells, and ignored cells are undefined. detachAll exports none.
 * Semantic tags follow the field host (object/function rejection tags 7/9),
 * without native string IDs, stack pointers or ownership transfers. invoke
 * returns exactly 1 for completed VOID, 0 for refusal/error; its copied error
 * is fatal, including an original script error after a committed mutation.
 * Reentry during validation/invocation is fatal; successful retirement of the
 * executing owner cancels its task. This does not publish initialized actors,
 * select a constructor coordinator or admit live animation/AI membership. */
enum { GSC_ENTITY_METHOD_ATTACH=1, GSC_ENTITY_METHOD_DETACH,
       GSC_ENTITY_METHOD_DETACH_ALL };
typedef struct {
    void *user;
    int (*invoke)(void *,const GscEntityRef *,uint32_t,
                  const GscStringCastValue *,int,char *,size_t);
} GscEntityMethodHost;
int gsc_program_select_entity_model_methods(GscProgram *);
int gsc_runtime_select_entity_methods(GscRuntime *,const GscEntityMethodHost *);
/* R180 independent selected VOID setspawnerteam route. Only copied plain
 * string argument0 is admitted; missing/wrong kinds refuse, ignored extras
 * still evaluate left to right. No actor/model method host is implied. */
typedef struct {
    void *user;
    int (*validate)(void *,const GscEntityRef *,char *,size_t);
    int (*set)(void *,const GscEntityRef *,const char *,char *,size_t);
} GscSpawnerTeamMethodHost;
int gsc_program_select_spawner_team_method(GscProgram *);
int gsc_runtime_select_spawner_team(GscRuntime *,const GscSpawnerTeamMethodHost *);
int gsc_runtime_spawner_team_available(const GscRuntime *);
/* R190 selected Actor critical-section methods. Not a scheduler lock or AI
 * readiness predicate. Native operands still evaluate before dispatch; the
 * host MUST call read_label only AFTER committing an admitted ENTER, and only
 * when has_argument is true. The optional current-script label belongs to the
 * Actor host, never inferred from a VM function/module name. Only plain string
 * labels (including empty) are admitted by this first VM lane. No retained
 * pointers, generic GetConstString coercion, native string IDs/refcounts.
 * invoke returns 1 on completion, 0 on refusal/error. query result must be
 * exactly 0/1. A failed label read deliberately does not roll back ENTER.
 * current/read_label/context are synchronous borrowed callbacks. A host must
 * revalidate the original Actor incarnation after each callback before commit.
 * User host storage lives through runtime teardown. Both selectors are opt-in;
 * statement-only ENTER/LEAVE; QUERY may be used in expressions or discarded. */
enum { GSC_ACTOR_CRITICAL_ENTER=1, GSC_ACTOR_CRITICAL_LEAVE=2,
       GSC_ACTOR_CRITICAL_QUERY=3 };
typedef struct {
    void *context;
    int has_argument;
    int (*read_label)(void *,char *,size_t,char *,size_t);
    int (*current)(void *);
} GscActorCriticalCall;
typedef struct {
    void *user;
    int (*invoke)(void *,const GscEntityRef *,uint32_t,
                  const GscActorCriticalCall *,int32_t *,char *,size_t);
} GscActorCriticalHost;
/* R243 opt-in native tracking. Actual Actor owns current/previous/reason.
 * Host validates Actor before argc, requires argc2, copies previous=current,
 * calls read0 then commits current, calls read1 then commits reason, and only
 * then tests byte-exact equality with presence. Failure retains write prefix.
 * read uses native StringArguments conversion in its existing bounded domain;
 * callbacks are synchronous borrowed, ordered exactly once, never retained.
 * No FSM/queue/main/goal/end readiness is implied by this tracking method. */
typedef struct {
 void*context;int argc;
 int(*read)(void*,unsigned,char*,size_t,char*,size_t);
 int(*current)(void*);
} GscActorScriptStateCall;
typedef struct {
 void*user;
 int(*invoke)(void*,const GscEntityRef*,const GscActorScriptStateCall*,char*,size_t);
 int(*read)(void*,const GscEntityRef*,const char*,GscStringCastValue*,char*,size_t);
} GscActorScriptStateHost;
int gsc_program_select_r243_script_state(GscProgram*);
int gsc_runtime_select_r243_script_state(GscRuntime*,const GscActorScriptStateHost*);
int gsc_program_select_actor_critical_methods(GscProgram *);
int gsc_runtime_select_actor_critical_methods(GscRuntime *,const GscActorCriticalHost *);
/* R172 selected runtime-owned global game array. Select once before start in
 * a linked state/array/scalar/names profile; old selectors stay unselected.
 * The initial root is an empty array shared across script frames, threads and
 * public entity entries. Indexed reads/writes, aliases and size use the normal
 * bounded VM array implementation. game=[] or another same-runtime array may
 * replace the root; non-array/foreign root writes refuse without changing it.
 * Exact game lookup precedes locals. A formal parameter or waittill output
 * named game anywhere in the linked program refuses selection before root
 * publication. Selection failure is nonfatal and does not allocate a root.
 * Captured indexed writes refuse if their root/ancestor changes while yielded.
 * Values and private runtime cookies are not native IDs or exported pointers.
 * Runtime teardown drops this root and all runtime arrays; a new runtime starts
 * empty. Native changelevel may preserve its game cell: cross-runtime/map/SAVE
 * carry and native pruning/refcounts are NOT supplied by this capability. */
int gsc_runtime_select_game_array(GscRuntime *);
/* R173 explicit external owner for a portable game ARRAY DAG checkpoint.
 * Capture a quiescent selected runtime, then restore into a fresh unstarted,
 * unselected state/array/scalar/names runtime. No Session/GAME transition is
 * attached automatically. A new/cleared owner restores one empty root.
 * The owner deep-copies exact int32/finite float32/plain or localized text/
 * finite vectors and shared arrays. Entity/structure/level/function members are pruned
 * without host callbacks. Numeric keys are never renumbered: capture refuses
 * unless surviving numeric keys remain exactly 0..N-1. Provider keybinding
 * arrays refuse capture; no stale IDs/cookies are carried.
 * Capture/restore failures leave the runtime and prior owner unchanged and
 * report only through the separate error buffer. Active execution, malformed
 * graphs, cycles, capacities and formal/waittill game shadowing refuse.
 * Owner/free/clear and later source mutations cannot affect a restored runtime.
 * Runtime cookies/array IDs are remapped; this is no native heap/refcount,
 * changelevel activation, SAVE serialization or full native pruning proof. */
typedef struct GscGameArrayOwner GscGameArrayOwner;
GscGameArrayOwner *gsc_game_array_owner_new(void);
void gsc_game_array_owner_free(GscGameArrayOwner *);
int gsc_game_array_owner_clear(GscGameArrayOwner *);
int gsc_game_array_owner_capture(GscGameArrayOwner *,const GscRuntime *,char *,size_t);
int gsc_runtime_restore_game_array(GscRuntime *,const GscGameArrayOwner *,char *,size_t);
int gsc_advance(GscRuntime *runtime, int now_ms);   /* monotonic logical clock */
/* R166 host entry for the constructor's zero-argument ExecuteEntThread lane.
 * Resolves only a loaded script function in this linked runtime, never a
 * builtin, raw code pointer or native thread ID. Runs that task immediately
 * until return/wait/waittill, discarding its return Value. Ready children and
 * other tasks remain deferred to the scheduler. A callback may nest entries
 * under the current instruction budget (bounded depth32); ordinary scheduler
 * reentry is still fatal. Input is copied before host validation. On failure
 * output is unchanged. The existing native registry still rejects a second
 * dispatch inside its active callback; this entry service does not unlock it.
 * No completed actor/readiness/publication is implied.
 * Requires started state/lifetime/results/scalar/names profiles. Invalid or
 * unavailable requests are nonfatal before task creation; execution errors
 * remain sticky. Cancellation is explicit, never a successful constructor. */
typedef struct { GscEntityRef entity; char module[128], function[64]; } GscEntityScriptEntry;
enum { GSC_ENTITY_ENTRY_RETURNED=1, GSC_ENTITY_ENTRY_SLEEPING,
       GSC_ENTITY_ENTRY_WAITING, GSC_ENTITY_ENTRY_CANCELLED };
typedef struct { GscEntityRef entity; uint64_t task_serial; int status; } GscEntityScriptRun;
int gsc_execute_entity_entry(GscRuntime *,const GscEntityScriptEntry *,GscEntityScriptRun *);
/* R179 separately selected fresh-runtime resource bootstrap. Each named
 * zero-argument root has level self, runs immediately to its first boundary,
 * and owns a copied map/resource receipt guard. Root and child continuations
 * retain that guard; consumption is not a lifetime check. Model/item admission
 * is scoped to each synchronous invocation and closes on return/wait/error.
 * Children stay deferred. The ordinary gsc_start window is unchanged; this
 * service starts the runtime without executing its configured entry function.
 * No spawner hook, native code handle or complete postinit is supplied. */
#define GSC_RESOURCE_BOOTSTRAP_MAX 32
typedef struct { uint64_t receipt;int (*current)(uint64_t); } GscMapExecutionCertificate;
/* Selection by the synchronous GAME host. Each certificate is still checked
 * on entry and exit of one read-only receiver scan. */
typedef struct {
    GscMapExecutionCertificate map, animation, developer;
    /* Optional native Training frame and read-only Actor lease callbacks. */
    int (*frame_active)(void);
    int (*actor_current)(const GscEntityRef *,uint64_t);
} GscReadOnlyContext;
void gsc_runtime_bind_readonly_context(GscRuntime *, const GscReadOnlyContext *,
    void *, int (*validate)(void *, const GscEntityRef *));
/* Fixed map-global anim data STRUCT, separate from level/game/self. One
 * selected runtime owns its bag/arrays for one map incarnation. Whole owner
 * values, animation resources/poses, persistence and Session wiring held.
 * Only unused generic_human directive metadata is admitted; animation values
 * and references remain refused. Selection is before startup and one-shot. */
int gsc_program_select_anim_data(GscProgram *);
int gsc_runtime_select_anim_data(GscRuntime *,const GscMapExecutionCertificate *);
/* Resource-backed reference VALUES only. Catalog ordinals are portable symbol
 * identities, never native tree/entry indices or playback handles. No caller
 * catalog/source pointer survives selection; current receipts are map-bound. */
#define GSC_ANIMATION_CATALOG_CAPACITY 4096
#define GSC_ANIMATION_REFERENCE_CAPACITY 256
#define GSC_R233_ANIMATION_NAMESPACE_CAPACITY 8192
int gsc_program_select_r233_native_animation_roots(GscProgram *);
enum { GSC_ANIMATION_NODE_BRANCH=1,GSC_ANIMATION_NODE_COMPLETE=2,
       GSC_ANIMATION_NODE_LOOPSYNC=4,GSC_ANIMATION_NODE_NONLOOPSYNC=8,
       GSC_ANIMATION_NODE_IMPLICIT_ROOT=16 };
typedef struct GscAnimationCatalog GscAnimationCatalog;
typedef struct { uint32_t ordinal,parent,flags;size_t source_offset;char name[64]; } GscAnimationCatalogEntry;
GscAnimationCatalog *gsc_animation_catalog_parse(const char *,const void *,size_t,char *,size_t);
void gsc_animation_catalog_free(GscAnimationCatalog *);
int gsc_animation_catalog_storage_overlap(const GscAnimationCatalog*,const void*,size_t);
size_t gsc_animation_catalog_count(const GscAnimationCatalog *);
int gsc_animation_catalog_entry(const GscAnimationCatalog *,size_t,GscAnimationCatalogEntry *);
int gsc_program_select_animation_references(GscProgram *);
int gsc_runtime_select_animation_references(GscRuntime *,const GscAnimationCatalog *,const GscMapExecutionCertificate *);
/* Explicit declared metadata only; these are not native tree/node identities.
 * Select Program before compile. Runtime retains no catalog pointers. */
enum { GSC_ANIMATION_NAMESPACE_CATALOGS=3, GSC_MAP_ANIMATION_CATALOGS=8 };
typedef struct {unsigned count;char names[GSC_MAP_ANIMATION_CATALOGS][64];} GscMapAnimationNames;
typedef struct {const GscAnimationCatalog *catalogs[GSC_MAP_ANIMATION_CATALOGS];GscMapExecutionCertificate map_certificate;} GscMapAnimationNamespaceSelection;
int gsc_program_select_map_animation_namespace(GscProgram*,const GscMapAnimationNames*);
int gsc_runtime_select_map_animation_namespace(GscRuntime*,const GscMapAnimationNamespaceSelection*);
typedef struct {const GscAnimationCatalog *catalogs[GSC_ANIMATION_NAMESPACE_CATALOGS];GscMapExecutionCertificate map_certificate;} GscAnimationNamespaceSelection;
int gsc_program_select_animation_namespace(GscProgram *);
int gsc_runtime_select_animation_namespace(GscRuntime *,const GscAnimationNamespaceSelection *);
/* Optional exact Guy+gear resource query under copied namespace authority.
 * Metadata-only namespace selection never installs this capability. */
int gsc_program_select_namespace_start_origin_query(GscProgram *);
uint64_t gsc_runtime_instance_identity(const GscRuntime *);
typedef struct { char module[128],function[64];GscMapExecutionCertificate certificate; } GscResourceBootstrapEntry;
typedef struct { uint64_t task_serial;int status; } GscResourceBootstrapRun;
typedef struct { uint32_t attempted;GscResourceBootstrapRun runs[GSC_RESOURCE_BOOTSTRAP_MAX]; } GscResourceBootstrapResult;
uint64_t gsc_runtime_program_identity(const GscRuntime *);
int gsc_runtime_resource_bootstrap_available(const GscRuntime *);
int gsc_runtime_script_function_available(const GscRuntime *,const char *,const char *);
/* Fixed native implicit callee: actual six-parameter whole-function admission,
 * not merely existence. Read-only; does not execute/init or admit any method. */
int gsc_runtime_r243_scripted_init_available(const GscRuntime *);
int gsc_start_resource_hooks(GscRuntime *,const GscResourceBootstrapEntry *,size_t,
    GscResourceBootstrapResult *,char *,size_t);
/* R167 constructor-stage inheritance of this runtime's owned dynamic fields.
 * Distinct live source/destination; destination must have no dynamic records,
 * including undefined ones. Native/static class properties are not synthesized.
 * Entire Value records are copied: arrays/structs/functions retain the same
 * runtime identities and entity-valued fields remain opaque (possibly stale).
 * No deep clone, native pool IDs/refcounts or readiness is implied. Atomic
 * admitted-domain transfer; capacity/invalid/stale requests leave fields/count
 * unchanged. This fresh-destination policy does not emulate native merging,
 * same-object alias, allocation-failure rollback or native iteration order.
 * Requires a started linked state/array/struct/scalar/names profile. Input
 * identities are copied before validation; scheduler/entry/inheritance reentry
 * during validation poisons the runtime. Ordinary refusal is nonfatal. */
typedef struct { GscEntityRef source,destination; } GscEntityFieldInheritance;
int gsc_inherit_entity_fields(GscRuntime *,const GscEntityFieldInheritance *,size_t *copied);
/* R174 explicit, quiescent middle-stage sequence. The synchronous host first
 * validates the still-fresh destination immediately before the field commit,
 * then prepares native data/link. Its certificate is copied into the entry
 * task and descendants; current must be a pure, lifetime-stable function with
 * no borrowed user/request pointers. Zero retires only that task context.
 * No aitype lookup, static projection, targetname replacement or readiness.
 * Once arguments are admitted, out reports the committed prefix on refusal;
 * an early malformed/overlapping request leaves out unchanged. */
typedef struct {
    GscEntityRef owner;
    uint64_t serial;
    int (*current)(const GscEntityRef *,uint64_t);
} GscEntityExecutionCertificate;
/* Explicit pure lookup of an existing typed allocation authority; no bootstrap. */
typedef struct {void *user;int (*resolve)(void *,const GscEntityRef *,GscEntityExecutionCertificate *,char *,size_t);} GscThreadEntityAuthorityHost;
/* R180 opt-in mixed roots. Entity roots additionally retain a copied typed
 * allocation certificate. Global roots remain map/resource-owned. Initial
 * invocations are ordered as supplied; children stay deferred. */
#define GSC_ENTITY_HOOK_BOOTSTRAP_MAX 64
enum { GSC_HOOK_ROOT_GLOBAL=1,GSC_HOOK_ROOT_ENTITY=2 };
typedef struct {
    uint32_t kind;char module[128],function[64];
    GscMapExecutionCertificate map_certificate;
    GscEntityExecutionCertificate entity_certificate;
} GscEntityHookBootstrapEntry;
typedef struct { uint32_t attempted;GscResourceBootstrapRun runs[GSC_ENTITY_HOOK_BOOTSTRAP_MAX]; } GscEntityHookBootstrapResult;
typedef struct { void *user;int (*after_initial)(void *,size_t); } GscEntityHookBootstrapHost;
int gsc_start_entity_hooks(GscRuntime *,const GscEntityHookBootstrapEntry *,size_t,
    const GscEntityHookBootstrapHost *,GscEntityHookBootstrapResult *,char *,size_t);

typedef struct {
    GscEntityFieldInheritance inheritance;
    GscEntityScriptEntry entry;
} GscInheritedEntityEntry;
typedef struct {
    void *user;
    int (*validate_copy)(void *,const GscEntityFieldInheritance *);
    int (*prepare)(void *,const GscEntityFieldInheritance *,
                   GscEntityExecutionCertificate *,char *,size_t);
} GscInheritedEntityEntryHost;
enum { GSC_INHERITED_ENTRY_RAW=0,GSC_INHERITED_ENTRY_FIELDS,
       GSC_INHERITED_ENTRY_PREPARED,GSC_INHERITED_ENTRY_ENTERED };
typedef struct {
    int stage;
    size_t copied;
    GscEntityScriptRun entry;
} GscInheritedEntityEntryRun;
int gsc_execute_inherited_entity_entry(GscRuntime *,const GscInheritedEntityEntry *,
    const GscInheritedEntityEntryHost *,GscInheritedEntityEntryRun *,char *,size_t);
/* Separate R175 prefix capability; the old R174 host layout is unchanged.
 * before_copy runs after nonexecuting entry/profile admission and before any
 * inherited field copy, under the same host-callback mutation guard. It may
 * retain a committed host prefix on refusal. Callback arguments are copies.
 * The existing prepare callback remains AFTER inheritance. */
typedef struct {
    GscInheritedEntityEntryHost inherited;
    int (*before_copy)(void *,const GscEntityFieldInheritance *,char *,size_t);
} GscProjectedEntityEntryHost;
typedef struct { int projected; GscInheritedEntityEntryRun inherited; } GscProjectedEntityEntryRun;
int gsc_execute_projected_entity_entry(GscRuntime *,const GscInheritedEntityEntry *,
    const GscProjectedEntityEntryHost *,GscProjectedEntityEntryRun *,char *,size_t);
/* One prefix during the selected ActorSpawnHost.prepare invocation, for its
 * exact source and executing caller. Copies the new owner's certificate;
 * preserves the caller/budget and sleeping entry. No actor factory, postinit,
 * spawned notification or success ticket is provided. The old API remains
 * quiescent-only. Nested coordinated prefixes remain explicitly refused. */
int gsc_execute_spawn_init(GscRuntime *,const GscEntityRef *,const GscEntityRef *,uint64_t,GscEntityScriptRun *,char *,size_t);
uint64_t gsc_actor_spawn_prepare_program(const GscRuntime *,const GscActorSpawnRequest *);
uint64_t gsc_actor_spawn_commit_program(const GscRuntime *,const GscActorSpawnTicket *);
uint64_t gsc_actor_spawn_finish_program(const GscRuntime *,const GscActorSpawnTicket *,int published);
uint64_t gsc_projected_spawn_entry_program(const GscRuntime *,const GscEntityRef *);
uint64_t gsc_spawn_issued_owner_program(const GscRuntime *,const GscEntityRef *,const GscEntityRef *,uint64_t);
int gsc_execute_projected_spawn_entry(GscRuntime *,const GscInheritedEntityEntry *,
    const GscProjectedEntityEntryHost *,GscProjectedEntityEntryRun *,char *,size_t);
/* R177 explicit same-allocation startup. Host-declared plain string values
 * enter a fresh private bag as one batch, after entry/profile admission and
 * before native preparation. No textual type inference or script evaluation.
 * commit_fields is the final synchronous precommit guard; after it succeeds
 * the VM performs only callback-free field writes. prepare returns the usual
 * copied persistent component certificate. Earlier entry APIs are unchanged. */
#define GSC_ENTITY_INITIAL_FIELDS_MAX 64
typedef struct { char name[128],string[128]; } GscEntityInitialStringField;
typedef struct {
    GscEntityScriptEntry entry;
    uint32_t field_count;
    GscEntityInitialStringField fields[GSC_ENTITY_INITIAL_FIELDS_MAX];
} GscPreparedEntityEntry;
typedef struct {
    void *user;
    int (*commit_fields)(void *,const GscEntityScriptEntry *,char *,size_t);
    int (*prepare)(void *,const GscEntityScriptEntry *,GscEntityExecutionCertificate *,char *,size_t);
} GscPreparedEntityEntryHost;
enum { GSC_PREPARED_ENTRY_RAW=0,GSC_PREPARED_ENTRY_FIELDS,
       GSC_PREPARED_ENTRY_PREPARED,GSC_PREPARED_ENTRY_ENTERED };
typedef struct { int stage;size_t fields;GscEntityScriptRun entry; } GscPreparedEntityEntryRun;
int gsc_execute_prepared_entity_entry(GscRuntime *,const GscPreparedEntityEntry *,
    const GscPreparedEntityEntryHost *,GscPreparedEntityEntryRun *,char *,size_t);
/* Pure external-host argument check; exposes no runtime storage or readiness. */
int gsc_runtime_storage_separate(const GscRuntime *,const void *,size_t);
int gsc_notify(GscRuntime *runtime, const char *name); /* event on level */
/* No event buffer: only current matching entity waiters become READY, and
 * execution is deferred until advance. Returns 0 for stale/invalid requests;
 * reentrant calls poison the runtime. out_awakened is required and cleared
 * before validation and counts only new EVENT-to-READY transitions. Match uses
 * full epoch/slot/generation + exact name, separate from level events. Pending
 * owner identity is checked on advance and just before continuation; retirement
 * cancels only that waiter task. External invalid/stale requests are nonfatal. */
int gsc_notify_entity(GscRuntime *runtime, const GscEntityRef *owner,
                      const char *event, int *out_awakened);
const char *gsc_runtime_error(const GscRuntime *runtime);
int gsc_active_threads(const GscRuntime *runtime);
/* Pure existence query; the exact count API remains unchanged. */
int gsc_has_active_threads(const GscRuntime *runtime);

/* R124 independently selected playfxOnTag request boundary. Exactly three
 * arguments; GetInt0/range precede entity/model query, then bounded GetString2,
 * quote validation and bone query. A publisher owns the original map-local
 * exact %02d%s dictionary and entity-attached event. A successful callback
 * acknowledges publication only, never client pose evaluation/rendering.
 * Printable ASCII tags up to 127 bytes, finite GetString coercions and bone
 * indices -1..127 are selected port bounds. Old public layouts remain intact.
 * model_identity is a provider cookie: the actual provider must revalidate
 * entity/model/tag at publication. Separate callbacks are not atomic.
 * Native argument evaluation remains LTR; undefined is selected result policy.
 * Receiver/thread/function-reference native forms remain unsupported. */
enum { GSC_TAG_FX_PLAY=1, GSC_TAG_FX_MODEL=1, GSC_TAG_FX_BONE=2,
       GSC_TAG_FX_TAG_CAPACITY=128, GSC_TAG_FX_TOKEN_CAPACITY=130,
       GSC_TAG_FX_ENTITY_LIMIT=2048, GSC_TAG_FX_BONE_LIMIT=128 };
typedef struct { int phase; uint64_t operation_id; GscEntityRef entity;
    int32_t index; char tag[GSC_TAG_FX_TAG_CAPACITY];
    int model_present; uint64_t model_identity; int bone_index;
    char model_name[128]; } GscTagFxQuery;
typedef struct { uint64_t operation_id; GscEntityRef entity; int32_t index;
    uint64_t model_identity; char tag[GSC_TAG_FX_TAG_CAPACITY];
    char token[GSC_TAG_FX_TOKEN_CAPACITY]; } GscTagFxEvent;
typedef int (*GscTagFxQueryCallback)(void *, int, GscTagFxQuery *, char *, size_t);
typedef int (*GscTagFxCallback)(void *, int, const GscTagFxEvent *, char *, size_t);
typedef struct { void *user; GscTagFxQueryCallback query;
    GscTagFxCallback publish; } GscTagFxHost;
enum { GSC_LINK_TAG_FX_NATIVE=GSC_LINK_SAVE_GAME_NATIVE+1 };
enum { GSC_EXPR_TARGET_TAG_FX_NATIVE=GSC_EXPR_TARGET_SAVE_GAME_NATIVE+1 };
GscProgram *gsc_program_create_with_tag_fx(void);
int gsc_program_link_with_tag_fx(GscProgram *, const char *, char *, size_t);
GscRuntime *gsc_runtime_new_with_tag_fx(const GscProgram *, GscFogCallback,
    void *user, const GscEntityHost *, const GscEntityAccessHost *,
    const GscEntityControlHost *, const GscEntityCollectionHost *,
    const GscEntityPredicateHost *, const GscActorSpawnHost *, const GscMusicHost *,
    const GscResourceHost *, const GscObjectiveHost *, const GscConsoleHost *,
    const GscHudHost *, const GscMissionHost *, const GscBlastHost *, const GscQuakeHost *, const GscFxHost *, const GscCvarHost *, const GscShellshockHost *, const GscDebugTextHost *, const GscDebugLineHost *, const GscAmbientHost *, const GscPrecacheItemHost *, const GscSaveGameHost *, const GscTagFxHost *, char *, size_t);
/* R128 optional source-backed getter superset. Existing modes and public
 * layouts remain frozen. getCvar/GetCvarInt/GetCvarFloat use ordinary arg0,
 * evaluate then ignore extras, and admit bounded/finite replies. Entity method
 * getEntityNumber/GetEntNum return the validated captured slot; no DObj pose is inferred.
 * Missing getters fail at use. The port string result domain is <=127 bytes,
 * narrower than the original getCvar buffer of 1024. precacheString is separately selected by the appended R129 constructor. */
enum { GSC_QUERY_CVAR_STRING=1, GSC_QUERY_CVAR_INTEGER, GSC_QUERY_CVAR_FLOAT };
typedef struct { uint64_t operation_id; int kind; char name[128];
    int32_t integer_value; float float_value; char string_value[128]; } GscNativeQuery;
typedef int (*GscNativeQueryCallback)(void *, int, GscNativeQuery *, char *, size_t);
typedef struct { void *user; GscNativeQueryCallback query; } GscNativeQueryHost;
enum { GSC_LINK_NATIVE_QUERY=GSC_LINK_TAG_FX_NATIVE+1 };
enum { GSC_EXPR_TARGET_NATIVE_QUERY=GSC_EXPR_TARGET_TAG_FX_NATIVE+1 };
GscProgram *gsc_program_create_with_native_queries(void);
int gsc_program_link_with_native_queries(GscProgram *, const char *, char *, size_t);
GscRuntime *gsc_runtime_new_with_native_queries(const GscProgram *, GscFogCallback,
    void *user, const GscEntityHost *, const GscEntityAccessHost *,
    const GscEntityControlHost *, const GscEntityCollectionHost *,
    const GscEntityPredicateHost *, const GscActorSpawnHost *, const GscMusicHost *,
    const GscResourceHost *, const GscObjectiveHost *, const GscConsoleHost *,
    const GscHudHost *, const GscMissionHost *, const GscBlastHost *, const GscQuakeHost *, const GscFxHost *, const GscCvarHost *, const GscShellshockHost *, const GscDebugTextHost *, const GscDebugLineHost *, const GscAmbientHost *, const GscPrecacheItemHost *, const GscSaveGameHost *, const GscTagFxHost *, const GscNativeQueryHost *, char *, size_t);

/* R129 optional startup string publication. No private pool or implicit
 * capability is provided. startup is a pure predicate and must return exactly
 * one for the current owner/clock/startup lease. Public configstring pool
 * publication belongs to publish. Ordinary/localized printable ASCII strings
 * <=127 bytes excluding quote, percent and backslash form the bounded domain.
 * Successful wait/waittill in any task permanently closes this initial window;
 * the unchanged deferred scheduler may admit fewer child prefixes than retail. */
enum { GSC_QUERY_PRECACHE_STRING=GSC_QUERY_CVAR_FLOAT+1 };
/* R130 highest-constructor query: retail GetTime reads the current GAME time
 * DWORD and AddInt returns its signed32 value. No runtime-elapsed clock or
 * synthetic fallback is used. Arg expressions evaluate then are ignored.
 * Existing query ids/layout and older constructor capability are retained. */
enum { GSC_QUERY_LEVEL_TIME=GSC_QUERY_PRECACHE_STRING+1 };
/* R148 selected cached GAME skill name; older query ids and payload unchanged. */
enum { GSC_QUERY_DIFFICULTY=GSC_QUERY_LEVEL_TIME+1 };
typedef struct { uint64_t operation_id; char name[128]; } GscPrecacheStringEvent;
typedef int (*GscPrecacheStringStartupCallback)(void *,int);
typedef int (*GscPrecacheStringCallback)(void *,int,const GscPrecacheStringEvent *,char *,size_t);
typedef struct { void *user; GscPrecacheStringStartupCallback startup;
    GscPrecacheStringCallback publish; } GscPrecacheStringHost;
GscProgram *gsc_program_create_with_precache_string(void);
int gsc_program_link_with_precache_string(GscProgram *,const char *,char *,size_t);
int gsc_runtime_precache_string_startup(const GscRuntime *,int);
GscRuntime *gsc_runtime_new_with_precache_string(const GscProgram *, GscFogCallback,
    void *user, const GscEntityHost *, const GscEntityAccessHost *,
    const GscEntityControlHost *, const GscEntityCollectionHost *,
    const GscEntityPredicateHost *, const GscActorSpawnHost *, const GscMusicHost *,
    const GscResourceHost *, const GscObjectiveHost *, const GscConsoleHost *,
    const GscHudHost *, const GscMissionHost *, const GscBlastHost *, const GscQuakeHost *, const GscFxHost *, const GscCvarHost *, const GscShellshockHost *, const GscDebugTextHost *, const GscDebugLineHost *, const GscAmbientHost *, const GscPrecacheItemHost *, const GscSaveGameHost *, const GscTagFxHost *, const GscNativeQueryHost *, const GscPrecacheStringHost *, char *, size_t);

/* R131 separately selected complete spawner collections. The retail wrapper
 * requires zero arguments and scans active native type12 records in table-index
 * order, passing each record's entity-slot word. This port selects ascending
 * slots for genuine typed BSP spawner intake and copies full epoch/slot/generation
 * identities, never original engine array handles. Record-word/index equality
 * and full retail collection-order parity are not claimed by this stage.
 * Empty success is a defined array; over 256 members fails without truncation.
 * That bound and the existing numeric keys 0..255 are explicit VM resource
 * policy. collect_all copies complete identities and retains no pointers. */
enum { GSC_SPAWNER_ARRAY_ALL=1 };
typedef int (*GscSpawnerArrayCallback)(void *,int logical_ms,GscEntityRef *,
    size_t capacity,size_t *count,char *,size_t);
typedef struct { void *user; GscSpawnerArrayCallback collect_all; } GscSpawnerArrayHost;
enum { GSC_LINK_SPAWNER_ARRAY_NATIVE=GSC_LINK_NATIVE_QUERY+1 };
enum { GSC_EXPR_TARGET_SPAWNER_ARRAY_NATIVE=GSC_EXPR_TARGET_NATIVE_QUERY+1 };
GscProgram *gsc_program_create_with_spawner_arrays(void);
int gsc_program_link_with_spawner_arrays(GscProgram *,const char *,char *,size_t);
GscRuntime *gsc_runtime_new_with_spawner_arrays(const GscProgram *, GscFogCallback,
    void *user, const GscEntityHost *, const GscEntityAccessHost *,
    const GscEntityControlHost *, const GscEntityCollectionHost *,
    const GscEntityPredicateHost *, const GscActorSpawnHost *, const GscMusicHost *,
    const GscResourceHost *, const GscObjectiveHost *, const GscConsoleHost *,
    const GscHudHost *, const GscMissionHost *, const GscBlastHost *, const GscQuakeHost *, const GscFxHost *, const GscCvarHost *, const GscShellshockHost *, const GscDebugTextHost *, const GscDebugLineHost *, const GscAmbientHost *, const GscPrecacheItemHost *, const GscSaveGameHost *, const GscTagFxHost *, const GscNativeQueryHost *, const GscPrecacheStringHost *, const GscSpawnerArrayHost *, char *, size_t);

/* R133 separately selected literal-int32 randomint subset. Only int32 literal
 * or unary-minus integer-literal call sites bind; dynamic/float/string/.size and
 * extra-argument sites remain unresolved. argc0 binds its missing-getter error.
 * This is not a general GetInt/FPU/locale contract or the original native ABI.
 * Requests carry exact signed32 bounds and a sentinel draw. The GAME callback
 * advances the shared map owner once, then returns a 15bit draw. Results use
 * the original low32 IMUL/SAR15 and retain their exact integer type. Effects
 * committed by a callback are never rolled back on a later wrapper refusal. */
enum { GSC_RANDOMINT_DRAW=1 };
typedef struct { uint64_t operation_id; int32_t bound; uint32_t draw15; } GscRandomIntRequest;
typedef int (*GscRandomIntDrawCallback)(void *,int,GscRandomIntRequest *,char *,size_t);
typedef struct { void *user; GscRandomIntDrawCallback draw; } GscRandomIntHost;
enum { GSC_LINK_RANDOMINT_NATIVE=GSC_LINK_SPAWNER_ARRAY_NATIVE+1 };
enum { GSC_EXPR_TARGET_RANDOMINT_NATIVE=GSC_EXPR_TARGET_SPAWNER_ARRAY_NATIVE+1 };
GscProgram *gsc_program_create_with_randomint(void);
int gsc_program_link_with_randomint(GscProgram *,const char *,char *,size_t);
GscRuntime *gsc_runtime_new_with_randomint(const GscProgram *, GscFogCallback,
    void *user, const GscEntityHost *, const GscEntityAccessHost *,
    const GscEntityControlHost *, const GscEntityCollectionHost *,
    const GscEntityPredicateHost *, const GscActorSpawnHost *, const GscMusicHost *,
    const GscResourceHost *, const GscObjectiveHost *, const GscConsoleHost *,
    const GscHudHost *, const GscMissionHost *, const GscBlastHost *, const GscQuakeHost *, const GscFxHost *, const GscCvarHost *, const GscShellshockHost *, const GscDebugTextHost *, const GscDebugLineHost *, const GscAmbientHost *, const GscPrecacheItemHost *, const GscSaveGameHost *, const GscTagFxHost *, const GscNativeQueryHost *, const GscPrecacheStringHost *, const GscSpawnerArrayHost *, const GscRandomIntHost *, char *, size_t);

/* R134 separately selected exact-dyadic randomfloat literal capability.
 * E_NUMBER or one unary-minus E_NUMBER, with an exact decimal-token proof;
 * dynamic/computed/string/rounded decimal/extra arguments remain unresolved.
 * argc0 binds the logical missing GetFloat error. Numeric bounds include zero
 * and negative values. Nonzero payloads are finite normal binary32 with unbiased
 * exponent >=-111 and normalized odd significand <=511. Integer GetFloat must
 * be exact. No generic FPU/decimal/heap/unwind/retail-ABI parity is claimed.
 * Request bound_bits/operation_id stay immutable; draw15 replaces UINT32_MAX.
 * The result is float-tagged; a consumed GAME draw survives later refusal. */
enum { GSC_RANDOMFLOAT_DRAW=1 };
typedef struct { uint64_t operation_id; uint32_t bound_bits, draw15; } GscRandomFloatRequest;
typedef int (*GscRandomFloatDrawCallback)(void *,int,GscRandomFloatRequest *,char *,size_t);
typedef struct { void *user; GscRandomFloatDrawCallback draw; } GscRandomFloatHost;
enum { GSC_LINK_RANDOMFLOAT_NATIVE=GSC_LINK_RANDOMINT_NATIVE+1 };
enum { GSC_EXPR_TARGET_RANDOMFLOAT_NATIVE=GSC_EXPR_TARGET_RANDOMINT_NATIVE+1 };
GscProgram *gsc_program_create_with_randomfloat(void);
int gsc_program_link_with_randomfloat(GscProgram *,const char *,char *,size_t);
GscRuntime *gsc_runtime_new_with_randomfloat(const GscProgram *, GscFogCallback,
    void *user, const GscEntityHost *, const GscEntityAccessHost *,
    const GscEntityControlHost *, const GscEntityCollectionHost *,
    const GscEntityPredicateHost *, const GscActorSpawnHost *, const GscMusicHost *,
    const GscResourceHost *, const GscObjectiveHost *, const GscConsoleHost *,
    const GscHudHost *, const GscMissionHost *, const GscBlastHost *, const GscQuakeHost *, const GscFxHost *, const GscCvarHost *, const GscShellshockHost *, const GscDebugTextHost *, const GscDebugLineHost *, const GscAmbientHost *, const GscPrecacheItemHost *, const GscSaveGameHost *, const GscTagFxHost *, const GscNativeQueryHost *, const GscPrecacheStringHost *, const GscSpawnerArrayHost *, const GscRandomIntHost *, const GscRandomFloatHost *, char *, size_t);

/* R135 selected plain ASCII getkeybinding literals with private copied array. */
enum { GSC_KEYBINDING_QUERY=1 };
typedef cod1KeyBindingQuery_t GscKeyBindingRequest;
typedef int (*GscKeyBindingQueryCallback)(void *,int,GscKeyBindingRequest *,char *,size_t);
typedef struct { void *user; GscKeyBindingQueryCallback query; } GscKeyBindingHost;
enum { GSC_LINK_KEYBINDING_NATIVE=GSC_LINK_RANDOMFLOAT_NATIVE+1 };
enum { GSC_EXPR_TARGET_KEYBINDING_NATIVE=GSC_EXPR_TARGET_RANDOMFLOAT_NATIVE+1 };
GscProgram *gsc_program_create_with_keybinding(void);
int gsc_program_link_with_keybinding(GscProgram *,const char *,char *,size_t);
GscRuntime *gsc_runtime_new_with_keybinding(const GscProgram *, GscFogCallback,
    void *user, const GscEntityHost *, const GscEntityAccessHost *,
    const GscEntityControlHost *, const GscEntityCollectionHost *,
    const GscEntityPredicateHost *, const GscActorSpawnHost *, const GscMusicHost *,
    const GscResourceHost *, const GscObjectiveHost *, const GscConsoleHost *,
    const GscHudHost *, const GscMissionHost *, const GscBlastHost *, const GscQuakeHost *, const GscFxHost *, const GscCvarHost *, const GscShellshockHost *, const GscDebugTextHost *, const GscDebugLineHost *, const GscAmbientHost *, const GscPrecacheItemHost *, const GscSaveGameHost *, const GscTagFxHost *, const GscNativeQueryHost *, const GscPrecacheStringHost *, const GscSpawnerArrayHost *, const GscRandomIntHost *, const GscRandomFloatHost *, const GscKeyBindingHost *, char *, size_t);

/* R136 opt-in exact computed integer upper bounds. Only proven local
 * reaching definitions, safe integer +/- and fresh dense local-array .size
 * acquire this binding. Evaluation and the shared typed GAME draw are retained.
 * Parameters, globals, ambiguous writes, aliases, yields and unsupported
 * control-flow/float/string origins remain unresolved. */
GscProgram *gsc_program_create_with_computed_randomint(void);
int gsc_program_link_with_computed_randomint(GscProgram *,const char *,char *,size_t);
GscRuntime *gsc_runtime_new_with_computed_randomint(const GscProgram *, GscFogCallback,
    void *user, const GscEntityHost *, const GscEntityAccessHost *,
    const GscEntityControlHost *, const GscEntityCollectionHost *,
    const GscEntityPredicateHost *, const GscActorSpawnHost *, const GscMusicHost *,
    const GscResourceHost *, const GscObjectiveHost *, const GscConsoleHost *,
    const GscHudHost *, const GscMissionHost *, const GscBlastHost *, const GscQuakeHost *, const GscFxHost *, const GscCvarHost *, const GscShellshockHost *, const GscDebugTextHost *, const GscDebugLineHost *, const GscAmbientHost *, const GscPrecacheItemHost *, const GscSaveGameHost *, const GscTagFxHost *, const GscNativeQueryHost *, const GscPrecacheStringHost *, const GscSpawnerArrayHost *, const GscRandomIntHost *, const GscRandomFloatHost *, const GscKeyBindingHost *, char *, size_t);

/* R148 runtime-owned script structures plus cached difficulty query. */
#define GSC_STRUCT_CAPACITY 256
enum { GSC_STRUCT_SPAWN=1 };
enum { GSC_LINK_STRUCT_NATIVE=GSC_LINK_KEYBINDING_NATIVE+1 };
enum { GSC_EXPR_TARGET_STRUCT_NATIVE=GSC_EXPR_TARGET_KEYBINDING_NATIVE+1 };
GscProgram *gsc_program_create_with_structs(void);
int gsc_program_link_with_structs(GscProgram *,const char *,char *,size_t);
GscRuntime *gsc_runtime_new_with_structs(const GscProgram *, GscFogCallback,
    void *user, const GscEntityHost *, const GscEntityAccessHost *,
    const GscEntityControlHost *, const GscEntityCollectionHost *,
    const GscEntityPredicateHost *, const GscActorSpawnHost *, const GscMusicHost *,
    const GscResourceHost *, const GscObjectiveHost *, const GscConsoleHost *,
    const GscHudHost *, const GscMissionHost *, const GscBlastHost *, const GscQuakeHost *, const GscFxHost *, const GscCvarHost *, const GscShellshockHost *, const GscDebugTextHost *, const GscDebugLineHost *, const GscAmbientHost *, const GscPrecacheItemHost *, const GscSaveGameHost *, const GscTagFxHost *, const GscNativeQueryHost *, const GscPrecacheStringHost *, const GscSpawnerArrayHost *, const GscRandomIntHost *, const GscRandomFloatHost *, const GscKeyBindingHost *, char *, size_t);

/* R149 selected number lookup and complete named-team spawner collections.
 * Earlier host layouts and selectors are retained. Copies are private GSC
 * restoration contracts, never engine import or snapshot ABI. */
enum { GSC_ENTITY_QUERY_BY_NUMBER=1, GSC_ENTITY_QUERY_SPAWNER_TEAM=2 };
typedef int (*GscEntityNumberCallback)(void *,int,int32_t,GscEntityRef *,int *,char *,size_t);
typedef int (*GscSpawnerTeamArrayCallback)(void *,int,unsigned int,GscEntityRef *,size_t,size_t *,char *,size_t);
typedef struct { void *user; GscEntityNumberCallback by_number; GscSpawnerTeamArrayCallback collect_team; } GscEntityQueryHost;
enum { GSC_LINK_ENTITY_QUERY_NATIVE=GSC_LINK_STRUCT_NATIVE+1 };
enum { GSC_EXPR_TARGET_ENTITY_QUERY_NATIVE=GSC_EXPR_TARGET_STRUCT_NATIVE+1 };
/* Explicit plain-string getWeaponModel lane. Results are copied into VM-owned
 * text; input conversion, absent native model pointers and Session wiring held.
 * query must check the synchronous current(context) between external callbacks
 * and before publishing output; neither callback nor context may be retained. */
enum { GSC_EXPR_TARGET_WEAPON_MODEL_NATIVE=GSC_EXPR_TARGET_ENTITY_QUERY_NATIVE+1 };
typedef struct { void *user; int (*query)(void *,const char *,char [128],char *,size_t,int (*)(void *),void *); } GscWeaponModelHost;
/* Append-only selected loaded-weapon classname query, string or undefined. */
/* Typed global model resource query; result copied into VM-owned text. */
/* Native spawnVehicle has five getters before allocation. The typed factory
 * owns construction/rollback; it may not run script or synthesize events.
 * A nonzero operation must be returned even on failure after reservation.
 * finish(0) releases exactly that operation and remains valid after the caller
 * lease expires. finish(1) requires current(); cancellation after failed
 * publication must also release an already committed matching allocation. */
enum {GSC_EXPR_TARGET_TURRET_PRECACHE_NATIVE=1006};
typedef struct {char name[128];uint64_t operation,program;int now_ms;} GscTurretPrecacheRequest;
typedef struct {void*user;int(*invoke)(void*,const GscTurretPrecacheRequest*,char*,size_t,int(*)(void*),void*);} GscTurretPrecacheHost;
int gsc_program_select_turret_precache(GscProgram*);
int gsc_runtime_select_turret_precache(GscRuntime*,const GscTurretPrecacheHost*);
enum {GSC_EXPR_TARGET_VEHICLE_SPAWN_NATIVE=1003};
typedef struct {char model[128],targetname[128],definition[128];float origin[3],angles[3];int now_ms;uint64_t program;} GscVehicleSpawnRequest;
typedef struct {uint64_t operation,program;GscEntityRef entity;} GscVehicleSpawnTicket;
typedef struct {void*user;
 int(*prepare)(void*,const GscVehicleSpawnRequest*,GscVehicleSpawnTicket*,char*,size_t,int(*)(void*),void*);
 int(*finish)(void*,const GscVehicleSpawnTicket*,int,char*,size_t,int(*)(void*),void*);
} GscVehicleSpawnHost;
int gsc_program_select_vehicle_spawn(GscProgram*);
int gsc_runtime_select_vehicle_spawn(GscRuntime*,const GscVehicleSpawnHost*);
enum {GSC_EXPR_TARGET_TURRET_SPAWN_NATIVE=1005};
typedef struct {char classname[128],weapon[128];float origin[3];int now_ms;uint64_t program,precache_operation;} GscTurretSpawnRequest;
typedef struct {uint64_t operation,program;GscEntityRef entity;} GscTurretSpawnTicket;
typedef struct {void*user;
 int(*prepare)(void*,const GscTurretSpawnRequest*,GscTurretSpawnTicket*,char*,size_t,int(*)(void*),void*);
 int(*finish)(void*,const GscTurretSpawnTicket*,int,char*,size_t,int(*)(void*),void*);
} GscTurretSpawnHost;
int gsc_program_select_turret_spawn(GscProgram*);
int gsc_runtime_select_turret_spawn(GscRuntime*,const GscTurretSpawnHost*);
enum {GSC_EXPR_TARGET_MODEL_PART_NATIVE=1002};
typedef struct {char model[128];int32_t index;int index_valid,now_ms;} GscModelPartRequest;
typedef struct {void*user;int(*query)(void*,const GscModelPartRequest*,char[128],char*,size_t,int(*)(void*),void*);} GscModelPartHost;
int gsc_program_select_model_part(GscProgram*);
int gsc_runtime_select_model_part(GscRuntime*,const GscModelPartHost*);
enum { GSC_EXPR_TARGET_WEAPON_CLASSNAME_NATIVE=1001 };
typedef struct {int defined;char classname[128];} GscWeaponClassnameResult;
typedef struct {void*user;int(*query)(void*,const char*,int,GscWeaponClassnameResult*,char*,size_t,int(*)(void*),void*);} GscWeaponClassnameHost;
int gsc_program_select_weapon_classname(GscProgram*);
int gsc_runtime_select_weapon_classname(GscRuntime*,const GscWeaponClassnameHost*);
int gsc_program_select_weapon_model_query(GscProgram *);
int gsc_runtime_select_weapon_model(GscRuntime *,const GscWeaponModelHost *);
/* Explicit resource-backed getanimlength. No playback/tree ordinal parity.
 * The provider returns validated clip metadata, never a host-computed float.
 * Callback/context are synchronous and must not be retained. */
enum { GSC_EXPR_TARGET_ANIMATION_LENGTH_NATIVE=GSC_EXPR_TARGET_WEAPON_MODEL_NATIVE+1 };
typedef struct { char tree[64],name[64];uint32_t ordinal,flags;uint64_t runtime_identity; } GscAnimationLengthRequest;
typedef struct { uint32_t frames,fps; } GscAnimationLengthReply;
typedef struct { void *user;int (*query)(void *,const GscAnimationLengthRequest *,GscAnimationLengthReply *,char *,size_t,int (*)(void *),void *); } GscAnimationLengthHost;
int gsc_program_select_animation_length_query(GscProgram *);
int gsc_program_select_r233_namespace_animation_length(GscProgram *);
int gsc_program_select_map_animation_length(GscProgram*);
int gsc_runtime_select_map_animation_length(GscRuntime*,const GscAnimationLengthHost*);
/* Read-only copied requirements from an idle prepared namespace:1 row,0 end,-1 invalid. */
int gsc_runtime_map_animation_metadata_request(const GscRuntime*,unsigned,GscAnimationLengthRequest*);
int gsc_runtime_select_r233_namespace_animation_length(GscRuntime *,const GscAnimationLengthHost *);
int gsc_runtime_select_animation_length(GscRuntime *,const GscAnimationLengthHost *);
/* Separately selected native animation goal commands. Resource descriptors
 * are copied portable names/ordinals; no native packed indices are exported.
 * validate/invoke must check borrowed current(context) between callbacks. */
enum { GSC_ANIMATION_SET_KNOB=1,GSC_ANIMATION_SET_KNOB_ALL,GSC_ANIMATION_SET_FLAGGED_KNOB_ALL };
typedef struct { GscEntityRef entity;int32_t now_ms;uint32_t method,precision_bits,rounding;
    GscAnimationLengthRequest animation,root;char notify[128];uint32_t weight_bits,blend_bits,rate_bits; } GscAnimationMethodRequest;
typedef struct { void *user;
    int (*validate)(void *,const GscEntityRef *,uint64_t,int32_t,char *,size_t,int (*)(void *),void *);
    int (*invoke)(void *,const GscAnimationMethodRequest *,char *,size_t,int (*)(void *),void *); } GscAnimationMethodHost;
/* R232 separate two-catalog actor/turret commands. Catalog identity is opaque
 * owned Runtime metadata, never an original packed animation/tree index.
 * Commands assign real provider state; they synthesize no completion/events. */
enum {GSC_ANIMATION_SET_TURRET=4,GSC_ANIMATION_SET_KNOB_RESTART,
      GSC_ANIMATION_USE_TREE,GSC_ANIMATION_STOP_USE_TREE};
typedef struct {
    GscEntityRef entity;int32_t now_ms;uint32_t method,precision_bits,rounding;
    GscAnimationLengthRequest animation;char tree[64];uint64_t catalog_identity;
    uint32_t weight_bits,blend_bits,rate_bits;
} GscTurretAnimationRequest;
typedef struct {void *user;
    int (*validate)(void *,const GscEntityRef *,uint64_t,int32_t,char *,size_t,int (*)(void *),void *);
    int (*invoke)(void *,const GscTurretAnimationRequest *,char *,size_t,int (*)(void *),void *);
} GscTurretAnimationHost;
typedef struct {const GscAnimationCatalog *catalogs[2];
    GscMapExecutionCertificate map_certificate;GscTurretAnimationHost host;
} GscTurretAnimationSelection;
enum {GSC_ACTOR_ANIMATION_CLEAR=1,GSC_ACTOR_ANIMATION_SET,GSC_ACTOR_ANIMATION_PRONE_NODES,
      GSC_ACTOR_ANIMATION_ENTER_PRONE,GSC_ACTOR_ANIMATION_EXIT_PRONE,GSC_ACTOR_ANIMATION_FLAGGED_RESTART,GSC_ACTOR_ANIMATION_KNOB_ALL,GSC_ACTOR_ANIMATION_ORIENT_MODE,GSC_ACTOR_ANIMATION_UPDATE_PRONE,GSC_ACTOR_ANIMATION_ANIM_MODE,GSC_ACTOR_ANIMATION_FLAGGED_ALL_RESTART,GSC_ACTOR_ANIMATION_FLAGGED_ALL,
      /* Internal failed-GetFloat prefix acknowledgment; never a GSC method. */
      GSC_ACTOR_ANIMATION_ORIENT_ERROR_PREFIX,GSC_ACTOR_ANIMATION_KNOB,GSC_ACTOR_INTERRUPT_POINT,GSC_ACTOR_ANIMATION_FLAGGED_KNOB,GSC_ACTOR_ANIMATION_KNOB_ALL_RESTART,GSC_ACTOR_ANIMATION_AIM_NODES};
typedef struct {GscEntityRef entity;int32_t now_ms;uint32_t method,precision_bits,rounding;
    uint64_t runtime_identity,catalog_identity;GscAnimationLengthRequest animation[6];
    uint32_t scalar_bits[3];char notify[64];} GscActorAnimationRequest;
typedef struct {void *user;
    int (*validate)(void *,const GscEntityRef *,uint64_t,int32_t,char *,size_t,int (*)(void *),void *);
    int (*invoke)(void *,const GscActorAnimationRequest *,char *,size_t,int (*)(void *),void *);
} GscActorAnimationHost;
int gsc_program_select_map_actor_animations(GscProgram*);
int gsc_runtime_select_map_actor_animations(GscRuntime*,const GscActorAnimationHost*);
int gsc_program_select_r232_actor_animations(GscProgram *);
/* Explicit whole-function certificates with preserved deferred callback bodies. */
enum {GSC_ACTOR_LOOK_NODES=1,GSC_ACTOR_LOOK_TARGET,GSC_ACTOR_LOOK_YAW_LIMITS,GSC_ACTOR_LOOK_STOP,GSC_ACTOR_LOOK_TAG_ANGLES,GSC_ACTOR_LOOK_CAN_SEE,GSC_ACTOR_LOOK_TAG_ORIGIN,GSC_ACTOR_LOOK_CAN_SHOOT,GSC_ACTOR_LOOK_SHOOT,GSC_ACTOR_LOOK_AIM_AT_POS,GSC_ACTOR_LOOK_MAY_MOVE,GSC_ACTOR_LOOK_CHECK_PRONE,GSC_ACTOR_TRAVERSE_MODE,GSC_ACTOR_NEGOTIATION_END};
enum {GSC_ACTOR_LOOK_RESULT_VOID=1,GSC_ACTOR_LOOK_RESULT_BOOL,GSC_ACTOR_LOOK_RESULT_VECTOR,GSC_ACTOR_LOOK_RESULT_FLOAT};
typedef struct {GscEntityRef entity,argument;int32_t now_ms;uint32_t method,precision_bits,rounding;uint64_t runtime_identity,catalog_identity;GscAnimationLengthRequest animation[3];uint32_t vector_bits[3],scalar_bits[3];char tag[64];} GscActorLookRequest;
typedef struct {uint32_t kind;int boolean;uint32_t vector_bits[3];} GscActorLookReply;
typedef struct {void *user;int(*validate)(void*,const GscEntityRef*,uint64_t,int32_t,char*,size_t,int(*)(void*),void*);int(*invoke)(void*,const GscActorLookRequest*,GscActorLookReply*,char*,size_t,int(*)(void*),void*);} GscActorLookHost;
int gsc_program_select_map_actor_look(GscProgram*);
int gsc_runtime_select_map_actor_look(GscRuntime*,const GscActorLookHost*);
int gsc_program_select_r232_actor_look(GscProgram *);
int gsc_runtime_select_r232_actor_look(GscRuntime *,const GscActorLookHost *);
int gsc_program_select_r232_function_admission(GscProgram *);
/* Captured actual developer_script cvar0/1. Certificate must attest the exact
 * setting and map throughout execution, not merely an unrelated map epoch. */
typedef struct {int enabled;GscMapExecutionCertificate certificate;} GscDeveloperScriptSelection;
int gsc_program_select_developer_value(GscProgram*,int);
int gsc_runtime_admit_developer_script(GscRuntime*,const GscDeveloperScriptSelection*);
int gsc_program_select_map_developer_script(GscProgram *,const GscDeveloperScriptSelection *);
int gsc_program_select_r232_developer_script(GscProgram *,const GscDeveloperScriptSelection *);
int gsc_runtime_select_r232_actor_animations(GscRuntime *,const GscActorAnimationHost *);
int gsc_program_select_r232_turret_animations(GscProgram *);
int gsc_program_select_training_rigs(GscProgram*);
int gsc_runtime_select_training_trees(GscRuntime*,const GscTurretAnimationSelection*,const GscAnimationCatalog*,const GscAnimationCatalog*);
int gsc_runtime_select_r232_turret_animations(GscRuntime *,const GscTurretAnimationSelection *);
int gsc_program_select_animation_methods(GscProgram *);
int gsc_runtime_select_animation_methods(GscRuntime *,const GscAnimationMethodHost *);
/* Read-only quiescent admission for synchronous external animation event ticks. */
int gsc_runtime_animation_delivery_available(const GscRuntime *,uint64_t,int32_t);
typedef struct { uint64_t runtime_identity,serial; } GscAnimationTickScope;
int gsc_runtime_animation_tick_begin(GscRuntime *,uint64_t,int32_t,GscAnimationTickScope *);
/* Commit the privately captured monotonic tick clock without executing tasks.
 * This accepted clock prefix is retained if a later notification refuses. */
int gsc_runtime_animation_tick_commit(GscRuntime *,const GscAnimationTickScope *);
int gsc_runtime_animation_tick_notify(GscRuntime *,const GscAnimationTickScope *,const GscEntityRef *,const char *,const GscEventValue *,size_t,int *);
void gsc_runtime_animation_tick_end(GscRuntime *,const GscAnimationTickScope *);



GscProgram *gsc_program_create_with_entity_queries(void);
int gsc_program_link_with_entity_queries(GscProgram *,const char *,char *,size_t);
GscRuntime *gsc_runtime_new_with_entity_queries(const GscProgram *, GscFogCallback,
    void *user, const GscEntityHost *, const GscEntityAccessHost *,
    const GscEntityControlHost *, const GscEntityCollectionHost *,
    const GscEntityPredicateHost *, const GscActorSpawnHost *, const GscMusicHost *,
    const GscResourceHost *, const GscObjectiveHost *, const GscConsoleHost *,
    const GscHudHost *, const GscMissionHost *, const GscBlastHost *, const GscQuakeHost *, const GscFxHost *, const GscCvarHost *, const GscShellshockHost *, const GscDebugTextHost *, const GscDebugLineHost *, const GscAmbientHost *, const GscPrecacheItemHost *, const GscSaveGameHost *, const GscTagFxHost *, const GscNativeQueryHost *, const GscPrecacheStringHost *, const GscSpawnerArrayHost *, const GscRandomIntHost *, const GscRandomFloatHost *, const GscKeyBindingHost *, const GscEntityQueryHost *, char *, size_t);

/* R150 dynamic bounds retain actual int32 tag/payload checks before RNG draw;
 * explicitly float-generating arguments and implicit coercion stay unselected. */
GscProgram *gsc_program_create_with_dynamic_randomint(void);
int gsc_program_link_with_dynamic_randomint(GscProgram *,const char *,char *,size_t);
GscRuntime *gsc_runtime_new_with_dynamic_randomint(const GscProgram *, GscFogCallback,
    void *user, const GscEntityHost *, const GscEntityAccessHost *,
    const GscEntityControlHost *, const GscEntityCollectionHost *,
    const GscEntityPredicateHost *, const GscActorSpawnHost *, const GscMusicHost *,
    const GscResourceHost *, const GscObjectiveHost *, const GscConsoleHost *,
    const GscHudHost *, const GscMissionHost *, const GscBlastHost *, const GscQuakeHost *, const GscFxHost *, const GscCvarHost *, const GscShellshockHost *, const GscDebugTextHost *, const GscDebugLineHost *, const GscAmbientHost *, const GscPrecacheItemHost *, const GscSaveGameHost *, const GscTagFxHost *, const GscNativeQueryHost *, const GscPrecacheStringHost *, const GscSpawnerArrayHost *, const GscRandomIntHost *, const GscRandomFloatHost *, const GscKeyBindingHost *, const GscEntityQueryHost *, char *, size_t);

/* R151 original GetInt FLOAT conversion: finite binary32 truncated toward zero
 * into int32, then the native positive-bound guard. No string conversion or
 * original arithmetic/FPU exception-state parity is selected. */
GscProgram *gsc_program_create_with_numeric_randomint(void);
int gsc_program_link_with_numeric_randomint(GscProgram *,const char *,char *,size_t);
GscRuntime *gsc_runtime_new_with_numeric_randomint(const GscProgram *, GscFogCallback,
    void *user, const GscEntityHost *, const GscEntityAccessHost *,
    const GscEntityControlHost *, const GscEntityCollectionHost *,
    const GscEntityPredicateHost *, const GscActorSpawnHost *, const GscMusicHost *,
    const GscResourceHost *, const GscObjectiveHost *, const GscConsoleHost *,
    const GscHudHost *, const GscMissionHost *, const GscBlastHost *, const GscQuakeHost *, const GscFxHost *, const GscCvarHost *, const GscShellshockHost *, const GscDebugTextHost *, const GscDebugLineHost *, const GscAmbientHost *, const GscPrecacheItemHost *, const GscSaveGameHost *, const GscTagFxHost *, const GscNativeQueryHost *, const GscPrecacheStringHost *, const GscSpawnerArrayHost *, const GscRandomIntHost *, const GscRandomFloatHost *, const GscKeyBindingHost *, const GscEntityQueryHost *, char *, size_t);

/* R152 dynamic arguments, same original exact-dyadic runtime domain as R134.
 * This conditional capability does not grant general rounded FLOAT arithmetic
 * or prove that a source field/parameter will satisfy the bounded GetFloat. */
GscProgram *gsc_program_create_with_dynamic_randomfloat(void);
int gsc_program_link_with_dynamic_randomfloat(GscProgram *,const char *,char *,size_t);
GscRuntime *gsc_runtime_new_with_dynamic_randomfloat(const GscProgram *, GscFogCallback,
    void *user, const GscEntityHost *, const GscEntityAccessHost *,
    const GscEntityControlHost *, const GscEntityCollectionHost *,
    const GscEntityPredicateHost *, const GscActorSpawnHost *, const GscMusicHost *,
    const GscResourceHost *, const GscObjectiveHost *, const GscConsoleHost *,
    const GscHudHost *, const GscMissionHost *, const GscBlastHost *, const GscQuakeHost *, const GscFxHost *, const GscCvarHost *, const GscShellshockHost *, const GscDebugTextHost *, const GscDebugLineHost *, const GscAmbientHost *, const GscPrecacheItemHost *, const GscSaveGameHost *, const GscTagFxHost *, const GscNativeQueryHost *, const GscPrecacheStringHost *, const GscSpawnerArrayHost *, const GscRandomIntHost *, const GscRandomFloatHost *, const GscKeyBindingHost *, const GscEntityQueryHost *, char *, size_t);
/* Selected-profile same-object BSP promotion; script execution stays closed until commit. */
GscRuntime *gsc_runtime_promote_bsp_dynamic_randomfloat(GscRuntime *,const GscProgram *, GscFogCallback,
    void *user, const GscEntityHost *, const GscEntityAccessHost *,
    const GscEntityControlHost *, const GscEntityCollectionHost *,
    const GscEntityPredicateHost *, const GscActorSpawnHost *, const GscMusicHost *,
    const GscResourceHost *, const GscObjectiveHost *, const GscConsoleHost *,
    const GscHudHost *, const GscMissionHost *, const GscBlastHost *, const GscQuakeHost *, const GscFxHost *, const GscCvarHost *, const GscShellshockHost *, const GscDebugTextHost *, const GscDebugLineHost *, const GscAmbientHost *, const GscPrecacheItemHost *, const GscSaveGameHost *, const GscTagFxHost *, const GscNativeQueryHost *, const GscPrecacheStringHost *, const GscSpawnerArrayHost *, const GscRandomIntHost *, const GscRandomFloatHost *, const GscKeyBindingHost *, const GscEntityQueryHost *, char *, size_t);
int gsc_runtime_commit_bsp_promotion(GscRuntime *);

/* R184 conditional, explicitly declared arithmetic for one source module.
 * No original startup/default control word is inferred. Precision is 24/53/64;
 * rounding is nearest-even/down/up/toward-zero. The two genuine decimal tokens
 * 0.4 and 0.8 have original software-parser witnesses independent of x87 CW;
 * other module float literals retain the earlier exact-domain admission.
 * Only normal finite float addition (operand exponent gap <=31), signed zeros,
 * and normal randomfloat projection are admitted. Other float binary operations
 * and selected-module numeric casts refuse. Unselected modules and earlier constructors retain their contracts.
 * Select the module before compiling it, and select the runtime before start.
 * The existing draw host supplies draw15 once; no Session wiring is implied. */
enum { GSC_FLOAT_ROUND_NEAREST=0, GSC_FLOAT_ROUND_DOWN=1,
       GSC_FLOAT_ROUND_UP=2, GSC_FLOAT_ROUND_ZERO=3 };
typedef struct { unsigned precision_bits,rounding; } GscModuleFloatArithmeticProfile;
int gsc_program_select_module_float_arithmetic(GscProgram *,const char *,const GscModuleFloatArithmeticProfile *);
int gsc_runtime_select_module_float_arithmetic(GscRuntime *);
/* Separately opt in to exact timed-animation operations. Old R184 profile
 * selection alone retains its addition-only/casts-held contract. */
int gsc_program_select_timed_animation_arithmetic(GscProgram *);
int gsc_runtime_select_timed_animation_arithmetic(GscRuntime *);
/* Separately selected R188 lane: explicit PC53/64+RC rounded ratios/native
 * wait deadlines with a private deadline-drain clock. Older lanes unchanged. */
int gsc_program_select_frame_animation_cadence(GscProgram *);
int gsc_runtime_select_frame_animation_cadence(GscRuntime *);
int gsc_runtime_frame_animation_cadence_selected(const GscRuntime *);

/* R237 opt-in native bucket order. HOST time is separate from the VM cursor.
 * Select the program before link, runtime before any startup. Mutually exclusive
 * with R188 cadence. Begin publishes host logical time without executing tasks;
 * current-bucket service is called only at an actual notify/yield seam. Timed
 * advance drains [cursor,target), excluding target. Positive PC53 nearest waits
 * only; zero-delay/wrap, native bytecode child and endon parity remain held.
 * Invalid order/time/generation leaves state/output unchanged; callback reentry
 * is fatal. The caller must finish or abandon a failed frame, never retry it.
 * Startup alone has a private bucket-zero drain; gsc_advance is not this API. */
typedef struct {int host_now_ms,vm_bucket_ms,frame_open;uint64_t frame_generation;} GscR237FrameView;
int gsc_program_select_r237_frame_buckets(GscProgram *);
int gsc_runtime_select_r237_frame_buckets(GscRuntime *);
int gsc_runtime_r237_frame_buckets_selected(const GscRuntime *);
int gsc_runtime_r237_frame_view(const GscRuntime *,GscR237FrameView *);
int gsc_r237_frame_begin(GscRuntime *,int host_logical_ms,uint64_t *out_generation);
int gsc_r237_run_current_bucket(GscRuntime *,uint64_t expected_generation);
/* A suspended phase retains the queue and the original instruction budget.
 * Returns 0 on failure, 1 on completion or 2 after a whole Task yielded. */
int gsc_r237_frame_phase_slice(GscRuntime *,uint64_t generation,int exclusive,int (*expired)(void));
int gsc_r237_advance_exclusive(GscRuntime *,uint64_t expected_generation);


/* R185 explicit bounded console display: ordinary strings and exact int32
 * decimal segments, with the existing per-argument/newline host events.
 * Other value kinds remain outside this selected capability. Accepted output
 * is retained on later refusal/cancellation. Earlier profiles stay strings-only.
 * Select the program before link and the runtime before start; no Session or
 * automatic GAME wiring is selected by these APIs. */
int gsc_program_select_console_int32_display(GscProgram *);
int gsc_runtime_select_console_int32_display(GscRuntime *);

/* R198 finite vector arithmetic is a selected port policy. Select before
 * linking/starting; legacy constructors retain their former arithmetic. */
int gsc_program_select_vector_arithmetic(GscProgram *);
int gsc_runtime_select_vector_arithmetic(GscRuntime *);
/* Finite portable vector math; no original x87 rounding-state parity implied. */
enum { GSC_EXPR_TARGET_VECTOR_MATH_NATIVE=GSC_EXPR_TARGET_ANIMATION_LENGTH_NATIVE+1 };
enum { GSC_LINK_VECTOR_MATH_NATIVE=GSC_LINK_ENTITY_QUERY_NATIVE+1 };
int gsc_program_select_vector_math(GscProgram *);
int gsc_runtime_select_vector_math(GscRuntime *);
int gsc_program_select_integer_bitwise(GscProgram *);
int gsc_runtime_select_integer_bitwise(GscRuntime *);
int gsc_program_select_string_int_concat(GscProgram *);
int gsc_runtime_select_string_int_concat(GscRuntime *);
int gsc_program_select_large_sources(GscProgram *);
/* R198 bounded task subscriptions: multiple level/entity event names; copied
 * complete owner identities, no child-thread inheritance. A notification
 * cancels subscribed tasks before waking ordinary waiters. */
/* Explicit native consumed-wait profile. Every delivered waiter requires its
 * own independently current map certificate; no legacy/default activation. */
int gsc_program_select_owned_wait_delivery(GscProgram *);
int gsc_runtime_select_owned_wait_delivery(GscRuntime *);
int gsc_program_select_endon(GscProgram *);
int gsc_runtime_select_endon(GscRuntime *);
/* R198 finite filters: event plus 1..15 captured values, exact payload count,
 * ordinary strings/numbers/vectors/entities/undefined. No output lvalues or
 * prefix/coercion/native object parity is inferred. */
int gsc_program_select_waittillmatch(GscProgram *);
int gsc_runtime_select_waittillmatch(GscRuntime *);
#define GSC_ENDON_CAPACITY 32

enum {
    GSC_PLAYER_GET_CURRENT_WEAPON=1, GSC_PLAYER_HAS_WEAPON,
    GSC_PLAYER_GIVE_WEAPON, GSC_PLAYER_TAKE_WEAPON,
    GSC_PLAYER_TAKE_ALL_WEAPONS, GSC_PLAYER_SWITCH_TO_WEAPON,
    GSC_PLAYER_SET_AMMO_CLIP, GSC_PLAYER_SET_AMMO_STOCK,
    GSC_PLAYER_ATTACK_PRESSED, GSC_PLAYER_USE_PRESSED,
    GSC_PLAYER_RELOAD_PRESSED, GSC_PLAYER_MELEE_PRESSED,
    GSC_PLAYER_FREEZE_CONTROLS,
    GSC_PLAYER_GET_FRACTION_MAX_AMMO,
    GSC_PLAYER_GET_STANCE,
    GSC_PLAYER_SET_NORMAL_HEALTH, GSC_PLAYER_GET_NORMAL_HEALTH, GSC_PLAYER_SET_VIEWMODEL, GSC_PLAYER_GIVE_MAX_AMMO, GSC_PLAYER_SHELLSHOCK, GSC_PLAYER_STOP_SHELLSHOCK, GSC_PLAYER_GET_SLOT_WEAPON, GSC_PLAYER_SET_ANGLES,
    GSC_PLAYER_ALLOW_STAND, GSC_PLAYER_ALLOW_CROUCH, GSC_PLAYER_ALLOW_PRONE
};
enum { GSC_PLAYER_RESULT_VOID=0,GSC_PLAYER_RESULT_INT,GSC_PLAYER_RESULT_STRING,
    GSC_PLAYER_RESULT_FLOAT };
typedef struct {
    GscEntityRef player;
    int kind,now_ms;
    char weapon[128];
    int32_t value;
    float number; /* Explicit numeric lane; zero for unrelated methods. */
    uint32_t angles_bits[3]; /* setPlayerAngles only; canonical zero otherwise. */
} GscPlayerNativeRequest;
typedef struct { int kind;int32_t value;char text[128];float number; } GscPlayerNativeResult;
/* Inputs/results are owned copies, never native engine pointers. Check the
 * borrowed current() lease around any callback that may retire a session. */
typedef struct {
    void *user;
    int (*invoke)(void *,const GscPlayerNativeRequest *,GscPlayerNativeResult *,
        char *,size_t,int (*current)(void *),void *);
} GscPlayerNativeHost;
int gsc_program_select_player_natives(GscProgram *);
int gsc_program_select_player_normal_health(GscProgram *);
int gsc_program_select_player_viewmodel(GscProgram *);
int gsc_program_select_player_max_ammo(GscProgram *);
int gsc_program_select_player_stance_permissions(GscProgram *);
int gsc_runtime_select_player_natives(GscRuntime *,const GscPlayerNativeHost *);
typedef struct {
    void *user;
    int (*invoke)(void *,const GscEntityRef *,int,char *,size_t,int (*current)(void *),void *);
} GscEntityDeleteHost;
enum {GSC_ENTITY_LINK_CHECK=1,GSC_ENTITY_LINK_ATTACH,GSC_ENTITY_UNLINK};
typedef struct {GscEntityRef child,parent;int operation,now_ms,argc,explicit_offsets;char tag[128];float origin[3],angles[3];} GscEntityLinkRequest;
typedef struct {void *user;int(*invoke)(void*,const GscEntityLinkRequest*,char*,size_t,int(*)(void*),void*);} GscEntityLinkHost;
int gsc_program_select_entity_links(GscProgram*);
int gsc_runtime_select_entity_links(GscRuntime*,const GscEntityLinkHost*);
int gsc_program_select_entity_operations(GscProgram *);
int gsc_runtime_select_entity_operations(GscRuntime *,const GscEntityDeleteHost *);
int gsc_runtime_select_origin_method(GscRuntime *,const GscEntityOriginMethodHost *);

/* R198 spatial queries use copied identities/results and the same borrowed
 * session-current lease as player methods. Unsupported engine topology refuses. */
typedef struct {uint64_t epoch;uint32_t index,generation;} GscNodeRef;
/* Native class3 vehicle nodes have their own identity and registry. They are
 * neither infantry class2 path nodes nor GAME entity allocations. */
typedef struct {uint64_t epoch;uint32_t index,generation;} GscVehicleNodeRef;
enum {GSC_VEHICLE_ATTACH_PATH=1,GSC_VEHICLE_START_PATH,GSC_VEHICLE_WAIT_NODE,GSC_VEHICLE_WAIT_SPEED,GSC_VEHICLE_SET_SPEED,GSC_VEHICLE_RESUME_SPEED};
typedef struct {GscEntityRef owner;GscVehicleNodeRef node;uint64_t program;int now_ms,kind;float values[2];} GscVehicleMethodRequest;
typedef struct {void*user;int(*invoke)(void*,const GscVehicleMethodRequest*,char*,size_t,int(*)(void*),void*);} GscVehicleMethodHost;
int gsc_program_select_vehicle_methods(GscProgram*);
int gsc_runtime_select_vehicle_methods(GscRuntime*,const GscVehicleMethodHost*);

enum {GSC_VEHICLE_NODE_CAPACITY=1500,GSC_VEHICLE_QUERY_ONE=4,GSC_VEHICLE_QUERY_ARRAY=5};
typedef struct {int kind,now_ms;char value[128],field[128];} GscVehicleNodeQuery;
enum {GSC_VEHICLE_VALUE_UNDEFINED=0,GSC_VEHICLE_VALUE_INT32,GSC_VEHICLE_VALUE_STRING,
 GSC_VEHICLE_VALUE_VECTOR,GSC_VEHICLE_VALUE_FLOAT};
typedef struct {int kind;int32_t integer;float vector[3];char text[128];float number;} GscVehicleNodeField;
typedef struct {
 void*user;
 /* Unknown descriptor returns defined0, count0. A known string descriptor
  * returns defined1 even for an empty array; duplicate ONE refuses. */
 int(*query)(void*,const GscVehicleNodeQuery*,GscVehicleNodeRef*,size_t,size_t*,int*,char*,size_t,int(*)(void*),void*);
 int(*validate)(void*,const GscVehicleNodeRef*,int,int*,char*,size_t,int(*)(void*),void*);
 int(*read)(void*,const GscVehicleNodeRef*,const char*,int,GscVehicleNodeField*,char*,size_t,int(*)(void*),void*);
} GscVehicleNodeHost;
int gsc_program_select_vehicle_nodes(GscProgram*);
int gsc_runtime_select_vehicle_nodes(GscRuntime*,const GscVehicleNodeHost*);

enum { GSC_SPATIAL_IS_TOUCHING=1,GSC_SPATIAL_GET_EYE,GSC_SPATIAL_STANDING_NODE };
enum { GSC_SPATIAL_RESULT_INT=1,GSC_SPATIAL_RESULT_VECTOR,GSC_SPATIAL_RESULT_NODE };
typedef struct {GscEntityRef receiver,other;int kind,now_ms;} GscSpatialNativeRequest;
typedef struct {int kind;int32_t value;union {float vector[3];struct {uint32_t epochLo,epochHi,index;} node;};} GscSpatialNativeResult;
typedef struct {
    void *user;
    int (*invoke)(void *,const GscSpatialNativeRequest *,GscSpatialNativeResult *,
        char *,size_t,int (*current)(void *),void *);
} GscSpatialNativeHost;
int gsc_program_select_spatial_natives(GscProgram *);
int gsc_runtime_select_spatial_natives(GscRuntime *,const GscSpatialNativeHost *);
/* Engine AnglesToAxis convention (forward,-right,up), validated pose reads. */
int gsc_program_select_entity_coordinates(GscProgram *);
int gsc_runtime_select_entity_coordinates(GscRuntime *);

/* R199 node identities name immutable map node records, never gentity slots.
 * Epoch and generation are nonzero; index is zero-based BSP/catalog order.
 * Callbacks copy all outputs and may not retain or mutate borrowed inputs.
 * Every callback returns exactly 1 on success, 0 on failure and observes the
 * supplied current lease before publishing a result. Missing metadata is an
 * UNDEFINED value; an explicitly empty string remains a STRING. */

enum {GSC_NODE_QUERY_ONE=1,GSC_NODE_QUERY_ARRAY,GSC_NODE_QUERY_ALL};
enum {GSC_NODE_FIELD_ORIGIN=1,GSC_NODE_FIELD_ANGLES,GSC_NODE_FIELD_TARGETNAME,
      GSC_NODE_FIELD_TARGET,GSC_NODE_FIELD_CLASSNAME,GSC_NODE_FIELD_ANIMSCRIPT,
      GSC_NODE_FIELD_SPAWNFLAGS,GSC_NODE_FIELD_TYPE,GSC_NODE_FIELD_SCRIPT_NOTEWORTHY,
      GSC_NODE_FIELD_SCRIPT_DELAY,GSC_NODE_FIELD_SCRIPT_FALLBACK,GSC_NODE_FIELD_RADIUS};
enum {GSC_NODE_VALUE_UNDEFINED=0,GSC_NODE_VALUE_INT32,GSC_NODE_VALUE_STRING,
      GSC_NODE_VALUE_VECTOR,GSC_NODE_VALUE_FLOAT};
enum {GSC_NODE_QUERY_CAPACITY=4096};
typedef struct {int kind,now_ms;char value[128],field[128];} GscNodeQueryRequest;
typedef struct {int kind;int32_t integer;float vector[3];char text[128];float number;} GscNodeFieldValue;
typedef struct {
    void *user;
    /* ONE: count 0 is a miss, count 1 is a hit; duplicates must refuse.
     * ARRAY/ALL: complete BSP-order result, including a defined empty array.
     * Overflow refuses; truncation and duplicate/mixed-epoch refs are invalid. */
    int (*query)(void *,const GscNodeQueryRequest *,GscNodeRef *,size_t,size_t *,
        char *,size_t,int (*current)(void *),void *);
    int (*validate)(void *,const GscNodeRef *,int,int *,char *,size_t,
        int (*current)(void *),void *);
    int (*read)(void *,const GscNodeRef *,const char *,int,GscNodeFieldValue *,char *,size_t,
        int (*current)(void *),void *);
} GscNodeQueryHost;
/* R222 typed goal ASSIGNMENT, not AI traversal or animation. The original
 * 200743f0 accepts a sentient and authored node; 200739a0 sets its goal,
 * resets +20 to -1 and invalidates the prior path when the position changes.
 * This selected port lane accepts real Actor reservations with no running
 * controller/path; it never fabricates an initialized or spawned Actor. */
/* R232 explicitly selected real turret effects. Entity refs and reply kinds
 * are validated around current(); callbacks must never retain borrowed input. */
enum {GSC_TURRET_SET_MODE=1,GSC_TURRET_START_FIRING,GSC_TURRET_STOP_FIRING,
      GSC_TURRET_SET_TARGET_ENTITY,GSC_TURRET_USE,GSC_TURRET_GET,
      GSC_TURRET_IS_FIRING,GSC_TURRET_SHOOT,GSC_TURRET_CLEAR_TARGET,GSC_TURRET_STOP_USE};
enum {GSC_TURRET_RESULT_VOID=0,GSC_TURRET_RESULT_BOOL,
      GSC_TURRET_RESULT_ENTITY,GSC_TURRET_RESULT_UNDEFINED,GSC_TURRET_RESULT_OWNER_RELEASED};
typedef struct {int kind,now_ms;GscEntityRef entity,argument;char mode[32];} GscTurretRequest;
typedef struct {int kind,boolean;GscEntityRef entity;} GscTurretReply;
typedef struct {void *user;int (*invoke)(void *,const GscTurretRequest *,
    GscTurretReply *,char *,size_t,int (*)(void *),void *);} GscTurretHost;
int gsc_program_select_r232_turrets(GscProgram *);
int gsc_runtime_select_r232_turrets(GscRuntime *,const GscTurretHost *);

typedef struct {float rate;char idle[8];int rest;} GscTrainingAppearance;
int gsc_runtime_training_appearance(GscRuntime*,const GscEntityRef*,int,GscTrainingAppearance*);
#define COD1_SET_GOAL_POS_METHOD 505
#define COD1_SET_GOAL_ENTITY_METHOD 507
#define COD1_ROTATE_VELOCITY 508
#define COD1_MOVE_GRAVITY 509
#define COD1_MOVE_TO 513
#define COD1_ROTATE_ROLL 514
#define COD1_LAUNCH 515
#define COD1_ROTATE_TO 516
int gsc_program_select_map_actor_goal(GscProgram*);
int gsc_program_select_entity_goal(GscProgram *);
typedef struct {GscEntityRef entity;union {GscNodeRef node;GscEntityRef target;};int now_ms;int positional;float position[3];} GscActorGoalRequest;
typedef struct {void *user;int (*invoke)(void *,const GscActorGoalRequest *,char *,size_t,int (*)(void *),void *);} GscActorGoalHost;
int gsc_program_select_r222_carbine_items(GscProgram *);
int gsc_program_select_r225_training_followup(GscProgram *);
int gsc_program_select_r228_training_opening(GscProgram *);
int gsc_program_select_r222_actor_goal(GscProgram *);
int gsc_runtime_select_r222_actor_goal(GscRuntime *,const GscActorGoalHost *);

enum {GSC_EXPR_TARGET_NODE_QUERY_NATIVE=GSC_EXPR_TARGET_VECTOR_MATH_NATIVE+1};
enum {GSC_LINK_NODE_QUERY_NATIVE=GSC_LINK_VECTOR_MATH_NATIVE+1};
int gsc_program_select_node_queries(GscProgram *);
int gsc_runtime_select_node_queries(GscRuntime *,const GscNodeQueryHost *);

/* R200 separate HUD elements. Owner is the globally unique runtime token,
 * epoch is map-owned, and index/generation identify one bounded allocation.
 * Entity/node conversion, private field bags and boolean coercion are absent.
 * A successful service operation publishes actual backend state; no input
 * pointer may be retained or mutated. current() fences session retirement. */
typedef struct {uint64_t epoch,owner;uint32_t index,generation;} GscHudElemRef;
enum {GSC_HUDELEM_CAPACITY=8};
enum {GSC_HUDELEM_CREATE=1,GSC_HUDELEM_WRITE,GSC_HUDELEM_READ,
      GSC_HUDELEM_SET_CLOCK,GSC_HUDELEM_DESTROY,GSC_HUDELEM_SET_SHADER,GSC_HUDELEM_SET_TEXT,GSC_HUDELEM_FADE};
enum {GSC_HUDELEM_FIELD_X=1,GSC_HUDELEM_FIELD_Y,GSC_HUDELEM_FIELD_ALIGN_X,GSC_HUDELEM_FIELD_ALIGN_Y,GSC_HUDELEM_FIELD_ALPHA,GSC_HUDELEM_FIELD_FONT_SCALE,GSC_HUDELEM_FIELD_SORT};
enum {GSC_HUDELEM_VALUE_NUMBER=1,GSC_HUDELEM_VALUE_STRING};
enum {GSC_HUDELEM_RESULT_VOID=0,GSC_HUDELEM_RESULT_REF,GSC_HUDELEM_RESULT_NUMBER,GSC_HUDELEM_RESULT_STRING};
typedef struct {
    uint64_t owner,operation_id;
    GscHudElemRef element;
    int kind,now_ms,field,value_kind;
    float number,remaining_seconds,total_seconds;
    int32_t width,height;
    char text[128];
} GscHudElemRequest;
typedef struct {GscHudElemRef element;int kind,alive;float number;char text[128];} GscHudElemResult;
typedef struct {
    void *user;
    /* CREATE returns REF+new identity. Other results preserve element exactly;
     * WRITE/SET_CLOCK/DESTROY return VOID, READ returns its field's kind.
     * alive is 1 except successful DESTROY, which returns 0. */
    int (*invoke)(void *,const GscHudElemRequest *,GscHudElemResult *,char *,size_t,
        int (*current)(void *),void *);
    int (*validate)(void *,const GscHudElemRef *,int,int *,char *,size_t,
        int (*current)(void *),void *);
    /* Retire only this owner's elements, even after abort or loss of player.
     * Success requires actual cleanup, or confirmed absence/map retirement.
     * Refusal leaves the owner available for an explicit cleanup retry. */
    int (*retire)(void *,uint64_t owner,char *,size_t);
} GscHudElemHost;
enum {GSC_EXPR_TARGET_HUDELEM_NATIVE=GSC_EXPR_TARGET_NODE_QUERY_NATIVE+1};
enum {GSC_LINK_HUDELEM_NATIVE=GSC_LINK_NODE_QUERY_NATIVE+1};
enum {GSC_RECEIVER_TARGET_HUDELEM_NATIVE=GSC_RECEIVER_TARGET_ENTITY_EVENT+1};
int gsc_program_select_hud_elements(GscProgram *);
int gsc_runtime_select_hud_elements(GscRuntime *,const GscHudElemHost *);
int gsc_runtime_retire_hud_elements(GscRuntime *,char *,size_t);
typedef int (*GscEntityWorldAction)(void *,const GscEntityRef *,int,char *,size_t,int (*)(void *),void *);
typedef struct {
    void *user;
    GscEntityWorldAction notsolid,solid,grenade_touch,grenade_bounce;
} GscEntityCollisionHost;
enum { GSC_CONNECTPATHS_METHOD=506, GSC_DISCONNECTPATHS_METHOD=519, GSC_SOLID_METHOD=510, GSC_GRENADE_TOUCH_METHOD=511, GSC_GRENADE_BOUNCE_METHOD=512 };
int gsc_program_select_disconnectpaths(GscProgram *);
int gsc_runtime_select_disconnectpaths(GscRuntime *,GscEntityWorldAction);
int gsc_program_select_connectpaths(GscProgram *);
int gsc_runtime_select_connectpaths(GscRuntime *,GscEntityWorldAction);
int gsc_program_select_entity_collision(GscProgram *);
int gsc_runtime_select_entity_collision(GscRuntime *,const GscEntityCollisionHost *);

/* R202 selected getaiarray. The native domain is the 32 occupied Actor pool
 * records, ordered by Actor index; health/initialization are not filters.
 * Requests use a nonzero subset of team bits 2/4/8; zero script arguments
 * become 0xE. A provider must prove the complete current map Actor domain.
 * An empty provisional registry is not proof of an empty game AI domain.
 * Callbacks return exactly 1 for success, retain no borrowed pointers, and
 * preserve request/snapshot inputs. Refusal leaves output storage untouched.
 * validate rechecks the complete observable snapshot and domain authority
 * after entity validation callbacks, before the VM publishes its array. */
enum {GSC_AI_ARRAY_CAPACITY=32};
typedef struct {int now_ms;uint32_t team_mask;} GscAiArrayRequest;
typedef struct {uint32_t actor_index;GscEntityRef entity;} GscAiArrayMember;
typedef struct {uint32_t count;GscAiArrayMember members[GSC_AI_ARRAY_CAPACITY];} GscAiArraySnapshot;
typedef struct {
    void *user;
    int (*query)(void *,const GscAiArrayRequest *,GscAiArraySnapshot *,char *,size_t,
        int (*current)(void *),void *);
    int (*validate)(void *,const GscAiArrayRequest *,const GscAiArraySnapshot *,char *,size_t,
        int (*current)(void *),void *);
} GscAiArrayHost;
enum {GSC_EXPR_TARGET_AI_ARRAY_NATIVE=GSC_EXPR_TARGET_HUDELEM_NATIVE+1};
enum {GSC_LINK_AI_ARRAY_NATIVE=GSC_LINK_HUDELEM_NATIVE+1};
int gsc_program_select_ai_arrays(GscProgram *);
int gsc_runtime_select_ai_arrays(GscRuntime *,const GscAiArrayHost *);

/* R203 copied restricted bootstrap service facade. */
enum {GSC_ACTOR_BOOTSTRAP_AVAILABLE_RESOURCES=1,
      GSC_ACTOR_BOOTSTRAP_AVAILABLE_SPAWNER_TEAM=2};
/* Restricted BSP constructor profile: only private-field publication and a
 * synchronous native prepare. Cannot start script tasks or ordinary entry. */
GscRuntime *gsc_runtime_new_bsp_constructor(const GscProgram *,GscFogCallback,void *,const GscEntityHost *,const GscEntityFieldHost *,char *,size_t);
int gsc_construct_bsp_entity(GscRuntime *,const GscPreparedEntityEntry *,const GscPreparedEntityEntryHost *,GscPreparedEntityEntryRun *,char *,size_t);
int gsc_bsp_fields_current(const GscRuntime *,const GscPreparedEntityEntry *);
int gsc_construct_prepared_entity(GscRuntime *,const GscPreparedEntityEntry *,const GscPreparedEntityEntryHost *,GscPreparedEntityEntryRun *,char *,size_t);
int gsc_runtime_locomotion_main(GscRuntime*,const GscEntityExecutionCertificate*,const GscMapExecutionCertificate*,int,GscEntityScriptRun*);
int gsc_execute_course_anim_entry(GscRuntime *,const GscEntityScriptEntry *,const GscEntityExecutionCertificate *,int,GscEntityScriptRun *);
int gsc_execute_constructed_entity(GscRuntime *,const GscEntityScriptEntry *,const GscEntityExecutionCertificate *,GscEntityScriptRun *);
/* Read-only copied node DATA, not a pathfinding or script certificate. */
typedef struct {int status;uint32_t physicalIndex,authoredIndex,type;float origin[3];} GscActorBootstrapNodeTarget;
typedef struct {
 GscEntityRef entity;uint64_t runtime_identity,catalog_identity,task_serial;
 int now_ms;
} GscActorInitReturnRequest;
/* VM pop host is synchronous; a successful caller-supplied result is never
 * a placed currentstate permit. Session/GAME separately authenticate this pop. */
typedef struct {void*user;int(*invoke)(void*,const GscActorInitReturnRequest*,char*,size_t,int(*)(void*),void*);} GscPlacedInitReturnHost;
typedef struct {GscActorInitReturnRequest init;uint64_t program_identity,session_id;int host_now_ms;} GscPlacedInitReturnRequest;
/* Actual fixed INIT initial invocation result. These identities describe the
 * owned VM work; they are not readiness or caller-issued authority. */
typedef struct {
 GscEntityScriptRun entry;uint64_t runtime_identity,catalog_identity,program_identity;
 uint32_t active_children;int root_pop_observed,entry_scope_released,currentstate_completed;
} GscPlacedInitRun;
int gsc_execute_placed_init_initial(GscRuntime *,const GscEntityRef *,const GscEntityExecutionCertificate *,const GscMapExecutionCertificate *,GscPlacedInitRun *,char *,size_t);
int gsc_execute_placed_init_with_return(GscRuntime *,const GscEntityRef *,const GscEntityExecutionCertificate *,const GscMapExecutionCertificate *,const GscPlacedInitReturnHost *,GscPlacedInitRun *,char *,size_t);
typedef struct GscActorBootstrapServices {
    /* Opaque borrowed Session stack lease, never a GscRuntime pointer.
     * No service/context/input pointer may be retained beyond run(). */
    void *context;
    int (*current)(void *);
    uint64_t (*program_identity)(void *);
    int (*script_function_available)(void *,const char *,const char *);
    int (*storage_separate)(void *,const void *,size_t);
    int (*available)(void *,int);
    /* Copies the current runtime diagnostic, including after a stage error;
     * returns 0 for an expired service lease, never an internal string. */
    int (*copy_error)(void *,char *,size_t);
    int (*start_entity_hooks)(void *,const GscEntityHookBootstrapEntry *,size_t,
        const GscEntityHookBootstrapHost *,GscEntityHookBootstrapResult *,char *,size_t);
    int (*execute_prepared_entity_entry)(void *,const GscPreparedEntityEntry *,
        const GscPreparedEntityEntryHost *,GscPreparedEntityEntryRun *,char *,size_t);
    /* Split startup: construct all selected rows before hooks, then enter each
     * row with a newly issued current certificate. No certificate is retained. */
    int (*construct_entity)(void *,const GscPreparedEntityEntry *,const GscPreparedEntityEntryHost *,GscPreparedEntityEntryRun *,char *,size_t);
    int (*adopt_bsp_entity)(void *,const GscPreparedEntityEntry *,char *,size_t);
    int (*enter_constructed_entity)(void *,const GscEntityScriptEntry *,const GscEntityExecutionCertificate *,GscEntityScriptRun *,char *,size_t);
    /* Optional selected graph lane. Valid only after this exact placed row's
     * returned entry, while the facade is alive. No Session pointer escapes. */
    int (*node_target)(void *,const GscEntityRef *,const char *,GscActorBootstrapNodeTarget *,char *,size_t);
    /* Separate fixed placed-row INIT lane. Its children retain durable map/
     * allocation certificates, never this borrowed facade/context. */
    int (*placed_init_initial)(void *,const GscEntityRef *,const GscEntityExecutionCertificate *,GscPlacedInitRun *,char *,size_t);
    int (*placed_init_return_current)(void *,const GscPlacedInitReturnRequest *);
    int (*postcohort_node_owners)(void *,char *,size_t); /* Fixed late phase; never main scheduling/completion. */
    int placed_cohort; /* Selected SP preserving-task lane; never completion. */
} GscActorBootstrapServices;

/* R203 read-only registered attachment queries. Exact integer indices 0..6
 * are the selected port policy; no float/string index coercion is provided.
 * Requests, output and current callback are borrowed for the synchronous call. */
enum { GSC_ATTACH_QUERY_SIZE=1,GSC_ATTACH_QUERY_MODEL_NAME=2,GSC_ATTACH_QUERY_TAG_NAME=3 };
enum { GSC_ATTACH_RESULT_INTEGER=1,GSC_ATTACH_RESULT_STRING=2 };
typedef struct { GscEntityRef entity;int now_ms,kind,index; } GscAttachmentQueryRequest;
typedef struct { int kind;int32_t integer;char string[128]; } GscAttachmentQueryResult;
typedef struct { void *user;int (*invoke)(void *,const GscAttachmentQueryRequest *,
    GscAttachmentQueryResult *,char *,size_t,int (*current)(void *),void *lease); } GscAttachmentQueryHost;
int gsc_program_select_attachment_queries(GscProgram *);
int gsc_runtime_select_attachment_queries(GscRuntime *,const GscAttachmentQueryHost *);

/* New explicit VM continuation. Requires an actual completed bootstrap,
 * quiescent clock0, no active children and no prior configured main. It marks
 * main attempted before task creation and never resets started or errors.
 * The configured root and children retain this copied map certificate. */
int gsc_runtime_model_starting(const GscRuntime *);
int gsc_start_configured_main_after_cohort(GscRuntime *,const GscMapExecutionCertificate *,uint64_t,uint32_t);
/* Ordinary map entry with a real map lifetime, independent of actor bootstrap.
 * The call lease guards only startup; child tasks retain the map certificate. */
int gsc_start_with_map(GscRuntime *,const GscMapExecutionCertificate *,int (*)(void *),void *);
int gsc_program_select_map_source_float(GscProgram*);
int gsc_runtime_select_map_source_float(GscRuntime*);
int gsc_start_configured_main_after_bootstrap(GscRuntime *,
    const GscMapExecutionCertificate *);

/* R204 selected synchronous delete transaction. The host admits the exact
 * copied owner before notify_death(), then frees it and commits its own
 * retirement accounting before consulting current() after the free.
 * Services/context are borrowed only for invoke(); retain no pointer.
 * current() keeps a map/transaction lease independent of death-canceled tasks.
 * notify_death() is one-shot, zero-payload and never drains script execution.
 * Matched ordinary waiters consume their completed wait-owner fence; their
 * map/entity execution certificates remain. This is selected port policy. */
typedef struct { GscEntityRef entity;int now_ms; } GscDeleteTransactionRequest;
typedef struct {
    void *context;
    int (*current)(void *);
    int (*notify_death)(void *,char *,size_t);
} GscDeleteTransactionServices;
typedef struct {
    void *user;
    int (*invoke)(void *,const GscDeleteTransactionRequest *,
        const GscDeleteTransactionServices *,char *,size_t);
} GscDeleteTransactionHost;
int gsc_program_select_delete_transactions(GscProgram *);
int gsc_runtime_select_delete_transactions(GscRuntime *,const GscDeleteTransactionHost *);

/* R209 explicitly selected, fixed generic_human/training_guy_practicejump
 * GetStartOrigin. Exactly vector,vector,current typed animation arguments.
 * The real GAME resource owner supplies PC53-nearest arithmetic; this VM only
 * copies finite binary32 values. Tokens are private runtime/catalog identities,
 * never original packed tree/node IDs. All pointers/current/context are borrowed
 * for one synchronous query; providers preserve request bytes and return1 only
 * after checking the scoped caller. No general leaf/playback capability. */
enum {GSC_EXPR_TARGET_START_ORIGIN_NATIVE=GSC_EXPR_TARGET_AI_ARRAY_NATIVE+1};
enum {GSC_EXPR_TARGET_R232_RANDOM_RANGE=GSC_EXPR_TARGET_START_ORIGIN_NATIVE+1};
int gsc_program_select_r232_randomfloat_range(GscProgram *);
int gsc_runtime_select_r232_randomfloat_range(GscRuntime *);
/* Explicit PC53/nearest scalar operations in unchanged init/look modules only. */
int gsc_program_select_r232_source_float_arithmetic(GscProgram *);
int gsc_runtime_select_r232_source_float_arithmetic(GscRuntime *);
/* Ordinary private arrays: bounded sparse numeric writes. Select before start;
 * missing keys read undefined and size counts present entries. Indexed undefined
 * deletion and portable game-owner persistence are not added by this selector.
 * Shared-container mutation refuses pending original copy-on-write support. */
int gsc_runtime_select_sparse_numeric_arrays(GscRuntime *);
int gsc_program_select_r233_owned_numeric_delete(GscProgram *);
int gsc_runtime_select_r233_owned_numeric_delete(GscRuntime *);
/* Explicit native array-value COW and safe-drain lifetime. The program selector
 * is parse-independent and may run after compile, before whole link. Runtime
 * selection requires arrays+sparse before start or any array allocation.
 * Non-undefined indexed
 * writes only; native entity field lvalues and legacy profiles remain held. */
int gsc_program_select_r238_array_value_cow(GscProgram *);
int gsc_runtime_select_r238_array_value_cow(GscRuntime *);
int gsc_runtime_r238_array_value_cow_selected(const GscRuntime *);
/* Explicit pre-start finite NUMBER/ordinary STRING EQ/NE; no default profile. */
/* Separate immediate statementthread and current-bucket wait capabilities.
 * Thread expressions, native serialized stacks,24bit wrap are not admitted. */
int gsc_program_select_r243_same_bucket_wait(GscProgram *);
int gsc_runtime_select_r243_same_bucket_wait(GscRuntime *);
int gsc_runtime_r243_same_bucket_wait_selected(const GscRuntime *);
/* Separate pre-start choice; it cannot open a resource/publication window. */
int gsc_program_select_configured_precache_interval(GscProgram *);
int gsc_runtime_select_configured_precache_interval(GscRuntime *);
int gsc_program_select_r243_thread_start(GscProgram *);
int gsc_runtime_select_r243_thread_start(GscRuntime *);
int gsc_runtime_select_r243_thread_entities(GscRuntime *,const GscThreadEntityAuthorityHost *);
int gsc_runtime_r243_thread_start_selected(const GscRuntime *);
int gsc_program_select_r239_mixed_scalar_equality(GscProgram *);
int gsc_runtime_select_r239_mixed_scalar_equality(GscRuntime *);
int gsc_runtime_r239_mixed_scalar_equality_selected(const GscRuntime *);
enum {GSC_LINK_START_ORIGIN_NATIVE=GSC_LINK_AI_ARRAY_NATIVE+1};
typedef struct {
    char tree[64],name[64];uint32_t ordinal,flags;
    uint64_t runtime_identity,catalog_identity;
    int now_ms;float origin[3],angles[3];
    unsigned query_kind; /* 0 legacy origin; map profile1 origin /2 angles. */
} GscStartOriginRequest;
typedef struct {float vector[3];} GscStartOriginReply;
typedef struct {
    void *user;
    int (*query)(void *,const GscStartOriginRequest *,GscStartOriginReply *,
        char *,size_t,int (*current)(void *),void *);
} GscStartOriginHost;
int gsc_program_select_start_origin_query(GscProgram *);
int gsc_program_select_map_start_pose(GscProgram*);
int gsc_runtime_select_start_origin(GscRuntime *,const GscStartOriginHost *);
int gsc_runtime_select_namespace_start_origin(GscRuntime *,const GscStartOriginHost *);

/* R216 selected grenade item spawn domain. Earlier spawn profiles are intact;
 * execution additionally requires the selected typed native field provider. */
int gsc_program_select_r216_items(GscProgram *);
/* Explicit nonblocking yaw motion with finite typed ramp times in seconds. */
typedef struct {
    GscEntityRef entity;
    float yaw,seconds,accel,decel;
    int method,operandsValid;
    float velocity[3];
    int64_t now_ms;
} GscYawMotionRequest;
typedef struct {
    void *user;
    int (*invoke)(void *,const GscYawMotionRequest *,char *,size_t,
                  int (*current)(void *),void *context);
} GscYawMotionHost;
int gsc_program_select_r216_yaw(GscProgram *);
int gsc_program_select_map_moveto(GscProgram *program);
int gsc_runtime_select_r216_yaw(GscRuntime *,const GscYawMotionHost *);


/* R233 native Actor-only DropWeapon, selected separately from animation. */
typedef struct {GscEntityRef entity;uint64_t runtime_identity;int32_t now_ms;uint32_t speed_bits;char name[64],hand[64];} GscActorDropRequest;
typedef struct {void *user;int(*invoke)(void*,const GscActorDropRequest*,char*,size_t,int(*)(void*),void*);} GscActorDropHost;
int gsc_program_select_r233_actor_drop(GscProgram*);
int gsc_runtime_select_r233_actor_drop(GscRuntime*,const GscActorDropHost*);

/* R233 bounded real flagged leaf host, separately selected. */
int gsc_program_select_r233_flagged_all_restart(GscProgram*);
int gsc_runtime_select_r233_flagged_all_restart(GscRuntime*);

/* Explicit fresh AI animscripted prefix. The host owns real queue/old-record
 * effects, never source fields. VM retains opaque Values and runs unchanged
 * scripted::init AFTER the guarded host callback closes. No state9 consumer,
 * normal-record construction, main/playback or full actor readiness is implied. */
typedef struct {
 GscEntityRef entity;int32_t now_ms;uint64_t runtime_identity,catalog_identity;
 GscAnimationLengthRequest animation,root;char notify[64];
 uint32_t origin_bits[3],angles_bits[3],argc,root_present;
} GscActorScriptedPrefixRequest;
typedef struct {void*user;
 int(*invoke)(void*,const GscActorScriptedPrefixRequest*,GscEntityExecutionCertificate*,
              int*,char*,size_t,int(*)(void*),void*);
} GscActorScriptedPrefixHost;
typedef struct {
 void*user;
 int(*invoke)(void*,const GscActorScriptedPrefixRequest*,char*,size_t,int(*)(void*),void*);
} GscActorScriptedStartHost;
int gsc_program_select_r243_scripted_start(GscProgram*);
int gsc_runtime_select_r243_scripted_start(GscRuntime*,const GscActorScriptedStartHost*);
int gsc_program_select_r243_scripted_prefix(GscProgram*);
int gsc_runtime_select_r243_scripted_prefix(GscRuntime*,const GscActorScriptedPrefixHost*);

/* Work draft: opt-in fixed scripted controller PREFIX, not full Think/tail. */
typedef struct {
 GscEntityRef entity;uint64_t actor_serial,runtime_identity,catalog_identity,frame_generation;
 int now_ms;
 /* Explicit ordinary MAIN producer: zero retains the existing scripted API;
  * -1 retains the current ordinary source,1 stop,2 run. */
 int locomotion_source;
} GscActorControllerRequest;
typedef struct {
 GscEntityExecutionCertificate certificate;
 uint64_t selection_serial;
 int source_changed; /* native force250/source23C comparison, not caller choice */
} GscActorControllerSelection;
typedef struct {
 uint64_t dispatch,task_serial,runtime_identity,catalog_identity,frame_generation;GscEntityRef entity;
} GscActorSourceAnimationReceipt;
typedef struct {void*context;
 int(*take)(void*,const GscActorAnimationRequest*,int(*)(void*),void*,GscActorSourceAnimationReceipt*);
 int(*claim)(void*,const GscActorAnimationRequest*,int(*)(void*),void*,GscActorSourceAnimationReceipt*);
 int(*current)(void*,const GscActorSourceAnimationReceipt*,const GscActorAnimationRequest*,int(*)(void*),void*);
} GscActorSourceAnimationBroker;
typedef struct {
 uint64_t dispatch,task_serial,runtime_identity,catalog_identity,frame_generation;GscEntityRef entity;
} GscActorSourceLookReceipt;
typedef struct {void*context;
 int(*take)(void*,const GscActorLookRequest*,int(*)(void*),void*,GscActorSourceLookReceipt*);
 int(*claim)(void*,const GscActorLookRequest*,int(*)(void*),void*,GscActorSourceLookReceipt*);
 int(*current)(void*,const GscActorSourceLookReceipt*,const GscActorLookRequest*,int(*)(void*),void*);
} GscActorSourceLookBroker;
typedef struct {
 void*context;int(*current)(void*);int(*run_current)(void*);
 int(*notify)(void*,const GscEntityRef*,const char*,const char*);
 GscActorSourceAnimationBroker source;
 GscActorSourceLookBroker sourceLook;
} GscActorControllerAnimationServices;
typedef struct {
 void*user;
 int(*init_return)(void*,const GscActorInitReturnRequest*,char*,size_t,int(*)(void*),void*);
 int(*select_main)(void*,const GscActorControllerRequest*,GscActorControllerSelection*,char*,size_t,int(*)(void*),void*);
 int(*animation_step)(void*,const GscActorControllerRequest*,const GscActorControllerAnimationServices*,char*,size_t,int(*)(void*),void*);
 /* Returns1 on real prefix, writes explicit frontier1/2 (presence derives
  * solely from that closed mapping). Exact normal13590 then absent/pop9. */
 int(*normal_update)(void*,const GscActorControllerRequest*,int*,char*,size_t,int(*)(void*),void*);
} GscActorScriptedControllerHost;
enum {GSC_ACTOR_CONTROLLER_STATE9_TAIL_HELD=1,GSC_ACTOR_CONTROLLER_RESUMED4_THINK_HELD=2};
typedef struct {
 GscEntityRef entity;uint64_t actor_serial,selection_serial,main_receipt,task_serial;
 int source_changed,main_status,frontier,record_present;
} GscActorControllerPrefixRun;

typedef struct {
 void*context;
 int(*run)(void*,int phase,uint64_t oldSelection,int(*fence)(void*),void*game);
} GscMainReplacementService;
typedef int(*GscControllerSelectWithCleanup)(void*,const GscActorControllerRequest*,GscActorControllerSelection*,char*,size_t,int(*)(void*),void*,const GscMainReplacementService*);
int gsc_runtime_select_r243_controller_with_cleanup(GscRuntime*,const GscActorScriptedControllerHost*,GscControllerSelectWithCleanup);
int gsc_runtime_select_r243_scripted_controller(GscRuntime*,const GscActorScriptedControllerHost*);
int gsc_runtime_r243_controller_prefix(GscRuntime*,const GscEntityRef*,uint64_t,uint64_t,const GscMapExecutionCertificate*,GscActorControllerPrefixRun*);

/* Retain the actual already-admitted stationary stop MAIN. This owns its
 * returned thread reference, not the still-unqualified native source/category
 * request producers. Source selection remains the existing GAME transaction. */
int gsc_runtime_initial_stop_main(GscRuntime*,const GscEntityExecutionCertificate*,const GscMapExecutionCertificate*,GscEntityScriptRun*);

int gsc_runtime_r243_controller_storage_separate(const GscRuntime*,const void*,size_t);
/* Internal exact readonly forwarded-request predicate, never output storage. */
int gsc_runtime_r243_controller_request_input(const GscRuntime*,const void*,size_t,int(*)(void*),void*);

/* Resolved node-script DATA cache; does not admit script execution. */
int gsc_runtime_node_scripts_build(GscRuntime*,const GscNodeScriptNames*);
void gsc_runtime_node_scripts_retire(GscRuntime*);
GscNodeScriptRef gsc_runtime_node_script_lookup(const GscRuntime*,const char*);
int gsc_runtime_node_script_current(const GscRuntime*,GscNodeScriptRef);
/* Committed program DATA only. Handles are function indexes+1, not callable
 * authority; scope is the exact immutable linked program identity. */
uint64_t gsc_program_identity(const GscProgram *);
uint32_t gsc_program_script_function_handle(const GscProgram *,const char *,const char *);
void gsc_program_script_function_handles(const GscProgram *,const char *,const char *const *,uint32_t *,unsigned);

#endif

#ifndef GSC_BULLET_TRACE_TYPES
#define GSC_BULLET_TRACE_TYPES
enum {GSC_EXPR_TARGET_BULLET_TRACE_NATIVE=1004};
typedef struct {uint64_t program;int now_ms,actors;float start[3],end[3];} GscBulletTraceRequest;
/* World/miss scope. Entity identities require a separate live-value contract.
 * surface0..22 native material table,23 miss/none; entity_kind0 none,1 world. */
typedef struct {float fraction,position[3],normal[3];unsigned surface,entity_kind;} GscBulletTraceReply;
typedef struct {void*user;int(*query)(void*,const GscBulletTraceRequest*,GscBulletTraceReply*,char*,size_t,int(*)(void*),void*);} GscBulletTraceHost;
int gsc_program_select_bullet_trace(GscProgram*);
int gsc_runtime_select_bullet_trace(GscRuntime*,const GscBulletTraceHost*);

#endif
