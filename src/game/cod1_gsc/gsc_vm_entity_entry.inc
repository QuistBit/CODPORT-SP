/* R166 immediate zero-argument host script entry. Native 4693f0 calls 4690b0
 * and the interpreter before returning; postinit 2004d7e0 then continues with
 * orientation/init/state. This bounded service does not complete that actor
 * chain, inherit objects or fabricate native handles/readiness. */
/* R243 internal entry takes VM-owned Values, never a public host/event value
 * encoding. In particular, animation namespace handles do not leave this VM.
 * The native 4693f0/4690b0 entry owns another script task and executes it now;
 * dropping the returned native thread reference does not cancel a waiter.
 * Actor state9 selection and the phase6 previous-time scope are separate work.
 */
static int execute_entity_values_owned_map_r243(GscRuntime *r,const GscEntityScriptEntry *input,
    const GscEntityExecutionCertificate *certificate,const GscMapExecutionCertificate *map_override,
    const Value *args,int argc,
    GscEntityScriptRun *out,Task **owned_task,PlacedInitReturnScope *placed) {
    GscEntityScriptEntry entry;GscEntityScriptRun result;Task *caller,*task=NULL;
    Value copied_args[MAX_PARAMS];
    GscEntityExecutionCertificate owned_certificate;
    GscMapExecutionCertificate owned_map;
    Value self;int module,fn,local_budget=MAX_STEPS,outer,ok=0;
    if(!r||r->bsp_construction_only||!input||!out||!r->started||!r->program->linked||r->error[0]||
       !r->state_enabled||!r->lifetime_enabled||!r->results_enabled||
       !r->scalar_switch_enabled||!r->names_enabled||argc<0||argc>MAX_PARAMS||(!args&&argc)||
       Cod1ModelStringOverlap(input,sizeof(*input),out,sizeof(*out))||
       (argc&&Cod1ModelStringOverlap(args,(size_t)argc*sizeof(*args),out,sizeof(*out)))||
       runtime_private_overlap(r,out,sizeof(*out)))return 0;
    if(owned_task)*owned_task=NULL;
    entry=*input;
    /* Validation may call the resource/entity host, and task acquisition can
     * reuse a dead task. Preserve the actual values before either boundary. */
    if(argc)memcpy(copied_args,args,(size_t)argc*sizeof(*args));
    if(certificate)owned_certificate=*certificate;
    if(map_override)owned_map=*map_override;
    if(!entry.entity.epoch||!entry.entity.generation||
       !memchr(entry.module,0,sizeof(entry.module))||!entry.module[0]||
       !memchr(entry.function,0,sizeof(entry.function))||!entry.function[0])return 0;
    module=find_module(r->program,entry.module);
    if(module<0||(fn=find_func(r->program,module,entry.function))<0)return 0;
    if(r->entity_inheritance_busy)return rfail(r,0,"entity script entry during field inheritance");
    if(r->entity_field_busy)return rfail(r,0,"entity script entry during native field callback");
    if(r->entity_method_busy)return rfail(r,0,"entity script entry during native method callback");
    if(r->entity_prepare_busy||r->entity_certificate_busy||r->animation_length_busy||r->animation_tick_serial)return rfail(r,0,"entity script entry during guarded host callback");
    if(r->entity_entry_depth>=32)return rfail(r,0,"entity script entry depth exceeded");
    outer=!r->advancing;caller=r->executing_task;
    /* The separate cadence lane does not infer a new public-entry queue key
     * between external frame publication and its pending deadline drain. */
    if(outer&&r->frame_cadence_enabled&&r->now_ms!=r->cadence_drain_ms)return 0;
    if(!outer&&(!caller||!r->execution_budget))return rfail(r,0,"entity script entry has no active execution context");
    if(outer){r->advancing=1;r->execution_budget=r->r243_actor_script_scope?&r->r237_frame_budget:&local_budget;}
    ++r->entity_entry_depth;
    if(caller&&(!refresh_state_context(r,caller)||caller->status==TASK_DEAD))goto done;
    if(state_entity_live(r,&entry.entity)!=1)goto done;
    if(caller&&(!refresh_state_context(r,caller)||caller->status==TASK_DEAD))goto done;
    task=task_acquire(r,0);if(!task)goto done;
    task->host_entry_pins=1;task->status=TASK_READY;task->serial=r->next_serial++;
    if(certificate)task->certificate=owned_certificate;
    else if(caller)task->certificate=caller->certificate;
    if(map_override)task->map_certificate=owned_map;
    else if(caller)task->map_certificate=caller->map_certificate;
    memset(&self,0,sizeof(self));self.kind=V_ENTITY;self.entity=entry.entity;
    task->initial_self=self;
    r->executing_task=task;
    if(!entity_certificate_current(r,task)) {
        if(r->error[0])goto done;
        memset(&result,0,sizeof(result));result.entity=entry.entity;
        result.task_serial=task->serial;result.status=GSC_ENTITY_ENTRY_CANCELLED;
        *out=result;ok=1;goto done;
    }
    r->executing_task=task;
    if(!push_frame(r,task,fn,copied_args,argc,&self,0)) {
        if(!r->error[0]&&task->context_retired){ok=1;goto status;}
        goto done;
    }
    if(r->factory_init_entry==input){
        if(!certificate||!r->actor_spawn_init_used||!r->entity_sequence_busy||r->factory_init_serial||
           task->depth!=1||!state_ref_equal(&entry.entity,&r->actor_spawn_issued.owner)||
           memcmp(&task->certificate,&r->actor_spawn_issued,sizeof(task->certificate)))
            {rfail(r,0,"dynamic INIT root lost its issued actor");goto done;}
        r->factory_init_serial=task->serial;
    }
    if(placed&&!placed_return_bind(r,placed,task,fn,&entry))goto done;
    ok=r->results_enabled?run_task_results(r,task,r->execution_budget):
                         run_task(r,task,r->execution_budget);
    r->executing_task=caller;
    if(r->error[0]){ok=0;goto done;}
    if(!ok&&!(task->status==TASK_DEAD&&task->context_retired))goto done;
status:
    memset(&result,0,sizeof(result));result.entity=entry.entity;result.task_serial=task->serial;
    if(task->context_retired)result.status=GSC_ENTITY_ENTRY_CANCELLED;
    else if(task->status==TASK_DEAD)result.status=GSC_ENTITY_ENTRY_RETURNED;
    else if(task->status==TASK_SLEEP)result.status=GSC_ENTITY_ENTRY_SLEEPING;
    else if(task->status==TASK_EVENT)result.status=GSC_ENTITY_ENTRY_WAITING;
    else{ok=rfail(r,0,"entity script entry did not stop at a continuation boundary");goto done;}
    *out=result;ok=1;
done:
    r->executing_task=caller;
    if(task){
        if(ok&&owned_task){
            if(task->retained_owner_refs==UINT_MAX)ok=rfail(r,0,"MAIN owner reference exhausted");
            else{++task->retained_owner_refs;*owned_task=task;}
        }
        if(task->status==TASK_DEAD||!ok)task_results_clear(task);task->host_entry_pins=0;
    }
    --r->entity_entry_depth;
    if(outer){r->execution_budget=NULL;r->advancing=0;endon_collect_retired(r);}
    return ok;
}
static int execute_entity_values_map_r243(GscRuntime*r,const GscEntityScriptEntry*input,
 const GscEntityExecutionCertificate*certificate,const GscMapExecutionCertificate*map,
 const Value*args,int argc,GscEntityScriptRun*out){
 return execute_entity_values_owned_map_r243(r,input,certificate,map,args,argc,out,NULL,NULL);
}
/* Preserve the existing private/public zeroarg/typed ABI. Only the owned fixed
 * native-prefix dispatch supplies its independently captured real map lease. */
static int execute_entity_values_r243(GscRuntime*r,const GscEntityScriptEntry*input,
 const GscEntityExecutionCertificate*certificate,const Value*args,int argc,
 GscEntityScriptRun*out){
 return execute_entity_values_map_r243(r,input,certificate,NULL,args,argc,out);
}
static int execute_entity_entry(GscRuntime *r,const GscEntityScriptEntry *input,
    const GscEntityExecutionCertificate *certificate,GscEntityScriptRun *out) {
    if(r&&r->r243_actor_script_scope)return rfail(r,0,"public entity entry during owned actor-script scope");
    return execute_entity_values_r243(r,input,certificate,NULL,0,out);
}
int gsc_execute_entity_entry(GscRuntime *r,const GscEntityScriptEntry *input,GscEntityScriptRun *out) {
    return execute_entity_entry(r,input,NULL,out);
}

static int inherited_entry_error(char *error,size_t size,const char *message) {
    if(error&&size)str_copy(error,size,message);
    return 0;
}
int gsc_runtime_storage_separate(const GscRuntime *r,const void *storage,size_t size) {
    return r&&!placed_cohort_private(r,storage,size)&&!(r->placed_return&&Cod1ModelStringOverlap(storage,size,r->placed_return,sizeof(*r->placed_return)))&&!r243_critical_vm_private(storage,size)&&!r243_command_vm_private(storage,size)&&!move_delta_private(storage,size)&&!r243_actor_float_vm_private(storage,size)&&!r243_stance_vm_private(storage,size)&&!r243_animtime_vm_private(storage,size)&&!r243s_init_private(r,storage,size)&&!r243_time_root_private(r,storage,size)&&!runtime_private_overlap(r,storage,size)&&
        !(r->r243_controller_capture&&Cod1ModelStringOverlap(storage,size,r->r243_controller_capture,r->r243_controller_capture_bytes));
}
static int execute_inherited_entity_entry(GscRuntime *r,const GscInheritedEntityEntry *input,
    const GscInheritedEntityEntryHost *host_input,GscInheritedEntityEntryRun *out,
    char *error,size_t size,
    int (*before_copy)(void *,const GscEntityFieldInheritance *,char *,size_t),int *projected,int spawning) {
    GscInheritedEntityEntry request;GscInheritedEntityEntryHost host;
    GscInheritedEntityEntryRun result;GscEntityExecutionCertificate certificate,produced;
    GscEntityFieldInheritance borrowed;
    char why[256];int module,ok=0,prepared,was_advancing;
    if(!r||!input||!host_input||!out||
       Cod1ModelStringOverlap(input,sizeof(*input),out,sizeof(*out))||
       Cod1ModelStringOverlap(host_input,sizeof(*host_input),out,sizeof(*out))||
       runtime_private_overlap(r,out,sizeof(*out))||
       runtime_private_overlap(r,error,size)||
       Cod1ModelStringOverlap(error,size,input,sizeof(*input))||
       Cod1ModelStringOverlap(error,size,host_input,sizeof(*host_input))||
       Cod1ModelStringOverlap(error,size,out,sizeof(*out)))return 0;
    request=*input;host=*host_input;
    if(!host.validate_copy||!host.prepare||
       !state_ref_equal(&request.inheritance.destination,&request.entry.entity)||
       !request.inheritance.source.epoch||!request.inheritance.source.generation||
       !request.entry.entity.epoch||!request.entry.entity.generation||
       state_ref_equal(&request.inheritance.source,&request.entry.entity)||
       !memchr(request.entry.module,0,sizeof(request.entry.module))||!request.entry.module[0]||
       !memchr(request.entry.function,0,sizeof(request.entry.function))||!request.entry.function[0])
        return inherited_entry_error(error,size,"invalid inherited entry request");
    memset(&result,0,sizeof(result));*out=result;
    if(r->entity_sequence_busy||r->entity_prepare_busy||r->entity_certificate_busy||
       r->entity_inheritance_busy||r->entity_field_busy||r->entity_method_busy||
       (!spawning&&(r->advancing||r->executing_task||r->entity_entry_depth||
        (r->frame_cadence_enabled&&r->now_ms!=r->cadence_drain_ms))))
        return inherited_entry_error(error,size,"inherited entry requires a quiescent runtime");
    if(spawning&&(!r->advancing||!r->execution_budget||!r->executing_task||
       r->executing_task!=r->actor_spawn_preparing||
       r->executing_task->serial!=r->actor_spawn_preparing_serial||
       r->actor_spawn_preparing_used||
       !state_ref_equal(&request.inheritance.source,&r->actor_spawn_preparing_source)))
        return inherited_entry_error(error,size,"projected spawn entry requires its active factory caller");
    if(!r->started||!r->program->linked||r->error[0]||!r->state_enabled||
       !r->arrays_enabled||!r->structs_enabled||!r->lifetime_enabled||!r->results_enabled||
       !r->scalar_switch_enabled||!r->names_enabled||
       (module=find_module(r->program,request.entry.module))<0||
       find_func(r->program,module,request.entry.function)<0)
        return inherited_entry_error(error,size,"inherited entry profile or function is unavailable");
    was_advancing=r->advancing;
    if(spawning)r->actor_spawn_preparing_used=1;
    r->entity_sequence_busy=1;
    if(before_copy) {
        memset(why,0,sizeof(why));borrowed=request.inheritance;
        r->advancing=1;r->entity_prepare_busy=1;
        prepared=before_copy(host.user,&borrowed,why,sizeof(why));
        r->entity_prepare_busy=0;r->advancing=was_advancing;why[sizeof(why)-1]=0;
        if(prepared!=1||r->error[0]) {
            inherited_entry_error(error,size,r->error[0]?r->error:why[0]?why:"static projection refused");goto done;
        }
        *projected=1;
    }
    if(!inherit_entity_fields(r,&request.inheritance,&result.copied,host.validate_copy,host.user)) {
        inherited_entry_error(error,size,r->error[0]?r->error:"inherited field copy refused");goto done;
    }
    result.stage=GSC_INHERITED_ENTRY_FIELDS;
    memset(&produced,0,sizeof(produced));memset(why,0,sizeof(why));borrowed=request.inheritance;
    r->advancing=1;r->entity_prepare_busy=1;
    prepared=host.prepare(host.user,&borrowed,&produced,why,sizeof(why));
    certificate=produced;
    r->entity_prepare_busy=0;r->advancing=was_advancing;why[sizeof(why)-1]=0;
    if(prepared!=1||r->error[0]) {
        inherited_entry_error(error,size,r->error[0]?r->error:why[0]?why:"inherited native preparation refused");goto done;
    }
    result.stage=GSC_INHERITED_ENTRY_PREPARED;
    if(!certificate.serial||!certificate.current||
       !state_ref_equal(&certificate.owner,&request.entry.entity)) {
        inherited_entry_error(error,size,"invalid inherited entry certificate");goto done;
    }
    if(!execute_entity_entry(r,&request.entry,&certificate,&result.entry)) {
        inherited_entry_error(error,size,r->error[0]?r->error:"inherited script entry refused");goto done;
    }
    result.stage=GSC_INHERITED_ENTRY_ENTERED;ok=1;
    if(spawning&&result.entry.status!=GSC_ENTITY_ENTRY_CANCELLED)r->actor_spawn_issued=certificate;
done:
    r->entity_sequence_busy=0;*out=result;return ok;
}
int gsc_execute_inherited_entity_entry(GscRuntime *r,const GscInheritedEntityEntry *input,
    const GscInheritedEntityEntryHost *host,GscInheritedEntityEntryRun *out,char *error,size_t size) {
    return execute_inherited_entity_entry(r,input,host,out,error,size,NULL,NULL,0);
}
static int execute_projected_entry_selected(GscRuntime *r,const GscInheritedEntityEntry *input,
    const GscProjectedEntityEntryHost *host_input,GscProjectedEntityEntryRun *out,
    char *error,size_t size,int spawning) {
    GscProjectedEntityEntryHost host;GscProjectedEntityEntryRun result;int ok;
    if(!r||!input||!host_input||!out||
       Cod1ModelStringOverlap(input,sizeof(*input),out,sizeof(*out))||
       Cod1ModelStringOverlap(host_input,sizeof(*host_input),out,sizeof(*out))||
       runtime_private_overlap(r,out,sizeof(*out))||
       runtime_private_overlap(r,error,size)||
       Cod1ModelStringOverlap(error,size,input,sizeof(*input))||
       Cod1ModelStringOverlap(error,size,host_input,sizeof(*host_input))||
       Cod1ModelStringOverlap(error,size,out,sizeof(*out)))return 0;
    host=*host_input;
    if(!host.before_copy)return inherited_entry_error(error,size,"static projection host is unavailable");
    memset(&result,0,sizeof(result));
    ok=execute_inherited_entity_entry(r,input,&host.inherited,&result.inherited,error,size,
        host.before_copy,&result.projected,spawning);
    *out=result;return ok;
}
int gsc_execute_projected_entity_entry(GscRuntime*r,const GscInheritedEntityEntry*q,
 const GscProjectedEntityEntryHost*h,GscProjectedEntityEntryRun*out,char*e,size_t n){
 return execute_projected_entry_selected(r,q,h,out,e,n,0);
}
int gsc_execute_projected_spawn_entry(GscRuntime*r,const GscInheritedEntityEntry*q,
 const GscProjectedEntityEntryHost*h,GscProjectedEntityEntryRun*out,char*e,size_t n){
 if(!r||!q||!h||!out||!gsc_runtime_storage_separate(r,q,sizeof(*q))||
    !gsc_runtime_storage_separate(r,h,sizeof(*h))||!gsc_runtime_storage_separate(r,out,sizeof(*out))||
    !gsc_runtime_storage_separate(r,e,n))return 0;
 return execute_projected_entry_selected(r,q,h,out,e,n,1);
}
#include "gsc_vm_prepared_entry.inc"
#include "gsc_vm_placed_init.inc"
#include "gsc_vm_placed_cohort.inc"
#include "gsc_vm_bsp_construction.inc"
#include "gsc_vm_resource_bootstrap.inc"

/* Pure scope query for a factory before it allocates a destination. It neither
 * invokes host callbacks nor consumes the single projected-entry attempt. */
uint64_t gsc_projected_spawn_entry_program(const GscRuntime *r,const GscEntityRef *source) {
    if(!r||!source||!gsc_runtime_storage_separate(r,source,sizeof(*source))||
       !r->started||r->error[0]||!r->advancing||!r->execution_budget||
       !r->executing_task||r->executing_task!=r->actor_spawn_preparing||
       r->executing_task->serial!=r->actor_spawn_preparing_serial||
       r->actor_spawn_preparing_used||r->entity_sequence_busy||r->entity_prepare_busy||
       r->entity_certificate_busy||r->entity_inheritance_busy||r->entity_field_busy||
       r->entity_method_busy||!state_ref_equal(source,&r->actor_spawn_preparing_source))return 0;
    return gsc_runtime_program_identity(r);
}

/* Factory-local issued identity, never a caller-supplied execution certificate.
 * Pure query: GAME must also validate the live lease/source before committing.
 * The record is saved/restored with prepare and unavailable in child callbacks,
 * finish, or after Session unwind. It does not authorize script execution. */
uint64_t gsc_spawn_issued_owner_program(const GscRuntime *r,const GscEntityRef *source,
    const GscEntityRef *owner,uint64_t serial) {
    if(!r||!source||!owner||!serial||
       !gsc_runtime_storage_separate(r,source,sizeof(*source))||
       !gsc_runtime_storage_separate(r,owner,sizeof(*owner))||
       !r->started||r->error[0]||!r->advancing||!r->execution_budget||
       !r->executing_task||r->executing_task!=r->actor_spawn_preparing||
       r->executing_task->serial!=r->actor_spawn_preparing_serial||
       r->executing_task->status==TASK_DEAD||r->executing_task->context_retired||
       !r->actor_spawn_preparing_used||r->entity_sequence_busy||r->entity_prepare_busy||
       r->entity_certificate_busy||r->entity_inheritance_busy||r->entity_field_busy||
       r->entity_method_busy||!state_ref_equal(source,&r->actor_spawn_preparing_source)||
       serial!=r->actor_spawn_issued.serial||!r->actor_spawn_issued.current||
       !state_ref_equal(owner,&r->actor_spawn_issued.owner))return 0;
    return gsc_runtime_program_identity(r);
}

/* Original4D7E0 second script entry. Only its own issued certificate is used;
 * caller cannot supply a certificate, module or arbitrary function handle. */
int gsc_execute_spawn_init(GscRuntime*r,const GscEntityRef*source,
 const GscEntityRef*owner,uint64_t serial,GscEntityScriptRun*out,char*error,size_t cap){
 GscEntityRef src,dst;GscEntityExecutionCertificate certificate;GscEntityScriptEntry entry;
 GscEntityScriptRun result;int module,ok,alive;
 if(!r||!source||!owner||!out||
    !gsc_runtime_storage_separate(r,source,sizeof(*source))||
    !gsc_runtime_storage_separate(r,owner,sizeof(*owner))||
    !gsc_runtime_storage_separate(r,out,sizeof(*out))||!gsc_runtime_storage_separate(r,error,cap)||
    Cod1ModelStringOverlap(source,sizeof(*source),out,sizeof(*out))||
    Cod1ModelStringOverlap(owner,sizeof(*owner),out,sizeof(*out))||
    Cod1ModelStringOverlap(error,cap,source,sizeof(*source))||
    Cod1ModelStringOverlap(error,cap,owner,sizeof(*owner))||
    Cod1ModelStringOverlap(error,cap,out,sizeof(*out)))return 0;
 src=*source;dst=*owner;memset(&result,0,sizeof(result));*out=result;
 if(!gsc_spawn_issued_owner_program(r,&src,&dst,serial)||r->actor_spawn_init_used||
    (module=find_module(r->program,"animscripts/init"))<0||find_func(r->program,module,"main")<0)
    return inherited_entry_error(error,cap,"spawn INIT requires unused issued owner and linked original root");
 certificate=r->actor_spawn_issued;r->actor_spawn_init_used=1;r->entity_sequence_busy=1;
 memset(&entry,0,sizeof(entry));entry.entity=dst;strcpy(entry.module,"animscripts/init");strcpy(entry.function,"main");
 /* Certificate fences surround only the foreign validation callback. The
  * generic entity_validate/state_entity_live helpers include these fences and
  * therefore cannot themselves be called under entity_prepare_busy. */
 alive=0;
 if(entity_certificate_current(r,r->executing_task)){
  GscEntityRef borrowed=src;
  r->entity_prepare_busy=1;alive=r->entities.validate(r->entities.user,&borrowed);r->entity_prepare_busy=0;
  if(!state_ref_equal(&borrowed,&src))rfail(r,0,"spawn INIT source validator mutated its borrowed identity");
  if(!entity_certificate_current(r,r->executing_task))alive=0;
 }
 if(r->factory_init_entry || r->factory_init_serial){r->entity_sequence_busy=0;return rfail(r,0,"nested dynamic INIT");}
 r->factory_init_entry=&entry;
 ok=alive==1&&!r->error[0]&&execute_entity_entry(r,&entry,&certificate,&result);
 r->factory_init_entry=NULL;r->factory_init_serial=0;
 r->entity_sequence_busy=0;*out=result;
 if(!ok||r->error[0]||result.status==GSC_ENTITY_ENTRY_CANCELLED)
    return inherited_entry_error(error,cap,r->error[0]?r->error:"spawn INIT owner retired or entry refused");
 return 1;
}
