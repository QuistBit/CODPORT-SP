/* R150 structural admission for the runtime-checked integer lane. Unknown
 * parameter/field/callee values are checked through their actual typed Value.
 * R151 admits numeric FLOAT expressions only in its additional selector.
 * No source
 * module, function, variable name or callsite list grants admission. */
static int dynamic_randomint_argument(const GscProgram *g,int index,int depth) {
    const Expr *e;int native;
    if(!g||index<0||index>=g->expr_count||depth>48)return 0;
    e=&g->expr[index];
    switch(e->kind) {
      case E_NUMBER:return g->numeric_randomint_mode||e->integer==1;
      case E_STRING:case E_LOCALIZED_STRING:case E_VECTOR:case E_EMPTY_ARRAY:
      case E_FUNC_REF:case E_ANIM_REF:case E_ANIMTREE_VALUE:
      case E_RAW_STRING:case E_RAW_LOCALIZED_STRING:case E_THREAD_CALL:return 0;
      case E_VAR:case E_FIELD:case E_INDEX:case E_METHOD_CALL:
      case E_SCOPED_CALL:case E_DYNAMIC_CALL:case E_ISDEFINED:return 1;
      case E_CAST:return e->op==GSC_CAST_INT||
          (g->numeric_randomint_mode&&e->op==GSC_CAST_FLOAT);
      case E_UNARY:
        if(e->op=='!')return 1;
        return dynamic_randomint_argument(g,e->a,depth+1);
      case E_BINARY:
        if(e->op==T_EQ||e->op==T_NE||e->op==T_LE||e->op==T_GE||
           e->op=='<'||e->op=='>'||e->op==T_AND||e->op==T_OR)return 1;
        return dynamic_randomint_argument(g,e->a,depth+1)&&
            dynamic_randomint_argument(g,e->b,depth+1);
      case E_NATIVE_CALL:
        if(e->module[0]||find_func(g,e->owner_module,e->name)>=0)return 1;
        if(randomfloat_descriptor(g,e->name,e->args,e->argc)>=0)return g->numeric_randomint_mode;
        native=native_query_descriptor(g,e->name);
        return (g->numeric_randomint_mode||native!=GSC_QUERY_CVAR_FLOAT)&&
            native!=GSC_QUERY_CVAR_STRING&&
            native!=GSC_QUERY_DIFFICULTY&&struct_descriptor(g,e->name)<0;
      default:return 0;
    }
}
