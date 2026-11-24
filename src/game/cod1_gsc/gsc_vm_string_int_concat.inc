/* R198 ordinary string/int32 concatenation. Reuse the reconstructed exact
 * integer decimal formatter; no FLOAT, localized string, vector or object
 * coercion is selected. Earlier arithmetic profiles keep their old domain. */
int gsc_runtime_select_string_int_concat(GscRuntime *r) {
    if(!r||r->started||r->advancing||r->executing_task||r->task_count||r->error[0]||
       r->string_int_concat_enabled||!r->program->string_int_concat_selected||!r->scalar_switch_enabled||
       r->entity_prepare_busy||r->entity_certificate_busy||r->entity_sequence_busy||
       r->entity_inheritance_busy||r->entity_field_busy||r->entity_method_busy)return 0;
    r->string_int_concat_enabled=1;return 1;
}
static int string_int_concat_pair(const GscRuntime *r,const Value *a,const Value *b) {
    return r->string_int_concat_enabled&&((a->kind==V_STRING&&b->kind==V_NUMBER)||
        (a->kind==V_NUMBER&&b->kind==V_STRING));
}
static int string_int_concat(GscRuntime *r,const Value *a,const Value *b,int line,Value *out) {
    int32_t integer;Value text;
    if(!string_int_concat_pair(r,a,b))return rfail(r,line,"string/int concatenation capability or operands are unavailable");
    const Value *number=a->kind==V_NUMBER?a:b;
    memset(&text,0,sizeof(text));text.kind=V_STRING;
    if(!number->integer && r->program->r232_source_float_selected){
        cod1ModelDecimal_t decimal;
        if(!Cod1ModelDecimal(number->number,&decimal))return rfail(r,line,"cannot concatenate a nonfinite float");
        Cod1ModelFloatText(&decimal,0,text.str);
    }else{
        if(!scalar_get_integer(r,number,line,&integer))return 0;
        Cod1ModelIntText(integer,text.str);
    }
    return a->kind==V_STRING?scalar_string_add(r,a,&text,line,out):scalar_string_add(r,&text,b,line,out);
}

/* ENGINE463550 pair coercion plus4660A9: an ordinary string selects
 * CastString for a vector on either side. Reuse the exact R164 copied
 * formatter, including its two-stage decimal conversion. No pool pointer
 * or ownership crosses into the VM. Earlier profiles remain unchanged. */
static int string_vector_concat_pair(const GscRuntime *r,const Value *a,const Value *b){
 return r->string_int_concat_enabled&&r->vector_arithmetic_enabled&&r->program->r232_source_float_selected&&
  ((a->kind==V_STRING&&b->kind==V_VECTOR)||(a->kind==V_VECTOR&&b->kind==V_STRING));
}
static int string_vector_concat(GscRuntime*r,const Value*a,const Value*b,int line,Value*out){
 GscStringCastValue input={0};GscStringCastResult formatted;Value text={0};char error[160]={0};const Value*v;
 if(!string_vector_concat_pair(r,a,b))return rfail(r,line,"string/vector concatenation capability or operands unavailable");
 v=a->kind==V_VECTOR?a:b;input.type=3;memcpy(input.vector,v->vector,12);
 if(GscStringCast(&input,&formatted,error,sizeof(error))!=1)return rfail(r,line,"string/vector conversion requires finite coordinates and bounded copied text");
 text.kind=V_STRING;memcpy(text.str,formatted.string,sizeof(text.str));
 return a->kind==V_STRING?scalar_string_add(r,a,&text,line,out):scalar_string_add(r,&text,b,line,out);
}
