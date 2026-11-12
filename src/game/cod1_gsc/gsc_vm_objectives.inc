/* R109 separately selected objective effects. Original DLL owns CS records;
 * this VM admits exact int32 indices, ordinary/localized text and finite vectors.
 * It never substitutes generic native registrations or mission completion. */
static GscObjectiveCallback objective_callback(const GscObjectiveHost *host,int kind) {
    if(!host)return NULL;
    switch(kind) {
      case GSC_OBJECTIVE_ADD:return host->add;
      case GSC_OBJECTIVE_DELETE:return host->delete_record;
      case GSC_OBJECTIVE_STATE:return host->state;
      case GSC_OBJECTIVE_STRING:return host->string;
      case GSC_OBJECTIVE_STRING_NOMESSAGE:return host->string_nomessage;
      case GSC_OBJECTIVE_POSITION:return host->position;
      case GSC_OBJECTIVE_CURRENT:return host->current;
      case GSC_OBJECTIVE_RING:return host->ring;
    }
    return NULL;
}
static int objective_runtime_preflight(const GscProgram *g,const GscObjectiveHost *host,
    char *error,size_t size) {
    int n,kind;
    for(n=0;n<g->code_count;n++) {
        const Instr *i=&g->code[n];
        if(i->kind!=I_CALL||i->target>=-1-OBJECTIVE_TARGET_BASE||i->target<-1-CONSOLE_TARGET_BASE)continue;
        kind=-1-OBJECTIVE_TARGET_BASE-i->target;
        if(!g->objectives_mode||i->module[0]||objective_descriptor(g,i->name)!=kind||
           find_func(g,i->owner_module,i->name)>=0||objective_static_issues(g,kind,i->args,i->argc)) {
            if(error&&size)snprintf(error,size,"line %d: invalid objective binding",i->line);
            return 0;
        }
        if(!objective_callback(host,kind)) {
            if(error&&size)snprintf(error,size,"line %d: objective requires enabled host capability",i->line);
            return 0;
        }
    }
    for(n=0;n<g->expr_count;n++) {
        const Expr *e=&g->expr[n];
        if(e->call_binding!=GSC_EXPR_TARGET_OBJECTIVE_NATIVE)continue;
        kind=e->target;
        if(!g->objectives_mode||e->kind!=E_NATIVE_CALL||e->module[0]||
           objective_descriptor(g,e->name)!=kind||find_func(g,e->owner_module,e->name)>=0||
           objective_static_issues(g,kind,e->args,e->argc)) {
            if(error&&size)snprintf(error,size,"line %d: invalid objective expression binding",e->line);
            return 0;
        }
        if(!objective_callback(host,kind)) {
            if(error&&size)snprintf(error,size,"line %d: objective expression requires enabled host capability",e->line);
            return 0;
        }
    }
    return 1;
}
static int objective_index(GscRuntime *r,const Value *value,int line,int *out) {
    int32_t index;
    if(!scalar_get_integer(r,value,line,&index))return 0;
    if(index<0||index>15)return rfail(r,line,"objective index must be in 0..15");
    *out=(int)index;return 1;
}
static int objective_state_value(GscRuntime *r,const Value *value,int line,int *out) {
    static const char *const states[]={"empty","active","invisible","done","current","failed"};
    int n;
    if(value->kind!=V_STRING)return rfail(r,line,"objective state must be an ordinary string");
    for(n=0;n<6;n++)if(!strcmp(value->str,states[n])){*out=n;return 1;}
    return rfail(r,line,"invalid objective state");
}
static int objective_text_value(GscRuntime *r,const Value *value,int line,const char **out,size_t *length,char numeric[16]) {
    const char *end;size_t n;int32_t integer;
    /* Decimal int32 segments cover authored objective counters. The original
     * engine GetString coercion remains an external boundary; floats/vectors,
     * refs and other tags are deliberately refused in this bounded stage. */
    if(value->kind==V_NUMBER) {
        if(!scalar_get_integer(r,value,line,&integer))return 0;
        snprintf(numeric,16,"%d",(int)integer);*out=numeric;*length=strlen(numeric);return 1;
    }
    if(value->kind!=V_STRING&&value->kind!=V_LOCALIZED_STRING)
        return rfail(r,line,"objective text admits strings or exact int32 decimal segments only");
    end=memchr(value->str,0,sizeof(value->str));
    if(!end)return rfail(r,line,"unterminated objective text");
    *length=(size_t)(end-value->str);*out=value->str;
    for(n=0;n<*length;n++) {
        unsigned char c=(unsigned char)value->str[n];
        if(c<32||c>126||c=='"'||c==';'||c=='\\')
            return rfail(r,line,"invalid objective text byte");
        if(value->kind==V_LOCALIZED_STRING&&*length>1&&
           !((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='_'))
            return rfail(r,line,"invalid localized objective token");
    }
    return 1;
}
static int objective_origin(GscRuntime *r,const Value *value,int line,int out[3]) {
    int n;double number;
    if(value->kind!=V_VECTOR)return rfail(r,line,"objective position must be a vector");
    for(n=0;n<3;n++) {
        if(!isfinite(value->vector[n]))return rfail(r,line,"objective position must be finite");
        number=trunc((double)value->vector[n]);
        if(number<INT32_MIN||number>INT32_MAX)return rfail(r,line,"objective position exceeds admitted int32 range");
        out[n]=(int)number;
    }
    return 1;
}
/* Original helper tracks localization and whether an ordinary segment has
 * C-locale ASCII alphabetic bytes. Marker spelling is intentionally retained,
 * including the final 0x15 after a last localized (or absent) segment. */
static int objective_text_compose(GscRuntime *r,const Value *args,int argc,int line,char out[1024]) {
    int n,last_localized=1,prefix,alpha;unsigned char delimiter;size_t used=0,length,k;
    const char *text;char numeric[16];
    for(n=1;n<argc;n++) {
        if(!objective_text_value(r,&args[n],line,&text,&length,numeric))return 0;
        if(args[n].kind==V_LOCALIZED_STRING) {
            prefix=used>0;delimiter=0x14;last_localized=1;
        } else {
            alpha=0;
            for(k=0;k<length;k++)if((text[k]>='A'&&text[k]<='Z')||(text[k]>='a'&&text[k]<='z')){alpha=1;break;}
            prefix=alpha?used>0:last_localized;delimiter=(unsigned char)(alpha?0x14:0x15);last_localized=0;
        }
        if(used+length+(size_t)prefix>=1024)return rfail(r,line,"composed objective text exceeds 1023 bytes");
        if(prefix)out[used++]=(char)delimiter;
        memcpy(out+used,text,length);used+=length;
    }
    if(last_localized) {
        if(used+1>=1024)return rfail(r,line,"composed objective text exceeds 1023 bytes");
        out[used++]=0x15;
    }
    out[used]=0;return 1;
}
static int objective_statement(GscRuntime *r,int kind,const Value *args,int argc,int line) {
    GscObjectiveEvent event;GscObjectiveCallback callback;char error[256];int ok,n,index=0;
    const char *text;size_t length;char numeric[16];
    if(!r->objectives_enabled||!r->names_enabled||r->error[0]||!objective_argc_valid(kind,argc))
        return rfail(r,line,"invalid objective invocation");
    memset(&event,0,sizeof(event));event.kind=kind;
    if(kind==GSC_OBJECTIVE_CURRENT) {
        for(n=0;n<argc;n++) {
            if(!objective_index(r,&args[n],line,&index))return 0;
            event.current_mask|=1u<<(unsigned)index;
        }
    } else {
        if(!args||!objective_index(r,&args[0],line,&event.index))return 0;
        if(kind==GSC_OBJECTIVE_ADD||kind==GSC_OBJECTIVE_STATE) {
            if(!objective_state_value(r,&args[1],line,&event.state))return 0;
        }
        if(kind==GSC_OBJECTIVE_ADD&&argc>2) {
            if(!objective_text_value(r,&args[2],line,&text,&length,numeric))return 0;
            memcpy(event.text,text,length+1);event.has_text=1;
        }
        if(kind==GSC_OBJECTIVE_STRING||kind==GSC_OBJECTIVE_STRING_NOMESSAGE) {
            if(!objective_text_compose(r,args,argc,line,event.text))return 0;
            event.has_text=1;
        }
        if((kind==GSC_OBJECTIVE_ADD&&argc>3)||kind==GSC_OBJECTIVE_POSITION) {
            n=kind==GSC_OBJECTIVE_ADD?3:1;
            if(!objective_origin(r,&args[n],line,event.origin))return 0;
            event.has_origin=1;
        }
    }
    callback=objective_callback(&r->objectives,kind);
    if(!callback)return rfail(r,line,"objective host capability is unavailable");
    error[0]=0;ok=callback(r->objectives.user,r->now_ms,&event,error,sizeof(error));error[sizeof(error)-1]=0;
    if(r->error[0])return 0;
    if(ok!=1)return rfail(r,line,"objective host rejected effect: %s",error[0]?error:"unspecified host failure");
    return 1;
}
