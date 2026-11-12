/* R71 exact qualified receiver + local named receiver script binding.
 * Fixture-only static policy;
 * no original CoD1 receiver, arity, name-case or return-value semantics claimed.
 * Separate from expression builtins and legacy signed statement targets. */
typedef struct {
    int kind, module, function, arity;
    unsigned issues;
    int native_index;
    int event_index;
} ReceiverBinding;
static int qualified_receiver_instruction(const Instr *i) {
    return (i->kind==I_METHOD||i->kind==I_METHOD_THREAD)&&i->module[0];
}
static int qualified_receiver_expression(const Expr *e) {
    return e->module[0]&&(e->kind==E_METHOD_CALL||
        (e->kind==E_THREAD_CALL&&e->op==GSC_THREAD_EXPR_NAMED&&e->b>=0));
}
/* Retain the qualifier selectors for older white-box regression fixtures. */
static int static_receiver_instruction(const Instr *i) {
    return qualified_receiver_instruction(i)||i->kind==I_METHOD||i->kind==I_METHOD_THREAD;
}
static int static_receiver_expression(const Expr *e) {
    return qualified_receiver_expression(e)||e->kind==E_METHOD_CALL||(e->kind==E_THREAD_CALL&&
        e->op==GSC_THREAD_EXPR_NAMED&&e->b>=0);
}
/* Use the actual enclosing function, not an expression's legacy owner field.
 * No module-global fallback; a same-name function in another file is not local. */
static int receiver_owner_module(const GscProgram *g,int index,int expr) {
    int n;
    for(n=0;n<g->func_count;n++) {
        const Func *f=&g->funcs[n];
        if(expr?(index>=f->expr_start&&index<f->expr_end):
                (index>=f->start&&index<f->end))return f->module;
    }
    return -1;
}
static ReceiverBinding resolve_receiver_binding(const GscProgram *g,int owner,
                                                int local_thread,const char *module,
                                                const char *name,int argc) {
    ReceiverBinding r={GSC_RECEIVER_TARGET_NONE,-1,-1,-1,GSC_LINK_ISSUE_RECEIVER,-1,-1};
    /* Exact-local-candidate policy for ordinary methods, NOT a claim about
     * original native/script precedence. The receiverless fixture builtin
     * table is not a receiver-native registry and is deliberately not queried.
     * Missing ordinary local declarations remain deferred, unlike threads.
     * No fallback after finding a script candidate. Its formal count is a
     * runtime parameter boundary, not a script-call linker admission rule. */
    if(!module[0]&&!local_thread&&
       (owner<0||owner>=g->module_count||find_func(g,owner,name)<0)) {
        if(owner>=0&&owner<g->module_count&&hud_elem_method(g,name)) {
            r.kind=GSC_RECEIVER_TARGET_HUDELEM_NATIVE;r.native_index=hud_elem_method(g,name);
            r.arity=hud_elem_arity(r.native_index);r.issues=argc==r.arity?0u:GSC_LINK_ISSUE_BUILTIN_ARITY;
        } else if(owner>=0&&owner<g->module_count&&entity_method_descriptor(g,name)>=0) {
            r.kind=GSC_RECEIVER_TARGET_ENTITY_NATIVE;r.native_index=entity_method_descriptor(g,name);
            r.arity=entity_method_arity(r.native_index);
            r.issues=entity_method_argc_valid(r.native_index,argc)?0u:GSC_LINK_ISSUE_BUILTIN_ARITY;
        } else if(owner>=0&&owner<g->module_count&&entity_event_descriptor(g,name)>=0) {
            r.kind=GSC_RECEIVER_TARGET_ENTITY_EVENT;r.event_index=entity_event_descriptor(g,name);
            r.arity=r.event_index==ENTITY_EVENT_WAITTILLMATCH?2:1;
            r.issues=entity_event_argc_valid(g,r.event_index,argc)?0u:GSC_LINK_ISSUE_BUILTIN_ARITY;
        }
        return r;
    }
    r.module=module[0]?find_module(g,module):owner;
    if(r.module<0||r.module>=g->module_count) {
        r.module=-1;
        r.issues=module[0]?GSC_LINK_ISSUE_MISSING_MODULE:GSC_LINK_ISSUE_UNKNOWN;
        return r;
    }
    r.function=find_func(g,r.module,name);
    if(r.function<0) {
        r.issues=module[0]?GSC_LINK_ISSUE_MISSING_FUNCTION:GSC_LINK_ISSUE_UNKNOWN;
        return r;
    }
    r.kind=module[0]?GSC_RECEIVER_TARGET_QUALIFIED_SCRIPT:GSC_RECEIVER_TARGET_LOCAL_SCRIPT;
    r.arity=g->funcs[r.function].argc;
    r.issues=argc<0||argc>MAX_PARAMS?GSC_LINK_ISSUE_SCRIPT_ARITY:0u;
    return r;
}
static int receiver_binding_preflight(const GscProgram *g,int owner,
                                      int local_thread,const char *module,
                                      const char *name,int argc,int line,
                                      char *error,size_t error_size) {
    ReceiverBinding b=resolve_receiver_binding(g,owner,local_thread,module,name,argc);
    if(!b.issues)return 1;
    /* Preserves the old non-runnable unresolved-method path. This is a skipped
     * static choice, not a resolved native or a successful dispatch. */
    if(!module[0]&&!local_thread&&b.kind==GSC_RECEIVER_TARGET_NONE&&
       b.issues==GSC_LINK_ISSUE_RECEIVER)return 1;
    if(error&&error_size) {
        if(b.kind==GSC_RECEIVER_TARGET_HUDELEM_NATIVE||b.kind==GSC_RECEIVER_TARGET_ENTITY_NATIVE||b.kind==GSC_RECEIVER_TARGET_ENTITY_EVENT)
            snprintf(error,error_size,"line %d: %s expects exactly %d argument%s, got %d",
                line,name,b.arity,b.arity==1?"":"s",argc);
        else if(b.issues&GSC_LINK_ISSUE_SCRIPT_ARITY)
            snprintf(error,error_size,
                "line %d: receiver call %s::%s has invalid structural argument count %d",
                line,module,name,argc);
        else if(!module[0])snprintf(error,error_size,
                "line %d: unresolved/unsupported local receiver thread %s in %s",
                line,name,owner>=0&&owner<g->module_count?g->modules[owner]:"<invalid owner>");
        else snprintf(error,error_size,
                "line %d: unresolved/unsupported qualified receiver call %s::%s (%s)",
                line,module,name,b.issues&GSC_LINK_ISSUE_MISSING_MODULE?
                "missing module":"missing function");
    }
    return 0;
}
static void commit_receiver_bindings(GscProgram *g) {
    int n;
    for(n=0;n<g->code_count;n++) {
        Instr *i=&g->code[n];ReceiverBinding b;
        if(!static_receiver_instruction(i))continue;
        b=resolve_receiver_binding(g,receiver_owner_module(g,n,0),
            i->kind==I_METHOD_THREAD,i->module,i->name,i->argc);
        if(b.issues||b.kind==GSC_RECEIVER_TARGET_NONE)continue;
        i->receiver_binding_kind=b.kind;i->receiver_script_target=b.function;
        i->receiver_native_target=b.native_index;
        i->receiver_event_target=b.event_index;
    }
    for(n=0;n<g->expr_count;n++) {
        Expr *e=&g->expr[n];ReceiverBinding b;
        if(!static_receiver_expression(e))continue;
        b=resolve_receiver_binding(g,receiver_owner_module(g,n,1),
            e->kind==E_THREAD_CALL,e->module,e->name,e->argc);
        if(b.issues||b.kind==GSC_RECEIVER_TARGET_NONE)continue;
        e->receiver_binding_kind=b.kind;e->receiver_script_target=b.function;
        e->receiver_native_target=b.native_index;
        e->receiver_event_target=b.event_index;
    }
}
