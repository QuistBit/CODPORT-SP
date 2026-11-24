/* Every dependency compiled under the independent map profile shares the
 * qualified PC53/binary32 arithmetic and RNG projection. A source pathname
 * does not change original numeric opcodes. This explicit profile owns no
 * animation namespace.
 * It still requires a genuine map-certified root at startup and on draws. */
int gsc_program_select_map_source_float(GscProgram*g){
 if(!g||g->invalid||g->linked||g->module_count||g->float_arithmetic_selected||!g->dynamic_randomfloat_syntax||g->random_range_selected)return 0;
 g->map_source_float_selected=1;g->r232_source_float_selected=1;g->float_arithmetic_selected=1;g->random_range_selected=1;
 g->float_arithmetic_profile.precision_bits=53;g->float_arithmetic_profile.rounding=GSC_FLOAT_ROUND_NEAREST;return 1;
}
int gsc_runtime_select_map_source_float(GscRuntime*r){
 if(!r||!r->program->map_source_float_selected||r->map_source_float_enabled||r->random_range_enabled||!gsc_runtime_select_module_float_arithmetic(r))return 0;
 r->map_source_float_enabled=1;r->random_range_enabled=1;return 1;
}
int gsc_program_select_r232_source_float_arithmetic(GscProgram *g) {
    if(!g||g->invalid||g->linked||g->module_count||g->float_arithmetic_selected||
       !g->turret_animations_selected||!g->dynamic_randomfloat_syntax||!g->random_range_selected)return 0;
    g->r232_source_float_selected=1;g->float_arithmetic_selected=1;
    g->float_arithmetic_profile.precision_bits=53;g->float_arithmetic_profile.rounding=GSC_FLOAT_ROUND_NEAREST;
    return 1;
}
int gsc_runtime_select_r232_source_float_arithmetic(GscRuntime *r) {
    if(!r||r->started||r->advancing||r->executing_task||r->task_count||r->error[0]||
       r->float_arithmetic_enabled||!r->program->r232_source_float_selected||!r->random_range_enabled||
       !r->turret_animations_enabled||!r->dynamic_randomfloat_enabled||!r->randomfloat_enabled||!r->randomfloat.draw||
       r->entity_prepare_busy||r->entity_certificate_busy||r->entity_sequence_busy||r->entity_inheritance_busy||r->entity_field_busy||r->entity_method_busy)return 0;
    if(!animation_namespace_quiescent(r)||!animation_namespace_scope(r)||!anim_data_current(r,NULL)||
       !animation_namespace_quiescent(r)||r->float_arithmetic_enabled||!r->random_range_enabled)return 0;
    r->float_arithmetic_enabled=1;return 1;
}
static int r232_source_float_value(GscRuntime*r,int op,const Value*a,const Value*b,int line,Value*out) {
    uint32_t ab,bb,bits;Value staged;
    if(!r->program->r232_source_float_selected||!r->float_arithmetic_enabled||
       !float_arithmetic_number_bits(r,a,line,&ab)||!float_arithmetic_number_bits(r,b,line,&bb)||
       !r232_source_float_binary(op,ab,bb,&bits))return rfail(r,line,"selected source arithmetic outside finite +/-2^24 domain");
    if(console_context(r)<=0)return 0;
    memset(&staged,0,sizeof(staged));staged.kind=V_NUMBER;memcpy(&staged.number,&bits,4);*out=staged;return 1;
}
static int r232_source_float_int_active(const GscRuntime*r){
    const Task*t=r->executing_task;int fn,module;
    if(!t||t->depth<1)return 0;fn=t->frames[t->depth-1].fn;
    if(fn<0||fn>=r->program->func_count)return 0;module=r->program->funcs[fn].module;
    return module>=0&&module<r->program->module_count&&(!strcmp(r->program->modules[module],"animscripts/combat")||!strcmp(r->program->modules[module],"animscripts/mg42/common"));
}
static int r232_source_float_cast(GscRuntime*r,int kind,const Value*input,int line,Value*out) {
    uint32_t bits;Value staged;int32_t integer;
    if(!r->program->r232_source_float_selected||!r->float_arithmetic_enabled)
        return rfail(r,line,"selected source cast capability unavailable");
    if(kind==GSC_CAST_INT){
        if(!r232_source_float_int_active(r))return rfail(r,line,"source int cast requires a selected integer conversion module");
        if(input->kind==V_NUMBER&&input->integer){if(!scalar_get_integer(r,input,line,&integer))return 0;}
        else if(!float_arithmetic_number_bits(r,input,line,&bits)||!r232_source_float_int(bits,&integer))
            return rfail(r,line,"selected source int cast outside finite +/-2^24 domain");
        if(console_context(r)<=0)return 0;scalar_integer(&staged,integer);*out=staged;return 1;
    }
    if(kind!=GSC_CAST_FLOAT||!float_arithmetic_number_bits(r,input,line,&bits)||!r232_source_float_bits(bits))
        return rfail(r,line,"selected source cast requires finite float or exact int32 within +/-2^24");
    if(console_context(r)<=0)return 0;
    memset(&staged,0,sizeof(staged));staged.kind=V_NUMBER;memcpy(&staged.number,&bits,4);*out=staged;return 1;
}
