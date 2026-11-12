/* R115 original wrapper getter order with explicitly bounded port data.
 * GetString0 precedes load identity/initial-registration admission. Raw names
 * and raw optional direction are copied; game owns identity and native axes.
 * The resource startup window closes once and never prohibits existing late
 * identities by itself. These typed calls do not add generic native imports. */
static int fx_runtime_preflight(const GscProgram *g,const GscFxHost *host,char *error,size_t size) {
    int n,kind;
    for(n=0;n<g->code_count;n++) {
        const Instr *i=&g->code[n];
        if(i->kind!=I_CALL||i->target>=-1-FX_TARGET_BASE||i->target<-1-CVAR_TARGET_BASE)continue;
        kind=-1-FX_TARGET_BASE-i->target;
        if(!g->fx_mode||i->module[0]||fx_descriptor(g,i->name)!=kind||
           find_func(g,i->owner_module,i->name)>=0||fx_static_issues(g,kind,i->args,i->argc)) {
            if(error&&size)snprintf(error,size,"line %d: invalid FX binding",i->line);
            return 0;
        }
        if(kind!=GSC_FX_LOOP&&(!host||!(kind==GSC_FX_LOAD?host->load!=NULL:host->play!=NULL))) {
            if(error&&size)snprintf(error,size,"line %d: FX requires selected %s host capability",i->line,kind==GSC_FX_LOAD?"load":"play");
            return 0;
        }
    }
    for(n=0;n<g->expr_count;n++) {
        const Expr *e=&g->expr[n];
        if(e->call_binding!=GSC_EXPR_TARGET_FX_NATIVE)continue;
        kind=e->target;
        if(!g->fx_mode||e->kind!=E_NATIVE_CALL||e->module[0]||
           fx_descriptor(g,e->name)!=kind||find_func(g,e->owner_module,e->name)>=0||
           fx_static_issues(g,kind,e->args,e->argc)) {
            if(error&&size)snprintf(error,size,"line %d: invalid FX expression binding",e->line);
            return 0;
        }
        if(kind!=GSC_FX_LOOP&&(!host||!(kind==GSC_FX_LOAD?host->load!=NULL:host->play!=NULL))) {
            if(error&&size)snprintf(error,size,"line %d: FX expression requires selected %s host capability",e->line,kind==GSC_FX_LOAD?"load":"play");
            return 0;
        }
    }
    return 1;
}
static int fx_call(GscRuntime *r,int kind,const Value *args,int argc,int line,Value *out) {
    GscFxLoadEvent load;GscFxPlayEvent play;char error[256];
    const char *end;size_t length;int n,accepted,context;int32_t index=0;
    if(!r->fx_enabled||!r->names_enabled||r->error[0]||!out||
       fx_static_issues(r->program,kind,NULL,argc)||(!args&&argc))
        return rfail(r,line,"invalid FX structural invocation");
    if(kind==GSC_FX_LOOP)return loop_fx_call(r,args,argc,line,out);
    if(kind==GSC_FX_LOAD) {
        if(argc<1)return rfail(r,line,"loadfx missing GetString argument 0");
        if(args[0].kind!=V_STRING)return rfail(r,line,"loadfx GetString argument 0 requires a string");
        end=memchr(args[0].str,0,sizeof(args[0].str));
        if(!end)return rfail(r,line,"loadfx unterminated resource name");
        length=(size_t)(end-args[0].str);
        if(length>=sizeof(load.name))return rfail(r,line,"loadfx oversized resource name");
        memset(&load,0,sizeof(load));memcpy(load.name,args[0].str,length+1);
        load.initial_window=!r->resources_closed;
        if(!r->fx.load)return rfail(r,line,"FX load host capability is unavailable");
        error[0]=0;accepted=r->fx.load(r->fx.user,r->now_ms,&load,&index,error,sizeof(error));error[sizeof(error)-1]=0;
        if(r->error[0])return 0;
        if(accepted!=1)return rfail(r,line,"FX load host rejected resource: %s",error[0]?error:"unspecified host failure");
        if(index<0||index>=GSC_FX_INDEX_CAPACITY)return rfail(r,line,"FX load host returned index outside bounded 0..63 domain");
        if(load.name[0]?index==0:(index!=0||!load.initial_window))
            return rfail(r,line,"FX load host returned index inconsistent with name and initial registration");
        context=console_context(r);if(context<0)return 0;
        scalar_integer(out,index);return 1;
    }
    memset(&play,0,sizeof(play));
    if(args[0].kind!=V_NUMBER||args[0].integer!=1)
        return rfail(r,line,"playfx GetInt argument 0 requires an int32");
    if(!scalar_get_integer(r,&args[0],line,&play.index))return 0;
    if(args[1].kind!=V_VECTOR)return rfail(r,line,"playfx GetVector argument 1 requires a vector");
    memcpy(play.origin,args[1].vector,sizeof(play.origin));
    if(argc==3) {
        if(args[2].kind!=V_VECTOR)return rfail(r,line,"playfx GetVector argument 2 requires a vector");
        play.has_direction=1;memcpy(play.direction,args[2].vector,sizeof(play.direction));
    }
    /* All intrinsic getters complete before the port finite-domain check. */
    for(n=0;n<3;n++)if(!isfinite(play.origin[n])||
       (play.has_direction&&!isfinite(play.direction[n])))
        return rfail(r,line,"playfx vector is outside finite bounded domain");
    if(!r->fx.play)return rfail(r,line,"FX play host capability is unavailable");
    error[0]=0;accepted=r->fx.play(r->fx.user,r->now_ms,&play,error,sizeof(error));error[sizeof(error)-1]=0;
    if(r->error[0])return 0;
    if(accepted!=1)return rfail(r,line,"FX play host rejected effect: %s",error[0]?error:"unspecified host failure");
    context=console_context(r);if(context<0)return 0;
    memset(out,0,sizeof(*out));return 1;
}
