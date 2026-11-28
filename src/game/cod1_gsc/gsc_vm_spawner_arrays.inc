/* R131 source-owned complete typed spawner collection bridge. This builds
 * private VM arrays; it neither imports retail array handles nor invents native
 * actor/sentient entities. The frozen 256 numeric-key bound refuses overflow. */
static int spawner_arrays_runtime_preflight(const GscProgram *g,
    const GscSpawnerArrayHost *host,char *error,size_t size) {
    int n;
    for(n=0;n<g->code_count;n++) {
        const Instr *i=&g->code[n];
        if(i->kind!=I_CALL||i->target>=-1-SPAWNER_ARRAY_TARGET_BASE ||
           i->target<-1-RANDOMINT_TARGET_BASE)continue;
        if(!g->spawner_arrays_mode||i->module[0]||
           spawner_array_descriptor(g,i->name)!=GSC_SPAWNER_ARRAY_ALL||
           i->target!=-1-SPAWNER_ARRAY_TARGET_BASE-GSC_SPAWNER_ARRAY_ALL||
           find_func(g,i->owner_module,i->name)>=0||i->argc!=0||!host||!host->collect_all) {
            if(error&&size)snprintf(error,size,"line %d: getspawnerarray requires its selected complete collection host",i->line);
            return 0;
        }
    }
    for(n=0;n<g->expr_count;n++) {
        const Expr *e=&g->expr[n];
        if(e->call_binding!=GSC_EXPR_TARGET_SPAWNER_ARRAY_NATIVE)continue;
        if(!g->spawner_arrays_mode||e->kind!=E_NATIVE_CALL||e->module[0]||
           spawner_array_descriptor(g,e->name)!=GSC_SPAWNER_ARRAY_ALL||
           e->target!=GSC_SPAWNER_ARRAY_ALL||find_func(g,e->owner_module,e->name)>=0||
           e->argc!=0||!host||!host->collect_all) {
            if(error&&size)snprintf(error,size,"line %d: getspawnerarray expression requires its selected complete collection host",e->line);
            return 0;
        }
    }
    return 1;
}
static int spawner_array_context(GscRuntime *r,int now,int line) {
    int context=console_context(r);
    if(context<=0)return 0;
    if(!r->spawner_arrays_enabled||!r->arrays_enabled||!r->names_enabled||
       r->now_ms!=now)return rfail(r,line,"getspawnerarray callback changed runtime clock or capability");
    return 1;
}
static int spawner_array_call(GscRuntime *r,int kind,const Value *args,int argc,
    int line,Value *out) {
    GscEntityRef refs[GSC_ARRAY_CONTAINER_CAPACITY];size_t count=(size_t)-1,n;
    char error[256];Value result;int accepted,now,live,old_arrays,old_entries;
    (void)args;
    if(!r->spawner_arrays_enabled||!r->arrays_enabled||!r->names_enabled||r->error[0]||
       !out||spawner_array_static_issues(kind,argc)||!r->spawner_arrays.collect_all)
        return rfail(r,line,"getspawnerarray requires zero arguments and its selected collection capability");
    now=r->now_ms;
    if(!spawner_array_context(r,now,line))return 0;
    if(r->array_count>=GSC_ARRAY_CONTAINER_CAPACITY)return rfail(r,line,"getspawnerarray array container limit");
    memset(refs,0,sizeof(refs));error[0]=0;
    accepted=r->spawner_arrays.collect_all(r->spawner_arrays.user,now,refs,
        GSC_ARRAY_CONTAINER_CAPACITY,&count,error,sizeof(error));
    error[sizeof(error)-1]=0;
    if(!spawner_array_context(r,now,line))return 0;
    if(accepted!=1)return rfail(r,line,"getspawnerarray collection host: %s",error[0]?error:"unspecified host failure");
    if(count>GSC_ARRAY_CONTAINER_CAPACITY)return rfail(r,line,"getspawnerarray host overflow; no truncation");
    for(n=0;n<count;n++) {
        if(!refs[n].epoch||!refs[n].generation||refs[n].slot>=2048||
           (n&&(refs[n].epoch!=refs[0].epoch||refs[n].slot<=refs[n-1].slot)))
            return rfail(r,line,"getspawnerarray requires ordered unique identities from one map epoch");
        live=state_entity_live(r,&refs[n]);
        if(live<0)return 0;
        if(!spawner_array_context(r,now,line))return 0;
        if(!live)return rfail(r,line,"getspawnerarray member is stale");
    }
    if(r->array_count>=GSC_ARRAY_CONTAINER_CAPACITY||
       count>(size_t)(GSC_ARRAY_ENTRY_CAPACITY-r->array_entry_count))
        return rfail(r,line,"getspawnerarray array arena limit");
    old_arrays=r->array_count;old_entries=r->array_entry_count;
    if(!array_new(r,line,&result))return 0;
    for(n=0;n<count;n++) {
        ArrayEntry entry;memset(&entry,0,sizeof(entry));entry.owner=(int)result.array;
        entry.key.kind=V_NUMBER;entry.key.number=(int)n;
        entry.value.kind=V_ENTITY;entry.value.entity=refs[n];
        array_entry_append55(r,&entry);
    }
    r->arrays[result.array-1].count=r->arrays[result.array-1].numeric_count=(int)count;
    /* Publication remains private until every full identity and task lease is
     * checked again. Any late refusal erases the entire newly staged array. */
    for(n=0;n<count;n++) {
        live=state_entity_live(r,&refs[n]);
        if(!spawner_array_context(r,now,line))goto rollback;
        if(live!=1) {if(live==0)rfail(r,line,"getspawnerarray member changed during publication");goto rollback;}
    }
    if(!spawner_array_context(r,now,line))goto rollback;
    *out=result;return 1;
rollback:
    memset(&r->arrays[old_arrays],0,(size_t)(r->array_count-old_arrays)*sizeof(r->arrays[0]));
    memset(&r->array_entries[old_entries],0,(size_t)(r->array_entry_count-old_entries)*sizeof(r->array_entries[0]));
    r->array_count=old_arrays;r->array_entry_count=old_entries;array_index_rebuild55(r);return 0;
}
