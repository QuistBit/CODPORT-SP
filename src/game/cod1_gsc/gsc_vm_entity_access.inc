/* R85 bounded model/property access. Native names and fixture field operations
 * remain distinct: property getters are typed host callbacks, not invented
 * GSC getorigin/getangles functions or generic mutable dictionaries. */
enum { ENTITY_PROPERTY_ORIGIN=0, ENTITY_PROPERTY_ANGLES=1 };
static int entity_property_descriptor(const char *name) {
    if(!strcmp(name,"origin"))return ENTITY_PROPERTY_ORIGIN;
    if(!strcmp(name,"angles"))return ENTITY_PROPERTY_ANGLES;
    return -1;
}
static int bridge_setmodel_callback(void *user,const GscNativeValue *receiver,
    const GscNativeValue *args,int argc,GscNativeValue *out,char *error,size_t size) {
    GscRuntime *r=user;int ok;
    if(!r||!r->access_enabled||!r->access.setmodel||!receiver||!args||argc!=1||out||
       receiver->kind!=GSC_NATIVE_ENTITY||args[0].kind!=GSC_NATIVE_STRING)
        return bridge_entity_error(error,size,"invalid setmodel callback state");
    if(entity_validate(r,&receiver->entity)!=1||r->error[0])
        return bridge_entity_error(error,size,"setmodel receiver is stale or missing");
    if(!entity_task_current(r))return !r->error[0];
    ok=r->access.setmodel(r->access.user,&receiver->entity,args[0].string,error,size);
    if(!entity_task_current(r))return !r->error[0];
    if(!ok||r->error[0])return 0;
    if(r->state_enabled&&r->executing_task) {
        if(!refresh_state_context(r,r->executing_task))return 0;
        if(r->executing_task->status==TASK_DEAD)return 1; /* accepted VOID effect */
    }
    if(entity_validate(r,&receiver->entity)!=1||r->error[0])
        return bridge_entity_error(error,size,"setmodel host retired the receiver");
    return 1;
}
static int bridge_add_access(GscNativeRegistry *registry,GscRuntime *r,
                            char *error,size_t size) {
    GscNativeContract c;int index=-1;
    if(!r->access.setmodel)return 1;
    memset(&c,0,sizeof(c));c.name_space=GSC_NATIVE_METHOD;
    str_copy(c.name,sizeof(c.name),"setmodel");
    str_copy(c.provenance,sizeof(c.provenance),
        "R85 bounded fixture setmodel/setModel contract, spellings observed in training.gsc; not native ABI");
    c.known=GSC_NATIVE_KNOWN_METHOD;c.contexts=GSC_NATIVE_STATEMENT;
    c.min_args=c.max_args=1;c.argument_types[0]=GSC_NATIVE_TYPE_STRING;
    c.receiver_types=GSC_NATIVE_TYPE_ENTITY;c.result_types=0;
    if(!gsc_native_registry_add(registry,&c,&index,error,size)||
       !gsc_native_registry_bind_method(registry,index,bridge_setmodel_callback,r,error,size))return 0;
    r->native_setmodel_import=index;return 1;
}
static int bridge_setmodel(GscRuntime *r,const Value *receiver,const Value *args,
                           int argc,int line) {
    GscNativeValue recv,argument;GscNativeResult result;char error[256];
    StringArguments scope={0};const char *text;int ok;
    if(!r->access_enabled||r->native_setmodel_import<0||argc!=1)
        return rfail(r,line,"invalid setmodel binding or arity");
    if(!bridge_export_value(r,receiver,&recv,line))return 0;
    /* Legacy runtime profiles keep their frozen float-only generic bridge.
     * Exact scalar/named profiles use the owned GetString lane. Existing GAME
     * capability still restricts setModel to genuine nonsolid script_model;
     * no actor lease is adopted and no new name is registered here. */
    if(r->scalar_switch_enabled && r->names_enabled) {
        if(recv.kind!=GSC_NATIVE_ENTITY ||
           entity_validate(r,&recv.entity)!=1 || r->error[0])
            return entity_task_current(r)?rfail(r,line,"setmodel receiver is stale or missing"):0;
        if(r->state_enabled && r->executing_task){
            if(!refresh_state_context(r,r->executing_task))return 0;
            if(r->executing_task->status==TASK_DEAD)return 1;
        }
        if(!string_arguments_begin(r,args,argc,line,&scope))return 0;
        ok=string_arguments_get(r,&scope,0,line,&text);
        if(ok){memset(&argument,0,sizeof(argument));argument.kind=GSC_NATIVE_STRING;
            memcpy(argument.string,text,strlen(text)+1);}
        string_arguments_end(&scope);if(!ok)return 0;
    } else if(!bridge_export_value(r,args,&argument,line))return 0;
    if(!gsc_native_invoke_method(r->native_registry,r->native_setmodel_import,
        GSC_NATIVE_STATEMENT,&recv,&argument,1,&result,error,sizeof(error)))
        return entity_task_current(r)?rfail(r,line,"setmodel native dispatch: %s",error):0;
    if(r->error[0])return 0;
    if(result.has_value)return rfail(r,line,"setmodel must return VOID");
    return 1;
}
static int bridge_property_read(GscRuntime *r,const Value *receiver,int property,
                                int line,Value *out) {
    GscEntityRef ref;float vector[3]={NAN,NAN,NAN};char error[256];Value copy;int ok;
    if(!r->access_enabled||!receiver||receiver->kind!=V_ENTITY||!out||
       property<ENTITY_PROPERTY_ORIGIN||property>ENTITY_PROPERTY_ANGLES)
        return rfail(r,line,"property read requires an entity receiver");
    ref=receiver->entity;
    if(!ref.epoch||!ref.generation||entity_validate(r,&ref)!=1||r->error[0])
        return entity_task_current(r)?rfail(r,line,"property receiver is stale or missing"):0;
    if(!entity_task_current(r))return 0;
    memset(error,0,sizeof(error));
    if(property==ENTITY_PROPERTY_ORIGIN) {
        if(!r->access.getorigin)return rfail(r,line,"origin read capability is unavailable");
        ok=r->access.getorigin(r->access.user,&ref,vector,error,sizeof(error));
    } else {
        if(!r->access.getangles)return rfail(r,line,"angles read capability is unavailable");
        ok=r->access.getangles(r->access.user,&ref,vector,error,sizeof(error));
    }
    error[sizeof(error)-1]=0;
    if(!entity_task_current(r))return 0;
    if(!ok||r->error[0])return rfail(r,line,"property read: %s",error[0]?error:"host callback failed");
    if(!isfinite(vector[0])||!isfinite(vector[1])||!isfinite(vector[2]))
        return rfail(r,line,"property getter must return a finite three-component vector");
    /* R88 successful reads can retire an active receiver context. Cancel that
     * task before its continuation; unrelated tasks remain runnable. Earlier
     * modes and stale non-context property receivers keep their strict errors. */
    if(r->state_enabled&&r->executing_task) {
        if(!refresh_state_context(r,r->executing_task))return 0;
        if(r->executing_task->status==TASK_DEAD)return 0;
    }
    if(entity_validate(r,&ref)!=1||r->error[0])
        return entity_task_current(r)?rfail(r,line,"property getter retired the receiver"):0;
    memset(&copy,0,sizeof(copy));copy.kind=V_VECTOR;
    memcpy(copy.vector,vector,sizeof(copy.vector));*out=copy;return 1;
}
static int bridge_property_write(GscRuntime *r,const Value *receiver,int property,
                                 const Value *value,int line) {
    GscNativeValue recv,argument;char error[256];int ok;
    if(!r->access_enabled||!receiver||!value||
       property<ENTITY_PROPERTY_ORIGIN||property>ENTITY_PROPERTY_ANGLES)
        return rfail(r,line,"invalid entity property assignment");
    if(property==ENTITY_PROPERTY_ORIGIN) {
        if(!bridge_setorigin(r,receiver,value,1,line))return 0;
        if(r->state_enabled&&r->executing_task) {
            if(!refresh_state_context(r,r->executing_task))return 0;
            if(r->executing_task->status==TASK_DEAD)return 0;
        }
        if(entity_validate(r,&receiver->entity)!=1||r->error[0])
            return entity_task_current(r)?rfail(r,line,"origin writer retired the receiver"):0;
        return 1;
    }
    if(!r->access.setangles)return rfail(r,line,"angles write capability is unavailable");
    if(!bridge_export_value(r,receiver,&recv,line)||
       !bridge_export_value(r,value,&argument,line))return 0;
    if(recv.kind!=GSC_NATIVE_ENTITY||argument.kind!=GSC_NATIVE_VECTOR)
        return rfail(r,line,"angles assignment requires an entity and vector");
    if(entity_validate(r,&recv.entity)!=1||r->error[0])
        return entity_task_current(r)?rfail(r,line,"angles receiver is stale or missing"):0;
    if(!entity_task_current(r))return 0;
    memset(error,0,sizeof(error));
    ok=r->access.setangles(r->access.user,&recv.entity,argument.vector,error,sizeof(error));
    error[sizeof(error)-1]=0;
    if(!entity_task_current(r))return 0;
    if(!ok||r->error[0])return rfail(r,line,"angles write: %s",error[0]?error:"host callback failed");
    if(r->state_enabled&&r->executing_task) {
        if(!refresh_state_context(r,r->executing_task))return 0;
        if(r->executing_task->status==TASK_DEAD)return 0;
    }
    if(entity_validate(r,&recv.entity)!=1||r->error[0])
        return entity_task_current(r)?rfail(r,line,"angles writer retired the receiver"):0;
    return 1;
}
