/* R149 named team collections use the genuine typed GAME spawner registry.
 * Every ordinary string argument contributes a named team bit. Native getters
 * and unknown/missing-team errors belong to execution, before collection.
 * Array ownership and 256-member limits are the inherited bounded port policy. */
static int spawner_team_array_context(GscRuntime *r,int now,int line) {
    int context=console_context(r);
    if(context<=0)return 0;
    if(!r->entity_queries_enabled||!r->arrays_enabled||!r->names_enabled||r->now_ms!=now)
        return rfail(r,line,"getspawnerteamarray callback changed runtime clock or capability");
    return 1;
}
static int spawner_team_array_call(GscRuntime *r,const Value *args,int argc,
    int line,Value *out) {
    GscEntityRef refs[GSC_ARRAY_CONTAINER_CAPACITY];size_t count=(size_t)-1,n;
    char error[256];Value result;unsigned int mask=0;int k,accepted,now,live,old_arrays,old_entries;
    if(!r)return 0;
    if(!r->entity_queries_enabled||!r->arrays_enabled||!r->names_enabled||r->error[0]||
       !out||argc<0||argc>MAX_PARAMS||(!args&&argc))
        return rfail(r,line,"invalid getspawnerteamarray invocation");
    for(k=0;k<argc;k++) {
        if(args[k].kind!=V_STRING||!memchr(args[k].str,0,sizeof(args[k].str)))
            return rfail(r,line,"getspawnerteamarray parameter %d requires an ordinary string",k+1);
        if(control_field_equal(args[k].str,"axis"))mask|=2u;
        else if(control_field_equal(args[k].str,"allies"))mask|=4u;
        else if(control_field_equal(args[k].str,"neutral"))mask|=8u;
        else return rfail(r,line,"unknown team '%s' in getspawnerteamarray",args[k].str);
    }
    if(!mask)return rfail(r,line,"no team was specified - use getspawnerarray instead");
    if(!r->entity_queries.collect_team)
        return rfail(r,line,"getspawnerteamarray collection capability unavailable");
    now=r->now_ms;
    if(!spawner_team_array_context(r,now,line))return 0;
    if(r->array_count>=GSC_ARRAY_CONTAINER_CAPACITY)return rfail(r,line,"getspawnerteamarray array container limit");
    memset(refs,0,sizeof(refs));error[0]=0;
    accepted=r->entity_queries.collect_team(r->entity_queries.user,now,mask,refs,
        GSC_ARRAY_CONTAINER_CAPACITY,&count,error,sizeof(error));
    error[sizeof(error)-1]=0;
    if(!spawner_team_array_context(r,now,line))return 0;
    if(accepted!=1)return rfail(r,line,"getspawnerteamarray collection host: %s",error[0]?error:"unspecified host failure");
    if(count>GSC_ARRAY_CONTAINER_CAPACITY)return rfail(r,line,"getspawnerteamarray host overflow; no truncation");
    for(n=0;n<count;n++) {
        if(!refs[n].epoch||!refs[n].generation||refs[n].slot>=2048||
           (n&&(refs[n].epoch!=refs[0].epoch||refs[n].slot<=refs[n-1].slot)))
            return rfail(r,line,"getspawnerteamarray requires ordered unique identities from one map epoch");
        live=state_entity_live(r,&refs[n]);
        if(live<0)return 0;
        if(!spawner_team_array_context(r,now,line))return 0;
        if(!live)return rfail(r,line,"getspawnerteamarray member is stale");
    }
    if(r->array_count>=GSC_ARRAY_CONTAINER_CAPACITY||
       count>(size_t)(GSC_ARRAY_ENTRY_CAPACITY-r->array_entry_count))
        return rfail(r,line,"getspawnerteamarray array arena limit");
    old_arrays=r->array_count;old_entries=r->array_entry_count;
    if(!array_new(r,line,&result))return 0;
    for(n=0;n<count;n++) {
        ArrayEntry entry;memset(&entry,0,sizeof(entry));entry.owner=(int)result.array;
        entry.key.kind=V_NUMBER;entry.key.number=(int)n;
        entry.value.kind=V_ENTITY;entry.value.entity=refs[n];
        r->array_entries[r->array_entry_count++]=entry;
    }
    r->arrays[result.array-1].count=r->arrays[result.array-1].numeric_count=(int)count;
    for(n=0;n<count;n++) {
        live=state_entity_live(r,&refs[n]);
        if(!spawner_team_array_context(r,now,line))goto rollback;
        if(live!=1) {if(live==0)rfail(r,line,"getspawnerteamarray member changed during publication");goto rollback;}
    }
    if(!spawner_team_array_context(r,now,line))goto rollback;
    *out=result;return 1;
rollback:
    memset(&r->arrays[old_arrays],0,(size_t)(r->array_count-old_arrays)*sizeof(r->arrays[0]));
    memset(&r->array_entries[old_entries],0,(size_t)(r->array_entry_count-old_entries)*sizeof(r->array_entries[0]));
    r->array_count=old_arrays;r->array_entry_count=old_entries;return 0;
}
