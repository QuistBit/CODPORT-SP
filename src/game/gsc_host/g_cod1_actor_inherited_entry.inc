/* R174 explicit middle segment. This is not the native entity allocator,
 * upstream static projection, aitype resolver or readiness producer. */
typedef struct {
    cod1ActorInheritedEntryRequest_t request;
    cod1ActorInheritedEntryResult_t result;
} ActorInheritedEntryContext;
static int cod1ActorInheritedEntryBusy;
static int ActorInheritedRaw(const GscEntityRef *ref) {
    const gentity_t *e;
    if(!EntityValidate(NULL,ref)||level.spawning||level.restarted||!ActorOwnerAllowed(ref,0))return 0;
    e=&g_entities[ref->slot];
    return !e->client&&!e->r.bmodel&&!e->r.linked&&!e->r.contents&&!e->s.solid&&
        !e->neverFree&&(e->r.svFlags&SVF_NOCLIENT)&&
        !(e->r.svFlags&(SVF_CASTAI|SVF_BOT));
}
static int ActorInheritedCopyGuard(void *user,const GscEntityFieldInheritance *request) {
    const ActorInheritedEntryContext *context=user;
    return ActorRefEqual(&request->source,&context->request.source)&&
        ActorRefEqual(&request->destination,&context->request.destination)&&
        EntityValidate(NULL,&request->source)&&ActorInheritedRaw(&request->destination);
}
static int ActorInheritedCertificate(const GscEntityRef *ref,uint64_t serial) {
    const cod1ActorRecord_t *a;cod1ActorLease_t lease;uint32_t index;
    if(!ref||!EntityValidate(NULL,ref)||!cod1ActorByEntity[ref->slot])return 0;
    index=cod1ActorByEntity[ref->slot]-1;if(index>=COD1_ACTOR_CAPACITY)return 0;
    a=&cod1Actors[index];memset(&lease,0,sizeof(lease));
    lease.entity=*ref;lease.serial=serial;lease.actorIndex=index;
    lease.sentientIndex=a->sentientIndex;lease.kind=COD1_ACTOR_LEASE_ACTOR;
    return ActorLeaseValid(&lease)&&a->serial==serial;
}
static int ActorInheritedPrepareCommon(void *user,const GscEntityFieldInheritance *request,
    GscEntityExecutionCertificate *certificate,char *error,size_t size,
    int (*after_native)(void *,const cod1ActorLease_t *,char *,size_t),void *after_user,
    int (*after_link)(void *,const cod1ActorLease_t *,char *,size_t),void *link_user) {
    ActorInheritedEntryContext *context=user;cod1ActorLease_t lease;
    if(!ActorInheritedCopyGuard(user,request)) {
        EntityError(error,size,"raw actor ownership changed after inheritance");return 0;
    }
    if(!G_Cod1ActorReserve(&request->destination,&lease,error,size))return 0;
    context->result.lease=lease;context->result.stage=COD1_ACTOR_ENTRY_RESERVED;
    if(!G_Cod1ActorInitializeNativeData(&lease,context->request.nativeServerFlags,
        context->request.nativeEntityFlags,error,size))return 0;
    if(after_native&&!after_native(after_user,&lease,error,size))return 0;
    context->result.stage=COD1_ACTOR_ENTRY_NATIVE_DATA;
    if(!ActorLeaseValid(&lease)||!EntityValidate(NULL,&request->source)) {
        EntityError(error,size,"actor source or component changed during native initialization");return 0;
    }
    if(!G_Cod1ActorPlaceAndLink(&lease,error,size))return 0;
    context->result.stage=COD1_ACTOR_ENTRY_LINKED;
    if(!ActorInheritedCertificate(&lease.entity,lease.serial)||!EntityValidate(NULL,&request->source)) {
        EntityError(error,size,"actor source or component changed during placement");return 0;
    }
    if(after_link&&!after_link(link_user,&lease,error,size))return 0;
    if(!ActorInheritedCertificate(&lease.entity,lease.serial)||!EntityValidate(NULL,&request->source)) {
        EntityError(error,size,"actor source or component changed after linked native prefix");return 0;
    }
    memset(certificate,0,sizeof(*certificate));certificate->owner=lease.entity;
    certificate->serial=lease.serial;certificate->current=ActorInheritedCertificate;return 1;
}
static int ActorInheritedPrepare(void *user,const GscEntityFieldInheritance *request,
    GscEntityExecutionCertificate *certificate,char *error,size_t size) {
    return ActorInheritedPrepareCommon(user,request,certificate,error,size,NULL,NULL,NULL,NULL);
}
int G_Cod1ActorRunInheritedEntryStage(GscRuntime *runtime,
    const cod1ActorInheritedEntryRequest_t *input,cod1ActorInheritedEntryResult_t *out,
    char *error,size_t size) {
    ActorInheritedEntryContext context;GscInheritedEntityEntry request;
    GscInheritedEntityEntryHost host;GscInheritedEntityEntryRun result;int ok;
    if(!runtime||!input||!out||
       !gsc_runtime_storage_separate(runtime,input,sizeof(*input))||
       !gsc_runtime_storage_separate(runtime,out,sizeof(*out))||
       !gsc_runtime_storage_separate(runtime,error,size)||
       !ActorArguments(input,sizeof(*input),out,sizeof(*out),error,size)||
       ActorOverlap(input,sizeof(*input),out,sizeof(*out)))return 0;
    memset(&context,0,sizeof(context));context.request=*input;
    if(!memchr(context.request.module,0,sizeof(context.request.module))||!context.request.module[0]||
       !memchr(context.request.function,0,sizeof(context.request.function))||!context.request.function[0]||
       ActorRefEqual(&context.request.source,&context.request.destination)) {
        EntityError(error,size,"invalid raw actor inherited entry request");return 0;
    }
    *out=context.result;
    if(cod1ActorInheritedEntryBusy||!EntityValidate(NULL,&context.request.source)||
       !ActorInheritedRaw(&context.request.destination)) {
        EntityError(error,size,"actor inherited entry requires a fresh unreserved hidden destination");return 0;
    }
    memset(&request,0,sizeof(request));request.inheritance.source=context.request.source;
    request.inheritance.destination=context.request.destination;request.entry.entity=context.request.destination;
    memcpy(request.entry.module,context.request.module,sizeof(request.entry.module));
    memcpy(request.entry.function,context.request.function,sizeof(request.entry.function));
    host.user=&context;host.validate_copy=ActorInheritedCopyGuard;host.prepare=ActorInheritedPrepare;
    memset(&result,0,sizeof(result));cod1ActorInheritedEntryBusy=1;
    ok=gsc_execute_inherited_entity_entry(runtime,&request,&host,&result,error,size);
    cod1ActorInheritedEntryBusy=0;
    context.result.inheritedFields=result.copied;
    if(result.stage>=GSC_INHERITED_ENTRY_FIELDS&&context.result.stage<COD1_ACTOR_ENTRY_FIELDS)
        context.result.stage=COD1_ACTOR_ENTRY_FIELDS;
    if(result.stage==GSC_INHERITED_ENTRY_ENTERED) {
        context.result.stage=COD1_ACTOR_ENTRY_ENTERED;context.result.entry=result.entry;
    }
    context.result.leaseRetained=context.result.lease.serial&&ActorLeaseValid(&context.result.lease);
    *out=context.result;return ok;
}
