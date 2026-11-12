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
    if(!scalar_get_integer(r,a->kind==V_NUMBER?a:b,line,&integer))return 0;
    memset(&text,0,sizeof(text));text.kind=V_STRING;Cod1ModelIntText(integer,text.str);
    return a->kind==V_STRING?scalar_string_add(r,a,&text,line,out):scalar_string_add(r,&text,b,line,out);
}
