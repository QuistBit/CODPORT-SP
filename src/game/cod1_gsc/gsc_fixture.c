#include "gsc_fixture.h"
#include "gsc_native_imports.h"
#include "stance-profile.h"
#include "actor-float-profile.h"
#include "move-delta-profile.h"
#include "animtime-profile.h"
#include "gsc_vm_r243_staged_protocol.h"

#include <ctype.h>
#include <errno.h>
#include <float.h>
#include <limits.h>
#include <math.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../g_cod1_animation_timeline_math.h"
#include "gsc_ambient_payload.inc"
#include "gsc_precache_item_payload.inc"
#include "gsc_save_game_payload.inc"
#include "gsc_tag_fx_payload.inc"
#include "gsc_module_identity.inc"

/* The capacity test injects realloc failures without changing the shipped
 * standalone fixture's allocator or public API. */
#ifndef GSC_IR_REALLOC
#define GSC_IR_REALLOC realloc
#endif

/* Test-only allocator hooks; default lifecycle remains calloc/free. */
#ifndef GSC_RUNTIME_CALLOC
#define GSC_RUNTIME_CALLOC calloc
#endif
#ifndef GSC_RUNTIME_FREE
#define GSC_RUNTIME_FREE free
#endif

#define MAX_SOURCE 131072
#define MAX_MODULES 1024
#define MAX_MODULE_PATH 128
#define MAX_FUNCS 8192
#define MAX_PARAMS 16
#define MAX_EXPR 131072
#define MAX_CODE 131072
#define MAX_LOOPS 8192
#define MAX_CONTROL_DEPTH 32
#define MAX_SWITCHES 8192
#define MAX_CASES 32768
#define MAX_DEV_BLOCKS 8192
#define MAX_STRING_LITERALS 131072
#define MAX_THREADS 32
#define MAX_FRAMES 16
#define MAX_LOCALS 32
#define MAX_STEPS 100000
#define MAX_STATE_FIELDS GSC_STATE_FIELD_CAPACITY
_Static_assert(MAX_PARAMS<=GSC_DYNAMIC_INFO_MAX_ARGS,
               "computed-call info args must cover parser argument capacity");

enum { T_EOF=256, T_ID, T_NUMBER, T_STRING, T_LOCALIZED_STRING,
       T_EQ, T_NE, T_LE, T_GE,
       T_AND, T_OR, T_SCOPE, T_DEV_OPEN, T_DEV_CLOSE,
       T_ADD_ASSIGN, T_SUB_ASSIGN, T_MUL_ASSIGN, T_DIV_ASSIGN,
       T_INC, T_DEC, T_INVALID,T_BIT_OR_ASSIGN,T_BIT_XOR_ASSIGN,T_BIT_AND_ASSIGN };
typedef struct { int type, line, col, integer; char word[128]; float number; int32_t integer_value;
                 size_t source_offset, raw_offset; int raw_length;
                 int unknown_escape_count, first_unknown_offset;
                 char raw[GSC_STRING_RAW_CAPACITY]; } Token;
typedef GscStringLiteralInfo StringLiteral;
_Static_assert(sizeof(((Token *)0)->word)==GSC_STRING_VALUE_CAPACITY,
               "lexer and public string value capacities must match");
_Static_assert(GSC_STRING_RAW_CAPACITY>=2*(GSC_STRING_VALUE_CAPACITY-1)+1,
               "raw string must hold two spelling bytes per lexical unit plus NUL");
typedef struct { int kind, instruction; } ControlContext;
typedef struct { const char *source; size_t pos, length; int line, col; Token tok;
                 char *error; size_t error_size; int failed, expr_depth, module,
                 control_depth, in_dev_block,integer_bitwise_syntax,native_quoted_strings;
                 ControlContext controls[MAX_CONTROL_DEPTH]; } Parser;

enum { E_NUMBER, E_STRING, E_LOCALIZED_STRING, E_VAR, E_UNARY, E_BINARY, E_ISDEFINED,
       E_FIELD, E_INDEX, E_NATIVE_CALL, E_METHOD_CALL, E_VECTOR, E_ANIM_REF,
       E_DYNAMIC_CALL, E_EMPTY_ARRAY, E_SCOPED_CALL, E_CAST, E_FUNC_REF,
       E_ANIMTREE_VALUE, E_RAW_STRING, E_RAW_LOCALIZED_STRING, E_THREAD_CALL };
typedef struct { int kind, op, a, b, integer, argc, line, target, owner_module, float_arithmetic,
                 args[MAX_PARAMS];
                 float number; int32_t integer_value; char name[128], module[MAX_MODULE_PATH];
                 /* R64: thread-keyword byte column, not a native PC. */
                 int col;
                 /* R66 call/reference origin; separate from legacy IR fields. */
                 int audit_line, audit_col; size_t audit_offset;
                 /* R67: explicit namespace for a committed bare-call target.
                  * E_NATIVE_CALL remains a historical syntax kind only. */
                 int call_binding;
                 /* R69: separate static receiver script namespace, never builtin. */
                 int receiver_binding_kind, receiver_script_target, receiver_native_target, receiver_event_target;
                 int field_write_target; } Expr;
enum { I_SET, I_GAME_SET, I_CALL, I_THREAD, I_WAIT, I_WAITER, I_NOTIFY, I_JZ, I_JUMP,
       I_METHOD, I_METHOD_THREAD, I_FOR, I_WHILE, I_RETURN, I_BREAK,
       I_DYNAMIC_CALL, I_DYNAMIC_THREAD, I_MUTATE, I_CONTINUE, I_SWITCH, I_EMPTY,
       I_DO_WHILE };
/* col is retained for statement starts, including empty and receiver-thread
 * statements. IR is not a serialized ABI. */
typedef struct { int kind, line, expr, dest, receiver, argc, args[MAX_PARAMS], owner_module, target, op, col;
                 char name[128], module[MAX_MODULE_PATH]; size_t audit_offset;
                 int receiver_binding_kind, receiver_script_target, receiver_native_target, receiver_event_target; } Instr;
enum { FOR_NO_STEP, FOR_ASSIGN, FOR_INCREMENT, FOR_DECREMENT, FOR_COMPOUND_ASSIGN };
_Static_assert((int)FOR_NO_STEP==(int)GSC_FOR_NO_STEP&&
               (int)FOR_ASSIGN==(int)GSC_FOR_ASSIGN&&
               (int)FOR_INCREMENT==(int)GSC_FOR_INCREMENT&&
               (int)FOR_DECREMENT==(int)GSC_FOR_DECREMENT&&
               (int)FOR_COMPOUND_ASSIGN==(int)GSC_FOR_COMPOUND_ASSIGN,
               "for inspection kinds must match syntax IR");
/* The syntax IR retains each header clause and the entire compiled body.
 * The instruction at body_start-1 references this record in its dest field.
 * Earlier constructors reject loops; explicit flow mode interprets this record
 * with private per-frame contexts and never rewrites it into jump opcodes. */
typedef struct { int line, init_target, init_expr, cond_expr, step_target,
                 step_expr, step_kind, body_start, body_end;
                 /* Only FOR_COMPOUND_ASSIGN uses step_op (GSC_MUT_ADD..DIV).
                  * No synthetic arithmetic/write instruction is emitted. */
                 int step_op; } ForNode;
/* A syntax-only while records the condition and the complete compiled body.
 * The instruction at body_start-1 references this record in its dest field. */
typedef struct { int line, cond_expr, body_start, body_end; } WhileNode;
/* Separate post-test record, never lowered to WhileNode. */
typedef GscDoWhileInfo DoWhileNode;
/* Case lists use indexes and next links because nested switches interleave
 * their allocations. A body ends at the next sibling label, not at any nested
 * label. No implicit break is inserted; adjacent labels may have empty spans. */
typedef struct { int module, line, selector_expr, instruction, body_start, body_end,
                 first_case, last_case, default_case, case_count; } SwitchNode;
typedef struct { int owner_switch, line, kind, integer, body_start, body_end,
                 next_case; char string[128]; int string_literal; } CaseNode;
typedef struct { char name[64]; int module, argc, start, end, params_capacity;
                 char (*params)[64]; int expr_start, expr_end; int admission;char admission_error[256]; } Func;
/* Syntax-only developer region. Both spans are half-open; statement regions
 * retain code, and file-scope regions retain their enclosed declarations. */
typedef struct { int scope, module, line, col, code_start, code_end,
                 func_start, func_end, expr_start, expr_end; } DevBlock;
/* The full dam closure is larger than the earlier fixture's global static
 * tables. These IR tables grow only as source is successfully compiled; the
 * caps above are explicit refusal boundaries, not preallocated storage. */
#include "gsc_vm_r243_staged_types.h"
struct GscProgram { int r243s_requested;R243SProgramDescriptor r243s_descriptor; Expr *expr; int expr_count, expr_capacity;
                    Instr *code; int code_count, code_capacity;
                    ForNode *loops; int loop_count, loop_capacity;
                    WhileNode *whiles; int while_count, while_capacity;
                    SwitchNode *switches; int switch_count, switch_capacity;
                    CaseNode *cases; int case_count, case_capacity;
                    Func *funcs; int func_count, func_capacity;
                    char (*modules)[MAX_MODULE_PATH]; int module_count, module_capacity;
                    DevBlock *dev_blocks; int dev_count, dev_capacity,dev_code_ordered;
                    StringLiteral *strings; int string_count, string_capacity;
                    int module_loader_mode, event_payload_syntax, event_payload_mode, predicate_syntax, predicate_mode, actor_spawn_syntax, actor_spawn_mode, function_ref_syntax, function_ref_mode;
                    uint64_t function_program_token;
                    int scalar_switch_syntax, scalar_switch_mode, lifetime_syntax, lifetime_mode;
                    int names_syntax, names_mode, music_syntax, music_mode;
                    int resources_syntax, resources_mode;
                    int objectives_syntax, objectives_mode;
                    int console_syntax, console_mode;
                    int hud_syntax, hud_mode;
                    int mission_syntax, mission_mode;
                    int blast_syntax, blast_mode;
                    int quake_syntax, quake_mode;
                    int fx_syntax, fx_mode, loop_fx_selected;
                    int cvars_syntax, cvars_mode;
                    int shellshock_syntax, shellshock_mode;
                    int debug_text_syntax, debug_text_mode;
                    int debug_line_syntax, debug_line_mode;
                    int ambient_syntax, ambient_mode;
                    int precache_item_syntax, precache_item_mode;
                    int save_game_syntax, save_game_mode;
                    int tag_fx_syntax, tag_fx_mode;
                    int native_queries_syntax, native_queries_mode;
                    int precache_string_syntax, precache_string_mode;
                    int spawner_arrays_syntax, spawner_arrays_mode;
                    int randomint_syntax, randomint_mode;
                    int randomfloat_syntax, randomfloat_mode;
                    int keybinding_syntax, keybinding_mode;
                    int computed_randomint_syntax, computed_randomint_mode;
                    int actor_critical_selected,actor_presence_selected,attachment_queries_selected;int actor_drop_selected,native_animation_roots_selected,owned_numeric_delete_selected,flagged_all_restart_selected,flagged_all_selected;int r238_array_value_cow_selected,r239_mixed_scalar_equality_selected;int configured_precache_interval_selected;int r243_thread_start_selected,r243_same_bucket_wait_selected,r243_scripted_prefix_selected,r243_script_state_selected,r243_scripted_start_selected;
                    int delete_transactions_selected,grenade_items_selected,yaw_motion_selected,map_moveto_selected,actor_goal_selected,map_actor_goal_selected,carbine_items_selected;
                    int turrets_selected;int turret_animations_selected,training_rigs_selected;int actor_animations_selected,map_actor_animations_selected;int actor_look_selected,map_actor_look_selected;int random_range_selected;int r232_source_float_selected,map_source_float_selected;int native_quoted_strings_selected;int function_admission_selected;uint64_t function_admission_token;int developer_selected,developer_value_only,map_developer_selected,map_developer_linking;GscDeveloperScriptSelection developer;
                    int endon_selected,waittillmatch_selected,owned_wait_delivery_selected;
                    int bullet_trace_selected;int local_identifier_case_selected;int loop_sound_selected;int sound_notify_selected; int training_opening_selected;int script_sound_selected,player_natives_selected,player_normal_health_selected,player_viewmodel_selected,player_max_ammo_selected,player_stance_permissions_selected,entity_links_selected,entity_operations_selected,spatial_natives_selected,entity_coordinates_selected; int entity_model_methods_selected,spawner_team_method_selected,vehicle_spawn_selected,turret_spawn_selected,turret_precache_selected,vehicle_methods_selected,model_part_selected,weapon_classname_selected,weapon_model_selected,namespace_length_selected,map_animation_length_selected,animation_length_selected,animation_methods_selected,anim_data_selected,animation_refs_selected; int start_origin_selected; int animation_namespace_selected; GscMapAnimationNames map_animation_names; int namespace_start_origin_selected,map_start_pose_selected; int stance_selected,animtime_selected,move_delta_selected;unsigned actor_float_selected;
                    int console_int32_selected; int vector_arithmetic_selected; int large_sources_selected;int vector_math_selected,integer_bitwise_selected,string_int_concat_selected,node_queries_selected,vehicle_nodes_selected,hud_elements_selected,entity_collision_selected,disconnectpaths_selected,ai_arrays_selected;
                    int linked, invalid, entry_fn, requires_animtree, entity_mode, access_mode, event_mode, state_mode, controls_mode, flow_mode, arrays_mode, results_mode;
                    int requires_game_values, requires_receiver_method;
                    int requires_literal_semantics, requires_loop_semantics;
                    int requires_localized_string, requires_flow_semantics;
                    int requires_vector_semantics, requires_anim_ref_semantics;
                    int requires_dynamic_call, requires_empty_array_semantics;
                    int requires_scoped_expr_call, requires_dev_block;
                    int requires_mutation_semantics, requires_cast_semantics;
                    int requires_func_ref_semantics, requires_animtree_value_semantics;
                    int requires_integer_op_semantics, requires_switch_semantics;
                    int requires_raw_string_semantics, requires_thread_expr_semantics;
                    DoWhileNode *do_whiles; int do_while_count, do_while_capacity;
                    int requires_do_while_semantics, requires_local_expr_call;
                    int struct_syntax, struct_mode;
                    int entity_queries_syntax, entity_queries_mode;
                    int dynamic_randomint_syntax, dynamic_randomint_mode;
                    int numeric_randomint_syntax, numeric_randomint_mode;
                    int dynamic_randomfloat_syntax, dynamic_randomfloat_mode;
                    int float_arithmetic_selected,timed_arithmetic_selected,frame_cadence_selected,r237_frame_buckets_selected; char float_arithmetic_module[MAX_MODULE_PATH];
                    GscModuleFloatArithmeticProfile float_arithmetic_profile; };

/* The disk game DLL's isdefined(type 0) path returns false. Keep this
 * distinct from the candidate VM's internal type 6 stack marker. */
enum { V_UNDEFINED, V_NUMBER, V_STRING, V_VECTOR, V_ENTITY, V_LEVEL, V_ARRAY, V_FUNCTION, V_LOCALIZED_STRING, V_STRUCT, V_ANIM, V_ANIMATION, V_NODE, V_HUDELEM, V_ANIMATION_TREE, V_NAMESPACED_ANIMATION, V_VEHICLE_NODE };
typedef struct { uint64_t program, runtime; int module, function; } FunctionIdentity;
typedef struct { int kind, integer; float number; int integer_exact; int32_t integer_value; char str[128];
                 float vector[3]; GscEntityRef entity; GscNodeRef node; GscVehicleNodeRef vehicle_node; GscHudElemRef hud; uint32_t array; FunctionIdentity function;
                 uint32_t object; uint64_t object_token; } Value;
typedef struct { char name[64]; Value value; } Local;
typedef struct { int owner, kind, index, start, end, cond; } FlowLoop;
typedef struct { int fn, pc, local_count; Value self; Local locals[MAX_LOCALS];
                 int loop_count; FlowLoop loops[MAX_CONTROL_DEPTH]; } Frame;
enum { TASK_READY, TASK_SLEEP, TASK_EVENT, TASK_DEAD };
enum { EVENT_LEVEL=0, EVENT_ENTITY, EVENT_STRUCT };
typedef struct ResultsTask ResultsTask;
typedef struct { int scope;uint32_t struct_id;GscEntityRef owner;char event[128]; } EndonSubscription;
typedef struct { int status, depth, wake_ms,cadence_event_resume; uint64_t serial; int r237_enqueued; uint64_t r237_insertion,r243_time_root_incarnation; char event[128];
                 int event_scope; uint32_t event_struct_id; GscEntityRef event_owner; Value initial_self;
                 unsigned host_entry_pins,retained_owner_refs; int context_retired;
                 GscEntityExecutionCertificate certificate;
                 GscMapExecutionCertificate map_certificate;
                 int event_frame, event_count, event_slots[GSC_EVENT_PAYLOAD_CAPACITY];
                 int endon_count;EndonSubscription endon[GSC_ENDON_CAPACITY];
                 int event_match_active,event_filter_count;
                 Value event_filters[GSC_EVENT_PAYLOAD_CAPACITY];
                 uint64_t blast_wait_serial;
                 Frame frames[MAX_FRAMES]; ResultsTask *results; Value *initial_thread_result; } Task;
typedef struct {Task *task;uint64_t serial;GscMapExecutionCertificate map;GscEntityExecutionCertificate entity;} ConfiguredPrecacheNode;
#include "gsc_placed_cohort_private.h"
typedef struct PlacedCohortWitness PlacedCohortWitness;
typedef struct PlacedCohortPost PlacedCohortPost;
static int placed_cohort_private(const GscRuntime*,const void*,size_t);
static void placed_cohort_free(GscRuntime*,PlacedCohortWitness*);
static int placed_cohort_destructing(GscRuntime*);
typedef struct PlacedInitReturnScope {
 GscRuntime*r;Task*root;const GscProgram*program;int fn,bound,popped,busy,completed,checking,now,bucket,*budget,budgetValue;
 uint64_t serial,instance,catalog;GscEntityExecutionCertificate certificate;GscMapExecutionCertificate map,animMap;
 GscPlacedInitReturnHost host;GscActorInitReturnRequest request,seal;
} PlacedInitReturnScope;
typedef struct { int used, owner_kind; GscEntityRef owner; char name[128]; Value value; uint32_t struct_owner;GscVehicleNodeRef vehicle_owner; } StateField;
typedef struct { int kind, number; char string[GSC_ARRAY_KEY_CAPACITY]; } ArrayKey;
typedef struct { int owner; ArrayKey key; Value value; } ArrayEntry;
typedef struct { int count, numeric_count, keybinding; } ArrayContainer;
typedef struct { int expr;uint32_t entry,flags;char name[64],tree[64]; } AnimationBinding;
typedef struct {char tree[64];uint32_t declared_count;uint64_t source_token;} AnimationNamespaceCatalog;
typedef struct {GscAnimationCatalogEntry declaration;uint32_t catalog;} AnimationNamespaceEntry;
typedef struct {int expr,kind;uint32_t object;} AnimationNamespaceAtom;
typedef struct {uint64_t identity,runtime_identity;const GscProgram *program;GscMapExecutionCertificate map_certificate;
 AnimationNamespaceCatalog catalogs[GSC_MAP_ANIMATION_CATALOGS];
 AnimationNamespaceEntry entries[GSC_R233_ANIMATION_NAMESPACE_CAPACITY];
 AnimationNamespaceAtom atoms[GSC_R233_ANIMATION_NAMESPACE_CAPACITY];int entry_count,atom_count;} AnimationNamespace;
typedef struct {
 int used,initial_stop,ordinary_source;GscEntityRef entity;uint64_t actor_serial,runtime,selection,receipt,task_serial,last_frame;
 Task*task;GscEntityExecutionCertificate certificate;GscMapExecutionCertificate map;
} R243ControllerMainOwner;
typedef struct R243TimeRoot R243TimeRoot;
static int r243_stance_vm_private(const void*,size_t);
static int r243_actor_float_vm_private(const void*,size_t);
static int move_delta_private(const void*,size_t);
static int r243_animtime_vm_private(const void*,size_t);
static int r243_command_vm_private(const void*,size_t);
static int r243_critical_vm_private(const void*,size_t);
struct GscRuntime { GscDeveloperScriptSelection developer_admission;int developer_admitted; uint64_t node_script_cache;unsigned node_script_count;struct {char name[64];FunctionIdentity identity;} node_scripts[GSC_NODE_SCRIPT_CAPACITY]; R243AnimTimeHost animtime_host;int animtime_enabled,animtime_busy; R243StanceHost stance_host;int stance_enabled,stance_busy;R243MoveDeltaHost move_delta;int move_delta_enabled,move_delta_busy; R243ActorFloatHost actor_float_host;int actor_float_enabled,actor_float_busy;unsigned actor_float_mask; R243SProfile r243s_profile; const GscProgram *program; GscFogCallback fog; void *user;
                    int endon_enabled,waittillmatch_enabled,owned_wait_delivery_enabled;
                    int started, advancing, now_ms, task_count; uint64_t next_serial;
                    int bootstrap_started,bootstrap_returned,configured_main_attempted;
                    int r237_frame_buckets_enabled,r237_vm_bucket_ms,r237_frame_open,r237_frame_target_ms,r237_start_draining,r237_frame_budget;
                     void *r54_dirty_scope; size_t r54_dirty_scope_bytes; /* opt-in synchronous scratch */
                    uint64_t r237_next_insertion,r237_frame_generation,r237_frame_time_root;
                    R243TimeRoot*r243_time_root;int r243_time_root_teardown;
                    int r243_previous_host_ms,r243_actor_script_scope;
                    int r243_source_step_active,r243_source_failed;uint64_t r243_next_source_dispatch;
                    void*r243_source_dispatch,*r402_source_dispatch,*r243_source_step_owner;size_t r243_source_step_owner_bytes;
                    void*r243_controller_capture;size_t r243_controller_capture_bytes;
                    void*r243_source_step_lease,*r243_source_step_api;size_t r243_source_step_lease_bytes,r243_source_step_api_bytes;
                    GscDeleteTransactionHost delete_transactions;int delete_transactions_enabled;
                    void *delete_transaction_scope;
                    GscAttachmentQueryHost attachment_queries;int attachment_queries_enabled;
                    int float_arithmetic_enabled,map_source_float_enabled,timed_arithmetic_enabled,frame_cadence_enabled,console_int32_enabled;int vector_arithmetic_enabled,vector_math_enabled,integer_bitwise_enabled,string_int_concat_enabled;int cadence_drain_ms;
                    int *execution_budget; unsigned entity_entry_depth; int entity_inheritance_busy;
                    int entity_sequence_busy,entity_prepare_busy,entity_certificate_busy;int placed_init_initial_scope;PlacedInitReturnScope *placed_return;PlacedCohortWitness *placed_cohort;PlacedCohortPost *placed_cohort_post;
                    GscEntityFieldHost entity_fields; int entity_fields_enabled,entity_field_busy;
                    GscLoopSoundHost loop_sound;int loop_sound_enabled;GscSoundNotifyHost sound_notify;int sound_notify_enabled,sound_notify_call_active;GscEntityRef sound_notify_owner; GscScriptSoundHost script_sound; int script_sound_enabled;
                    GscPlayerNativeHost player_natives;int player_natives_enabled;
                    GscYawMotionHost yaw_motion;int yaw_motion_enabled;
                    GscActorGoalHost actor_goal;int actor_goal_enabled;
                    GscTurretHost turrets;int turrets_enabled;GscTurretAnimationHost turret_animations;int turret_animations_enabled;GscActorAnimationHost actor_animations;int actor_animations_enabled,map_actor_animations_enabled;int flagged_all_restart_enabled,flagged_all_enabled;GscActorLookHost actor_look;int actor_look_enabled,map_actor_look_enabled;int random_range_enabled;
                    GscEntityDeleteHost entity_delete;int entity_operations_enabled;
                    GscEntityOriginMethodHost origin_method;int origin_method_enabled,origin_method_busy;
    GscEntityLinkHost entity_links;int entity_links_enabled;
                    GscSpatialNativeHost spatial_natives;int spatial_natives_enabled,entity_coordinates_enabled;
                    GscEntityMethodHost entity_methods; int entity_methods_enabled,entity_method_busy;
                    GscSpawnerTeamMethodHost spawner_team;int spawner_team_enabled;
                    GscActorCriticalHost actor_critical;int actor_critical_enabled;GscActorScriptStateHost r243_script_state;int r243_script_state_enabled;
                    GscActorPresenceHost actor_presence;int actor_presence_enabled,actor_presence_busy;GscActorDropHost actor_drop;int actor_drop_enabled;
                    GscModelPartHost model_part;int model_part_enabled,model_part_busy;
 GscVehicleSpawnHost vehicle_spawn;int vehicle_spawn_enabled,vehicle_spawn_busy;
 GscTurretPrecacheHost turret_precache;int turret_precache_enabled,turret_precache_busy;
 GscTurretSpawnHost turret_spawn;int turret_spawn_enabled,turret_spawn_busy;
 GscBulletTraceHost bullet_trace;int bullet_trace_enabled,bullet_trace_busy;
 GscVehicleMethodHost vehicle_methods;int vehicle_methods_enabled;
    GscWeaponClassnameHost weapon_classname;int weapon_classname_enabled,weapon_classname_busy;
    GscWeaponModelHost weapon_model;int weapon_model_enabled,weapon_model_busy;
                    GscAnimationLengthHost animation_length;int animation_length_enabled,animation_length_busy;
                    GscStartOriginHost start_origin;int start_origin_enabled,start_origin_busy;
                    GscStartOriginHost namespace_start_origin;int namespace_start_origin_enabled,namespace_start_origin_busy;
                    GscAnimationMethodHost animation_methods;int animation_methods_enabled;uint64_t animation_tick_serial;int32_t animation_tick_ms;
                    Value game_array_root; int game_array_enabled;
                    GscMapExecutionCertificate anim_data;int anim_data_enabled;
                    GscMapExecutionCertificate animation_refs;int animation_refs_enabled,animation_ref_count;
                    uint64_t animation_catalog_token;AnimationBinding animation_bindings[GSC_ANIMATION_REFERENCE_CAPACITY];
                    AnimationNamespace animation_namespace;int animation_namespace_enabled,animation_namespace_busy;
                    Task tasks[MAX_THREADS];
                    Task *lifetime_tasks[GSC_LIFETIME_TASK_CAPACITY]; int lifetime_enabled;
                    Task **extra_tasks; int extra_capacity,expand_map_tasks;
                    char error[256];
                    GscNativeRegistry *native_registry;
                    int native_isdefined_import;
                    int native_exp_fog_import, native_cull_fog_import;
                    int native_fog_active, native_fog_line;
                    GscRuntimeNativeStats native_stats;
                    GscRuntimeFogNativeStats fog_native_stats;
                    GscEntityHost entities; int entities_enabled;
                    GscReadOnlyContext readonly_context;
                    int native_readonly_scan;
                    void *readonly_entity_user;
                    int (*readonly_entity_validate)(void *, const GscEntityRef *);
                    int native_getent_import, native_setorigin_import, native_spawn_import;
                    GscEntityAccessHost access; int access_enabled, native_setmodel_import;
                    int events_enabled, state_enabled;
                    GscEntityControlHost controls; int controls_enabled, flow_enabled;
                    int native_hide_import, native_show_import;
                    Task *executing_task;
                    StateField *state_fields;int state_field_capacity;uint32_t state_field_limit;int state_arena_busy;void *state_scratch;size_t state_scratch_bytes;
                    void *r329_cow_scratch;size_t r329_cow_scratch_bytes;
                    GscEntityCollectionHost collections; int arrays_enabled, results_enabled, event_payloads_enabled;
                    int sparse_numeric_arrays_enabled,owned_numeric_delete_enabled,r238_array_value_cow_enabled,r239_mixed_scalar_equality_enabled;int r243_thread_start_enabled,r243_same_bucket_wait_enabled;unsigned r243_thread_start_depth;int r243_scripted_prefix_enabled,r243_scripted_init_fn;GscActorScriptedPrefixHost r243_scripted_prefix;GscActorScriptedStartHost r243_scripted_start;int r243_scripted_start_enabled;GscActorScriptedControllerHost r243_controller;GscControllerSelectWithCleanup r243_select_with_cleanup;int r243_controller_enabled,r243_controller_busy,r243_controller_callback_busy;uint64_t r243_next_main_receipt;R243ControllerMainOwner r243_controller_main[32];GscThreadEntityAuthorityHost r243_thread_entities;int r243_thread_entities_enabled;
                    GscEntityPredicateHost predicates; int predicates_enabled;
                    GscActorSpawnHost actor_spawners; int actor_spawners_enabled, function_refs_enabled;unsigned char function_admission[MAX_FUNCS],verified_function_closure[MAX_FUNCS];uint64_t function_admission_token;
                    Task *actor_spawn_preparing;uint64_t actor_spawn_preparing_serial;
                    GscEntityRef actor_spawn_preparing_source;int actor_spawn_preparing_used;
                    GscEntityExecutionCertificate actor_spawn_issued;int actor_spawn_init_used;const GscEntityScriptEntry *factory_init_entry;uint64_t factory_init_serial;
                    const GscActorSpawnTicket *actor_spawn_finishing;int actor_spawn_finish_published;
                    const GscActorSpawnRequest *actor_spawn_preparing_request;
                    int scalar_switch_enabled, names_enabled, music_enabled; GscMusicHost music;
                    int resources_enabled, resources_closed;uint64_t resources_entry_serial; GscResourceHost resources;
                    int objectives_enabled; GscObjectiveHost objectives;
                    int console_enabled; GscConsoleHost console;
                    int hud_enabled; GscHudHost hud;
                    int mission_enabled; GscMissionHost mission;
                    int blast_enabled; GscBlastHost blast; uint64_t next_blast_wait_serial;
                    int quake_enabled; GscQuakeHost quake;
                    int fx_enabled; GscFxHost fx;
                    int loop_fx_enabled; GscLoopFxHost loop_fx; uint64_t loop_fx_operation; void *loop_fx_scope;
                    int cvars_enabled; GscCvarHost cvars; uint64_t next_cvar_operation;
                    int shellshock_enabled, shellshock_initial_window;
                    GscShellshockHost shellshock; uint64_t next_shellshock_operation;
                    int debug_text_enabled; GscDebugTextHost debug_text; uint64_t next_debug_text_operation;
                    int debug_line_enabled; GscDebugLineHost debug_line; uint64_t next_debug_line_operation;
                    int ambient_enabled; GscAmbientHost ambient; uint64_t next_ambient_operation;
                    int precache_item_enabled, precache_item_initial_window;
                    GscPrecacheItemHost precache_item; uint64_t next_precache_item_operation;
                    int save_game_enabled;GscSaveGameHost save_game;uint64_t next_save_game_operation;
                    GscTagFxHost tag_fx; int tag_fx_enabled; uint64_t next_tag_fx_operation;
                    GscNativeQueryHost native_queries; int native_queries_enabled; uint64_t next_native_query_operation;
                    int configured_precache_interval_enabled,configured_precache_phase,configured_precache_checking;
                    unsigned configured_precache_depth;ConfiguredPrecacheNode configured_precache_chain[33];
                    const GscProgram *configured_precache_program;uint64_t configured_precache_runtime_identity,configured_precache_time_root;
                    int configured_precache_now,configured_precache_bucket,configured_precache_anim_enabled,configured_precache_entry_fn;int *configured_precache_budget;GscMapExecutionCertificate configured_precache_anim;
                    GscPrecacheStringHost precache_string; int precache_string_enabled, precache_string_initial_window; uint64_t next_precache_string_operation;
                    GscSpawnerArrayHost spawner_arrays; int spawner_arrays_enabled;
                    GscRandomIntHost randomint; int randomint_enabled, computed_randomint_enabled, dynamic_randomint_enabled, numeric_randomint_enabled; uint64_t next_randomint_operation;
                    GscRandomFloatHost randomfloat; int randomfloat_enabled, dynamic_randomfloat_enabled; uint64_t next_randomfloat_operation;
                    GscKeyBindingHost keybinding; int keybinding_enabled, keybinding_busy; uint64_t next_keybinding_operation;
                    uint64_t array_token;
                    int structs_enabled,training_tower_bootstrap_waited; uint32_t struct_count;
                    GscEntityQueryHost entity_queries; int entity_queries_enabled;
                    GscNodeQueryHost node_queries; int node_queries_enabled,node_query_busy;
                    GscVehicleNodeHost vehicle_nodes;int vehicle_nodes_enabled,vehicle_node_busy;
                    GscAiArrayHost ai_arrays;int ai_arrays_enabled,ai_array_busy;
                    GscEntityCollisionHost entity_collision;int entity_collision_enabled;GscEntityWorldAction connectpaths,disconnectpaths;
                    GscHudElemHost hud_elements;int hud_elements_enabled,hud_elements_busy,hud_elements_retired;
                    uint64_t hud_elements_operation,hud_elements_epoch;GscHudElemRef hud_refs[GSC_HUDELEM_CAPACITY];int hud_live[GSC_HUDELEM_CAPACITY];
                    uint64_t map_model_start_stamp; Task *array_dispatch_root; int array_next_scan, array_next_entry_scan;
                    int array_count, array_entry_count;
                    ArrayContainer arrays[GSC_ARRAY_CONTAINER_CAPACITY];
                    ArrayEntry array_entries[GSC_ARRAY_ENTRY_CAPACITY]; int bsp_construction_only,bsp_promoted; };

static int r237_busy(const GscRuntime *);
static int r237_queue_available(GscRuntime *,int,int);
static int r237_queue_offer(GscRuntime *,Task *,int,int);
static int r237_timed_wait(GscRuntime *,Task *,const Value *,int);
static int r237_bootstrap_drain(GscRuntime *);

/* This is a deliberately tiny, case-sensitive fixture registry, not a list
 * of CoD1 natives. Every registered name has one calling context, exact
 * arity, accepted value kinds, result kind and effect. Unregistered names
 * remain unsupported even when their spelling appears in the source corpus. */
typedef struct Builtin Builtin;
enum { B_STATEMENT, B_EXPRESSION };
enum { B_RET_VOID=-1, B_EFFECT_PURE, B_EFFECT_EXP_FOG, B_EFFECT_CULL_FOG };
#define B_VALUE(kind) (1u << (kind))
static int rfail(GscRuntime*,int,const char*,...);
static void configured_precache_close(GscRuntime*);
static int Cod1ModelStringOverlap(const void *,size_t,const void *,size_t);
static int entity_ref_equal(const GscEntityRef *,const GscEntityRef *);
static int actor_animation_binding(GscRuntime *,const Value *,int,GscAnimationLengthRequest *);
#if defined(_MSC_VER)
#define GSC_STATE_INLINE __forceinline
#else
#define GSC_STATE_INLINE inline
#endif
/* Includes unused allocated cells: moving state to heap must not turn private
 * VM memory into legal host output/error storage. Allocator reentry poisons. */
static GSC_STATE_INLINE int runtime_private_overlap(const GscRuntime*r,const void*p,size_t bytes){
 if(!r||!bytes)return 0;
 if(r->state_arena_busy){rfail((GscRuntime*)r,0,"state arena allocator reentered");return 1;}
 return Cod1ModelStringOverlap(p,bytes,r,sizeof(*r))||
  (r->r54_dirty_scope&&Cod1ModelStringOverlap(p,bytes,r->r54_dirty_scope,r->r54_dirty_scope_bytes))||
  (r->extra_tasks && Cod1ModelStringOverlap(p,bytes,r->extra_tasks,(size_t)r->extra_capacity*sizeof(*r->extra_tasks)))||
  (r->state_fields&&Cod1ModelStringOverlap(p,bytes,r->state_fields,(size_t)r->state_field_capacity*sizeof(*r->state_fields)))||
  (r->state_scratch&&Cod1ModelStringOverlap(p,bytes,r->state_scratch,r->state_scratch_bytes))||
  (r->r329_cow_scratch&&Cod1ModelStringOverlap(p,bytes,r->r329_cow_scratch,r->r329_cow_scratch_bytes));
}
struct Builtin {
    const char *name;
    int context, arity, result_kind, effect;
    unsigned accepted[MAX_PARAMS];
    int (*statement)(GscRuntime *,const Builtin *,const Value *,int);
    int (*expression)(GscRuntime *,const Builtin *,const Value *,int,Value *);
};
static int builtin_fog(GscRuntime *,const Builtin *,const Value *,int);
static int builtin_isdefined(GscRuntime *,const Builtin *,const Value *,int,Value *);
static const Builtin builtins[]={
    {"setExpFog",B_STATEMENT,5,B_RET_VOID,B_EFFECT_EXP_FOG,
     {B_VALUE(V_NUMBER),B_VALUE(V_NUMBER),B_VALUE(V_NUMBER),
      B_VALUE(V_NUMBER),B_VALUE(V_NUMBER)},builtin_fog,NULL},
    {"setCullFog",B_STATEMENT,6,B_RET_VOID,B_EFFECT_CULL_FOG,
     {B_VALUE(V_NUMBER),B_VALUE(V_NUMBER),B_VALUE(V_NUMBER),
      B_VALUE(V_NUMBER),B_VALUE(V_NUMBER),B_VALUE(V_NUMBER)},builtin_fog,NULL},
    {"isdefined",B_EXPRESSION,1,V_NUMBER,B_EFFECT_PURE,
     {B_VALUE(V_UNDEFINED)|B_VALUE(V_NUMBER)|B_VALUE(V_STRING)|
      B_VALUE(V_VECTOR)|B_VALUE(V_ENTITY)},
     NULL,builtin_isdefined}
};
enum { BUILTIN_COUNT=(int)(sizeof(builtins)/sizeof(builtins[0])) };
/* R107 folds only callable names in its selected mode. The original lexer
 * and script-handle path lower identifiers before exact symbol/native lookup.
 * Variables, fields, events and string values retain earlier mode semantics. */
static int callable_name_equal(const GscProgram *g,const char *a,const char *b) {
    unsigned char x,y;
    if(!g->names_syntax)return !strcmp(a,b);
    do {
        x=(unsigned char)*a++;y=(unsigned char)*b++;
        if(x>='A'&&x<='Z')x=(unsigned char)(x+('a'-'A'));
        if(y>='A'&&y<='Z')y=(unsigned char)(y+('a'-'A'));
        if(x!=y)return 0;
    } while(x);
    return 1;
}
static int find_func(const GscProgram *,int,const char *);
static int find_builtin(const char *name) {
    int n;
    for(n=0;n<BUILTIN_COUNT;n++)if(!strcmp(name,builtins[n].name))return n;
    return -1;
}
/* Only the selected R91 policy accepts the authored utility spelling. Exact
 * local declarations still win over this additional spelling. */
static int program_builtin(const GscProgram *g,const char *name,int owner) {
    int builtin=find_builtin(name),n;
    if(g->names_syntax) {
        if(owner>=0&&find_func(g,owner,name)>=0)return -1;
        for(n=0;n<BUILTIN_COUNT;n++)if(callable_name_equal(g,name,builtins[n].name))return n;
        return -1;
    }
    if(builtin<0&&g->controls_mode&&!strcmp(name,"isDefined")&&
       (owner<0||find_func(g,owner,name)<0))builtin=find_builtin("isdefined");
    return builtin;
}
static int control_field_equal(const char *a,const char *b) {
    unsigned char ca,cb;
    do {
        ca=(unsigned char)*a++;cb=(unsigned char)*b++;
        if(ca>='A'&&ca<='Z')ca=(unsigned char)(ca+('a'-'A'));
        if(cb>='A'&&cb<='Z')cb=(unsigned char)(cb+('a'-'A'));
        if(ca!=cb)return 0;
    } while(ca);
    return 1;
}
/* Fixed R84 descriptors are separate from both the legacy builtin table and
 * private registry indexes. The old parser/linker does not reserve these names. */
enum { ENTITY_GLOBAL_GETENT=0, ENTITY_GLOBAL_SPAWN=1, ENTITY_GLOBAL_GETENTARRAY=2,
       ENTITY_GLOBAL_ISALIVE=3, ENTITY_GLOBAL_ISSENTIENT=4, ENTITY_GLOBAL_ISAI=5,
       ENTITY_METHOD_SETORIGIN=0, ENTITY_METHOD_SETMODEL=1,
       ENTITY_METHOD_HIDE=2, ENTITY_METHOD_SHOW=3, ENTITY_METHOD_DOSPAWN=4, ENTITY_METHOD_GETNUMBER=5,
       ENTITY_METHOD_ATTACH=6, ENTITY_METHOD_DETACH=7, ENTITY_METHOD_DETACH_ALL=8,ENTITY_METHOD_SETSPAWNERTEAM=9,ENTITY_METHOD_SETANIMKNOB=10,ENTITY_METHOD_SETANIMKNOBALL=11,ENTITY_METHOD_SETFLAGGEDANIMKNOBALL=12,
       ENTITY_METHOD_ENTERCRITICAL=13,ENTITY_METHOD_LEAVECRITICAL=14,ENTITY_METHOD_ISCRITICAL=15,ENTITY_METHOD_PLAYSOUND=16,ENTITY_METHOD_PLAYLOOPSOUND=17,ENTITY_METHOD_STOPLOOPSOUND=18,ENTITY_METHOD_STALINGRADSPAWN=19 };
static int actor_spawn_native_descriptor(int d){return d==ENTITY_METHOD_DOSPAWN||d==ENTITY_METHOD_STALINGRADSPAWN;}
static const char*actor_spawn_native_name(int d){return d==ENTITY_METHOD_STALINGRADSPAWN?"stalingradspawn":"dospawn";}
static int music_descriptor(const GscProgram *g,const char *name) {
    if(!g->music_mode)return -1;
    if(callable_name_equal(g,name,"musicplay"))return GSC_MUSIC_PLAY;
    if(callable_name_equal(g,name,"musicstop"))return GSC_MUSIC_STOP;
    return -1;
}
static int music_argc_valid(int kind,int argc) {
    return kind==GSC_MUSIC_PLAY?(argc>=1&&argc<=MAX_PARAMS):
        kind==GSC_MUSIC_STOP?(argc==0||argc==1):0;
}
/* Private negative instruction targets are separate from frozen builtins and
 * music IDs. Public native registry layouts and import indexes stay unchanged. */
#define RESOURCE_TARGET_BASE (BUILTIN_COUNT+GSC_MUSIC_STOP)
static int resource_descriptor(const GscProgram *g,const char *name) {
    if(!g->resources_mode)return -1;
    if(callable_name_equal(g,name,"precacheshader"))return GSC_RESOURCE_SHADER;
    if(callable_name_equal(g,name,"precachemodel"))return GSC_RESOURCE_MODEL;
    return -1;
}
static int resource_argc_valid(int kind,int argc) {
    return (kind==GSC_RESOURCE_SHADER||kind==GSC_RESOURCE_MODEL)&&argc>=1&&argc<=MAX_PARAMS;
}
#define OBJECTIVE_TARGET_BASE (RESOURCE_TARGET_BASE+GSC_RESOURCE_MODEL)
static int objective_descriptor(const GscProgram *g,const char *name) {
    static const char *const names[]={"objective_add","objective_delete","objective_state",
        "objective_string","objective_string_nomessage","objective_position",
        "objective_current","objective_ring"};int n;
    if(!g->objectives_mode)return -1;
    for(n=0;n<8;n++)if(callable_name_equal(g,name,names[n]))return n+1;
    return -1;
}
static int objective_argc_valid(int kind,int argc) {
    int minimum=kind==GSC_OBJECTIVE_ADD||kind==GSC_OBJECTIVE_STATE||kind==GSC_OBJECTIVE_POSITION?2:
        kind==GSC_OBJECTIVE_CURRENT?0:1;
    return kind>=GSC_OBJECTIVE_ADD&&kind<=GSC_OBJECTIVE_RING&&argc>=minimum&&argc<=MAX_PARAMS;
}
#define CONSOLE_TARGET_BASE (OBJECTIVE_TARGET_BASE+GSC_OBJECTIVE_RING)
static int console_descriptor(const GscProgram *g,const char *name) {
    if(!g->console_mode)return -1;
    if(callable_name_equal(g,name,"print"))return GSC_CONSOLE_PRINT;
    if(callable_name_equal(g,name,"println"))return GSC_CONSOLE_PRINTLN;
    return -1;
}
static unsigned console_static_issues(const GscProgram *g,int kind,const int *args,int argc) {
    (void)g;(void)args;
    return (kind==GSC_CONSOLE_PRINT||kind==GSC_CONSOLE_PRINTLN)&&argc>=0&&argc<=MAX_PARAMS?
        0u:GSC_LINK_ISSUE_BUILTIN_ARITY;
}
#define HUD_TARGET_BASE (CONSOLE_TARGET_BASE+GSC_CONSOLE_PRINTLN)
static int hud_descriptor(const GscProgram *g,const char *name) {
    if(!g->hud_mode)return -1;
    if(callable_name_equal(g,name,"iprintln"))return GSC_HUD_PRINT;
    if(callable_name_equal(g,name,"iprintlnbold"))return GSC_HUD_BOLD;
    return -1;
}
#define MISSION_TARGET_BASE (HUD_TARGET_BASE+GSC_HUD_BOLD)
static int mission_descriptor(const GscProgram *g,const char *name) {
    if(!g->mission_mode)return -1;
    if(callable_name_equal(g,name,"missionsuccess"))return GSC_MISSION_SUCCESS;
    if(callable_name_equal(g,name,"missionfailed"))return GSC_MISSION_FAILED;
    if(callable_name_equal(g,name,"changelevel"))return GSC_MISSION_CHANGELEVEL;
    return -1;
}
static unsigned mission_static_issues(const GscProgram *g,int kind,const int *args,int argc) {
    (void)g;(void)args;
    /* Getter validation follows the native gate, including zero-argument
     * success and ignored failed arguments. MAX_PARAMS is bounded port policy. */
    return argc>=0&&argc<=MAX_PARAMS&&
        (((kind==GSC_MISSION_SUCCESS||kind==GSC_MISSION_CHANGELEVEL)&&argc<=2)||kind==GSC_MISSION_FAILED)?
        0u:GSC_LINK_ISSUE_BUILTIN_ARITY;
}
#define BLAST_TARGET_BASE (MISSION_TARGET_BASE+GSC_MISSION_CHANGELEVEL)
#define QUAKE_TARGET_BASE (BLAST_TARGET_BASE+GSC_BLAST_RADIUS)
#define FX_TARGET_BASE (QUAKE_TARGET_BASE+GSC_QUAKE_EARTHQUAKE)
#define CVAR_TARGET_BASE (FX_TARGET_BASE+GSC_FX_LOOP)
#define SHELLSHOCK_TARGET_BASE (CVAR_TARGET_BASE+GSC_CVAR_SETVAR)
#define DEBUG_TEXT_TARGET_BASE (SHELLSHOCK_TARGET_BASE+GSC_SHELLSHOCK_PRECACHE)
#define DEBUG_LINE_TARGET_BASE (DEBUG_TEXT_TARGET_BASE+GSC_DEBUG_TEXT_PRINT3D)
#define AMBIENT_TARGET_BASE (DEBUG_LINE_TARGET_BASE+GSC_DEBUG_LINE_DRAW)
#define PRECACHE_ITEM_TARGET_BASE (AMBIENT_TARGET_BASE+GSC_AMBIENT_STOP)
#define SAVE_GAME_TARGET_BASE (PRECACHE_ITEM_TARGET_BASE+GSC_PRECACHE_ITEM_REGISTER)
#define TAG_FX_TARGET_BASE (SAVE_GAME_TARGET_BASE+GSC_SAVE_GAME_REQUEST)
#define WEAPON_MODEL_STATEMENT_TARGET 8192
#define VEHICLE_SPAWN_STATEMENT_TARGET 8196
#define TURRET_PRECACHE_STATEMENT_TARGET 8199
#define TURRET_SPAWN_STATEMENT_TARGET 8198
#define BULLET_TRACE_STATEMENT_TARGET 8197
#define MODEL_PART_STATEMENT_TARGET 8195
#define WEAPON_CLASSNAME_STATEMENT_TARGET 8194
#define ANIMATION_LENGTH_STATEMENT_TARGET 8193
#define NATIVE_QUERY_TARGET_BASE (TAG_FX_TARGET_BASE+GSC_TAG_FX_PLAY)
#define SPAWNER_ARRAY_TARGET_BASE (NATIVE_QUERY_TARGET_BASE+GSC_QUERY_DIFFICULTY)
#define RANDOMINT_TARGET_BASE (SPAWNER_ARRAY_TARGET_BASE+GSC_SPAWNER_ARRAY_ALL)
#define RANDOMFLOAT_TARGET_BASE (RANDOMINT_TARGET_BASE+GSC_RANDOMINT_DRAW)
#define KEYBINDING_TARGET_BASE (RANDOMFLOAT_TARGET_BASE+GSC_RANDOMFLOAT_DRAW)
#define STRUCT_TARGET_BASE (KEYBINDING_TARGET_BASE+GSC_KEYBINDING_QUERY)
#include "gsc_struct_descriptor.inc"
#define ENTITY_QUERY_TARGET_BASE (STRUCT_TARGET_BASE+GSC_STRUCT_SPAWN)
#include "gsc_entity_queries_descriptor.inc"
#include "gsc_keybinding_payload.inc"
static int string_literal_decoded(const GscProgram *g,const StringLiteral *s) {
    return s&&(s->decode_status==GSC_STRING_FIXTURE_DECODED||
        (g&&g->native_quoted_strings_selected&&s->decode_status==GSC_STRING_NATIVE_DECODED));
}
/* Literal spellings retain their existing byte-domain checks. Computed
 * values are evaluated once by the normal VM and must pass the same bounded
 * ordinary ASCII string getter before QUERY. No extra coercions are added. */
static int keybinding_descriptor(const GscProgram *g,const char *name,const int *args,int argc) {
    const Expr *e;const StringLiteral *s;
    if(!g||!g->keybinding_mode||!callable_name_equal(g,name,"getkeybinding"))return -1;
    if(argc==0)return GSC_KEYBINDING_QUERY;
    if(argc!=1||!args||args[0]<0||args[0]>=g->expr_count)return -1;
    e=&g->expr[args[0]];
    if(e->kind!=E_STRING)return GSC_KEYBINDING_QUERY;
    if(e->a<0||e->a>=g->string_count||!keybinding_command_valid(e->name))return -1;
    s=&g->strings[e->a];
    if(s->localized||!string_literal_decoded(g,s)||s->unknown_escape_count||
       s->raw_length!=(int)strlen(e->name)||memcmp(s->raw,e->name,(size_t)s->raw_length))return -1;
    return GSC_KEYBINDING_QUERY;
}

#include "gsc_randomfloat_payload.inc"
#include "gsc_float_arithmetic.inc"
/* Exact decimal token proof avoids inheriting a binding from strtof rounding.
 * Digits remain within the existing 127-character numeric-token limit. Decimal
 * division is integer-only; no locale, rounding or general cast policy is used. */
static int randomfloat_decimal_divide(char *digits,int *length,int divisor) {
    int n,remainder=0,start=0;
    for(n=0;n<*length;n++) {
        int value=remainder*10+digits[n]-'0';
        digits[n]=(char)('0'+value/divisor);remainder=value%divisor;
    }
    if(remainder)return 0;
    while(start+1<*length && digits[start]=='0')start++;
    if(start){memmove(digits,digits+start,(size_t)(*length-start));*length-=start;}
    return 1;
}
static int randomfloat_decimal_bits(const char *text,uint32_t *bits) {
    char digits[128];int length=0,fraction=0,dot=0,exp_sign=1,decimal_exp=0;
    int seen=0,exp_seen=0,start=0,odd=0,e,top=0,n;uint32_t sig;
    const char *p=text;
    if(!text||!bits)return 0;
    while(*p && *p!='e' && *p!='E') {
        if(*p=='.'){if(dot)return 0;dot=1;p++;continue;}
        if(*p<'0'||*p>'9'||length>=127)return 0;
        digits[length++]=*p++;seen=1;if(dot)fraction++;
    }
    if(!seen)return 0;
    if(*p) {
        p++;if(*p=='+'||*p=='-'){if(*p=='-')exp_sign=-1;p++;}
        while(*p) {
            if(*p<'0'||*p>'9'||decimal_exp>1024)return 0;
            decimal_exp=decimal_exp*10+*p++-'0';exp_seen=1;
        }
        if(!exp_seen)return 0;
    }
    while(start+1<length && digits[start]=='0')start++;
    if(start){memmove(digits,digits+start,(size_t)(length-start));length-=start;}
    if(length==1&&digits[0]=='0'){*bits=0;return 1;}
    decimal_exp=decimal_exp*exp_sign-fraction;e=decimal_exp;
    if(decimal_exp<0) {
        for(n=0;n<-decimal_exp;n++)if(!randomfloat_decimal_divide(digits,&length,5))return 0;
    }
    while(!((digits[length-1]-'0')&1)) {
        if(!randomfloat_decimal_divide(digits,&length,2))return 0;e++;
    }
    if(length>3)return 0;
    for(n=0;n<length;n++)odd=odd*10+digits[n]-'0';
    if(decimal_exp>0) {
        for(n=0;n<decimal_exp;n++){if(odd>102)return 0;odd*=5;}
    }
    if(odd<1||odd>511)return 0;
    n=odd;while(n>>1){n>>=1;top++;}
    if(top+e < -111 || top+e > 127)return 0;
    sig=(uint32_t)odd<<(23-top);
    *bits=((uint32_t)(top+e+127)<<23)|(sig&UINT32_C(0x7fffff));
    return randomfloat_decode_bound(*bits,NULL,NULL);
}
static int randomfloat_literal_bits(const GscProgram *g,int index,uint32_t *out) {
    const Expr *e;uint32_t bits,parsed;int negate=0;
    if(!g||!out||index<0||index>=g->expr_count)return 0;
    e=&g->expr[index];
    if(e->kind==E_UNARY && e->op=='-' && e->a>=0 && e->a<g->expr_count) {
        negate=1;e=&g->expr[e->a];
    }
    if(e->kind!=E_NUMBER)return 0;
    if(e->integer==1) {
        int32_t value=e->integer_value;
        if(negate)value=value?-(value):0;
        if(!randomfloat_integer_bits(value,&bits))return 0;
    } else {
        if(e->integer!=0 || !randomfloat_decimal_bits(e->name,&bits))return 0;
        memcpy(&parsed,&e->number,sizeof(parsed));
        if(parsed!=bits)return 0;
        if(negate)bits^=UINT32_C(0x80000000);
    }
    *out=bits;return 1;
}
static int bullet_trace_descriptor(const GscProgram*g,const char*n){return g->bullet_trace_selected&&callable_name_equal(g,n,"bulletTrace");}
static int vehicle_spawn_descriptor(const GscProgram*g,const char*n){return g->vehicle_spawn_selected&&callable_name_equal(g,n,"spawnVehicle");}
static int turret_precache_descriptor(const GscProgram*g,const char*n){return g->turret_precache_selected&&callable_name_equal(g,n,"precacheTurret");}
static int turret_spawn_descriptor(const GscProgram*g,const char*n){return g->turret_spawn_selected&&callable_name_equal(g,n,"spawnTurret");}
static int model_part_descriptor(const GscProgram*g,const char*n){return g->model_part_selected&&callable_name_equal(g,n,"getPartName");}
static int weapon_classname_descriptor(const GscProgram*g,const char*n){return g->weapon_classname_selected&&callable_name_equal(g,n,"getWeaponClassname");}
static int weapon_model_descriptor(const GscProgram *g,const char *name) { return g->weapon_model_selected&&callable_name_equal(g,name,"getWeaponModel"); }
static int animation_length_descriptor(const GscProgram *g,const char *name) { return g->animation_length_selected&&callable_name_equal(g,name,"getanimlength"); }
static int native_query_descriptor(const GscProgram *,const char *);
static int float_arithmetic_literal_bits(const GscProgram *,int,uint32_t *);
#include "gsc_dynamic_randomfloat_descriptor.inc"
#include "gsc_r232_random_range_payload.inc"
#include "gsc_r232_source_float_payload.inc"
#define R232_RANDOM_RANGE_STATEMENT_TARGET 16000
static int random_range_descriptor(const GscProgram*g,const char*name){return g&&g->random_range_selected&&callable_name_equal(g,name,"randomFloatRange");}
static int known_value_kind(const GscProgram *,int);
static unsigned random_range_issues(const GscProgram*g,const int*args,int argc){int i;if(argc!=2||!args)return GSC_LINK_ISSUE_BUILTIN_ARITY;for(i=0;i<2;i++){int k=known_value_kind(g,args[i]);if(k>=0&&k!=V_NUMBER)return GSC_LINK_ISSUE_BUILTIN_LITERAL;}return 0;}
static int randomfloat_descriptor(const GscProgram *g,const char *name,
    const int *args,int argc) {
    uint32_t bits;
    if(!g||!g->randomfloat_mode||!callable_name_equal(g,name,"randomfloat"))return -1;
    if(g->dynamic_randomfloat_mode) {
        if(argc<0||argc>MAX_PARAMS)return -1;
        if(argc==0)return GSC_RANDOMFLOAT_DRAW;
        return args&&dynamic_randomfloat_argument(g,args[0],0)?GSC_RANDOMFLOAT_DRAW:-1;
    }
    if(argc==0)return GSC_RANDOMFLOAT_DRAW;
    return argc==1&&args&&randomfloat_literal_bits(g,args[0],&bits)?GSC_RANDOMFLOAT_DRAW:-1;
}
static int float_arithmetic_literal_bits(const GscProgram *g,int index,uint32_t *bits) {
    const Expr *e;
    if(!g||index<0||index>=g->expr_count)return 0;
    e=&g->expr[index];
    if(e->kind!=E_NUMBER||!e->float_arithmetic)return 0;
    if(e->integer){
        if(e->float_arithmetic==2&&e->integer==1&&e->integer_value>=-16777216&&e->integer_value<=16777216){
            float exact=(float)e->integer_value;memcpy(bits,&exact,4);return r232_source_float_bits(*bits);
        }
        return randomfloat_literal_bits(g,index,bits);
    }
    if(e->float_arithmetic==2){memcpy(bits,&e->number,4);return r232_source_float_bits(*bits);}
    if(strcmp(e->name,"0.4")&&strcmp(e->name,"0.8"))return randomfloat_literal_bits(g,index,bits);
    return float_arithmetic_decimal_nearest(e->name,bits);
}
/* Per-site admission is a syntax/type proof for this selected bounded lane.
 * No field/identifier/arithmetic expression inherits the randomint binding. */
static int randomint_literal_int32(const GscProgram *g,int index) {
    const Expr *e;
    if(!g || index<0 || index>=g->expr_count)return 0;
    e=&g->expr[index];
    if(e->kind==E_NUMBER)return e->integer==1;
    if(e->kind!=E_UNARY || e->op!='-' ||
       e->a<0 || e->a>=g->expr_count)return 0;
    e=&g->expr[e->a];return e->kind==E_NUMBER && e->integer==1;
}
static int computed_randomint_bound(const GscProgram *,int,int32_t *,int32_t *);
/* R138 flags use the parser's signed integer payload, never its float cache.
 * This lane admits integer literals (including unary minus), not arbitrary
 * float/string GetInt coercions or computed flag expressions. */
static int entity_spawn_flags_literal(const GscProgram *g,int index,int32_t *out) {
    const Expr *e;int negate=0;int32_t value;
    if(!g||!out||index<0||index>=g->expr_count)return 0;
    e=&g->expr[index];
    if(e->kind==E_UNARY&&e->op=='-'&&e->a>=0&&e->a<g->expr_count) {
        negate=1;e=&g->expr[e->a];
    }
    if(e->kind!=E_NUMBER||e->integer!=1)return 0;
    value=e->integer_value;
    if(negate){if(value==INT32_MIN)return 0;value=-value;}
    *out=value;return 1;
}
int gsc_program_select_r222_carbine_items(GscProgram *g) {
 if(!g||g->invalid||g->linked||g->module_count||g->carbine_items_selected||g->grenade_items_selected)return 0;
 g->carbine_items_selected=1;return 1;
}
static int entity_spawn_flags_class(const GscProgram *g,int index) {
    const Expr *e;
    if(!g||index<0||index>=g->expr_count)return 0;e=&g->expr[index];
    /* Same currently implemented class domain as the production GAME host.
     * Item/actor classes remain held until their constructors are supported. */
    return e->kind==E_STRING&&(!strcmp(e->name,"script_origin")||
                              !strcmp(e->name,"script_model")||
        (g->carbine_items_selected&&!strcmp(e->name,"weapon_m1carbine"))||
        ((g->grenade_items_selected || g->carbine_items_selected==2)&&(!strcmp(e->name,"weapon_fraggrenade")||
         !strcmp(e->name,"weapon_stielhandgranate")||
         !strcmp(e->name,"item_ammo_stielhandgranate_open")||
         !strcmp(e->name,"item_ammo_stielhandgranate_closed"))));
}
static int native_query_descriptor(const GscProgram *,const char *);
#include "gsc_dynamic_randomint_descriptor.inc"
static int randomint_descriptor(const GscProgram *g,const char *name,
    const int *args,int argc) {
    if(!g || !g->randomint_mode || !callable_name_equal(g,name,"randomint"))return -1;
    if(g->dynamic_randomint_mode) {
        if(argc<0||argc>MAX_PARAMS)return -1;
        if(argc==0)return GSC_RANDOMINT_DRAW;
        return args&&dynamic_randomint_argument(g,args[0],0)?GSC_RANDOMINT_DRAW:-1;
    }
    if(argc==0)return GSC_RANDOMINT_DRAW;
    return argc==1 && args && (randomint_literal_int32(g,args[0])||
        (g->computed_randomint_mode&&computed_randomint_bound(g,args[0],NULL,NULL)))?GSC_RANDOMINT_DRAW:-1;
}
static int spawner_array_descriptor(const GscProgram *g,const char *name) {
    return g->spawner_arrays_mode&&callable_name_equal(g,name,"getspawnerarray")?
        GSC_SPAWNER_ARRAY_ALL:-1;
}
static unsigned spawner_array_static_issues(int kind,int argc) {
    return kind==GSC_SPAWNER_ARRAY_ALL&&argc==0?0u:GSC_LINK_ISSUE_BUILTIN_ARITY;
}
static int native_query_descriptor(const GscProgram *g,const char *name) {
    if(!g->native_queries_mode)return -1;
    if(callable_name_equal(g,name,"getcvar"))return GSC_QUERY_CVAR_STRING;
    if(callable_name_equal(g,name,"getcvarint"))return GSC_QUERY_CVAR_INTEGER;
    if(callable_name_equal(g,name,"getcvarfloat"))return GSC_QUERY_CVAR_FLOAT;
    if(g->precache_string_mode&&callable_name_equal(g,name,"precachestring"))return GSC_QUERY_PRECACHE_STRING;
    if(g->precache_string_mode&&callable_name_equal(g,name,"gettime"))return GSC_QUERY_LEVEL_TIME;
    if(g->struct_mode&&callable_name_equal(g,name,"getdifficulty"))return GSC_QUERY_DIFFICULTY;
    return -1;
}
static unsigned native_query_static_issues(const GscProgram *g,int kind,const int *args,int argc) {
    (void)args;
    return kind>=GSC_QUERY_CVAR_STRING&&
        (kind<=GSC_QUERY_CVAR_FLOAT ||
         ((kind==GSC_QUERY_PRECACHE_STRING||kind==GSC_QUERY_LEVEL_TIME)&&g&&g->precache_string_mode) ||
         (kind==GSC_QUERY_DIFFICULTY&&g&&g->struct_mode))&&
        argc>=0&&argc<=MAX_PARAMS?0u:GSC_LINK_ISSUE_BUILTIN_ARITY;
}
static int blast_descriptor(const GscProgram *g,const char *name) {
    return g->blast_mode&&callable_name_equal(g,name,"radiusDamage")?GSC_BLAST_RADIUS:-1;
}
static unsigned blast_static_issues(const GscProgram *g,int kind,const int *args,int argc) {
    (void)g;(void)args;
    /* Original wrapper has no count/type guard; ordered getters refuse at use.
     * MAX_PARAMS alone is structural bounded parser policy. */
    return kind==GSC_BLAST_RADIUS&&argc>=0&&argc<=MAX_PARAMS?0u:GSC_LINK_ISSUE_BUILTIN_ARITY;
}
static unsigned hud_static_issues(const GscProgram *,int,const int *,int);
static unsigned objective_static_issues(const GscProgram *,int,const int *,int);
static int quake_descriptor(const GscProgram *g,const char *name) {
    return g->quake_mode&&callable_name_equal(g,name,"earthquake")?GSC_QUAKE_EARTHQUAKE:-1;
}
static unsigned quake_static_issues(const GscProgram *g,int kind,const int *args,int argc) {
    (void)g;(void)args;
    return kind==GSC_QUAKE_EARTHQUAKE&&argc>=0&&argc<=MAX_PARAMS?0u:GSC_LINK_ISSUE_BUILTIN_ARITY;
}
static int fx_descriptor(const GscProgram *g,const char *name) {
    if(!g->fx_mode)return -1;
    if(callable_name_equal(g,name,"loadfx"))return GSC_FX_LOAD;
    if(callable_name_equal(g,name,"playfx"))return GSC_FX_PLAY;
    if(g->loop_fx_selected&&callable_name_equal(g,name,"playLoopedFx"))return GSC_FX_LOOP;
    return -1;
}
static unsigned fx_static_issues(const GscProgram *g,int kind,const int *args,int argc) {
    (void)g;(void)args;
    return argc>=0&&argc<=MAX_PARAMS&&
        (kind==GSC_FX_LOAD||(kind==GSC_FX_PLAY&&argc>=2&&argc<=3)||
         (g->loop_fx_selected&&kind==GSC_FX_LOOP&&argc>=3&&argc<=5))?
        0u:GSC_LINK_ISSUE_BUILTIN_ARITY;
}
static int cvar_descriptor(const GscProgram *g,const char *name) {
    return g->cvars_mode&&callable_name_equal(g,name,"setcvar")?GSC_CVAR_SETVAR:-1;
}
static unsigned cvar_static_issues(const GscProgram *g,int kind,const int *args,int argc) {
    (void)g;(void)args;
    /* No original argument-count guard. Missing getters fail at runtime. */
    return kind==GSC_CVAR_SETVAR&&argc>=0&&argc<=MAX_PARAMS?0u:GSC_LINK_ISSUE_BUILTIN_ARITY;
}
static int shellshock_descriptor(const GscProgram *g,const char *name) {
    return g->shellshock_mode&&callable_name_equal(g,name,"precacheshellshock")?GSC_SHELLSHOCK_PRECACHE:-1;
}
static unsigned shellshock_static_issues(const GscProgram *g,int kind,const int *args,int argc) {
    (void)g;(void)args;
    return kind==GSC_SHELLSHOCK_PRECACHE&&argc>=0&&argc<=MAX_PARAMS?0u:GSC_LINK_ISSUE_BUILTIN_ARITY;
}
static int debug_text_descriptor(const GscProgram *g,const char *name) {
    return g->debug_text_mode&&callable_name_equal(g,name,"print3d")?GSC_DEBUG_TEXT_PRINT3D:-1;
}
static unsigned debug_text_static_issues(const GscProgram *g,int kind,const int *args,int argc) {
    (void)g;(void)args;
    /* Original wrapper imposes no count maximum. MAX_PARAMS is parser policy. */
    return kind==GSC_DEBUG_TEXT_PRINT3D&&argc>=0&&argc<=MAX_PARAMS?0u:GSC_LINK_ISSUE_BUILTIN_ARITY;
}
static int precache_item_descriptor(const GscProgram *g,const char *name) {
    return g->precache_item_mode&&callable_name_equal(g,name,"precacheItem")?GSC_PRECACHE_ITEM_REGISTER:-1;
}
static unsigned precache_item_static_issues(const GscProgram *g,int kind,const int *args,int argc) {
    (void)g;(void)args;
    /* No native count branch; missing GetString0 is a runtime boundary. */
    return kind==GSC_PRECACHE_ITEM_REGISTER&&argc>=0&&argc<=MAX_PARAMS?0u:GSC_LINK_ISSUE_BUILTIN_ARITY;
}
static int save_game_descriptor(const GscProgram *g,const char *name) {
    return g->save_game_mode&&callable_name_equal(g,name,"saveGame")?GSC_SAVE_GAME_REQUEST:-1;
}
static int tag_fx_descriptor(const GscProgram *g,const char *name) {
    return g->tag_fx_mode&&callable_name_equal(g,name,"playfxOnTag")?GSC_TAG_FX_PLAY:-1;
}
static unsigned tag_fx_static_issues(const GscProgram *g,int kind,const int *args,int argc) {
    (void)g;(void)args;return kind!=GSC_TAG_FX_PLAY||argc!=3?GSC_LINK_ISSUE_BUILTIN_ARITY:0u;
}

static unsigned save_game_static_issues(const GscProgram *g,int kind,const int *args,int argc) {
    (void)g;(void)args;
    /* Optional getter branches impose no native maximum. Missing GetString0
     * faults dynamically only after the positive-health gate. */
    return kind==GSC_SAVE_GAME_REQUEST&&argc>=0&&argc<=MAX_PARAMS?0u:GSC_LINK_ISSUE_BUILTIN_ARITY;
}
static int ambient_descriptor(const GscProgram *g,const char *name) {
    if(!g->ambient_mode)return -1;
    if(callable_name_equal(g,name,"ambientPlay"))return GSC_AMBIENT_PLAY;
    if(callable_name_equal(g,name,"ambientStop"))return GSC_AMBIENT_STOP;
    return -1;
}
static unsigned ambient_static_issues(const GscProgram *g,int kind,const int *args,int argc) {
    (void)g;(void)args;
    return ((kind==GSC_AMBIENT_PLAY&&(argc==1||argc==2))||
            (kind==GSC_AMBIENT_STOP&&(argc==0||argc==1)))?0u:GSC_LINK_ISSUE_BUILTIN_ARITY;
}
static int debug_line_descriptor(const GscProgram *g,const char *name) {
    return g->debug_line_mode&&callable_name_equal(g,name,"line")?GSC_DEBUG_LINE_DRAW:-1;
}
static unsigned debug_line_static_issues(const GscProgram *g,int kind,const int *args,int argc) {
    (void)g;(void)args;
    return kind==GSC_DEBUG_LINE_DRAW&&argc>=0&&argc<=MAX_PARAMS?0u:GSC_LINK_ISSUE_BUILTIN_ARITY;
}
static int objective_preflight_args(const GscProgram *,int,const int *,int,int,char *,size_t);
static unsigned resource_static_issues(const GscProgram *,int,const int *,int);
static int resource_preflight_args(const GscProgram *,int,const int *,int,int,char *,size_t);

static unsigned music_static_issues(const GscProgram *g,int kind,const int *args,int argc);
static int music_preflight_args(const GscProgram *g,int kind,const int *args,int argc,
    int line,char *error,size_t size);
static int entity_predicate_descriptor(const GscProgram *g,const char *name) {
    if(g->actor_presence_selected&&callable_name_equal(g,name,"isai"))return ENTITY_GLOBAL_ISAI;
    if(!g->predicate_mode)return -1;
    if(callable_name_equal(g,name,"isalive")||!strcmp(name,"isAlive"))return ENTITY_GLOBAL_ISALIVE;
    if(callable_name_equal(g,name,"issentient")||!strcmp(name,"isSentient"))return ENTITY_GLOBAL_ISSENTIENT;
    return -1;
}
static int entity_global_expression_binding(const GscRuntime *r,const Expr *e) {
    return r->entities_enabled&&e->kind==E_NATIVE_CALL&&
        ((e->target==ENTITY_GLOBAL_GETENT&&callable_name_equal(r->program,e->name,"getent"))||
         (e->target==ENTITY_GLOBAL_SPAWN&&callable_name_equal(r->program,e->name,"spawn"))||
         (r->arrays_enabled&&e->target==ENTITY_GLOBAL_GETENTARRAY&&callable_name_equal(r->program,e->name,"getentarray"))||
         (r->actor_presence_enabled&&e->target==ENTITY_GLOBAL_ISAI&&
          entity_predicate_descriptor(r->program,e->name)==e->target)||
         (r->predicates_enabled&&
          (e->target==ENTITY_GLOBAL_ISALIVE||e->target==ENTITY_GLOBAL_ISSENTIENT)&&
          entity_predicate_descriptor(r->program,e->name)==e->target));
}
#include "gsc_player_descriptor.inc"
#include "gsc_vehicle_methods_descriptor.inc"
#include "gsc_entity_operations_descriptor.inc"
#include "gsc_entity_links_descriptor.inc"
#include "gsc_spatial_descriptor.inc"
#include "gsc_entity_coordinates_descriptor.inc"
#include "gsc_attachment_queries_descriptor.inc"
int gsc_program_select_r228_training_opening(GscProgram *g){if(!g||g->invalid||g->linked||g->training_opening_selected||g->carbine_items_selected!=2)return 0;g->training_opening_selected=1;return 1;}
int gsc_program_select_r225_training_followup(GscProgram *g){if(!gsc_program_select_r222_carbine_items(g))return 0;g->carbine_items_selected=2;return 1;}
enum {ENTITY_METHOD_ACTOR_DROP=1301,ENTITY_METHOD_SCRIPTED_PREFIX=1401,ENTITY_METHOD_SCRIPT_STATE=1402,ENTITY_METHOD_SCRIPTED_START=1403};
enum {ENTITY_METHOD_ACTOR_LOOK_BASE=1200,ENTITY_METHOD_ACTOR_LOOK_LAST=1214};
static int actor_look_kind(int d){return d>ENTITY_METHOD_ACTOR_LOOK_BASE&&d<=ENTITY_METHOD_ACTOR_LOOK_LAST?d-ENTITY_METHOD_ACTOR_LOOK_BASE:0;}
static int actor_look_tag(int k){return k==GSC_ACTOR_LOOK_TAG_ANGLES||k==GSC_ACTOR_LOOK_TAG_ORIGIN;}
static int actor_look_descriptor(const GscProgram *g,const char *name){static const char *const names[]={"setlookatanimnodes","setlookat","setlookatyawlimits","stoplookat","gettagangles","cansee","gettagorigin","canshoot","shoot","aimatpos","maymovetopoint","checkprone","traversemode","getnegotiationendpos"};int n;if(!g->actor_look_selected)return -1;for(n=0;n<14;n++)if(callable_name_equal(g,name,names[n]))return ENTITY_METHOD_ACTOR_LOOK_BASE+n+1;return -1;}
enum {ENTITY_METHOD_ACTOR_ANIMATION_BASE=1100,ENTITY_METHOD_ACTOR_ANIMATION_LAST=ENTITY_METHOD_ACTOR_ANIMATION_BASE+GSC_ACTOR_ANIMATION_FLAGGED_ALL};
static int actor_animation_kind(int d){if(d==ENTITY_METHOD_ACTOR_ANIMATION_BASE+GSC_ACTOR_ANIMATION_AIM_NODES)return GSC_ACTOR_ANIMATION_AIM_NODES;if(d==ENTITY_METHOD_ACTOR_ANIMATION_BASE+GSC_ACTOR_INTERRUPT_POINT || d==ENTITY_METHOD_ACTOR_ANIMATION_BASE+GSC_ACTOR_ANIMATION_FLAGGED_KNOB)return d-ENTITY_METHOD_ACTOR_ANIMATION_BASE;
 if(d==ENTITY_METHOD_ACTOR_ANIMATION_BASE+GSC_ACTOR_ANIMATION_KNOB_ALL_RESTART)return GSC_ACTOR_ANIMATION_KNOB_ALL_RESTART;
 return d==ENTITY_METHOD_ACTOR_ANIMATION_BASE+GSC_ACTOR_ANIMATION_KNOB?GSC_ACTOR_ANIMATION_KNOB:d>ENTITY_METHOD_ACTOR_ANIMATION_BASE&&d<=ENTITY_METHOD_ACTOR_ANIMATION_LAST?d-ENTITY_METHOD_ACTOR_ANIMATION_BASE:0;}
static int actor_animation_descriptor(const GscProgram *g,const char *name){
 if(g->actor_animations_selected&&g->node_queries_selected&&callable_name_equal(g,name,"setaimanims"))return ENTITY_METHOD_ACTOR_ANIMATION_BASE+GSC_ACTOR_ANIMATION_AIM_NODES;
 static const char *const names[]={"clearanim","setanim","setproneanimnodes","enterprone","exitprone","setflaggedanimknobrestart","setanimknoball","orientmode","updateprone","animmode","setflaggedanimknoballrestart","setflaggedanimknoball"};int n;
 _Static_assert(sizeof(names)/sizeof(names[0])==GSC_ACTOR_ANIMATION_FLAGGED_ALL,"actor animation descriptor range");
 if(g->actor_animations_selected && g->node_queries_selected){
    if(callable_name_equal(g,name,"interruptpoint"))return ENTITY_METHOD_ACTOR_ANIMATION_BASE+GSC_ACTOR_INTERRUPT_POINT;
    if(callable_name_equal(g,name,"setflaggedanimknob"))return ENTITY_METHOD_ACTOR_ANIMATION_BASE+GSC_ACTOR_ANIMATION_FLAGGED_KNOB;
 }
 if(g->actor_animations_selected && g->node_queries_selected && callable_name_equal(g,name,"setanimknoballrestart"))
    return ENTITY_METHOD_ACTOR_ANIMATION_BASE+GSC_ACTOR_ANIMATION_KNOB_ALL_RESTART;
 if(g->actor_animations_selected && g->node_queries_selected && callable_name_equal(g,name,"setanimknob"))
    return ENTITY_METHOD_ACTOR_ANIMATION_BASE+GSC_ACTOR_ANIMATION_KNOB;
 if(!g->actor_animations_selected)return -1;for(n=0;n<GSC_ACTOR_ANIMATION_FLAGGED_ALL;n++)if((n+1!=GSC_ACTOR_ANIMATION_FLAGGED_ALL_RESTART||g->flagged_all_restart_selected)&&(n+1!=GSC_ACTOR_ANIMATION_FLAGGED_ALL||g->flagged_all_selected)&&callable_name_equal(g,name,names[n]))return ENTITY_METHOD_ACTOR_ANIMATION_BASE+n+1;return -1;
}
enum {ENTITY_METHOD_TURRET_ANIMATION_BASE=1000,ENTITY_METHOD_TURRET_ANIMATION_LAST=1004};
static int turret_animation_kind(int d){return d>ENTITY_METHOD_TURRET_ANIMATION_BASE&&d<=ENTITY_METHOD_TURRET_ANIMATION_LAST?d-ENTITY_METHOD_TURRET_ANIMATION_BASE+3:0;}
static int turret_animation_descriptor(const GscProgram *g,const char *name){
 static const char *const names[]={"setturretanim","setanimknobrestart","useanimtree","stopuseanimtree"};int i;
 if(!g->turret_animations_selected)return -1;for(i=0;i<4;i++)if(callable_name_equal(g,name,names[i]))return ENTITY_METHOD_TURRET_ANIMATION_BASE+i+1;return -1;
}
enum { ENTITY_METHOD_TURRET_BASE=900, ENTITY_METHOD_TURRET_LAST=ENTITY_METHOD_TURRET_BASE+GSC_TURRET_STOP_USE };
static int turret_method_kind(int descriptor){return descriptor>ENTITY_METHOD_TURRET_BASE&&descriptor<=ENTITY_METHOD_TURRET_LAST?descriptor-ENTITY_METHOD_TURRET_BASE:0;}
static int turret_method_descriptor(const GscProgram *g,const char *name){
    static const char *names[]={"setmode","startfiring","stopfiring","settargetentity","useturret","getturret","isfiringturret","shootturret","cleartargetentity","stopuseturret"};unsigned n;
    if(!g->turrets_selected)return -1;
    for(n=0;n<sizeof(names)/sizeof(names[0]);++n)if(callable_name_equal(g,name,names[n]))return ENTITY_METHOD_TURRET_BASE+n+1;
    return -1;
}
static int entity_method_descriptor(const GscProgram *g,const char *name) {
    {int vehicle=vehicle_method_descriptor(g,name);if(vehicle>=0)return vehicle;}
    {int link=entity_link_descriptor(g,name);if(link>=0)return link;}
    if((g->actor_float_selected&1u)&&callable_name_equal(g,name,"approxpathdist"))return R243_ACTOR_FLOAT_PATH;
    if((g->actor_float_selected&2u)&&callable_name_equal(g,name,"getclosestenemysqdist"))return R243_ACTOR_FLOAT_ENEMY;
    if((g->actor_float_selected&4u)&&callable_name_equal(g,name,"getmotionangle"))return COD1_ACTOR_MOTION_ANGLE;
    if(g->stance_selected==2&&callable_name_equal(g,name,"allowedstances"))return COD1_ALLOWED_STANCES_METHOD;
    if(g->stance_selected&&callable_name_equal(g,name,"isstanceallowed"))return R243_STANCE_METHOD;
    if(g->animtime_selected&&callable_name_equal(g,name,"getanimtime"))return R243_ANIMTIME_METHOD;
    if(g->r243_scripted_start_selected&&callable_name_equal(g,name,"startscriptedanim"))return ENTITY_METHOD_SCRIPTED_START;
    if(g->r243_script_state_selected&&callable_name_equal(g,name,"trackscriptstate"))return ENTITY_METHOD_SCRIPT_STATE;
    if(g->r243_scripted_prefix_selected&&callable_name_equal(g,name,"animscripted"))return ENTITY_METHOD_SCRIPTED_PREFIX;
    if(g->actor_drop_selected&&callable_name_equal(g,name,"dropweapon"))return ENTITY_METHOD_ACTOR_DROP;
    int look=actor_look_descriptor(g,name);if(look>=0)return look;
    int actorAnimation=actor_animation_descriptor(g,name);if(actorAnimation>=0)return actorAnimation;
    int animation=turret_animation_descriptor(g,name);if(animation>=0)return animation;
    int turret=turret_method_descriptor(g,name);if(turret>=0)return turret;
    if(g->carbine_items_selected==2&&callable_name_equal(g,name,"sethintstring"))return 504;
    if(g->map_moveto_selected&&g->yaw_motion_selected&&callable_name_equal(g,name,"moveto"))return COD1_MOVE_TO;
    if(g->yaw_motion_selected && g->actor_goal_selected==2){
        if(callable_name_equal(g,name,"rotatevelocity"))return COD1_ROTATE_VELOCITY;
        if(callable_name_equal(g,name,"moveto"))return COD1_MOVE_TO;
        if(callable_name_equal(g,name,"movegravity"))return COD1_MOVE_GRAVITY;
    }
    if(g->yaw_motion_selected&&callable_name_equal(g,name,"rotateto"))return COD1_ROTATE_TO;
    if(g->yaw_motion_selected&&callable_name_equal(g,name,"rotateroll"))return COD1_ROTATE_ROLL;
    if(g->yaw_motion_selected&&callable_name_equal(g,name,"launch"))return COD1_LAUNCH;
    if(g->yaw_motion_selected&&callable_name_equal(g,name,"rotateyaw"))return 502;
    if(g->actor_goal_selected==2 && callable_name_equal(g,name,"setgoalentity"))return COD1_SET_GOAL_ENTITY_METHOD;
    if(g->actor_goal_selected&&callable_name_equal(g,name,"setgoalnode"))return 503;
    if(g->actor_goal_selected && g->actor_animations_selected && (g->carbine_items_selected==2||g->map_actor_goal_selected) && callable_name_equal(g,name,"setgoalpos"))return COD1_SET_GOAL_POS_METHOD;
    int attachment=attachment_query_descriptor(g,name);if(attachment>=0)return attachment;
    if(g->entity_collision_selected&&callable_name_equal(g,name,"notsolid"))return 501;
    if(g->entity_collision_selected==2&&callable_name_equal(g,name,"enablegrenadetouchdamage"))return GSC_GRENADE_TOUCH_METHOD;
    if(g->entity_collision_selected==2&&callable_name_equal(g,name,"disablegrenadebounce"))return GSC_GRENADE_BOUNCE_METHOD;
    if(g->entity_collision_selected==2&&callable_name_equal(g,name,"solid"))return GSC_SOLID_METHOD;
    if(g->entity_collision_selected==2&&callable_name_equal(g,name,"connectpaths"))return GSC_CONNECTPATHS_METHOD;
    if(g->disconnectpaths_selected&&callable_name_equal(g,name,"disconnectpaths"))return GSC_DISCONNECTPATHS_METHOD;
    int spatial=spatial_method_descriptor(g,name);if(spatial>=0)return spatial;
    int coordinates=entity_coordinate_descriptor(g,name);if(coordinates>=0)return coordinates;
    int operation=entity_operation_descriptor(g,name);if(operation>=0)return operation;
    int player=player_method_descriptor(g,name);if(player>=0)return player;
    if(g->actor_critical_selected) {
        if(callable_name_equal(g,name,"entercriticalsection"))return ENTITY_METHOD_ENTERCRITICAL;
        if(callable_name_equal(g,name,"leavecriticalsection"))return ENTITY_METHOD_LEAVECRITICAL;
        if(callable_name_equal(g,name,"isincriticalsection"))return ENTITY_METHOD_ISCRITICAL;
    }
    if(g->animation_methods_selected) {
        if(callable_name_equal(g,name,"setanimknob"))return ENTITY_METHOD_SETANIMKNOB;
        if(callable_name_equal(g,name,"setanimknoball"))return ENTITY_METHOD_SETANIMKNOBALL;
        if(callable_name_equal(g,name,"setflaggedanimknoball"))return ENTITY_METHOD_SETFLAGGEDANIMKNOBALL;
    }
    if(g->loop_sound_selected){
        if(callable_name_equal(g,name,"playloopsound"))return ENTITY_METHOD_PLAYLOOPSOUND;
        if(callable_name_equal(g,name,"stoploopsound"))return ENTITY_METHOD_STOPLOOPSOUND;
    }
    if(g->script_sound_selected&&callable_name_equal(g,name,"playsound"))return ENTITY_METHOD_PLAYSOUND;
    if(g->spawner_team_method_selected&&callable_name_equal(g,name,"setspawnerteam"))return ENTITY_METHOD_SETSPAWNERTEAM;
    if(g->entity_model_methods_selected) {
        if(callable_name_equal(g,name,"attach"))return ENTITY_METHOD_ATTACH;
        if(callable_name_equal(g,name,"detach"))return ENTITY_METHOD_DETACH;
        if(callable_name_equal(g,name,"detachall"))return ENTITY_METHOD_DETACH_ALL;
    }
    if(g->native_queries_mode&&(callable_name_equal(g,name,"getentitynumber")||callable_name_equal(g,name,"getentnum")))return ENTITY_METHOD_GETNUMBER;
    if(g->actor_spawn_mode&&callable_name_equal(g,name,"dospawn"))return ENTITY_METHOD_DOSPAWN;
    if(g->actor_spawn_mode&&callable_name_equal(g,name,"stalingradspawn"))return ENTITY_METHOD_STALINGRADSPAWN;
    if(g->entity_mode&&callable_name_equal(g,name,"setorigin"))return ENTITY_METHOD_SETORIGIN;
    if(g->access_mode&&(callable_name_equal(g,name,"setmodel")||!strcmp(name,"setModel")))
        return ENTITY_METHOD_SETMODEL;
    if(g->controls_mode&&callable_name_equal(g,name,"hide"))return ENTITY_METHOD_HIDE;
    if(g->controls_mode&&callable_name_equal(g,name,"show"))return ENTITY_METHOD_SHOW;
    return -1;
}
static int entity_method_arity(int descriptor) {
    if(vehicle_method_kind(descriptor))return -1;
    if(actor_animation_kind(descriptor)==GSC_ACTOR_INTERRUPT_POINT)return 0;
    if(R243ActorFloatMask(descriptor))return 0;
    if(entity_link_method(descriptor))return -1;
    if(descriptor==R243_STANCE_METHOD||descriptor==R243_ANIMTIME_METHOD)return 1;
    if(descriptor==ENTITY_METHOD_SCRIPTED_START||descriptor==ENTITY_METHOD_SCRIPT_STATE||descriptor==ENTITY_METHOD_SCRIPTED_PREFIX||descriptor==ENTITY_METHOD_ACTOR_DROP)return -1;
    if(actor_look_kind(descriptor)){int k=actor_look_kind(descriptor);return k==GSC_ACTOR_NEGOTIATION_END?0:k==GSC_ACTOR_LOOK_NODES||k==GSC_ACTOR_LOOK_YAW_LIMITS||k==GSC_ACTOR_LOOK_CHECK_PRONE?3:k==GSC_ACTOR_LOOK_TARGET||k==GSC_ACTOR_LOOK_STOP?-1:1;}
    if(actor_animation_kind(descriptor)){int k=actor_animation_kind(descriptor);return k==GSC_ACTOR_ANIMATION_AIM_NODES?6:k==GSC_ACTOR_ANIMATION_ORIENT_MODE?-1:k==GSC_ACTOR_ANIMATION_CLEAR?2:(k==GSC_ACTOR_ANIMATION_PRONE_NODES||k==GSC_ACTOR_ANIMATION_UPDATE_PRONE)?5:(k==GSC_ACTOR_ANIMATION_SET||(k==GSC_ACTOR_ANIMATION_FLAGGED_RESTART||k==GSC_ACTOR_ANIMATION_FLAGGED_KNOB)||(k==GSC_ACTOR_ANIMATION_KNOB_ALL||k==GSC_ACTOR_ANIMATION_KNOB_ALL_RESTART)||(k==GSC_ACTOR_ANIMATION_FLAGGED_ALL_RESTART||k==GSC_ACTOR_ANIMATION_FLAGGED_ALL))?-1:1;}
    int animation=turret_animation_kind(descriptor);if(animation)return animation==GSC_ANIMATION_STOP_USE_TREE?0:animation==GSC_ANIMATION_SET_KNOB_RESTART?-1:1;
    int turret=turret_method_kind(descriptor);if(turret)return turret==GSC_TURRET_SET_MODE||turret==GSC_TURRET_SET_TARGET_ENTITY||turret==GSC_TURRET_USE?1:0;
    if(descriptor==COD1_LAUNCH || descriptor==COD1_ROTATE_ROLL || descriptor==COD1_ROTATE_TO || descriptor==COD1_MOVE_TO)return 2;
    if(descriptor==COD1_ROTATE_VELOCITY || descriptor==COD1_MOVE_GRAVITY)return 2;
    if(descriptor==502)return 2;
    if(attachment_query_kind(descriptor))return descriptor==ATTACH_QUERY_SIZE?0:1;
    if(descriptor==501||descriptor==GSC_CONNECTPATHS_METHOD||descriptor==GSC_DISCONNECTPATHS_METHOD||descriptor==GSC_SOLID_METHOD||descriptor==GSC_GRENADE_TOUCH_METHOD||descriptor==GSC_GRENADE_BOUNCE_METHOD)return 0;
    if(spatial_method_kind(descriptor))return descriptor==SPATIAL_METHOD_ISTOUCHING?1:0;
    if(entity_coordinate(descriptor))return 1;
    if(entity_operation(descriptor))return descriptor==ENTITY_OP_SETANGLES?1:0;
    if(player_method_kind(descriptor))return player_method_arity(descriptor);
    return actor_spawn_native_descriptor(descriptor)||descriptor==ENTITY_METHOD_GETNUMBER||
        (descriptor>=ENTITY_METHOD_ATTACH&&descriptor<=ENTITY_METHOD_STOPLOOPSOUND)?-1:
        descriptor==ENTITY_METHOD_HIDE||descriptor==ENTITY_METHOD_SHOW?0:1;
}
static int entity_method_argc_valid(int descriptor,int argc) {
    if(vehicle_method_kind(descriptor)){int k=vehicle_method_kind(descriptor);return argc>=(k==2?0:k==5?2:1)&&argc<=MAX_PARAMS;}
    if(descriptor==COD1_LAUNCH)return argc==1;
    if(descriptor==502 || descriptor==COD1_LAUNCH || descriptor==COD1_ROTATE_ROLL || descriptor==COD1_ROTATE_TO || descriptor==COD1_MOVE_TO)return argc>=2&&argc<=MAX_PARAMS;
    if(descriptor==COD1_ALLOWED_STANCES_METHOD)return argc>=1&&argc<=MAX_PARAMS;
    if(entity_link_method(descriptor))return argc>=0&&argc<=MAX_PARAMS;
    if(descriptor==ENTITY_METHOD_SCRIPTED_START)return argc>=0&&argc<=6;
    if(descriptor==ENTITY_METHOD_SCRIPT_STATE)return argc>=0&&argc<=MAX_PARAMS;
    if(descriptor==ENTITY_METHOD_SCRIPTED_PREFIX)return argc>=0&&argc<=6;
    if(descriptor==ENTITY_METHOD_ACTOR_DROP)return argc>=1&&argc<=MAX_PARAMS;
    if(actor_look_kind(descriptor)){int k=actor_look_kind(descriptor);return k==GSC_ACTOR_LOOK_SHOOT?argc>=0&&argc<=MAX_PARAMS:(k==GSC_ACTOR_LOOK_TARGET||k==GSC_ACTOR_LOOK_CAN_SHOOT)?argc>=1&&argc<=2:k==GSC_ACTOR_LOOK_STOP?argc>=0&&argc<=1:argc==entity_method_arity(descriptor);}
    if(actor_animation_kind(descriptor)){int k=actor_animation_kind(descriptor);return k==GSC_ACTOR_ANIMATION_ORIENT_MODE?(argc>=0&&argc<=MAX_PARAMS):(k==GSC_ACTOR_ANIMATION_FLAGGED_ALL_RESTART||k==GSC_ACTOR_ANIMATION_FLAGGED_ALL)?argc>=3&&argc<=6:(k==GSC_ACTOR_ANIMATION_SET||k==GSC_ACTOR_ANIMATION_KNOB)?argc>=1&&argc<=4:((k==GSC_ACTOR_ANIMATION_FLAGGED_RESTART||k==GSC_ACTOR_ANIMATION_FLAGGED_KNOB)||(k==GSC_ACTOR_ANIMATION_KNOB_ALL||k==GSC_ACTOR_ANIMATION_KNOB_ALL_RESTART))?argc>=2&&argc<=5:argc==entity_method_arity(descriptor);}
    if(turret_animation_kind(descriptor))return turret_animation_kind(descriptor)==GSC_ANIMATION_SET_KNOB_RESTART?(argc>=1&&argc<=4):argc==entity_method_arity(descriptor);
    if(spatial_method_kind(descriptor)||entity_coordinate(descriptor))return argc==entity_method_arity(descriptor);
    if(entity_operation(descriptor))return argc==(descriptor==ENTITY_OP_SETANGLES?1:0);
    if(player_method_kind(descriptor))return argc==player_method_arity(descriptor);
    return actor_spawn_native_descriptor(descriptor)||descriptor==ENTITY_METHOD_GETNUMBER||
        (descriptor>=ENTITY_METHOD_ATTACH&&descriptor<=ENTITY_METHOD_STOPLOOPSOUND)?(argc>=0&&argc<=MAX_PARAMS):
        argc==entity_method_arity(descriptor);
}
static int entity_method_has_result(int descriptor) {
    if(vehicle_method_kind(descriptor))return 0;
    if(R243ActorFloatMask(descriptor))return 1;
    if(entity_link_method(descriptor))return 0;
    if(descriptor==R243_STANCE_METHOD||descriptor==R243_ANIMTIME_METHOD)return 1;
    if(descriptor==ENTITY_METHOD_SCRIPTED_START||descriptor==ENTITY_METHOD_SCRIPT_STATE||descriptor==ENTITY_METHOD_SCRIPTED_PREFIX||descriptor==ENTITY_METHOD_ACTOR_DROP)return 0;
    if(actor_look_kind(descriptor))return actor_look_kind(descriptor)>=GSC_ACTOR_LOOK_TAG_ANGLES&&actor_look_kind(descriptor)!=GSC_ACTOR_LOOK_SHOOT&&actor_look_kind(descriptor)!=GSC_ACTOR_TRAVERSE_MODE;
    if(actor_animation_kind(descriptor))return 0;
    if(turret_animation_kind(descriptor))return 0;
    int turret=turret_method_kind(descriptor);if(turret)return turret==GSC_TURRET_USE||turret==GSC_TURRET_GET||turret==GSC_TURRET_IS_FIRING;
    if(attachment_query_kind(descriptor))return 1;
    if(spatial_method_kind(descriptor)||entity_coordinate(descriptor))return 1;
    if(entity_operation(descriptor))return descriptor==ENTITY_OP_GETORIGIN||descriptor==ENTITY_OP_GETANGLES;
    if(player_method_kind(descriptor))return player_method_result(descriptor)!=GSC_PLAYER_RESULT_VOID;
    return actor_spawn_native_descriptor(descriptor)||descriptor==ENTITY_METHOD_GETNUMBER||descriptor==ENTITY_METHOD_ISCRITICAL;
}
static int entity_method_argument_kind(int descriptor) {
    if(descriptor==R243_STANCE_METHOD||descriptor==COD1_ALLOWED_STANCES_METHOD)return V_STRING;
    if(actor_look_kind(descriptor))return (actor_look_tag(actor_look_kind(descriptor))||actor_look_kind(descriptor)==GSC_ACTOR_TRAVERSE_MODE)?V_STRING:actor_look_kind(descriptor)==GSC_ACTOR_LOOK_CAN_SEE?V_ENTITY:V_NUMBER;
    if(turret_method_kind(descriptor))return turret_method_kind(descriptor)==GSC_TURRET_SET_MODE?V_STRING:V_ENTITY;
    if(descriptor==504)return V_LOCALIZED_STRING;
    if(descriptor==COD1_SET_GOAL_ENTITY_METHOD)return V_ENTITY;
    if(descriptor==503)return V_NODE;
    if(attachment_query_kind(descriptor))return V_NUMBER;
    if(spatial_method_kind(descriptor))return V_ENTITY;
    if(player_method_kind(descriptor)>=GSC_PLAYER_ALLOW_STAND&&player_method_kind(descriptor)<=GSC_PLAYER_ALLOW_PRONE)return V_NUMBER;
    if(player_method_kind(descriptor)==GSC_PLAYER_SET_ANGLES)return V_VECTOR;
    if(player_method_kind(descriptor))return (player_method_kind(descriptor)==GSC_PLAYER_FREEZE_CONTROLS||player_method_kind(descriptor)==GSC_PLAYER_SET_NORMAL_HEALTH)?V_NUMBER:V_STRING;
    return descriptor==ENTITY_METHOD_SETMODEL?V_STRING:V_VECTOR;
}
enum { ENTITY_EVENT_WAITTILL=0, ENTITY_EVENT_NOTIFY=1, ENTITY_EVENT_ENDON=2, ENTITY_EVENT_WAITTILLMATCH=3 };
static int entity_event_descriptor(const GscProgram *g,const char *name) {
    if(!g->event_mode)return -1;
    if(!strcmp(name,"waittill"))return ENTITY_EVENT_WAITTILL;
    if(!strcmp(name,"notify"))return ENTITY_EVENT_NOTIFY;
    if(g->endon_selected&&!strcmp(name,"endon"))return ENTITY_EVENT_ENDON;
    if(g->waittillmatch_selected&&!strcmp(name,"waittillmatch"))return ENTITY_EVENT_WAITTILLMATCH;
    return -1;
}
static int entity_event_argc_valid(const GscProgram *g,int event,int argc) {
    if(event==ENTITY_EVENT_ENDON)return argc==1;
    if(event==ENTITY_EVENT_WAITTILLMATCH)return argc>=2&&argc<=MAX_PARAMS;
    return g->event_payload_mode?(argc>=1&&argc<=MAX_PARAMS):argc==1;
}

static void fail(Parser *p, int line, int col, const char *fmt, ...) {
    va_list args;
    size_t prefix;
    if (p->failed) return;
    p->failed=1;
    if (!p->error || !p->error_size) return;
    snprintf(p->error,p->error_size,"%d:%d: ",line,col);
    prefix=strlen(p->error);
    va_start(args,fmt);
    vsnprintf(p->error+prefix,p->error_size-prefix,fmt,args);
    va_end(args);
}
static void lex_fail(Parser *p, const char *s) { fail(p,p->line,p->col,"%s",s); }
static int peek(Parser *p) { return (unsigned char)p->source[p->pos]; }
static int peek_next(Parser *p) { return (unsigned char)p->source[p->pos+1]; }
static void bump(Parser *p) {
    if (!peek(p)) return;
    if (peek(p)=='\n') {p->line++;p->col=1;} else p->col++;
    p->pos++;
}
static void next(Parser *p) {
    size_t n=0, start;
    int c, esc, dot=0;
    Token *t=&p->tok;
    if (p->failed) {t->type=T_INVALID;return;}
    while ((c=peek(p))!=0) {
        if (isspace((unsigned char)c)) {bump(p);continue;}
        if (c=='/' && peek_next(p)=='/') {
            while (peek(p) && peek(p)!='\n') bump(p);
            continue;
        }
        if (c=='/' && peek_next(p)=='*') {
            int line=p->line,col=p->col;
            bump(p);bump(p);
            while (peek(p) && !(peek(p)=='*'&&peek_next(p)=='/')) bump(p);
            if (!peek(p)) {fail(p,line,col,"unterminated comment");t->type=T_INVALID;return;}
            bump(p);bump(p);continue;
        }
        break;
    }
    memset(t,0,sizeof(*t));t->line=p->line;t->col=p->col;
    t->source_offset=p->pos;t->first_unknown_offset=-1;
    c=peek(p);
    if (!c) {t->type=T_EOF;return;}
    if (isalpha((unsigned char)c)||c=='_') {
        t->type=T_ID;
        while (isalnum((unsigned char)peek(p))||peek(p)=='_') {
            if (n+1>=sizeof(t->word)) {lex_fail(p,"identifier too long");t->type=T_INVALID;return;}
            t->word[n++]=(char)peek(p);bump(p);
        }
        return;
    }
    if (isdigit((unsigned char)c)||(c=='.'&&isdigit((unsigned char)peek_next(p)))) {
        char *end;
        long integer;
        start=p->pos;t->type=T_NUMBER;
        if (c=='.') {dot=1;bump(p);}
        while (isdigit((unsigned char)peek(p))) bump(p);
        if (peek(p)=='.'&&!dot) {dot=1;bump(p);while(isdigit((unsigned char)peek(p)))bump(p);}
        if (peek(p)=='e'||peek(p)=='E') {
            dot=1;bump(p);
            if (peek(p)=='+'||peek(p)=='-')bump(p);
            if (!isdigit((unsigned char)peek(p))) {fail(p,t->line,t->col,"bad numeric exponent");t->type=T_INVALID;return;}
            while(isdigit((unsigned char)peek(p)))bump(p);
        }
        n=p->pos-start;
        if (n>=sizeof(t->word)) {fail(p,t->line,t->col,"number too long");t->type=T_INVALID;return;}
        memcpy(t->word,p->source+start,n);t->word[n]=0;
        errno=0;t->number=strtof(t->word,&end);
        if (errno||*end||!isfinite(t->number)) {fail(p,t->line,t->col,"number outside float32 range");t->type=T_INVALID;return;}
        if (!dot) {
            errno=0;integer=strtol(t->word,&end,10);
            if (errno||*end||integer<INT_MIN||integer>INT_MAX) {
                fail(p,t->line,t->col,"integer outside int32 range");t->type=T_INVALID;return;
            }
            t->integer=1;t->integer_value=(int32_t)integer;
        }
        return;
    }
    /* Preserve the & prefix in the token kind. Localized strings are not
     * ordinary strings, and must never inherit the fixture string value. */
    if (c=='&'&&peek_next(p)=='"') {
        t->type=T_LOCALIZED_STRING;bump(p);c=peek(p);
    }
    if (c=='"') {
        size_t raw_n=0;
        if(t->type!=T_LOCALIZED_STRING)t->type=T_STRING;
        bump(p);t->raw_offset=p->pos;
        while ((c=peek(p))&&c!='"') {
            if (c=='\n'||c=='\r') {fail(p,t->line,t->col,"newline in string");t->type=T_INVALID;return;}
            /* This is a lexical-unit limit, not an invented decoded length
             * for unresolved literals. At most two spelling bytes per unit. */
            if(n+1>=sizeof(t->word)) {fail(p,t->line,t->col,"string too long");t->type=T_INVALID;return;}
            if (c=='\\') {
                t->raw[raw_n++]='\\';bump(p);esc=peek(p);
                if(!esc) {fail(p,t->line,t->col,"unterminated string escape");t->type=T_INVALID;return;}
                if(esc=='\n'||esc=='\r') {fail(p,t->line,t->col,"newline in string");t->type=T_INVALID;return;}
                if(p->native_quoted_strings&&(esc<32||esc>126)) {fail(p,t->line,t->col,"escaped byte outside selected native printable domain");t->type=T_INVALID;return;}
                t->raw[raw_n++]=(char)esc;
                if (esc=='n') c='\n'; else if (esc=='t') c='\t';
                else if(p->native_quoted_strings&&esc=='r')c='\r';
                else if (esc=='"'||esc=='\\') c=esc;
                else {
                    if(!t->unknown_escape_count)t->first_unknown_offset=(int)raw_n-2;
                    t->unknown_escape_count++;
                    /* Temporary bytes are never published as a value. */
                    c=p->native_quoted_strings?esc:0;
                }
            } else t->raw[raw_n++]=(char)c;
            t->word[n++]=(char)c;bump(p);
        }
        if (!peek(p)) {fail(p,t->line,t->col,"unterminated string");t->type=T_INVALID;return;}
        t->raw_length=(int)raw_n;
        if(t->unknown_escape_count&&!p->native_quoted_strings)memset(t->word,0,sizeof(t->word));
        bump(p);return;
    }
    if(c=='/'&&peek_next(p)=='#') {
        bump(p);bump(p);t->type=T_DEV_OPEN;return;
    }
    if(c=='#'&&peek_next(p)=='/') {
        bump(p);bump(p);t->type=T_DEV_CLOSE;return;
    }
    bump(p);t->type=c;
    if ((c=='+'||c=='-'||c=='*'||c=='/')&&peek(p)=='=') {
        bump(p);t->type=c=='+'?T_ADD_ASSIGN:c=='-'?T_SUB_ASSIGN:
                            c=='*'?T_MUL_ASSIGN:T_DIV_ASSIGN;
    } else if ((c=='+'||c=='-')&&peek(p)==c) {
        bump(p);t->type=c=='+'?T_INC:T_DEC;
    } else if ((c=='='&&peek(p)=='=')||(c=='!'&&peek(p)=='=')||
        (c=='<'&&peek(p)=='=')||(c=='>'&&peek(p)=='=')||
        (c=='&'&&peek(p)=='&')||(c=='|'&&peek(p)=='|')) {
        bump(p);
        t->type=(c=='=')?T_EQ:(c=='!')?T_NE:(c=='<')?T_LE:
                (c=='>')?T_GE:(c=='&')?T_AND:T_OR;
    } else if(p->integer_bitwise_syntax&&(c=='|'||c=='^'||c=='&')&&peek(p)=='=') {
        bump(p);t->type=c=='|'?T_BIT_OR_ASSIGN:c=='^'?T_BIT_XOR_ASSIGN:T_BIT_AND_ASSIGN;
    } else if(c==':'&&peek(p)==':') {bump(p);t->type=T_SCOPE;}
    else if (!strchr("{}[]();,.=+-*/%<>!\\#&:",c)&&
             !(p->integer_bitwise_syntax&&strchr("|^~",c))) {
        fail(p,t->line,t->col,"unsupported token '%c'",c);t->type=T_INVALID;
    }
}
static int word(Parser *p,const char *s) {return p->tok.type==T_ID&&!strcmp(p->tok.word,s);}
static int literal_name(const char *s) {
    return !strcmp(s,"true")||!strcmp(s,"false")||!strcmp(s,"undefined");
}
static int accept(Parser *p,int type) {if (p->tok.type==type){next(p);return 1;}return 0;}
static int expect(Parser *p,int type,const char *name) {
    if (accept(p,type)) return 1;
    if (!p->failed) fail(p,p->tok.line,p->tok.col,"expected %s",name);
    return 0;
}
static void str_copy(char *dst,size_t size,const char *src) {snprintf(dst,size,"%s",src);}
/* On failure the old allocation and capacity remain valid; compilation then
 * poisons the program via gsc_module_compile's invalid state. Counts are int
 * IR indexes, so allocation arithmetic is checked in size_t and bounded by
 * each table's hard cap. */
static void *grow_table(Parser *p,void *old,int *capacity,int maximum,
                        size_t item_size,const char *name) {
    int next;
    void *fresh;
    if(*capacity>=maximum) {
        fail(p,p->tok.line,p->tok.col,"%s limit",name);return NULL;
    }
    next=*capacity?*capacity*2:8;
    if(next>maximum)next=maximum;
    if(item_size==0||(size_t)next>SIZE_MAX/item_size) {
        fail(p,p->tok.line,p->tok.col,"%s allocation overflow",name);return NULL;
    }
    fresh=GSC_IR_REALLOC(old,(size_t)next*item_size);
    if(!fresh) {
        fail(p,p->tok.line,p->tok.col,"out of memory growing %s",name);return NULL;
    }
    *capacity=next;
    return fresh;
}
/* Retain only consumed tokens. Parser copies used for lookahead remain pure
 * and cannot append duplicate literals or change whole-program guards. */
static int retain_string(Parser *p,GscProgram *g,int owner_kind,int owner_index) {
    const Token *t=&p->tok;StringLiteral literal;
    if(g->string_count>=MAX_STRING_LITERALS) {
        fail(p,t->line,t->col,"string literal limit");return -1;
    }
    if(g->string_count==g->string_capacity) {
        StringLiteral *fresh=grow_table(p,g->strings,&g->string_capacity,
                                MAX_STRING_LITERALS,sizeof(*g->strings),"string literal");
        if(!fresh)return -1;
        g->strings=fresh;
    }
    memset(&literal,0,sizeof(literal));
    literal.module=p->module;literal.line=t->line;literal.col=t->col;
    literal.owner_kind=owner_kind;literal.owner_index=owner_index;
    literal.localized=t->type==T_LOCALIZED_STRING;
    literal.decode_status=p->native_quoted_strings?GSC_STRING_NATIVE_DECODED:
        t->unknown_escape_count?GSC_STRING_RAW_UNRESOLVED:GSC_STRING_FIXTURE_DECODED;
    literal.raw_length=t->raw_length;literal.unknown_escape_count=t->unknown_escape_count;
    literal.first_unknown_offset=t->first_unknown_offset;
    literal.source_offset=t->source_offset;literal.raw_offset=t->raw_offset;
    memcpy(literal.raw,t->raw,sizeof(literal.raw));
    if(!t->unknown_escape_count||p->native_quoted_strings)memcpy(literal.decoded,t->word,sizeof(literal.decoded));
    else g->requires_raw_string_semantics=1;
    g->strings[g->string_count]=literal;return g->string_count++;
}
/* Selected local symbol canonicalization: original lexer token102 calls
 * ENGINE45D340, which folds ASCII A..Z through 51B3DD/51B315 before interning.
 * Preserve source, strings, associative keys, fields and legacy modes. The
 * field/keyword domains are deliberately not claimed by this local slice. */
int gsc_program_select_local_identifier_case(GscProgram *g){
 if(!g||g->invalid||g->linked||g->module_count||g->local_identifier_case_selected)return 0;
 g->local_identifier_case_selected=1;return 1;
}
static void local_identifier_case(char *p){
 for(;*p;p++)if(*p>='A'&&*p<='Z')*p=(char)(*p+('a'-'A'));
}
static int new_expr(Parser *p,GscProgram *g,Expr e) {
    if(g->local_identifier_case_selected&&e.kind==E_VAR){
        local_identifier_case(e.name);
        if(literal_name(e.name))g->requires_literal_semantics=1;
        if(!strcmp(e.name,"self")||!strcmp(e.name,"level"))g->requires_game_values=1;
    }
    if (g->expr_count>=MAX_EXPR) {fail(p,p->tok.line,p->tok.col,"expression limit");return -1;}
    if(g->expr_count==g->expr_capacity) {
        Expr *fresh=grow_table(p,g->expr,&g->expr_capacity,MAX_EXPR,sizeof(*g->expr),"expression");
        if(!fresh)return -1;
        g->expr=fresh;
    }
    e.float_arithmetic=p->module>=0&&p->module<g->module_count?
        (g->r232_source_float_selected&&(r232_source_float_module(g->modules[p->module])||g->map_source_float_selected)?2:
         g->float_arithmetic_selected&&!strcmp(g->modules[p->module],g->float_arithmetic_module)):0;
    if(e.float_arithmetic&&e.kind==E_NUMBER&&!e.integer) {
        uint32_t bits;
        if((e.float_arithmetic==2||!strcmp(e.name,"0.4")||!strcmp(e.name,"0.8"))?
            !float_arithmetic_decimal_nearest(e.name,&bits):!randomfloat_decimal_bits(e.name,&bits)) {
            fail(p,e.line,1,"float literal outside selected module arithmetic domain");return -1;
        }
        memcpy(&e.number,&bits,sizeof(bits));
    }
    e.receiver_binding_kind=GSC_RECEIVER_TARGET_NONE;e.receiver_script_target=-1;e.receiver_native_target=-1;
    e.receiver_event_target=-1;
    e.field_write_target=0;
    g->expr[g->expr_count]=e;return g->expr_count++;
}
static int precedence(int t) {
    /* Syntax convention, not yet a verified CoD1 precedence table. */
    switch(t) {case T_OR:return 1;case T_AND:return 2;case '&':return 3;
      case T_EQ:case T_NE:return 4;
      case '<':case '>':case T_LE:case T_GE:return 5;
      case '+':case '-':return 6;case '*':case '/':case '%':return 7;default:return 0;}
}
static int integer_bitwise_precedence(const GscProgram *g,int t) {
    if(!g->integer_bitwise_selected)return precedence(t);
    /* Explicit C-like restoration policy; the retail compiler's precedence
     * has not been inferred from this table. Existing syntax stays intact. */
    switch(t) {case T_OR:return 1;case T_AND:return 2;
      case '|':return 3;case '^':return 4;case '&':return 5;
      case T_EQ:case T_NE:return 6;
      case '<':case '>':case T_LE:case T_GE:return 7;
      case '+':case '-':return 8;case '*':case '/':case '%':return 9;default:return 0;}
}
static int expression(Parser *p,GscProgram *g,int min_prec);
/* Statement receivers stop at their top-level `thread`; nested groups, call
 * arguments and indexes still parse ordinary expressions. */
static int expression_mode(Parser *p,GscProgram *g,int min_prec,int thread_suffix);
static int parse_call_name(Parser *p,Instr *i);
static int parse_args(Parser *p,GscProgram *g,Instr *i);
/* A computed call starts with two contiguous opening brackets. Keep each
 * closing bracket as its own token: the common sthreads[i]]] has three. */
static int dynamic_open(Parser *p) {return p->tok.type=='['&&peek(p)=='[';}
static int parse_dynamic_target(Parser *p,GscProgram *g,int *target) {
    if(!dynamic_open(p)) {
        fail(p,p->tok.line,p->tok.col,"expected '[[' computed call target");return 0;
    }
    next(p);
    if(!expect(p,'[',"second '['"))return 0;
    *target=expression(p,g,1);
    if(*target<0||!expect(p,']',"first ']' in computed call")||
       !expect(p,']',"second ']' in computed call"))return 0;
    return 1;
}
/* A thread in an expression has no assumed Value/return type. Store its
 * target, optional receiver, and arguments once; emit NO instruction. A named
 * receiver target is statically bound only when qualified (R69), without
 * evaluating the receiver. An implicit named
 * target is link-checked like I_THREAD (never as a builtin expression call).
 * a = computed target or -1; b = receiver or -1; op = named/computed kind. */
static int parse_thread_expr(Parser *p,GscProgram *g,int receiver) {
    Expr e;Instr call;
    if(!word(p,"thread"))return -1;
    memset(&e,0,sizeof(e));memset(&call,0,sizeof(call));
    e.kind=E_THREAD_CALL;e.a=-1;e.b=receiver;e.target=-1;
    e.owner_module=p->module;e.line=p->tok.line;e.col=p->tok.col;
    e.audit_line=p->tok.line;e.audit_col=p->tok.col;e.audit_offset=p->tok.source_offset;
    next(p);
    if(dynamic_open(p)) {
        e.op=GSC_THREAD_EXPR_COMPUTED;
        if(!parse_dynamic_target(p,g,&e.a)||!parse_args(p,g,&call))return -1;
        g->requires_dynamic_call=1;
    } else {
        e.op=GSC_THREAD_EXPR_NAMED;
        if(!parse_call_name(p,&call)||!parse_args(p,g,&call))return -1;
        str_copy(e.name,sizeof(e.name),call.name);
        str_copy(e.module,sizeof(e.module),call.module);
        if(receiver>=0)g->requires_receiver_method=1;
    }
    e.argc=call.argc;
    memcpy(e.args,call.args,(size_t)e.argc*sizeof(e.args[0]));
    g->requires_thread_expr_semantics=1;
    return new_expr(p,g,e);
}
/* A statement lvalue stops before the method name. An rvalue can include a
 * receiver call as a distinct AST node, without pretending it is a builtin. */
static int postfix(Parser *p,GscProgram *g,int base,int allow_method) {
    Expr e;Instr method;
    while(!p->failed&&(p->tok.type=='.'||
                       (p->tok.type=='['&&(!dynamic_open(p)||allow_method))||
                       (allow_method&&p->tok.type==T_ID&&!word(p,"thread")))) {
        memset(&e,0,sizeof(e));e.a=base;e.b=-1;
        e.audit_line=p->tok.line;e.audit_col=p->tok.col;e.audit_offset=p->tok.source_offset;
        if(accept(p,'.')) {
            if(p->tok.type!=T_ID) {
                fail(p,p->tok.line,p->tok.col,"expected field name");return -1;
            }
            e.kind=E_FIELD;str_copy(e.name,sizeof(e.name),p->tok.word);next(p);
        } else if(dynamic_open(p)) {
            e.line=p->tok.line;
            memset(&method,0,sizeof(method));
            if(!parse_dynamic_target(p,g,&method.expr)||!parse_args(p,g,&method))return -1;
            e.kind=E_DYNAMIC_CALL;e.a=method.expr;e.b=base;e.argc=method.argc;
            memcpy(e.args,method.args,(size_t)e.argc*sizeof(e.args[0]));
            g->requires_dynamic_call=1;
            base=new_expr(p,g,e);
            if(base<0)return -1;
            continue;
        } else if(p->tok.type=='[') {
            next(p);e.kind=E_INDEX;
            e.b=expression(p,g,1);
            if(e.b<0||!expect(p,']',"']'"))return -1;
        } else {
            memset(&method,0,sizeof(method));
            if(!parse_call_name(p,&method)||!parse_args(p,g,&method))return -1;
            e.kind=E_METHOD_CALL;e.argc=method.argc;
            memcpy(e.args,method.args,(size_t)e.argc*sizeof(e.args[0]));
            str_copy(e.name,sizeof(e.name),method.name);
            str_copy(e.module,sizeof(e.module),method.module);
            g->requires_receiver_method=1;
            base=new_expr(p,g,e);
            if(base<0)return -1;
            continue;
        }
        g->requires_game_values=1;
        base=new_expr(p,g,e);
        if(base<0)return -1;
    }
    return base;
}
/* Recognize a cast before consuming '(' in the real parser. Only a precise
 * `(float|int)` prefix with a following operand can be a cast; `(float)` by
 * itself remains a grouped local named float. Minus is ambiguous with binary
 * subtraction, so the fixture treats it as unary cast operand only if it is
 * directly adjacent to ')'. This is a syntax convention, not a CoD1 fact. */
static int cast_prefix(Parser *p,int *kind,Parser *after) {
    Parser probe=*p;
    char probe_error[256]={0};
    int close_line,close_col;
    if(p->tok.type!='(')return 0;
    probe.error=probe_error;probe.error_size=sizeof(probe_error);
    next(&probe);
    if(probe.failed||probe.tok.type!=T_ID)return 0;
    if(!strcmp(probe.tok.word,"float"))*kind=GSC_CAST_FLOAT;
    else if(!strcmp(probe.tok.word,"int"))*kind=GSC_CAST_INT;
    else return 0;
    next(&probe);
    if(probe.failed||probe.tok.type!=')')return 0;
    close_line=probe.tok.line;close_col=probe.tok.col;
    next(&probe);
    if(probe.failed)return 0;
    if(probe.tok.type==T_ID||probe.tok.type==T_NUMBER||
       probe.tok.type==T_STRING||probe.tok.type==T_LOCALIZED_STRING||
       probe.tok.type=='('||probe.tok.type=='['||probe.tok.type=='%'||
       probe.tok.type=='!'||probe.tok.type==T_SCOPE||probe.tok.type=='#'||
       (probe.tok.type=='-'&&probe.tok.line==close_line&&
        probe.tok.col==close_col+1)) {
        probe.error=p->error;probe.error_size=p->error_size;
        *after=probe;return 1;
    }
    return 0;
}
static int primary(Parser *p,GscProgram *g,int thread_suffix) {
    Expr e;Instr dynamic;int out,cast_kind;Parser after;
    memset(&e,0,sizeof(e));e.a=e.b=-1;e.line=p->tok.line;
    e.audit_line=p->tok.line;e.audit_col=p->tok.col;e.audit_offset=p->tok.source_offset;
    if(word(p,"thread")&&thread_suffix) {
        out=parse_thread_expr(p,g,-1);
        return out<0?-1:postfix(p,g,out,1);
    }
    if(dynamic_open(p)) {
        e.line=p->tok.line;
        memset(&dynamic,0,sizeof(dynamic));
        if(!parse_dynamic_target(p,g,&dynamic.expr)||!parse_args(p,g,&dynamic))return -1;
        e.kind=E_DYNAMIC_CALL;e.a=dynamic.expr;e.argc=dynamic.argc;
        memcpy(e.args,dynamic.args,(size_t)e.argc*sizeof(e.args[0]));
        g->requires_dynamic_call=1;
        out=new_expr(p,g,e);
        return out<0?-1:postfix(p,g,out,1);
    }
    if(p->tok.type=='[') {
        /* [] is a value, distinct from postfix indexing and [[target]]().
         * Preserve the node and reject execution until arrays have native
         * GSC allocation, indexing, and mutation semantics. */
        e.kind=E_EMPTY_ARRAY;e.line=p->tok.line;
        next(p);
        if(!expect(p,']',"']' in empty array literal"))return -1;
        g->requires_empty_array_semantics=1;
        out=new_expr(p,g,e);
        return out<0?-1:postfix(p,g,out,1);
    }
    if (p->tok.type==T_NUMBER) {
        e.kind=E_NUMBER;e.number=p->tok.number;e.integer=p->tok.integer;
        e.integer_value=p->tok.integer_value;
        if(g->randomfloat_syntax)str_copy(e.name,sizeof(e.name),p->tok.word);
        out=new_expr(p,g,e);next(p);return out;
    }
    if (p->tok.type==T_STRING||p->tok.type==T_LOCALIZED_STRING) {
        int localized=p->tok.type==T_LOCALIZED_STRING;
        e.kind=p->tok.unknown_escape_count&&!p->native_quoted_strings?
            (localized?E_RAW_LOCALIZED_STRING:E_RAW_STRING):
            (localized?E_LOCALIZED_STRING:E_STRING);
        if(localized)g->requires_localized_string=1;
        e.owner_module=p->module;
        e.a=retain_string(p,g,GSC_STRING_OWNER_EXPR,g->expr_count);
        if(e.a<0)return -1;
        if(!p->tok.unknown_escape_count||p->native_quoted_strings)str_copy(e.name,sizeof(e.name),p->tok.word);
        out=new_expr(p,g,e);next(p);return out;
    }
    if (p->tok.type=='#') {
        int line=p->tok.line,col=p->tok.col;
        next(p);
        /* Keep this expression atom distinct from the top-level
         * #using_animtree directive. The exact spelling is retained as a
         * node kind, without inventing a tree value or native lookup. */
        if(!word(p,"animtree")||p->tok.line!=line||p->tok.col!=col+1) {
            fail(p,line,col,"expected '#animtree' value");return -1;
        }
        e.kind=E_ANIMTREE_VALUE;e.line=line;if(g->animation_namespace_selected)e.owner_module=p->module;
        g->requires_animtree_value_semantics=1;
        out=new_expr(p,g,e);next(p);
        return out<0?-1:postfix(p,g,out,1);
    }
    if (p->tok.type=='%') {
        int line=p->tok.line,col=p->tok.col;
        next(p);
        /* A reference is one contiguous %name atom. The same '%' between
         * expressions is retained as a separate binary modulo node. */
        if(p->tok.type!=T_ID||p->tok.line!=line||p->tok.col!=col+1) {
            fail(p,line,col,"expected animation reference name after '%%'");return -1;
        }
        e.kind=E_ANIM_REF;e.owner_module=p->module;
        str_copy(e.name,sizeof(e.name),p->tok.word);
        g->requires_anim_ref_semantics=1;
        out=new_expr(p,g,e);next(p);return out;
    }
    if (!g->names_syntax&&word(p,"isdefined")) {
        e.kind=E_ISDEFINED;e.owner_module=p->module;next(p);
        if (!expect(p,'(',"'('") || (e.a=expression(p,g,1))<0 ||
            !expect(p,')',"')'"))return -1;
        out=new_expr(p,g,e);
        return out<0?-1:postfix(p,g,out,1);
    }
    if(p->tok.type==T_SCOPE) {
        e.kind=E_FUNC_REF;e.owner_module=p->module;e.target=-1;
        next(p);
        if(p->tok.type!=T_ID||strlen(p->tok.word)>=64) {
            fail(p,p->tok.line,p->tok.col,"invalid local function reference name");return -1;
        }
        str_copy(e.name,sizeof(e.name),p->tok.word);
        next(p);
        g->requires_func_ref_semantics=1;
        out=new_expr(p,g,e);
        return out<0?-1:postfix(p,g,out,1);
    }
    if (p->tok.type==T_ID&&!word(p,"thread")&&!word(p,"wait")&&
        !word(p,"if")) {
        Parser before=*p;
        Instr named;
        int scoped=0;
        str_copy(e.name,sizeof(e.name),p->tok.word);
        /* These are GSC literals, not missing local variables. Do not let
         * the fixture's undefined-local convention assign them a meaning. */
        if(literal_name(e.name))g->requires_literal_semantics=1;
        if(!strcmp(e.name,"level")||!strcmp(e.name,"self"))
            g->requires_game_values=1;
        next(p);
        if(p->tok.type==T_SCOPE||p->tok.type=='\\') {
            /* A scope or backslash is unambiguously a module path here. */
            *p=before;memset(&named,0,sizeof(named));
            if(!parse_call_name(p,&named))return -1;
            scoped=1;
        } else if(p->tok.type=='/') {
            /* Slash is also division. Probe a complete module path on a
             * parser copy, committing only after a real '::' was found. */
            Parser probe=before;char probe_error[256]={0};
            memset(&named,0,sizeof(named));
            probe.error=probe_error;probe.error_size=sizeof(probe_error);
            if(parse_call_name(&probe,&named)&&named.module[0]) {
                probe.error=before.error;probe.error_size=before.error_size;
                *p=probe;scoped=1;
            }
        }
        if(scoped) {
            if(!named.module[0]) {
                fail(p,p->tok.line,p->tok.col,"expected qualified function name");return -1;
            }
            if(p->tok.type!='(') {
                e.kind=E_FUNC_REF;e.line=before.tok.line;
                e.owner_module=p->module;e.target=-1;
                str_copy(e.name,sizeof(e.name),named.name);
                str_copy(e.module,sizeof(e.module),named.module);
                g->requires_func_ref_semantics=1;
                out=new_expr(p,g,e);
                return out<0?-1:postfix(p,g,out,1);
            }
            memset(&dynamic,0,sizeof(dynamic));
            if(!parse_args(p,g,&dynamic))return -1;
            e.kind=E_SCOPED_CALL;e.line=before.tok.line;e.target=-1;
            str_copy(e.name,sizeof(e.name),named.name);
            str_copy(e.module,sizeof(e.module),named.module);
            e.argc=dynamic.argc;
            memcpy(e.args,dynamic.args,(size_t)e.argc*sizeof(e.args[0]));
            g->requires_scoped_expr_call=1;
            out=new_expr(p,g,e);
            return out<0?-1:postfix(p,g,out,1);
        }
        if(accept(p,'(')) {
            e.kind=E_NATIVE_CALL;e.owner_module=p->module;
            if(p->tok.type!=')')do {
                if(e.argc>=MAX_PARAMS) {
                    fail(p,p->tok.line,p->tok.col,"too many expression call arguments");return -1;
                }
                e.args[e.argc]=expression(p,g,1);
                if(e.args[e.argc++]<0)return -1;
            } while(accept(p,','));
            if(!expect(p,')',"')'"))return -1;
        } else e.kind=E_VAR;
        out=new_expr(p,g,e);
        if(out<0)return -1;
        return postfix(p,g,out,1);
    }
    if (p->tok.type=='-'||p->tok.type=='!'||(g->integer_bitwise_selected&&p->tok.type=='~')) {
        e.kind=E_UNARY;e.op=p->tok.type;next(p);e.a=expression_mode(p,g,g->integer_bitwise_selected?10:8,thread_suffix);
        if (e.a<0)return -1;
        return new_expr(p,g,e);
    }
    if (cast_prefix(p,&cast_kind,&after)) {
        e.kind=E_CAST;e.op=cast_kind;e.line=p->tok.line;
        *p=after;
        e.a=expression_mode(p,g,g->integer_bitwise_selected?10:8,thread_suffix);
        if(e.a<0)return -1;
        g->requires_cast_semantics=1;
        out=new_expr(p,g,e);
        return out<0?-1:postfix(p,g,out,1);
    }
    if (accept(p,'(')) {
        /* Parentheses group one expression; a comma selects a three-part
         * CoD GSC vector. Keep all three component expression roots in IR. */
        out=expression(p,g,1);
        if(out<0)return -1;
        if(accept(p,',')) {
            e.kind=E_VECTOR;e.argc=3;e.args[0]=out;
            e.args[1]=expression(p,g,1);
            if(e.args[1]<0||!expect(p,',',"','"))return -1;
            e.args[2]=expression(p,g,1);
            if(e.args[2]<0||!expect(p,')',"')'"))return -1;
            g->requires_vector_semantics=1;
            out=new_expr(p,g,e);
            if(out<0)return -1;
        } else if(!expect(p,')',"')'"))return -1;
        return postfix(p,g,out,1);
    }
    if (!p->failed) fail(p,p->tok.line,p->tok.col,"expected expression");
    return -1;
}
static int expression_mode(Parser *p,GscProgram *g,int min_prec,int thread_suffix) {
    int lhs,rhs,op,prec,line;Expr e;
    if (++p->expr_depth>64) {
        fail(p,p->tok.line,p->tok.col,"expression nesting limit");
        p->expr_depth--;return -1;
    }
    lhs=primary(p,g,thread_suffix);
    if (lhs<0) {p->expr_depth--;return -1;}
    /* Fixture syntax convention: a thread invocation binds like a postfix
     * call, before binary operators. Grouping can select a compound receiver.
     * This is NOT a recovered original precedence/evaluation table. The
     * statement receiver path passes 0 and retains its previous parse. */
    while(thread_suffix&&word(p,"thread")) {
        lhs=parse_thread_expr(p,g,lhs);
        if(lhs>=0)lhs=postfix(p,g,lhs,1);
        if(lhs<0) {p->expr_depth--;return -1;}
    }
    while ((prec=integer_bitwise_precedence(g,p->tok.type))>=min_prec) {
        op=p->tok.type;line=p->tok.line;next(p);
        rhs=expression_mode(p,g,prec+1,thread_suffix);
        if (rhs<0) {p->expr_depth--;return -1;}
        memset(&e,0,sizeof(e));e.kind=E_BINARY;e.op=op;e.a=lhs;e.b=rhs;e.line=line;
        if(op=='%'||op=='&')g->requires_integer_op_semantics=1;
        lhs=new_expr(p,g,e);
        if (lhs<0) {p->expr_depth--;return -1;}
    }
    p->expr_depth--;
    return lhs;
}
static int expression(Parser *p,GscProgram *g,int min_prec) {
    return expression_mode(p,g,min_prec,1);
}
static int emit(Parser *p,GscProgram *g,Instr i) {
    if(g->local_identifier_case_selected&&i.kind==I_SET)local_identifier_case(i.name);
    if (g->code_count>=MAX_CODE) {fail(p,i.line,1,"instruction limit");return -1;}
    if(g->code_count==g->code_capacity) {
        Instr *fresh=grow_table(p,g->code,&g->code_capacity,MAX_CODE,sizeof(*g->code),"instruction");
        if(!fresh)return -1;
        g->code=fresh;
    }
    i.receiver_binding_kind=GSC_RECEIVER_TARGET_NONE;i.receiver_script_target=-1;i.receiver_native_target=-1;
    i.receiver_event_target=-1;
    g->code[g->code_count]=i;return g->code_count++;
}
static int parse_stmt(Parser *p,GscProgram *g,int depth);
static int parse_dev_region(Parser *p,GscProgram *g,int file_scope,int depth);
static int parse_block(Parser *p,GscProgram *g,int depth) {
    if (depth>32) {fail(p,p->tok.line,p->tok.col,"block nesting limit");return 0;}
    if (!expect(p,'{',"'{'"))return 0;
    while (!p->failed&&p->tok.type!='}'&&p->tok.type!=T_EOF)
        if (!parse_stmt(p,g,depth+1))return 0;
    return expect(p,'}',"'}'");
}
static int parse_args(Parser *p,GscProgram *g,Instr *i) {
    if (!expect(p,'(',"'('"))return 0;
    if (p->tok.type!=')') {
        do {
            if (i->argc>=MAX_PARAMS) {fail(p,p->tok.line,p->tok.col,"too many arguments");return 0;}
            i->args[i->argc++]=expression(p,g,1);
            if (p->failed||i->args[i->argc-1]<0)return 0;
        } while (accept(p,','));
    }
    return expect(p,')',"')'");
}
/* Parse a local function or a path-qualified maps\foo::bar function. */
static int parse_call_name(Parser *p,Instr *i) {
    char path[MAX_MODULE_PATH];size_t used;
    if(p->tok.type!=T_ID) {fail(p,p->tok.line,p->tok.col,"expected function name");return 0;}
    str_copy(path,sizeof(path),p->tok.word);next(p);
    if(p->tok.type!='\\'&&p->tok.type!='/') {
        if(p->tok.type==T_SCOPE) {
            next(p);
            if(p->tok.type!=T_ID||strlen(p->tok.word)>=64) {
                fail(p,p->tok.line,p->tok.col,"invalid namespaced function name");return 0;
            }
            str_copy(i->module,sizeof(i->module),path);
            str_copy(i->name,sizeof(i->name),p->tok.word);next(p);
            return 1;
        }
        str_copy(i->name,sizeof(i->name),path);return 1;
    }
    while(p->tok.type=='\\'||p->tok.type=='/') {
        next(p);
        if(p->tok.type!=T_ID) {fail(p,p->tok.line,p->tok.col,"expected module path component");return 0;}
        used=strlen(path);
        if(used+1+strlen(p->tok.word)>=sizeof(path)) {
            fail(p,p->tok.line,p->tok.col,"module path too long");return 0;
        }
        path[used++]='/';strcpy(path+used,p->tok.word);next(p);
    }
    if(!expect(p,T_SCOPE,"'::'"))return 0;
    if(p->tok.type!=T_ID||strlen(p->tok.word)>=64) {
        fail(p,p->tok.line,p->tok.col,"invalid namespaced function name");return 0;
    }
    str_copy(i->module,sizeof(i->module),path);
    str_copy(i->name,sizeof(i->name),p->tok.word);next(p);
    return 1;
}
/* Read-only token lookahead, not speculative AST compilation. A top-level
 * `thread` after a receiver selects expression parsing for that statement.
 * Strings/comments are lexed as units and don't create retained literals.
 * Assignment RHSs / nested thread expressions do not select this path.
 * R64 handles those in expression(), without emitting a statement. Field and qualified names named thread are not delimiters.
 * The normal parser, not this selector, validates all bracket pairs/syntax. */
static int receiver_thread_ahead(const Parser *p) {
    Parser probe=*p;char error[256]={0};int parens=0,brackets=0,previous=0;
    probe.error=error;probe.error_size=sizeof(error);
    while(!probe.failed&&probe.tok.type!=T_EOF) {
        int t=probe.tok.type;
        if(!parens&&!brackets) {
            if(t==';'||t=='{'||t=='}'||t==T_DEV_CLOSE||t==T_DEV_OPEN||
               t=='='||t==T_ADD_ASSIGN||t==T_SUB_ASSIGN||t==T_MUL_ASSIGN||
               t==T_DIV_ASSIGN||t==T_INC||t==T_DEC||t==T_BIT_OR_ASSIGN||
               t==T_BIT_XOR_ASSIGN||t==T_BIT_AND_ASSIGN)return 0;
            if(word(&probe,"thread")&&previous&&previous!='.'&&
               previous!=T_SCOPE&&previous!='/'&&previous!='\\')return 1;
        }
        if(t=='(')parens++;
        else if(t==')') {if(!parens)return 0;parens--;}
        else if(t=='[')brackets++;
        else if(t==']') {if(!brackets)return 0;brackets--;}
        previous=t;next(&probe);
    }
    return 0;
}
/* Parse the target ONCE, separately from its already-built receiver tree.
 * A method thread is never emitted as an ordinary thread with implicit self.
 * Static methods remain unresolved by this fixture's tiny native registry;
 * computed targets retain their own full expression, with the existing guard. */
static int parse_receiver_call(Parser *p,GscProgram *g,Instr *i,int receiver) {
    int threaded=word(p,"thread");
    i->receiver=receiver;i->name[0]=i->module[0]=0;i->argc=0;i->expr=-1;
    if(threaded)next(p);
    if(dynamic_open(p)) {
        i->kind=threaded?I_DYNAMIC_THREAD:I_DYNAMIC_CALL;
        if(!parse_dynamic_target(p,g,&i->expr)||!parse_args(p,g,i))return 0;
        g->requires_dynamic_call=1;
    } else {
        i->kind=threaded?I_METHOD_THREAD:I_METHOD;
        if(!parse_call_name(p,i)||!parse_args(p,g,i))return 0;
        g->requires_receiver_method=1;
    }
    return 1;
}
static int for_lvalue(Parser *p,GscProgram *g) {
    Expr root;int receiver;
    if(p->tok.type!=T_ID) {
        fail(p,p->tok.line,p->tok.col,"expected for-loop assignment target");return -1;
    }
    memset(&root,0,sizeof(root));root.kind=E_VAR;
    str_copy(root.name,sizeof(root.name),p->tok.word);
    if(literal_name(root.name))g->requires_literal_semantics=1;
    if(!strcmp(root.name,"level")||!strcmp(root.name,"self"))g->requires_game_values=1;
    next(p);
    receiver=new_expr(p,g,root);
    return receiver<0?-1:postfix(p,g,receiver,0);
}
static int mutation_op(int token) {
    switch(token) {
      case T_ADD_ASSIGN:return GSC_MUT_ADD;
      case T_SUB_ASSIGN:return GSC_MUT_SUB;
      case T_MUL_ASSIGN:return GSC_MUT_MUL;
      case T_DIV_ASSIGN:return GSC_MUT_DIV;
      case T_INC:return GSC_MUT_INC;
      case T_DEC:return GSC_MUT_DEC;
      case T_BIT_OR_ASSIGN:return GSC_MUT_BIT_OR;
      case T_BIT_XOR_ASSIGN:return GSC_MUT_BIT_XOR;
      case T_BIT_AND_ASSIGN:return GSC_MUT_BIT_AND;
      default:return 0;
    }
}
static int integer_bitwise_mutation_op(int op) {
    return op==GSC_MUT_BIT_OR||op==GSC_MUT_BIT_XOR||op==GSC_MUT_BIT_AND;
}
/* Do not rewrite a mutation into a read/add/write sequence: even the source
 * local-variable semantics do not establish the game's field/index rules. */
static int parse_mutation(Parser *p,GscProgram *g,Instr *i,int lvalue) {
    int op=mutation_op(p->tok.type);
    if(!op||lvalue<0)return 0;
    if(g->expr[lvalue].kind!=E_VAR&&g->expr[lvalue].kind!=E_FIELD&&
       g->expr[lvalue].kind!=E_INDEX) {
        fail(p,p->tok.line,p->tok.col,"invalid mutation lvalue");return 0;
    }
    i->kind=I_MUTATE;i->dest=lvalue;i->op=op;i->expr=-1;
    g->requires_mutation_semantics=1;
    next(p);
    if(op!=GSC_MUT_INC&&op!=GSC_MUT_DEC) {
        i->expr=expression(p,g,1);
        if(i->expr<0)return 0;
    }
    return 1;
}
/* Store IR owner instructions, not speculative VM jump addresses. The
 * code array can realloc, so contexts must never contain Instr pointers. */
static int push_control(Parser *p,int kind,int instruction) {
    if(p->control_depth>=MAX_CONTROL_DEPTH) {
        fail(p,p->tok.line,p->tok.col,"control nesting limit");return 0;
    }
    p->controls[p->control_depth].kind=kind;
    p->controls[p->control_depth++].instruction=instruction;
    return 1;
}
static int parse_for(Parser *p,GscProgram *g,int depth) {
    Instr i;ForNode loop;int index,instruction,ok;
    memset(&loop,0,sizeof(loop));loop.line=p->tok.line;
    loop.init_target=loop.init_expr=loop.cond_expr=-1;
    loop.step_target=loop.step_expr=-1;loop.step_kind=FOR_NO_STEP;
    g->requires_loop_semantics=1;
    next(p);
    if(!expect(p,'(',"'('"))return 0;
    if(p->tok.type!=';') {
        loop.init_target=for_lvalue(p,g);
        if(loop.init_target<0||!expect(p,'=',"'='"))return 0;
        loop.init_expr=expression(p,g,1);
        if(loop.init_expr<0)return 0;
    }
    if(!expect(p,';',"';'"))return 0;
    if(p->tok.type!=';') {
        loop.cond_expr=expression(p,g,1);
        if(loop.cond_expr<0)return 0;
    }
    if(!expect(p,';',"';'"))return 0;
    if(p->tok.type!=')') {
        int op;
        loop.step_target=for_lvalue(p,g);
        if(loop.step_target<0)return 0;
        if(accept(p,'=')) {
            loop.step_kind=FOR_ASSIGN;
            loop.step_expr=expression(p,g,1);
            if(loop.step_expr<0)return 0;
        } else if(((op=mutation_op(p->tok.type))>=GSC_MUT_ADD&&op<=GSC_MUT_DIV)||
                  (g->integer_bitwise_selected&&integer_bitwise_mutation_op(op))) {
            /* Share lvalue/RHS validation with statement mutations, but retain
             * the result in the header. This temporary Instr is NEVER emitted:
             * a continue still owns this loop, not a guessed instruction PC. */
            Instr mutation;memset(&mutation,0,sizeof(mutation));
            if(!parse_mutation(p,g,&mutation,loop.step_target))return 0;
            loop.step_kind=FOR_COMPOUND_ASSIGN;
            loop.step_expr=mutation.expr;loop.step_op=mutation.op;
        } else if((op=p->tok.type)==T_INC||op==T_DEC) {
            next(p);
            loop.step_kind=op==T_INC?FOR_INCREMENT:FOR_DECREMENT;
        } else if((op=p->tok.type)=='+'||op=='-') {
            /* Retain R52's spaced header form; statement mutations require
             * one contiguous operator token. */
            next(p);
            if(!expect(p,op,op=='+'?"second '+'":"second '-'"))return 0;
            loop.step_kind=op=='+'?FOR_INCREMENT:FOR_DECREMENT;
        } else {
            fail(p,p->tok.line,p->tok.col,"expected for-loop assignment (=, +=, -=, *=, /=) or ++/--");return 0;
        }
    }
    if(!expect(p,')',"')'"))return 0;
    if(g->loop_count>=MAX_LOOPS) {
        fail(p,loop.line,1,"loop limit");return 0;
    }
    if(g->loop_count==g->loop_capacity) {
        ForNode *fresh=grow_table(p,g->loops,&g->loop_capacity,MAX_LOOPS,sizeof(*g->loops),"loop");
        if(!fresh)return 0;
        g->loops=fresh;
    }
    index=g->loop_count++;
    memset(&i,0,sizeof(i));i.kind=I_FOR;i.line=loop.line;i.dest=index;
    i.owner_module=p->module;i.expr=-1;i.receiver=-1;i.target=-1;
    instruction=emit(p,g,i);if(instruction<0)return 0;
    loop.body_start=g->code_count;
    /* Parse every body through the same statement path. A lone semicolon emits
     * I_EMPTY; unlike an empty brace block it occupies one IR instruction. */
    if(!push_control(p,I_FOR,instruction))return 0;
    ok=parse_stmt(p,g,depth+1);
    p->control_depth--;
    if(!ok)return 0;
    loop.body_end=g->code_count;
    g->loops[index]=loop;
    return 1;
}
static int parse_while(Parser *p,GscProgram *g,int depth) {
    Instr i;WhileNode loop;int index,instruction,ok;
    memset(&loop,0,sizeof(loop));loop.line=p->tok.line;
    g->requires_flow_semantics=1;
    next(p);
    if(!expect(p,'(',"'('"))return 0;
    loop.cond_expr=expression(p,g,1);
    if(loop.cond_expr<0||!expect(p,')',"')'"))return 0;
    if(g->while_count>=MAX_LOOPS) {
        fail(p,loop.line,1,"while-loop limit");return 0;
    }
    if(g->while_count==g->while_capacity) {
        WhileNode *fresh=grow_table(p,g->whiles,&g->while_capacity,MAX_LOOPS,sizeof(*g->whiles),"while-loop");
        if(!fresh)return 0;
        g->whiles=fresh;
    }
    index=g->while_count++;
    memset(&i,0,sizeof(i));i.kind=I_WHILE;i.line=loop.line;i.dest=index;
    i.owner_module=p->module;i.expr=loop.cond_expr;i.receiver=-1;i.target=-1;
    instruction=emit(p,g,i);if(instruction<0)return 0;
    loop.body_start=g->code_count;
    if(!push_control(p,I_WHILE,instruction))return 0;
    ok=parse_stmt(p,g,depth+1);
    p->control_depth--;
    if(!ok)return 0;
    loop.body_end=g->code_count;
    g->whiles[index]=loop;
    return 1;
}
/* Post-test syntax is retained in source order: one owner before its body,
 * one condition AST parsed AFTER the body, and no generated while/jump/empty
 * terminator. Keep only indexes across recursion/reallocations. This is not
 * execution or a claim about original bytecode/VM scheduling. */
static int parse_do_while(Parser *p,GscProgram *g,int depth) {
    DoWhileNode loop;Instr i;int index,instruction,ok;
    memset(&loop,0,sizeof(loop));loop.module=p->module;
    loop.line=p->tok.line;loop.col=p->tok.col;loop.cond_expr=-1;
    g->requires_do_while_semantics=1;g->requires_flow_semantics=1;
    next(p);if(p->failed)return 0;
    if(g->do_while_count>=MAX_LOOPS) {
        fail(p,loop.line,loop.col,"do-while loop limit");return 0;
    }
    if(g->do_while_count==g->do_while_capacity) {
        DoWhileNode *fresh=grow_table(p,g->do_whiles,&g->do_while_capacity,
                                     MAX_LOOPS,sizeof(*g->do_whiles),"do-while loop");
        if(!fresh)return 0;
        g->do_whiles=fresh;
    }
    index=g->do_while_count++;
    memset(&i,0,sizeof(i));i.kind=I_DO_WHILE;i.line=loop.line;i.col=loop.col;
    i.dest=index;i.expr=i.receiver=i.target=-1;i.owner_module=p->module;
    instruction=emit(p,g,i);if(instruction<0)return 0;
    loop.instruction=instruction;loop.body_start=g->code_count;
    if(!push_control(p,I_DO_WHILE,instruction))return 0;
    ok=parse_stmt(p,g,depth+1);
    p->control_depth--;
    if(!ok)return 0;
    loop.body_end=g->code_count;
    if(!word(p,"while")) {
        fail(p,p->tok.line,p->tok.col,"expected while after do body");return 0;
    }
    loop.while_line=p->tok.line;loop.while_col=p->tok.col;
    next(p);
    if(!expect(p,'(',"'('"))return 0;
    loop.cond_expr=expression(p,g,1);
    if(loop.cond_expr<0||!expect(p,')',"')'"))return 0;
    loop.end_line=p->tok.line;loop.end_col=p->tok.col;
    if(!expect(p,';',"';' after do-while condition"))return 0;
    g->do_whiles[index]=loop;
    g->code[instruction].expr=loop.cond_expr;
    return 1;
}
/* Preserve exact integer label text via strtol, never through float32:
 * 16777216 and 16777217 must be distinct labels. The existing tokeniser's
 * magnitude limit remains in force (including its INT_MIN limitation). */
static int parse_case_label(Parser *p,GscProgram *g,int owner) {
    CaseNode label;int n,index,is_default=word(p,"default"),sign=1;
    memset(&label,0,sizeof(label));label.owner_switch=owner;label.line=p->tok.line;
    label.next_case=-1;label.body_start=g->code_count;label.body_end=-1;label.string_literal=-1;
    next(p);
    if(is_default)label.kind=GSC_CASE_DEFAULT;
    else {
        if(accept(p,'-'))sign=-1;
        if(p->tok.type==T_NUMBER&&p->tok.integer) {
            label.kind=GSC_CASE_INTEGER;
            label.integer=sign*(int)strtol(p->tok.word,NULL,10);
        } else if(sign==1&&p->tok.type==T_STRING) {
            label.kind=p->tok.unknown_escape_count&&!p->native_quoted_strings?GSC_CASE_RAW_STRING:GSC_CASE_STRING;
            label.string_literal=retain_string(p,g,GSC_STRING_OWNER_CASE,g->case_count);
            if(label.string_literal<0)return 0;
            if(!p->tok.unknown_escape_count||p->native_quoted_strings)str_copy(label.string,sizeof(label.string),p->tok.word);
        } else {
            fail(p,p->tok.line,p->tok.col,"case requires integer or ordinary string literal");
            return 0;
        }
        next(p);
    }
    if(!expect(p,':',"':' after case/default label"))return 0;
    for(n=g->switches[owner].first_case;n>=0;n=g->cases[n].next_case) {
        const CaseNode *old=&g->cases[n];
        if(old->kind==label.kind&&
           (label.kind==GSC_CASE_DEFAULT||
            (label.kind==GSC_CASE_INTEGER&&old->integer==label.integer)||
            (label.kind==GSC_CASE_STRING&&!strcmp(old->string,label.string))||
            (label.kind==GSC_CASE_RAW_STRING&&
             !strcmp(g->strings[old->string_literal].raw,g->strings[label.string_literal].raw)))) {
            fail(p,label.line,1,"duplicate %s label%s",is_default?"default":"case",
                 label.kind==GSC_CASE_RAW_STRING?" (identical raw spelling; value unresolved)":"");return 0;
        }
    }
    if(g->case_count>=MAX_CASES) {fail(p,label.line,1,"case limit");return 0;}
    if(g->case_count==g->case_capacity) {
        CaseNode *fresh=grow_table(p,g->cases,&g->case_capacity,MAX_CASES,
                                   sizeof(*g->cases),"case");
        if(!fresh)return 0;
        g->cases=fresh;
    }
    index=g->case_count++;g->cases[index]=label;
    if(g->switches[owner].last_case>=0) {
        n=g->switches[owner].last_case;
        g->cases[n].next_case=index;g->cases[n].body_end=g->code_count;
    } else g->switches[owner].first_case=index;
    g->switches[owner].last_case=index;g->switches[owner].case_count++;
    if(is_default)g->switches[owner].default_case=index;
    return 1;
}
static int parse_switch(Parser *p,GscProgram *g,int depth) {
    SwitchNode sw;Instr i;int owner,instruction,ok=1;
    memset(&sw,0,sizeof(sw));sw.line=p->tok.line;sw.module=p->module;
    sw.first_case=sw.last_case=sw.default_case=-1;sw.body_end=-1;
    g->requires_switch_semantics=1;
    next(p);
    if(!expect(p,'(',"'('"))return 0;
    sw.selector_expr=expression(p,g,1);
    if(sw.selector_expr<0||!expect(p,')',"')'")||!expect(p,'{',"'{'") )return 0;
    if(g->switch_count>=MAX_SWITCHES) {fail(p,sw.line,1,"switch limit");return 0;}
    if(g->switch_count==g->switch_capacity) {
        SwitchNode *fresh=grow_table(p,g->switches,&g->switch_capacity,MAX_SWITCHES,
                                     sizeof(*g->switches),"switch");
        if(!fresh)return 0;
        g->switches=fresh;
    }
    owner=g->switch_count++;
    memset(&i,0,sizeof(i));i.kind=I_SWITCH;i.line=sw.line;i.dest=owner;
    i.owner_module=p->module;i.expr=sw.selector_expr;i.receiver=-1;i.target=-1;
    instruction=emit(p,g,i);if(instruction<0)return 0;
    sw.instruction=instruction;sw.body_start=g->code_count;
    g->switches[owner]=sw;
    if(!push_control(p,I_SWITCH,instruction))return 0;
    while(!p->failed&&p->tok.type!='}'&&p->tok.type!=T_EOF) {
        if(word(p,"case")||word(p,"default"))ok=parse_case_label(p,g,owner);
        else if(g->switches[owner].first_case<0) {
            fail(p,p->tok.line,p->tok.col,"switch body requires case/default label");ok=0;
        } else ok=parse_stmt(p,g,depth+1);
        if(!ok)break;
    }
    p->control_depth--;
    if(!ok||!expect(p,'}',"'}' in switch"))return 0;
    g->switches[owner].body_end=g->code_count;
    if(g->switches[owner].last_case>=0)
        g->cases[g->switches[owner].last_case].body_end=g->code_count;
    return 1;
}
static int parse_stmt(Parser *p,GscProgram *g,int depth) {
    Instr i,j;int branch,skip;
    if (p->failed)return 0;
    if (p->tok.type==';') {
        /* A terminal empty body adds no nesting. Handle it before the nesting
         * check, preserving R59's accepted 32 nested loops ending in ';'.
         * Consume exactly ONE statement: the second ';' in `if(x);; else`
         * must not become part of the if body. No text preprocessing occurs. */
        memset(&i,0,sizeof(i));i.kind=I_EMPTY;i.line=p->tok.line;i.col=p->tok.col;
        i.expr=i.dest=i.receiver=i.target=-1;i.owner_module=p->module;
        next(p);
        return !p->failed&&emit(p,g,i)>=0;
    }
    if (depth>32) {fail(p,p->tok.line,p->tok.col,"statement nesting limit");return 0;}
    if (p->tok.type==T_DEV_OPEN)return parse_dev_region(p,g,0,depth);
    if (p->tok.type==T_DEV_CLOSE) {
        fail(p,p->tok.line,p->tok.col,"stray '#/' developer region closer");return 0;
    }
    if (p->tok.type=='{')return parse_block(p,g,depth);
    if (word(p,"for"))return parse_for(p,g,depth);
    if (word(p,"while"))return parse_while(p,g,depth);
    if (word(p,"do"))return parse_do_while(p,g,depth);
    if (word(p,"switch"))return parse_switch(p,g,depth);
    if (word(p,"case")||word(p,"default")) {
        fail(p,p->tok.line,p->tok.col,"case/default outside switch label list");return 0;
    }
    memset(&i,0,sizeof(i));i.line=p->tok.line;i.col=p->tok.col;
    i.audit_offset=p->tok.source_offset;
    i.expr=-1;i.dest=-1;i.receiver=-1;
    i.owner_module=p->module;i.target=-1;
    if (word(p,"if")) {
        next(p);
        if (!expect(p,'(',"'('"))return 0;
        i.kind=I_JZ;i.expr=expression(p,g,1);
        if (i.expr<0||!expect(p,')',"')'"))return 0;
        branch=emit(p,g,i);
        if (branch<0||!parse_stmt(p,g,depth+1))return 0;
        if (word(p,"else")) {
            memset(&j,0,sizeof(j));j.kind=I_JUMP;j.line=p->tok.line;
            skip=emit(p,g,j);if(skip<0)return 0;
            g->code[branch].dest=g->code_count;
            next(p);
            if (!parse_stmt(p,g,depth+1))return 0;
            g->code[skip].dest=g->code_count;
        } else g->code[branch].dest=g->code_count;
        return 1;
    }
    if (word(p,"return")) {
        i.kind=I_RETURN;g->requires_flow_semantics=1;next(p);
        if(p->tok.type!=';') {
            i.expr=expression(p,g,1);
            if(i.expr<0)return 0;
        }
        if(!expect(p,';',"';'"))return 0;
        return emit(p,g,i)>=0;
    } else if (word(p,"break")||word(p,"continue")) {
        int continuing=word(p,"continue"),context=p->control_depth-1;
        if(continuing)while(context>=0&&p->controls[context].kind!=I_FOR&&
                          p->controls[context].kind!=I_WHILE&&
                          p->controls[context].kind!=I_DO_WHILE)context--;
        if(context<0) {
            fail(p,i.line,p->tok.col,"%s",continuing?"continue outside loop":"break outside loop or switch");
            return 0;
        }
        i.kind=continuing?I_CONTINUE:I_BREAK;
        i.target=p->controls[context].instruction;
        g->requires_flow_semantics=1;next(p);
        if(!expect(p,';',"';'"))return 0;
        return emit(p,g,i)>=0;
    } else if (word(p,"thread")) {
        next(p);
        i.kind=dynamic_open(p)?I_DYNAMIC_THREAD:I_THREAD;
        if(i.kind==I_DYNAMIC_THREAD) {
            if(!parse_dynamic_target(p,g,&i.expr))return 0;
            g->requires_dynamic_call=1;
        } else if (!parse_call_name(p,&i))return 0;
        if (!parse_args(p,g,&i))return 0;
    } else if(!word(p,"wait")&&receiver_thread_ahead(p)) {
        int receiver=expression_mode(p,g,1,0);
        if(receiver<0)return 0;
        if(!word(p,"thread")) {
            fail(p,p->tok.line,p->tok.col,"expected thread after receiver expression");return 0;
        }
        if(!parse_receiver_call(p,g,&i,receiver))return 0;
    } else if(dynamic_open(p)) {
        i.kind=I_DYNAMIC_CALL;
        if(!parse_dynamic_target(p,g,&i.expr)||!parse_args(p,g,&i))return 0;
        g->requires_dynamic_call=1;
    } else if (word(p,"wait")) {
        i.kind=I_WAIT;next(p);i.expr=expression(p,g,1);
        if (i.expr<0)return 0;
    } else if (p->tok.type==T_ID) {
        Expr root;int receiver;
        if (!parse_call_name(p,&i))return 0;
        if(!i.module[0]&&literal_name(i.name))g->requires_literal_semantics=1;
        if(!i.module[0]&&!strcmp(i.name,"level")&&
           (word(p,"waittill")||word(p,"notify"))) {
            i.kind=word(p,"waittill")?I_WAITER:I_NOTIFY;next(p);
            if(g->event_payload_syntax) {
                if(!parse_args(p,g,&i))return 0;
                i.expr=i.argc?i.args[0]:-1;
            } else {
                if(!expect(p,'(',"'('"))return 0;
                i.expr=expression(p,g,1);
                if(i.expr<0||!expect(p,')',"')'"))return 0;
            }
        } else if(p->tok.type=='.'||p->tok.type=='['||mutation_op(p->tok.type)||
                  word(p,"thread")||p->tok.type==T_ID) {
            if(i.module[0]) {fail(p,i.line,1,"qualified assignment unsupported");return 0;}
            memset(&root,0,sizeof(root));root.kind=E_VAR;
            str_copy(root.name,sizeof(root.name),i.name);
            if(!strcmp(i.name,"level")||!strcmp(i.name,"self"))g->requires_game_values=1;
            receiver=new_expr(p,g,root);
            if(receiver<0||(receiver=postfix(p,g,receiver,0))<0)return 0;
            if(accept(p,'=')) {
                if(g->expr[receiver].kind==E_VAR) {
                    i.kind=I_SET;
                } else {
                    i.kind=I_GAME_SET;i.dest=receiver;
                    if(g->expr[receiver].kind==E_FIELD)g->expr[receiver].field_write_target=1;
                }
                i.expr=expression(p,g,1);
                if(i.expr<0)return 0;
            } else if(mutation_op(p->tok.type)) {
                if(!parse_mutation(p,g,&i,receiver))return 0;
            } else if(word(p,"thread")||p->tok.type==T_ID||dynamic_open(p)) {
                if(!parse_receiver_call(p,g,&i,receiver))return 0;
            } else {fail(p,p->tok.line,p->tok.col,"expected assignment or receiver call");return 0;}
        } else if (accept(p,'=')) {
            if(i.module[0]) {fail(p,i.line,1,"qualified assignment unsupported");return 0;}
            if(!strcmp(i.name,"level")||!strcmp(i.name,"self"))g->requires_game_values=1;
            i.kind=I_SET;i.expr=expression(p,g,1);
            if (i.expr<0)return 0;
        } else if (p->tok.type=='(') {
            i.kind=I_CALL;
            if (!parse_args(p,g,&i))return 0;
            /* Original maps use call results directly as statement receivers,
             * e.g. getent("trigger", "targetname") waittill("trigger").
             * Preserve the call as an expression exactly once; the existing
             * receiver binder, event lane and resumable evaluator own it.
             * Bare calls keep their original statement representation. */
            if(p->tok.type=='.'||p->tok.type=='['||word(p,"thread")||p->tok.type==T_ID) {
                memset(&root,0,sizeof(root));root.kind=i.module[0]?E_SCOPED_CALL:E_NATIVE_CALL;
                root.owner_module=p->module;root.target=-1;root.line=i.line;
                root.audit_line=i.line;root.audit_col=i.col;root.audit_offset=i.audit_offset;
                root.argc=i.argc;memcpy(root.args,i.args,(size_t)i.argc*sizeof(i.args[0]));
                str_copy(root.name,sizeof(root.name),i.name);str_copy(root.module,sizeof(root.module),i.module);
                if(root.kind==E_SCOPED_CALL)g->requires_scoped_expr_call=1;
                receiver=new_expr(p,g,root);
                if(receiver<0||(receiver=postfix(p,g,receiver,0))<0)return 0;
                if(accept(p,'=')) {
                    if(g->expr[receiver].kind!=E_FIELD&&g->expr[receiver].kind!=E_INDEX) {
                        fail(p,i.line,i.col,"call result is not an assignment target");return 0;
                    }
                    i.kind=I_GAME_SET;i.dest=receiver;i.receiver=-1;i.argc=0;
                    if(g->expr[receiver].kind==E_FIELD)g->expr[receiver].field_write_target=1;
                    i.expr=expression(p,g,1);if(i.expr<0)return 0;
                } else if(mutation_op(p->tok.type)) {
                    if(!parse_mutation(p,g,&i,receiver))return 0;
                } else if(!parse_receiver_call(p,g,&i,receiver))return 0;
            }
        } else {fail(p,p->tok.line,p->tok.col,"expected assignment or call");return 0;}
    } else {fail(p,p->tok.line,p->tok.col,"unsupported statement");return 0;}
    if (!expect(p,';',"';'"))return 0;
    return emit(p,g,i)>=0;
}
static int find_module(const GscProgram *g,const char *path) {
    int n;char canonical[MAX_MODULE_PATH];
    /* Loader mode is selected at construction, and module_compile stores
     * only canonical paths. Normalize the query once, not for every slot.
     * Retain the original comparison for tiny tables (including NULL). */
    if(g->module_loader_mode) {
        if(g->module_count<=2) {
            for(n=0;n<g->module_count;n++)
                if(gsc_module_identity_equal(g->modules[n],path))return n;
            return -1;
        }
        if(!gsc_module_identity_canonical(path,canonical,sizeof(canonical)))return -1;
        path=canonical;
    }
    for(n=0;n<g->module_count;n++)
        if(!strcmp(g->modules[n],path))return n;
    return -1;
}
static int developer_func_hidden(const GscProgram *,int);
static int find_func(const GscProgram *g,int module,const char *s) {
    int n,first=0,end=g->func_count;
    /* Modules append all their declarations together. Admission views preserve their order. */
    while(first<end){
        int middle=first+(end-first)/2;
        if(g->funcs[middle].module<module)first=middle+1;
        else end=middle;
    }
    for(n=first;n<g->func_count&&g->funcs[n].module==module;n++)
        if(callable_name_equal(g,g->funcs[n].name,s)&&
           !(g->map_developer_selected&&(g->map_developer_linking||g->linked)&&developer_func_hidden(g,n)))return n;
    return -1;
}
static int parse_top_item(Parser *p,GscProgram *g) {
    int k;
    Func *fn;
        if(p->tok.type=='#') {
            next(p);
            if(!word(p,"using_animtree")) {
                fail(p,p->tok.line,p->tok.col,"unsupported directive");return 0;
            }
            next(p);
            if(!expect(p,'(',"'('") )return 0;
            if(p->tok.type!=T_STRING||!p->tok.raw_length) {
                fail(p,p->tok.line,p->tok.col,"expected animtree name string");return 0;
            }
            if(retain_string(p,g,GSC_STRING_OWNER_ANIMTREE,-1)<0)return 0;
            next(p);
            if(!expect(p,')',"')'")||!expect(p,';',"';'"))return 0;
            /* Syntax accepted; runtime must reject until animtree loading and
             * scope semantics are implemented. */
            g->requires_animtree=1;
            return 1;
        }
        if (p->tok.type!=T_ID) {fail(p,p->tok.line,p->tok.col,"expected function declaration");return 0;}
        if (g->func_count>=MAX_FUNCS) {fail(p,p->tok.line,p->tok.col,"function limit");return 0;}
        if (strlen(p->tok.word)>=64) {fail(p,p->tok.line,p->tok.col,"function name too long");return 0;}
        if (find_func(g,p->module,p->tok.word)>=0) {fail(p,p->tok.line,p->tok.col,"duplicate function %s",p->tok.word);return 0;}
        if(g->func_count==g->func_capacity) {
            Func *fresh=grow_table(p,g->funcs,&g->func_capacity,MAX_FUNCS,sizeof(*g->funcs),"function");
            if(!fresh)return 0;
            g->funcs=fresh;
        }
        if(g->names_syntax&&(callable_name_equal(g,p->tok.word,"waittill")||
           callable_name_equal(g,p->tok.word,"notify"))) {
            fail(p,p->tok.line,p->tok.col,"event primitive shadowing is unsupported");return 0;
        }
        if (!g->names_syntax&&find_builtin(p->tok.word)>=0) {
            fail(p,p->tok.line,p->tok.col,"builtin cannot be redefined");return 0;
        }
        if(literal_name(p->tok.word))g->requires_literal_semantics=1;
        fn=&g->funcs[g->func_count++];memset(fn,0,sizeof(*fn));fn->module=p->module;
        str_copy(fn->name,sizeof(fn->name),p->tok.word);
        next(p);
        if (!expect(p,'(',"'('"))return 0;
        if (p->tok.type!=')') do {
            if (p->tok.type!=T_ID||fn->argc>=MAX_PARAMS||strlen(p->tok.word)>=64) {
                fail(p,p->tok.line,p->tok.col,"invalid function parameter");return 0;
            }
            if(g->local_identifier_case_selected)local_identifier_case(p->tok.word);
            for(k=0;k<fn->argc;k++)if(!strcmp(fn->params[k],p->tok.word)){
                fail(p,p->tok.line,p->tok.col,"duplicate parameter");return 0;
            }
            if(literal_name(p->tok.word))g->requires_literal_semantics=1;
            if(fn->argc==fn->params_capacity) {
                char (*fresh)[64]=grow_table(p,fn->params,&fn->params_capacity,
                                              MAX_PARAMS,sizeof(*fn->params),"parameter");
                if(!fresh)return 0;
                fn->params=fresh;
            }
            str_copy(fn->params[fn->argc++],sizeof(fn->params[0]),p->tok.word);
            next(p);
        } while(accept(p,','));
        if (!expect(p,')',"')'"))return 0;
        fn->start=g->code_count;fn->expr_start=g->expr_count;
        if (!parse_block(p,g,0))return 0;
        if(p->control_depth!=0) {
            fail(p,p->tok.line,p->tok.col,"unbalanced control scope");return 0;
        }
        fn->end=g->code_count;fn->expr_end=g->expr_count;
    return !p->failed;
}
static void developer_index_code(GscProgram *g);
static int parse_dev_region(Parser *p,GscProgram *g,int file_scope,int depth) {
    DevBlock *block;
    int line=p->tok.line,col=p->tok.col;
    if(p->in_dev_block) {
        fail(p,line,col,"nested '/#' developer region is unsupported");return 0;
    }
    if(g->dev_count>=MAX_DEV_BLOCKS) {
        fail(p,line,col,"developer region limit");return 0;
    }
    if(g->dev_count==g->dev_capacity) {
        DevBlock *fresh=grow_table(p,g->dev_blocks,&g->dev_capacity,
                                   MAX_DEV_BLOCKS,sizeof(*g->dev_blocks),
                                   "developer region");
        if(!fresh)return 0;
        g->dev_blocks=fresh;
    }
    block=&g->dev_blocks[g->dev_count++];
    memset(block,0,sizeof(*block));
    block->scope=file_scope?GSC_DEV_FILE:GSC_DEV_STATEMENT;
    block->module=p->module;block->line=line;block->col=col;
    block->code_start=g->code_count;block->func_start=g->func_count;block->expr_start=g->expr_count;
    g->requires_dev_block=1;p->in_dev_block=1;
    next(p);
    while(!p->failed&&p->tok.type!=T_DEV_CLOSE&&p->tok.type!=T_EOF) {
        if(p->tok.type==T_DEV_OPEN) {
            fail(p,p->tok.line,p->tok.col,"nested '/#' developer region is unsupported");return 0;
        }
        if(file_scope) {
            if(!parse_top_item(p,g))return 0;
        } else if(!parse_stmt(p,g,depth+1))return 0;
    }
    if(p->tok.type!=T_DEV_CLOSE) {
        fail(p,line,col,"unterminated '/#' developer region");return 0;
    }
    block->code_end=g->code_count;block->func_end=g->func_count;block->expr_end=g->expr_count;
    p->in_dev_block=0;next(p);
    return !p->failed;
}
static int parse_program(Parser *p,GscProgram *g) {
    while(!p->failed&&p->tok.type!=T_EOF) {
        if(p->tok.type==T_DEV_OPEN) {
            if(!parse_dev_region(p,g,1,0))return 0;
        } else if(p->tok.type==T_DEV_CLOSE) {
            fail(p,p->tok.line,p->tok.col,"stray '#/' developer region closer");return 0;
        } else if(!parse_top_item(p,g))return 0;
    }
    return !p->failed;
}
GscProgram *gsc_program_create(void) {return calloc(1,sizeof(GscProgram));}
GscProgram *gsc_program_create_with_modules(void) {
    GscProgram *g=gsc_program_create();
    if(g)g->module_loader_mode=1;
    return g;
}
GscProgram *gsc_program_create_with_event_payloads(void) {
    GscProgram *g=gsc_program_create_with_modules();
    if(g)g->event_payload_syntax=1;
    return g;
}

GscProgram *gsc_program_create_with_predicates(void) {
    GscProgram *g=gsc_program_create_with_event_payloads();
    if(g)g->predicate_syntax=1;
    return g;
}

static int function_token_new(uint64_t *);
GscProgram *gsc_program_create_with_actor_spawners(void) {
    GscProgram *g=gsc_program_create_with_predicates();
    if(g)g->actor_spawn_syntax=1;
    return g;
}

GscProgram *gsc_program_create_with_function_refs(void) {
    GscProgram *g=gsc_program_create_with_actor_spawners();
    if(g) {
        if(!function_token_new(&g->function_program_token)){free(g);return NULL;}
        g->function_ref_syntax=1;
    }
    return g;
}

GscProgram *gsc_program_create_with_scalar_switch(void) {
    GscProgram *g=gsc_program_create_with_function_refs();
    if(g)g->scalar_switch_syntax=1;
    return g;
}

GscProgram *gsc_program_create_with_lifetime(void) {
    GscProgram *g=gsc_program_create_with_scalar_switch();
    if(g)g->lifetime_syntax=1;
    return g;
}

GscProgram *gsc_program_create_with_names(void) {
    GscProgram *g=gsc_program_create_with_lifetime();
    if(g)g->names_syntax=1;
    return g;
}
GscProgram *gsc_program_create_with_music(void) {
    GscProgram *g=gsc_program_create_with_names();
    if(g)g->music_syntax=1;
    return g;
}

GscProgram *gsc_program_create_with_resources(void) {
    GscProgram *g=gsc_program_create_with_music();
    if(g)g->resources_syntax=1;
    return g;
}

GscProgram *gsc_program_create_with_objectives(void) {
    GscProgram *g=gsc_program_create_with_resources();
    if(g)g->objectives_syntax=1;
    return g;
}

GscProgram *gsc_program_create_with_console(void) {
    GscProgram *g=gsc_program_create_with_objectives();
    if(g)g->console_syntax=1;
    return g;
}
GscProgram *gsc_program_create_with_hud(void) {
    GscProgram *g=gsc_program_create_with_console();
    if(g)g->hud_syntax=1;
    return g;
}


GscProgram *gsc_program_create_with_mission(void) {
    GscProgram *g=gsc_program_create_with_hud();
    if(g)g->mission_syntax=1;
    return g;
}

GscProgram *gsc_program_create_with_blast(void) {
    GscProgram *g=gsc_program_create_with_mission();
    if(g)g->blast_syntax=1;
    return g;
}
GscProgram *gsc_program_create_with_quake(void) {
    GscProgram *g=gsc_program_create_with_blast();
    if(g)g->quake_syntax=1;
    return g;
}
GscProgram *gsc_program_create_with_fx(void) {
    GscProgram *g=gsc_program_create_with_quake();
    if(g)g->fx_syntax=1;
    return g;
}

GscProgram *gsc_program_create_with_cvars(void) {
    GscProgram *g=gsc_program_create_with_fx();
    if(g)g->cvars_syntax=1;
    return g;
}
GscProgram *gsc_program_create_with_shellshock(void) {
    GscProgram *g=gsc_program_create_with_cvars();
    if(g)g->shellshock_syntax=1;
    return g;
}
GscProgram *gsc_program_create_with_debug_text(void) {
    GscProgram *g=gsc_program_create_with_shellshock();
    if(g)g->debug_text_syntax=1;
    return g;
}
GscProgram *gsc_program_create_with_debug_line(void) {
    GscProgram *g=gsc_program_create_with_debug_text();
    if(g)g->debug_line_syntax=1;
    return g;
}
GscProgram *gsc_program_create_with_ambient(void) {
    GscProgram *g=gsc_program_create_with_debug_line();
    if(g)g->ambient_syntax=1;
    return g;
}
GscProgram *gsc_program_create_with_precache_item(void) {
    GscProgram *g=gsc_program_create_with_ambient();
    if(g)g->precache_item_syntax=1;
    return g;
}
GscProgram *gsc_program_create_with_save_game(void) {
    GscProgram *g=gsc_program_create_with_precache_item();
    if(g)g->save_game_syntax=1;
    return g;
}
GscProgram *gsc_program_create_with_tag_fx(void) {
    GscProgram *g=gsc_program_create_with_save_game();
    if(g)g->tag_fx_syntax=1;
    return g;
}
GscProgram *gsc_program_create_with_native_queries(void) {
    GscProgram *g=gsc_program_create_with_tag_fx();
    if(g)g->native_queries_syntax=1;
    return g;
}
GscProgram *gsc_program_create_with_precache_string(void) {
    GscProgram *g=gsc_program_create_with_native_queries();
    if(g)g->precache_string_syntax=1;
    return g;
}
GscProgram *gsc_program_create_with_spawner_arrays(void) {
    GscProgram *g=gsc_program_create_with_precache_string();
    if(g)g->spawner_arrays_syntax=1;
    return g;
}
GscProgram *gsc_program_create_with_randomint(void) {
    GscProgram *g=gsc_program_create_with_spawner_arrays();
    if(g)g->randomint_syntax=1;
    return g;
}
GscProgram *gsc_program_create_with_randomfloat(void) {
    GscProgram *g=gsc_program_create_with_randomint();
    if(g)g->randomfloat_syntax=1;
    return g;
}
GscProgram *gsc_program_create_with_keybinding(void) {
    GscProgram *g=gsc_program_create_with_randomfloat();
    if(g)g->keybinding_syntax=1;
    return g;
}




static int valid_module_path(const char *path) {
    size_t n;int component=0;
    if(!path||!path[0]||strlen(path)>=MAX_MODULE_PATH)return 0;
    for(n=0;path[n];n++) {
        unsigned char c=(unsigned char)path[n];
        if(c=='/') {
            if(!component)return 0;
            component=0;
        } else if(!component&&(isalpha(c)||c=='_'))component=1;
        else if(component&&(isalnum(c)||c=='_'))continue;
        else return 0;
    }
    return component;
}

int gsc_program_select_module_float_arithmetic(GscProgram *g,const char *module,const GscModuleFloatArithmeticProfile *input) {
    GscModuleFloatArithmeticProfile profile;char canonical[MAX_MODULE_PATH];
    if(!g||!module||!input||g->invalid||g->linked||g->float_arithmetic_selected||!g->dynamic_randomfloat_syntax||
       !gsc_module_identity_canonical(module,canonical,sizeof(canonical))||!valid_module_path(canonical)||find_module(g,canonical)>=0||
       Cod1ModelStringOverlap(input,sizeof(*input),g,sizeof(*g))||Cod1ModelStringOverlap(module,strlen(module)+1,g,sizeof(*g)))return 0;
    profile=*input;if(!float_arithmetic_profile_valid(&profile))return 0;
    str_copy(g->float_arithmetic_module,sizeof(g->float_arithmetic_module),canonical);
    g->float_arithmetic_profile=profile;g->float_arithmetic_selected=1;return 1;
}

int gsc_program_select_timed_animation_arithmetic(GscProgram *g) {
    if(!g||g->invalid||g->linked||!g->float_arithmetic_selected||g->timed_arithmetic_selected)return 0;
    g->timed_arithmetic_selected=1;return 1;
}
int gsc_program_select_vector_arithmetic(GscProgram *g) {
    if(!g||g->invalid||g->linked||g->vector_arithmetic_selected)return 0;
    g->vector_arithmetic_selected=1;return 1;
}
int gsc_program_select_large_sources(GscProgram *g) {
    if(!g||g->invalid||g->linked||g->large_sources_selected)return 0;
    g->large_sources_selected=1;return 1;
}
int gsc_program_select_vector_math(GscProgram *g) {
    if(!g||g->invalid||g->linked||g->vector_math_selected)return 0;
    g->vector_math_selected=1;return 1;
}
int gsc_program_select_vehicle_nodes(GscProgram*g){if(!g||g->invalid||g->linked||g->vehicle_nodes_selected)return 0;g->vehicle_nodes_selected=1;return 1;}
int gsc_program_select_node_queries(GscProgram *g) {
    if(!g||g->invalid||g->linked||g->node_queries_selected)return 0;
    g->node_queries_selected=1;return 1;
}
int gsc_program_select_ai_arrays(GscProgram *g) {
    if(!g||g->invalid||g->linked||g->ai_arrays_selected)return 0;
    g->ai_arrays_selected=1;return 1;
}
int gsc_program_select_r216_items(GscProgram *g) {
    if(!g||g->invalid||g->linked||g->module_count||g->grenade_items_selected)return 0;
    g->grenade_items_selected=1;return 1;
}
int gsc_program_select_hud_elements(GscProgram *g) {
    if(!g||g->invalid||g->linked||g->hud_elements_selected)return 0;
    g->hud_elements_selected=1;return 1;
}
int gsc_program_select_integer_bitwise(GscProgram *g) {
    if(!g||g->invalid||g->linked||g->integer_bitwise_selected||!g->scalar_switch_syntax||g->module_count)return 0;
    g->integer_bitwise_selected=1;return 1;
}
int gsc_program_select_string_int_concat(GscProgram *g) {
    if(!g||g->invalid||g->linked||g->string_int_concat_selected||!g->scalar_switch_syntax)return 0;
    g->string_int_concat_selected=1;return 1;
}
/* ENGINE46C208/46C234 -> 46BDE0: selected compiler emission only.
 * Syntax provenance remains in StringLiteral, including exact raw spelling. */
int gsc_program_select_r232_quoted_strings(GscProgram *g) {
    if(!g||g->invalid||g->linked||g->module_count||g->native_quoted_strings_selected)return 0;
    g->native_quoted_strings_selected=1;return 1;
}

int gsc_program_select_frame_animation_cadence(GscProgram *g) {
    if(!g||g->invalid||g->linked||!g->timed_arithmetic_selected||g->frame_cadence_selected||g->r237_frame_buckets_selected||
       (g->float_arithmetic_profile.precision_bits!=53&&g->float_arithmetic_profile.precision_bits!=64))return 0;
    g->frame_cadence_selected=1;return 1;
}

int gsc_module_compile(GscProgram *g,const char *path,const char *source,
                       char *error,size_t error_size) {
    Parser p;char canonical[MAX_MODULE_PATH];
    if(error&&error_size)error[0]=0;
    if(g&&g->module_loader_mode) {
        if(!gsc_module_identity_canonical(path,canonical,sizeof(canonical))) {
            if(error&&error_size)snprintf(error,error_size,"invalid canonical module path");
            g->invalid=1;return 0;
        }
        path=canonical;
    }
    if(!g||g->invalid||g->linked||g->module_count>=MAX_MODULES||
       !valid_module_path(path)||find_module(g,path)>=0) {
        if(error&&error_size)snprintf(error,error_size,"invalid/duplicate module path or program state");
        if(g)g->invalid=1;
        return 0;
    }
    if(!source||strlen(source)>(g->large_sources_selected?1048576u:MAX_SOURCE)) {
        if(error&&error_size)snprintf(error,error_size,"missing or oversized source");
        g->invalid=1;return 0;
    }
    if(g->module_count==g->module_capacity) {
        char (*fresh)[MAX_MODULE_PATH];
        memset(&p,0,sizeof(p));p.line=p.col=1;p.tok.line=p.tok.col=1;
        p.error=error;p.error_size=error_size;
        fresh=grow_table(&p,g->modules,&g->module_capacity,MAX_MODULES,
                         sizeof(*g->modules),"module");
        if(!fresh) {g->invalid=1;return 0;}
        g->modules=fresh;
    }
    str_copy(g->modules[g->module_count++],MAX_MODULE_PATH,path);
    memset(&p,0,sizeof(p));p.source=source;p.length=strlen(source);p.line=p.col=1;
    p.module=g->module_count-1;p.error=error;p.error_size=error_size;
    p.integer_bitwise_syntax=g->integer_bitwise_selected;
    p.native_quoted_strings=g->native_quoted_strings_selected;
    next(&p);
    if(!parse_program(&p,g)) {g->invalid=1;return 0;}
    return 1;
}

/* Literal-only check: locals and computed expressions may acquire a different
 * value at execution time. This preflight does not promise effect rollback. */
static int known_value_kind(const GscProgram *g,int root) {
    if(root<0||root>=g->expr_count)return -1;
    if(g->expr[root].kind==E_NUMBER)return V_NUMBER;
    if(g->expr[root].kind==E_STRING)return V_STRING;
    if(g->objectives_mode&&g->expr[root].kind==E_LOCALIZED_STRING)return V_LOCALIZED_STRING;
    if(g->arrays_mode&&g->expr[root].kind==E_EMPTY_ARRAY)return V_ARRAY;
    if(g->entity_mode&&g->expr[root].kind==E_VECTOR)return V_VECTOR;
    if(g->struct_mode&&g->expr[root].kind==E_NATIVE_CALL&&find_func(g,g->expr[root].owner_module,g->expr[root].name)<0&&struct_descriptor(g,g->expr[root].name)>=0)return V_STRUCT;
    if(g->flow_mode&&g->expr[root].kind==E_VAR&&literal_name(g->expr[root].name))
        return !strcmp(g->expr[root].name,"undefined")?V_UNDEFINED:V_NUMBER;
    return -1;
}
#include "gsc_vector_math_descriptor.inc"
#include "gsc_vehicle_spawn_descriptor.inc"
#include "gsc_turret_precache_descriptor.inc"
#include "gsc_turret_spawn_descriptor.inc"
#include "gsc_bullet_trace_descriptor.inc"
#include "gsc_node_query_descriptor.inc"
#include "gsc_ai_array_descriptor.inc"
#include "gsc_start_origin_descriptor.inc"
#include "move-delta-descriptor.inc"
#include "stance-descriptor.inc"
#include "animtime-descriptor.inc"
#include "gsc_hud_elem_descriptor.inc"
static int builtin_preflight_args(const GscProgram *g,const Builtin *b,
                                  const int *args,int argc,int line,
                                  char *error,size_t error_size) {
    int n,kind;
    if(argc!=b->arity) {
        if(error&&error_size)snprintf(error,error_size,
            "line %d: %s expects exactly %d arguments, got %d",line,b->name,b->arity,argc);
        return 0;
    }
    for(n=0;n<argc;n++) {
        kind=known_value_kind(g,args[n]);
        if(kind>=0&&!(g->arrays_mode&&kind==V_ARRAY&&!strcmp(b->name,"isdefined"))&&
           !(g->struct_mode&&kind==V_STRUCT&&!strcmp(b->name,"isdefined"))&&
           !(b->accepted[n]&B_VALUE(kind))) {
            if(error&&error_size)snprintf(error,error_size,
                "line %d: %s argument %d has unsupported literal type",line,b->name,n+1);
            return 0;
        }
    }
    return 1;
}
/* R136 compiler-only reaching-definition proof for the separately selected
 * computed randomint lane. Facts never replace or evaluate a script operand.
 * Unknown origins, cycles and unsupported control flow keep native HOLD. */
typedef struct { int definition; int32_t minimum, maximum; } ComputedIntegerFact;
/* Memo entries belong to one function and one descriptor invocation. The
 * bounds are explicit compiler port policy; exhaustion preserves HOLD. Facts
 * cache IR provenance/ranges and never a runtime RNG result or operand Value. */
enum { COMPUTED_INTEGER_MEMO_CAPACITY=256, COMPUTED_INTEGER_PROOF_BUDGET=16384 };
typedef struct { int pc, state; char name[64]; ComputedIntegerFact fact; } ComputedIntegerMemo;
typedef struct { int remaining, count; ComputedIntegerMemo memo[COMPUTED_INTEGER_MEMO_CAPACITY]; } ComputedIntegerProof;
static int computed_integer_expression(const GscProgram *,const Func *,int,int,int,
                                      ComputedIntegerFact *,ComputedIntegerProof *);
static int computed_integer_local(const GscProgram *,const Func *,int,const char *,int,
                                 ComputedIntegerFact *,ComputedIntegerProof *);
static int computed_expression_has(const GscProgram *g,int root,int wanted,int depth) {
    const Expr *e;int k;
    if(root<0||root>=g->expr_count||depth>48)return 0;
    if(root==wanted)return 1;
    e=&g->expr[root];
    switch(e->kind) {
      case E_UNARY:case E_FIELD:case E_ISDEFINED:case E_CAST:
        if(e->a<0||e->a>=root)return 0;
        return computed_expression_has(g,e->a,wanted,depth+1);
      case E_BINARY:case E_INDEX:
        if(e->a<0||e->a>=root||e->b<0||e->b>=root)return 0;
        return computed_expression_has(g,e->a,wanted,depth+1)||
               computed_expression_has(g,e->b,wanted,depth+1);
      case E_NATIVE_CALL:case E_SCOPED_CALL:case E_METHOD_CALL:
      case E_DYNAMIC_CALL:case E_THREAD_CALL:case E_VECTOR:
        if(e->kind==E_METHOD_CALL||e->kind==E_DYNAMIC_CALL||e->kind==E_THREAD_CALL) {
            if(e->a>=0) {
                if(e->a>=root)return 0;
                if(computed_expression_has(g,e->a,wanted,depth+1))return 1;
            }
            if(e->b>=0) {
                if(e->b>=root)return 0;
                if(computed_expression_has(g,e->b,wanted,depth+1))return 1;
            }
        }
        if(e->argc<0||e->argc>MAX_PARAMS)return 0;
        for(k=0;k<e->argc;k++) {
            if(e->args[k]<0||e->args[k]>=root)return 0;
            if(computed_expression_has(g,e->args[k],wanted,depth+1))return 1;
        }
        return 0;
      default:return 0;
    }
}
static int computed_instruction_has(const GscProgram *g,const Instr *i,int wanted) {
    int k;
    switch(i->kind) {
      case I_SET:case I_JZ:case I_WAIT:case I_WAITER:case I_NOTIFY:case I_RETURN:
        return computed_expression_has(g,i->expr,wanted,0);
      case I_GAME_SET:case I_MUTATE:
        return computed_expression_has(g,i->dest,wanted,0)||
               computed_expression_has(g,i->expr,wanted,0);
      case I_CALL:case I_THREAD:case I_METHOD:case I_METHOD_THREAD:
      case I_DYNAMIC_CALL:case I_DYNAMIC_THREAD:
        if(computed_expression_has(g,i->receiver,wanted,0)||
           computed_expression_has(g,i->expr,wanted,0))return 1;
        if(i->argc<0||i->argc>MAX_PARAMS)return 0;
        for(k=0;k<i->argc;k++)if(computed_expression_has(g,i->args[k],wanted,0))return 1;
        return 0;
      default:return 0;
    }
}
/* Structured loops/switches and their exit joins require another proof lane.
 * A new literal definition after such a region can still dominate a later site. */
static int computed_integer_point(const GscProgram *g,const Func *f,int pc) {
    int n;
    if(pc<f->start||pc>=f->end)return 0;
    for(n=0;n<g->loop_count;n++)if(pc>=g->loops[n].body_start-1&&pc<=g->loops[n].body_end)return 0;
    for(n=0;n<g->while_count;n++)if(pc>=g->whiles[n].body_start-1&&pc<=g->whiles[n].body_end)return 0;
    for(n=0;n<g->do_while_count;n++)if(pc>=g->do_whiles[n].body_start-1&&pc<=g->do_whiles[n].body_end)return 0;
    for(n=0;n<g->switch_count;n++)if(pc>=g->switches[n].body_start-1&&pc<=g->switches[n].body_end)return 0;
    return 1;
}
static int computed_predecessor(const GscProgram *g,int from,int to) {
    const Instr *i=&g->code[from];
    if(i->kind==I_JUMP)return i->dest==to;
    if(i->kind==I_RETURN||i->kind==I_BREAK||i->kind==I_CONTINUE)return 0;
    return from+1==to||(i->kind==I_JZ&&i->dest==to);
}
static int computed_only_previous(const GscProgram *g,const Func *f,int pc) {
    int n,count=0;
    if(pc<=f->start||!computed_integer_point(g,f,pc))return 0;
    for(n=f->start;n<f->end;n++)if(computed_predecessor(g,n,pc)) {
        if(n!=pc-1)return 0;
        count++;
    }
    return count==1;
}
static int computed_local_name(const Func *f,const char *name) {
    int n;
    if(!name[0]||strlen(name)>=64||literal_name(name)||!strcmp(name,"self")||
       !strcmp(name,"level")||!strcmp(name,"game")||!strcmp(name,"anim"))return 0;
    for(n=0;n<f->argc;n++)if(!strcmp(f->params[n],name))return 0;
    return 1;
}
static int computed_lvalue_has_local(const GscProgram *g,int root,const char *name) {
    const Expr *e;int depth=0;
    while(root>=0&&root<g->expr_count&&depth++<=48) {
        e=&g->expr[root];
        if(e->kind==E_VAR)return !strcmp(e->name,name);
        if(e->kind!=E_FIELD&&e->kind!=E_INDEX)return 0;
        root=e->a;
    }
    return 0;
}
/* Only already selected synchronous native expressions are transparent to
 * reaching definitions. Module/shadow/arity/type/domain and, after linking,
 * committed target namespace are checked. No callee spelling alone suffices. */
static int computed_expression_stable(const GscProgram *g,const Func *f,int pc,
                                     int root,int depth,ComputedIntegerProof *proof) {
    const Expr *e;ComputedIntegerFact bound;int k,selected=0,builtin;
    if(root<0)return 1;
    if(root>=g->expr_count||depth>48)return 0;
    e=&g->expr[root];
    switch(e->kind) {
      case E_NUMBER:case E_STRING:case E_LOCALIZED_STRING:case E_VAR:
      case E_EMPTY_ARRAY:case E_FUNC_REF:return 1;
      case E_UNARY:case E_FIELD:case E_CAST:
        return e->a>=0&&e->a<root&&computed_expression_stable(g,f,pc,e->a,depth+1,proof);
      case E_ISDEFINED:
        return !e->module[0]&&find_func(g,e->owner_module,"isdefined")<0&&
            program_builtin(g,"isdefined",e->owner_module)>=0&&
            (!g->linked||e->call_binding==GSC_EXPR_TARGET_BUILTIN)&&
            e->a>=0&&e->a<root&&computed_expression_stable(g,f,pc,e->a,depth+1,proof);
      case E_BINARY:case E_INDEX:
        return e->a>=0&&e->a<root&&e->b>=0&&e->b<root&&
            computed_expression_stable(g,f,pc,e->a,depth+1,proof)&&
            computed_expression_stable(g,f,pc,e->b,depth+1,proof);
      case E_NATIVE_CALL:
        if(e->module[0]||find_func(g,e->owner_module,e->name)>=0)return 0;
        builtin=program_builtin(g,e->name,e->owner_module);
        /* names_syntax parses isdefined as an ordinary native expression.
         * Admit only this registered pure expression implementation; module,
         * exact local shadow, arity, backward IR and committed target remain
         * checked. Other builtin/native/script calls are still HOLD. */
        if(builtin>=0&&callable_name_equal(g,e->name,"isdefined")) {
            if(builtin>=BUILTIN_COUNT||e->argc!=1||e->args[0]<0||e->args[0]>=root||
               builtins[builtin].context!=B_EXPRESSION||builtins[builtin].arity!=1||
               builtins[builtin].result_kind!=V_NUMBER||builtins[builtin].effect!=B_EFFECT_PURE||
               builtins[builtin].expression!=builtin_isdefined)return 0;
            selected=1;
            if(g->linked&&(e->call_binding!=GSC_EXPR_TARGET_BUILTIN||e->target!=builtin))return 0;
        } else if(g->randomint_mode&&callable_name_equal(g,e->name,"randomint")) {
            if(e->argc==0)selected=1;
            else if(e->argc==1&&e->args[0]>=0&&e->args[0]<root) {
                selected=randomint_literal_int32(g,e->args[0])||
                    (g->computed_randomint_mode&&computed_integer_expression(g,f,e->args[0],pc,0,&bound,proof)&&
                     bound.minimum>0);
            }
            if(g->linked&&(e->call_binding!=GSC_EXPR_TARGET_RANDOMINT_NATIVE||
                           e->target!=GSC_RANDOMINT_DRAW))return 0;
        } else if(g->randomfloat_mode&&randomfloat_descriptor(g,e->name,e->args,e->argc)>=0) {
            selected=1;
            if(g->linked&&(e->call_binding!=GSC_EXPR_TARGET_RANDOMFLOAT_NATIVE||
                           e->target!=GSC_RANDOMFLOAT_DRAW))return 0;
        }
        if(!selected)return 0;
        break;
      case E_VECTOR:break;
      default:return 0;
    }
    if(e->argc<0||e->argc>MAX_PARAMS)return 0;
    for(k=0;k<e->argc;k++)if(e->args[k]<0||e->args[k]>=root||
        !computed_expression_stable(g,f,pc,e->args[k],depth+1,proof))return 0;
    return 1;
}
static int computed_instruction_stable(const GscProgram *g,const Func *f,int pc,
                                      const Instr *i,ComputedIntegerProof *proof) {
    switch(i->kind) {
      case I_CALL:case I_THREAD:case I_METHOD:case I_METHOD_THREAD:
      case I_DYNAMIC_CALL:case I_DYNAMIC_THREAD:case I_WAIT:case I_WAITER:
      case I_FOR:case I_WHILE:case I_DO_WHILE:case I_SWITCH:return 0;
      case I_SET:case I_JZ:case I_NOTIFY:case I_RETURN:
        return computed_expression_stable(g,f,pc,i->expr,0,proof);
      case I_GAME_SET:case I_MUTATE:
        return computed_expression_stable(g,f,pc,i->dest,0,proof)&&
               computed_expression_stable(g,f,pc,i->expr,0,proof);
      case I_JUMP:case I_EMPTY:return 1;
      default:return 0;
    }
}
static int computed_current_instruction_stable(const GscProgram *g,const Func *f,int pc,
                                              const Instr *i,ComputedIntegerProof *proof) {
    int k;
    if(i->kind!=I_CALL)return computed_instruction_stable(g,f,pc,i,proof);
    if(i->module[0]||find_func(g,i->owner_module,i->name)>=0||
       !g->randomint_mode||!callable_name_equal(g,i->name,"randomint")||i->argc!=1)return 0;
    for(k=0;k<i->argc;k++)if(!computed_expression_stable(g,f,pc,i->args[k],0,proof))return 0;
    return 1;
}
static int computed_integer_local_uncached(const GscProgram *g,const Func *f,int pc,
                                 const char *name,int depth,ComputedIntegerFact *out,ComputedIntegerProof *proof) {
    ComputedIntegerFact result,next;const Instr *i;int n,found=0;
    if(depth>256||pc<=f->start||!computed_local_name(f,name)||
       !computed_integer_point(g,f,pc))return 0;
    for(n=f->start;n<f->end;n++)if(computed_predecessor(g,n,pc)) {
        if(n>=pc)return 0;
        i=&g->code[n];
        if(i->kind==I_SET&&!strcmp(i->name,name)) {
            if(!computed_integer_expression(g,f,i->expr,n,depth+1,&next,proof))return 0;
            next.definition=n;
        } else {
            if(!computed_instruction_stable(g,f,n,i,proof)||
               ((i->kind==I_GAME_SET||i->kind==I_MUTATE)&&
                computed_lvalue_has_local(g,i->dest,name)))return 0;
            if(!computed_integer_local(g,f,n,name,depth+1,&next,proof))return 0;
        }
        if(found&&(result.definition!=next.definition||result.minimum!=next.minimum||
                   result.maximum!=next.maximum))return 0;
        result=next;found=1;
    }
    if(!found)return 0;
    *out=result;return 1;
}
static int computed_integer_local(const GscProgram *g,const Func *f,int pc,
                                 const char *name,int depth,ComputedIntegerFact *out,ComputedIntegerProof *proof) {
    ComputedIntegerMemo *memo;int n,accepted;
    if(!proof||proof->remaining--<=0||depth>256||pc<=f->start||
       !computed_local_name(f,name)||!computed_integer_point(g,f,pc))return 0;
    for(n=0;n<proof->count;n++)if(proof->memo[n].pc==pc&&!strcmp(proof->memo[n].name,name)) {
        memo=&proof->memo[n];
        if(memo->state!=2)return 0;
        *out=memo->fact;return 1;
    }
    if(proof->count>=COMPUTED_INTEGER_MEMO_CAPACITY)return 0;
    memo=&proof->memo[proof->count++];memo->pc=pc;memo->state=1;
    str_copy(memo->name,sizeof(memo->name),name);
    accepted=computed_integer_local_uncached(g,f,pc,name,depth,out,proof);
    memo->state=accepted?2:3;
    if(accepted)memo->fact=*out;
    return accepted;
}
static int computed_dense_count(const GscProgram *g,const Func *f,int pc,
                                const char *name,ComputedIntegerFact *out) {
    const Instr *i;const Expr *target,*base,*key,*value;int n,key_index=-1,count=0,first=-1,root=-1,empty_defs=0;
    if(!computed_local_name(f,name))return 0;
    /* Only uninterrupted, fresh literal writes are selected. No alias can
     * escape and no callback/yield can change the receiver/count in this span. */
    for(n=pc-1;n>=f->start;n--) {
        if(!computed_only_previous(g,f,n+1))return 0;
        i=&g->code[n];
        if(i->kind!=I_GAME_SET||i->dest<0||i->dest>=g->expr_count||
           i->expr<0||i->expr>=g->expr_count)return 0;
        target=&g->expr[i->dest];value=&g->expr[i->expr];
        if(target->kind!=E_INDEX||target->a<0||target->a>=i->dest||
           target->b<0||target->b>=i->dest)return 0;
        base=&g->expr[target->a];key=&g->expr[target->b];
        if(base->kind!=E_VAR||strcmp(base->name,name)||key->kind!=E_NUMBER||
           key->integer!=1||key->integer_value<0||
           key->integer_value>GSC_ARRAY_NUMERIC_KEY_MAX)return 0;
        if(value->kind!=E_STRING&&value->kind!=E_LOCALIZED_STRING&&
           !(value->kind==E_NUMBER&&value->integer==1))return 0;
        if(key_index<0){key_index=key->integer_value;count=key_index+1;}
        if(key->integer_value!=key_index)return 0;
        if(!key_index){first=n;root=target->a;break;}
        key_index--;
    }
    if(first<0||count<=0)return 0;
    /* The implicit local was never read/aliased before its initial [0] write.
     * One explicit fresh [] initialization is also harmless; parameters,
     * prior writes, reinitializations and conditional aliases are refused. */
    for(n=f->start;n<first;n++)if(g->code[n].kind==I_SET&&!strcmp(g->code[n].name,name)) {
        int expression=g->code[n].expr;
        if(++empty_defs>1||expression<0||expression>=g->expr_count||
           g->expr[expression].kind!=E_EMPTY_ARRAY)return 0;
    }
    for(n=f->expr_start;n<root;n++)if(g->expr[n].kind==E_VAR&&!strcmp(g->expr[n].name,name))return 0;
    out->definition=first;out->minimum=out->maximum=count;return 1;
}
static int computed_integer_expression(const GscProgram *g,const Func *f,int index,
                                      int pc,int depth,ComputedIntegerFact *out,ComputedIntegerProof *proof) {
    const Expr *e;ComputedIntegerFact a,b;int64_t low,high;
    if(!proof||proof->remaining--<=0||depth>256||index<0||index>=g->expr_count)return 0;
    e=&g->expr[index];
    if(e->kind==E_NUMBER&&e->integer==1) {
        out->definition=index;out->minimum=out->maximum=e->integer_value;return 1;
    }
    if(e->kind==E_VAR)return computed_integer_local(g,f,pc,e->name,depth+1,out,proof);
    if(e->kind==E_UNARY&&e->op=='-') {
        if(e->a<0||e->a>=index)return 0;
        if(!computed_integer_expression(g,f,e->a,pc,depth+1,&a,proof))return 0;
        low=-(int64_t)a.maximum;high=-(int64_t)a.minimum;
    } else if(e->kind==E_BINARY&&(e->op=='+'||e->op=='-')) {
        if(e->a<0||e->a>=index||e->b<0||e->b>=index)return 0;
        if(!computed_integer_expression(g,f,e->a,pc,depth+1,&a,proof)||
           !computed_integer_expression(g,f,e->b,pc,depth+1,&b,proof))return 0;
        low=e->op=='+'?(int64_t)a.minimum+b.minimum:(int64_t)a.minimum-b.maximum;
        high=e->op=='+'?(int64_t)a.maximum+b.maximum:(int64_t)a.maximum-b.minimum;
    } else if(e->kind==E_NATIVE_CALL&&!e->module[0]&&e->argc==1&&
              callable_name_equal(g,e->name,"randomint")&&find_func(g,e->owner_module,e->name)<0) {
        if(e->args[0]<0||e->args[0]>=index)return 0;
        if(!computed_integer_expression(g,f,e->args[0],pc,depth+1,&a,proof)||
           a.minimum<=0||a.maximum>65536)return 0;
        /* This bound keeps every draw15*upper product below INT32_MAX.
         * The low32/SAR15 result is therefore an exact integer in 0..upper-1. */
        low=0;high=(int64_t)a.maximum-1;
    } else if(e->kind==E_FIELD&&control_field_equal(e->name,"size")&&
              e->a>=0&&e->a<index&&g->expr[e->a].kind==E_VAR) {
        return computed_dense_count(g,f,pc,g->expr[e->a].name,out);
    } else return 0;
    if(low<INT32_MIN||high>INT32_MAX||low>high)return 0;
    out->definition=index;out->minimum=(int32_t)low;out->maximum=(int32_t)high;return 1;
}
static int computed_randomint_bound(const GscProgram *g,int index,int32_t *low,int32_t *high) {
    const Func *f=NULL;ComputedIntegerFact fact;ComputedIntegerProof proof;int n,pc=-1;
    if(!g||!g->computed_randomint_mode||index<0||index>=g->expr_count)return 0;
    for(n=0;n<g->func_count;n++)if(index>=g->funcs[n].expr_start&&index<g->funcs[n].expr_end) {
        if(f)return 0;
        f=&g->funcs[n];
    }
    if(!f)return 0;
    for(n=f->start;n<f->end;n++)if(computed_instruction_has(g,&g->code[n],index)) {
        if(pc>=0)return 0;
        pc=n;
    }
    memset(&proof,0,sizeof(proof));proof.remaining=COMPUTED_INTEGER_PROOF_BUDGET;
    if(pc<0||!computed_integer_point(g,f,pc)||!computed_current_instruction_stable(g,f,pc,&g->code[pc],&proof)||
       !computed_integer_expression(g,f,index,pc,0,&fact,&proof)||fact.minimum<=0)return 0;
    if(low)*low=fact.minimum;
    if(high)*high=fact.maximum;
    return 1;
}

#include "gsc_expression_bind.inc"
#include "gsc_receiver_bind.inc"
static unsigned music_static_issues(const GscProgram *g,int kind,const int *args,int argc) {
    int type;
    if(!music_argc_valid(kind,argc))return GSC_LINK_ISSUE_BUILTIN_ARITY;
    if(!argc)return 0;
    type=known_value_kind(g,args[0]);
    if(type>=0&&type!=(kind==GSC_MUSIC_PLAY?V_STRING:V_NUMBER))
        return GSC_LINK_ISSUE_BUILTIN_LITERAL;
    return 0;
}
static int music_preflight_args(const GscProgram *g,int kind,const int *args,int argc,
    int line,char *error,size_t size) {
    unsigned issues=music_static_issues(g,kind,args,argc);
    if(!issues)return 1;
    if(error&&size)snprintf(error,size,"line %d: music %s argument count or admitted type is invalid",
        line,kind==GSC_MUSIC_PLAY?"play":"stop");
    return 0;
}

static unsigned resource_static_issues(const GscProgram *g,int kind,const int *args,int argc) {
    int type;
    if(!resource_argc_valid(kind,argc))return GSC_LINK_ISSUE_BUILTIN_ARITY;
    type=known_value_kind(g,args[0]);
    return type>=0&&type!=V_STRING?GSC_LINK_ISSUE_BUILTIN_LITERAL:0u;
}
static int resource_preflight_args(const GscProgram *g,int kind,const int *args,int argc,
    int line,char *error,size_t size) {
    if(!resource_static_issues(g,kind,args,argc))return 1;
    if(error&&size)snprintf(error,size,"line %d: precache %s argument count or admitted type is invalid",
        line,kind==GSC_RESOURCE_SHADER?"shader":"model");
    return 0;
}

/* Objective preflight validates only proved argument positions and bounded
 * admitted types. Ignored extra arguments retain normal evaluation effects. */
static unsigned hud_static_issues(const GscProgram *g,int kind,const int *args,int argc) {
    int n,type;
    if((kind!=GSC_HUD_PRINT&&kind!=GSC_HUD_BOLD)||argc<0||argc>MAX_PARAMS)
        return GSC_LINK_ISSUE_BUILTIN_ARITY;
    if(args)for(n=0;n<argc;n++) {
        type=known_value_kind(g,args[n]);
        if(type>=0&&type!=V_STRING&&type!=V_LOCALIZED_STRING&&type!=V_NUMBER)
            return GSC_LINK_ISSUE_BUILTIN_LITERAL;
    }
    return 0;
}
static unsigned objective_static_issues(const GscProgram *g,int kind,const int *args,int argc) {
    int n,type;
    if(!objective_argc_valid(kind,argc))return GSC_LINK_ISSUE_BUILTIN_ARITY;
    for(n=0;n<argc;n++) {
        int wanted=-1;
        if(kind==GSC_OBJECTIVE_CURRENT||n==0)wanted=V_NUMBER;
        else if((kind==GSC_OBJECTIVE_ADD||kind==GSC_OBJECTIVE_STATE)&&n==1)wanted=V_STRING;
        else if((kind==GSC_OBJECTIVE_ADD&&n==2)||
                ((kind==GSC_OBJECTIVE_STRING||kind==GSC_OBJECTIVE_STRING_NOMESSAGE)&&n>=1))wanted=V_LOCALIZED_STRING;
        else if((kind==GSC_OBJECTIVE_ADD&&n==3)||(kind==GSC_OBJECTIVE_POSITION&&n==1))wanted=V_VECTOR;
        if(wanted<0)continue;
        type=known_value_kind(g,args[n]);
        if(type>=0&&type!=wanted&&!(wanted==V_LOCALIZED_STRING&&(type==V_STRING||type==V_NUMBER)))
            return GSC_LINK_ISSUE_BUILTIN_LITERAL;
    }
    return 0;
}
static int objective_preflight_args(const GscProgram *g,int kind,const int *args,int argc,
    int line,char *error,size_t size) {
    if(!objective_static_issues(g,kind,args,argc))return 1;
    if(error&&size)snprintf(error,size,"line %d: objective argument count or admitted type is invalid",line);
    return 0;
}

#include "gsc_event_payload_preflight.inc"

static int program_link_strict(GscProgram *g,const char *entry_module,int entities,int access,int events,int state,int controls,int flow,int arrays,int results,int payloads,int predicates,int actor_spawners,int function_refs,
                        char *error,size_t error_size) {
    int n,module,target,builtin,music,resource,objective,console,hud,mission,blast,quake,fx,cvar,shellshock,debug_text,debug_line,ambient,precache_item,save_game,tag_fx,native_query,spawner_array;char canonical[MAX_MODULE_PATH];
    if(error&&error_size)error[0]=0;
    if(g&&g->module_loader_mode) {
        if(!gsc_module_identity_canonical(entry_module,canonical,sizeof(canonical))) {
            if(error&&error_size)snprintf(error,error_size,"invalid canonical entry module");
            return 0;
        }
        entry_module=canonical;
    }
    if(!g||g->invalid||g->linked||(g->names_syntax&&!g->names_mode)||
       (g->music_syntax&&!g->music_mode)||(g->resources_syntax&&!g->resources_mode)||
       (g->objectives_syntax&&!g->objectives_mode)||(g->console_syntax&&!g->console_mode)||(g->hud_syntax&&!g->hud_mode)||(g->mission_syntax&&!g->mission_mode)||(g->blast_syntax&&!g->blast_mode)||(g->quake_syntax&&!g->quake_mode)||(g->fx_syntax&&!g->fx_mode)||(g->cvars_syntax&&!g->cvars_mode)||(g->shellshock_syntax&&!g->shellshock_mode)||(g->debug_text_syntax&&!g->debug_text_mode)||(g->debug_line_syntax&&!g->debug_line_mode)||(g->ambient_syntax&&!g->ambient_mode)||(g->precache_item_syntax&&!g->precache_item_mode)||(g->save_game_syntax&&!g->save_game_mode)||(g->tag_fx_syntax&&!g->tag_fx_mode)||(g->native_queries_syntax&&!g->native_queries_mode)||(g->precache_string_syntax&&!g->precache_string_mode)||(g->spawner_arrays_syntax&&!g->spawner_arrays_mode)||(g->randomint_syntax&&!g->randomint_mode)||(g->randomfloat_syntax&&!g->randomfloat_mode)||(g->keybinding_syntax&&!g->keybinding_mode)||(g->computed_randomint_syntax&&!g->computed_randomint_mode)||!valid_module_path(entry_module)||
       (module=find_module(g,entry_module))<0) {
        if(error&&error_size)snprintf(error,error_size,"invalid entry module or program state");
        return 0;
    }
    g->entity_mode=entities;
    g->access_mode=access;
    g->event_mode=events;
    g->state_mode=state;
    g->controls_mode=controls;
    g->flow_mode=flow;g->arrays_mode=arrays;g->results_mode=results;g->event_payload_mode=payloads;g->predicate_mode=predicates;g->actor_spawn_mode=actor_spawners;g->function_ref_mode=function_refs;
    if(!event_payload_preflight(g,error,error_size))return 0;
    g->entry_fn=find_func(g,module,"main");
    if(g->entry_fn<0) {
        if(error&&error_size)snprintf(error,error_size,"main function is required");
        return 0;
    }
    /* Validate every call, including unreachable bodies in every module. */
    for(n=0;n<g->code_count;n++) {
        Instr *i=&g->code[n];
        /* R71: exact ordinary local-method candidates join qualified/local
         * thread targets. Unknown ordinary methods remain deferred; computed
         * dispatch and runtime are unchanged. Fixture-only static policy. */
        if(static_receiver_instruction(i)) {
            if(!i->module[0]&&move_delta_descriptor(g,i->name)&&find_func(g,receiver_owner_module(g,n,0),i->name)<0) {if(error&&error_size)snprintf(error,error_size,"line %d: GetMoveDelta is a global query",i->line);return 0;}
            if(!i->module[0]&&start_origin_descriptor(g,i->name)&&find_func(g,receiver_owner_module(g,n,0),i->name)<0) {
                if(error&&error_size)snprintf(error,error_size,"line %d: getstartorigin is a global query",i->line);return 0;
            }
            if(!receiver_binding_preflight(g,receiver_owner_module(g,n,0),
                                           i->kind==I_METHOD_THREAD,
                                           i->module,i->name,i->argc,i->line,
                                           error,error_size))return 0;
            if(i->kind==I_METHOD&&!i->module[0]&&entity_method_descriptor(g,i->name)==R243_STANCE_METHOD&&find_func(g,receiver_owner_module(g,n,0),i->name)<0&&stance_static_issues(g,i->args,i->argc)){if(error&&error_size)snprintf(error,error_size,"line %d: isStanceAllowed requires one stance string",i->line);return 0;}
            if(i->kind==I_METHOD&&!i->module[0]&&entity_method_descriptor(g,i->name)==R243_ANIMTIME_METHOD&&find_func(g,receiver_owner_module(g,n,0),i->name)<0&&animtime_static_issues(g,i->args,i->argc)){if(error&&error_size)snprintf(error,error_size,"line %d: getAnimTime requires one typed animation",i->line);return 0;}
            if(i->kind==I_METHOD&&!i->module[0]&&actor_look_kind(entity_method_descriptor(g,i->name))&&find_func(g,i->owner_module,i->name)<0){
                int k=actor_look_kind(entity_method_descriptor(g,i->name)),a;
                for(a=0;a<i->argc;a++){const Expr *e=&g->expr[i->args[a]];int type=known_value_kind(g,i->args[a]);int want=k==GSC_ACTOR_LOOK_CHECK_PRONE?(a==0?V_VECTOR:V_NUMBER):k==GSC_ACTOR_LOOK_SHOOT?(i->argc==1?V_NUMBER:a==1?V_VECTOR:-1):(k==GSC_ACTOR_LOOK_CAN_SHOOT||k==GSC_ACTOR_LOOK_AIM_AT_POS||k==GSC_ACTOR_LOOK_MAY_MOVE)?V_VECTOR:k==GSC_ACTOR_LOOK_NODES?V_NAMESPACED_ANIMATION:k==GSC_ACTOR_LOOK_TARGET&&a==0?V_VECTOR:(actor_look_tag(k)||k==GSC_ACTOR_TRAVERSE_MODE)?V_STRING:k==GSC_ACTOR_LOOK_CAN_SEE?V_ENTITY:V_NUMBER;
                    if(want>=0&&((type>=0&&type!=want)||(want==V_NAMESPACED_ANIMATION&&e->kind==E_ANIMTREE_VALUE)||(want!=V_NAMESPACED_ANIMATION&&(e->kind==E_ANIM_REF||e->kind==E_ANIMTREE_VALUE)))){if(error&&error_size)snprintf(error,error_size,"line %d: actor look literal operand type mismatch",i->line);return 0;}
                }
            }
            if(i->kind==I_METHOD&&!i->module[0]&&actor_animation_kind(entity_method_descriptor(g,i->name))&&find_func(g,i->owner_module,i->name)<0) {
                int k=actor_animation_kind(entity_method_descriptor(g,i->name)),a;
                /* Orient getters run after actual Actor validation. Literal
                 * operands must not bypass their committed-error prefix. */
                for(a=0;k!=GSC_ACTOR_ANIMATION_ORIENT_MODE&&a<i->argc;a++) {const Expr *e=&g->expr[i->args[a]];int type=known_value_kind(g,i->args[a]);
                    int string=((k==GSC_ACTOR_ANIMATION_FLAGGED_ALL_RESTART||k==GSC_ACTOR_ANIMATION_FLAGGED_ALL)||(k==GSC_ACTOR_ANIMATION_FLAGGED_RESTART||k==GSC_ACTOR_ANIMATION_FLAGGED_KNOB)||k==GSC_ACTOR_ANIMATION_ORIENT_MODE||k==GSC_ACTOR_ANIMATION_ANIM_MODE)&&a==0;
                    int anim=k==GSC_ACTOR_ANIMATION_AIM_NODES?1:(k==GSC_ACTOR_ANIMATION_FLAGGED_ALL_RESTART||k==GSC_ACTOR_ANIMATION_FLAGGED_ALL)?(a==1||a==2):((k==GSC_ACTOR_ANIMATION_KNOB_ALL||k==GSC_ACTOR_ANIMATION_KNOB_ALL_RESTART)||k==GSC_ACTOR_ANIMATION_UPDATE_PRONE)?a<2:k==GSC_ACTOR_ANIMATION_PRONE_NODES?a>=2:(k==GSC_ACTOR_ANIMATION_CLEAR||k==GSC_ACTOR_ANIMATION_SET||k==GSC_ACTOR_ANIMATION_KNOB)?a==0:(k==GSC_ACTOR_ANIMATION_FLAGGED_RESTART||k==GSC_ACTOR_ANIMATION_FLAGGED_KNOB)&&a==1;
                    if((string&&((type>=0&&type!=V_STRING)||e->kind==E_ANIM_REF||e->kind==E_ANIMTREE_VALUE))||
                       (anim&&(type>=0||e->kind==E_ANIMTREE_VALUE))||
                       (!string&&!anim&&((type>=0&&type!=V_NUMBER)||e->kind==E_ANIM_REF||e->kind==E_ANIMTREE_VALUE))) {
                        if(error&&error_size)snprintf(error,error_size,"line %d: %s typed animation/scalar/string operand mismatch",i->line,i->name);return 0;
                    }
                }
            }
            if(i->kind==I_METHOD&&!i->module[0]&&
               turret_animation_kind(entity_method_descriptor(g,i->name))&&
               find_func(g,i->owner_module,i->name)<0) {
                int method=turret_animation_kind(entity_method_descriptor(g,i->name)),a;
                for(a=0;a<i->argc;a++) {
                    const Expr *e=&g->expr[i->args[a]];int type=known_value_kind(g,i->args[a]);
                    if((a==0&&(type>=0||
                       (e->kind==E_ANIM_REF&&method==GSC_ANIMATION_USE_TREE)||
                       (e->kind==E_ANIMTREE_VALUE&&method!=GSC_ANIMATION_USE_TREE)))||
                       (a>0&&((type>=0&&type!=V_NUMBER)||e->kind==E_ANIM_REF||e->kind==E_ANIMTREE_VALUE))) {
                        if(error&&error_size)snprintf(error,error_size,"line %d: %s requires typed tree/animation and numeric optional operands",i->line,i->name);return 0;
                    }
                }
            }
            if(g->entity_mode&&i->kind==I_METHOD&&
               !i->module[0]&&entity_method_descriptor(g,i->name)>=0&&!turret_animation_kind(entity_method_descriptor(g,i->name))&&!actor_animation_kind(entity_method_descriptor(g,i->name))&&!actor_look_kind(entity_method_descriptor(g,i->name))&&
               entity_method_arity(entity_method_descriptor(g,i->name))==1&&
               find_func(g,i->owner_module,i->name)<0) {
                int type=known_value_kind(g,i->args[0]);
                if(type>=0&&type!=entity_method_argument_kind(entity_method_descriptor(g,i->name))&&
                   !(entity_method_descriptor(g,i->name)==504&&g->expr[i->args[0]].kind==E_STRING&&!g->expr[i->args[0]].name[0])) {
                    if(error&&error_size)snprintf(error,error_size,
                        "line %d: %s argument 1 must be a %s",i->line,i->name,
                        entity_method_argument_kind(entity_method_descriptor(g,i->name))==V_STRING?"string":"vector");
                    return 0;
                }
                if(turret_method_kind(entity_method_descriptor(g,i->name))==GSC_TURRET_SET_MODE&&g->expr[i->args[0]].kind==E_STRING&&
                   (!g->expr[i->args[0]].name[0]||strlen(g->expr[i->args[0]].name)>=sizeof(((GscTurretRequest*)0)->mode))){
                    if(error&&error_size)snprintf(error,error_size,"line %d: setmode requires a bounded nonempty string",i->line);return 0;
                }
            }
            if(i->kind==I_METHOD&&!i->module[0]&&entity_event_descriptor(g,i->name)>=0&&
               find_func(g,i->owner_module,i->name)<0) {
                int type=known_value_kind(g,i->args[0]);
                if((type>=0&&type!=V_STRING)||
                   (type==V_STRING&&!g->expr[i->args[0]].name[0])) {
                    if(error&&error_size)snprintf(error,error_size,
                        "line %d: %s requires one nonempty event string",i->line,i->name);
                    return 0;
                }
            }
            continue;
        }
        if(i->kind!=I_CALL&&i->kind!=I_THREAD)continue;
        builtin=i->kind==I_CALL&&!i->module[0]?program_builtin(g,i->name,i->owner_module):-1;
        music=i->kind==I_CALL&&!i->module[0]&&
            find_func(g,i->owner_module,i->name)<0?music_descriptor(g,i->name):-1;
        resource=i->kind==I_CALL&&!i->module[0]&&
            find_func(g,i->owner_module,i->name)<0?resource_descriptor(g,i->name):-1;
        objective=i->kind==I_CALL&&!i->module[0]&&
            find_func(g,i->owner_module,i->name)<0?objective_descriptor(g,i->name):-1;
        console=i->kind==I_CALL&&!i->module[0]&&
            find_func(g,i->owner_module,i->name)<0?console_descriptor(g,i->name):-1;
        hud=i->kind==I_CALL&&!i->module[0]&&
            find_func(g,i->owner_module,i->name)<0?hud_descriptor(g,i->name):-1;
        mission=i->kind==I_CALL&&!i->module[0]&&
            find_func(g,i->owner_module,i->name)<0?mission_descriptor(g,i->name):-1;
        if(i->kind==I_CALL&&!i->module[0]&&find_func(g,i->owner_module,i->name)<0&&hud_elem_global(g,i->name)){
            if(i->argc){if(error&&error_size)snprintf(error,error_size,"line %d: newHudElem expects no arguments",i->line);return 0;}
            i->target=-1-HUDELEM_TARGET_BASE-GSC_HUDELEM_CREATE;continue;
        }
        if(i->kind==I_CALL&&!i->module[0]&&find_func(g,i->owner_module,i->name)<0&&move_delta_descriptor(g,i->name)){if(move_delta_static_issues(g,i->args,i->argc)){if(error&&error_size)snprintf(error,error_size,"line %d: GetMoveDelta requires current animation and up to2 finite phases",i->line);return 0;}i->target=-1-R243_MOVE_DELTA_STATEMENT_TARGET;continue;}
        if(i->kind==I_CALL&&!i->module[0]&&find_func(g,i->owner_module,i->name)<0&&start_origin_descriptor(g,i->name)) {
            if(start_origin_static_issues(g,i->args,i->argc)){if(error&&error_size)snprintf(error,error_size,"line %d: getstartorigin requires vector,vector,fixed Guy animation",i->line);return 0;}
            i->target=-1-(start_origin_descriptor(g,i->name)==2?START_ANGLES_STATEMENT_TARGET:START_ORIGIN_STATEMENT_TARGET);continue;
        }
        if(i->kind==I_CALL&&!i->module[0]&&find_func(g,i->owner_module,i->name)<0&&ai_array_descriptor(g,i->name)) {
            if(ai_array_static_issues(g,i->args,i->argc)){if(error&&error_size)snprintf(error,error_size,"line %d: getaiarray requires ordinary team strings",i->line);return 0;}
            i->target=-1-AI_ARRAY_STATEMENT_TARGET;continue;
        }
        {int node_query=i->kind==I_CALL&&!i->module[0]&&find_func(g,i->owner_module,i->name)<0?node_query_descriptor(g,i->name):0;
         if(node_query) {
            if(node_query_static_issues(g,node_query,i->args,i->argc)) {
                if(error&&error_size)snprintf(error,error_size,"line %d: node query arguments outside selected domain",i->line);
                return 0;
            }
            i->target=-1-NODE_QUERY_TARGET_BASE-node_query;continue;
         }}
        {int selected_query=i->kind==I_CALL&&!i->module[0]&&find_func(g,i->owner_module,i->name)<0?entity_query_descriptor(g,i->name):-1;
         if(selected_query>=0) {
            if(entity_query_static_issues(g,selected_query,i->args,i->argc)) {
                if(error&&error_size)snprintf(error,error_size,"line %d: entity query arguments outside selected domain",i->line);
                return 0;
            }
            i->target=-1-ENTITY_QUERY_TARGET_BASE-selected_query;continue;
         }}
        {int selected_struct=i->kind==I_CALL&&!i->module[0]&&find_func(g,i->owner_module,i->name)<0?struct_descriptor(g,i->name):-1;
         if(selected_struct>=0){
            if(struct_static_issues(selected_struct,i->argc))return 0;
            i->target=-1-STRUCT_TARGET_BASE-selected_struct;continue;}}
        {int selected_keybinding=i->kind==I_CALL&&!i->module[0]&&find_func(g,i->owner_module,i->name)<0?
            keybinding_descriptor(g,i->name,i->args,i->argc):-1;
         if(selected_keybinding>=0){i->target=-1-KEYBINDING_TARGET_BASE-selected_keybinding;continue;}}
        if(i->kind==I_CALL&&!i->module[0]&&find_func(g,i->owner_module,i->name)<0&&random_range_descriptor(g,i->name)){if(random_range_issues(g,i->args,i->argc)){if(error&&error_size)snprintf(error,error_size,"line %d: randomFloatRange requires two numeric arguments",i->line);return 0;}i->target=-1-R232_RANDOM_RANGE_STATEMENT_TARGET;continue;}
        {int selected_randomfloat=i->kind==I_CALL && !i->module[0] &&
            find_func(g,i->owner_module,i->name)<0?
            randomfloat_descriptor(g,i->name,i->args,i->argc):-1;
         if(selected_randomfloat>=0) {
            i->target=-1-RANDOMFLOAT_TARGET_BASE-selected_randomfloat;continue;
         }}
        {int selected_randomint=i->kind==I_CALL && !i->module[0] &&
            find_func(g,i->owner_module,i->name)<0?
            randomint_descriptor(g,i->name,i->args,i->argc):-1;
         if(selected_randomint>=0) {
            i->target=-1-RANDOMINT_TARGET_BASE-selected_randomint;continue;
         }}
        spawner_array=i->kind==I_CALL&&!i->module[0]&&
            find_func(g,i->owner_module,i->name)<0?spawner_array_descriptor(g,i->name):-1;
        if(spawner_array>=0) {
            if(spawner_array_static_issues(spawner_array,i->argc)) {
                if(error&&error_size)snprintf(error,error_size,"line %d: getspawnerarray requires zero arguments",i->line);
                return 0;
            }
            i->target=-1-SPAWNER_ARRAY_TARGET_BASE-spawner_array;continue;
        }
        if(i->kind==I_CALL&&!i->module[0]&&find_func(g,i->owner_module,i->name)<0&&animation_length_descriptor(g,i->name)) {
            i->target=-1-ANIMATION_LENGTH_STATEMENT_TARGET;continue;
        }
        if(i->kind==I_CALL&&!i->module[0]&&find_func(g,i->owner_module,i->name)<0&&bullet_trace_descriptor(g,i->name)){
         if(bullet_trace_issues(g,i->args,i->argc)){if(error&&error_size)snprintf(error,error_size,"line %d: bulletTrace requires vector,vector,numeric boolean,undefined",i->line);return 0;}
         i->target=-1-BULLET_TRACE_STATEMENT_TARGET;continue;
        }
        if(i->kind==I_CALL&&!i->module[0]&&find_func(g,i->owner_module,i->name)<0&&vehicle_spawn_descriptor(g,i->name)){
         if(vehicle_spawn_issues(g,i->args,i->argc)){if(error&&error_size)snprintf(error,error_size,"line %d: spawnVehicle requires three strings and two vectors",i->line);return 0;}
         i->target=-1-VEHICLE_SPAWN_STATEMENT_TARGET;continue;
        }
        if(i->kind==I_CALL&&!i->module[0]&&find_func(g,i->owner_module,i->name)<0&&turret_precache_descriptor(g,i->name)){
         if(turret_precache_issues(g,i->args,i->argc)){if(error&&error_size)snprintf(error,error_size,"line %d: precacheTurret requires valid argument count",i->line);return 0;}
         i->target=-1-TURRET_PRECACHE_STATEMENT_TARGET;continue;
        }
        if(i->kind==I_CALL&&!i->module[0]&&find_func(g,i->owner_module,i->name)<0&&turret_spawn_descriptor(g,i->name)){
         if(turret_spawn_issues(g,i->args,i->argc)){if(error&&error_size)snprintf(error,error_size,"line %d: spawnTurret requires string,vector,string",i->line);return 0;}
         i->target=-1-TURRET_SPAWN_STATEMENT_TARGET;continue;
        }
        if(i->kind==I_CALL&&!i->module[0]&&find_func(g,i->owner_module,i->name)<0&&model_part_descriptor(g,i->name)){i->target=-1-MODEL_PART_STATEMENT_TARGET;continue;}
        if(i->kind==I_CALL&&!i->module[0]&&find_func(g,i->owner_module,i->name)<0&&weapon_classname_descriptor(g,i->name)){i->target=-1-WEAPON_CLASSNAME_STATEMENT_TARGET;continue;}
        if(i->kind==I_CALL&&!i->module[0]&&find_func(g,i->owner_module,i->name)<0&&weapon_model_descriptor(g,i->name)) {
            i->target=-1-WEAPON_MODEL_STATEMENT_TARGET;continue;
        }
        if(i->kind==I_CALL&&!i->module[0]&&find_func(g,i->owner_module,i->name)<0&&vector_math_descriptor(g,i->name)) {
            int math=vector_math_descriptor(g,i->name);
            if(vector_math_static_issues(g,math,i->args,i->argc)) {
                if(error&&error_size)snprintf(error,error_size,"line %d: invalid selected vector math arguments",i->line);
                return 0;
            }
            i->target=-1-VECTOR_MATH_TARGET_BASE-math;continue;
        }
        native_query=i->kind==I_CALL&&!i->module[0]&&
            find_func(g,i->owner_module,i->name)<0?native_query_descriptor(g,i->name):-1;
        if(native_query>=0) {
            if(native_query_static_issues(g,native_query,i->args,i->argc))return 0;
            i->target=-1-NATIVE_QUERY_TARGET_BASE-native_query;continue;
        }
        tag_fx=i->kind==I_CALL&&!i->module[0]&&
            find_func(g,i->owner_module,i->name)<0?tag_fx_descriptor(g,i->name):-1;
        if(tag_fx>=0) {
            if(tag_fx_static_issues(g,tag_fx,i->args,i->argc)) {
                if(error&&error_size)snprintf(error,error_size,"line %d: playfxOnTag requires exactly three arguments",i->line);
                return 0;
            }
            i->target=-1-TAG_FX_TARGET_BASE-tag_fx;continue;
        }
        save_game=i->kind==I_CALL&&!i->module[0]&&
            find_func(g,i->owner_module,i->name)<0?save_game_descriptor(g,i->name):-1;
        if(save_game>=0) {
            if(save_game_static_issues(g,save_game,i->args,i->argc)) {
                if(error&&error_size)snprintf(error,error_size,"line %d: invalid saveGame structural arguments",i->line);
                return 0;
            }
            i->target=-1-SAVE_GAME_TARGET_BASE-save_game;continue;
        }
        precache_item=i->kind==I_CALL&&!i->module[0]&&
            find_func(g,i->owner_module,i->name)<0?precache_item_descriptor(g,i->name):-1;
        if(precache_item>=0) {
            if(precache_item_static_issues(g,precache_item,i->args,i->argc)) {
                if(error&&error_size)snprintf(error,error_size,"line %d: invalid precacheItem structural arguments",i->line);
                return 0;
            }
            i->target=-1-PRECACHE_ITEM_TARGET_BASE-precache_item;continue;
        }
        ambient=i->kind==I_CALL&&!i->module[0]&&
            find_func(g,i->owner_module,i->name)<0?ambient_descriptor(g,i->name):-1;
        if(ambient>=0) {
            if(ambient_static_issues(g,ambient,i->args,i->argc)) {
                if(error&&error_size)snprintf(error,error_size,"line %d: invalid ambient structural arguments",i->line);
                return 0;
            }
            i->target=-1-AMBIENT_TARGET_BASE-ambient;continue;
        }
        debug_line=i->kind==I_CALL&&!i->module[0]&&
            find_func(g,i->owner_module,i->name)<0?debug_line_descriptor(g,i->name):-1;
        if(debug_line>=0) {
            if(debug_line_static_issues(g,debug_line,i->args,i->argc)) {
                if(error&&error_size)snprintf(error,error_size,"line %d: invalid debug line structural arguments",i->line);
                return 0;
            }
            i->target=-1-DEBUG_LINE_TARGET_BASE-debug_line;continue;
        }
        debug_text=i->kind==I_CALL&&!i->module[0]&&
            find_func(g,i->owner_module,i->name)<0?debug_text_descriptor(g,i->name):-1;
        if(debug_text>=0) {
            if(debug_text_static_issues(g,debug_text,i->args,i->argc)) {
                if(error&&error_size)snprintf(error,error_size,"line %d: invalid debug text structural arguments",i->line);
                return 0;
            }
            i->target=-1-DEBUG_TEXT_TARGET_BASE-debug_text;continue;
        }
        shellshock=i->kind==I_CALL&&!i->module[0]&&
            find_func(g,i->owner_module,i->name)<0?shellshock_descriptor(g,i->name):-1;
        if(shellshock>=0) {
            if(shellshock_static_issues(g,shellshock,i->args,i->argc)) {
                if(error&&error_size)snprintf(error,error_size,"line %d: invalid shellshock structural arguments",i->line);
                return 0;
            }
            i->target=-1-SHELLSHOCK_TARGET_BASE-shellshock;continue;
        }
        cvar=i->kind==I_CALL&&!i->module[0]&&
            find_func(g,i->owner_module,i->name)<0?cvar_descriptor(g,i->name):-1;
        if(cvar>=0) {
            if(cvar_static_issues(g,cvar,i->args,i->argc)) {
                if(error&&error_size)snprintf(error,error_size,"line %d: invalid cvar structural arguments",i->line);
                return 0;
            }
            i->target=-1-CVAR_TARGET_BASE-cvar;continue;
        }
        fx=i->kind==I_CALL&&!i->module[0]&&
            find_func(g,i->owner_module,i->name)<0?fx_descriptor(g,i->name):-1;
        if(fx>=0) {
            if(fx_static_issues(g,fx,i->args,i->argc)) {
                if(error&&error_size)snprintf(error,error_size,"line %d: invalid FX structural arguments",i->line);
                return 0;
            }
            i->target=-1-FX_TARGET_BASE-fx;continue;
        }
        quake=i->kind==I_CALL&&!i->module[0]&&
            find_func(g,i->owner_module,i->name)<0?quake_descriptor(g,i->name):-1;
        if(quake>=0) {
            if(quake_static_issues(g,quake,i->args,i->argc)) {
                if(error&&error_size)snprintf(error,error_size,"line %d: invalid quake structural arguments",i->line);
                return 0;
            }
            i->target=-1-QUAKE_TARGET_BASE-quake;continue;
        }
        blast=i->kind==I_CALL&&!i->module[0]&&
            find_func(g,i->owner_module,i->name)<0?blast_descriptor(g,i->name):-1;
        if(blast>=0) {
            if(blast_static_issues(g,blast,i->args,i->argc)) {
                if(error&&error_size)snprintf(error,error_size,"line %d: invalid blast structural arguments",i->line);
                return 0;
            }
            i->target=-1-BLAST_TARGET_BASE-blast;continue;
        }
        if(mission>=0) {
            if(mission_static_issues(g,mission,i->args,i->argc)) {
                if(error&&error_size)snprintf(error,error_size,"line %d: invalid mission arity",i->line);
                return 0;
            }
            i->target=-1-MISSION_TARGET_BASE-mission;continue;
        }
        if(hud>=0) {
            if(hud_static_issues(g,hud,i->args,i->argc)) {
                if(error&&error_size)snprintf(error,error_size,"line %d: invalid HUD argument count or admitted type",i->line);
                return 0;
            }
            i->target=-1-HUD_TARGET_BASE-hud;continue;
        }
        if(console>=0) {
            if(console_static_issues(g,console,i->args,i->argc)) {
                if(error&&error_size)snprintf(error,error_size,"line %d: invalid console arity",i->line);
                return 0;
            }
            i->target=-1-CONSOLE_TARGET_BASE-console;
            continue;
        }
        if(objective>=0) {
            if(!objective_preflight_args(g,objective,i->args,i->argc,i->line,error,error_size))return 0;
            i->target=-1-OBJECTIVE_TARGET_BASE-objective;
            continue;
        }
        if(resource>=0) {
            if(!resource_preflight_args(g,resource,i->args,i->argc,i->line,error,error_size))return 0;
            i->target=-1-RESOURCE_TARGET_BASE-resource;
            continue;
        }
        if(music>=0) {
            if(!music_preflight_args(g,music,i->args,i->argc,i->line,error,error_size))return 0;
            i->target=-1-BUILTIN_COUNT-music;
            continue;
        }
        if(builtin>=0&&builtins[builtin].context==B_STATEMENT) {
            const Builtin *b=&builtins[builtin];
            if(!builtin_preflight_args(g,b,i->args,i->argc,i->line,error,error_size))return 0;
            i->target=-1-builtin;
            continue;
        } else {
            module=i->module[0]?find_module(g,i->module):i->owner_module;
            target=module<0?-1:find_func(g,module,i->name);
            if(target<0) {
                if(error&&error_size)
                    snprintf(error,error_size,"line %d: unresolved/unsupported call %s%s%s",
                             i->line,i->module[0]?i->module:"",
                             i->module[0]?"::":"",i->name);
                return 0;
            }
            i->target=target;
        }
        if(i->argc<0||i->argc>MAX_PARAMS) {
            if(error&&error_size)snprintf(error,error_size,
                "line %d: %s has invalid structural argument count %d",i->line,i->name,i->argc);
            return 0;
        }
    }
    /* Every expression, including ones nested in dead bodies or modules,
     * shares the same registry and must be resolved before runtime creation. */
    for(n=0;n<g->expr_count;n++) {
        Expr *e=&g->expr[n];
        if(static_receiver_expression(e)) {
            if(!e->module[0]&&move_delta_descriptor(g,e->name)&&find_func(g,receiver_owner_module(g,n,1),e->name)<0) {if(error&&error_size)snprintf(error,error_size,"line %d: GetMoveDelta is a global query",e->line);return 0;}
            if(!e->module[0]&&start_origin_descriptor(g,e->name)&&find_func(g,receiver_owner_module(g,n,1),e->name)<0) {
                if(error&&error_size)snprintf(error,error_size,"line %d: getstartorigin is a global query",e->line);return 0;
            }
            if(g->entity_mode&&e->kind==E_METHOD_CALL&&!e->module[0]&&
               (hud_elem_method(g,e->name)||(entity_method_descriptor(g,e->name)>=0&&!entity_method_has_result(entity_method_descriptor(g,e->name)))||entity_event_descriptor(g,e->name)>=0)&&
               find_func(g,receiver_owner_module(g,n,1),e->name)<0) {
                if(error&&error_size)snprintf(error,error_size,
                    "line %d: %s is a statement-only VOID method",e->line,e->name);
                return 0;
            }
            if(!e->module[0]&&turret_method_kind(entity_method_descriptor(g,e->name))&&
               find_func(g,receiver_owner_module(g,n,1),e->name)<0&&e->argc==1){
                int type=known_value_kind(g,e->args[0]);
                if(type>=0&&type!=entity_method_argument_kind(entity_method_descriptor(g,e->name))){
                    if(error&&error_size)snprintf(error,error_size,"line %d: turret method argument has wrong literal type",e->line);return 0;
                }
            }
            if(!receiver_binding_preflight(g,receiver_owner_module(g,n,1),
                                           e->kind==E_THREAD_CALL,
                                           e->module,e->name,e->argc,e->audit_line,
                                           error,error_size))return 0;
            continue;
        }
        if(e->kind==E_ISDEFINED||e->kind==E_NATIVE_CALL) {
            ExpressionBinding binding=resolve_expression_binding(g,e);
            if(binding.kind==GSC_EXPR_TARGET_NONE&&music_descriptor(g,e->name)>=0) {
                if(error&&error_size)snprintf(error,error_size,"line %d: %s is a statement-only VOID music call",e->line,e->name);
                return 0;
            }
            if(binding.kind==GSC_EXPR_TARGET_LOCAL_SCRIPT) {
                if(binding.issues) {
                    if(error&&error_size)snprintf(error,error_size,
                        "line %d: expression call %s has invalid structural argument count %d",
                        e->line,e->name,e->argc);
                    return 0;
                }
            } else if(binding.kind==GSC_EXPR_TARGET_BULLET_TRACE_NATIVE||binding.kind==GSC_EXPR_TARGET_VEHICLE_SPAWN_NATIVE||binding.kind==GSC_EXPR_TARGET_TURRET_PRECACHE_NATIVE||binding.kind==GSC_EXPR_TARGET_TURRET_SPAWN_NATIVE||binding.kind==R243_EXPR_TARGET_MOVE_DELTA||binding.kind==GSC_EXPR_TARGET_R232_RANDOM_RANGE||binding.kind==GSC_EXPR_TARGET_START_ORIGIN_NATIVE||binding.kind==GSC_EXPR_TARGET_AI_ARRAY_NATIVE||binding.kind==GSC_EXPR_TARGET_HUDELEM_NATIVE||binding.kind==GSC_EXPR_TARGET_NODE_QUERY_NATIVE||binding.kind==GSC_EXPR_TARGET_VECTOR_MATH_NATIVE||binding.kind==GSC_EXPR_TARGET_ENTITY_NATIVE||binding.kind==GSC_EXPR_TARGET_RESOURCE_NATIVE||
                      binding.kind==GSC_EXPR_TARGET_OBJECTIVE_NATIVE||binding.kind==GSC_EXPR_TARGET_CONSOLE_NATIVE||binding.kind==GSC_EXPR_TARGET_HUD_NATIVE||binding.kind==GSC_EXPR_TARGET_MISSION_NATIVE||binding.kind==GSC_EXPR_TARGET_BLAST_NATIVE||binding.kind==GSC_EXPR_TARGET_QUAKE_NATIVE||binding.kind==GSC_EXPR_TARGET_FX_NATIVE||binding.kind==GSC_EXPR_TARGET_CVAR_NATIVE||binding.kind==GSC_EXPR_TARGET_SHELLSHOCK_NATIVE||binding.kind==GSC_EXPR_TARGET_DEBUG_TEXT_NATIVE||binding.kind==GSC_EXPR_TARGET_DEBUG_LINE_NATIVE||binding.kind==GSC_EXPR_TARGET_AMBIENT_NATIVE||binding.kind==GSC_EXPR_TARGET_PRECACHE_ITEM_NATIVE||binding.kind==GSC_EXPR_TARGET_SAVE_GAME_NATIVE||binding.kind==GSC_EXPR_TARGET_TAG_FX_NATIVE||binding.kind==GSC_EXPR_TARGET_NATIVE_QUERY||binding.kind==GSC_EXPR_TARGET_SPAWNER_ARRAY_NATIVE||binding.kind==GSC_EXPR_TARGET_RANDOMINT_NATIVE||binding.kind==GSC_EXPR_TARGET_RANDOMFLOAT_NATIVE||binding.kind==GSC_EXPR_TARGET_KEYBINDING_NATIVE||binding.kind==GSC_EXPR_TARGET_STRUCT_NATIVE||binding.kind==GSC_EXPR_TARGET_ENTITY_QUERY_NATIVE||binding.kind==GSC_EXPR_TARGET_WEAPON_MODEL_NATIVE||binding.kind==GSC_EXPR_TARGET_MODEL_PART_NATIVE||binding.kind==GSC_EXPR_TARGET_WEAPON_CLASSNAME_NATIVE||binding.kind==GSC_EXPR_TARGET_ANIMATION_LENGTH_NATIVE) {
                if(binding.issues) {
                    if(error&&error_size)snprintf(error,error_size,
                        "line %d: %s has invalid argument count or literal types",e->line,e->name);
                    return 0;
                }
            } else {
                builtin=binding.kind==GSC_EXPR_TARGET_BUILTIN?binding.target:-1;
                if(builtin<0||builtins[builtin].context!=B_EXPRESSION) {
                    if(error&&error_size)snprintf(error,error_size,
                        "line %d: unresolved/unsupported native expression call %s",
                        e->line,e->kind==E_ISDEFINED?"isdefined":e->name);
                    return 0;
                }
                if(e->kind==E_ISDEFINED) {
                    int only_arg=e->a;
                    if(!builtin_preflight_args(g,&builtins[builtin],&only_arg,1,e->line,
                                              error,error_size))return 0;
                } else if(!builtin_preflight_args(g,&builtins[builtin],e->args,
                                                 e->argc,e->line,error,error_size))return 0;
            }
            /* Bare-call tags are committed together, only after ALL calls
             * and references pass. A failed link cannot expose a new tag. */
            continue;
        }
        if(e->kind==E_FUNC_REF) {
            module=e->module[0]?find_module(g,e->module):e->owner_module;
            target=module<0||module>=g->module_count?-1:find_func(g,module,e->name);
            if(target<0) {
                if(error&&error_size)
                    snprintf(error,error_size,"line %d: unresolved function reference %s::%s",
                             e->line,e->module,e->name);
                return 0;
            }
            e->target=target;
            continue;
        }
        if(e->kind==E_THREAD_CALL&&e->op==GSC_THREAD_EXPR_NAMED&&e->b<0) {
            module=e->module[0]?find_module(g,e->module):e->owner_module;
            target=module<0||module>=g->module_count?-1:find_func(g,module,e->name);
            if(target<0) {
                if(error&&error_size)snprintf(error,error_size,
                    "line %d: unresolved/unsupported thread expression target %s%s%s",
                    e->line,e->module[0]?e->module:"",e->module[0]?"::":"",e->name);
                return 0;
            }
            if(e->argc<0||e->argc>MAX_PARAMS) {
                if(error&&error_size)snprintf(error,error_size,
                    "line %d: thread expression %s has invalid structural argument count %d",
                    e->line,e->name,e->argc);
                return 0;
            }
            e->target=target;
            continue;
        }
        if(e->kind!=E_SCOPED_CALL)continue;
        module=find_module(g,e->module);
        target=module<0?-1:find_func(g,module,e->name);
        if(target<0) {
            if(error&&error_size)
                snprintf(error,error_size,"line %d: unresolved/unsupported expression call %s::%s",
                         e->line,e->module,e->name);
            return 0;
        }
        if(e->argc<0||e->argc>MAX_PARAMS) {
            if(error&&error_size)
                snprintf(error,error_size,
                         "line %d: expression call %s::%s has invalid structural argument count %d",
                         e->line,e->module,e->name,e->argc);
            return 0;
        }
        e->target=target;
    }
    g->requires_local_expr_call=0;
    for(n=0;n<g->expr_count;n++) {
        Expr *e=&g->expr[n];ExpressionBinding binding;
        if(e->kind!=E_ISDEFINED&&e->kind!=E_NATIVE_CALL)continue;
        binding=resolve_expression_binding(g,e);
        e->call_binding=binding.kind;e->target=binding.target;
        if(binding.kind==GSC_EXPR_TARGET_LOCAL_SCRIPT)g->requires_local_expr_call=1;
    }
    /* All checks have passed; this allocation-free commit cannot fail. Only
     * the new receiver tags are covered by this transactional guarantee. */
    for(n=0;n<g->expr_count;n++){const Expr *e=&g->expr[n];int d=actor_look_descriptor(g,e->name),k=actor_look_kind(d),wanted,type;
        if(e->kind!=E_METHOD_CALL||!k||e->module[0]||find_func(g,e->owner_module,e->name)>=0||e->argc!=1)continue;
        wanted=(k==GSC_ACTOR_LOOK_CAN_SHOOT||k==GSC_ACTOR_LOOK_AIM_AT_POS||k==GSC_ACTOR_LOOK_MAY_MOVE)?V_VECTOR:(actor_look_tag(k)||k==GSC_ACTOR_TRAVERSE_MODE)?V_STRING:k==GSC_ACTOR_LOOK_CAN_SEE?V_ENTITY:-1;type=known_value_kind(g,e->args[0]);
        if(wanted>=0&&((type>=0&&type!=wanted)||g->expr[e->args[0]].kind==E_ANIM_REF||g->expr[e->args[0]].kind==E_ANIMTREE_VALUE)){if(error&&error_size)snprintf(error,error_size,"line %d: actor look expression literal type mismatch",e->audit_line);return 0;}
    }
    for(n=0;n<g->expr_count;n++){const Expr *e=&g->expr[n];int a;
        if(e->kind!=E_METHOD_CALL||e->module[0]||find_func(g,e->owner_module,e->name)>=0||actor_look_kind(actor_look_descriptor(g,e->name))!=GSC_ACTOR_LOOK_CHECK_PRONE)continue;
        if(e->argc!=3){if(error&&error_size)snprintf(error,error_size,"checkProne requires three arguments");return 0;}
        for(a=0;a<3;a++){int type=known_value_kind(g,e->args[a]),want=a==0?V_VECTOR:V_NUMBER;
         if((type>=0&&type!=want)||g->expr[e->args[a]].kind==E_ANIM_REF||g->expr[e->args[a]].kind==E_ANIMTREE_VALUE){if(error&&error_size)snprintf(error,error_size,"checkProne expression literal type mismatch");return 0;}}
    }
    commit_receiver_bindings(g);
    developer_index_code(g);g->linked=1;return 1;
}
#include "gsc_r232_developer.inc"
#include "gsc_r232_function_admission.inc"
#include "gsc_vm_r243_staged_descriptor.inc"
int gsc_program_link(GscProgram *g,const char *entry_module,
                     char *error,size_t error_size) {
    return program_link(g,entry_module,0,0,0,0,0,0,0,0,0,0,0,0,error,error_size);
}
int gsc_program_link_with_entities(GscProgram *g,const char *entry_module,
                                   char *error,size_t error_size) {
    return program_link(g,entry_module,1,0,0,0,0,0,0,0,0,0,0,0,error,error_size);
}
int gsc_program_link_with_access(GscProgram *g,const char *entry_module,
                                 char *error,size_t error_size) {
    return program_link(g,entry_module,1,1,0,0,0,0,0,0,0,0,0,0,error,error_size);
}
int gsc_program_link_with_events(GscProgram *g,const char *entry_module,
                                 char *error,size_t error_size) {
    return program_link(g,entry_module,1,1,1,0,0,0,0,0,0,0,0,0,error,error_size);
}
int gsc_program_link_with_state(GscProgram *g,const char *entry_module,
                                char *error,size_t error_size) {
    return program_link(g,entry_module,1,1,1,1,0,0,0,0,0,0,0,0,error,error_size);
}

int gsc_program_link_with_controls(GscProgram *g,const char *entry_module,
                                   char *error,size_t error_size) {
    return program_link(g,entry_module,1,1,1,1,1,0,0,0,0,0,0,0,error,error_size);
}
int gsc_program_link_with_flow(GscProgram *g,const char *entry_module,
                               char *error,size_t error_size) {
    return program_link(g,entry_module,1,1,1,1,1,1,0,0,0,0,0,0,error,error_size);
}

int gsc_program_link_with_arrays(GscProgram *g,const char *entry_module,
                                 char *error,size_t error_size) {
    return program_link(g,entry_module,1,1,1,1,1,1,1,0,0,0,0,0,error,error_size);
}

int gsc_program_link_with_results(GscProgram *g,const char *entry_module,
                                  char *error,size_t error_size) {
    return program_link(g,entry_module,1,1,1,1,1,1,1,1,0,0,0,0,error,error_size);
}
int gsc_program_link_with_event_payloads(GscProgram *g,const char *entry_module,
                                        char *error,size_t error_size) {
    if(!g||!g->event_payload_syntax||g->linked) {
        if(error&&error_size)snprintf(error,error_size,"event payload program constructor is required");
        return 0;
    }
    return program_link(g,entry_module,1,1,1,1,1,1,1,1,1,0,0,0,error,error_size);
}

int gsc_program_link_with_predicates(GscProgram *g,const char *entry_module,
                                    char *error,size_t error_size) {
    if(!g||!g->predicate_syntax||!g->event_payload_syntax||g->linked) {
        if(error&&error_size)snprintf(error,error_size,"predicate program constructor is required");
        return 0;
    }
    return program_link(g,entry_module,1,1,1,1,1,1,1,1,1,1,0,0,error,error_size);
}

int gsc_program_link_with_actor_spawners(GscProgram *g,const char *entry_module,
    char *error,size_t error_size) {
    if(!g||!g->actor_spawn_syntax||!g->predicate_syntax||!g->event_payload_syntax||g->linked) {
        if(error&&error_size)snprintf(error,error_size,"actor-spawner program constructor is required");
        return 0;
    }
    return program_link(g,entry_module,1,1,1,1,1,1,1,1,1,1,1,0,error,error_size);
}

int gsc_program_link_with_function_refs(GscProgram *g,const char *entry_module,
    char *error,size_t error_size) {
    if(!g||!g->function_ref_syntax||!g->function_program_token||g->linked) {
        if(error&&error_size)snprintf(error,error_size,"function-reference program constructor is required");
        return 0;
    }
    return program_link(g,entry_module,1,1,1,1,1,1,1,1,1,1,1,1,error,error_size);
}

int gsc_program_link_with_scalar_switch(GscProgram *g,const char *entry_module,
    char *error,size_t error_size) {
    int ok;
    if(!g||!g->scalar_switch_syntax||g->linked) {
        if(error&&error_size)snprintf(error,error_size,"scalar/switch program constructor is required");
        return 0;
    }
    ok=gsc_program_link_with_function_refs(g,entry_module,error,error_size);
    if(ok)g->scalar_switch_mode=1;
    return ok;
}

int gsc_program_link_with_lifetime(GscProgram *g,const char *entry_module,
    char *error,size_t error_size) {
    int ok;
    if(!g||!g->lifetime_syntax||g->linked) {
        if(error&&error_size)snprintf(error,error_size,"lifetime program constructor is required");
        return 0;
    }
    ok=gsc_program_link_with_scalar_switch(g,entry_module,error,error_size);
    if(ok)g->lifetime_mode=1;
    return ok;
}

int gsc_program_link_with_names(GscProgram *g,const char *entry_module,
    char *error,size_t error_size) {
    int ok;
    if(!g||!g->names_syntax||g->linked) {
        if(error&&error_size)snprintf(error,error_size,"names program constructor is required");
        return 0;
    }
    g->names_mode=1;
    ok=gsc_program_link_with_lifetime(g,entry_module,error,error_size);
    if(!ok)g->names_mode=0;
    return ok;
}
int gsc_program_link_with_music(GscProgram *g,const char *entry_module,
    char *error,size_t error_size) {
    int ok;
    if(!g||!g->music_syntax||g->linked) {
        if(error&&error_size)snprintf(error,error_size,"music program constructor is required");
        return 0;
    }
    g->music_mode=1;
    ok=gsc_program_link_with_names(g,entry_module,error,error_size);
    if(!ok)g->music_mode=0;
    return ok;
}

int gsc_program_link_with_resources(GscProgram *g,const char *entry_module,
    char *error,size_t error_size) {
    int ok;
    if(!g||!g->resources_syntax||g->linked) {
        if(error&&error_size)snprintf(error,error_size,"resources program constructor is required");
        return 0;
    }
    g->resources_mode=1;
    ok=gsc_program_link_with_music(g,entry_module,error,error_size);
    if(!ok)g->resources_mode=0;
    return ok;
}

int gsc_program_link_with_objectives(GscProgram *g,const char *entry_module,
    char *error,size_t error_size) {
    int ok;
    if(!g||!g->objectives_syntax||g->linked) {
        if(error&&error_size)snprintf(error,error_size,"objectives program constructor is required");
        return 0;
    }
    g->objectives_mode=1;
    ok=gsc_program_link_with_resources(g,entry_module,error,error_size);
    if(!ok)g->objectives_mode=0;
    return ok;
}

int gsc_program_link_with_console(GscProgram *g,const char *entry_module,
    char *error,size_t error_size) {
    int ok;
    if(!g||!g->console_syntax||g->linked) {
        if(error&&error_size)snprintf(error,error_size,"console program constructor is required");
        return 0;
    }
    g->console_mode=1;
    ok=gsc_program_link_with_objectives(g,entry_module,error,error_size);
    if(!ok)g->console_mode=0;
    return ok;
}
int gsc_program_link_with_hud(GscProgram *g,const char *entry_module,char *error,size_t error_size) {
    int ok;
    if(!g||!g->hud_syntax||g->linked) {
        if(error&&error_size)snprintf(error,error_size,"HUD program constructor is required");
        return 0;
    }
    g->hud_mode=1;
    ok=gsc_program_link_with_console(g,entry_module,error,error_size);
    if(!ok)g->hud_mode=0;
    return ok;
}


int gsc_program_link_with_mission(GscProgram *g,const char *entry_module,char *error,size_t error_size) {
    int ok;
    if(!g||!g->mission_syntax||g->linked) {
        if(error&&error_size)snprintf(error,error_size,"mission program constructor is required");
        return 0;
    }
    g->mission_mode=1;
    ok=gsc_program_link_with_hud(g,entry_module,error,error_size);
    if(!ok)g->mission_mode=0;
    return ok;
}

int gsc_program_link_with_blast(GscProgram *g,const char *entry_module,char *error,size_t error_size) {
    int ok;
    if(!g||!g->blast_syntax||g->linked) {
        if(error&&error_size)snprintf(error,error_size,"blast program constructor is required");
        return 0;
    }
    g->blast_mode=1;
    ok=gsc_program_link_with_mission(g,entry_module,error,error_size);
    if(!ok)g->blast_mode=0;
    return ok;
}
int gsc_program_link_with_quake(GscProgram *g,const char *entry_module,char *error,size_t error_size) {
    int ok;
    if(!g||!g->quake_syntax||g->linked) {
        if(error&&error_size)snprintf(error,error_size,"quake program constructor is required");
        return 0;
    }
    g->quake_mode=1;
    ok=gsc_program_link_with_blast(g,entry_module,error,error_size);
    if(!ok)g->quake_mode=0;
    return ok;
}
int gsc_program_link_with_fx(GscProgram *g,const char *entry_module,char *error,size_t error_size) {
    int ok;
    if(!g||!g->fx_syntax||g->linked) {
        if(error&&error_size)snprintf(error,error_size,"FX program constructor is required");
        return 0;
    }
    g->fx_mode=1;
    ok=gsc_program_link_with_quake(g,entry_module,error,error_size);
    if(!ok)g->fx_mode=0;
    return ok;
}


int gsc_program_link_with_cvars(GscProgram *g,const char *entry_module,char *error,size_t error_size) {
    int ok;
    if(!g||!g->cvars_syntax||g->linked) {
        if(error&&error_size)snprintf(error,error_size,"cvars program constructor is required");
        return 0;
    }
    g->cvars_mode=1;
    ok=gsc_program_link_with_fx(g,entry_module,error,error_size);
    if(!ok)g->cvars_mode=0;
    return ok;
}
int gsc_program_link_with_shellshock(GscProgram *g,const char *entry_module,char *error,size_t error_size) {
    int ok;
    if(!g||!g->shellshock_syntax||g->linked) {
        if(error&&error_size)snprintf(error,error_size,"shellshock program constructor is required");
        return 0;
    }
    g->shellshock_mode=1;
    ok=gsc_program_link_with_cvars(g,entry_module,error,error_size);
    if(!ok)g->shellshock_mode=0;
    return ok;
}
int gsc_program_link_with_debug_text(GscProgram *g,const char *entry_module,char *error,size_t error_size) {
    int ok;
    if(!g||!g->debug_text_syntax||g->linked) {
        if(error&&error_size)snprintf(error,error_size,"debug text program constructor is required");
        return 0;
    }
    g->debug_text_mode=1;
    ok=gsc_program_link_with_shellshock(g,entry_module,error,error_size);
    if(!ok)g->debug_text_mode=0;
    return ok;
}
int gsc_program_link_with_debug_line(GscProgram *g,const char *entry_module,char *error,size_t error_size) {
    int ok;
    if(!g||!g->debug_line_syntax||g->linked) {
        if(error&&error_size)snprintf(error,error_size,"debug line program constructor is required");
        return 0;
    }
    g->debug_line_mode=1;
    ok=gsc_program_link_with_debug_text(g,entry_module,error,error_size);
    if(!ok)g->debug_line_mode=0;
    return ok;
}
int gsc_program_link_with_ambient(GscProgram *g,const char *entry_module,char *error,size_t error_size) {
    int ok;
    if(!g||!g->ambient_syntax||g->linked) {
        if(error&&error_size)snprintf(error,error_size,"ambient program constructor is required");
        return 0;
    }
    g->ambient_mode=1;
    ok=gsc_program_link_with_debug_line(g,entry_module,error,error_size);
    if(!ok)g->ambient_mode=0;
    return ok;
}
int gsc_program_link_with_precache_item(GscProgram *g,const char *entry_module,char *error,size_t error_size) {
    int ok;
    if(!g||!g->precache_item_syntax||g->linked) {
        if(error&&error_size)snprintf(error,error_size,"precacheItem program constructor is required");
        return 0;
    }
    g->precache_item_mode=1;
    ok=gsc_program_link_with_ambient(g,entry_module,error,error_size);
    if(!ok)g->precache_item_mode=0;
    return ok;
}
int gsc_program_link_with_save_game(GscProgram *g,const char *entry_module,char *error,size_t error_size) {
    int ok;
    if(!g||!g->save_game_syntax||g->linked) {
        if(error&&error_size)snprintf(error,error_size,"saveGame program constructor is required");
        return 0;
    }
    g->save_game_mode=1;
    ok=gsc_program_link_with_precache_item(g,entry_module,error,error_size);
    if(!ok)g->save_game_mode=0;
    return ok;
}
int gsc_program_link_with_tag_fx(GscProgram *g,const char *entry_module,char *error,size_t error_size) {
    int ok;
    if(!g||!g->tag_fx_syntax||g->linked) {
        if(error&&error_size)snprintf(error,error_size,"playfxOnTag program constructor is required");
        return 0;
    }
    g->tag_fx_mode=1;
    ok=gsc_program_link_with_save_game(g,entry_module,error,error_size);
    if(!ok)g->tag_fx_mode=0;
    return ok;
}
int gsc_program_link_with_native_queries(GscProgram *g,const char *entry_module,char *error,size_t error_size) {
    int ok;
    if(!g||!g->native_queries_syntax||g->linked) {
        if(error&&error_size)snprintf(error,error_size,"native query program constructor is required");
        return 0;
    }
    g->native_queries_mode=1;
    ok=gsc_program_link_with_tag_fx(g,entry_module,error,error_size);
    if(!ok)g->native_queries_mode=0;
    return ok;
}
int gsc_program_link_with_precache_string(GscProgram *g,const char *entry_module,char *error,size_t error_size) {
    int ok;
    if(!g||!g->precache_string_syntax||g->linked) {
        if(error&&error_size)snprintf(error,error_size,"precacheString program constructor is required");
        return 0;
    }
    g->precache_string_mode=1;
    ok=gsc_program_link_with_native_queries(g,entry_module,error,error_size);
    if(!ok)g->precache_string_mode=0;
    return ok;
}
int gsc_program_link_with_spawner_arrays(GscProgram *g,const char *entry_module,char *error,size_t error_size) {
    int ok;
    if(!g||!g->spawner_arrays_syntax||g->linked) {
        if(error&&error_size)snprintf(error,error_size,"spawner array program constructor is required");
        return 0;
    }
    g->spawner_arrays_mode=1;
    ok=gsc_program_link_with_precache_string(g,entry_module,error,error_size);
    if(!ok)g->spawner_arrays_mode=0;
    return ok;
}
int gsc_program_link_with_randomint(GscProgram *g,const char *entry_module,char *error,size_t error_size) {
    int ok;
    if(!g||!g->randomint_syntax||g->linked) {
        if(error&&error_size)snprintf(error,error_size,"randomint program constructor is required");
        return 0;
    }
    g->randomint_mode=1;
    ok=gsc_program_link_with_spawner_arrays(g,entry_module,error,error_size);
    if(!ok)g->randomint_mode=0;
    return ok;
}
int gsc_program_link_with_randomfloat(GscProgram *g,const char *entry_module,char *error,size_t error_size) {
    int ok;
    if(!g||!g->randomfloat_syntax||g->linked) {
        if(error&&error_size)snprintf(error,error_size,"randomfloat program constructor is required");
        return 0;
    }
    g->randomfloat_mode=1;
    ok=gsc_program_link_with_randomint(g,entry_module,error,error_size);
    if(!ok)g->randomfloat_mode=0;
    return ok;
}
int gsc_program_link_with_keybinding(GscProgram *g,const char *entry_module,char *error,size_t error_size) {
    int ok;
    if(!g||!g->keybinding_syntax||g->linked) {
        if(error&&error_size)snprintf(error,error_size,"keybinding program constructor is required");
        return 0;
    }
    g->keybinding_mode=1;
    ok=gsc_program_link_with_randomfloat(g,entry_module,error,error_size);
    if(!ok)g->keybinding_mode=0;
    return ok;
}




#include "gsc_link_audit.inc"
#include "gsc_receiver_inspect.inc"

GscProgram *gsc_compile(const char *source,char *error,size_t error_size) {
    GscProgram *g=gsc_program_create();
    if(!g) {if(error&&error_size)snprintf(error,error_size,"out of memory");return NULL;}
    if(!gsc_module_compile(g,"fixture",source,error,error_size)||
       !gsc_program_link(g,"fixture",error,error_size)) {
        gsc_program_free(g);return NULL;
    }
    return g;
}
void gsc_program_free(GscProgram *program) {
    int n;
    if(!program)return;
    for(n=0;n<program->func_count;n++)free(program->funcs[n].params);
    free(program->expr);free(program->code);
    free(program->loops);free(program->whiles);free(program->do_whiles);
    free(program->switches);free(program->cases);
    free(program->funcs);free(program->modules);free(program->dev_blocks);
    free(program->strings);
    free(program);
}

int gsc_program_string_literal_count(const GscProgram *g) {
    return g&&!g->invalid?g->string_count:0;
}
int gsc_program_string_literal_get(const GscProgram *g,int index,GscStringLiteralInfo *out) {
    if(!g||g->invalid||!out||index<0||index>=g->string_count)return 0;
    *out=g->strings[index];return 1;
}

int gsc_program_empty_stmt_count(const GscProgram *g) {
    int n,count=0;
    if(!g||!g->linked||g->invalid)return 0;
    for(n=0;n<g->code_count;n++)if(g->code[n].kind==I_EMPTY)count++;
    return count;
}
int gsc_program_empty_stmt_get(const GscProgram *g,int index,GscEmptyStmtInfo *out) {
    int n,seen=0;
    if(!g||!g->linked||g->invalid||!out||index<0)return 0;
    for(n=0;n<g->code_count;n++) {
        const Instr *i=&g->code[n];
        if(i->kind!=I_EMPTY)continue;
        if(seen++!=index)continue;
        if(i->owner_module<0||i->owner_module>=g->module_count||i->line<1||i->col<1)
            return 0;
        out->instruction=n;out->module=i->owner_module;
        out->line=i->line;out->col=i->col;
        return 1;
    }
    return 0;
}

int gsc_program_dev_block_count(const GscProgram *g) {
    if(!g||!g->linked||g->invalid)return 0;
    return g->dev_count;
}
int gsc_program_dev_block_get(const GscProgram *g,int index,GscDevBlockInfo *out) {
    const DevBlock *block;
    if(!g||!g->linked||g->invalid||!out||index<0||index>=g->dev_count)return 0;
    block=&g->dev_blocks[index];
    out->scope=block->scope;out->module=block->module;
    out->line=block->line;out->col=block->col;
    out->code_start=block->code_start;out->code_end=block->code_end;
    out->func_start=block->func_start;out->func_end=block->func_end;
    return 1;
}

int gsc_program_mutation_count(const GscProgram *g) {
    int n,count=0;
    if(!g||!g->linked||g->invalid)return 0;
    for(n=0;n<g->code_count;n++)if(g->code[n].kind==I_MUTATE)count++;
    return count;
}
int gsc_program_mutation_get(const GscProgram *g,int index,GscMutationInfo *out) {
    int n,seen=0;
    if(!g||!g->linked||g->invalid||!out||index<0)return 0;
    for(n=0;n<g->code_count;n++) {
        const Instr *i=&g->code[n];
        if(i->kind!=I_MUTATE)continue;
        if(seen++!=index)continue;
        if(i->dest<0||i->dest>=g->expr_count||i->op<GSC_MUT_ADD||
           (i->op>GSC_MUT_DEC&&!(g->integer_bitwise_selected&&integer_bitwise_mutation_op(i->op)))||
           i->expr>=g->expr_count||i->expr< -1||
           ((i->op<=GSC_MUT_DIV||integer_bitwise_mutation_op(i->op))!=(i->expr>=0)))return 0;
        out->line=i->line;out->op=i->op;
        out->lvalue_expr=i->dest;out->rhs_expr=i->expr;
        return 1;
    }
    return 0;
}
int gsc_program_lvalue_get(const GscProgram *g,int expr,GscLvalueInfo *out) {
    const Expr *e;
    if(!g||!g->linked||g->invalid||!out||expr<0||expr>=g->expr_count)return 0;
    e=&g->expr[expr];memset(out,0,sizeof(*out));
    out->base_expr=-1;out->index_expr=-1;
    if(e->kind==E_VAR) {
        out->kind=GSC_LVALUE_VAR;str_copy(out->name,sizeof(out->name),e->name);
    } else if(e->kind==E_FIELD) {
        if(e->a<0||e->a>=g->expr_count)return 0;
        out->kind=GSC_LVALUE_FIELD;out->base_expr=e->a;
        str_copy(out->name,sizeof(out->name),e->name);
    } else if(e->kind==E_INDEX) {
        if(e->a<0||e->a>=g->expr_count||e->b<0||e->b>=g->expr_count)return 0;
        out->kind=GSC_LVALUE_INDEX;out->base_expr=e->a;out->index_expr=e->b;
    } else return 0;
    return 1;
}

/* Header inspection is syntax-only and does not require a successful link.
 * A for-loop's sole owner instruction immediately precedes its body span. */
int gsc_program_for_count(const GscProgram *g) {
    return g&&!g->invalid?g->loop_count:0;
}
int gsc_program_for_get(const GscProgram *g,int index,GscForInfo *out) {
    const ForNode *f;const Instr *owner;int instruction;
    if(!g||g->invalid||!out||index<0||index>=g->loop_count)return 0;
    f=&g->loops[index];
    if(f->body_start<1||f->body_start>g->code_count||
       f->body_end<f->body_start||f->body_end>g->code_count)return 0;
    instruction=f->body_start-1;owner=&g->code[instruction];
    if(owner->kind!=I_FOR||owner->dest!=index||f->line<1||
       owner->owner_module<0||owner->owner_module>=g->module_count)return 0;
    if(f->init_target< -1||f->init_target>=g->expr_count||
       f->init_expr< -1||f->init_expr>=g->expr_count||
       ((f->init_target>=0)!=(f->init_expr>=0))||
       f->cond_expr< -1||f->cond_expr>=g->expr_count||
       f->step_target< -1||f->step_target>=g->expr_count||
       f->step_expr< -1||f->step_expr>=g->expr_count||
       f->step_kind<FOR_NO_STEP||f->step_kind>FOR_COMPOUND_ASSIGN)return 0;
    if(f->step_kind==FOR_NO_STEP) {
        if(f->step_target!=-1||f->step_expr!=-1||f->step_op)return 0;
    } else {
        if(f->step_target<0)return 0;
        if((f->step_kind==FOR_ASSIGN||f->step_kind==FOR_COMPOUND_ASSIGN)!=(f->step_expr>=0))return 0;
        if(f->step_kind==FOR_COMPOUND_ASSIGN) {
            int kind=g->expr[f->step_target].kind;
            if(f->step_op<GSC_MUT_ADD||(f->step_op>GSC_MUT_DIV&&
               !(g->integer_bitwise_selected&&integer_bitwise_mutation_op(f->step_op)))||
               (kind!=E_VAR&&kind!=E_FIELD&&kind!=E_INDEX))return 0;
        } else if(f->step_op)return 0;
    }
    memset(out,0,sizeof(*out));out->index=index;out->instruction=instruction;
    out->owner_module=owner->owner_module;out->line=f->line;
    out->init_target=f->init_target;out->init_expr=f->init_expr;out->cond_expr=f->cond_expr;
    out->step_target=f->step_target;out->step_expr=f->step_expr;
    out->step_kind=f->step_kind;out->step_op=f->step_op;
    out->body_start=f->body_start;out->body_end=f->body_end;
    return 1;
}

int gsc_program_flow_count(const GscProgram *g) {
    int n,count=0;
    if(!g||!g->linked||g->invalid)return 0;
    for(n=0;n<g->code_count;n++)
        if(g->code[n].kind==I_FOR||g->code[n].kind==I_WHILE||
           g->code[n].kind==I_RETURN||g->code[n].kind==I_BREAK||
           g->code[n].kind==I_CONTINUE||g->code[n].kind==I_SWITCH||
           g->code[n].kind==I_DO_WHILE)count++;
    return count;
}
int gsc_program_flow_get(const GscProgram *g,int index,GscFlowInfo *out) {
    int n,seen=0;
    if(!g||!g->linked||g->invalid||!out||index<0)return 0;
    for(n=0;n<g->code_count;n++) {
        const Instr *i=&g->code[n];
        int kind=i->kind;
        if(kind!=I_FOR&&kind!=I_WHILE&&kind!=I_RETURN&&kind!=I_BREAK&&
           kind!=I_CONTINUE&&kind!=I_SWITCH&&kind!=I_DO_WHILE)continue;
        if(seen++!=index)continue;
        memset(out,0,sizeof(*out));out->line=i->line;out->expr=-1;
        out->body_start=out->body_end=-1;out->instruction=n;
        out->target_instruction=-1;out->target_kind=0;
        if(kind==I_FOR) {
            const ForNode *loop;
            if(i->dest<0||i->dest>=g->loop_count)return 0;
            loop=&g->loops[i->dest];out->kind=GSC_FLOW_FOR;
            out->expr=loop->cond_expr;
            out->body_start=loop->body_start;out->body_end=loop->body_end;
        } else if(kind==I_WHILE) {
            const WhileNode *loop;
            if(i->dest<0||i->dest>=g->while_count)return 0;
            loop=&g->whiles[i->dest];out->kind=GSC_FLOW_WHILE;
            out->expr=loop->cond_expr;
            out->body_start=loop->body_start;out->body_end=loop->body_end;
        } else if(kind==I_DO_WHILE) {
            const DoWhileNode *loop;
            if(i->dest<0||i->dest>=g->do_while_count)return 0;
            loop=&g->do_whiles[i->dest];out->kind=GSC_FLOW_DO_WHILE;
            out->expr=loop->cond_expr;
            out->body_start=loop->body_start;out->body_end=loop->body_end;
        } else if(kind==I_SWITCH) {
            const SwitchNode *sw;
            if(i->dest<0||i->dest>=g->switch_count)return 0;
            sw=&g->switches[i->dest];out->kind=GSC_FLOW_SWITCH;
            out->expr=sw->selector_expr;
            out->body_start=sw->body_start;out->body_end=sw->body_end;
        } else if(kind==I_RETURN) {
            out->kind=GSC_FLOW_RETURN;out->expr=i->expr;
        } else {
            int target_kind;
            if(i->target<0||i->target>=n)return 0;
            target_kind=g->code[i->target].kind;
            if(target_kind!=I_FOR&&target_kind!=I_WHILE&&target_kind!=I_SWITCH&&
               target_kind!=I_DO_WHILE)return 0;
            if(kind==I_CONTINUE&&target_kind==I_SWITCH)return 0;
            out->kind=kind==I_BREAK?GSC_FLOW_BREAK:GSC_FLOW_CONTINUE;
            out->target_instruction=i->target;
            out->target_kind=target_kind==I_FOR?GSC_FLOW_FOR:
                             target_kind==I_WHILE?GSC_FLOW_WHILE:
                             target_kind==I_DO_WHILE?GSC_FLOW_DO_WHILE:GSC_FLOW_SWITCH;
        }
        return 1;
    }
    return 0;
}

int gsc_program_do_while_count(const GscProgram *g) {
    return g&&!g->invalid?g->do_while_count:0;
}
int gsc_program_do_while_get(const GscProgram *g,int index,GscDoWhileInfo *out) {
    const DoWhileNode *loop;const Instr *i;
    if(!g||g->invalid||!out||index<0||index>=g->do_while_count)return 0;
    loop=&g->do_whiles[index];
    if(loop->module<0||loop->module>=g->module_count||loop->instruction<0||
       loop->instruction>=g->code_count||loop->body_start!=loop->instruction+1||
       loop->body_end<loop->body_start||loop->body_end>g->code_count||
       loop->cond_expr<0||loop->cond_expr>=g->expr_count||
       loop->line<1||loop->col<1||loop->while_line<loop->line||loop->while_col<1||
       loop->end_line<loop->while_line||loop->end_col<1)return 0;
    i=&g->code[loop->instruction];
    if(i->kind!=I_DO_WHILE||i->dest!=index||i->expr!=loop->cond_expr||
       i->owner_module!=loop->module||i->line!=loop->line||i->col!=loop->col)return 0;
    *out=*loop;return 1;
}

int gsc_program_switch_count(const GscProgram *g) {
    return g&&g->linked&&!g->invalid?g->switch_count:0;
}
int gsc_program_switch_get(const GscProgram *g,int index,GscSwitchInfo *out) {
    const SwitchNode *sw;
    if(!g||!g->linked||g->invalid||!out||index<0||index>=g->switch_count)return 0;
    sw=&g->switches[index];memset(out,0,sizeof(*out));
    out->module=sw->module;out->line=sw->line;out->selector_expr=sw->selector_expr;
    out->instruction=sw->instruction;out->body_start=sw->body_start;out->body_end=sw->body_end;
    out->first_case=sw->first_case;out->default_case=sw->default_case;
    out->case_count=sw->case_count;
    return 1;
}
int gsc_program_case_count(const GscProgram *g) {
    return g&&g->linked&&!g->invalid?g->case_count:0;
}
int gsc_program_case_get(const GscProgram *g,int index,GscCaseInfo *out) {
    const CaseNode *c;
    if(!g||!g->linked||g->invalid||!out||index<0||index>=g->case_count)return 0;
    c=&g->cases[index];memset(out,0,sizeof(*out));
    out->owner_switch=c->owner_switch;out->line=c->line;out->kind=c->kind;
    out->integer=c->integer;str_copy(out->string,sizeof(out->string),c->string);
    out->string_literal=c->string_literal;
    out->body_start=c->body_start;out->body_end=c->body_end;out->next_case=c->next_case;
    return 1;
}

int gsc_program_integer_op_count(const GscProgram *g) {
    int n,count=0;
    if(!g||!g->linked||g->invalid)return 0;
    for(n=0;n<g->expr_count;n++)if(g->expr[n].kind==E_BINARY&&
        (g->expr[n].op=='%'||g->expr[n].op=='&'))count++;
    return count;
}
int gsc_program_integer_op_get(const GscProgram *g,int index,GscIntegerOpInfo *out) {
    int n,seen=0;
    if(!g||!g->linked||g->invalid||!out||index<0)return 0;
    for(n=0;n<g->expr_count;n++) {
        const Expr *e=&g->expr[n];
        if(e->kind!=E_BINARY||(e->op!='%'&&e->op!='&'))continue;
        if(seen++!=index)continue;
        out->expr=n;out->line=e->line;out->left_expr=e->a;out->right_expr=e->b;
        out->kind=e->op=='%'?GSC_INTEGER_MOD:GSC_INTEGER_BITAND;
        return 1;
    }
    return 0;
}

int gsc_program_vector_count(const GscProgram *g) {
    int n,count=0;
    if(!g||!g->linked||g->invalid)return 0;
    for(n=0;n<g->expr_count;n++)if(g->expr[n].kind==E_VECTOR)count++;
    return count;
}
int gsc_program_vector_get(const GscProgram *g,int index,GscVectorInfo *out) {
    int n,seen=0,k;
    if(!g||!g->linked||g->invalid||!out||index<0)return 0;
    for(n=0;n<g->expr_count;n++) {
        const Expr *e=&g->expr[n];
        if(e->kind!=E_VECTOR)continue;
        if(seen++!=index)continue;
        if(e->argc!=3)return 0;
        out->expr=n;
        for(k=0;k<3;k++) {
            if(e->args[k]<0||e->args[k]>=n)return 0;
            out->components[k]=e->args[k];
        }
        return 1;
    }
    return 0;
}

int gsc_program_cast_count(const GscProgram *g) {
    int n,count=0;
    if(!g||!g->linked||g->invalid)return 0;
    for(n=0;n<g->expr_count;n++)if(g->expr[n].kind==E_CAST)count++;
    return count;
}
int gsc_program_cast_get(const GscProgram *g,int index,GscCastInfo *out) {
    int n,seen=0;
    if(!g||!g->linked||g->invalid||!out||index<0)return 0;
    for(n=0;n<g->expr_count;n++) {
        const Expr *e=&g->expr[n];
        if(e->kind!=E_CAST)continue;
        if(seen++!=index)continue;
        if((e->op!=GSC_CAST_FLOAT&&e->op!=GSC_CAST_INT)||e->a<0||e->a>=n)return 0;
        out->expr=n;out->line=e->line;out->kind=e->op;out->operand_expr=e->a;
        return 1;
    }
    return 0;
}

int gsc_program_anim_ref_count(const GscProgram *g) {
    int n,count=0;
    if(!g||!g->linked||g->invalid)return 0;
    for(n=0;n<g->expr_count;n++)if(g->expr[n].kind==E_ANIM_REF)count++;
    return count;
}
int gsc_program_anim_ref_get(const GscProgram *g,int index,GscAnimRefInfo *out) {
    int n,seen=0;
    if(!g||!g->linked||g->invalid||!out||index<0)return 0;
    for(n=0;n<g->expr_count;n++) {
        const Expr *e=&g->expr[n];
        if(e->kind!=E_ANIM_REF)continue;
        if(seen++!=index)continue;
        out->expr=n;str_copy(out->name,sizeof(out->name),e->name);
        return 1;
    }
    return 0;
}

int gsc_program_animtree_value_count(const GscProgram *g) {
    int n,count=0;
    if(!g||!g->linked||g->invalid)return 0;
    for(n=0;n<g->expr_count;n++)if(g->expr[n].kind==E_ANIMTREE_VALUE)count++;
    return count;
}
int gsc_program_animtree_value_get(const GscProgram *g,int index,
                                   GscAnimtreeValueInfo *out) {
    int n,seen=0;
    if(!g||!g->linked||g->invalid||!out||index<0)return 0;
    for(n=0;n<g->expr_count;n++) {
        const Expr *e=&g->expr[n];
        if(e->kind!=E_ANIMTREE_VALUE)continue;
        if(seen++!=index)continue;
        out->expr=n;out->line=e->line;return 1;
    }
    return 0;
}

int gsc_program_empty_array_count(const GscProgram *g) {
    int n,count=0;
    if(!g||!g->linked||g->invalid)return 0;
    for(n=0;n<g->expr_count;n++)if(g->expr[n].kind==E_EMPTY_ARRAY)count++;
    return count;
}
int gsc_program_empty_array_get(const GscProgram *g,int index,GscEmptyArrayInfo *out) {
    int n,seen=0;
    if(!g||!g->linked||g->invalid||!out||index<0)return 0;
    for(n=0;n<g->expr_count;n++) {
        const Expr *e=&g->expr[n];
        if(e->kind!=E_EMPTY_ARRAY)continue;
        if(seen++!=index)continue;
        out->expr=n;out->line=e->line;return 1;
    }
    return 0;
}

int gsc_program_scalar_integer_literal_get(const GscProgram *g,int expr,int32_t *out) {
    if(!g||!g->linked||g->invalid||!g->scalar_switch_mode||!out||expr<0||
       expr>=g->expr_count||g->expr[expr].kind!=E_NUMBER||!g->expr[expr].integer)return 0;
    *out=g->expr[expr].integer_value;return 1;
}

/* Statement nodes first in instruction order, then expression nodes in AST
 * order. Both retain target, optional receiver, and every argument root. */
int gsc_program_dynamic_call_count(const GscProgram *g) {
    int n,count=0;
    if(!g||!g->linked||g->invalid)return 0;
    for(n=0;n<g->code_count;n++)if(g->code[n].kind==I_DYNAMIC_CALL||
                                   g->code[n].kind==I_DYNAMIC_THREAD)count++;
    for(n=0;n<g->expr_count;n++)if(g->expr[n].kind==E_DYNAMIC_CALL)count++;
    return count;
}
int gsc_program_dynamic_call_get(const GscProgram *g,int index,GscDynamicCallInfo *out) {
    int n,k,seen=0;
    if(!g||!g->linked||g->invalid||!out||index<0)return 0;
    memset(out,0,sizeof(*out));
    for(n=0;n<g->code_count;n++) {
        const Instr *i=&g->code[n];
        if(i->kind!=I_DYNAMIC_CALL&&i->kind!=I_DYNAMIC_THREAD)continue;
        if(seen++!=index)continue;
        if(i->expr<0||i->expr>=g->expr_count||i->receiver<-1||
           i->receiver>=g->expr_count||
           i->argc<0||i->argc>MAX_PARAMS)return 0;
        out->kind=i->kind==I_DYNAMIC_THREAD?GSC_DYNAMIC_THREAD:GSC_DYNAMIC_CALL;
        out->line=i->line;out->target_expr=i->expr;
        out->receiver=i->receiver;out->argc=i->argc;
        for(k=0;k<i->argc;k++) {
            if(i->args[k]<0||i->args[k]>=g->expr_count)return 0;
            out->args[k]=i->args[k];
        }
        return 1;
    }
    for(n=0;n<g->expr_count;n++) {
        const Expr *e=&g->expr[n];
        if(e->kind!=E_DYNAMIC_CALL)continue;
        if(seen++!=index)continue;
        if(e->a<0||e->a>=n||e->b<-1||e->b>=n||
           e->argc<0||e->argc>MAX_PARAMS)return 0;
        out->kind=GSC_DYNAMIC_EXPR_CALL;out->line=e->line;
        out->target_expr=e->a;out->receiver=e->b;out->argc=e->argc;
        for(k=0;k<e->argc;k++) {
            if(e->args[k]<0||e->args[k]>=n)return 0;
            out->args[k]=e->args[k];
        }
        return 1;
    }
    return 0;
}

int gsc_program_scoped_expr_call_count(const GscProgram *g) {
    int n,count=0;
    if(!g||!g->linked||g->invalid)return 0;
    for(n=0;n<g->expr_count;n++)if(g->expr[n].kind==E_SCOPED_CALL)count++;
    return count;
}
int gsc_program_scoped_expr_call_get(const GscProgram *g,int index,
                                     GscScopedExprCallInfo *out) {
    int n,k,seen=0;
    if(!g||!g->linked||g->invalid||!out||index<0)return 0;
    for(n=0;n<g->expr_count;n++) {
        const Expr *e=&g->expr[n];
        if(e->kind!=E_SCOPED_CALL)continue;
        if(seen++!=index)continue;
        if(e->target<0||e->target>=g->func_count||e->argc<0||
           e->argc>MAX_PARAMS)return 0;
        memset(out,0,sizeof(*out));out->expr=n;out->line=e->line;
        out->target=e->target;out->argc=e->argc;
        str_copy(out->module,sizeof(out->module),e->module);
        str_copy(out->name,sizeof(out->name),e->name);
        for(k=0;k<e->argc;k++) {
            if(e->args[k]<0||e->args[k]>=n)return 0;
            out->args[k]=e->args[k];
        }
        return 1;
    }
    return 0;
}

int gsc_program_func_ref_count(const GscProgram *g) {
    int n,count=0;
    if(!g||!g->linked||g->invalid)return 0;
    for(n=0;n<g->expr_count;n++)if(g->expr[n].kind==E_FUNC_REF)count++;
    return count;
}
int gsc_program_func_ref_get(const GscProgram *g,int index,GscFuncRefInfo *out) {
    int n,seen=0,module;
    const Expr *e;
    if(!g||!g->linked||g->invalid||!out||index<0)return 0;
    for(n=0;n<g->expr_count;n++) {
        e=&g->expr[n];
        if(e->kind!=E_FUNC_REF)continue;
        if(seen++!=index)continue;
        module=e->module[0]?find_module(g,e->module):e->owner_module;
        if(module<0||module>=g->module_count||e->target<0||
           e->target>=g->func_count||g->funcs[e->target].module!=module||
           !callable_name_equal(g,g->funcs[e->target].name,e->name))return 0;
        memset(out,0,sizeof(*out));
        out->expr=n;out->line=e->line;out->target=e->target;
        out->local=!e->module[0];
        str_copy(out->module,sizeof(out->module),g->modules[module]);
        str_copy(out->name,sizeof(out->name),e->name);
        return 1;
    }
    return 0;
}

static int rfail(GscRuntime *r,int line,const char *fmt,...) {
    va_list args;size_t prefix;
    configured_precache_close(r);
    if(r->error[0])return 0;
    snprintf(r->error,sizeof(r->error),"line %d: ",line);
    prefix=strlen(r->error);va_start(args,fmt);
    vsnprintf(r->error+prefix,sizeof(r->error)-prefix,fmt,args);
    va_end(args);
    if(r->executing_task && r->executing_task->depth>0 && r->executing_task->depth<=MAX_FRAMES){
     int k;size_t used;
     for(k=r->executing_task->depth-1;k>=0;k--){
      const Frame *f=r->executing_task->frames+k;
      if(f->fn<0||f->fn>=r->program->func_count)break;
      used=strlen(r->error);if(used+6>=sizeof(r->error))break;
      snprintf(r->error+used,sizeof(r->error)-used," [%s::%s/%d]",r->program->modules[r->program->funcs[f->fn].module],r->program->funcs[f->fn].name,f->pc);
     }
    }
    return 0;
}
static Task*task_at(const GscRuntime*,int);
#include "gsc_vm_r237_dirty_scope54.inc"
static int task_member(const GscRuntime*,const Task*);
static int task_directory_limit(const GscRuntime *r){
    return r->lifetime_enabled?GSC_LIFETIME_TASK_CAPACITY+r->extra_capacity:MAX_THREADS;
}
static int task_reserve_next(GscRuntime *,int);

static int r243s_init_private(const GscRuntime*,const void*,size_t);
#include "gsc_vm_r243_time_root.inc"
static int refresh_state_context(GscRuntime *,Task *);
static int entity_task_current(GscRuntime *);
static int entity_validate(GscRuntime *,const GscEntityRef *);
static int array_value_valid(const GscRuntime *,uint32_t);
static int animation_compare(GscRuntime *,int,const Value *,const Value *,int,Value *);
static int animation_value_valid(const GscRuntime *,const Value *);
static int animation_namespace_kind(int);
static int animation_namespace_scope(const GscRuntime *);
static int animation_namespace_tree_index(const char *);
static int animation_namespace_tree_index_for_program(const GscProgram *,const char *);
static int animation_namespace_value_valid(const GscRuntime *,const Value *);
static int animation_namespace_compare(GscRuntime *,int,const Value *,const Value *,int,Value *);
static int animation_references_current(GscRuntime *,Task *);
static int animation_value_exportable(const GscRuntime *,const Value *);
static int anim_data_current(GscRuntime *,Task *);
static int anim_data_value_valid(const GscRuntime *,const Value *);
static int anim_data_read(GscRuntime *,int,Value *);
static int game_array_selected(const GscRuntime *,const char *);
static int game_array_read(GscRuntime *,int,Value *);
static int game_array_root_matches(GscRuntime *,const Value *,int);
static int truth(Value);
static int console_context(GscRuntime *);
static int float_arithmetic_active(const GscRuntime *);
static int float_arithmetic_binary(GscRuntime *,int,const Value *,const Value *,int,Value *);
static int r232_source_float_value(GscRuntime *,int,const Value *,const Value *,int,Value *);
static int r232_source_float_cast(GscRuntime *,int,const Value *,int,Value *);
static int string_int_concat_pair(const GscRuntime *,const Value *,const Value *);
static int string_int_concat(GscRuntime *,const Value *,const Value *,int,Value *);
static int vehicle_node_value_valid(const GscRuntime*,const Value*);
static int vehicle_node_value_live(GscRuntime*,const Value*,int,int*);
static int vehicle_node_field_read(GscRuntime*,const Value*,const char*,int,Value*);
static int vehicle_node_compare(GscRuntime*,int,const Value*,const Value*,int,Value*);
static int vehicle_node_ref_equal(const GscVehicleNodeRef*,const GscVehicleNodeRef*);
static int vehicle_node_field_builtin(const char*);
static int node_query_capability(const GscRuntime*r,int kind){return kind==GSC_VEHICLE_QUERY_ONE||kind==GSC_VEHICLE_QUERY_ARRAY?r->vehicle_nodes_enabled:r->node_queries_enabled;}
static int node_value_valid(const GscRuntime *,const Value *);
static int node_value_live(GscRuntime *,const Value *,int,int *);
static int node_field_read(GscRuntime *,const Value *,const char *,int,Value *);
static int node_compare(GscRuntime *,int,const Value *,const Value *,int,Value *);
static int hud_receiver(GscRuntime *,const Value *,int);
static int hud_value_valid(const GscRuntime *,const Value *);
static int hud_value_live(GscRuntime *,const Value *,int,int *);
static int hud_field_read(GscRuntime *,const Value *,const char *,int,Value *);
static int hud_field_write(GscRuntime *,const Value *,const char *,const Value *,int);
static int hud_compare(GscRuntime *,int,const Value *,const Value *,int,Value *);
#include "gsc_vm_scalar.inc"
#include "gsc_vm_mixed_scalar_equality_r239.inc"
#include "gsc_vm_float_arithmetic.inc"
#include "gsc_vm_vector_arithmetic.inc"
#include "gsc_vm_vector_math.inc"
#include "gsc_vm_integer_bitwise.inc"
int gsc_runtime_select_timed_animation_arithmetic(GscRuntime *r) {
    if(!r||r->started||r->advancing||r->executing_task||r->task_count||r->error[0]||r->timed_arithmetic_enabled||
       !r->program->timed_arithmetic_selected||!r->float_arithmetic_enabled||r->entity_prepare_busy||r->entity_certificate_busy||
       r->entity_sequence_busy||r->entity_inheritance_busy||r->entity_field_busy||r->entity_method_busy)return 0;
    r->timed_arithmetic_enabled=1;return 1;
}
int gsc_runtime_select_frame_animation_cadence(GscRuntime *r) {
    if(!r||r->started||r->advancing||r->executing_task||r->task_count||r->error[0]||r->frame_cadence_enabled||r->r237_frame_buckets_enabled||
       !r->program->frame_cadence_selected||!r->timed_arithmetic_enabled||r->entity_prepare_busy||r->entity_certificate_busy||
       r->entity_sequence_busy||r->entity_inheritance_busy||r->entity_field_busy||r->entity_method_busy)return 0;
    r->frame_cadence_enabled=1;return 1;
}
int gsc_runtime_frame_animation_cadence_selected(const GscRuntime *r) {
    return r&&r->program->frame_cadence_selected&&r->frame_cadence_enabled&&!r->error[0];
}

#include "gsc_vm_function_refs.inc"
#include "gsc_vm_struct.inc"
#include "gsc_string_cast.inc"
#include "gsc_vm_string_int_concat.inc"
#include "gsc_vm_string_arguments.inc"
#include "gsc_vm_native_bridge.inc"
#include "gsc_vm_r232_developer.inc"
#include "gsc_vm_state_arena.inc"
#include "gsc_vm_state.inc"
static int eval(GscRuntime *,Frame *,int,int,Value *,int);
static int assignment(GscRuntime *,Frame *,const char *,Value,int);
#include "gsc_vm_arrays.inc"
#include "gsc_vm_game.inc"
#include "gsc_vm_anim_owner.inc"
#include "gsc_vm_game_owner.inc"
#include "gsc_animation_catalog.inc"
#include "gsc_vm_animation_refs.inc"
#include "gsc_vm_animation_namespace.inc"
#include "gsc_vm_predicates.inc"
#include "gsc_vm_actor_presence.inc"

static int builtin_values(GscRuntime *r,const Builtin *b,const Value *args,
                          int argc,int line) {
    int n;
    if(argc!=b->arity)return rfail(r,line,"invalid builtin arity for %s",b->name);
    for(n=0;n<argc;n++) {
        if(args[n].kind<V_UNDEFINED||
           !((r->arrays_enabled&&args[n].kind==V_ARRAY&&!strcmp(b->name,"isdefined"))||
             (r->animation_refs_enabled&&args[n].kind==V_ANIMATION&&!strcmp(b->name,"isdefined")&&animation_value_valid(r,&args[n]))||
             (animation_namespace_kind(args[n].kind)&&!strcmp(b->name,"isdefined")&&animation_namespace_value_valid(r,&args[n]))||
             (r->structs_enabled&&args[n].kind==V_STRUCT&&!strcmp(b->name,"isdefined")&&struct_value_valid(r,&args[n]))||
             (r->function_refs_enabled&&args[n].kind==V_FUNCTION&&!strcmp(b->name,"isdefined")&&function_value_valid(r,&args[n]))||
             (args[n].kind<=V_ENTITY&&(b->accepted[n]&B_VALUE(args[n].kind)))))
            return rfail(r,line,"%s argument %d has unsupported value type",b->name,n+1);
    }
    return 1;
}
#include "gsc_vm_music.inc"
#include "gsc_vm_resources.inc"
#include "gsc_vm_objectives.inc"
#include "gsc_vm_console.inc"
#include "gsc_vm_hud.inc"
#include "gsc_vm_mission.inc"
#include "gsc_vm_blast.inc"
#include "gsc_vm_quake.inc"
static int loop_fx_call(GscRuntime *,const Value *,int,int,Value *);
#include "gsc_vm_fx.inc"
#include "gsc_vm_cvars.inc"
#include "gsc_vm_shellshock.inc"
#include "gsc_vm_debug_text.inc"
#include "gsc_vm_debug_line.inc"
#include "gsc_vm_ambient.inc"
#include "gsc_vm_precache_item.inc"
#include "gsc_vm_save_game.inc"
#include "gsc_vm_tag_fx.inc"
#include "gsc_native_query_payload.inc"
#include "gsc_precache_string_payload.inc"
#include "gsc_vm_configured_precache.inc"
#include "gsc_vm_precache_string.inc"
#include "gsc_vm_native_queries.inc"
#include "gsc_vm_weapon_model.inc"
#include "gsc_vm_weapon_classname.inc"
#include "gsc_vm_vehicle_spawn.inc"
#include "gsc_vm_turret_precache.inc"
#include "gsc_vm_turret_spawn.inc"
#include "gsc_vm_bullet_trace.inc"
#include "gsc_vm_model_part.inc"
#include "gsc_vm_animation_length.inc"
#include "gsc_vm_start_origin.inc"
#include "gsc_vm_namespace_start_origin.inc"
#include "move-delta-vm.inc"
#include "stance-vm.inc"
#include "actor-float-vm.inc"
#include "animtime-vm.inc"
#include "gsc_vm_spawner_arrays.inc"
#include "gsc_randomint_payload.inc"
#include "gsc_vm_randomint.inc"
#include "gsc_vm_randomfloat.inc"
#include "gsc_vm_keybinding.inc"
#include "gsc_vm_entity_number.inc"
#include "gsc_vm_spawner_team_arrays.inc"
#include "gsc_vm_entity_queries.inc"
#include "gsc_vm_vehicle_nodes.inc"
#include "gsc_vm_node_queries.inc"
#include "gsc_vm_actor_goal.inc"
#include "gsc_vm_r232_turrets.inc"
#include "gsc_vm_r232_turret_animations.inc"
#include "gsc_vm_r232_actor_animations.inc"
#include "gsc_vm_r232_actor_look.inc"
#include "gsc_vm_r233_actor_drop.inc"
#include "gsc_vm_r243_scripted_prefix.inc"
#include "gsc_vm_r243_scripted_start.inc"
#include "gsc_vm_r232_random_range.inc"
#include "gsc_vm_r232_source_float.inc"
#include "gsc_vm_ai_arrays.inc"
#include "gsc_vm_hud_elements.inc"
#include "gsc_vm_entity_collision.inc"

static int invoke_builtin_statement(GscRuntime *r,int id,const Value *args,
                                    int argc,int line) {
    const Builtin *b;
    if(id==BULLET_TRACE_STATEMENT_TARGET){Value ignored;return bullet_trace_call(r,args,argc,line,&ignored);}
    if(id==VEHICLE_SPAWN_STATEMENT_TARGET){Value ignored;return vehicle_spawn_call(r,args,argc,line,&ignored);}
    if(id==TURRET_PRECACHE_STATEMENT_TARGET){Value ignored;return turret_precache_call(r,args,argc,line,&ignored);}
    if(id==TURRET_SPAWN_STATEMENT_TARGET){Value ignored;return turret_spawn_call(r,args,argc,line,&ignored);}
    if(id==R232_RANDOM_RANGE_STATEMENT_TARGET){Value ignored;return random_range_call(r,args,argc,line,&ignored);}
    if(id==R243_MOVE_DELTA_STATEMENT_TARGET){Value ignored;return move_delta_call(r,args,argc,line,&ignored);}
    if(id==START_ORIGIN_STATEMENT_TARGET||id==START_ANGLES_STATEMENT_TARGET){Value ignored;return start_origin_selected_call(r,args,argc,line,id==START_ANGLES_STATEMENT_TARGET?2:1,&ignored);}
    if(id==AI_ARRAY_STATEMENT_TARGET){Value ignored;return ai_array_call(r,args,argc,line,&ignored);}
    if(id==HUDELEM_TARGET_BASE+GSC_HUDELEM_CREATE) {Value ignored;return hud_create(r,argc,line,&ignored);}
    if(id>NODE_QUERY_TARGET_BASE) {Value ignored;return node_query_call(r,id-NODE_QUERY_TARGET_BASE,args,argc,line,&ignored);}
    if(id>VECTOR_MATH_TARGET_BASE) {Value ignored;return vector_math_call(r,id-VECTOR_MATH_TARGET_BASE,args,argc,line,&ignored);}
    if(id==ANIMATION_LENGTH_STATEMENT_TARGET) {Value ignored;return animation_length_call(r,args,argc,float_arithmetic_active(r),line,&ignored);}
    if(id==MODEL_PART_STATEMENT_TARGET){Value ignored;return model_part_call(r,args,argc,line,&ignored);}
    if(id==WEAPON_CLASSNAME_STATEMENT_TARGET){Value ignored;return weapon_classname_call(r,args,argc,line,&ignored);}
    if(id==WEAPON_MODEL_STATEMENT_TARGET) {Value ignored;return weapon_model_call(r,args,argc,line,&ignored);}
    if(id>ENTITY_QUERY_TARGET_BASE) {Value ignored;return entity_query_call(r,id-ENTITY_QUERY_TARGET_BASE,args,argc,line,&ignored);}
    if(id>STRUCT_TARGET_BASE) {Value ignored;return struct_call(r,args,argc,line,&ignored);}
    if(id>KEYBINDING_TARGET_BASE) {Value ignored;return keybinding_call(r,id-KEYBINDING_TARGET_BASE,args,argc,line,&ignored);}
    if(id>RANDOMFLOAT_TARGET_BASE) {Value ignored;return randomfloat_call_selected(r,id-RANDOMFLOAT_TARGET_BASE,args,argc,float_arithmetic_active(r),line,&ignored);}
    if(id>RANDOMINT_TARGET_BASE) {Value ignored;return randomint_call(r,id-RANDOMINT_TARGET_BASE,args,argc,line,&ignored);}
    if(id>SPAWNER_ARRAY_TARGET_BASE) {Value ignored;return spawner_array_call(r,id-SPAWNER_ARRAY_TARGET_BASE,args,argc,line,&ignored);}
    if(id>NATIVE_QUERY_TARGET_BASE) {Value ignored;return native_query_call(r,id-NATIVE_QUERY_TARGET_BASE,args,argc,line,&ignored);}
    if(id>TAG_FX_TARGET_BASE) {Value ignored;return tag_fx_call(r,id-TAG_FX_TARGET_BASE,args,argc,line,&ignored);}
    if(id>SAVE_GAME_TARGET_BASE) {Value ignored;return save_game_call(r,id-SAVE_GAME_TARGET_BASE,args,argc,line,&ignored);}
    if(id>PRECACHE_ITEM_TARGET_BASE) {Value ignored;return precache_item_call(r,id-PRECACHE_ITEM_TARGET_BASE,args,argc,line,&ignored);}
    if(id>AMBIENT_TARGET_BASE) {Value ignored;return ambient_call(r,id-AMBIENT_TARGET_BASE,args,argc,line,&ignored);}
    if(id>DEBUG_LINE_TARGET_BASE) {Value ignored;return debug_line_call(r,id-DEBUG_LINE_TARGET_BASE,args,argc,line,&ignored);}
    if(id>DEBUG_TEXT_TARGET_BASE) {Value ignored;return debug_text_call(r,id-DEBUG_TEXT_TARGET_BASE,args,argc,line,&ignored);}
    if(id>SHELLSHOCK_TARGET_BASE) {Value ignored;return shellshock_call(r,id-SHELLSHOCK_TARGET_BASE,args,argc,line,&ignored);}
    if(id>CVAR_TARGET_BASE) {Value ignored;return cvar_call(r,id-CVAR_TARGET_BASE,args,argc,line,&ignored);}
    if(id>FX_TARGET_BASE) {Value ignored;return fx_call(r,id-FX_TARGET_BASE,args,argc,line,&ignored);}
    if(id>QUAKE_TARGET_BASE)return quake_statement(r,id-QUAKE_TARGET_BASE,args,argc,line);
    if(id>BLAST_TARGET_BASE)return blast_statement(r,id-BLAST_TARGET_BASE,args,argc,line);
    if(id>MISSION_TARGET_BASE)return mission_statement(r,id-MISSION_TARGET_BASE,args,argc,line);
    if(id>HUD_TARGET_BASE)return hud_statement(r,id-HUD_TARGET_BASE,NULL,args,argc,line);
    if(id>CONSOLE_TARGET_BASE)return console_statement(r,id-CONSOLE_TARGET_BASE,args,argc,line);
    if(id>OBJECTIVE_TARGET_BASE)return objective_statement(r,id-OBJECTIVE_TARGET_BASE,args,argc,line);
    if(id>RESOURCE_TARGET_BASE)return resource_statement(r,id-RESOURCE_TARGET_BASE,args,argc,line);
    if(id>=BUILTIN_COUNT)return music_statement(r,id-BUILTIN_COUNT,args,argc,line);
    if(id<0)return rfail(r,line,"invalid builtin target");
    b=&builtins[id];
    if(b->context!=B_STATEMENT||!b->statement||b->result_kind!=B_RET_VOID||
       !builtin_values(r,b,args,argc,line))return 0;
    return b->statement(r,b,args,line);
}
static int invoke_builtin_expression(GscRuntime *r,int id,const Value *args,
                                     int argc,int line,Value *out) {
    const Builtin *b;
    if(id<0||id>=BUILTIN_COUNT)return rfail(r,line,"invalid builtin target");
    b=&builtins[id];
    /* Localized text is a real defined VM value. Preserve its distinct type;
     * the frozen scalar native bridge does not carry this value category. */
    if(argc==1&&args&&args[0].kind==V_LOCALIZED_STRING&&b->expression==builtin_isdefined) {
        if(!r->objectives_enabled||!memchr(args[0].str,0,sizeof(args[0].str)))return rfail(r,line,"invalid localized text value");
        scalar_integer(out,1);return 1;
    }
    if(argc==1&&args&&args[0].kind==V_HUDELEM&&b->expression==builtin_isdefined) {
        int alive;if(!hud_value_live(r,&args[0],line,&alive))return 0;scalar_integer(out,alive);return 1;
    }
    if(argc==1&&args&&args[0].kind==V_VEHICLE_NODE&&b->expression==builtin_isdefined){int alive;if(!vehicle_node_value_live(r,args,line,&alive))return 0;scalar_integer(out,alive);return 1;}
    if(argc==1&&args&&args[0].kind==V_NODE&&b->expression==builtin_isdefined) {
        int alive;if(!node_value_live(r,&args[0],line,&alive))return 0;scalar_integer(out,alive);return 1;
    }
    if(r->state_enabled&&argc==1&&args[0].kind==V_LEVEL&&!strcmp(b->name,"isdefined"))
        return rfail(r,line,"isdefined of whole level object is unsupported");
    if(b->context!=B_EXPRESSION||!b->expression||
       !builtin_values(r,b,args,argc,line))return 0;
    if(argc==1&&animation_namespace_kind(args[0].kind)&&!strcmp(b->name,"isdefined")){if(!animation_namespace_value_valid(r,&args[0]))return rfail(r,line,"invalid animation metadata value");if(!anim_data_current(r,r->executing_task))return 0;scalar_integer(out,1);return 1;}
    if(argc==1&&args[0].kind==V_ANIMATION&&!strcmp(b->name,"isdefined")){if(!anim_data_current(r,r->executing_task))return 0;scalar_integer(out,1);return 1;}
    if(r->function_refs_enabled&&argc==1&&args[0].kind==V_FUNCTION&&!strcmp(b->name,"isdefined")) {
        /* Valid script references are defined; no invented original tag or
         * serialization through the frozen native host ABI. This pure VM path
         * does not claim a native-registry callback statistic. */
        memset(out,0,sizeof(*out));out->kind=V_NUMBER;out->integer=1;out->number=1;return 1;
    }
    if(!b->expression(r,b,args,line,out))return 0;
    if(out->kind!=b->result_kind)return rfail(r,line,"invalid builtin result type");
    return 1;
}
static int builtin_isdefined(GscRuntime *r,const Builtin *b,const Value *args,
                             int line,Value *out) {
    return bridge_isdefined(r,b,args,line,out);
}
static int truth(Value v) {
    return v.kind==V_STRING ? v.str[0]!=0 : v.number!=0.0f;
}
static int eval(GscRuntime *,Frame *,int,int,Value *,int);
static int eval_inner(GscRuntime *r,Frame *f,int index,int depth,Value *out,int line,int *localOnly) {
    const Expr *e;Value a,b,args[MAX_PARAMS];int k;
    if(localOnly)*localOnly=0;
    if(index<0||index>=r->program->expr_count||depth>48)
        return rfail(r,line,"expression nesting limit");
    e=&r->program->expr[index];memset(out,0,sizeof(*out));
    if(e->kind==E_NATIVE_CALL||e->kind==E_ISDEFINED) {
        if(e->call_binding==GSC_EXPR_TARGET_LOCAL_SCRIPT)
            return rfail(r,line,"unsupported local script expression call runtime semantics");
        if(e->call_binding==GSC_EXPR_TARGET_BULLET_TRACE_NATIVE){if(!r->bullet_trace_enabled||e->module[0]||e->kind!=E_NATIVE_CALL||e->target!=1||find_func(r->program,e->owner_module,e->name)>=0||!bullet_trace_descriptor(r->program,e->name)||bullet_trace_issues(r->program,e->args,e->argc))return rfail(r,line,"invalid bulletTrace binding");
        } else if(e->call_binding==GSC_EXPR_TARGET_VEHICLE_SPAWN_NATIVE){if(!r->vehicle_spawn_enabled||e->module[0]||e->kind!=E_NATIVE_CALL||e->target!=1||find_func(r->program,e->owner_module,e->name)>=0||!vehicle_spawn_descriptor(r->program,e->name)||vehicle_spawn_issues(r->program,e->args,e->argc))return rfail(r,line,"invalid spawnVehicle binding");
        } else if(e->call_binding==GSC_EXPR_TARGET_TURRET_PRECACHE_NATIVE){if(!r->turret_precache_enabled||e->module[0]||e->kind!=E_NATIVE_CALL||e->target!=1||find_func(r->program,e->owner_module,e->name)>=0||!turret_precache_descriptor(r->program,e->name)||turret_precache_issues(r->program,e->args,e->argc))return rfail(r,line,"invalid precacheTurret binding");
        } else if(e->call_binding==GSC_EXPR_TARGET_TURRET_SPAWN_NATIVE){if(!r->turret_spawn_enabled||e->module[0]||e->kind!=E_NATIVE_CALL||e->target!=1||find_func(r->program,e->owner_module,e->name)>=0||!turret_spawn_descriptor(r->program,e->name)||turret_spawn_issues(r->program,e->args,e->argc))return rfail(r,line,"invalid spawnTurret binding");
        } else if(e->call_binding==GSC_EXPR_TARGET_HUDELEM_NATIVE) {
            if(!r->hud_elements_enabled||e->module[0]||e->kind!=E_NATIVE_CALL||e->argc||
               find_func(r->program,e->owner_module,e->name)>=0||!hud_elem_global(r->program,e->name))
                return rfail(r,line,"invalid selected HUD element expression binding");
        } else if(e->call_binding==GSC_EXPR_TARGET_R232_RANDOM_RANGE){if(!r->random_range_enabled||e->module[0]||e->kind!=E_NATIVE_CALL||e->target!=1||find_func(r->program,e->owner_module,e->name)>=0||!random_range_descriptor(r->program,e->name)||random_range_issues(r->program,e->args,e->argc))return rfail(r,line,"invalid selected randomFloatRange binding");
        } else if(e->call_binding==R243_EXPR_TARGET_MOVE_DELTA){if(!r->move_delta_enabled||e->module[0]||e->kind!=E_NATIVE_CALL||e->target!=1||find_func(r->program,e->owner_module,e->name)>=0||!move_delta_descriptor(r->program,e->name)||move_delta_static_issues(r->program,e->args,e->argc))return rfail(r,line,"invalid selected GetMoveDelta binding");
        } else if(e->call_binding==GSC_EXPR_TARGET_START_ORIGIN_NATIVE) {
            if((!r->start_origin_enabled&&!r->namespace_start_origin_enabled)||e->module[0]||e->kind!=E_NATIVE_CALL||e->target!=start_origin_descriptor(r->program,e->name)||
               find_func(r->program,e->owner_module,e->name)>=0||!start_origin_descriptor(r->program,e->name)||
               start_origin_static_issues(r->program,e->args,e->argc))return rfail(r,line,"invalid start-origin expression binding");
        } else if(e->call_binding==GSC_EXPR_TARGET_AI_ARRAY_NATIVE) {
            if(!r->ai_arrays_enabled||e->module[0]||e->kind!=E_NATIVE_CALL||e->target!=1||
               find_func(r->program,e->owner_module,e->name)>=0||!ai_array_descriptor(r->program,e->name)||
               ai_array_static_issues(r->program,e->args,e->argc))return rfail(r,line,"invalid AI array expression binding");
        } else if(e->call_binding==GSC_EXPR_TARGET_NODE_QUERY_NATIVE) {
            if(!node_query_capability(r,e->target)||e->module[0]||e->kind!=E_NATIVE_CALL||
               find_func(r->program,e->owner_module,e->name)>=0||
               node_query_descriptor(r->program,e->name)!=e->target||
               node_query_static_issues(r->program,e->target,e->args,e->argc))
                return rfail(r,line,"invalid selected node query expression binding");
        } else if(e->call_binding==GSC_EXPR_TARGET_VECTOR_MATH_NATIVE) {
            if(!r->vector_math_enabled||e->module[0]||
               vector_math_descriptor(r->program,e->name)!=e->target||
               vector_math_static_issues(r->program,e->target,e->args,e->argc))
                return rfail(r,line,"invalid selected vector math expression binding");
        } else if(e->call_binding==GSC_EXPR_TARGET_ENTITY_NATIVE) {
            if(!entity_global_expression_binding(r,e))
                return rfail(r,line,"invalid entity expression binding");
        } else if(e->call_binding==GSC_EXPR_TARGET_ANIMATION_LENGTH_NATIVE) {
            if(!r->animation_length_enabled||e->module[0]||!animation_length_descriptor(r->program,e->name))return rfail(r,line,"invalid animation length binding");
        } else if(e->call_binding==GSC_EXPR_TARGET_MODEL_PART_NATIVE){
            if(!r->model_part_enabled||e->module[0]||!model_part_descriptor(r->program,e->name))return rfail(r,line,"invalid weapon classname binding");
        } else if(e->call_binding==GSC_EXPR_TARGET_WEAPON_CLASSNAME_NATIVE){
            if(!r->weapon_classname_enabled||e->module[0]||!weapon_classname_descriptor(r->program,e->name))return rfail(r,line,"invalid weapon classname binding");
        } else if(e->call_binding==GSC_EXPR_TARGET_WEAPON_MODEL_NATIVE) {
            if(!r->weapon_model_enabled||e->module[0]||!weapon_model_descriptor(r->program,e->name))return rfail(r,line,"invalid weapon model binding");
        } else if(e->call_binding==GSC_EXPR_TARGET_ENTITY_QUERY_NATIVE) {
            if(!r->entity_queries_enabled||e->kind!=E_NATIVE_CALL||e->module[0]||
               find_func(r->program,e->owner_module,e->name)>=0||
               entity_query_descriptor(r->program,e->name)!=e->target||
               entity_query_static_issues(r->program,e->target,e->args,e->argc))
                return rfail(r,line,"invalid selected entity query expression binding");
        } else if(e->call_binding==GSC_EXPR_TARGET_STRUCT_NATIVE) {
            if(!r->structs_enabled||e->kind!=E_NATIVE_CALL||e->module[0]||
               struct_descriptor(r->program,e->name)!=e->target||struct_static_issues(e->target,e->argc))
                return rfail(r,line,"invalid structure expression binding");
        } else if(e->call_binding!=GSC_EXPR_TARGET_BUILTIN||e->target<0||
           e->target>=BUILTIN_COUNT||builtins[e->target].context!=B_EXPRESSION)
            return rfail(r,line,"invalid expression call binding");
    }
    if(e->kind==E_THREAD_CALL)
        return rfail(r,line,"unsupported thread expression runtime semantics");
    if(e->kind==E_BINARY&&(e->op=='%'||e->op=='&')&&!r->scalar_switch_enabled)
        return rfail(r,line,"unsupported modulo/bitwise-and runtime semantics");
    if(e->kind==E_FUNC_REF)return function_reference(r,e,line,out);
    if(e->kind==E_SCOPED_CALL)
        return rfail(r,line,"unsupported qualified expression call runtime semantics");
    if(e->kind==E_CAST) {
        if(!r->scalar_switch_enabled)return rfail(r,line,"unsupported cast runtime semantics");
        if(!eval(r,f,e->a,depth+1,&a,line))return 0;
        if(e->float_arithmetic==2)return r232_source_float_cast(r,e->op,&a,line,out);
        if(e->float_arithmetic==1){if(!r->timed_arithmetic_enabled)return rfail(r,line,"selected module numeric casts are unsupported");return float_arithmetic_cast(r,e->op,&a,line,out);}
        return scalar_cast(r,e->op,&a,line,out);
    }
    if(e->kind==E_ANIMTREE_VALUE&&r->program->animation_namespace_selected)return animation_namespace_reference_value(r,e,index,line,out);
    if(e->kind==E_ANIMTREE_VALUE)
        return rfail(r,line,"unsupported #animtree value runtime semantics");
    if(e->kind==E_DYNAMIC_CALL)
        return rfail(r,line,"unsupported computed call expression runtime semantics");
    if(e->kind==E_EMPTY_ARRAY) {
        if(!r->arrays_enabled)return rfail(r,line,"unsupported empty array runtime semantics");
        return array_new(r,line,out);
    }
    if(e->kind==E_ANIM_REF)return r->program->animation_namespace_selected?animation_namespace_reference_value(r,e,index,line,out):animation_reference_value(r,e,index,line,out);
    if(e->kind==E_VECTOR) {
        if(!r->entities_enabled||e->argc!=3)
            return rfail(r,line,"unsupported vector runtime semantics");
        for(k=0;k<3;k++) {
            if(!eval(r,f,e->args[k],depth+1,&a,line))return 0;
            if(a.kind!=V_NUMBER||!isfinite(a.number))
                return rfail(r,line,"vector component must be a finite number");
            out->vector[k]=a.number;
        }
        out->kind=V_VECTOR;return 1;
    }
    if(e->kind==E_RAW_STRING||e->kind==E_RAW_LOCALIZED_STRING)
        return rfail(r,line,"unsupported unresolved string escape runtime semantics");
    if(e->kind==E_LOCALIZED_STRING) {
        if(!r->objectives_enabled)return rfail(r,line,"unsupported localized string runtime semantics");
        out->kind=V_LOCALIZED_STRING;str_copy(out->str,sizeof(out->str),e->name);if(localOnly)*localOnly=1;return 1;
    }
    if(e->kind==E_FIELD) {
        if(!r->access_enabled||e->a<0||e->a>=index)
            return rfail(r,line,"unsupported entity property read");
        if(!eval(r,f,e->a,depth+1,&a,line))return 0;
        if(r->arrays_enabled&&(a.kind==V_ARRAY||string_size_field(&a,e->name)))return array_field_read(r,&a,e->name,line,out);
        if(r->state_enabled)return state_field_read(r,&a,e->name,line,out);
        return bridge_property_read(r,&a,entity_property_descriptor(e->name),line,out);
    }
    if(e->kind==E_INDEX&&r->arrays_enabled) {
        if(!eval(r,f,e->a,depth+1,&a,line)||!eval(r,f,e->b,depth+1,&b,line))return 0;
        return array_index_read(r,&a,&b,line,out);
    }
    if(e->kind==E_METHOD_CALL&&e->receiver_binding_kind==GSC_RECEIVER_TARGET_ENTITY_NATIVE&&R243ActorFloatMask(e->receiver_native_target)){
        if(!r->actor_float_enabled||e->module[0]||e->argc!=0||entity_method_descriptor(r->program,e->name)!=e->receiver_native_target||e->receiver_script_target!=-1||e->receiver_event_target!=-1)return rfail(r,line,"invalid ordinary actor float query binding");
        if(!eval(r,f,e->a,depth+1,&a,line))return 0;return r243_actor_float_call(r,&a,e->receiver_native_target,0,line,out);
    }
    if(e->kind==E_METHOD_CALL&&e->receiver_binding_kind==GSC_RECEIVER_TARGET_ENTITY_NATIVE&&e->receiver_native_target==R243_STANCE_METHOD){
        if(!r->stance_enabled||e->module[0]||e->argc!=1||entity_method_descriptor(r->program,e->name)!=R243_STANCE_METHOD||e->receiver_script_target!=-1||e->receiver_event_target!=-1)return rfail(r,line,"invalid ordinary isStanceAllowed binding");
        if(!eval(r,f,e->a,depth+1,&a,line)||!eval(r,f,e->args[0],depth+1,args,line))return 0;return r243_stance_call(r,&a,args,1,line,out);
    }
    if(e->kind==E_METHOD_CALL&&e->receiver_binding_kind==GSC_RECEIVER_TARGET_ENTITY_NATIVE&&e->receiver_native_target==R243_ANIMTIME_METHOD){
        if(!r->animtime_enabled||e->module[0]||e->argc!=1||entity_method_descriptor(r->program,e->name)!=R243_ANIMTIME_METHOD||e->receiver_script_target!=-1||e->receiver_event_target!=-1)return rfail(r,line,"invalid ordinary getAnimTime binding");
        if(!eval(r,f,e->a,depth+1,&a,line)||!eval(r,f,e->args[0],depth+1,args,line))return 0;return r243_animtime_call(r,&a,args,1,line,out);
    }
    if(e->kind==E_INDEX||e->kind==E_METHOD_CALL)
        return rfail(r,line,"unsupported game field/index/receiver expression");
    if(e->kind==E_NATIVE_CALL) {
        if(e->argc<0||e->argc>MAX_PARAMS)return rfail(r,line,"invalid expression arity");
        for(k=0;k<e->argc;k++) {
            if(!eval(r,f,e->args[k],depth+1,&args[k],line))return 0;
        }
        if(e->call_binding==GSC_EXPR_TARGET_BULLET_TRACE_NATIVE)return bullet_trace_call(r,args,e->argc,line,out);
        if(e->call_binding==GSC_EXPR_TARGET_VEHICLE_SPAWN_NATIVE)return vehicle_spawn_call(r,args,e->argc,line,out);
        if(e->call_binding==GSC_EXPR_TARGET_TURRET_PRECACHE_NATIVE)return turret_precache_call(r,args,e->argc,line,out);
        if(e->call_binding==GSC_EXPR_TARGET_TURRET_SPAWN_NATIVE)return turret_spawn_call(r,args,e->argc,line,out);
        if(e->call_binding==GSC_EXPR_TARGET_HUDELEM_NATIVE)return hud_create(r,e->argc,line,out);
        if(e->call_binding==GSC_EXPR_TARGET_R232_RANDOM_RANGE)return random_range_call(r,args,e->argc,line,out);
        if(e->call_binding==R243_EXPR_TARGET_MOVE_DELTA)return move_delta_call(r,args,e->argc,line,out);
        if(e->call_binding==GSC_EXPR_TARGET_START_ORIGIN_NATIVE)return start_origin_selected_call(r,args,e->argc,line,e->target,out);
        if(e->call_binding==GSC_EXPR_TARGET_AI_ARRAY_NATIVE)return ai_array_call(r,args,e->argc,line,out);
        if(e->call_binding==GSC_EXPR_TARGET_NODE_QUERY_NATIVE)return node_query_call(r,e->target,args,e->argc,line,out);
        if(e->call_binding==GSC_EXPR_TARGET_VECTOR_MATH_NATIVE)return vector_math_call(r,e->target,args,e->argc,line,out);
        if(e->call_binding==GSC_EXPR_TARGET_ANIMATION_LENGTH_NATIVE)return animation_length_call(r,args,e->argc,e->float_arithmetic,line,out);
        if(e->call_binding==GSC_EXPR_TARGET_MODEL_PART_NATIVE)return model_part_call(r,args,e->argc,line,out);
        if(e->call_binding==GSC_EXPR_TARGET_WEAPON_CLASSNAME_NATIVE)return weapon_classname_call(r,args,e->argc,line,out);
        if(e->call_binding==GSC_EXPR_TARGET_WEAPON_MODEL_NATIVE)return weapon_model_call(r,args,e->argc,line,out);
        if(e->call_binding==GSC_EXPR_TARGET_ENTITY_QUERY_NATIVE)return entity_query_call(r,e->target,args,e->argc,line,out);
        if(e->call_binding==GSC_EXPR_TARGET_STRUCT_NATIVE)return struct_call(r,args,e->argc,line,out);
        if(e->call_binding==GSC_EXPR_TARGET_ENTITY_NATIVE)
            return e->target==ENTITY_GLOBAL_SPAWN?bridge_spawn(r,args,e->argc,
                e->argc==3?e->args[2]:-1,line,out):
                e->target==ENTITY_GLOBAL_GETENTARRAY?array_getentarray(r,args,e->argc,line,out):
                e->target==ENTITY_GLOBAL_ISAI?actor_presence_expression(r,args,e->argc,line,out):
                e->target==ENTITY_GLOBAL_ISALIVE||e->target==ENTITY_GLOBAL_ISSENTIENT?
                    predicate_expression(r,e->target,args,e->argc,line,out):
                bridge_getent(r,args,e->argc,line,out);
        return invoke_builtin_expression(r,e->target,args,e->argc,line,out);
    }
    if(e->kind==E_NUMBER) {
        if(r->scalar_switch_enabled&&e->integer){scalar_integer(out,e->integer_value);if(localOnly)*localOnly=1;return 1;}
        out->kind=V_NUMBER;out->integer=e->integer;out->number=e->number;if(localOnly)*localOnly=1;return 1;
    }
    if(e->kind==E_STRING) {out->kind=V_STRING;str_copy(out->str,sizeof(out->str),e->name);if(localOnly)*localOnly=1;return 1;}
    if(e->kind==E_VAR) {
        if(r->flow_enabled&&literal_name(e->name)) {
            if(!strcmp(e->name,"undefined")){out->kind=V_UNDEFINED;if(localOnly)*localOnly=1;return 1;}
            out->kind=V_NUMBER;out->integer=1;
            out->number=(float)!strcmp(e->name,"true");if(localOnly)*localOnly=1;return 1;
        }
        if(r->state_enabled&&!strcmp(e->name,"self")){*out=f->self;if(localOnly)*localOnly=1;return 1;}
        if(r->state_enabled&&!strcmp(e->name,"level")){out->kind=V_LEVEL;if(localOnly)*localOnly=1;return 1;}
        if(r->anim_data_enabled&&!strcmp(e->name,"anim"))return anim_data_read(r,line,out);
        if(game_array_selected(r,e->name))return game_array_read(r,line,out);
        for(k=f->local_count-1;k>=0;k--)if(!strcmp(f->locals[k].name,e->name)){
            *out=f->locals[k].value;if(localOnly)*localOnly=1;return 1;
        }
        /* Missing local lookup creates an undefined (type 0) value in the
         * candidate VM. This is observable through isdefined. */
        out->kind=V_UNDEFINED;if(localOnly)*localOnly=1;return 1;
    }
    if(!eval(r,f,e->a,depth+1,&a,line))return 0;
    if(e->kind==E_ISDEFINED) {
        return invoke_builtin_expression(r,e->target,&a,1,line,out);
    }
    if(e->kind==E_UNARY) {
        if(a.kind==V_UNDEFINED)return rfail(r,line,"undefined value in unary expression");
        if(e->op=='~')return integer_bitwise_unary(r,&a,line,out);
        if(e->op=='!') {
            if(a.kind>V_STRING)return rfail(r,line,"vector/entity boolean semantics are unsupported");
            out->kind=V_NUMBER;out->integer=1;out->number=(float)!truth(a);return 1;
        }
        if(r->vector_arithmetic_enabled&&a.kind==V_VECTOR)
            return vector_arithmetic_unary(r,e->op,&a,line,out);
        if(a.kind!=V_NUMBER)return rfail(r,line,"numeric unary operator requires number");
        if(r->scalar_switch_enabled)return scalar_unary(r,&a,line,out);
        *out=a;out->number=-a.number;return 1;
    }
    if(a.kind==V_UNDEFINED)return rfail(r,line,"undefined value in expression");
    if((e->op==T_AND||e->op==T_OR)&&a.kind>V_STRING)
        return rfail(r,line,"vector/entity boolean semantics are unsupported");
    if(e->float_arithmetic&&(e->op==T_AND||e->op==T_OR)&&a.kind==V_NUMBER&&!a.integer)return rfail(r,line,"selected module float boolean operation is unsupported");
    /* Fixture convention: condition operators skip an unneeded right side. */
    if(e->op==T_AND&&!truth(a)) {
        out->kind=V_NUMBER;out->integer=1;out->number=0;return 1;
    }
    if(e->op==T_OR&&truth(a)) {
        out->kind=V_NUMBER;out->integer=1;out->number=1;return 1;
    }
    if(!eval(r,f,e->b,depth+1,&b,line))return 0;
    if(animation_namespace_kind(a.kind)||animation_namespace_kind(b.kind))return animation_namespace_compare(r,e->op,&a,&b,line,out);
    if(a.kind==V_ANIMATION||b.kind==V_ANIMATION)return animation_compare(r,e->op,&a,&b,line,out);
    if(a.kind==V_VEHICLE_NODE||b.kind==V_VEHICLE_NODE)return vehicle_node_compare(r,e->op,&a,&b,line,out);
    if(a.kind==V_NODE||b.kind==V_NODE)return node_compare(r,e->op,&a,&b,line,out);
    if(a.kind==V_HUDELEM||b.kind==V_HUDELEM)return hud_compare(r,e->op,&a,&b,line,out);
    if(b.kind==V_UNDEFINED)return rfail(r,line,"undefined value in expression");
    if(r->integer_bitwise_enabled&&(e->op=='|'||e->op=='^'||e->op=='&'))
        return integer_bitwise_binary(r,e->op,&a,&b,line,out);
    if(e->op=='+'&&string_vector_concat_pair(r,&a,&b))return string_vector_concat(r,&a,&b,line,out);
    if(r->vector_arithmetic_enabled&&(a.kind==V_VECTOR||b.kind==V_VECTOR)&&
       e->op!=T_EQ&&e->op!=T_NE)
        return vector_arithmetic_binary(r,e->op,&a,&b,line,out);
    if(r->controls_enabled&&a.kind==V_VECTOR&&b.kind==V_VECTOR&&
       (e->op=='+'||e->op==T_EQ||e->op==T_NE)) {
        if(e->op=='+') {
            out->kind=V_VECTOR;
            for(k=0;k<3;k++) {
                out->vector[k]=a.vector[k]+b.vector[k];
                if(!isfinite(out->vector[k]))return rfail(r,line,"nonfinite vector sum");
            }
        } else {
            k=a.vector[0]==b.vector[0]&&a.vector[1]==b.vector[1]&&a.vector[2]==b.vector[2];
            out->kind=V_NUMBER;out->integer=1;out->number=(float)(e->op==T_EQ?k:!k);
        }
        return 1;
    }
    if((e->op==T_EQ||e->op==T_NE||e->op==T_AND||e->op==T_OR)&&
       (a.kind>V_STRING||b.kind>V_STRING))
        return rfail(r,line,"vector/entity comparison semantics are unsupported");
    if(r239_mixed_scalar_pair(r,e->op,&a,&b))return r239_mixed_scalar_compare(r,e->op,&a,&b,line,out);
    if(r->scalar_switch_enabled)return scalar_binary_selected(r,e->float_arithmetic,e->op,&a,&b,line,out);
    out->kind=V_NUMBER;out->integer=1;
    switch(e->op) {
      case T_EQ:case T_NE:
        if(a.kind!=b.kind)return rfail(r,line,"equality type mismatch");
        k=a.kind==V_STRING ? strcmp(a.str,b.str)==0 : a.number==b.number;
        out->number=(float)(e->op==T_EQ?k:!k);return 1;
      case T_AND:out->number=(float)(truth(a)&&truth(b));return 1;
      case T_OR:out->number=(float)(truth(a)||truth(b));return 1;
      default:break;
    }
    if(a.kind!=V_NUMBER||b.kind!=V_NUMBER)return rfail(r,line,"numeric operator requires numbers");
    switch(e->op) {
      case '<':out->number=(float)(a.number<b.number);break;
      case '>':out->number=(float)(a.number>b.number);break;
      case T_LE:out->number=(float)(a.number<=b.number);break;
      case T_GE:out->number=(float)(a.number>=b.number);break;
      case '+':out->number=a.number+b.number;out->integer=a.integer&&b.integer;break;
      case '-':out->number=a.number-b.number;out->integer=a.integer&&b.integer;break;
      case '*':out->number=a.number*b.number;out->integer=a.integer&&b.integer;break;
      case '/':
        if(b.number==0.0f)return rfail(r,line,"division by zero");
        out->number=a.number/b.number;out->integer=0;break;
      default:return rfail(r,line,"unsupported operator");
    }
    if(!isfinite(out->number))return rfail(r,line,"nonfinite expression");
    return 1;
}
/* Check between nested argument evaluations as well as whole instructions.
 * A callback may retire a different active self context. Once retired, further
 * arguments cannot spawn/mutate or emit effects before run_task notices DEAD. */
static int eval(GscRuntime *r,Frame *f,int index,int depth,Value *out,int line) {
    int ok;
    if(r->state_enabled&&r->executing_task) {
        if(!refresh_state_context(r,r->executing_task))return 0;
        if(r->executing_task->status==TASK_DEAD)return 0;
    }
    ok=eval_inner(r,f,index,depth,out,line,NULL);
    if(ok&&r->state_enabled&&r->executing_task) {
        if(!refresh_state_context(r,r->executing_task))return 0;
        if(r->executing_task->status==TASK_DEAD)return 0;
    }
    return ok;
}
static int assignment(GscRuntime *r,Frame *f,const char *name,Value v,int line) {
    int k;
    if(game_array_selected(r,name)) {
        if(!game_array_value_valid(r,&v))
            return rfail(r,line,"selected game root requires a same-runtime array");
        r->game_array_root=v;return 1;
    }
    if((r->anim_data_enabled&&!strcmp(name,"anim"))||v.kind==V_ANIM)return rfail(r,line,"whole anim owner binding is unsupported");
    if(animation_namespace_kind(v.kind)){if(!animation_namespace_value_valid(r,&v))return rfail(r,line,"invalid animation metadata local value");if(!anim_data_current(r,r->executing_task))return 0;}
    if(v.kind==V_ANIMATION){if(!animation_value_valid(r,&v))return rfail(r,line,"invalid animation reference local value");if(!anim_data_current(r,r->executing_task))return 0;}
    if(v.kind==V_FUNCTION&&!function_value_valid(r,&v))return rfail(r,line,"invalid function reference local value");
    if(v.kind==V_STRUCT&&!struct_value_valid(r,&v))return rfail(r,line,"invalid structure local value");
    if(v.kind==V_VEHICLE_NODE&&!vehicle_node_value_valid(r,&v))return rfail(r,line,"invalid vehicle-node local identity");
    if(v.kind==V_NODE&&!node_value_valid(r,&v))return rfail(r,line,"invalid node reference local value");
    if(v.kind==V_HUDELEM&&!hud_value_valid(r,&v))return rfail(r,line,"invalid HUD element local value");
    if(r->flow_enabled&&(literal_name(name)||!strcmp(name,"self")||!strcmp(name,"level")))
        return rfail(r,line,"reserved local binding");
    if(strlen(name)>=sizeof(f->locals[0].name))return rfail(r,line,"local name too long");
    for(k=0;k<f->local_count;k++)if(!strcmp(f->locals[k].name,name)) {
        f->locals[k].value=v;return 1;
    }
    if(f->local_count>=MAX_LOCALS)return rfail(r,line,"local limit");
    str_copy(f->locals[f->local_count].name,sizeof(f->locals[0].name),name);
    f->locals[f->local_count++].value=v;return 1;
}
static int push_frame(GscRuntime *r,Task *task,int fn,const Value *args,int argc,
                      const Value *self,int line) {
    /* After start the linked program and per-runtime admission mask are fixed.
     * Developer and ownership checks still run on every entry. Only successful
     * structural closure walks are shared by subsequent calls to that function. */
    if(!developer_runtime_current(r))return 0;
    if(r->program->function_admission_selected){
        char why[384]={0};
        int reusable=r->started && r->program->linked &&
            r->function_admission_token && r->function_admission_token==r->program->function_admission_token &&
            fn>=0 && fn<r->program->func_count && r->function_admission[fn];
        if(!reusable || !r->verified_function_closure[fn]){
            if(!(reusable?function_admission_runtime_cache(r,fn,why,sizeof(why)):
                          function_admission_runtime_current(r,fn,why,sizeof(why))))
                return rfail(r,0,"callee admission refused before entry: %s",why);
        }
    }

    Frame *f;const Func *function;int n,bound;
    if(fn<0||fn>=r->program->func_count||argc<0||argc>MAX_PARAMS||(!args&&argc))
        return rfail(r,line,"invalid script call frame arguments");
    if(self&&(self->kind==V_ANIM||self->kind==V_ANIMATION||animation_namespace_kind(self->kind)))return rfail(r,line,"whole anim owner script receiver is unsupported");
    if(self&&self->kind==V_NODE)return rfail(r,line,"node script receiver is unsupported");
    if(self&&self->kind==V_HUDELEM)return rfail(r,line,"HUD element script receiver is unsupported");
    function=&r->program->funcs[fn];
    if(function->argc<0||function->argc>MAX_PARAMS)
        return rfail(r,line,"invalid script function parameter count");
    if(task->depth>=MAX_FRAMES)return rfail(r,line,"call depth limit");
    f=&task->frames[task->depth++];memset(f,0,sizeof(*f));
    f->fn=fn;f->pc=function->start;
    if(self)f->self=*self;
    bound=argc<function->argc?argc:function->argc;
    for(n=0;n<bound;n++)if(!assignment(r,f,function->params[n],args[n],line))return 0;
    /* Original formal bind opcodes precede checkclearparams (opcode 32).
     * Excess actuals fault before the first body instruction; they are not
     * ignored and they do not concatenate into the last formal parameter. */
    if(argc>function->argc)return rfail(r,line,"function called with too many parameters");
    return 1;
}
#include "gsc_vm_lifetime.inc"
#include "gsc_vm_r243_thread_start.inc"
#include "gsc_vm_r243_staged_init.inc"
#include "gsc_vm_placed_return.inc"
#include "gsc_vm_r243_controller_return.inc"
#include "gsc_vm_r243_same_bucket_wait.inc"
static int spawn(GscRuntime *r,int fn,const Value *args,int argc,const Value *self,int line) {
    Task *task;
    if(r->r243_thread_start_enabled&&r->executing_task)return r243_spawn_initial(r,fn,args,argc,self,line,NULL);
    /* Fixed placed INIT still uses deferred statements in this profile. Give
     * its actual level child map-only lifetime, matching the existing native
     * R243 inline ownership rule without selecting that broader scheduler. */
    if(r->placed_init_initial_scope&&(!r->executing_task||!self||
       (self->kind!=V_LEVEL&&(self->kind!=V_ENTITY||!state_ref_equal(&self->entity,&r->executing_task->certificate.owner)))))
        return rfail(r,line,"placed INIT child requires exact Actor self or level map owner");
    task=task_acquire(r,line);
    if(!task)return 0;
    task->status=TASK_READY;task->serial=r->next_serial++;
    if(r->executing_task){task->certificate=r->executing_task->certificate;task->map_certificate=r->executing_task->map_certificate;}
    if(r->placed_init_initial_scope&&self->kind==V_LEVEL)memset(&task->certificate,0,sizeof(task->certificate));
    if(self)task->initial_self=*self;
    if(!push_frame(r,task,fn,args,argc,self,line)) {
        if(r->lifetime_enabled) {task->depth=0;task->status=TASK_DEAD;task_results_clear(task);}
        return 0;
    }
    if(r->r237_frame_buckets_enabled&&!r237_queue_offer(r,task,r237_bucket(r),line))return 0;
    return 1;
}
static int event_string(GscRuntime *r,Value value,char *dst,size_t size,int line) {
    if(value.kind!=V_STRING||!value.str[0]||strlen(value.str)>=size)
        return rfail(r,line,"event name must be a nonempty string");
    str_copy(dst,size,value.str);return 1;
}
static int payload_deliver(GscRuntime *,const GscEntityRef *,const char *,const Value *,int,int);
static void endon_cancel(GscRuntime *,const GscEntityRef *,const char *);
static int notify_internal(GscRuntime *r,const char *name) {
    int n,count=0;
    if(r->event_payloads_enabled)return payload_deliver(r,NULL,name,NULL,0,0)>=0;
    if(r->r237_frame_buckets_enabled){for(n=0;n<r->task_count;n++){Task *t=task_at(r,n);if(t->status==TASK_EVENT&&t->event_scope==EVENT_LEVEL&&!strcmp(t->event,name))count++;}
        if(!r237_queue_available(r,count,0))return 0;}
    endon_cancel(r,NULL,name);
    for(n=0;n<r->task_count;n++) {
        Task *task=task_at(r,n);
        if(task->status==TASK_EVENT&&task->event_scope==EVENT_LEVEL&&!strcmp(task->event,name)) {
            if(!entity_certificate_current(r,task)){if(r->error[0])return 0;continue;}
            if(r->frame_cadence_enabled){task->wake_ms=r->cadence_drain_ms;task->cadence_event_resume=1;}
            if(r->r237_frame_buckets_enabled&&!r237_queue_offer(r,task,r237_bucket(r),0))return 0;
            task->status=TASK_READY;
        }
    }
    return 1;
}
static int entity_ref_equal(const GscEntityRef *a,const GscEntityRef *b) {
    return a->epoch==b->epoch&&a->slot==b->slot&&a->generation==b->generation;
}
/* 1 alive, 0 stale/missing, -1 callback poisoned this runtime. */
static int event_owner_live(GscRuntime *r,const GscEntityRef *owner) {
    int alive;
    if(!r->events_enabled||!owner||!owner->epoch||!owner->generation)return 0;
    alive=entity_validate(r,owner)==1;
    return r->error[0]?-1:alive;
}
static int entity_notify_internal(GscRuntime *r,const GscEntityRef *owner,const char *name) {
    int n,awakened=0,count=0;
    if(r->event_payloads_enabled)return payload_deliver(r,owner,name,NULL,0,0);
    if(r->r237_frame_buckets_enabled){for(n=0;n<r->task_count;n++){Task *t=task_at(r,n);if(t->status==TASK_EVENT&&t->event_scope==EVENT_ENTITY&&entity_ref_equal(&t->event_owner,owner)&&!strcmp(t->event,name))count++;}
        if(!r237_queue_available(r,count,0))return -1;}
    endon_cancel(r,owner,name);
    for(n=0;n<r->task_count;n++) {
        Task *t=task_at(r,n);
        if(t->status==TASK_EVENT&&t->event_scope==EVENT_ENTITY&&
           entity_ref_equal(&t->event_owner,owner)&&!strcmp(t->event,name)) {
            if(!entity_certificate_current(r,t)){if(r->error[0])return -1;continue;}
            if(r->frame_cadence_enabled){t->wake_ms=r->cadence_drain_ms;t->cadence_event_resume=1;}
            if(r->r237_frame_buckets_enabled&&!r237_queue_offer(r,t,r237_bucket(r),0))return -1;
            t->status=TASK_READY;awakened++;
        }
    }
    return awakened;
}
static void clear_wait_owner(Task *task) {
    task->event_scope=EVENT_LEVEL;memset(&task->event_owner,0,sizeof(task->event_owner));
    task->event[0]=0;
    task->event_count=0;task->event_frame=0;
    task->event_match_active=task->event_filter_count=0;
    memset(task->event_filters,0,sizeof(task->event_filters));
}
#include "gsc_vm_owned_wait_delivery.inc"
#include "gsc_vm_endon.inc"
/* R86 bounded lifetime policy: retire only this pending waiter task. Never
 * retarget an entity slot or execute its continuation after identity loss. */
static int refresh_wait_owner(GscRuntime *r,Task *task) {
    int alive;
    if(task->event_scope!=EVENT_ENTITY||
       (task->status!=TASK_EVENT&&task->status!=TASK_READY))return 1;
    alive=event_owner_live(r,&task->event_owner);
    if(!entity_certificate_current(r,task))return !r->error[0];
    if(alive<0)return 0;
    if(!alive) {r54_dirty_mark_task(r,task);task->depth=0;task->status=TASK_DEAD;clear_wait_owner(task);}
    return 1;
}
static int run_entity_event(GscRuntime *r,Task *task,int descriptor,const Value *receiver,
                             const Value *arg,int line) {
    char event[128];int alive;
    if(r->state_enabled&&receiver&&receiver->kind==V_LEVEL) {
        if(!event_string(r,*arg,event,sizeof(event),line))return 0;
        if(descriptor==ENTITY_EVENT_ENDON)return endon_register(r,task,NULL,event,line,0);
        if(descriptor==ENTITY_EVENT_WAITTILL) {
            task->event_scope=EVENT_LEVEL;memset(&task->event_owner,0,sizeof(task->event_owner));
            str_copy(task->event,sizeof(task->event),event);task->status=TASK_EVENT;if(r->precache_string_enabled)precache_string_wait_boundary(r,task);return 1;
        }
        if(descriptor!=ENTITY_EVENT_NOTIFY)return rfail(r,line,"invalid level event descriptor");
        return notify_internal(r,event);
    }
    if(!r->events_enabled||!receiver||receiver->kind!=V_ENTITY)
        return rfail(r,line,"entity event requires an entity receiver");
    if(!event_string(r,*arg,event,sizeof(event),line))return 0;
    alive=event_owner_live(r,&receiver->entity);
    if(!entity_task_current(r))return 0;
    if(alive<0)return 0;
    if(!alive)return rfail(r,line,"entity event owner is stale or missing");
    if(descriptor==ENTITY_EVENT_ENDON)return endon_register(r,task,&receiver->entity,event,line,0);
    if(descriptor==ENTITY_EVENT_WAITTILL) {
        task->event_scope=EVENT_ENTITY;task->event_owner=receiver->entity;
        str_copy(task->event,sizeof(task->event),event);task->status=TASK_EVENT;if(r->precache_string_enabled)precache_string_wait_boundary(r,task);
        return 1;
    }
    if(descriptor!=ENTITY_EVENT_NOTIFY)return rfail(r,line,"invalid entity event descriptor");
    (void)entity_notify_internal(r,&receiver->entity,event);return 1;
}
#include "gsc_vm_event_payloads.inc"
#include "gsc_vm_delete_transactions.inc"
#include "gsc_vm_loop_fx.inc"
#include "gsc_vm_actor_spawn.inc"
static int transition_ms(GscRuntime *r,float seconds,int line,int *out) {
    char text[32],*end;double product;long parsed;
    if(!(seconds>=0.0f&&seconds<=FLT_MAX))return rfail(r,line,"invalid fog transition");
    /* R29 evidence: original fld DWORD, fmul DWORD 1000.0f, fstp QWORD,
     * then %.0f; do not round the product to float32. */
    product=(double)seconds*(double)1000.0f;
    if(!(product>=0.0&&product<=INT_MAX))return rfail(r,line,"fog transition overflow");
    if(snprintf(text,sizeof(text),"%.0f",product)<=0)return rfail(r,line,"fog transition formatting");
    errno=0;parsed=strtol(text,&end,10);
    if(errno||*end||parsed<0||parsed>INT_MAX)return rfail(r,line,"fog transition overflow");
    *out=(int)parsed;return 1;
}
#include "gsc_vm_fog_bridge.inc"
#include "gsc_vm_flow.inc"
static int run_task(GscRuntime *r,Task *task,int *budget) {
    const GscProgram *g=r->program;const Instr *i;const Func *fun;
    Frame *f;Value v,args[MAX_PARAMS];int n,target,delay;double ms;
    while(task->status==TASK_READY&&task->depth>0) {
        if(!refresh_state_context(r,task))return 0;
        if(task->status!=TASK_READY)break;
        if(--*budget<0)return rfail(r,0,"instruction budget exceeded");
        f=&task->frames[task->depth-1];fun=&g->funcs[f->fn];
        if(r->flow_enabled&&f->loop_count) {
            FlowLoop *loop=&f->loops[f->loop_count-1];
            if(f->pc>loop->end)return rfail(r,0,"escaped active loop body");
            if(f->pc==loop->end) {
                if(!flow_loop_boundary(r,task,f,budget))return 0;
                continue;
            }
        }
        if(f->pc>=fun->end) {if(r243_controller_lane_selected(r)||r->placed_return){if(!r243_pop_normal_frame(r,task,0))return 0;}else task->depth--;continue;}
        {int next_pc=developer_skip_region(g,f->pc);if(next_pc!=f->pc){f->pc=next_pc;continue;}}
        i=&g->code[f->pc++];
        switch(i->kind) {
          case I_EMPTY:
            /* Fixture no-op: no evaluation, effect, yield or clock change.
             * It still consumes the ordinary instruction budget above. */
            break;
          case I_SET:
            if(!eval(r,f,i->expr,0,&v,i->line)||!refresh_state_context(r,task))return 0;
            if(task->status==TASK_DEAD)return 1;
            if(!assignment(r,f,i->name,v,i->line))return 0;
            break;
          case I_GAME_SET: {
            const Expr *field;
            if(r->arrays_enabled&&g->expr[i->dest].kind==E_INDEX) {
                if(!array_assign(r,task,f,i->dest,i->expr,0,i->line))return 0;
                break;
            }
            if(!r->access_enabled||i->dest<0||i->dest>=g->expr_count)
                return rfail(r,i->line,"unsupported game-value assignment");
            field=&g->expr[i->dest];
            if(field->kind!=E_FIELD||!field->field_write_target)
                return rfail(r,i->line,"unsupported entity property assignment");
            if(!eval(r,f,field->a,0,&v,i->line))return 0;
            if(r->arrays_enabled&&v.kind==V_ARRAY)return rfail(r,i->line,"array size field is read-only");
            if(r->arrays_enabled&&!state_receiver(r,&v,i->line))return 0;
            if(!eval(r,f,i->expr,0,&args[0],i->line))return 0;
            if(!refresh_state_context(r,task))return 0;
            if(task->status==TASK_DEAD)return 1;
            if(!(r->state_enabled?state_field_write(r,&v,field->name,&args[0],i->line):
                 bridge_property_write(r,&v,entity_property_descriptor(field->name),&args[0],i->line)))return 0;
            break;
          }
          case I_MUTATE: {
            const Expr *field;Value current,sum;
            if(r->flow_enabled) {
                if(!flow_mutation(r,task,f,i->dest,i->expr,i->op,i->line))return 0;
                break;
            }
            if(!control_mutation_valid(g,i))
                return rfail(r,i->line,"unsupported compound assignment or increment runtime semantics");
            field=&g->expr[i->dest];
            if(!eval(r,f,field->a,0,&v,i->line)||
               !state_field_read(r,&v,field->name,i->line,&current)||
               !eval(r,f,i->expr,0,&args[0],i->line))return 0;
            if(!refresh_state_context(r,task))return 0;
            if(task->status==TASK_DEAD)return 1;
            if(current.kind!=V_VECTOR||args[0].kind!=V_VECTOR)
                return rfail(r,i->line,"origin += requires two vectors");
            memset(&sum,0,sizeof(sum));sum.kind=V_VECTOR;
            for(n=0;n<3;n++) {
                sum.vector[n]=current.vector[n]+args[0].vector[n];
                if(!isfinite(sum.vector[n]))return rfail(r,i->line,"nonfinite origin sum");
            }
            if(!state_field_write(r,&v,field->name,&sum,i->line))return 0;
            break;
          }
          case I_METHOD: {
            int descriptor=entity_method_descriptor(g,i->name);
            if(r->state_enabled&&(i->receiver_binding_kind==GSC_RECEIVER_TARGET_LOCAL_SCRIPT||
               i->receiver_binding_kind==GSC_RECEIVER_TARGET_QUALIFIED_SCRIPT)) {
                for(n=i->argc-1;n>=0;n--)if(!eval(r,f,i->args[n],0,&args[n],i->line))return 0;
                if(!eval(r,f,i->receiver,0,&v,i->line))return 0;
                if(!refresh_state_context(r,task))return 0;
                if(task->status==TASK_DEAD)return 1;
                if(!state_receiver(r,&v,i->line))return 0;
                if(task->status==TASK_DEAD)return 1;
                if(!push_frame(r,task,i->receiver_script_target,args,i->argc,&v,i->line))return 0;
                break;
            }
            if(i->receiver_binding_kind==GSC_RECEIVER_TARGET_HUDELEM_NATIVE) {
                int method=hud_elem_method(g,i->name);
                if(!method||i->receiver_native_target!=method||i->receiver_script_target!=-1||i->receiver_event_target!=-1||i->module[0]||i->argc!=hud_elem_arity(method))return rfail(r,i->line,"invalid HUD method binding");
                if(!eval(r,f,i->receiver,0,&v,i->line))return 0;
                for(n=0;n<i->argc;n++)if(!eval(r,f,i->args[n],0,&args[n],i->line))return 0;
                if(!hud_method_invoke(r,&v,method,args,i->argc,i->line))return 0;break;
            }
            if(i->receiver_binding_kind==GSC_RECEIVER_TARGET_ENTITY_EVENT) {
                int event=entity_event_descriptor(g,i->name);
                if(!r->events_enabled||event<0||i->receiver_event_target!=event||
                   i->receiver_native_target!=-1||i->receiver_script_target!=-1||
                   i->module[0]||i->argc!=1)
                    return rfail(r,i->line,"invalid entity event binding");
                if(!eval(r,f,i->receiver,0,&v,i->line)||
                   !eval(r,f,i->args[0],0,&args[0],i->line))return 0;
                if(!refresh_state_context(r,task))return 0;
                if(task->status==TASK_DEAD)return 1;
                if(!run_entity_event(r,task,event,&v,&args[0],i->line))return 0;
                break;
            }
            if(!r->entities_enabled||i->receiver_binding_kind!=GSC_RECEIVER_TARGET_ENTITY_NATIVE||
               descriptor<0||i->receiver_native_target!=descriptor||i->receiver_script_target!=-1||
               i->receiver_event_target!=-1||i->module[0]||
               (r->controls_enabled&&
                !(descriptor>=ENTITY_METHOD_ATTACH&&descriptor<=ENTITY_METHOD_STOPLOOPSOUND)&&
                ((descriptor==502 || descriptor==COD1_LAUNCH || descriptor==COD1_ROTATE_ROLL || descriptor==COD1_ROTATE_TO || descriptor==COD1_MOVE_TO||descriptor==ENTITY_METHOD_SCRIPTED_START||descriptor==ENTITY_METHOD_SCRIPT_STATE||descriptor==ENTITY_METHOD_SCRIPTED_PREFIX||descriptor==ENTITY_METHOD_ACTOR_DROP||entity_link_method(descriptor)||vehicle_method_kind(descriptor))?!entity_method_argc_valid(descriptor,i->argc):i->argc!=entity_method_arity(descriptor))))
                return rfail(r,i->line,"unsupported receiver method/thread");
            if(!eval(r,f,i->receiver,0,&v,i->line))return 0;
            for(n=0;n<i->argc;n++)if(!eval(r,f,i->args[n],0,&args[n],i->line))return 0;
            if(!refresh_state_context(r,task))return 0;
            if(task->status==TASK_DEAD)return 1;
            if(!(descriptor==ENTITY_METHOD_SCRIPTED_START?r243_scripted_start_invoke(r,&v,args,i->argc,i->line):descriptor==ENTITY_METHOD_SCRIPT_STATE?r243_script_state_invoke(r,&v,args,i->argc,i->line):descriptor==ENTITY_METHOD_SCRIPTED_PREFIX?r243_scripted_prefix_invoke(r,&v,args,i->argc,i->line):R243ActorFloatMask(descriptor)?r243_actor_float_call(r,&v,descriptor,i->argc,i->line,NULL):descriptor==COD1_ALLOWED_STANCES_METHOD?cod1_stance_invoke(r,&v,args,i->argc,i->line,NULL,1):descriptor==R243_STANCE_METHOD?r243_stance_call(r,&v,args,i->argc,i->line,NULL):descriptor==R243_ANIMTIME_METHOD?r243_animtime_call(r,&v,args,i->argc,i->line,NULL):descriptor==ENTITY_METHOD_ACTOR_DROP?actor_drop_invoke(r,&v,args,i->argc,i->line):actor_look_kind(descriptor)?actor_look_invoke(r,&v,descriptor,args,i->argc,i->line,NULL):actor_animation_kind(descriptor)?actor_animation_invoke(r,&v,descriptor,args,i->argc,i->line):turret_animation_kind(descriptor)?turret_animation_invoke(r,&v,descriptor,args,i->argc,i->line):turret_method_kind(descriptor)?turret_invoke(r,&v,descriptor,args,i->argc,i->line,NULL):
                 attachment_query_kind(descriptor)?attachment_query_invoke(r,&v,descriptor,args,i->argc,i->line,NULL):
                 descriptor==504?entity_method_invoke(r,&v,descriptor,args,i->argc,i->line):
                 (descriptor==503||descriptor==COD1_SET_GOAL_POS_METHOD||descriptor==COD1_SET_GOAL_ENTITY_METHOD)?actor_goal_invoke(r,descriptor,&v,args,i->argc,i->line):
                 (descriptor==502 || descriptor==COD1_ROTATE_VELOCITY || descriptor==COD1_MOVE_GRAVITY || descriptor==COD1_LAUNCH || descriptor==COD1_ROTATE_ROLL || descriptor==COD1_ROTATE_TO || descriptor==COD1_MOVE_TO)?yaw_motion_invoke(r,descriptor,&v,args,i->argc,i->line):
                 (descriptor==501||descriptor==GSC_CONNECTPATHS_METHOD||descriptor==GSC_DISCONNECTPATHS_METHOD||descriptor==GSC_SOLID_METHOD||descriptor==GSC_GRENADE_TOUCH_METHOD||descriptor==GSC_GRENADE_BOUNCE_METHOD)?entity_collision_invoke(r,descriptor,&v,i->argc,i->line):
                 spatial_method_kind(descriptor)?spatial_native_invoke(r,&v,descriptor,args,i->argc,i->line,NULL):
                 entity_coordinate(descriptor)?entity_coordinate_invoke(r,&v,descriptor,args,i->argc,i->line,NULL):
                 entity_link_method(descriptor)?entity_link_invoke(r,&v,descriptor,args,i->argc,i->line):
                 vehicle_method_kind(descriptor)?vehicle_method_invoke(r,&v,descriptor,args,i->argc,i->line):
                 entity_operation(descriptor)?entity_operation_invoke(r,&v,descriptor,args,i->argc,i->line,NULL):
                 player_method_kind(descriptor)?player_native_invoke(r,&v,descriptor,args,i->argc,i->line,NULL):
                 descriptor>=ENTITY_METHOD_ATTACH&&descriptor<=ENTITY_METHOD_STOPLOOPSOUND?
                 entity_method_invoke(r,&v,descriptor,args,i->argc,i->line):
                 descriptor==ENTITY_METHOD_SETMODEL?bridge_setmodel(r,&v,args,i->argc,i->line):
                 descriptor==ENTITY_METHOD_HIDE||descriptor==ENTITY_METHOD_SHOW?
                 bridge_setvisible(r,&v,descriptor,i->argc,i->line):
                 bridge_setorigin(r,&v,args,i->argc,i->line)))return 0;
            break;
          }
          case I_METHOD_THREAD:
            if(r->state_enabled&&(i->receiver_binding_kind==GSC_RECEIVER_TARGET_LOCAL_SCRIPT||
               i->receiver_binding_kind==GSC_RECEIVER_TARGET_QUALIFIED_SCRIPT)) {
                for(n=i->argc-1;n>=0;n--)if(!eval(r,f,i->args[n],0,&args[n],i->line))return 0;
                if(!eval(r,f,i->receiver,0,&v,i->line))return 0;
                if(!refresh_state_context(r,task))return 0;
                if(task->status==TASK_DEAD)return 1;
                if(!state_receiver(r,&v,i->line))return 0;
                if(task->status==TASK_DEAD)return 1;
                if(!spawn(r,i->receiver_script_target,args,i->argc,&v,i->line))return 0;
                break;
            }
            return rfail(r,i->line,"unsupported receiver method/thread");
          case I_DYNAMIC_CALL:case I_DYNAMIC_THREAD:
            return rfail(r,i->line,"unsupported computed call/thread runtime semantics");
          case I_SWITCH:
            if(!r->scalar_switch_enabled)return rfail(r,i->line,"unsupported switch runtime semantics");
            if(!eval(r,f,i->expr,0,&v,i->line)||!flow_switch_select(r,f,f->pc-1,&v,i->line))return 0;
            break;
          case I_FOR:case I_WHILE:case I_DO_WHILE:
            if(!r->flow_enabled)return rfail(r,i->line,"unsupported loop runtime semantics");
            if(!flow_loop_begin(r,task,f,f->pc-1,i,budget))return 0;
            break;
          case I_RETURN:
            if(!r->flow_enabled)return rfail(r,i->line,"unsupported return runtime semantics");
            if(i->expr>=0&&!eval(r,f,i->expr,0,&v,i->line))return 0;
            if(!refresh_state_context(r,task))return 0;
            if(task->status==TASK_DEAD)return 1;
            if(!r243_pop_normal_frame(r,task,i->line))return 0;break;
          case I_BREAK:case I_CONTINUE:
            if(!r->flow_enabled)return rfail(r,i->line,"unsupported break/continue runtime semantics");
            if(!flow_transfer(r,f,i))return 0;
            break;
          case I_JZ:
            if(!eval(r,f,i->expr,0,&v,i->line))return 0;
            if(!refresh_state_context(r,task))return 0;
            if(task->status==TASK_DEAD)return 1;
            if(v.kind==V_UNDEFINED)return rfail(r,i->line,"undefined condition");
            if(v.kind>V_STRING)return rfail(r,i->line,"vector/entity condition semantics are unsupported");
            if(!truth(v))f->pc=i->dest;
            break;
          case I_JUMP:f->pc=i->dest;break;
          case I_WAIT:
            if(!eval(r,f,i->expr,0,&v,i->line))return 0;
            if(!refresh_state_context(r,task))return 0;
            if(task->status==TASK_DEAD)return 1;
            if(v.kind!=V_NUMBER||!(v.number>=0.0f))return rfail(r,i->line,"wait requires nonnegative seconds");
            if(r->r237_frame_buckets_enabled){if(!r237_timed_wait(r,task,&v,i->line))return 0;}
            else if(r->frame_cadence_enabled&&float_arithmetic_active(r)) {
                if(task->cadence_event_resume)return rfail(r,i->line,"selected cadence event-to-timed-wait queue base is held");
                if(!frame_cadence_wait_delay(r,&v,i->line,&delay))return 0;
                task->wake_ms=r->cadence_drain_ms+delay;
            } else {
                ms=(double)v.number*1000.0;
                if(ms>INT_MAX-r->now_ms)return rfail(r,i->line,"wait time overflow");
                /* Legacy fixture convention: nearest millisecond. */
                delay=(int)floor(ms+0.5);if(delay<1)delay=1;
                task->wake_ms=r->now_ms+delay;
            }
            task->status=TASK_SLEEP;if(r->precache_string_enabled)precache_string_wait_boundary(r,task);break;
          case I_WAITER:
            if(!eval(r,f,i->expr,0,&v,i->line))return 0;
            if(!refresh_state_context(r,task))return 0;
            if(task->status==TASK_DEAD)return 1;
            if(!event_string(r,v,task->event,sizeof(task->event),i->line))return 0;
            task->event_scope=EVENT_LEVEL;memset(&task->event_owner,0,sizeof(task->event_owner));
            task->status=TASK_EVENT;if(r->precache_string_enabled)precache_string_wait_boundary(r,task);break;
          case I_NOTIFY:
            if(!eval(r,f,i->expr,0,&v,i->line))return 0;
            if(!refresh_state_context(r,task))return 0;
            if(task->status==TASK_DEAD)return 1;
            if(!event_string(r,v,task->event,sizeof(task->event),i->line))return 0;
            if(!notify_internal(r,task->event)&&r->r237_frame_buckets_enabled)return 0;task->event[0]=0;break;
          case I_CALL:case I_THREAD:
            if(i->target>=0) {
                for(n=i->argc-1;n>=0;n--)if(!eval(r,f,i->args[n],0,&args[n],i->line))return 0;
            } else for(n=0;n<i->argc;n++)if(!eval(r,f,i->args[n],0,&args[n],i->line))return 0;
            if(!refresh_state_context(r,task))return 0;
            if(task->status==TASK_DEAD)return 1;
            if(i->kind==I_CALL&&i->target<0) {
                if(!invoke_builtin_statement(r,-1-i->target,args,i->argc,i->line))return 0;
            } else {
                target=i->target;
                if(target<0)return rfail(r,i->line,"unresolved function at runtime");
                if(i->kind==I_THREAD) {if(!spawn(r,target,args,i->argc,&f->self,i->line))return 0;}
                else {Value inherited=f->self;
                    if(!push_frame(r,task,target,args,i->argc,&inherited,i->line))return 0;}
            }
            break;
          default:return rfail(r,i->line,"invalid instruction");
        }
    }
    if(task->depth==0)task->status=TASK_DEAD;
    return 1;
}
#include "gsc_vm_results.inc"
#include "gsc_vm_array_collect.inc"
#include "gsc_vm_array_cow_r238.inc"

/* These syntax-only accessors intentionally do not require link(). Corpus
 * callers must be able to inspect unknown native calls without registering
 * fictitious implementations. Ordinary receiver-target linking is unchanged. */
int gsc_program_receiver_thread_count(const GscProgram *g) {
    int n,count=0;if(!g||g->invalid)return 0;
    for(n=0;n<g->code_count;n++)if(g->code[n].kind==I_METHOD_THREAD||
        (g->code[n].kind==I_DYNAMIC_THREAD&&g->code[n].receiver>=0))count++;
    return count;
}
int gsc_program_receiver_thread_get(const GscProgram *g,int index,
                                    GscReceiverThreadInfo *out) {
    int n,k,count=0;GscReceiverThreadInfo info;
    if(!g||g->invalid||!out||index<0)return 0;
    for(n=0;n<g->code_count;n++) {
        const Instr *i=&g->code[n];
        if(i->kind!=I_METHOD_THREAD&&
           !(i->kind==I_DYNAMIC_THREAD&&i->receiver>=0))continue;
        if(count++!=index)continue;
        if(i->receiver<0||i->receiver>=g->expr_count||i->line<1||i->col<1||
           i->owner_module<0||i->owner_module>=g->module_count||
           i->argc<0||i->argc>MAX_PARAMS)return 0;
        if(i->kind==I_DYNAMIC_THREAD&&(i->expr<0||i->expr>=g->expr_count))return 0;
        memset(&info,0,sizeof(info));info.instruction=n;
        info.owner_module=i->owner_module;info.line=i->line;info.col=i->col;
        info.kind=i->kind==I_METHOD_THREAD?GSC_RECEIVER_THREAD_NAMED:GSC_RECEIVER_THREAD_COMPUTED;
        info.receiver_expr=i->receiver;info.target_expr=i->kind==I_DYNAMIC_THREAD?i->expr:-1;
        info.argc=i->argc;
        for(k=0;k<GSC_DYNAMIC_INFO_MAX_ARGS;k++)info.args[k]=-1;
        for(k=0;k<i->argc;k++) {
            if(i->args[k]<0||i->args[k]>=g->expr_count)return 0;
            info.args[k]=i->args[k];
        }
        if(i->kind==I_METHOD_THREAD) {
            str_copy(info.name,sizeof(info.name),i->name);
            str_copy(info.module,sizeof(info.module),i->module);
        }
        *out=info;return 1;
    }
    return 0;
}
int gsc_program_thread_expr_count(const GscProgram *g) {
    int n,count=0;if(!g||g->invalid)return 0;
    for(n=0;n<g->expr_count;n++)if(g->expr[n].kind==E_THREAD_CALL)count++;
    return count;
}
int gsc_program_thread_expr_get(const GscProgram *g,int index,GscThreadExprInfo *out) {
    int n,k,count=0;GscThreadExprInfo info;GscSyntaxExprInfo syntax;
    if(!g||g->invalid||!out||index<0)return 0;
    for(n=0;n<g->expr_count;n++) {
        const Expr *e=&g->expr[n];
        if(e->kind!=E_THREAD_CALL||count++!=index)continue;
        if(e->line<1||e->col<1||e->owner_module<0||e->owner_module>=g->module_count||
           !gsc_program_syntax_expr_get(g,n,&syntax))return 0;
        memset(&info,0,sizeof(info));
        info.expr=n;info.owner_module=e->owner_module;info.line=e->line;info.col=e->col;
        info.kind=e->op;info.receiver_expr=e->b;info.target_expr=e->a;info.argc=e->argc;
        str_copy(info.name,sizeof(info.name),e->name);str_copy(info.module,sizeof(info.module),e->module);
        for(k=0;k<GSC_DYNAMIC_INFO_MAX_ARGS;k++)info.args[k]=syntax.args[k];
        *out=info;return 1;
    }
    return 0;
}
int gsc_program_syntax_expr_count(const GscProgram *g) {
    return g&&!g->invalid?g->expr_count:0;
}
int gsc_program_syntax_expr_get(const GscProgram *g,int index,GscSyntaxExprInfo *out) {
    static const int kinds[]={GSC_SYNTAX_NUMBER,GSC_SYNTAX_STRING,GSC_SYNTAX_LOCALIZED_STRING,
        GSC_SYNTAX_VAR,GSC_SYNTAX_UNARY,GSC_SYNTAX_BINARY,GSC_SYNTAX_ISDEFINED,
        GSC_SYNTAX_FIELD,GSC_SYNTAX_INDEX,GSC_SYNTAX_NATIVE_CALL,GSC_SYNTAX_METHOD_CALL,
        GSC_SYNTAX_VECTOR,GSC_SYNTAX_ANIM_REF,GSC_SYNTAX_DYNAMIC_CALL,GSC_SYNTAX_EMPTY_ARRAY,
        GSC_SYNTAX_SCOPED_CALL,GSC_SYNTAX_CAST,GSC_SYNTAX_FUNC_REF,GSC_SYNTAX_ANIMTREE_VALUE,
        GSC_SYNTAX_RAW_STRING,GSC_SYNTAX_RAW_LOCALIZED_STRING,GSC_SYNTAX_THREAD_CALL};
    const Expr *e;GscSyntaxExprInfo info;int n;
    _Static_assert(sizeof(kinds)/sizeof(kinds[0])==E_THREAD_CALL+1,
                   "syntax-kind table must cover every expression kind");
    if(!g||g->invalid||!out||index<0||index>=g->expr_count)return 0;
    e=&g->expr[index];
    if(e->kind<0||e->kind>E_THREAD_CALL)return 0;
    memset(&info,0,sizeof(info));info.expr=index;info.kind=kinds[e->kind];
    info.left_expr=info.right_expr=info.string_literal=-1;
    for(n=0;n<GSC_DYNAMIC_INFO_MAX_ARGS;n++)info.args[n]=-1;
    str_copy(info.name,sizeof(info.name),e->name);
    str_copy(info.module,sizeof(info.module),e->module);
    switch(e->kind) {
      case E_NUMBER:info.number=e->number;info.integer=e->integer;break;
      case E_UNARY:case E_CAST:
        info.operator_token=e->op;info.left_expr=e->a;break;
      case E_BINARY:
        info.operator_token=e->op;info.left_expr=e->a;info.right_expr=e->b;break;
      case E_FIELD:info.left_expr=e->a;break;
      case E_INDEX:info.left_expr=e->a;info.right_expr=e->b;break;
      case E_ISDEFINED:info.argc=1;info.args[0]=e->a;break;
      case E_THREAD_CALL:
        if((e->op!=GSC_THREAD_EXPR_NAMED&&e->op!=GSC_THREAD_EXPR_COMPUTED)||
           (e->op==GSC_THREAD_EXPR_NAMED&&(e->a!=-1||!e->name[0]))||
           (e->op==GSC_THREAD_EXPR_COMPUTED&&(e->a<0||e->name[0]||e->module[0])))return 0;
        info.operator_token=e->op;info.left_expr=e->a;info.right_expr=e->b;
        if(e->argc<0||e->argc>MAX_PARAMS)return 0;
        info.argc=e->argc;
        memcpy(info.args,e->args,(size_t)e->argc*sizeof(e->args[0]));break;
      case E_METHOD_CALL:case E_DYNAMIC_CALL:case E_NATIVE_CALL:case E_SCOPED_CALL:case E_VECTOR:
        if(e->kind==E_METHOD_CALL||e->kind==E_DYNAMIC_CALL)info.left_expr=e->a;
        if(e->kind==E_DYNAMIC_CALL)info.right_expr=e->b;
        if(e->argc<0||e->argc>MAX_PARAMS)return 0;
        info.argc=e->argc;
        memcpy(info.args,e->args,(size_t)e->argc*sizeof(e->args[0]));break;
      case E_STRING:case E_LOCALIZED_STRING:case E_RAW_STRING:case E_RAW_LOCALIZED_STRING:
        if(e->a<0||e->a>=g->string_count)return 0;
        info.string_literal=e->a;break;
      default:break;
    }
    /* The compiler emits children before parents. This also rejects broken
     * handles/cycles without recursively walking untrusted internal indexes. */
    if(info.left_expr<-1||info.left_expr>=index||info.right_expr<-1||info.right_expr>=index)return 0;
    if((e->kind==E_UNARY||e->kind==E_CAST||e->kind==E_BINARY||e->kind==E_FIELD||
        e->kind==E_INDEX||e->kind==E_METHOD_CALL||e->kind==E_DYNAMIC_CALL)&&info.left_expr<0)return 0;
    if((e->kind==E_BINARY||e->kind==E_INDEX)&&info.right_expr<0)return 0;
    for(n=0;n<info.argc;n++)if(info.args[n]<0||info.args[n]>=index)return 0;
    *out=info;return 1;
}
static int entity_runtime_preflight(const GscProgram *g,const GscEntityAccessHost *access,
                                    const GscEntityControlHost *controls,
                                    char *error,size_t size) {
    int n;
    if(g->state_mode)for(n=0;n<g->func_count;n++) {
        int k;const Func *f=&g->funcs[n];
        for(k=0;k<f->argc;k++)if(!strcmp(f->params[k],"self")||!strcmp(f->params[k],"level")) {
            if(error&&size)snprintf(error,size,"self/level parameter bindings are unsupported");
            return 0;
        }
    }
    for(n=0;n<g->code_count;n++) {
        const Instr *i=&g->code[n];
        int descriptor=entity_method_descriptor(g,i->name);
        int event=entity_event_descriptor(g,i->name);
        if(i->kind==I_MUTATE&&g->controls_mode&&!g->flow_mode) {
            int type=control_mutation_valid(g,i)?known_value_kind(g,i->expr):-1;
            if(!control_mutation_valid(g,i)||(type>=0&&type!=V_VECTOR)||
               !controls||!controls->setorigin||!access||!access->getorigin) {
                if(error&&size)snprintf(error,size,"line %d: only origin += vector with enabled pose capabilities is supported",i->line);
                return 0;
            }
        }
        if(g->state_mode&&(i->kind==I_METHOD||i->kind==I_METHOD_THREAD)&&
           (i->receiver_binding_kind==GSC_RECEIVER_TARGET_LOCAL_SCRIPT||
            i->receiver_binding_kind==GSC_RECEIVER_TARGET_QUALIFIED_SCRIPT)) {
            int target=i->receiver_script_target;
            if(target<0||target>=g->func_count||i->receiver_native_target!=-1||
               i->receiver_event_target!=-1||i->receiver<0||i->receiver>=g->expr_count||
               i->argc<0||i->argc>MAX_PARAMS||
               !callable_name_equal(g,g->funcs[target].name,i->name)||
               (i->module[0]?g->funcs[target].module!=find_module(g,i->module):
                 g->funcs[target].module!=receiver_owner_module(g,n,0))) {
                if(error&&size)snprintf(error,size,"line %d: invalid receiver script binding",i->line);
                return 0;
            }
            continue;
        }
        if(i->kind==I_METHOD&&i->receiver_binding_kind==GSC_RECEIVER_TARGET_HUDELEM_NATIVE) {
            int method=hud_elem_method(g,i->name);
            if(!method||i->receiver_native_target!=method||i->receiver_script_target!=-1||i->receiver_event_target!=-1||i->module[0]||i->argc!=hud_elem_arity(method)) {
                if(error&&size)snprintf(error,size,"line %d: invalid HUD method binding",i->line);return 0;
            }continue;
        }
        if(i->kind==I_METHOD_THREAD||
           (i->kind==I_METHOD&&!(i->receiver_script_target==-1&&!i->module[0]&&
            ((i->receiver_binding_kind==GSC_RECEIVER_TARGET_ENTITY_NATIVE&&descriptor>=0&&
              entity_method_argc_valid(descriptor,i->argc)&&
              i->receiver_native_target==descriptor&&i->receiver_event_target==-1)||
             (i->receiver_binding_kind==GSC_RECEIVER_TARGET_ENTITY_EVENT&&event>=0&&
              entity_event_argc_valid(g,event,i->argc)&&
              i->receiver_event_target==event&&i->receiver_native_target==-1))))) {
            if(error&&size)snprintf(error,size,"%s line %d: unsupported receiver method/thread %s",g->modules[i->owner_module],i->line,i->name);
            return 0;
        }
        if(i->kind==I_METHOD&&descriptor==504&&i->argc==1){
            int kind=known_value_kind(g,i->args[0]);if(kind>=0&&kind!=V_LOCALIZED_STRING&&
              !(g->expr[i->args[0]].kind==E_STRING&&!g->expr[i->args[0]].name[0])){if(error&&size)snprintf(error,size,"line %d: setHintString requires a localized string or empty clear",i->line);return 0;}
        }
        if(i->kind==I_METHOD&&descriptor==ENTITY_METHOD_SETMODEL&&(!access||!access->setmodel)) {
            if(error&&size)snprintf(error,size,"line %d: setmodel requires an enabled host capability",i->line);
            return 0;
        }
        if(g->controls_mode&&i->kind==I_METHOD&&
           i->receiver_binding_kind==GSC_RECEIVER_TARGET_ENTITY_NATIVE&&
           ((descriptor==ENTITY_METHOD_SETORIGIN&&(!controls||!controls->setorigin))||
            ((descriptor==ENTITY_METHOD_HIDE||descriptor==ENTITY_METHOD_SHOW)&&
             (!controls||!controls->setvisible)))) {
            if(error&&size)snprintf(error,size,"line %d: %s requires an enabled control capability",i->line,i->name);
            return 0;
        }
        if(access&&i->kind==I_SET&&(!strcmp(i->name,"self")||!strcmp(i->name,"level"))) {
            if(error&&size)snprintf(error,size,"line %d: self/level values are unsupported",i->line);
            return 0;
        }
        if(i->kind==I_GAME_SET) {
            const Expr *field;
            if(!access||i->dest<0||i->dest>=g->expr_count||
               ((field=&g->expr[i->dest])->kind!=E_FIELD&&!(g->arrays_mode&&field->kind==E_INDEX))||
               (field->kind==E_FIELD&&!field->field_write_target)||
               (!g->state_mode&&entity_property_descriptor(field->name)<0)) {
                if(error&&size)snprintf(error,size,"line %d: unsupported entity property assignment",i->line);
                return 0;
            }
        }
    }
    for(n=0;n<g->expr_count;n++) {
        const Expr *e=&g->expr[n];
            if(e->kind==E_METHOD_CALL&&!e->module[0]&&entity_method_descriptor(g,e->name)==R243_STANCE_METHOD&&find_func(g,receiver_owner_module(g,n,1),e->name)<0&&stance_static_issues(g,e->args,e->argc)){if(error&&size)snprintf(error,size,"line %d: isStanceAllowed requires one stance string",e->line);return 0;}
            if(e->kind==E_METHOD_CALL&&!e->module[0]&&entity_method_descriptor(g,e->name)==R243_ANIMTIME_METHOD&&find_func(g,receiver_owner_module(g,n,1),e->name)<0&&animtime_static_issues(g,e->args,e->argc)){if(error&&size)snprintf(error,size,"line %d: getAnimTime requires one typed animation",e->line);return 0;}
        if(e->kind==E_THREAD_CALL && !thread_expression_bound(g,e)){
            if(error&&size)snprintf(error,size,"line %d: unresolved thread expression %s",e->line,e->name);
            return 0;
        }
        if(e->kind==E_METHOD_CALL&&!g->results_mode) {
            if(error&&size)snprintf(error,size,"line %d: unsupported receiver expression runtime semantics",e->line);
            return 0;
        }
        if(g->results_mode&&e->kind==E_METHOD_CALL&&
           !(e->receiver_binding_kind==GSC_RECEIVER_TARGET_LOCAL_SCRIPT||
             e->receiver_binding_kind==GSC_RECEIVER_TARGET_QUALIFIED_SCRIPT||
             (g->actor_spawn_mode&&e->receiver_binding_kind==GSC_RECEIVER_TARGET_ENTITY_NATIVE&&
              actor_spawn_native_descriptor(e->receiver_native_target)&&e->receiver_script_target==-1&&
              e->receiver_event_target==-1&&!e->module[0]&&callable_name_equal(g,e->name,actor_spawn_native_name(e->receiver_native_target))&&
              e->argc>=0&&e->argc<=MAX_PARAMS)||
             (g->native_queries_mode&&e->receiver_binding_kind==GSC_RECEIVER_TARGET_ENTITY_NATIVE&&
              e->receiver_native_target==ENTITY_METHOD_GETNUMBER&&e->receiver_script_target==-1&&
              e->receiver_event_target==-1&&!e->module[0]&&(callable_name_equal(g,e->name,"getentitynumber")||callable_name_equal(g,e->name,"getentnum"))&&
              entity_method_argc_valid(ENTITY_METHOD_GETNUMBER,e->argc))||
             (g->actor_float_selected&&e->receiver_binding_kind==GSC_RECEIVER_TARGET_ENTITY_NATIVE&&R243ActorFloatMask(e->receiver_native_target)&&e->receiver_script_target==-1&&e->receiver_event_target==-1&&!e->module[0]&&entity_method_descriptor(g,e->name)==e->receiver_native_target&&e->argc==0)||
             (g->stance_selected&&e->receiver_binding_kind==GSC_RECEIVER_TARGET_ENTITY_NATIVE&&e->receiver_native_target==R243_STANCE_METHOD&&e->receiver_script_target==-1&&e->receiver_event_target==-1&&!e->module[0]&&entity_method_descriptor(g,e->name)==R243_STANCE_METHOD&&e->argc==1)||
             (g->animtime_selected&&e->receiver_binding_kind==GSC_RECEIVER_TARGET_ENTITY_NATIVE&&e->receiver_native_target==R243_ANIMTIME_METHOD&&e->receiver_script_target==-1&&e->receiver_event_target==-1&&!e->module[0]&&entity_method_descriptor(g,e->name)==R243_ANIMTIME_METHOD&&e->argc==1)||
             (g->spatial_natives_selected&&e->receiver_binding_kind==GSC_RECEIVER_TARGET_ENTITY_NATIVE&&
              spatial_method_kind(e->receiver_native_target)&&e->receiver_script_target==-1&&
              e->receiver_event_target==-1&&!e->module[0]&&
              spatial_method_descriptor(g,e->name)==e->receiver_native_target&&
              entity_method_argc_valid(e->receiver_native_target,e->argc))||
             (g->entity_coordinates_selected&&e->receiver_binding_kind==GSC_RECEIVER_TARGET_ENTITY_NATIVE&&
              entity_coordinate(e->receiver_native_target)&&e->receiver_script_target==-1&&
              e->receiver_event_target==-1&&!e->module[0]&&
              entity_coordinate_descriptor(g,e->name)==e->receiver_native_target&&
              entity_method_argc_valid(e->receiver_native_target,e->argc))||
             (g->entity_operations_selected&&e->receiver_binding_kind==GSC_RECEIVER_TARGET_ENTITY_NATIVE&&
              entity_operation(e->receiver_native_target)&&entity_method_has_result(e->receiver_native_target)&&
              e->receiver_script_target==-1&&e->receiver_event_target==-1&&!e->module[0]&&
              entity_operation_descriptor(g,e->name)==e->receiver_native_target&&
              entity_method_argc_valid(e->receiver_native_target,e->argc))||
             (g->turrets_selected&&e->receiver_binding_kind==GSC_RECEIVER_TARGET_ENTITY_NATIVE&&
              turret_method_kind(e->receiver_native_target)&&entity_method_has_result(e->receiver_native_target)&&
              e->receiver_script_target==-1&&e->receiver_event_target==-1&&!e->module[0]&&
              turret_method_descriptor(g,e->name)==e->receiver_native_target&&
              entity_method_argc_valid(e->receiver_native_target,e->argc))||
             (g->player_natives_selected&&e->receiver_binding_kind==GSC_RECEIVER_TARGET_ENTITY_NATIVE&&
              player_method_kind(e->receiver_native_target)&&entity_method_has_result(e->receiver_native_target)&&
              e->receiver_script_target==-1&&e->receiver_event_target==-1&&!e->module[0]&&
              player_method_descriptor(g,e->name)==e->receiver_native_target&&
              entity_method_argc_valid(e->receiver_native_target,e->argc))||
             (g->actor_look_selected&&e->receiver_binding_kind==GSC_RECEIVER_TARGET_ENTITY_NATIVE&&actor_look_kind(e->receiver_native_target)&&e->receiver_script_target==-1&&e->receiver_event_target==-1&&!e->module[0]&&actor_look_descriptor(g,e->name)==e->receiver_native_target&&entity_method_has_result(e->receiver_native_target)&&entity_method_argc_valid(e->receiver_native_target,e->argc))||
             (g->attachment_queries_selected&&e->receiver_binding_kind==GSC_RECEIVER_TARGET_ENTITY_NATIVE&&
              attachment_query_kind(e->receiver_native_target)&&e->receiver_script_target==-1&&
              e->receiver_event_target==-1&&!e->module[0]&&
              attachment_query_descriptor(g,e->name)==e->receiver_native_target&&
              entity_method_argc_valid(e->receiver_native_target,e->argc))||
             (g->actor_critical_selected&&e->receiver_binding_kind==GSC_RECEIVER_TARGET_ENTITY_NATIVE&&
              e->receiver_native_target==ENTITY_METHOD_ISCRITICAL&&e->receiver_script_target==-1&&
              e->receiver_event_target==-1&&!e->module[0]&&callable_name_equal(g,e->name,"isincriticalsection")&&
              entity_method_argc_valid(ENTITY_METHOD_ISCRITICAL,e->argc)))) {
            if(error&&size)snprintf(error,size,"line %d: unsupported receiver expression binding %s (%d)",e->audit_line,e->name,e->receiver_native_target);
            return 0;
        }
        if(access&&((e->kind==E_INDEX&&!g->arrays_mode)||
           (!g->state_mode&&e->kind==E_VAR&&(!strcmp(e->name,"self")||!strcmp(e->name,"level"))))) {
            if(error&&size)snprintf(error,size,"line %d: indices and self/level values are unsupported",e->audit_line);
            return 0;
        }
        if(e->kind==E_FIELD) {
            int property=control_property_descriptor(g,e->name);
            if(!access||e->a<0||e->a>=n||
               (!g->state_mode&&(property<0||g->expr[e->a].kind==E_FIELD))) {
                if(error&&size)snprintf(error,size,"line %d: unsupported or nested entity property",e->audit_line);
                return 0;
            }
            if(g->controls_mode&&control_metadata_name(e->name)) {
                int level_receiver=g->expr[e->a].kind==E_VAR&&!strcmp(g->expr[e->a].name,"level");
                if(e->field_write_target&&!level_receiver&&!g->struct_mode) {
                    if(error&&size)snprintf(error,size,"line %d: entity target/targetname metadata is read-only",e->audit_line);
                    return 0;
                }
                if(!e->field_write_target&&!g->node_queries_selected&&(!controls||!controls->getstring)) {
                    if(error&&size)snprintf(error,size,"line %d: metadata read requires an enabled string capability",e->audit_line);
                    return 0;
                }
            }
            if(g->controls_mode&&property==ENTITY_PROPERTY_ORIGIN&&e->field_write_target&&
               (!controls||!controls->setorigin)) {
                if(error&&size)snprintf(error,size,"line %d: origin write requires an enabled control capability",e->audit_line);
                return 0;
            }
            if(property>=0&&(e->field_write_target||!g->node_queries_selected)&&
               (e->field_write_target?property==ENTITY_PROPERTY_ANGLES&&!access->setangles:
               (property==ENTITY_PROPERTY_ORIGIN?!access->getorigin:!access->getangles))) {
                if(error&&size)snprintf(error,size,"line %d: %s %s requires an enabled host capability",
                    e->audit_line,e->name,e->field_write_target?"write":"read");
                return 0;
            }
        }
        if(e->kind==E_VECTOR&&e->argc!=3) {
            if(error&&size)snprintf(error,size,"line %d: invalid three-component vector",e->line);
            return 0;
        }
    }
    return 1;
}
static int runtime_admission_preflight(const GscProgram *program,GscFogCallback fog,void *user,
    const GscEntityHost *entities,const GscEntityAccessHost *access,int events,int state,
    const GscEntityControlHost *controls,int flow,int arrays,const GscEntityCollectionHost *collections,int results,int payloads,int predicates,const GscEntityPredicateHost *predicate_host,
    int actor_spawners,const GscActorSpawnHost *spawn_host,int function_refs,int scalar_switch,int lifetime,int names,int music,const GscMusicHost *music_host,int resources,const GscResourceHost *resource_host,int objectives,const GscObjectiveHost *objective_host,int console,const GscConsoleHost *console_host,int hud,const GscHudHost *hud_host,int mission,const GscMissionHost *mission_host,int blast,const GscBlastHost *blast_host,int quake,const GscQuakeHost *quake_host,int fx,const GscFxHost *fx_host,int cvars,const GscCvarHost *cvar_host,int shellshock,const GscShellshockHost *shellshock_host,int debug_text,const GscDebugTextHost *debug_text_host,int debug_line,const GscDebugLineHost *debug_line_host,int ambient,const GscAmbientHost *ambient_host,int precache_item,const GscPrecacheItemHost *precache_item_host,int save_game,const GscSaveGameHost *save_game_host,int tag_fx,const GscTagFxHost *tag_fx_host,int native_queries,const GscNativeQueryHost *native_query_host,int spawner_arrays,const GscSpawnerArrayHost *spawner_array_host,int randomint,const GscRandomIntHost *randomint_host,int computed_randomint,int structs,int entity_queries,const GscEntityQueryHost *entity_query_host,int dynamic_randomint,int numeric_randomint,int dynamic_randomfloat,char *error,size_t error_size) {
    if(error&&error_size)error[0]=0;
    if(!program||!program->linked||program->invalid||program->entry_fn<0) {
        if(error&&error_size)snprintf(error,error_size,"invalid compiled program");
        return 0;
    }
    if(program->dynamic_randomfloat_mode!=dynamic_randomfloat) {
        if(error&&error_size)snprintf(error,error_size,"dynamic randomfloat program and runtime selectors must match");
        return 0;
    }
    if(program->numeric_randomint_mode!=numeric_randomint) {
        if(error&&error_size)snprintf(error,error_size,"numeric randomint program and runtime selectors must match");
        return 0;
    }
    if(program->dynamic_randomint_mode!=dynamic_randomint) {
        if(error&&error_size)snprintf(error,error_size,"dynamic randomint program and runtime selectors must match");
        return 0;
    }
    if(program->entity_queries_mode!=entity_queries) {
        if(error&&error_size)snprintf(error,error_size,"entity query program and runtime selectors must match");
        return 0;
    }
    if(entity_queries&&!entity_queries_runtime_preflight(program,entity_query_host,error,error_size))return 0;
    if(program->struct_mode!=structs) {
        if(error&&error_size)snprintf(error,error_size,"structure program and runtime selectors must match");
        return 0;
    }
    if(program->computed_randomint_mode!=computed_randomint) {
        if(error&&error_size)snprintf(error,error_size,"computed randomint program and runtime selectors must match");
        return 0;
    }
    if(entities?(!program->entity_mode||!entities->lookup||!entities->setorigin||!entities->validate):
                program->entity_mode) {
        if(error&&error_size)snprintf(error,error_size,"entity-aware link and complete host must be selected together");
        return 0;
    }
    if(access?!program->access_mode:program->access_mode) {
        if(error&&error_size)snprintf(error,error_size,"access-aware link and host must be selected together");
        return 0;
    }
    if(program->event_mode!=events) {
        if(error&&error_size)snprintf(error,error_size,"event-aware link and runtime must be selected together");
        return 0;
    }
    if(program->state_mode!=state) {
        if(error&&error_size)snprintf(error,error_size,"state-aware link and runtime must be selected together");
        return 0;
    }
    if((controls!=NULL)!=program->controls_mode) {
        if(error&&error_size)snprintf(error,error_size,"control-aware link and runtime must be selected together");
        return 0;
    }
    if(program->names_mode!=names||program->music_mode!=music||program->resources_mode!=resources||program->objectives_mode!=objectives||program->console_mode!=console||program->hud_mode!=hud||program->mission_mode!=mission||program->blast_mode!=blast||program->quake_mode!=quake||program->fx_mode!=fx||program->cvars_mode!=cvars||program->shellshock_mode!=shellshock||program->debug_text_mode!=debug_text||program->debug_line_mode!=debug_line||program->ambient_mode!=ambient||program->precache_item_mode!=precache_item||program->save_game_mode!=save_game||program->tag_fx_mode!=tag_fx||program->native_queries_mode!=native_queries||program->spawner_arrays_mode!=spawner_arrays||program->randomint_mode!=randomint) {
        if(error&&error_size)snprintf(error,error_size,"names/music/resources/objectives/console/HUD/mission/blast/quake/FX/cvars/shellshock/debug text/debug line/ambient/precacheItem/saveGame link and runtime must be selected together");
        return 0;
    }
    if(program->lifetime_mode!=lifetime) {
        if(error&&error_size)snprintf(error,error_size,"lifetime link and runtime must be selected together");
        return 0;
    }
    if(program->scalar_switch_mode!=scalar_switch) {
        if(error&&error_size)snprintf(error,error_size,"scalar/switch link and runtime must be selected together");
        return 0;
    }
    if(program->function_ref_mode!=function_refs) {
        if(error&&error_size)snprintf(error,error_size,"function-reference link and runtime must be selected together");
        return 0;
    }
    if(program->actor_spawn_mode!=actor_spawners) {
        if(error&&error_size)snprintf(error,error_size,"actor-spawner link and runtime must be selected together");
        return 0;
    }
    if(program->predicate_mode!=predicates) {
        if(error&&error_size)snprintf(error,error_size,"predicate-aware link and runtime must be selected together");
        return 0;
    }
    if(program->event_payload_mode!=payloads) {
        if(error&&error_size)snprintf(error,error_size,"event payload link and runtime must be selected together");
        return 0;
    }
    if(program->results_mode!=results) {
        if(error&&error_size)snprintf(error,error_size,"result-aware link and runtime must be selected together");
        return 0;
    }
    if(program->arrays_mode!=arrays) {
        if(error&&error_size)snprintf(error,error_size,"array-aware link and runtime must be selected together");
        return 0;
    }
    if(program->flow_mode!=flow) {
        if(error&&error_size)snprintf(error,error_size,"flow-aware link and runtime must be selected together");
        return 0;
    }
    if(entities&&!entities->spawn) {
        int n;
        for(n=0;n<program->expr_count;n++) {
            const Expr *e=&program->expr[n];
            if(e->kind==E_NATIVE_CALL&&e->call_binding==GSC_EXPR_TARGET_ENTITY_NATIVE&&
               e->target==ENTITY_GLOBAL_SPAWN) {
                if(error&&error_size)snprintf(error,error_size,"spawn requires an enabled host capability");
                return 0;
            }
        }
    }
    if(entities) {
        int n;
        for(n=0;n<program->expr_count;n++) {
            const Expr *e=&program->expr[n];
            if(e->kind==E_NATIVE_CALL&&e->call_binding==GSC_EXPR_TARGET_ENTITY_NATIVE&&
               e->target==ENTITY_GLOBAL_SPAWN&&e->argc==3&&!entities->spawn_with_flags) {
                if(error&&error_size)snprintf(error,error_size,"spawn flags require an enabled host capability");
                return 0;
            }
        }
    }
    if(program->requires_local_expr_call&&!results) {
        if(error&&error_size)snprintf(error,error_size,
            "local script expression call requires unsupported runtime semantics");
        return 0;
    }
    if(program->requires_thread_expr_semantics && !(results && program->r243_thread_start_selected)) {
        if(error&&error_size)snprintf(error,error_size,
            "thread expression requires unsupported runtime semantics");
        return 0;
    }
    if(program->requires_do_while_semantics&&!flow) {
        if(error&&error_size)snprintf(error,error_size,
            "do-while requires unsupported runtime semantics");
        return 0;
    }
    if(program->requires_raw_string_semantics) {
        if(error&&error_size)snprintf(error,error_size,
            "unresolved string escape requires unsupported runtime semantics");
        return 0;
    }
    if(program->requires_switch_semantics&&!scalar_switch) {
        if(error&&error_size)snprintf(error,error_size,
            "switch requires unsupported runtime semantics");
        return 0;
    }
    if(program->requires_integer_op_semantics&&!scalar_switch) {
        if(error&&error_size)snprintf(error,error_size,
            "modulo/bitwise-and requires unsupported runtime semantics");
        return 0;
    }
    if(program->requires_dev_block) {
        if(error&&error_size)snprintf(error,error_size,
            "developer '/# ... #/' region requires unsupported runtime semantics");
        return 0;
    }
    if(program->requires_mutation_semantics&&!program->controls_mode) {
        if(error&&error_size)snprintf(error,error_size,
            "compound assignment or increment requires unsupported runtime semantics");
        return 0;
    }
    if(program->requires_cast_semantics&&!scalar_switch) {
        if(error&&error_size)snprintf(error,error_size,
            "numeric cast requires unsupported runtime semantics");
        return 0;
    }
    if(program->requires_func_ref_semantics&&!function_refs) {
        if(error&&error_size)snprintf(error,error_size,
            "function reference requires unsupported runtime semantics");
        return 0;
    }
    if(program->requires_animtree_value_semantics&&!program->animation_namespace_selected) {
        if(error&&error_size)snprintf(error,error_size,
            "#animtree value requires unsupported runtime semantics");
        return 0;
    }
    if(program->anim_data_selected){
        if(error&&error_size)snprintf(error,error_size,"selected anim data profile or reserved binding is unsupported");
        if(!anim_data_program_ready_checked(program,NULL,error,error_size))return 0;
        if(error&&error_size)error[0]=0;
    }
    if(program->requires_animtree&&!program->anim_data_selected) {
        if(error&&error_size)snprintf(error,error_size,
            "animtree directive requires unsupported runtime semantics");
        return 0;
    }
    if(program->requires_dynamic_call&&!function_refs) {
        if(error&&error_size)snprintf(error,error_size,
            "computed call/thread requires unsupported runtime semantics");
        return 0;
    }
    if(program->requires_scoped_expr_call&&!results) {
        if(error&&error_size)snprintf(error,error_size,
            "qualified expression call requires unsupported runtime semantics");
        return 0;
    }
    if(program->requires_empty_array_semantics&&!arrays) {
        if(error&&error_size)snprintf(error,error_size,
            "empty array literal requires unsupported runtime semantics");
        return 0;
    }
    if(program->requires_anim_ref_semantics&&!program->animation_refs_selected&&!program->animation_namespace_selected) {
        if(error&&error_size)snprintf(error,error_size,
            "animation reference requires unsupported runtime semantics");
        return 0;
    }
    if(program->requires_loop_semantics&&!flow) {
        if(error&&error_size)snprintf(error,error_size,
            "for-loop requires unsupported runtime semantics");
        return 0;
    }
    if(program->requires_flow_semantics&&!flow) {
        if(error&&error_size)snprintf(error,error_size,
            "while/return/break/continue require unsupported runtime semantics");
        return 0;
    }
    if(program->requires_localized_string&&!objectives) {
        if(error&&error_size)snprintf(error,error_size,
            "localized string requires unsupported runtime semantics");
        return 0;
    }
    if(program->requires_vector_semantics&&!entities) {
        if(error&&error_size)snprintf(error,error_size,
            "vector literal requires unsupported runtime semantics");
        return 0;
    }
    if(program->requires_literal_semantics&&!flow) {
        if(error&&error_size)snprintf(error,error_size,
            "true/false/undefined require unsupported literal semantics");
        return 0;
    }
    if(program->requires_game_values&&!access) {
        if(error&&error_size)snprintf(error,error_size,
            "game values and field/index expressions require unsupported runtime semantics");
        return 0;
    }
    if(program->requires_receiver_method&&!entities) {
        if(error&&error_size)snprintf(error,error_size,
            "receiver methods/threads require unsupported runtime semantics");
        return 0;
    }
    if(entities&&!entity_runtime_preflight(program,access,controls,error,error_size))return 0;
    if(flow&&!flow_runtime_preflight(program,access,controls,error,error_size))return 0;
    if(arrays&&!array_runtime_preflight(program,collections,error,error_size))return 0;
    if(predicates&&!predicate_runtime_preflight(program,predicate_host,error,error_size))return 0;
    if(actor_spawners&&!actor_spawn_runtime_preflight(program,spawn_host,error,error_size))return 0;
    if(function_refs&&!function_runtime_preflight(program,error,error_size))return 0;
    if(music&&!music_runtime_preflight(program,music_host,error,error_size))return 0;
    if(resources&&!resource_runtime_preflight(program,resource_host,error,error_size))return 0;
    if(objectives&&!objective_runtime_preflight(program,objective_host,error,error_size))return 0;
    if(console&&!console_runtime_preflight(program,console_host,error,error_size))return 0;
    if(hud&&!hud_runtime_preflight(program,hud_host,error,error_size))return 0;
    if(mission&&!mission_runtime_preflight(program,mission_host,error,error_size))return 0;
    if(blast&&!blast_runtime_preflight(program,blast_host,error,error_size))return 0;
    if(quake&&!quake_runtime_preflight(program,quake_host,error,error_size))return 0;
    if(fx&&!fx_runtime_preflight(program,fx_host,error,error_size))return 0;
    if(cvars&&!cvar_runtime_preflight(program,cvar_host,error,error_size))return 0;
    if(shellshock&&!shellshock_runtime_preflight(program,shellshock_host,error,error_size))return 0;
    if(debug_text&&!debug_text_runtime_preflight(program,debug_text_host,error,error_size))return 0;
    if(debug_line&&!debug_line_runtime_preflight(program,debug_line_host,error,error_size))return 0;
    if(ambient&&!ambient_runtime_preflight(program,ambient_host,error,error_size))return 0;
    if(precache_item&&!precache_item_runtime_preflight(program,precache_item_host,error,error_size))return 0;
    if(save_game&&!save_game_runtime_preflight(program,save_game_host,error,error_size))return 0;
    if(tag_fx&&!tag_fx_runtime_preflight(program,tag_fx_host,error,error_size))return 0;
    if(native_queries&&!native_queries_runtime_preflight(program,native_query_host,error,error_size))return 0;
    if(spawner_arrays&&!spawner_arrays_runtime_preflight(program,spawner_array_host,error,error_size))return 0;
    if(randomint&&!randomint_runtime_preflight(program,randomint_host,error,error_size))return 0;
    return 1;
}
static GscRuntime *runtime_new_computed_randomint_common_strict(GscRuntime *target,const GscProgram *program,GscFogCallback fog,void *user,
    const GscEntityHost *entities,const GscEntityAccessHost *access,int events,int state,
    const GscEntityControlHost *controls,int flow,int arrays,const GscEntityCollectionHost *collections,int results,int payloads,int predicates,const GscEntityPredicateHost *predicate_host,
    int actor_spawners,const GscActorSpawnHost *spawn_host,int function_refs,int scalar_switch,int lifetime,int names,int music,const GscMusicHost *music_host,int resources,const GscResourceHost *resource_host,int objectives,const GscObjectiveHost *objective_host,int console,const GscConsoleHost *console_host,int hud,const GscHudHost *hud_host,int mission,const GscMissionHost *mission_host,int blast,const GscBlastHost *blast_host,int quake,const GscQuakeHost *quake_host,int fx,const GscFxHost *fx_host,int cvars,const GscCvarHost *cvar_host,int shellshock,const GscShellshockHost *shellshock_host,int debug_text,const GscDebugTextHost *debug_text_host,int debug_line,const GscDebugLineHost *debug_line_host,int ambient,const GscAmbientHost *ambient_host,int precache_item,const GscPrecacheItemHost *precache_item_host,int save_game,const GscSaveGameHost *save_game_host,int tag_fx,const GscTagFxHost *tag_fx_host,int native_queries,const GscNativeQueryHost *native_query_host,int spawner_arrays,const GscSpawnerArrayHost *spawner_array_host,int randomint,const GscRandomIntHost *randomint_host,int computed_randomint,int structs,int entity_queries,const GscEntityQueryHost *entity_query_host,int dynamic_randomint,int numeric_randomint,int dynamic_randomfloat,char *error,size_t error_size) {
    GscRuntime *r;
    if(!runtime_admission_preflight(program,fog,user,entities,access,events,state,controls,flow,arrays,collections,results,payloads,predicates,predicate_host,actor_spawners,spawn_host,function_refs,scalar_switch,lifetime,names,music,music_host,resources,resource_host,objectives,objective_host,console,console_host,hud,hud_host,mission,mission_host,blast,blast_host,quake,quake_host,fx,fx_host,cvars,cvar_host,shellshock,shellshock_host,debug_text,debug_text_host,debug_line,debug_line_host,ambient,ambient_host,precache_item,precache_item_host,save_game,save_game_host,tag_fx,tag_fx_host,native_queries,native_query_host,spawner_arrays,spawner_array_host,randomint,randomint_host,computed_randomint,structs,entity_queries,entity_query_host,dynamic_randomint,numeric_randomint,dynamic_randomfloat,error,error_size))return NULL;
    r=target?target:GSC_RUNTIME_CALLOC(1,sizeof(*r));if(!r) {
        if(error&&error_size)snprintf(error,error_size,"out of memory");
        return NULL;
    }
    r->program=program;r->fog=fog;r->user=user;r->native_isdefined_import=-1;
    if(!target&&!state_fields_reserve_free(r,GSC_STATE_FIELD_CAPACITY,0)){state_fields_release(r);GSC_RUNTIME_FREE(r);if(error&&error_size)snprintf(error,error_size,"state arena initial allocation failed");return NULL;}
    r->native_exp_fog_import=r->native_cull_fog_import=-1;
    r->native_getent_import=r->native_setorigin_import=r->native_spawn_import=-1;
    r->native_setmodel_import=-1;
    r->native_hide_import=r->native_show_import=-1;
    if(entities) {r->entities=*entities;r->entities_enabled=1;}
    if(access) {r->access=*access;r->access_enabled=1;}
    r->events_enabled=events;
    r->state_enabled=state;
    if(controls){r->controls=*controls;r->controls_enabled=1;}
    r->flow_enabled=flow;r->arrays_enabled=arrays;r->results_enabled=results;r->event_payloads_enabled=payloads;
    if(collections)r->collections=*collections;
    r->predicates_enabled=predicates;if(predicate_host)r->predicates=*predicate_host;
    r->actor_spawners_enabled=actor_spawners;if(spawn_host)r->actor_spawners=*spawn_host;
    r->function_refs_enabled=function_refs;r->scalar_switch_enabled=scalar_switch;
    r->lifetime_enabled=lifetime;r->names_enabled=names;r->music_enabled=music;
    if(music_host)r->music=*music_host;
    r->resources_enabled=resources;r->resources_closed=1;if(resource_host)r->resources=*resource_host;
    r->objectives_enabled=objectives;if(objective_host)r->objectives=*objective_host;
    r->console_enabled=console;if(console_host)r->console=*console_host;
    r->hud_enabled=hud;if(hud_host)r->hud=*hud_host;
    r->mission_enabled=mission;if(mission_host)r->mission=*mission_host;
    r->blast_enabled=blast;r->next_blast_wait_serial=1;if(blast_host)r->blast=*blast_host;
    r->quake_enabled=quake;if(quake_host)r->quake=*quake_host;
    r->fx_enabled=fx;if(fx_host)r->fx=*fx_host;
    r->cvars_enabled=cvars;r->next_cvar_operation=1;if(cvar_host)r->cvars=*cvar_host;
    r->shellshock_enabled=shellshock;r->next_shellshock_operation=1;
    if(shellshock_host)r->shellshock=*shellshock_host;
    r->debug_text_enabled=debug_text;r->next_debug_text_operation=1;
    if(debug_text_host)r->debug_text=*debug_text_host;
    r->debug_line_enabled=debug_line;r->next_debug_line_operation=1;
    if(debug_line_host)r->debug_line=*debug_line_host;
    r->ambient_enabled=ambient;r->next_ambient_operation=1;
    if(ambient_host)r->ambient=*ambient_host;
    r->precache_item_enabled=precache_item;r->next_precache_item_operation=1;
    if(precache_item_host)r->precache_item=*precache_item_host;
    r->save_game_enabled=save_game;r->next_save_game_operation=1;
    if(save_game_host)r->save_game=*save_game_host;
    r->tag_fx_enabled=tag_fx;r->next_tag_fx_operation=1;
    if(tag_fx_host)r->tag_fx=*tag_fx_host;
    r->structs_enabled=structs;
    r->entity_queries_enabled=entity_queries;if(entity_query_host)r->entity_queries=*entity_query_host;
    r->native_queries_enabled=native_queries;r->next_native_query_operation=1;
    if(native_query_host)r->native_queries=*native_query_host;
    r->spawner_arrays_enabled=spawner_arrays;if(spawner_array_host)r->spawner_arrays=*spawner_array_host;
    r->computed_randomint_enabled=computed_randomint;
    r->dynamic_randomint_enabled=dynamic_randomint;
    r->numeric_randomint_enabled=numeric_randomint;
    r->dynamic_randomfloat_enabled=dynamic_randomfloat;
    r->randomint_enabled=randomint;r->next_randomint_operation=1;if(randomint_host)r->randomint=*randomint_host;
    if(arrays&&!target&&!array_token_new(&r->array_token)) {
        state_fields_release(r);GSC_RUNTIME_FREE(r);
        if(error&&error_size)snprintf(error,error_size,"array runtime token exhausted");
        return NULL;
    }
    if(!bridge_init(r,error,error_size)) {if(!target){state_fields_release(r);GSC_RUNTIME_FREE(r);}return NULL;}
    if(target){r->entity_fields_enabled=0;memset(&r->entity_fields,0,sizeof(r->entity_fields));}
    return r;
}
static GscRuntime *runtime_new_computed_randomint_common_into(GscRuntime *target,const GscProgram *program,GscFogCallback fog,void *user,
    const GscEntityHost *entities,const GscEntityAccessHost *access,int events,int state,
    const GscEntityControlHost *controls,int flow,int arrays,const GscEntityCollectionHost *collections,int results,int payloads,int predicates,const GscEntityPredicateHost *predicate_host,
    int actor_spawners,const GscActorSpawnHost *spawn_host,int function_refs,int scalar_switch,int lifetime,int names,int music,const GscMusicHost *music_host,int resources,const GscResourceHost *resource_host,int objectives,const GscObjectiveHost *objective_host,int console,const GscConsoleHost *console_host,int hud,const GscHudHost *hud_host,int mission,const GscMissionHost *mission_host,int blast,const GscBlastHost *blast_host,int quake,const GscQuakeHost *quake_host,int fx,const GscFxHost *fx_host,int cvars,const GscCvarHost *cvar_host,int shellshock,const GscShellshockHost *shellshock_host,int debug_text,const GscDebugTextHost *debug_text_host,int debug_line,const GscDebugLineHost *debug_line_host,int ambient,const GscAmbientHost *ambient_host,int precache_item,const GscPrecacheItemHost *precache_item_host,int save_game,const GscSaveGameHost *save_game_host,int tag_fx,const GscTagFxHost *tag_fx_host,int native_queries,const GscNativeQueryHost *native_query_host,int spawner_arrays,const GscSpawnerArrayHost *spawner_array_host,int randomint,const GscRandomIntHost *randomint_host,int computed_randomint,int structs,int entity_queries,const GscEntityQueryHost *entity_query_host,int dynamic_randomint,int numeric_randomint,int dynamic_randomfloat,char *error,size_t error_size) {
    GscProgram view;GscRuntime *r=NULL;const GscProgram *original=program;char why[384]={0};int f;unsigned char admitted[MAX_FUNCS];
    int previous_function = -1, disjoint_bodies;
    unsigned char required[MAX_FUNCS]={0};char firstMissing[300]={0};
    if(!program||!program->function_admission_selected)return runtime_new_computed_randomint_common_strict(target,program,fog,user,entities,access,events,state,controls,flow,arrays,collections,results,payloads,predicates,predicate_host,actor_spawners,spawn_host,function_refs,scalar_switch,lifetime,names,music,music_host,resources,resource_host,objectives,objective_host,console,console_host,hud,hud_host,mission,mission_host,blast,blast_host,quake,quake_host,fx,fx_host,cvars,cvar_host,shellshock,shellshock_host,debug_text,debug_text_host,debug_line,debug_line_host,ambient,ambient_host,precache_item,precache_item_host,save_game,save_game_host,tag_fx,tag_fx_host,native_queries,native_query_host,spawner_arrays,spawner_array_host,randomint,randomint_host,computed_randomint,structs,entity_queries,entity_query_host,dynamic_randomint,numeric_randomint,dynamic_randomfloat,error,error_size);
    if(!program->linked||!function_admission_current(program,program->entry_fn,why,sizeof(why))){if(error&&error_size)snprintf(error,error_size,"%s",why);return NULL;}
    memset(&view,0,sizeof(view));memset(admitted,0,sizeof(admitted));if(!function_admission_view(program,&view))return NULL;
    if(!function_admission_collect(original,original->entry_fn,NULL,required,error,error_size))goto done;
    disjoint_bodies = admission_bodies_disjoint(original);
    for(f=0;f<original->func_count;f++)if(original->funcs[f].admission){
        runtime_admission_view_function(&view, original, previous_function, f, disjoint_bodies);
        previous_function = f;
        program = &view;
        admitted[f]=(unsigned char)(runtime_admission_preflight(program,fog,user,entities,access,events,state,controls,flow,arrays,collections,results,payloads,predicates,predicate_host,actor_spawners,spawn_host,function_refs,scalar_switch,lifetime,names,music,music_host,resources,resource_host,objectives,objective_host,console,console_host,hud,hud_host,mission,mission_host,blast,blast_host,quake,quake_host,fx,fx_host,cvars,cvar_host,shellshock,shellshock_host,debug_text,debug_text_host,debug_line,debug_line_host,ambient,ambient_host,precache_item,precache_item_host,save_game,save_game_host,tag_fx,tag_fx_host,native_queries,native_query_host,spawner_arrays,spawner_array_host,randomint,randomint_host,computed_randomint,structs,entity_queries,entity_query_host,dynamic_randomint,numeric_randomint,dynamic_randomfloat,error,error_size)==1);
        if(!admitted[f]&&required[f]&&!firstMissing[0])snprintf(firstMissing,sizeof(firstMissing),"%.70s::%.55s: %.160s",
            original->modules[original->funcs[f].module],original->funcs[f].name,error&&error_size?error:"");
    }
    program=original;
    if(!function_admission_current_mask(original,original->entry_fn,admitted,error,error_size)){
        if(firstMissing[0]&&error&&error_size)snprintf(error,error_size,"required function preflight: %s",firstMissing);
        goto done;
    }
    memcpy(view.code,original->code,(size_t)original->code_count*sizeof(*view.code));memcpy(view.expr,original->expr,(size_t)original->expr_count*sizeof(*view.expr));memcpy(view.funcs,original->funcs,(size_t)original->func_count*sizeof(*view.funcs));
    for(f=0;f<original->func_count;f++)view.funcs[f].admission=admitted[f];function_admission_mask(&view,-1);program=&view;
    r=runtime_new_computed_randomint_common_strict(target,program,fog,user,entities,access,events,state,controls,flow,arrays,collections,results,payloads,predicates,predicate_host,actor_spawners,spawn_host,function_refs,scalar_switch,lifetime,names,music,music_host,resources,resource_host,objectives,objective_host,console,console_host,hud,hud_host,mission,mission_host,blast,blast_host,quake,quake_host,fx,fx_host,cvars,cvar_host,shellshock,shellshock_host,debug_text,debug_text_host,debug_line,debug_line_host,ambient,ambient_host,precache_item,precache_item_host,save_game,save_game_host,tag_fx,tag_fx_host,native_queries,native_query_host,spawner_arrays,spawner_array_host,randomint,randomint_host,computed_randomint,structs,entity_queries,entity_query_host,dynamic_randomint,numeric_randomint,dynamic_randomfloat,error,error_size);
    if(r){r->program=original;memcpy(r->function_admission,admitted,sizeof(admitted));r->function_admission_token=original->function_admission_token;}
done:if(target)target->program=original;function_admission_view_free(&view);return r;
}

static GscRuntime *runtime_new_computed_randomint_common(const GscProgram *program,GscFogCallback fog,void *user,
    const GscEntityHost *entities,const GscEntityAccessHost *access,int events,int state,
    const GscEntityControlHost *controls,int flow,int arrays,const GscEntityCollectionHost *collections,int results,int payloads,int predicates,const GscEntityPredicateHost *predicate_host,
    int actor_spawners,const GscActorSpawnHost *spawn_host,int function_refs,int scalar_switch,int lifetime,int names,int music,const GscMusicHost *music_host,int resources,const GscResourceHost *resource_host,int objectives,const GscObjectiveHost *objective_host,int console,const GscConsoleHost *console_host,int hud,const GscHudHost *hud_host,int mission,const GscMissionHost *mission_host,int blast,const GscBlastHost *blast_host,int quake,const GscQuakeHost *quake_host,int fx,const GscFxHost *fx_host,int cvars,const GscCvarHost *cvar_host,int shellshock,const GscShellshockHost *shellshock_host,int debug_text,const GscDebugTextHost *debug_text_host,int debug_line,const GscDebugLineHost *debug_line_host,int ambient,const GscAmbientHost *ambient_host,int precache_item,const GscPrecacheItemHost *precache_item_host,int save_game,const GscSaveGameHost *save_game_host,int tag_fx,const GscTagFxHost *tag_fx_host,int native_queries,const GscNativeQueryHost *native_query_host,int spawner_arrays,const GscSpawnerArrayHost *spawner_array_host,int randomint,const GscRandomIntHost *randomint_host,int computed_randomint,int structs,int entity_queries,const GscEntityQueryHost *entity_query_host,int dynamic_randomint,int numeric_randomint,int dynamic_randomfloat,char *error,size_t error_size) {
 return runtime_new_computed_randomint_common_into(NULL,program,fog,user,entities,access,events,state,controls,flow,arrays,collections,results,payloads,predicates,predicate_host,actor_spawners,spawn_host,function_refs,scalar_switch,lifetime,names,music,music_host,resources,resource_host,objectives,objective_host,console,console_host,hud,hud_host,mission,mission_host,blast,blast_host,quake,quake_host,fx,fx_host,cvars,cvar_host,shellshock,shellshock_host,debug_text,debug_text_host,debug_line,debug_line_host,ambient,ambient_host,precache_item,precache_item_host,save_game,save_game_host,tag_fx,tag_fx_host,native_queries,native_query_host,spawner_arrays,spawner_array_host,randomint,randomint_host,computed_randomint,structs,entity_queries,entity_query_host,dynamic_randomint,numeric_randomint,dynamic_randomfloat,error,error_size);
}

static GscRuntime *runtime_new_native_queries_common(const GscProgram *program,GscFogCallback fog,void *user,
    const GscEntityHost *entities,const GscEntityAccessHost *access,int events,int state,
    const GscEntityControlHost *controls,int flow,int arrays,const GscEntityCollectionHost *collections,int results,int payloads,int predicates,const GscEntityPredicateHost *predicate_host,
    int actor_spawners,const GscActorSpawnHost *spawn_host,int function_refs,int scalar_switch,int lifetime,int names,int music,const GscMusicHost *music_host,int resources,const GscResourceHost *resource_host,int objectives,const GscObjectiveHost *objective_host,int console,const GscConsoleHost *console_host,int hud,const GscHudHost *hud_host,int mission,const GscMissionHost *mission_host,int blast,const GscBlastHost *blast_host,int quake,const GscQuakeHost *quake_host,int fx,const GscFxHost *fx_host,int cvars,const GscCvarHost *cvar_host,int shellshock,const GscShellshockHost *shellshock_host,int debug_text,const GscDebugTextHost *debug_text_host,int debug_line,const GscDebugLineHost *debug_line_host,int ambient,const GscAmbientHost *ambient_host,int precache_item,const GscPrecacheItemHost *precache_item_host,int save_game,const GscSaveGameHost *save_game_host,int tag_fx,const GscTagFxHost *tag_fx_host,int native_queries,const GscNativeQueryHost *native_query_host,int spawner_arrays,const GscSpawnerArrayHost *spawner_array_host,int randomint,const GscRandomIntHost *randomint_host,char *error,size_t error_size) {
    return runtime_new_computed_randomint_common(program,fog,user,entities,access,events,state,controls,flow,arrays,collections,results,payloads,predicates,predicate_host,actor_spawners,spawn_host,function_refs,scalar_switch,lifetime,names,music,music_host,resources,resource_host,objectives,objective_host,console,console_host,hud,hud_host,mission,mission_host,blast,blast_host,quake,quake_host,fx,fx_host,cvars,cvar_host,shellshock,shellshock_host,debug_text,debug_text_host,debug_line,debug_line_host,ambient,ambient_host,precache_item,precache_item_host,save_game,save_game_host,tag_fx,tag_fx_host,native_queries,native_query_host,spawner_arrays,spawner_array_host,randomint,randomint_host,0,0,0,NULL,0,0,0,error,error_size);
}
static GscRuntime *runtime_new_tag_fx_common(const GscProgram *program,GscFogCallback fog,void *user,
    const GscEntityHost *entities,const GscEntityAccessHost *access,int events,int state,
    const GscEntityControlHost *controls,int flow,int arrays,const GscEntityCollectionHost *collections,int results,int payloads,int predicates,const GscEntityPredicateHost *predicate_host,
    int actor_spawners,const GscActorSpawnHost *spawn_host,int function_refs,int scalar_switch,int lifetime,int names,int music,const GscMusicHost *music_host,int resources,const GscResourceHost *resource_host,int objectives,const GscObjectiveHost *objective_host,int console,const GscConsoleHost *console_host,int hud,const GscHudHost *hud_host,int mission,const GscMissionHost *mission_host,int blast,const GscBlastHost *blast_host,int quake,const GscQuakeHost *quake_host,int fx,const GscFxHost *fx_host,int cvars,const GscCvarHost *cvar_host,int shellshock,const GscShellshockHost *shellshock_host,int debug_text,const GscDebugTextHost *debug_text_host,int debug_line,const GscDebugLineHost *debug_line_host,int ambient,const GscAmbientHost *ambient_host,int precache_item,const GscPrecacheItemHost *precache_item_host,int save_game,const GscSaveGameHost *save_game_host,int tag_fx,const GscTagFxHost *tag_fx_host,char *error,size_t error_size) {
    return runtime_new_native_queries_common(program,fog,user,entities,access,events,state,controls,flow,arrays,collections,results,payloads,predicates,predicate_host,actor_spawners,spawn_host,function_refs,scalar_switch,lifetime,names,music,music_host,resources,resource_host,objectives,objective_host,console,console_host,hud,hud_host,mission,mission_host,blast,blast_host,quake,quake_host,fx,fx_host,cvars,cvar_host,shellshock,shellshock_host,debug_text,debug_text_host,debug_line,debug_line_host,ambient,ambient_host,precache_item,precache_item_host,save_game,save_game_host,tag_fx,tag_fx_host,0,NULL,0,NULL,0,NULL,error,error_size);
}

/* Retain R123 and earlier private capability selections. */
static GscRuntime *runtime_new_save_game_common(const GscProgram *program,GscFogCallback fog,void *user,
    const GscEntityHost *entities,const GscEntityAccessHost *access,int events,int state,
    const GscEntityControlHost *controls,int flow,int arrays,const GscEntityCollectionHost *collections,int results,int payloads,int predicates,const GscEntityPredicateHost *predicate_host,
    int actor_spawners,const GscActorSpawnHost *spawn_host,int function_refs,int scalar_switch,int lifetime,int names,int music,const GscMusicHost *music_host,int resources,const GscResourceHost *resource_host,int objectives,const GscObjectiveHost *objective_host,int console,const GscConsoleHost *console_host,int hud,const GscHudHost *hud_host,int mission,const GscMissionHost *mission_host,int blast,const GscBlastHost *blast_host,int quake,const GscQuakeHost *quake_host,int fx,const GscFxHost *fx_host,int cvars,const GscCvarHost *cvar_host,int shellshock,const GscShellshockHost *shellshock_host,int debug_text,const GscDebugTextHost *debug_text_host,int debug_line,const GscDebugLineHost *debug_line_host,int ambient,const GscAmbientHost *ambient_host,int precache_item,const GscPrecacheItemHost *precache_item_host,int save_game,const GscSaveGameHost *save_game_host,char *error,size_t error_size) {
    return runtime_new_tag_fx_common(program,fog,user,entities,access,events,state,controls,flow,arrays,collections,results,payloads,predicates,predicate_host,actor_spawners,spawn_host,function_refs,scalar_switch,lifetime,names,music,music_host,resources,resource_host,objectives,objective_host,console,console_host,hud,hud_host,mission,mission_host,blast,blast_host,quake,quake_host,fx,fx_host,cvars,cvar_host,shellshock,shellshock_host,debug_text,debug_text_host,debug_line,debug_line_host,ambient,ambient_host,precache_item,precache_item_host,save_game,save_game_host,0,NULL,error,error_size);
}
/* Preserve R121/R122 constructor selection. */
static GscRuntime *runtime_new_precache_item_common(const GscProgram *program,GscFogCallback fog,void *user,
    const GscEntityHost *entities,const GscEntityAccessHost *access,int events,int state,
    const GscEntityControlHost *controls,int flow,int arrays,const GscEntityCollectionHost *collections,int results,int payloads,int predicates,const GscEntityPredicateHost *predicate_host,
    int actor_spawners,const GscActorSpawnHost *spawn_host,int function_refs,int scalar_switch,int lifetime,int names,int music,const GscMusicHost *music_host,int resources,const GscResourceHost *resource_host,int objectives,const GscObjectiveHost *objective_host,int console,const GscConsoleHost *console_host,int hud,const GscHudHost *hud_host,int mission,const GscMissionHost *mission_host,int blast,const GscBlastHost *blast_host,int quake,const GscQuakeHost *quake_host,int fx,const GscFxHost *fx_host,int cvars,const GscCvarHost *cvar_host,int shellshock,const GscShellshockHost *shellshock_host,int debug_text,const GscDebugTextHost *debug_text_host,int debug_line,const GscDebugLineHost *debug_line_host,int ambient,const GscAmbientHost *ambient_host,int precache_item,const GscPrecacheItemHost *precache_item_host,char *error,size_t error_size) {
    return runtime_new_save_game_common(program,fog,user,entities,access,events,state,controls,flow,arrays,collections,
        results,payloads,predicates,predicate_host,actor_spawners,spawn_host,function_refs,scalar_switch,lifetime,names,
        music,music_host,resources,resource_host,objectives,objective_host,console,console_host,hud,hud_host,mission,mission_host,
        blast,blast_host,quake,quake_host,fx,fx_host,cvars,cvar_host,shellshock,shellshock_host,debug_text,debug_text_host,
        debug_line,debug_line_host,ambient,ambient_host,precache_item,precache_item_host,0,NULL,error,error_size);
}
/* Retain R120 constructors with no precacheItem selection. */
static GscRuntime *runtime_new_ambient_common(const GscProgram *program,GscFogCallback fog,void *user,
    const GscEntityHost *entities,const GscEntityAccessHost *access,int events,int state,
    const GscEntityControlHost *controls,int flow,int arrays,const GscEntityCollectionHost *collections,int results,int payloads,int predicates,const GscEntityPredicateHost *predicate_host,
    int actor_spawners,const GscActorSpawnHost *spawn_host,int function_refs,int scalar_switch,int lifetime,int names,int music,const GscMusicHost *music_host,int resources,const GscResourceHost *resource_host,int objectives,const GscObjectiveHost *objective_host,int console,const GscConsoleHost *console_host,int hud,const GscHudHost *hud_host,int mission,const GscMissionHost *mission_host,int blast,const GscBlastHost *blast_host,int quake,const GscQuakeHost *quake_host,int fx,const GscFxHost *fx_host,int cvars,const GscCvarHost *cvar_host,int shellshock,const GscShellshockHost *shellshock_host,int debug_text,const GscDebugTextHost *debug_text_host,int debug_line,const GscDebugLineHost *debug_line_host,int ambient,const GscAmbientHost *ambient_host,char *error,size_t error_size) {
    return runtime_new_precache_item_common(program,fog,user,entities,access,events,state,controls,flow,arrays,collections,
        results,payloads,predicates,predicate_host,actor_spawners,spawn_host,function_refs,scalar_switch,lifetime,names,
        music,music_host,resources,resource_host,objectives,objective_host,console,console_host,hud,hud_host,mission,mission_host,
        blast,blast_host,quake,quake_host,fx,fx_host,cvars,cvar_host,shellshock,shellshock_host,debug_text,debug_text_host,
        debug_line,debug_line_host,ambient,ambient_host,0,NULL,error,error_size);
}
/* Retain R119 constructors with no ambient selection. */
static GscRuntime *runtime_new_debug_line_common(const GscProgram *program,GscFogCallback fog,void *user,
    const GscEntityHost *entities,const GscEntityAccessHost *access,int events,int state,
    const GscEntityControlHost *controls,int flow,int arrays,const GscEntityCollectionHost *collections,int results,int payloads,int predicates,const GscEntityPredicateHost *predicate_host,
    int actor_spawners,const GscActorSpawnHost *spawn_host,int function_refs,int scalar_switch,int lifetime,int names,int music,const GscMusicHost *music_host,int resources,const GscResourceHost *resource_host,int objectives,const GscObjectiveHost *objective_host,int console,const GscConsoleHost *console_host,int hud,const GscHudHost *hud_host,int mission,const GscMissionHost *mission_host,int blast,const GscBlastHost *blast_host,int quake,const GscQuakeHost *quake_host,int fx,const GscFxHost *fx_host,int cvars,const GscCvarHost *cvar_host,int shellshock,const GscShellshockHost *shellshock_host,int debug_text,const GscDebugTextHost *debug_text_host,int debug_line,const GscDebugLineHost *debug_line_host,char *error,size_t error_size) {
    return runtime_new_ambient_common(program,fog,user,entities,access,events,state,controls,flow,arrays,collections,
        results,payloads,predicates,predicate_host,actor_spawners,spawn_host,function_refs,scalar_switch,lifetime,names,
        music,music_host,resources,resource_host,objectives,objective_host,console,console_host,hud,hud_host,mission,mission_host,
        blast,blast_host,quake,quake_host,fx,fx_host,cvars,cvar_host,shellshock,shellshock_host,debug_text,debug_text_host,
        debug_line,debug_line_host,0,NULL,error,error_size);
}
/* Retain R118 constructors with no debug line selection. */
static GscRuntime *runtime_new_debug_text_common(const GscProgram *program,GscFogCallback fog,void *user,
    const GscEntityHost *entities,const GscEntityAccessHost *access,int events,int state,
    const GscEntityControlHost *controls,int flow,int arrays,const GscEntityCollectionHost *collections,int results,int payloads,int predicates,const GscEntityPredicateHost *predicate_host,
    int actor_spawners,const GscActorSpawnHost *spawn_host,int function_refs,int scalar_switch,int lifetime,int names,int music,const GscMusicHost *music_host,int resources,const GscResourceHost *resource_host,int objectives,const GscObjectiveHost *objective_host,int console,const GscConsoleHost *console_host,int hud,const GscHudHost *hud_host,int mission,const GscMissionHost *mission_host,int blast,const GscBlastHost *blast_host,int quake,const GscQuakeHost *quake_host,int fx,const GscFxHost *fx_host,int cvars,const GscCvarHost *cvar_host,int shellshock,const GscShellshockHost *shellshock_host,int debug_text,const GscDebugTextHost *debug_text_host,char *error,size_t error_size) {
    return runtime_new_debug_line_common(program,fog,user,entities,access,events,state,controls,flow,arrays,collections,
        results,payloads,predicates,predicate_host,actor_spawners,spawn_host,function_refs,scalar_switch,lifetime,names,
        music,music_host,resources,resource_host,objectives,objective_host,console,console_host,hud,hud_host,mission,mission_host,
        blast,blast_host,quake,quake_host,fx,fx_host,cvars,cvar_host,shellshock,shellshock_host,debug_text,debug_text_host,0,NULL,error,error_size);
}
/* Retain R117 constructors with no debug text selection. */
static GscRuntime *runtime_new_shellshock_common(const GscProgram *program,GscFogCallback fog,void *user,
    const GscEntityHost *entities,const GscEntityAccessHost *access,int events,int state,
    const GscEntityControlHost *controls,int flow,int arrays,const GscEntityCollectionHost *collections,int results,int payloads,int predicates,const GscEntityPredicateHost *predicate_host,
    int actor_spawners,const GscActorSpawnHost *spawn_host,int function_refs,int scalar_switch,int lifetime,int names,int music,const GscMusicHost *music_host,int resources,const GscResourceHost *resource_host,int objectives,const GscObjectiveHost *objective_host,int console,const GscConsoleHost *console_host,int hud,const GscHudHost *hud_host,int mission,const GscMissionHost *mission_host,int blast,const GscBlastHost *blast_host,int quake,const GscQuakeHost *quake_host,int fx,const GscFxHost *fx_host,int cvars,const GscCvarHost *cvar_host,int shellshock,const GscShellshockHost *shellshock_host,char *error,size_t error_size) {
    return runtime_new_debug_text_common(program,fog,user,entities,access,events,state,controls,flow,arrays,collections,
        results,payloads,predicates,predicate_host,actor_spawners,spawn_host,function_refs,scalar_switch,lifetime,names,
        music,music_host,resources,resource_host,objectives,objective_host,console,console_host,hud,hud_host,mission,mission_host,
        blast,blast_host,quake,quake_host,fx,fx_host,cvars,cvar_host,shellshock,shellshock_host,0,NULL,error,error_size);
}
/* Retain R116 constructors with no shellshock selection. */
static GscRuntime *runtime_new_cvars_common(const GscProgram *program,GscFogCallback fog,void *user,
    const GscEntityHost *entities,const GscEntityAccessHost *access,int events,int state,
    const GscEntityControlHost *controls,int flow,int arrays,const GscEntityCollectionHost *collections,int results,int payloads,int predicates,const GscEntityPredicateHost *predicate_host,
    int actor_spawners,const GscActorSpawnHost *spawn_host,int function_refs,int scalar_switch,int lifetime,int names,int music,const GscMusicHost *music_host,int resources,const GscResourceHost *resource_host,int objectives,const GscObjectiveHost *objective_host,int console,const GscConsoleHost *console_host,int hud,const GscHudHost *hud_host,int mission,const GscMissionHost *mission_host,int blast,const GscBlastHost *blast_host,int quake,const GscQuakeHost *quake_host,int fx,const GscFxHost *fx_host,int cvars,const GscCvarHost *cvar_host,char *error,size_t error_size) {
    return runtime_new_shellshock_common(program,fog,user,entities,access,events,state,controls,flow,arrays,collections,
        results,payloads,predicates,predicate_host,actor_spawners,spawn_host,function_refs,scalar_switch,lifetime,names,
        music,music_host,resources,resource_host,objectives,objective_host,console,console_host,hud,hud_host,mission,mission_host,blast,blast_host,quake,quake_host,fx,fx_host,cvars,cvar_host,0,NULL,error,error_size);
}
/* Retain the R115 private signature and select no cvar publications. */
static GscRuntime *runtime_new_fx_common(const GscProgram *program,GscFogCallback fog,void *user,
    const GscEntityHost *entities,const GscEntityAccessHost *access,int events,int state,
    const GscEntityControlHost *controls,int flow,int arrays,const GscEntityCollectionHost *collections,int results,int payloads,int predicates,const GscEntityPredicateHost *predicate_host,
    int actor_spawners,const GscActorSpawnHost *spawn_host,int function_refs,int scalar_switch,int lifetime,int names,int music,const GscMusicHost *music_host,int resources,const GscResourceHost *resource_host,int objectives,const GscObjectiveHost *objective_host,int console,const GscConsoleHost *console_host,int hud,const GscHudHost *hud_host,int mission,const GscMissionHost *mission_host,int blast,const GscBlastHost *blast_host,int quake,const GscQuakeHost *quake_host,int fx,const GscFxHost *fx_host,char *error,size_t error_size) {
    return runtime_new_cvars_common(program,fog,user,entities,access,events,state,controls,flow,arrays,collections,
        results,payloads,predicates,predicate_host,actor_spawners,spawn_host,function_refs,scalar_switch,lifetime,names,
        music,music_host,resources,resource_host,objectives,objective_host,console,console_host,hud,hud_host,mission,mission_host,blast,blast_host,quake,quake_host,fx,fx_host,0,NULL,error,error_size);
}
/* Retain the R114 private signature and select no FX capability. */
static GscRuntime *runtime_new_quake_common(const GscProgram *program,GscFogCallback fog,void *user,
    const GscEntityHost *entities,const GscEntityAccessHost *access,int events,int state,
    const GscEntityControlHost *controls,int flow,int arrays,const GscEntityCollectionHost *collections,int results,int payloads,int predicates,const GscEntityPredicateHost *predicate_host,
    int actor_spawners,const GscActorSpawnHost *spawn_host,int function_refs,int scalar_switch,int lifetime,int names,int music,const GscMusicHost *music_host,int resources,const GscResourceHost *resource_host,int objectives,const GscObjectiveHost *objective_host,int console,const GscConsoleHost *console_host,int hud,const GscHudHost *hud_host,int mission,const GscMissionHost *mission_host,int blast,const GscBlastHost *blast_host,int quake,const GscQuakeHost *quake_host,char *error,size_t error_size) {
    return runtime_new_fx_common(program,fog,user,entities,access,events,state,controls,flow,arrays,collections,
        results,payloads,predicates,predicate_host,actor_spawners,spawn_host,function_refs,scalar_switch,lifetime,names,
        music,music_host,resources,resource_host,objectives,objective_host,console,console_host,hud,hud_host,mission,mission_host,blast,blast_host,quake,quake_host,0,NULL,error,error_size);
}
/* Retain the pre-R114 private signature and select no quake effects. */
static GscRuntime *runtime_new_blast_common(const GscProgram *program,GscFogCallback fog,void *user,
    const GscEntityHost *entities,const GscEntityAccessHost *access,int events,int state,
    const GscEntityControlHost *controls,int flow,int arrays,const GscEntityCollectionHost *collections,int results,int payloads,int predicates,const GscEntityPredicateHost *predicate_host,
    int actor_spawners,const GscActorSpawnHost *spawn_host,int function_refs,int scalar_switch,int lifetime,int names,int music,const GscMusicHost *music_host,int resources,const GscResourceHost *resource_host,int objectives,const GscObjectiveHost *objective_host,int console,const GscConsoleHost *console_host,int hud,const GscHudHost *hud_host,int mission,const GscMissionHost *mission_host,int blast,const GscBlastHost *blast_host,char *error,size_t error_size) {
    return runtime_new_quake_common(program,fog,user,entities,access,events,state,controls,flow,arrays,collections,
        results,payloads,predicates,predicate_host,actor_spawners,spawn_host,function_refs,scalar_switch,lifetime,names,
        music,music_host,resources,resource_host,objectives,objective_host,console,console_host,hud,hud_host,mission,mission_host,blast,blast_host,0,NULL,error,error_size);
}
/* Retain the pre-R113 private call signature and select no blast effects. */
static GscRuntime *runtime_new_mission_common(const GscProgram *program,GscFogCallback fog,void *user,
    const GscEntityHost *entities,const GscEntityAccessHost *access,int events,int state,
    const GscEntityControlHost *controls,int flow,int arrays,const GscEntityCollectionHost *collections,int results,int payloads,int predicates,const GscEntityPredicateHost *predicate_host,
    int actor_spawners,const GscActorSpawnHost *spawn_host,int function_refs,int scalar_switch,int lifetime,int names,int music,const GscMusicHost *music_host,int resources,const GscResourceHost *resource_host,int objectives,const GscObjectiveHost *objective_host,int console,const GscConsoleHost *console_host,int hud,const GscHudHost *hud_host,int mission,const GscMissionHost *mission_host,char *error,size_t error_size) {
    return runtime_new_blast_common(program,fog,user,entities,access,events,state,controls,flow,arrays,collections,
        results,payloads,predicates,predicate_host,actor_spawners,spawn_host,function_refs,scalar_switch,lifetime,names,
        music,music_host,resources,resource_host,objectives,objective_host,console,console_host,hud,hud_host,mission,mission_host,0,NULL,error,error_size);
}
/* Keep the pre-R112 private call signature; it selects no mission effects. */
static GscRuntime *runtime_new_hud_common(const GscProgram *program,GscFogCallback fog,void *user,
    const GscEntityHost *entities,const GscEntityAccessHost *access,int events,int state,
    const GscEntityControlHost *controls,int flow,int arrays,const GscEntityCollectionHost *collections,int results,int payloads,int predicates,const GscEntityPredicateHost *predicate_host,
    int actor_spawners,const GscActorSpawnHost *spawn_host,int function_refs,int scalar_switch,int lifetime,int names,int music,const GscMusicHost *music_host,int resources,const GscResourceHost *resource_host,int objectives,const GscObjectiveHost *objective_host,int console,const GscConsoleHost *console_host,int hud,const GscHudHost *hud_host,char *error,size_t error_size) {
    return runtime_new_mission_common(program,fog,user,entities,access,events,state,controls,flow,arrays,collections,
        results,payloads,predicates,predicate_host,actor_spawners,spawn_host,function_refs,scalar_switch,lifetime,names,
        music,music_host,resources,resource_host,objectives,objective_host,console,console_host,hud,hud_host,0,NULL,error,error_size);
}
static GscRuntime *runtime_new_console_common(const GscProgram *program,GscFogCallback fog,void *user,
    const GscEntityHost *entities,const GscEntityAccessHost *access,int events,int state,
    const GscEntityControlHost *controls,int flow,int arrays,const GscEntityCollectionHost *collections,int results,int payloads,int predicates,const GscEntityPredicateHost *predicate_host,
    int actor_spawners,const GscActorSpawnHost *spawn_host,int function_refs,int scalar_switch,int lifetime,int names,int music,const GscMusicHost *music_host,int resources,const GscResourceHost *resource_host,int objectives,const GscObjectiveHost *objective_host,int console,const GscConsoleHost *console_host,char *error,size_t error_size) {
    return runtime_new_hud_common(program,fog,user,entities,access,events,state,controls,flow,arrays,collections,
        results,payloads,predicates,predicate_host,actor_spawners,spawn_host,function_refs,scalar_switch,lifetime,names,
        music,music_host,resources,resource_host,objectives,objective_host,console,console_host,0,NULL,error,error_size);
}
/* Retained private call signature selects no new console semantics. */
static GscRuntime *runtime_new(const GscProgram *program,GscFogCallback fog,void *user,
    const GscEntityHost *entities,const GscEntityAccessHost *access,int events,int state,
    const GscEntityControlHost *controls,int flow,int arrays,const GscEntityCollectionHost *collections,int results,int payloads,int predicates,const GscEntityPredicateHost *predicate_host,
    int actor_spawners,const GscActorSpawnHost *spawn_host,int function_refs,int scalar_switch,int lifetime,int names,int music,const GscMusicHost *music_host,int resources,const GscResourceHost *resource_host,int objectives,const GscObjectiveHost *objective_host,char *error,size_t error_size) {
    return runtime_new_console_common(program,fog,user,entities,access,events,state,controls,flow,arrays,collections,
        results,payloads,predicates,predicate_host,actor_spawners,spawn_host,function_refs,scalar_switch,lifetime,names,
        music,music_host,resources,resource_host,objectives,objective_host,0,NULL,error,error_size);
}
GscRuntime *gsc_runtime_new(const GscProgram *program,GscFogCallback fog,void *user,
                            char *error,size_t error_size) {
    return runtime_new(program,fog,user,NULL,NULL,0,0,NULL,0,0,NULL,0,0,0,NULL,0,NULL,0,0,0,0,0,NULL,0,NULL,0,NULL,error,error_size);
}
GscRuntime *gsc_runtime_new_with_entities(const GscProgram *program,GscFogCallback fog,void *user,
    const GscEntityHost *entities,char *error,size_t error_size) {
    if(!entities) {
        if(error&&error_size)snprintf(error,error_size,"entity host is required");
        return NULL;
    }
    return runtime_new(program,fog,user,entities,NULL,0,0,NULL,0,0,NULL,0,0,0,NULL,0,NULL,0,0,0,0,0,NULL,0,NULL,0,NULL,error,error_size);
}
GscRuntime *gsc_runtime_new_with_access(const GscProgram *program,GscFogCallback fog,void *user,
    const GscEntityHost *entities,const GscEntityAccessHost *access,char *error,size_t error_size) {
    if(!entities||!access) {
        if(error&&error_size)snprintf(error,error_size,"entity and access hosts are required");
        return NULL;
    }
    return runtime_new(program,fog,user,entities,access,0,0,NULL,0,0,NULL,0,0,0,NULL,0,NULL,0,0,0,0,0,NULL,0,NULL,0,NULL,error,error_size);
}
GscRuntime *gsc_runtime_new_with_events(const GscProgram *program,GscFogCallback fog,void *user,
    const GscEntityHost *entities,const GscEntityAccessHost *access,char *error,size_t error_size) {
    if(!entities||!access) {
        if(error&&error_size)snprintf(error,error_size,"entity and access hosts are required");
        return NULL;
    }
    return runtime_new(program,fog,user,entities,access,1,0,NULL,0,0,NULL,0,0,0,NULL,0,NULL,0,0,0,0,0,NULL,0,NULL,0,NULL,error,error_size);
}
GscRuntime *gsc_runtime_new_with_state(const GscProgram *program,GscFogCallback fog,void *user,
    const GscEntityHost *entities,const GscEntityAccessHost *access,char *error,size_t error_size) {
    if(!entities||!access) {
        if(error&&error_size)snprintf(error,error_size,"entity and access hosts are required");
        return NULL;
    }
    return runtime_new(program,fog,user,entities,access,1,1,NULL,0,0,NULL,0,0,0,NULL,0,NULL,0,0,0,0,0,NULL,0,NULL,0,NULL,error,error_size);
}
GscRuntime *gsc_runtime_new_with_controls(const GscProgram *program,GscFogCallback fog,void *user,
    const GscEntityHost *entities,const GscEntityAccessHost *access,
    const GscEntityControlHost *controls,char *error,size_t error_size) {
    if(!entities||!access||!controls) {
        if(error&&error_size)snprintf(error,error_size,"entity, access and control hosts are required");
        return NULL;
    }
    return runtime_new(program,fog,user,entities,access,1,1,controls,0,0,NULL,0,0,0,NULL,0,NULL,0,0,0,0,0,NULL,0,NULL,0,NULL,error,error_size);
}
GscRuntime *gsc_runtime_new_with_flow(const GscProgram *program,GscFogCallback fog,void *user,
    const GscEntityHost *entities,const GscEntityAccessHost *access,
    const GscEntityControlHost *controls,char *error,size_t error_size) {
    if(!entities||!access||!controls) {
        if(error&&error_size)snprintf(error,error_size,"entity, access and control hosts are required");
        return NULL;
    }
    return runtime_new(program,fog,user,entities,access,1,1,controls,1,0,NULL,0,0,0,NULL,0,NULL,0,0,0,0,0,NULL,0,NULL,0,NULL,error,error_size);
}
GscRuntime *gsc_runtime_new_with_arrays(const GscProgram *program,GscFogCallback fog,void *user,
    const GscEntityHost *entities,const GscEntityAccessHost *access,
    const GscEntityControlHost *controls,const GscEntityCollectionHost *collections,
    char *error,size_t error_size) {
    if(!entities||!access||!controls) {
        if(error&&error_size)snprintf(error,error_size,"entity, access and control hosts are required");
        return NULL;
    }
    return runtime_new(program,fog,user,entities,access,1,1,controls,1,1,collections,0,0,0,NULL,0,NULL,0,0,0,0,0,NULL,0,NULL,0,NULL,error,error_size);
}
GscRuntime *gsc_runtime_new_with_results(const GscProgram *program,GscFogCallback fog,void *user,
    const GscEntityHost *entities,const GscEntityAccessHost *access,
    const GscEntityControlHost *controls,const GscEntityCollectionHost *collections,
    char *error,size_t error_size) {
    if(!entities||!access||!controls) {
        if(error&&error_size)snprintf(error,error_size,"entity, access and control hosts are required");
        return NULL;
    }
    return runtime_new(program,fog,user,entities,access,1,1,controls,1,1,collections,1,0,0,NULL,0,NULL,0,0,0,0,0,NULL,0,NULL,0,NULL,error,error_size);
}
GscRuntime *gsc_runtime_new_with_event_payloads(const GscProgram *program,GscFogCallback fog,void *user,
    const GscEntityHost *entities,const GscEntityAccessHost *access,
    const GscEntityControlHost *controls,const GscEntityCollectionHost *collections,
    char *error,size_t error_size) {
    if(!entities||!access||!controls) {
        if(error&&error_size)snprintf(error,error_size,"entity, access and control hosts are required");
        return NULL;
    }
    return runtime_new(program,fog,user,entities,access,1,1,controls,1,1,collections,1,1,0,NULL,0,NULL,0,0,0,0,0,NULL,0,NULL,0,NULL,error,error_size);
}

GscRuntime *gsc_runtime_new_with_predicates(const GscProgram *program,GscFogCallback fog,void *user,
    const GscEntityHost *entities,const GscEntityAccessHost *access,
    const GscEntityControlHost *controls,const GscEntityCollectionHost *collections,
    const GscEntityPredicateHost *predicates,char *error,size_t error_size) {
    if(!entities||!access||!controls) {
        if(error&&error_size)snprintf(error,error_size,"entity, access and control hosts are required");
        return NULL;
    }
    return runtime_new(program,fog,user,entities,access,1,1,controls,1,1,collections,1,1,1,predicates,0,NULL,0,0,0,0,0,NULL,0,NULL,0,NULL,error,error_size);
}

GscRuntime *gsc_runtime_new_with_actor_spawners(const GscProgram *program,GscFogCallback fog,void *user,
    const GscEntityHost *entities,const GscEntityAccessHost *access,
    const GscEntityControlHost *controls,const GscEntityCollectionHost *collections,
    const GscEntityPredicateHost *predicates,const GscActorSpawnHost *actor_spawners,
    char *error,size_t error_size) {
    if(!entities||!access||!controls) {
        if(error&&error_size)snprintf(error,error_size,"entity, access and control hosts are required");
        return NULL;
    }
    return runtime_new(program,fog,user,entities,access,1,1,controls,1,1,collections,
                       1,1,1,predicates,1,actor_spawners,0,0,0,0,0,NULL,0,NULL,0,NULL,error,error_size);
}

GscRuntime *gsc_runtime_new_with_function_refs(const GscProgram *program,GscFogCallback fog,void *user,
    const GscEntityHost *entities,const GscEntityAccessHost *access,
    const GscEntityControlHost *controls,const GscEntityCollectionHost *collections,
    const GscEntityPredicateHost *predicates,const GscActorSpawnHost *actor_spawners,
    char *error,size_t error_size) {
    if(!entities||!access||!controls) {
        if(error&&error_size)snprintf(error,error_size,"entity, access and control hosts are required");
        return NULL;
    }
    return runtime_new(program,fog,user,entities,access,1,1,controls,1,1,collections,
                       1,1,1,predicates,1,actor_spawners,1,0,0,0,0,NULL,0,NULL,0,NULL,error,error_size);
}

GscRuntime *gsc_runtime_new_with_scalar_switch(const GscProgram *program,GscFogCallback fog,void *user,
    const GscEntityHost *entities,const GscEntityAccessHost *access,
    const GscEntityControlHost *controls,const GscEntityCollectionHost *collections,
    const GscEntityPredicateHost *predicates,const GscActorSpawnHost *actor_spawners,
    char *error,size_t error_size) {
    if(!entities||!access||!controls) {
        if(error&&error_size)snprintf(error,error_size,"entity, access and control hosts are required");
        return NULL;
    }
    return runtime_new(program,fog,user,entities,access,1,1,controls,1,1,collections,
                       1,1,1,predicates,1,actor_spawners,1,1,0,0,0,NULL,0,NULL,0,NULL,error,error_size);
}

GscRuntime *gsc_runtime_new_with_lifetime(const GscProgram *program,GscFogCallback fog,void *user,
    const GscEntityHost *entities,const GscEntityAccessHost *access,
    const GscEntityControlHost *controls,const GscEntityCollectionHost *collections,
    const GscEntityPredicateHost *predicates,const GscActorSpawnHost *actor_spawners,
    char *error,size_t error_size) {
    if(!entities||!access||!controls) {
        if(error&&error_size)snprintf(error,error_size,"entity, access and control hosts are required");
        return NULL;
    }
    return runtime_new(program,fog,user,entities,access,1,1,controls,1,1,collections,
                       1,1,1,predicates,1,actor_spawners,1,1,1,0,0,NULL,0,NULL,0,NULL,error,error_size);
}

GscRuntime *gsc_runtime_new_with_names(const GscProgram *program,GscFogCallback fog,void *user,
    const GscEntityHost *entities,const GscEntityAccessHost *access,
    const GscEntityControlHost *controls,const GscEntityCollectionHost *collections,
    const GscEntityPredicateHost *predicates,const GscActorSpawnHost *actor_spawners,
    char *error,size_t error_size) {
    if(!entities||!access||!controls) {
        if(error&&error_size)snprintf(error,error_size,"entity, access and control hosts are required");
        return NULL;
    }
    return runtime_new(program,fog,user,entities,access,1,1,controls,1,1,collections,
                       1,1,1,predicates,1,actor_spawners,1,1,1,1,0,NULL,0,NULL,0,NULL,error,error_size);
}

GscRuntime *gsc_runtime_new_with_music(const GscProgram *program,GscFogCallback fog,void *user,
    const GscEntityHost *entities,const GscEntityAccessHost *access,
    const GscEntityControlHost *controls,const GscEntityCollectionHost *collections,
    const GscEntityPredicateHost *predicates,const GscActorSpawnHost *actor_spawners,
    const GscMusicHost *music_host,char *error,size_t error_size) {
    if(!entities||!access||!controls) {
        if(error&&error_size)snprintf(error,error_size,"entity, access and control hosts are required");
        return NULL;
    }
    return runtime_new(program,fog,user,entities,access,1,1,controls,1,1,collections,
                       1,1,1,predicates,1,actor_spawners,1,1,1,1,1,music_host,0,NULL,0,NULL,error,error_size);
}

GscRuntime *gsc_runtime_new_with_resources(const GscProgram *program,GscFogCallback fog,void *user,
    const GscEntityHost *entities,const GscEntityAccessHost *access,
    const GscEntityControlHost *controls,const GscEntityCollectionHost *collections,
    const GscEntityPredicateHost *predicates,const GscActorSpawnHost *actor_spawners,
    const GscMusicHost *music_host,const GscResourceHost *resource_host,char *error,size_t error_size) {
    if(!entities||!access||!controls) {
        if(error&&error_size)snprintf(error,error_size,"entity, access and control hosts are required");
        return NULL;
    }
    return runtime_new(program,fog,user,entities,access,1,1,controls,1,1,collections,
                       1,1,1,predicates,1,actor_spawners,1,1,1,1,1,music_host,1,resource_host,0,NULL,error,error_size);
}

GscRuntime *gsc_runtime_new_with_objectives(const GscProgram *program,GscFogCallback fog,void *user,
    const GscEntityHost *entities,const GscEntityAccessHost *access,
    const GscEntityControlHost *controls,const GscEntityCollectionHost *collections,
    const GscEntityPredicateHost *predicates,const GscActorSpawnHost *actor_spawners,
    const GscMusicHost *music_host,const GscResourceHost *resource_host,
    const GscObjectiveHost *objective_host,char *error,size_t error_size) {
    if(!entities||!access||!controls) {
        if(error&&error_size)snprintf(error,error_size,"entity, access and control hosts are required");
        return NULL;
    }
    return runtime_new(program,fog,user,entities,access,1,1,controls,1,1,collections,
        1,1,1,predicates,1,actor_spawners,1,1,1,1,1,music_host,1,resource_host,1,objective_host,error,error_size);
}

GscRuntime *gsc_runtime_new_with_console(const GscProgram *program,GscFogCallback fog,void *user,
    const GscEntityHost *entities,const GscEntityAccessHost *access,
    const GscEntityControlHost *controls,const GscEntityCollectionHost *collections,
    const GscEntityPredicateHost *predicates,const GscActorSpawnHost *actor_spawners,
    const GscMusicHost *music_host,const GscResourceHost *resource_host,
    const GscObjectiveHost *objective_host,const GscConsoleHost *console_host,char *error,size_t error_size) {
    if(!entities||!access||!controls) {
        if(error&&error_size)snprintf(error,error_size,"entity, access and control hosts are required");
        return NULL;
    }
    return runtime_new_console_common(program,fog,user,entities,access,1,1,controls,1,1,collections,
        1,1,1,predicates,1,actor_spawners,1,1,1,1,1,music_host,1,resource_host,1,objective_host,1,console_host,error,error_size);
}
GscRuntime *gsc_runtime_new_with_hud(const GscProgram *program,GscFogCallback fog,void *user,
    const GscEntityHost *entities,const GscEntityAccessHost *access,
    const GscEntityControlHost *controls,const GscEntityCollectionHost *collections,
    const GscEntityPredicateHost *predicates,const GscActorSpawnHost *actor_spawners,
    const GscMusicHost *music_host,const GscResourceHost *resource_host,
    const GscObjectiveHost *objective_host,const GscConsoleHost *console_host,const GscHudHost *hud_host,char *error,size_t error_size) {
    if(!entities||!access||!controls) {
        if(error&&error_size)snprintf(error,error_size,"entity, access and control hosts are required");
        return NULL;
    }
    return runtime_new_hud_common(program,fog,user,entities,access,1,1,controls,1,1,collections,
        1,1,1,predicates,1,actor_spawners,1,1,1,1,1,music_host,1,resource_host,1,objective_host,1,console_host,1,hud_host,error,error_size);
}

GscRuntime *gsc_runtime_new_with_mission(const GscProgram *program,GscFogCallback fog,void *user,
    const GscEntityHost *entities,const GscEntityAccessHost *access,
    const GscEntityControlHost *controls,const GscEntityCollectionHost *collections,
    const GscEntityPredicateHost *predicates,const GscActorSpawnHost *actor_spawners,
    const GscMusicHost *music_host,const GscResourceHost *resource_host,
    const GscObjectiveHost *objective_host,const GscConsoleHost *console_host,const GscHudHost *hud_host,
    const GscMissionHost *mission_host,char *error,size_t error_size) {
    if(!entities||!access||!controls) {
        if(error&&error_size)snprintf(error,error_size,"entity, access and control hosts are required");
        return NULL;
    }
    return runtime_new_mission_common(program,fog,user,entities,access,1,1,controls,1,1,collections,
        1,1,1,predicates,1,actor_spawners,1,1,1,1,1,music_host,1,resource_host,1,objective_host,1,console_host,1,hud_host,1,mission_host,error,error_size);
}

GscRuntime *gsc_runtime_new_with_blast(const GscProgram *program,GscFogCallback fog,void *user,
    const GscEntityHost *entities,const GscEntityAccessHost *access,
    const GscEntityControlHost *controls,const GscEntityCollectionHost *collections,
    const GscEntityPredicateHost *predicates,const GscActorSpawnHost *actor_spawners,
    const GscMusicHost *music_host,const GscResourceHost *resource_host,
    const GscObjectiveHost *objective_host,const GscConsoleHost *console_host,const GscHudHost *hud_host,
    const GscMissionHost *mission_host,const GscBlastHost *blast_host,char *error,size_t error_size) {
    if(!entities||!access||!controls) {
        if(error&&error_size)snprintf(error,error_size,"entity, access and control hosts are required");
        return NULL;
    }
    return runtime_new_blast_common(program,fog,user,entities,access,1,1,controls,1,1,collections,
        1,1,1,predicates,1,actor_spawners,1,1,1,1,1,music_host,1,resource_host,1,objective_host,1,console_host,1,hud_host,1,mission_host,1,blast_host,error,error_size);
}

GscRuntime *gsc_runtime_new_with_quake(const GscProgram *program,GscFogCallback fog,void *user,
    const GscEntityHost *entities,const GscEntityAccessHost *access,
    const GscEntityControlHost *controls,const GscEntityCollectionHost *collections,
    const GscEntityPredicateHost *predicates,const GscActorSpawnHost *actor_spawners,
    const GscMusicHost *music_host,const GscResourceHost *resource_host,
    const GscObjectiveHost *objective_host,const GscConsoleHost *console_host,const GscHudHost *hud_host,
    const GscMissionHost *mission_host,const GscBlastHost *blast_host,const GscQuakeHost *quake_host,char *error,size_t error_size) {
    if(!entities||!access||!controls) {
        if(error&&error_size)snprintf(error,error_size,"entity, access and control hosts are required");
        return NULL;
    }
    return runtime_new_quake_common(program,fog,user,entities,access,1,1,controls,1,1,collections,
        1,1,1,predicates,1,actor_spawners,1,1,1,1,1,music_host,1,resource_host,1,objective_host,1,console_host,1,hud_host,1,mission_host,1,blast_host,1,quake_host,error,error_size);
}
GscRuntime *gsc_runtime_new_with_fx(const GscProgram *program,GscFogCallback fog,void *user,
    const GscEntityHost *entities,const GscEntityAccessHost *access,
    const GscEntityControlHost *controls,const GscEntityCollectionHost *collections,
    const GscEntityPredicateHost *predicates,const GscActorSpawnHost *actor_spawners,
    const GscMusicHost *music_host,const GscResourceHost *resource_host,
    const GscObjectiveHost *objective_host,const GscConsoleHost *console_host,const GscHudHost *hud_host,
    const GscMissionHost *mission_host,const GscBlastHost *blast_host,const GscQuakeHost *quake_host,const GscFxHost *fx_host,char *error,size_t error_size) {
    if(!entities||!access||!controls) {
        if(error&&error_size)snprintf(error,error_size,"entity, access and control hosts are required");
        return NULL;
    }
    return runtime_new_fx_common(program,fog,user,entities,access,1,1,controls,1,1,collections,
        1,1,1,predicates,1,actor_spawners,1,1,1,1,1,music_host,1,resource_host,1,objective_host,1,console_host,1,hud_host,1,mission_host,1,blast_host,1,quake_host,1,fx_host,error,error_size);
}

GscRuntime *gsc_runtime_new_with_cvars(const GscProgram *program,GscFogCallback fog,void *user,
    const GscEntityHost *entities,const GscEntityAccessHost *access,
    const GscEntityControlHost *controls,const GscEntityCollectionHost *collections,
    const GscEntityPredicateHost *predicates,const GscActorSpawnHost *actor_spawners,
    const GscMusicHost *music_host,const GscResourceHost *resource_host,
    const GscObjectiveHost *objective_host,const GscConsoleHost *console_host,const GscHudHost *hud_host,
    const GscMissionHost *mission_host,const GscBlastHost *blast_host,const GscQuakeHost *quake_host,const GscFxHost *fx_host,const GscCvarHost *cvar_host,char *error,size_t error_size) {
    if(!entities||!access||!controls) {
        if(error&&error_size)snprintf(error,error_size,"entity, access and control hosts are required");
        return NULL;
    }
    return runtime_new_cvars_common(program,fog,user,entities,access,1,1,controls,1,1,collections,
        1,1,1,predicates,1,actor_spawners,1,1,1,1,1,music_host,1,resource_host,1,objective_host,1,console_host,1,hud_host,1,mission_host,1,blast_host,1,quake_host,1,fx_host,1,cvar_host,error,error_size);
}
GscRuntime *gsc_runtime_new_with_shellshock(const GscProgram *program,GscFogCallback fog,void *user,
    const GscEntityHost *entities,const GscEntityAccessHost *access,
    const GscEntityControlHost *controls,const GscEntityCollectionHost *collections,
    const GscEntityPredicateHost *predicates,const GscActorSpawnHost *actor_spawners,
    const GscMusicHost *music_host,const GscResourceHost *resource_host,
    const GscObjectiveHost *objective_host,const GscConsoleHost *console_host,const GscHudHost *hud_host,
    const GscMissionHost *mission_host,const GscBlastHost *blast_host,const GscQuakeHost *quake_host,const GscFxHost *fx_host,const GscCvarHost *cvar_host,const GscShellshockHost *shellshock_host,char *error,size_t error_size) {
    if(!entities||!access||!controls) {
        if(error&&error_size)snprintf(error,error_size,"entity, access and control hosts are required");
        return NULL;
    }
    return runtime_new_shellshock_common(program,fog,user,entities,access,1,1,controls,1,1,collections,
        1,1,1,predicates,1,actor_spawners,1,1,1,1,1,music_host,1,resource_host,1,objective_host,1,console_host,1,hud_host,1,mission_host,1,blast_host,1,quake_host,1,fx_host,1,cvar_host,1,shellshock_host,error,error_size);
}
GscRuntime *gsc_runtime_new_with_debug_text(const GscProgram *program,GscFogCallback fog,void *user,
    const GscEntityHost *entities,const GscEntityAccessHost *access,
    const GscEntityControlHost *controls,const GscEntityCollectionHost *collections,
    const GscEntityPredicateHost *predicates,const GscActorSpawnHost *actor_spawners,
    const GscMusicHost *music_host,const GscResourceHost *resource_host,
    const GscObjectiveHost *objective_host,const GscConsoleHost *console_host,const GscHudHost *hud_host,
    const GscMissionHost *mission_host,const GscBlastHost *blast_host,const GscQuakeHost *quake_host,const GscFxHost *fx_host,const GscCvarHost *cvar_host,const GscShellshockHost *shellshock_host,const GscDebugTextHost *debug_text_host,char *error,size_t error_size) {
    if(!entities||!access||!controls) {
        if(error&&error_size)snprintf(error,error_size,"entity, access and control hosts are required");
        return NULL;
    }
    return runtime_new_debug_text_common(program,fog,user,entities,access,1,1,controls,1,1,collections,
        1,1,1,predicates,1,actor_spawners,1,1,1,1,1,music_host,1,resource_host,1,objective_host,1,console_host,1,hud_host,1,mission_host,1,blast_host,1,quake_host,1,fx_host,1,cvar_host,1,shellshock_host,1,debug_text_host,error,error_size);
}
GscRuntime *gsc_runtime_new_with_debug_line(const GscProgram *program,GscFogCallback fog,void *user,
    const GscEntityHost *entities,const GscEntityAccessHost *access,
    const GscEntityControlHost *controls,const GscEntityCollectionHost *collections,
    const GscEntityPredicateHost *predicates,const GscActorSpawnHost *actor_spawners,
    const GscMusicHost *music_host,const GscResourceHost *resource_host,
    const GscObjectiveHost *objective_host,const GscConsoleHost *console_host,const GscHudHost *hud_host,
    const GscMissionHost *mission_host,const GscBlastHost *blast_host,const GscQuakeHost *quake_host,const GscFxHost *fx_host,const GscCvarHost *cvar_host,const GscShellshockHost *shellshock_host,const GscDebugTextHost *debug_text_host,const GscDebugLineHost *debug_line_host,char *error,size_t error_size) {
    if(!entities||!access||!controls) {
        if(error&&error_size)snprintf(error,error_size,"entity, access and control hosts are required");
        return NULL;
    }
    return runtime_new_debug_line_common(program,fog,user,entities,access,1,1,controls,1,1,collections,
        1,1,1,predicates,1,actor_spawners,1,1,1,1,1,music_host,1,resource_host,1,objective_host,1,console_host,1,hud_host,1,mission_host,1,blast_host,1,quake_host,1,fx_host,1,cvar_host,1,shellshock_host,1,debug_text_host,1,debug_line_host,error,error_size);
}
GscRuntime *gsc_runtime_new_with_ambient(const GscProgram *program,GscFogCallback fog,void *user,
    const GscEntityHost *entities,const GscEntityAccessHost *access,
    const GscEntityControlHost *controls,const GscEntityCollectionHost *collections,
    const GscEntityPredicateHost *predicates,const GscActorSpawnHost *actor_spawners,
    const GscMusicHost *music_host,const GscResourceHost *resource_host,
    const GscObjectiveHost *objective_host,const GscConsoleHost *console_host,const GscHudHost *hud_host,
    const GscMissionHost *mission_host,const GscBlastHost *blast_host,const GscQuakeHost *quake_host,const GscFxHost *fx_host,const GscCvarHost *cvar_host,const GscShellshockHost *shellshock_host,const GscDebugTextHost *debug_text_host,const GscDebugLineHost *debug_line_host,const GscAmbientHost *ambient_host,char *error,size_t error_size) {
    if(!entities||!access||!controls) {
        if(error&&error_size)snprintf(error,error_size,"entity, access and control hosts are required");
        return NULL;
    }
    return runtime_new_ambient_common(program,fog,user,entities,access,1,1,controls,1,1,collections,
        1,1,1,predicates,1,actor_spawners,1,1,1,1,1,music_host,1,resource_host,1,objective_host,1,console_host,1,hud_host,1,mission_host,1,blast_host,1,quake_host,1,fx_host,1,cvar_host,1,shellshock_host,1,debug_text_host,1,debug_line_host,1,ambient_host,error,error_size);
}
GscRuntime *gsc_runtime_new_with_precache_item(const GscProgram *program,GscFogCallback fog,void *user,
    const GscEntityHost *entities,const GscEntityAccessHost *access,
    const GscEntityControlHost *controls,const GscEntityCollectionHost *collections,
    const GscEntityPredicateHost *predicates,const GscActorSpawnHost *actor_spawners,
    const GscMusicHost *music_host,const GscResourceHost *resource_host,
    const GscObjectiveHost *objective_host,const GscConsoleHost *console_host,const GscHudHost *hud_host,
    const GscMissionHost *mission_host,const GscBlastHost *blast_host,const GscQuakeHost *quake_host,const GscFxHost *fx_host,const GscCvarHost *cvar_host,const GscShellshockHost *shellshock_host,const GscDebugTextHost *debug_text_host,const GscDebugLineHost *debug_line_host,const GscAmbientHost *ambient_host,const GscPrecacheItemHost *precache_item_host,char *error,size_t error_size) {
    if(!entities||!access||!controls) {
        if(error&&error_size)snprintf(error,error_size,"entity, access and control hosts are required");
        return NULL;
    }
    return runtime_new_precache_item_common(program,fog,user,entities,access,1,1,controls,1,1,collections,
        1,1,1,predicates,1,actor_spawners,1,1,1,1,1,music_host,1,resource_host,1,objective_host,1,console_host,1,hud_host,1,mission_host,1,blast_host,1,quake_host,1,fx_host,1,cvar_host,1,shellshock_host,1,debug_text_host,1,debug_line_host,1,ambient_host,1,precache_item_host,error,error_size);
}
GscRuntime *gsc_runtime_new_with_save_game(const GscProgram *program,GscFogCallback fog,void *user,
    const GscEntityHost *entities,const GscEntityAccessHost *access,
    const GscEntityControlHost *controls,const GscEntityCollectionHost *collections,
    const GscEntityPredicateHost *predicates,const GscActorSpawnHost *actor_spawners,
    const GscMusicHost *music_host,const GscResourceHost *resource_host,
    const GscObjectiveHost *objective_host,const GscConsoleHost *console_host,const GscHudHost *hud_host,
    const GscMissionHost *mission_host,const GscBlastHost *blast_host,const GscQuakeHost *quake_host,const GscFxHost *fx_host,const GscCvarHost *cvar_host,const GscShellshockHost *shellshock_host,const GscDebugTextHost *debug_text_host,const GscDebugLineHost *debug_line_host,const GscAmbientHost *ambient_host,const GscPrecacheItemHost *precache_item_host,const GscSaveGameHost *save_game_host,char *error,size_t error_size) {
    if(!entities||!access||!controls) {
        if(error&&error_size)snprintf(error,error_size,"entity, access and control hosts are required");
        return NULL;
    }
    return runtime_new_save_game_common(program,fog,user,entities,access,1,1,controls,1,1,collections,
        1,1,1,predicates,1,actor_spawners,1,1,1,1,1,music_host,1,resource_host,1,objective_host,1,console_host,1,hud_host,1,mission_host,1,blast_host,1,quake_host,1,fx_host,1,cvar_host,1,shellshock_host,1,debug_text_host,1,debug_line_host,1,ambient_host,1,precache_item_host,1,save_game_host,error,error_size);
}
GscRuntime *gsc_runtime_new_with_tag_fx(const GscProgram *program,GscFogCallback fog,void *user,
    const GscEntityHost *entities,const GscEntityAccessHost *access,
    const GscEntityControlHost *controls,const GscEntityCollectionHost *collections,
    const GscEntityPredicateHost *predicates,const GscActorSpawnHost *actor_spawners,
    const GscMusicHost *music_host,const GscResourceHost *resource_host,
    const GscObjectiveHost *objective_host,const GscConsoleHost *console_host,const GscHudHost *hud_host,
    const GscMissionHost *mission_host,const GscBlastHost *blast_host,const GscQuakeHost *quake_host,const GscFxHost *fx_host,const GscCvarHost *cvar_host,const GscShellshockHost *shellshock_host,const GscDebugTextHost *debug_text_host,const GscDebugLineHost *debug_line_host,const GscAmbientHost *ambient_host,const GscPrecacheItemHost *precache_item_host,const GscSaveGameHost *save_game_host,const GscTagFxHost *tag_fx_host,char *error,size_t error_size) {
    if(!entities||!access||!controls) {
        if(error&&error_size)snprintf(error,error_size,"entity, access and control hosts are required");
        return NULL;
    }
    return runtime_new_tag_fx_common(program,fog,user,entities,access,1,1,controls,1,1,collections,
        1,1,1,predicates,1,actor_spawners,1,1,1,1,1,music_host,1,resource_host,1,objective_host,1,console_host,1,hud_host,1,mission_host,1,blast_host,1,quake_host,1,fx_host,1,cvar_host,1,shellshock_host,1,debug_text_host,1,debug_line_host,1,ambient_host,1,precache_item_host,1,save_game_host,1,tag_fx_host,error,error_size);
}
GscRuntime *gsc_runtime_new_with_native_queries(const GscProgram *program,GscFogCallback fog,void *user,
    const GscEntityHost *entities,const GscEntityAccessHost *access,
    const GscEntityControlHost *controls,const GscEntityCollectionHost *collections,
    const GscEntityPredicateHost *predicates,const GscActorSpawnHost *actor_spawners,
    const GscMusicHost *music_host,const GscResourceHost *resource_host,
    const GscObjectiveHost *objective_host,const GscConsoleHost *console_host,const GscHudHost *hud_host,
    const GscMissionHost *mission_host,const GscBlastHost *blast_host,const GscQuakeHost *quake_host,const GscFxHost *fx_host,const GscCvarHost *cvar_host,const GscShellshockHost *shellshock_host,const GscDebugTextHost *debug_text_host,const GscDebugLineHost *debug_line_host,const GscAmbientHost *ambient_host,const GscPrecacheItemHost *precache_item_host,const GscSaveGameHost *save_game_host,const GscTagFxHost *tag_fx_host,const GscNativeQueryHost *native_query_host,char *error,size_t error_size) {
    if(!entities||!access||!controls) {
        if(error&&error_size)snprintf(error,error_size,"entity, access and control hosts are required");
        return NULL;
    }
    return runtime_new_native_queries_common(program,fog,user,entities,access,1,1,controls,1,1,collections,
        1,1,1,predicates,1,actor_spawners,1,1,1,1,1,music_host,1,resource_host,1,objective_host,1,console_host,1,hud_host,1,mission_host,1,blast_host,1,quake_host,1,fx_host,1,cvar_host,1,shellshock_host,1,debug_text_host,1,debug_line_host,1,ambient_host,1,precache_item_host,1,save_game_host,1,tag_fx_host,1,native_query_host,0,NULL,0,NULL,error,error_size);
}
GscRuntime *gsc_runtime_new_with_precache_string(const GscProgram *program,GscFogCallback fog,void *user,
    const GscEntityHost *entities,const GscEntityAccessHost *access,
    const GscEntityControlHost *controls,const GscEntityCollectionHost *collections,
    const GscEntityPredicateHost *predicates,const GscActorSpawnHost *actor_spawners,
    const GscMusicHost *music_host,const GscResourceHost *resource_host,
    const GscObjectiveHost *objective_host,const GscConsoleHost *console_host,const GscHudHost *hud_host,
    const GscMissionHost *mission_host,const GscBlastHost *blast_host,const GscQuakeHost *quake_host,const GscFxHost *fx_host,const GscCvarHost *cvar_host,const GscShellshockHost *shellshock_host,const GscDebugTextHost *debug_text_host,const GscDebugLineHost *debug_line_host,const GscAmbientHost *ambient_host,const GscPrecacheItemHost *precache_item_host,const GscSaveGameHost *save_game_host,const GscTagFxHost *tag_fx_host,const GscNativeQueryHost *native_query_host,const GscPrecacheStringHost *precache_string_host,char *error,size_t error_size) {
    GscRuntime *r;
    if(!precache_string_runtime_preflight(program,precache_string_host,error,error_size))return NULL;
    r=gsc_runtime_new_with_native_queries(program,fog,user,entities,access,controls,collections,predicates,actor_spawners,music_host,resource_host,objective_host,console_host,hud_host,mission_host,blast_host,quake_host,fx_host,cvar_host,shellshock_host,debug_text_host,debug_line_host,ambient_host,precache_item_host,save_game_host,tag_fx_host,native_query_host,error,error_size);
    if(r){r->precache_string_enabled=1;r->next_precache_string_operation=1;if(precache_string_host)r->precache_string=*precache_string_host;}
    return r;
}
GscRuntime *gsc_runtime_new_with_spawner_arrays(const GscProgram *program,GscFogCallback fog,void *user,
    const GscEntityHost *entities,const GscEntityAccessHost *access,
    const GscEntityControlHost *controls,const GscEntityCollectionHost *collections,
    const GscEntityPredicateHost *predicates,const GscActorSpawnHost *actor_spawners,
    const GscMusicHost *music_host,const GscResourceHost *resource_host,
    const GscObjectiveHost *objective_host,const GscConsoleHost *console_host,const GscHudHost *hud_host,
    const GscMissionHost *mission_host,const GscBlastHost *blast_host,const GscQuakeHost *quake_host,const GscFxHost *fx_host,const GscCvarHost *cvar_host,const GscShellshockHost *shellshock_host,const GscDebugTextHost *debug_text_host,const GscDebugLineHost *debug_line_host,const GscAmbientHost *ambient_host,const GscPrecacheItemHost *precache_item_host,const GscSaveGameHost *save_game_host,const GscTagFxHost *tag_fx_host,const GscNativeQueryHost *native_query_host,const GscPrecacheStringHost *precache_string_host,const GscSpawnerArrayHost *spawner_array_host,char *error,size_t error_size) {
    GscRuntime *r;
    if(!program||!program->spawner_arrays_mode||!entities||!access||!controls) {
        if(error&&error_size)snprintf(error,error_size,"spawner array constructor requires matching program and entity/access/control hosts");
        return NULL;
    }
    if(!precache_string_runtime_preflight(program,precache_string_host,error,error_size))return NULL;
    r=runtime_new_native_queries_common(program,fog,user,entities,access,1,1,controls,1,1,collections,
        1,1,1,predicates,1,actor_spawners,1,1,1,1,1,music_host,1,resource_host,1,objective_host,1,console_host,1,hud_host,1,mission_host,1,blast_host,1,quake_host,1,fx_host,1,cvar_host,1,shellshock_host,1,debug_text_host,1,debug_line_host,1,ambient_host,1,precache_item_host,1,save_game_host,1,tag_fx_host,1,native_query_host,1,spawner_array_host,0,NULL,error,error_size);
    if(r){r->precache_string_enabled=1;r->next_precache_string_operation=1;if(precache_string_host)r->precache_string=*precache_string_host;}
    return r;
}
GscRuntime *gsc_runtime_new_with_randomint(const GscProgram *program,GscFogCallback fog,void *user,
    const GscEntityHost *entities,const GscEntityAccessHost *access,
    const GscEntityControlHost *controls,const GscEntityCollectionHost *collections,
    const GscEntityPredicateHost *predicates,const GscActorSpawnHost *actor_spawners,
    const GscMusicHost *music_host,const GscResourceHost *resource_host,
    const GscObjectiveHost *objective_host,const GscConsoleHost *console_host,const GscHudHost *hud_host,
    const GscMissionHost *mission_host,const GscBlastHost *blast_host,const GscQuakeHost *quake_host,const GscFxHost *fx_host,const GscCvarHost *cvar_host,const GscShellshockHost *shellshock_host,const GscDebugTextHost *debug_text_host,const GscDebugLineHost *debug_line_host,const GscAmbientHost *ambient_host,const GscPrecacheItemHost *precache_item_host,const GscSaveGameHost *save_game_host,const GscTagFxHost *tag_fx_host,const GscNativeQueryHost *native_query_host,const GscPrecacheStringHost *precache_string_host,const GscSpawnerArrayHost *spawner_array_host,const GscRandomIntHost *randomint_host,char *error,size_t error_size) {
    GscRuntime *r;
    if(!program||!program->randomint_mode||!entities||!access||!controls) {
        if(error&&error_size)snprintf(error,error_size,"randomint constructor requires matching program and entity/access/control hosts");
        return NULL;
    }
    if(!precache_string_runtime_preflight(program,precache_string_host,error,error_size))return NULL;
    r=runtime_new_native_queries_common(program,fog,user,entities,access,1,1,controls,1,1,collections,
        1,1,1,predicates,1,actor_spawners,1,1,1,1,1,music_host,1,resource_host,1,objective_host,1,console_host,1,hud_host,1,mission_host,1,blast_host,1,quake_host,1,fx_host,1,cvar_host,1,shellshock_host,1,debug_text_host,1,debug_line_host,1,ambient_host,1,precache_item_host,1,save_game_host,1,tag_fx_host,1,native_query_host,1,spawner_array_host,1,randomint_host,error,error_size);
    if(r){r->precache_string_enabled=1;r->next_precache_string_operation=1;if(precache_string_host)r->precache_string=*precache_string_host;}
    return r;
}
GscRuntime *gsc_runtime_new_with_randomfloat(const GscProgram *program,GscFogCallback fog,void *user,
    const GscEntityHost *entities,const GscEntityAccessHost *access,
    const GscEntityControlHost *controls,const GscEntityCollectionHost *collections,
    const GscEntityPredicateHost *predicates,const GscActorSpawnHost *actor_spawners,
    const GscMusicHost *music_host,const GscResourceHost *resource_host,
    const GscObjectiveHost *objective_host,const GscConsoleHost *console_host,const GscHudHost *hud_host,
    const GscMissionHost *mission_host,const GscBlastHost *blast_host,const GscQuakeHost *quake_host,const GscFxHost *fx_host,const GscCvarHost *cvar_host,const GscShellshockHost *shellshock_host,const GscDebugTextHost *debug_text_host,const GscDebugLineHost *debug_line_host,const GscAmbientHost *ambient_host,const GscPrecacheItemHost *precache_item_host,const GscSaveGameHost *save_game_host,const GscTagFxHost *tag_fx_host,const GscNativeQueryHost *native_query_host,const GscPrecacheStringHost *precache_string_host,const GscSpawnerArrayHost *spawner_array_host,const GscRandomIntHost *randomint_host,const GscRandomFloatHost *randomfloat_host,char *error,size_t error_size) {
    GscRuntime *r;
    if(!program||!program->randomfloat_mode||!entities||!access||!controls) {
        if(error&&error_size)snprintf(error,error_size,"randomfloat constructor requires matching program and entity/access/control hosts");
        return NULL;
    }
    if(!randomfloat_runtime_preflight(program,randomfloat_host,error,error_size))return NULL;
    r=gsc_runtime_new_with_randomint(program,fog,user,entities,access,controls,collections,
        predicates,actor_spawners,music_host,resource_host,objective_host,console_host,hud_host,
        mission_host,blast_host,quake_host,fx_host,cvar_host,shellshock_host,debug_text_host,
        debug_line_host,ambient_host,precache_item_host,save_game_host,tag_fx_host,native_query_host,
        precache_string_host,spawner_array_host,randomint_host,error,error_size);
    if(r){r->randomfloat_enabled=1;r->next_randomfloat_operation=1;if(randomfloat_host)r->randomfloat=*randomfloat_host;}
    return r;
}
GscRuntime *gsc_runtime_new_with_keybinding(const GscProgram *program,GscFogCallback fog,void *user,
    const GscEntityHost *entities,const GscEntityAccessHost *access,
    const GscEntityControlHost *controls,const GscEntityCollectionHost *collections,
    const GscEntityPredicateHost *predicates,const GscActorSpawnHost *actor_spawners,
    const GscMusicHost *music_host,const GscResourceHost *resource_host,
    const GscObjectiveHost *objective_host,const GscConsoleHost *console_host,const GscHudHost *hud_host,
    const GscMissionHost *mission_host,const GscBlastHost *blast_host,const GscQuakeHost *quake_host,const GscFxHost *fx_host,const GscCvarHost *cvar_host,const GscShellshockHost *shellshock_host,const GscDebugTextHost *debug_text_host,const GscDebugLineHost *debug_line_host,const GscAmbientHost *ambient_host,const GscPrecacheItemHost *precache_item_host,const GscSaveGameHost *save_game_host,const GscTagFxHost *tag_fx_host,const GscNativeQueryHost *native_query_host,const GscPrecacheStringHost *precache_string_host,const GscSpawnerArrayHost *spawner_array_host,const GscRandomIntHost *randomint_host,const GscRandomFloatHost *randomfloat_host,const GscKeyBindingHost *keybinding_host,char *error,size_t error_size) {
    GscRuntime *r;
    if(!program||!program->keybinding_mode||!entities||!access||!controls) {
        if(error&&error_size)snprintf(error,error_size,"getkeybinding constructor requires matching program and entity/access/control hosts");
        return NULL;
    }
    if(!keybinding_runtime_preflight(program,keybinding_host,error,error_size))return NULL;
    r=gsc_runtime_new_with_randomfloat(program,fog,user,entities,access,controls,collections,
        predicates,actor_spawners,music_host,resource_host,objective_host,console_host,hud_host,
        mission_host,blast_host,quake_host,fx_host,cvar_host,shellshock_host,debug_text_host,
        debug_line_host,ambient_host,precache_item_host,save_game_host,tag_fx_host,native_query_host,
        precache_string_host,spawner_array_host,randomint_host,randomfloat_host,error,error_size);
    if(r){r->keybinding_enabled=1;r->next_keybinding_operation=1;if(keybinding_host)r->keybinding=*keybinding_host;}
    return r;
}


void gsc_runtime_free(GscRuntime *runtime) {
    if(!runtime)return;
    if(runtime->r54_dirty_scope){rfail(runtime,0,"runtime destruction during queue audit");return;}
    if(runtime->origin_method_busy){rfail(runtime,0,"GetOrigin runtime destruction during callback");return;}
    /* Program selection also covers certificate callbacks before Runtime
     * R243 enablement. Reject before any owner/storage teardown; pins only
     * prevent Task reuse and cannot keep a freed Runtime alive. */
    if((runtime->r243_thread_start_enabled||
        (runtime->program&&runtime->program->r243_thread_start_selected))&&
       (r237_busy(runtime)||runtime->execution_budget||
        runtime->entity_entry_depth||runtime->r243_thread_start_depth)){
        rfail(runtime,0,"R243 runtime destruction during callback/execution");return;
    }
    if(runtime->state_arena_busy||runtime->state_scratch||runtime->entity_inheritance_busy){rfail(runtime,0,"state arena runtime destruction during transaction");return;}
    if(placed_cohort_destructing(runtime))return;
    if(runtime->bsp_construction_only&&(runtime->advancing||runtime->executing_task||runtime->entity_sequence_busy||
       runtime->entity_prepare_busy||runtime->entity_certificate_busy||runtime->entity_inheritance_busy||runtime->entity_field_busy)){
        rfail(runtime,0,"BSP construction runtime destruction during callback");return;
    }
    if(runtime->loop_fx_scope){rfail(runtime,0,"loop FX runtime destruction during transaction");return;}
    if(runtime->r243_time_root_teardown){rfail(runtime,0,"selected time-root destruction reentered");return;}
    if((runtime->turrets_enabled||runtime->turret_animations_enabled||runtime->actor_animations_enabled)&&(runtime->entity_method_busy||runtime->advancing||runtime->executing_task)){rfail(runtime,0,"turret runtime destruction during callback/execution");return;}
    if(runtime->program->animation_namespace_selected&&(runtime->animation_namespace_busy||runtime->entity_certificate_busy||runtime->advancing||runtime->executing_task)){rfail(runtime,0,"animation namespace runtime destruction during callback/execution");return;}
    if(runtime->hud_elements_enabled&&(runtime->hud_elements_busy||runtime->advancing||runtime->executing_task)){rfail(runtime,0,"HUD runtime destruction during callback/execution");return;}
    if(!r243_time_root_free_begin(runtime))return;
    gsc_runtime_node_scripts_retire(runtime);
    int n;
    if(runtime->hud_elements_enabled&&!runtime->hud_elements_retired)(void)gsc_runtime_retire_hud_elements(runtime,NULL,0);
    r243_time_root_release_mains(runtime);
    for(n=0;n<runtime->task_count;n++) {
        Task *task=task_at(runtime,n);task_results_clear(task);
        if(runtime->lifetime_enabled)GSC_RUNTIME_FREE(task);
    }
    GSC_RUNTIME_FREE(runtime->extra_tasks);
    gsc_native_registry_free(runtime->native_registry);
    runtime->native_registry=NULL;
    r243_time_root_free_end(runtime);
    /* The cached stage has no live values or retained owners. Free only after
     * transaction/reentry guards above, using the same guarded allocator. */
    {void *scratch=runtime->r329_cow_scratch;
     runtime->r329_cow_scratch=NULL;runtime->r329_cow_scratch_bytes=0;
     if(scratch)(void)state_storage_free(runtime,scratch);}
    state_fields_release(runtime);
    placed_cohort_free(runtime,runtime->placed_cohort);runtime->placed_cohort=NULL;
    GSC_RUNTIME_FREE(runtime);
}
static int runtime_start_capabilities(GscRuntime *r) {
 if(r->program->move_delta_selected&&!r->move_delta_enabled)return rfail(r,0,"selected GetMoveDelta requires typed current resource host");
    if(r->program->namespace_start_origin_selected&&!r->namespace_start_origin_enabled)return rfail(r,0,"selected namespace start-origin query requires typed host capability");
    if(r->program->actor_float_selected&&(!r->actor_float_enabled||r->actor_float_mask!=r->program->actor_float_selected||!r->results_enabled))return rfail(r,0,"selected actor float query requires matching typed Results host");
    if(r->program->stance_selected&&(!r->stance_enabled||!r->results_enabled))return rfail(r,0,"selected isStanceAllowed requires matching typed Results host");
    if(r->program->animtime_selected&&(!r->animtime_enabled||!r->results_enabled))return rfail(r,0,"selected getAnimTime requires matching typed Results host");
    if(r->program->animation_namespace_selected&&!r->animation_namespace_enabled)return rfail(r,0,"selected animation namespace requires copied metadata authority");
    if(r->program->delete_transactions_selected&&!r->delete_transactions_enabled)return rfail(r,0,"selected delete transaction requires host capability");
    if(r->program->attachment_queries_selected&&!r->attachment_queries_enabled)return rfail(r,0,"selected attachment queries require host capability");
    if(r->program->entity_collision_selected&&!r->entity_collision_enabled)return rfail(r,0,"selected collision service requires runtime capability");
    if(r->program->spatial_natives_selected&&!r->spatial_natives_enabled)return rfail(r,0,"selected spatial natives require host capability");
    if(r->program->entity_coordinates_selected&&!r->entity_coordinates_enabled)return rfail(r,0,"selected coordinates require pose capability");
    if(r->program->entity_links_selected&&!r->entity_links_enabled)return rfail(r,0,"selected entity links require typed host");
    if(r->program->entity_operations_selected&&!r->entity_operations_enabled)return rfail(r,0,"selected entity operations require host capability");
    if(r->program->player_natives_selected&&!r->player_natives_enabled)return rfail(r,0,"selected player natives require host capability");
    if(r->program->string_int_concat_selected&&!r->string_int_concat_enabled)return rfail(r,0,"selected string/int concatenation requires runtime capability");
    if(r->program->integer_bitwise_selected&&!r->integer_bitwise_enabled)return rfail(r,0,"selected integer bitwise requires runtime capability");
    if(r->program->vector_math_selected&&!r->vector_math_enabled)return rfail(r,0,"selected vector math requires runtime capability");
    if(r->program->ai_arrays_selected&&!r->ai_arrays_enabled)return rfail(r,0,"selected AI arrays require runtime capability");
    if(r->program->carbine_items_selected==2&&!r->entity_fields_enabled)return rfail(r,0,"selected training hints require typed native fields");
    if(r->program->grenade_items_selected&&!r->entity_fields_enabled)return rfail(r,0,"selected grenade items require typed native field capability");
    if(r->program->turret_animations_selected&&!r->turret_animations_enabled)return rfail(r,0,"selected turret animation methods require a real runtime host");
    if(r->program->random_range_selected&&(!r->random_range_enabled||!r->randomfloat_enabled||!r->randomfloat.draw))return rfail(r,0,"randomFloatRange requires real selected shared draw owner");
    if(r->program->actor_drop_selected&&!r->actor_drop_enabled)return rfail(r,0,"selected DropWeapon requires a real runtime host");
    if(r->program->actor_look_selected&&!r->actor_look_enabled)return rfail(r,0,"selected actor look methods require a real runtime host");
    if(r->program->actor_animations_selected&&!r->actor_animations_enabled)return rfail(r,0,"selected actor animation methods require a real runtime host");
    if(r->program->flagged_all_selected&&!r->flagged_all_enabled)return rfail(r,0,"method12 runtime capability absent");
    if(r->program->flagged_all_restart_selected&&!r->flagged_all_restart_enabled)return rfail(r,0,"selected flagged all restart requires runtime capability");
    if(r->program->turrets_selected&&!r->turrets_enabled)return rfail(r,0,"selected turret methods require a real runtime host");
    if(r->program->actor_goal_selected&&!r->actor_goal_enabled)return rfail(r,0,"selected goal assignment requires a real runtime host");
    if(r->program->yaw_motion_selected&&!r->yaw_motion_enabled)return rfail(r,0,"selected yaw motion requires a real runtime host");
    if(r->program->vehicle_nodes_selected&&!r->vehicle_nodes_enabled)return rfail(r,0,"selected vehicle nodes require runtime capability");
    if(r->program->node_queries_selected&&!r->node_queries_enabled)return rfail(r,0,"selected node queries require runtime capability");
    if(r->program->hud_elements_selected&&!r->hud_elements_enabled)return rfail(r,0,"selected HUD elements require runtime capability");
    if(r->program->vector_arithmetic_selected&&!r->vector_arithmetic_enabled)return rfail(r,0,"selected vector arithmetic requires runtime capability");
    if(r->program->endon_selected&&!r->endon_enabled)return rfail(r,0,"selected endon requires runtime capability");
    if(r->program->owned_wait_delivery_selected&&!r->owned_wait_delivery_enabled)return rfail(r,0,"selected native wait delivery requires runtime capability");
    if(r->program->waittillmatch_selected&&!r->waittillmatch_enabled)return rfail(r,0,"selected waittillmatch requires runtime capability");
    if(r->program->console_int32_selected&&!r->console_int32_enabled)return rfail(r,0,"selected console int32 display requires runtime capability");
    if(r->program->timed_arithmetic_selected&&!r->timed_arithmetic_enabled)return rfail(r,0,"selected timed animation arithmetic requires a runtime capability");
    if(r->program->frame_cadence_selected&&!r->frame_cadence_enabled)return rfail(r,0,"selected frame animation cadence requires a runtime capability");
    if(r->program->r237_frame_buckets_selected&&!r->r237_frame_buckets_enabled)return rfail(r,0,"selected R237 frame buckets require a runtime capability");
    if(r->program->float_arithmetic_selected&&!r->float_arithmetic_enabled)return rfail(r,0,"selected module float arithmetic requires runtime capability");
    if(r->program->sound_notify_selected&&!r->sound_notify_enabled)return rfail(r,0,"selected sound notify requires host capability");
    if(r->program->loop_sound_selected&&!r->loop_sound_enabled)return rfail(r,0,"selected loop sound requires host capability");
    if(r->program->script_sound_selected&&!r->script_sound_enabled)return rfail(r,0,"selected playsound requires host capability");
    if(r->program->entity_model_methods_selected&&!r->entity_methods_enabled)
        return rfail(r,0,"selected entity model methods require a host capability");
    if(r->program->animation_methods_selected&&!r->animation_methods_enabled)return rfail(r,0,"selected animation methods require a host capability");
    if(r->program->actor_presence_selected&&!r->actor_presence_enabled)
        return rfail(r,0,"selected isAI requires an Actor presence capability");
    if(r->program->actor_critical_selected&&!r->actor_critical_enabled)
        return rfail(r,0,"selected Actor critical methods require a host capability");
    if(r->program->spawner_team_method_selected&&!r->spawner_team_enabled)
        return rfail(r,0,"selected spawner team method requires a host capability");
    if(r->program->animation_refs_selected&&!r->animation_refs_enabled)return rfail(r,0,"selected animation references require catalog authority");
    if(r->program->anim_data_selected&&!r->anim_data_enabled)return rfail(r,0,"selected anim data requires a map owner capability");
    if(r->program->start_origin_selected&&!r->start_origin_enabled)return rfail(r,0,"selected start-origin query requires a host capability");
    if(r->program->r238_array_value_cow_selected&&!r->r238_array_value_cow_enabled)return rfail(r,0,"selected R238 array COW requires runtime capability");
    if(r->program->r239_mixed_scalar_equality_selected&&!r->r239_mixed_scalar_equality_enabled)return rfail(r,0,"selected R239 mixed equality requires runtime capability");
    if(r->program->r243_thread_start_selected&&!r->r243_thread_start_enabled)return rfail(r,0,"selected R243 thread start requires runtime capability");
    if(r->program->configured_precache_interval_selected&&!r->configured_precache_interval_enabled)return rfail(r,0,"selected configured precache interval requires runtime capability");
    if(r->program->r243_same_bucket_wait_selected&&!r->r243_same_bucket_wait_enabled)return rfail(r,0,"selected R243 same-bucket wait requires runtime capability");
    if(r->program->r243_scripted_start_selected&&!r->r243_scripted_start_enabled)return rfail(r,0,"selected normal scripted start requires real Actor host");
    if(r->program->r243_script_state_selected&&!r->r243_script_state_enabled)return rfail(r,0,"selected trackScriptState requires genuine Actor host");
    if(r->program->r243_scripted_prefix_selected&&!r->r243_scripted_prefix_enabled)return rfail(r,0,"selected animscripted prefix requires genuine native host capability");
    if(r->program->owned_numeric_delete_selected&&!r->owned_numeric_delete_enabled)return rfail(r,0,"selected owned numeric deletion requires runtime capability");
    if(r->program->animation_length_selected&&!r->animation_length_enabled)return rfail(r,0,"selected animation length query requires a host capability");
    if(r->program->vehicle_methods_selected&&!r->vehicle_methods_enabled)return rfail(r,0,"selected vehicle methods require host");
    if(r->program->bullet_trace_selected&&!r->bullet_trace_enabled)return rfail(r,0,"selected bulletTrace requires host");
    if(r->program->vehicle_spawn_selected&&!r->vehicle_spawn_enabled)return rfail(r,0,"selected spawnVehicle requires host");
    if(r->program->turret_precache_selected&&!r->turret_precache_enabled)return rfail(r,0,"selected precacheTurret requires host");
    if(r->program->turret_spawn_selected&&!r->turret_spawn_enabled)return rfail(r,0,"selected spawnTurret requires host");
    if(r->program->model_part_selected&&!r->model_part_enabled)return rfail(r,0,"selected model part requires host");
    if(r->program->weapon_classname_selected&&!r->weapon_classname_enabled)return rfail(r,0,"selected weapon classname requires host");
    if(r->program->weapon_model_selected&&!r->weapon_model_enabled)return rfail(r,0,"selected weapon model query requires a host capability");
    if(!r->fog)return rfail(r,0,"fog callback required");
    return 1;
}
static int r237_run_start_root(GscRuntime *,uint64_t);
static int runtime_start_configured_main(GscRuntime *r,const GscMapExecutionCertificate *certificate,int synchronous) {
    if(r&&r->map_source_float_enabled&&(!certificate||!certificate->receipt||!certificate->current))return rfail(r,0,"map arithmetic requires map-certified root");
    if(r&&r->owned_wait_delivery_enabled&&(!certificate||!certificate->receipt||!certificate->current))return rfail(r,0,"native wait delivery main requires its own map certificate");
    if(r&&r->bsp_construction_only)return rfail(r,0,"BSP construction profile cannot execute script main");
    int ok;
    /* Required root is born only by genuine hook bootstrap. */
    if(r243_time_root_required(r)&&(!r->bootstrap_started||!r237_owner_current(r,r237_owner_stamp(r))))
        return rfail(r,0,"selected time-root requires completed hook bootstrap before configured main");
    r->configured_main_attempted=1;
    r->started=1;
    r->resources_entry_serial=r->next_serial;
    /* Original map main runs with level as self. Its plain child threads
     * inherit that receiver. Host-bound entries retain their supplied self. */
    {Value mapSelf={0};const Value *self=NULL;
     if(certificate&&r->state_enabled){mapSelf.kind=V_LEVEL;self=&mapSelf;}
     if(!spawn(r,r->program->entry_fn,NULL,0,self,1))return 0;
    }
    if(certificate||r->configured_precache_interval_enabled){int n;Task *root=NULL;for(n=0;n<r->task_count;n++){Task *task=task_at(r,n);if(task->serial==r->resources_entry_serial){root=task;if(certificate)task->map_certificate=*certificate;break;}}if(!root)return rfail(r,0,"configured main task identity was not created");if(!configured_precache_arm(r,root))return 0;}
    if(r->resources_enabled)r->resources_closed=0;
    /* Original game flag surrounds a synchronous engine script invocation.
     * Selected shellshock scope therefore includes this initial ready drain;
     * the older resource root-prefix window and child scheduler are unchanged. */
    r->shellshock_initial_window=r->shellshock_enabled;
    /* Independent item window mirrors the outer native startup invocation.
     * Deferred children remain admitted until this initial ready drain returns. */
    r->precache_item_initial_window=r->precache_item_enabled;
    /* Separate conservative port lifetime: first successful wait/waittill in
     * any scheduled task closes immediately; the initial drain also closes on
     * return. Original global startup gate proves neither this deferred child
     * ordering nor the exact port lifetime boundary. */
    r->precache_string_initial_window=r->precache_string_enabled;
    r->r237_start_draining=r->r237_frame_buckets_enabled;
    ok=synchronous?r237_run_start_root(r,r->resources_entry_serial):gsc_advance(r,0);
    r->r237_start_draining=0;
    r->shellshock_initial_window=0;
    r->precache_item_initial_window=0;
    r->precache_string_initial_window=0;
    configured_precache_close(r);
    r->resources_closed=1;
    return ok;
}
int gsc_start(GscRuntime *r) {
    if(!r)return 0;
    if(r->advancing||r->hud_elements_busy||r->entity_prepare_busy||r->entity_certificate_busy)return rfail(r,0,"reentrant scheduler call");
    if(r->started)return 0;
    if(!runtime_start_capabilities(r))return 0;
    return runtime_start_configured_main(r,NULL,0);
}
int gsc_start_with_map(GscRuntime *r,const GscMapExecutionCertificate *input,int(*current)(void*),void*context) {
    GscMapExecutionCertificate certificate;const GscProgram*program;int ok;
    if(!r||!input||!current||!gsc_runtime_storage_separate(r,input,sizeof(*input)))return 0;
    if(r->advancing||r->executing_task||r->entity_sequence_busy||r->hud_elements_busy||
       r->entity_prepare_busy||r->entity_certificate_busy||r->entity_field_busy||r->entity_method_busy)
        return rfail(r,0,"map root startup reentered");
    if(r->started||r->bootstrap_started||r->configured_main_attempted||r->task_count||r->now_ms||r->error[0])return 0;
    certificate=*input;program=r->program;
    if(!certificate.receipt||!certificate.current)return rfail(r,0,"map root requires a genuine map certificate");
    if(!runtime_start_capabilities(r))return 0;
    r->entity_certificate_busy=1;
    ok=current(context)==1&&!r->error[0];
    if(ok)ok=certificate.current(certificate.receipt)==1;
    if(ok)ok=current(context)==1;
    r->entity_certificate_busy=0;
    if(!ok||r->error[0]||r->program!=program||memcmp(input,&certificate,sizeof(certificate)))
        return r->error[0]?0:rfail(r,0,"map root startup authority retired or changed");
    return runtime_start_configured_main(r,&certificate,0);
}
int gsc_start_configured_main_after_bootstrap(GscRuntime *r,const GscMapExecutionCertificate *input) {
    GscMapExecutionCertificate certificate;int current;
    if(!r||!input||!gsc_runtime_storage_separate(r,input,sizeof(*input)))return 0;
    if(r->advancing||r->executing_task||r->entity_sequence_busy||r->hud_elements_busy||
       r->entity_prepare_busy||r->entity_certificate_busy||r->entity_field_busy||r->entity_method_busy)
        return rfail(r,0,"configured main bootstrap continuation reentered");
    if(!r->started||!r->bootstrap_started||!r->bootstrap_returned||r->configured_main_attempted||
       r->now_ms!=0||gsc_active_threads(r)||r->error[0])return 0;
    certificate=*input;
    if(!certificate.receipt||!certificate.current)return rfail(r,0,"configured main requires a map certificate");
    if(!runtime_start_capabilities(r))return 0;
    r->configured_main_attempted=1;r->entity_certificate_busy=1;
    current=certificate.current(certificate.receipt);r->entity_certificate_busy=0;
    if(current!=1||r->error[0])return r->error[0]?0:rfail(r,0,"configured main bootstrap map retired");
    return runtime_start_configured_main(r,&certificate,0);
}
int gsc_advance(GscRuntime *r,int now_ms) {
    int n,budget=MAX_STEPS;Task *chosen;
    if(r&&(r->advancing||r->hud_elements_busy||r->entity_prepare_busy||r->entity_certificate_busy))return rfail(r,0,"reentrant scheduler call");
    if(!r||!r->started||r->error[0]||now_ms<r->now_ms||(r->frame_cadence_enabled&&now_ms>INT32_C(0xffffff)))return 0;
    if(r->r237_frame_buckets_enabled)return r->r237_start_draining&&now_ms==0?r237_bootstrap_drain(r):0;
    r->advancing=1;
    r->execution_budget=&budget;
    r->now_ms=now_ms;
    for(n=0;n<r->task_count;n++) {
        if(!refresh_state_context(r,task_at(r,n))) {r->execution_budget=NULL;r->advancing=0;return 0;}
        if(!refresh_wait_owner(r,task_at(r,n))) {r->execution_budget=NULL;r->advancing=0;return 0;}
        if(task_at(r,n)->status==TASK_DEAD&&task_at(r,n)->results) {
            GSC_RUNTIME_FREE(task_at(r,n)->results);task_at(r,n)->results=NULL;
        }
        if(task_at(r,n)->status==TASK_SLEEP&&task_at(r,n)->wake_ms<=now_ms)
            task_at(r,n)->status=TASK_READY;
    }
    while(1) {
        chosen=NULL;
        for(n=0;n<r->task_count;n++) {
            Task *candidate=task_at(r,n);
            if(r->frame_cadence_enabled&&candidate->status==TASK_SLEEP&&candidate->wake_ms<=now_ms)candidate->status=TASK_READY;
            if(candidate->status==TASK_READY&&(!chosen||
               (r->frame_cadence_enabled?(candidate->wake_ms<chosen->wake_ms||(candidate->wake_ms==chosen->wake_ms&&candidate->serial<chosen->serial)):candidate->serial<chosen->serial)))chosen=candidate;
        }
        if(!chosen)break;
        if(!refresh_wait_owner(r,chosen)) {r->execution_budget=NULL;r->advancing=0;return 0;}
        if(chosen->status!=TASK_READY) {
            if(r->resources_enabled&&chosen->serial==r->resources_entry_serial)r->resources_closed=1;
            continue;
        }
        if(chosen->event_scope==EVENT_ENTITY)clear_wait_owner(chosen);
        if(r->frame_cadence_enabled) {
            if(chosen->wake_ms<r->cadence_drain_ms||chosen->wake_ms>now_ms){r->execution_budget=NULL;r->advancing=0;return rfail(r,0,"selected cadence deadline order invalid");}
            r->cadence_drain_ms=chosen->wake_ms;
        }
        r->executing_task=chosen;
        if(!(r->results_enabled?run_task_results(r,chosen,&budget):
             run_task(r,chosen,&budget))) {
            r->executing_task=NULL;
            if(r->resources_enabled&&chosen->serial==r->resources_entry_serial)r->resources_closed=1;
            /* Private cancellation signal: no fabricated Value and no runtime
             * error. Ordinary callback/type/reentry failures remain fatal. */
            if(r->state_enabled&&chosen->status==TASK_DEAD&&!r->error[0]) {
                if(chosen->results)GSC_RUNTIME_FREE(chosen->results);
                chosen->results=NULL;
                continue;
            }
            r->execution_budget=NULL;r->advancing=0;return 0;
        }
        r->executing_task=NULL;
        /* The original DLL clears its global gate when initial ExecuteThread
         * returns. This port's deferred child order is unchanged: initial root
         * suspension/retirement closes before any later chosen child executes.
         * This is a conservative port window, not native child-order parity. */
        if(r->resources_enabled&&chosen->serial==r->resources_entry_serial&&
           chosen->status!=TASK_READY)r->resources_closed=1;
        if(chosen->status==TASK_DEAD&&chosen->results) {
            GSC_RUNTIME_FREE(chosen->results);chosen->results=NULL;
        }
    }
    if(r->frame_cadence_enabled)r->cadence_drain_ms=now_ms;
    r->execution_budget=NULL;r->advancing=0;
    endon_collect_retired(r);
    return array_collect_safe_point(r);
}
#include "gsc_vm_r237_frame_buckets.inc"
#include "gsc_vm_entity_inheritance.inc"
#include "gsc_vm_entity_entry.inc"
#include "gsc_vm_r243_actor_script_context.inc"
#include "gsc_vm_r243_controller_driver.inc"
#include "gsc_vm_locomotion_main.inc"
int gsc_notify(GscRuntime *r,const char *name) {
    int ok;
    if(r&&(r->advancing||r->hud_elements_busy||r->entity_prepare_busy||r->entity_certificate_busy))return rfail(r,0,"reentrant scheduler call");
    if(!r||!r->started||r->error[0]||!name||!name[0]||strlen(name)>=128)return 0;
    r->advancing=1;ok=notify_internal(r,name);r->advancing=0;
    endon_collect_retired(r);
    return ok&&!r->error[0];
}
/* Narrow native producer: same internal event operation as a script notify,
 * only during this selected sound callback. Never starts a scheduler turn. */
int gsc_runtime_sound_notify_native(GscRuntime *r,const GscEntityRef *owner,const char *name,int *awakened){
 size_t n=0;int matched;GscEntityRef copy;char event[128];
 if(awakened)*awakened=0;
 if(!r||!r->sound_notify_enabled||!r->sound_notify_call_active||!r->advancing||!r->entity_method_busy||
    !r->started||r->error[0]||!owner||!name||!awakened||!entity_ref_equal(owner,&r->sound_notify_owner))return 0;
 while(n<sizeof(event)&&name[n])n++;if(!n||n==sizeof(event))return 0;
 copy=*owner;memcpy(event,name,n+1);if(event_owner_live(r,&copy)!=1)return 0;
 matched=entity_notify_internal(r,&copy,event);if(matched<0||r->error[0])return 0;
 *awakened=matched;return 1;
}
int gsc_notify_entity(GscRuntime *r,const GscEntityRef *owner,const char *name,int *out_awakened) {
    GscEntityRef copy;char event[128];size_t n=0;int alive,matched;
    if(out_awakened)*out_awakened=0;
    if(r&&(r->advancing||r->hud_elements_busy||r->entity_prepare_busy||r->entity_certificate_busy))return rfail(r,0,"reentrant scheduler call");
    if(!r||!r->started||r->error[0]||!r->events_enabled||!out_awakened||!owner||!name)return 0;
    while(n<128&&name[n])n++;
    if(!n||n==128)return 0;
    copy=*owner;memcpy(event,name,n+1);
    /* The pure host validation still runs under the scheduler mutation guard.
     * A nested start/advance/notify is fatal, even outside an advance frame. */
    r->advancing=1;alive=event_owner_live(r,&copy);
    if(alive!=1) {r->advancing=0;return 0;}
    matched=entity_notify_internal(r,&copy,event);r->advancing=0;
    endon_collect_retired(r);
    if(matched<0||r->error[0])return 0;
    *out_awakened=matched;return 1;
}
static int notify_with_payload_selected(GscRuntime *r,const GscEntityRef *owner,
    const char *name,const GscEventValue *values,size_t count,int *out_awakened,int entity,const GscBlastWaiterSet *selected) {
    Value copy[GSC_EVENT_PAYLOAD_CAPACITY];GscEntityRef identity;
    char event[128];size_t length=0,n,k;int alive,matched;GscBlastWaiterSet selection;
    if(out_awakened)*out_awakened=0;
    if(r&&(r->advancing||r->hud_elements_busy||r->entity_prepare_busy||r->entity_certificate_busy))return rfail(r,0,"reentrant scheduler call");
    if(!r||!r->started||r->error[0]||!r->event_payloads_enabled||!out_awakened||!name||
       (entity&&!owner))return 0;
    while(length<sizeof(event)&&name[length])length++;
    if(!length||length==sizeof(event)||!payload_external_copy(values,count,copy))return 0;
    memcpy(event,name,length+1);
    if(entity)identity=*owner;
    if(selected) {
        selection=*selected;
        if(!r->blast_enabled||!entity||selection.runtime_token!=r->array_token||
           selection.count>GSC_BLAST_WAITER_CAPACITY||!entity_ref_equal(&selection.owner,&identity)||
           !memchr(selection.event,0,sizeof(selection.event))||strcmp(selection.event,event))return 0;
        for(n=0;n<selection.count;n++) {
            if(!selection.registrations[n].task_serial||!selection.registrations[n].registration_serial)return 0;
            for(k=0;k<n;k++)if(selection.registrations[k].task_serial==selection.registrations[n].task_serial)return 0;
        }
        selected=&selection;
    }
    r->advancing=1;
    if(entity) {
        alive=event_owner_live(r,&identity);
        if(alive!=1) {r->advancing=0;return 0;}
    }
    matched=payload_deliver_selected(r,entity?&identity:NULL,event,copy,(int)count,0,selected);
    r->advancing=0;
    endon_collect_retired(r);
    if(matched<0||r->error[0])return 0;
    *out_awakened=matched;return 1;
}
/* Preserve the exact old private notify shape and semantics. */
static int notify_with_payload(GscRuntime *r,const GscEntityRef *owner,
    const char *name,const GscEventValue *values,size_t count,int *out_awakened,int entity) {
    return notify_with_payload_selected(r,owner,name,values,count,out_awakened,entity,NULL);
}
int gsc_blast_capture_waiters(const GscRuntime *r,const GscEntityRef *owner,const char *name,
    GscBlastWaiterSet *out,char *error,size_t size) {
    GscBlastWaiterSet copy;GscEntityRef identity;char event[128];size_t length=0;int n;
    if(!r||!r->blast_enabled||!r->started||r->error[0]||!owner||!owner->epoch||!owner->generation||!name||!out) {
        if(out)memset(out,0,sizeof(*out));
        return bridge_entity_error(error,size,"invalid blast waiter capture context");
    }
    identity=*owner;
    while(length<sizeof(event)&&name[length])length++;
    if(length<sizeof(event))memcpy(event,name,length+1);
    memset(out,0,sizeof(*out));
    if(!length||length==sizeof(event)||(strcmp(event,"damage")&&strcmp(event,"death")))
        return bridge_entity_error(error,size,"invalid blast waiter capture event");
    memset(&copy,0,sizeof(copy));copy.runtime_token=r->array_token;copy.owner=identity;memcpy(copy.event,event,length+1);
    for(n=0;n<r->task_count;n++) {
        const Task *task=task_at(r,n);
        if(!payload_matches(task,&identity,event))continue;
        if(copy.count>=GSC_BLAST_WAITER_CAPACITY||!task->serial||!task->blast_wait_serial)
            return bridge_entity_error(error,size,"invalid blast wait registration");
        copy.registrations[copy.count].task_serial=task->serial;
        copy.registrations[copy.count++].registration_serial=task->blast_wait_serial;
    }
    *out=copy;return 1;
}
int gsc_blast_notify_captured(GscRuntime *r,const GscBlastWaiterSet *selected,
    const GscEventValue *values,size_t count,int *out_awakened) {
    GscBlastWaiterSet copy;if(out_awakened)*out_awakened=0;
    if(!selected)return 0;
    copy=*selected;
    return notify_with_payload_selected(r,&copy.owner,copy.event,values,count,out_awakened,1,&copy);
}
int gsc_notify_with_payload(GscRuntime *r,const char *name,
    const GscEventValue *values,size_t count,int *out_awakened) {
    return notify_with_payload(r,NULL,name,values,count,out_awakened,0);
}
int gsc_notify_entity_with_payload(GscRuntime *r,const GscEntityRef *owner,
    const char *name,const GscEventValue *values,size_t count,int *out_awakened) {
    return notify_with_payload(r,owner,name,values,count,out_awakened,1);
}
const char *gsc_runtime_error(const GscRuntime *r) {return r?r->error:"null runtime";}
int gsc_has_active_threads(const GscRuntime *r) {
    int n;if(!r)return 0;
    for(n=0;n<r->task_count;n++)if(task_at(r,n)->status!=TASK_DEAD)return 1;
    return 0;
}
int gsc_active_threads(const GscRuntime *r) {
    int n,count=0;if(!r)return 0;
    for(n=0;n<r->task_count;n++)if(task_at(r,n)->status!=TASK_DEAD)count++;
    return count;
}

#include "gsc_name_audit.inc"

/* R136 opt-in computed integer bounds; old constructors retain their lane. */
GscProgram *gsc_program_create_with_computed_randomint(void) {
    GscProgram *g=gsc_program_create_with_keybinding();
    if(g)g->computed_randomint_syntax=1;
    return g;
}
int gsc_program_link_with_computed_randomint(GscProgram *g,const char *entry_module,char *error,size_t error_size) {
    int ok;
    if(!g||!g->computed_randomint_syntax||g->linked) {
        if(error&&error_size)snprintf(error,error_size,"computed randomint program constructor is required");
        return 0;
    }
    g->computed_randomint_mode=1;
    ok=gsc_program_link_with_keybinding(g,entry_module,error,error_size);
    if(!ok)g->computed_randomint_mode=0;
    return ok;
}
GscRuntime *gsc_runtime_new_with_computed_randomint(const GscProgram *program,GscFogCallback fog,void *user,
    const GscEntityHost *entities,const GscEntityAccessHost *access,
    const GscEntityControlHost *controls,const GscEntityCollectionHost *collections,
    const GscEntityPredicateHost *predicates,const GscActorSpawnHost *actor_spawners,
    const GscMusicHost *music_host,const GscResourceHost *resource_host,
    const GscObjectiveHost *objective_host,const GscConsoleHost *console_host,const GscHudHost *hud_host,
    const GscMissionHost *mission_host,const GscBlastHost *blast_host,const GscQuakeHost *quake_host,const GscFxHost *fx_host,const GscCvarHost *cvar_host,const GscShellshockHost *shellshock_host,const GscDebugTextHost *debug_text_host,const GscDebugLineHost *debug_line_host,const GscAmbientHost *ambient_host,const GscPrecacheItemHost *precache_item_host,const GscSaveGameHost *save_game_host,const GscTagFxHost *tag_fx_host,const GscNativeQueryHost *native_query_host,const GscPrecacheStringHost *precache_string_host,const GscSpawnerArrayHost *spawner_array_host,const GscRandomIntHost *randomint_host,const GscRandomFloatHost *randomfloat_host,const GscKeyBindingHost *keybinding_host,char *error,size_t error_size) {
    GscRuntime *r;
    if(!program||!program->computed_randomint_mode||!program->keybinding_mode||!program->randomfloat_mode||!entities||!access||!controls) {
        if(error&&error_size)snprintf(error,error_size,"computed randomint constructor requires matching program and entity/access/control hosts");
        return NULL;
    }
    if(!keybinding_runtime_preflight(program,keybinding_host,error,error_size))return NULL;
    if(!randomfloat_runtime_preflight(program,randomfloat_host,error,error_size))return NULL;
    if(!precache_string_runtime_preflight(program,precache_string_host,error,error_size))return NULL;
    r=runtime_new_computed_randomint_common(program,fog,user,entities,access,1,1,controls,1,1,collections,
        1,1,1,predicates,1,actor_spawners,1,1,1,1,1,music_host,1,resource_host,1,objective_host,1,console_host,1,hud_host,1,mission_host,1,blast_host,1,quake_host,1,fx_host,1,cvar_host,1,shellshock_host,1,debug_text_host,1,debug_line_host,1,ambient_host,1,precache_item_host,1,save_game_host,1,tag_fx_host,1,native_query_host,1,spawner_array_host,1,randomint_host,1,0,0,NULL,0,0,0,error,error_size);
    if(r) {
        r->precache_string_enabled=1;r->next_precache_string_operation=1;if(precache_string_host)r->precache_string=*precache_string_host;
        r->randomfloat_enabled=1;r->next_randomfloat_operation=1;if(randomfloat_host)r->randomfloat=*randomfloat_host;
        r->keybinding_enabled=1;r->next_keybinding_operation=1;if(keybinding_host)r->keybinding=*keybinding_host;
    }
    return r;
}

/* R148 grouped independent native capabilities; previous selectors stay frozen. */
GscProgram *gsc_program_create_with_structs(void) {
    GscProgram *g=gsc_program_create_with_computed_randomint();
    if(g)g->struct_syntax=1;
    return g;
}
int gsc_program_link_with_structs(GscProgram *g,const char *entry_module,char *error,size_t error_size) {
    int ok;
    if(!g||!g->struct_syntax||g->linked) {
        if(error&&error_size)snprintf(error,error_size,"structure program constructor is required");
        return 0;
    }
    g->struct_mode=1;ok=gsc_program_link_with_computed_randomint(g,entry_module,error,error_size);
    if(!ok)g->struct_mode=0;
    return ok;
}
GscRuntime *gsc_runtime_new_with_structs(const GscProgram *program,GscFogCallback fog,void *user,
    const GscEntityHost *entities,const GscEntityAccessHost *access,
    const GscEntityControlHost *controls,const GscEntityCollectionHost *collections,
    const GscEntityPredicateHost *predicates,const GscActorSpawnHost *actor_spawners,
    const GscMusicHost *music_host,const GscResourceHost *resource_host,
    const GscObjectiveHost *objective_host,const GscConsoleHost *console_host,const GscHudHost *hud_host,
    const GscMissionHost *mission_host,const GscBlastHost *blast_host,const GscQuakeHost *quake_host,const GscFxHost *fx_host,const GscCvarHost *cvar_host,const GscShellshockHost *shellshock_host,const GscDebugTextHost *debug_text_host,const GscDebugLineHost *debug_line_host,const GscAmbientHost *ambient_host,const GscPrecacheItemHost *precache_item_host,const GscSaveGameHost *save_game_host,const GscTagFxHost *tag_fx_host,const GscNativeQueryHost *native_query_host,const GscPrecacheStringHost *precache_string_host,const GscSpawnerArrayHost *spawner_array_host,const GscRandomIntHost *randomint_host,const GscRandomFloatHost *randomfloat_host,const GscKeyBindingHost *keybinding_host,char *error,size_t error_size) {
    GscRuntime *r;
    if(!program||!program->struct_mode||!program->computed_randomint_mode||!program->keybinding_mode||!program->randomfloat_mode||!entities||!access||!controls) {
        if(error&&error_size)snprintf(error,error_size,"structure constructor requires matching program and entity/access/control hosts");
        return NULL;
    }
    if(!keybinding_runtime_preflight(program,keybinding_host,error,error_size))return NULL;
    if(!randomfloat_runtime_preflight(program,randomfloat_host,error,error_size))return NULL;
    if(!precache_string_runtime_preflight(program,precache_string_host,error,error_size))return NULL;
    r=runtime_new_computed_randomint_common(program,fog,user,entities,access,1,1,controls,1,1,collections,
        1,1,1,predicates,1,actor_spawners,1,1,1,1,1,music_host,1,resource_host,1,objective_host,1,console_host,1,hud_host,1,mission_host,1,blast_host,1,quake_host,1,fx_host,1,cvar_host,1,shellshock_host,1,debug_text_host,1,debug_line_host,1,ambient_host,1,precache_item_host,1,save_game_host,1,tag_fx_host,1,native_query_host,1,spawner_array_host,1,randomint_host,1,1,0,NULL,0,0,0,error,error_size);
    if(r) {
        r->precache_string_enabled=1;r->next_precache_string_operation=1;if(precache_string_host)r->precache_string=*precache_string_host;
        r->randomfloat_enabled=1;r->next_randomfloat_operation=1;if(randomfloat_host)r->randomfloat=*randomfloat_host;
        r->keybinding_enabled=1;r->next_keybinding_operation=1;if(keybinding_host)r->keybinding=*keybinding_host;
    }
    return r;
}

/* R149 entity query selector, preserving all prior constructors. */
GscProgram *gsc_program_create_with_entity_queries(void) {
    GscProgram *g=gsc_program_create_with_structs();
    if(g)g->entity_queries_syntax=1;
    return g;
}
int gsc_program_link_with_entity_queries(GscProgram *g,const char *entry_module,char *error,size_t error_size) {
    int ok;
    if(!g||!g->entity_queries_syntax||g->linked) {
        if(error&&error_size)snprintf(error,error_size,"entity query program constructor is required");
        return 0;
    }
    g->entity_queries_mode=1;ok=gsc_program_link_with_structs(g,entry_module,error,error_size);
    if(!ok)g->entity_queries_mode=0;
    return ok;
}
GscRuntime *gsc_runtime_new_with_entity_queries(const GscProgram *program,GscFogCallback fog,void *user,
    const GscEntityHost *entities,const GscEntityAccessHost *access,
    const GscEntityControlHost *controls,const GscEntityCollectionHost *collections,
    const GscEntityPredicateHost *predicates,const GscActorSpawnHost *actor_spawners,
    const GscMusicHost *music_host,const GscResourceHost *resource_host,
    const GscObjectiveHost *objective_host,const GscConsoleHost *console_host,const GscHudHost *hud_host,
    const GscMissionHost *mission_host,const GscBlastHost *blast_host,const GscQuakeHost *quake_host,const GscFxHost *fx_host,const GscCvarHost *cvar_host,const GscShellshockHost *shellshock_host,const GscDebugTextHost *debug_text_host,const GscDebugLineHost *debug_line_host,const GscAmbientHost *ambient_host,const GscPrecacheItemHost *precache_item_host,const GscSaveGameHost *save_game_host,const GscTagFxHost *tag_fx_host,const GscNativeQueryHost *native_query_host,const GscPrecacheStringHost *precache_string_host,const GscSpawnerArrayHost *spawner_array_host,const GscRandomIntHost *randomint_host,const GscRandomFloatHost *randomfloat_host,const GscKeyBindingHost *keybinding_host,const GscEntityQueryHost *entity_queries_host,char *error,size_t error_size) {
    GscRuntime *r;
    if(!program||!program->entity_queries_mode||!program->struct_mode||!program->computed_randomint_mode||!program->keybinding_mode||!program->randomfloat_mode||!entities||!access||!controls) {
        if(error&&error_size)snprintf(error,error_size,"structure constructor requires matching program and entity/access/control hosts");
        return NULL;
    }
    if(!keybinding_runtime_preflight(program,keybinding_host,error,error_size))return NULL;
    if(!randomfloat_runtime_preflight(program,randomfloat_host,error,error_size))return NULL;
    if(!precache_string_runtime_preflight(program,precache_string_host,error,error_size))return NULL;
    r=runtime_new_computed_randomint_common(program,fog,user,entities,access,1,1,controls,1,1,collections,
        1,1,1,predicates,1,actor_spawners,1,1,1,1,1,music_host,1,resource_host,1,objective_host,1,console_host,1,hud_host,1,mission_host,1,blast_host,1,quake_host,1,fx_host,1,cvar_host,1,shellshock_host,1,debug_text_host,1,debug_line_host,1,ambient_host,1,precache_item_host,1,save_game_host,1,tag_fx_host,1,native_query_host,1,spawner_array_host,1,randomint_host,1,1,1,entity_queries_host,0,0,0,error,error_size);
    if(r) {
        r->precache_string_enabled=1;r->next_precache_string_operation=1;if(precache_string_host)r->precache_string=*precache_string_host;
        r->randomfloat_enabled=1;r->next_randomfloat_operation=1;if(randomfloat_host)r->randomfloat=*randomfloat_host;
        r->keybinding_enabled=1;r->next_keybinding_operation=1;if(keybinding_host)r->keybinding=*keybinding_host;
    }
    return r;
}

/* R150 runtime-checked integer bounds, previous proof lanes stay frozen. */
GscProgram *gsc_program_create_with_dynamic_randomint(void) {
    GscProgram *g=gsc_program_create_with_entity_queries();
    if(g)g->dynamic_randomint_syntax=1;
    return g;
}
int gsc_program_link_with_dynamic_randomint(GscProgram *g,const char *entry_module,char *error,size_t error_size) {
    int ok;
    if(!g||!g->dynamic_randomint_syntax||g->linked) {
        if(error&&error_size)snprintf(error,error_size,"dynamic randomint program constructor is required");
        return 0;
    }
    g->dynamic_randomint_mode=1;ok=gsc_program_link_with_entity_queries(g,entry_module,error,error_size);
    if(!ok)g->dynamic_randomint_mode=0;
    return ok;
}
GscRuntime *gsc_runtime_new_with_dynamic_randomint(const GscProgram *program,GscFogCallback fog,void *user,
    const GscEntityHost *entities,const GscEntityAccessHost *access,
    const GscEntityControlHost *controls,const GscEntityCollectionHost *collections,
    const GscEntityPredicateHost *predicates,const GscActorSpawnHost *actor_spawners,
    const GscMusicHost *music_host,const GscResourceHost *resource_host,
    const GscObjectiveHost *objective_host,const GscConsoleHost *console_host,const GscHudHost *hud_host,
    const GscMissionHost *mission_host,const GscBlastHost *blast_host,const GscQuakeHost *quake_host,const GscFxHost *fx_host,const GscCvarHost *cvar_host,const GscShellshockHost *shellshock_host,const GscDebugTextHost *debug_text_host,const GscDebugLineHost *debug_line_host,const GscAmbientHost *ambient_host,const GscPrecacheItemHost *precache_item_host,const GscSaveGameHost *save_game_host,const GscTagFxHost *tag_fx_host,const GscNativeQueryHost *native_query_host,const GscPrecacheStringHost *precache_string_host,const GscSpawnerArrayHost *spawner_array_host,const GscRandomIntHost *randomint_host,const GscRandomFloatHost *randomfloat_host,const GscKeyBindingHost *keybinding_host,const GscEntityQueryHost *entity_queries_host,char *error,size_t error_size) {
    GscRuntime *r;
    if(!program||!program->dynamic_randomint_mode||!program->entity_queries_mode||!program->struct_mode||!program->computed_randomint_mode||!program->keybinding_mode||!program->randomfloat_mode||!entities||!access||!controls) {
        if(error&&error_size)snprintf(error,error_size,"structure constructor requires matching program and entity/access/control hosts");
        return NULL;
    }
    if(!keybinding_runtime_preflight(program,keybinding_host,error,error_size))return NULL;
    if(!randomfloat_runtime_preflight(program,randomfloat_host,error,error_size))return NULL;
    if(!precache_string_runtime_preflight(program,precache_string_host,error,error_size))return NULL;
    r=runtime_new_computed_randomint_common(program,fog,user,entities,access,1,1,controls,1,1,collections,
        1,1,1,predicates,1,actor_spawners,1,1,1,1,1,music_host,1,resource_host,1,objective_host,1,console_host,1,hud_host,1,mission_host,1,blast_host,1,quake_host,1,fx_host,1,cvar_host,1,shellshock_host,1,debug_text_host,1,debug_line_host,1,ambient_host,1,precache_item_host,1,save_game_host,1,tag_fx_host,1,native_query_host,1,spawner_array_host,1,randomint_host,1,1,1,entity_queries_host,1,0,0,error,error_size);
    if(r) {
        r->precache_string_enabled=1;r->next_precache_string_operation=1;if(precache_string_host)r->precache_string=*precache_string_host;
        r->randomfloat_enabled=1;r->next_randomfloat_operation=1;if(randomfloat_host)r->randomfloat=*randomfloat_host;
        r->keybinding_enabled=1;r->next_keybinding_operation=1;if(keybinding_host)r->keybinding=*keybinding_host;
    }
    return r;
}

/* R151 bounded original FLOAT GetInt conversion, earlier selectors retained. */
GscProgram *gsc_program_create_with_numeric_randomint(void) {
    GscProgram *g=gsc_program_create_with_dynamic_randomint();
    if(g)g->numeric_randomint_syntax=1;
    return g;
}
int gsc_program_link_with_numeric_randomint(GscProgram *g,const char *entry_module,char *error,size_t error_size) {
    int ok;
    if(!g||!g->numeric_randomint_syntax||g->linked) {
        if(error&&error_size)snprintf(error,error_size,"numeric randomint program constructor is required");
        return 0;
    }
    g->numeric_randomint_mode=1;ok=gsc_program_link_with_dynamic_randomint(g,entry_module,error,error_size);
    if(!ok)g->numeric_randomint_mode=0;
    return ok;
}
GscRuntime *gsc_runtime_new_with_numeric_randomint(const GscProgram *program,GscFogCallback fog,void *user,
    const GscEntityHost *entities,const GscEntityAccessHost *access,
    const GscEntityControlHost *controls,const GscEntityCollectionHost *collections,
    const GscEntityPredicateHost *predicates,const GscActorSpawnHost *actor_spawners,
    const GscMusicHost *music_host,const GscResourceHost *resource_host,
    const GscObjectiveHost *objective_host,const GscConsoleHost *console_host,const GscHudHost *hud_host,
    const GscMissionHost *mission_host,const GscBlastHost *blast_host,const GscQuakeHost *quake_host,const GscFxHost *fx_host,const GscCvarHost *cvar_host,const GscShellshockHost *shellshock_host,const GscDebugTextHost *debug_text_host,const GscDebugLineHost *debug_line_host,const GscAmbientHost *ambient_host,const GscPrecacheItemHost *precache_item_host,const GscSaveGameHost *save_game_host,const GscTagFxHost *tag_fx_host,const GscNativeQueryHost *native_query_host,const GscPrecacheStringHost *precache_string_host,const GscSpawnerArrayHost *spawner_array_host,const GscRandomIntHost *randomint_host,const GscRandomFloatHost *randomfloat_host,const GscKeyBindingHost *keybinding_host,const GscEntityQueryHost *entity_queries_host,char *error,size_t error_size) {
    GscRuntime *r;
    if(!program||!program->numeric_randomint_mode||!program->dynamic_randomint_mode||!program->entity_queries_mode||!program->struct_mode||!program->computed_randomint_mode||!program->keybinding_mode||!program->randomfloat_mode||!entities||!access||!controls) {
        if(error&&error_size)snprintf(error,error_size,"numeric randomint constructor requires matching program and entity/access/control hosts");
        return NULL;
    }
    if(!keybinding_runtime_preflight(program,keybinding_host,error,error_size))return NULL;
    if(!randomfloat_runtime_preflight(program,randomfloat_host,error,error_size))return NULL;
    if(!precache_string_runtime_preflight(program,precache_string_host,error,error_size))return NULL;
    r=runtime_new_computed_randomint_common(program,fog,user,entities,access,1,1,controls,1,1,collections,
        1,1,1,predicates,1,actor_spawners,1,1,1,1,1,music_host,1,resource_host,1,objective_host,1,console_host,1,hud_host,1,mission_host,1,blast_host,1,quake_host,1,fx_host,1,cvar_host,1,shellshock_host,1,debug_text_host,1,debug_line_host,1,ambient_host,1,precache_item_host,1,save_game_host,1,tag_fx_host,1,native_query_host,1,spawner_array_host,1,randomint_host,1,1,1,entity_queries_host,1,1,0,error,error_size);
    if(r) {
        r->precache_string_enabled=1;r->next_precache_string_operation=1;if(precache_string_host)r->precache_string=*precache_string_host;
        r->randomfloat_enabled=1;r->next_randomfloat_operation=1;if(randomfloat_host)r->randomfloat=*randomfloat_host;
        r->keybinding_enabled=1;r->next_keybinding_operation=1;if(keybinding_host)r->keybinding=*keybinding_host;
    }
    return r;
}

/* R152 exact-domain dynamic GetFloat arguments; previous lanes retained. */
GscProgram *gsc_program_create_with_dynamic_randomfloat(void) {
    GscProgram *g=gsc_program_create_with_numeric_randomint();
    if(g)g->dynamic_randomfloat_syntax=1;
    return g;
}
int gsc_program_link_with_dynamic_randomfloat(GscProgram *g,const char *entry_module,char *error,size_t error_size) {
    int ok;
    if(!g||!g->dynamic_randomfloat_syntax||g->linked) {
        if(error&&error_size)snprintf(error,error_size,"dynamic randomfloat program constructor is required");
        return 0;
    }
    g->dynamic_randomfloat_mode=1;ok=gsc_program_link_with_numeric_randomint(g,entry_module,error,error_size);
    if(!ok)g->dynamic_randomfloat_mode=0;
    return ok;
}
GscRuntime *gsc_runtime_new_with_dynamic_randomfloat(const GscProgram *program,GscFogCallback fog,void *user,
    const GscEntityHost *entities,const GscEntityAccessHost *access,
    const GscEntityControlHost *controls,const GscEntityCollectionHost *collections,
    const GscEntityPredicateHost *predicates,const GscActorSpawnHost *actor_spawners,
    const GscMusicHost *music_host,const GscResourceHost *resource_host,
    const GscObjectiveHost *objective_host,const GscConsoleHost *console_host,const GscHudHost *hud_host,
    const GscMissionHost *mission_host,const GscBlastHost *blast_host,const GscQuakeHost *quake_host,const GscFxHost *fx_host,const GscCvarHost *cvar_host,const GscShellshockHost *shellshock_host,const GscDebugTextHost *debug_text_host,const GscDebugLineHost *debug_line_host,const GscAmbientHost *ambient_host,const GscPrecacheItemHost *precache_item_host,const GscSaveGameHost *save_game_host,const GscTagFxHost *tag_fx_host,const GscNativeQueryHost *native_query_host,const GscPrecacheStringHost *precache_string_host,const GscSpawnerArrayHost *spawner_array_host,const GscRandomIntHost *randomint_host,const GscRandomFloatHost *randomfloat_host,const GscKeyBindingHost *keybinding_host,const GscEntityQueryHost *entity_queries_host,char *error,size_t error_size) {
    GscRuntime *r;
    if(!program||!program->dynamic_randomfloat_mode||!program->numeric_randomint_mode||!program->dynamic_randomint_mode||!program->entity_queries_mode||!program->struct_mode||!program->computed_randomint_mode||!program->keybinding_mode||!program->randomfloat_mode||!entities||!access||!controls) {
        if(error&&error_size)snprintf(error,error_size,"dynamic randomfloat constructor requires matching program and entity/access/control hosts");
        return NULL;
    }
    if(!keybinding_runtime_preflight(program,keybinding_host,error,error_size))return NULL;
    if(!randomfloat_runtime_preflight(program,randomfloat_host,error,error_size))return NULL;
    if(!precache_string_runtime_preflight(program,precache_string_host,error,error_size))return NULL;
    r=runtime_new_computed_randomint_common(program,fog,user,entities,access,1,1,controls,1,1,collections,
        1,1,1,predicates,1,actor_spawners,1,1,1,1,1,music_host,1,resource_host,1,objective_host,1,console_host,1,hud_host,1,mission_host,1,blast_host,1,quake_host,1,fx_host,1,cvar_host,1,shellshock_host,1,debug_text_host,1,debug_line_host,1,ambient_host,1,precache_item_host,1,save_game_host,1,tag_fx_host,1,native_query_host,1,spawner_array_host,1,randomint_host,1,1,1,entity_queries_host,1,1,1,error,error_size);
    if(r) {
        r->precache_string_enabled=1;r->next_precache_string_operation=1;if(precache_string_host)r->precache_string=*precache_string_host;
        r->randomfloat_enabled=1;r->next_randomfloat_operation=1;if(randomfloat_host)r->randomfloat=*randomfloat_host;
        r->keybinding_enabled=1;r->next_keybinding_operation=1;if(keybinding_host)r->keybinding=*keybinding_host;
    }
    return r;
}

GscRuntime *gsc_runtime_promote_bsp_dynamic_randomfloat(GscRuntime *target,const GscProgram *program,GscFogCallback fog,void *user,
    const GscEntityHost *entities,const GscEntityAccessHost *access,
    const GscEntityControlHost *controls,const GscEntityCollectionHost *collections,
    const GscEntityPredicateHost *predicates,const GscActorSpawnHost *actor_spawners,
    const GscMusicHost *music_host,const GscResourceHost *resource_host,
    const GscObjectiveHost *objective_host,const GscConsoleHost *console_host,const GscHudHost *hud_host,
    const GscMissionHost *mission_host,const GscBlastHost *blast_host,const GscQuakeHost *quake_host,const GscFxHost *fx_host,const GscCvarHost *cvar_host,const GscShellshockHost *shellshock_host,const GscDebugTextHost *debug_text_host,const GscDebugLineHost *debug_line_host,const GscAmbientHost *ambient_host,const GscPrecacheItemHost *precache_item_host,const GscSaveGameHost *save_game_host,const GscTagFxHost *tag_fx_host,const GscNativeQueryHost *native_query_host,const GscPrecacheStringHost *precache_string_host,const GscSpawnerArrayHost *spawner_array_host,const GscRandomIntHost *randomint_host,const GscRandomFloatHost *randomfloat_host,const GscKeyBindingHost *keybinding_host,const GscEntityQueryHost *entity_queries_host,char *error,size_t error_size) {
    GscRuntime *r;int i,j;
    if(target&&!gsc_runtime_storage_separate(target,error,error_size))return NULL;
    if(!target||!target->bsp_construction_only||target->bsp_promoted||target->program!=program||
       target->started||target->native_registry||target->error[0]||gsc_active_threads(target)||
       target->advancing||target->executing_task||target->entity_sequence_busy||target->entity_prepare_busy||
       target->entity_certificate_busy||target->entity_inheritance_busy||!target->array_token){
        if(error&&error_size)snprintf(error,error_size,"BSP promotion requires the current quiescent original runtime");return NULL;
    }
    /* The new profile uses case-insensitive controls: existing exact private
     * fields must remain unambiguous under that lookup policy. */
    for(i=0;i<target->state_field_capacity;i++)if(target->state_fields[i].used)
     for(j=0;j<i;j++)if(target->state_fields[j].used&&
      target->state_fields[i].owner_kind==target->state_fields[j].owner_kind&&
      state_ref_equal(&target->state_fields[i].owner,&target->state_fields[j].owner)&&
      control_field_equal(target->state_fields[i].name,target->state_fields[j].name)){
       if(error&&error_size)snprintf(error,error_size,"BSP private fields collide under promoted control lookup");return NULL;
     }
    if(!program||!program->dynamic_randomfloat_mode||!program->numeric_randomint_mode||!program->dynamic_randomint_mode||!program->entity_queries_mode||!program->struct_mode||!program->computed_randomint_mode||!program->keybinding_mode||!program->randomfloat_mode||!entities||!access||!controls) {
        if(error&&error_size)snprintf(error,error_size,"dynamic randomfloat constructor requires matching program and entity/access/control hosts");
        return NULL;
    }
    if(!keybinding_runtime_preflight(program,keybinding_host,error,error_size))return NULL;
    if(!randomfloat_runtime_preflight(program,randomfloat_host,error,error_size))return NULL;
    if(!precache_string_runtime_preflight(program,precache_string_host,error,error_size))return NULL;
    r=runtime_new_computed_randomint_common_into(target,program,fog,user,entities,access,1,1,controls,1,1,collections,
        1,1,1,predicates,1,actor_spawners,1,1,1,1,1,music_host,1,resource_host,1,objective_host,1,console_host,1,hud_host,1,mission_host,1,blast_host,1,quake_host,1,fx_host,1,cvar_host,1,shellshock_host,1,debug_text_host,1,debug_line_host,1,ambient_host,1,precache_item_host,1,save_game_host,1,tag_fx_host,1,native_query_host,1,spawner_array_host,1,randomint_host,1,1,1,entity_queries_host,1,1,1,error,error_size);
    if(r) {
        r->precache_string_enabled=1;r->next_precache_string_operation=1;if(precache_string_host)r->precache_string=*precache_string_host;
        r->randomfloat_enabled=1;r->next_randomfloat_operation=1;if(randomfloat_host)r->randomfloat=*randomfloat_host;
        r->keybinding_enabled=1;r->next_keybinding_operation=1;if(keybinding_host)r->keybinding=*keybinding_host;
    }
    return r;
}

#include "gsc_program_function_cache.inc"
