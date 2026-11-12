/* Same-allocation initial private fields then guarded native preparation.
 * The host's final commit guard is synchronous; no callback follows it before
 * the staged private fields are published. */
static int prepare_entity_entry_phase(GscRuntime *r,const GscPreparedEntityEntry *input,
    const GscPreparedEntityEntryHost *host_input,GscPreparedEntityEntryRun *out,char *error,size_t size,int construct_only) {
    GscPreparedEntityEntry request;GscPreparedEntityEntryHost host;GscPreparedEntityEntryRun result;
    StateField staged[GSC_ENTITY_INITIAL_FIELDS_MAX];int slots[GSC_ENTITY_INITIAL_FIELDS_MAX];
    GscEntityExecutionCertificate produced,certificate;GscEntityScriptEntry borrowed;
    char why[256];uint32_t i,j;int n,free_count=0,module,accepted,ok=0;
    if(!r||!input||!host_input||!out||
       runtime_private_overlap(r,input,sizeof(*input))||
       runtime_private_overlap(r,host_input,sizeof(*host_input))||
       Cod1ModelStringOverlap(input,sizeof(*input),out,sizeof(*out))||
       Cod1ModelStringOverlap(host_input,sizeof(*host_input),out,sizeof(*out))||
       runtime_private_overlap(r,out,sizeof(*out))||
       runtime_private_overlap(r,error,size)||
       Cod1ModelStringOverlap(error,size,input,sizeof(*input))||
       Cod1ModelStringOverlap(error,size,host_input,sizeof(*host_input))||
       Cod1ModelStringOverlap(error,size,out,sizeof(*out)))return 0;
    request=*input;host=*host_input;
    if(!host.commit_fields||!host.prepare||!request.entry.entity.epoch||!request.entry.entity.generation||
       request.field_count>GSC_ENTITY_INITIAL_FIELDS_MAX||
       !memchr(request.entry.module,0,sizeof(request.entry.module))||!request.entry.module[0]||
       !memchr(request.entry.function,0,sizeof(request.entry.function))||!request.entry.function[0])
        return inherited_entry_error(error,size,"invalid prepared entity entry request");
    memset(&result,0,sizeof(result));*out=result;
    if(r->entity_sequence_busy||r->entity_prepare_busy||r->entity_certificate_busy||
       r->advancing||r->executing_task||r->entity_entry_depth||r->entity_inheritance_busy||r->entity_field_busy||r->entity_method_busy||
       (r->frame_cadence_enabled&&r->now_ms!=r->cadence_drain_ms))
        return inherited_entry_error(error,size,"prepared entry requires a quiescent runtime");
    if((construct_only==2?!r->bsp_construction_only:
        construct_only?!gsc_runtime_resource_bootstrap_available(r):(!r->started||r->bsp_construction_only))||!r->program->linked||r->error[0]||!r->state_enabled||!r->arrays_enabled||
       !r->structs_enabled||!r->lifetime_enabled||!r->results_enabled||!r->scalar_switch_enabled||!r->names_enabled||
       (module=find_module(r->program,request.entry.module))<0||find_func(r->program,module,request.entry.function)<0)
        return inherited_entry_error(error,size,"prepared entry profile or function is unavailable");
    memset(staged,0,sizeof(staged));
    for(i=0;i<request.field_count;++i) {
        const GscEntityInitialStringField *field=&request.fields[i];
        if(!memchr(field->name,0,sizeof(field->name))||!field->name[0]||
           !memchr(field->string,0,sizeof(field->string))||control_metadata_name(field->name)||
           control_property_descriptor(r->program,field->name)>=0)
            return inherited_entry_error(error,size,"invalid initial private string field");
        for(j=0;j<i;++j)if(r->controls_enabled?control_field_equal(field->name,staged[j].name):!strcmp(field->name,staged[j].name))
            return inherited_entry_error(error,size,"duplicate initial private field");
        staged[i].used=1;staged[i].owner_kind=V_ENTITY;staged[i].owner=request.entry.entity;
        strcpy(staged[i].name,field->name);staged[i].value.kind=V_STRING;strcpy(staged[i].value.str,field->string);
    }
    if(!state_fields_can_fit(r,request.field_count))return inherited_entry_error(error,size,"initial private field capacity is unavailable");
    if(!state_fields_reserve_free(r,request.field_count,0))return inherited_entry_error(error,size,r->error);
    for(n=0;n<r->state_field_capacity;++n) {
        if(r->state_fields[n].used) {
            if(r->state_fields[n].owner_kind==V_ENTITY&&state_ref_equal(&r->state_fields[n].owner,&request.entry.entity))
                return inherited_entry_error(error,size,"prepared entry requires a fresh private field bag");
        } else if(free_count<(int)request.field_count)slots[free_count++]=n;
    }
    if(free_count!=(int)request.field_count)return inherited_entry_error(error,size,"initial private field capacity is unavailable");
    r->entity_sequence_busy=1;r->advancing=1;r->entity_inheritance_busy=1;
    if(state_entity_live(r,&request.entry.entity)!=1)goto fields_refused;
    for(i=0;i<request.field_count;++i)
        if(entity_field_classify(r,&request.entry.entity,staged[i].name,0)!=0)goto fields_refused;
    if(state_entity_live(r,&request.entry.entity)!=1||r->error[0])goto fields_refused;
    for(n=0;n<r->state_field_capacity;++n)if(r->state_fields[n].used&&r->state_fields[n].owner_kind==V_ENTITY&&
        state_ref_equal(&r->state_fields[n].owner,&request.entry.entity))goto fields_refused;
    for(i=0;i<request.field_count;++i)if(r->state_fields[slots[i]].used)goto fields_refused;
    memset(why,0,sizeof(why));borrowed=request.entry;r->entity_prepare_busy=1;
    accepted=host.commit_fields(host.user,&borrowed,why,sizeof(why));r->entity_prepare_busy=0;
    why[sizeof(why)-1]=0;
    if(accepted!=1||r->error[0]) {
        inherited_entry_error(error,size,r->error[0]?r->error:why[0]?why:"initial field commit refused");goto done;
    }
    for(i=0;i<request.field_count;++i)r->state_fields[slots[i]]=staged[i];
    result.stage=GSC_PREPARED_ENTRY_FIELDS;result.fields=request.field_count;
    r->entity_inheritance_busy=0;
    memset(why,0,sizeof(why));memset(&produced,0,sizeof(produced));borrowed=request.entry;r->entity_prepare_busy=1;
    accepted=host.prepare(host.user,&borrowed,&produced,why,sizeof(why));certificate=produced;
    r->entity_prepare_busy=0;r->advancing=0;why[sizeof(why)-1]=0;
    if(accepted!=1||r->error[0]) {
        inherited_entry_error(error,size,r->error[0]?r->error:why[0]?why:"prepared native stage refused");goto done;
    }
    result.stage=GSC_PREPARED_ENTRY_PREPARED;
    if(!certificate.serial||!certificate.current||!state_ref_equal(&certificate.owner,&request.entry.entity)) {
        inherited_entry_error(error,size,"invalid prepared entry certificate");goto done;
    }
    /* Construction has no script task and exports no execution certificate. */
    if(construct_only){ok=1;goto done;}
    if(!execute_entity_entry(r,&request.entry,&certificate,&result.entry)) {
        inherited_entry_error(error,size,r->error[0]?r->error:"prepared script entry refused");goto done;
    }
    result.stage=GSC_PREPARED_ENTRY_ENTERED;ok=1;goto done;
fields_refused:
    inherited_entry_error(error,size,r->error[0]?r->error:"initial private field batch refused");
done:
    r->entity_prepare_busy=0;r->entity_inheritance_busy=0;r->advancing=0;r->entity_sequence_busy=0;*out=result;return ok;
}

int gsc_execute_prepared_entity_entry(GscRuntime*r,const GscPreparedEntityEntry*q,const GscPreparedEntityEntryHost*h,GscPreparedEntityEntryRun*out,char*error,size_t size){return prepare_entity_entry_phase(r,q,h,out,error,size,0);}
int gsc_construct_prepared_entity(GscRuntime*r,const GscPreparedEntityEntry*q,const GscPreparedEntityEntryHost*h,GscPreparedEntityEntryRun*out,char*error,size_t size){return prepare_entity_entry_phase(r,q,h,out,error,size,1);}
int gsc_execute_constructed_entity(GscRuntime*r,const GscEntityScriptEntry*entry,const GscEntityExecutionCertificate*cert,GscEntityScriptRun*out){
 if(!r||!entry||!cert||!out||!cert->serial||!cert->current||!state_ref_equal(&cert->owner,&entry->entity)||
    runtime_private_overlap(r,entry,sizeof(*entry))||runtime_private_overlap(r,cert,sizeof(*cert))||
    Cod1ModelStringOverlap(cert,sizeof(*cert),out,sizeof(*out))||Cod1ModelStringOverlap(entry,sizeof(*entry),out,sizeof(*out))||
    runtime_private_overlap(r,out,sizeof(*out))||r->entity_sequence_busy||r->advancing||r->executing_task||r->entity_entry_depth)return 0;
 return execute_entity_entry(r,entry,cert,out);
}
