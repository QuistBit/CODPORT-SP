/* R114 ordered original earthquake wrapper getters. Duration is admitted only
 * when float32 seconds*1000 is exactly integral, nonnegative int32. This avoids
 * inventing an ambient x87 rounding mode for fractional milliseconds. Positive
 * finite radius is a bounded visual domain, not an original wrapper predicate.
 * The typed host owns event transport and native four-slot camera evaluation. */
static int quake_runtime_preflight(const GscProgram *g,const GscQuakeHost *host,char *error,size_t size) {
    int n,kind;
    for(n=0;n<g->code_count;n++) {
        const Instr *i=&g->code[n];
        if(i->kind!=I_CALL||i->target>=-1-QUAKE_TARGET_BASE||i->target<-1-FX_TARGET_BASE)continue;
        kind=-1-QUAKE_TARGET_BASE-i->target;
        if(!g->quake_mode||i->module[0]||quake_descriptor(g,i->name)!=kind||
           find_func(g,i->owner_module,i->name)>=0||quake_static_issues(g,kind,i->args,i->argc)) {
            if(error&&size)snprintf(error,size,"line %d: invalid quake binding",i->line);
            return 0;
        }
        if(!host||!host->emit) {
            if(error&&size)snprintf(error,size,"line %d: quake requires selected emit host capability",i->line);
            return 0;
        }
    }
    for(n=0;n<g->expr_count;n++) {
        const Expr *e=&g->expr[n];
        if(e->call_binding!=GSC_EXPR_TARGET_QUAKE_NATIVE)continue;
        if(!g->quake_mode||e->kind!=E_NATIVE_CALL||e->module[0]||
           quake_descriptor(g,e->name)!=e->target||find_func(g,e->owner_module,e->name)>=0||
           quake_static_issues(g,e->target,e->args,e->argc)) {
            if(error&&size)snprintf(error,size,"line %d: invalid quake expression binding",e->line);
            return 0;
        }
        if(!host||!host->emit) {
            if(error&&size)snprintf(error,size,"line %d: quake expression requires selected emit host capability",e->line);
            return 0;
        }
    }
    return 1;
}
static int quake_float_get(GscRuntime *r,const Value *args,int argc,int index,int line,float *out) {
    int32_t integer;
    if(argc<=index)return rfail(r,line,"earthquake missing GetFloat argument %d",index);
    if(args[index].kind!=V_NUMBER)return rfail(r,line,"earthquake GetFloat argument %d requires a number",index);
    if(args[index].integer) {
        if(!scalar_get_integer(r,&args[index],line,&integer))return 0;
        *out=(float)integer;
    } else *out=args[index].number;
    return 1;
}
static int quake_statement(GscRuntime *r,int kind,const Value *args,int argc,int line) {
    GscQuakeEvent event;char error[256];float seconds;volatile float milliseconds;int n,accepted,context;
    if(!r->quake_enabled||!r->names_enabled||r->error[0]||
       quake_static_issues(r->program,kind,NULL,argc)||(!args&&argc))return rfail(r,line,"invalid quake invocation");
    if(!r->quake.emit)return rfail(r,line,"quake host capability is unavailable");
    memset(&event,0,sizeof(event));
    if(!quake_float_get(r,args,argc,0,line,&event.scale)||
       !quake_float_get(r,args,argc,1,line,&seconds))return 0;
    if(argc<=2)return rfail(r,line,"earthquake missing GetVector argument 2");
    if(args[2].kind!=V_VECTOR)return rfail(r,line,"earthquake GetVector argument 2 requires a vector");
    for(n=0;n<3;n++) {
        event.origin[n]=args[2].vector[n];
    }
    if(!quake_float_get(r,args,argc,3,line,&event.radius))return 0;
    if(!isfinite(event.scale)||!isfinite(seconds)||!isfinite(event.radius))
        return rfail(r,line,"earthquake scalar is outside finite bounded domain");
    for(n=0;n<3;n++)if(!isfinite(event.origin[n]))
        return rfail(r,line,"earthquake origin is outside finite bounded domain");
    milliseconds=seconds*1000.0f;
    if(!isfinite(milliseconds)||milliseconds<0.0f||(double)milliseconds>2147483647.0||
       floor((double)milliseconds)!=(double)milliseconds)
        return rfail(r,line,"earthquake duration requires exact nonnegative int32 milliseconds");
    if(event.radius<=0.0f)return rfail(r,line,"earthquake radius is outside positive finite bounded domain");
    event.duration_ms=(int32_t)milliseconds;
    error[0]=0;accepted=r->quake.emit(r->quake.user,r->now_ms,&event,error,sizeof(error));error[sizeof(error)-1]=0;
    if(r->error[0])return 0;
    if(accepted!=1)return rfail(r,line,"quake host rejected effect: %s",error[0]?error:"unspecified host failure");
    context=console_context(r);return context>=0;
}
