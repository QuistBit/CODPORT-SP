/* R124 GAME 200361e0: argc==3, GetInt0/range 1..63, entity class/slot,
 * model presence, GetString2/quote check, bone lookup, exact %02d%s token.
 * The publisher owns map dictionary admission and attached event delivery.
 * No world position/axis is guessed from the entity's origin. */
static int tag_fx_runtime_preflight(const GscProgram *g,const GscTagFxHost *host,char *error,size_t size) {
    int n,needed=0;
    for(n=0;n<g->code_count;n++) {
        const Instr *i=&g->code[n];
        if(i->kind!=I_CALL || i->target>=-1-TAG_FX_TARGET_BASE ||
           i->target < -1-NATIVE_QUERY_TARGET_BASE)continue;
        if(!g->tag_fx_mode || i->module[0] ||
           tag_fx_descriptor(g,i->name)!=-1-TAG_FX_TARGET_BASE-i->target ||
           find_func(g,i->owner_module,i->name)>=0 || tag_fx_static_issues(g,GSC_TAG_FX_PLAY,i->args,i->argc))goto invalid;
        needed=1;
    }
    for(n=0;n<g->expr_count;n++) {
        const Expr *e=&g->expr[n];
        if(e->call_binding!=GSC_EXPR_TARGET_TAG_FX_NATIVE)continue;
        if(!g->tag_fx_mode || e->kind!=E_NATIVE_CALL || e->module[0] ||
           tag_fx_descriptor(g,e->name)!=e->target || find_func(g,e->owner_module,e->name)>=0 ||
           tag_fx_static_issues(g,e->target,e->args,e->argc))goto invalid;
        needed=1;
    }
    if(needed && (!host || !host->query || !host->publish)) {
        if(error&&size)snprintf(error,size,"playfxOnTag requires selected model/bone query and attached-event publisher");
        return 0;
    }
    return 1;
invalid:
    if(error&&size)snprintf(error,size,"invalid playfxOnTag binding");
    return 0;
}
static int tag_fx_current(GscRuntime *r,const GscEntityRef *ref,int line) {
    GscEntityRef copy=*ref;int ok;
    if(!entity_task_current(r))return 0;
    if(!tag_fx_ref_valid(ref) || !r->entities_enabled || !r->entities.validate)
        return rfail(r,line,"playfxOnTag requires a current entity");
    ok=entity_validate(r,&copy);
    if(r->error[0])return 0;
    if(ok!=1 || !tag_fx_ref_same(ref,&copy))return entity_task_current(r)?rfail(r,line,"playfxOnTag entity is stale or missing"):0;
    if(console_context(r)<=0)return 0;
    return 1;
}
static int tag_fx_query(GscRuntime *r,GscTagFxQuery *query,int line) {
    GscTagFxQuery copy=*query;char error[256];int accepted,now=r->now_ms;
    if(!tag_fx_query_valid(query) || !r->tag_fx.query || !tag_fx_current(r,&query->entity,line))return 0;
    error[0]=0;accepted=r->tag_fx.query(r->tag_fx.user,now,&copy,error,sizeof(error));error[sizeof(error)-1]=0;
    if(r->error[0])return 0;
    if(!entity_task_current(r))return 0;
    if(accepted!=1)return rfail(r,line,"playfxOnTag host rejected query: %s",error[0]?error:"unspecified host failure");
    if(r->now_ms!=now)return rfail(r,line,"playfxOnTag query changed logical clock");
    if(!tag_fx_reply_valid(query,&copy))return rfail(r,line,"playfxOnTag host returned an invalid query payload");
    if(!tag_fx_current(r,&query->entity,line))return 0;
    *query=copy;return 1;
}
static int tag_fx_get_string(GscRuntime *r,const Value *value,int line,char *out,size_t size) {
    int length,n;int32_t integer;size_t bytes;
    if(value->kind==V_STRING) {
        if(!memchr(value->str,0,sizeof(value->str)))return rfail(r,line,"playfxOnTag GetString2 exceeds selected string domain");
        bytes=strlen(value->str);if(bytes>=size)return rfail(r,line,"playfxOnTag GetString2 exceeds selected string domain");
        memcpy(out,value->str,bytes+1);return 1;
    }
    if(value->kind==V_NUMBER) {
        if(value->integer) {
            if(!scalar_get_integer(r,value,line,&integer))return 0;
            length=snprintf(out,size,"%ld",(long)integer);
        } else {
            if(!isfinite(value->number))return rfail(r,line,"playfxOnTag GetString2 is outside finite domain");
            length=snprintf(out,size,"%g",(double)value->number);
        }
    } else if(value->kind==V_VECTOR) {
        for(n=0;n<3;n++)if(!isfinite(value->vector[n]))return rfail(r,line,"playfxOnTag GetString2 is outside finite domain");
        length=snprintf(out,size,"(%.2f, %.2f, %.2f)",(double)value->vector[0],(double)value->vector[1],(double)value->vector[2]);
    } else return rfail(r,line,"playfxOnTag GetString2 cannot cast admitted value to string");
    if(length<0 || (size_t)length>=size)return rfail(r,line,"playfxOnTag formatted tag exceeds selected string domain");
    return 1;
}
static int tag_fx_call(GscRuntime *r,int kind,const Value *args,int argc,int line,Value *out) {
    GscTagFxQuery q;GscTagFxEvent event,copy;char error[256];int accepted,now;
    if(!r->tag_fx_enabled || !r->names_enabled || r->error[0] || !out || !args ||
       tag_fx_static_issues(r->program,kind,NULL,argc))return rfail(r,line,"playfxOnTag requires exactly three arguments");
    memset(&q,0,sizeof(q));q.phase=GSC_TAG_FX_MODEL;q.bone_index=-1;
    if(args[0].kind!=V_NUMBER || args[0].integer!=1 || !scalar_get_integer(r,&args[0],line,&q.index))
        return rfail(r,line,"playfxOnTag GetInt0 requires an int32");
    if(q.index<1 || q.index>=64)return rfail(r,line,"playfxOnTag effect id is outside 1..63");
    if(args[1].kind!=V_ENTITY)return rfail(r,line,"playfxOnTag argument 1 is not an entity");
    q.entity=args[1].entity;if(!tag_fx_current(r,&q.entity,line))return 0;
    if(!r->next_tag_fx_operation || r->next_tag_fx_operation==UINT64_MAX)
        return rfail(r,line,"playfxOnTag operation token exhausted");
    q.operation_id=r->next_tag_fx_operation++;
    if(!tag_fx_query(r,&q,line))return 0;
    if(!q.model_present)return rfail(r,line,"cannot play fx on entity with no model");
    if(!tag_fx_get_string(r,&args[2],line,q.tag,sizeof(q.tag)))return 0;
    if(!tag_fx_name_valid(q.tag,sizeof(q.tag)))return rfail(r,line,"playfxOnTag tag is outside selected printable quote-free domain");
    q.phase=GSC_TAG_FX_BONE;
    if(!tag_fx_query(r,&q,line))return 0;
    if(q.bone_index<0)return rfail(r,line,"tag '%s' does not exist on entity with model '%s'",q.tag,q.model_name);
    memset(&event,0,sizeof(event));event.operation_id=q.operation_id;event.entity=q.entity;
    event.index=q.index;event.model_identity=q.model_identity;memcpy(event.tag,q.tag,sizeof(event.tag));
    snprintf(event.token,sizeof(event.token),"%02d%s",(int)event.index,event.tag);
    if(!tag_fx_event_valid(&event) || !r->tag_fx.publish || !tag_fx_current(r,&event.entity,line))return 0;
    copy=event;error[0]=0;now=r->now_ms;
    accepted=r->tag_fx.publish(r->tag_fx.user,now,&copy,error,sizeof(error));error[sizeof(error)-1]=0;
    if(r->error[0])return 0;
    if(!entity_task_current(r))return 0;
    if(accepted!=1)return rfail(r,line,"playfxOnTag host rejected attached event: %s",error[0]?error:"unspecified host failure");
    if(r->now_ms!=now || !tag_fx_event_same(&event,&copy))return rfail(r,line,"playfxOnTag publisher changed clock or copied request");
    if(!tag_fx_current(r,&event.entity,line))return 0;
    memset(out,0,sizeof(*out));return 1;
}
