/* R243: readonly model field. Occupied registrations use query ownership,
 * never constructor/model-mutator eligibility. The original constructor/zero
 * reader is retained separately; this does not admit new zero-model states. */
typedef struct {
    GscEntityRef ref;const GscEntityRef *input;
    cod1ActorLease_t lease;cod1ActorRecord_t actor;
    cod1SentientRecord_t sentient;gentity_t entity;
    cod1ActorRegisteredModel_t registered;
    unsigned char actorRegistry,sentientRegistry;
    uint64_t epoch;int hostTime,spawning,locked,cancelled;
    GscStringCastValue value;
} R243ActorModelReadFrame;
static R243ActorModelReadFrame *r243ActorModelReadActive;
static int ActorModelReadStoragePrivate(const void *p,size_t n) {
    return ActorOverlap(p,n,&r243ActorModelReadActive,sizeof(r243ActorModelReadActive)) ||
        (r243ActorModelReadActive && ActorOverlap(p,n,r243ActorModelReadActive,sizeof(*r243ActorModelReadActive)));
}
static int ActorModelReadPrivate(const void *p,size_t n) {
    return n && (!p || n>UINTPTR_MAX-(uintptr_t)p ||
        ActorModelReadStoragePrivate(p,n) || G_Cod1EntityStorageProtected(p,n));
}
static int ActorModelReadCurrent(const R243ActorModelReadFrame *f) {
    cod1ActorLease_t after;
    return f && r243ActorModelReadActive==f && !f->cancelled &&
        f->epoch==cod1EntityEpoch && ActorMapAvailable() && cod1EntityReady &&
        !level.restarted && level.time==f->hostTime && !!level.spawning==f->spawning &&
        !memcmp(f->input,&f->ref,sizeof(f->ref)) &&
        (f->locked?
            (ActorAttachmentStage(&f->lease,f->hostTime) &&
             !memcmp(&f->registered,&cod1ActorRegisteredModels[f->actor.assignedModel.registration.modelIndex],sizeof(f->registered))):
            (ActorPoseLease(&f->ref,&after) && !memcmp(&after,&f->lease,sizeof(after)) &&
             !cod1ActorModelRegistryBusyEpoch && ActorLeaseValid(&f->lease))) &&
        f->actorRegistry==cod1ActorByEntity[f->ref.slot] &&
        f->sentientRegistry==cod1SentientByEntity[f->ref.slot] &&
        !memcmp(&f->actor,&cod1Actors[f->lease.actorIndex],sizeof(f->actor)) &&
        !memcmp(&f->sentient,&cod1Sentients[f->lease.sentientIndex],sizeof(f->sentient)) &&
        !memcmp(&f->entity,&g_entities[f->ref.slot],sizeof(f->entity));
}
int G_Cod1ActorReadModelField(const GscEntityRef *input,GscStringCastValue *out,char *error,size_t size) {
    R243ActorModelReadFrame f;cod1ActorLease_t query;
    const cod1ActorModelRegistration_t *model;const cod1ActorRegisteredModel_t *saved;
    uint32_t index;char text[MAX_STRING_CHARS];int ok=0,diagnostic=0;
    if(!input || !out || ActorModelReadPrivate(input,sizeof(*input)) ||
       ActorModelReadPrivate(out,sizeof(*out)) || ActorModelReadPrivate(error,size) ||
       !ActorArguments(input,sizeof(*input),out,sizeof(*out),error,size) ||
       ActorOverlap(input,sizeof(*input),out,sizeof(*out)))return 0;
    if(r243ActorModelReadActive){r243ActorModelReadActive->cancelled=1;return 0;}
    memset(&f,0,sizeof(f));f.ref=*input;f.input=input;
    if(!ActorPoseLease(&f.ref,&f.lease))return 0;
    f.epoch=cod1EntityEpoch;f.hostTime=level.time;f.spawning=!!level.spawning;
    f.actor=cod1Actors[f.lease.actorIndex];f.sentient=cod1Sentients[f.lease.sentientIndex];
    f.entity=g_entities[f.ref.slot];f.actorRegistry=cod1ActorByEntity[f.ref.slot];
    f.sentientRegistry=cod1SentientByEntity[f.ref.slot];
    r243ActorModelReadActive=&f;
    model=&f.actor.assignedModel.registration;index=model->modelIndex;
    if(!index || !ActorAttachmentOwner(&f.ref,&query)) {
        /* Preserve the former constructor/zero domain and its real CS read.
         * No query fallback creates an absent registration or an empty name. */
        ok=ActorReadModelFieldConstructor(input,&f.value,error,size) && ActorModelReadCurrent(&f);
        goto done;
    }
    if(memcmp(&query,&f.lease,sizeof(query)) || index>=MAX_MODELS || model->reserved ||
       memcmp(&model->lease,&f.lease,sizeof(f.lease)) ||
       !memchr(model->nativeName,0,sizeof(model->nativeName)) ||
       !Cod1PrecacheName(model->nativeName,1))goto done;
    f.registered=cod1ActorRegisteredModels[index];
    cod1Actors[f.lease.actorIndex].modelBusy=1;
    cod1Actors[f.lease.actorIndex].modelMethodBusy=1;
    cod1ActorModelRegistryBusyEpoch=f.lease.entity.epoch;
    f.locked=1;f.actor.modelBusy=f.actor.modelMethodBusy=1;
    if(!ActorModelReadCurrent(&f))goto done;
    saved=&f.registered;
    if(saved->epoch!=f.lease.entity.epoch || strcmp(saved->name,model->nativeName) ||
       memcmp(&saved->resource,&model->resource,sizeof(model->resource)) ||
       !ActorModelValidateResource(&model->resource) || !ActorModelReadCurrent(&f))goto done;
    /* Qualified synchronous readonly ENGINE calls: no FS/allocator/renderer
     * or GAME callback. They do not acquire/pin the resource or change a model. */
    memset(text,0xa5,sizeof(text));trap_GetConfigstring(CS_MODELS+index,text,sizeof(text));
    if(!ActorModelReadCurrent(&f) || !memchr(text,0,sizeof(text)) ||
       strcmp(text,model->nativeName) || strlen(text)>=sizeof(f.value.string))goto done;
    memset(&f.value,0,sizeof(f.value));f.value.type=1;strcpy(f.value.string,text);
    ok=ActorModelValidateResource(&model->resource) && ActorModelReadCurrent(&f);
done:
    if(ActorModelReadPrivate(input,sizeof(*input)) || ActorModelReadPrivate(out,sizeof(*out)) ||
       ActorModelReadPrivate(error,size) ||
       !ActorArguments(input,sizeof(*input),out,sizeof(*out),error,size))ok=0;
    diagnostic=!ActorModelReadPrivate(error,size);
    if(f.locked) {
        ActorModelEnd(&f.lease);
        if(ActorLeaseValid(&f.lease))cod1Actors[f.lease.actorIndex].modelMethodBusy=0;
    }
    r243ActorModelReadActive=NULL;
    if(ok){*out=f.value;return 1;}
    if(diagnostic)EntityError(error,size,"model field registration, resource, map or query owner changed");
    return 0;
}
