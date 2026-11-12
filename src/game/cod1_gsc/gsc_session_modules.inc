/* R95 graph acquisition: callbacks have the same lease contract as the old
 * single-file loader. Sources are copied and released before any parsing;
 * no pointer from a host lease is stored in the compiled program. */
#define GSC_MODULE_IDENTITY_CANONICAL_ONLY
#include "gsc_module_identity.inc"
#undef GSC_MODULE_IDENTITY_CANONICAL_ONLY

typedef struct {
    char (*modules)[GSC_SESSION_PATH_CAP];
    unsigned count, limit;
    size_t total;
    char error[GSC_SESSION_ERROR_CAP];
} SessionModuleGraph;

static int session_module_source_path(const char *path,char *module,char *out) {
    char stem[GSC_SESSION_PATH_CAP];size_t n=0;
    if(!path)return 0;
    while(n<GSC_SESSION_PATH_CAP&&path[n])++n;
    if(n<5||n>=GSC_SESSION_PATH_CAP||path[n-4]!='.'||
       (path[n-3]!='g'&&path[n-3]!='G')||
       (path[n-2]!='s'&&path[n-2]!='S')||
       (path[n-1]!='c'&&path[n-1]!='C'))return 0;
    memcpy(stem,path,n-4);stem[n-4]=0;
    if(!gsc_module_identity_canonical(stem,module,GSC_SESSION_PATH_CAP))return 0;
    memcpy(out,module,n-4);memcpy(out+n-4,".gsc",5);return 1;
}
static int session_graph_add(SessionModuleGraph *graph,const char *module) {
    char canonical[GSC_SESSION_PATH_CAP];unsigned n;size_t length;
    if(!gsc_module_identity_canonical(module,canonical,sizeof(canonical))) {
        session_message(graph->error,sizeof(graph->error),"invalid dependency module path");return 0;
    }
    length=strlen(canonical);
    if(length+4>=GSC_SESSION_PATH_CAP) {
        session_message(graph->error,sizeof(graph->error),"dependency source path too long");return 0;
    }
    for(n=0;n<graph->count;++n)if(!strcmp(graph->modules[n],canonical))return 1;
    if(graph->count>=graph->limit) {
        snprintf(graph->error,sizeof(graph->error),"module graph exceeds %u modules",graph->limit);return 0;
    }
    session_message(graph->modules[graph->count++],GSC_SESSION_PATH_CAP,canonical);
    return 1;
}
static int session_graph_dependency(void *user,const GscLinkAuditSite *site) {
    SessionModuleGraph *graph=user;
    /* requested_module is retained typed call/reference metadata. Local named
     * sites without a module add nothing; a self-qualified site is deduplicated.
     * String contents, comments and computed Values never create VFS reads. */
    return !site->requested_module[0]||session_graph_add(graph,site->requested_module);
}
static int session_load_modules_with_storage(GscSession *s,char (*storage)[GSC_SESSION_PATH_CAP],unsigned limit,char *error,size_t cap) {
    SessionModuleGraph graph;unsigned index;
    session_message(error,cap,"");memset(&graph,0,sizeof(graph));
    graph.modules=storage;graph.limit=limit;
    if(s->script_roots_enabled){unsigned n;char root[128];
        for(n=0;n<GSC_GLOBAL_SCRIPT_ROOT_COUNT;n++){
            snprintf(root,sizeof(root),"animscripts/%s",gsc_global_script_names[n]);
            if(!session_graph_add(&graph,root)){session_message(error,cap,graph.error);return 0;}
        }
        if(s->script_map_module[0]&&!session_graph_add(&graph,s->script_map_module)){session_message(error,cap,graph.error);return 0;}
    }
    if(!session_graph_add(&graph,s->view.module)) {
        session_message(error,cap,graph.error);return 0;
    }
    /* Native GAME36C70 owns an implicit scripted::init call. Acquire its
     * unchanged whole module through the same copied/released VFS graph BEFORE
     * any gameplay prefix. This selection does not admit animscripted/main. */
    if(s->r243_scripted_init_dependency_enabled&&
       !session_graph_add(&graph,"animscripts/scripted")) {
        session_message(error,cap,graph.error);return 0;
    }
    if(s->node_scripts_enabled){unsigned n;char module[128];for(n=0;n<s->node_script_selection.scripts.count;n++){
        snprintf(module,sizeof(module),"animscripts/traverse/%s",s->node_script_selection.scripts.names[n]);
        if(!session_graph_add(&graph,module)){session_message(error,cap,graph.error);return 0;}}}
    if(s->actor_program_enabled){unsigned n;for(n=0;n<s->actor_program.module_count;n++)
        if(!session_graph_add(&graph,s->actor_program.module_roots[n])){session_message(error,cap,graph.error);return 0;}}
    else if(s->actor_bootstrap_enabled){unsigned n;for(n=0;n<s->actor_bootstrap.module_count;n++)
        if(!session_graph_add(&graph,s->actor_bootstrap.module_roots[n])){session_message(error,cap,graph.error);return 0;}}
    s->program=s->dynamic_randomfloat_enabled?gsc_program_create_with_dynamic_randomfloat():s->numeric_randomint_enabled?gsc_program_create_with_numeric_randomint():s->dynamic_randomint_enabled?gsc_program_create_with_dynamic_randomint():s->entity_queries_enabled?gsc_program_create_with_entity_queries():s->structs_enabled?gsc_program_create_with_structs():s->computed_randomint_enabled?gsc_program_create_with_computed_randomint():s->keybinding_enabled?gsc_program_create_with_keybinding():s->randomfloat_enabled?gsc_program_create_with_randomfloat():s->randomint_enabled?gsc_program_create_with_randomint():s->spawner_arrays_enabled?gsc_program_create_with_spawner_arrays():s->precache_string_enabled?gsc_program_create_with_precache_string():s->native_queries_enabled?gsc_program_create_with_native_queries():s->tag_fx_enabled?gsc_program_create_with_tag_fx():s->save_game_enabled?gsc_program_create_with_save_game():s->precache_item_enabled?gsc_program_create_with_precache_item():s->ambient_enabled?gsc_program_create_with_ambient():s->debug_line_enabled?gsc_program_create_with_debug_line():s->debug_text_enabled?gsc_program_create_with_debug_text():s->shellshock_enabled?gsc_program_create_with_shellshock():s->cvars_enabled?gsc_program_create_with_cvars():s->fx_enabled?gsc_program_create_with_fx():s->quake_enabled?gsc_program_create_with_quake():s->blast_enabled?gsc_program_create_with_blast():s->mission_enabled?gsc_program_create_with_mission():s->hud_enabled?gsc_program_create_with_hud():s->console_enabled?gsc_program_create_with_console():s->objectives_enabled?gsc_program_create_with_objectives():s->resources_enabled?gsc_program_create_with_resources():s->music_enabled?gsc_program_create_with_music():s->names_enabled?gsc_program_create_with_names():s->lifetime_enabled?gsc_program_create_with_lifetime():s->scalar_switch_enabled?gsc_program_create_with_scalar_switch():s->function_refs_enabled?gsc_program_create_with_function_refs():s->actor_spawners_enabled?gsc_program_create_with_actor_spawners():s->predicates_enabled?gsc_program_create_with_predicates():s->event_payloads_enabled?gsc_program_create_with_event_payloads():
                                      gsc_program_create_with_modules();
    if(!s->program) {
        session_message(error,cap,"program allocation failed");return 0;
    }
    if(s->start_origin_enabled&&
       (!gsc_program_select_anim_data(s->program)||
        !gsc_program_select_animation_references(s->program)||
        !gsc_program_select_start_origin_query(s->program))) {
        session_message(error,cap,"R209 start-origin program selection failed");return 0;
    }
    if(s->animation_namespace_enabled&&
       (!gsc_program_select_anim_data(s->program)||
        !(s->turret_animations_enabled?gsc_program_select_r232_turret_animations(s->program):gsc_program_select_animation_namespace(s->program)))) {
        session_message(error,cap,"R211 animation namespace program selection failed");return 0;
    }
    if(s->training_rigs&&!gsc_program_select_training_rigs(s->program))return 0;
    if(!r243_move_delta_program_select(s)){session_message(error,cap,"GetMoveDelta program selection refused");return 0;}
    if(!r243_actor_float_program_select(s)){session_message(error,cap,"N0087 program selection refused");return 0;}
    if(!r243_stance_program_select(s)){session_message(error,cap,"R243 live stance program selection refused");return 0;}
    if(!r243_animtime_program_select(s)){session_message(error,cap,"R243 live animtime program selection refused");return 0;}
    if(s->namespace_start_origin_enabled&&
       !gsc_program_select_namespace_start_origin_query(s->program)) {
        session_message(error,cap,"R212 namespace query program selection failed");return 0;
    }
    if((s->actor_bootstrap_enabled||s->actor_program_enabled)&&(!gsc_program_select_entity_model_methods(s->program)||!gsc_program_select_spawner_team_method(s->program)||!gsc_program_select_attachment_queries(s->program))){session_message(error,cap,"actor bootstrap program selection failed");return 0;}
    if(s->execution_extensions&&
       (!gsc_program_select_large_sources(s->program)||
        !gsc_program_select_integer_bitwise(s->program)||
        !gsc_program_select_string_int_concat(s->program)||
        !gsc_program_select_vector_arithmetic(s->program)||
        !gsc_program_select_vector_math(s->program)||
        !gsc_program_select_endon(s->program)||
        !gsc_program_select_waittillmatch(s->program)||
        !gsc_program_select_console_int32_display(s->program)||
        !gsc_program_select_player_natives(s->program))) {
        session_message(error,cap,"R198 execution program selection failed");return 0;
    }
    if(s->player_normal_health_enabled&&!gsc_program_select_player_normal_health(s->program)) {
        session_message(error,cap,"normal health program selection failed");return 0;
    }
    if(s->player_max_ammo_enabled&&!gsc_program_select_player_max_ammo(s->program)) {
        session_message(error,cap,"max ammo program selection failed");return 0;
    }
    if(s->player_viewmodel_enabled&&!gsc_program_select_player_viewmodel(s->program)) {
        session_message(error,cap,"viewmodel program selection failed");return 0;
    }
    if(s->entity_operations_enabled&&(!gsc_program_select_entity_operations(s->program)||
       !gsc_program_select_entity_coordinates(s->program))) {
        session_message(error,cap,"R198 entity operations program selection failed");return 0;
    }
    if(s->entity_links_enabled&&!gsc_program_select_entity_links(s->program)){session_message(error,cap,"entity links program selection failed");return 0;}
    if(s->delete_transactions_enabled&&!gsc_program_select_delete_transactions(s->program)) {
        session_message(error,cap,"R204 delete transaction program selection failed");return 0;
    }
    if(s->loop_fx_enabled&&!gsc_program_select_loop_fx(s->program)) {
        session_message(error,cap,"loop FX program selection failed");return 0;
    }
    if(s->spatial_natives_enabled&&!gsc_program_select_spatial_natives(s->program)) {
        session_message(error,cap,"R198 spatial natives program selection failed");return 0;
    }
    if(s->ai_arrays_enabled&&!gsc_program_select_ai_arrays(s->program)){session_message(error,cap,"R202 AI array program selection failed");return 0;}
    if(s->grenade_items_enabled&&!gsc_program_select_r216_items(s->program)){session_message(error,cap,"R216 grenade item program selection failed");return 0;}
    if(s->carbine_items_enabled&&!(s->carbine_items_enabled==2?gsc_program_select_r225_training_followup(s->program):gsc_program_select_r222_carbine_items(s->program))){session_message(error,cap,"R222 carbine item selection failed");return 0;}
    if(s->yaw_motion_enabled&&!gsc_program_select_r216_yaw(s->program)){session_message(error,cap,"R216 yaw program selection failed");return 0;}
    if(s->node_queries_enabled&&!gsc_program_select_node_queries(s->program)) {
        session_message(error,cap,"R199 node query program selection failed");return 0;
    }
    if(s->turrets_enabled&&!gsc_program_select_r232_turrets(s->program)){session_message(error,cap,"R232 turret program selection failed");return 0;}
    if(s->actor_animations_enabled&&!gsc_program_select_r232_actor_animations(s->program)){session_message(error,cap,"R232 actor animation program selection failed");return 0;}
    if(s->random_range_enabled&&!gsc_program_select_r232_randomfloat_range(s->program)){session_message(error,cap,"randomFloatRange selector refused");return 0;}
    if(s->r232_source_float_enabled&&!gsc_program_select_r232_source_float_arithmetic(s->program)){session_message(error,cap,"init/look arithmetic selector refused");return 0;}
    if(s->quoted_strings_enabled&&!gsc_program_select_r232_quoted_strings(s->program)){session_message(error,cap,"native quoted-string selector refused");return 0;}
    if(s->weapon_classname_enabled&&!gsc_program_select_weapon_classname(s->program)){session_message(error,cap,"weapon classname program selection failed");return 0;}
    if(s->weapon_model_enabled&&!gsc_program_select_weapon_model_query(s->program)){session_message(error,cap,"weapon model program selection failed");return 0;}
    if(s->actor_drop_enabled&&!gsc_program_select_r233_actor_drop(s->program)){session_message(error,cap,"DropWeapon program selection failed");return 0;}
    if(s->actor_presence_enabled&&!gsc_program_select_actor_presence(s->program)){session_message(error,cap,"actor presence program selection failed");return 0;}
    if(s->actor_look_enabled&&!gsc_program_select_r232_actor_look(s->program)){session_message(error,cap,"actor look program selection failed");return 0;}
    if(s->function_admission_selected&&!gsc_program_select_r232_function_admission(s->program)){session_message(error,cap,"whole-function admission selector refused");return 0;}
    if(s->owned_numeric_delete_enabled&&!gsc_program_select_r233_owned_numeric_delete(s->program)){session_message(error,cap,"owned numeric deletion program selector refused");return 0;}
    if(s->native_animation_roots_enabled&&!gsc_program_select_r233_native_animation_roots(s->program)){session_message(error,cap,"native animation root selector refused");return 0;}
    if(s->flagged_all_restart_enabled&&!gsc_program_select_r233_flagged_all_restart(s->program)){session_message(error,cap,"flagged all restart program selector refused");return 0;}
    if(!r243_command_program_select(s)){session_message(error,cap,"method12 program binding refused");return 0;}
    if(s->critical_selected&&!gsc_program_select_actor_critical_methods(s->program)){session_message(error,cap,"critical program selection refused");return 0;}
    if(s->namespace_length_enabled&&!gsc_program_select_r233_namespace_animation_length(s->program)){session_message(error,cap,"namespace length program selection failed");return 0;}
    if(s->developer_selected&&!(s->turret_program_enabled?gsc_program_select_developer_value(s->program,s->developer.enabled):gsc_program_select_r232_developer_script(s->program,&s->developer))){session_message(error,cap,"developer setting selector refused");return 0;}
    if(s->actor_goal_enabled&&!gsc_program_select_r222_actor_goal(s->program)){session_message(error,cap,"Actor goal program selection failed");return 0;}
    if(s->hud_elements_enabled&&!gsc_program_select_hud_elements(s->program)){session_message(error,cap,"R200 HUD program selection failed");return 0;}
    if(s->entity_collision_enabled&&!gsc_program_select_entity_collision(s->program)){session_message(error,cap,"R200 collision program selection failed");return 0;}
    if(s->training_opening_enabled&&!gsc_program_select_r228_training_opening(s->program)){session_message(error,cap,"opening selection failed");return 0;}
    if(s->local_identifier_case_enabled&&!gsc_program_select_local_identifier_case(s->program)) {session_message(error,cap,"local identifier selection failed");return 0;}
    if(s->loop_sound_enabled&&!gsc_program_select_loop_sound(s->program)) {session_message(error,cap,"loop sound selection failed");return 0;}
    if(s->script_sound_enabled&&!gsc_program_select_script_sound(s->program)) {session_message(error,cap,"playsound selection failed");return 0;}
    if(s->sound_notify_enabled&&!gsc_program_select_sound_notify(s->program)){session_message(error,cap,"sound notify selection failed");return 0;}
    if(s->r243_script_state_enabled&&!gsc_program_select_r243_script_state(s->program)) {
        session_message(error,cap,"selected Actor tracking program binding refused");return 0;
    }
    if(s->r243_scripted_prefix_enabled&&!gsc_program_select_r243_scripted_prefix(s->program)) {
        session_message(error,cap,"selected native scripted prefix program binding refused");return 0;
    }
    if(s->r243_scripted_start_enabled&&!gsc_program_select_r243_scripted_start(s->program)) {
        session_message(error,cap,"normal scripted start program binding refused");return 0;
    }
    for(index=0;index<graph.count;++index) {
        GscSessionSource source;GscLinkAuditSummary summary;
        char path[GSC_SESSION_PATH_CAP],why[GSC_SESSION_ERROR_CAP],*copy=NULL;
        size_t length=strlen(graph.modules[index]);int read_ok,compiled;
        if(s->abort_requested||s->view.error[0]||!session_script_roots_current(s)||!session_animation_resources_current(s)||!session_node_scripts_current(s)) {
            session_message(error,cap,"module graph acquisition cancelled");return 0;
        }
        memcpy(path,graph.modules[index],length);memcpy(path+length,".gsc",5);
        memset(&source,0,sizeof(source));memset(why,0,sizeof(why));
        read_ok=s->host.read(s->host.user,path,&source,why,sizeof(why));
        why[sizeof(why)-1]=0;
        if(!read_ok) {
            session_message(error,cap,why[0]?why:"module source read failed");return 0;
        }
        if(!source.data||!source.size||source.size>(s->execution_extensions?GSC_SESSION_EXTENDED_MAX_SOURCE:GSC_SESSION_MAX_SOURCE))
            session_terminal(s,"invalid module source buffer or source size limit");
        else if(source.size>(s->execution_extensions?GSC_SESSION_EXTENDED_MAX_TOTAL_SOURCE:GSC_SESSION_MAX_TOTAL_SOURCE)-graph.total)
            session_terminal(s,s->execution_extensions?"module graph exceeds 8388608 source bytes":"module graph exceeds 1048576 source bytes");
        else if(!s->abort_requested) {
            if(memchr(source.data,0,source.size))
                session_terminal(s,"embedded NUL in module source");
            else {
                copy=GSC_SESSION_MALLOC(source.size+1);
                if(!copy)session_terminal(s,"module source copy allocation failed");
                else {
                    memcpy(copy,source.data,source.size);copy[source.size]=0;
                    graph.total+=source.size;
                }
            }
        }
        /* read-success owns a lease even for malformed output, size failure,
         * allocation failure or callback reentry. read-failure owns no lease. */
        s->host.release(s->host.user,&source);
        if(s->abort_requested||s->view.error[0]||!copy||!session_script_roots_current(s)||!session_animation_resources_current(s)||!session_node_scripts_current(s)) {
            GSC_SESSION_FREE(copy);
            session_message(error,cap,"module source acquisition cancelled");return 0;
        }
        why[0]=0;
        compiled=gsc_module_compile(s->program,graph.modules[index],copy,why,sizeof(why));
        GSC_SESSION_FREE(copy);
        if(!compiled) {
            if(s->execution_extensions&&error&&cap)snprintf(error,cap,"%s: %s",path,why[0]?why:"module source compile failed");
            else session_message(error,cap,why[0]?why:"module source compile failed");return 0;
        }
        /* Compile every module whole and discover from every retained site;
         * link outcome or reachability never hides a dependency or unknown
         * native. The final common loader performs the single whole preflight. */
        if(!gsc_program_audit_links(s->program,s->view.module,
            session_graph_dependency,&graph,&summary,why,sizeof(why))) {
            session_message(error,cap,graph.error[0]?graph.error:
                            (why[0]?why:"module dependency audit failed"));return 0;
        }
    }
    return 1;
}

/* Legacy profiles retain their bounded stack storage and limits. The R198
 * table is private to this synchronous prepare, not a Session/Program owner. */
static int session_load_modules_legacy(GscSession *s,char *error,size_t cap) {
    char storage[GSC_SESSION_LIFETIME_MAX_MODULES][GSC_SESSION_PATH_CAP];
    return session_load_modules_with_storage(s,storage,
        s->lifetime_enabled?GSC_SESSION_LIFETIME_MAX_MODULES:GSC_SESSION_MAX_MODULES,error,cap);
}
static int session_load_modules(GscSession *s,char *error,size_t cap) {
    char (*storage)[GSC_SESSION_PATH_CAP];int result;
    if(!s->execution_extensions)return session_load_modules_legacy(s,error,cap);
    storage=GSC_SESSION_MALLOC(sizeof(*storage)*GSC_SESSION_EXTENDED_MAX_MODULES);
    if(!storage){session_message(error,cap,"module graph allocation failed");return 0;}
    result=session_load_modules_with_storage(s,storage,GSC_SESSION_EXTENDED_MAX_MODULES,error,cap);
    GSC_SESSION_FREE(storage);return result;
}
