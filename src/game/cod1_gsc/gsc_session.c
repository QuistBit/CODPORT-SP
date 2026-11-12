#include "gsc_session.h"
#include "gsc_script_roots.h"
#include "stance-profile.h"
#include "move-delta-profile.h"
static int r243_move_delta_session_private(const void*,size_t);
static int r243_move_delta_program_select(GscSession*);
static int r243_move_delta_runtime_bind(GscSession*);
#include "actor-float-profile.h"
static int r243_actor_float_session_private(const void*,size_t);
static int r243_actor_float_program_select(GscSession*);
static int r243_actor_float_runtime_bind(GscSession*);
#include "animtime-profile.h"
static int r243_stance_session_private(const void*,size_t);
static int r243_animtime_session_private(const void*,size_t);
static int r243_command_session_private(const void*,size_t);
static int r243_critical_session_private(const void*,size_t);
static int r243_command_program_select(GscSession*);
static int r243_command_runtime_bind(GscSession*);
extern int R243ProgramSelectFlaggedAll(GscProgram*);
extern int R243RuntimeSelectFlaggedAll(GscRuntime*);
static int r243_stance_program_select(GscSession*);
static int r243_animtime_program_select(GscSession*);
static int r243_stance_runtime_bind(GscSession*);
static int r243_animtime_runtime_bind(GscSession*);
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gsc_ambient_payload.inc"
#include "gsc_precache_item_payload.inc"
#include "gsc_save_game_payload.inc"
#include "gsc_tag_fx_payload.inc"
#include "gsc_native_query_payload.inc"
#include "gsc_precache_string_payload.inc"
#include "gsc_randomint_payload.inc"
#include "gsc_randomfloat_payload.inc"
#include "gsc_keybinding_payload.inc"

#ifndef GSC_SESSION_CALLOC
#define GSC_SESSION_CALLOC calloc
#endif
#ifndef GSC_SESSION_MALLOC
#define GSC_SESSION_MALLOC malloc
#endif
#ifndef GSC_SESSION_FREE
#define GSC_SESSION_FREE free
#endif
struct GscSession { int actor_graph_query,actor_graph_check_busy;GscEntityRef actor_graph_row;uint64_t actor_graph_program; uint64_t node_graph_id,node_graph_receipt,node_graph_pin,node_graph_pin_id;void (*node_graph_retire)(uint64_t); GscSessionNodeProgramPlan node_program;int node_program_enabled,node_program_busy,node_program_admitted,node_admission_attempted;GscSessionNodeScriptSelection node_script_selection;int node_scripts_enabled,node_script_phase,node_script_map_busy; int stance_query_enabled;R243StanceHost stance_provider;int move_delta_query_enabled;R243MoveDeltaHost move_delta_provider;int actor_float_query_enabled;unsigned actor_float_profile_mask;R243ActorFloatHost actor_float_provider;R243AnimTimeHost animtime_provider;int animtime_query_enabled;
    GscSessionHost host;
    GscProgram *program;
    GscRuntime *runtime;
    GscSessionSnapshot view;
    int busy, abort_requested;
    int r237_frame_buckets_enabled,owned_wait_delivery_enabled;
    int r243_scripted_init_dependency_enabled;
    int r243_scripted_prefix_enabled;GscActorScriptedPrefixHost r243_scripted_prefix;
    int r243_scripted_start_enabled;GscActorScriptedStartHost r243_scripted_start;
    int r243_controller_enabled;GscActorScriptedControllerHost r243_controller;GscControllerSelectWithCleanup r243_select_with_cleanup;void*r243_source_controller,*r243_source_active;
    int r243_script_state_enabled;GscActorScriptStateHost r243_script_state;
    GscEntityHost entities;
    int entities_enabled;
    GscEntityAccessHost access;
    int access_enabled;
    int events_enabled, state_enabled;
    GscEntityControlHost controls;
    int controls_enabled, flow_enabled, arrays_enabled, results_enabled, modules_enabled, event_payloads_enabled;
    int sparse_numeric_arrays_enabled,owned_numeric_delete_enabled,r238_array_value_cow_enabled,r239_mixed_scalar_equality_enabled;int configured_precache_interval_enabled;int r243_thread_start_enabled,r243_same_bucket_wait_enabled;GscThreadEntityAuthorityHost r243_thread_entities;int r243_thread_entities_enabled;int native_animation_roots_enabled;
    GscEntityCollectionHost collections;
    GscEntityPredicateHost predicates;int predicates_enabled;
    GscActorSpawnHost actor_spawners;int actor_spawn_preparing;
    const GscActorSpawnTicket *actor_spawn_finishing;int actor_spawn_finish_published;
    const GscActorSpawnRequest *actor_spawn_prepare_request;int actor_spawners_enabled, function_refs_enabled, scalar_switch_enabled, lifetime_enabled, names_enabled, music_enabled;
    GscSessionMusicHost music;
    GscSessionResourceHost resources;int resources_enabled;
    GscSessionObjectiveHost objectives;int objectives_enabled;
    GscSessionConsoleHost console;int console_enabled;
    GscSessionHudHost hud;int hud_enabled;
    GscSessionMissionHost mission;int mission_enabled;
    GscSessionBlastHost blast;int blast_enabled,blast_notify_completed;
    int training_opening_enabled;int local_identifier_case_enabled;GscLoopSoundHost loop_sound;int loop_sound_enabled;GscSoundNotifyHost sound_notify;int sound_notify_enabled,sound_notify_call_active;GscScriptSoundHost script_sound;int script_sound_enabled;
    GscPlayerNativeHost player_natives;int execution_extensions,player_normal_health_enabled,player_viewmodel_enabled,player_max_ammo_enabled,map_game_array_enabled;
    GscEntityDeleteHost entity_delete;int entity_operations_enabled;
    GscEntityOriginMethodHost origin_method;int origin_method_enabled;
    GscEntityLinkHost entity_links;int entity_links_enabled;
    GscDeleteTransactionHost delete_transactions;int delete_transactions_enabled;
    GscLoopFxHost loop_fx;int loop_fx_enabled;
    GscSpatialNativeHost spatial_natives;int spatial_natives_enabled;
    GscNodeQueryHost node_queries;int node_queries_enabled;
    GscAiArrayHost ai_arrays;int ai_arrays_enabled;
    GscEntityFieldHost grenade_item_fields;int grenade_items_enabled,carbine_items_enabled;
    GscYawMotionHost yaw_motion;int yaw_motion_enabled;
    GscActorGoalHost actor_goal;int actor_goal_enabled;
    GscSessionTurretProgramPlan turret_program;
    unsigned char *turret_program_bytes[4];size_t turret_program_sizes[4];int training_rigs;GscMapExecutionCertificate training_rig_map;
    int turret_program_enabled,turret_program_busy,turret_program_admitted,turret_admission_attempted;
    GscTurretAnimationHost turret_animations;int turret_animations_enabled;GscActorAnimationHost actor_animations;int actor_animations_enabled;int flagged_all_restart_enabled,flagged_all_method_enabled;GscActorAnimationHost command_provider;GscActorCriticalHost critical_provider;int critical_selected;int actor_animation_end_enabled;uint64_t actor_animation_end_runtime,actor_animation_end_catalog;GscActorLookHost actor_look;int actor_look_enabled;GscActorPresenceHost actor_presence;int actor_presence_enabled;GscActorDropHost actor_drop;int actor_drop_enabled;GscAnimationLengthHost namespace_length;int namespace_length_enabled;int random_range_enabled;int r232_source_float_enabled;int quoted_strings_enabled;int function_admission_selected;int developer_selected;GscDeveloperScriptSelection developer;
    GscWeaponClassnameHost weapon_classname;int weapon_classname_enabled,weapon_classname_busy;
    GscWeaponModelHost weapon_model;int weapon_model_enabled;
    GscTurretHost turrets;int turrets_enabled;
    GscSessionActorProgramPlan actor_program;GscEntityAccessHost actor_program_base_access;
    GscSessionScriptRootsPlan script_roots;
    int script_roots_enabled,script_roots_busy,script_roots_ready;
    char script_map_module[128],script_name_pool[GSC_GLOBAL_SCRIPT_ROOT_COUNT][64];
    unsigned script_name_count;
    struct {uint32_t function;uint16_t name,padding;} script_root_cache[GSC_GLOBAL_SCRIPT_ROOT_COUNT];
    uint64_t script_roots_program;
    uint32_t script_init_function,script_map_function;
    uint64_t actor_cache_program;
    uint32_t actor_function_cache[GSC_ACTOR_BOOTSTRAP_MODULE_CAPACITY][3];
    int actor_cache_ready;
    int actor_program_enabled,actor_program_busy,actor_admission_attempted;
    GscSessionActorBootstrapHost actor_bootstrap;
    int actor_bootstrap_enabled,actor_bootstrap_running,actor_bootstrap_attempted,actor_map_busy;
    int actor_construct_attempted,actor_construct_running,actor_construct_complete;
    uint64_t actor_construct_program;
    uint32_t actor_construct_count;
    GscEntityScriptEntry actor_construct_entries[GSC_AI_ARRAY_CAPACITY];
    GscHudElemHost hud_elements;int hud_elements_enabled;
    GscEntityCollisionHost entity_collision;int entity_collision_enabled;
    GscSessionQuakeHost quake;int quake_enabled;
    GscSessionFxHost fx;int fx_enabled;
    GscSessionCvarHost cvars;int cvars_enabled;
    GscSessionShellshockHost shellshock;int shellshock_enabled,shellshock_initial_window;
    GscSessionDebugTextHost debug_text;int debug_text_enabled;
    GscSessionDebugLineHost debug_line;int debug_line_enabled;
    GscSessionAmbientHost ambient;int ambient_enabled;
    GscSessionPrecacheItemHost precache_item;int precache_item_enabled;
    GscSessionSaveGameHost save_game;int save_game_enabled;
    GscSessionTagFxHost tag_fx;int tag_fx_enabled;
    GscSessionNativeQueryHost native_queries;int native_queries_enabled;
    GscSessionPrecacheStringHost precache_string;int precache_string_enabled;
    GscSessionSpawnerArrayHost spawner_arrays;int spawner_arrays_enabled;
    GscSessionRandomIntHost randomint;int randomint_enabled;
    GscSessionRandomFloatHost randomfloat;int randomfloat_enabled;
    GscSessionKeyBindingHost keybinding;int keybinding_enabled;
    int computed_randomint_enabled;
    int dynamic_randomint_enabled;
    int numeric_randomint_enabled;
    int dynamic_randomfloat_enabled;
    int structs_enabled;
    GscSessionEntityQueryHost entity_queries;int entity_queries_enabled;
    GscAnimationCatalog *start_origin_catalog;
    GscMapExecutionCertificate start_origin_map;
    GscStartOriginHost start_origin_host;
    uint32_t start_origin_ordinal;
    int start_origin_enabled,start_origin_map_busy;
    GscAnimationCatalog *animation_namespace_catalogs[4];
    GscMapExecutionCertificate animation_namespace_map;
    int animation_namespace_enabled,animation_namespace_map_busy;
    GscStartOriginHost namespace_start_origin_host;
    uint32_t namespace_start_origin_ordinals[2];
    int namespace_start_origin_enabled;
    GscRuntime *bsp_runtime;void *bsp_scope;int bsp_promoted;uint32_t bsp_count;GscEntityScriptEntry bsp_entries[GSC_AI_ARRAY_CAPACITY];
};
static void session_message(char *dst, size_t cap, const char *text) {
    if (dst && cap) snprintf(dst, cap, "%s", text ? text : "adapter failure");
}
static int session_reject(char *err, size_t cap, const char *text) {
    session_message(err, cap, text); return 0;
}
static void session_terminal(GscSession *s, const char *text) {
    if (!s->view.error[0]) session_message(s->view.error, sizeof(s->view.error), text);
}
static int session_enter(GscSession *s, char *err, size_t cap) {
    session_message(err, cap, "");
    if (!s) return session_reject(err, cap, "null adapter");
    if (s->busy) {
        s->abort_requested = 1;
        session_terminal(s, "reentrant adapter operation");
        return session_reject(err, cap, "reentrant adapter operation");
    }
    s->busy = 1; s->abort_requested = 0; return 1;
}
static int session_turret_program_current(GscSession*);
static int session_actor_program_current(GscSession*);
static int session_script_roots_current(GscSession*);
static int session_script_roots_cache_current(const GscSession*);
static int session_source_private(const GscSession*,const void*,size_t);
static int session_node_scripts_current(GscSession*);
static int session_node_scripts_publish(GscSession*);
static void session_capture(GscSession *s) {
    if (!s->runtime) return;
    (void)gsc_runtime_native_stats(s->runtime, &s->view.native);
    (void)gsc_runtime_fog_native_stats(s->runtime, &s->view.fog);
}
static void session_node_graph_retire(GscSession*s){
 uint64_t receipt=s->node_graph_receipt;void(*retire)(uint64_t)=s->node_graph_retire;
 s->node_graph_receipt=s->node_graph_id=0;s->node_graph_retire=NULL;
 if(receipt&&retire)retire(receipt);
}
static int session_drop(GscSession *s) {
    char why[256];
    session_capture(s);
    session_node_graph_retire(s);
    s->node_script_phase=0;gsc_runtime_node_scripts_retire(s->runtime);
    if(s->node_program_enabled){memset(&s->node_script_selection.map_certificate,0,sizeof(s->node_script_selection.map_certificate));s->node_program_admitted=s->node_admission_attempted=0;}
    if(!gsc_runtime_retire_hud_elements(s->runtime,why,sizeof(why))){session_terminal(s,why);return 0;}
    s->actor_construct_running=s->actor_construct_complete=0;
    s->actor_construct_program=0;s->actor_construct_count=0;
    memset(s->actor_construct_entries,0,sizeof(s->actor_construct_entries));
    gsc_runtime_free(s->runtime); s->runtime = NULL;
    gsc_runtime_free(s->bsp_runtime);s->bsp_runtime=NULL;s->bsp_scope=NULL;s->bsp_promoted=0;s->bsp_count=0;memset(s->bsp_entries,0,sizeof(s->bsp_entries));
    s->script_roots_ready=0;s->script_roots_program=0;s->script_name_count=0;
    s->script_init_function=s->script_map_function=0;
    memset(s->script_name_pool,0,sizeof(s->script_name_pool));memset(s->script_root_cache,0,sizeof(s->script_root_cache));
    s->actor_cache_ready=0;s->actor_cache_program=0;memset(s->actor_function_cache,0,sizeof(s->actor_function_cache));
    gsc_program_free(s->program); s->program = NULL;
    if(s->turret_program_enabled){
        memset(&s->animation_namespace_map,0,sizeof(s->animation_namespace_map));
        memset(&s->turret_animations,0,sizeof(s->turret_animations));
        memset(&s->developer.certificate,0,sizeof(s->developer.certificate));
        s->turret_program_admitted=s->turret_admission_attempted=0;
    }
    if(s->actor_program_enabled){memset(&s->actor_bootstrap,0,sizeof(s->actor_bootstrap));s->actor_bootstrap_enabled=0;s->actor_admission_attempted=0;s->access=s->actor_program_base_access;}
    return 1;
}
static void session_clear(GscSession *s) {
    uint64_t id = s->view.session_id;
    s->actor_bootstrap_attempted=s->actor_bootstrap_running=s->actor_map_busy=0;
    s->actor_construct_attempted=s->actor_construct_running=s->actor_construct_complete=0;
    s->actor_construct_program=0;s->actor_construct_count=0;
    memset(s->actor_construct_entries,0,sizeof(s->actor_construct_entries));
    memset(&s->view, 0, sizeof(s->view)); s->view.session_id = id;
    s->view.state = s->host.enabled ? GSC_SESSION_EMPTY : GSC_SESSION_DISABLED;
    s->view.origin_host_ms = s->view.last_host_ms = -1; s->view.logical_ms = -1;
    s->view.native.isdefined_builtin_index = s->view.native.isdefined_import_index = -1;
    s->view.fog.exp_builtin_index = s->view.fog.exp_import_index = -1;
    s->view.fog.cull_builtin_index = s->view.fog.cull_import_index = -1;
}
static int session_failed(GscSession *s, const char *text, char *err, size_t cap) {
    session_terminal(s, text);
    session_drop(s);
    s->view.state = s->host.enabled ? GSC_SESSION_FAILED : GSC_SESSION_DISABLED;
    session_message(err, cap, s->view.error);
    s->busy = 0; return 0;
}
static int session_leave_rejected(GscSession *s, const char *text, char *err, size_t cap) {
    s->busy = 0; return session_reject(err, cap, text);
}
static int session_new_id(GscSession *s) {
    if (s->view.session_id == UINT64_MAX) return 0;
    ++s->view.session_id; return 1;
}
static int session_id_matches(const GscSession *s, uint64_t id) {
    return id != 0 && id == s->view.session_id;
}
static int session_path(const char *path, char *module) {
    size_t i, n = 0, segment = 0;
    if (!path) return 0;
    while (n < GSC_SESSION_PATH_CAP && path[n]) ++n;
    if (n < 5 || n == GSC_SESSION_PATH_CAP || strcmp(path + n - 4, ".gsc")) return 0;
    for (i = 0; i < n - 4; ++i) {
        unsigned char c = (unsigned char)path[i];
        if (c == '/') { if (!segment) return 0; segment = 0; continue; }
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
              (segment && c >= '0' && c <= '9') || c == '_')) return 0;
        ++segment;
    }
    if (!segment) return 0;
    memcpy(module, path, n - 4); module[n - 4] = 0; return 1;
}
static int session_fog(void *user, const GscFogEvent *event) {
    GscSession *s = user;
    GscFogEvent copy;
    int ok;
    if (!s || !s->busy || s->abort_requested || !event ||
        s->view.state != GSC_SESSION_RUNNING || !s->host.fog) return 0;
    copy = *event;
    ok = s->host.fog(s->host.user, s->view.session_id, s->view.logical_ms, &copy);
    return ok && !s->abort_requested;
}
static int session_entity_lookup(void *user,const char *name,const char *field,
    GscEntityRef *out,int *found,char *error,size_t size) {
    GscSession *s=user;GscEntityRef ref;int got=-1,ok;
    if(!s||!s->busy||s->abort_requested||!s->entities_enabled||
       s->view.state!=GSC_SESSION_RUNNING||!out||!found)
        return session_reject(error,size,"entity lookup outside running session");
    memset(&ref,0,sizeof(ref));
    ok=s->entities.lookup(s->entities.user,name,field,&ref,&got,error,size);
    if(!ok||s->abort_requested)return 0;
    *out=ref;*found=got;return 1;
}
static int session_entity_setorigin(void *user,const GscEntityRef *entity,
    const float origin[3],char *error,size_t size) {
    GscSession *s=user;GscEntityRef ref;float copy[3];int ok;
    if(!s||!s->busy||s->abort_requested||!s->entities_enabled||
       s->view.state!=GSC_SESSION_RUNNING||!entity||!origin)
        return session_reject(error,size,"entity mutation outside running session");
    ref=*entity;memcpy(copy,origin,sizeof(copy));
    ok=s->entities.setorigin(s->entities.user,&ref,copy,error,size);
    return ok&&!s->abort_requested;
}
static int session_entity_validate(void *user,const GscEntityRef *entity) {
    GscSession *s=user;GscEntityRef ref;int valid;
    if(!s||!s->busy||s->abort_requested||!s->entities_enabled||
       (s->view.state!=GSC_SESSION_RUNNING&&
        !(s->actor_construct_running&&s->actor_bootstrap_running&&s->runtime&&
          s->view.state==GSC_SESSION_PREPARED&&s->view.logical_ms==0)&&
        !(s->blast_enabled&&s->blast_notify_completed&&s->view.state==GSC_SESSION_COMPLETED&&
          s->runtime&&!gsc_active_threads(s->runtime)))||!entity)return 0;
    ref=*entity;valid=s->entities.validate(s->entities.user,&ref);
    return valid==1&&!s->abort_requested;
}
static int session_entity_spawn(void *user,const char *classname,const float origin[3],
    GscEntityRef *out,char *error,size_t size) {
    GscSession *s=user;GscEntityRef ref;float copy[3];int ok;
    if(!s||!s->busy||s->abort_requested||!s->entities_enabled||!s->entities.spawn||
       s->view.state!=GSC_SESSION_RUNNING||!classname||!origin||!out)
        return session_reject(error,size,"entity creation outside running session");
    memset(&ref,0,sizeof(ref));memcpy(copy,origin,sizeof(copy));
    ok=s->entities.spawn(s->entities.user,classname,copy,&ref,error,size);
    if(!ok||s->abort_requested)return 0;
    *out=ref;return 1;
}
/* R205 forward the existing R138 capability without using the two-argument
 * callback as a substitute. The compiler still owns its frozen class/flags
 * admission; this adapter copies inputs and stages output across the host. */
static int session_entity_spawn_with_flags(void *user,const char *classname,const float origin[3],
    int32_t flags,GscEntityRef *out,char *error,size_t size) {
    GscSession *s=user;GscRuntime *runtime;GscEntityRef ref;uint64_t id;
    char name[128],expected_name[128];float position[3],expected_position[3];size_t n;int now,ok;
    if(!s||!s->busy||s->abort_requested||s->view.error[0]||!s->entities_enabled||
       !s->entities.spawn_with_flags||s->view.state!=GSC_SESSION_RUNNING||!s->runtime||
       !classname||!origin||!out)
        return session_reject(error,size,"flagged entity creation outside running session");
    memset(name,0,sizeof(name));
    for(n=0;n<sizeof(name)&&classname[n];++n)name[n]=classname[n];
    if(!n||n==sizeof(name))return session_reject(error,size,"flagged spawn requires a bounded classname");
    for(n=0;n<3;n++)if(!isfinite(origin[n]))return session_reject(error,size,"flagged spawn requires finite origin");
    memcpy(expected_name,name,sizeof(name));memcpy(position,origin,sizeof(position));
    memcpy(expected_position,position,sizeof(position));memset(&ref,0,sizeof(ref));
    id=s->view.session_id;runtime=s->runtime;now=s->view.logical_ms;
    ok=s->entities.spawn_with_flags(s->entities.user,name,position,flags,&ref,error,size);
    if(!s->busy||s->abort_requested||s->view.error[0]||s->view.state!=GSC_SESSION_RUNNING||
       s->view.session_id!=id||s->runtime!=runtime||s->view.logical_ms!=now)return 0;
    if(memcmp(name,expected_name,sizeof(name))||memcmp(position,expected_position,sizeof(position)))
        return session_reject(error,size,"flagged spawn host mutated borrowed classname or origin");
    if(ok!=1)return ok==0?0:session_reject(error,size,"flagged spawn host returned invalid success status");
    *out=ref;return 1;
}
static int session_access_ready(const GscSession *s) {
    return s&&s->busy&&!s->abort_requested&&s->access_enabled&&
        s->view.state==GSC_SESSION_RUNNING;
}
static int session_access_setmodel(void *user,const GscEntityRef *entity,
    const char *model,char *error,size_t size) {
    GscSession *s=user;GscEntityRef ref;int ok;
    if(!session_access_ready(s)||!s->access.setmodel||!entity||!model)
        return session_reject(error,size,"model mutation outside running session");
    ref=*entity;ok=s->access.setmodel(s->access.user,&ref,model,error,size);
    return ok&&!s->abort_requested;
}
static int session_access_getorigin(void *user,const GscEntityRef *entity,
    float out[3],char *error,size_t size) {
    GscSession *s=user;GscEntityRef ref;float copy[3];int ok;
    if(!session_access_ready(s)||!s->access.getorigin||!entity||!out)
        return session_reject(error,size,"origin read outside running session");
    ref=*entity;memcpy(copy,out,sizeof(copy));
    ok=s->access.getorigin(s->access.user,&ref,copy,error,size);
    if(!ok||s->abort_requested)return 0;
    memcpy(out,copy,sizeof(copy));return 1;
}
static int session_origin_method_read(void *user,const GscEntityRef *entity,float out[3],char *error,size_t size) {
    GscSession *s=user;GscEntityRef ref;GscRuntime *runtime;uint64_t id;int now,ok;float copy[3];
    if(!session_access_ready(s)||!s->origin_method_enabled||!s->origin_method.read||!entity||!out)
        return session_reject(error,size,"GetOrigin method outside running selected Session");
    ref=*entity;runtime=s->runtime;id=s->view.session_id;now=s->view.logical_ms;
    memcpy(copy,out,sizeof(copy));ok=s->origin_method.read(s->origin_method.user,&ref,copy,error,size);
    if(!s->busy||s->abort_requested||s->view.error[0]||s->view.state!=GSC_SESSION_RUNNING||
       s->runtime!=runtime||s->view.session_id!=id||s->view.logical_ms!=now)return 0;
    if(ref.epoch!=entity->epoch||ref.slot!=entity->slot||ref.generation!=entity->generation)
        return session_reject(error,size,"GetOrigin host changed borrowed Session identity");
    if(ok!=1)return ok==0?0:session_reject(error,size,"GetOrigin host returned invalid status");
    memcpy(out,copy,sizeof(copy));return 1;
}
static int session_access_getangles(void *user,const GscEntityRef *entity,
    float out[3],char *error,size_t size) {
    GscSession *s=user;GscEntityRef ref;float copy[3];int ok;
    if(!session_access_ready(s)||!s->access.getangles||!entity||!out)
        return session_reject(error,size,"angles read outside running session");
    ref=*entity;memcpy(copy,out,sizeof(copy));
    ok=s->access.getangles(s->access.user,&ref,copy,error,size);
    if(!ok||s->abort_requested)return 0;
    memcpy(out,copy,sizeof(copy));return 1;
}
static int session_access_setangles(void *user,const GscEntityRef *entity,
    const float angles[3],char *error,size_t size) {
    GscSession *s=user;GscEntityRef ref;float copy[3];int ok;
    if(!session_access_ready(s)||!s->access.setangles||!entity||!angles)
        return session_reject(error,size,"angles mutation outside running session");
    ref=*entity;memcpy(copy,angles,sizeof(copy));
    ok=s->access.setangles(s->access.user,&ref,copy,error,size);
    return ok&&!s->abort_requested;
}
static int session_control_getstring(void *user,const GscEntityRef *entity,const char *name,
    char *out,size_t capacity,int *found,char *error,size_t size) {
    GscSession *s=user;GscEntityRef ref;char copy[128];int got=-1,ok;
    if(!s||!s->busy||s->abort_requested||!s->controls_enabled||
       !s->controls.getstring||s->view.state!=GSC_SESSION_RUNNING||!entity||!name||
       !out||!found||!capacity||capacity>sizeof(copy))
        return session_reject(error,size,"metadata read outside running control session");
    ref=*entity;memset(copy,0xa5,sizeof(copy));
    ok=s->controls.getstring(s->controls.user,&ref,name,copy,capacity,&got,error,size);
    if(!ok||s->abort_requested)return 0;
    memcpy(out,copy,capacity);*found=got;return 1;
}
static int session_control_setvisible(void *user,const GscEntityRef *entity,
    int visible,char *error,size_t size) {
    GscSession *s=user;GscEntityRef ref;int ok;
    if(!s||!s->busy||s->abort_requested||!s->controls_enabled||
       !s->controls.setvisible||s->view.state!=GSC_SESSION_RUNNING||!entity||
       (visible!=0&&visible!=1))
        return session_reject(error,size,"visibility mutation outside running control session");
    ref=*entity;ok=s->controls.setvisible(s->controls.user,&ref,visible,error,size);
    return ok&&!s->abort_requested;
}
static int session_control_setorigin(void *user,const GscEntityRef *entity,
    const float origin[3],char *error,size_t size) {
    GscSession *s=user;GscEntityRef ref;float copy[3];int ok;
    if(!s||!s->busy||s->abort_requested||!s->controls_enabled||
       !s->controls.setorigin||s->view.state!=GSC_SESSION_RUNNING||!entity||!origin)
        return session_reject(error,size,"origin mutation outside running control session");
    ref=*entity;memcpy(copy,origin,sizeof(copy));
    ok=s->controls.setorigin(s->controls.user,&ref,copy,error,size);
    return ok&&!s->abort_requested;
}
static int session_collection_lookup(void *user,const char *name,const char *field,
    GscEntityRef *out,size_t capacity,size_t *count,char *error,size_t size) {
    GscSession *s=user;GscEntityRef copy[GSC_ARRAY_CONTAINER_CAPACITY];size_t got=(size_t)-1;int ok;
    if(!s||!s->busy||s->abort_requested||!s->arrays_enabled||!s->collections.lookup_array||
       s->view.state!=GSC_SESSION_RUNNING||!name||!field||!out||!count||!capacity||
       capacity>GSC_ARRAY_CONTAINER_CAPACITY)
        return session_reject(error,size,"collection read outside running array session");
    memset(copy,0,sizeof(copy));
    ok=s->collections.lookup_array(s->collections.user,name,field,copy,capacity,&got,error,size);
    if(!ok||s->abort_requested)return 0;
    if(got>capacity)return session_reject(error,size,"collection host overflow");
    memcpy(out,copy,got*sizeof(copy[0]));*count=got;return 1;
}
static GscSession *session_create(const GscSessionHost *host,const GscEntityHost *entities,
    const GscEntityAccessHost *access,int events,int state,const GscEntityControlHost *controls,int flow,int arrays,const GscEntityCollectionHost *collections,int results,
    char *err,size_t cap) {
    GscSession *s;
    session_message(err, cap, "");
    if (!host || (host->enabled != 0 && host->enabled != 1)) {
        session_reject(err, cap, "invalid adapter configuration"); return NULL;
    }
    if (host->enabled && (!host->read || !host->release || !host->fog)) {
        session_reject(err, cap, "enabled adapter requires read, release and fog callbacks"); return NULL;
    }
    if(entities&&(!entities->lookup||!entities->setorigin||!entities->validate)) {
        session_reject(err,cap,"entity adapter requires lookup, setorigin and validate callbacks");return NULL;
    }
    if(access&&!entities) {
        session_reject(err,cap,"access adapter requires an entity host");return NULL;
    }
    if(events&&(!entities||!access)) {
        session_reject(err,cap,"event adapter requires entity and access hosts");return NULL;
    }
    if(controls&&(!entities||!access)) {
        session_reject(err,cap,"control adapter requires entity and access hosts");return NULL;
    }
    s = GSC_SESSION_CALLOC(1, sizeof(*s));
    if (!s) { session_reject(err, cap, "adapter allocation failed"); return NULL; }
    s->host = *host;
    if(entities) {s->entities=*entities;s->entities_enabled=1;}
    if(access) {s->access=*access;s->access_enabled=1;}
    s->events_enabled=events;
    s->state_enabled=state;
    if(controls){s->controls=*controls;s->controls_enabled=1;}
    s->flow_enabled=flow;s->arrays_enabled=arrays;s->results_enabled=results;
    if(collections)s->collections=*collections;
    session_clear(s); return s;
}
GscSession *gsc_session_create(const GscSessionHost *host,char *err,size_t cap) {
    return session_create(host,NULL,NULL,0,0,NULL,0,0,NULL,0,err,cap);
}
GscSession *gsc_session_create_with_entities(const GscSessionHost *host,
    const GscEntityHost *entities,char *err,size_t cap) {
    if(!entities) {session_reject(err,cap,"entity host is required");return NULL;}
    return session_create(host,entities,NULL,0,0,NULL,0,0,NULL,0,err,cap);
}
GscSession *gsc_session_create_with_access(const GscSessionHost *host,
    const GscEntityHost *entities,const GscEntityAccessHost *access,char *err,size_t cap) {
    if(!entities||!access) {session_reject(err,cap,"entity and access hosts are required");return NULL;}
    return session_create(host,entities,access,0,0,NULL,0,0,NULL,0,err,cap);
}
GscSession *gsc_session_create_with_events(const GscSessionHost *host,
    const GscEntityHost *entities,const GscEntityAccessHost *access,char *err,size_t cap) {
    if(!entities||!access) {session_reject(err,cap,"entity and access hosts are required");return NULL;}
    return session_create(host,entities,access,1,0,NULL,0,0,NULL,0,err,cap);
}
GscSession *gsc_session_create_with_state(const GscSessionHost *host,
    const GscEntityHost *entities,const GscEntityAccessHost *access,char *err,size_t cap) {
    if(!entities||!access) {session_reject(err,cap,"entity and access hosts are required");return NULL;}
    return session_create(host,entities,access,1,1,NULL,0,0,NULL,0,err,cap);
}
GscSession *gsc_session_create_with_controls(const GscSessionHost *host,
    const GscEntityHost *entities,const GscEntityAccessHost *access,
    const GscEntityControlHost *controls,char *err,size_t cap) {
    if(!entities||!access||!controls) {
        session_reject(err,cap,"entity, access and control hosts are required");return NULL;
    }
    return session_create(host,entities,access,1,1,controls,0,0,NULL,0,err,cap);
}
GscSession *gsc_session_create_with_flow(const GscSessionHost *host,
    const GscEntityHost *entities,const GscEntityAccessHost *access,
    const GscEntityControlHost *controls,char *err,size_t cap) {
    if(!entities||!access||!controls) {
        session_reject(err,cap,"entity, access and control hosts are required");return NULL;
    }
    return session_create(host,entities,access,1,1,controls,1,0,NULL,0,err,cap);
}
GscSession *gsc_session_create_with_arrays(const GscSessionHost *host,
    const GscEntityHost *entities,const GscEntityAccessHost *access,
    const GscEntityControlHost *controls,const GscEntityCollectionHost *collections,
    char *err,size_t cap) {
    if(!entities||!access||!controls) {
        session_reject(err,cap,"entity, access and control hosts are required");return NULL;
    }
    return session_create(host,entities,access,1,1,controls,1,1,collections,0,err,cap);
}
GscSession *gsc_session_create_with_results(const GscSessionHost *host,
    const GscEntityHost *entities,const GscEntityAccessHost *access,
    const GscEntityControlHost *controls,const GscEntityCollectionHost *collections,
    char *err,size_t cap) {
    if(!entities||!access||!controls) {
        session_reject(err,cap,"entity, access and control hosts are required");return NULL;
    }
    return session_create(host,entities,access,1,1,controls,1,1,collections,1,err,cap);
}
GscSession *gsc_session_create_with_modules(const GscSessionHost *host,
    const GscEntityHost *entities,const GscEntityAccessHost *access,
    const GscEntityControlHost *controls,const GscEntityCollectionHost *collections,
    char *err,size_t cap) {
    GscSession *s=gsc_session_create_with_results(host,entities,access,controls,
                                               collections,err,cap);
    if(s)s->modules_enabled=1;
    return s;
}
GscSession *gsc_session_create_with_event_payloads(const GscSessionHost *host,
    const GscEntityHost *entities,const GscEntityAccessHost *access,
    const GscEntityControlHost *controls,const GscEntityCollectionHost *collections,
    char *err,size_t cap) {
    GscSession *s=gsc_session_create_with_modules(host,entities,access,controls,
                                               collections,err,cap);
    if(s)s->event_payloads_enabled=1;
    return s;
}
GscSession *gsc_session_create_with_predicates(const GscSessionHost *host,
    const GscEntityHost *entities,const GscEntityAccessHost *access,
    const GscEntityControlHost *controls,const GscEntityCollectionHost *collections,
    const GscEntityPredicateHost *predicates,char *err,size_t cap) {
    GscSession *s=gsc_session_create_with_event_payloads(host,entities,access,controls,
                                                       collections,err,cap);
    if(s){s->predicates_enabled=1;if(predicates)s->predicates=*predicates;}
    return s;
}
GscSession *gsc_session_create_with_actor_spawners(const GscSessionHost *host,
    const GscEntityHost *entities,const GscEntityAccessHost *access,
    const GscEntityControlHost *controls,const GscEntityCollectionHost *collections,
    const GscEntityPredicateHost *predicates,const GscActorSpawnHost *actor_spawners,
    char *err,size_t cap) {
    GscSession *s=gsc_session_create_with_predicates(host,entities,access,controls,
                                                   collections,predicates,err,cap);
    if(s){s->actor_spawners_enabled=1;if(actor_spawners)s->actor_spawners=*actor_spawners;}
    return s;
}
GscSession *gsc_session_create_with_function_refs(const GscSessionHost *host,
    const GscEntityHost *entities,const GscEntityAccessHost *access,
    const GscEntityControlHost *controls,const GscEntityCollectionHost *collections,
    const GscEntityPredicateHost *predicates,const GscActorSpawnHost *actor_spawners,
    char *err,size_t cap) {
    GscSession *s=gsc_session_create_with_actor_spawners(host,entities,access,controls,
                                                      collections,predicates,actor_spawners,err,cap);
    if(s)s->function_refs_enabled=1;
    return s;
}
GscSession *gsc_session_create_with_scalar_switch(const GscSessionHost *host,
    const GscEntityHost *entities,const GscEntityAccessHost *access,
    const GscEntityControlHost *controls,const GscEntityCollectionHost *collections,
    const GscEntityPredicateHost *predicates,const GscActorSpawnHost *actor_spawners,
    char *err,size_t cap) {
    GscSession *s=gsc_session_create_with_function_refs(host,entities,access,controls,
                                                      collections,predicates,actor_spawners,err,cap);
    if(s)s->scalar_switch_enabled=1;
    return s;
}
GscSession *gsc_session_create_with_lifetime(const GscSessionHost *host,
    const GscEntityHost *entities,const GscEntityAccessHost *access,
    const GscEntityControlHost *controls,const GscEntityCollectionHost *collections,
    const GscEntityPredicateHost *predicates,const GscActorSpawnHost *actor_spawners,
    char *err,size_t cap) {
    GscSession *s=gsc_session_create_with_scalar_switch(host,entities,access,controls,
                                                      collections,predicates,actor_spawners,err,cap);
    if(s)s->lifetime_enabled=1;
    return s;
}
GscSession *gsc_session_create_with_names(const GscSessionHost *host,
    const GscEntityHost *entities,const GscEntityAccessHost *access,
    const GscEntityControlHost *controls,const GscEntityCollectionHost *collections,
    const GscEntityPredicateHost *predicates,const GscActorSpawnHost *actor_spawners,
    char *err,size_t cap) {
    GscSession *s=gsc_session_create_with_lifetime(host,entities,access,controls,
                                                      collections,predicates,actor_spawners,err,cap);
    if(s)s->names_enabled=1;
    return s;
}
GscSession *gsc_session_create_with_music(const GscSessionHost *host,
    const GscEntityHost *entities,const GscEntityAccessHost *access,
    const GscEntityControlHost *controls,const GscEntityCollectionHost *collections,
    const GscEntityPredicateHost *predicates,const GscActorSpawnHost *actor_spawners,
    const GscSessionMusicHost *music,char *err,size_t cap) {
    GscSession *s=gsc_session_create_with_names(host,entities,access,controls,
                                                      collections,predicates,actor_spawners,err,cap);
    if(s){s->music_enabled=1;if(music)s->music=*music;}
    return s;
}
GscSession *gsc_session_create_with_resources(const GscSessionHost *host,
    const GscEntityHost *entities,const GscEntityAccessHost *access,
    const GscEntityControlHost *controls,const GscEntityCollectionHost *collections,
    const GscEntityPredicateHost *predicates,const GscActorSpawnHost *spawners,
    const GscSessionMusicHost *music,const GscSessionResourceHost *resources,char *err,size_t cap) {
    GscSession *s=gsc_session_create_with_music(host,entities,access,controls,
        collections,predicates,spawners,music,err,cap);
    if(s){s->resources_enabled=1;if(resources)s->resources=*resources;}
    return s;
}
GscSession *gsc_session_create_with_objectives(const GscSessionHost *host,
    const GscEntityHost *entities,const GscEntityAccessHost *access,
    const GscEntityControlHost *controls,const GscEntityCollectionHost *collections,
    const GscEntityPredicateHost *predicates,const GscActorSpawnHost *spawners,
    const GscSessionMusicHost *music,const GscSessionResourceHost *resources,
    const GscSessionObjectiveHost *objectives,char *err,size_t cap) {
    GscSession *s=gsc_session_create_with_resources(host,entities,access,controls,
        collections,predicates,spawners,music,resources,err,cap);
    if(s){s->objectives_enabled=1;if(objectives)s->objectives=*objectives;}
    return s;
}
GscSession *gsc_session_create_with_console(const GscSessionHost *host,
    const GscEntityHost *entities,const GscEntityAccessHost *access,
    const GscEntityControlHost *controls,const GscEntityCollectionHost *collections,
    const GscEntityPredicateHost *predicates,const GscActorSpawnHost *spawners,
    const GscSessionMusicHost *music,const GscSessionResourceHost *resources,
    const GscSessionObjectiveHost *objectives,const GscSessionConsoleHost *console,char *err,size_t cap) {
    GscSession *s=gsc_session_create_with_objectives(host,entities,access,controls,
        collections,predicates,spawners,music,resources,objectives,err,cap);
    if(s){s->console_enabled=1;if(console)s->console=*console;}
    return s;
}
GscSession *gsc_session_create_with_hud(const GscSessionHost *host,
    const GscEntityHost *entities,const GscEntityAccessHost *access,
    const GscEntityControlHost *controls,const GscEntityCollectionHost *collections,
    const GscEntityPredicateHost *predicates,const GscActorSpawnHost *spawners,
    const GscSessionMusicHost *music,const GscSessionResourceHost *resources,
    const GscSessionObjectiveHost *objectives,const GscSessionConsoleHost *console,const GscSessionHudHost *hud,char *err,size_t cap) {
    GscSession *s=gsc_session_create_with_console(host,entities,access,controls,
        collections,predicates,spawners,music,resources,objectives,console,err,cap);
    if(s){s->hud_enabled=1;if(hud)s->hud=*hud;}
    return s;
}
GscSession *gsc_session_create_with_mission(const GscSessionHost *host,
    const GscEntityHost *entities,const GscEntityAccessHost *access,
    const GscEntityControlHost *controls,const GscEntityCollectionHost *collections,
    const GscEntityPredicateHost *predicates,const GscActorSpawnHost *spawners,
    const GscSessionMusicHost *music,const GscSessionResourceHost *resources,
    const GscSessionObjectiveHost *objectives,const GscSessionConsoleHost *console,
    const GscSessionHudHost *hud,const GscSessionMissionHost *mission,char *err,size_t cap) {
    GscSession *s=gsc_session_create_with_hud(host,entities,access,controls,
        collections,predicates,spawners,music,resources,objectives,console,hud,err,cap);
    if(s){s->mission_enabled=1;if(mission)s->mission=*mission;}
    return s;
}
GscSession *gsc_session_create_with_blast(const GscSessionHost *host,
    const GscEntityHost *entities,const GscEntityAccessHost *access,
    const GscEntityControlHost *controls,const GscEntityCollectionHost *collections,
    const GscEntityPredicateHost *predicates,const GscActorSpawnHost *spawners,
    const GscSessionMusicHost *music,const GscSessionResourceHost *resources,
    const GscSessionObjectiveHost *objectives,const GscSessionConsoleHost *console,
    const GscSessionHudHost *hud,const GscSessionMissionHost *mission,const GscSessionBlastHost *blast,char *err,size_t cap) {
    GscSession *s=gsc_session_create_with_mission(host,entities,access,controls,
        collections,predicates,spawners,music,resources,objectives,console,hud,mission,err,cap);
    if(s){s->blast_enabled=1;if(blast)s->blast=*blast;}
    return s;
}

GscSession *gsc_session_create_with_quake(const GscSessionHost *host,
    const GscEntityHost *entities,const GscEntityAccessHost *access,
    const GscEntityControlHost *controls,const GscEntityCollectionHost *collections,
    const GscEntityPredicateHost *predicates,const GscActorSpawnHost *spawners,
    const GscSessionMusicHost *music,const GscSessionResourceHost *resources,
    const GscSessionObjectiveHost *objectives,const GscSessionConsoleHost *console,
    const GscSessionHudHost *hud,const GscSessionMissionHost *mission,const GscSessionBlastHost *blast,const GscSessionQuakeHost *quake,char *err,size_t cap) {
    GscSession *s=gsc_session_create_with_blast(host,entities,access,controls,
        collections,predicates,spawners,music,resources,objectives,console,hud,mission,blast,err,cap);
    if(s){s->quake_enabled=1;if(quake)s->quake=*quake;}
    return s;
}
GscSession *gsc_session_create_with_fx(const GscSessionHost *host,
    const GscEntityHost *entities,const GscEntityAccessHost *access,
    const GscEntityControlHost *controls,const GscEntityCollectionHost *collections,
    const GscEntityPredicateHost *predicates,const GscActorSpawnHost *spawners,
    const GscSessionMusicHost *music,const GscSessionResourceHost *resources,
    const GscSessionObjectiveHost *objectives,const GscSessionConsoleHost *console,
    const GscSessionHudHost *hud,const GscSessionMissionHost *mission,const GscSessionBlastHost *blast,const GscSessionQuakeHost *quake,const GscSessionFxHost *fx,char *err,size_t cap) {
    GscSession *s=gsc_session_create_with_quake(host,entities,access,controls,
        collections,predicates,spawners,music,resources,objectives,console,hud,mission,blast,quake,err,cap);
    if(s){s->fx_enabled=1;if(fx)s->fx=*fx;}
    return s;
}
GscSession *gsc_session_create_with_cvars(const GscSessionHost *host,
    const GscEntityHost *entities,const GscEntityAccessHost *access,
    const GscEntityControlHost *controls,const GscEntityCollectionHost *collections,
    const GscEntityPredicateHost *predicates,const GscActorSpawnHost *spawners,
    const GscSessionMusicHost *music,const GscSessionResourceHost *resources,
    const GscSessionObjectiveHost *objectives,const GscSessionConsoleHost *console,
    const GscSessionHudHost *hud,const GscSessionMissionHost *mission,const GscSessionBlastHost *blast,const GscSessionQuakeHost *quake,const GscSessionFxHost *fx,const GscSessionCvarHost *cvars,char *err,size_t cap) {
    GscSession *s=gsc_session_create_with_fx(host,entities,access,controls,
        collections,predicates,spawners,music,resources,objectives,console,hud,mission,blast,quake,fx,err,cap);
    if(s){s->cvars_enabled=1;if(cvars)s->cvars=*cvars;}
    return s;
}
GscSession *gsc_session_create_with_shellshock(const GscSessionHost *host,
    const GscEntityHost *entities,const GscEntityAccessHost *access,
    const GscEntityControlHost *controls,const GscEntityCollectionHost *collections,
    const GscEntityPredicateHost *predicates,const GscActorSpawnHost *spawners,
    const GscSessionMusicHost *music,const GscSessionResourceHost *resources,
    const GscSessionObjectiveHost *objectives,const GscSessionConsoleHost *console,
    const GscSessionHudHost *hud,const GscSessionMissionHost *mission,const GscSessionBlastHost *blast,const GscSessionQuakeHost *quake,const GscSessionFxHost *fx,const GscSessionCvarHost *cvars,const GscSessionShellshockHost *shellshock,char *err,size_t cap) {
    GscSession *s=gsc_session_create_with_cvars(host,entities,access,controls,
        collections,predicates,spawners,music,resources,objectives,console,hud,mission,blast,quake,fx,cvars,err,cap);
    if(s){s->shellshock_enabled=1;if(shellshock)s->shellshock=*shellshock;}
    return s;
}
GscSession *gsc_session_create_with_debug_text(const GscSessionHost *host,
    const GscEntityHost *entities,const GscEntityAccessHost *access,
    const GscEntityControlHost *controls,const GscEntityCollectionHost *collections,
    const GscEntityPredicateHost *predicates,const GscActorSpawnHost *spawners,
    const GscSessionMusicHost *music,const GscSessionResourceHost *resources,
    const GscSessionObjectiveHost *objectives,const GscSessionConsoleHost *console,
    const GscSessionHudHost *hud,const GscSessionMissionHost *mission,const GscSessionBlastHost *blast,const GscSessionQuakeHost *quake,const GscSessionFxHost *fx,const GscSessionCvarHost *cvars,const GscSessionShellshockHost *shellshock,const GscSessionDebugTextHost *debug_text,char *err,size_t cap) {
    GscSession *s=gsc_session_create_with_shellshock(host,entities,access,controls,
        collections,predicates,spawners,music,resources,objectives,console,hud,mission,blast,quake,fx,cvars,shellshock,err,cap);
    if(s){s->debug_text_enabled=1;if(debug_text)s->debug_text=*debug_text;}
    return s;
}
GscSession *gsc_session_create_with_debug_line(const GscSessionHost *host,
    const GscEntityHost *entities,const GscEntityAccessHost *access,
    const GscEntityControlHost *controls,const GscEntityCollectionHost *collections,
    const GscEntityPredicateHost *predicates,const GscActorSpawnHost *spawners,
    const GscSessionMusicHost *music,const GscSessionResourceHost *resources,
    const GscSessionObjectiveHost *objectives,const GscSessionConsoleHost *console,
    const GscSessionHudHost *hud,const GscSessionMissionHost *mission,const GscSessionBlastHost *blast,const GscSessionQuakeHost *quake,const GscSessionFxHost *fx,const GscSessionCvarHost *cvars,const GscSessionShellshockHost *shellshock,const GscSessionDebugTextHost *debug_text,const GscSessionDebugLineHost *debug_line,char *err,size_t cap) {
    GscSession *s=gsc_session_create_with_debug_text(host,entities,access,controls,
        collections,predicates,spawners,music,resources,objectives,console,hud,mission,blast,quake,fx,cvars,shellshock,debug_text,err,cap);
    if(s){s->debug_line_enabled=1;if(debug_line)s->debug_line=*debug_line;}
    return s;
}
GscSession *gsc_session_create_with_ambient(const GscSessionHost *host,
    const GscEntityHost *entities,const GscEntityAccessHost *access,
    const GscEntityControlHost *controls,const GscEntityCollectionHost *collections,
    const GscEntityPredicateHost *predicates,const GscActorSpawnHost *spawners,
    const GscSessionMusicHost *music,const GscSessionResourceHost *resources,
    const GscSessionObjectiveHost *objectives,const GscSessionConsoleHost *console,
    const GscSessionHudHost *hud,const GscSessionMissionHost *mission,const GscSessionBlastHost *blast,const GscSessionQuakeHost *quake,const GscSessionFxHost *fx,const GscSessionCvarHost *cvars,const GscSessionShellshockHost *shellshock,const GscSessionDebugTextHost *debug_text,const GscSessionDebugLineHost *debug_line,const GscSessionAmbientHost *ambient,char *err,size_t cap) {
    GscSession *s=gsc_session_create_with_debug_line(host,entities,access,controls,
        collections,predicates,spawners,music,resources,objectives,console,hud,mission,blast,quake,fx,cvars,shellshock,debug_text,debug_line,err,cap);
    if(s){s->ambient_enabled=1;if(ambient)s->ambient=*ambient;}
    return s;
}
GscSession *gsc_session_create_with_precache_item(const GscSessionHost *host,
    const GscEntityHost *entities,const GscEntityAccessHost *access,
    const GscEntityControlHost *controls,const GscEntityCollectionHost *collections,
    const GscEntityPredicateHost *predicates,const GscActorSpawnHost *spawners,
    const GscSessionMusicHost *music,const GscSessionResourceHost *resources,
    const GscSessionObjectiveHost *objectives,const GscSessionConsoleHost *console,
    const GscSessionHudHost *hud,const GscSessionMissionHost *mission,const GscSessionBlastHost *blast,const GscSessionQuakeHost *quake,const GscSessionFxHost *fx,const GscSessionCvarHost *cvars,const GscSessionShellshockHost *shellshock,const GscSessionDebugTextHost *debug_text,const GscSessionDebugLineHost *debug_line,const GscSessionAmbientHost *ambient,const GscSessionPrecacheItemHost *precache_item,char *err,size_t cap) {
    GscSession *s=gsc_session_create_with_ambient(host,entities,access,controls,
        collections,predicates,spawners,music,resources,objectives,console,hud,mission,blast,quake,fx,cvars,shellshock,debug_text,debug_line,ambient,err,cap);
    if(s){s->precache_item_enabled=1;if(precache_item)s->precache_item=*precache_item;}
    return s;
}
GscSession *gsc_session_create_with_save_game(const GscSessionHost *host,
    const GscEntityHost *entities,const GscEntityAccessHost *access,
    const GscEntityControlHost *controls,const GscEntityCollectionHost *collections,
    const GscEntityPredicateHost *predicates,const GscActorSpawnHost *spawners,
    const GscSessionMusicHost *music,const GscSessionResourceHost *resources,
    const GscSessionObjectiveHost *objectives,const GscSessionConsoleHost *console,
    const GscSessionHudHost *hud,const GscSessionMissionHost *mission,const GscSessionBlastHost *blast,const GscSessionQuakeHost *quake,const GscSessionFxHost *fx,const GscSessionCvarHost *cvars,const GscSessionShellshockHost *shellshock,const GscSessionDebugTextHost *debug_text,const GscSessionDebugLineHost *debug_line,const GscSessionAmbientHost *ambient,const GscSessionPrecacheItemHost *precache_item,const GscSessionSaveGameHost *save_game,char *err,size_t cap) {
    GscSession *s=gsc_session_create_with_precache_item(host,entities,access,controls,
        collections,predicates,spawners,music,resources,objectives,console,hud,mission,blast,quake,fx,cvars,shellshock,debug_text,debug_line,ambient,precache_item,err,cap);
    if(s){s->save_game_enabled=1;if(save_game)s->save_game=*save_game;}
    return s;
}
GscSession *gsc_session_create_with_tag_fx(const GscSessionHost *host,
    const GscEntityHost *entities,const GscEntityAccessHost *access,
    const GscEntityControlHost *controls,const GscEntityCollectionHost *collections,
    const GscEntityPredicateHost *predicates,const GscActorSpawnHost *spawners,
    const GscSessionMusicHost *music,const GscSessionResourceHost *resources,
    const GscSessionObjectiveHost *objectives,const GscSessionConsoleHost *console,
    const GscSessionHudHost *hud,const GscSessionMissionHost *mission,const GscSessionBlastHost *blast,const GscSessionQuakeHost *quake,const GscSessionFxHost *fx,const GscSessionCvarHost *cvars,const GscSessionShellshockHost *shellshock,const GscSessionDebugTextHost *debug_text,const GscSessionDebugLineHost *debug_line,const GscSessionAmbientHost *ambient,const GscSessionPrecacheItemHost *precache_item,const GscSessionSaveGameHost *save_game,const GscSessionTagFxHost *tag_fx,char *err,size_t cap) {
    GscSession *s=gsc_session_create_with_save_game(host,entities,access,controls,
        collections,predicates,spawners,music,resources,objectives,console,hud,mission,blast,quake,fx,cvars,shellshock,debug_text,debug_line,ambient,precache_item,save_game,err,cap);
    if(s){s->tag_fx_enabled=1;if(tag_fx)s->tag_fx=*tag_fx;}
    return s;
}
GscSession *gsc_session_create_with_native_queries(const GscSessionHost *host,
    const GscEntityHost *entities,const GscEntityAccessHost *access,
    const GscEntityControlHost *controls,const GscEntityCollectionHost *collections,
    const GscEntityPredicateHost *predicates,const GscActorSpawnHost *spawners,
    const GscSessionMusicHost *music,const GscSessionResourceHost *resources,
    const GscSessionObjectiveHost *objectives,const GscSessionConsoleHost *console,
    const GscSessionHudHost *hud,const GscSessionMissionHost *mission,const GscSessionBlastHost *blast,const GscSessionQuakeHost *quake,const GscSessionFxHost *fx,const GscSessionCvarHost *cvars,const GscSessionShellshockHost *shellshock,const GscSessionDebugTextHost *debug_text,const GscSessionDebugLineHost *debug_line,const GscSessionAmbientHost *ambient,const GscSessionPrecacheItemHost *precache_item,const GscSessionSaveGameHost *save_game,const GscSessionTagFxHost *tag_fx,const GscSessionNativeQueryHost *native_queries,char *err,size_t cap) {
    GscSession *s=gsc_session_create_with_tag_fx(host,entities,access,controls,
        collections,predicates,spawners,music,resources,objectives,console,hud,mission,blast,quake,fx,cvars,shellshock,debug_text,debug_line,ambient,precache_item,save_game,tag_fx,err,cap);
    if(s){s->native_queries_enabled=1;if(native_queries)s->native_queries=*native_queries;}
    return s;
}
GscSession *gsc_session_create_with_precache_string(const GscSessionHost *host,
    const GscEntityHost *entities,const GscEntityAccessHost *access,
    const GscEntityControlHost *controls,const GscEntityCollectionHost *collections,
    const GscEntityPredicateHost *predicates,const GscActorSpawnHost *spawners,
    const GscSessionMusicHost *music,const GscSessionResourceHost *resources,
    const GscSessionObjectiveHost *objectives,const GscSessionConsoleHost *console,
    const GscSessionHudHost *hud,const GscSessionMissionHost *mission,const GscSessionBlastHost *blast,const GscSessionQuakeHost *quake,const GscSessionFxHost *fx,const GscSessionCvarHost *cvars,const GscSessionShellshockHost *shellshock,const GscSessionDebugTextHost *debug_text,const GscSessionDebugLineHost *debug_line,const GscSessionAmbientHost *ambient,const GscSessionPrecacheItemHost *precache_item,const GscSessionSaveGameHost *save_game,const GscSessionTagFxHost *tag_fx,const GscSessionNativeQueryHost *native_queries,const GscSessionPrecacheStringHost *precache_string,char *err,size_t cap) {
    GscSession *s=gsc_session_create_with_native_queries(host,entities,access,controls,collections,predicates,spawners,music,resources,objectives,console,hud,mission,blast,quake,fx,cvars,shellshock,debug_text,debug_line,ambient,precache_item,save_game,tag_fx,native_queries,err,cap);
    if(s){s->precache_string_enabled=1;if(precache_string)s->precache_string=*precache_string;}
    return s;
}
GscSession *gsc_session_create_with_spawner_arrays(const GscSessionHost *host,
    const GscEntityHost *entities,const GscEntityAccessHost *access,
    const GscEntityControlHost *controls,const GscEntityCollectionHost *collections,
    const GscEntityPredicateHost *predicates,const GscActorSpawnHost *spawners,
    const GscSessionMusicHost *music,const GscSessionResourceHost *resources,
    const GscSessionObjectiveHost *objectives,const GscSessionConsoleHost *console,
    const GscSessionHudHost *hud,const GscSessionMissionHost *mission,const GscSessionBlastHost *blast,const GscSessionQuakeHost *quake,const GscSessionFxHost *fx,const GscSessionCvarHost *cvars,const GscSessionShellshockHost *shellshock,const GscSessionDebugTextHost *debug_text,const GscSessionDebugLineHost *debug_line,const GscSessionAmbientHost *ambient,const GscSessionPrecacheItemHost *precache_item,const GscSessionSaveGameHost *save_game,const GscSessionTagFxHost *tag_fx,const GscSessionNativeQueryHost *native_queries,const GscSessionPrecacheStringHost *precache_string,const GscSessionSpawnerArrayHost *spawner_arrays,char *err,size_t cap)  {
    GscSession *s=gsc_session_create_with_precache_string(host,entities,access,controls,collections,predicates,spawners,music,resources,objectives,console,hud,mission,blast,quake,fx,cvars,shellshock,debug_text,debug_line,ambient,precache_item,save_game,tag_fx,native_queries,precache_string,err,cap);
    if(s){s->spawner_arrays_enabled=1;if(spawner_arrays)s->spawner_arrays=*spawner_arrays;}
    return s;
}
GscSession *gsc_session_create_with_randomint(const GscSessionHost *host,
    const GscEntityHost *entities,const GscEntityAccessHost *access,
    const GscEntityControlHost *controls,const GscEntityCollectionHost *collections,
    const GscEntityPredicateHost *predicates,const GscActorSpawnHost *spawners,
    const GscSessionMusicHost *music,const GscSessionResourceHost *resources,
    const GscSessionObjectiveHost *objectives,const GscSessionConsoleHost *console,
    const GscSessionHudHost *hud,const GscSessionMissionHost *mission,const GscSessionBlastHost *blast,const GscSessionQuakeHost *quake,const GscSessionFxHost *fx,const GscSessionCvarHost *cvars,const GscSessionShellshockHost *shellshock,const GscSessionDebugTextHost *debug_text,const GscSessionDebugLineHost *debug_line,const GscSessionAmbientHost *ambient,const GscSessionPrecacheItemHost *precache_item,const GscSessionSaveGameHost *save_game,const GscSessionTagFxHost *tag_fx,const GscSessionNativeQueryHost *native_queries,const GscSessionPrecacheStringHost *precache_string,const GscSessionSpawnerArrayHost *spawner_arrays,const GscSessionRandomIntHost *randomint,char *err,size_t cap)  {
    GscSession *s=gsc_session_create_with_spawner_arrays(host,entities,access,controls,collections,predicates,spawners,music,resources,objectives,console,hud,mission,blast,quake,fx,cvars,shellshock,debug_text,debug_line,ambient,precache_item,save_game,tag_fx,native_queries,precache_string,spawner_arrays,err,cap);
    if(s){s->randomint_enabled=1;if(randomint)s->randomint=*randomint;}
    return s;
}
GscSession *gsc_session_create_with_randomfloat(const GscSessionHost *host,
    const GscEntityHost *entities,const GscEntityAccessHost *access,
    const GscEntityControlHost *controls,const GscEntityCollectionHost *collections,
    const GscEntityPredicateHost *predicates,const GscActorSpawnHost *spawners,
    const GscSessionMusicHost *music,const GscSessionResourceHost *resources,
    const GscSessionObjectiveHost *objectives,const GscSessionConsoleHost *console,
    const GscSessionHudHost *hud,const GscSessionMissionHost *mission,const GscSessionBlastHost *blast,const GscSessionQuakeHost *quake,const GscSessionFxHost *fx,const GscSessionCvarHost *cvars,const GscSessionShellshockHost *shellshock,const GscSessionDebugTextHost *debug_text,const GscSessionDebugLineHost *debug_line,const GscSessionAmbientHost *ambient,const GscSessionPrecacheItemHost *precache_item,const GscSessionSaveGameHost *save_game,const GscSessionTagFxHost *tag_fx,const GscSessionNativeQueryHost *native_queries,const GscSessionPrecacheStringHost *precache_string,const GscSessionSpawnerArrayHost *spawner_arrays,const GscSessionRandomIntHost *randomint,const GscSessionRandomFloatHost *randomfloat,char *err,size_t cap)  {
    GscSession *s=gsc_session_create_with_randomint(host,entities,access,controls,collections,predicates,spawners,music,resources,objectives,console,hud,mission,blast,quake,fx,cvars,shellshock,debug_text,debug_line,ambient,precache_item,save_game,tag_fx,native_queries,precache_string,spawner_arrays,randomint,err,cap);
    if(s){s->randomfloat_enabled=1;if(randomfloat)s->randomfloat=*randomfloat;}
    return s;
}
GscSession *gsc_session_create_with_keybinding(const GscSessionHost *host,
    const GscEntityHost *entities,const GscEntityAccessHost *access,
    const GscEntityControlHost *controls,const GscEntityCollectionHost *collections,
    const GscEntityPredicateHost *predicates,const GscActorSpawnHost *spawners,
    const GscSessionMusicHost *music,const GscSessionResourceHost *resources,
    const GscSessionObjectiveHost *objectives,const GscSessionConsoleHost *console,
    const GscSessionHudHost *hud,const GscSessionMissionHost *mission,const GscSessionBlastHost *blast,const GscSessionQuakeHost *quake,const GscSessionFxHost *fx,const GscSessionCvarHost *cvars,const GscSessionShellshockHost *shellshock,const GscSessionDebugTextHost *debug_text,const GscSessionDebugLineHost *debug_line,const GscSessionAmbientHost *ambient,const GscSessionPrecacheItemHost *precache_item,const GscSessionSaveGameHost *save_game,const GscSessionTagFxHost *tag_fx,const GscSessionNativeQueryHost *native_queries,const GscSessionPrecacheStringHost *precache_string,const GscSessionSpawnerArrayHost *spawner_arrays,const GscSessionRandomIntHost *randomint,const GscSessionRandomFloatHost *randomfloat,const GscSessionKeyBindingHost *keybinding,char *err,size_t cap)  {
    GscSession *s=gsc_session_create_with_randomfloat(host,entities,access,controls,collections,predicates,spawners,music,resources,objectives,console,hud,mission,blast,quake,fx,cvars,shellshock,debug_text,debug_line,ambient,precache_item,save_game,tag_fx,native_queries,precache_string,spawner_arrays,randomint,randomfloat,err,cap);
    if(s){s->keybinding_enabled=1;if(keybinding)s->keybinding=*keybinding;}
    return s;
}
static int session_keybinding_query(void *user,int now,GscKeyBindingRequest *query,char *error,size_t size) {
    GscSession *s=user;GscKeyBindingRequest before,copy;uint64_t id;int accepted;
    if(!keybinding_no_alias(query,error,size))return 0;
    if(!s||!query||!s->busy||s->abort_requested||s->view.error[0]||!s->keybinding_enabled||
       s->view.state!=GSC_SESSION_RUNNING||!s->view.session_id||s->view.logical_ms!=now||now<0||!s->keybinding.query)
        return session_reject(error,size,"getkeybinding outside current running session");
    id=s->view.session_id;before=*query;copy=before;
    if(!keybinding_request_valid(&before))return session_reject(error,size,"invalid getkeybinding query request");
    accepted=s->keybinding.query(s->keybinding.user,id,now,&copy,error,size);
    if(s->abort_requested||s->view.error[0]||s->view.state!=GSC_SESSION_RUNNING||
       s->view.session_id!=id||s->view.logical_ms!=now||!s->keybinding_enabled)
        return session_reject(error,size,"getkeybinding callback cancelled or changed session");
    if(accepted!=1)return 0;
    if(!keybinding_reply_valid(&before,&copy))return session_reject(error,size,"getkeybinding callback changed copied intent or reply");
    *query=copy;return 1;
}


static int session_randomint_draw(void *user,int now,GscRandomIntRequest *query,
    char *error,size_t size) {
    GscSession *s=user;GscRandomIntRequest before,copy;uint64_t id;int accepted,computed,dynamic,numeric,dynamic_float;
    if(!s || !query || !s->busy || s->abort_requested || s->view.error[0] ||
       !s->randomint_enabled || s->view.state!=GSC_SESSION_RUNNING ||
       !s->view.session_id || s->view.logical_ms!=now || now<0 || !s->randomint.draw)
        return session_reject(error,size,"randomint outside current running session");
    before=*query;copy=before;id=s->view.session_id;computed=s->computed_randomint_enabled;dynamic=s->dynamic_randomint_enabled;numeric=s->numeric_randomint_enabled;dynamic_float=s->dynamic_randomfloat_enabled;
    if(!randomint_request_valid(&before))return session_reject(error,size,"invalid randomint draw request");
    accepted=s->randomint.draw(s->randomint.user,id,now,&copy,error,size);
    if(s->abort_requested || s->view.error[0] || s->view.state!=GSC_SESSION_RUNNING ||
       s->view.session_id!=id || s->view.logical_ms!=now || !s->randomint_enabled||
       s->computed_randomint_enabled!=computed||s->dynamic_randomint_enabled!=dynamic||s->numeric_randomint_enabled!=numeric||s->dynamic_randomfloat_enabled!=dynamic_float)
        return session_reject(error,size,"randomint callback cancelled or changed session");
    if(accepted!=1)return 0;
    if(!randomint_reply_valid(&before,&copy))return session_reject(error,size,"randomint callback changed intent or returned invalid draw");
    *query=copy;return 1;
}
static int session_randomfloat_draw(void *user,int now,GscRandomFloatRequest *query,
    char *error,size_t size) {
    GscSession *s=user;GscRandomFloatRequest before,copy;uint64_t id;int accepted,dynamic;
    if(!s || !query || !s->busy || s->abort_requested || s->view.error[0] ||
       !s->randomfloat_enabled || s->view.state!=GSC_SESSION_RUNNING ||
       !s->view.session_id || s->view.logical_ms!=now || now<0 || !s->randomfloat.draw)
        return session_reject(error,size,"randomfloat outside current running session");
    before=*query;copy=before;id=s->view.session_id;dynamic=s->dynamic_randomfloat_enabled;
    if(!randomfloat_request_valid(&before))return session_reject(error,size,"invalid randomfloat draw request");
    accepted=s->randomfloat.draw(s->randomfloat.user,id,now,&copy,error,size);
    if(s->abort_requested || s->view.error[0] || s->view.state!=GSC_SESSION_RUNNING ||
       s->view.session_id!=id || s->view.logical_ms!=now || !s->randomfloat_enabled||s->dynamic_randomfloat_enabled!=dynamic)
        return session_reject(error,size,"randomfloat callback cancelled or changed session");
    if(accepted!=1)return 0;
    if(!randomfloat_reply_valid(&before,&copy))return session_reject(error,size,"randomfloat callback changed intent or returned invalid draw");
    *query=copy;return 1;
}

#include "gsc_session_entity_queries.inc"

static int session_spawner_array_collect(void *user,int now,GscEntityRef *out,
    size_t capacity,size_t *count,char *error,size_t size) {
    GscSession *s=user;GscEntityRef refs[GSC_ARRAY_CONTAINER_CAPACITY];
    size_t copied=(size_t)-1;uint64_t id;int accepted;
    if(!s||!s->busy||s->abort_requested||s->view.error[0]||!s->spawner_arrays_enabled||
       s->view.state!=GSC_SESSION_RUNNING||!s->view.session_id||s->view.logical_ms!=now||now<0||
       !s->spawner_arrays.collect_all||!out||!count||capacity>GSC_ARRAY_CONTAINER_CAPACITY)
        return session_reject(error,size,"getspawnerarray outside current running session");
    id=s->view.session_id;memset(refs,0,sizeof(refs));
    accepted=s->spawner_arrays.collect_all(s->spawner_arrays.user,id,now,refs,capacity,&copied,error,size);
    if(s->abort_requested||s->view.error[0]||s->view.state!=GSC_SESSION_RUNNING||
       s->view.session_id!=id||s->view.logical_ms!=now||!s->spawner_arrays_enabled)
        return session_reject(error,size,"getspawnerarray callback cancelled or changed session");
    if(accepted!=1)return 0;
    if(copied>capacity)return session_reject(error,size,"getspawnerarray callback exceeded complete output capacity");
    if(copied)memcpy(out,refs,copied*sizeof(*out));
    *count=copied;return 1;
}

int gsc_session_precache_string_startup(const GscSession *s,uint64_t id,int now) {
    return s && s->busy && !s->abort_requested && !s->view.error[0] &&
        s->precache_string_enabled && s->view.state==GSC_SESSION_RUNNING && id &&
        s->view.session_id==id && now>=0 && s->view.logical_ms==now &&
        gsc_runtime_precache_string_startup(s->runtime,now);
}
static int session_precache_string_startup(void *user,int now) {
    GscSession *s=user;uint64_t id;int accepted;
    if(!s)return 0;
    id=s->view.session_id;
    if(!gsc_session_precache_string_startup(s,id,now)||!s->precache_string.startup)return 0;
    accepted=s->precache_string.startup(s->precache_string.user,id,now);
    return accepted==1&&gsc_session_precache_string_startup(s,id,now);
}
static int session_precache_string_publish(void *user,int now,const GscPrecacheStringEvent *event,char *error,size_t size) {
    GscSession *s=user;GscPrecacheStringEvent before,copy;uint64_t id;int accepted;
    if(!s||!event)return session_reject(error,size,"missing precacheString session request");
    id=s->view.session_id;
    if(!gsc_session_precache_string_startup(s,id,now)||!s->precache_string.publish)
        return session_reject(error,size,"precacheString outside current startup session");
    before=*event;copy=before;
    if(!precache_string_event_valid(&before))return session_reject(error,size,"invalid precacheString request");
    accepted=s->precache_string.publish(s->precache_string.user,id,now,&copy,error,size);
    if(!gsc_session_precache_string_startup(s,id,now))return session_reject(error,size,"precacheString callback cancelled or changed startup session");
    if(accepted!=1)return 0;
    if(memcmp(&before,&copy,sizeof(before)))return session_reject(error,size,"precacheString callback changed intent");
    return 1;
}

static int session_native_query(void *user,int now,GscNativeQuery *query,char *error,size_t size) {
    GscSession *s=user;GscNativeQuery before,copy;uint64_t id;int accepted,structs;
    if(!s||!query||!s->busy||s->abort_requested||!s->native_queries_enabled||
       s->view.state!=GSC_SESSION_RUNNING||s->view.logical_ms!=now||
       !s->view.session_id||!s->native_queries.query)
        return session_reject(error,size,"native query outside current running session");
    before=*query;copy=before;id=s->view.session_id;structs=s->structs_enabled;
    if(before.kind==GSC_QUERY_DIFFICULTY&&!structs)return session_reject(error,size,"difficulty query requires structure selector");
    if(!native_query_request_valid(&before))return session_reject(error,size,"invalid native query request");
    accepted=s->native_queries.query(s->native_queries.user,id,now,&copy,error,size);
    if(s->abort_requested||s->view.state!=GSC_SESSION_RUNNING||s->view.session_id!=id||s->view.logical_ms!=now||s->structs_enabled!=structs)
        return session_reject(error,size,"native query callback cancelled or changed session");
    if(accepted!=1)return 0;
    if(!native_query_reply_valid(&before,&copy))return session_reject(error,size,"native query callback changed intent or returned invalid value");
    *query=copy;return 1;
}
static int session_precache_item_publish(void *user,int logical_ms,const GscPrecacheItemEvent *event,char *error,size_t size) {
    GscSession *s=user;GscPrecacheItemEvent copy;int accepted;uint64_t id;
    if(!s||!s->busy||s->abort_requested||!s->precache_item_enabled||
       s->view.state!=GSC_SESSION_RUNNING||!s->view.session_id||logical_ms!=s->view.logical_ms||logical_ms<0||!event)
        return session_reject(error,size,"precacheItem publication outside running session");
    copy=*event;id=s->view.session_id;
    if(!copy.operation_id||!precache_item_event_name_valid(&copy))
        return session_reject(error,size,"invalid precacheItem payload");
    if(!s->precache_item.publish)return session_reject(error,size,"precacheItem publish host capability is unavailable");
    accepted=s->precache_item.publish(s->precache_item.user,id,logical_ms,&copy,error,size);
    if(s->abort_requested||s->view.error[0]||s->view.state!=GSC_SESSION_RUNNING||s->view.session_id!=id)
        return session_reject(error,size,"precacheItem callback cancelled session");
    if(accepted!=1){if(!error||!size||!error[0])session_message(error,size,"precacheItem host rejected publication");return 0;}
    return 1;
}
static int session_tag_fx_running(const GscSession *s,uint64_t id,int now) {
    return s && s->busy && !s->abort_requested && !s->view.error[0] &&
        s->tag_fx_enabled && s->view.state==GSC_SESSION_RUNNING && id &&
        s->view.session_id==id && now>=0 && s->view.logical_ms==now;
}
static int session_tag_fx_query(void *user,int now,GscTagFxQuery *query,char *error,size_t size) {
    GscSession *s=user;GscTagFxQuery before,copy;uint64_t id;int accepted;
    if(!s || !query || !session_tag_fx_running(s,s->view.session_id,now))
        return session_reject(error,size,"playfxOnTag query outside running session");
    before=*query;copy=before;id=s->view.session_id;
    if(!tag_fx_query_valid(&before) || !s->tag_fx.query)
        return session_reject(error,size,"playfxOnTag query payload or capability unavailable");
    accepted=s->tag_fx.query(s->tag_fx.user,id,now,&copy,error,size);
    if(!session_tag_fx_running(s,id,now))return session_reject(error,size,"playfxOnTag callback cancelled or changed session");
    if(accepted!=1){if(!error||!size||!error[0])session_message(error,size,"playfxOnTag host rejected query");return 0;}
    if(!tag_fx_reply_valid(&before,&copy))return session_reject(error,size,"invalid playfxOnTag query reply");
    *query=copy;return 1;
}
static int session_tag_fx_publish(void *user,int now,const GscTagFxEvent *event,char *error,size_t size) {
    GscSession *s=user;GscTagFxEvent before,copy;uint64_t id;int accepted;
    if(!s || !event || !session_tag_fx_running(s,s->view.session_id,now))
        return session_reject(error,size,"playfxOnTag publication outside running session");
    before=*event;copy=before;id=s->view.session_id;
    if(!tag_fx_event_valid(&before) || !s->tag_fx.publish)
        return session_reject(error,size,"playfxOnTag event payload or capability unavailable");
    accepted=s->tag_fx.publish(s->tag_fx.user,id,now,&copy,error,size);
    if(!session_tag_fx_running(s,id,now))return session_reject(error,size,"playfxOnTag publisher cancelled or changed session");
    if(accepted!=1){if(!error||!size||!error[0])session_message(error,size,"playfxOnTag host rejected publication");return 0;}
    if(!tag_fx_event_same(&before,&copy))return session_reject(error,size,"playfxOnTag publisher changed copied event");
    return 1;
}

static int session_save_game_query(void *user,int logical_ms,GscSaveGameQuery *query,char *error,size_t size) {
    GscSession *s=user;GscSaveGameQuery copy;int accepted,kind;uint64_t id,operation;
    if(!s||!s->busy||s->abort_requested||!s->save_game_enabled||
       s->view.state!=GSC_SESSION_RUNNING||!s->view.session_id||logical_ms!=s->view.logical_ms||logical_ms<0||!query)
        return session_reject(error,size,"saveGame query outside running session");
    copy=*query;id=s->view.session_id;operation=copy.operation_id;kind=copy.kind;
    if(!operation||operation==UINT64_MAX||kind<GSC_SAVE_GAME_PLAYER_HEALTH||kind>GSC_SAVE_GAME_CAMPAIGN)
        return session_reject(error,size,"invalid saveGame query");
    if(!s->save_game.query)return session_reject(error,size,"saveGame query host capability is unavailable");
    accepted=s->save_game.query(s->save_game.user,id,logical_ms,&copy,error,size);
    if(s->abort_requested||s->view.error[0]||s->view.state!=GSC_SESSION_RUNNING||s->view.session_id!=id)
        return session_reject(error,size,"saveGame query cancelled session");
    if(accepted!=1){if(!error||!size||!error[0])session_message(error,size,"saveGame host rejected query");return 0;}
    if(copy.kind!=kind||copy.operation_id!=operation||
       (kind!=GSC_SAVE_GAME_PLAYER_HEALTH&&!memchr(copy.text,0,sizeof(copy.text))))
        return session_reject(error,size,"invalid saveGame query reply");
    *query=copy;return 1;
}
static int session_save_game_publish(void *user,int logical_ms,const GscSaveGameEvent *event,char *error,size_t size) {
    GscSession *s=user;GscSaveGameEvent copy;int accepted;uint64_t id;
    if(!s||!s->busy||s->abort_requested||!s->save_game_enabled||
       s->view.state!=GSC_SESSION_RUNNING||!s->view.session_id||logical_ms!=s->view.logical_ms||logical_ms<0||!event)
        return session_reject(error,size,"saveGame publication outside running session");
    copy=*event;id=s->view.session_id;
    if(!copy.operation_id||!save_game_event_valid(&copy))
        return session_reject(error,size,"invalid saveGame payload");
    if(!s->save_game.publish)return session_reject(error,size,"saveGame publish host capability is unavailable");
    accepted=s->save_game.publish(s->save_game.user,id,logical_ms,&copy,error,size);
    if(s->abort_requested||s->view.error[0]||s->view.state!=GSC_SESSION_RUNNING||s->view.session_id!=id)
        return session_reject(error,size,"saveGame callback cancelled session");
    if(accepted!=1){if(!error||!size||!error[0])session_message(error,size,"saveGame host rejected publication");return 0;}
    return 1;
}

static int session_ambient_publish(void *user,int logical_ms,const GscAmbientEvent *event,char *error,size_t size) {
    GscSession *s=user;GscAmbientEvent copy;int accepted;uint64_t id;
    if(!s||!s->busy||s->abort_requested||!s->ambient_enabled||
       s->view.state!=GSC_SESSION_RUNNING||!s->view.session_id||logical_ms!=s->view.logical_ms||logical_ms<0||!event)
        return session_reject(error,size,"ambient publication outside running session");
    copy=*event;id=s->view.session_id;
    if(!copy.operation_id||copy.transition_ms<0||!ambient_event_alias_valid(&copy))
        return session_reject(error,size,"invalid ambient payload");
    if(!s->ambient.publish)return session_reject(error,size,"ambient publish host capability is unavailable");
    accepted=s->ambient.publish(s->ambient.user,id,logical_ms,&copy,error,size);
    if(s->abort_requested||s->view.error[0]||s->view.state!=GSC_SESSION_RUNNING||s->view.session_id!=id)
        return session_reject(error,size,"ambient callback cancelled session");
    if(accepted!=1){if(!error||!size||!error[0])session_message(error,size,"ambient host rejected publication");return 0;}
    return 1;
}

static int session_debug_line_publish(void *user,int logical_ms,const GscDebugLineEvent *event,char *error,size_t size) {
    GscSession *s=user;GscDebugLineEvent copy;int accepted,n;uint64_t id;
    if(!s||!s->busy||s->abort_requested||!s->debug_line_enabled||
       s->view.state!=GSC_SESSION_RUNNING||!s->view.session_id||logical_ms!=s->view.logical_ms||logical_ms<0||!event)
        return session_reject(error,size,"debug line publication outside running session");
    copy=*event;id=s->view.session_id;
    if(!copy.operation_id||!isfinite(copy.color[3]))return session_reject(error,size,"invalid debug line payload");
    for(n=0;n<3;n++)if(!isfinite(copy.start[n])||!isfinite(copy.end[n])||!isfinite(copy.color[n]))
        return session_reject(error,size,"invalid debug line vector");
    if(!s->debug_line.publish)return session_reject(error,size,"debug line publish host capability is unavailable");
    accepted=s->debug_line.publish(s->debug_line.user,id,logical_ms,&copy,error,size);
    if(s->abort_requested||s->view.error[0]||s->view.state!=GSC_SESSION_RUNNING||s->view.session_id!=id)
        return session_reject(error,size,"debug line callback cancelled session");
    if(accepted!=1){if(!error||!size||!error[0])session_message(error,size,"debug line host rejected publication");return 0;}
    return 1;
}

static int session_debug_text_publish(void *user,int logical_ms,const GscDebugTextEvent *event,char *error,size_t size) {
    GscSession *s=user;GscDebugTextEvent copy;int accepted;uint64_t id;size_t n;
    if(!s||!s->busy||s->abort_requested||!s->debug_text_enabled||
       s->view.state!=GSC_SESSION_RUNNING||!s->view.session_id||logical_ms!=s->view.logical_ms||logical_ms<0||!event)
        return session_reject(error,size,"debug text publication outside running session");
    copy=*event;id=s->view.session_id;
    if(!copy.operation_id||copy.length>=sizeof(copy.text)||copy.text[copy.length]||
       memchr(copy.text,0,copy.length)||!isfinite(copy.scale)||!isfinite(copy.color[3]))
        return session_reject(error,size,"invalid debug text payload");
    for(n=0;n<3;n++)if(!isfinite(copy.origin[n])||!isfinite(copy.color[n]))
        return session_reject(error,size,"invalid debug text vector");
    for(n=0;n<copy.length;n++) {
        unsigned char c=(unsigned char)copy.text[n];
        if(c<32||c>126)return session_reject(error,size,"debug text outside printable ASCII transport domain");
    }
    if(!s->debug_text.publish)return session_reject(error,size,"debug text publish host capability is unavailable");
    accepted=s->debug_text.publish(s->debug_text.user,id,logical_ms,&copy,error,size);
    if(s->abort_requested||s->view.error[0]||s->view.state!=GSC_SESSION_RUNNING||s->view.session_id!=id)
        return session_reject(error,size,"debug text callback cancelled session");
    if(accepted!=1){if(!error||!size||!error[0])session_message(error,size,"debug text host rejected publication");return 0;}
    return 1;
}

static int session_shellshock_publish(void *user,int logical_ms,const GscShellshockEvent *event,char *error,size_t size) {
    GscSession *s=user;GscShellshockEvent copy;int accepted;uint64_t id;size_t n;
    if(!s||!s->busy||s->abort_requested||!s->shellshock_enabled||!s->shellshock_initial_window||
       s->view.state!=GSC_SESSION_RUNNING||!s->view.session_id||logical_ms!=s->view.logical_ms||logical_ms<0||!event)
        return session_reject(error,size,"shellshock registration outside initial running session");
    copy=*event;id=s->view.session_id;
    if(!copy.operation_id||!memchr(copy.name,0,sizeof(copy.name))||!copy.name[0])
        return session_reject(error,size,"invalid shellshock registration payload");
    for(n=0;copy.name[n];n++) {
        unsigned char c=(unsigned char)copy.name[n];
        if(c<32||c>126||c=='"')return session_reject(error,size,"shellshock name outside configstring transport domain");
    }
    if(!s->shellshock.publish)return session_reject(error,size,"shellshock publish host capability is unavailable");
    accepted=s->shellshock.publish(s->shellshock.user,id,logical_ms,&copy,error,size);
    if(s->abort_requested||s->view.error[0]||s->view.state!=GSC_SESSION_RUNNING||s->view.session_id!=id||!s->shellshock_initial_window)
        return session_reject(error,size,"shellshock callback cancelled session");
    if(accepted!=1){if(!error||!size||!error[0])session_message(error,size,"shellshock host rejected registration");return 0;}
    return 1;
}
static int session_cvar_publish(void *user,int logical_ms,const GscCvarEvent *event,char *error,size_t size) {
    GscSession *s=user;GscCvarEvent copy;int accepted;uint64_t id;
    if(!s||!s->busy||s->abort_requested||!s->cvars_enabled||
       s->view.state!=GSC_SESSION_RUNNING||!s->view.session_id||logical_ms<0||!event)
        return session_reject(error,size,"cvar publication outside running session");
    copy=*event;id=s->view.session_id;
    if((copy.phase!=GSC_CVAR_REGISTER&&copy.phase!=GSC_CVAR_SET)||
       copy.native_flags!=GSC_CVAR_NATIVE_FLAGS||!copy.operation_id||!memchr(copy.name,0,sizeof(copy.name))||
       copy.length>=sizeof(copy.value)||copy.value[copy.length]||
       memchr(copy.value,0,copy.length))
        return session_reject(error,size,"invalid cvar publication payload");
    if(!s->cvars.publish)return session_reject(error,size,"cvar publish host capability is unavailable");
    accepted=s->cvars.publish(s->cvars.user,id,logical_ms,&copy,error,size);
    if(s->abort_requested||s->view.error[0]||s->view.state!=GSC_SESSION_RUNNING||s->view.session_id!=id)
        return session_reject(error,size,"cvar callback cancelled session");
    if(accepted!=1){if(!error||!size||!error[0])session_message(error,size,"cvar host rejected publication");return 0;}
    return 1;
}
static int session_fx_load(void *user,int logical_ms,const GscFxLoadEvent *event,int32_t *out,char *error,size_t size) {
    GscSession *s=user;GscFxLoadEvent copy;int accepted;int32_t index=0;uint64_t id;
    if(!s||!s->busy||s->abort_requested||!s->fx_enabled||
       s->view.state!=GSC_SESSION_RUNNING||!s->view.session_id||logical_ms<0||!event||!out)
        return session_reject(error,size,"FX load outside running session");
    copy=*event;id=s->view.session_id;
    if(!memchr(copy.name,0,sizeof(copy.name))||(copy.initial_window!=0&&copy.initial_window!=1))
        return session_reject(error,size,"invalid FX load payload");
    if(!s->fx.load)return session_reject(error,size,"FX load host capability is unavailable");
    accepted=s->fx.load(s->fx.user,id,logical_ms,&copy,&index,error,size);
    if(s->abort_requested||s->view.error[0]||s->view.state!=GSC_SESSION_RUNNING||s->view.session_id!=id)
        return session_reject(error,size,"FX load callback cancelled session");
    if(accepted!=1){if(!error||!size||!error[0])session_message(error,size,"FX load host rejected resource");return 0;}
    if(index<0||index>=GSC_FX_INDEX_CAPACITY)return session_reject(error,size,"FX load index outside bounded 0..63 domain");
    if(copy.name[0]?index==0:(index!=0||!copy.initial_window))
        return session_reject(error,size,"FX load index inconsistent with name and initial registration");
    *out=index;return 1;
}
static int session_fx_play(void *user,int logical_ms,const GscFxPlayEvent *event,char *error,size_t size) {
    GscSession *s=user;GscFxPlayEvent copy;int n,accepted;uint64_t id;
    if(!s||!s->busy||s->abort_requested||!s->fx_enabled||
       s->view.state!=GSC_SESSION_RUNNING||!s->view.session_id||logical_ms<0||!event)
        return session_reject(error,size,"FX play outside running session");
    copy=*event;id=s->view.session_id;
    if(copy.has_direction!=0&&copy.has_direction!=1)return session_reject(error,size,"invalid FX direction selector");
    for(n=0;n<3;n++)if(!isfinite(copy.origin[n])||(copy.has_direction&&!isfinite(copy.direction[n])))
        return session_reject(error,size,"invalid FX play vector");
    if(!s->fx.play)return session_reject(error,size,"FX play host capability is unavailable");
    accepted=s->fx.play(s->fx.user,id,logical_ms,&copy,error,size);
    if(s->abort_requested||s->view.error[0]||s->view.state!=GSC_SESSION_RUNNING||s->view.session_id!=id)
        return session_reject(error,size,"FX play callback cancelled session");
    if(accepted!=1){if(!error||!size||!error[0])session_message(error,size,"FX play host rejected effect");return 0;}
    return 1;
}
static int session_quake_emit(void *user,int logical_ms,const GscQuakeEvent *event,char *error,size_t size) {
    GscSession *s=user;GscQuakeEvent copy;int n,accepted;
    if(!s||!s->busy||s->abort_requested||!s->quake_enabled||
       s->view.state!=GSC_SESSION_RUNNING||!s->view.session_id||logical_ms<0||!event)
        return session_reject(error,size,"quake effect outside running session");
    copy=*event;
    if(!isfinite(copy.scale)||copy.duration_ms<0||!isfinite(copy.radius)||copy.radius<=0)
        return session_reject(error,size,"invalid quake event payload");
    for(n=0;n<3;n++)if(!isfinite(copy.origin[n]))return session_reject(error,size,"invalid quake origin");
    if(!s->quake.emit)return session_reject(error,size,"quake host capability is unavailable");
    accepted=s->quake.emit(s->quake.user,s->view.session_id,logical_ms,&copy,error,size);
    if(s->abort_requested||s->view.error[0])return session_reject(error,size,"quake callback cancelled session");
    if(accepted!=1){if(!error||!size||!error[0])session_message(error,size,"quake host rejected effect");return 0;}
    return 1;
}
static int session_blast_emit(void *user,int logical_ms,const GscBlastEvent *event,char *error,size_t size) {
    GscSession *s=user;GscBlastEvent copy;int n,accepted;
    if(!s||!s->busy||s->abort_requested||!s->blast_enabled||
       s->view.state!=GSC_SESSION_RUNNING||!s->view.session_id||logical_ms<0||!event)
        return session_reject(error,size,"blast effect outside running session");
    copy=*event;
    if(copy.kind!=GSC_BLAST_RADIUS||!isfinite(copy.radius)||!isfinite(copy.inner_damage)||!isfinite(copy.outer_damage))
        return session_reject(error,size,"invalid blast event payload");
    for(n=0;n<3;n++)if(!isfinite(copy.origin[n]))return session_reject(error,size,"invalid blast origin");
    if(!s->blast.radius)return session_reject(error,size,"blast host capability is unavailable");
    accepted=s->blast.radius(s->blast.user,s->view.session_id,logical_ms,&copy,error,size);
    if(s->abort_requested||s->view.error[0])return session_reject(error,size,"blast callback cancelled session");
    if(accepted!=1) {
        if(!error||!size||!error[0])session_message(error,size,"blast host rejected effect");
        return 0;
    }
    return 1;
}
static int session_mission_gate(void *user,int logical_ms,int kind,int *enabled,char *error,size_t size) {
    GscSession *s=user;int accepted,copy=-1;
    if(!s||!s->busy||s->abort_requested||!s->mission_enabled||
       s->view.state!=GSC_SESSION_RUNNING||!s->view.session_id||logical_ms<0||!enabled||
       (kind!=GSC_MISSION_SUCCESS&&kind!=GSC_MISSION_FAILED)||!s->mission.query_gate)
        return session_reject(error,size,"mission gate outside running session");
    accepted=s->mission.query_gate(s->mission.user,s->view.session_id,logical_ms,kind,&copy,error,size);
    if(s->abort_requested||s->view.error[0])return session_reject(error,size,"mission gate cancelled session");
    if(accepted!=1) {
        if(!error||!size||!error[0])session_message(error,size,"mission host rejected gate");
        return 0;
    }
    if(copy!=0&&copy!=1)return session_reject(error,size,"invalid mission gate result");
    *enabled=copy;return 1;
}
static int session_mission_emit(void *user,int logical_ms,const GscMissionEvent *event,char *error,size_t size) {
    GscSession *s=user;GscMissionEvent copy;GscSessionMissionCallback callback;int accepted,valid=0;
    if(!s||!s->busy||s->abort_requested||!s->mission_enabled||
       s->view.state!=GSC_SESSION_RUNNING||!s->view.session_id||logical_ms<0||!event)
        return session_reject(error,size,"mission effect outside running session");
    copy=*event;
    if(copy.length>=sizeof(copy.map)||copy.map[copy.length]||memchr(copy.map,0,copy.length)||
       (copy.has_arg1!=0&&copy.has_arg1!=1)||(!copy.has_arg1&&copy.arg1))
        return session_reject(error,size,"invalid mission event payload");
    if(copy.kind==GSC_MISSION_SUCCESS) {
        valid=copy.phase==GSC_MISSION_REQUEST||
            (copy.phase==GSC_MISSION_ARGUMENT&&copy.has_arg1&&!copy.length)||
            (copy.phase==GSC_MISSION_SUCCESS_MARK&&!copy.has_arg1&&!copy.length);
    } else if(copy.kind==GSC_MISSION_FAILED)
        valid=(copy.phase==GSC_MISSION_DEATH_FADE||copy.phase==GSC_MISSION_FAILURE_MARK)&&
            !copy.has_arg1&&!copy.length;
    if(!valid)return session_reject(error,size,"invalid mission event phase");
    callback=copy.kind==GSC_MISSION_SUCCESS?s->mission.success:s->mission.failed;
    if(!callback)return session_reject(error,size,"mission host capability is unavailable");
    accepted=callback(s->mission.user,s->view.session_id,logical_ms,&copy,error,size);
    if(s->abort_requested||s->view.error[0])return session_reject(error,size,"mission callback cancelled session");
    if(accepted!=1) {
        if(!error||!size||!error[0])session_message(error,size,"mission host rejected effect");
        return 0;
    }
    return 1;
}
static int session_hud_emit(void *user,int logical_ms,const GscHudEvent *event,char *error,size_t size) {
    GscSession *s=user;GscHudEvent copy;GscSessionHudCallback callback;int accepted;
    if(!s||!s->busy||s->abort_requested||!s->hud_enabled||
       s->view.state!=GSC_SESSION_RUNNING||!s->view.session_id||logical_ms<0||!event)
        return session_reject(error,size,"HUD effect outside running session");
    copy=*event;
    if(copy.audience!=GSC_HUD_GLOBAL||copy.receiver.epoch||copy.receiver.slot||copy.receiver.generation||
       copy.length>=sizeof(copy.text)||copy.text[copy.length]||
       memchr(copy.text,0,copy.length))
        return session_reject(error,size,"invalid global HUD event");
    callback=copy.kind==GSC_HUD_PRINT?s->hud.print:copy.kind==GSC_HUD_BOLD?s->hud.bold:NULL;
    if(!callback)return session_reject(error,size,"HUD host capability is unavailable");
    accepted=callback(s->hud.user,s->view.session_id,logical_ms,&copy,error,size);
    if(s->abort_requested||s->view.error[0])return session_reject(error,size,"HUD callback cancelled session");
    if(accepted!=1) {
        if(!error||!size||!error[0])session_message(error,size,"HUD host rejected effect");
        return 0;
    }
    return 1;
}

static int session_console_gate(void *user,int logical_ms,int kind,int phase,int *enabled,
    char *error,size_t size) {
    GscSession *s=user;int accepted,copy=-1;
    if(!s||!s->busy||s->abort_requested||!s->console_enabled||
       s->view.state!=GSC_SESSION_RUNNING||logical_ms<0||!enabled||
       (kind!=GSC_CONSOLE_PRINT&&kind!=GSC_CONSOLE_PRINTLN)||
       (phase!=GSC_CONSOLE_GATE_ENTRY&&phase!=GSC_CONSOLE_GATE_ARGUMENTS)||!s->console.query_gate)
        return session_reject(error,size,"console gate outside running session");
    accepted=s->console.query_gate(s->console.user,s->view.session_id,logical_ms,kind,phase,&copy,error,size);
    if(s->abort_requested||s->view.error[0])return session_reject(error,size,"console gate cancelled session");
    if(accepted!=1) {
        if(!error||!size||!error[0])session_message(error,size,"console host rejected gate");
        return 0;
    }
    if(copy!=0&&copy!=1)return session_reject(error,size,"invalid console gate result");
    *enabled=copy;return 1;
}
static int session_console_emit(void *user,int logical_ms,const GscConsoleEvent *event,
    char *error,size_t size) {
    GscSession *s=user;GscConsoleEvent copy;GscSessionConsoleCallback callback;int accepted;
    if(!s||!s->busy||s->abort_requested||!s->console_enabled||
       s->view.state!=GSC_SESSION_RUNNING||logical_ms<0||!event)
        return session_reject(error,size,"console effect outside running session");
    copy=*event;
    callback=copy.kind==GSC_CONSOLE_PRINT?s->console.print:
        copy.kind==GSC_CONSOLE_PRINTLN?s->console.println:NULL;
    if(!callback)return session_reject(error,size,"console host sink capability is unavailable");
    accepted=callback(s->console.user,s->view.session_id,logical_ms,&copy,error,size);
    if(s->abort_requested||s->view.error[0])return session_reject(error,size,"console callback cancelled session");
    if(accepted!=1) {
        if(!error||!size||!error[0])session_message(error,size,"console host rejected effect");
        return 0;
    }
    return 1;
}
static int session_objective_emit(void *user,int logical_ms,const GscObjectiveEvent *event,
    char *error,size_t size) {
    GscSession *s=user;GscObjectiveEvent copy;GscSessionObjectiveCallback callback=NULL;int ok;
    if(!s||!s->busy||s->abort_requested||!s->objectives_enabled||
       s->view.state!=GSC_SESSION_RUNNING||logical_ms<0||!event)
        return session_reject(error,size,"objective outside running session");
    copy=*event;
    switch(copy.kind) {
      case GSC_OBJECTIVE_ADD:callback=s->objectives.add;break;
      case GSC_OBJECTIVE_DELETE:callback=s->objectives.delete_record;break;
      case GSC_OBJECTIVE_STATE:callback=s->objectives.state;break;
      case GSC_OBJECTIVE_STRING:callback=s->objectives.string;break;
      case GSC_OBJECTIVE_STRING_NOMESSAGE:callback=s->objectives.string_nomessage;break;
      case GSC_OBJECTIVE_POSITION:callback=s->objectives.position;break;
      case GSC_OBJECTIVE_CURRENT:callback=s->objectives.current;break;
      case GSC_OBJECTIVE_RING:callback=s->objectives.ring;break;
    }
    if(!callback)return session_reject(error,size,"objective host capability is unavailable");
    ok=callback(s->objectives.user,s->view.session_id,logical_ms,&copy,error,size);
    if(s->abort_requested||s->view.error[0])return session_reject(error,size,"objective callback cancelled session");
    if(ok!=1) {
        if(!error||!size||!error[0])session_message(error,size,"objective host rejected effect");
        return 0;
    }
    return 1;
}
static int session_resource_emit(void *user,int logical_ms,const GscResourceEvent *event,
    char *error,size_t size) {
    GscSession *s=user;GscResourceEvent copy;GscSessionResourceCallback callback;int ok;
    if(!s||!s->busy||s->abort_requested||!s->resources_enabled||
       s->view.state!=GSC_SESSION_RUNNING||!event||logical_ms<0)
        return session_reject(error,size,"resource outside running session");
    copy=*event;callback=copy.kind==GSC_RESOURCE_SHADER?s->resources.shader:
        copy.kind==GSC_RESOURCE_MODEL?s->resources.model:NULL;
    if(!callback)return session_reject(error,size,"resource host capability is unavailable");
    ok=callback(s->resources.user,s->view.session_id,logical_ms,&copy,error,size);
    if(s->abort_requested||s->view.error[0])return session_reject(error,size,"resource callback cancelled session");
    if(ok!=1) {
        if(!error||!size||!error[0])session_message(error,size,"resource host rejected effect");
        return 0;
    }
    return 1;
}

static int session_music_emit(void *user,int logical_ms,const GscMusicEvent *event,
    char *error,size_t size) {
    GscSession *s=user;GscMusicEvent copy;GscSessionMusicCallback callback;int ok;
    if(!s||!s->busy||s->abort_requested||!s->music_enabled||
       s->view.state!=GSC_SESSION_RUNNING||!event||logical_ms<0)
        return session_reject(error,size,"music outside running session");
    copy=*event;callback=copy.kind==GSC_MUSIC_PLAY?s->music.play:
        copy.kind==GSC_MUSIC_STOP?s->music.stop:NULL;
    if(!callback)return session_reject(error,size,"music host capability is unavailable");
    ok=callback(s->music.user,s->view.session_id,logical_ms,&copy,error,size);
    if(s->abort_requested||s->view.error[0])return session_reject(error,size,"music callback cancelled session");
    if(ok!=1) {
        if(!error||!size||!error[0])session_message(error,size,"music host rejected effect");
        return 0;
    }
    return 1;
}
static int session_actor_spawn_prepare(void *user,const GscActorSpawnRequest *request,
    GscActorSpawnTicket *out,int *created,char *error,size_t size) {
    GscSession *s=user;GscActorSpawnRequest copy;const GscActorSpawnRequest*prior_request;GscActorSpawnTicket ticket;int got=-1,ok,prior;
    if(!s||!s->busy||s->abort_requested||!s->actor_spawners_enabled||
       s->view.state!=GSC_SESSION_RUNNING||!request||!out||!created||!s->actor_spawners.prepare)
        return session_reject(error,size,"spawn outside running actor-spawner session");
    copy=*request;memset(&ticket,0,sizeof(ticket));
    prior=s->actor_spawn_preparing;s->actor_spawn_preparing=1;
    prior_request=s->actor_spawn_prepare_request;s->actor_spawn_prepare_request=&copy;
    ok=s->actor_spawners.prepare(s->actor_spawners.user,&copy,&ticket,&got,error,size);
    s->actor_spawn_preparing=prior;s->actor_spawn_prepare_request=prior_request;
    /* Preserve a returned reservation even after reentry so VM unwind can
     * invoke finish(cancel) exactly once before session_drop. */
    *out=ticket;*created=got;
    if(s->abort_requested)return session_reject(error,size,"actor-spawner prepare cancelled by reentry");
    return ok;
}
static int session_actor_spawn_finish(void *user,const GscActorSpawnTicket *ticket,
    int published,char *error,size_t size) {
    GscSession *s=user;GscActorSpawnTicket copy;int ok;
    if(!s||!s->busy||!s->actor_spawners_enabled||
       s->view.state!=GSC_SESSION_RUNNING||!ticket||!s->actor_spawners.finish)
        return session_reject(error,size,"finalization outside running actor-spawner session");
    /* Cancellation must remain callable after abort_requested; replacement
     * cannot run while busy, so the original host owns this reservation. */
    if(s->actor_spawn_finishing||!gsc_actor_spawn_finish_program(s->runtime,ticket,published))
        return session_reject(error,size,"finalization requires actual private VM delivery/cancellation scope");
    copy=*ticket;s->actor_spawn_finishing=&copy;s->actor_spawn_finish_published=published;
    ok=s->actor_spawners.finish(s->actor_spawners.user,&copy,published,error,size);
    s->actor_spawn_finishing=NULL;s->actor_spawn_finish_published=0;
    if(published&&s->abort_requested)return session_reject(error,size,"actor-spawner finalization cancelled by reentry");
    return ok;
}
static int session_predicate(void *user,const GscEntityRef *entity,int *out,
    char *error,size_t size,int sentient) {
    GscSession *s=user;GscEntityRef ref;int got=-1,ok;
    int (*callback)(void *,const GscEntityRef *,int *,char *,size_t);
    if(!s||!s->busy||s->abort_requested||!s->predicates_enabled||
       s->view.state!=GSC_SESSION_RUNNING||!entity||!out)
        return session_reject(error,size,"predicate read outside running predicate session");
    callback=sentient?s->predicates.issentient:s->predicates.isalive;
    if(!callback)return session_reject(error,size,"predicate capability is unavailable");
    ref=*entity;ok=callback(s->predicates.user,&ref,&got,error,size);
    if(!ok||s->abort_requested)return 0;
    *out=got;return 1;
}
static int session_predicate_isalive(void *user,const GscEntityRef *entity,int *out,
    char *error,size_t size) {
    return session_predicate(user,entity,out,error,size,0);
}
static int session_predicate_issentient(void *user,const GscEntityRef *entity,int *out,
    char *error,size_t size) {
    return session_predicate(user,entity,out,error,size,1);
}
#include "gsc_session_start_origin.inc"
#include "gsc_session_animation_namespace.inc"
#include "gsc_session_spawn_entry.inc"
#include "gsc_session_namespace_start_origin.inc"
#include "gsc_session_modules.inc"
#include "gsc_session_script_roots.inc"
int gsc_session_select_sound_notify(GscSession *s,const GscSoundNotifyHost *host,char *err,size_t cap){
 if(!s||!host||!host->invoke||s->busy||s->program||s->runtime||s->sound_notify_enabled||!s->script_sound_enabled||
    ((uintptr_t)host<=(uintptr_t)s ? (uintptr_t)s-(uintptr_t)host<sizeof(*host) : (uintptr_t)host-(uintptr_t)s<sizeof(*s)))return session_reject(err,cap,"sound notify requires idle selected sound session");
 s->sound_notify=*host;s->sound_notify_enabled=1;return 1;
}
#include "gsc_session_loop_sound.inc"
int gsc_session_select_script_sound(GscSession *s,const GscScriptSoundHost *host,char *err,size_t cap) {
 if(!s||!host||!host->invoke||s->busy||s->program||s->runtime||s->script_sound_enabled||!s->dynamic_randomfloat_enabled)
  return session_reject(err,cap,"playsound requires idle selected mode40 session");
 s->script_sound=*host;s->script_sound_enabled=1;return 1;
}
static int session_r243_thread_resolve(void*,const GscEntityRef*,GscEntityExecutionCertificate*,char*,size_t);
int gsc_session_select_r198_execution(GscSession *s,const GscPlayerNativeHost *host,char *err,size_t cap) {
 if(!s||!host||!host->invoke||s->busy||s->program||s->runtime||
    s->execution_extensions||!s->dynamic_randomfloat_enabled)
  return session_reject(err,cap,"R198 execution requires idle selected mode40 session and player host");
 s->player_natives=*host;s->execution_extensions=1;return 1;
}
int gsc_session_select_player_normal_health(GscSession *s,char *err,size_t cap) {
 if(!s||s->busy||s->program||s->runtime||s->view.error[0]||s->abort_requested||
    !s->execution_extensions||s->player_normal_health_enabled)
  return session_reject(err,cap,"normal health requires idle selected player session");
 s->player_normal_health_enabled=1;return 1;
}
int gsc_session_select_player_viewmodel(GscSession *s,char *err,size_t cap) {
 if(!s||s->busy||s->program||s->runtime||s->view.error[0]||s->abort_requested||
    !s->execution_extensions||s->player_viewmodel_enabled)
  return session_reject(err,cap,"viewmodel requires idle selected player session");
 s->player_viewmodel_enabled=1;return 1;
}
int gsc_session_select_player_max_ammo(GscSession *s,char *err,size_t cap) {
 if(!s||s->busy||s->program||s->runtime||s->view.error[0]||s->abort_requested||
    !s->execution_extensions||s->player_max_ammo_enabled)
  return session_reject(err,cap,"max_ammo requires idle selected player session");
 s->player_max_ammo_enabled=1;return 1;
}
int gsc_session_select_map_game_array(GscSession *s,char *err,size_t cap) {
 if(!s||s->busy||s->program||s->runtime||s->view.error[0]||s->abort_requested||
    !s->dynamic_randomfloat_enabled||s->map_game_array_enabled)
  return session_reject(err,cap,"map game array requires idle selected mode40 session");
 s->map_game_array_enabled=1;return 1;
}
int gsc_session_select_r198_entity_operations(GscSession *s,const GscEntityDeleteHost *host,char *err,size_t cap) {
 if(!s||!host||!host->invoke||s->busy||s->program||s->runtime||
    !s->execution_extensions||s->entity_operations_enabled)
  return session_reject(err,cap,"entity operations require idle selected R198 session and delete host");
 s->entity_delete=*host;s->entity_operations_enabled=1;return 1;
}
static int session_actor_overlap(const void *,size_t,const void *,size_t);
int gsc_session_select_origin_method(GscSession *s,const GscEntityOriginMethodHost *host,char *err,size_t cap) {
    if(s&&((host&&session_actor_overlap(host,sizeof(*host),s,sizeof(*s)))||
           (err&&session_actor_overlap(err,cap,s,sizeof(*s)))))return 0;
    if(!s||!host||!host->read||s->busy||s->program||s->runtime||s->view.error[0]||s->abort_requested||
       !s->entity_operations_enabled||!s->access_enabled||s->origin_method_enabled)
        return session_reject(err,cap,"GetOrigin method requires idle selected entity-operation Session and host");
    s->origin_method=*host;s->origin_method_enabled=1;return 1;
}
int gsc_session_select_r198_spatial_natives(GscSession *s,const GscSpatialNativeHost *host,char *err,size_t cap) {
 if(!s||!host||!host->invoke||s->busy||s->program||s->runtime||
    !s->execution_extensions||s->spatial_natives_enabled)
  return session_reject(err,cap,"spatial natives require idle selected R198 session and typed host");
 s->spatial_natives=*host;s->spatial_natives_enabled=1;return 1;
}
typedef struct {GscSession *session;uint64_t id;int (*current)(void *);void *context;} SessionPlayerLease;
static int session_player_current(void *context) {
 SessionPlayerLease *lease=context;GscSession *s=lease->session;
 return s&&s->busy&&!s->abort_requested&&s->view.state==GSC_SESSION_RUNNING&&
    s->view.session_id==lease->id&&lease->current&&lease->current(lease->context);
}
#include "gsc_session_entity_links.inc"
#include "gsc_session_node_queries.inc"
#include "gsc_session_ai_arrays.inc"
#include "gsc_session_grenade_items.inc"
#include "gsc_session_yaw_motion.inc"
#include "gsc_session_actor_goal.inc"
#include "gsc_session_r232_turrets.inc"
#include "gsc_session_r232_turret_animations.inc"
#include "gsc_session_turret_program.inc"
#include "gsc_session_r243_source_dispatch_types.inc"
#include "gsc_session_r232_actor_animations.inc"
#include "gsc_session_r243_critical.inc"
#include "stance-session.inc"
#include "actor-float-session.inc"
#include "move-delta-session.inc"
#include "animtime-session.inc"
#include "gsc_session_r232_actor_look.inc"
#include "gsc_session_r233_actor_presence.inc"
#include "gsc_session_r233_actor_drop.inc"
#include "gsc_session_r243_scripted_prefix.inc"
#include "gsc_session_r243_script_state.inc"
#include "gsc_session_r243_scripted_start.inc"
static int session_after_vm(GscSession*,int,char*,size_t);
#include "gsc_session_r243_controller.inc"
#include "gsc_session_r243_source_dispatch.inc"
#include "gsc_session_r233_namespace_length.inc"
#include "gsc_session_r232_weapon_model.inc"
#include "gsc_session_weapon_classname.inc"
#include "gsc_session_r232_random_range.inc"
#include "gsc_session_r232_source_float.inc"
#include "gsc_session_r232_quoted_strings.inc"
#include "gsc_session_r232_function_admission.inc"
#include "gsc_session_r232_developer.inc"
#include "gsc_session_hud_elements.inc"
static int session_actor_overlap(const void *,size_t,const void *,size_t);
#include "gsc_session_actor_bootstrap.inc"
#include "gsc_session_delete_transactions.inc"
#include "gsc_session_loop_fx.inc"
static int session_player_native(void *user,const GscPlayerNativeRequest *request,
 GscPlayerNativeResult *out,char *error,size_t cap,int (*current)(void *),void *context) {
 GscSession *s=user;GscPlayerNativeRequest copy,expected;GscPlayerNativeResult result;SessionPlayerLease lease;int ok;
 if(!s||!request||!out||!s->execution_extensions||!s->player_natives.invoke)
  return session_reject(error,cap,"player native outside selected session");
 lease.session=s;lease.id=s->view.session_id;lease.current=current;lease.context=context;
 if(!session_player_current(&lease))return session_reject(error,cap,"player native session lease expired");
 copy=*request;memcpy(&expected,&copy,sizeof(copy));memset(&result,0,sizeof(result));
 ok=s->player_natives.invoke(s->player_natives.user,&copy,&result,error,cap,session_player_current,&lease);
 if(ok!=1||!session_player_current(&lease))return 0;
 if(memcmp(&copy,&expected,sizeof(copy)))return session_reject(error,cap,"player host mutated its borrowed request");
 *out=result;return 1;
}
static int session_spatial_native(void *user,const GscSpatialNativeRequest *request,
 GscSpatialNativeResult *out,char *error,size_t cap,int (*current)(void *),void *context) {
 GscSession *s=user;GscSpatialNativeRequest copy,expected;GscSpatialNativeResult result;SessionPlayerLease lease;int ok;
 if(!s||!request||!out||!s->spatial_natives_enabled||!s->spatial_natives.invoke)
  return session_reject(error,cap,"spatial native outside selected session");
 lease.session=s;lease.id=s->view.session_id;lease.current=current;lease.context=context;
 if(!session_player_current(&lease))return session_reject(error,cap,"spatial native session lease expired");
 copy=*request;memcpy(&expected,&copy,sizeof(copy));memset(&result,0,sizeof(result));
 ok=s->spatial_natives.invoke(s->spatial_natives.user,&copy,&result,error,cap,session_player_current,&lease);
 if(ok!=1||!session_player_current(&lease))return 0;
 if(memcmp(&copy,&expected,sizeof(copy)))return session_reject(error,cap,"spatial host mutated its borrowed request");
 *out=result;return 1;
}
static int session_entity_delete(void *user,const GscEntityRef *owner,int now,char *error,
 size_t cap,int (*current)(void *),void *context) {
 GscSession *s=user;GscEntityRef copy,expected;SessionPlayerLease lease;int ok;
 if(!s||!owner||!s->entity_operations_enabled||!s->entity_delete.invoke)
  return session_reject(error,cap,"entity delete outside selected session");
 lease.session=s;lease.id=s->view.session_id;lease.current=current;lease.context=context;
 if(!session_player_current(&lease))return session_reject(error,cap,"entity delete session lease expired");
 copy=*owner;expected=copy;ok=s->entity_delete.invoke(s->entity_delete.user,&copy,now,error,cap,session_player_current,&lease);
 if(ok!=1||!session_player_current(&lease))return 0;
 if(copy.epoch!=expected.epoch||copy.slot!=expected.slot||copy.generation!=expected.generation)
  return session_reject(error,cap,"entity delete host mutated its borrowed owner");
 return 1;
}
int gsc_session_sound_notify_native(GscSession *s,uint64_t id,const GscEntityRef *owner,const char *event,int *awakened,char *error,size_t cap){
 if(!s||!s->busy||!s->sound_notify_enabled||!s->sound_notify_call_active||s->abort_requested||
    s->view.state!=GSC_SESSION_RUNNING||!session_id_matches(s,id)||!s->runtime)
  return session_reject(error,cap,"sound native event outside selected callback");
 return gsc_runtime_sound_notify_native(s->runtime,owner,event,awakened);
}
static int session_sound_notify(void *user,const GscSoundNotifyRequest *request,char *error,size_t cap){
 GscSession *s=user;GscSoundNotifyRequest copy,expected;int ok;
 if(!s||!s->busy||s->abort_requested||s->view.state!=GSC_SESSION_RUNNING||!s->sound_notify_enabled||!request)
  return session_reject(error,cap,"sound notify outside running session");
 copy=*request;expected=copy;s->sound_notify_call_active=1;ok=s->sound_notify.invoke(s->sound_notify.user,&copy,error,cap);s->sound_notify_call_active=0;
 return ok==1&&!s->abort_requested&&!memcmp(&copy,&expected,sizeof(copy));
}
static int session_script_sound(void *user,const GscEntityRef *owner,const char *alias,int argc,char *error,size_t cap) {
 GscSession *s=user;GscEntityRef copy;char text[128];int ok;
 if(!s||!s->busy||s->abort_requested||s->view.state!=GSC_SESSION_RUNNING||!s->script_sound_enabled||!owner||!alias)
  return session_reject(error,cap,"playsound outside running session");
 copy=*owner;memcpy(text,alias,sizeof(text));
 ok=s->script_sound.invoke(s->script_sound.user,&copy,text,argc,error,cap);
 return ok==1&&!s->abort_requested;
}
int gsc_session_destroy(GscSession *s, char *err, size_t cap) {
    session_message(err, cap, ""); if (!s) return 1;
    if(s->node_graph_pin){
        session_terminal(s,"destroy during node graph acquisition");
        session_node_graph_retire(s);
        return session_reject(err,cap,s->view.error);
    }
    if (!session_enter(s, err, cap)) return 0;
    if(!session_drop(s)){s->view.state=GSC_SESSION_FAILED;s->busy=0;return session_reject(err,cap,s->view.error);}
    gsc_animation_catalog_free(s->start_origin_catalog);
    session_animation_namespace_destroy(s);
    GSC_SESSION_FREE(s); return 1;
}
int gsc_session_prepare_program(GscSession *s, const char *path, int ready,
                       uint64_t *out_id, char *err, size_t cap) {
    GscSessionSource source;
    char why[GSC_SESSION_ERROR_CAP], module[GSC_SESSION_PATH_CAP],
         canonical_path[GSC_SESSION_PATH_CAP], *copy = NULL;
    int read_ok, valid;
    if (out_id) *out_id = 0;
    if (!session_enter(s, err, cap)) return 0;
    /* Retire before ANY replacement validation: a bad new map cannot continue
     * the previous map's threads. Reentrant attempts are refused above. */
    if(!session_drop(s)){s->view.state=GSC_SESSION_FAILED;s->busy=0;return session_reject(err,cap,s->view.error);}
    session_clear(s);
    if (!session_new_id(s)) return session_failed(s, "session ID exhausted", err, cap);
    if (!s->host.enabled) return session_failed(s, "GSC adapter disabled", err, cap);
    if (!out_id) return session_failed(s, "session ID output required", err, cap);
    if (ready != 1) return session_failed(s, "map bootstrap not ready", err, cap);
    if(s->modules_enabled) {
        if(!session_module_source_path(path,module,canonical_path))
            return session_failed(s,"invalid selected module graph path",err,cap);
        path=canonical_path;
    } else if (!session_path(path, module))
        return session_failed(s, "invalid selected GSC path", err, cap);
    session_message(s->view.path, sizeof(s->view.path), path);
    session_message(s->view.module, sizeof(s->view.module), module);
    if(!session_animation_resources_current(s))return session_failed(s,"start-origin resource retired before graph acquisition",err,cap);
    if(s->actor_bootstrap_enabled&&!session_actor_map_current(s))return session_failed(s,"actor bootstrap map retired before graph acquisition",err,cap);
    if(!session_script_roots_current(s))return session_failed(s,"global/map declarations retired before graph acquisition",err,cap);
    if(!session_actor_program_current(s))return session_failed(s,"actor program declaration retired before graph acquisition",err,cap);
    s->node_script_phase=s->node_scripts_enabled?2:0;
    if(!session_node_scripts_current(s))return session_failed(s,"node script map retired before acquisition",err,cap);
    if(s->modules_enabled) {
        if(!session_load_modules(s,why,sizeof(why)))
            return session_failed(s,why[0]?why:"module graph acquisition failed",err,cap);
    } else {
    memset(&source, 0, sizeof(source)); memset(why, 0, sizeof(why));
    read_ok = s->host.read(s->host.user, s->view.path, &source, why, sizeof(why));
    why[sizeof(why)-1] = 0;
    if (!read_ok) return session_failed(s, why[0] ? why : "GSC read failed", err, cap);
    valid = source.data && source.size > 0 && source.size <= GSC_SESSION_MAX_SOURCE;
    if (!valid) session_terminal(s, "invalid source buffer or source size limit");
    else if (!s->abort_requested) {
        if (memchr(source.data, 0, source.size)) session_terminal(s, "embedded NUL in GSC source");
        else {
            copy = GSC_SESSION_MALLOC(source.size + 1);
            if (!copy) session_terminal(s, "source copy allocation failed");
            else { memcpy(copy, source.data, source.size); copy[source.size] = 0; }
        }
    }
    /* Every acquired lease is released exactly once, even after reentry/error.
     * No callback retains access to the copied parser source. */
    s->host.release(s->host.user, &source);
    if (s->abort_requested || s->view.error[0] || !copy) {
        GSC_SESSION_FREE(copy);
        return session_failed(s, "source acquisition cancelled", err, cap);
    }
    s->program = gsc_program_create();
    if (!s->program) { GSC_SESSION_FREE(copy); return session_failed(s, "program allocation failed", err, cap); }
    why[0] = 0;
    valid = gsc_module_compile(s->program, s->view.module, copy, why, sizeof(why));
    GSC_SESSION_FREE(copy); copy = NULL;
    if (!valid) return session_failed(s, why[0] ? why : "GSC compile failed", err, cap);
    }
    if(s->owned_wait_delivery_enabled&&!gsc_program_select_owned_wait_delivery(s->program))return session_failed(s,"native wait delivery program selection failed",err,cap);
    if(s->r238_array_value_cow_enabled&&!gsc_program_select_r238_array_value_cow(s->program))return session_failed(s,"R238 array COW program selector refused",err,cap);
    if(s->r239_mixed_scalar_equality_enabled&&!gsc_program_select_r239_mixed_scalar_equality(s->program))return session_failed(s,"R239 mixed equality program selector refused",err,cap);
    if(s->r237_frame_buckets_enabled&&!gsc_program_select_r237_frame_buckets(s->program))
        return session_failed(s,"R237 frame program selection failed",err,cap);
    if(s->r243_thread_start_enabled&&!gsc_program_select_r243_thread_start(s->program))return session_failed(s,"R243 thread start program selector refused",err,cap);
    if(s->configured_precache_interval_enabled&&!gsc_program_select_configured_precache_interval(s->program))return session_failed(s,"configured precache program selector refused",err,cap);
    if(s->r243_same_bucket_wait_enabled&&!gsc_program_select_r243_same_bucket_wait(s->program))return session_failed(s,"R243 same-bucket wait program selector refused",err,cap);
    if(!session_animation_resources_current(s))return session_failed(s,"start-origin resource retired before whole link",err,cap);
    if (!(s->dynamic_randomfloat_enabled?
          gsc_program_link_with_dynamic_randomfloat(s->program,s->view.module,why,sizeof(why)):
          s->numeric_randomint_enabled?
          gsc_program_link_with_numeric_randomint(s->program,s->view.module,why,sizeof(why)):
          s->dynamic_randomint_enabled?
          gsc_program_link_with_dynamic_randomint(s->program,s->view.module,why,sizeof(why)):
          s->entity_queries_enabled?
          gsc_program_link_with_entity_queries(s->program,s->view.module,why,sizeof(why)):
          s->structs_enabled?
          gsc_program_link_with_structs(s->program,s->view.module,why,sizeof(why)):
          s->computed_randomint_enabled?
          gsc_program_link_with_computed_randomint(s->program,s->view.module,why,sizeof(why)):
          s->keybinding_enabled?
          gsc_program_link_with_keybinding(s->program,s->view.module,why,sizeof(why)):
          s->randomfloat_enabled?
          gsc_program_link_with_randomfloat(s->program,s->view.module,why,sizeof(why)):
          s->randomint_enabled?
          gsc_program_link_with_randomint(s->program,s->view.module,why,sizeof(why)):
          s->spawner_arrays_enabled?
          gsc_program_link_with_spawner_arrays(s->program,s->view.module,why,sizeof(why)):
          s->precache_string_enabled?
          gsc_program_link_with_precache_string(s->program,s->view.module,why,sizeof(why)):
          s->native_queries_enabled?
          gsc_program_link_with_native_queries(s->program,s->view.module,why,sizeof(why)):
          s->tag_fx_enabled?
          gsc_program_link_with_tag_fx(s->program,s->view.module,why,sizeof(why)):
          s->save_game_enabled?
          gsc_program_link_with_save_game(s->program,s->view.module,why,sizeof(why)):
          s->precache_item_enabled?
          gsc_program_link_with_precache_item(s->program,s->view.module,why,sizeof(why)):
          s->ambient_enabled?
          gsc_program_link_with_ambient(s->program,s->view.module,why,sizeof(why)):
          s->debug_line_enabled?
          gsc_program_link_with_debug_line(s->program,s->view.module,why,sizeof(why)):
          s->debug_text_enabled?
          gsc_program_link_with_debug_text(s->program,s->view.module,why,sizeof(why)):
          s->shellshock_enabled?
          gsc_program_link_with_shellshock(s->program,s->view.module,why,sizeof(why)):
          s->cvars_enabled?
          gsc_program_link_with_cvars(s->program,s->view.module,why,sizeof(why)):
          s->fx_enabled?
          gsc_program_link_with_fx(s->program,s->view.module,why,sizeof(why)):
          s->quake_enabled?
          gsc_program_link_with_quake(s->program,s->view.module,why,sizeof(why)):
          s->blast_enabled?
          gsc_program_link_with_blast(s->program,s->view.module,why,sizeof(why)):
          s->mission_enabled?
          gsc_program_link_with_mission(s->program,s->view.module,why,sizeof(why)):
          s->hud_enabled?
          gsc_program_link_with_hud(s->program,s->view.module,why,sizeof(why)):
          s->console_enabled?
          gsc_program_link_with_console(s->program,s->view.module,why,sizeof(why)):
          s->objectives_enabled?
          gsc_program_link_with_objectives(s->program,s->view.module,why,sizeof(why)):
          s->resources_enabled?
          gsc_program_link_with_resources(s->program,s->view.module,why,sizeof(why)):
          s->music_enabled?
          gsc_program_link_with_music(s->program,s->view.module,why,sizeof(why)):
          s->names_enabled?
          gsc_program_link_with_names(s->program,s->view.module,why,sizeof(why)):
          s->lifetime_enabled?
          gsc_program_link_with_lifetime(s->program,s->view.module,why,sizeof(why)):
          s->scalar_switch_enabled?
          gsc_program_link_with_scalar_switch(s->program,s->view.module,why,sizeof(why)):
          s->function_refs_enabled?
          gsc_program_link_with_function_refs(s->program,s->view.module,why,sizeof(why)):
          s->actor_spawners_enabled?
          gsc_program_link_with_actor_spawners(s->program,s->view.module,why,sizeof(why)):
          s->predicates_enabled?
          gsc_program_link_with_predicates(s->program,s->view.module,why,sizeof(why)):
          s->event_payloads_enabled?
          gsc_program_link_with_event_payloads(s->program,s->view.module,why,sizeof(why)):
          s->results_enabled?
          gsc_program_link_with_results(s->program,s->view.module,why,sizeof(why)):
          s->arrays_enabled?
          gsc_program_link_with_arrays(s->program,s->view.module,why,sizeof(why)):
          s->flow_enabled?
          gsc_program_link_with_flow(s->program,s->view.module,why,sizeof(why)):
          s->controls_enabled?
          gsc_program_link_with_controls(s->program,s->view.module,why,sizeof(why)):
          s->state_enabled?
          gsc_program_link_with_state(s->program,s->view.module,why,sizeof(why)):
          s->events_enabled?
          gsc_program_link_with_events(s->program,s->view.module,why,sizeof(why)):
          s->access_enabled?
          gsc_program_link_with_access(s->program,s->view.module,why,sizeof(why)):
          s->entities_enabled?
          gsc_program_link_with_entities(s->program,s->view.module,why,sizeof(why)):
          gsc_program_link(s->program, s->view.module, why, sizeof(why))))
        return session_failed(s, why[0] ? why : "GSC link failed", err, cap);
    if(s->script_roots_enabled&&!session_script_roots_build(s,why,sizeof(why)))return session_failed(s,why,err,cap);
    if(s->actor_program_enabled&&!session_actor_cache_build(s,why,sizeof(why)))
        return session_failed(s,why,err,cap);
    /* Own only the fully linked program. No runtime, tasks, hooks or Actor
     * constructor exists here. The selected owners are rechecked on both sides
     * of the public boundary; no synchronous execution certificate is kept. */
    if(s->abort_requested||s->view.error[0]||!session_script_roots_current(s)||!session_animation_resources_current(s)||
       (s->actor_bootstrap_enabled&&!session_actor_map_current(s))||!session_actor_program_current(s)||!session_node_scripts_current(s))
        return session_failed(s,"program owner retired during compile/link",err,cap);
    s->view.state=GSC_SESSION_COMPILED;*out_id=s->view.session_id;s->busy=0;return 1;
}
#include "gsc_session_bsp_construction.inc"
void gsc_session_bind_readonly_context(GscSession *s, uint64_t id,
    const GscEntityHost *host, const GscReadOnlyContext *context) {
    if (!s || !host || !context || s->busy || s->abort_requested ||
        !session_id_matches(s,id) || s->view.state != GSC_SESSION_PREPARED ||
        !s->runtime || !s->turret_program_admitted || !s->entities_enabled ||
        s->entities.validate != host->validate || s->entities.user != host->user) return;
    gsc_runtime_bind_readonly_context(s->runtime,context,s,session_entity_validate);
}
int gsc_session_prepare_runtime(GscSession *s,uint64_t id,char *err,size_t cap){
    char why[GSC_SESSION_ERROR_CAP];int promote;
    GscEntityHost runtime_entities;
    GscEntityAccessHost runtime_access;
    GscEntityControlHost runtime_controls;
    GscEntityCollectionHost runtime_collections;
    GscEntityPredicateHost runtime_predicates; GscMusicHost runtime_music;GscResourceHost runtime_resources;
    GscObjectiveHost runtime_objectives;GscConsoleHost runtime_console;GscHudHost runtime_hud;GscMissionHost runtime_mission;GscBlastHost runtime_blast;GscQuakeHost runtime_quake;GscFxHost runtime_fx;GscCvarHost runtime_cvars;GscShellshockHost runtime_shellshock;GscDebugTextHost runtime_debug_text;GscDebugLineHost runtime_debug_line;GscAmbientHost runtime_ambient;GscPrecacheItemHost runtime_precache_item;GscSaveGameHost runtime_save_game;GscTagFxHost runtime_tag_fx;GscNativeQueryHost runtime_native_queries;GscPrecacheStringHost runtime_precache_string;GscSpawnerArrayHost runtime_spawner_arrays;GscRandomIntHost runtime_randomint;GscRandomFloatHost runtime_randomfloat;GscKeyBindingHost runtime_keybinding;GscEntityQueryHost runtime_entity_queries;
    GscActorSpawnHost runtime_actor_spawners;
    if(s&&s->bsp_runtime&&(!gsc_runtime_storage_separate(s->bsp_runtime,err,cap)||session_actor_overlap(err,cap,s,sizeof(*s))))return 0;
    if(!session_enter(s,err,cap))return 0;
    if(!session_id_matches(s,id))return session_leave_rejected(s,"stale session ID",err,cap);
    promote=s->bsp_runtime!=NULL;
    if(promote&&(!s->dynamic_randomfloat_enabled||!s->actor_bootstrap_enabled||
       !s->bsp_count||s->bsp_count!=s->actor_bootstrap.placed_count||s->bsp_scope||s->bsp_promoted))
        return session_leave_rejected(s,"BSP promotion requires advanced profile and exact admitted complete cohort",err,cap);
    if(s->view.state!=GSC_SESSION_COMPILED||!s->program||s->runtime)
        return session_leave_rejected(s,"session program not compiled for runtime preparation",err,cap);
    if(s->turret_program_enabled&&(!s->turret_program_admitted||!s->turret_admission_attempted))
        return session_failed(s,"turret program needs fresh resource admission",err,cap);
    if(s->node_program_enabled&&(!s->node_program_admitted||!s->node_admission_attempted))
        return session_failed(s,"node program requires fresh registry admission before runtime",err,cap);
    if(s->actor_program_enabled&&(!s->actor_bootstrap_enabled||!s->actor_admission_attempted))
        return session_failed(s,"actor program requires real cohort admission before runtime",err,cap);
    if(!session_script_roots_current(s)||!session_script_roots_cache_current(s)||!session_actor_cache_current(s)||!session_actor_program_current(s)||!session_animation_resources_current(s)||(s->actor_bootstrap_enabled&&!session_actor_map_current(s))||
       !session_node_scripts_current(s))return session_failed(s,"program owner retired before runtime preparation",err,cap);
    why[0]=0;
    if(s->entities_enabled) {
        memset(&runtime_entities,0,sizeof(runtime_entities));runtime_entities.user=s;
        runtime_entities.lookup=session_entity_lookup;runtime_entities.setorigin=session_entity_setorigin;
        runtime_entities.validate=session_entity_validate;
        if(s->entities.spawn)runtime_entities.spawn=session_entity_spawn;
        if(s->entities.spawn_with_flags)runtime_entities.spawn_with_flags=session_entity_spawn_with_flags;
        if(s->access_enabled) {
            memset(&runtime_access,0,sizeof(runtime_access));runtime_access.user=s;
            if(s->access.setmodel)runtime_access.setmodel=session_access_setmodel;
            if(s->access.getorigin)runtime_access.getorigin=session_access_getorigin;
            if(s->access.getangles)runtime_access.getangles=session_access_getangles;
            if(s->access.setangles)runtime_access.setangles=session_access_setangles;
            memset(&runtime_controls,0,sizeof(runtime_controls));runtime_controls.user=s;
            if(s->controls.getstring)runtime_controls.getstring=session_control_getstring;
            if(s->controls.setvisible)runtime_controls.setvisible=session_control_setvisible;
            if(s->controls.setorigin)runtime_controls.setorigin=session_control_setorigin;
            memset(&runtime_collections,0,sizeof(runtime_collections));runtime_collections.user=s;
            if(s->collections.lookup_array)runtime_collections.lookup_array=session_collection_lookup;
            memset(&runtime_predicates,0,sizeof(runtime_predicates));runtime_predicates.user=s;
            if(s->predicates.isalive)runtime_predicates.isalive=session_predicate_isalive;
            if(s->predicates.issentient)runtime_predicates.issentient=session_predicate_issentient;
            memset(&runtime_actor_spawners,0,sizeof(runtime_actor_spawners));runtime_actor_spawners.user=s;
            if(s->actor_spawners.prepare)runtime_actor_spawners.prepare=session_actor_spawn_prepare;
            if(s->actor_spawners.finish)runtime_actor_spawners.finish=session_actor_spawn_finish;
            memset(&runtime_music,0,sizeof(runtime_music));runtime_music.user=s;
            if(s->music.play)runtime_music.play=session_music_emit;
            if(s->music.stop)runtime_music.stop=session_music_emit;
            memset(&runtime_resources,0,sizeof(runtime_resources));runtime_resources.user=s;
            if(s->resources.shader)runtime_resources.shader=session_resource_emit;
            if(s->resources.model)runtime_resources.model=session_resource_emit;
            memset(&runtime_objectives,0,sizeof(runtime_objectives));runtime_objectives.user=s;
            if(s->objectives.add)runtime_objectives.add=session_objective_emit;
            if(s->objectives.delete_record)runtime_objectives.delete_record=session_objective_emit;
            if(s->objectives.state)runtime_objectives.state=session_objective_emit;
            if(s->objectives.string)runtime_objectives.string=session_objective_emit;
            if(s->objectives.string_nomessage)runtime_objectives.string_nomessage=session_objective_emit;
            if(s->objectives.position)runtime_objectives.position=session_objective_emit;
            if(s->objectives.current)runtime_objectives.current=session_objective_emit;
            if(s->objectives.ring)runtime_objectives.ring=session_objective_emit;
            memset(&runtime_console,0,sizeof(runtime_console));runtime_console.user=s;
            if(s->console.query_gate)runtime_console.query_gate=session_console_gate;
            if(s->console.print)runtime_console.print=session_console_emit;
            if(s->console.println)runtime_console.println=session_console_emit;
            memset(&runtime_hud,0,sizeof(runtime_hud));runtime_hud.user=s;
            if(s->hud.print)runtime_hud.print=session_hud_emit;
            if(s->hud.bold)runtime_hud.bold=session_hud_emit;
            memset(&runtime_mission,0,sizeof(runtime_mission));runtime_mission.user=s;
            if(s->mission.query_gate)runtime_mission.query_gate=session_mission_gate;
            if(s->mission.success)runtime_mission.success=session_mission_emit;
            if(s->mission.failed)runtime_mission.failed=session_mission_emit;
            memset(&runtime_blast,0,sizeof(runtime_blast));runtime_blast.user=s;
            if(s->blast.radius)runtime_blast.radius=session_blast_emit;
            memset(&runtime_quake,0,sizeof(runtime_quake));runtime_quake.user=s;
            if(s->quake.emit)runtime_quake.emit=session_quake_emit;
            memset(&runtime_fx,0,sizeof(runtime_fx));runtime_fx.user=s;
            if(s->fx.load)runtime_fx.load=session_fx_load;
            if(s->fx.play)runtime_fx.play=session_fx_play;
            memset(&runtime_cvars,0,sizeof(runtime_cvars));runtime_cvars.user=s;
            if(s->cvars.publish)runtime_cvars.publish=session_cvar_publish;
            memset(&runtime_shellshock,0,sizeof(runtime_shellshock));runtime_shellshock.user=s;
            if(s->shellshock.publish)runtime_shellshock.publish=session_shellshock_publish;
            memset(&runtime_debug_text,0,sizeof(runtime_debug_text));runtime_debug_text.user=s;
            if(s->debug_text.publish)runtime_debug_text.publish=session_debug_text_publish;
            memset(&runtime_debug_line,0,sizeof(runtime_debug_line));runtime_debug_line.user=s;
            if(s->debug_line.publish)runtime_debug_line.publish=session_debug_line_publish;
            memset(&runtime_ambient,0,sizeof(runtime_ambient));runtime_ambient.user=s;
            if(s->ambient.publish)runtime_ambient.publish=session_ambient_publish;
            memset(&runtime_precache_item,0,sizeof(runtime_precache_item));runtime_precache_item.user=s;
            if(s->precache_item.publish)runtime_precache_item.publish=session_precache_item_publish;
            memset(&runtime_save_game,0,sizeof(runtime_save_game));runtime_save_game.user=s;
            if(s->save_game.query)runtime_save_game.query=session_save_game_query;
            if(s->save_game.publish)runtime_save_game.publish=session_save_game_publish;
            memset(&runtime_tag_fx,0,sizeof(runtime_tag_fx));runtime_tag_fx.user=s;
            if(s->tag_fx.query)runtime_tag_fx.query=session_tag_fx_query;
            if(s->tag_fx.publish)runtime_tag_fx.publish=session_tag_fx_publish;
            memset(&runtime_native_queries,0,sizeof(runtime_native_queries));runtime_native_queries.user=s;
            if(s->native_queries.query)runtime_native_queries.query=session_native_query;
            memset(&runtime_precache_string,0,sizeof(runtime_precache_string));runtime_precache_string.user=s;
            if(s->precache_string.startup)runtime_precache_string.startup=session_precache_string_startup;
            if(s->precache_string.publish)runtime_precache_string.publish=session_precache_string_publish;
            memset(&runtime_spawner_arrays,0,sizeof(runtime_spawner_arrays));runtime_spawner_arrays.user=s;
            if(s->spawner_arrays.collect_all)runtime_spawner_arrays.collect_all=session_spawner_array_collect;
            memset(&runtime_randomint,0,sizeof(runtime_randomint));runtime_randomint.user=s;
            if(s->randomint.draw)runtime_randomint.draw=session_randomint_draw;
            memset(&runtime_randomfloat,0,sizeof(runtime_randomfloat));runtime_randomfloat.user=s;
            if(s->randomfloat.draw)runtime_randomfloat.draw=session_randomfloat_draw;
            memset(&runtime_keybinding,0,sizeof(runtime_keybinding));runtime_keybinding.user=s;
            if(s->keybinding.query)runtime_keybinding.query=session_keybinding_query;
            memset(&runtime_entity_queries,0,sizeof(runtime_entity_queries));runtime_entity_queries.user=s;
            if(s->entity_queries.by_number)runtime_entity_queries.by_number=session_entity_number_query;
            if(s->entity_queries.collect_team)runtime_entity_queries.collect_team=session_spawner_team_collect;
            s->runtime=promote?
                gsc_runtime_promote_bsp_dynamic_randomfloat(s->bsp_runtime,s->program,session_fog,s,
                    &runtime_entities,&runtime_access,&runtime_controls,
                    s->arrays_enabled?&runtime_collections:NULL,&runtime_predicates,&runtime_actor_spawners,&runtime_music,&runtime_resources,&runtime_objectives,&runtime_console,&runtime_hud,&runtime_mission,&runtime_blast,&runtime_quake,&runtime_fx,&runtime_cvars,&runtime_shellshock,&runtime_debug_text,&runtime_debug_line,&runtime_ambient,&runtime_precache_item,&runtime_save_game,&runtime_tag_fx,&runtime_native_queries,&runtime_precache_string,&runtime_spawner_arrays,&runtime_randomint,&runtime_randomfloat,&runtime_keybinding,&runtime_entity_queries,why,sizeof(why)):
                s->dynamic_randomfloat_enabled?
                gsc_runtime_new_with_dynamic_randomfloat(s->program,session_fog,s,
                    &runtime_entities,&runtime_access,&runtime_controls,
                    s->arrays_enabled?&runtime_collections:NULL,&runtime_predicates,&runtime_actor_spawners,&runtime_music,&runtime_resources,&runtime_objectives,&runtime_console,&runtime_hud,&runtime_mission,&runtime_blast,&runtime_quake,&runtime_fx,&runtime_cvars,&runtime_shellshock,&runtime_debug_text,&runtime_debug_line,&runtime_ambient,&runtime_precache_item,&runtime_save_game,&runtime_tag_fx,&runtime_native_queries,&runtime_precache_string,&runtime_spawner_arrays,&runtime_randomint,&runtime_randomfloat,&runtime_keybinding,&runtime_entity_queries,why,sizeof(why)):
            s->numeric_randomint_enabled?
                gsc_runtime_new_with_numeric_randomint(s->program,session_fog,s,
                    &runtime_entities,&runtime_access,&runtime_controls,
                    s->arrays_enabled?&runtime_collections:NULL,&runtime_predicates,&runtime_actor_spawners,&runtime_music,&runtime_resources,&runtime_objectives,&runtime_console,&runtime_hud,&runtime_mission,&runtime_blast,&runtime_quake,&runtime_fx,&runtime_cvars,&runtime_shellshock,&runtime_debug_text,&runtime_debug_line,&runtime_ambient,&runtime_precache_item,&runtime_save_game,&runtime_tag_fx,&runtime_native_queries,&runtime_precache_string,&runtime_spawner_arrays,&runtime_randomint,&runtime_randomfloat,&runtime_keybinding,&runtime_entity_queries,why,sizeof(why)):
            s->dynamic_randomint_enabled?
                gsc_runtime_new_with_dynamic_randomint(s->program,session_fog,s,
                    &runtime_entities,&runtime_access,&runtime_controls,
                    s->arrays_enabled?&runtime_collections:NULL,&runtime_predicates,&runtime_actor_spawners,&runtime_music,&runtime_resources,&runtime_objectives,&runtime_console,&runtime_hud,&runtime_mission,&runtime_blast,&runtime_quake,&runtime_fx,&runtime_cvars,&runtime_shellshock,&runtime_debug_text,&runtime_debug_line,&runtime_ambient,&runtime_precache_item,&runtime_save_game,&runtime_tag_fx,&runtime_native_queries,&runtime_precache_string,&runtime_spawner_arrays,&runtime_randomint,&runtime_randomfloat,&runtime_keybinding,&runtime_entity_queries,why,sizeof(why)):
            s->entity_queries_enabled?
                gsc_runtime_new_with_entity_queries(s->program,session_fog,s,
                    &runtime_entities,&runtime_access,&runtime_controls,
                    s->arrays_enabled?&runtime_collections:NULL,&runtime_predicates,&runtime_actor_spawners,&runtime_music,&runtime_resources,&runtime_objectives,&runtime_console,&runtime_hud,&runtime_mission,&runtime_blast,&runtime_quake,&runtime_fx,&runtime_cvars,&runtime_shellshock,&runtime_debug_text,&runtime_debug_line,&runtime_ambient,&runtime_precache_item,&runtime_save_game,&runtime_tag_fx,&runtime_native_queries,&runtime_precache_string,&runtime_spawner_arrays,&runtime_randomint,&runtime_randomfloat,&runtime_keybinding,&runtime_entity_queries,why,sizeof(why)):
            s->structs_enabled?
                gsc_runtime_new_with_structs(s->program,session_fog,s,
                    &runtime_entities,&runtime_access,&runtime_controls,
                    s->arrays_enabled?&runtime_collections:NULL,&runtime_predicates,&runtime_actor_spawners,&runtime_music,&runtime_resources,&runtime_objectives,&runtime_console,&runtime_hud,&runtime_mission,&runtime_blast,&runtime_quake,&runtime_fx,&runtime_cvars,&runtime_shellshock,&runtime_debug_text,&runtime_debug_line,&runtime_ambient,&runtime_precache_item,&runtime_save_game,&runtime_tag_fx,&runtime_native_queries,&runtime_precache_string,&runtime_spawner_arrays,&runtime_randomint,&runtime_randomfloat,&runtime_keybinding,why,sizeof(why)):
            s->computed_randomint_enabled?
                gsc_runtime_new_with_computed_randomint(s->program,session_fog,s,
                    &runtime_entities,&runtime_access,&runtime_controls,
                    s->arrays_enabled?&runtime_collections:NULL,&runtime_predicates,&runtime_actor_spawners,&runtime_music,&runtime_resources,&runtime_objectives,&runtime_console,&runtime_hud,&runtime_mission,&runtime_blast,&runtime_quake,&runtime_fx,&runtime_cvars,&runtime_shellshock,&runtime_debug_text,&runtime_debug_line,&runtime_ambient,&runtime_precache_item,&runtime_save_game,&runtime_tag_fx,&runtime_native_queries,&runtime_precache_string,&runtime_spawner_arrays,&runtime_randomint,&runtime_randomfloat,&runtime_keybinding,why,sizeof(why)):
                s->keybinding_enabled?
                gsc_runtime_new_with_keybinding(s->program,session_fog,s,
                    &runtime_entities,&runtime_access,&runtime_controls,
                    s->arrays_enabled?&runtime_collections:NULL,&runtime_predicates,&runtime_actor_spawners,&runtime_music,&runtime_resources,&runtime_objectives,&runtime_console,&runtime_hud,&runtime_mission,&runtime_blast,&runtime_quake,&runtime_fx,&runtime_cvars,&runtime_shellshock,&runtime_debug_text,&runtime_debug_line,&runtime_ambient,&runtime_precache_item,&runtime_save_game,&runtime_tag_fx,&runtime_native_queries,&runtime_precache_string,&runtime_spawner_arrays,&runtime_randomint,&runtime_randomfloat,&runtime_keybinding,why,sizeof(why)):
            s->randomfloat_enabled?
                gsc_runtime_new_with_randomfloat(s->program,session_fog,s,
                    &runtime_entities,&runtime_access,&runtime_controls,
                    s->arrays_enabled?&runtime_collections:NULL,&runtime_predicates,&runtime_actor_spawners,&runtime_music,&runtime_resources,&runtime_objectives,&runtime_console,&runtime_hud,&runtime_mission,&runtime_blast,&runtime_quake,&runtime_fx,&runtime_cvars,&runtime_shellshock,&runtime_debug_text,&runtime_debug_line,&runtime_ambient,&runtime_precache_item,&runtime_save_game,&runtime_tag_fx,&runtime_native_queries,&runtime_precache_string,&runtime_spawner_arrays,&runtime_randomint,&runtime_randomfloat,why,sizeof(why)):
            s->randomint_enabled?
                gsc_runtime_new_with_randomint(s->program,session_fog,s,
                    &runtime_entities,&runtime_access,&runtime_controls,
                    s->arrays_enabled?&runtime_collections:NULL,&runtime_predicates,&runtime_actor_spawners,&runtime_music,&runtime_resources,&runtime_objectives,&runtime_console,&runtime_hud,&runtime_mission,&runtime_blast,&runtime_quake,&runtime_fx,&runtime_cvars,&runtime_shellshock,&runtime_debug_text,&runtime_debug_line,&runtime_ambient,&runtime_precache_item,&runtime_save_game,&runtime_tag_fx,&runtime_native_queries,&runtime_precache_string,&runtime_spawner_arrays,&runtime_randomint,why,sizeof(why)):
            s->spawner_arrays_enabled?
                gsc_runtime_new_with_spawner_arrays(s->program,session_fog,s,
                    &runtime_entities,&runtime_access,&runtime_controls,
                    s->arrays_enabled?&runtime_collections:NULL,&runtime_predicates,&runtime_actor_spawners,&runtime_music,&runtime_resources,&runtime_objectives,&runtime_console,&runtime_hud,&runtime_mission,&runtime_blast,&runtime_quake,&runtime_fx,&runtime_cvars,&runtime_shellshock,&runtime_debug_text,&runtime_debug_line,&runtime_ambient,&runtime_precache_item,&runtime_save_game,&runtime_tag_fx,&runtime_native_queries,&runtime_precache_string,&runtime_spawner_arrays,why,sizeof(why)):
                s->precache_string_enabled?
                gsc_runtime_new_with_precache_string(s->program,session_fog,s,
                    &runtime_entities,&runtime_access,&runtime_controls,
                    s->arrays_enabled?&runtime_collections:NULL,&runtime_predicates,&runtime_actor_spawners,&runtime_music,&runtime_resources,
                    &runtime_objectives,&runtime_console,&runtime_hud,&runtime_mission,&runtime_blast,&runtime_quake,&runtime_fx,&runtime_cvars,&runtime_shellshock,&runtime_debug_text,&runtime_debug_line,&runtime_ambient,&runtime_precache_item,&runtime_save_game,&runtime_tag_fx,&runtime_native_queries,&runtime_precache_string,why,sizeof(why)):
                s->native_queries_enabled?
                gsc_runtime_new_with_native_queries(s->program,session_fog,s,
                    &runtime_entities,&runtime_access,&runtime_controls,
                    s->arrays_enabled?&runtime_collections:NULL,&runtime_predicates,&runtime_actor_spawners,&runtime_music,&runtime_resources,
                    &runtime_objectives,&runtime_console,&runtime_hud,&runtime_mission,&runtime_blast,&runtime_quake,&runtime_fx,&runtime_cvars,&runtime_shellshock,&runtime_debug_text,&runtime_debug_line,&runtime_ambient,&runtime_precache_item,&runtime_save_game,&runtime_tag_fx,&runtime_native_queries,why,sizeof(why)):
                s->tag_fx_enabled?
                gsc_runtime_new_with_tag_fx(s->program,session_fog,s,
                    &runtime_entities,&runtime_access,&runtime_controls,
                    s->arrays_enabled?&runtime_collections:NULL,&runtime_predicates,&runtime_actor_spawners,&runtime_music,&runtime_resources,
                    &runtime_objectives,&runtime_console,&runtime_hud,&runtime_mission,&runtime_blast,&runtime_quake,&runtime_fx,&runtime_cvars,&runtime_shellshock,&runtime_debug_text,&runtime_debug_line,&runtime_ambient,&runtime_precache_item,&runtime_save_game,&runtime_tag_fx,why,sizeof(why)):
                s->save_game_enabled?
                gsc_runtime_new_with_save_game(s->program,session_fog,s,
                    &runtime_entities,&runtime_access,&runtime_controls,
                    s->arrays_enabled?&runtime_collections:NULL,&runtime_predicates,&runtime_actor_spawners,&runtime_music,&runtime_resources,
                    &runtime_objectives,&runtime_console,&runtime_hud,&runtime_mission,&runtime_blast,&runtime_quake,&runtime_fx,&runtime_cvars,&runtime_shellshock,&runtime_debug_text,&runtime_debug_line,&runtime_ambient,&runtime_precache_item,&runtime_save_game,why,sizeof(why)):
                s->precache_item_enabled?
                gsc_runtime_new_with_precache_item(s->program,session_fog,s,
                    &runtime_entities,&runtime_access,&runtime_controls,
                    s->arrays_enabled?&runtime_collections:NULL,&runtime_predicates,&runtime_actor_spawners,&runtime_music,&runtime_resources,
                    &runtime_objectives,&runtime_console,&runtime_hud,&runtime_mission,&runtime_blast,&runtime_quake,&runtime_fx,&runtime_cvars,&runtime_shellshock,&runtime_debug_text,&runtime_debug_line,&runtime_ambient,&runtime_precache_item,why,sizeof(why)):
                s->ambient_enabled?
                gsc_runtime_new_with_ambient(s->program,session_fog,s,
                    &runtime_entities,&runtime_access,&runtime_controls,
                    s->arrays_enabled?&runtime_collections:NULL,&runtime_predicates,&runtime_actor_spawners,&runtime_music,&runtime_resources,
                    &runtime_objectives,&runtime_console,&runtime_hud,&runtime_mission,&runtime_blast,&runtime_quake,&runtime_fx,&runtime_cvars,&runtime_shellshock,&runtime_debug_text,&runtime_debug_line,&runtime_ambient,why,sizeof(why)):
                s->debug_line_enabled?
                gsc_runtime_new_with_debug_line(s->program,session_fog,s,
                    &runtime_entities,&runtime_access,&runtime_controls,
                    s->arrays_enabled?&runtime_collections:NULL,&runtime_predicates,&runtime_actor_spawners,&runtime_music,&runtime_resources,
                    &runtime_objectives,&runtime_console,&runtime_hud,&runtime_mission,&runtime_blast,&runtime_quake,&runtime_fx,&runtime_cvars,&runtime_shellshock,&runtime_debug_text,&runtime_debug_line,why,sizeof(why)):
                s->debug_text_enabled?
                gsc_runtime_new_with_debug_text(s->program,session_fog,s,
                    &runtime_entities,&runtime_access,&runtime_controls,
                    s->arrays_enabled?&runtime_collections:NULL,&runtime_predicates,&runtime_actor_spawners,&runtime_music,&runtime_resources,
                    &runtime_objectives,&runtime_console,&runtime_hud,&runtime_mission,&runtime_blast,&runtime_quake,&runtime_fx,&runtime_cvars,&runtime_shellshock,&runtime_debug_text,why,sizeof(why)):
                s->shellshock_enabled?
                gsc_runtime_new_with_shellshock(s->program,session_fog,s,
                    &runtime_entities,&runtime_access,&runtime_controls,
                    s->arrays_enabled?&runtime_collections:NULL,&runtime_predicates,&runtime_actor_spawners,&runtime_music,&runtime_resources,
                    &runtime_objectives,&runtime_console,&runtime_hud,&runtime_mission,&runtime_blast,&runtime_quake,&runtime_fx,&runtime_cvars,&runtime_shellshock,why,sizeof(why)):
                s->cvars_enabled?
                gsc_runtime_new_with_cvars(s->program,session_fog,s,
                    &runtime_entities,&runtime_access,&runtime_controls,
                    s->arrays_enabled?&runtime_collections:NULL,&runtime_predicates,&runtime_actor_spawners,&runtime_music,&runtime_resources,
                    &runtime_objectives,&runtime_console,&runtime_hud,&runtime_mission,&runtime_blast,&runtime_quake,&runtime_fx,&runtime_cvars,why,sizeof(why)):
                s->fx_enabled?
                gsc_runtime_new_with_fx(s->program,session_fog,s,
                    &runtime_entities,&runtime_access,&runtime_controls,
                    s->arrays_enabled?&runtime_collections:NULL,&runtime_predicates,&runtime_actor_spawners,&runtime_music,&runtime_resources,
                    &runtime_objectives,&runtime_console,&runtime_hud,&runtime_mission,&runtime_blast,&runtime_quake,&runtime_fx,why,sizeof(why)):
                s->quake_enabled?
                gsc_runtime_new_with_quake(s->program,session_fog,s,
                    &runtime_entities,&runtime_access,&runtime_controls,
                    s->arrays_enabled?&runtime_collections:NULL,&runtime_predicates,&runtime_actor_spawners,&runtime_music,&runtime_resources,
                    &runtime_objectives,&runtime_console,&runtime_hud,&runtime_mission,&runtime_blast,&runtime_quake,why,sizeof(why)):
                s->blast_enabled?
                gsc_runtime_new_with_blast(s->program,session_fog,s,
                    &runtime_entities,&runtime_access,&runtime_controls,&runtime_collections,
                    &runtime_predicates,&runtime_actor_spawners,&runtime_music,&runtime_resources,
                    &runtime_objectives,&runtime_console,&runtime_hud,&runtime_mission,&runtime_blast,why,sizeof(why)):
                s->mission_enabled?
                gsc_runtime_new_with_mission(s->program,session_fog,s,
                    &runtime_entities,&runtime_access,&runtime_controls,&runtime_collections,
                    &runtime_predicates,&runtime_actor_spawners,&runtime_music,&runtime_resources,
                    &runtime_objectives,&runtime_console,&runtime_hud,&runtime_mission,why,sizeof(why)):
                s->hud_enabled?
                gsc_runtime_new_with_hud(s->program,session_fog,s,
                    &runtime_entities,&runtime_access,&runtime_controls,&runtime_collections,
                    &runtime_predicates,&runtime_actor_spawners,&runtime_music,&runtime_resources,
                    &runtime_objectives,&runtime_console,&runtime_hud,why,sizeof(why)):
                s->console_enabled?
                gsc_runtime_new_with_console(s->program,session_fog,s,
                    &runtime_entities,&runtime_access,&runtime_controls,&runtime_collections,
                    &runtime_predicates,&runtime_actor_spawners,&runtime_music,&runtime_resources,
                    &runtime_objectives,&runtime_console,why,sizeof(why)):
                s->objectives_enabled?
                gsc_runtime_new_with_objectives(s->program,session_fog,s,
                    &runtime_entities,&runtime_access,&runtime_controls,&runtime_collections,
                    &runtime_predicates,&runtime_actor_spawners,&runtime_music,&runtime_resources,
                    &runtime_objectives,why,sizeof(why)):
                s->resources_enabled?
                gsc_runtime_new_with_resources(s->program,session_fog,s,
                    &runtime_entities,&runtime_access,&runtime_controls,&runtime_collections,
                    &runtime_predicates,&runtime_actor_spawners,&runtime_music,&runtime_resources,why,sizeof(why)):
                s->music_enabled?
                gsc_runtime_new_with_music(s->program,session_fog,s,
                    &runtime_entities,&runtime_access,&runtime_controls,&runtime_collections,
                    &runtime_predicates,&runtime_actor_spawners,&runtime_music,why,sizeof(why)):
                s->names_enabled?
                gsc_runtime_new_with_names(s->program,session_fog,s,
                    &runtime_entities,&runtime_access,&runtime_controls,&runtime_collections,
                    &runtime_predicates,&runtime_actor_spawners,why,sizeof(why)):
                s->lifetime_enabled?
                gsc_runtime_new_with_lifetime(s->program,session_fog,s,
                    &runtime_entities,&runtime_access,&runtime_controls,&runtime_collections,
                    &runtime_predicates,&runtime_actor_spawners,why,sizeof(why)):
                s->scalar_switch_enabled?
                gsc_runtime_new_with_scalar_switch(s->program,session_fog,s,
                    &runtime_entities,&runtime_access,&runtime_controls,&runtime_collections,
                    &runtime_predicates,&runtime_actor_spawners,why,sizeof(why)):
                s->function_refs_enabled?
                gsc_runtime_new_with_function_refs(s->program,session_fog,s,
                    &runtime_entities,&runtime_access,&runtime_controls,&runtime_collections,
                    &runtime_predicates,&runtime_actor_spawners,why,sizeof(why)):
                s->actor_spawners_enabled?
                gsc_runtime_new_with_actor_spawners(s->program,session_fog,s,
                    &runtime_entities,&runtime_access,&runtime_controls,&runtime_collections,
                    &runtime_predicates,&runtime_actor_spawners,why,sizeof(why)):
                s->predicates_enabled?
                gsc_runtime_new_with_predicates(s->program,session_fog,s,
                    &runtime_entities,&runtime_access,&runtime_controls,&runtime_collections,
                    &runtime_predicates,why,sizeof(why)):
                s->event_payloads_enabled?
                gsc_runtime_new_with_event_payloads(s->program,session_fog,s,
                    &runtime_entities,&runtime_access,&runtime_controls,&runtime_collections,why,sizeof(why)):
                s->results_enabled?
                gsc_runtime_new_with_results(s->program,session_fog,s,
                    &runtime_entities,&runtime_access,&runtime_controls,&runtime_collections,why,sizeof(why)):
                s->arrays_enabled?
                gsc_runtime_new_with_arrays(s->program,session_fog,s,
                    &runtime_entities,&runtime_access,&runtime_controls,&runtime_collections,why,sizeof(why)):
                s->flow_enabled?
                gsc_runtime_new_with_flow(s->program,session_fog,s,
                    &runtime_entities,&runtime_access,&runtime_controls,why,sizeof(why)):
                s->controls_enabled?
                gsc_runtime_new_with_controls(s->program,session_fog,s,
                    &runtime_entities,&runtime_access,&runtime_controls,why,sizeof(why)):
                s->state_enabled?
                gsc_runtime_new_with_state(s->program,session_fog,s,
                    &runtime_entities,&runtime_access,why,sizeof(why)):
                s->events_enabled?
                gsc_runtime_new_with_events(s->program,session_fog,s,
                    &runtime_entities,&runtime_access,why,sizeof(why)):
                gsc_runtime_new_with_access(s->program,session_fog,s,
                    &runtime_entities,&runtime_access,why,sizeof(why));
        } else s->runtime=gsc_runtime_new_with_entities(s->program,session_fog,s,
            &runtime_entities,why,sizeof(why));
    } else s->runtime = gsc_runtime_new(s->program, session_fog, s, why, sizeof(why));
    if (!s->runtime) return session_failed(s, why[0] ? why : "GSC runtime preflight failed", err, cap);
    if(promote)s->bsp_runtime=NULL; /* Sole ownership moves without a callback. */
    if(s->turret_program_enabled&&(!session_animation_resources_current(s)||
       !gsc_runtime_admit_developer_script(s->runtime,&s->developer)||!session_animation_resources_current(s)))
        return session_failed(s,"developer runtime admission refused",err,cap);
    if(s->r237_frame_buckets_enabled&&!gsc_runtime_select_r237_frame_buckets(s->runtime))
        return session_failed(s,"R237 frame runtime selection failed",err,cap);
    if(s->r243_thread_start_enabled&&!gsc_runtime_select_r243_thread_start(s->runtime))return session_failed(s,"R243 thread start runtime selector refused",err,cap);
    if(s->configured_precache_interval_enabled&&!gsc_runtime_select_configured_precache_interval(s->runtime))return session_failed(s,"configured precache runtime selector refused",err,cap);
    if(s->r243_same_bucket_wait_enabled&&!gsc_runtime_select_r243_same_bucket_wait(s->runtime))return session_failed(s,"R243 same-bucket wait runtime selector refused",err,cap);
    if(s->r243_thread_entities_enabled){GscThreadEntityAuthorityHost host={s,session_r243_thread_resolve};if(!gsc_runtime_select_r243_thread_entities(s->runtime,&host))return session_failed(s,"R243 thread entities runtime selector refused",err,cap);}
    if(s->sparse_numeric_arrays_enabled&&!gsc_runtime_select_sparse_numeric_arrays(s->runtime))
        return session_failed(s,"sparse numeric array runtime selection failed",err,cap);
    if(s->r238_array_value_cow_enabled&&!gsc_runtime_select_r238_array_value_cow(s->runtime))return session_failed(s,"R238 array COW runtime selector refused",err,cap);
    if(s->r239_mixed_scalar_equality_enabled&&!gsc_runtime_select_r239_mixed_scalar_equality(s->runtime))return session_failed(s,"R239 mixed equality runtime selector refused",err,cap);
    if(s->owned_numeric_delete_enabled&&!gsc_runtime_select_r233_owned_numeric_delete(s->runtime))return session_failed(s,"owned numeric deletion runtime selector refused",err,cap);
    if(s->start_origin_enabled) {
        GscStartOriginHost query;query.user=s;query.query=session_start_origin_query;
        if(!session_animation_resources_current(s)||
           !gsc_runtime_select_anim_data(s->runtime,&s->start_origin_map)||
           !session_animation_resources_current(s)||
           !gsc_runtime_select_animation_references(s->runtime,s->start_origin_catalog,&s->start_origin_map)||
           !session_animation_resources_current(s)||
           !gsc_runtime_select_start_origin(s->runtime,&query)||
           !session_animation_resources_current(s))return session_failed(s,"R209 start-origin runtime selection failed",err,cap);
    }
    if(s->animation_namespace_enabled) {
        GscAnimationNamespaceSelection selection;int n;
        memset(&selection,0,sizeof(selection));
        for(n=0;n<3;n++)selection.catalogs[n]=s->animation_namespace_catalogs[n];
        selection.map_certificate=s->animation_namespace_map;
        if(!session_animation_resources_current(s)||
           !gsc_runtime_select_anim_data(s->runtime,&s->animation_namespace_map)||
           !session_animation_resources_current(s)||
           !(s->turret_animations_enabled?session_turret_animation_runtime(s):
             gsc_runtime_select_animation_namespace(s->runtime,&selection))||
           !session_animation_resources_current(s))
            return session_failed(s,"R211 animation namespace runtime selection failed",err,cap);
    }
    if(s->random_range_enabled&&(!session_animation_resources_current(s)||!gsc_runtime_select_r232_randomfloat_range(s->runtime)||!session_animation_resources_current(s)))return session_failed(s,"randomFloatRange runtime selection failed",err,cap);
    if(s->r232_source_float_enabled&&(!session_animation_resources_current(s)||!gsc_runtime_select_r232_source_float_arithmetic(s->runtime)||!session_animation_resources_current(s)))return session_failed(s,"init/look arithmetic runtime selection failed",err,cap);
    if(s->weapon_classname_enabled){GscWeaponClassnameHost host={s,session_weapon_classname};if(!gsc_runtime_select_weapon_classname(s->runtime,&host))return session_failed(s,"weapon classname runtime selection failed",err,cap);}
    if(s->weapon_model_enabled){GscWeaponModelHost host={s,session_weapon_model_query};
        if(!session_animation_resources_current(s)||!gsc_runtime_select_weapon_model(s->runtime,&host)||!session_animation_resources_current(s))return session_failed(s,"weapon model runtime selection failed",err,cap);
    }
    if(s->namespace_length_enabled){GscAnimationLengthHost host={s,session_namespace_length_query};if(!session_animation_resources_current(s)||!gsc_runtime_select_r233_namespace_animation_length(s->runtime,&host)||!session_animation_resources_current(s))return session_failed(s,"namespace length runtime selection failed",err,cap);}
    if(s->actor_drop_enabled){GscActorDropHost host={s,session_actor_drop_invoke};if(!session_animation_resources_current(s)||!gsc_runtime_select_r233_actor_drop(s->runtime,&host)||!session_animation_resources_current(s))return session_failed(s,"DropWeapon runtime selection failed",err,cap);}
    if(s->actor_presence_enabled){GscActorPresenceHost host={s,session_actor_presence_read,session_actor_presence_current};
        if(!session_animation_resources_current(s)||!gsc_runtime_select_actor_presence(s->runtime,&host)||!session_animation_resources_current(s))return session_failed(s,"actor presence runtime selection failed",err,cap);
    }
    if(s->actor_look_enabled){GscActorLookHost host={s,session_actor_look_validate,session_actor_look_invoke};
        if(!session_animation_resources_current(s)||!gsc_runtime_select_r232_actor_look(s->runtime,&host)||!session_animation_resources_current(s))return session_failed(s,"actor look runtime selection failed",err,cap);
    }
    if(s->actor_animations_enabled) {
        GscActorAnimationHost host={s,session_actor_animation_validate,session_actor_animation_invoke};
        if(!session_animation_resources_current(s)||!gsc_runtime_select_r232_actor_animations(s->runtime,&host)||!session_animation_resources_current(s))
            return session_failed(s,"R232 actor animation runtime selection failed",err,cap);
    }
    if(s->flagged_all_restart_enabled&&(!session_animation_resources_current(s)||!gsc_runtime_select_r233_flagged_all_restart(s->runtime)||!session_animation_resources_current(s)))return session_failed(s,"flagged all restart runtime selection failed",err,cap);
    if(s->critical_selected){GscActorCriticalHost h={s,r243_critical_session_invoke};if(!session_animation_resources_current(s)||!gsc_runtime_select_actor_critical_methods(s->runtime,&h)||!session_animation_resources_current(s))return session_failed(s,"critical runtime selection refused",err,cap);}
    if(!r243_command_runtime_bind(s))return session_failed(s,"method12 runtime binding refused",err,cap);
    if(!r243_move_delta_runtime_bind(s))return session_failed(s,"GetMoveDelta runtime binding refused",err,cap);
    if(!r243_actor_float_runtime_bind(s))return session_failed(s,"N0087 runtime binding refused",err,cap);
    if(!r243_stance_runtime_bind(s))return session_failed(s,"R243 live stance runtime binding refused",err,cap);
    if(!r243_animtime_runtime_bind(s))return session_failed(s,"R243 live animtime runtime binding refused",err,cap);
    if(s->namespace_start_origin_enabled) {
        GscStartOriginHost query;query.user=s;query.query=session_namespace_start_origin_query;
        if(!session_animation_resources_current(s)||
           !gsc_runtime_select_namespace_start_origin(s->runtime,&query)||
           !session_animation_resources_current(s))
            return session_failed(s,"R212 namespace query runtime selection failed",err,cap);
    }
    if(s->grenade_items_enabled) {
        GscEntityFieldHost fields={s,session_grenade_item_classify,
            session_grenade_item_read,session_grenade_item_write};
        if(!gsc_runtime_select_entity_fields(s->runtime,&fields))
            return session_failed(s,"R216 grenade native field selection failed",err,cap);
    }
    if(s->yaw_motion_enabled) {
        GscYawMotionHost host={s,session_yaw_motion_invoke};
        if(!gsc_runtime_select_r216_yaw(s->runtime,&host))
            return session_failed(s,"R216 yaw runtime selection failed",err,cap);
    }
    if(s->actor_bootstrap_enabled){
        GscEntityFieldHost fields={s,session_actor_classify,session_actor_read,session_actor_write};
        GscEntityMethodHost methods={s,session_actor_method};
        GscSpawnerTeamMethodHost teams={s,session_actor_team_validate,session_actor_team_set};
        GscAttachmentQueryHost attachments={s,session_actor_attachment};
        if(!session_actor_map_current(s)||!gsc_runtime_select_entity_fields(s->runtime,&fields)||
           !gsc_runtime_select_entity_methods(s->runtime,&methods)||!gsc_runtime_select_spawner_team(s->runtime,&teams)||
           !gsc_runtime_select_game_array(s->runtime)||!gsc_runtime_select_attachment_queries(s->runtime,&attachments))return session_failed(s,"actor bootstrap runtime selection failed",err,cap);
    }
    /* Actor bootstrap already selects the same root. Do not select twice. */
    if(s->map_game_array_enabled&&!s->actor_bootstrap_enabled&&
       !gsc_runtime_select_game_array(s->runtime))
        return session_failed(s,"map game array runtime selection failed",err,cap);
    if(s->loop_sound_enabled) {
        GscLoopSoundHost sound;sound.user=s;sound.invoke=session_loop_sound;sound.finish=session_loop_sound_finish;
        if(!gsc_runtime_select_loop_sound(s->runtime,&sound))return session_failed(s,"loop sound runtime selection failed",err,cap);
    }
    if(s->script_sound_enabled) {
        GscScriptSoundHost sound;sound.user=s;sound.invoke=session_script_sound;
        if(!gsc_runtime_select_script_sound(s->runtime,&sound))return session_failed(s,"playsound runtime selection failed",err,cap);
        if(s->sound_notify_enabled){GscSoundNotifyHost h;h.user=s;h.invoke=session_sound_notify;
         if(!gsc_runtime_select_sound_notify(s->runtime,&h))return session_failed(s,"sound notify runtime selection failed",err,cap);}

    }
    if(s->execution_extensions) {
        GscPlayerNativeHost player;player.user=s;player.invoke=session_player_native;
        if(!gsc_runtime_select_owned_state_arena(s->runtime,GSC_STATE_FIELD_OWNED_LIMIT)||
           !gsc_runtime_select_vector_arithmetic(s->runtime)||
           !gsc_runtime_select_integer_bitwise(s->runtime)||
           !gsc_runtime_select_string_int_concat(s->runtime)||
           !gsc_runtime_select_vector_math(s->runtime)||
           !gsc_runtime_select_endon(s->runtime)||
           !gsc_runtime_select_waittillmatch(s->runtime)||
           !gsc_runtime_select_console_int32_display(s->runtime)||
           !gsc_runtime_select_player_natives(s->runtime,&player))
            return session_failed(s,"R198 execution runtime selection failed",err,cap);
    }
    if(s->entity_operations_enabled) {
        GscEntityDeleteHost deletion;deletion.user=s;deletion.invoke=session_entity_delete;
        if(!gsc_runtime_select_entity_operations(s->runtime,&deletion)||
           !gsc_runtime_select_entity_coordinates(s->runtime))
            return session_failed(s,"R198 entity operations runtime selection failed",err,cap);
    }
    if(s->origin_method_enabled){GscEntityOriginMethodHost host={s,session_origin_method_read};
        if(!gsc_runtime_select_origin_method(s->runtime,&host))return session_failed(s,"GetOrigin method runtime selection failed",err,cap);}
    if(s->entity_links_enabled){GscEntityLinkHost links={s,session_entity_link};if(!gsc_runtime_select_entity_links(s->runtime,&links))return session_failed(s,"entity links runtime selection failed",err,cap);}
    if(s->delete_transactions_enabled) {
        GscDeleteTransactionHost deletion={s,session_delete_transaction};
        if(!gsc_runtime_select_delete_transactions(s->runtime,&deletion))
            return session_failed(s,"R204 delete transaction runtime selection failed",err,cap);
    }
    if(s->owned_wait_delivery_enabled&&!gsc_runtime_select_owned_wait_delivery(s->runtime))
        return session_failed(s,"native wait delivery runtime selection failed",err,cap);
    if(s->loop_fx_enabled) {
        GscLoopFxHost loops={s,session_loop_fx_prepare,session_loop_fx_finish};
        if(!gsc_runtime_select_loop_fx(s->runtime,&loops))return session_failed(s,"loop FX runtime selection failed",err,cap);
    }
    if(s->spatial_natives_enabled) {
        GscSpatialNativeHost spatial;spatial.user=s;spatial.invoke=session_spatial_native;
        if(!gsc_runtime_select_spatial_natives(s->runtime,&spatial))
            return session_failed(s,"R198 spatial natives runtime selection failed",err,cap);
    }
    if(s->ai_arrays_enabled) {
        GscAiArrayHost ai;memset(&ai,0,sizeof(ai));ai.user=s;ai.query=session_ai_array_query;ai.validate=session_ai_array_validate;
        if(!gsc_runtime_select_ai_arrays(s->runtime,&ai))return session_failed(s,"R202 AI array runtime selection failed",err,cap);
    }
    if(s->node_queries_enabled) {
        GscNodeQueryHost nodes;memset(&nodes,0,sizeof(nodes));nodes.user=s;
        nodes.query=session_node_query;nodes.validate=session_node_validate;nodes.read=session_node_read;
        if(!gsc_runtime_select_node_queries(s->runtime,&nodes))
            return session_failed(s,"R199 node query runtime selection failed",err,cap);
    }
    if(s->turrets_enabled){
        GscTurretHost host={s,session_turret_invoke};
        if(!gsc_runtime_select_r232_turrets(s->runtime,&host))return session_failed(s,"R232 turret runtime selection failed",err,cap);
    }
    if(s->actor_goal_enabled){
        GscActorGoalHost goal={s,session_actor_goal_invoke};
        if(!gsc_runtime_select_r222_actor_goal(s->runtime,&goal))return session_failed(s,"Actor goal runtime selection failed",err,cap);
    }
    if(s->hud_elements_enabled){
        GscHudElemHost h;memset(&h,0,sizeof(h));h.user=s;h.invoke=session_hud_element_invoke;h.validate=session_hud_element_validate;h.retire=session_hud_element_retire;
        if(!gsc_runtime_select_hud_elements(s->runtime,&h))return session_failed(s,"R200 HUD runtime selection failed",err,cap);
    }
    if(s->entity_collision_enabled){GscEntityCollisionHost h;h.user=s;h.notsolid=session_entity_notsolid;
        if(!gsc_runtime_select_entity_collision(s->runtime,&h))return session_failed(s,"R200 collision runtime selection failed",err,cap);
    }
    if(!session_animation_resources_current(s))return session_failed(s,"start-origin resources retired before prepare",err,cap);
    if(s->r243_script_state_enabled){
        GscActorScriptStateHost tracking={s,session_script_state_invoke};
        if(!gsc_runtime_select_r243_script_state(s->runtime,&tracking))
            return session_failed(s,"selected Actor tracking runtime binding refused",err,cap);
    }
    if(s->r243_scripted_prefix_enabled){
        GscActorScriptedPrefixHost prefix={s,session_scripted_prefix_invoke};
        if(!gsc_runtime_select_r243_scripted_prefix(s->runtime,&prefix))
            return session_failed(s,"selected native scripted prefix runtime binding refused",err,cap);
    }
    if(s->r243_scripted_start_enabled){
        GscActorScriptedStartHost start={s,session_scripted_start_invoke};
        if(!gsc_runtime_select_r243_scripted_start(s->runtime,&start))
            return session_failed(s,"normal scripted start runtime binding refused",err,cap);
    }
    if(s->r243_controller_enabled){
        GscActorScriptedControllerHost h={s,session_controller_init,session_controller_select,session_controller_step,session_controller_update};
        if(!gsc_runtime_select_r243_controller_with_cleanup(s->runtime,&h,s->r243_select_with_cleanup?session_controller_select_with_cleanup:NULL))
            return session_failed(s,"scripted controller runtime binding refused",err,cap);
    }
    if(s->r243_scripted_init_dependency_enabled&&
       !gsc_runtime_r243_scripted_init_available(s->runtime))
        return session_failed(s,"original scripted::init whole-function runtime admission unavailable",err,cap);
    if(!session_node_scripts_publish(s))return session_failed(s,"node script compiled cache unavailable or map retired",err,cap);
    if(!session_script_roots_current(s)||!session_script_roots_cache_current(s))return session_failed(s,"global/map program retired during runtime preparation",err,cap);
    if(promote){
        if(!session_actor_map_current(s)||!session_actor_program_current(s)||!session_actor_cache_current(s)||
           !gsc_runtime_commit_bsp_promotion(s->runtime))return session_failed(s,"BSP promotion final owner/commit refused",err,cap);
        s->bsp_promoted=1;
    }
    session_capture(s); s->view.state = GSC_SESSION_PREPARED;
    s->busy = 0; return 1;
}
/* Compatibility transaction: callers still receive an ID only after both
 * stages succeed. Explicit users may retain COMPILED without a runtime. */
int gsc_session_load(GscSession*s,const char*path,int ready,uint64_t*out_id,char*err,size_t cap){
 uint64_t id=0;if(out_id)*out_id=0;
 if(!gsc_session_prepare_program(s,path,ready,out_id?&id:NULL,err,cap))return 0;
 if(!gsc_session_prepare_runtime(s,id,err,cap))return 0;
 *out_id=id;return 1;
}
static int session_after_vm(GscSession *s, int ok, char *err, size_t cap) {
    if (!ok || s->abort_requested || gsc_runtime_error(s->runtime)[0]) {
        const char *why = gsc_runtime_error(s->runtime);
        return session_failed(s, why[0] ? why : "GSC execution failed", err, cap);
    }
    if(!session_animation_resources_current(s))
        return session_failed(s,"start-origin resources retired during execution",err,cap);
    session_capture(s);
    s->view.state = gsc_active_threads(s->runtime) ? GSC_SESSION_RUNNING : GSC_SESSION_COMPLETED;
    s->busy = 0; return 1;
}
#include "gsc_session_r232_turret_entry.inc"
int gsc_session_start(GscSession *s, uint64_t id, int64_t now, char *err, size_t cap) {
    int ok;
    if (!session_enter(s, err, cap)) return 0;
    if (!session_id_matches(s, id)) return session_leave_rejected(s, "stale session ID", err, cap);
    if (s->view.state != GSC_SESSION_PREPARED)
        return session_leave_rejected(s, "session not prepared", err, cap);
    if (now < 0) return session_failed(s, "negative host time", err, cap);
    if(s->actor_bootstrap_enabled&&s->actor_bootstrap.construct&&
       (!s->actor_construct_complete||s->actor_construct_count!=s->actor_bootstrap.placed_count||
        s->actor_construct_program!=gsc_runtime_program_identity(s->runtime)))
        return session_failed(s,"explicit actor construction phase is incomplete or stale",err,cap);
    if(!session_script_roots_current(s)||!session_script_roots_cache_current(s))return session_failed(s,"global/map declarations retired before start",err,cap);
    if(!session_animation_resources_current(s))return session_failed(s,"start-origin resources retired before start",err,cap);
    s->view.origin_host_ms = s->view.last_host_ms = now; s->view.logical_ms = 0;
    s->view.state = GSC_SESSION_RUNNING;
    s->shellshock_initial_window=s->shellshock_enabled;
    if(s->actor_bootstrap_enabled){char why[GSC_SESSION_ERROR_CAP];why[0]=0;
        if(!session_actor_prelude(s,why,sizeof(why))){s->shellshock_initial_window=0;return session_failed(s,why[0]?why:"actor bootstrap prelude refused",err,cap);}
        ok=gsc_start_configured_main_after_bootstrap(s->runtime,&s->actor_bootstrap.map_certificate);
    }else ok=gsc_start(s->runtime);
    s->shellshock_initial_window=0;
    return session_after_vm(s,ok,err,cap);
}
int gsc_session_advance(GscSession *s, uint64_t id, int64_t now, char *err, size_t cap) {
    int64_t delta;
    if (!session_enter(s, err, cap)) return 0;
    if (!session_id_matches(s, id)) return session_leave_rejected(s, "stale session ID", err, cap);
    if(s->r237_frame_buckets_enabled)
        return session_leave_rejected(s,"R237 frame Session requires explicit frame phases",err,cap);
    if (s->view.state != GSC_SESSION_RUNNING && s->view.state != GSC_SESSION_COMPLETED)
        return session_leave_rejected(s, "session not started", err, cap);
    if (now < 0 || now < s->view.last_host_ms)
        return session_failed(s, "negative or backward host time", err, cap);
    if(!session_animation_resources_current(s))return session_failed(s,"start-origin resources retired before advance",err,cap);
    /* Both now and origin are nonnegative and now>=origin; no signed overflow. */
    delta = now - s->view.origin_host_ms;
    if (delta > INT_MAX) return session_failed(s, "VM elapsed time exceeds INT_MAX", err, cap);
    s->view.last_host_ms = now; s->view.logical_ms = (int)delta;
    return session_after_vm(s, gsc_advance(s->runtime, (int)delta), err, cap);
}
int gsc_session_notify(GscSession *s, uint64_t id, const char *name, char *err, size_t cap) {
    size_t n = 0;
    if (!session_enter(s, err, cap)) return 0;
    if (!session_id_matches(s, id)) return session_leave_rejected(s, "stale session ID", err, cap);
    if (s->view.state != GSC_SESSION_RUNNING)
        return session_leave_rejected(s, "session not running", err, cap);
    if (name) while (n < 128 && name[n]) ++n;
    if (!name || !n || n == 128) return session_leave_rejected(s, "invalid level event name", err, cap);
    if (!gsc_notify(s->runtime, name)) return session_failed(s, "level notify failed", err, cap);
    s->busy = 0; return 1;
}
int gsc_session_notify_entity(GscSession *s,uint64_t id,const GscEntityRef *owner,
    const char *name,int *out_awakened,char *err,size_t cap) {
    size_t n=0;int ok;
    if(out_awakened)*out_awakened=0;
    if(!session_enter(s,err,cap))return 0;
    if(!session_id_matches(s,id))return session_leave_rejected(s,"stale session ID",err,cap);
    if(!s->events_enabled)return session_leave_rejected(s,"entity events are not enabled",err,cap);
    if(s->view.state!=GSC_SESSION_RUNNING)
        return session_leave_rejected(s,"session not running",err,cap);
    if(name)while(n<128&&name[n])++n;
    if(!out_awakened||!owner||!name||!n||n==128)
        return session_leave_rejected(s,"invalid entity event request",err,cap);
    ok=gsc_notify_entity(s->runtime,owner,name,out_awakened);
    if(s->abort_requested||gsc_runtime_error(s->runtime)[0]) {
        const char *why=gsc_runtime_error(s->runtime);
        *out_awakened=0;
        return session_failed(s,why[0]?why:"entity notify cancelled",err,cap);
    }
    if(!ok)return session_leave_rejected(s,"invalid or stale entity event owner",err,cap);
    s->busy=0;return 1;
}
static int session_notify_payload(GscSession *s,uint64_t id,const GscEntityRef *owner,
    const char *name,const GscEventValue *values,size_t count,int *out_awakened,
    int entity,char *err,size_t cap) {
    int ok;
    if(out_awakened)*out_awakened=0;
    if(!session_enter(s,err,cap))return 0;
    if(!session_id_matches(s,id))return session_leave_rejected(s,"stale session ID",err,cap);
    if(!s->event_payloads_enabled)return session_leave_rejected(s,"event payloads are not enabled",err,cap);
    if(s->view.state!=GSC_SESSION_RUNNING)return session_leave_rejected(s,"session not running",err,cap);
    if(!out_awakened)return session_leave_rejected(s,"event awakened output required",err,cap);
    ok=entity?gsc_notify_entity_with_payload(s->runtime,owner,name,values,count,out_awakened):
              gsc_notify_with_payload(s->runtime,name,values,count,out_awakened);
    if(s->abort_requested||gsc_runtime_error(s->runtime)[0]) {
        const char *why=gsc_runtime_error(s->runtime);*out_awakened=0;
        return session_failed(s,why[0]?why:"payload notify cancelled",err,cap);
    }
    if(!ok)return session_leave_rejected(s,"invalid or stale event payload request",err,cap);
    s->busy=0;return 1;
}
int gsc_session_notify_with_payload(GscSession *s,uint64_t id,const char *name,
    const GscEventValue *values,size_t count,int *out_awakened,char *err,size_t cap) {
    return session_notify_payload(s,id,NULL,name,values,count,out_awakened,0,err,cap);
}
int gsc_session_notify_entity_with_payload(GscSession *s,uint64_t id,const GscEntityRef *owner,
    const char *name,const GscEventValue *values,size_t count,int *out_awakened,char *err,size_t cap) {
    return session_notify_payload(s,id,owner,name,values,count,out_awakened,1,err,cap);
}
/* The old public notify APIs continue to require RUNNING. This separately
 * selected combat lane may validate and notify a started, completed blast VM
 * with no recipients. gsc_active_threads includes READY/SLEEP/EVENT tasks, so
 * zero proves zero blocked/waiting tasks too. It neither restarts nor executes
 * a scheduler; the genuine runtime notifier still validates copied payload,
 * entity generation/epoch and the event name. */
static int session_blast_notify(GscSession *s,uint64_t id,const GscEntityRef *owner,
    const char *name,const GscEventValue *values,size_t count,int *out_awakened,
    const GscBlastWaiterSet *selected,char *err,size_t cap) {
    int ok,completed,attacker;size_t length=0;char event[128];GscEventValue copy[2];
    if(out_awakened)*out_awakened=0;
    if(!session_enter(s,err,cap))return 0;
    if(!session_id_matches(s,id))return session_leave_rejected(s,"stale session ID",err,cap);
    if(!s->blast_enabled||!s->event_payloads_enabled)
        return session_leave_rejected(s,"blast combat events are not enabled",err,cap);
    completed=s->view.state==GSC_SESSION_COMPLETED;
    if(s->view.state!=GSC_SESSION_RUNNING&&!completed)
        return session_leave_rejected(s,"blast session not started",err,cap);
    if(!s->runtime||(completed&&gsc_active_threads(s->runtime)))
        return session_leave_rejected(s,"completed blast session has active tasks",err,cap);
    if(!out_awakened)return session_leave_rejected(s,"event awakened output required",err,cap);
    if(name)while(length<sizeof(event)&&name[length])length++;
    if(!name||!length||length==sizeof(event)||!values||count<1||count>2)
        return session_leave_rejected(s,"invalid blast combat event",err,cap);
    memcpy(event,name,length+1);memcpy(copy,values,count*sizeof(copy[0]));
    if(!strcmp(event,"damage")) {
        if(count!=2||copy[0].kind!=GSC_EVENT_NUMBER||!isfinite(copy[0].number))
            return session_leave_rejected(s,"invalid blast damage payload",err,cap);
        attacker=1;
    } else if(!strcmp(event,"death")) {
        if(count!=1)return session_leave_rejected(s,"invalid blast death payload",err,cap);
        attacker=0;
    } else return session_leave_rejected(s,"unsupported blast combat event",err,cap);
    if(copy[attacker].kind!=GSC_EVENT_UNDEFINED&&copy[attacker].kind!=GSC_EVENT_ENTITY)
        return session_leave_rejected(s,"invalid blast attacker payload",err,cap);
    s->blast_notify_completed=completed;
    ok=selected?gsc_blast_notify_captured(s->runtime,selected,copy,count,out_awakened):
                gsc_notify_entity_with_payload(s->runtime,owner,event,copy,count,out_awakened);
    s->blast_notify_completed=0;
    if(s->abort_requested||gsc_runtime_error(s->runtime)[0]) {
        const char *why=gsc_runtime_error(s->runtime);*out_awakened=0;
        return session_failed(s,why[0]?why:"blast payload notify cancelled",err,cap);
    }
    if(!ok)return session_leave_rejected(s,"invalid or stale blast event payload request",err,cap);
    if(completed&&(*out_awakened||gsc_active_threads(s->runtime))) {
        *out_awakened=0;
        return session_failed(s,"completed blast notify unexpectedly awakened tasks",err,cap);
    }
    s->busy=0;return 1;
}
int gsc_session_blast_notify_entity_with_payload(GscSession *s,uint64_t id,const GscEntityRef *owner,
    const char *name,const GscEventValue *values,size_t count,int *out_awakened,char *err,size_t cap) {
    return session_blast_notify(s,id,owner,name,values,count,out_awakened,NULL,err,cap);
}
int gsc_session_blast_capture_waiters(const GscSession *s,uint64_t id,const GscEntityRef *owner,
    const char *name,GscBlastWaiterSet *out,char *err,size_t cap) {
    if(!s||s->abort_requested||!s->blast_enabled||!s->event_payloads_enabled||
       !session_id_matches(s,id)||!s->runtime||
       (s->view.state!=GSC_SESSION_RUNNING&&s->view.state!=GSC_SESSION_COMPLETED)||
       (s->view.state==GSC_SESSION_COMPLETED&&gsc_active_threads(s->runtime))) {
        if(out)memset(out,0,sizeof(*out));
        return session_reject(err,cap,"invalid blast session waiter capture");
    }
    return gsc_blast_capture_waiters(s->runtime,owner,name,out,err,cap);
}
int gsc_session_blast_notify_captured(GscSession *s,uint64_t id,const GscBlastWaiterSet *selected,
    const GscEventValue *values,size_t count,int *out_awakened,char *err,size_t cap) {
    GscBlastWaiterSet copy;
    if(out_awakened)*out_awakened=0;
    if(!selected)return session_reject(err,cap,"blast waiter set required");
    copy=*selected;
    return session_blast_notify(s,id,&copy.owner,copy.event,values,count,out_awakened,&copy,err,cap);
}
int gsc_session_unload(GscSession *s, uint64_t id, char *err, size_t cap) {
    if (!session_enter(s, err, cap)) return 0;
    if (!session_id_matches(s, id)) return session_leave_rejected(s, "stale session ID", err, cap);
    if(!session_drop(s)){s->view.state=GSC_SESSION_FAILED;s->busy=0;return session_reject(err,cap,s->view.error);}
    session_clear(s);
    if (!session_new_id(s)) return session_failed(s, "session ID exhausted", err, cap);
    s->busy = 0; return 1;
}
uint64_t gsc_session_prepared_program_identity(const GscSession *s,uint64_t id) {
    if(!s||!id||s->abort_requested||s->view.session_id!=id||s->view.error[0]||
       (s->view.state!=GSC_SESSION_COMPILED&&s->view.state!=GSC_SESSION_PREPARED))return 0;
    return gsc_program_identity(s->program);
}
/* The gameplay host reads these fields at callback boundaries. No thread
 * count or native statistics are needed to check the live session. */
int gsc_session_is_running(const GscSession *s, uint64_t id) {
    return s && s->view.session_id == id && s->view.state == GSC_SESSION_RUNNING &&
        !s->view.error[0];
}
int gsc_session_running_at(const GscSession *s,uint64_t id,int logical,int host) {
    return gsc_session_is_running(s,id) && logical>=0 && host>=logical &&
        s->view.logical_ms==logical && s->view.origin_host_ms==(int64_t)host-logical;
}
int gsc_session_snapshot(const GscSession *s, GscSessionSnapshot *out) {
    if (!s || !out) return 0;
    *out = s->view; out->busy = s->busy;
    out->has_program = s->program != NULL; out->has_runtime = s->runtime != NULL || s->bsp_runtime != NULL;
    out->active_threads = gsc_active_threads(s->runtime)+gsc_active_threads(s->bsp_runtime);
    if (s->runtime) {
        (void)gsc_runtime_native_stats(s->runtime, &out->native);
        (void)gsc_runtime_fog_native_stats(s->runtime, &out->fog);
    }
    return 1;
}

/* R136 matching compiler/runtime selector; existing callbacks are inherited. */
GscSession *gsc_session_create_with_computed_randomint(const GscSessionHost *host,
    const GscEntityHost *entities,const GscEntityAccessHost *access,
    const GscEntityControlHost *controls,const GscEntityCollectionHost *collections,
    const GscEntityPredicateHost *predicates,const GscActorSpawnHost *spawners,
    const GscSessionMusicHost *music,const GscSessionResourceHost *resources,
    const GscSessionObjectiveHost *objectives,const GscSessionConsoleHost *console,
    const GscSessionHudHost *hud,const GscSessionMissionHost *mission,const GscSessionBlastHost *blast,const GscSessionQuakeHost *quake,const GscSessionFxHost *fx,const GscSessionCvarHost *cvars,const GscSessionShellshockHost *shellshock,const GscSessionDebugTextHost *debug_text,const GscSessionDebugLineHost *debug_line,const GscSessionAmbientHost *ambient,const GscSessionPrecacheItemHost *precache_item,const GscSessionSaveGameHost *save_game,const GscSessionTagFxHost *tag_fx,const GscSessionNativeQueryHost *native_queries,const GscSessionPrecacheStringHost *precache_string,const GscSessionSpawnerArrayHost *spawner_arrays,const GscSessionRandomIntHost *randomint,const GscSessionRandomFloatHost *randomfloat,const GscSessionKeyBindingHost *keybinding,char *err,size_t cap)  {
    GscSession *s=gsc_session_create_with_keybinding(host,entities,access,controls,collections,predicates,spawners,music,resources,objectives,console,hud,mission,blast,quake,fx,cvars,shellshock,debug_text,debug_line,ambient,precache_item,save_game,tag_fx,native_queries,precache_string,spawner_arrays,randomint,randomfloat,keybinding,err,cap);
    if(s)s->computed_randomint_enabled=1;
    return s;
}

/* R148 per-runtime structure objects and cached difficulty query selector. */
GscSession *gsc_session_create_with_structs(const GscSessionHost *host,
    const GscEntityHost *entities,const GscEntityAccessHost *access,
    const GscEntityControlHost *controls,const GscEntityCollectionHost *collections,
    const GscEntityPredicateHost *predicates,const GscActorSpawnHost *spawners,
    const GscSessionMusicHost *music,const GscSessionResourceHost *resources,
    const GscSessionObjectiveHost *objectives,const GscSessionConsoleHost *console,
    const GscSessionHudHost *hud,const GscSessionMissionHost *mission,const GscSessionBlastHost *blast,const GscSessionQuakeHost *quake,const GscSessionFxHost *fx,const GscSessionCvarHost *cvars,const GscSessionShellshockHost *shellshock,const GscSessionDebugTextHost *debug_text,const GscSessionDebugLineHost *debug_line,const GscSessionAmbientHost *ambient,const GscSessionPrecacheItemHost *precache_item,const GscSessionSaveGameHost *save_game,const GscSessionTagFxHost *tag_fx,const GscSessionNativeQueryHost *native_queries,const GscSessionPrecacheStringHost *precache_string,const GscSessionSpawnerArrayHost *spawner_arrays,const GscSessionRandomIntHost *randomint,const GscSessionRandomFloatHost *randomfloat,const GscSessionKeyBindingHost *keybinding,char *err,size_t cap)  {
    GscSession *s=gsc_session_create_with_computed_randomint(host,entities,access,controls,collections,predicates,spawners,music,resources,objectives,console,hud,mission,blast,quake,fx,cvars,shellshock,debug_text,debug_line,ambient,precache_item,save_game,tag_fx,native_queries,precache_string,spawner_arrays,randomint,randomfloat,keybinding,err,cap);
    if(s)s->structs_enabled=1;
    return s;
}

/* R149 selected entity queries, same earlier session capabilities. */
GscSession *gsc_session_create_with_entity_queries(const GscSessionHost *host,
    const GscEntityHost *entities,const GscEntityAccessHost *access,
    const GscEntityControlHost *controls,const GscEntityCollectionHost *collections,
    const GscEntityPredicateHost *predicates,const GscActorSpawnHost *spawners,
    const GscSessionMusicHost *music,const GscSessionResourceHost *resources,
    const GscSessionObjectiveHost *objectives,const GscSessionConsoleHost *console,
    const GscSessionHudHost *hud,const GscSessionMissionHost *mission,const GscSessionBlastHost *blast,const GscSessionQuakeHost *quake,const GscSessionFxHost *fx,const GscSessionCvarHost *cvars,const GscSessionShellshockHost *shellshock,const GscSessionDebugTextHost *debug_text,const GscSessionDebugLineHost *debug_line,const GscSessionAmbientHost *ambient,const GscSessionPrecacheItemHost *precache_item,const GscSessionSaveGameHost *save_game,const GscSessionTagFxHost *tag_fx,const GscSessionNativeQueryHost *native_queries,const GscSessionPrecacheStringHost *precache_string,const GscSessionSpawnerArrayHost *spawner_arrays,const GscSessionRandomIntHost *randomint,const GscSessionRandomFloatHost *randomfloat,const GscSessionKeyBindingHost *keybinding,const GscSessionEntityQueryHost *entity_queries,char *err,size_t cap)  {
    GscSession *s=gsc_session_create_with_structs(host,entities,access,controls,collections,predicates,spawners,music,resources,objectives,console,hud,mission,blast,quake,fx,cvars,shellshock,debug_text,debug_line,ambient,precache_item,save_game,tag_fx,native_queries,precache_string,spawner_arrays,randomint,randomfloat,keybinding,err,cap);
    if(s){s->entity_queries_enabled=1;if(entity_queries)s->entity_queries=*entity_queries;}
    return s;
}

/* R150 selected dynamic integer argument validation. */
GscSession *gsc_session_create_with_dynamic_randomint(const GscSessionHost *host,
    const GscEntityHost *entities,const GscEntityAccessHost *access,
    const GscEntityControlHost *controls,const GscEntityCollectionHost *collections,
    const GscEntityPredicateHost *predicates,const GscActorSpawnHost *spawners,
    const GscSessionMusicHost *music,const GscSessionResourceHost *resources,
    const GscSessionObjectiveHost *objectives,const GscSessionConsoleHost *console,
    const GscSessionHudHost *hud,const GscSessionMissionHost *mission,const GscSessionBlastHost *blast,const GscSessionQuakeHost *quake,const GscSessionFxHost *fx,const GscSessionCvarHost *cvars,const GscSessionShellshockHost *shellshock,const GscSessionDebugTextHost *debug_text,const GscSessionDebugLineHost *debug_line,const GscSessionAmbientHost *ambient,const GscSessionPrecacheItemHost *precache_item,const GscSessionSaveGameHost *save_game,const GscSessionTagFxHost *tag_fx,const GscSessionNativeQueryHost *native_queries,const GscSessionPrecacheStringHost *precache_string,const GscSessionSpawnerArrayHost *spawner_arrays,const GscSessionRandomIntHost *randomint,const GscSessionRandomFloatHost *randomfloat,const GscSessionKeyBindingHost *keybinding,const GscSessionEntityQueryHost *entity_queries,char *err,size_t cap)  {
    GscSession *s=gsc_session_create_with_entity_queries(host,entities,access,controls,collections,predicates,spawners,music,resources,objectives,console,hud,mission,blast,quake,fx,cvars,shellshock,debug_text,debug_line,ambient,precache_item,save_game,tag_fx,native_queries,precache_string,spawner_arrays,randomint,randomfloat,keybinding,entity_queries,err,cap);
    if(s)s->dynamic_randomint_enabled=1;
    return s;
}

/* R151 selected original FLOAT argument conversion. */
GscSession *gsc_session_create_with_numeric_randomint(const GscSessionHost *host,
    const GscEntityHost *entities,const GscEntityAccessHost *access,
    const GscEntityControlHost *controls,const GscEntityCollectionHost *collections,
    const GscEntityPredicateHost *predicates,const GscActorSpawnHost *spawners,
    const GscSessionMusicHost *music,const GscSessionResourceHost *resources,
    const GscSessionObjectiveHost *objectives,const GscSessionConsoleHost *console,
    const GscSessionHudHost *hud,const GscSessionMissionHost *mission,const GscSessionBlastHost *blast,const GscSessionQuakeHost *quake,const GscSessionFxHost *fx,const GscSessionCvarHost *cvars,const GscSessionShellshockHost *shellshock,const GscSessionDebugTextHost *debug_text,const GscSessionDebugLineHost *debug_line,const GscSessionAmbientHost *ambient,const GscSessionPrecacheItemHost *precache_item,const GscSessionSaveGameHost *save_game,const GscSessionTagFxHost *tag_fx,const GscSessionNativeQueryHost *native_queries,const GscSessionPrecacheStringHost *precache_string,const GscSessionSpawnerArrayHost *spawner_arrays,const GscSessionRandomIntHost *randomint,const GscSessionRandomFloatHost *randomfloat,const GscSessionKeyBindingHost *keybinding,const GscSessionEntityQueryHost *entity_queries,char *err,size_t cap)  {
    GscSession *s=gsc_session_create_with_dynamic_randomint(host,entities,access,controls,collections,predicates,spawners,music,resources,objectives,console,hud,mission,blast,quake,fx,cvars,shellshock,debug_text,debug_line,ambient,precache_item,save_game,tag_fx,native_queries,precache_string,spawner_arrays,randomint,randomfloat,keybinding,entity_queries,err,cap);
    if(s)s->numeric_randomint_enabled=1;
    return s;
}

int gsc_session_select_sparse_numeric_arrays(GscSession *s,char *error,size_t cap) {
    if((cap&&!error)||session_animation_namespace_private(s,error,cap))return 0;
    if(!session_enter(s,error,cap))return 0;
    if(s->view.state!=GSC_SESSION_EMPTY||s->program||s->runtime||!s->arrays_enabled||s->sparse_numeric_arrays_enabled)
        return session_leave_rejected(s,"sparse numeric arrays require an idle array-capable Session",error,cap);
    s->sparse_numeric_arrays_enabled=1;s->busy=0;return 1;
}

/* R152 selected exact-domain dynamic GetFloat arguments. */
GscSession *gsc_session_create_with_dynamic_randomfloat(const GscSessionHost *host,
    const GscEntityHost *entities,const GscEntityAccessHost *access,
    const GscEntityControlHost *controls,const GscEntityCollectionHost *collections,
    const GscEntityPredicateHost *predicates,const GscActorSpawnHost *spawners,
    const GscSessionMusicHost *music,const GscSessionResourceHost *resources,
    const GscSessionObjectiveHost *objectives,const GscSessionConsoleHost *console,
    const GscSessionHudHost *hud,const GscSessionMissionHost *mission,const GscSessionBlastHost *blast,const GscSessionQuakeHost *quake,const GscSessionFxHost *fx,const GscSessionCvarHost *cvars,const GscSessionShellshockHost *shellshock,const GscSessionDebugTextHost *debug_text,const GscSessionDebugLineHost *debug_line,const GscSessionAmbientHost *ambient,const GscSessionPrecacheItemHost *precache_item,const GscSessionSaveGameHost *save_game,const GscSessionTagFxHost *tag_fx,const GscSessionNativeQueryHost *native_queries,const GscSessionPrecacheStringHost *precache_string,const GscSessionSpawnerArrayHost *spawner_arrays,const GscSessionRandomIntHost *randomint,const GscSessionRandomFloatHost *randomfloat,const GscSessionKeyBindingHost *keybinding,const GscSessionEntityQueryHost *entity_queries,char *err,size_t cap)  {
    GscSession *s=gsc_session_create_with_numeric_randomint(host,entities,access,controls,collections,predicates,spawners,music,resources,objectives,console,hud,mission,blast,quake,fx,cvars,shellshock,debug_text,debug_line,ambient,precache_item,save_game,tag_fx,native_queries,precache_string,spawner_arrays,randomint,randomfloat,keybinding,entity_queries,err,cap);
    if(s)s->dynamic_randomfloat_enabled=1;
    return s;
}

int gsc_session_select_r233_native_animation_roots(GscSession*s,char*error,size_t cap){
 if((cap&&!error)||session_animation_namespace_private(s,error,cap))return 0;if(!s)return session_reject(error,cap,"native animation roots require Session");if(!session_enter(s,error,cap))return 0;
 if(s->view.state!=GSC_SESSION_EMPTY||s->program||s->runtime||!s->turret_animations_enabled||!s->function_admission_selected||s->native_animation_roots_enabled)return session_leave_rejected(s,"native roots require exclusive idle two-tree function-admission selection",error,cap);
 if(!session_animation_resources_current(s))return session_leave_rejected(s,"native root resources expired",error,cap);s->native_animation_roots_enabled=1;s->busy=0;return 1;
}

int gsc_session_select_r233_owned_numeric_delete(GscSession*s,char*error,size_t cap){
 if((cap&&!error)||session_animation_namespace_private(s,error,cap))return 0;
 if(!s)return session_reject(error,cap,"owned numeric deletion requires Session");if(!session_enter(s,error,cap))return 0;
 if(s->view.state!=GSC_SESSION_EMPTY||s->program||s->runtime||!s->arrays_enabled||!s->sparse_numeric_arrays_enabled||s->owned_numeric_delete_enabled)
  return session_leave_rejected(s,"owned numeric deletion requires idle selected sparse arrays",error,cap);
 s->owned_numeric_delete_enabled=1;s->busy=0;return 1;
}

#include "gsc_session_r233_actor_animation_end.inc"

#include "gsc_session_r237_frame_buckets.inc"
#include "gsc_session_owned_wait_delivery.inc"

#include "gsc_session_r238_array_value_cow.inc"
#include "gsc_session_r239_mixed_scalar_equality.inc"
#include "gsc_session_r243_thread_start.inc"
#include "gsc_session_r243_same_bucket_wait.inc"
#include "gsc_session_r243_scripted_init.inc"

int gsc_session_select_r243_flagged_all(GscSession*s,const GscActorAnimationHost*input,char*error,size_t cap){
 GscActorAnimationHost h;
 if(!s||!input||(cap&&!error)||session_animation_namespace_private(s,input,sizeof(*input))||session_animation_namespace_private(s,error,cap)||session_animation_namespace_overlap(input,sizeof(*input),error,cap))return 0;
 h=*input;if(!h.validate||!h.invoke||!session_enter(s,error,cap))return 0;
 if(s->view.state!=GSC_SESSION_EMPTY||s->program||s->runtime||!s->actor_animations_enabled||!s->native_animation_roots_enabled||!s->actor_animation_end_enabled||s->flagged_all_method_enabled)
 return session_leave_rejected(s,"method12 requires idle Actor/end/root configuration",error,cap);
 if(!session_animation_resources_current(s)||memcmp(input,&h,sizeof(h)))return session_leave_rejected(s,"method12 provider retired",error,cap);
 s->command_provider=h;s->flagged_all_method_enabled=1;s->busy=0;return 1;
}
static int r243_command_program_select(GscSession*s){return !s->flagged_all_method_enabled||R243ProgramSelectFlaggedAll(s->program);}
static int r243_command_runtime_bind(GscSession*s){return !s->flagged_all_method_enabled||(session_animation_resources_current(s)&&R243RuntimeSelectFlaggedAll(s->runtime)&&session_animation_resources_current(s));}

#include "gsc_session_node_scripts.inc"
#include "gsc_session_node_graph.inc"
#include "gsc_session_actor_graph.inc"
#include "gsc_session_actor_placed_init.inc"
#include "gsc_session_postcohort.inc"
