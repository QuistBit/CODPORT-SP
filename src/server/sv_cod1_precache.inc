/* R108 map-owned server precache bank, independent of actors and DObj leases.
 * A candidate remains private across FS callbacks; reset cancels acquisition
 * without freeing its in-flight storage. Missing assets are refused here,
 * unlike retail's optional warning/default-model fallback. */
#include "../qcommon/cod1_model_resource.h"
#define CP_BLOB_MAX (16 * 1024 * 1024)
typedef struct {
    char name[MAX_QPATH];
    c1mrResource_t *resource;
} cod1PrecacheEntry_t;
static cod1PrecacheEntry_t cod1PrecacheBank[COD1_PRECACHE_MODELS];
static uint64_t cod1PrecacheMutation;
static int cod1PrecacheBusy, cod1PrecacheCancelled;

void SV_Cod1PrecacheClearScene(void) {
    int i;
    if (cod1PrecacheMutation != UINT64_MAX) ++cod1PrecacheMutation;
    if (cod1PrecacheBusy) cod1PrecacheCancelled=1;
    for (i=0;i<COD1_PRECACHE_MODELS;++i) {
        if (cod1PrecacheBank[i].resource) Z_Free(cod1PrecacheBank[i].resource);
        memset(&cod1PrecacheBank[i],0,sizeof(cod1PrecacheBank[i]));
    }
}
static int CP_Current(uint64_t mutation, int serverId) {
    return cod1PrecacheBusy && !cod1PrecacheCancelled &&
        cod1PrecacheMutation==mutation && sv.serverId==serverId &&
        CM_HasV59() && (sv.state==SS_LOADING || sv.state==SS_GAME);
}
static void CP_FSPath(char *out,const char *name) {
    size_t i;
    for(i=0;name[i];++i) out[i]=name[i]=='\\'?'/':name[i];
    out[i]=0;
}
int SV_Cod1ModelPrecache(const cod1ModelPrecache_t *input) {
    cod1ModelPrecache_t request;
    c1mrResource_t *resource=NULL;
    void *modelData=NULL,*partsData=NULL;
    char modelPath[MAX_QPATH],partsPath[sizeof("xmodelparts/")+C1MR_NAME_MAX];
    int i,slot=-1,modelSize,partsSize,status=COD1_PRECACHE_UNAVAILABLE;
    int serverId=sv.serverId;
    uint64_t mutation=cod1PrecacheMutation;
    if (!input || input->version!=COD1_PRECACHE_VERSION ||
        input->bytes!=sizeof(request)) return COD1_PRECACHE_INVALID;
    memcpy(&request,input,sizeof(request));
    if (!memchr(request.name,0,sizeof(request.name)) ||
        !Cod1PrecacheName(request.name,1)) return COD1_PRECACHE_INVALID;
    if (!CM_HasV59() || (sv.state!=SS_LOADING && sv.state!=SS_GAME) ||
        cod1PrecacheBusy) return COD1_PRECACHE_UNAVAILABLE;
    for(i=0;i<COD1_PRECACHE_MODELS;++i) {
        if (!cod1PrecacheBank[i].resource) {if(slot<0)slot=i;continue;}
        if (!Q_stricmp(cod1PrecacheBank[i].name,request.name)) return i+1;
    }
    if (slot<0 || mutation==UINT64_MAX) return COD1_PRECACHE_OVERFLOW;
    cod1PrecacheBusy=1;cod1PrecacheCancelled=0;
    resource=(c1mrResource_t *)Z_Malloc((int)sizeof(*resource));
    if (!resource) goto finished;
    CP_FSPath(modelPath,request.name);
    memcpy(modelPath,"xmodel/",7);
    modelSize=FS_ReadFile(modelPath,&modelData);
    if (!CP_Current(mutation,serverId)) {status=COD1_PRECACHE_STALE;goto finished;}
    if (modelSize<=0 || modelSize>CP_BLOB_MAX || !modelData) goto finished;
    status=COD1_PRECACHE_INVALID;
    if (!C1MR_DecodeModel(modelData,(size_t)modelSize,&resource->model) ||
        !Cod1PrecacheName(resource->model.lodName[0],0)) goto finished;
    strcpy(partsPath,"xmodelparts/");
    CP_FSPath(partsPath+strlen(partsPath),resource->model.lodName[0]);
    status=COD1_PRECACHE_UNAVAILABLE;
    partsSize=FS_ReadFile(partsPath,&partsData);
    if (!CP_Current(mutation,serverId)) {status=COD1_PRECACHE_STALE;goto finished;}
    if (partsSize<=0 || partsSize>CP_BLOB_MAX || !partsData) goto finished;
    status=COD1_PRECACHE_INVALID;
    if (!C1MR_DecodeResource(request.name,modelData,(size_t)modelSize,
        resource->model.lodName[0],partsData,(size_t)partsSize,resource) ||
        resource->skeleton.boneCount>=C1MR_BONES_MAX) goto finished;
    status=1;
finished:
    if (partsData) FS_FreeFile(partsData);
    if (modelData) FS_FreeFile(modelData);
    if (!CP_Current(mutation,serverId)) status=COD1_PRECACHE_STALE;
    if(status==1) {
        cod1PrecacheBank[slot].resource=resource;
        strcpy(cod1PrecacheBank[slot].name,request.name);
        resource=NULL;status=slot+1;
    }
    if(resource)Z_Free(resource);
    cod1PrecacheBusy=0;cod1PrecacheCancelled=0;
    return status;
}

int SV_Cod1ModelResourceReference(const cod1ModelResourceReference_t *input,
                                cod1ModelResourceReference_t *out) {
    cod1ModelResourceReference_t request, result;
    cod1PrecacheEntry_t *entry;
    uintptr_t a=(uintptr_t)input,b=(uintptr_t)out;
    uint32_t slot;
    if(!input || !out || (a<=b ? b-a<sizeof(*input) : a-b<sizeof(*out)))
        return COD1_PRECACHE_INVALID;
    request=*input;
    if(request.version!=COD1_MODEL_RESOURCE_REFERENCE_VERSION ||
       request.bytes!=sizeof(request) || request.reserved ||
       request.slot>COD1_PRECACHE_MODELS ||
       !memchr(request.name,0,sizeof(request.name)) ||
       !Cod1PrecacheName(request.name,1) ||
       (request.operation!=COD1_MODEL_RESOURCE_QUERY &&
        request.operation!=COD1_MODEL_RESOURCE_VALIDATE)) return COD1_PRECACHE_INVALID;
    if(!CM_HasV59() || (sv.state!=SS_LOADING && sv.state!=SS_GAME) ||
       cod1PrecacheBusy || cod1PrecacheMutation==UINT64_MAX) return COD1_PRECACHE_UNAVAILABLE;
    if(request.operation==COD1_MODEL_RESOURCE_QUERY) {
        if(request.scene || request.serverId) return COD1_PRECACHE_INVALID;
    } else if(!request.slot || request.scene!=cod1PrecacheMutation+1 ||
              request.serverId!=sv.serverId) return COD1_PRECACHE_STALE;
    slot=request.slot;
    if(!slot) {
        for(slot=1;slot<=COD1_PRECACHE_MODELS;++slot)
            if(cod1PrecacheBank[slot-1].resource &&
               !Q_stricmp(cod1PrecacheBank[slot-1].name,request.name)) break;
        if(slot>COD1_PRECACHE_MODELS) return COD1_PRECACHE_UNAVAILABLE;
    }
    entry=&cod1PrecacheBank[slot-1];
    if(!entry->resource || Q_stricmp(entry->name,request.name) ||
       Q_stricmp(entry->resource->modelName,entry->name)) return COD1_PRECACHE_STALE;
    memset(&result,0,sizeof(result));
    result.version=COD1_MODEL_RESOURCE_REFERENCE_VERSION;result.bytes=sizeof(result);
    result.operation=request.operation;result.slot=slot;
    result.scene=cod1PrecacheMutation+1;result.serverId=sv.serverId;
    strcpy(result.name,entry->name);*out=result;
    return 1;
}

/* Private engine-to-engine copy. The bank is never borrowed across an FS,
 * allocator or GAME callback. DObj owns the destination independently. */
int SV_Cod1CopyRegisteredModel(const cod1ModelResourceReference_t *input,
                              c1mrResource_t *out) {
    cod1ModelResourceReference_t request, result;
    if (!input || !out) return COD1_PRECACHE_INVALID;
    request = *input; request.operation = COD1_MODEL_RESOURCE_VALIDATE;
    if (SV_Cod1ModelResourceReference(&request, &result) != 1 ||
        memcmp(&request, &result, sizeof(request))) return COD1_PRECACHE_STALE;
    *out = *cod1PrecacheBank[result.slot-1].resource;
    return 1;
}

#include "sv_cod1_model_part.inc"
#include "sv_cod1_model_bounds.inc"
