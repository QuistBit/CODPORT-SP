/* Native actor state3 may start exactly the linked WeaponDef script entry.
 * This service owns no timer, animation event or synthetic completion. */
int gsc_session_execute_r232_turret_entry(GscSession *s,uint64_t id,
    const GscEntityScriptEntry *input,GscEntityScriptRun *out,char *error,size_t cap) {
    GscEntityScriptEntry copy,original;GscEntityScriptRun result;GscLinkFunctionInfo fn;
    GscRuntime *runtime;uint64_t instance;int i,found=0,now,ok;
    if((cap&&!error)||!input||!out||session_animation_namespace_private(s,input,sizeof(*input))||
       session_animation_namespace_private(s,out,sizeof(*out))||session_animation_namespace_private(s,error,cap)||
       session_animation_namespace_overlap(input,sizeof(*input),out,sizeof(*out))||
       session_animation_namespace_overlap(error,cap,input,sizeof(*input))||
       session_animation_namespace_overlap(error,cap,out,sizeof(*out)))return 0;
    copy=original=*input;
    if(!session_enter(s,error,cap))return 0;
    if(!session_id_matches(s,id)||!s->turret_animations_enabled||!s->runtime||!s->program||
       s->view.state!=GSC_SESSION_RUNNING||!memchr(copy.module,0,sizeof(copy.module))||
       !memchr(copy.function,0,sizeof(copy.function))||strcmp(copy.module,"animscripts/mg42/stand")||
       strcmp(copy.function,"main")||!copy.entity.epoch||!copy.entity.generation)
        return session_leave_rejected(s,"turret entry requires current RUNNING selected Session and exact stand::main",error,cap);
    for(i=0;i<gsc_program_link_function_count(s->program);i++)
        if(gsc_program_link_function_get(s->program,i,&fn)&&!strcmp(fn.module,copy.module)&&!strcmp(fn.name,copy.function)) {
            if(fn.argc||found)return session_leave_rejected(s,"turret entry requires one linked zero-argument function",error,cap);found=1;
        }
    if(!found)return session_leave_rejected(s,"turret entry function not linked",error,cap);
    runtime=s->runtime;instance=gsc_runtime_instance_identity(runtime);now=s->view.logical_ms;
    if(!session_animation_resources_current(s)||s->runtime!=runtime||s->view.session_id!=id||
       s->view.logical_ms!=now||!instance||gsc_runtime_instance_identity(runtime)!=instance||
       memcmp(input,&original,sizeof(original)))
        return session_failed(s,"turret entry resource/caller lease expired",error,cap);
    memset(&result,0,sizeof(result));ok=gsc_execute_entity_entry(runtime,&copy,&result);
    if(s->runtime!=runtime||s->view.session_id!=id||s->view.logical_ms!=now||
       gsc_runtime_instance_identity(runtime)!=instance||memcmp(&copy,&original,sizeof(copy))||
       memcmp(input,&original,sizeof(original)))
        return session_failed(s,"turret entry caller lease changed",error,cap);
    if(!ok&&!s->abort_requested&&!gsc_runtime_error(runtime)[0])
        return session_leave_rejected(s,"turret entry refused stale receiver or unavailable continuation boundary",error,cap);
    if(!session_after_vm(s,ok,error,cap))return 0;
    if(!result.task_serial||memcmp(&result.entity,&original.entity,sizeof(result.entity))||
       result.status<GSC_ENTITY_ENTRY_RETURNED||result.status>GSC_ENTITY_ENTRY_CANCELLED)
        return session_reject(error,cap,"turret entry returned invalid task report");
    *out=result;return 1;
}
