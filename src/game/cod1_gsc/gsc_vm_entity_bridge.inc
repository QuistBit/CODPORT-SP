/* R84 explicitly selected fixture host operations. These descriptors establish
 * this adapter contract only, not the original CoD1 native ABI or signatures. */
static int bridge_entity_error(char *error,size_t size,const char *text) {
    if(error&&size)snprintf(error,size,"%s",text);
    return 0;
}
static int bridge_getent_callback(void *user,const GscNativeValue *args,int argc,
    GscNativeValue *out,char *error,size_t size) {
    GscRuntime *r=user;GscEntityRef ref;int found=-1;
    if(!r||!r->entities_enabled||!args||argc!=2||!out||
       args[0].kind!=GSC_NATIVE_STRING||args[1].kind!=GSC_NATIVE_STRING)
        return bridge_entity_error(error,size,"invalid getent callback state");
    memset(&ref,0,sizeof(ref));
    if(!r->entities.lookup(r->entities.user,args[0].string,args[1].string,
                          &ref,&found,error,size)||r->error[0])return 0;
    if(!entity_task_current(r))return 0;
    if(found!=0&&found!=1)
        return bridge_entity_error(error,size,"getent host returned invalid found flag");
    memset(out,0,sizeof(*out));out->kind=GSC_NATIVE_UNDEFINED;
    if(!found)return 1;
    if(!ref.epoch||!ref.generation||
       entity_validate(r,&ref)!=1||r->error[0])
        return bridge_entity_error(error,size,"getent host returned invalid or stale entity");
    out->kind=GSC_NATIVE_ENTITY;out->entity=ref;return 1;
}
static int bridge_setorigin_callback(void *user,const GscNativeValue *receiver,
    const GscNativeValue *args,int argc,GscNativeValue *out,char *error,size_t size) {
    GscRuntime *r=user;int ok;
    if(!r||!r->entities_enabled||!receiver||!args||argc!=1||out||
       receiver->kind!=GSC_NATIVE_ENTITY||args[0].kind!=GSC_NATIVE_VECTOR)
        return bridge_entity_error(error,size,"invalid setorigin callback state");
    if(entity_validate(r,&receiver->entity)!=1||r->error[0])
        return bridge_entity_error(error,size,"setorigin receiver is stale or missing");
    if(!entity_task_current(r))return !r->error[0];
    /* The host must also revalidate at its actual mutation boundary. */
    if(r->controls_enabled) {
        if(!r->controls.setorigin)
            return bridge_entity_error(error,size,"control origin capability is unavailable");
        ok=r->controls.setorigin(r->controls.user,&receiver->entity,args[0].vector,error,size);
    } else ok=r->entities.setorigin(r->entities.user,&receiver->entity,args[0].vector,error,size);
    if(!entity_task_current(r))return !r->error[0];
    if(!ok||r->error[0])return 0;
    /* A successful effect stays accepted even if it retires an active R88
     * context. The scheduler then cancels that task without registry failure. */
    if(r->state_enabled&&r->executing_task) {
        if(!refresh_state_context(r,r->executing_task))return 0;
        if(r->executing_task->status==TASK_DEAD)return 1;
        if(entity_validate(r,&receiver->entity)!=1||r->error[0])
            return bridge_entity_error(error,size,"setorigin host retired the receiver");
    }
    return 1;
}
static int bridge_spawn_callback(void *user,const GscNativeValue *args,int argc,
    GscNativeValue *out,char *error,size_t size) {
    GscRuntime *r=user;GscEntityRef ref;
    if(!r||!r->entities_enabled||!r->entities.spawn||!args||argc!=2||!out||
       args[0].kind!=GSC_NATIVE_STRING||args[1].kind!=GSC_NATIVE_VECTOR)
        return bridge_entity_error(error,size,"invalid spawn callback state");
    memset(&ref,0,sizeof(ref));
    if(!r->entities.spawn(r->entities.user,args[0].string,args[1].vector,&ref,error,size)||
       r->error[0])return 0;
    if(!entity_task_current(r))return 0;
    if(!ref.epoch||!ref.generation||
       entity_validate(r,&ref)!=1||r->error[0])
        return bridge_entity_error(error,size,"spawn host returned invalid or stale entity");
    memset(out,0,sizeof(*out));out->kind=GSC_NATIVE_ENTITY;out->entity=ref;return 1;
}
static int bridge_add_entities(GscNativeRegistry *registry,GscRuntime *r,
                               char *error,size_t size) {
    GscNativeContract c;int global=-1,method=-1,spawn_index=-1;
    memset(&c,0,sizeof(c));c.name_space=GSC_NATIVE_GLOBAL;
    str_copy(c.name,sizeof(c.name),"getent");
    str_copy(c.provenance,sizeof(c.provenance),
        "R84 explicitly enabled fixture entity lookup contract; not original CoD1 ABI/signature");
    c.known=GSC_NATIVE_KNOWN_GLOBAL;c.contexts=GSC_NATIVE_EXPRESSION;
    c.min_args=c.max_args=2;c.argument_types[0]=c.argument_types[1]=GSC_NATIVE_TYPE_STRING;
    c.result_types=GSC_NATIVE_TYPE_UNDEFINED|GSC_NATIVE_TYPE_ENTITY;
    if(!gsc_native_registry_add(registry,&c,&global,error,size)||
       !gsc_native_registry_bind_global(registry,global,bridge_getent_callback,r,error,size))return 0;
    memset(&c,0,sizeof(c));c.name_space=GSC_NATIVE_METHOD;
    str_copy(c.name,sizeof(c.name),"setorigin");
    str_copy(c.provenance,sizeof(c.provenance),
        "R84 explicitly enabled fixture entity position contract; not original CoD1 ABI/signature");
    c.known=GSC_NATIVE_KNOWN_METHOD;c.contexts=GSC_NATIVE_STATEMENT;
    c.min_args=c.max_args=1;c.argument_types[0]=GSC_NATIVE_TYPE_VECTOR;
    c.receiver_types=GSC_NATIVE_TYPE_ENTITY;c.result_types=0;
    if(!gsc_native_registry_add(registry,&c,&method,error,size)||
       !gsc_native_registry_bind_method(registry,method,bridge_setorigin_callback,r,error,size))return 0;
    if(r->entities.spawn) {
        memset(&c,0,sizeof(c));c.name_space=GSC_NATIVE_GLOBAL;
        str_copy(c.name,sizeof(c.name),"spawn");
        str_copy(c.provenance,sizeof(c.provenance),
            "R84 bounded fixture spawn contract, argument shape observed in training.gsc; not original native ABI");
        c.known=GSC_NATIVE_KNOWN_GLOBAL;c.contexts=GSC_NATIVE_EXPRESSION;
        c.min_args=c.max_args=2;c.argument_types[0]=GSC_NATIVE_TYPE_STRING;
        c.argument_types[1]=GSC_NATIVE_TYPE_VECTOR;c.result_types=GSC_NATIVE_TYPE_ENTITY;
        if(!gsc_native_registry_add(registry,&c,&spawn_index,error,size)||
           !gsc_native_registry_bind_global(registry,spawn_index,bridge_spawn_callback,r,error,size))return 0;
    }
    r->native_spawn_import=spawn_index;
    r->native_getent_import=global;r->native_setorigin_import=method;return 1;
}
static int bridge_getent(GscRuntime *r,const Value *args,int argc,int line,Value *out) {
    GscNativeValue arguments[2];GscNativeResult result;Value copy;char error[256];int n;
    if(!r->entities_enabled||r->native_getent_import<0||argc<0||argc>2||!out)
        return rfail(r,line,"invalid getent binding or arity");
    /* eval has already evaluated all supplied argument expressions. Original
     * GAME 20041f60 reads GetConstString(0), then GetString(1), without a
     * default lookup field. Missing inputs fail here before any host lookup. */
    if(argc==0)return rfail(r,line,"getent parameter 1 does not exist");
    if(argc==1) {
        if(args[0].kind!=V_STRING)
            return rfail(r,line,"getent parameter 1 requires a string");
        return rfail(r,line,"getent parameter 2 does not exist");
    }
    for(n=0;n<2;n++)if(!bridge_export_value(r,&args[n],&arguments[n],line))return 0;
    if(!gsc_native_invoke_global(r->native_registry,r->native_getent_import,
        GSC_NATIVE_EXPRESSION,arguments,2,&result,error,sizeof(error)))
        return entity_task_current(r)?rfail(r,line,"getent native dispatch: %s",error):0;
    if(r->error[0])return 0;
    if(result.has_value!=1||(result.value.kind!=GSC_NATIVE_UNDEFINED&&
                            result.value.kind!=GSC_NATIVE_ENTITY))
        return rfail(r,line,"invalid getent result");
    memset(&copy,0,sizeof(copy));
    if(result.value.kind==GSC_NATIVE_ENTITY) {
        copy.kind=V_ENTITY;copy.entity=result.value.entity;
    }
    *out=copy;return 1;
}
static int bridge_spawn(GscRuntime *r,const Value *args,int argc,int flags_expr,
                        int line,Value *out) {
    GscNativeValue arguments[2];GscNativeResult result;Value copy;char error[256];int n;
    if(!r->entities_enabled||r->native_spawn_import<0||(argc!=2&&argc!=3)||!out)
        return rfail(r,line,"invalid spawn binding or arity");
    for(n=0;n<2;n++)if(!bridge_export_value(r,&args[n],&arguments[n],line))return 0;
    if(argc==3) {
        GscEntityRef ref;int32_t flags;int accepted;
        /* All supplied expressions were evaluated by eval. The admitted flag
         * expression is pure literal syntax; retain its exact integer payload
         * rather than rounding it through GscNativeValue's legacy float. */
        if(arguments[0].kind!=GSC_NATIVE_STRING||arguments[1].kind!=GSC_NATIVE_VECTOR||
           !entity_spawn_flags_literal(r->program,flags_expr,&flags)||
           !r->entities.spawn_with_flags)
            return rfail(r,line,"invalid selected spawn flags arguments or capability");
        memset(&ref,0,sizeof(ref));error[0]=0;
        accepted=r->entities.spawn_with_flags(r->entities.user,arguments[0].string,
            arguments[1].vector,flags,&ref,error,sizeof(error));
        error[sizeof(error)-1]=0;
        if(r->error[0])return 0;
        if(!entity_task_current(r))return 0;
        if(accepted!=1)return rfail(r,line,"spawn flags host refused: %s",error);
        if(!ref.epoch||!ref.generation||entity_validate(r,&ref)!=1||r->error[0])
            return entity_task_current(r)?rfail(r,line,"spawn flags host returned invalid or stale entity"):0;
        memset(&copy,0,sizeof(copy));copy.kind=V_ENTITY;copy.entity=ref;*out=copy;return 1;
    }
    if(!gsc_native_invoke_global(r->native_registry,r->native_spawn_import,
        GSC_NATIVE_EXPRESSION,arguments,2,&result,error,sizeof(error)))
        return entity_task_current(r)?rfail(r,line,"spawn native dispatch: %s",error):0;
    if(r->error[0])return 0;
    if(result.has_value!=1||result.value.kind!=GSC_NATIVE_ENTITY)
        return rfail(r,line,"invalid spawn result");
    memset(&copy,0,sizeof(copy));copy.kind=V_ENTITY;copy.entity=result.value.entity;
    *out=copy;return 1;
}
static int bridge_setorigin(GscRuntime *r,const Value *receiver,const Value *args,
                            int argc,int line) {
    GscNativeValue recv,argument;GscNativeResult result;char error[256];
    if(!r->entities_enabled||r->native_setorigin_import<0||argc!=1)
        return rfail(r,line,"invalid setorigin binding or arity");
    if(!bridge_export_value(r,receiver,&recv,line)||
       !bridge_export_value(r,args,&argument,line))return 0;
    if(!gsc_native_invoke_method(r->native_registry,r->native_setorigin_import,
        GSC_NATIVE_STATEMENT,&recv,&argument,1,&result,error,sizeof(error)))
        return entity_task_current(r)?rfail(r,line,"setorigin native dispatch: %s",error):0;
    if(r->error[0])return 0;
    if(result.has_value)return rfail(r,line,"setorigin must return VOID");
    return 1;
}
