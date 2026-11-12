/* Exact signed-int32 bit patterns. Unsigned C operations avoid signed
 * overflow; the existing scalar decoder preserves all 32 payload bits.
 * No float, string, undefined, vector, entity or object coercion is selected. */
int gsc_runtime_select_integer_bitwise(GscRuntime *r) {
    if(!r||r->started||r->advancing||r->executing_task||r->task_count||r->error[0]||
       r->integer_bitwise_enabled||!r->program->integer_bitwise_selected||!r->scalar_switch_enabled||
       r->entity_prepare_busy||r->entity_certificate_busy||r->entity_sequence_busy||
       r->entity_inheritance_busy||r->entity_field_busy||r->entity_method_busy)return 0;
    r->integer_bitwise_enabled=1;return 1;
}
static int integer_bitwise_unary(GscRuntime *r,const Value *a,int line,Value *out) {
    int32_t value;Value result;
    if(!r->integer_bitwise_enabled)return rfail(r,line,"integer bitwise capability is unavailable");
    if(!scalar_get_integer(r,a,line,&value))return 0;
    scalar_integer(&result,scalar_signed(~(uint32_t)value));*out=result;return 1;
}
static int integer_bitwise_binary(GscRuntime *r,int op,const Value *a,
                                    const Value *b,int line,Value *out) {
    int32_t left,right;uint32_t bits;Value result;
    if(!r->integer_bitwise_enabled)return rfail(r,line,"integer bitwise capability is unavailable");
    if(!scalar_get_integer(r,a,line,&left)||!scalar_get_integer(r,b,line,&right))return 0;
    if(op=='|')bits=(uint32_t)left|(uint32_t)right;
    else if(op=='^')bits=(uint32_t)left^(uint32_t)right;
    else if(op=='&')bits=(uint32_t)left&(uint32_t)right;
    else return rfail(r,line,"unsupported integer bitwise operator");
    scalar_integer(&result,scalar_signed(bits));*out=result;return 1;
}
static int integer_bitwise_mutation(GscRuntime *r,int op,const Value *a,
                                      const Value *b,int line,Value *out) {
    if(!integer_bitwise_mutation_op(op))return rfail(r,line,"unsupported integer bitwise mutation");
    return integer_bitwise_binary(r,op==GSC_MUT_BIT_OR?'|':op==GSC_MUT_BIT_XOR?'^':'&',a,b,line,out);
}
