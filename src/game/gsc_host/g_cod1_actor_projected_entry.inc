/* Selected native-cell prefix. Caller retains destination and any current lease
 * on refusal; this is not the native allocator/failure-free or postinit path. */
typedef struct {
    ActorInheritedEntryContext base;
    int seedInitial;
    cod1NativeStaticText_t targetname;
    cod1ActorProjectedEntryResult_t result;
} ActorProjectedEntryContext;
static int ActorProjectedCopyGuard(void *user,const GscEntityFieldInheritance *request) {
    ActorProjectedEntryContext *context=user;
    cod1EntityNativeStaticRecord_t *record=NativeStaticFind(&request->destination);
    return ActorInheritedCopyGuard(&context->base,request)&&record&&record->projected&&
        record->sourceCertified;
}
static int ActorProjectedBeforeCopy(void *user,const GscEntityFieldInheritance *request,
    char *error,size_t size) {
    ActorProjectedEntryContext *context=user;cod1EntityNativeStaticSnapshot_t snapshot;
    if(!ActorInheritedCopyGuard(&context->base,request)||
       !G_Cod1EntityNativeStaticInitialAuthorityAvailable(&request->destination)) {
        EntityError(error,size,"static projection requires a fresh raw destination");return 0;
    }
    if(!G_Cod1EntityCaptureNativeStaticSource(&request->source,&snapshot,error,size)||
       !NativeStaticProject(&snapshot,&request->destination,error,size))return 0;
    context->result.copiedStatic=snapshot;context->result.staticProjected=1;return 1;
}
static int ActorProjectedAfterNative(void *user,const cod1ActorLease_t *lease,
    char *error,size_t size) {
    ActorProjectedEntryContext *context=user;
    cod1EntityNativeStaticRecord_t *record=NativeStaticFind(&lease->entity);
    if(!ActorLeaseValid(lease)||!record||!record->projected||
       !ActorRefEqual(&lease->entity,&context->base.request.destination)) {
        EntityError(error,size,"projected native data ownership changed");return 0;
    }
    /* Native c2a2 writes both health cells after successful reservations. The
     * legacy initializer remains unchanged for its existing callers. */
    g_entities[lease->entity.slot].health=100;
    record->stored.health=record->stored.maxhealth=100;return 1;
}
/* Exact source flag8 branch from6AEE4; destination flags are not its
 * authority. This callback is private to the actual dynamic Session path. */
static int ActorProjectedAfterLink(void*user,const cod1ActorLease_t*lease,char*error,size_t size){
 ActorProjectedEntryContext*context=user;uint32_t flags;
 if(!context->seedInitial||!ActorLeaseValid(lease)||
    !ActorRefEqual(&lease->entity,&context->base.request.destination)||
    !EntityValidate(NULL,&context->base.request.source)||
    !G_Cod1SpawnerIntakeIdentity(&context->base.request.source)){
  EntityError(error,size,"initial observations require current projected spawner and fresh linked Actor");return 0;}
 memcpy(&flags,&g_entities[context->base.request.source.slot].spawnflags,4);
 if(flags&8u)return 1; /* Native NOENEMYINFO skips the row writes only. */
 if(!ActorObservationSeedInitial(lease)){
  EntityError(error,size,"initial observations require the complete current Sentient cohort");return 0;}
 return 1;
}
static int ActorProjectedPrepare(void *user,const GscEntityFieldInheritance *request,
    GscEntityExecutionCertificate *certificate,char *error,size_t size) {
    ActorProjectedEntryContext *context=user;cod1EntityNativeStaticRecord_t *record;
    gentity_t *e;uint32_t flags;
    if(!ActorProjectedCopyGuard(user,request)) {
        EntityError(error,size,"projected raw ownership changed after inheritance");return 0;
    }
    record=NativeStaticFind(&request->destination);e=&g_entities[request->destination.slot];
    NativeStaticTextCopy(&record->stored.targetname,&context->targetname);
    e->targetname=record->stored.targetname.present?record->stored.targetname.text:NULL;
    memcpy(&flags,&e->spawnflags,sizeof(flags));flags&=~(uint32_t)1;
    memcpy(&e->spawnflags,&flags,sizeof(flags));record->stored.spawnflags=flags;
    context->result.overrideApplied=1;
    /* Native c1e6 precedes both component allocations. This raw byte is never
     * projected into the port model string/resource/configstring namespace. */
    record->stored.modelByte=0;
    return ActorInheritedPrepareCommon(&context->base,request,certificate,error,size,
        ActorProjectedAfterNative,context,
        context->seedInitial?ActorProjectedAfterLink:NULL,context);
}
static int ActorRunProjectedEntrySelected(GscRuntime *runtime,
    const cod1ActorProjectedEntryRequest_t *input,cod1ActorProjectedEntryResult_t *out,
    char *error,size_t size,int spawning,GscSession *session,uint64_t session_id) {
    ActorProjectedEntryContext context;GscInheritedEntityEntry request;
    GscProjectedEntityEntryHost host;GscProjectedEntityEntryRun result;int ok;
    if((!runtime&&!session)||!input||!out||
       (session?(!gsc_session_spawn_entry_storage_separate(session,input,sizeof(*input))||
                 !gsc_session_spawn_entry_storage_separate(session,out,sizeof(*out))||
                 !gsc_session_spawn_entry_storage_separate(session,error,size)):
                (!gsc_runtime_storage_separate(runtime,input,sizeof(*input))||
                 !gsc_runtime_storage_separate(runtime,out,sizeof(*out))||
                 !gsc_runtime_storage_separate(runtime,error,size)))||
       !ActorArguments(input,sizeof(*input),out,sizeof(*out),error,size)||
       ActorOverlap(input,sizeof(*input),out,sizeof(*out)))return 0;
    memset(&context,0,sizeof(context));context.base.request=input->inherited;context.seedInitial=session!=NULL;
    if(!NativeStaticTextValid(&input->targetname)||
       !memchr(context.base.request.module,0,sizeof(context.base.request.module))||!context.base.request.module[0]||
       !memchr(context.base.request.function,0,sizeof(context.base.request.function))||!context.base.request.function[0]||
       ActorRefEqual(&context.base.request.source,&context.base.request.destination)) {
        EntityError(error,size,"invalid projected actor entry request");return 0;
    }
    NativeStaticTextCopy(&context.targetname,&input->targetname);*out=context.result;
    if(cod1ActorInheritedEntryBusy||!EntityValidate(NULL,&context.base.request.source)||
       !ActorInheritedRaw(&context.base.request.destination)||
       !G_Cod1EntityNativeStaticInitialAuthorityAvailable(&context.base.request.destination)) {
        EntityError(error,size,"projected actor entry requires an untouched raw destination");return 0;
    }
    memset(&request,0,sizeof(request));request.inheritance.source=context.base.request.source;
    request.inheritance.destination=context.base.request.destination;request.entry.entity=context.base.request.destination;
    memcpy(request.entry.module,context.base.request.module,sizeof(request.entry.module));
    memcpy(request.entry.function,context.base.request.function,sizeof(request.entry.function));
    memset(&host,0,sizeof(host));host.inherited.user=&context;
    host.inherited.validate_copy=ActorProjectedCopyGuard;host.inherited.prepare=ActorProjectedPrepare;
    host.before_copy=ActorProjectedBeforeCopy;
    memset(&result,0,sizeof(result));cod1ActorInheritedEntryBusy=1;
    ok=session?gsc_session_execute_projected_spawn_entry(session,session_id,&request,&host,&result,error,size):
        spawning?gsc_execute_projected_spawn_entry(runtime,&request,&host,&result,error,size):
        gsc_execute_projected_entity_entry(runtime,&request,&host,&result,error,size);
    cod1ActorInheritedEntryBusy=0;
    context.base.result.inheritedFields=result.inherited.copied;
    if(result.inherited.stage>=GSC_INHERITED_ENTRY_FIELDS&&context.base.result.stage<COD1_ACTOR_ENTRY_FIELDS)
        context.base.result.stage=COD1_ACTOR_ENTRY_FIELDS;
    if(result.inherited.stage==GSC_INHERITED_ENTRY_ENTERED) {
        context.base.result.stage=COD1_ACTOR_ENTRY_ENTERED;context.base.result.entry=result.inherited.entry;
    }
    context.base.result.leaseRetained=context.base.result.lease.serial&&ActorLeaseValid(&context.base.result.lease);
    context.result.inherited=context.base.result;*out=context.result;return ok;
}
int G_Cod1ActorRunProjectedEntryStage(GscRuntime*r,const cod1ActorProjectedEntryRequest_t*q,
 cod1ActorProjectedEntryResult_t*out,char*e,size_t n){return ActorRunProjectedEntrySelected(r,q,out,e,n,0,NULL,0);}
int G_Cod1ActorRunProjectedSpawnEntryStage(GscRuntime*r,const cod1ActorProjectedEntryRequest_t*q,
 cod1ActorProjectedEntryResult_t*out,char*e,size_t n){return ActorRunProjectedEntrySelected(r,q,out,e,n,1,NULL,0);}

int G_Cod1ActorRunProjectedSpawnEntrySession(GscSession*s,uint64_t id,
 const cod1ActorProjectedEntryRequest_t*q,cod1ActorProjectedEntryResult_t*out,char*e,size_t n){
 return ActorRunProjectedEntrySelected(NULL,q,out,e,n,1,s,id);
}

#include "g_cod1_actor_spawn_allocation.inc"
