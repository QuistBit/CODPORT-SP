/* R111 bounded HUD messages. Original wrappers invoke the same localized-text
 * builder as objectives, beginning at argument zero, then publish one gm/gmb.
 * They never query the console gate and never add a console newline. Engine
 * GetString conversion remains an external boundary; this port admits plain
 * strings/localized tokens and explicit int32 decimal segments only. */
static GscHudCallback hud_callback(const GscHudHost *host,int kind) {
    if(!host)return NULL;
    return kind==GSC_HUD_PRINT?host->print:kind==GSC_HUD_BOLD?host->bold:NULL;
}
static int hud_runtime_preflight(const GscProgram *g,const GscHudHost *host,char *error,size_t size) {
    int n,kind;
    for(n=0;n<g->code_count;n++) {
        const Instr *i=&g->code[n];
        if(i->kind!=I_CALL||i->target>=-1-HUD_TARGET_BASE||i->target<-1-MISSION_TARGET_BASE)continue;
        kind=-1-HUD_TARGET_BASE-i->target;
        if(!g->hud_mode||i->module[0]||hud_descriptor(g,i->name)!=kind||
           find_func(g,i->owner_module,i->name)>=0||hud_static_issues(g,kind,i->args,i->argc)) {
            if(error&&size)snprintf(error,size,"line %d: invalid HUD binding",i->line);
            return 0;
        }
        if(!hud_callback(host,kind)) {
            if(error&&size)snprintf(error,size,"line %d: HUD requires selected host capability",i->line);
            return 0;
        }
    }
    for(n=0;n<g->expr_count;n++) {
        const Expr *e=&g->expr[n];
        if(e->call_binding!=GSC_EXPR_TARGET_HUD_NATIVE)continue;
        kind=e->target;
        if(!g->hud_mode||e->kind!=E_NATIVE_CALL||e->module[0]||
           hud_descriptor(g,e->name)!=kind||find_func(g,e->owner_module,e->name)>=0||
           hud_static_issues(g,kind,e->args,e->argc)) {
            if(error&&size)snprintf(error,size,"line %d: invalid HUD expression binding",e->line);
            return 0;
        }
        if(!hud_callback(host,kind)) {
            if(error&&size)snprintf(error,size,"line %d: HUD expression requires selected host capability",e->line);
            return 0;
        }
    }
    return 1;
}
static int hud_text_value(GscRuntime *r,const Value *value,int line,const char **out,size_t *length,char numeric[16]) {
    const char *end;size_t n;int32_t integer;
    /* HUD text is a quoted command payload, not objective Info-string data.
     * Percent, semicolon and backslash retain their literal native bytes. */
    if(value->kind==V_NUMBER) {
        if(!scalar_get_integer(r,value,line,&integer))return 0;
        snprintf(numeric,16,"%d",(int)integer);*out=numeric;*length=strlen(numeric);return 1;
    }
    if(value->kind!=V_STRING&&value->kind!=V_LOCALIZED_STRING)
        return rfail(r,line,"HUD text admits strings or exact int32 decimal segments only");
    end=memchr(value->str,0,sizeof(value->str));
    if(!end)return rfail(r,line,"unterminated HUD text");
    *length=(size_t)(end-value->str);*out=value->str;
    for(n=0;n<*length;n++) {
        unsigned char c=(unsigned char)value->str[n];
        if(c<32||c>126)return rfail(r,line,"invalid bounded HUD text byte");
        if(value->kind==V_LOCALIZED_STRING&&*length>1&&
           !((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='_'))
            return rfail(r,line,"invalid localized HUD token");
    }
    return 1;
}
static int hud_text_compose(GscRuntime *r,const Value *args,int argc,int line,char out[1024]) {
    int n,last_localized=1,prefix,alpha;unsigned char delimiter;size_t used=0,length=0,k;
    const char *text=NULL;char numeric[16];
    for(n=0;n<argc;n++) {
        if(!hud_text_value(r,&args[n],line,&text,&length,numeric))return 0;
        if(args[n].kind==V_LOCALIZED_STRING) {
            prefix=used>0;delimiter=0x14;last_localized=1;
        } else {
            alpha=0;
            for(k=0;k<length;k++)if((text[k]>='A'&&text[k]<='Z')||(text[k]>='a'&&text[k]<='z')){alpha=1;break;}
            prefix=alpha?used>0:last_localized;delimiter=(unsigned char)(alpha?0x14:0x15);last_localized=0;
        }
        if(used+length+(size_t)prefix>=1024)return rfail(r,line,"composed HUD text exceeds 1023 bytes");
        if(prefix)out[used++]=(char)delimiter;
        memcpy(out+used,text,length);used+=length;
    }
    if(last_localized) {
        if(used+1>=1024)return rfail(r,line,"composed HUD text exceeds 1023 bytes");
        out[used++]=0x15;
    }
    out[used]=0;return 1;
}
static int hud_statement(GscRuntime *r,int kind,const Value *receiver,const Value *args,int argc,int line) {
    GscHudEvent event;GscHudCallback callback;char error[256];int ok,context;
    if(!r->hud_enabled||!r->names_enabled||r->error[0]||receiver||
       hud_static_issues(r->program,kind,NULL,argc)||(!args&&argc))
        return rfail(r,line,"invalid HUD invocation");
    callback=hud_callback(&r->hud,kind);
    if(!callback)return rfail(r,line,"HUD host capability is unavailable");
    memset(&event,0,sizeof(event));event.kind=kind;event.audience=GSC_HUD_GLOBAL;
    if(!hud_text_compose(r,args,argc,line,event.text))return 0;
    event.length=strlen(event.text);
    error[0]=0;ok=callback(r->hud.user,r->now_ms,&event,error,sizeof(error));error[sizeof(error)-1]=0;
    if(r->error[0])return 0;
    if(ok!=1)return rfail(r,line,"HUD host rejected message: %s",error[0]?error:"unspecified host failure");
    context=console_context(r);if(context<0)return 0;
    return 1;
}
