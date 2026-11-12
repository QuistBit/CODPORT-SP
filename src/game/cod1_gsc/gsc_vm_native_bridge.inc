/* R74: private bridge for the EXISTING fixture isdefined and fog builtins.
 * Not a public native-registration hook, original CoD1 ABI, or precedence rule.
 * The owner is GscRuntime; its registry and callback context die together.
 * Included once after rfail, before builtin_isdefined.
 */
static void bridge_count(unsigned long long *counter) {
    if(*counter<ULLONG_MAX)(*counter)++;
}

/* Copy only meaningful fields; no numeric tag aliasing.
 * A malformed value is refused before the dispatcher sees it. Unused fields
 * are canonicalized, NOT interpreted as a second payload. */
static int bridge_export_value(GscRuntime *r,const Value *value,
                              GscNativeValue *out,int line) {
    GscNativeValue copy;const char *end;size_t length;
    if(r->entity_prepare_busy||r->entity_certificate_busy)return rfail(r,line,"native dispatch during guarded host callback");
    if(!value||!out)return rfail(r,line,"invalid isdefined bridge value");
    memset(&copy,0,sizeof(copy));
    switch(value->kind) {
      case V_UNDEFINED:copy.kind=GSC_NATIVE_UNDEFINED;break;
      case V_NUMBER:
        if(!scalar_native_exact(r,value,line))return 0;
        if(!isfinite(value->number)||(value->integer!=0&&value->integer!=1)||
           (value->integer&&truncf(value->number)!=value->number))
            return rfail(r,line,"invalid numeric value in isdefined bridge");
        copy.kind=GSC_NATIVE_NUMBER;copy.number=value->number;
        copy.integer=value->integer;break;
      case V_STRING:
        end=memchr(value->str,0,sizeof(value->str));
        if(!end)return rfail(r,line,"unterminated string in isdefined bridge");
        length=(size_t)(end-value->str);
        if(length>=sizeof(copy.string))return rfail(r,line,"string exceeds isdefined bridge capacity");
        copy.kind=GSC_NATIVE_STRING;memcpy(copy.string,value->str,length+1);break;
      case V_VECTOR:
        if(!r->entities_enabled||!isfinite(value->vector[0])||
           !isfinite(value->vector[1])||!isfinite(value->vector[2]))
            return rfail(r,line,"invalid vector bridge value");
        copy.kind=GSC_NATIVE_VECTOR;memcpy(copy.vector,value->vector,sizeof(copy.vector));break;
      case V_ENTITY:
        if(!r->entities_enabled||!value->entity.epoch||!value->entity.generation)
            return rfail(r,line,"invalid entity bridge value");
        copy.kind=GSC_NATIVE_ENTITY;copy.entity=value->entity;break;
      case V_STRUCT:
        if(!struct_value_valid(r,value))return rfail(r,line,"invalid structure bridge value");
        copy.kind=GSC_NATIVE_STRUCT;copy.object.runtime_token=value->object_token;copy.object.handle=value->object;break;
      case V_ARRAY:
        if(!array_value_valid(r,value->array))return rfail(r,line,"invalid array bridge value");
        copy.kind=GSC_NATIVE_ARRAY;copy.array.runtime_token=r->array_token;
        copy.array.handle=value->array;break;
      default:return rfail(r,line,"unsupported isdefined bridge value type");
    }
    *out=copy;return 1;
}

static int bridge_import_isdefined(GscRuntime *r,const GscNativeResult *result,
                                   Value *out,int line) {
    Value copy;
    /* This bridge promises the old fixture's NUMBER, integer-tagged 0 or 1.
     * VOID, undefined, strings, fractional/nonfinite and other numbers cannot
     * silently become a predicate result. Caller output stays intact on error. */
    if(!out||!result||result->has_value!=1||result->value.kind!=GSC_NATIVE_NUMBER||
       result->value.integer!=1||
       (result->value.number!=0.0f&&result->value.number!=1.0f))
        return rfail(r,line,"invalid isdefined bridge result");
    memset(&copy,0,sizeof(copy));copy.kind=V_NUMBER;copy.integer=1;
    copy.number=result->value.number;*out=copy;return 1;
}

static int bridge_isdefined_callback(void *user,const GscNativeValue *args,
                                    int argc,GscNativeValue *out,
                                    char *error,size_t error_size) {
    GscRuntime *r=user;
    if(r&&(r->entity_prepare_busy||r->entity_certificate_busy))return rfail(r,0,"native dispatch during guarded host callback");
    if(!r||!args||argc!=1||!out||args[0].kind<GSC_NATIVE_UNDEFINED||
       args[0].kind>(r->structs_enabled?GSC_NATIVE_STRUCT:r->arrays_enabled?GSC_NATIVE_ARRAY:r->entities_enabled?GSC_NATIVE_ENTITY:GSC_NATIVE_STRING)) {
        if(error&&error_size)snprintf(error,error_size,"invalid private isdefined callback state");
        return 0;
    }
    if(args[0].kind==GSC_NATIVE_ARRAY&&
       (args[0].array.runtime_token!=r->array_token||!array_value_valid(r,args[0].array.handle))) {
        if(error&&error_size)snprintf(error,error_size,"invalid private array provenance");
        return 0;
    }
    if(args[0].kind==GSC_NATIVE_STRUCT&&
       (!r->structs_enabled||args[0].object.runtime_token!=r->array_token||
        !args[0].object.handle||args[0].object.handle>r->struct_count)) {
        if(error&&error_size)snprintf(error,error_size,"invalid private structure provenance");
        return 0;
    }
    /* Count INSIDE the real registered callback, not in its caller. */
    bridge_count(&r->native_stats.isdefined_callbacks);
    memset(out,0,sizeof(*out));out->kind=GSC_NATIVE_NUMBER;out->integer=1;
    out->number=(float)(args[0].kind!=GSC_NATIVE_UNDEFINED);
    if(args[0].kind==GSC_NATIVE_ENTITY)
        out->number=(float)(entity_validate(r,&args[0].entity)==1);
    return !r->error[0];
}

/* The fog handlers use transition_ms below in the core. No public caller can
 * inject or access this private registry. Source line belongs to the current
 * single, non-reentrant fog invocation and is cleared after dispatch. */
static int bridge_fog_callback(GscRuntime *r,const GscNativeValue *args,
                              int argc,GscNativeValue *out,int exp,
                              char *error,size_t error_size);
static int bridge_exp_fog_callback(void *user,const GscNativeValue *args,int argc,
                                  GscNativeValue *out,char *error,size_t size) {
    return bridge_fog_callback(user,args,argc,out,1,error,size);
}
static int bridge_cull_fog_callback(void *user,const GscNativeValue *args,int argc,
                                   GscNativeValue *out,char *error,size_t size) {
    return bridge_fog_callback(user,args,argc,out,0,error,size);
}
static int bridge_add_fog(GscNativeRegistry *registry,GscRuntime *r,int exp,
                          int *index,int *builtin,char *error,size_t size) {
    const char *name=exp?"setExpFog":"setCullFog";
    int n,arity=exp?5:6;GscNativeContract c;
    *builtin=find_builtin(name);
    if(*builtin<0||builtins[*builtin].context!=B_STATEMENT||
       builtins[*builtin].arity!=arity||builtins[*builtin].result_kind!=B_RET_VOID||
       builtins[*builtin].effect!=(exp?B_EFFECT_EXP_FOG:B_EFFECT_CULL_FOG)) {
        if(error&&size)snprintf(error,size,"invalid fixture fog descriptor");
        return 0;
    }
    memset(&c,0,sizeof(c));c.name_space=GSC_NATIVE_GLOBAL;
    str_copy(c.name,sizeof(c.name),name);
    str_copy(c.provenance,sizeof(c.provenance),
        "R74 migration of existing R73 fixture fog output; NOT original native ABI/signature");
    c.known=GSC_NATIVE_KNOWN_GLOBAL;c.contexts=GSC_NATIVE_STATEMENT;
    c.min_args=c.max_args=arity;c.result_types=0; /* VOID, not undefined */
    for(n=0;n<arity;n++)c.argument_types[n]=GSC_NATIVE_TYPE_NUMBER;
    return gsc_native_registry_add(registry,&c,index,error,size)&&
        gsc_native_registry_bind_global(registry,*index,
            exp?bridge_exp_fog_callback:bridge_cull_fog_callback,r,error,size);
}

#include "gsc_vm_entity_bridge.inc"
#include "gsc_vm_entity_access.inc"
#include "gsc_vm_entity_controls.inc"

static int bridge_init(GscRuntime *r,char *error,size_t error_size) {
    GscNativeContract c;GscNativeRegistry *registry;int index=-1,builtin;
    int exp_index=-1,cull_index=-1,exp_builtin=-1,cull_builtin=-1;
    builtin=find_builtin("isdefined");
    if(builtin<0||builtins[builtin].context!=B_EXPRESSION||
       builtins[builtin].arity!=1||builtins[builtin].result_kind!=V_NUMBER) {
        if(error&&error_size)snprintf(error,error_size,"invalid fixture isdefined descriptor");
        return 0;
    }
    registry=gsc_native_registry_new();
    if(!registry) {
        if(error&&error_size)snprintf(error,error_size,"native registry allocation failed");
        return 0;
    }
    memset(&c,0,sizeof(c));c.name_space=GSC_NATIVE_GLOBAL;
    str_copy(c.name,sizeof(c.name),"isdefined");
    str_copy(c.provenance,sizeof(c.provenance),
        "R73 migration of R72 fixture isdefined scalar predicate; not original CoD1 ABI/signature");
    c.known=GSC_NATIVE_KNOWN_GLOBAL;c.contexts=GSC_NATIVE_EXPRESSION;
    c.min_args=c.max_args=1;c.argument_types[0]=r->entities_enabled?
        (r->structs_enabled?GSC_NATIVE_TYPE_ALL_WITH_STRUCTS:r->arrays_enabled?GSC_NATIVE_TYPE_ALL_WITH_ARRAYS:GSC_NATIVE_TYPE_ALL):GSC_NATIVE_TYPE_SCALAR;
    c.result_types=GSC_NATIVE_TYPE_NUMBER;
    if(!gsc_native_registry_add(registry,&c,&index,error,error_size)||
       !gsc_native_registry_bind_global(registry,index,bridge_isdefined_callback,r,error,error_size)||
       !bridge_add_fog(registry,r,1,&exp_index,&exp_builtin,error,error_size)||
       !bridge_add_fog(registry,r,0,&cull_index,&cull_builtin,error,error_size)||
       (r->entities_enabled&&!bridge_add_entities(registry,r,error,error_size))||
       (r->access_enabled&&!bridge_add_access(registry,r,error,error_size))||
       (r->controls_enabled&&!bridge_add_controls(registry,r,error,error_size))||
       !gsc_native_registry_seal(registry,error,error_size)) {
        gsc_native_registry_free(registry);return 0;
    }
    /* Commit only a sealed, fully bound private registry. A builtin table ID
     * is never passed to the native registry. Here the indexes happen to be
     * builtin 2 vs import 0, but neither number is hardcoded into dispatch. */
    r->native_registry=registry;r->native_isdefined_import=index;
    r->native_stats.isdefined_builtin_index=builtin;
    r->native_stats.isdefined_import_index=index;
    r->native_exp_fog_import=exp_index;r->native_cull_fog_import=cull_index;
    r->fog_native_stats.exp_builtin_index=exp_builtin;
    r->fog_native_stats.exp_import_index=exp_index;
    r->fog_native_stats.cull_builtin_index=cull_builtin;
    r->fog_native_stats.cull_import_index=cull_index;return 1;
}

static int bridge_isdefined(GscRuntime *r,const Builtin *b,const Value *args,
                            int line,Value *out) {
    GscNativeValue argument;GscNativeResult result;char error[256];
    if(!r->native_registry||r->native_isdefined_import<0||
       r->native_stats.isdefined_builtin_index<0||
       r->native_stats.isdefined_builtin_index>=BUILTIN_COUNT||
       b!=&builtins[r->native_stats.isdefined_builtin_index])
        return rfail(r,line,"invalid isdefined bridge binding");
    if(r->error[0])return 0;
    if(!bridge_export_value(r,args,&argument,line))return 0;
    bridge_count(&r->native_stats.isdefined_dispatches);
    if(!gsc_native_invoke_global(r->native_registry,r->native_isdefined_import,
        GSC_NATIVE_EXPRESSION,&argument,1,&result,error,sizeof(error)))
        return rfail(r,line,"isdefined native dispatch: %s",error);
    if(r->error[0])return 0;
    if(!bridge_import_isdefined(r,&result,out,line))return 0;
    bridge_count(&r->native_stats.isdefined_completed);return 1;
}

int gsc_runtime_native_stats(const GscRuntime *r,GscRuntimeNativeStats *out) {
    if(!r||!out||!r->native_registry)return 0;
    *out=r->native_stats;return 1;
}
