/* The integer lane never invokes a foreign numeric/type adapter. Borrow
 * operands only for this lane; the original by-value path retains snapshots
 * for every other type and mode. The caller supplies a separate result. */
static int results_binary56(GscRuntime *r,const Expr *e,const Value *a,const Value *b,int line,Value *out) {
    if(e->kind==E_BINARY&&r->scalar_switch_enabled&&a->kind==V_NUMBER&&b->kind==V_NUMBER&&
       a->integer==1&&b->integer==1) {
        memset(out,0,sizeof(*out));
        if(r->integer_bitwise_enabled&&(e->op=='|'||e->op=='^'||e->op=='&'))
            return integer_bitwise_binary(r,e->op,a,b,line,out);
        return scalar_binary_selected(r,e->float_arithmetic,e->op,a,b,line,out);
    }
    return results_operator(r,e,*a,*b,line,out);
}
