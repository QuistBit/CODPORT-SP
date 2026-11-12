/* Explicit shared resource roots, separate from the ordinary startup entry. */
#include "gsc_vm_node_scripts.inc"
uint64_t gsc_runtime_program_identity(const GscRuntime *r) {
    return r&&r->program&&r->program->linked?r->program->function_program_token:0;
}
int gsc_runtime_resource_bootstrap_available(const GscRuntime *r) {
    return r&&!r->bsp_construction_only&&!r->r243_time_root_teardown&&!r->started&&!r->advancing&&!r->executing_task&&!r->task_count&&!r->entity_sequence_busy&&
        !r->entity_entry_depth&&!r->entity_prepare_busy&&!r->entity_certificate_busy&&!r->entity_inheritance_busy&&
        !r->entity_field_busy&&!r->entity_method_busy&&!r->error[0]&&r->fog&&
        r->state_enabled&&r->arrays_enabled&&r->results_enabled&&r->lifetime_enabled&&
        r->scalar_switch_enabled&&r->names_enabled&&r->resources_enabled&&r->precache_item_enabled&&
        (!r->program->entity_model_methods_selected||r->entity_methods_enabled)&&
        (!r->program->attachment_queries_selected||r->attachment_queries_enabled)&&
        (!r->program->delete_transactions_selected||r->delete_transactions_enabled)&&
        (!r->program->r243_scripted_start_selected||r->r243_scripted_start_enabled)&&
        (!r->program->r243_script_state_selected||r->r243_script_state_enabled)&&
        (!r->program->actor_critical_selected||r->actor_critical_enabled)&&
        (!r->program->actor_presence_selected||r->actor_presence_enabled)&&
        (!r->program->spawner_team_method_selected||r->spawner_team_enabled)&&
        (!r->program->animation_methods_selected||r->animation_methods_enabled)&&
        (!r->program->weapon_model_selected||r->weapon_model_enabled)&&
        (!r->program->start_origin_selected||r->start_origin_enabled)&&
        (!r->program->namespace_start_origin_selected||r->namespace_start_origin_enabled)&&
        (!r->program->animation_length_selected||r->animation_length_enabled)&&
        (!r->program->anim_data_selected||r->anim_data_enabled)&&
        (!r->program->animation_refs_selected||r->animation_refs_enabled)&&
        (!r->program->animation_namespace_selected||r->animation_namespace_enabled)&&
        (!r->program->float_arithmetic_selected||r->float_arithmetic_enabled)&&
        (!r->program->timed_arithmetic_selected||r->timed_arithmetic_enabled)&&
        (!r->program->frame_cadence_selected||r->frame_cadence_enabled)&&
        (!r->program->console_int32_selected||r->console_int32_enabled);
}
int gsc_runtime_script_function_available(const GscRuntime *r,const char *module,const char *function) {
    int m;size_t n;
    if(!r||!r->program->linked||r->error[0]||!module||!function)return 0;
    for(n=0;n<128&&module[n];++n){}if(!n||n==128)return 0;
    for(n=0;n<64&&function[n];++n){}if(!n||n==64)return 0;
    m=find_module(r->program,module);return m>=0&&find_func(r->program,m,function)>=0;
}
int gsc_runtime_r243_scripted_init_available(const GscRuntime*r){
 int module,fn;char why[384];
 /* Boolean-only query: there is no caller-owned writable output which could
  * alias any of the Program's separately allocated IR/parameter/string banks. */
 if(!r||!r->program||!r->program->linked||r->error[0]||
    !r->program->function_admission_selected||
    (module=find_module(r->program,"animscripts/scripted"))<0||
    (fn=find_func(r->program,module,"init"))<0||r->program->funcs[fn].argc!=6)return 0;
 return function_admission_runtime_current(r,fn,why,sizeof(why));
}
int gsc_start_entity_hooks(GscRuntime *r,const GscEntityHookBootstrapEntry *input,size_t count,
    const GscEntityHookBootstrapHost *host_input,GscEntityHookBootstrapResult *out,char *error,size_t size) {
    GscEntityHookBootstrapEntry entries[GSC_ENTITY_HOOK_BOOTSTRAP_MAX];GscEntityHookBootstrapResult result;
    GscEntityHookBootstrapHost host;size_t i;int fns[GSC_ENTITY_HOOK_BOOTSTRAP_MAX],module,budget,ok=0,accepted;
    Task *task=NULL;Value self;int time_root_required;
    if(!r||!input||!out||count>GSC_ENTITY_HOOK_BOOTSTRAP_MAX||!count||
       Cod1ModelStringOverlap(input,count*sizeof(*input),out,sizeof(*out))||
       Cod1ModelStringOverlap(input,count*sizeof(*input),error,size)||Cod1ModelStringOverlap(out,sizeof(*out),error,size)||
       Cod1ModelStringOverlap(host_input,host_input?sizeof(*host_input):0,out,sizeof(*out))||
       Cod1ModelStringOverlap(host_input,host_input?sizeof(*host_input):0,error,size)||
       !gsc_runtime_storage_separate(r,input,count*sizeof(*input))||
       !gsc_runtime_storage_separate(r,host_input,host_input?sizeof(*host_input):0)||
       !gsc_runtime_storage_separate(r,out,sizeof(*out))||!gsc_runtime_storage_separate(r,error,size))return 0;
    memcpy(entries,input,count*sizeof(*input));memset(&result,0,sizeof(result));*out=result;
    memset(&host,0,sizeof(host));if(host_input)host=*host_input;
    if(!gsc_runtime_resource_bootstrap_available(r))
        return inherited_entry_error(error,size,"entity hooks require a fresh selected runtime");
    time_root_required=r243_time_root_required(r);
    for(i=0;i<count;++i) {
        GscEntityExecutionCertificate *entity=&entries[i].entity_certificate;
        if((entries[i].kind!=GSC_HOOK_ROOT_GLOBAL&&entries[i].kind!=GSC_HOOK_ROOT_ENTITY)||
           !entries[i].map_certificate.receipt||!entries[i].map_certificate.current||
           (entries[i].kind==GSC_HOOK_ROOT_ENTITY&&(!entity->owner.epoch||!entity->owner.generation||!entity->serial||!entity->current))||
           !gsc_runtime_script_function_available(r,entries[i].module,entries[i].function))
            return inherited_entry_error(error,size,"entity hook function or certificate is unavailable");
        module=find_module(r->program,entries[i].module);fns[i]=find_func(r->program,module,entries[i].function);
    }
    r->advancing=1;r->entity_sequence_busy=1;
    for(i=0;i<count;++i) {
        r->entity_certificate_busy=1;
        accepted=entries[i].map_certificate.current(entries[i].map_certificate.receipt);
        if(accepted==1&&entries[i].kind==GSC_HOOK_ROOT_ENTITY) {
            GscEntityExecutionCertificate borrowed=entries[i].entity_certificate;
            accepted=borrowed.current(&borrowed.owner,borrowed.serial);
        }
        r->entity_certificate_busy=0;
        if(accepted!=1||r->error[0]){inherited_entry_error(error,size,r->error[0]?r->error:"entity hook authority changed");goto done;}
    }
    if(time_root_required!=r243_time_root_required(r)){rfail(r,0,"time-root profile changed during bootstrap preflight");goto failed;}
    if(!r243_time_root_construct(r))goto failed;
    if(!r->next_serial)r->next_serial=1;
    r->started=1;r->bootstrap_started=1;r->bootstrap_returned=0;
    for(i=0;i<count;++i) {
        memset(&self,0,sizeof(self));self.kind=entries[i].kind==GSC_HOOK_ROOT_ENTITY?V_ENTITY:V_LEVEL;
        if(self.kind==V_ENTITY)self.entity=entries[i].entity_certificate.owner;
        budget=MAX_STEPS;r->execution_budget=&budget;task=task_acquire(r,0);if(!task)goto failed;
        task->host_entry_pins=1;task->status=TASK_READY;task->serial=r->next_serial++;
        task->map_certificate=entries[i].map_certificate;
        if(self.kind==V_ENTITY)task->certificate=entries[i].entity_certificate;
        task->initial_self=self;result.attempted=(uint32_t)i+1;result.runs[i].task_serial=task->serial;
        if(!entity_certificate_current(r,task)) {
            if(r->error[0])goto failed;
            result.runs[i].status=GSC_ENTITY_ENTRY_CANCELLED;goto cancelled;
        }
        if(!push_frame(r,task,fns[i],NULL,0,&self,0))goto failed;
        r->resources_entry_serial=task->serial;r->resources_closed=0;r->precache_item_initial_window=1;
        r->executing_task=task;accepted=run_task_results(r,task,&budget);r->executing_task=NULL;
        r->resources_closed=1;r->precache_item_initial_window=0;
        if(r->error[0]||(!accepted&&!(task->status==TASK_DEAD&&task->context_retired)))goto failed;
        if(task->context_retired)result.runs[i].status=GSC_ENTITY_ENTRY_CANCELLED;
        else if(task->status==TASK_DEAD)result.runs[i].status=GSC_ENTITY_ENTRY_RETURNED;
        else if(task->status==TASK_SLEEP)result.runs[i].status=GSC_ENTITY_ENTRY_SLEEPING;
        else if(task->status==TASK_EVENT)result.runs[i].status=GSC_ENTITY_ENTRY_WAITING;
        else{rfail(r,0,"entity hook did not reach a continuation boundary");goto failed;}
        if(task->status==TASK_DEAD)task_results_clear(task);
        task->host_entry_pins=0;task=NULL;
        if(result.runs[i].status==GSC_ENTITY_ENTRY_CANCELLED)goto cancelled;
        if(host.after_initial) {
            r->entity_prepare_busy=1;accepted=host.after_initial(host.user,i+1);r->entity_prepare_busy=0;
            if(accepted!=1||r->error[0]){inherited_entry_error(error,size,r->error[0]?r->error:"entity hook phase membership changed");goto done;}
        }
    }
    ok=1;goto done;
cancelled:
    inherited_entry_error(error,size,"entity hook authority retired");goto done;
failed:
    inherited_entry_error(error,size,r->error[0]?r->error:"entity hook execution refused");
done:
    if(task){if(task->status==TASK_DEAD||r->error[0])task_results_clear(task);task->host_entry_pins=0;}
    r->executing_task=NULL;r->execution_budget=NULL;r->resources_closed=1;r->precache_item_initial_window=0;
    r->entity_sequence_busy=0;r->advancing=0;
    if(ok&&!gsc_active_threads(r)){r->bootstrap_returned=1;for(i=0;i<count;i++)if(result.runs[i].status!=GSC_ENTITY_ENTRY_RETURNED)r->bootstrap_returned=0;}
    *out=result;return ok;
}
int gsc_start_resource_hooks(GscRuntime *r,const GscResourceBootstrapEntry *input,size_t count,
    GscResourceBootstrapResult *out,char *error,size_t size) {
    GscEntityHookBootstrapEntry entries[GSC_RESOURCE_BOOTSTRAP_MAX];GscEntityHookBootstrapResult result;
    size_t i;int ok;
    if(!r||!input||!out||!count||count>GSC_RESOURCE_BOOTSTRAP_MAX||
       Cod1ModelStringOverlap(input,count*sizeof(*input),out,sizeof(*out))||
       Cod1ModelStringOverlap(input,count*sizeof(*input),error,size)||Cod1ModelStringOverlap(out,sizeof(*out),error,size)||
       !gsc_runtime_storage_separate(r,input,count*sizeof(*input))||!gsc_runtime_storage_separate(r,out,sizeof(*out))||
       !gsc_runtime_storage_separate(r,error,size))return 0;
    memset(entries,0,sizeof(entries));
    for(i=0;i<count;++i){entries[i].kind=GSC_HOOK_ROOT_GLOBAL;memcpy(entries[i].module,input[i].module,sizeof(entries[i].module));
        memcpy(entries[i].function,input[i].function,sizeof(entries[i].function));entries[i].map_certificate=input[i].certificate;}
    memset(&result,0,sizeof(result));ok=gsc_start_entity_hooks(r,entries,count,NULL,&result,error,size);
    memset(out,0,sizeof(*out));out->attempted=result.attempted;
    for(i=0;i<result.attempted;++i)out->runs[i]=result.runs[i];
    return ok;
}
