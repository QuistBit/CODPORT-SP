/* R113 wrapper getter order only. Spatial selection, original MOD remapping,
 * combat/LOS and the unresolved original caller ignore-players context belong
 * to the game host. Finite float32 is explicit narrower bounded port policy. */
static int blast_runtime_preflight(const GscProgram *g,const GscBlastHost *host,char *error,size_t size) {
    int n,kind;
    for(n=0;n<g->code_count;n++) {
        const Instr *i=&g->code[n];
        if(i->kind!=I_CALL||i->target>=-1-BLAST_TARGET_BASE||i->target<-1-QUAKE_TARGET_BASE)continue;
        kind=-1-BLAST_TARGET_BASE-i->target;
        if(!g->blast_mode||i->module[0]||blast_descriptor(g,i->name)!=kind||
           find_func(g,i->owner_module,i->name)>=0||blast_static_issues(g,kind,i->args,i->argc)) {
            if(error&&size)snprintf(error,size,"line %d: invalid blast binding",i->line);
            return 0;
        }
        if(!host||!host->radius) {
            if(error&&size)snprintf(error,size,"line %d: blast requires selected radius host capability",i->line);
            return 0;
        }
    }
    for(n=0;n<g->expr_count;n++) {
        const Expr *e=&g->expr[n];
        if(e->call_binding!=GSC_EXPR_TARGET_BLAST_NATIVE)continue;
        if(!g->blast_mode||e->kind!=E_NATIVE_CALL||e->module[0]||
           blast_descriptor(g,e->name)!=e->target||find_func(g,e->owner_module,e->name)>=0||
           blast_static_issues(g,e->target,e->args,e->argc)) {
            if(error&&size)snprintf(error,size,"line %d: invalid blast expression binding",e->line);
            return 0;
        }
        if(!host||!host->radius) {
            if(error&&size)snprintf(error,size,"line %d: blast expression requires selected radius host capability",e->line);
            return 0;
        }
    }
    return 1;
}
static int blast_float_get(GscRuntime *r,const Value *args,int argc,int index,int line,float *out) {
    if(argc<=index)return rfail(r,line,"radiusDamage missing GetFloat argument %d",index);
    return scalar_float(r,&args[index],line,out);
}
static int blast_statement(GscRuntime *r,int kind,const Value *args,int argc,int line) {
    GscBlastEvent event;char error[256];int n,accepted,context;
    if(!r->blast_enabled||!r->names_enabled||r->error[0]||
       blast_static_issues(r->program,kind,NULL,argc)||(!args&&argc))
        return rfail(r,line,"invalid blast invocation");
    if(!r->blast.radius)return rfail(r,line,"blast host capability is unavailable");
    memset(&event,0,sizeof(event));event.kind=kind;
    if(!argc)return rfail(r,line,"radiusDamage missing GetVector argument 0");
    if(args[0].kind!=V_VECTOR)return rfail(r,line,"radiusDamage GetVector argument 0 requires a finite vector");
    for(n=0;n<3;n++) {
        if(!isfinite(args[0].vector[n]))return rfail(r,line,"radiusDamage GetVector argument 0 requires a finite vector");
        event.origin[n]=args[0].vector[n];
    }
    if(!blast_float_get(r,args,argc,1,line,&event.radius)||
       !blast_float_get(r,args,argc,2,line,&event.inner_damage)||
       !blast_float_get(r,args,argc,3,line,&event.outer_damage))return 0;
    error[0]=0;accepted=r->blast.radius(r->blast.user,r->now_ms,&event,error,sizeof(error));error[sizeof(error)-1]=0;
    if(r->error[0])return 0;
    if(accepted!=1)return rfail(r,line,"blast host rejected effect: %s",error[0]?error:"unspecified host failure");
    context=console_context(r);return context>=0;
}
