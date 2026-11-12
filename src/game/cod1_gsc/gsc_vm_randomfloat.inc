/* R134 selected exact-dyadic literal randomfloat lane. GetFloat accepts the
 * selected float32 literal or an exactly representable int32 literal. Missing
 * and type/domain failures precede the draw. Original zero/negative bounds are
 * valid, with no positivity guard. The final type is float, including zero.
 * Integer projection proves PC/RC independence only for this exact domain;
 * consumed shared GAME state is never restored on later refusal. */
static int randomfloat_runtime_preflight(const GscProgram *g,
    const GscRandomFloatHost *host,char *error,size_t size) {
    int n;
    for(n=0;n<g->code_count;n++) {
        const Instr *i=&g->code[n];
        if(i->kind!=I_CALL||i->target!=-1-RANDOMFLOAT_TARGET_BASE-GSC_RANDOMFLOAT_DRAW)continue;
        if(!g->randomfloat_mode||i->module[0]||
           randomfloat_descriptor(g,i->name,i->args,i->argc)!=GSC_RANDOMFLOAT_DRAW||
           find_func(g,i->owner_module,i->name)>=0||!host||!host->draw) {
            if(error&&size)snprintf(error,size,"line %d: randomfloat requires selected exact-dyadic draw host",i->line);
            return 0;
        }
    }
    for(n=0;n<g->expr_count;n++) {
        const Expr *e=&g->expr[n];
        if(e->call_binding!=GSC_EXPR_TARGET_RANDOMFLOAT_NATIVE)continue;
        if(!g->randomfloat_mode||e->kind!=E_NATIVE_CALL||e->module[0]||
           randomfloat_descriptor(g,e->name,e->args,e->argc)!=GSC_RANDOMFLOAT_DRAW||
           e->target!=GSC_RANDOMFLOAT_DRAW||find_func(g,e->owner_module,e->name)>=0||!host||!host->draw) {
            if(error&&size)snprintf(error,size,"line %d: randomfloat expression requires selected exact-dyadic draw host",e->line);
            return 0;
        }
    }
    return 1;
}
static int randomfloat_context(const GscRuntime *r,int now) {
    return r&&r->randomfloat_enabled&&r->names_enabled&&r->scalar_switch_enabled&&
        r->program&&r->program->dynamic_randomfloat_mode==r->dynamic_randomfloat_enabled&&
        !r->error[0]&&r->now_ms==now&&now>=0;
}
static int randomfloat_call_selected(GscRuntime *r,int kind,const Value *args,int argc,
    int selected,int line,Value *out) {
    GscRandomFloatRequest request,reply;Value result;char error[256];
    uint32_t bound,bits;int accepted,now;
    if(!r||!r->randomfloat_enabled||!r->names_enabled||!r->scalar_switch_enabled||
       r->error[0]||!out||kind!=GSC_RANDOMFLOAT_DRAW||argc<0||argc>(r->dynamic_randomfloat_enabled?MAX_PARAMS:1)||(!args&&argc))
        return rfail(r,line,"invalid selected exact-dyadic randomfloat invocation");
    if(selected&&!r->float_arithmetic_enabled)return rfail(r,line,"module float arithmetic runtime capability unavailable");
    if(!argc)return rfail(r,line,"randomfloat missing GetFloat argument 0");
    if(args[0].kind!=V_NUMBER)return rfail(r,line,"randomfloat argument 0 requires selected numeric payload");
    if(args[0].integer==1) {
        if(!args[0].integer_exact||!randomfloat_integer_bits(args[0].integer_value,&bound))
            return rfail(r,line,"randomfloat integer argument 0 outside exact GetFloat domain");
    } else {
        if(args[0].integer!=0)return rfail(r,line,"randomfloat argument 0 has invalid numeric tag");
        memcpy(&bound,&args[0].number,sizeof(bound));
        if(!(selected?float_arithmetic_bound_valid(bound):randomfloat_decode_bound(bound,NULL,NULL)))
            return rfail(r,line,"randomfloat argument 0 outside selected exact-dyadic domain");
    }
    if(!r->randomfloat.draw)return rfail(r,line,"randomfloat draw host unavailable");
    if(!r->next_randomfloat_operation)return rfail(r,line,"randomfloat operation token exhausted");
    now=r->now_ms;
    if(!randomfloat_context(r,now))return rfail(r,line,"randomfloat capability changed before draw");
    /* R232 keeps the numeric bound private to the selected projection; the
     * genuine shared owner receives its unchanged neutral draw request. */
    memset(&request,0,sizeof(request));request.bound_bits=selected==2?UINT32_C(0x3f800000):bound;request.draw15=UINT32_MAX;
    request.operation_id=r->next_randomfloat_operation;
    r->next_randomfloat_operation=request.operation_id==UINT64_MAX?0:request.operation_id+1;
    reply=request;error[0]=0;
    if(selected&&console_context(r)<=0)return 0;
    if(selected)r->entity_prepare_busy=1;
    accepted=r->randomfloat.draw(r->randomfloat.user,now,&reply,error,sizeof(error));
    if(selected)r->entity_prepare_busy=0;
    error[sizeof(error)-1]=0;
    if(r->error[0])return 0;
    if(selected&&(!entity_task_current(r)||console_context(r)<=0))return 0;
    if(!randomfloat_context(r,now))return rfail(r,line,"randomfloat callback changed clock or selected capability");
    if(accepted!=1)return rfail(r,line,"randomfloat draw host refused: %s",error[0]?error:"unspecified host failure");
    if(selected?(reply.operation_id!=request.operation_id||reply.bound_bits!=request.bound_bits||reply.draw15>32767):!randomfloat_reply_valid(&request,&reply))return rfail(r,line,"randomfloat draw host changed intent or returned invalid draw");
    if(selected) {
        if(!float_arithmetic_project(bound,reply.draw15,r->program->float_arithmetic_profile.rounding,&bits))return rfail(r,line,"selected float projection outside admitted domain");
    } else bits=randomfloat_project_bits(bound,reply.draw15);
    memset(&result,0,sizeof(result));result.kind=V_NUMBER;
    memcpy(&result.number,&bits,sizeof(bits));*out=result;return 1;
}

/* Preserve the retained private exact-lane helper contract. */
static int randomfloat_call(GscRuntime *r,int kind,const Value *args,int argc,int line,Value *out) {
    return randomfloat_call_selected(r,kind,args,argc,0,line,out);
}
