/* R67: one allocation-free resolver shared by strict linking and inspection.
 * This is fixture policy, not evidence for original GSC resolution semantics. */
typedef struct { int kind, target, arity; unsigned issues; } ExpressionBinding;
static ExpressionBinding resolve_expression_binding(const GscProgram *g,const Expr *e) {
    ExpressionBinding r={GSC_EXPR_TARGET_NONE,-1,-1,GSC_LINK_ISSUE_UNKNOWN};
    const char *name=e->kind==E_ISDEFINED?"isdefined":e->name;
    int k,argc=e->kind==E_ISDEFINED?1:e->argc,builtin=program_builtin(g,name,e->owner_module);
    if(g->names_syntax&&e->owner_module>=0&&e->owner_module<g->module_count) {
        int target=find_func(g,e->owner_module,name);
        if(target>=0) {
            r.kind=GSC_EXPR_TARGET_LOCAL_SCRIPT;r.target=target;r.arity=g->funcs[target].argc;
            r.issues=argc<0||argc>MAX_PARAMS?GSC_LINK_ISSUE_SCRIPT_ARITY:0u;return r;
        }
    }
    if(builtin>=0) {
        const Builtin *b=&builtins[builtin];
        r.kind=GSC_EXPR_TARGET_BUILTIN;r.target=builtin;r.arity=b->arity;r.issues=0;
        if(b->context!=B_EXPRESSION)r.issues|=GSC_LINK_ISSUE_BUILTIN_CONTEXT;
        if(argc!=b->arity)r.issues|=GSC_LINK_ISSUE_BUILTIN_ARITY;
        else for(k=0;k<argc;k++) {
            int type=known_value_kind(g,e->kind==E_ISDEFINED?e->a:e->args[k]);
            if(type>=0&&!(g->arrays_mode&&type==V_ARRAY&&!strcmp(b->name,"isdefined"))&&
               !(g->struct_mode&&type==V_STRUCT&&!strcmp(b->name,"isdefined"))&&
               !(b->accepted[k]&B_VALUE(type)))r.issues|=GSC_LINK_ISSUE_BUILTIN_LITERAL;
        }
        return r;
    }
    if(e->owner_module>=0&&e->owner_module<g->module_count) {
        int target=find_func(g,e->owner_module,name);
        if(target>=0) {
            r.kind=GSC_EXPR_TARGET_LOCAL_SCRIPT;r.target=target;r.arity=g->funcs[target].argc;
            r.issues=argc<0||argc>MAX_PARAMS?GSC_LINK_ISSUE_SCRIPT_ARITY:0u;
        }
    }
    if(r.kind==GSC_EXPR_TARGET_NONE&&hud_elem_global(g,name)) {
        r.kind=GSC_EXPR_TARGET_HUDELEM_NATIVE;r.target=GSC_HUDELEM_CREATE;r.arity=0;r.issues=argc?GSC_LINK_ISSUE_BUILTIN_ARITY:0u;
    }
    if(r.kind==GSC_EXPR_TARGET_NONE&&vector_math_descriptor(g,name)) {
        r.kind=GSC_EXPR_TARGET_VECTOR_MATH_NATIVE;r.target=vector_math_descriptor(g,name);
        r.arity=vector_math_arity(r.target);r.issues=vector_math_static_issues(g,r.target,e->args,argc);
    }
    if(r.kind==GSC_EXPR_TARGET_NONE&&move_delta_descriptor(g,name)){r.kind=R243_EXPR_TARGET_MOVE_DELTA;r.target=1;r.arity=-1;r.issues=move_delta_static_issues(g,e->args,argc);}
    if(r.kind==GSC_EXPR_TARGET_NONE&&start_origin_descriptor(g,name)) {
        r.kind=GSC_EXPR_TARGET_START_ORIGIN_NATIVE;r.target=start_origin_descriptor(g,name);r.arity=g->map_start_pose_selected?-1:3;
        r.issues=start_origin_static_issues(g,e->args,argc);
    }
    if(r.kind==GSC_EXPR_TARGET_NONE&&ai_array_descriptor(g,name)) {
        r.kind=GSC_EXPR_TARGET_AI_ARRAY_NATIVE;r.target=1;r.arity=-1;r.issues=ai_array_static_issues(g,e->args,argc);
    }
    if(r.kind==GSC_EXPR_TARGET_NONE&&node_query_descriptor(g,name)) {
        r.kind=GSC_EXPR_TARGET_NODE_QUERY_NATIVE;r.target=node_query_descriptor(g,name);
        r.arity=node_query_arity(r.target);r.issues=node_query_static_issues(g,r.target,e->args,argc);
    }
    if(r.kind==GSC_EXPR_TARGET_NONE&&g->entity_mode&&
       (callable_name_equal(g,name,"getent")||callable_name_equal(g,name,"spawn")||(g->arrays_mode&&callable_name_equal(g,name,"getentarray")))) {
        r.kind=GSC_EXPR_TARGET_ENTITY_NATIVE;r.target=callable_name_equal(g,name,"spawn")?
            ENTITY_GLOBAL_SPAWN:callable_name_equal(g,name,"getentarray")?ENTITY_GLOBAL_GETENTARRAY:ENTITY_GLOBAL_GETENT;
        /* Original getent reads its two parameters in the native call.
         * A missing parameter must not reject an unexecuted branch at link.
         * Extra arguments and wider GetString coercions remain outside this
         * bounded adapter; getentarray keeps its existing policy. */
        r.arity=2;r.issues=(r.target==ENTITY_GLOBAL_GETENT?
            (argc>=0&&argc<=2):argc==2)?0u:GSC_LINK_ISSUE_BUILTIN_ARITY;
        if(r.target==ENTITY_GLOBAL_SPAWN&&argc==3) {
            int32_t flags;
            r.arity=3;r.issues=0;
            if(!entity_spawn_flags_class(g,e->args[0])||
               !entity_spawn_flags_literal(g,e->args[2],&flags))
                r.issues|=GSC_LINK_ISSUE_BUILTIN_LITERAL;
        }
        if(!r.issues)for(k=0;k<argc&&k<2;k++) {
            int type=known_value_kind(g,e->args[k]);
            if(type>=0&&type!=((r.target==ENTITY_GLOBAL_SPAWN&&k==1)?V_VECTOR:V_STRING))
                r.issues|=GSC_LINK_ISSUE_BUILTIN_LITERAL;
        }
    }
    if(r.kind==GSC_EXPR_TARGET_NONE&&entity_query_descriptor(g,name)>=0) {
        r.kind=GSC_EXPR_TARGET_ENTITY_QUERY_NATIVE;r.target=entity_query_descriptor(g,name);
        r.arity=-1;r.issues=entity_query_static_issues(g,r.target,e->args,argc);
    }
    if(r.kind==GSC_EXPR_TARGET_NONE&&struct_descriptor(g,name)>=0) {
        r.kind=GSC_EXPR_TARGET_STRUCT_NATIVE;r.target=struct_descriptor(g,name);
        r.arity=-1;r.issues=struct_static_issues(r.target,argc);
    }
    if(r.kind==GSC_EXPR_TARGET_NONE&&entity_predicate_descriptor(g,name)>=0) {
        r.kind=GSC_EXPR_TARGET_ENTITY_NATIVE;r.target=entity_predicate_descriptor(g,name);
        r.arity=1;r.issues=(r.target==ENTITY_GLOBAL_ISAI?
            (argc>=1&&argc<=MAX_PARAMS):argc==1)?0u:GSC_LINK_ISSUE_BUILTIN_ARITY;
    }
    if(r.kind==GSC_EXPR_TARGET_NONE&&resource_descriptor(g,name)>=0) {
        r.kind=GSC_EXPR_TARGET_RESOURCE_NATIVE;r.target=resource_descriptor(g,name);
        r.arity=-1;r.issues=resource_static_issues(g,r.target,e->args,argc);
    }
    if(r.kind==GSC_EXPR_TARGET_NONE&&objective_descriptor(g,name)>=0) {
        r.kind=GSC_EXPR_TARGET_OBJECTIVE_NATIVE;r.target=objective_descriptor(g,name);
        r.arity=-1;r.issues=objective_static_issues(g,r.target,e->args,argc);
    }
    if(r.kind==GSC_EXPR_TARGET_NONE&&console_descriptor(g,name)>=0) {
        r.kind=GSC_EXPR_TARGET_CONSOLE_NATIVE;r.target=console_descriptor(g,name);
        r.arity=-1;r.issues=console_static_issues(g,r.target,e->args,argc);
    }
    if(r.kind==GSC_EXPR_TARGET_NONE&&hud_descriptor(g,name)>=0) {
        r.kind=GSC_EXPR_TARGET_HUD_NATIVE;r.target=hud_descriptor(g,name);
        r.arity=-1;r.issues=hud_static_issues(g,r.target,e->args,argc);
    }
    if(r.kind==GSC_EXPR_TARGET_NONE&&mission_descriptor(g,name)>=0) {
        r.kind=GSC_EXPR_TARGET_MISSION_NATIVE;r.target=mission_descriptor(g,name);
        r.arity=-1;r.issues=mission_static_issues(g,r.target,e->args,argc);
    }
    if(r.kind==GSC_EXPR_TARGET_NONE&&blast_descriptor(g,name)>=0) {
        r.kind=GSC_EXPR_TARGET_BLAST_NATIVE;r.target=blast_descriptor(g,name);
        r.arity=-1;r.issues=blast_static_issues(g,r.target,e->args,argc);
    }
    if(r.kind==GSC_EXPR_TARGET_NONE&&quake_descriptor(g,name)>=0) {
        r.kind=GSC_EXPR_TARGET_QUAKE_NATIVE;r.target=quake_descriptor(g,name);
        r.arity=-1;r.issues=quake_static_issues(g,r.target,e->args,argc);
    }
    if(r.kind==GSC_EXPR_TARGET_NONE&&keybinding_descriptor(g,name,e->args,argc)>=0) {
        r.kind=GSC_EXPR_TARGET_KEYBINDING_NATIVE;r.target=GSC_KEYBINDING_QUERY;
        r.arity=-1;r.issues=0;
    }
    if(r.kind==GSC_EXPR_TARGET_NONE&&random_range_descriptor(g,name)){r.kind=GSC_EXPR_TARGET_R232_RANDOM_RANGE;r.target=1;r.arity=2;r.issues=random_range_issues(g,e->args,argc);}
    if(r.kind==GSC_EXPR_TARGET_NONE&&randomfloat_descriptor(g,name,e->args,argc)>=0) {
        r.kind=GSC_EXPR_TARGET_RANDOMFLOAT_NATIVE;r.target=GSC_RANDOMFLOAT_DRAW;
        r.arity=-1;r.issues=0;
    }
    if(r.kind==GSC_EXPR_TARGET_NONE&&randomint_descriptor(g,name,e->args,argc)>=0) {
        r.kind=GSC_EXPR_TARGET_RANDOMINT_NATIVE;r.target=GSC_RANDOMINT_DRAW;
        r.arity=-1;r.issues=0;
    }
    if(r.kind==GSC_EXPR_TARGET_NONE&&spawner_array_descriptor(g,name)>=0) {
        r.kind=GSC_EXPR_TARGET_SPAWNER_ARRAY_NATIVE;r.target=spawner_array_descriptor(g,name);
        r.arity=0;r.issues=spawner_array_static_issues(r.target,argc);
    }
    if(r.kind==GSC_EXPR_TARGET_NONE&&animation_length_descriptor(g,name)) {
        r.kind=GSC_EXPR_TARGET_ANIMATION_LENGTH_NATIVE;r.target=1;r.arity=-1;
        r.issues=argc>=0&&argc<=MAX_PARAMS?0u:GSC_LINK_ISSUE_BUILTIN_ARITY;
    }
    if(r.kind==GSC_EXPR_TARGET_NONE&&bullet_trace_descriptor(g,name)){r.kind=GSC_EXPR_TARGET_BULLET_TRACE_NATIVE;r.target=1;r.arity=-1;r.issues=bullet_trace_issues(g,e->args,argc);}
    if(r.kind==GSC_EXPR_TARGET_NONE&&vehicle_spawn_descriptor(g,name)){r.kind=GSC_EXPR_TARGET_VEHICLE_SPAWN_NATIVE;r.target=1;r.arity=-1;r.issues=vehicle_spawn_issues(g,e->args,argc);}
    if(r.kind==GSC_EXPR_TARGET_NONE&&turret_precache_descriptor(g,name)){r.kind=GSC_EXPR_TARGET_TURRET_PRECACHE_NATIVE;r.target=1;r.arity=-1;r.issues=turret_precache_issues(g,e->args,argc);}
    if(r.kind==GSC_EXPR_TARGET_NONE&&turret_spawn_descriptor(g,name)){r.kind=GSC_EXPR_TARGET_TURRET_SPAWN_NATIVE;r.target=1;r.arity=-1;r.issues=turret_spawn_issues(g,e->args,argc);}
    if(r.kind==GSC_EXPR_TARGET_NONE&&model_part_descriptor(g,name)){r.kind=GSC_EXPR_TARGET_MODEL_PART_NATIVE;r.target=1;r.arity=-1;r.issues=argc>=0&&argc<=MAX_PARAMS?0u:GSC_LINK_ISSUE_BUILTIN_ARITY;}
    if(r.kind==GSC_EXPR_TARGET_NONE&&weapon_classname_descriptor(g,name)){r.kind=GSC_EXPR_TARGET_WEAPON_CLASSNAME_NATIVE;r.target=1;r.arity=-1;r.issues=argc>=0&&argc<=MAX_PARAMS?0u:GSC_LINK_ISSUE_BUILTIN_ARITY;}
    if(r.kind==GSC_EXPR_TARGET_NONE&&weapon_model_descriptor(g,name)) {
        r.kind=GSC_EXPR_TARGET_WEAPON_MODEL_NATIVE;r.target=1;r.arity=-1;
        r.issues=argc>=0&&argc<=MAX_PARAMS?0u:GSC_LINK_ISSUE_BUILTIN_ARITY;
    }
    if(r.kind==GSC_EXPR_TARGET_NONE&&native_query_descriptor(g,name)>=0) {
        r.kind=GSC_EXPR_TARGET_NATIVE_QUERY;r.target=native_query_descriptor(g,name);
        r.arity=-1;r.issues=native_query_static_issues(g,r.target,e->args,argc);
    }
    if(r.kind==GSC_EXPR_TARGET_NONE&&tag_fx_descriptor(g,name)>=0) {
        r.kind=GSC_EXPR_TARGET_TAG_FX_NATIVE;r.target=tag_fx_descriptor(g,name);
        r.arity=3;r.issues=tag_fx_static_issues(g,r.target,e->args,argc);
    }
    if(r.kind==GSC_EXPR_TARGET_NONE&&save_game_descriptor(g,name)>=0) {
        r.kind=GSC_EXPR_TARGET_SAVE_GAME_NATIVE;r.target=save_game_descriptor(g,name);
        r.arity=-1;r.issues=save_game_static_issues(g,r.target,e->args,argc);
    }
    if(r.kind==GSC_EXPR_TARGET_NONE&&precache_item_descriptor(g,name)>=0) {
        r.kind=GSC_EXPR_TARGET_PRECACHE_ITEM_NATIVE;r.target=precache_item_descriptor(g,name);
        r.arity=-1;r.issues=precache_item_static_issues(g,r.target,e->args,argc);
    }
    if(r.kind==GSC_EXPR_TARGET_NONE&&ambient_descriptor(g,name)>=0) {
        r.kind=GSC_EXPR_TARGET_AMBIENT_NATIVE;r.target=ambient_descriptor(g,name);
        r.arity=-1;r.issues=ambient_static_issues(g,r.target,e->args,argc);
    }
    if(r.kind==GSC_EXPR_TARGET_NONE&&debug_line_descriptor(g,name)>=0) {
        r.kind=GSC_EXPR_TARGET_DEBUG_LINE_NATIVE;r.target=debug_line_descriptor(g,name);
        r.arity=-1;r.issues=debug_line_static_issues(g,r.target,e->args,argc);
    }
    if(r.kind==GSC_EXPR_TARGET_NONE&&debug_text_descriptor(g,name)>=0) {
        r.kind=GSC_EXPR_TARGET_DEBUG_TEXT_NATIVE;r.target=debug_text_descriptor(g,name);
        r.arity=-1;r.issues=debug_text_static_issues(g,r.target,e->args,argc);
    }
    if(r.kind==GSC_EXPR_TARGET_NONE&&shellshock_descriptor(g,name)>=0) {
        r.kind=GSC_EXPR_TARGET_SHELLSHOCK_NATIVE;r.target=shellshock_descriptor(g,name);
        r.arity=-1;r.issues=shellshock_static_issues(g,r.target,e->args,argc);
    }
    if(r.kind==GSC_EXPR_TARGET_NONE&&cvar_descriptor(g,name)>=0) {
        r.kind=GSC_EXPR_TARGET_CVAR_NATIVE;r.target=cvar_descriptor(g,name);
        r.arity=-1;r.issues=cvar_static_issues(g,r.target,e->args,argc);
    }
    if(r.kind==GSC_EXPR_TARGET_NONE&&fx_descriptor(g,name)>=0) {
        r.kind=GSC_EXPR_TARGET_FX_NATIVE;r.target=fx_descriptor(g,name);
        r.arity=-1;r.issues=fx_static_issues(g,r.target,e->args,argc);
    }
    return r;
}
int gsc_program_expression_call_get(const GscProgram *g,int index,GscExpressionCallInfo *out) {
    GscExpressionCallInfo info;ExpressionBinding binding;const Expr *e;int k;
    if(!g||g->invalid||!out||index<0||index>=g->expr_count)return 0;
    e=&g->expr[index];
    if((e->kind!=E_NATIVE_CALL&&e->kind!=E_ISDEFINED)||e->owner_module<0||
       e->owner_module>=g->module_count||e->argc<0||e->argc>MAX_PARAMS)return 0;
    memset(&info,0,sizeof(info));info.expr=index;info.owner_module=e->owner_module;
    info.line=e->audit_line;info.col=e->audit_col;info.source_offset=e->audit_offset;
    info.argc=e->kind==E_ISDEFINED?1:e->argc;
    for(k=0;k<16;k++)info.args[k]=-1;
    for(k=0;k<info.argc;k++) {
        info.args[k]=e->kind==E_ISDEFINED?e->a:e->args[k];
        if(info.args[k]<0||info.args[k]>=index)return 0;
    }
    str_copy(info.name,sizeof(info.name),e->kind==E_ISDEFINED?"isdefined":e->name);
    str_copy(info.module,sizeof(info.module),g->modules[e->owner_module]);
    binding=resolve_expression_binding(g,e);info.target_kind=binding.kind;
    info.issues=binding.issues;info.expected_arity=binding.arity;
    info.builtin_index=binding.kind==GSC_EXPR_TARGET_BUILTIN?binding.target:-1;
    info.script_function=binding.kind==GSC_EXPR_TARGET_LOCAL_SCRIPT?binding.target:-1;
    info.entity_native_index=binding.kind==GSC_EXPR_TARGET_ENTITY_NATIVE?binding.target:-1;
    if(g->linked) {
        if(binding.issues||binding.kind==GSC_EXPR_TARGET_NONE||
           e->call_binding!=binding.kind||e->target!=binding.target)return 0;
        info.committed=1;
    }
    *out=info;return 1;
}
