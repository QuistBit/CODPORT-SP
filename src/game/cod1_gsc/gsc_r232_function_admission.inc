/* Optional whole-function admission. Original sources and executable IR are
 * retained. Only temporary validation views hide OTHER functions, never an
 * instruction or dead branch of the function being certified. Legacy profiles
 * still run the original whole-program linker and runtime preflight. */
int gsc_program_select_r232_function_admission(GscProgram *g) {
    if(!g||g->invalid||g->linked||g->module_count||!g->turret_animations_selected||
       g->function_admission_selected)return 0;
    g->function_admission_selected=1;return 1;
}
static int admission_target_push(const GscProgram *g,int target,unsigned char seen[MAX_FUNCS],int stack[MAX_FUNCS],int *count) {
    if(target<0||target>=g->func_count)return 0;
    if(!seen[target]){if(*count>=MAX_FUNCS)return 0;seen[target]=1;stack[(*count)++]=target;}return 1;
}
static int function_admission_collect(const GscProgram *g,int first,const unsigned char *runtime_mask,unsigned char *collected,char *error,size_t size) {
    unsigned char seen[MAX_FUNCS];int stack[MAX_FUNCS],count=0,n,f,target;const Func *fn;
    if(!g||!g->function_admission_selected)return g!=NULL;
    if(g->func_count<1||g->func_count>MAX_FUNCS||!g->function_admission_token)return 0;
    memset(seen,0,sizeof(seen));if(!admission_target_push(g,first,seen,stack,&count))return 0;
    while(count){f=stack[--count];fn=&g->funcs[f];
        if(!fn->admission||(runtime_mask&&!runtime_mask[f])){if(error&&size)snprintf(error,size,"function %s::%s deferred: %s",g->modules[fn->module],fn->name,fn->admission_error[0]?fn->admission_error:"Runtime host or structural capability unavailable");return 0;}
        for(n=fn->start;n<fn->end;n++){const Instr *i=&g->code[n];target=-1;if(developer_code_hidden(g,n))continue;
            if(i->kind==I_CALL||i->kind==I_THREAD)target=i->target;
            else if((i->kind==I_METHOD||i->kind==I_METHOD_THREAD)&&(i->receiver_binding_kind==GSC_RECEIVER_TARGET_LOCAL_SCRIPT||i->receiver_binding_kind==GSC_RECEIVER_TARGET_QUALIFIED_SCRIPT))target=i->receiver_script_target;
            else if((i->kind==I_DYNAMIC_CALL||i->kind==I_DYNAMIC_THREAD)&&i->expr>=0&&i->expr<g->expr_count&&g->expr[i->expr].kind==E_FUNC_REF)target=g->expr[i->expr].target;
            if(target>=0&&!admission_target_push(g,target,seen,stack,&count))return 0;
        }
        for(n=fn->expr_start;n<fn->expr_end;n++){const Expr *e=&g->expr[n];target=-1;if(developer_expr_hidden(g,n))continue;
            if(e->kind==E_SCOPED_CALL||((e->kind==E_NATIVE_CALL||e->kind==E_ISDEFINED)&&e->call_binding==GSC_EXPR_TARGET_LOCAL_SCRIPT)||
               (e->kind==E_THREAD_CALL&&e->op==GSC_THREAD_EXPR_NAMED&&e->b<0))target=e->target;
            else if((e->kind==E_METHOD_CALL||e->kind==E_THREAD_CALL)&&(e->receiver_binding_kind==GSC_RECEIVER_TARGET_LOCAL_SCRIPT||e->receiver_binding_kind==GSC_RECEIVER_TARGET_QUALIFIED_SCRIPT))target=e->receiver_script_target;
            else if(e->kind==E_DYNAMIC_CALL&&e->a>=0&&e->a<g->expr_count&&g->expr[e->a].kind==E_FUNC_REF)target=g->expr[e->a].target;
            if(target>=0&&!admission_target_push(g,target,seen,stack,&count))return 0;
        }
    }if(collected)memcpy(collected,seen,sizeof(seen));return 1;
}
static int function_admission_current_mask(const GscProgram *g,int first,const unsigned char *runtime_mask,char *error,size_t size){return function_admission_collect(g,first,runtime_mask,NULL,error,size);}
static int function_admission_current(const GscProgram *g,int first,char *error,size_t size){return function_admission_current_mask(g,first,NULL,error,size);}
static int function_admission_runtime_current(const GscRuntime *r,int first,char *error,size_t size){
    if(!r||!r->program)return 0;if(!r->program->function_admission_selected)return 1;
    return r->function_admission_token==r->program->function_admission_token&&
        function_admission_current_mask(r->program,first,r->function_admission,error,size);
}
/* Parser-wide OR flags describe the retained whole corpus. A validation view
 * must instead derive its requirements from the unmasked IR, otherwise an
 * unentered future callback's thread-expression/raw literal poisons every
 * unrelated function. The executable program and its source are untouched. */
static void function_admission_requirements(GscProgram*v,int only){
    int n,f;v->requires_game_values=v->requires_receiver_method=0;
    v->requires_literal_semantics=v->requires_loop_semantics=v->requires_localized_string=v->requires_flow_semantics=0;
    v->requires_vector_semantics=v->requires_anim_ref_semantics=v->requires_dynamic_call=v->requires_empty_array_semantics=0;
    v->requires_scoped_expr_call=v->requires_mutation_semantics=v->requires_cast_semantics=v->requires_func_ref_semantics=0;
    v->requires_animtree_value_semantics=v->requires_integer_op_semantics=v->requires_switch_semantics=0;
    v->requires_raw_string_semantics=v->requires_thread_expr_semantics=v->requires_do_while_semantics=v->requires_local_expr_call=0;
    for(n=0;n<v->expr_count;n++){const Expr*e=v->expr+n;
        if(e->kind==E_VAR){if(literal_name(e->name))v->requires_literal_semantics=1;if(!strcmp(e->name,"level")||!strcmp(e->name,"self"))v->requires_game_values=1;}
        if(e->kind==E_FIELD||e->kind==E_INDEX)v->requires_game_values=1;
        if(e->kind==E_METHOD_CALL||(e->kind==E_THREAD_CALL&&e->b>=0))v->requires_receiver_method=1;
        if(e->kind==E_LOCALIZED_STRING||e->kind==E_RAW_LOCALIZED_STRING)v->requires_localized_string=1;
        if(e->kind==E_RAW_STRING||e->kind==E_RAW_LOCALIZED_STRING)v->requires_raw_string_semantics=1;
        if(e->kind==E_VECTOR)v->requires_vector_semantics=1;
        if(e->kind==E_ANIM_REF)v->requires_anim_ref_semantics=1;
        if(e->kind==E_DYNAMIC_CALL||(e->kind==E_THREAD_CALL&&e->op==GSC_THREAD_EXPR_COMPUTED))v->requires_dynamic_call=1;
        if(e->kind==E_EMPTY_ARRAY)v->requires_empty_array_semantics=1;
        if(e->kind==E_SCOPED_CALL)v->requires_scoped_expr_call=1;
        if(e->kind==E_CAST)v->requires_cast_semantics=1;
        if(e->kind==E_FUNC_REF)v->requires_func_ref_semantics=1;
        if(e->kind==E_ANIMTREE_VALUE)v->requires_animtree_value_semantics=1;
        if(e->kind==E_BINARY&&(e->op=='%'||e->op=='&'))v->requires_integer_op_semantics=1;
        if(e->kind==E_THREAD_CALL)v->requires_thread_expr_semantics=1;
        if((e->kind==E_NATIVE_CALL||e->kind==E_ISDEFINED)&&e->call_binding==GSC_EXPR_TARGET_LOCAL_SCRIPT)v->requires_local_expr_call=1;
    }
    for(n=0;n<v->code_count;n++){const Instr*i=v->code+n;
        if(i->kind==I_GAME_SET)v->requires_game_values=1;
        if(i->kind==I_METHOD||i->kind==I_METHOD_THREAD)v->requires_receiver_method=1;
        if(i->kind==I_FOR){v->requires_loop_semantics=1;if(i->dest>=0&&i->dest<v->loop_count&&v->loops[i->dest].step_kind>=FOR_INCREMENT)v->requires_mutation_semantics=1;}
        if(i->kind==I_WHILE||i->kind==I_RETURN||i->kind==I_BREAK||i->kind==I_CONTINUE||i->kind==I_DO_WHILE)v->requires_flow_semantics=1;
        if(i->kind==I_DO_WHILE)v->requires_do_while_semantics=1;
        if(i->kind==I_DYNAMIC_CALL||i->kind==I_DYNAMIC_THREAD)v->requires_dynamic_call=1;
        if(i->kind==I_MUTATE)v->requires_mutation_semantics=1;
        if(i->kind==I_SWITCH){int c;v->requires_switch_semantics=1;
            if(i->dest>=0&&i->dest<v->switch_count)for(c=v->switches[i->dest].first_case;c>=0&&c<v->case_count;c=v->cases[c].next_case)if(v->cases[c].kind==GSC_CASE_RAW_STRING)v->requires_raw_string_semantics=1;
        }
    }
    v->requires_dev_block=0;
    if(!v->developer_selected)for(n=0;n<v->dev_count;n++){const DevBlock*b=v->dev_blocks+n;
        for(f=0;f<v->func_count;f++)if((only>=0?f==only:v->funcs[f].admission)){
            const Func*fn=v->funcs+f;
            if((b->scope==GSC_DEV_FILE&&f>=b->func_start&&f<b->func_end)||
               (b->scope==GSC_DEV_STATEMENT&&fn->module==b->module&&b->code_start>=fn->start&&b->code_end<=fn->end&&b->expr_start>=fn->expr_start&&b->expr_end<=fn->expr_end))v->requires_dev_block=1;
        }
    }
}
static void function_admission_hide_body(GscProgram *view, int f) {
    Func *fn = &view->funcs[f];
    int pc;
    fn->argc = 0;
    for (pc = fn->start; pc < fn->end; ++pc) {
        memset(&view->code[pc], 0, sizeof(view->code[pc]));
        view->code[pc].kind = I_EMPTY;
    }
    for (pc = fn->expr_start; pc < fn->expr_end; ++pc) {
        memset(&view->expr[pc], 0, sizeof(view->expr[pc]));
        view->expr[pc].kind = E_NUMBER;
    }
}

static void function_admission_mask(GscProgram *view, int only) {
    int f;
    for (f = 0; f < view->func_count; ++f) {
        if (only >= 0 ? f != only : !view->funcs[f].admission)
            function_admission_hide_body(view, f);
    }
    developer_validation_view(view);
    function_admission_requirements(view,only);
}
/* A runtime preflight only reads the temporary IR. With disjoint bodies,
 * moving to the next function changes just two spans. Gaps remain intact.
 * Use the complete mask if the compiler supplied any other layout. */
static int admission_bodies_disjoint(const GscProgram *g) {
    int f, code_end = 0, expr_end = 0;
    for (f = 0; f < g->func_count; ++f) {
        const Func *fn = &g->funcs[f];
        if (fn->start < code_end || fn->end < fn->start || fn->end > g->code_count ||
            fn->expr_start < expr_end || fn->expr_end < fn->expr_start || fn->expr_end > g->expr_count)
            return 0;
        code_end = fn->end;
        expr_end = fn->expr_end;
    }
    return 1;
}

static void runtime_admission_view_function(GscProgram *view, const GscProgram *original,
                                            int previous, int selected, int disjoint) {
    const Func *fn = &original->funcs[selected];
    if (previous < 0 || !disjoint) {
        if (previous >= 0) {
            memcpy(view->code, original->code, (size_t)original->code_count * sizeof(*view->code));
            memcpy(view->expr, original->expr, (size_t)original->expr_count * sizeof(*view->expr));
            memcpy(view->funcs, original->funcs, (size_t)original->func_count * sizeof(*view->funcs));
        }
        function_admission_mask(view, selected);
        return;
    }
    function_admission_hide_body(view, previous);
    view->funcs[selected].argc = fn->argc;
    if (fn->end > fn->start)
        memcpy(view->code + fn->start, original->code + fn->start,
               (size_t)(fn->end - fn->start) * sizeof(*view->code));
    if (fn->expr_end > fn->expr_start)
        memcpy(view->expr + fn->expr_start, original->expr + fn->expr_start,
               (size_t)(fn->expr_end - fn->expr_start) * sizeof(*view->expr));
    developer_validation_view(view);
    function_admission_requirements(view, selected);
}
static int function_admission_view(const GscProgram *g,GscProgram *view) {
    *view=*g;view->code=NULL;view->expr=NULL;view->funcs=NULL;
    view->code=malloc((size_t)g->code_count*sizeof(*view->code));view->expr=malloc((size_t)g->expr_count*sizeof(*view->expr));
    view->funcs=malloc((size_t)g->func_count*sizeof(*view->funcs));
    if((g->code_count&&!view->code)||(g->expr_count&&!view->expr)||(g->func_count&&!view->funcs)){free(view->code);free(view->expr);free(view->funcs);memset(view,0,sizeof(*view));return 0;}
    memcpy(view->code,g->code,(size_t)g->code_count*sizeof(*view->code));memcpy(view->expr,g->expr,(size_t)g->expr_count*sizeof(*view->expr));memcpy(view->funcs,g->funcs,(size_t)g->func_count*sizeof(*view->funcs));return 1;
}
static void function_admission_view_free(GscProgram *view){free(view->code);free(view->expr);free(view->funcs);memset(view,0,sizeof(*view));}
static int program_link(GscProgram *g,const char *entry,int entities,int access,int events,int state,int controls,int flow,int arrays,int results,int payloads,int predicates,int actor_spawners,int function_refs,char *error,size_t size) {
    GscProgram staged,probe;int f,n,module,target,ok=0,local_calls=0;static uint64_t serial;
    if(!g||!g->function_admission_selected)return program_link_strict(g,entry,entities,access,events,state,controls,flow,arrays,results,payloads,predicates,actor_spawners,function_refs,error,size);
    if(g->linked||g->invalid||!g->turret_animations_selected||!function_refs||g->func_count<1||g->func_count>MAX_FUNCS||serial==UINT64_MAX)return 0;
    memset(&staged,0,sizeof(staged));memset(&probe,0,sizeof(probe));
    if(!function_admission_view(g,&staged)||!function_admission_view(g,&probe))goto done;
    for(f=0;f<g->func_count;f++){
        Func *fn=&staged.funcs[f];char why[256];
        memcpy(probe.code,g->code,(size_t)g->code_count*sizeof(*g->code));memcpy(probe.expr,g->expr,(size_t)g->expr_count*sizeof(*g->expr));
        memcpy(probe.funcs,g->funcs,(size_t)g->func_count*sizeof(*g->funcs));probe.linked=0;probe.function_admission_selected=0;function_admission_mask(&probe,f);why[0]=0;
        fn->admission=!developer_func_hidden(g,f)&&program_link_strict(&probe,entry,entities,access,events,state,controls,flow,arrays,results,payloads,predicates,actor_spawners,function_refs,why,sizeof(why));
        if(fn->admission)for(n=fn->start;n<fn->end;n++)if(static_receiver_instruction(&probe.code[n])&&probe.code[n].receiver_binding_kind==GSC_RECEIVER_TARGET_NONE){
            fn->admission=0;snprintf(why,sizeof(why),"line %d: unsupported receiver method/thread %s",probe.code[n].line,probe.code[n].name);break;}
        if(fn->admission)for(n=fn->expr_start;n<fn->expr_end;n++)if(static_receiver_expression(&probe.expr[n])&&probe.expr[n].receiver_binding_kind==GSC_RECEIVER_TARGET_NONE){
            fn->admission=0;snprintf(why,sizeof(why),"line %d: unsupported receiver expression %s",probe.expr[n].line,probe.expr[n].name);break;}
        memset(fn->admission_error,0,sizeof(fn->admission_error));if(!fn->admission)str_copy(fn->admission_error,sizeof(fn->admission_error),why[0]?why:"whole function validation refused");
        else{for(n=fn->start;n<fn->end;n++)if(!developer_code_hidden(g,n))staged.code[n]=probe.code[n];for(n=fn->expr_start;n<fn->expr_end;n++)if(!developer_expr_hidden(g,n))staged.expr[n]=probe.expr[n];local_calls|=probe.requires_local_expr_call;}
    }
    /* References bind to the real retained function, irrespective of admission.
     * Missing functions are never replaced by a success placeholder. */
    for(n=0;n<staged.expr_count;n++)if(staged.expr[n].kind==E_FUNC_REF&&!developer_expr_hidden(&staged,n)){Expr *e=&staged.expr[n];module=e->module[0]?find_module(&staged,e->module):e->owner_module;target=module<0?-1:find_func(&staged,module,e->name);
        if(target<0){if(error&&size)snprintf(error,size,"line %d: unresolved retained function reference %s::%s",e->line,e->module,e->name);goto done;}e->target=target;}
    staged.entity_mode=entities;staged.access_mode=access;staged.event_mode=events;staged.state_mode=state;staged.controls_mode=controls;staged.flow_mode=flow;staged.arrays_mode=arrays;staged.results_mode=results;staged.event_payload_mode=payloads;staged.predicate_mode=predicates;staged.actor_spawn_mode=actor_spawners;staged.function_ref_mode=function_refs;
    module=find_module(&staged,entry);staged.entry_fn=module<0?-1:find_func(&staged,module,"main");staged.function_admission_token=serial+1;staged.requires_local_expr_call=local_calls;
    if(staged.entry_fn<0||!function_admission_current(&staged,staged.entry_fn,error,size))goto done;
    memcpy(g->code,staged.code,(size_t)g->code_count*sizeof(*g->code));memcpy(g->expr,staged.expr,(size_t)g->expr_count*sizeof(*g->expr));memcpy(g->funcs,staged.funcs,(size_t)g->func_count*sizeof(*g->funcs));
    {Expr *expr=g->expr;Instr *code=g->code;Func *funcs=g->funcs;*g=staged;g->expr=expr;g->code=code;g->funcs=funcs;g->linked=1;g->function_admission_token=++serial;}
    ok=1;if(error&&size)error[0]=0;
done:function_admission_view_free(&probe);function_admission_view_free(&staged);return ok;
}
