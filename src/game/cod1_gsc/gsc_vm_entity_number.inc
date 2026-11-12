/* R149 exact-int32 getentbynum. Native GetInt(0) precedes its unsigned 2048
 * bound and inuse read. Float/string coercion and native VM memory tags remain
 * unselected. Extra arguments have already been evaluated and are ignored. */
static int entity_number_context(GscRuntime *r,int now,int line) {
    int context=console_context(r);
    if(context<=0)return 0;
    if(!r->entity_queries_enabled || !r->entities_enabled || !r->names_enabled ||
       !r->scalar_switch_enabled || !r->program || !r->program->entity_queries_mode ||
       r->now_ms!=now || now<0)
        return rfail(r,line,"getentbynum callback changed runtime clock or capability");
    return 1;
}
static int entity_number_call(GscRuntime *r,const Value *args,int argc,int line,Value *out) {
    int32_t slot;int accepted,found=-1,now,live;
    GscEntityRef ref;Value result;char error[256];
    if(!r || !r->entity_queries_enabled || !r->entities_enabled || !r->names_enabled ||
       !r->scalar_switch_enabled || r->error[0] || !out || argc<0 || argc>MAX_PARAMS ||
       (!args&&argc))return rfail(r,line,"invalid selected getentbynum invocation");
    if(!argc)return rfail(r,line,"getentbynum missing GetInt argument 0");
    if(args[0].kind!=V_NUMBER || args[0].integer!=1)
        return rfail(r,line,"getentbynum argument 0 is outside selected int32 domain");
    if(!scalar_get_integer(r,&args[0],line,&slot))return 0;
    if(!r->entity_queries.by_number)return rfail(r,line,"getentbynum host unavailable");
    now=r->now_ms;memset(&result,0,sizeof(result));
    if(!entity_number_context(r,now,line))return 0;
    if((uint32_t)slot>=2048) {*out=result;return 1;}
    memset(&ref,0,sizeof(ref));error[0]=0;
    accepted=r->entity_queries.by_number(r->entity_queries.user,now,slot,&ref,&found,error,sizeof(error));
    error[sizeof(error)-1]=0;
    if(!entity_number_context(r,now,line))return 0;
    if(accepted!=1)return rfail(r,line,"getentbynum host refused: %s",error[0]?error:"unspecified host failure");
    if(found!=0&&found!=1)return rfail(r,line,"getentbynum host returned invalid found flag");
    if(!found) {
        if(ref.epoch || ref.slot || ref.generation)
            return rfail(r,line,"getentbynum miss returned a nonempty identity");
        *out=result;return 1;
    }
    if(!ref.epoch || !ref.generation || ref.slot!=(uint32_t)slot)
        return rfail(r,line,"getentbynum host changed the requested slot or returned invalid identity");
    live=state_entity_live(r,&ref);
    if(live<0)return 0;
    if(!entity_number_context(r,now,line))return 0;
    if(!live)return rfail(r,line,"getentbynum host returned stale allocation");
    live=state_entity_live(r,&ref);
    if(live<0)return 0;
    if(!entity_number_context(r,now,line))return 0;
    if(!live)return rfail(r,line,"getentbynum allocation retired before publication");
    result.kind=V_ENTITY;result.entity=ref;*out=result;return 1;
}
