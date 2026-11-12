/* R102 internal script references. These identities are port provenance,
 * never original VM tags, code pointers or names resolved during execution. */
static int function_token_new(uint64_t *out) {
    static uint64_t serial;
    if(!out||serial==UINT64_MAX)return 0;
    *out=++serial;return 1;
}
static int function_value_valid(const GscRuntime *r,const Value *value) {
    const FunctionIdentity *id;
    if(!r||!r->function_refs_enabled||!value||value->kind!=V_FUNCTION)return 0;
    id=&value->function;
    return id->program&&id->program==r->program->function_program_token&&
        id->runtime&&id->runtime==r->array_token&&
        id->module>=0&&id->module<r->program->module_count&&
        id->function>=0&&id->function<r->program->func_count&&
        r->program->funcs[id->function].module==id->module;
}
static int function_reference(GscRuntime *r,const Expr *e,int line,Value *out) {
    Value value;int module;
    if(!r->function_refs_enabled||!e||e->kind!=E_FUNC_REF||!out)
        return rfail(r,line,"unsupported function reference runtime semantics");
    module=e->module[0]?find_module(r->program,e->module):e->owner_module;
    if(module<0||module>=r->program->module_count||e->target<0||e->target>=r->program->func_count||
       r->program->funcs[e->target].module!=module||!callable_name_equal(r->program,r->program->funcs[e->target].name,e->name))
        return rfail(r,line,"invalid committed script function reference");
    memset(&value,0,sizeof(value));value.kind=V_FUNCTION;
    value.function.program=r->program->function_program_token;value.function.runtime=r->array_token;
    value.function.module=module;value.function.function=e->target;
    if(!function_value_valid(r,&value))return rfail(r,line,"invalid function provenance");
    *out=value;return 1;
}
static int function_target(GscRuntime *r,const Value *value,int argc,int line,int *out) {
    int target;
    if(!function_value_valid(r,value))return rfail(r,line,"computed target requires a current script function reference");
    target=value->function.function;
    if(argc<0||argc>MAX_PARAMS)
        return rfail(r,line,"invalid computed script argument count");
    *out=target;return 1;
}
static int function_static_target_preflight(const GscProgram *g,int target,int argc,int line,
    char *error,size_t size) {
    const Expr *e=&g->expr[target];
    if(argc<0||argc>MAX_PARAMS||
       (e->kind==E_FUNC_REF&&(e->target<0||e->target>=g->func_count))) {
        if(error&&size)snprintf(error,size,"line %d: invalid function-reference target or structural argument count",line);
        return 0;
    }
    return 1;
}
static int function_runtime_preflight(const GscProgram *g,char *error,size_t size) {
    int n,k,module;
    if(!g->function_ref_mode||!g->function_program_token)return 0;
    for(n=0;n<g->expr_count;n++) {
        const Expr *e=&g->expr[n];
        if(e->kind==E_FUNC_REF) {
            module=e->module[0]?find_module(g,e->module):e->owner_module;
            if(module<0||module>=g->module_count||e->target<0||e->target>=g->func_count||
               g->funcs[e->target].module!=module||!callable_name_equal(g,g->funcs[e->target].name,e->name)) {
                if(error&&size)snprintf(error,size,"line %d: invalid script function reference binding",e->line);
                return 0;
            }
        } else if(e->kind==E_DYNAMIC_CALL) {
            if(e->a<0||e->a>=n||e->b< -1||e->b>=n||e->argc<0||e->argc>MAX_PARAMS)goto invalid;
            for(k=0;k<e->argc;k++)if(e->args[k]<0||e->args[k]>=n)goto invalid;
            if(!function_static_target_preflight(g,e->a,e->argc,e->line,error,size))return 0;
        }
    }
    for(n=0;n<g->code_count;n++) {
        const Instr *i=&g->code[n];
        if(i->kind!=I_DYNAMIC_CALL&&i->kind!=I_DYNAMIC_THREAD)continue;
        if(i->expr<0||i->expr>=g->expr_count||i->receiver< -1||i->receiver>=g->expr_count||
           i->argc<0||i->argc>MAX_PARAMS)goto invalid;
        for(k=0;k<i->argc;k++)if(i->args[k]<0||i->args[k]>=g->expr_count)goto invalid;
        if(!function_static_target_preflight(g,i->expr,i->argc,i->line,error,size))return 0;
    }
    return 1;
  invalid:
    if(error&&size)snprintf(error,size,"invalid computed call syntax binding");
    return 0;
}
