/* Private VM half of original GAME200570D0 phase6. Original46ACA0(previous)
 * changes the queue base without dispatch on a backward move;4693F0 runs the
 * new entity task immediately; native2001EEB0 may drain current repeatedly;
 *46ACA0(current) drains [previous,current), leaving current pending.
 *
 * No public clock-rewind API, source/native method admission, state9/FSM,
 * previous-thread cleanup or GAME actor binding is provided by this helper.
 * The future actor controller must supply its genuine execution certificate
 * and perform its real state/cleanup/selection before invoking this VM half.
 */
typedef struct {
    GscRuntime *runtime;uint64_t frame,instance,time_root;
    int host_ms,previous_ms;GscEntityExecutionCertificate certificate;
} R243ActorScriptScope;
static int r243_actor_script_scope_shape(const R243ActorScriptScope *s){
    const GscRuntime *r=s->runtime;
    return r&&r237_owner_current(r,s->time_root)&&r->r243_actor_script_scope&&r->advancing&&!r->error[0]&&
       !r->executing_task&&
       r->array_token==s->instance&&r->r237_frame_generation==s->frame&&!r->r237_frame_open&&
       r->now_ms==s->host_ms&&r->r243_previous_host_ms==s->previous_ms&&
       r237_bucket(r)>=s->previous_ms&&r237_bucket(r)<=s->host_ms&&
       !r->entity_certificate_busy&&!r->entity_prepare_busy;
}
static int r243_actor_script_scope_current(R243ActorScriptScope *s){
    GscRuntime *r=s->runtime;R243ActorScriptScope before;GscEntityExecutionCertificate copy;int current;
    if(!r243_actor_script_scope_shape(s))return 0;
    before=*s;
    /* The actor owner may validate allocations/resources; neither a successful
     * result nor a retained actor name permits changing the execution scope. */
    copy=s->certificate;r->entity_certificate_busy=1;
    current=copy.current(&copy.owner,copy.serial);r->entity_certificate_busy=0;
    if(r->error[0])return 0;
    if(memcmp(s,&before,sizeof(before))||!r243_actor_script_scope_shape(s))return rfail(r,0,"actor-script scope changed during certificate callback");
    if(memcmp(&copy,&s->certificate,sizeof(copy)))return rfail(r,0,"actor-script scope certificate was mutated");
    if(current!=0&&current!=1)return rfail(r,0,"invalid actor-script scope certificate result");
    return current;
}
static int r243_actor_script_run_current(void *context){
    R243ActorScriptScope *s=context;GscRuntime *r=s->runtime;
    if(r->executing_task||r->entity_method_busy||r->entity_field_busy||
       r->entity_prepare_busy||r->entity_certificate_busy||r->entity_inheritance_busy)
        return rfail(r,0,"reentrant actor-script current-bucket call");
    if(!r243_actor_script_scope_current(s))return 0;
    if(!r237_drain_bucket(r,&r->r237_frame_budget))return 0;
    return r243_actor_script_scope_current(s);
}
/* Trusted VM-only continuation, never a configured host callback/service. */
static int r243_actor_script_phase6_owned_hook(GscRuntime *r,uint64_t generation,
    const GscEntityScriptEntry *input,const GscEntityExecutionCertificate *certificate,const GscMapExecutionCertificate*map,
    int(*actor_step)(void *,int(*)(void *),void *),void *actor_context,GscEntityScriptRun *out,Task**owned_task,
    int(*entry_retained)(void*,Task*,const GscEntityScriptRun*),void*retained_context){
    R243ActorScriptScope scope,sealed_scope;GscEntityScriptEntry entry;GscEntityScriptRun run;
    int ok=0,entered=0;
    if(!r||!input||!certificate||!out||r237_busy(r)||!gsc_runtime_r237_frame_buckets_selected(r)||
       !r->started||!generation||generation!=r->r237_frame_generation||r->r237_frame_open||
       r237_bucket(r)!=r->now_ms||r->r243_previous_host_ms<0||
       r->r243_previous_host_ms>=r->now_ms||!certificate->serial||!certificate->current||
       !state_ref_equal(&certificate->owner,&input->entity)||
       Cod1ModelStringOverlap(input,sizeof(*input),out,sizeof(*out))||
       Cod1ModelStringOverlap(certificate,sizeof(*certificate),out,sizeof(*out))||
       !gsc_runtime_storage_separate(r,out,sizeof(*out)))return 0;
    entry=*input;memset(&scope,0,sizeof(scope));scope.runtime=r;scope.frame=generation;
    scope.instance=r->array_token;scope.time_root=r237_owner_stamp(r);scope.host_ms=r->now_ms;scope.previous_ms=r->r243_previous_host_ms;scope.certificate=*certificate;
    sealed_scope=scope;
    r->r243_actor_script_scope=1;r->advancing=1;r->execution_budget=&r->r237_frame_budget;
    if(!r243_actor_script_scope_current(&scope))goto done;
    if(!r237_bucket_set(r,scope.previous_ms,scope.time_root))goto done;
    /* The private entry creates/runs another task; this is not a nested public
     * callback entry and does not execute any existing queued task yet. */
    r->execution_budget=NULL;r->advancing=0;
    entered=execute_entity_values_owned_map_r243(r,&entry,&scope.certificate,map,NULL,0,&run,owned_task,NULL);
    r->advancing=1;r->execution_budget=&r->r237_frame_budget;
    if(!entered)goto done;
    /* Original571E1 stores the returned retained handle before1EEB0.
     * Move the already-acquired reference; do not pin, run or notify here. */
    if(entry_retained&&(!owned_task||!*owned_task||
       entry_retained(retained_context,*owned_task,&run)!=1)){
        if(!r->error[0])rfail(r,0,"native retained MAIN transfer refused");goto done;
    }
    if(!r243_actor_script_scope_current(&scope))goto done;
    if(actor_step&&actor_step(actor_context,r243_actor_script_run_current,&scope)!=1){
        if(!r->error[0])rfail(r,0,"actor-script phase6 continuation refused");goto done;
    }
    if(memcmp(&scope,&sealed_scope,sizeof(scope))){rfail(r,0,"actor-script scope changed during actor continuation");goto done;}
    if(!r243_actor_script_scope_current(&scope))goto done;
    while(r237_bucket(r)<scope.host_ms){
        if(!r237_drain_bucket(r,&r->r237_frame_budget)||!r243_actor_script_scope_current(&scope))goto done;
        if(!r237_bucket_set(r,r237_bucket(r)+1,scope.time_root))goto done;
    }
    ok=1;
done:
    /* An error never publishes a completed transition. Restore the physical
     * queue base even on failure, retaining the real committed source prefix.
     * Failed runtime errors remain sticky; no queued work is marked finished. */
    if(!ok&&!r->error[0])rfail(r,0,"actor-script phase6 scope retired before completion");
    if(!r237_bucket_set(r,sealed_scope.host_ms,sealed_scope.time_root))ok=0;
    r->r243_actor_script_scope=0;
    ok=r237_finish_operation(r,ok);
    if(ok)*out=run;
    return ok;
}

/* Historical/private callers keep precisely the old NULL-hook behavior. */
static int r243_actor_script_phase6_owned(GscRuntime*r,uint64_t generation,
 const GscEntityScriptEntry*input,const GscEntityExecutionCertificate*certificate,const GscMapExecutionCertificate*map,
 int(*step)(void*,int(*)(void*),void*),void*context,GscEntityScriptRun*out,Task**owned_task){
 return r243_actor_script_phase6_owned_hook(r,generation,input,certificate,map,step,context,out,owned_task,NULL,NULL);
}

/* Retain private historical component ABI; only real fixed controller passes
 * its own map lease and captures a separate Task owner reference. */
static int r243_actor_script_phase6(GscRuntime*r,uint64_t generation,const GscEntityScriptEntry*input,
 const GscEntityExecutionCertificate*certificate,int(*step)(void*,int(*)(void*),void*),void*context,GscEntityScriptRun*out){
 return r243_actor_script_phase6_owned(r,generation,input,certificate,NULL,step,context,out,NULL);
}
