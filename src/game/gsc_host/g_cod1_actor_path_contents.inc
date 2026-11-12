/* Native6DBB0 needs actual entity134 flags and entityC4 contents. These
 * accessors expose only the existing allocation-owned native actor record.
 * The complete world cohort/bracket and graph publisher remain separate. */
static int ActorPathOutputPrivate(const void *p,size_t n) {
    return ActorOverlap(p,n,cod1Actors,sizeof(cod1Actors)) ||
        ActorOverlap(p,n,cod1Sentients,sizeof(cod1Sentients)) ||
        ActorOverlap(p,n,cod1ActorByEntity,sizeof(cod1ActorByEntity)) ||
        ActorOverlap(p,n,cod1SentientByEntity,sizeof(cod1SentientByEntity)) ||
        ActorOverlap(p,n,g_entities,sizeof(g_entities)) ||
        ActorOverlap(p,n,cod1EntityGeneration,sizeof(cod1EntityGeneration)) ||
        ActorOverlap(p,n,cod1EntityRetired,sizeof(cod1EntityRetired));
}
int G_Cod1ActorPathRead(const GscEntityRef *input,cod1ActorPathView_t *out) {
    GscEntityRef ref;cod1ActorPathView_t copy;const cod1ActorRecord_t *a;
    unsigned index;
    if(!input||!out||ActorPathOutputPrivate(out,sizeof(*out)) ||
       ActorOverlap(input,sizeof(*input),out,sizeof(*out)))return 0;
    ref=*input;
    if(!ActorMapAvailable()||!G_Cod1EntitySameAllocation(&ref))return 0;
    index=cod1ActorByEntity[ref.slot];
    if(!index||index>COD1_ACTOR_CAPACITY)return 0;
    a=cod1Actors+index-1;memset(&copy,0,sizeof(copy));
    copy.lease.entity=ref;copy.lease.serial=a->serial;
    copy.lease.actorIndex=index-1;copy.lease.sentientIndex=a->sentientIndex;
    copy.lease.kind=COD1_ACTOR_LEASE_ACTOR;
    if(!ActorLeaseValid(&copy.lease)||!a->nativeDataInitialized)return 0;
    copy.flags=a->nativeData.entity.nativeEntityFlags;
    copy.contents=a->nativeData.entity.nativeContents;copy.linked=a->spatial.linked;
    *out=copy;return 1;
}
int G_Cod1ActorPathRetired(const cod1ActorLease_t *input) {
    cod1ActorLease_t old,current;const cod1ActorRecord_t *a;
    if(!input)return 0;old=*input;
    if(!ActorMapAvailable()||!old.serial||old.kind!=COD1_ACTOR_LEASE_ACTOR||
       old.actorIndex>=COD1_ACTOR_CAPACITY||old.sentientIndex>=COD1_SENTIENT_CAPACITY||
       !old.entity.epoch||!old.entity.generation||old.entity.slot>=MAX_GENTITIES)return 0;
    a=cod1Actors+old.actorIndex;
    if(a->activation<0)return !a->serial&&!a->owner.epoch&&a->sentientIndex==COD1_ACTOR_NO_INDEX;
    memset(&current,0,sizeof(current));current.entity=a->owner;current.serial=a->serial;
    current.actorIndex=old.actorIndex;current.sentientIndex=a->sentientIndex;current.kind=COD1_ACTOR_LEASE_ACTOR;
    return ActorLeaseValid(&current)&&(current.serial!=old.serial||!ActorRefEqual(&current.entity,&old.entity));
}
int G_Cod1ActorPathContents(const cod1ActorLease_t *input,
    uint32_t expected,uint32_t replacement) {
    cod1ActorLease_t lease;cod1ActorRecord_t *a;
    cod1SpatialActorContents_t request;int status,restore=COD1_SPATIAL_OK;
    if(!input)return 0;lease=*input;
    if(!ActorLeaseValid(&lease)||lease.kind!=COD1_ACTOR_LEASE_ACTOR)return 0;
    a=cod1Actors+lease.actorIndex;
    if(!a->nativeDataInitialized||a->spatial.busy||a->dobj.busy||a->modelBusy||
       a->modelMethodBusy||cod1ActorEngineSceneBusyEpoch==lease.entity.epoch||
       a->nativeData.entity.nativeContents!=expected)return 0;
    if(!a->spatial.linked){
        /* A hidden unlinked native record has no SERVER contents owner. Do
         * not treat a legacy/shared link as an absent native registration. */
        if(g_entities[lease.entity.slot].r.linked)return 0;
        a->nativeData.entity.nativeContents=replacement;return 1;
    }
    memset(&request,0,sizeof(request));request.version=COD1_SPATIAL_VERSION;
    request.bytes=sizeof(request);request.key=ActorSpatialKey(&lease);
    request.expectedContents=expected;request.replacementContents=replacement;
    a->spatial.busy=1;status=trap_Cod1SpatialActorContents(&request);
    if(ActorLeaseValid(&lease)){
        a=cod1Actors+lease.actorIndex;
        if(status==COD1_SPATIAL_OK&&a->nativeDataInitialized&&a->spatial.linked&&
           a->spatial.busy==1&&a->nativeData.entity.nativeContents==expected){
            a->nativeData.entity.nativeContents=replacement;a->spatial.busy=0;return 1;
        }
    }
    /* The actual SERVER operation has no callbacks. Defensively compensate
     * an OK across an invalidated GAME owner with the OLD key only. It can
     * never restore into a new lease/map occupying the same entity slot. */
    if(status==COD1_SPATIAL_OK){
        request.expectedContents=replacement;request.replacementContents=expected;
        restore=trap_Cod1SpatialActorContents(&request);
    }
    /* An unconfirmed compensation on a surviving lease retains its fence;
     * no retry may pretend that GAME and SERVER contents still agree. */
    if(restore==COD1_SPATIAL_OK&&ActorLeaseValid(&lease)&&cod1Actors[lease.actorIndex].spatial.busy==1)
        cod1Actors[lease.actorIndex].spatial.busy=0;
    return 0;
}
