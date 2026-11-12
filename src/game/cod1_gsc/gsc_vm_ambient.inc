/* R120 original global ambientPlay argc1/2, ambientStop argc0/1. Optional
 * GetFloat/conversion precedes GetString0; play empty alias precedes negative
 * converted-time rejection. f32 product + double2^-30 then FISTP is observed.
 * The deterministic biased-nearest profile below is explicitly selected; retail
 * FPU control state is unproved. Finite bounded transitions/ordinary alias
 * string transport and undefined return are selected port admission policies. */
static int ambient_runtime_preflight(const GscProgram *g,const GscAmbientHost *host,char *error,size_t size) {
    int n,kind;
    for(n=0;n<g->code_count;n++) {
        const Instr *i=&g->code[n];
        if(i->kind!=I_CALL||i->target>=-1-AMBIENT_TARGET_BASE||i->target<-1-PRECACHE_ITEM_TARGET_BASE)continue;
        kind=-1-AMBIENT_TARGET_BASE-i->target;
        if(!g->ambient_mode||i->module[0]||ambient_descriptor(g,i->name)!=kind||
           find_func(g,i->owner_module,i->name)>=0||ambient_static_issues(g,kind,i->args,i->argc)) {
            if(error&&size)snprintf(error,size,"line %d: invalid ambient binding",i->line);
            return 0;
        }
        if(!host||!host->publish) {
            if(error&&size)snprintf(error,size,"line %d: ambient requires selected publish host capability",i->line);
            return 0;
        }
    }
    for(n=0;n<g->expr_count;n++) {
        const Expr *e=&g->expr[n];
        if(e->call_binding!=GSC_EXPR_TARGET_AMBIENT_NATIVE)continue;
        if(!g->ambient_mode||e->kind!=E_NATIVE_CALL||e->module[0]||
           ambient_descriptor(g,e->name)!=e->target||find_func(g,e->owner_module,e->name)>=0||
           ambient_static_issues(g,e->target,e->args,e->argc)) {
            if(error&&size)snprintf(error,size,"line %d: invalid ambient expression binding",e->line);
            return 0;
        }
        if(!host||!host->publish) {
            if(error&&size)snprintf(error,size,"line %d: ambient expression requires selected publish host capability",e->line);
            return 0;
        }
    }
    return 1;
}
/* Domain rejection of converted milliseconds is deferred until after play's
 * GetString0 and empty-alias check, as in the observed wrapper. */
static int ambient_convert(GscRuntime *r,const Value *args,int index,int line,double *converted) {
    float seconds,scaled;
    if(args[index].kind!=V_NUMBER)return rfail(r,line,"ambient GetFloat argument %d requires a numeric scalar",index);
    if(!scalar_float(r,&args[index],line,&seconds))return 0;
    scaled=(float)((double)seconds*1000.0);
    if(!isfinite(scaled)){*converted=INFINITY;return 1;}
    /* Under the declared x87 0x037f extended-precision profile, +2^-30
     * survives every admitted float32 half tie (ties occur only below 2^23).
     * Other float32 fractional values are farther from a tie than this bias.
     * floor(product+.5) retains that result without a binary64 biased-store
     * which loses the bias at e.g. 4194304.5. Retail FPU state remains unproved. */
    *converted=floor((double)scaled+.5);
    return 1;
}
static int ambient_call(GscRuntime *r,int kind,const Value *args,int argc,int line,Value *out) {
    GscAmbientEvent event,copy;double converted=0;size_t length;char error[256];int accepted,context;
    if(!r->ambient_enabled||!r->names_enabled||r->error[0]||!out||
       ambient_static_issues(r->program,kind,NULL,argc)||(!args&&argc))
        return rfail(r,line,"invalid ambient structural invocation");
    memset(&event,0,sizeof(event));event.kind=kind;
    if((kind==GSC_AMBIENT_PLAY&&argc==2)||(kind==GSC_AMBIENT_STOP&&argc==1))
        if(!ambient_convert(r,args,kind==GSC_AMBIENT_PLAY?1:0,line,&converted))return 0;
    if(kind==GSC_AMBIENT_PLAY) {
        if(args[0].kind!=V_STRING)return rfail(r,line,"ambient GetString argument 0 requires an ordinary string");
        if(!memchr(args[0].str,0,sizeof(args[0].str)))return rfail(r,line,"ambient GetString argument 0 exceeds selected string domain");
        if(!args[0].str[0])return rfail(r,line,"ambientPlay alias must be nonempty");
    }
    if(converted<0)return rfail(r,line,"ambient time must be nonnegative after conversion");
    if(!isfinite(converted)||converted>INT_MAX)return rfail(r,line,"ambient time overflow");
    event.transition_ms=(int)converted;
    if(kind==GSC_AMBIENT_PLAY) {
        length=strlen(args[0].str);
        if(length>=sizeof(event.alias))return rfail(r,line,"ambient alias exceeds selected 127-byte transport domain");
        memcpy(event.alias,args[0].str,length+1);
    }
    if(!ambient_event_alias_valid(&event))return rfail(r,line,"ambient alias is outside selected transport domain");
    if(!r->ambient.publish)return rfail(r,line,"ambient publish host capability is unavailable");
    if(!r->next_ambient_operation)return rfail(r,line,"ambient operation token exhausted");
    event.operation_id=r->next_ambient_operation;
    r->next_ambient_operation=event.operation_id==UINT64_MAX?0:event.operation_id+1;
    copy=event;error[0]=0;
    accepted=r->ambient.publish(r->ambient.user,r->now_ms,&copy,error,sizeof(error));error[sizeof(error)-1]=0;
    if(r->error[0])return 0;
    if(accepted!=1)return rfail(r,line,"ambient host rejected publication: %s",error[0]?error:"unspecified host failure");
    context=console_context(r);if(context<0)return 0;
    memset(out,0,sizeof(*out));return 1;
}
