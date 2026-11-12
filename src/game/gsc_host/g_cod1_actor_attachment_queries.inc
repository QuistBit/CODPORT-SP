/* R203: genuine dense registered attachment queries. Retail getAttachSize
 * scans the seven model bytes until the first zero (20031450); the two name
 * queries require index<7 and an occupied slot (200314A9/20031529). The
 * admitted port representation has dense nonempty registrations only. Raw
 * descriptor mixtures and native empty-model/tag holes remain unavailable. */
static int cod1ActorAttachmentQueryBusy, cod1ActorAttachmentQueryCancelled;
static int ActorAttachmentStage(const cod1ActorLease_t *owner,int hostTime) {
    const cod1ActorRecord_t *a;
    if(!ActorLeaseValid(owner)||owner->kind!=COD1_ACTOR_LEASE_ACTOR||
       !cod1EntityReady||level.spawning||level.restarted||level.time!=hostTime)return 0;
    a=&cod1Actors[owner->actorIndex];
    /* Read-only native queries have neither an alive nor an initialized
     * predicate. Owned registered data and both mutation locks are required. */
    return a->nativeDataInitialized&&!a->spatial.busy&&!a->dobj.busy&&
        a->spatial.linked&&g_entities[owner->entity.slot].r.linked&&
        cod1ActorEngineSceneBusyEpoch!=owner->entity.epoch&&a->modelBusy==1&&
        a->modelMethodBusy==1&&cod1ActorModelRegistryBusyEpoch==owner->entity.epoch;
}
static int ActorAttachmentOwner(const GscEntityRef *ref,cod1ActorLease_t *owner) {
    const cod1ActorRecord_t *a;uint32_t index;
    if(!EntityValidate(NULL,ref)||!ActorMapAvailable()||level.spawning||level.restarted||
       !cod1ActorByEntity[ref->slot]||cod1ActorModelRegistryBusyEpoch)return 0;
    index=cod1ActorByEntity[ref->slot]-1;if(index>=COD1_ACTOR_CAPACITY)return 0;
    a=&cod1Actors[index];memset(owner,0,sizeof(*owner));owner->entity=*ref;owner->serial=a->serial;
    owner->actorIndex=index;owner->sentientIndex=a->sentientIndex;owner->kind=COD1_ACTOR_LEASE_ACTOR;
    return ActorLeaseValid(owner)&&a->nativeDataInitialized&&!a->spatial.busy&&!a->dobj.busy&&
        !a->modelBusy&&!a->modelMethodBusy&&a->spatial.linked&&g_entities[ref->slot].r.linked&&
        cod1ActorEngineSceneBusyEpoch!=ref->epoch;
}
/* Leaf only: the common protected-storage aggregate calls this function. */
static int ActorAttachmentStoragePrivate(const void *p,size_t n) {
    return ActorOverlap(p,n,G_Cod1ActorAttachmentQueryHost(),sizeof(GscAttachmentQueryHost)) ||
        ActorOverlap(p,n,cod1Actors,sizeof(cod1Actors)) ||
        ActorOverlap(p,n,cod1Sentients,sizeof(cod1Sentients)) ||
        ActorOverlap(p,n,cod1ActorByEntity,sizeof(cod1ActorByEntity)) ||
        ActorOverlap(p,n,cod1SentientByEntity,sizeof(cod1SentientByEntity)) ||
        ActorOverlap(p,n,cod1ActorRegisteredModels,sizeof(cod1ActorRegisteredModels)) ||
        ActorOverlap(p,n,&cod1ActorModelRegistryBusyEpoch,sizeof(cod1ActorModelRegistryBusyEpoch)) ||
        ActorOverlap(p,n,&cod1ActorAttachmentQueryBusy,sizeof(cod1ActorAttachmentQueryBusy)) ||
        ActorOverlap(p,n,&cod1ActorAttachmentQueryCancelled,sizeof(cod1ActorAttachmentQueryCancelled)) ||
        ActorOverlap(p,n,g_entities,sizeof(g_entities)) || ActorOverlap(p,n,&level,sizeof(level));
}
static int ActorAttachmentPrivate(const void *p,size_t n) {
    if(!n)return 0;
    return !p || n>UINTPTR_MAX-(uintptr_t)p ||
        ActorAttachmentStoragePrivate(p,n) || G_Cod1EntityStorageProtected(p,n);
}
static int ActorAttachmentLocalCurrent(const cod1ActorLease_t *owner,int hostTime,int spawning,
    const GscAttachmentQueryRequest *input,const GscAttachmentQueryRequest *request,
    const cod1ActorModelComposition_t *composition,
    const cod1ActorRegisteredAttachments_t *attachments) {
    if(cod1ActorAttachmentQueryCancelled || memcmp(input,request,sizeof(*request)) ||
       !!level.spawning!=spawning || !ActorAttachmentStage(owner,hostTime))return 0;
    return !memcmp(composition,&cod1Actors[owner->actorIndex].modelComposition,sizeof(*composition)) &&
        !memcmp(attachments,&cod1Actors[owner->actorIndex].registeredAttachments,sizeof(*attachments));
}
static int ActorAttachmentCurrent(const cod1ActorLease_t *owner,int hostTime,int spawning,
    const GscAttachmentQueryRequest *input,const GscAttachmentQueryRequest *request,
    const cod1ActorModelComposition_t *composition,
    const cod1ActorRegisteredAttachments_t *attachments,int (*current)(void *),void *lease) {
    return !cod1ActorAttachmentQueryCancelled && current && current(lease)==1 &&
        ActorAttachmentLocalCurrent(owner,hostTime,spawning,input,request,composition,attachments);
}
static int ActorAttachmentInvoke(void *user,const GscAttachmentQueryRequest *input,
    GscAttachmentQueryResult *output,char *error,size_t size,
    int (*current)(void *),void *lease) {
    GscAttachmentQueryRequest request;GscAttachmentQueryResult result;
    cod1ActorLease_t owner;
    cod1ActorModelComposition_t composition;cod1ActorRegisteredAttachments_t attachments;
    int hostTime,spawning,accepted=0,locked=0;uint32_t i;char text[MAX_STRING_CHARS],path[COD1_DOBJ_NAME_MAX];
    (void)user;
    if(!input || !output || !current || ActorAttachmentPrivate(input,sizeof(*input)) ||
       ActorAttachmentPrivate(output,sizeof(*output)) || ActorAttachmentPrivate(error,size) ||
       !ActorArguments(input,sizeof(*input),output,sizeof(*output),error,size) ||
       ActorOverlap(input,sizeof(*input),output,sizeof(*output)))return 0;
    if(cod1ActorAttachmentQueryBusy) {
        cod1ActorAttachmentQueryCancelled=1;
        EntityError(error,size,"actor attachment query reentry refused");return 0;
    }
    request=*input;hostTime=level.time;
    if(request.now_ms<0 || request.kind<GSC_ATTACH_QUERY_SIZE || request.kind>GSC_ATTACH_QUERY_TAG_NAME ||
       request.index<0 || request.index>=COD1_DOBJ_MODELS_MAX-1 ||
       (request.kind==GSC_ATTACH_QUERY_SIZE && request.index) ||
       !ActorAttachmentOwner(&request.entity,&owner)) {
        EntityError(error,size,"actor attachment query requires a current registered actor and occupied bounded index");return 0;
    }
    composition=cod1Actors[owner.actorIndex].modelComposition;
    attachments=cod1Actors[owner.actorIndex].registeredAttachments;
    if(attachments.count>COD1_DOBJ_MODELS_MAX-1 ||
       !composition.descriptorCount || composition.descriptorCount!=attachments.count+1 ||
       (request.kind!=GSC_ATTACH_QUERY_SIZE && (uint32_t)request.index>=attachments.count))return 0;
    /* Logical Session now_ms is distinct from absolute level.time. Both are
     * fenced in their own domains; no equality between these clocks is used. */
    cod1ActorAttachmentQueryBusy=1;cod1ActorAttachmentQueryCancelled=0;
    cod1Actors[owner.actorIndex].modelMethodBusy=1;spawning=!!level.spawning;
    cod1Actors[owner.actorIndex].modelBusy=1;cod1ActorModelRegistryBusyEpoch=owner.entity.epoch;
    locked=1;
    if(!ActorAttachmentCurrent(&owner,hostTime,spawning,input,&request,&composition,&attachments,current,lease))goto done;
    for(i=0;i<attachments.count;++i) {
        const cod1ActorModelRegistration_t *model=&attachments.models[i];
        const cod1ActorRegisteredModel_t *saved;
        if(memcmp(&model->lease,&owner,sizeof(owner)) || !model->modelIndex || model->modelIndex>=MAX_MODELS ||
           model->reserved || !memchr(model->nativeName,0,sizeof(model->nativeName)) ||
           !Cod1PrecacheName(model->nativeName,1) ||
           !memchr(composition.descriptors[i+1].tag,0,sizeof(composition.descriptors[i+1].tag)))goto done;
        saved=&cod1ActorRegisteredModels[model->modelIndex];
        if(saved->epoch!=owner.entity.epoch || strcmp(saved->name,model->nativeName) ||
           memcmp(&saved->resource,&model->resource,sizeof(model->resource)))goto done;
        ActorApplyPath(path,model->nativeName);
        if(strcmp(path,composition.descriptors[i+1].model) ||
           !ActorAttachmentCurrent(&owner,hostTime,spawning,input,&request,&composition,&attachments,current,lease))goto done;
        memset(text,0xa5,sizeof(text));trap_GetConfigstring(CS_MODELS+model->modelIndex,text,sizeof(text));
        if(!ActorAttachmentCurrent(&owner,hostTime,spawning,input,&request,&composition,&attachments,current,lease) ||
           !memchr(text,0,sizeof(text)) || strcmp(text,model->nativeName))goto done;
        if(!ActorModelValidateResource(&model->resource) ||
           !ActorAttachmentCurrent(&owner,hostTime,spawning,input,&request,&composition,&attachments,current,lease))goto done;
    }
    memset(&result,0,sizeof(result));
    if(request.kind==GSC_ATTACH_QUERY_SIZE){result.kind=GSC_ATTACH_RESULT_INTEGER;result.integer=(int32_t)attachments.count;}
    else {
        result.kind=GSC_ATTACH_RESULT_STRING;
        strcpy(result.string,request.kind==GSC_ATTACH_QUERY_MODEL_NAME?
            attachments.models[request.index].nativeName:composition.descriptors[request.index+1].tag);
    }
    if(!ActorAttachmentCurrent(&owner,hostTime,spawning,input,&request,&composition,&attachments,current,lease) ||
       ActorAttachmentPrivate(input,sizeof(*input)) || ActorAttachmentPrivate(output,sizeof(*output)) ||
       ActorAttachmentPrivate(error,size) ||
       !ActorArguments(input,sizeof(*input),output,sizeof(*output),error,size))goto done;
    /* LAST foreign current is above. These two qualified synchronous ENGINE
     * queries only validate/copy existing storage: no FS, allocation, GAME
     * callback or resource acquisition. Recheck every occupied registration
     * after that callback; another current here would reopen the witness gap. */
    for(i=0;i<attachments.count;++i) {
        const cod1ActorModelRegistration_t *model=&attachments.models[i];
        const cod1ActorRegisteredModel_t *saved=&cod1ActorRegisteredModels[model->modelIndex];
        if(saved->epoch!=owner.entity.epoch || strcmp(saved->name,model->nativeName) ||
           memcmp(&saved->resource,&model->resource,sizeof(model->resource)) ||
           !ActorModelValidateResource(&model->resource))goto done;
        memset(text,0xa5,sizeof(text));trap_GetConfigstring(CS_MODELS+model->modelIndex,text,sizeof(text));
        if(!memchr(text,0,sizeof(text)) || strcmp(text,model->nativeName))goto done;
    }
    accepted=ActorAttachmentLocalCurrent(&owner,hostTime,spawning,input,&request,&composition,&attachments);
done:
    if(locked)ActorModelEnd(&owner);
    if(ActorLeaseValid(&owner))cod1Actors[owner.actorIndex].modelMethodBusy=0;
    cod1ActorAttachmentQueryBusy=0;
    if(accepted){*output=result;return 1;}
    if(!ActorAttachmentPrivate(error,size))
        EntityError(error,size,"actor attachment registration, component, map or Session changed");return 0;
}
const GscAttachmentQueryHost *G_Cod1ActorAttachmentQueryHost(void) {
    static const GscAttachmentQueryHost host={NULL,ActorAttachmentInvoke};return &host;
}
