/* Original allocation, no source clone/inheritance/targetname override. */
typedef struct {
    GscActorBootstrapServices services;
    cod1ActorPlacedEntryRequest_t request;
    cod1ActorPlacedEntryResult_t result;
    uint64_t declarationReceipt;void *bsp;
} ActorPlacedEntryContext;
static int ActorBspPrepareCurrent(ActorPlacedEntryContext *,const GscEntityScriptEntry *);
static int ActorBspCertificate(const GscEntityRef *,uint64_t);
static int ActorPlacedEntryCurrent(ActorPlacedEntryContext *context) {
    Cod1PlacedActorRecord *record=PlacedFind(&context->request.entity);
    cod1EntityNativeStaticRecord_t *native=NativeStaticFind(&context->request.entity);
    return record&&!record->snapshot.startupConsumed&&record->snapshot.privateFieldsAvailable&&
        record->snapshot.nativeStaticAvailable&&record->declarationReceipt==context->declarationReceipt&&
        native&&native->sourceCertified&&PlacedOriginalRaw(&context->request.entity)&&ActorInheritedRaw(&context->request.entity);
}
static int ActorPlacedCommitFields(void *user,const GscEntityScriptEntry *entry,char *error,size_t size) {
    ActorPlacedEntryContext *context=user;Cod1PlacedActorRecord *record;int current;
    if(!ActorRefEqual(&entry->entity,&context->request.entity)||!ActorPlacedEntryCurrent(context)||cod1PlacedBusy) {
        EntityError(error,size,"placed startup identity or one-shot admission changed");return 0;
    }
    cod1PlacedBusy=1;current=PlacedDeclarationsCurrent(context->declarationReceipt,context->request.entity.epoch);cod1PlacedBusy=0;
    if(!current||ActorServicesFailed(&context->services)||!ActorPlacedEntryCurrent(context)) {
        EntityError(error,size,"placed declaration or allocation authority changed");return 0;
    }
    /* Final pure exact-ref guard. No callback follows this one-shot mark
     * before the VM publishes its already staged private string records. */
    record=PlacedFind(&context->request.entity);record->snapshot.startupConsumed=1;
    context->result.startupConsumed=1;return 1;
}
static int ActorPlacedPrepare(void *user,const GscEntityScriptEntry *entry,
    GscEntityExecutionCertificate *certificate,char *error,size_t size) {
    ActorPlacedEntryContext *context=user;Cod1PlacedActorRecord *record;
    cod1EntityNativeStaticRecord_t *native;cod1ActorLease_t lease;
    record=PlacedFind(&context->request.entity);native=NativeStaticFind(&context->request.entity);
    if(context->bsp?!ActorBspPrepareCurrent(context,entry):
       (!ActorRefEqual(&entry->entity,&context->request.entity)||!record||!record->snapshot.startupConsumed||
        !native||!native->sourceCertified||!PlacedOriginalRaw(&context->request.entity)||!ActorInheritedRaw(&context->request.entity))) {
        EntityError(error,size,"placed actor changed after private field import");return 0;
    }
    /* Early scope validation invokes its current owner. Reacquire the source
     * after that callback; a same-allocation source invalidation is terminal. */
    native=NativeStaticFind(&context->request.entity);
    if(!native||!native->sourceCertified){EntityError(error,size,"placed native source retired before reservation");return 0;}
    native->stored.modelByte=0;
    if(!G_Cod1ActorReserve(&context->request.entity,&lease,error,size))return 0;
    context->result.lease=lease;context->result.stage=COD1_ACTOR_ENTRY_RESERVED;
    if(!G_Cod1ActorInitializeNativeData(&lease,context->request.nativeServerFlags,context->request.nativeEntityFlags,error,size))return 0;
    native=NativeStaticFind(&lease.entity);
    if(!ActorLeaseValid(&lease)||!native) {EntityError(error,size,"placed native ownership changed");return 0;}
    g_entities[lease.entity.slot].health=100;native->stored.health=native->stored.maxhealth=100;
    context->result.stage=COD1_ACTOR_ENTRY_NATIVE_DATA;
    if(!G_Cod1ActorPlaceAndLink(&lease,error,size))return 0;
    context->result.stage=COD1_ACTOR_ENTRY_LINKED;
    if(context->bsp?!ActorBspPrepareCurrent(context,entry):!ActorInheritedCertificate(&lease.entity,lease.serial)) {EntityError(error,size,"placed component changed during placement");return 0;}
    memset(certificate,0,sizeof(*certificate));certificate->owner=lease.entity;
    certificate->serial=lease.serial;certificate->current=context->bsp?ActorBspCertificate:ActorInheritedCertificate;return 1;
}
static int ActorPlacedEntryPhase(const GscActorBootstrapServices *services,const cod1ActorPlacedEntryRequest_t *input,
    cod1ActorPlacedEntryResult_t *out,char *error,size_t size,int construct_only) {
    ActorPlacedEntryContext context;GscPreparedEntityEntry request;GscPreparedEntityEntryHost host;
    GscPreparedEntityEntryRun result;Cod1PlacedActorRecord *record;uint32_t i;int ok;
    if(!ActorServicesValid(services)||(construct_only&&!services->construct_entity)||!input||!out||!services->storage_separate(services->context,input,sizeof(*input))||
       !services->storage_separate(services->context,out,sizeof(*out))||!services->storage_separate(services->context,error,size)||
       !ActorArguments(input,sizeof(*input),out,sizeof(*out),error,size)||ActorOverlap(input,sizeof(*input),out,sizeof(*out)))return 0;
    memset(&context,0,sizeof(context));context.services=*services;context.request=*input;
    if(!memchr(context.request.module,0,sizeof(context.request.module))||!context.request.module[0]||
       !memchr(context.request.function,0,sizeof(context.request.function))||!context.request.function[0]) {
        EntityError(error,size,"invalid placed actor entry request");return 0;
    }
    *out=context.result;record=PlacedFind(&context.request.entity);
    if(record){context.result.startupConsumed=record->snapshot.startupConsumed;*out=context.result;}
    if(!record||cod1ActorInheritedEntryBusy||cod1PlacedBusy||!EntityValidate(NULL,&context.request.entity)||
       !PlacedOriginalRaw(&context.request.entity)||!ActorInheritedRaw(&context.request.entity)||record->snapshot.startupConsumed||
       !record->snapshot.privateFieldsAvailable||!record->snapshot.nativeStaticAvailable) {
        EntityError(error,size,"placed startup requires a fresh typed original allocation");return 0;
    }
    context.declarationReceipt=record->declarationReceipt;
    memset(&request,0,sizeof(request));request.entry.entity=context.request.entity;
    memcpy(request.entry.module,context.request.module,sizeof(request.entry.module));memcpy(request.entry.function,context.request.function,sizeof(request.entry.function));
    request.field_count=record->snapshot.privateFieldCount;
    for(i=0;i<request.field_count;++i) {
        strcpy(request.fields[i].name,cod1BspDeclarations.fields[record->privateDecl[i]].name);
        strcpy(request.fields[i].string,record->raw+record->privateValue[i]);
    }
    host.user=&context;host.commit_fields=ActorPlacedCommitFields;host.prepare=ActorPlacedPrepare;
    memset(&result,0,sizeof(result));cod1ActorInheritedEntryBusy=1;
    ok=construct_only?context.services.construct_entity(context.services.context,&request,&host,&result,error,size):
        context.services.execute_prepared_entity_entry(context.services.context,&request,&host,&result,error,size);cod1ActorInheritedEntryBusy=0;
    context.result.initialFields=result.fields;
    if(result.stage>=GSC_PREPARED_ENTRY_FIELDS&&context.result.stage<COD1_ACTOR_ENTRY_FIELDS)context.result.stage=COD1_ACTOR_ENTRY_FIELDS;
    if(result.stage==GSC_PREPARED_ENTRY_ENTERED){context.result.stage=COD1_ACTOR_ENTRY_ENTERED;context.result.entry=result.entry;}
    context.result.leaseRetained=context.result.lease.serial&&ActorLeaseValid(&context.result.lease);
    *out=context.result;return ok;
}

int G_Cod1ActorRunPlacedEntryStageViaServices(const GscActorBootstrapServices*s,const cod1ActorPlacedEntryRequest_t*q,cod1ActorPlacedEntryResult_t*out,char*error,size_t cap){return ActorPlacedEntryPhase(s,q,out,error,cap,0);}

int G_Cod1ActorRunPlacedEntryStage(GscRuntime *runtime,const cod1ActorPlacedEntryRequest_t *input,
    cod1ActorPlacedEntryResult_t *out,char *error,size_t size) {
    GscActorBootstrapServices services=ActorLegacyServices(runtime);
    return G_Cod1ActorRunPlacedEntryStageViaServices(&services,input,out,error,size);
}

/* A constructed owner is already reserved and linked. Fresh-allocation
 * predicates deliberately remain unsuitable here. Callers additionally check
 * the exact bootstrap phase/row; this predicate grants no phase authority. */
static int ActorConstructedOriginalCurrent(const GscEntityRef *ref) {
    Cod1PlacedActorRecord *record;cod1EntityNativeStaticRecord_t *native;
    cod1ActorRecord_t *a;cod1ActorLease_t lease;cod1NativeStaticText_t classname;
    const gentity_t *e;uint32_t index;
    if(!ref||ref->slot>=MAX_GENTITIES||!EntityValidate(NULL,ref)||level.spawning||level.restarted||
       !(index=cod1ActorByEntity[ref->slot])||index>COD1_ACTOR_CAPACITY)return 0;
    a=&cod1Actors[index-1];memset(&lease,0,sizeof(lease));lease.entity=*ref;lease.serial=a->serial;
    lease.actorIndex=index-1;lease.sentientIndex=a->sentientIndex;lease.kind=COD1_ACTOR_LEASE_ACTOR;
    if(!ActorLeaseValid(&lease))return 0;
    record=PlacedFind(ref);native=NativeStaticFind(ref);e=&g_entities[ref->slot];
    /* ActorReserve consumes the static source certificate permanently. Later
     * execution uses the exact Actor lease, never restores that source ticket. */
    return record&&record->snapshot.startupConsumed&&record->snapshot.privateFieldsAvailable&&
        record->snapshot.nativeStaticAvailable&&native&&!native->sourceCertified&&native->everChanged&&
        native->extrasPresent&&native->published&&!native->projected&&
        NativeStaticTextCapture(e->classname,&classname)&&classname.present&&
        !strcmp(classname.text,record->originalClassname)&&!(e->spawnflags&1)&&
        a->alive&&a->dataInitialized&&a->nativeDataInitialized&&!a->initialized&&!a->positionPostinit&&
        a->spatial.linked&&!a->spatial.busy&&!a->dobj.busy&&!a->modelBusy&&!a->modelMethodBusy&&
        cod1ActorEngineSceneBusyEpoch!=ref->epoch&&!e->client&&!e->r.bmodel&&e->r.linked&&
        !e->r.contents&&!e->s.solid&&!e->neverFree&&(e->r.svFlags&SVF_NOCLIENT)&&
        !(e->r.svFlags&(SVF_CASTAI|SVF_BOT));
}
