/* R133 selected literal-int32 randomint lane. Binding accepts only an int32
 * literal or one unary-minus integer literal (and argc0 for the missing getter
 * error). Other source expressions remain unresolved, even if they sometimes
 * produce an integer. No generalized GetInt float/string/FPU policy is assumed.
 * Original positive bound uses low32 IMUL and SAR15; it is not modulo/scaled64.
 * State commits in the typed GAME host before result publication. A later
 * callback, assignment or scheduler refusal never restores the consumed draw. */
static int randomint_runtime_preflight(const GscProgram *g,
    const GscRandomIntHost *host,char *error,size_t size) {
    int n;
    for(n=0;n<g->code_count;n++) {
        const Instr *i=&g->code[n];
        if(i->kind!=I_CALL || i->target>=-1-RANDOMINT_TARGET_BASE ||
           i->target<-1-RANDOMFLOAT_TARGET_BASE)continue;
        if(!g->randomint_mode || i->module[0] ||
           randomint_descriptor(g,i->name,i->args,i->argc)!=GSC_RANDOMINT_DRAW ||
           i->target!=-1-RANDOMINT_TARGET_BASE-GSC_RANDOMINT_DRAW ||
           find_func(g,i->owner_module,i->name)>=0 || !host || !host->draw) {
            if(error&&size)snprintf(error,size,"line %d: randomint requires its selected literal-int32 draw host",i->line);
            return 0;
        }
    }
    for(n=0;n<g->expr_count;n++) {
        const Expr *e=&g->expr[n];
        if(e->call_binding!=GSC_EXPR_TARGET_RANDOMINT_NATIVE)continue;
        if(!g->randomint_mode || e->kind!=E_NATIVE_CALL || e->module[0] ||
           randomint_descriptor(g,e->name,e->args,e->argc)!=GSC_RANDOMINT_DRAW ||
           e->target!=GSC_RANDOMINT_DRAW || find_func(g,e->owner_module,e->name)>=0 ||
           !host || !host->draw) {
            if(error&&size)snprintf(error,size,"line %d: randomint expression requires its selected literal-int32 draw host",e->line);
            return 0;
        }
    }
    return 1;
}
static int randomint_context(const GscRuntime *r,int now) {
    return r && r->randomint_enabled && r->names_enabled &&
        r->scalar_switch_enabled && r->program &&
        r->program->computed_randomint_mode==r->computed_randomint_enabled &&
        r->program->dynamic_randomint_mode==r->dynamic_randomint_enabled &&
        r->program->numeric_randomint_mode==r->numeric_randomint_enabled &&
        r->program->dynamic_randomfloat_mode==r->dynamic_randomfloat_enabled &&
        !r->error[0] && r->now_ms==now && now>=0;
}
static int randomint_call(GscRuntime *r,int kind,const Value *args,int argc,
    int line,Value *out) {
    GscRandomIntRequest request,reply;Value result,converted;char error[256];
    int32_t bound,value;uint32_t product;int accepted,now;
    if(!r || !r->randomint_enabled || !r->names_enabled || !r->scalar_switch_enabled ||
       r->error[0] || !out || kind!=GSC_RANDOMINT_DRAW || argc<0 || argc>(r->dynamic_randomint_enabled?MAX_PARAMS:1) ||
       (!args&&argc))return rfail(r,line,"invalid selected literal-int32 randomint invocation");
    /* GetInt and the signed JG guard precede the draw host. Inline Value copies
     * have no retail reference-counter/heap argument-release side effects. */
    if(!argc)return rfail(r,line,"randomint missing GetInt argument 0");
    if(r->numeric_randomint_enabled) {
        /* Original CastInt type4 loads binary32 and calls 0x52e218. Its x87
         * rounded-int64/residual correction produces truncation toward zero
         * for finite inputs with an int32 result, independently of PC/RC with
         * masked exceptions. Reuse the bounded scalar conversion; it keeps
         * integer payloads exact and rejects nonnumeric/invalid inputs before
         * the positive guard or GAME state draw. FPU status is not emulated. */
        if(!scalar_cast(r,GSC_CAST_INT,&args[0],line,&converted)||
           !scalar_get_integer(r,&converted,line,&bound))return 0;
    } else {
        if(args[0].kind!=V_NUMBER || args[0].integer!=1)
            return rfail(r,line,"randomint argument 0 is outside selected int32 domain");
        if(!scalar_get_integer(r,&args[0],line,&bound))return 0;
    }
    if(bound<=0)return rfail(r,line,"randomint bound must be positive");
    if(!r->randomint.draw)return rfail(r,line,"randomint draw host unavailable");
    if(!r->next_randomint_operation)return rfail(r,line,"randomint operation token exhausted");
    now=r->now_ms;
    if(!randomint_context(r,now))return rfail(r,line,"randomint runtime capability changed before draw");
    memset(&request,0,sizeof(request));request.bound=bound;request.draw15=UINT32_MAX;
    request.operation_id=r->next_randomint_operation;
    r->next_randomint_operation=request.operation_id==UINT64_MAX?0:request.operation_id+1;
    reply=request;error[0]=0;
    accepted=r->randomint.draw(r->randomint.user,now,&reply,error,sizeof(error));
    error[sizeof(error)-1]=0;
    if(r->error[0])return 0;
    if(!randomint_context(r,now))return rfail(r,line,"randomint callback changed runtime clock or selected capability");
    if(accepted!=1)return rfail(r,line,"randomint draw host refused: %s",error[0]?error:"unspecified host failure");
    if(!randomint_reply_valid(&request,&reply))return rfail(r,line,"randomint draw host changed intent or returned invalid draw");
    product=reply.draw15*(uint32_t)bound;
    /* Logical shift is defined for uint32_t; the correction implements x86
     * SAR15 without signed overflow or an implementation-defined signed shift. */
    value=(int32_t)(product>>15);
    if(product&UINT32_C(0x80000000))value-=INT32_C(131072);
    scalar_integer(&result,value);*out=result;return 1;
}
