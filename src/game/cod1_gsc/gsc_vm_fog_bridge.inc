/* R74 private host effect path. Included after transition_ms. The scheduler,
 * parser, domain checks, formatting/rounding and user callback contract are
 * inherited from R73; no original native ABI or new rendering is claimed. */
static int bridge_fog_callback(GscRuntime *r,const GscNativeValue *args,
                              int argc,GscNativeValue *out,int exp,
                              char *error,size_t error_size) {
    GscFogEvent event;float a[MAX_PARAMS];int n,line;
    if(!r||!r->native_fog_active||r->error[0]||!args||out||argc!=(exp?5:6)) {
        if(error&&error_size)snprintf(error,error_size,"invalid private fog callback state");
        return 0;
    }
    line=r->native_fog_line;
    for(n=0;n<argc;n++) {
        if(args[n].kind!=GSC_NATIVE_NUMBER||!isfinite(args[n].number)) {
            if(error&&error_size)snprintf(error,error_size,"invalid private fog numeric argument");
            return 0;
        }
        a[n]=args[n].number;
    }
    /* Actual C handler reached. Domain failures count as handler entries,
     * never as completed effects or user callback calls. */
    bridge_count(exp?&r->fog_native_stats.exp_callbacks:&r->fog_native_stats.cull_callbacks);
    if(exp) {
        event.near_dist=0;event.far_dist=1;event.density=a[0];
        event.red=a[1];event.green=a[2];event.blue=a[3];
        if(!(event.density>0.0f&&event.density<1.0f))
            return rfail(r,line,"setExpFog density must be within (0,1)");
        if(!transition_ms(r,a[4],line,&event.transition_ms))return 0;
    } else {
        event.near_dist=a[0];event.far_dist=a[1];event.density=1;
        event.red=a[2];event.green=a[3];event.blue=a[4];
        if(!(event.near_dist>=0.0f&&event.far_dist>event.near_dist&&
             (double)event.far_dist<2147483648.0))
            return rfail(r,line,"setCullFog distance range");
        if(!transition_ms(r,a[5],line,&event.transition_ms))return 0;
    }
    if(!(event.red>=0.0f&&event.red<=1.0f&&event.green>=0.0f&&
         event.green<=1.0f&&event.blue>=0.0f&&event.blue<=1.0f))
        return rfail(r,line,"fog color outside [0,1]");
    if(!r->fog)return rfail(r,line,"fog callback required");
    bridge_count(&r->fog_native_stats.output_calls);
    if(!r->fog(r->user,&event))return rfail(r,line,"fog callback rejected event");
    bridge_count(&r->fog_native_stats.output_accepted);
    /* Preserve the first error from attempts to re-enter the scheduler. */
    if(r->error[0])return 0;
    return 1;
}

static int bridge_import_void(GscRuntime *r,const GscNativeResult *result,int line) {
    if(!result||result->has_value!=0)
        return rfail(r,line,"invalid fog bridge VOID result");
    return 1;
}

static int builtin_fog(GscRuntime *r,const Builtin *b,const Value *args,int line) {
    GscNativeValue values[6];GscNativeResult result;char error[256];
    int exp,index,builtin,n,ok;
    if(!r||!b)return 0;
    exp=b->effect==B_EFFECT_EXP_FOG;
    if(!exp&&b->effect!=B_EFFECT_CULL_FOG)
        return rfail(r,line,"invalid fog builtin effect");
    if(b->arity!=(exp?5:6))return rfail(r,line,"invalid fog builtin arity");
    index=exp?r->native_exp_fog_import:r->native_cull_fog_import;
    builtin=exp?r->fog_native_stats.exp_builtin_index:r->fog_native_stats.cull_builtin_index;
    if(!r->native_registry||index<0||builtin<0||builtin>=BUILTIN_COUNT||b!=&builtins[builtin])
        return rfail(r,line,"invalid fog bridge binding");
    if(r->error[0])return 0;
    if(r->native_fog_active)return rfail(r,line,"reentrant fog bridge call");
    if(!args)return rfail(r,line,"missing fog bridge arguments");
    memset(values,0,sizeof(values));
    for(n=0;n<b->arity;n++) {
        if(args[n].kind!=V_NUMBER)return rfail(r,line,"fog arguments must be numeric");
        /* Parser/evaluator already require finite, well-tagged numbers.
         * Corrupt internal Values are refused, not cast into a native ABI. */
        if(!isfinite(args[n].number)||(args[n].integer!=0&&args[n].integer!=1)||
           (args[n].integer&&truncf(args[n].number)!=args[n].number))
            return rfail(r,line,"invalid numeric value in fog bridge");
        values[n].kind=GSC_NATIVE_NUMBER;values[n].integer=args[n].integer;
        values[n].number=args[n].number;
        /* Original fog access is floating: conversion of an exact script int
         * to float32 is intentional and does not claim an integer export. */
        if(r->scalar_switch_enabled) {
            if(!scalar_float(r,&args[n],line,&values[n].number))return 0;
            values[n].integer=0;
        }
    }
    r->native_fog_line=line;r->native_fog_active=1;
    bridge_count(exp?&r->fog_native_stats.exp_dispatches:&r->fog_native_stats.cull_dispatches);
    ok=gsc_native_invoke_global(r->native_registry,index,GSC_NATIVE_STATEMENT,
                               values,b->arity,&result,error,sizeof(error));
    /* Clear in-flight context on every dispatcher outcome. No stale source
     * line/context leaks into the next call, including refused/reentrant calls. */
    r->native_fog_active=0;r->native_fog_line=0;
    if(!ok)return rfail(r,line,"fog native dispatch: %s",error);
    if(r->error[0]||!bridge_import_void(r,&result,line))return 0;
    bridge_count(exp?&r->fog_native_stats.exp_completed:&r->fog_native_stats.cull_completed);
    return 1;
}

int gsc_runtime_fog_native_stats(const GscRuntime *r,GscRuntimeFogNativeStats *out) {
    if(!r||!out||!r->native_registry)return 0;
    *out=r->fog_native_stats;return 1;
}
