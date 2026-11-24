/* R190 selected GAME owner for the unmodified R189 kernel. No pointers or
 * native string IDs in persistent records. Actor existence, not AI readiness.
 * Native argument getters are synchronous lazy callbacks: commit precedes
 * reading label; error/cancellation never rolls that commit back. */
static int ActorCriticalReady(const cod1ActorLease_t *lease) {
    return cod1EntityReady&&ActorLeaseValid(lease)&&lease->kind==COD1_ACTOR_LEASE_ACTOR&&
        !level.spawning&&!level.restarted&&
        !cod1Actors[lease->actorIndex].criticalBusy;
}
static int ActorCriticalCapture(const GscEntityRef *input,cod1ActorLease_t *out) {
    GscEntityRef ref;cod1ActorLease_t lease;cod1ActorRecord_t *a;uint32_t i;
    if(!input||!out)return 0;
    ref=*input;
    if(!ActorMapAvailable()||!G_Cod1EntitySameAllocation(&ref)||
       ref.slot>=MAX_GENTITIES||!cod1ActorByEntity[ref.slot])return 0;
    i=cod1ActorByEntity[ref.slot]-1;if(i>=COD1_ACTOR_CAPACITY)return 0;
    a=&cod1Actors[i];memset(&lease,0,sizeof(lease));lease.entity=ref;
    lease.serial=a->serial;lease.actorIndex=i;lease.sentientIndex=a->sentientIndex;
    lease.kind=COD1_ACTOR_LEASE_ACTOR;
    if(!ActorCriticalReady(&lease))return 0;
    *out=lease;return 1;
}
int G_Cod1ActorCriticalRead(const cod1ActorLease_t *input,
    cod1ActorCriticalSnapshot_t *out,char *error,size_t size) {
    cod1ActorLease_t lease;cod1ActorRecord_t *a;cod1ActorCriticalSnapshot_t copy;cod1ActorCriticalState_t kernel;int i;
    if(!ActorArguments(input,sizeof(*input),out,sizeof(*out),error,size))return 0;
    if(!input||!out){EntityError(error,size,"critical read requires lease and output");return 0;}
    lease=*input;
    /* Read-only snapshots during a getter are deliberately allowed: callers
     * can observe the already committed native phase without mutating it. */
    if(!ActorLeaseValid(&lease)||lease.kind!=COD1_ACTOR_LEASE_ACTOR){
        EntityError(error,size,"critical read requires the current Actor lease");return 0;}
    a=&cod1Actors[lease.actorIndex];memset(&copy,0,sizeof(copy));copy.lease=lease;
    for(i=0;i<COD1_CRITICAL_COMMAND_CAPACITY;++i){
        copy.command_operations[i]=a->critical.commands[i].operation;
        copy.command_states[i]=a->critical.commands[i].state;
    }
    for(i=0;i<COD1_CRITICAL_STACK_CAPACITY;++i)copy.stack[i]=a->critical.stack[i];
    copy.command_count=a->critical.command_count;copy.top=a->critical.top;
    kernel=ActorStateQueueKernel(a);copy.native_cell02a4=kernel.native_cell02a4;
    copy.has_label=a->criticalHasLabel;copy.has_script_label=a->criticalHasScriptLabel;
    memcpy(copy.label,a->criticalLabel,sizeof(copy.label));
    memcpy(copy.script_label,a->criticalScriptLabel,sizeof(copy.script_label));
    *out=copy;return 1;
}
int G_Cod1ActorCriticalSetScriptLabel(const cod1ActorLease_t *input,
    const char *label,char *error,size_t size) {
    cod1ActorLease_t lease;cod1ActorRecord_t *a;char text[128];size_t n=0;
    if(!ActorArguments(input,sizeof(*input),NULL,0,error,size)||
       ActorOverlap(label,label?128:0,error,size))return 0;
    if(!input){EntityError(error,size,"script metadata requires Actor lease");return 0;}
    lease=*input;
    if(cod1ActorCriticalBusy||!ActorCriticalReady(&lease)){
        EntityError(error,size,"script metadata requires idle current Actor");return 0;}
    memset(text,0,sizeof(text));
    if(label){while(n<sizeof(text)&&label[n])++n;
        if(n==sizeof(text)){EntityError(error,size,"script metadata label exceeds128bytes");return 0;}
        memcpy(text,label,n);
    }
    a=&cod1Actors[lease.actorIndex];memcpy(a->criticalScriptLabel,text,sizeof(text));
    a->criticalHasScriptLabel=label!=NULL;return 1;
}
int G_Cod1ActorCriticalSetDisabled(int32_t value,char *error,size_t size) {
    if(!ActorMapAvailable()||level.spawning||level.restarted||cod1ActorCriticalBusy){
        EntityError(error,size,"critical flag requires idle ready CoD map");return 0;}
    cod1ActorCriticalDisabled=value;return 1;
}
int G_Cod1ActorCriticalInvoke(const cod1ActorLease_t *input,uint32_t method,
    const GscActorCriticalCall *request,int32_t *out,char *error,size_t size) {
    cod1ActorLease_t lease;GscActorCriticalCall call;cod1ActorRecord_t *a;
    char text[128];cod1ActorCriticalState_t kernel;int status,ok=0,read=0;int32_t result=0;
    if(!ActorArguments(input,sizeof(*input),out,sizeof(*out),error,size)||
       ActorOverlap(request,sizeof(*request),error,size))return 0;
    if(!input||!request||!out){EntityError(error,size,"critical call requires copied inputs and output");return 0;}
    lease=*input;call=*request;
    if(cod1ActorCriticalBusy||!ActorCriticalReady(&lease)){
        EntityError(error,size,"critical method requires a current idle Actor, not a player or stale lease");return 0;}
    if(method<GSC_ACTOR_CRITICAL_ENTER||method>GSC_ACTOR_CRITICAL_QUERY||
       (call.has_argument!=0&&call.has_argument!=1)||!call.current||
       (call.has_argument&&!call.read_label)){
        EntityError(error,size,"critical method callback contract unavailable");return 0;}
    a=&cod1Actors[lease.actorIndex];a->criticalBusy=1;cod1ActorCriticalBusy=1;
    if(call.current(call.context)!=1||!ActorLeaseValid(&lease)){
        EntityError(error,size,"critical caller or Actor retired before operation");goto done;
    }
    kernel=ActorStateQueueKernel(a);
    /* Pure query ignores the native disable flag. Original wrapper validates
     * Actor before flag; this validation deliberately requires no AI init. */
    if(method==GSC_ACTOR_CRITICAL_QUERY){
        status=Cod1ActorCriticalContains(&kernel);
        if(status<0){EntityError(error,size,"critical stack outside selected bounds (HELD)");goto done;}
        result=status;ok=1;goto done;
    }
    status=method==GSC_ACTOR_CRITICAL_ENTER?
        Cod1ActorCriticalBegin(&kernel,cod1ActorCriticalDisabled):
        Cod1ActorCriticalEnd(&kernel,cod1ActorCriticalDisabled);
    if(status==COD1_CRITICAL_HELD){EntityError(error,size,"critical state outside selected safe domain (HELD)");goto done;}
    ActorStateQueueCommit(a,&kernel);
    if(status==COD1_CRITICAL_DISABLED){ok=1;goto done;}
    if(method==GSC_ACTOR_CRITICAL_LEAVE){
        if(status==COD1_CRITICAL_LEAVE_WARNING)
            G_Printf("WARNING: entity %u leaveCriticalSection without top critical state\n",(unsigned)lease.entity.slot);
        ok=1;goto done;
    }
    if(status==COD1_CRITICAL_DUPLICATE){
        EntityError(error,size,"enterCriticalSection called while already in a critical section");goto done;}
    /* State commit above is retained even if this getter, its context, or the
     * later metadata resolution fails. Do not stage or restore the record. */
    memset(text,0,sizeof(text));
    if(call.has_argument){
        read=call.read_label(call.context,text,sizeof(text),error,size);
        if(!ActorLeaseValid(&lease)){EntityError(error,size,"critical Actor retired during label read");goto done;}
        if(call.current(call.context)!=1||!ActorLeaseValid(&lease)){
            EntityError(error,size,"critical caller retired during label read");goto done;}
        if(read!=1)goto done;
        if(!memchr(text,0,sizeof(text))){EntityError(error,size,"critical label is not bounded (HELD)");goto done;}
    }else{
        if(!a->criticalHasScriptLabel){EntityError(error,size,"current-script critical label not supplied (HELD)");goto done;}
        memcpy(text,a->criticalScriptLabel,sizeof(text));
    }
    a=&cod1Actors[lease.actorIndex];memcpy(a->criticalLabel,text,sizeof(text));
    a->criticalHasLabel=1;ok=1;
 done:
    if(ok&&(call.current(call.context)!=1||!ActorLeaseValid(&lease))){
        EntityError(error,size,"critical caller or Actor retired during callback");ok=0;
    }
    /* A callback may free/reuse an Actor slot or reset the whole map. Never
     * unlock, label, or roll back a replacement through the old lease. */
    if(ActorLeaseValid(&lease))cod1Actors[lease.actorIndex].criticalBusy=0;
    cod1ActorCriticalBusy=0;
    if(ok)*out=result;
    return ok;
}
static int ActorCriticalHostInvoke(void *user,const GscEntityRef *input,uint32_t method,
    const GscActorCriticalCall *call,int32_t *out,char *error,size_t size) {
    cod1ActorLease_t lease;(void)user;
    if(!ActorArguments(input,sizeof(*input),out,sizeof(*out),error,size)||
       ActorOverlap(call,sizeof(*call),error,size))return 0;
    if(!ActorCriticalCapture(input,&lease)){
        EntityError(error,size,"critical receiver has no current Actor");return 0;}
    return G_Cod1ActorCriticalInvoke(&lease,method,call,out,error,size);
}
const GscActorCriticalHost *G_Cod1ActorCriticalHost(void) {
    static const GscActorCriticalHost host={NULL,ActorCriticalHostInvoke};return &host;
}

extern int R243CriticalSessionTake(const GscEntityRef*,uint32_t,const GscActorCriticalCall*);
static int ActorCriticalSessionInvoke(void*u,const GscEntityRef*e,uint32_t method,const GscActorCriticalCall*q,int32_t*out,char*error,size_t cap){
 if(!R243CriticalSessionTake(e,method,q))return 0;
 return ActorCriticalHostInvoke(u,e,method,q,out,error,cap);
}
const GscActorCriticalHost*G_Cod1ActorCriticalSessionHost(void){static const GscActorCriticalHost h={NULL,ActorCriticalSessionInvoke};return &h;}

/* Native state8 think 200593E7..2005940E requests the CURRENT MAIN pair.
 * The selected prospective stack is the canonical enter/leaveCriticalSection
 * state; do not replace its owner during an unfinished pose transition.
 * source0 means retain the actual owned task, never create a fallback task. */
int G_Cod1ActorLocomotionSource(const GscEntityRef*input,uint64_t serial,int now,
 int desired,int*out,int(*current)(void*),void*ctx){
 GscEntityRef ref;cod1ActorLease_t lease,after;cod1ActorCriticalState_t before,again;int critical;
 if(!input||!out||!current||(desired!=1&&desired!=2)||now!=level.time||
    !ActorArguments(input,sizeof(*input),out,sizeof(*out),NULL,0))return 0;
 ref=*input;
 if(current(ctx)!=1||!ActorCriticalCapture(&ref,&lease)||lease.serial!=serial)return 0;
 before=ActorStateQueueKernel(cod1Actors+lease.actorIndex);
 critical=Cod1ActorCriticalContains(&before);if(critical<0)return 0;
 if(current(ctx)!=1||now!=level.time||memcmp(input,&ref,sizeof(ref))||
    !ActorCriticalCapture(&ref,&after)||memcmp(&lease,&after,sizeof(lease)))return 0;
 again=ActorStateQueueKernel(cod1Actors+lease.actorIndex);
 if(memcmp(&before,&again,sizeof(before)))return 0;
 *out=critical?0:desired;return 1;
}
