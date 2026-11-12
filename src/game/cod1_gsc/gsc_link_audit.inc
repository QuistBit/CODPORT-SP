/* R66: included after the original strict linker. This observer never writes
 * to the program, never allocates and does not make unresolved calls runnable. */
_Static_assert(MAX_PARAMS<=16,"audit argument copies must cover parser cap");

int gsc_program_link_function_count(const GscProgram *g) {
    return g&&!g->invalid?g->func_count:0;
}
int gsc_program_link_function_get(const GscProgram *g,int index,GscLinkFunctionInfo *out) {
    const Func *f;
    if(!g||g->invalid||!out||index<0||index>=g->func_count)return 0;
    f=&g->funcs[index];
    if(f->module<0||f->module>=g->module_count||f->start<0||f->end<f->start||
       f->end>g->code_count||f->expr_start<0||f->expr_end<f->expr_start||
       f->expr_end>g->expr_count)return 0;
    memset(out,0,sizeof(*out));out->index=index;out->module_index=f->module;
    out->argc=f->argc;out->code_start=f->start;out->code_end=f->end;
    out->expr_start=f->expr_start;out->expr_end=f->expr_end;
    str_copy(out->module,sizeof(out->module),g->modules[f->module]);
    str_copy(out->name,sizeof(out->name),f->name);return 1;
}
static int audit_owner(const GscProgram *g,int index,int expr) {
    int k;
    for(k=0;k<g->func_count;k++) {
        const Func *f=&g->funcs[k];
        if(expr?(index>=f->expr_start&&index<f->expr_end):
                (index>=f->start&&index<f->end))return k;
    }
    return -1;
}
static int audit_site_init(const GscProgram *g,GscLinkAuditSite *s,
                           int index,int expr,int line,int col,size_t offset) {
    int k,owner=audit_owner(g,index,expr);const Func *f;
    if(owner<0||line<1||col<1)return 0;
    f=&g->funcs[owner];if(f->module<0||f->module>=g->module_count)return 0;
    memset(s,0,sizeof(*s));
    s->instruction=expr?-1:index;s->expr=expr?index:-1;
    s->function_index=owner;s->module_index=f->module;
    s->line=line;s->col=col;s->source_offset=offset;
    s->receiver_expr=s->target_expr=s->candidate_function=s->candidate_module=-1;
    s->expected_arity=s->builtin_index=-1;
    s->entity_native_index=-1;
    s->entity_event_index=-1;
    for(k=0;k<MAX_PARAMS;k++)s->args[k]=-1;
    str_copy(s->source_module,sizeof(s->source_module),g->modules[f->module]);
    str_copy(s->source_function,sizeof(s->source_function),f->name);
    return 1;
}
static int audit_instruction(const GscProgram *g,int n,GscLinkAuditSite *s) {
    const Instr *i=&g->code[n];int kind;
    switch(i->kind) {
      case I_CALL:kind=GSC_LINK_STATEMENT_CALL;break;
      case I_THREAD:kind=GSC_LINK_STATEMENT_THREAD;break;
      case I_METHOD:kind=GSC_LINK_RECEIVER_CALL;break;
      case I_METHOD_THREAD:kind=GSC_LINK_RECEIVER_THREAD;break;
      case I_DYNAMIC_CALL:kind=GSC_LINK_COMPUTED_CALL;break;
      case I_DYNAMIC_THREAD:kind=GSC_LINK_COMPUTED_THREAD;break;
      case I_WAIT:kind=GSC_LINK_WAIT;break;
      case I_WAITER:kind=GSC_LINK_LEVEL_WAITTILL;break;
      case I_NOTIFY:kind=GSC_LINK_LEVEL_NOTIFY;break;
      default:return 0;
    }
    if(!audit_site_init(g,s,n,0,i->line,i->col,i->audit_offset))return -1;
    s->site_kind=kind;s->argc=i->argc;
    if(i->argc<0||i->argc>MAX_PARAMS)return -1;
    memcpy(s->args,i->args,(size_t)i->argc*sizeof(s->args[0]));
    str_copy(s->requested_module,sizeof(s->requested_module),i->module);
    str_copy(s->name,sizeof(s->name),i->name);
    if(i->kind==I_METHOD||i->kind==I_METHOD_THREAD||
       i->kind==I_DYNAMIC_CALL||i->kind==I_DYNAMIC_THREAD)s->receiver_expr=i->receiver;
    if(i->kind==I_DYNAMIC_CALL||i->kind==I_DYNAMIC_THREAD)s->target_expr=i->expr;
    if(i->kind==I_WAIT||i->kind==I_WAITER||i->kind==I_NOTIFY) {
        if(i->kind==I_WAIT||!g->event_payload_syntax) {
            s->argc=1;s->args[0]=i->expr;
        }
        str_copy(s->name,sizeof(s->name),i->kind==I_WAIT?"wait":
                 i->kind==I_WAITER?"level waittill":"level notify");
    }
    return 1;
}
static int audit_expression(const GscProgram *g,int n,GscLinkAuditSite *s) {
    const Expr *e=&g->expr[n];int kind;
    switch(e->kind) {
      case E_ISDEFINED:case E_NATIVE_CALL:kind=GSC_LINK_EXPRESSION_CALL;break;
      case E_METHOD_CALL:kind=GSC_LINK_RECEIVER_EXPRESSION;break;
      case E_SCOPED_CALL:kind=GSC_LINK_SCOPED_EXPRESSION;break;
      case E_FUNC_REF:kind=GSC_LINK_FUNCTION_REFERENCE;break;
      case E_DYNAMIC_CALL:kind=GSC_LINK_COMPUTED_EXPRESSION;break;
      case E_THREAD_CALL:kind=e->op==GSC_THREAD_EXPR_COMPUTED?
          GSC_LINK_COMPUTED_THREAD_EXPRESSION:GSC_LINK_THREAD_EXPRESSION;break;
      default:return 0;
    }
    if(!audit_site_init(g,s,n,1,e->audit_line,e->audit_col,e->audit_offset))return -1;
    s->site_kind=kind;s->argc=e->argc;
    if(e->argc<0||e->argc>MAX_PARAMS)return -1;
    memcpy(s->args,e->args,(size_t)e->argc*sizeof(s->args[0]));
    str_copy(s->requested_module,sizeof(s->requested_module),e->module);
    str_copy(s->name,sizeof(s->name),e->name);
    if(e->kind==E_ISDEFINED) {
        str_copy(s->name,sizeof(s->name),"isdefined");s->argc=1;s->args[0]=e->a;
    } else if(e->kind==E_FUNC_REF)s->argc=-1;
    else if(e->kind==E_METHOD_CALL)s->receiver_expr=e->a;
    else if(e->kind==E_DYNAMIC_CALL||e->kind==E_THREAD_CALL) {
        s->receiver_expr=e->b;
        if(e->kind==E_DYNAMIC_CALL||e->op==GSC_THREAD_EXPR_COMPUTED)s->target_expr=e->a;
    }
    return 1;
}
static void audit_resolve(const GscProgram *g,GscLinkAuditSite *s) {
    int k,builtin=-1,module,target,context,primitive,computed,receiver,music,resource,objective,console,hud,mission,blast,quake,fx,cvar,shellshock,debug_text,debug_line,ambient,precache_item,save_game,tag_fx,native_query,spawner_array;
    primitive=s->site_kind==GSC_LINK_WAIT||s->site_kind==GSC_LINK_LEVEL_WAITTILL||
              s->site_kind==GSC_LINK_LEVEL_NOTIFY;
    computed=s->site_kind==GSC_LINK_COMPUTED_CALL||s->site_kind==GSC_LINK_COMPUTED_THREAD||
             s->site_kind==GSC_LINK_COMPUTED_EXPRESSION||
             s->site_kind==GSC_LINK_COMPUTED_THREAD_EXPRESSION;
    receiver=s->receiver_expr>=0;
    if(primitive) {
        s->binding=GSC_LINK_FIXTURE_PRIMITIVE;s->outcome=GSC_LINK_CHECKED;return;
    }
    if(computed) {
        s->binding=GSC_LINK_COMPUTED;s->outcome=GSC_LINK_DEFERRED;
        s->issues=GSC_LINK_ISSUE_COMPUTED|(receiver?GSC_LINK_ISSUE_RECEIVER:0u);return;
    }
    music=!s->requested_module[0]&&!receiver&&!computed&&
        find_func(g,s->module_index,s->name)<0?music_descriptor(g,s->name):-1;
    resource=!s->requested_module[0]&&!receiver&&!computed&&
        find_func(g,s->module_index,s->name)<0?resource_descriptor(g,s->name):-1;
    objective=!s->requested_module[0]&&!receiver&&!computed&&
        find_func(g,s->module_index,s->name)<0?objective_descriptor(g,s->name):-1;
    console=!s->requested_module[0]&&!receiver&&!computed&&
        find_func(g,s->module_index,s->name)<0?console_descriptor(g,s->name):-1;
    hud=!s->requested_module[0]&&!receiver&&!computed&&
        find_func(g,s->module_index,s->name)<0?hud_descriptor(g,s->name):-1;
    mission=!s->requested_module[0]&&!receiver&&!computed&&
        find_func(g,s->module_index,s->name)<0?mission_descriptor(g,s->name):-1;
    blast=!s->requested_module[0]&&!receiver&&!computed&&
        find_func(g,s->module_index,s->name)<0?blast_descriptor(g,s->name):-1;
    quake=!s->requested_module[0]&&!receiver&&!computed&&
        find_func(g,s->module_index,s->name)<0?quake_descriptor(g,s->name):-1;
    fx=!s->requested_module[0]&&!receiver&&!computed&&
        find_func(g,s->module_index,s->name)<0?fx_descriptor(g,s->name):-1;
    cvar=!s->requested_module[0]&&!receiver&&!computed&&
        find_func(g,s->module_index,s->name)<0?cvar_descriptor(g,s->name):-1;
    if(!s->requested_module[0]&&!receiver&&!computed&&find_func(g,s->module_index,s->name)<0&&hud_elem_global(g,s->name)) {
        s->binding=GSC_LINK_HUDELEM_NATIVE;s->expected_arity=0;s->issues=s->argc?GSC_LINK_ISSUE_BUILTIN_ARITY:0u;
        if(s->site_kind!=GSC_LINK_STATEMENT_CALL&&s->site_kind!=GSC_LINK_EXPRESSION_CALL)s->issues|=GSC_LINK_ISSUE_BUILTIN_CONTEXT;
        s->outcome=s->issues?GSC_LINK_BLOCKED:GSC_LINK_CHECKED;return;
    }
    if(!s->requested_module[0]&&receiver&&!computed&&find_func(g,s->module_index,s->name)<0&&start_origin_descriptor(g,s->name)) {
        s->binding=GSC_LINK_START_ORIGIN_NATIVE;s->expected_arity=3;
        s->issues=start_origin_static_issues(g,s->args,s->argc)|GSC_LINK_ISSUE_BUILTIN_CONTEXT;
        s->outcome=GSC_LINK_BLOCKED;return;
    }
    if(!s->requested_module[0]&&!receiver&&!computed&&find_func(g,s->module_index,s->name)<0&&start_origin_descriptor(g,s->name)) {
        s->binding=GSC_LINK_START_ORIGIN_NATIVE;s->expected_arity=3;s->issues=start_origin_static_issues(g,s->args,s->argc);
        if(s->site_kind!=GSC_LINK_STATEMENT_CALL&&s->site_kind!=GSC_LINK_EXPRESSION_CALL)s->issues|=GSC_LINK_ISSUE_BUILTIN_CONTEXT;
        s->outcome=s->issues?GSC_LINK_BLOCKED:GSC_LINK_CHECKED;return;
    }
    if(!s->requested_module[0]&&!receiver&&!computed&&find_func(g,s->module_index,s->name)<0&&ai_array_descriptor(g,s->name)) {
        s->binding=GSC_LINK_AI_ARRAY_NATIVE;s->expected_arity=-1;s->issues=ai_array_static_issues(g,s->args,s->argc);
        if(s->site_kind!=GSC_LINK_STATEMENT_CALL&&s->site_kind!=GSC_LINK_EXPRESSION_CALL)s->issues|=GSC_LINK_ISSUE_BUILTIN_CONTEXT;
        s->outcome=s->issues?GSC_LINK_BLOCKED:GSC_LINK_CHECKED;return;
    }
    {int node_query=!s->requested_module[0]&&!receiver&&!computed&&
        find_func(g,s->module_index,s->name)<0?node_query_descriptor(g,s->name):0;
     if(node_query) {
        s->binding=GSC_LINK_NODE_QUERY_NATIVE;s->expected_arity=node_query_arity(node_query);
        s->issues=node_query_static_issues(g,node_query,s->args,s->argc);
        if(s->site_kind!=GSC_LINK_STATEMENT_CALL&&s->site_kind!=GSC_LINK_EXPRESSION_CALL)
            s->issues|=GSC_LINK_ISSUE_BUILTIN_CONTEXT;
        s->outcome=s->issues?GSC_LINK_BLOCKED:GSC_LINK_CHECKED;return;
     }}
    {int math=!s->requested_module[0]&&!receiver&&!computed&&
        find_func(g,s->module_index,s->name)<0?vector_math_descriptor(g,s->name):0;
     if(math) {
        s->binding=GSC_LINK_VECTOR_MATH_NATIVE;s->expected_arity=vector_math_arity(math);
        s->issues=vector_math_static_issues(g,math,s->args,s->argc);
        if(s->site_kind!=GSC_LINK_STATEMENT_CALL&&s->site_kind!=GSC_LINK_EXPRESSION_CALL)
            s->issues|=GSC_LINK_ISSUE_BUILTIN_CONTEXT;
        s->outcome=s->issues?GSC_LINK_BLOCKED:GSC_LINK_CHECKED;return;
     }}
    {int selected_query=!s->requested_module[0]&&!receiver&&!computed&&
        find_func(g,s->module_index,s->name)<0?entity_query_descriptor(g,s->name):-1;
     if(selected_query>=0) {
        s->binding=GSC_LINK_ENTITY_QUERY_NATIVE;s->expected_arity=-1;
        s->issues=entity_query_static_issues(g,selected_query,s->args,s->argc);
        if(s->site_kind!=GSC_LINK_STATEMENT_CALL&&s->site_kind!=GSC_LINK_EXPRESSION_CALL)
            s->issues|=GSC_LINK_ISSUE_BUILTIN_CONTEXT;
        s->outcome=s->issues?GSC_LINK_BLOCKED:GSC_LINK_CHECKED;return;
     }}
    {int selected_struct=!s->requested_module[0]&&!receiver&&!computed&&
        find_func(g,s->module_index,s->name)<0?struct_descriptor(g,s->name):-1;
     if(selected_struct>=0) {
        s->binding=GSC_LINK_STRUCT_NATIVE;s->expected_arity=-1;
        s->issues=struct_static_issues(selected_struct,s->argc);
        if(s->site_kind!=GSC_LINK_STATEMENT_CALL&&s->site_kind!=GSC_LINK_EXPRESSION_CALL)
            s->issues|=GSC_LINK_ISSUE_BUILTIN_CONTEXT;
        s->outcome=s->issues?GSC_LINK_BLOCKED:GSC_LINK_CHECKED;return;
     }}
    {int selected_keybinding=!s->requested_module[0]&&!receiver&&!computed&&find_func(g,s->module_index,s->name)<0?
        keybinding_descriptor(g,s->name,s->args,s->argc):-1;
     if(selected_keybinding>=0){s->binding=GSC_LINK_KEYBINDING_NATIVE;s->expected_arity=-1;s->issues=0;
        if(s->site_kind!=GSC_LINK_STATEMENT_CALL&&s->site_kind!=GSC_LINK_EXPRESSION_CALL)s->issues|=GSC_LINK_ISSUE_BUILTIN_CONTEXT;
        s->outcome=s->issues?GSC_LINK_BLOCKED:GSC_LINK_CHECKED;return;}}
    {int selected_randomfloat=!s->requested_module[0] && !receiver && !computed &&
        find_func(g,s->module_index,s->name)<0?
        randomfloat_descriptor(g,s->name,s->args,s->argc):-1;
     if(selected_randomfloat>=0) {
        s->binding=GSC_LINK_RANDOMFLOAT_NATIVE;s->expected_arity=-1;
        s->issues=0;
        if(s->site_kind!=GSC_LINK_STATEMENT_CALL&&s->site_kind!=GSC_LINK_EXPRESSION_CALL)
            s->issues|=GSC_LINK_ISSUE_BUILTIN_CONTEXT;
        s->outcome=s->issues?GSC_LINK_BLOCKED:GSC_LINK_CHECKED;
        return;
     }}
    {int selected_randomint=!s->requested_module[0] && !receiver && !computed &&
        find_func(g,s->module_index,s->name)<0?
        randomint_descriptor(g,s->name,s->args,s->argc):-1;
     if(selected_randomint>=0) {
        s->binding=GSC_LINK_RANDOMINT_NATIVE;s->expected_arity=-1;
        s->issues=0;
        if(s->site_kind!=GSC_LINK_STATEMENT_CALL&&s->site_kind!=GSC_LINK_EXPRESSION_CALL)
            s->issues|=GSC_LINK_ISSUE_BUILTIN_CONTEXT;
        s->outcome=s->issues?GSC_LINK_BLOCKED:GSC_LINK_CHECKED;
        return;
     }}
    spawner_array=!s->requested_module[0]&&!receiver&&!computed&&
        find_func(g,s->module_index,s->name)<0?spawner_array_descriptor(g,s->name):-1;
    if(spawner_array>=0) {
        s->binding=GSC_LINK_SPAWNER_ARRAY_NATIVE;s->expected_arity=0;
        s->issues=spawner_array_static_issues(spawner_array,s->argc);
        if(s->site_kind!=GSC_LINK_STATEMENT_CALL&&s->site_kind!=GSC_LINK_EXPRESSION_CALL)
            s->issues|=GSC_LINK_ISSUE_BUILTIN_CONTEXT;
        s->outcome=s->issues?GSC_LINK_BLOCKED:GSC_LINK_CHECKED;return;
    }
    native_query=!s->requested_module[0]&&!receiver&&!computed&&
        find_func(g,s->module_index,s->name)<0?native_query_descriptor(g,s->name):-1;
    if(native_query>=0) {
        s->binding=GSC_LINK_NATIVE_QUERY;s->expected_arity=-1;
        s->issues=native_query_static_issues(g,native_query,s->args,s->argc);
        if(s->site_kind!=GSC_LINK_STATEMENT_CALL&&s->site_kind!=GSC_LINK_EXPRESSION_CALL)
            s->issues|=GSC_LINK_ISSUE_BUILTIN_CONTEXT;
        s->outcome=s->issues?GSC_LINK_BLOCKED:GSC_LINK_CHECKED;return;
    }
    tag_fx=!s->requested_module[0]&&!receiver&&!computed&&
        find_func(g,s->module_index,s->name)<0?tag_fx_descriptor(g,s->name):-1;
    if(tag_fx>=0) {
        s->binding=GSC_LINK_TAG_FX_NATIVE;s->expected_arity=3;
        s->issues=tag_fx_static_issues(g,tag_fx,s->args,s->argc);
        if(s->site_kind!=GSC_LINK_STATEMENT_CALL&&s->site_kind!=GSC_LINK_EXPRESSION_CALL)
            s->issues|=GSC_LINK_ISSUE_BUILTIN_CONTEXT;
        s->outcome=s->issues?GSC_LINK_BLOCKED:GSC_LINK_CHECKED;return;
    }
    save_game=!s->requested_module[0]&&!receiver&&!computed&&
        find_func(g,s->module_index,s->name)<0?save_game_descriptor(g,s->name):-1;
    if(save_game>=0) {
        s->binding=GSC_LINK_SAVE_GAME_NATIVE;s->expected_arity=-1;
        s->issues=save_game_static_issues(g,save_game,s->args,s->argc);
        if(s->site_kind!=GSC_LINK_STATEMENT_CALL&&s->site_kind!=GSC_LINK_EXPRESSION_CALL)
            s->issues|=GSC_LINK_ISSUE_BUILTIN_CONTEXT;
        s->outcome=s->issues?GSC_LINK_BLOCKED:GSC_LINK_CHECKED;return;
    }
    precache_item=!s->requested_module[0]&&!receiver&&!computed&&
        find_func(g,s->module_index,s->name)<0?precache_item_descriptor(g,s->name):-1;
    if(precache_item>=0) {
        s->binding=GSC_LINK_PRECACHE_ITEM_NATIVE;s->expected_arity=-1;
        s->issues=precache_item_static_issues(g,precache_item,s->args,s->argc);
        if(s->site_kind!=GSC_LINK_STATEMENT_CALL&&s->site_kind!=GSC_LINK_EXPRESSION_CALL)
            s->issues|=GSC_LINK_ISSUE_BUILTIN_CONTEXT;
        s->outcome=s->issues?GSC_LINK_BLOCKED:GSC_LINK_CHECKED;return;
    }
    ambient=!s->requested_module[0]&&!receiver&&!computed&&
        find_func(g,s->module_index,s->name)<0?ambient_descriptor(g,s->name):-1;
    if(ambient>=0) {
        s->binding=GSC_LINK_AMBIENT_NATIVE;s->expected_arity=-1;
        s->issues=ambient_static_issues(g,ambient,s->args,s->argc);
        if(s->site_kind!=GSC_LINK_STATEMENT_CALL&&s->site_kind!=GSC_LINK_EXPRESSION_CALL)
            s->issues|=GSC_LINK_ISSUE_BUILTIN_CONTEXT;
        s->outcome=s->issues?GSC_LINK_BLOCKED:GSC_LINK_CHECKED;return;
    }
    debug_line=!s->requested_module[0]&&!receiver&&!computed&&
        find_func(g,s->module_index,s->name)<0?debug_line_descriptor(g,s->name):-1;
    if(debug_line>=0) {
        s->binding=GSC_LINK_DEBUG_LINE_NATIVE;s->expected_arity=-1;
        s->issues=debug_line_static_issues(g,debug_line,s->args,s->argc);
        if(s->site_kind!=GSC_LINK_STATEMENT_CALL&&s->site_kind!=GSC_LINK_EXPRESSION_CALL)
            s->issues|=GSC_LINK_ISSUE_BUILTIN_CONTEXT;
        s->outcome=s->issues?GSC_LINK_BLOCKED:GSC_LINK_CHECKED;return;
    }
    debug_text=!s->requested_module[0]&&!receiver&&!computed&&
        find_func(g,s->module_index,s->name)<0?debug_text_descriptor(g,s->name):-1;
    if(debug_text>=0) {
        s->binding=GSC_LINK_DEBUG_TEXT_NATIVE;s->expected_arity=-1;
        s->issues=debug_text_static_issues(g,debug_text,s->args,s->argc);
        if(s->site_kind!=GSC_LINK_STATEMENT_CALL&&s->site_kind!=GSC_LINK_EXPRESSION_CALL)
            s->issues|=GSC_LINK_ISSUE_BUILTIN_CONTEXT;
        s->outcome=s->issues?GSC_LINK_BLOCKED:GSC_LINK_CHECKED;return;
    }
    shellshock=!s->requested_module[0]&&!receiver&&!computed&&
        find_func(g,s->module_index,s->name)<0?shellshock_descriptor(g,s->name):-1;
    if(shellshock>=0) {
        s->binding=GSC_LINK_SHELLSHOCK_NATIVE;s->expected_arity=-1;
        s->issues=shellshock_static_issues(g,shellshock,s->args,s->argc);
        if(s->site_kind!=GSC_LINK_STATEMENT_CALL&&s->site_kind!=GSC_LINK_EXPRESSION_CALL)
            s->issues|=GSC_LINK_ISSUE_BUILTIN_CONTEXT;
        s->outcome=s->issues?GSC_LINK_BLOCKED:GSC_LINK_CHECKED;return;
    }
    if(cvar>=0) {
        s->binding=GSC_LINK_CVAR_NATIVE;s->expected_arity=-1;
        s->issues=cvar_static_issues(g,cvar,s->args,s->argc);
        if(s->site_kind!=GSC_LINK_STATEMENT_CALL&&s->site_kind!=GSC_LINK_EXPRESSION_CALL)
            s->issues|=GSC_LINK_ISSUE_BUILTIN_CONTEXT;
        s->outcome=s->issues?GSC_LINK_BLOCKED:GSC_LINK_CHECKED;return;
    }
    if(fx>=0) {
        s->binding=GSC_LINK_FX_NATIVE;s->expected_arity=-1;
        s->issues=fx_static_issues(g,fx,s->args,s->argc);
        if(s->site_kind!=GSC_LINK_STATEMENT_CALL&&s->site_kind!=GSC_LINK_EXPRESSION_CALL)
            s->issues|=GSC_LINK_ISSUE_BUILTIN_CONTEXT;
        s->outcome=s->issues?GSC_LINK_BLOCKED:GSC_LINK_CHECKED;return;
    }
    if(quake>=0) {
        s->binding=GSC_LINK_QUAKE_NATIVE;s->expected_arity=-1;
        s->issues=quake_static_issues(g,quake,s->args,s->argc);
        if(s->site_kind!=GSC_LINK_STATEMENT_CALL&&s->site_kind!=GSC_LINK_EXPRESSION_CALL)
            s->issues|=GSC_LINK_ISSUE_BUILTIN_CONTEXT;
        s->outcome=s->issues?GSC_LINK_BLOCKED:GSC_LINK_CHECKED;return;
    }
    if(blast>=0) {
        s->binding=GSC_LINK_BLAST_NATIVE;s->expected_arity=-1;
        s->issues=blast_static_issues(g,blast,s->args,s->argc);
        if(s->site_kind!=GSC_LINK_STATEMENT_CALL&&s->site_kind!=GSC_LINK_EXPRESSION_CALL)
            s->issues|=GSC_LINK_ISSUE_BUILTIN_CONTEXT;
        s->outcome=s->issues?GSC_LINK_BLOCKED:GSC_LINK_CHECKED;return;
    }
    if(mission>=0) {
        s->binding=GSC_LINK_MISSION_NATIVE;s->expected_arity=-1;
        s->issues=mission_static_issues(g,mission,s->args,s->argc);
        if(s->site_kind!=GSC_LINK_STATEMENT_CALL&&s->site_kind!=GSC_LINK_EXPRESSION_CALL)
            s->issues|=GSC_LINK_ISSUE_BUILTIN_CONTEXT;
        s->outcome=s->issues?GSC_LINK_BLOCKED:GSC_LINK_CHECKED;return;
    }
    if(hud>=0) {
        s->binding=GSC_LINK_HUD_NATIVE;s->expected_arity=-1;
        s->issues=hud_static_issues(g,hud,s->args,s->argc);
        if(s->site_kind!=GSC_LINK_STATEMENT_CALL&&s->site_kind!=GSC_LINK_EXPRESSION_CALL)
            s->issues|=GSC_LINK_ISSUE_BUILTIN_CONTEXT;
        s->outcome=s->issues?GSC_LINK_BLOCKED:GSC_LINK_CHECKED;return;
    }
    if(console>=0) {
        s->binding=GSC_LINK_CONSOLE_NATIVE;s->expected_arity=-1;
        s->issues=console_static_issues(g,console,s->args,s->argc);
        if(s->site_kind!=GSC_LINK_STATEMENT_CALL&&s->site_kind!=GSC_LINK_EXPRESSION_CALL)
            s->issues|=GSC_LINK_ISSUE_BUILTIN_CONTEXT;
        s->outcome=s->issues?GSC_LINK_BLOCKED:GSC_LINK_CHECKED;return;
    }
    if(objective>=0) {
        s->binding=GSC_LINK_OBJECTIVE_NATIVE;s->expected_arity=-1;
        s->issues=objective_static_issues(g,objective,s->args,s->argc);
        if(s->site_kind!=GSC_LINK_STATEMENT_CALL&&s->site_kind!=GSC_LINK_EXPRESSION_CALL)
            s->issues|=GSC_LINK_ISSUE_BUILTIN_CONTEXT;
        s->outcome=s->issues?GSC_LINK_BLOCKED:GSC_LINK_CHECKED;return;
    }
    if(resource>=0) {
        s->binding=GSC_LINK_RESOURCE_NATIVE;s->expected_arity=-1;
        s->issues=resource_static_issues(g,resource,s->args,s->argc);
        if(s->site_kind!=GSC_LINK_STATEMENT_CALL&&s->site_kind!=GSC_LINK_EXPRESSION_CALL)
            s->issues|=GSC_LINK_ISSUE_BUILTIN_CONTEXT;
        s->outcome=s->issues?GSC_LINK_BLOCKED:GSC_LINK_CHECKED;return;
    }
    if(music>=0) {
        s->binding=GSC_LINK_MUSIC_NATIVE;s->expected_arity=-1;
        s->issues=music_static_issues(g,music,s->args,s->argc);
        if(s->site_kind!=GSC_LINK_STATEMENT_CALL)s->issues|=GSC_LINK_ISSUE_BUILTIN_CONTEXT;
        s->outcome=s->issues?GSC_LINK_BLOCKED:GSC_LINK_CHECKED;return;
    }
    if(s->site_kind==GSC_LINK_EXPRESSION_CALL) {
        const Expr *e=&g->expr[s->expr];
        ExpressionBinding binding=resolve_expression_binding(g,e);
        s->issues=binding.issues;s->expected_arity=binding.arity;
        if(binding.kind==GSC_EXPR_TARGET_BUILTIN) {
            s->binding=GSC_LINK_FIXTURE_BUILTIN;s->builtin_index=binding.target;
        } else if(binding.kind==GSC_EXPR_TARGET_ENTITY_NATIVE) {
            s->binding=GSC_LINK_ENTITY_NATIVE;s->entity_native_index=binding.target;
        } else {
            s->candidate_module=e->owner_module;
            if(e->owner_module>=0&&e->owner_module<g->module_count)
                str_copy(s->candidate_module_path,sizeof(s->candidate_module_path),g->modules[e->owner_module]);
            if(binding.kind==GSC_EXPR_TARGET_LOCAL_SCRIPT) {
                s->binding=GSC_LINK_SCRIPT_LOCAL;s->candidate_function=binding.target;
            } else s->binding=GSC_LINK_UNKNOWN_UNQUALIFIED;
        }
        s->outcome=s->issues?GSC_LINK_BLOCKED:GSC_LINK_CHECKED;return;
    }
    context=s->site_kind==GSC_LINK_STATEMENT_CALL?B_STATEMENT:B_EXPRESSION;
    if(!s->requested_module[0]&&(s->site_kind==GSC_LINK_STATEMENT_CALL||
       s->site_kind==GSC_LINK_EXPRESSION_CALL))builtin=program_builtin(g,s->name,s->module_index);
    if(builtin>=0) {
        const Builtin *b=&builtins[builtin];
        s->binding=GSC_LINK_FIXTURE_BUILTIN;s->builtin_index=builtin;
        s->expected_arity=b->arity;
        if(b->context!=context)s->issues|=GSC_LINK_ISSUE_BUILTIN_CONTEXT;
        if(s->argc!=b->arity)s->issues|=GSC_LINK_ISSUE_BUILTIN_ARITY;
        else for(k=0;k<s->argc;k++) {
            int type=known_value_kind(g,s->args[k]);
            if(type>=0&&!(b->accepted[k]&B_VALUE(type)))s->issues|=GSC_LINK_ISSUE_BUILTIN_LITERAL;
        }
    } else {
        module=s->requested_module[0]?find_module(g,s->requested_module):s->module_index;
        target=module<0?-1:find_func(g,module,s->name);
        s->candidate_module=module;
        if(module>=0)str_copy(s->candidate_module_path,sizeof(s->candidate_module_path),g->modules[module]);
        if(target>=0) {
            s->candidate_function=target;s->expected_arity=g->funcs[target].argc;
            s->binding=s->requested_module[0]?GSC_LINK_SCRIPT_QUALIFIED:GSC_LINK_SCRIPT_LOCAL;
            /* Original callee checkclearparams rejects excess values only
             * when the admitted call executes, after argument evaluation. */
            /* A function reference has no argument list: its public argc=-1
             * inspection sentinel is not a malformed script call. */
            if(s->site_kind!=GSC_LINK_FUNCTION_REFERENCE&&
               (s->argc<0||s->argc>MAX_PARAMS))s->issues|=GSC_LINK_ISSUE_SCRIPT_ARITY;
            /* R67 exact bare expression calls now have a separate script
             * binding path. This classification is not a committed link. */
        } else if(!s->requested_module[0]) {
            s->binding=GSC_LINK_UNKNOWN_UNQUALIFIED;s->issues|=GSC_LINK_ISSUE_UNKNOWN;
        } else if(module<0) {
            s->binding=GSC_LINK_MISSING_MODULE;s->issues|=GSC_LINK_ISSUE_MISSING_MODULE;
        } else {
            s->binding=GSC_LINK_MISSING_FUNCTION;s->issues|=GSC_LINK_ISSUE_MISSING_FUNCTION;
        }
    }
    if(receiver) {
        ReceiverBinding b=resolve_receiver_binding(g,s->module_index,
            s->site_kind==GSC_LINK_RECEIVER_THREAD||s->site_kind==GSC_LINK_THREAD_EXPRESSION,
            s->requested_module,s->name,s->argc);
        if(b.kind==GSC_RECEIVER_TARGET_HUDELEM_NATIVE) {
            s->binding=GSC_LINK_HUDELEM_NATIVE;s->entity_native_index=b.native_index;
            s->candidate_module=s->candidate_function=-1;s->candidate_module_path[0]=0;
            s->expected_arity=b.arity;s->issues=b.issues;
            if(s->site_kind==GSC_LINK_RECEIVER_EXPRESSION)s->issues|=GSC_LINK_ISSUE_BUILTIN_CONTEXT;
            s->outcome=s->issues?GSC_LINK_BLOCKED:GSC_LINK_CHECKED;return;
        }
        if(b.kind==GSC_RECEIVER_TARGET_ENTITY_EVENT) {
            s->binding=GSC_LINK_FIXTURE_PRIMITIVE;s->entity_event_index=b.event_index;
            s->candidate_module=s->candidate_function=-1;s->candidate_module_path[0]=0;
            s->expected_arity=b.arity;s->issues=b.issues;
            if(s->site_kind==GSC_LINK_RECEIVER_EXPRESSION)s->issues|=GSC_LINK_ISSUE_BUILTIN_CONTEXT;
            if(s->argc==1) {
                int type=known_value_kind(g,s->args[0]);
                if((type>=0&&type!=V_STRING)||
                   (type==V_STRING&&!g->expr[s->args[0]].name[0]))
                    s->issues|=GSC_LINK_ISSUE_BUILTIN_LITERAL;
            }
            s->outcome=s->issues?GSC_LINK_BLOCKED:GSC_LINK_CHECKED;
            return;
        }
        if(b.kind==GSC_RECEIVER_TARGET_ENTITY_NATIVE) {
            s->binding=GSC_LINK_ENTITY_NATIVE;s->entity_native_index=b.native_index;
            s->candidate_module=s->candidate_function=-1;s->candidate_module_path[0]=0;
            s->expected_arity=b.arity;s->issues=b.issues;
            if(s->site_kind==GSC_LINK_RECEIVER_EXPRESSION&&!entity_method_has_result(b.native_index))s->issues|=GSC_LINK_ISSUE_BUILTIN_CONTEXT;
            if(s->argc==1&&!entity_method_has_result(b.native_index)&&b.native_index<ENTITY_METHOD_ENTERCRITICAL&&
               b.native_index!=ENTITY_METHOD_ATTACH&&b.native_index!=ENTITY_METHOD_DETACH&&
               b.native_index!=ENTITY_METHOD_SETSPAWNERTEAM) {
                int type=known_value_kind(g,s->args[0]);
                if(type>=0&&type!=entity_method_argument_kind(b.native_index))
                    s->issues|=GSC_LINK_ISSUE_BUILTIN_LITERAL;
            }
            s->outcome=s->issues?GSC_LINK_BLOCKED:GSC_LINK_CHECKED;
            return;
        }
        if(s->requested_module[0]||s->site_kind==GSC_LINK_RECEIVER_THREAD||
           s->site_kind==GSC_LINK_THREAD_EXPRESSION||b.kind!=GSC_RECEIVER_TARGET_NONE) {
            /* R71 exact ordinary local candidates use the same script path;
             * no receiver-native resolution or original precedence claim. */
            s->issues=b.issues;s->outcome=s->issues?GSC_LINK_BLOCKED:GSC_LINK_CHECKED;
        } else {
            /* Unknown ordinary methods preserve their previous audit issues. */
            s->issues|=GSC_LINK_ISSUE_RECEIVER;s->outcome=GSC_LINK_DEFERRED;
        }
    } else s->outcome=s->issues?GSC_LINK_BLOCKED:GSC_LINK_CHECKED;
}
static void audit_tally(GscLinkAuditSummary *r,const GscLinkAuditSite *s) {
    r->sites++;
    if(s->outcome==GSC_LINK_CHECKED)r->checked++;
    else if(s->outcome==GSC_LINK_BLOCKED)r->blocked++;
    else r->deferred++;
    switch(s->binding) {
      case GSC_LINK_SCRIPT_LOCAL:r->local_scripts++;break;
      case GSC_LINK_SCRIPT_QUALIFIED:r->qualified_scripts++;break;
      case GSC_LINK_FIXTURE_BUILTIN:r->fixture_builtins++;break;
      case GSC_LINK_UNKNOWN_UNQUALIFIED:r->unknown_unqualified++;break;
      case GSC_LINK_MISSING_MODULE:r->missing_modules++;break;
      case GSC_LINK_MISSING_FUNCTION:r->missing_functions++;break;
      case GSC_LINK_COMPUTED:r->computed++;break;
      case GSC_LINK_FIXTURE_PRIMITIVE:r->fixture_primitives++;break;
      case GSC_LINK_ENTITY_NATIVE:r->entity_natives++;break;
      default:break;
    }
    if(s->issues&(GSC_LINK_ISSUE_SCRIPT_ARITY|GSC_LINK_ISSUE_BUILTIN_ARITY))r->arity_issues++;
    if(s->issues&GSC_LINK_ISSUE_BUILTIN_LITERAL)r->literal_issues++;
    if(s->issues&GSC_LINK_ISSUE_BUILTIN_CONTEXT)r->context_issues++;
    if(s->issues&GSC_LINK_ISSUE_LOCAL_EXPR)r->unsupported_local_expressions++;
}
int gsc_program_audit_links(const GscProgram *g,const char *entry_module,
                            GscLinkAuditCallback callback,void *user,
                            GscLinkAuditSummary *out,char *error,size_t error_size) {
    GscLinkAuditSummary summary;GscLinkAuditSite s;int n,mode,result,module;
    if(error&&error_size)error[0]=0;
    if(out)memset(out,0,sizeof(*out));
    if(!g||g->invalid||!out||!g->module_count||!valid_module_path(entry_module)) {
        if(error&&error_size)snprintf(error,error_size,"invalid link-audit program, entry path or output");
        return 0;
    }
    memset(&summary,0,sizeof(summary));summary.modules=g->module_count;
    summary.functions=g->func_count;summary.instructions=g->code_count;
    summary.expressions=g->expr_count;module=find_module(g,entry_module);
    summary.entry_function=module<0?-1:find_func(g,module,"main");
    for(mode=0;mode<2;mode++)for(n=0;n<(mode?g->expr_count:g->code_count);n++) {
        result=mode?audit_expression(g,n,&s):audit_instruction(g,n,&s);
        if(result<0) {
            if(error&&error_size)snprintf(error,error_size,"invalid link-audit IR at %s %d",mode?"expression":"instruction",n);
            return 0;
        }
        if(!result)continue;
        audit_resolve(g,&s);audit_tally(&summary,&s);
        if(callback&&!callback(user,&s)) {
            if(error&&error_size)snprintf(error,error_size,"link audit cancelled after %d sites",summary.sites);
            return 0;
        }
    }
    *out=summary;return 1;
}
