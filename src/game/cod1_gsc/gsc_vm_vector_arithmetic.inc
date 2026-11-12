/* R198 explicit finite vector arithmetic for restored script execution.
 * Values are private VM vectors, with portable binary32 component arithmetic;
 * this does not select retail x87 control-word or exception-state parity.
 * The same operation serves direct expressions, suspended expressions and
 * mutations. A failed operation never publishes a partial result. */
int gsc_runtime_select_vector_arithmetic(GscRuntime *r) {
    if(!r||r->started||r->advancing||r->executing_task||r->task_count||r->error[0]||
       r->vector_arithmetic_enabled||!r->program->vector_arithmetic_selected||
       !r->controls_enabled||r->entity_prepare_busy||r->entity_certificate_busy||
       r->entity_sequence_busy||r->entity_inheritance_busy||r->entity_field_busy||
       r->entity_method_busy)return 0;
    r->vector_arithmetic_enabled=1;return 1;
}
static int vector_arithmetic_finite(GscRuntime *r,const Value *v,int line) {
    int k;
    if(v->kind!=V_VECTOR)return rfail(r,line,"vector operand is required");
    for(k=0;k<3;k++)if(!isfinite(v->vector[k]))
        return rfail(r,line,"nonfinite vector operand");
    return 1;
}
static int vector_arithmetic_unary(GscRuntime *r,int op,const Value *a,int line,Value *out) {
    Value result;int k;
    if(!r->vector_arithmetic_enabled||op!='-')
        return rfail(r,line,"unsupported vector unary operator");
    if(!vector_arithmetic_finite(r,a,line))return 0;
    memset(&result,0,sizeof(result));result.kind=V_VECTOR;
    for(k=0;k<3;k++)result.vector[k]=-a->vector[k];
    *out=result;return 1;
}
static int vector_arithmetic_binary(GscRuntime *r,int op,const Value *a,
                                     const Value *b,int line,Value *out) {
    Value result;const Value *vector=NULL,*scale=NULL;float factor=0;int k;
    if(!r->vector_arithmetic_enabled)return rfail(r,line,"vector arithmetic is unavailable");
    memset(&result,0,sizeof(result));result.kind=V_VECTOR;
    if(a->kind==V_VECTOR&&b->kind==V_VECTOR&&(op=='+'||op=='-')) {
        if(!vector_arithmetic_finite(r,a,line)||!vector_arithmetic_finite(r,b,line))return 0;
        for(k=0;k<3;k++)result.vector[k]=op=='+'?a->vector[k]+b->vector[k]:a->vector[k]-b->vector[k];
    } else {
        if(a->kind==V_VECTOR&&b->kind==V_NUMBER&&(op=='*'||op=='/')){vector=a;scale=b;}
        else if(a->kind==V_NUMBER&&b->kind==V_VECTOR&&op=='*'){vector=b;scale=a;}
        else return rfail(r,line,"vector operator requires vector +/- vector, vector * or / scalar, or scalar * vector");
        if(!vector_arithmetic_finite(r,vector,line)||!scalar_float(r,scale,line,&factor))return 0;
        if(op=='/'&&factor==0)return rfail(r,line,"vector division by zero");
        for(k=0;k<3;k++)result.vector[k]=op=='/'?vector->vector[k]/factor:vector->vector[k]*factor;
    }
    for(k=0;k<3;k++)if(!isfinite(result.vector[k]))
        return rfail(r,line,"nonfinite vector result");
    *out=result;return 1;
}
static int vector_arithmetic_mutation(GscRuntime *r,int op,const Value *a,
                                       const Value *b,int line,Value *out) {
    if(a->kind!=V_VECTOR||op<GSC_MUT_ADD||op>GSC_MUT_DIV)
        return rfail(r,line,"vector mutation requires a vector destination and +=, -=, *= or /=");
    return vector_arithmetic_binary(r,op==GSC_MUT_ADD?'+':op==GSC_MUT_SUB?'-':
        op==GSC_MUT_MUL?'*':'/',a,b,line,out);
}
