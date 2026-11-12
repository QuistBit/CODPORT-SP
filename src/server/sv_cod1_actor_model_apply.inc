/* R161 registered base -> private candidate -> native area relink -> topology.
 * SV/CM link and the final identity reads are synchronous and never call GAME.
 * FS/zone callbacks during candidate creation cannot publish a partial model. */
static int CAM_Reference(const cod1ModelResourceReference_t *reference,
    uint32_t index,const char *name,const char *modelPath) {
    cod1ModelResourceReference_t request, result;
    char text[MAX_STRING_CHARS], path[MAX_QPATH];
    request=*reference; request.operation=COD1_MODEL_RESOURCE_VALIDATE;
    if (SV_Cod1ModelResourceReference(&request,&result)!=1 ||
        memcmp(&request,&result,sizeof(request))) return 0;
    memset(text,0xa5,sizeof(text));
    SV_GetConfigstring(CS_MODELS+(int)index,text,sizeof(text));
    if (!memchr(text,0,sizeof(text)) || strcmp(text,name)) return 0;
    CP_FSPath(path,name); memcpy(path,"xmodel/",7);
    return (!modelPath || !strcmp(path,modelPath)) && !Q_stricmp(reference->name,name);
}
static int CAM_Registration(const cod1ActorModelApply_t *input) {
    return !input->modelIndex || CAM_Reference(&input->base,input->modelIndex,
        input->nativeName,input->build.models[0].model);
}
static int CAM_Attachments(const cod1ActorModelApply_t *input,
    const cod1ActorRegisteredModelInput_t *attachments,uint32_t count) {
    uint32_t i;
    for(i=0;i<count;++i)
        if(!CAM_Reference(&attachments[i].resource,attachments[i].modelIndex,
            attachments[i].nativeName,input->modelIndex?input->build.models[i+1].model:NULL))return 0;
    return 1;
}
static int CAM_AcquireRegistered(const cod1SpatialActorKey_t *key,
    const cod1ModelResourceReference_t *reference,uint32_t index,
    const char *name,const char *path,
    sharedEntity_t *entity,uint64_t mutation,cod1DObjResourceRef_t *out,
    const c1mrResource_t **resourceOut) {
    cod1DObjResourceEntry_t *entry;
    c1mrResource_t *resource;
    int i,freeSlot=-1;
    if (!CD_TransactionValid(key,entity,mutation) || !CAM_Reference(reference,index,name,path))
        return COD1_DOBJ_STALE;
    for (i=0;i<CD_RESOURCE_MAX;++i) {
        entry=&cod1DObjResources[i];
        if (!entry->references) {if(freeSlot<0)freeSlot=i;continue;}
        if (!memcmp(&entry->registeredBase,reference,sizeof(*reference)) &&
            !strcmp(entry->path,path)) {
            if (entry->references==UINT_MAX) return COD1_DOBJ_OVERFLOW;
            ++entry->references; out->slot=i; out->identity=entry->identity;
            *resourceOut=entry->resource; return COD1_DOBJ_OK;
        }
    }
    if (freeSlot<0 || cod1DObjResourceIdentity==UINT64_MAX) return COD1_DOBJ_OVERFLOW;
    resource=(c1mrResource_t *)Z_Malloc((int)sizeof(*resource));
    if (!resource) return COD1_DOBJ_UNAVAILABLE;
    if (!CD_TransactionValid(key,entity,mutation) ||
        SV_Cod1CopyRegisteredModel(reference,resource)!=1 ||
        !CAM_Reference(reference,index,name,path) ||
        !CD_TransactionValid(key,entity,mutation)) {Z_Free(resource);return COD1_DOBJ_STALE;}
    /* Resource content is the admitted decoded bank object; the DObj path is
     * canonical VFS spelling while nativeName keeps exact CS spelling. */
    strcpy(resource->modelName,path);
    entry=&cod1DObjResources[freeSlot]; memset(entry,0,sizeof(*entry));
    entry->identity=++cod1DObjResourceIdentity; entry->references=1;
    entry->resource=resource; entry->registeredBase=*reference;
    strcpy(entry->path,path);
    out->slot=freeSlot;out->identity=entry->identity;*resourceOut=resource;
    return COD1_DOBJ_OK;
}
/* A rebuilt/freed topology cannot keep a previously linked DObj collision
 * lane current. Query eligibility is stricter than cleanup association. */
static int CS_ModelLaneCurrent(int slot) {
    const cod1SpatialActorState_t *state=&cod1SpatialActors[slot];
    const cod1DObjInstance_t *object=cod1DObjInstances[slot];
    if (!state->hasDObj) return object==NULL;
    return object && CS_KeyEqual(&object->key,&state->key) &&
        object->serial==state->topologySerial && CD_Associated(object) &&
        CD_QueryResourcesValid(object);
}
/* Pure current-model bounds for a keyed motion relink. No topology rebuild. */
static int CS_LinkModelArea(int slot,cod1SpatialActorState_t *state){
 c1mBounds_t bounds;int i;const cod1DObjInstance_t*object=cod1DObjInstances[slot];
 if(!object||!CS_KeyEqual(&object->key,&state->key)||!CS_ModelLaneCurrent(slot)||!C1DT_FirstModelBounds(&object->topology,&bounds))return 0;
 for(i=0;i<3;i++){state->areaMins[i]=state->origin[i]+state->mins[i]-1;state->areaMaxs[i]=state->origin[i]+state->maxs[i]+1;}
 if(state->flags&UINT32_C(0x18))for(i=0;i<2;i++){
  if(state->flags&COD1_SPATIAL_NATIVE_DOBJ_FLAG){state->areaMins[i]=state->origin[i]-64;state->areaMaxs[i]=state->origin[i]+64;}
  else{state->areaMins[i]=state->origin[i]+bounds.mins[i];state->areaMaxs[i]=state->origin[i]+bounds.maxs[i];}
 }
 return CS_Bounds(state->areaMins,state->areaMaxs);
}
static int CAM_LinkReady(const cod1ActorModelApply_t *input,sharedEntity_t *entity,
    const cod1DObjSummary_t *summary,cod1SpatialActorState_t *state) {
    const cod1SpatialActorState_t *old=&cod1SpatialActors[input->build.key.slot];
    int i,lastLeaf,leafs[MAX_TOTAL_ENT_LEAFS];
    vec3_t low,high;
    if (!sv_numworldSectors || !CM_HasV59() || !old->active ||
        !CS_KeyEqual(&old->key,&input->build.key) ||
        !CS_StateAssociated((int)input->build.key.slot,entity) ||
        !CS_StateValid((int)input->build.key.slot,entity) ||
        old->contents!=input->nativeContents || old->flags!=input->nativeServerFlags ||
        !CD_OwnedHull((int)input->build.key.slot,entity) ||
        !CS_Finite(entity->r.currentOrigin) || !CS_Finite(entity->r.currentAngles) ||
        !CS_Bounds(entity->r.mins,entity->r.maxs)) return COD1_DOBJ_STALE;
    *state=*old; state->hasDObj=input->modelIndex!=0;
    for(i=0;i<3;++i) {
        double a=(double)entity->r.currentOrigin[i]+entity->r.mins[i]-1.0;
        double b=(double)entity->r.currentOrigin[i]+entity->r.maxs[i]+1.0;
        if (a< -FLT_MAX || b>FLT_MAX) return COD1_DOBJ_INVALID;
        low[i]=state->areaMins[i]=(float)a; high[i]=state->areaMaxs[i]=(float)b;
    }
    if (input->modelIndex && (input->nativeServerFlags & UINT32_C(0x18))) {
        for(i=0;i<2;++i) {
            double a,b;
            if (input->nativeServerFlags & COD1_SPATIAL_NATIVE_DOBJ_FLAG) {
                a=(double)entity->r.currentOrigin[i]-64.0;
                b=(double)entity->r.currentOrigin[i]+64.0;
            } else {
                a=(double)entity->r.currentOrigin[i]+summary->firstModelBounds.mins[i];
                b=(double)entity->r.currentOrigin[i]+summary->firstModelBounds.maxs[i];
            }
            if (a< -FLT_MAX || b>FLT_MAX || a>b) return COD1_DOBJ_INVALID;
            state->areaMins[i]=(float)a;state->areaMaxs[i]=(float)b;
        }
    }
    /* Preflight the unchanged hull/PVS before a destructive ordinary unlink.
     * No FS, allocator or GAME callback follows this point before publish. */
    if (!CM_BoxLeafnums(low,high,leafs,MAX_TOTAL_ENT_LEAFS,&lastLeaf))
        return COD1_DOBJ_UNAVAILABLE;
    return COD1_DOBJ_OK;
}
static int CAM_Apply(const cod1ActorModelApply_t *request,
    const cod1ActorRegisteredModelInput_t *attachments,uint32_t attachmentCount,
    int relink,cod1ActorModelApplyResult_t *out) {
    cod1ActorModelApply_t input;
    cod1ActorModelApplyResult_t result;
    cod1DObjInstance_t *candidate=NULL,*old;
    cod1DObjResourceRef_t acquired[COD1_DOBJ_MODELS_MAX];
    c1dtModelInput_t models[COD1_DOBJ_MODELS_MAX];
    cod1SpatialActorState_t link;
    cod1ModelResourceReference_t empty;
    sharedEntity_t *entity;
    uint64_t mutation;
    unsigned int i,accepted;
    uintptr_t a=(uintptr_t)request,b=(uintptr_t)out;
    int status,topologyStatus;
    if (!request || !out || (a<=b ? b-a<sizeof(*request) : a-b<sizeof(*out)))
        return COD1_DOBJ_INVALID;
    input=*request;memset(&empty,0,sizeof(empty));
    if (input.version!=COD1_ACTOR_MODEL_APPLY_VERSION || input.bytes!=sizeof(input) ||
        input.reserved || input.modelIndex>=MAX_MODELS ||
        input.nativeContents!=COD1_SPATIAL_ACTOR_CONTENTS ||
        input.build.version!=COD1_DOBJ_VERSION || input.build.bytes!=sizeof(input.build) ||
        !CS_KeyValid(&input.build.key) || !CD_String(input.nativeName,sizeof(input.nativeName)) ||
        input.build.animationTree) return COD1_DOBJ_INVALID;
    if (input.modelIndex) {
        if (!Cod1PrecacheName(input.nativeName,1) || !input.build.modelCount ||
            input.build.modelCount>COD1_DOBJ_MODELS_MAX || input.build.models[0].tag[0] ||
            input.base.operation!=COD1_MODEL_RESOURCE_QUERY) return COD1_DOBJ_INVALID;
        for(i=0;i<input.build.modelCount;++i)
            if (!CD_ModelPath(input.build.models[i].model) ||
                !CD_String(input.build.models[i].tag,COD1_DOBJ_NAME_MAX)) return COD1_DOBJ_INVALID;
    } else if (input.nativeName[0] || input.build.modelCount ||
               memcmp(&input.base,&empty,sizeof(empty))) return COD1_DOBJ_INVALID;
    if (cod1DObjBuildBusy) return COD1_DOBJ_UNAVAILABLE;
    if (cod1DObjMutation==UINT64_MAX || cod1DObjTopologySerial==UINT64_MAX)
        return COD1_DOBJ_OVERFLOW;
    status=CD_Eligible(&input.build.key,&entity);
    if(status!=COD1_DOBJ_OK)return status;
    old=cod1DObjInstances[input.build.key.slot];
    if ((old?old->serial:0)!=input.previousTopologySerial) return COD1_DOBJ_STALE;
    if (!CAM_Registration(&input) || !CAM_Attachments(&input,attachments,attachmentCount))
        return COD1_DOBJ_STALE;
    /* Native attachment operations never link42. Their base resource/path
     * must already be the current first model; an equal filename alone is
     * insufficient after a resource generation reset. */
    if(!relink) {
        if((old!=NULL)!=(input.modelIndex!=0) || !CS_ModelLaneCurrent((int)input.build.key.slot))
            return COD1_DOBJ_STALE;
        if(old) {
            const cod1DObjResourceEntry_t *base=&cod1DObjResources[old->resources[0].slot];
            if(!old->resources[0].identity || base->identity!=old->resources[0].identity ||
               memcmp(&base->registeredBase,&input.base,sizeof(input.base)) ||
               strcmp(old->authored[0].model,input.build.models[0].model))return COD1_DOBJ_STALE;
        }
    }
    memset(&result,0,sizeof(result));result.version=COD1_ACTOR_MODEL_APPLY_VERSION;
    result.bytes=sizeof(result);result.linked=1;
    memset(acquired,0,sizeof(acquired));memset(models,0,sizeof(models));
    cod1DObjBuildBusy=1;cod1DObjBuildCancelled=0;cod1DObjPendingKey=input.build.key;
    mutation=cod1DObjMutation;
    if(input.modelIndex) {
        candidate=(cod1DObjInstance_t *)Z_Malloc((int)sizeof(*candidate));
        if(!candidate){status=COD1_DOBJ_UNAVAILABLE;goto finished;}
        memset(candidate,0,sizeof(*candidate));candidate->key=input.build.key;
        candidate->entity=entity;candidate->authoredCount=input.build.modelCount;
        memcpy(candidate->authored,input.build.models,input.build.modelCount*sizeof(input.build.models[0]));
        for(i=0;i<input.build.modelCount;++i) {
            if(i==0)status=CAM_AcquireRegistered(&input.build.key,&input.base,input.modelIndex,
                input.nativeName,input.build.models[0].model,entity,mutation,&acquired[i],&models[i].resource);
            else if(attachments)status=CAM_AcquireRegistered(&input.build.key,&attachments[i-1].resource,
                attachments[i-1].modelIndex,attachments[i-1].nativeName,input.build.models[i].model,
                entity,mutation,&acquired[i],&models[i].resource);
            else status=CD_AcquireResource(input.build.models[i].model,&input.build.key,entity,mutation,
                &acquired[i],&models[i].resource);
            if(status!=COD1_DOBJ_OK)goto finished;
            models[i].tagName=input.build.models[i].tag;
        }
        topologyStatus=C1DT_Build(models,input.build.modelCount,&candidate->topology);
        if(topologyStatus!=C1DT_OK) {
            status=topologyStatus==C1DT_CAPACITY?COD1_DOBJ_OVERFLOW:COD1_DOBJ_INVALID;goto finished;
        }
        result.topology.version=COD1_DOBJ_VERSION;result.topology.bytes=sizeof(result.topology);
        result.topology.modelCount=candidate->topology.modelCount;
        result.topology.boneCount=candidate->topology.boneCount;
        result.topology.duplicateCount=candidate->topology.duplicateCount;
        result.topology.classificationModelsMask=candidate->topology.classificationModelMask;
        for(i=0;i<input.build.modelCount;++i)
            if(candidate->topology.missingTagInputMask&(1u<<i))++result.topology.skippedCount;
        if(!C1DT_FirstModelBounds(&candidate->topology,&result.topology.firstModelBounds)) {
            status=COD1_DOBJ_INVALID;goto finished;
        }
    }
    if (!CD_TransactionValid(&input.build.key,entity,mutation) || !CAM_Registration(&input) ||
        !CAM_Attachments(&input,attachments,attachmentCount)) {
        status=COD1_DOBJ_STALE;goto finished;
    }
    status=CAM_LinkReady(&input,entity,&result.topology,&link);
    if(status!=COD1_DOBJ_OK)goto finished;
    if(!relink) {
        const cod1SpatialActorState_t *saved=&cod1SpatialActors[input.build.key.slot];
        if(saved->hasDObj!=link.hasDObj || !VectorCompare(saved->areaMins,link.areaMins) ||
           !VectorCompare(saved->areaMaxs,link.areaMaxs)) {status=COD1_DOBJ_STALE;goto finished;}
        /* Preserve the entire old area/PVS/link lane. Only its topology
         * revision advances after publication below. */
        link=*saved;
    } else SV_LinkEntityArea(entity,link.areaMins,link.areaMaxs);
    /* Link preflight and actual link are pure SV/CM calls in this transaction.
     * Link counters/PVS diagnostics are engine observations, not GAME fields. */
    if(!entity->r.linked) {status=COD1_DOBJ_UNAVAILABLE;goto finished;}
    if(candidate) {
        for(i=0;i<candidate->topology.modelCount;++i) {
            accepted=candidate->topology.models[i].inputIndex;
            candidate->resources[i]=acquired[accepted];memset(&acquired[accepted],0,sizeof(acquired[accepted]));
        }
        candidate->serial=++cod1DObjTopologySerial;
        result.topology.topologySerial=link.topologySerial=candidate->serial;
    } else link.topologySerial=0;
    cod1DObjInstances[input.build.key.slot]=candidate;candidate=NULL;
    cod1DObjEpoch=input.build.key.epoch;CD_Mutated();CD_DisposeInstance(old);
    if(!input.modelIndex) {
        for(i=0;i<MAX_GENTITIES;++i)if(cod1DObjInstances[i])break;
        if(i==MAX_GENTITIES)cod1DObjEpoch=0;
    }
    if(!cod1SpatialActors[input.build.key.slot].active)++cod1SpatialActive;
    cod1SpatialActors[input.build.key.slot]=link;cod1SpatialEpoch=input.build.key.epoch;
    *out=result;status=COD1_DOBJ_OK;
finished:
    for(i=0;i<COD1_DOBJ_MODELS_MAX;++i)CD_ReleaseResource(&acquired[i]);
    CD_DisposeInstance(candidate);cod1DObjBuildBusy=cod1DObjBuildCancelled=0;
    memset(&cod1DObjPendingKey,0,sizeof(cod1DObjPendingKey));return status;
}
int SV_Cod1ActorModelApply(const cod1ActorModelApply_t *request,
    cod1ActorModelApplyResult_t *out) {
    return CAM_Apply(request,NULL,0,1,out);
}
int SV_Cod1ActorRegisteredModelsApply(const cod1ActorRegisteredModelsApply_t *request,
    cod1ActorModelApplyResult_t *out) {
    cod1ActorRegisteredModelsApply_t input;
    cod1ActorRegisteredModelInput_t zero;
    uint32_t i;
    uintptr_t a=(uintptr_t)request,b=(uintptr_t)out;
    if(!request || !out || (a<=b?b-a<sizeof(*request):a-b<sizeof(*out)))return COD1_DOBJ_INVALID;
    input=*request;memset(&zero,0,sizeof(zero));
    if(input.version!=COD1_ACTOR_REGISTERED_MODELS_VERSION || input.bytes!=sizeof(input) ||
       input.relink>1 || input.attachmentCount>COD1_DOBJ_MODELS_MAX-1 ||
       (input.model.modelIndex && input.model.build.modelCount!=input.attachmentCount+1))return COD1_DOBJ_INVALID;
    for(i=0;i<COD1_DOBJ_MODELS_MAX-1;++i) {
        const cod1ActorRegisteredModelInput_t *r=&input.attachments[i];
        if(i>=input.attachmentCount) {if(memcmp(r,&zero,sizeof(zero)))return COD1_DOBJ_INVALID;}
        else if(!r->modelIndex || r->modelIndex>=MAX_MODELS || r->reserved ||
            r->resource.operation!=COD1_MODEL_RESOURCE_QUERY ||
            !CD_String(r->nativeName,sizeof(r->nativeName)) ||
            !Cod1PrecacheName(r->nativeName,1))return COD1_DOBJ_INVALID;
    }
    return CAM_Apply(&input.model,input.attachments,input.attachmentCount,(int)input.relink,out);
}
