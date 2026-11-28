/* R135 selected copied associative getkeybinding result. Original syscall/
 * CGAME cache effects precede MakeArray; private array rollback never undoes them. */
static int keybinding_runtime_preflight(const GscProgram *g,const GscKeyBindingHost *host,char *error,size_t size) {
    int n;
    for(n=0;n<g->code_count;n++) {
        const Instr *i=&g->code[n];
        if(i->kind!=I_CALL||i->target!=-1-KEYBINDING_TARGET_BASE-GSC_KEYBINDING_QUERY)continue;
        if(!g->keybinding_mode||i->module[0]||keybinding_descriptor(g,i->name,i->args,i->argc)!=GSC_KEYBINDING_QUERY||
           find_func(g,i->owner_module,i->name)>=0||!host||!host->query) {
            if(error&&size)snprintf(error,size,"line %d: getkeybinding requires selected copied query host",i->line);
            return 0;
        }
    }
    for(n=0;n<g->expr_count;n++) {
        const Expr *e=&g->expr[n];
        if(e->call_binding!=GSC_EXPR_TARGET_KEYBINDING_NATIVE)continue;
        if(!g->keybinding_mode||e->kind!=E_NATIVE_CALL||e->module[0]||
           keybinding_descriptor(g,e->name,e->args,e->argc)!=GSC_KEYBINDING_QUERY||e->target!=GSC_KEYBINDING_QUERY||
           find_func(g,e->owner_module,e->name)>=0||!host||!host->query) {
            if(error&&size)snprintf(error,size,"line %d: getkeybinding expression requires selected copied query host",e->line);
            return 0;
        }
    }
    return 1;
}
static int keybinding_context(const GscRuntime *r,int now) {
    return r&&r->keybinding_enabled&&r->arrays_enabled&&r->names_enabled&&r->scalar_switch_enabled&&
        !r->error[0]&&r->now_ms==now&&now>=0;
}
static int keybinding_call(GscRuntime *r,int kind,const Value *args,int argc,int line,Value *out) {
    static const char *names[3]={"key1","key2","count"};
    GscKeyBindingRequest request,reply,validate;Value result;char error[256];
    int accepted,now,n,old_arrays,old_entries;
    if(!r||!out||kind!=GSC_KEYBINDING_QUERY||argc<0||argc>1||(!args&&argc)||
       !keybinding_context(r,r->now_ms))return rfail(r,line,"invalid selected getkeybinding invocation");
    if(!argc)return rfail(r,line,"getkeybinding missing GetString argument 0");
    if(args[0].kind!=V_STRING||!keybinding_command_valid(args[0].str))
        return rfail(r,line,"getkeybinding argument 0 requires selected plain ASCII string");
    if(r->keybinding_busy)return rfail(r,line,"getkeybinding callback reentry refused");
    if(!r->keybinding.query||!r->next_keybinding_operation)return rfail(r,line,"getkeybinding query host or operation unavailable");
    now=r->now_ms;r->keybinding_busy=1;
    memset(&request,0,sizeof(request));request.abi=COD1_KEYBINDING_WIRE_ABI;request.phase=COD1_KEYBINDING_PHASE_QUERY;
    request.operation_id=r->next_keybinding_operation;request.cg_time=request.count=-1;
    memcpy(request.command,args[0].str,sizeof(request.command));
    r->next_keybinding_operation=request.operation_id==UINT64_MAX?0:request.operation_id+1;
    reply=request;error[0]=0;
    accepted=r->keybinding.query(r->keybinding.user,now,&reply,error,sizeof(error));error[sizeof(error)-1]=0;
    if(!keybinding_context(r,now))goto fail;
    if(accepted!=1){rfail(r,line,"getkeybinding query host refused: %s",error[0]?error:"unspecified host failure");goto fail;}
    if(!keybinding_reply_valid(&request,&reply)){rfail(r,line,"getkeybinding query changed intent or returned invalid copied reply");goto fail;}
    /* The actual QUERY/cache touch already happened. Both arena checks are late. */
    if(r->array_count>=GSC_ARRAY_CONTAINER_CAPACITY||r->array_entry_count>GSC_ARRAY_ENTRY_CAPACITY-3) {
        rfail(r,line,"getkeybinding array arena limit after query");goto fail;
    }
    old_arrays=r->array_count;old_entries=r->array_entry_count;
    if(!array_new(r,line,&result))goto fail;
    for(n=0;n<3;n++) {
        ArrayEntry entry;memset(&entry,0,sizeof(entry));entry.owner=(int)result.array;entry.key.kind=V_STRING;
        str_copy(entry.key.string,sizeof(entry.key.string),names[n]);
        if(n<2){entry.value.kind=V_STRING;memcpy(entry.value.str,n?reply.key2:reply.key1,sizeof(entry.value.str));}
        else {entry.value.kind=V_NUMBER;entry.value.integer=entry.value.integer_exact=1;
            entry.value.integer_value=reply.count;entry.value.number=(float)reply.count;}
        array_entry_append55(r,&entry);
    }
    r->arrays[result.array-1].count=3;r->arrays[result.array-1].keybinding=1;
    validate=reply;validate.phase=COD1_KEYBINDING_PHASE_VALIDATE;request=validate;error[0]=0;
    accepted=r->keybinding.query(r->keybinding.user,now,&validate,error,sizeof(error));error[sizeof(error)-1]=0;
    if(!keybinding_context(r,now))goto rollback;
    if(accepted!=1){rfail(r,line,"getkeybinding final host validation refused: %s",error[0]?error:"unspecified host failure");goto rollback;}
    if(!keybinding_reply_valid(&request,&validate)){rfail(r,line,"getkeybinding final validation changed copied intent or reply");goto rollback;}
    *out=result;r->keybinding_busy=0;return 1;
rollback:
    memset(&r->arrays[old_arrays],0,(size_t)(r->array_count-old_arrays)*sizeof(r->arrays[0]));
    memset(&r->array_entries[old_entries],0,(size_t)(r->array_entry_count-old_entries)*sizeof(r->array_entries[0]));
    r->array_count=old_arrays;r->array_entry_count=old_entries;array_index_rebuild55(r);
fail:
    r->keybinding_busy=0;return 0;
}
