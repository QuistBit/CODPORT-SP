/* R161 private native base-byte assignment. No shared modelindex, AI identity,
 * initialized/spawned flag, renderer pose or native GSC method is published. */
static int ActorApplyRegistration(const cod1ActorModelRegistration_t *model,int spawning,int busy) {
    const cod1ActorRegisteredModel_t *saved;
    char text[MAX_STRING_CHARS];
    if(!model->modelIndex || model->modelIndex>=MAX_MODELS || model->reserved ||
       !memchr(model->nativeName,0,sizeof(model->nativeName)) ||
       !Cod1PrecacheName(model->nativeName,1))return 0;
    saved=&cod1ActorRegisteredModels[model->modelIndex];
    return saved->epoch==model->lease.entity.epoch &&
        !strcmp(saved->name,model->nativeName) &&
        !memcmp(&saved->resource,&model->resource,sizeof(model->resource)) &&
        ActorModelReadCSBusy(&model->lease,spawning,busy,model->modelIndex,text) &&
        !strcmp(text,model->nativeName) && ActorModelValidateResource(&model->resource) &&
        ActorModelStageBusy(&model->lease,spawning,busy);
}
static int ActorApplyStage(const cod1ActorLease_t *lease,int spawning,int busy) {
    return ActorModelStageBusy(lease,spawning,busy) &&
        cod1Actors[lease->actorIndex].spatial.linked &&
        g_entities[lease->entity.slot].r.linked;
}
static void ActorApplyPath(char *out,const char *name) {
    unsigned i;
    for(i=0;name[i];++i)out[i]=name[i]=='\\'?'/':name[i];
    out[i]=0;memcpy(out,"xmodel/",7);
}
static int ActorApplyAttachments(const cod1ActorLease_t *lease,
    const cod1ActorRegisteredAttachments_t *attachments,
    const cod1ActorModelComposition_t *composition,int spawning,int busy) {
    unsigned i;char path[COD1_DOBJ_NAME_MAX];
    if(attachments->count>COD1_DOBJ_MODELS_MAX-1 ||
       (attachments->count && attachments->count+1!=composition->descriptorCount))return 0;
    for(i=0;i<attachments->count;++i) {
        const cod1ActorModelRegistration_t *model=&attachments->models[i];
        if(memcmp(&model->lease,lease,sizeof(*lease)) || !ActorApplyRegistration(model,spawning,busy))return 0;
        ActorApplyPath(path,model->nativeName);
        if(strcmp(path,composition->descriptors[i+1].model))return 0;
    }
    return 1;
}
static int ActorApplyModel(const cod1ActorModelRegistration_t *model,
    const cod1ActorModelComposition_t *edited,
    const cod1ActorRegisteredAttachments_t *editedAttachments,int relink,
    cod1ActorAssignedModel_t *out,char *error,size_t size,int methodOwned) {
    cod1ActorLease_t lease=model->lease;
    cod1ActorModelApply_t request;
    cod1ActorModelApplyResult_t result;
    cod1ActorRegisteredModelsApply_t registered;
    cod1ActorRegisteredAttachments_t attachments;
    cod1DObjSummary_t emptySummary;
    cod1ActorModelComposition_t composition;
    cod1ActorAssignedModel_t assigned;
    cod1ActorNativeData_t native;
    entityState_t pose;
    vec3_t origin,angles,mins,maxs;
    cod1ActorRecord_t *a;
    gentity_t *e;
    uint64_t previousSerial;
    unsigned int i;
    int spawning,status,accepted=0,canceled,unlinked;
    if(!ActorModelBeginOwned(&lease,&spawning,methodOwned))goto refused;
    if(!ActorApplyStage(&lease,spawning,0) ||
       (model->modelIndex && !ActorApplyRegistration(model,spawning,0)))goto done;
    a=&cod1Actors[lease.actorIndex];e=&g_entities[lease.entity.slot];
    composition=edited?*edited:a->modelComposition;
    attachments=editedAttachments?*editedAttachments:a->registeredAttachments;
    if(composition.descriptorCount>COD1_DOBJ_MODELS_MAX)goto done;
    if(!composition.descriptorCount)composition.descriptorCount=1;
    if(!ActorApplyAttachments(&lease,&attachments,&composition,spawning,0))goto done;
    memset(&composition.descriptors[0],0,sizeof(composition.descriptors[0]));
    memset(&request,0,sizeof(request));memset(&result,0,sizeof(result));
    memset(&emptySummary,0,sizeof(emptySummary));
    request.version=COD1_ACTOR_MODEL_APPLY_VERSION;request.bytes=sizeof(request);
    request.build.version=COD1_DOBJ_VERSION;request.build.bytes=sizeof(request.build);
    request.build.key=ActorSpatialKey(&lease);
    if(model->modelIndex) {
        ActorApplyPath(composition.descriptors[0].model,model->nativeName);
        request.build.modelCount=composition.descriptorCount;
        memcpy(request.build.models,composition.descriptors,
               composition.descriptorCount*sizeof(composition.descriptors[0]));
        request.base=model->resource;request.modelIndex=model->modelIndex;
        strcpy(request.nativeName,model->nativeName);
    }
    previousSerial=request.previousTopologySerial=a->dobj.summary.topologySerial;
    request.nativeContents=a->nativeData.entity.nativeContents;
    request.nativeServerFlags=a->nativeData.entity.nativeServerFlags;
    native=a->nativeData;pose=e->s;
    VectorCopy(e->r.currentOrigin,origin);VectorCopy(e->r.currentAngles,angles);
    VectorCopy(e->r.mins,mins);VectorCopy(e->r.maxs,maxs);
    a->dobj.busy=1;
    if(attachments.count || !relink) {
        memset(&registered,0,sizeof(registered));registered.version=COD1_ACTOR_REGISTERED_MODELS_VERSION;
        registered.bytes=sizeof(registered);registered.relink=relink;registered.attachmentCount=attachments.count;
        registered.model=request;
        for(i=0;i<attachments.count;++i) {
            registered.attachments[i].modelIndex=attachments.models[i].modelIndex;
            registered.attachments[i].resource=attachments.models[i].resource;
            strcpy(registered.attachments[i].nativeName,attachments.models[i].nativeName);
        }
        status=trap_Cod1ActorRegisteredModelsApply(&registered,&result);
    } else status=trap_Cod1ActorModelApply(&request,&result);
    if(status==COD1_DOBJ_OK && ActorApplyStage(&lease,spawning,1) &&
       result.version==COD1_ACTOR_MODEL_APPLY_VERSION && result.bytes==sizeof(result) &&
       result.linked==1 && !result.reserved &&
       (model->modelIndex ? (ActorDObjSummaryValid(&result.topology,composition.descriptorCount) &&
                            result.topology.topologySerial>previousSerial &&
                            ActorApplyRegistration(model,spawning,1)) :
                           (!memcmp(&result.topology,&emptySummary,sizeof(emptySummary)))) &&
       ActorApplyAttachments(&lease,&attachments,&composition,spawning,1) &&
       ActorApplyStage(&lease,spawning,1)) {
        a=&cod1Actors[lease.actorIndex];e=&g_entities[lease.entity.slot];
        if(!memcmp(&native,&a->nativeData,sizeof(native)) &&
           !memcmp(&pose,&e->s,sizeof(pose)) &&
           VectorCompare(origin,e->r.currentOrigin) && VectorCompare(angles,e->r.currentAngles) &&
           VectorCompare(mins,e->r.mins) && VectorCompare(maxs,e->r.maxs)) {
            memset(&assigned,0,sizeof(assigned));assigned.registration=*model;
            assigned.topology=result.topology;
            a->modelComposition=composition;a->dobj.busy=0;a->dobj.present=model->modelIndex!=0;
            a->dobj.summary=result.topology;a->assignedModel=assigned;
            a->registeredAttachments=attachments;
            *out=assigned;accepted=1;goto done;
        }
    }
    if(status==COD1_DOBJ_OK) {
        /* A success followed by changed GAME ownership is not ordinary
         * backend refusal. Cancel only that old key; never adopt a new owner. */
        canceled=ActorDObjCancel(&lease);unlinked=ActorSpatialCancel(&lease);
        if(ActorLeaseValid(&lease)) {
            a=&cod1Actors[lease.actorIndex];a->dobj.busy=0;
            a->dobj.present=canceled!=COD1_DOBJ_OK;
            memset(&a->dobj.summary,0,sizeof(a->dobj.summary));
            memset(&a->assignedModel,0,sizeof(a->assignedModel));
            memset(&a->registeredAttachments,0,sizeof(a->registeredAttachments));
            a->spatial.linked=unlinked!=COD1_SPATIAL_OK;
        } else if ((canceled!=COD1_DOBJ_OK && canceled!=COD1_DOBJ_STALE) ||
                   (unlinked!=COD1_SPATIAL_OK && unlinked!=COD1_SPATIAL_STALE))
            G_Error("R161: changed actor model owner could not be canceled by its exact old key");
    }
done:
    if(ActorLeaseValid(&lease))cod1Actors[lease.actorIndex].dobj.busy=0;
    ActorModelEnd(&lease);
refused:
    if(!accepted)EntityError(error,size,"registered actor model, native placement, topology or owner changed");
    return accepted;
}
int G_Cod1ActorApplyRegisteredModel(const cod1ActorModelRegistration_t *input,
    cod1ActorAssignedModel_t *out,char *error,size_t size) {
    cod1ActorModelRegistration_t model;
    if(!ActorArguments(input,sizeof(*input),out,sizeof(*out),error,size) ||
       ActorOverlap(input,sizeof(*input),out,sizeof(*out)))return 0;
    if(!input || !out){EntityError(error,size,"actor model assignment requires registration and output");return 0;}
    model=*input;
    if(!model.modelIndex)return 0;
    return ActorApplyModel(&model,NULL,NULL,1,out,error,size,0);
}
int G_Cod1ActorClearRegisteredModel(const cod1ActorLease_t *input,
    cod1ActorAssignedModel_t *out,char *error,size_t size) {
    cod1ActorModelRegistration_t model;
    if(!ActorArguments(input,sizeof(*input),out,sizeof(*out),error,size) ||
       ActorOverlap(input,sizeof(*input),out,sizeof(*out)))return 0;
    if(!input || !out){EntityError(error,size,"actor model clear requires lease and output");return 0;}
    memset(&model,0,sizeof(model));model.lease=*input;
    return ActorApplyModel(&model,NULL,NULL,1,out,error,size,0);
}
int G_Cod1ActorReadAssignedModel(const cod1ActorLease_t *input,
    cod1ActorAssignedModel_t *out,char *error,size_t size) {
    cod1ActorLease_t lease;
    if(!ActorArguments(input,sizeof(*input),out,sizeof(*out),error,size) ||
       ActorOverlap(input,sizeof(*input),out,sizeof(*out)))return 0;
    if(!input || !out)return 0;
    lease=*input;
    if(!ActorLeaseValid(&lease) || lease.kind!=COD1_ACTOR_LEASE_ACTOR)return 0;
    *out=cod1Actors[lease.actorIndex].assignedModel;return 1;
}
