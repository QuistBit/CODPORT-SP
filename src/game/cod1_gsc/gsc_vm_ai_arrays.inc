/* R202 real Actor-domain snapshots. Only entity values enter the VM array;
 * Actor indices are ordering/validation metadata, never script identities. */
typedef struct {GscRuntime *runtime;int now_ms;GscAiArrayHost host;} AiArrayLease;
static int ai_array_current(void *context) {
    AiArrayLease *lease=context;GscRuntime *r=lease->runtime;
    if(!r||r->error[0]||!r->ai_arrays_enabled||!r->ai_array_busy||
       r->now_ms!=lease->now_ms||memcmp(&r->ai_arrays,&lease->host,sizeof(lease->host)))return 0;
    if(!entity_task_current(r))return 0;
    return !r->error[0]&&r->now_ms==lease->now_ms&&r->ai_arrays_enabled&&
        r->ai_array_busy&&!memcmp(&r->ai_arrays,&lease->host,sizeof(lease->host));
}
int gsc_runtime_select_ai_arrays(GscRuntime *r,const GscAiArrayHost *host) {
    if(!r||!host||!host->query||!host->validate||r->started||r->advancing||
       r->executing_task||r->task_count||r->error[0]||r->ai_arrays_enabled||
       !r->program->ai_arrays_selected||!r->state_enabled||!r->arrays_enabled||
       r->ai_array_busy||r->entity_method_busy||r->entity_field_busy||r->entity_inheritance_busy||
       r->entity_prepare_busy||r->entity_certificate_busy||r->entity_sequence_busy||
       runtime_private_overlap(r,host,sizeof(*host)))return 0;
    r->ai_arrays=*host;r->ai_arrays_enabled=1;return 1;
}
static int ai_array_call(GscRuntime *r,const Value *args,int argc,int line,Value *out) {
    GscAiArrayRequest request,expected_request;GscAiArraySnapshot snapshot,borrowed,expected_snapshot;
    AiArrayLease lease;Value staged;char error[256];uint32_t mask=0,n,k;int ok,live,accepted=0;
    if(!out||ai_array_static_issues(r->program,NULL,argc)||(!args&&argc))
        return rfail(r,line,"invalid getaiarray arity");
    if(!r->ai_arrays_enabled||!r->program->ai_arrays_selected||!r->arrays_enabled||!r->state_enabled||r->error[0])
        return rfail(r,line,"getaiarray capability is unavailable");
    for(k=0;k<(uint32_t)argc;k++) {
        if(args[k].kind!=V_STRING||!memchr(args[k].str,0,sizeof(args[k].str)))
            return rfail(r,line,"getaiarray parameter %u requires an ordinary string",k+1);
        if(control_field_equal(args[k].str,"axis"))mask|=2u;
        else if(control_field_equal(args[k].str,"allies"))mask|=4u;
        else if(control_field_equal(args[k].str,"neutral"))mask|=8u;
        else return rfail(r,line,"unknown team '%s' in getai (should be axis, allies, or neutral)",args[k].str);
    }
    if(!argc)mask=14u;
    if(r->ai_array_busy||r->entity_method_busy||r->entity_field_busy||r->entity_inheritance_busy||
       r->entity_prepare_busy||r->entity_certificate_busy||r->entity_sequence_busy)
        return rfail(r,line,"getaiarray callback reentered");
    if(!entity_task_current(r))return 0;
    memset(&request,0,sizeof(request));request.now_ms=r->now_ms;request.team_mask=mask;
    expected_request=request;memset(&snapshot,0,sizeof(snapshot));snapshot.count=UINT32_MAX;
    lease.runtime=r;lease.now_ms=r->now_ms;lease.host=r->ai_arrays;
    r->ai_array_busy=1;r->entity_method_busy=1;error[0]=0;
    ok=lease.host.query(lease.host.user,&request,&snapshot,error,sizeof(error),ai_array_current,&lease);error[sizeof(error)-1]=0;
    if(!ai_array_current(&lease))goto done;
    if(memcmp(&request,&expected_request,sizeof(request))){rfail(r,line,"getaiarray host mutated its borrowed request");goto done;}
    if(ok!=1){rfail(r,line,"getaiarray query refused: %s",error[0]?error:"host callback failed");goto done;}
    if(snapshot.count>GSC_AI_ARRAY_CAPACITY){rfail(r,line,"getaiarray host count exceeds 32 Actor records");goto done;}
    for(n=0;n<snapshot.count;n++) {
        const GscAiArrayMember *member=&snapshot.members[n];
        if(member->actor_index>=GSC_AI_ARRAY_CAPACITY||!member->entity.epoch||
           !member->entity.generation||member->entity.slot>=2048||
           (n&&(member->actor_index<=snapshot.members[n-1].actor_index||
                member->entity.epoch!=snapshot.members[0].entity.epoch))) {
            rfail(r,line,"getaiarray requires ordered Actor indices and current single-map entity identities");goto done;
        }
        for(k=0;k<n;k++)if(state_ref_equal(&member->entity,&snapshot.members[k].entity)) {
            rfail(r,line,"getaiarray returned duplicate entity identities");goto done;
        }
        live=state_entity_live(r,&member->entity);
        if(!ai_array_current(&lease))goto done;
        if(live!=1){if(!r->error[0])rfail(r,line,"getaiarray returned a stale entity identity");goto done;}
    }
    if(r->array_count>=GSC_ARRAY_CONTAINER_CAPACITY||snapshot.count>(uint32_t)(GSC_ARRAY_ENTRY_CAPACITY-r->array_entry_count)) {
        rfail(r,line,"getaiarray array arena limit");goto done;
    }
    borrowed=snapshot;expected_snapshot=snapshot;error[0]=0;
    ok=lease.host.validate(lease.host.user,&request,&borrowed,error,sizeof(error),ai_array_current,&lease);error[sizeof(error)-1]=0;
    if(!ai_array_current(&lease))goto done;
    if(memcmp(&request,&expected_request,sizeof(request))||memcmp(&borrowed,&expected_snapshot,sizeof(borrowed))) {
        rfail(r,line,"getaiarray validator mutated its borrowed request or snapshot");goto done;
    }
    if(ok!=1){rfail(r,line,"getaiarray snapshot validation refused: %s",error[0]?error:"host callback failed");goto done;}
    accepted=1;
done:
    r->ai_array_busy=0;r->entity_method_busy=0;
    if(!accepted&&!r->error[0]&&entity_task_current(r))
        rfail(r,line,"getaiarray callback changed its clock or capability");
    if(!accepted||r->error[0])return 0;
    /* No callbacks follow final snapshot validation. Allocation/publication
     * is atomic after all capacity and current-domain checks have passed. */
    if(r->array_count>=GSC_ARRAY_CONTAINER_CAPACITY||r->array_entry_count<0||
       r->array_entry_count>GSC_ARRAY_ENTRY_CAPACITY||
       snapshot.count>(uint32_t)(GSC_ARRAY_ENTRY_CAPACITY-r->array_entry_count))
        return rfail(r,line,"getaiarray array arena limit after callback");
    if(!array_new(r,line,&staged))return 0;
    for(n=0;n<snapshot.count;n++) {
        ArrayEntry entry;memset(&entry,0,sizeof(entry));entry.owner=(int)staged.array;
        entry.key.kind=V_NUMBER;entry.key.number=(int)n;entry.value.kind=V_ENTITY;
        entry.value.entity=snapshot.members[n].entity;array_entry_append55(r,&entry);
    }
    r->arrays[staged.array-1].count=r->arrays[staged.array-1].numeric_count=(int)snapshot.count;
    *out=staged;return 1;
}
