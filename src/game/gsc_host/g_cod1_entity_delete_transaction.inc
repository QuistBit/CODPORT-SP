/* R204 opt-in death notification then actual production G_FreeEntity. This
 * lane admits exact completed provisional prefix owners without weakening any
 * generic allocator, actor, player, visibility or legacy delete predicate. */
typedef struct { GscEntityRef entity;int attempted,notified,retired; } Cod1DeleteAttempt;
static Cod1DeleteAttempt cod1DeleteAttempts[MAX_GENTITIES];
static cod1ActorDeleteOwnerHooks_t cod1DeleteOwnerHooks;
static uint64_t cod1DeleteOwnerEpoch;
static void DeleteTransactionOwnerReset(void) {
    if(cod1EntityDeleteBusy)cod1EntityDeleteCancelled=1;
    RuntimeActorDeleteReset();
    memset(&cod1DeleteOwnerHooks,0,sizeof(cod1DeleteOwnerHooks));cod1DeleteOwnerEpoch=0;
}
static const cod1ActorDeleteOwnerHooks_t jumperRetireHooks;
static int DeleteTransactionOwnerCurrent(uint64_t epoch,const cod1ActorDeleteOwnerHooks_t *owner) {
    if(!memcmp(owner,&jumperRetireHooks,sizeof(*owner)))return epoch&&epoch==G_Cod1EntityMapEpoch();
    return epoch&&epoch==G_Cod1EntityMapEpoch()&&epoch==cod1DeleteOwnerEpoch&&
        !memcmp(owner,&cod1DeleteOwnerHooks,sizeof(*owner));
}
static struct {
    int active,checking,isActor,hostTime,runtimeActor,stage;GscEntityRef entity;cod1ActorLease_t lease;
    gentity_t runtimeBody;cod1ActorRecord_t runtimeOwner;
    const GscDeleteTransactionRequest *input;const GscDeleteTransactionServices *services;
    GscDeleteTransactionRequest request;GscDeleteTransactionServices api;
    cod1ActorDeleteOwnerHooks_t owner;
} cod1DeleteFreeFence;
static int DeleteTransactionTopology(const GscEntityRef *,const cod1ActorLease_t *);
static int DeleteTransactionFreeGuard(const GscEntityRef *entity) {
    gentity_t before;cod1ActorRecord_t actor,expected;int live,result;
    if(!cod1DeleteFreeFence.active||!ActorRefEqual(entity,&cod1DeleteFreeFence.entity))return 1;
    if(cod1DeleteFreeFence.checking){cod1EntityDeleteCancelled=1;return 0;}
    if(cod1EntityDeleteCancelled||G_Cod1EntityMapEpoch()!=entity->epoch||
       level.time!=cod1DeleteFreeFence.hostTime||level.spawning||level.restarted||
       !G_Cod1EntitySameAllocation(entity)||
       memcmp(cod1DeleteFreeFence.input,&cod1DeleteFreeFence.request,sizeof(cod1DeleteFreeFence.request))||
       memcmp(cod1DeleteFreeFence.services,&cod1DeleteFreeFence.api,sizeof(cod1DeleteFreeFence.api)))return 0;
    if(cod1DeleteFreeFence.isActor) {
        if(!DeleteTransactionOwnerCurrent(entity->epoch,&cod1DeleteFreeFence.owner)||
           !DeleteTransactionTopology(entity,&cod1DeleteFreeFence.lease))return 0;
        if(cod1DeleteFreeFence.runtimeActor){
            if(cod1DeleteFreeFence.stage<0||cod1DeleteFreeFence.stage>1)return 0;
            expected=cod1DeleteFreeFence.runtimeOwner;
            if(cod1DeleteFreeFence.stage==1){expected.dobj.present=0;memset(&expected.dobj.summary,0,sizeof(expected.dobj.summary));memset(&expected.assignedModel,0,sizeof(expected.assignedModel));memset(&expected.registeredAttachments,0,sizeof(expected.registeredAttachments));}
            if(memcmp(&cod1DeleteFreeFence.runtimeBody,&g_entities[entity->slot],sizeof(gentity_t))||
               memcmp(&expected,&cod1Actors[cod1DeleteFreeFence.lease.actorIndex],sizeof(expected)))return 0;
        }
        actor=cod1Actors[cod1DeleteFreeFence.lease.actorIndex];
    } else if(!EntityDeleteEligible(entity))return 0;
    before=g_entities[entity->slot];cod1DeleteFreeFence.checking=1;
    /* Recheck the Session/map transaction after each engine callback, while
     * the original component still exists. Expected death-canceled tasks do
     * not invalidate this lease; true Session abort/reentry does. */
    live=cod1DeleteFreeFence.api.current(cod1DeleteFreeFence.api.context);
    cod1DeleteFreeFence.checking=0;
    if(live!=1||cod1EntityDeleteCancelled||G_Cod1EntityMapEpoch()!=entity->epoch||
       level.time!=cod1DeleteFreeFence.hostTime||level.spawning||level.restarted||
       !G_Cod1EntitySameAllocation(entity)||memcmp(&before,&g_entities[entity->slot],sizeof(before))||
       memcmp(cod1DeleteFreeFence.input,&cod1DeleteFreeFence.request,sizeof(cod1DeleteFreeFence.request))||
       memcmp(cod1DeleteFreeFence.services,&cod1DeleteFreeFence.api,sizeof(cod1DeleteFreeFence.api)))return 0;
    if(cod1DeleteFreeFence.isActor){
        result=DeleteTransactionOwnerCurrent(entity->epoch,&cod1DeleteFreeFence.owner)&&
            DeleteTransactionTopology(entity,&cod1DeleteFreeFence.lease)&&
            !memcmp(&actor,&cod1Actors[cod1DeleteFreeFence.lease.actorIndex],sizeof(actor));
        if(result&&cod1DeleteFreeFence.runtimeActor)cod1DeleteFreeFence.stage++;
        return result;
    }
    return EntityDeleteEligible(entity);
}
/* Pure range check shared by all host storage fences; no host callbacks or
 * global storage recursion. These transaction witnesses must remain immutable. */
static int DeleteStoragePrivate(const void *p,size_t n) {
    return RuntimeActorDeletePrivate(p,n)||
        ActorOverlap(p,n,cod1DeleteAttempts,sizeof(cod1DeleteAttempts))||
        ActorOverlap(p,n,&cod1DeleteOwnerHooks,sizeof(cod1DeleteOwnerHooks))||
        ActorOverlap(p,n,&cod1DeleteOwnerEpoch,sizeof(cod1DeleteOwnerEpoch))||
        ActorOverlap(p,n,&cod1DeleteFreeFence,sizeof(cod1DeleteFreeFence))||
        ActorOverlap(p,n,&cod1EntityDeleteBusy,sizeof(cod1EntityDeleteBusy))||
        ActorOverlap(p,n,&cod1EntityDeleteCancelled,sizeof(cod1EntityDeleteCancelled));
}
static int DeleteTransactionPrivate(const void *p,size_t n) {
    return (n&&(!p||n>UINTPTR_MAX-(uintptr_t)p))||G_Cod1EntityStorageProtected(p,n)||DeleteStoragePrivate(p,n)||ActorOverlap(p,n,g_entities,sizeof(g_entities))||ActorOverlap(p,n,&level,sizeof(level))||
        ActorOverlap(p,n,cod1Actors,sizeof(cod1Actors))||ActorOverlap(p,n,cod1Sentients,sizeof(cod1Sentients))||
        ActorOverlap(p,n,cod1EntityNativeStatic,sizeof(cod1EntityNativeStatic))||
        ActorOverlap(p,n,cod1AmmoCandidates,sizeof(cod1AmmoCandidates));
}

int G_Cod1EntitySelectActorDeleteOwner(uint64_t epoch,const cod1ActorDeleteOwnerHooks_t *hooks) {
    if(cod1EntityDeleteBusy){cod1EntityDeleteCancelled=1;return 0;}
    if(!hooks||DeleteTransactionPrivate(hooks,sizeof(*hooks))||
       !epoch||epoch!=G_Cod1EntityMapEpoch()||!hooks->candidate||!hooks->admit||!hooks->notified||
       !hooks->current||!hooks->commit||!hooks->fail)return 0;
    if(cod1DeleteOwnerEpoch==epoch)return !memcmp(hooks,&cod1DeleteOwnerHooks,sizeof(*hooks));
    cod1DeleteOwnerHooks=*hooks;cod1DeleteOwnerEpoch=epoch;return 1;
}
static int DeleteTransactionShape(const GscEntityRef *ref,const cod1ActorLease_t *lease) {
    const gentity_t *e;const cod1ActorRecord_t *a;int i;
    if(!ref||!lease||ref->slot<MAX_CLIENTS||ref->slot>=ENTITYNUM_MAX_NORMAL||
       !EntityValidate(NULL,ref)||!ActorRefEqual(ref,&lease->entity)||
       !ActorLeaseValid(lease)||AmmoCandidateOwned(ref))return 0;
    e=&g_entities[ref->slot];a=&cod1Actors[lease->actorIndex];
    if(e->neverFree||e->client||e->teamchain||e->teammaster||e->tagParent||e->tagName||
       (e->team&&e->team[0])||(e->spawnflags&1)||e->aiName||e->aiCharacter||e->aiAttributes||
       e->AIScript_AlertEntity||e->aiInactive||e->item||a->criticalBusy||cod1ActorCriticalBusy||
       a->initialized||!a->dataInitialized||!a->nativeDataInitialized)return 0;
    for(i=0;i<level.num_entities;++i)if(g_entities[i].inuse&&g_entities[i].tagParent==e)return 0;
    return 1;
}
static int DeleteTransactionTopology(const GscEntityRef *ref,const cod1ActorLease_t *lease) {
    return DeleteTransactionShape(ref,lease)&&ActorDObjStageValid(lease,0);
}
/* Pure identity/component postcondition, deliberately independent of the VM
 * caller (which death may already have canceled). At commit no replacement
 * generation is permitted. A committed tombstone later survives slot reuse. */
int G_Cod1EntityDeleteRetiredIdentity(const GscEntityRef *ref,const cod1ActorLease_t *lease) {
    uint32_t i;
    if(!ref||!ref->epoch||ref->epoch!=G_Cod1EntityMapEpoch()||!ref->generation||
       ref->slot<MAX_CLIENTS||ref->slot>=ENTITYNUM_MAX_NORMAL||
       cod1EntityGeneration[ref->slot]!=ref->generation||g_entities[ref->slot].inuse||
       cod1ActorByEntity[ref->slot]||cod1SentientByEntity[ref->slot]||AmmoCandidateOwned(ref)||
       ActorRefEqual(ref,&cod1EntityNativeStatic[ref->slot].owner))return 0;
    if(lease){
        if(!ActorRefEqual(ref,&lease->entity)||!lease->serial||lease->kind!=COD1_ACTOR_LEASE_ACTOR||
           lease->actorIndex>=COD1_ACTOR_CAPACITY||lease->sentientIndex>=COD1_SENTIENT_CAPACITY)return 0;
        for(i=0;i<COD1_ACTOR_CAPACITY;++i)if(cod1Actors[i].activation>=0&&
            (cod1Actors[i].serial==lease->serial||ActorRefEqual(ref,&cod1Actors[i].owner)))return 0;
        for(i=0;i<COD1_SENTIENT_CAPACITY;++i)if(cod1Sentients[i].activation>=0&&
            (cod1Sentients[i].serial==lease->serial||ActorRefEqual(ref,&cod1Sentients[i].owner)))return 0;
    }
    return 1;
}
typedef struct {
 const GscDeleteTransactionRequest*input;const GscDeleteTransactionServices*services;
 GscDeleteTransactionRequest request;GscDeleteTransactionServices api;
 cod1ActorDeleteOwnerHooks_t owner;cod1ActorDeleteReceipt_t receipt;int time;
} Cod1RuntimeDeleteCall;
static int DeleteRuntimeCurrent(void*opaque){Cod1RuntimeDeleteCall*c=opaque;
 return c&&!memcmp(c->input,&c->request,sizeof(c->request))&&!memcmp(c->services,&c->api,sizeof(c->api))&&
  DeleteTransactionOwnerCurrent(c->request.entity.epoch,&c->owner)&&
  EntityDeleteLease(c->request.entity.epoch,c->time,c->api.current,c->api.context)&&
  !memcmp(c->input,&c->request,sizeof(c->request))&&!memcmp(c->services,&c->api,sizeof(c->api))&&
  DeleteTransactionOwnerCurrent(c->request.entity.epoch,&c->owner)&&c->owner.current(&c->receipt);
}
static int DeleteTransactionInvoke(void *user,const GscDeleteTransactionRequest *input,
    const GscDeleteTransactionServices *services,char *error,size_t size) {
    GscDeleteTransactionRequest request;GscDeleteTransactionServices api;
    cod1ActorLease_t actor;cod1ActorDeleteReceipt_t receipt;
    cod1ActorRecord_t beforeActor;gentity_t before;Cod1DeleteAttempt *attempt;
    cod1ActorDeleteOwnerHooks_t owner;
    int isActor=0,runtimeActor=0,hostTime,admitted=0,ok=0;Cod1RuntimeDeleteCall runtimeCall;
    (void)user;memset(&actor,0,sizeof(actor));memset(&receipt,0,sizeof(receipt));
    if(cod1EntityDeleteBusy){cod1EntityDeleteCancelled=1;return 0;}
    if(!input||!services||DeleteTransactionPrivate(input,sizeof(*input))||
       DeleteTransactionPrivate(services,sizeof(*services))||DeleteTransactionPrivate(error,size)||
       ActorOverlap(input,sizeof(*input),services,sizeof(*services))||
       ActorOverlap(input,sizeof(*input),error,size)||ActorOverlap(services,sizeof(*services),error,size))return 0;
    request=*input;api=*services;
    memset(&owner,0,sizeof(owner));
    if(cod1DeleteOwnerEpoch==request.entity.epoch&&cod1DeleteOwnerEpoch==G_Cod1EntityMapEpoch())owner=cod1DeleteOwnerHooks;
    if(!api.current||!api.notify_death||request.now_ms<0||!EntityValidate(NULL,&request.entity)||
       request.entity.slot<MAX_CLIENTS||request.entity.slot>=ENTITYNUM_MAX_NORMAL)goto refuse;
    attempt=&cod1DeleteAttempts[request.entity.slot];
    if(ActorRefEqual(&attempt->entity,&request.entity)&&attempt->attempted)goto refuse;
    if(jumperRetireHooks.candidate(&request.entity,&actor))owner=jumperRetireHooks;
    if(!EntityDeleteEligible(&request.entity)) {
        if(!owner.candidate||!owner.candidate(&request.entity,&actor)||
           !(owner.candidate==jumperRetireHooks.candidate?(runtimeActor=RuntimeActorDeleteEntry(&request.entity,&actor)):
             (DeleteTransactionTopology(&request.entity,&actor)||(runtimeActor=RuntimeActorDeleteEntry(&request.entity,&actor)))))goto refuse;
        isActor=1;beforeActor=cod1Actors[actor.actorIndex];
    }
    before=g_entities[request.entity.slot];hostTime=level.time;
    cod1EntityDeleteBusy=1;cod1EntityDeleteCancelled=0;
    if(!EntityDeleteLease(request.entity.epoch,hostTime,api.current,api.context)||
       memcmp(input,&request,sizeof(request))||memcmp(services,&api,sizeof(api))||
       memcmp(&before,&g_entities[request.entity.slot],sizeof(before)))goto done;
    if(isActor) {
        if(!DeleteTransactionOwnerCurrent(request.entity.epoch,&owner)||!(runtimeActor?RuntimeActorDeleteEntry(&request.entity,&actor):DeleteTransactionTopology(&request.entity,&actor))||
           memcmp(&beforeActor,&cod1Actors[actor.actorIndex],sizeof(beforeActor))||
           !owner.admit(&request.entity,&actor,&receipt))goto done;
        admitted=1;
    } else if(!EntityDeleteEligible(&request.entity))goto done;
    memset(attempt,0,sizeof(*attempt));attempt->entity=request.entity;attempt->attempted=1;
    if(isActor&&!owner.notified(&receipt))goto done;
    attempt->notified=1;
    if(api.notify_death(api.context,error,size)!=1||
       memcmp(input,&request,sizeof(request))||memcmp(services,&api,sizeof(api))||
       !EntityDeleteLease(request.entity.epoch,hostTime,api.current,api.context)||
       memcmp(&before,&g_entities[request.entity.slot],sizeof(before)))goto done;
    if(isActor) {
        if(!DeleteTransactionOwnerCurrent(request.entity.epoch,&owner)||!owner.current(&receipt)||!(runtimeActor?RuntimeActorDeleteEntry(&request.entity,&actor):DeleteTransactionTopology(&request.entity,&actor))||
           memcmp(&beforeActor,&cod1Actors[actor.actorIndex],sizeof(beforeActor)))goto done;
    } else if(!EntityDeleteEligible(&request.entity))goto done;
    if(runtimeActor){
        memset(&runtimeCall,0,sizeof(runtimeCall));runtimeCall.input=input;runtimeCall.services=services;
        runtimeCall.request=request;runtimeCall.api=api;runtimeCall.owner=owner;runtimeCall.receipt=receipt;runtimeCall.time=hostTime;
        if(!RuntimeActorDeleteQuiesce(&request.entity,&actor,DeleteRuntimeCurrent,&runtimeCall)||
           !DeleteTransactionTopology(&request.entity,&actor))goto done;
    }
    /* Final pure admission to actual production free. No host callback here. */
    cod1DeleteFreeFence.active=1;cod1DeleteFreeFence.isActor=isActor;
    cod1DeleteFreeFence.hostTime=hostTime;cod1DeleteFreeFence.entity=request.entity;cod1DeleteFreeFence.lease=actor;
    cod1DeleteFreeFence.input=input;cod1DeleteFreeFence.services=services;
    cod1DeleteFreeFence.request=request;cod1DeleteFreeFence.api=api;
    cod1DeleteFreeFence.owner=owner;
    if(runtimeActor){
        cod1DeleteFreeFence.runtimeActor=1;cod1DeleteFreeFence.stage=0;
        cod1DeleteFreeFence.runtimeBody=g_entities[request.entity.slot];cod1DeleteFreeFence.runtimeBody.r.linked=qfalse;
        cod1DeleteFreeFence.runtimeOwner=cod1Actors[actor.actorIndex];
    }
    G_FreeEntity(&g_entities[request.entity.slot]);
    memset(&cod1DeleteFreeFence,0,sizeof(cod1DeleteFreeFence));
    if(cod1EntityDeleteCancelled||G_Cod1EntityMapEpoch()!=request.entity.epoch||
       level.time!=hostTime||!G_Cod1EntityDeleteRetiredIdentity(&request.entity,isActor?&actor:NULL))goto done;
    if(isActor&&(!DeleteTransactionOwnerCurrent(request.entity.epoch,&owner)||!owner.commit(&receipt)))goto done;
    attempt->retired=1;
    /* The tombstone now precedes the first copied-map-certificate callback. */
    if(!EntityDeleteLease(request.entity.epoch,hostTime,api.current,api.context)||
       (isActor&&!DeleteTransactionOwnerCurrent(request.entity.epoch,&owner))||
       memcmp(input,&request,sizeof(request))||memcmp(services,&api,sizeof(api)))goto done;
    ok=1;
done:
    memset(&cod1DeleteFreeFence,0,sizeof(cod1DeleteFreeFence));
    if(admitted&&!ok)owner.fail(&receipt);
    cod1EntityDeleteBusy=0;
    if(ok){
        if(runtimeActor>=2 && g_debugMove.integer)G_Printf("[actor retired] owner=%u/%u tick=%d goal=%llu motion=%llu\n",
            request.entity.slot,request.entity.generation,hostTime,(unsigned long long)beforeActor.goal.assignment,
            (unsigned long long)beforeActor.motion.assignment);
        return 1;
    }
refuse:
    if(g_debugMove.integer)G_Printf("[delete refused] owner=%u/%u tick=%d actor=%d admitted=%d lane=%d\n",
        request.entity.slot,request.entity.generation,level.time,isActor,admitted,runtimeActor);
    EntityError(error,size,"R204 delete requires a fresh admitted ordinary entity or exact completed hidden actor prefix owner; death/free failure is terminal");return 0;
}
const GscDeleteTransactionHost *G_Cod1EntityDeleteTransactionHost(void) {
    static const GscDeleteTransactionHost host={NULL,DeleteTransactionInvoke};return &host;
}
