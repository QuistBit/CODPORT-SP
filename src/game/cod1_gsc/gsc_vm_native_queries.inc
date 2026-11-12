/* Original wrappers GetString(0), then integer/float/string cvar service,
 * then AddInt/AddFloat/AddString. There is no wrapper argc guard. Extra value
 * expressions evaluate normally; their values are ignored. Ordinary strings
 * <=127 bytes and finite float32 replies are explicit bounded port domains. */
static int native_queries_runtime_preflight(const GscProgram *g,
    const GscNativeQueryHost *host,char *error,size_t size) {
    int n,kind;
    for(n=0;n<g->code_count;n++) {
        const Instr *i=&g->code[n];
        if(i->kind!=I_CALL || i->target>=-1-NATIVE_QUERY_TARGET_BASE ||
           i->target<-1-SPAWNER_ARRAY_TARGET_BASE)continue;
        kind=-1-NATIVE_QUERY_TARGET_BASE-i->target;
        if(kind==GSC_QUERY_PRECACHE_STRING&&g->precache_string_mode)continue;
        if(!g->native_queries_mode || i->module[0] ||
           native_query_descriptor(g,i->name)!=kind ||
           native_query_static_issues(g,kind,i->args,i->argc) || !host || !host->query) {
            if(error&&size)snprintf(error,size,"line %d: native cvar query requires valid selected host capability",i->line);
            return 0;
        }
    }
    for(n=0;n<g->expr_count;n++) {
        const Expr *e=&g->expr[n];
        if(e->call_binding!=GSC_EXPR_TARGET_NATIVE_QUERY)continue;
        if(e->target==GSC_QUERY_PRECACHE_STRING&&g->precache_string_mode)continue;
        if(!g->native_queries_mode || e->kind!=E_NATIVE_CALL || e->module[0] ||
           native_query_descriptor(g,e->name)!=e->target ||
           native_query_static_issues(g,e->target,e->args,e->argc) || !host || !host->query) {
            if(error&&size)snprintf(error,size,"line %d: native cvar query expression requires valid selected host capability",e->line);
            return 0;
        }
    }
    return 1;
}
static int native_query_call(GscRuntime *r,int kind,const Value *args,int argc,
    int line,Value *out) {
    GscNativeQuery request,reply;Value result;char error[256];int accepted,context,now;
    if(kind==GSC_QUERY_PRECACHE_STRING)return precache_string_call(r,args,argc,line,out);
    if(!r->native_queries_enabled || !r->names_enabled || r->error[0] || !out ||
       native_query_static_issues(r->program,kind,NULL,argc) || (!args&&argc))
        return rfail(r,line,"invalid native cvar query invocation");
    if(kind==GSC_QUERY_LEVEL_TIME&&(!r->precache_string_enabled||r->now_ms<0))
        return rfail(r,line,"GetTime requires the selected highest runtime constructor");
    if(kind==GSC_QUERY_DIFFICULTY&&(!r->structs_enabled||r->now_ms<0))
        return rfail(r,line,"GetDifficulty requires the selected R148 runtime constructor");
    if(kind!=GSC_QUERY_LEVEL_TIME && kind!=GSC_QUERY_DIFFICULTY) {
        if(argc<1)return rfail(r,line,"native cvar query missing GetString argument 0");
        if(args[0].kind!=V_STRING || !memchr(args[0].str,0,sizeof(args[0].str)))
            return rfail(r,line,"native cvar query requires admitted ordinary string argument 0");
    }
    if(!r->native_queries.query)return rfail(r,line,"native cvar query host unavailable");
    if(!r->next_native_query_operation)return rfail(r,line,"native cvar query operation exhausted");
    memset(&request,0,sizeof(request));request.kind=kind;
    request.operation_id=r->next_native_query_operation;
    r->next_native_query_operation=request.operation_id==UINT64_MAX?0:request.operation_id+1;
    if(kind!=GSC_QUERY_LEVEL_TIME && kind!=GSC_QUERY_DIFFICULTY)memcpy(request.name,args[0].str,sizeof(request.name));
    reply=request;error[0]=0;now=r->now_ms;
    accepted=r->native_queries.query(r->native_queries.user,now,&reply,error,sizeof(error));
    error[sizeof(error)-1]=0;
    if(r->error[0])return 0;
    if(r->console_int32_enabled&&console_context(r)<=0)return 0;
    if(r->now_ms!=now || (kind==GSC_QUERY_LEVEL_TIME &&
       (!r->native_queries_enabled||!r->names_enabled||!r->precache_string_enabled)) ||
       (kind==GSC_QUERY_DIFFICULTY &&
        (!r->structs_enabled||!r->native_queries_enabled||!r->names_enabled)))
        return rfail(r,line,"native query callback changed runtime clock or selected capability");
    if(accepted!=1)return rfail(r,line,"native cvar query host refused: %s",error[0]?error:"unspecified host failure");
    if(!native_query_reply_valid(&request,&reply))return rfail(r,line,"native cvar query host changed intent or returned invalid value");
    context=console_context(r);if(context<=0)return 0;
    if(r->now_ms!=now || (kind==GSC_QUERY_LEVEL_TIME &&
       (!r->native_queries_enabled||!r->names_enabled||!r->precache_string_enabled)) ||
       (kind==GSC_QUERY_DIFFICULTY &&
        (!r->structs_enabled||!r->native_queries_enabled||!r->names_enabled)))
        return rfail(r,line,"native query context changed runtime clock or selected capability");
    memset(&result,0,sizeof(result));
    if(kind==GSC_QUERY_CVAR_STRING || kind==GSC_QUERY_DIFFICULTY) {
        result.kind=V_STRING;memcpy(result.str,reply.string_value,sizeof(result.str));
    } else if(kind==GSC_QUERY_CVAR_INTEGER||kind==GSC_QUERY_LEVEL_TIME)scalar_integer(&result,reply.integer_value);
    else {result.kind=V_NUMBER;result.number=reply.float_value;}
    *out=result;return 1;
}
/* Original getEntityNumber tail-calls AddInt with its already decoded receiver
 * slot. Port admission requires a live full epoch/slot/generation reference;
 * repeated validation around callbacks never retargets a replacement entity. */
static int native_entity_number(GscRuntime *r,const Value *receiver,int argc,
    int line,Value *out) {
    GscEntityRef identity;Value result;int live,context;
    if(!r->native_queries_enabled || !receiver || receiver->kind!=V_ENTITY || !out ||
       argc<0 || argc>MAX_PARAMS)return rfail(r,line,"getEntityNumber requires an admitted entity receiver");
    identity=receiver->entity;
    if(identity.slot>=2048)return rfail(r,line,"getEntityNumber entity slot outside original 2048-slot range");
    live=state_entity_live(r,&identity);if(live<0)return 0;
    context=console_context(r);if(context<=0)return 0;
    if(!live)return rfail(r,line,"getEntityNumber receiver is stale");
    live=state_entity_live(r,&identity);if(live<0)return 0;
    context=console_context(r);if(context<=0)return 0;
    if(!live)return rfail(r,line,"getEntityNumber receiver changed during validation");
    scalar_integer(&result,(int32_t)identity.slot);*out=result;return 1;
}
