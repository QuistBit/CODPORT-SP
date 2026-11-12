/* R71 public inspector, included after the read-only link audit helpers. */
int gsc_program_receiver_binding_get(const GscProgram *g,int instruction,int expr,
                                     GscReceiverBindingInfo *out) {
    GscLinkAuditSite s;GscReceiverBindingInfo info;ReceiverBinding b;
    int stored_kind,stored_target,stored_native,stored_event,k,mode=expr>=0;
    if(!g||g->invalid||!out||instruction< -1||expr< -1||
       (instruction>=0)==(expr>=0))return 0;
    if(mode) {
        const Expr *e;
        if(expr>=g->expr_count)return 0;
        e=&g->expr[expr];
        if(e->kind!=E_METHOD_CALL&&!(e->kind==E_THREAD_CALL&&
           e->op==GSC_THREAD_EXPR_NAMED&&e->b>=0))return 0;
        if(audit_expression(g,expr,&s)!=1)return 0;
        stored_kind=e->receiver_binding_kind;stored_target=e->receiver_script_target;
        stored_native=e->receiver_native_target;
        stored_event=e->receiver_event_target;
    } else {
        const Instr *i;
        if(instruction>=g->code_count)return 0;
        i=&g->code[instruction];
        if(i->kind!=I_METHOD&&i->kind!=I_METHOD_THREAD)return 0;
        if(audit_instruction(g,instruction,&s)!=1)return 0;
        stored_kind=i->receiver_binding_kind;stored_target=i->receiver_script_target;
        stored_native=i->receiver_native_target;
        stored_event=i->receiver_event_target;
    }
    if(s.receiver_expr<0||s.receiver_expr>=g->expr_count||
       (mode&&s.receiver_expr>=expr))return 0;
    for(k=0;k<s.argc;k++)if(s.args[k]<0||s.args[k]>=g->expr_count||
                           (mode&&s.args[k]>=expr))return 0;
    b=resolve_receiver_binding(g,s.module_index,
        s.site_kind==GSC_LINK_RECEIVER_THREAD||s.site_kind==GSC_LINK_THREAD_EXPRESSION,
        s.requested_module,s.name,s.argc);
    if(b.kind==GSC_RECEIVER_TARGET_HUDELEM_NATIVE||b.kind==GSC_RECEIVER_TARGET_ENTITY_NATIVE||b.kind==GSC_RECEIVER_TARGET_ENTITY_EVENT) {
        /* Native inspection must report the same context/literal checks as
         * its read-only link audit, including before a failed link. Keep this
         * outside the legacy script receiver path and its index namespace. */
        audit_resolve(g,&s);
        b.issues|=s.issues&(GSC_LINK_ISSUE_BUILTIN_ARITY|
            GSC_LINK_ISSUE_BUILTIN_LITERAL|GSC_LINK_ISSUE_BUILTIN_CONTEXT);
    }
    memset(&info,0,sizeof(info));
    info.instruction=instruction;info.expr=expr;info.site_kind=s.site_kind;
    info.owner_module=s.module_index;info.owner_function=s.function_index;
    info.line=s.line;info.col=s.col;info.source_offset=s.source_offset;
    info.receiver_expr=s.receiver_expr;info.argc=s.argc;
    memcpy(info.args,s.args,sizeof(info.args));
    str_copy(info.source_module,sizeof(info.source_module),s.source_module);
    str_copy(info.source_function,sizeof(info.source_function),s.source_function);
    str_copy(info.requested_module,sizeof(info.requested_module),s.requested_module);
    str_copy(info.name,sizeof(info.name),s.name);
    info.target_kind=b.kind;info.script_module=b.module;info.script_function=b.function;
    info.builtin_index=-1;info.expected_arity=b.arity;info.issues=b.issues;
    info.entity_native_index=b.native_index;
    info.entity_event_index=b.event_index;
    if(g->linked&&(s.requested_module[0]||s.site_kind==GSC_LINK_RECEIVER_THREAD||
                   s.site_kind==GSC_LINK_THREAD_EXPRESSION||b.kind!=GSC_RECEIVER_TARGET_NONE)) {
        if(b.issues||b.kind==GSC_RECEIVER_TARGET_NONE||
           stored_kind!=b.kind||stored_target!=b.function||stored_native!=b.native_index||
           stored_event!=b.event_index)return 0;
        info.committed=1;
    } else if(stored_kind!=GSC_RECEIVER_TARGET_NONE||stored_target!=-1||stored_native!=-1||stored_event!=-1)return 0;
    *out=info;return 1;
}
