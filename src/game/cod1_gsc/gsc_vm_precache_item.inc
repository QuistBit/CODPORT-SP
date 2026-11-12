/* R121 original global precacheItem checks the outer game initialization flag
 * before GetString0, has no argc branch and ignores extra arguments. Original
 * engine children execute inline prefixes; this VM retains deferred scheduling
 * and independently admits the entire initial gsc_start ready drain. The old
 * shader/model root-prefix window remains unchanged. Name resolution and real
 * idempotent resource registration belong to the genuine typed host. Native
 * GetString formats int32 as %i, float32 as %g, vec3 with two decimal places;
 * finite values and a 127-byte printable name are selected port admission.
 * CRT locale/formatting parity outside this bounded domain is not claimed.
 * Undefined result is an explicit selected port policy. */
static int precache_item_runtime_preflight(const GscProgram *g,const GscPrecacheItemHost *host,char *error,size_t size) {
    int n,kind;
    for(n=0;n<g->code_count;n++) {
        const Instr *i=&g->code[n];
        if(i->kind!=I_CALL||i->target>=-1-PRECACHE_ITEM_TARGET_BASE)continue;
        if(g->save_game_mode&&i->target<-1-PRECACHE_ITEM_TARGET_BASE-GSC_PRECACHE_ITEM_REGISTER)continue;
        kind=-1-PRECACHE_ITEM_TARGET_BASE-i->target;
        if(!g->precache_item_mode||i->module[0]||precache_item_descriptor(g,i->name)!=kind||
           find_func(g,i->owner_module,i->name)>=0||precache_item_static_issues(g,kind,i->args,i->argc)) {
            if(error&&size)snprintf(error,size,"line %d: invalid precacheItem binding",i->line);
            return 0;
        }
        if(!host||!host->publish) {
            if(error&&size)snprintf(error,size,"line %d: precacheItem requires selected publish host capability",i->line);
            return 0;
        }
    }
    for(n=0;n<g->expr_count;n++) {
        const Expr *e=&g->expr[n];
        if(e->call_binding!=GSC_EXPR_TARGET_PRECACHE_ITEM_NATIVE)continue;
        if(!g->precache_item_mode||e->kind!=E_NATIVE_CALL||e->module[0]||
           precache_item_descriptor(g,e->name)!=e->target||find_func(g,e->owner_module,e->name)>=0||
           precache_item_static_issues(g,e->target,e->args,e->argc)) {
            if(error&&size)snprintf(error,size,"line %d: invalid precacheItem expression binding",e->line);
            return 0;
        }
        if(!host||!host->publish) {
            if(error&&size)snprintf(error,size,"line %d: precacheItem expression requires selected publish host capability",e->line);
            return 0;
        }
    }
    return 1;
}
static int precache_item_get_string(GscRuntime *r,const Value *value,int line,GscPrecacheItemEvent *event) {
    int length,n;int32_t integer;size_t bytes;
    if(value->kind==V_STRING) {
        if(!memchr(value->str,0,sizeof(value->str)))
            return rfail(r,line,"precacheItem GetString argument 0 exceeds selected string domain");
        bytes=strlen(value->str);memcpy(event->name,value->str,bytes+1);return 1;
    }
    if(value->kind==V_NUMBER) {
        if(value->integer) {
            if(!scalar_get_integer(r,value,line,&integer))return 0;
            /* int32 decimal matches native %i while remaining portable where
             * C int widths differ. No numeric state or Value limit is changed. */
            length=snprintf(event->name,sizeof(event->name),"%ld",(long)integer);
        } else {
            if(!isfinite(value->number))return rfail(r,line,"precacheItem GetString numeric argument is outside finite domain");
            length=snprintf(event->name,sizeof(event->name),"%g",(double)value->number);
        }
    } else if(value->kind==V_VECTOR) {
        for(n=0;n<3;n++)if(!isfinite(value->vector[n]))
            return rfail(r,line,"precacheItem GetString vector argument is outside finite domain");
        length=snprintf(event->name,sizeof(event->name),"(%.2f, %.2f, %.2f)",
            (double)value->vector[0],(double)value->vector[1],(double)value->vector[2]);
    } else return rfail(r,line,"precacheItem GetString argument 0 cannot cast admitted value to string");
    if(length<0||(size_t)length>=sizeof(event->name))
        return rfail(r,line,"precacheItem GetString formatted name exceeds selected 127-byte domain");
    return 1;
}
static int precache_item_call(GscRuntime *r,int kind,const Value *args,int argc,int line,Value *out) {
    GscPrecacheItemEvent event,copy;char error[256];int accepted,context;
    if(!r->precache_item_enabled||!r->names_enabled||r->error[0]||!out||
       precache_item_static_issues(r->program,kind,NULL,argc))
        return rfail(r,line,"invalid precacheItem structural invocation");
    /* Script expressions, including unused extras, have already evaluated.
     * The native gate then precedes the missing/wrong GetString0 boundary. */
    if(!r->precache_item_initial_window)
        return rfail(r,line,"precacheItem must occur during initial level startup before a wait");
    if(argc<1||!args)return rfail(r,line,"precacheItem GetString argument 0 is missing");
    memset(&event,0,sizeof(event));
    if(!precache_item_get_string(r,&args[0],line,&event))return 0;
    if(!precache_item_event_name_valid(&event))
        return rfail(r,line,"precacheItem name is outside selected printable string domain");
    if(!r->precache_item.publish)return rfail(r,line,"precacheItem publish host capability is unavailable");
    if(!r->next_precache_item_operation)return rfail(r,line,"precacheItem operation token exhausted");
    event.operation_id=r->next_precache_item_operation;
    r->next_precache_item_operation=event.operation_id==UINT64_MAX?0:event.operation_id+1;
    copy=event;error[0]=0;
    accepted=r->precache_item.publish(r->precache_item.user,r->now_ms,&copy,error,sizeof(error));error[sizeof(error)-1]=0;
    if(r->error[0])return 0;
    if(!entity_task_current(r))return 0;
    if(accepted!=1)return rfail(r,line,"precacheItem host rejected publication: %s",error[0]?error:"unspecified host failure");
    context=console_context(r);if(context<0)return 0;
    memset(out,0,sizeof(*out));return 1;
}
