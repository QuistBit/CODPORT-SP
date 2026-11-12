/* R152 conditional runtime-checked exact-dyadic bounds. Known literals keep
 * their original exact-domain admission. Parameters/fields/callees are checked
 * through their actual Value; no site/name list or source-origin assumption
 * guarantees a numeric value or membership in that bounded domain. */
static int dynamic_randomfloat_argument(const GscProgram *g,int index,int depth) {
    const Expr *e;uint32_t bits;int native;
    if(!g||index<0||index>=g->expr_count||depth>48)return 0;
    e=&g->expr[index];
    switch(e->kind) {
      case E_NUMBER:return float_arithmetic_literal_bits(g,index,&bits)||randomfloat_literal_bits(g,index,&bits);
      case E_STRING:case E_LOCALIZED_STRING:case E_VECTOR:case E_EMPTY_ARRAY:
      case E_FUNC_REF:case E_ANIM_REF:case E_ANIMTREE_VALUE:
      case E_RAW_STRING:case E_RAW_LOCALIZED_STRING:case E_THREAD_CALL:return 0;
      case E_VAR:case E_FIELD:case E_INDEX:case E_METHOD_CALL:
      case E_SCOPED_CALL:case E_DYNAMIC_CALL:case E_ISDEFINED:return 1;
      case E_CAST:return e->op==GSC_CAST_INT||e->op==GSC_CAST_FLOAT;
      case E_UNARY:
        if(e->op=='!')return 1;
        return dynamic_randomfloat_argument(g,e->a,depth+1);
      case E_BINARY:
        if(e->op==T_EQ||e->op==T_NE||e->op==T_LE||e->op==T_GE||
           e->op=='<'||e->op=='>'||e->op==T_AND||e->op==T_OR)return 1;
        return dynamic_randomfloat_argument(g,e->a,depth+1)&&
            dynamic_randomfloat_argument(g,e->b,depth+1);
      case E_NATIVE_CALL:
        if(e->module[0]||find_func(g,e->owner_module,e->name)>=0)return 1;
        native=native_query_descriptor(g,e->name);
        return native!=GSC_QUERY_CVAR_STRING&&native!=GSC_QUERY_DIFFICULTY&&
            struct_descriptor(g,e->name)<0;
      default:return 0;
    }
}
