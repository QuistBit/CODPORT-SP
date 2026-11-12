/* R104 headless, fresh-map DObj resource/topology owner. Sole translation
 * unit: sv_world.c. Bind transforms are source records, never a current pose.
 * Ordinary SV_UnlinkEntity deliberately does not dispose these instances.
 * Port limits (255 resources and 16 MiB/blob) are not retail pool parity. */
#include "../qcommon/cod1_dobj_topology.h"

#define CD_RESOURCE_MAX 255
#define CD_BLOB_MAX (16 * 1024 * 1024)

typedef struct {
    int slot;
    uint64_t identity;
} cod1DObjResourceRef_t;

typedef struct {
    unsigned int references;
    uint64_t identity;
    char path[COD1_DOBJ_NAME_MAX];
    c1mrResource_t *resource;
    cod1ModelResourceReference_t registeredBase;
} cod1DObjResourceEntry_t;

typedef struct {
    cod1SpatialActorKey_t key;
    sharedEntity_t *entity;
    uint64_t serial;
    c1dtTopology_t topology;
    cod1DObjResourceRef_t resources[COD1_DOBJ_MODELS_MAX];
    uint32_t authoredCount;
    cod1DObjModel_t authored[COD1_DOBJ_MODELS_MAX];
} cod1DObjInstance_t;

static cod1DObjResourceEntry_t cod1DObjResources[CD_RESOURCE_MAX];
static cod1DObjInstance_t *cod1DObjInstances[MAX_GENTITIES];
static uint64_t cod1DObjEpoch, cod1DObjMutation, cod1DObjResourceIdentity;
static uint64_t cod1DObjTopologySerial;
static int cod1DObjBuildBusy, cod1DObjBuildCancelled;
static cod1SpatialActorKey_t cod1DObjPendingKey;

static void CD_Mutated(void) {
    if (cod1DObjMutation != UINT64_MAX) ++cod1DObjMutation;
}

static void CD_CancelPending(void) {
    if (cod1DObjBuildBusy) cod1DObjBuildCancelled = 1;
}

static int CD_String(const char *s, size_t capacity) {
    size_t i;
    for (i = 0; i < capacity; ++i) if (!s[i]) return 1;
    return 0;
}

/* No absolute path, backslash, empty/dot traversal segment, control byte or
 * filesystem metacharacter. Resource spelling remains caller-owned data. */
static int CD_Path(const char *s, size_t capacity) {
    size_t i, start;
    unsigned char c;
    if (!s || !s[0] || !CD_String(s, capacity)) return 0;
    start = 0;
    for (i = 0; ; ++i) {
        c = (unsigned char)s[i];
        if (!c || c == '/') {
            if (i == start || (i - start == 1 && s[start] == '.') ||
                (i - start == 2 && s[start] == '.' && s[start + 1] == '.'))
                return 0;
            if (!c) break;
            start = i + 1;
        } else if (c < 33 || c > 126 || c == '\\' || c == ':' ||
                   c == '*' || c == '?' || c == '"' || c == '<' ||
                   c == '>' || c == '|') return 0;
    }
    return 1;
}

static int CD_ModelPath(const char *s) {
    return CD_Path(s, COD1_DOBJ_NAME_MAX) &&
        !Q_stricmpn(s, "xmodel/", 7) && s[7];
}

static int CD_Associated(const cod1DObjInstance_t *object) {
    int slot;
    if (!object || !CS_TableReady()) return 0;
    slot = (int)object->key.slot;
    return slot >= 0 && slot < sv.num_entities && slot < ENTITYNUM_WORLD &&
        SV_GentityNum(slot) == object->entity && object->entity->s.number == slot;
}

static int CD_OwnedHull(int slot,const sharedEntity_t*ent){
 int x,zmin,zmax;if(!ent->r.contents&&!ent->s.solid)return 1;
 if(ent->r.contents!=CONTENTS_BODY||!cod1SpatialActors[slot].active||!CS_StateValid(slot,ent)||!CS_Bounds(ent->r.mins,ent->r.maxs))return 0;
 if(ent->r.maxs[0]<-1000000||ent->r.maxs[0]>1000000||ent->r.mins[2]<-1000000||ent->r.mins[2]>1000000||ent->r.maxs[2]<-1000000||ent->r.maxs[2]>1000000)return 0;
 x=(int)ent->r.maxs[0];zmin=(int)-ent->r.mins[2];zmax=(int)(ent->r.maxs[2]+32);
 if(x<1)x=1;if(x>255)x=255;if(zmin<1)zmin=1;if(zmin>255)zmin=255;if(zmax<1)zmax=1;if(zmax>255)zmax=255;
 return ent->s.solid==((zmax<<16)|(zmin<<8)|x);
}
static int CD_Eligible(const cod1SpatialActorKey_t *key, sharedEntity_t **out) {
    sharedEntity_t *ent;
    int slot;
    if (!CS_TableReady()) return COD1_DOBJ_UNAVAILABLE;
    if (!CS_KeyValid(key) || key->slot < MAX_CLIENTS) return COD1_DOBJ_INVALID;
    slot = (int)key->slot;
    if ((cod1DObjEpoch && cod1DObjEpoch != key->epoch) ||
        (cod1SpatialEpoch && cod1SpatialEpoch != key->epoch)) return COD1_DOBJ_STALE;
    if (cod1DObjInstances[slot] &&
        !CS_KeyEqual(&cod1DObjInstances[slot]->key, key)) return COD1_DOBJ_STALE;
    if (cod1SpatialActors[slot].active &&
        !CS_KeyEqual(&cod1SpatialActors[slot].key, key)) return COD1_DOBJ_STALE;
    if (key->slot >= (uint32_t)sv.num_entities)
        return cod1DObjInstances[slot] || cod1SpatialActors[slot].active ?
            COD1_DOBJ_STALE : COD1_DOBJ_INVALID;
    ent = SV_GentityNum(slot);
    if (cod1DObjInstances[slot] && !CD_Associated(cod1DObjInstances[slot]))
        return COD1_DOBJ_STALE;
    if (cod1SpatialActors[slot].active &&
        !CS_StateAssociated(slot, cod1SpatialActors[slot].entity)) return COD1_DOBJ_STALE;
    if (ent->s.number != slot || ent->r.bmodel || !CD_OwnedHull(slot,ent) ||
        !(ent->r.svFlags & SVF_NOCLIENT) || (ent->r.svFlags & (SVF_BOT | SVF_CASTAI)))
        return COD1_DOBJ_INVALID;
    if (ent->r.linked && (!cod1SpatialActors[slot].active ||
        !CS_StateValid(slot, ent))) return COD1_DOBJ_INVALID;
    *out = ent;
    return COD1_DOBJ_OK;
}

static int CD_TransactionValid(const cod1SpatialActorKey_t *key,
                              sharedEntity_t *entity, uint64_t mutation) {
    sharedEntity_t *current;
    return cod1DObjBuildBusy && !cod1DObjBuildCancelled &&
        cod1DObjMutation == mutation && CS_KeyEqual(&cod1DObjPendingKey, key) &&
        CD_Eligible(key, &current) == COD1_DOBJ_OK && current == entity;
}

static void CD_ReleaseResource(cod1DObjResourceRef_t *ref) {
    cod1DObjResourceEntry_t *entry;
    if (!ref->identity || ref->slot < 0 || ref->slot >= CD_RESOURCE_MAX) return;
    entry = &cod1DObjResources[ref->slot];
    if (entry->identity == ref->identity && entry->references) {
        --entry->references;
        if (!entry->references) {
            if (entry->resource) Z_Free(entry->resource);
            memset(entry, 0, sizeof(*entry));
        }
    }
    ref->identity = 0;
    ref->slot = -1;
}

static void CD_DisposeInstance(cod1DObjInstance_t *object) {
    unsigned int i;
    if (!object) return;
    for (i = 0; i < COD1_DOBJ_MODELS_MAX; ++i) CD_ReleaseResource(&object->resources[i]);
    Z_Free(object);
}

static int CD_AcquireResource(const char *path, const cod1SpatialActorKey_t *key,
    sharedEntity_t *entity, uint64_t mutation, cod1DObjResourceRef_t *out,
    const c1mrResource_t **resourceOut) {
    int i, freeSlot, modelSize, partsSize, status;
    void *modelData, *partsData;
    c1mrResource_t *resource;
    char partsPath[sizeof("xmodelparts/") + C1MR_NAME_MAX];
    cod1DObjResourceEntry_t *entry;
    freeSlot = -1;
    if (!CD_TransactionValid(key, entity, mutation)) return COD1_DOBJ_STALE;
    for (i = 0; i < CD_RESOURCE_MAX; ++i) {
        entry = &cod1DObjResources[i];
        if (!entry->references) { if (freeSlot < 0) freeSlot = i; continue; }
        /* The VFS is case-insensitive. No model-specific alias is involved. */
        if (!entry->registeredBase.scene && !Q_stricmp(entry->path, path)) {
            if (entry->references == UINT_MAX) return COD1_DOBJ_OVERFLOW;
            ++entry->references;
            out->slot = i; out->identity = entry->identity;
            *resourceOut = entry->resource;
            return COD1_DOBJ_OK;
        }
    }
    if (freeSlot < 0 || cod1DObjResourceIdentity == UINT64_MAX) return COD1_DOBJ_OVERFLOW;
    resource = (c1mrResource_t *)Z_Malloc((int)sizeof(*resource));
    if (!resource) return COD1_DOBJ_UNAVAILABLE;
    modelData = partsData = NULL;
    status = COD1_DOBJ_UNAVAILABLE;
    modelSize = FS_ReadFile(path, &modelData);
    if (!CD_TransactionValid(key, entity, mutation)) { status = COD1_DOBJ_STALE; goto finished; }
    if (modelSize <= 0 || modelSize > CD_BLOB_MAX || !modelData) goto finished;
    status = COD1_DOBJ_INVALID;
    if (!C1MR_DecodeModel(modelData, (size_t)modelSize, &resource->model) ||
        !CD_Path(resource->model.lodName[0], C1MR_NAME_MAX)) goto finished;
    strcpy(partsPath, "xmodelparts/");
    strcat(partsPath, resource->model.lodName[0]);
    status = COD1_DOBJ_UNAVAILABLE;
    partsSize = FS_ReadFile(partsPath, &partsData);
    if (!CD_TransactionValid(key, entity, mutation)) { status = COD1_DOBJ_STALE; goto finished; }
    if (partsSize <= 0 || partsSize > CD_BLOB_MAX || !partsData) goto finished;
    status = COD1_DOBJ_INVALID;
    if (!C1MR_DecodeResource(path, modelData, (size_t)modelSize,
        resource->model.lodName[0], partsData, (size_t)partsSize, resource)) goto finished;
    if (resource->skeleton.boneCount >= C1MR_BONES_MAX) {
        status = COD1_DOBJ_OVERFLOW; goto finished;
    }
    status = COD1_DOBJ_OK;
finished:
    if (partsData) FS_FreeFile(partsData);
    if (modelData) FS_FreeFile(modelData);
    /* FS_ReadFile and FS_FreeFile are callback boundaries in the checked
     * bridge. A cancelled candidate retains its own refs until this rollback;
     * reset never frees memory underneath the in-flight loader. */
    if (!CD_TransactionValid(key, entity, mutation)) status = COD1_DOBJ_STALE;
    if (status != COD1_DOBJ_OK) { Z_Free(resource); return status; }
    /* Nested Build is refused, so only frees/resets could change the bank.
     * Re-select a currently free slot rather than using a stale reservation. */
    for (i = 0; i < CD_RESOURCE_MAX; ++i)
        if (!cod1DObjResources[i].references) break;
    if (i == CD_RESOURCE_MAX) { Z_Free(resource); return COD1_DOBJ_OVERFLOW; }
    entry = &cod1DObjResources[i];
    memset(entry, 0, sizeof(*entry));
    entry->identity = ++cod1DObjResourceIdentity;
    entry->references = 1; entry->resource = resource;
    strcpy(entry->path, path);
    out->slot = i; out->identity = entry->identity;
    *resourceOut = resource;
    return COD1_DOBJ_OK;
}

static void SV_Cod1DObjClearScene(void) {
    int i;
    CD_CancelPending();
    CD_Mutated();
    for (i = 0; i < MAX_GENTITIES; ++i) {
        cod1DObjInstance_t *object = cod1DObjInstances[i];
        cod1DObjInstances[i] = NULL;
        CD_DisposeInstance(object);
    }
    cod1DObjEpoch = 0;
    /* Candidate-only references, if this is an FS callback, are released by
     * that cancelled Build. Published resources have all been released here. */
}

int SV_Cod1DObjBuild(const cod1DObjBuild_t *request, cod1DObjSummary_t *summary) {
    cod1DObjBuild_t input;
    cod1DObjSummary_t result;
    cod1DObjInstance_t *candidate, *old;
    cod1DObjResourceRef_t acquired[COD1_DOBJ_MODELS_MAX];
    c1dtModelInput_t models[COD1_DOBJ_MODELS_MAX];
    sharedEntity_t *entity;
    uint64_t mutation;
    unsigned int i, accepted;
    int status, topologyStatus;
    if (!request || !summary) return COD1_DOBJ_INVALID;
    input = *request;
    if (input.version != COD1_DOBJ_VERSION || input.bytes != sizeof(input) ||
        !input.modelCount ||
        !CS_KeyValid(&input.key)) return COD1_DOBJ_INVALID;
    if (input.modelCount > COD1_DOBJ_MODELS_MAX) return COD1_DOBJ_OVERFLOW;
    for (i = 0; i < input.modelCount; ++i)
        if (!CD_ModelPath(input.models[i].model) ||
            !CD_String(input.models[i].tag, COD1_DOBJ_NAME_MAX)) return COD1_DOBJ_INVALID;
    if (input.animationTree || cod1DObjBuildBusy) return COD1_DOBJ_UNAVAILABLE;
    if (cod1DObjMutation == UINT64_MAX || cod1DObjTopologySerial == UINT64_MAX)
        return COD1_DOBJ_OVERFLOW;
    status = CD_Eligible(&input.key, &entity);
    if (status != COD1_DOBJ_OK) return status;
    memset(acquired, 0, sizeof(acquired));
    memset(models, 0, sizeof(models));
    candidate = NULL;
    cod1DObjBuildBusy = 1; cod1DObjBuildCancelled = 0;
    cod1DObjPendingKey = input.key;
    mutation = cod1DObjMutation;
    candidate = (cod1DObjInstance_t *)Z_Malloc((int)sizeof(*candidate));
    if (!candidate) { status = COD1_DOBJ_UNAVAILABLE; goto finished; }
    memset(candidate, 0, sizeof(*candidate));
    candidate->key = input.key; candidate->entity = entity;
    candidate->authoredCount = input.modelCount;
    memcpy(candidate->authored, input.models, input.modelCount * sizeof(input.models[0]));
    for (i = 0; i < input.modelCount; ++i) {
        status = CD_AcquireResource(input.models[i].model, &input.key, entity,
            mutation, &acquired[i], &models[i].resource);
        if (status != COD1_DOBJ_OK) goto finished;
        models[i].tagName = input.models[i].tag;
    }
    topologyStatus = C1DT_Build(models, input.modelCount, &candidate->topology);
    if (topologyStatus != C1DT_OK) {
        status = topologyStatus == C1DT_CAPACITY ? COD1_DOBJ_OVERFLOW : COD1_DOBJ_INVALID;
        goto finished;
    }
    memset(&result, 0, sizeof(result));
    result.version = COD1_DOBJ_VERSION; result.bytes = sizeof(result);
    result.modelCount = candidate->topology.modelCount;
    result.boneCount = candidate->topology.boneCount;
    result.duplicateCount = candidate->topology.duplicateCount;
    result.classificationModelsMask = candidate->topology.classificationModelMask;
    for (i = 0; i < input.modelCount; ++i)
        if (candidate->topology.missingTagInputMask & (1u << i)) ++result.skippedCount;
    if (!C1DT_FirstModelBounds(&candidate->topology, &result.firstModelBounds)) {
        status = COD1_DOBJ_INVALID; goto finished;
    }
    if (!CD_TransactionValid(&input.key, entity, mutation)) {
        status = COD1_DOBJ_STALE; goto finished;
    }
    for (i = 0; i < candidate->topology.modelCount; ++i) {
        accepted = candidate->topology.models[i].inputIndex;
        candidate->resources[i] = acquired[accepted];
        memset(&acquired[accepted], 0, sizeof(acquired[accepted]));
    }
    candidate->serial = ++cod1DObjTopologySerial;
    result.topologySerial = candidate->serial;
    old = cod1DObjInstances[input.key.slot];
    cod1DObjInstances[input.key.slot] = candidate;
    cod1DObjEpoch = input.key.epoch;
    candidate = NULL;
    CD_Mutated();
    CD_DisposeInstance(old);
    *summary = result;
    status = COD1_DOBJ_OK;
finished:
    for (i = 0; i < COD1_DOBJ_MODELS_MAX; ++i) CD_ReleaseResource(&acquired[i]);
    CD_DisposeInstance(candidate);
    cod1DObjBuildBusy = 0; cod1DObjBuildCancelled = 0;
    memset(&cod1DObjPendingKey, 0, sizeof(cod1DObjPendingKey));
    return status;
}

int SV_Cod1DObjFree(const cod1DObjFree_t *request) {
    cod1DObjFree_t input;
    cod1DObjInstance_t *object;
    int i;
    if (!request) return COD1_DOBJ_INVALID;
    input = *request;
    if (input.version != COD1_DOBJ_VERSION || input.bytes != sizeof(input) ||
        !CS_KeyValid(&input.key)) return COD1_DOBJ_INVALID;
    object = cod1DObjInstances[input.key.slot];
    if (object && !CS_KeyEqual(&object->key, &input.key)) return COD1_DOBJ_STALE;
    if (cod1DObjBuildBusy && cod1DObjPendingKey.slot == input.key.slot &&
        !CS_KeyEqual(&cod1DObjPendingKey, &input.key)) return COD1_DOBJ_STALE;
    /* Absence is a cancellation and does not inspect a possibly unloaded or
     * smaller replacement table. No replacement object/key may be disposed. */
    if (!object) {
        if (cod1DObjBuildBusy && CS_KeyEqual(&cod1DObjPendingKey, &input.key)) {
            CD_CancelPending(); CD_Mutated();
        }
        return COD1_DOBJ_OK;
    }
    if (!CS_TableReady()) return COD1_DOBJ_UNAVAILABLE;
    if (!CD_Associated(object)) return COD1_DOBJ_STALE;
    if (cod1DObjBuildBusy && CS_KeyEqual(&cod1DObjPendingKey, &input.key)) CD_CancelPending();
    cod1DObjInstances[input.key.slot] = NULL;
    CD_Mutated();
    CD_DisposeInstance(object);
    for (i = 0; i < MAX_GENTITIES; ++i) if (cod1DObjInstances[i]) break;
    if (i == MAX_GENTITIES) cod1DObjEpoch = 0;
    return COD1_DOBJ_OK;
}

int SV_Cod1DObjSceneReset(const cod1DObjSceneReset_t *request) {
    cod1DObjSceneReset_t input;
    int i;
    if (!request) return COD1_DOBJ_INVALID;
    input = *request;
    if (input.version != COD1_DOBJ_VERSION || input.bytes != sizeof(input) || !input.epoch)
        return COD1_DOBJ_INVALID;
    if ((cod1DObjEpoch && cod1DObjEpoch != input.epoch) ||
        (cod1DObjBuildBusy && cod1DObjPendingKey.epoch != input.epoch)) return COD1_DOBJ_STALE;
    if (!cod1DObjEpoch && !cod1DObjBuildBusy) return COD1_DOBJ_OK;
    if (cod1DObjEpoch && !CS_TableReady()) return COD1_DOBJ_UNAVAILABLE;
    for (i = 0; i < MAX_GENTITIES; ++i)
        if (cod1DObjInstances[i] && !CD_Associated(cod1DObjInstances[i])) return COD1_DOBJ_STALE;
    SV_Cod1DObjClearScene();
    return COD1_DOBJ_OK;
}

#include "sv_cod1_dobj_query.inc"
