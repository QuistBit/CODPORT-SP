static int float_arithmetic_active(const GscRuntime *r) {
    const Task *task;int fn,module;
    if(!r||!r->program||!r->program->float_arithmetic_selected)return 0;
    task=r->executing_task;if(!task||task->depth<1)return 0;
    fn=task->frames[task->depth-1].fn;if(fn<0||fn>=r->program->func_count)return 0;
    module=r->program->funcs[fn].module;
    if(module<0||module>=r->program->module_count)return 0;
    if(r->program->r232_source_float_selected&&(r232_source_float_module(r->program->modules[module])||r->map_source_float_enabled))return 2;
    return !strcmp(r->program->modules[module],r->program->float_arithmetic_module);
}
int gsc_runtime_select_module_float_arithmetic(GscRuntime *r) {
    if(!r||r->started||r->advancing||r->executing_task||r->task_count||r->error[0]||r->float_arithmetic_enabled||
       !r->program->float_arithmetic_selected||!float_arithmetic_profile_valid(&r->program->float_arithmetic_profile)||
       !r->dynamic_randomfloat_enabled||!r->randomfloat_enabled||!r->randomfloat.draw||
       r->entity_prepare_busy||r->entity_certificate_busy||r->entity_sequence_busy||r->entity_inheritance_busy||r->entity_field_busy||r->entity_method_busy)return 0;
    r->float_arithmetic_enabled=1;return 1;
}
static int float_arithmetic_number_bits(GscRuntime *r,const Value *v,int line,uint32_t *bits) {
    int32_t i;uint32_t magnitude,scan,sign,significand;int top=0;
    if(v->kind!=V_NUMBER)return rfail(r,line,"selected float arithmetic requires numbers");
    if(!v->integer){memcpy(bits,&v->number,sizeof(*bits));return 1;}
    if(!scalar_get_integer(r,v,line,&i))return 0;
    sign=i<0?UINT32_C(0x80000000):0;magnitude=i<0?0u-(uint32_t)i:(uint32_t)i;
    if(!magnitude){*bits=0;return 1;}scan=magnitude;while(scan>>1){scan>>=1;++top;}
    if(top>23&&(magnitude&((UINT32_C(1)<<(top-23))-1)))return rfail(r,line,"selected float promotion requires exactly representable int32");
    significand=top>23?magnitude>>(top-23):magnitude<<(23-top);
    *bits=sign|((uint32_t)(top+127)<<23)|(significand&UINT32_C(0x7fffff));return 1;
}
static int float_arithmetic_binary(GscRuntime *r,int op,const Value *a,const Value *b,int line,Value *out) {
    uint32_t ab,bb,result;Value staged;
    if(!r->float_arithmetic_enabled)return rfail(r,line,"module float arithmetic runtime capability unavailable");
    if(op!='+'&&!r->timed_arithmetic_enabled)return rfail(r,line,"selected module float binary operation is unsupported");
    if(!float_arithmetic_number_bits(r,a,line,&ab)||!float_arithmetic_number_bits(r,b,line,&bb))return 0;
    if(op=='+'||op=='-') {
        if(op=='-')bb^=UINT32_C(0x80000000);
        if(!float_arithmetic_add(ab,bb,r->program->float_arithmetic_profile.rounding,&result))return rfail(r,line,op=='+'?"selected float addition outside normal finite gap31 domain":"selected float subtraction outside normal finite gap31 domain");
    } else if(op=='*'||op=='/') {
        if(op=='/'&&r->frame_cadence_enabled){cod1AnimationTimelinePolicy_t policy;
            policy.precision_bits=r->program->float_arithmetic_profile.precision_bits;policy.rounding=r->program->float_arithmetic_profile.rounding;
            if(!Cod1TimelineDivideFinite(ab,bb,&policy,&result))return rfail(r,line,"selected cadence ratio outside finite normal domain");
        } else if(!float_arithmetic_exact_product_ratio(ab,bb,op,&result))return rfail(r,line,"selected float product/ratio requires exact normal finite result or signed zero");
    } else return rfail(r,line,"selected module float binary operation is unsupported");
    if(console_context(r)<=0)return 0;
    memset(&staged,0,sizeof(staged));staged.kind=V_NUMBER;memcpy(&staged.number,&result,sizeof(result));*out=staged;return 1;
}
static int frame_cadence_wait_delay(GscRuntime *r,const Value *value,int line,int *delay) {
    uint32_t bits,staged;cod1AnimationTimelinePolicy_t policy;
    if(!float_arithmetic_number_bits(r,value,line,&bits))return 0;
    policy.precision_bits=r->program->float_arithmetic_profile.precision_bits;policy.rounding=r->program->float_arithmetic_profile.rounding;
    if(!Cod1TimelineWaitMilliseconds(bits,&policy,&staged)||!staged||staged>INT32_MAX||
       r->cadence_drain_ms<0||r->cadence_drain_ms>INT32_C(0xffffff)||staged>(uint32_t)(INT32_C(0xffffff)-r->cadence_drain_ms))return rfail(r,line,"selected cadence wait outside positive nonwrapping deadline domain");
    *delay=(int)staged;return 1;
}
/* Explicit selected-module float casts preserve a normal finite binary32
 * payload or promote an exactly representable int32. This does not enable
 * float-to-int conversions or inexact integer promotion. */
static int float_arithmetic_cast(GscRuntime *r,int kind,const Value *value,int line,Value *out) {
    uint32_t bits;Value staged;
    if(!r->float_arithmetic_enabled||!r->timed_arithmetic_enabled)return rfail(r,line,"timed animation arithmetic runtime capability unavailable");
    if(kind!=GSC_CAST_FLOAT)return rfail(r,line,"selected module numeric cast is unsupported");
    if(!float_arithmetic_number_bits(r,value,line,&bits))return 0;
    if(!float_arithmetic_normal_or_zero(bits))return rfail(r,line,"selected float cast requires normal finite value or signed zero");
    if(console_context(r)<=0)return 0;
    memset(&staged,0,sizeof(staged));staged.kind=V_NUMBER;memcpy(&staged.number,&bits,sizeof(bits));*out=staged;return 1;
}
