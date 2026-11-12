/* R164 typed/copied getter -> checked model-method host stage. Native VM
 * name binding, pool allocation and stack/refcount cleanup remain separate. */
static int ActorMethodLegacyValue(const GscNativeValue *value,cod1ActorModelValue_t *out) {
    memset(out,0,sizeof(*out));
    switch(value->kind) {
    case GSC_NATIVE_UNDEFINED:out->type=0;break;
    case GSC_NATIVE_STRING:out->type=1;memcpy(out->string,value->string,sizeof(out->string));break;
    case GSC_NATIVE_NUMBER:
        if(value->integer!=0 && value->integer!=1)return 0;
        if(value->integer){out->type=5;if(!Cod1ModelLegacyInt(value->number,&out->integer))return 0;}
        else{out->type=4;out->number=value->number;}break;
    case GSC_NATIVE_VECTOR:out->type=3;memcpy(out->vector,value->vector,sizeof(out->vector));break;
    case GSC_NATIVE_ENTITY:case GSC_NATIVE_ARRAY:case GSC_NATIVE_STRUCT:out->type=7;break;
    default:return 0;
    }
    return 1;
}
static int ActorMethodCurrent(const cod1ActorLease_t *lease,int spawning) {
    return ActorModelTopologyStageValid(lease,0) && !level.restarted &&
        !!level.spawning==spawning && cod1Actors[lease->actorIndex].modelMethodBusy==1 &&
        !cod1Actors[lease->actorIndex].modelBusy &&
        cod1Actors[lease->actorIndex].spatial.linked && g_entities[lease->entity.slot].r.linked;
}
/* A native lower-find miss and a known-but-unattached tag both have no model
 * CS reads. Copied canonical strings represent equality only, never pool IDs
 * or the observable collision-chain reorder/refcount of the original pool. */
static int ActorMethodLookup(const cod1ActorLease_t *lease,int spawning,
    const cod1ActorModelComposition_t *composition,
    const cod1ActorRegisteredAttachments_t *attachments,const char *model,
    const char *tag,uint32_t *found,char *error,size_t size) {
    uint32_t i;int phase,ok=1;
    *found=0;
    if(!ActorModelBeginOwned(lease,&phase,1))return 0;
    if(phase!=spawning){ActorModelEnd(lease);return 0;}
    for(i=0;i<attachments->count;++i) {
        if(strcmp(composition->descriptors[i+1].tag,tag))continue;
        if(!ActorApplyRegistration(&attachments->models[i],spawning,0)){ok=0;break;}
        if(!Q_stricmp(attachments->models[i].nativeName,model)){*found=i+1;break;}
    }
    ActorModelEnd(lease);
    if(!ok)EntityError(error,size,"actor attachment lookup CS/resource changed");
    return ok && ActorMethodCurrent(lease,spawning);
}
static void ActorMethodRemove(cod1ActorModelComposition_t *composition,
    cod1ActorRegisteredAttachments_t *attachments,uint32_t index) {
    size_t tail=composition->descriptorCount-index-1;
    memmove(&composition->descriptors[index],&composition->descriptors[index+1],tail*sizeof(composition->descriptors[0]));
    memmove(&attachments->models[index-1],&attachments->models[index],tail*sizeof(attachments->models[0]));
    --composition->descriptorCount;--attachments->count;
    memset(&composition->descriptors[composition->descriptorCount],0,sizeof(composition->descriptors[0]));
    memset(&attachments->models[attachments->count],0,sizeof(attachments->models[0]));
}
static int ActorMethodDiagnostics(const cod1ActorLease_t *lease,int spawning,
    const cod1ActorModelComposition_t *composition,const cod1ActorRegisteredAttachments_t *attachments,
    cod1ActorModelMethodResult_t *result) {
    uint32_t i;int phase,ok=1;
    if(!ActorModelBeginOwned(lease,&phase,1))return 0;
    if(phase!=spawning){ActorModelEnd(lease);return 0;}
    for(i=0;i<attachments->count;++i) {
        /* Every admitted nonempty slot owns a tag identity, including the
         * interned empty tag. Native diagnostics therefore include it. */
        cod1DObjModel_t *d;
        if(!ActorApplyRegistration(&attachments->models[i],spawning,0)){ok=0;break;}
        d=&result->diagnostics[result->diagnosticCount++];
        strcpy(d->model,attachments->models[i].nativeName);strcpy(d->tag,composition->descriptors[i+1].tag);
    }
    ActorModelEnd(lease);return ok && ActorMethodCurrent(lease,spawning);
}
static int ActorInvokeModelMethod(const cod1ActorLease_t *input,uint32_t method,
    const void *args,int argc,size_t argSize,int legacy,cod1ActorModelMethodResult_t *out,
    char *error,size_t size) {
    cod1ActorLease_t lease;
    cod1ActorModelMethodResult_t result;
    cod1ActorModelRegistration_t base,registration;
    cod1ActorModelComposition_t composition;
    cod1ActorRegisteredAttachments_t attachments;
    cod1ModelPrecache_t precache;
    cod1ActorAssignedModel_t assigned;
    GscNativeValue copiedLegacy[2];cod1ActorModelValue_t copied[2];
    cod1ActorModelString_t stringResult;
    char model[128],tag[128],lowerTag[128];
    uint32_t found=0,i;
    int spawning,consumed,completed=0,relink=0;
    if(argc<0 || argc>GSC_NATIVE_MAX_ARGS || method<COD1_ACTOR_METHOD_SETMODEL ||
       method>COD1_ACTOR_METHOD_DETACH_ALL ||
       !ActorArguments(input,sizeof(*input),out,sizeof(*out),error,size) ||
       !ActorArguments(args,(size_t)argc*argSize,out,sizeof(*out),error,size) ||
       ActorOverlap(input,sizeof(*input),out,sizeof(*out)) ||
       ActorOverlap(args,(size_t)argc*argSize,out,sizeof(*out)) ||
       ActorOverlap(input,sizeof(*input),args,(size_t)argc*argSize))return 0;
    if(!input || !out || (method!=COD1_ACTOR_METHOD_DETACH_ALL && argc && !args))return 0;
    lease=*input;
    if(!ActorRegisteredEdit(&lease,&base,&composition,&attachments))return 0;
    spawning=!!level.spawning;
    if(level.restarted)return 0;
    cod1Actors[lease.actorIndex].modelMethodBusy=1;
    memset(&result,0,sizeof(result));result.assigned=cod1Actors[lease.actorIndex].assignedModel;
    memset(copied,0,sizeof(copied));memset(model,0,sizeof(model));memset(tag,0,sizeof(tag));
    memset(lowerTag,0,sizeof(lowerTag));
    consumed=method==COD1_ACTOR_METHOD_DETACH_ALL?0:(method==COD1_ACTOR_METHOD_SETMODEL?1:(argc>=2?2:1));
    if(consumed && !argc) {
        result.disposition=COD1_ACTOR_METHOD_PARAMETER_ERROR;result.errorKind=COD1_ACTOR_METHOD_ERROR_MISSING_PARAMETER;
        result.parameterIndex=1;strcpy(result.message,"parameter 1 does not exist");completed=1;goto done;
    }
    for(i=0;i<(uint32_t)consumed;++i) {
        if(legacy)copiedLegacy[i]=((const GscNativeValue *)args)[i];
        else copied[i]=((const cod1ActorModelValue_t *)args)[i];
    }
    for(i=0;i<(uint32_t)consumed;++i) {
        int cast;
        if(legacy && !ActorMethodLegacyValue(&copiedLegacy[i],&copied[i]))goto done;
        ++result.consumedParameterCount;
        cast=G_Cod1ActorConvertModelValue(&copied[i],&stringResult,result.message,sizeof(result.message));
        if(!cast)goto done;
        if(cast<0){result.disposition=COD1_ACTOR_METHOD_PARAMETER_ERROR;
            result.errorKind=COD1_ACTOR_METHOD_ERROR_TYPE_CAST;result.parameterIndex=i+1;completed=1;goto done;}
        strcpy(result.parameterText[i],stringResult.string);
        if(stringResult.converted)result.coercedParameterMask|=1u<<i;
    }
    if(consumed)strcpy(model,result.parameterText[0]);
    if(consumed>1)strcpy(tag,result.parameterText[1]);
    for(i=0;tag[i];++i) {
        unsigned char c=(unsigned char)tag[i];if(c<32 || c>126)goto done;
        lowerTag[i]=(char)(c>='A' && c<='Z'?c+('a'-'A'):c);
    }
    if(!ActorMethodCurrent(&lease,spawning))goto done;
    if(method==COD1_ACTOR_METHOD_ATTACH || method==COD1_ACTOR_METHOD_DETACH) {
        if(!ActorMethodLookup(&lease,spawning,&composition,&attachments,model,lowerTag,&found,error,size))goto done;
        if(found) {
            ActorMethodRemove(&composition,&attachments,found);
            if(!ActorApplyModel(&base,&composition,&attachments,0,&assigned,error,size,1))goto done;
            result.mutation=1;result.assigned=assigned;result.descriptorIndex=found;
            if(method==COD1_ACTOR_METHOD_ATTACH) {
                result.disposition=COD1_ACTOR_METHOD_SCRIPT_ERROR;result.errorKind=COD1_ACTOR_METHOD_ERROR_DUPLICATE;
                snprintf(result.message,sizeof(result.message),"model '%s' already attached to tag '%s'",model,tag);
            } else result.disposition=COD1_ACTOR_METHOD_VOID;
            completed=1;goto done;
        }
        if(method==COD1_ACTOR_METHOD_DETACH) {
            if(!ActorMethodDiagnostics(&lease,spawning,&composition,&attachments,&result))goto done;
            result.disposition=COD1_ACTOR_METHOD_SCRIPT_ERROR;result.errorKind=COD1_ACTOR_METHOD_ERROR_NOT_ATTACHED;
            snprintf(result.message,sizeof(result.message),"failed to detach model '%s' from tag '%s'",model,tag);
            completed=1;goto done;
        }
        if(attachments.count==COD1_DOBJ_MODELS_MAX-1) {
            result.disposition=COD1_ACTOR_METHOD_SCRIPT_ERROR;result.errorKind=COD1_ACTOR_METHOD_ERROR_CAPACITY;
            strcpy(result.message,"maximum attached models exceeded");completed=1;goto done;
        }
        /* BYTE0/nonzero tag holes need their separate native slot/string-ID
         * representation. Never substitute a fake registered model index. */
        if(!model[0])goto done;
    }
    if(method==COD1_ACTOR_METHOD_SETMODEL || method==COD1_ACTOR_METHOD_ATTACH) {
        memset(&registration,0,sizeof(registration));registration.lease=lease;
        if(model[0]) {
            if(strlen(model)>=MAX_QPATH || !Cod1PrecacheName(model,1))goto done;
            memset(&precache,0,sizeof(precache));precache.version=COD1_PRECACHE_VERSION;
            precache.bytes=sizeof(precache);strcpy(precache.name,model);
            if(!ActorAcquireModelRegistration(&lease,&precache,&registration,error,size,1) ||
               !ActorMethodCurrent(&lease,spawning))goto done;
        }
        if(method==COD1_ACTOR_METHOD_SETMODEL){base=registration;relink=1;}
        else {
            cod1DObjModel_t *d=&composition.descriptors[composition.descriptorCount++];
            memset(d,0,sizeof(*d));ActorApplyPath(d->model,registration.nativeName);strcpy(d->tag,lowerTag);
            attachments.models[attachments.count++]=registration;
        }
    } else if(method==COD1_ACTOR_METHOD_DETACH_ALL) {
        composition.descriptorCount=1;memset(composition.descriptors+1,0,7*sizeof(composition.descriptors[0]));
        memset(&attachments,0,sizeof(attachments));
    }
    if(!ActorApplyModel(&base,&composition,&attachments,relink,&assigned,error,size,1))goto done;
    result.disposition=COD1_ACTOR_METHOD_VOID;result.mutation=1;result.assigned=assigned;completed=1;
done:
    if(completed && !ActorMethodCurrent(&lease,spawning))completed=0;
    if(ActorLeaseValid(&lease))cod1Actors[lease.actorIndex].modelMethodBusy=0;
    if(completed){*out=result;return (int)result.disposition;}
    EntityError(error,size,"model method operand/coercion, hidden owner, registration or topology is unavailable");return 0;
}
int G_Cod1ActorInvokeModelMethod(const cod1ActorLease_t *lease,uint32_t method,
    const GscNativeValue *args,int argc,cod1ActorModelMethodResult_t *out,char *error,size_t size) {
    return ActorInvokeModelMethod(lease,method,args,argc,sizeof(*args),1,out,error,size);
}
int G_Cod1ActorInvokeTypedModelMethod(const cod1ActorLease_t *lease,uint32_t method,
    const cod1ActorModelValue_t *args,int argc,cod1ActorModelMethodResult_t *out,char *error,size_t size) {
    return ActorInvokeModelMethod(lease,method,args,argc,sizeof(*args),0,out,error,size);
}
