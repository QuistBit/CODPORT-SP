/* R162 dense, nonempty registered operands after native lookup. Native
 * wrappers/tag IDs/error ordering remain separately proved pending adapters. */
static int ActorRegisteredEditOwned(const cod1ActorLease_t *lease,
    cod1ActorModelRegistration_t *base,cod1ActorModelComposition_t *composition,
    cod1ActorRegisteredAttachments_t *attachments,int methodOwned) {
    const cod1ActorRecord_t *a;
    if(!ActorModelTopologyStageValid(lease,0) || cod1Actors[lease->actorIndex].modelBusy ||
       cod1Actors[lease->actorIndex].modelMethodBusy!=methodOwned)return 0;
    a=&cod1Actors[lease->actorIndex];
    if(!a->spatial.linked || !g_entities[lease->entity.slot].r.linked)return 0;
    *base=a->assignedModel.registration;*composition=a->modelComposition;
    *attachments=a->registeredAttachments;
    if(!composition->descriptorCount)composition->descriptorCount=1;
    if(composition->descriptorCount>COD1_DOBJ_MODELS_MAX ||
       attachments->count+1!=composition->descriptorCount)return 0;
    if(!!composition->descriptors[0].model[0]!=!!base->modelIndex)return 0;
    if(base->modelIndex && memcmp(&base->lease,lease,sizeof(*lease)))return 0;
    if(!base->modelIndex){memset(base,0,sizeof(*base));base->lease=*lease;}
    return 1;
}
static int ActorRegisteredEdit(const cod1ActorLease_t *lease,
    cod1ActorModelRegistration_t *base,cod1ActorModelComposition_t *composition,
    cod1ActorRegisteredAttachments_t *attachments) {
    return ActorRegisteredEditOwned(lease,base,composition,attachments,0);
}
int G_Cod1ActorAppendRegisteredAttachment(const cod1ActorModelRegistration_t *input,
    const char tag[COD1_DOBJ_NAME_MAX],cod1ActorAssignedModel_t *out,char *error,size_t size) {
    cod1ActorModelRegistration_t model,base;
    cod1ActorModelComposition_t composition;
    cod1ActorRegisteredAttachments_t attachments;
    cod1DObjModel_t descriptor;
    size_t i;
    if(!ActorArguments(input,sizeof(*input),out,sizeof(*out),error,size) ||
       !ActorArguments(tag,COD1_DOBJ_NAME_MAX,out,sizeof(*out),error,size) ||
       ActorOverlap(input,sizeof(*input),tag,COD1_DOBJ_NAME_MAX) ||
       ActorOverlap(input,sizeof(*input),out,sizeof(*out)) ||
       ActorOverlap(tag,COD1_DOBJ_NAME_MAX,out,sizeof(*out)))return 0;
    if(!input || !tag || !out)return 0;
    model=*input;memset(&descriptor,0,sizeof(descriptor));
    for(i=0;i<COD1_DOBJ_NAME_MAX && tag[i];++i) {
        unsigned char c=(unsigned char)tag[i];
        if(c<32 || c>126)return 0;
        descriptor.tag[i]=(char)(c>='A' && c<='Z'?c+('a'-'A'):c);
    }
    if(i==COD1_DOBJ_NAME_MAX || !model.modelIndex ||
       !memchr(model.nativeName,0,sizeof(model.nativeName)) ||
       !Cod1PrecacheName(model.nativeName,1) ||
       !ActorRegisteredEdit(&model.lease,&base,&composition,&attachments) ||
       attachments.count==COD1_DOBJ_MODELS_MAX-1)return 0;
    ActorApplyPath(descriptor.model,model.nativeName);
    composition.descriptors[composition.descriptorCount++]=descriptor;
    attachments.models[attachments.count++]=model;
    return ActorApplyModel(&base,&composition,&attachments,0,out,error,size,0);
}
int G_Cod1ActorRemoveRegisteredAttachment(const cod1ActorLease_t *input,
    uint32_t descriptorIndex,cod1ActorAssignedModel_t *out,char *error,size_t size) {
    cod1ActorLease_t lease;
    cod1ActorModelRegistration_t base;
    cod1ActorModelComposition_t composition;
    cod1ActorRegisteredAttachments_t attachments;
    size_t tail;
    if(!ActorArguments(input,sizeof(*input),out,sizeof(*out),error,size) ||
       ActorOverlap(input,sizeof(*input),out,sizeof(*out)))return 0;
    if(!input || !out)return 0;
    lease=*input;
    if(!ActorRegisteredEdit(&lease,&base,&composition,&attachments) ||
       !descriptorIndex || descriptorIndex>=composition.descriptorCount)return 0;
    tail=composition.descriptorCount-descriptorIndex-1;
    memmove(&composition.descriptors[descriptorIndex],&composition.descriptors[descriptorIndex+1],
        tail*sizeof(composition.descriptors[0]));
    memmove(&attachments.models[descriptorIndex-1],&attachments.models[descriptorIndex],
        tail*sizeof(attachments.models[0]));
    --composition.descriptorCount;--attachments.count;
    memset(&composition.descriptors[composition.descriptorCount],0,sizeof(composition.descriptors[0]));
    memset(&attachments.models[attachments.count],0,sizeof(attachments.models[0]));
    return ActorApplyModel(&base,&composition,&attachments,0,out,error,size,0);
}
int G_Cod1ActorClearRegisteredAttachments(const cod1ActorLease_t *input,
    cod1ActorAssignedModel_t *out,char *error,size_t size) {
    cod1ActorLease_t lease;
    cod1ActorModelRegistration_t base;
    cod1ActorModelComposition_t composition;
    cod1ActorRegisteredAttachments_t attachments;
    if(!ActorArguments(input,sizeof(*input),out,sizeof(*out),error,size) ||
       ActorOverlap(input,sizeof(*input),out,sizeof(*out)))return 0;
    if(!input || !out)return 0;
    lease=*input;
    if(!ActorRegisteredEdit(&lease,&base,&composition,&attachments))return 0;
    composition.descriptorCount=1;
    memset(composition.descriptors+1,0,7*sizeof(composition.descriptors[0]));
    memset(&attachments,0,sizeof(attachments));
    return ActorApplyModel(&base,&composition,&attachments,0,out,error,size,0);
}
int G_Cod1ActorReadRegisteredAttachments(const cod1ActorLease_t *input,
    cod1ActorRegisteredAttachments_t *out,char *error,size_t size) {
    cod1ActorLease_t lease;
    if(!ActorArguments(input,sizeof(*input),out,sizeof(*out),error,size) ||
       ActorOverlap(input,sizeof(*input),out,sizeof(*out)))return 0;
    if(!input || !out)return 0;
    lease=*input;
    if(!ActorLeaseValid(&lease) || lease.kind!=COD1_ACTOR_LEASE_ACTOR)return 0;
    *out=cod1Actors[lease.actorIndex].registeredAttachments;return 1;
}
