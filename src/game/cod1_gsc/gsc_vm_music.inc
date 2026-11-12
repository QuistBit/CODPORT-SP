/* R107 typed music effects, independent of the frozen generic native ABI.
 * Original DLL handlers: musicplay 0x20035330, musicstop 0x20035380.
 * Admitted play argument is a string; original numeric/vector string coercions
 * are outside this bounded stage. Extra arguments are evaluated and ignored.
 * Stop admits 0/1 finite numeric argument. Alias resolution and actual playback
 * belong to the real host; there is no synthetic sound completion event. */
static int music_stop_ms(GscRuntime *r,const Value *argument,int line,int *out) {
    float seconds,scaled;double biased,converted;
    if(!scalar_float(r,argument,line,&seconds))return 0;
    /* Original stores seconds*1000 to float32, then adds double 2^-30 and
     * FISTP under nearest x87 rounding. Float32 half ties within int32 range
     * become strictly above half after this bias. Never inherit host fenv. */
    scaled=(float)((double)seconds*1000.0);
    if(!isfinite(scaled))return rfail(r,line,"music stop time overflow");
    biased=(double)scaled+0x1p-30;
    converted=floor(biased+0.5);
    if(converted<0.0)return rfail(r,line,"music stop time must be nonnegative after conversion");
    if(converted>(double)INT_MAX)return rfail(r,line,"music stop time overflow");
    *out=(int)converted;return 1;
}
static int music_runtime_preflight(const GscProgram *g,const GscMusicHost *host,
    char *error,size_t size) {
    int n,kind;
    for(n=0;n<g->code_count;n++) {
        const Instr *i=&g->code[n];
        if(i->kind!=I_CALL||i->target>=-1-BUILTIN_COUNT||i->target<-1-RESOURCE_TARGET_BASE)continue;
        kind=-1-BUILTIN_COUNT-i->target;
        if(!g->music_mode||i->module[0]||music_descriptor(g,i->name)!=kind||
           find_func(g,i->owner_module,i->name)>=0||!music_argc_valid(kind,i->argc)) {
            if(error&&size)snprintf(error,size,"line %d: invalid music binding",i->line);
            return 0;
        }
        if(!host||!(kind==GSC_MUSIC_PLAY?host->play:host->stop)) {
            if(error&&size)snprintf(error,size,"line %d: music %s requires enabled host capability",
                i->line,kind==GSC_MUSIC_PLAY?"play":"stop");
            return 0;
        }
    }
    return 1;
}
static int music_statement(GscRuntime *r,int kind,const Value *args,int argc,int line) {
    GscMusicEvent event;GscMusicCallback callback;char error[256];size_t length;int ok;
    if(!r->music_enabled||!r->names_enabled||r->error[0]||!music_argc_valid(kind,argc))
        return rfail(r,line,"invalid music invocation");
    memset(&event,0,sizeof(event));event.kind=kind;
    if(kind==GSC_MUSIC_PLAY) {
        const char *end;
        if(!args||args[0].kind!=V_STRING)return rfail(r,line,"musicplay admitted argument must be a string");
        end=memchr(args[0].str,0,sizeof(args[0].str));
        if(!end)return rfail(r,line,"unterminated music alias");
        length=(size_t)(end-args[0].str);
        if(!length||length>=sizeof(event.alias))return rfail(r,line,"empty or oversized music alias");
        memcpy(event.alias,args[0].str,length+1);callback=r->music.play;
    } else {
        if(argc&&(!args||!music_stop_ms(r,&args[0],line,&event.transition_ms)))return 0;
        callback=r->music.stop;
    }
    if(!callback)return rfail(r,line,"music host capability is unavailable");
    error[0]=0;ok=callback(r->music.user,r->now_ms,&event,error,sizeof(error));error[sizeof(error)-1]=0;
    /* Reentrant VM mutation poisons the outer call even if host returned 1. */
    if(r->error[0])return 0;
    if(ok!=1)return rfail(r,line,"music host rejected effect: %s",error[0]?error:"unspecified host failure");
    return 1;
}
