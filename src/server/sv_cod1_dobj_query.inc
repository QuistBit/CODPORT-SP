/* R125: synchronous read-only use of the engine-owned R104 topology. No FS,
 * allocation or callback boundary occurs while the instance is borrowed. */
static int CD_QueryResourcesValid(const cod1DObjInstance_t *object) {
    const cod1DObjResourceRef_t *ref;
    const cod1DObjResourceEntry_t *entry;
    const c1dtModel_t *model;
    unsigned int i, first = 0;
    if (!object->topology.modelCount ||
        object->topology.modelCount > COD1_DOBJ_MODELS_MAX ||
        !object->topology.boneCount || object->topology.boneCount >= C1DT_MAX_BONES)
        return 0;
    for (i = 0; i < object->topology.modelCount; ++i) {
        ref = &object->resources[i];
        if (!ref->identity || ref->slot < 0 || ref->slot >= CD_RESOURCE_MAX) return 0;
        entry = &cod1DObjResources[ref->slot];
        model = &object->topology.models[i];
        if (!entry->references || entry->identity != ref->identity ||
            !entry->resource || model->resource != entry->resource ||
            !model->boneCount || model->boneCount != entry->resource->skeleton.boneCount ||
            model->firstBone != first || model->boneCount > object->topology.boneCount - first ||
            !CD_ModelPath(entry->path)) return 0;
        first += model->boneCount;
    }
    return first == object->topology.boneCount;
}

int SV_Cod1DObjQuery(const cod1DObjQuery_t *request, cod1DObjQueryResult_t *out) {
    cod1DObjQuery_t input;
    cod1DObjQueryResult_t result;
    const cod1DObjInstance_t *object;
    const cod1DObjResourceEntry_t *first;
    if (!request || !out) return COD1_DOBJ_INVALID;
    input = *request;
    if (input.version != COD1_DOBJ_QUERY_VERSION || input.bytes != sizeof(input) ||
        !CS_KeyValid(&input.key) || input.key.slot < MAX_CLIENTS ||
        !input.topologySerial || !CD_String(input.tag, sizeof(input.tag)) ||
        (input.phase != COD1_DOBJ_QUERY_MODEL && input.phase != COD1_DOBJ_QUERY_BONE) ||
        (input.phase == COD1_DOBJ_QUERY_MODEL && input.tag[0])) return COD1_DOBJ_INVALID;
    if (cod1DObjBuildBusy) return COD1_DOBJ_UNAVAILABLE;
    object = cod1DObjInstances[input.key.slot];
    if ((cod1DObjEpoch && cod1DObjEpoch != input.key.epoch) ||
        (object && !CS_KeyEqual(&object->key, &input.key))) return COD1_DOBJ_STALE;
    if (!CS_TableReady()) return COD1_DOBJ_UNAVAILABLE;
    if (!object) return COD1_DOBJ_UNAVAILABLE;
    if (!CD_Associated(object) || object->serial != input.topologySerial)
        return COD1_DOBJ_STALE;
    if (!CD_QueryResourcesValid(object)) return COD1_DOBJ_STALE;
    first = &cod1DObjResources[object->resources[0].slot];
    memset(&result, 0, sizeof(result));
    result.version = COD1_DOBJ_QUERY_VERSION; result.bytes = sizeof(result);
    result.topologySerial = object->serial;
    result.modelCount = object->topology.modelCount;
    result.boneCount = object->topology.boneCount;
    result.boneIndex = input.phase == COD1_DOBJ_QUERY_BONE ?
        C1DT_FindBone(&object->topology, input.tag) : -1;
    memcpy(result.model, first->path, strlen(first->path) + 1);
    *out = result;
    return COD1_DOBJ_OK;
}

/* R129 copied topology export. All checks precede the sole output commit;
 * this interval has no allocator, FS call or external callback. The caller
 * receives source topology associations, not borrowed pointers or a pose. */
static int CD_ExportOverlap(const void *a, size_t an, const void *b, size_t bn) {
    uintptr_t ap = (uintptr_t)a, bp = (uintptr_t)b;
    if (!a || !b || !an || !bn) return 0;
    return ap <= bp ? bp - ap < an : ap - bp < bn;
}

static int CD_ExportAliased(const cod1DObjExportRequest_t *request,
                           const cod1DObjExportResult_t *out) {
    unsigned int i;
    size_t bytes = sizeof(*out);
    if (CD_ExportOverlap(out, bytes, request, sizeof(*request)) ||
        CD_ExportOverlap(out, bytes, cod1DObjResources, sizeof(cod1DObjResources)) ||
        CD_ExportOverlap(out, bytes, cod1DObjInstances, sizeof(cod1DObjInstances)) ||
        CD_ExportOverlap(out, bytes, cod1SpatialActors, sizeof(cod1SpatialActors)) ||
        CD_ExportOverlap(out, bytes, &sv, sizeof(sv)) ||
        CD_ExportOverlap(out, bytes, &cod1DObjEpoch, sizeof(cod1DObjEpoch)) ||
        CD_ExportOverlap(out, bytes, &cod1DObjMutation, sizeof(cod1DObjMutation)) ||
        CD_ExportOverlap(out, bytes, &cod1DObjResourceIdentity, sizeof(cod1DObjResourceIdentity)) ||
        CD_ExportOverlap(out, bytes, &cod1DObjTopologySerial, sizeof(cod1DObjTopologySerial)) ||
        CD_ExportOverlap(out, bytes, &cod1DObjBuildBusy, sizeof(cod1DObjBuildBusy)) ||
        CD_ExportOverlap(out, bytes, &cod1DObjBuildCancelled, sizeof(cod1DObjBuildCancelled)) ||
        CD_ExportOverlap(out, bytes, &cod1DObjPendingKey, sizeof(cod1DObjPendingKey))) return 1;
    for (i = 0; i < MAX_GENTITIES; ++i) if (cod1DObjInstances[i]) {
        if (CD_ExportOverlap(out, bytes, cod1DObjInstances[i], sizeof(*cod1DObjInstances[i])) ||
            CD_ExportOverlap(out, bytes, cod1DObjInstances[i]->entity,
                             sizeof(*cod1DObjInstances[i]->entity))) return 1;
    }
    for (i = 0; i < CD_RESOURCE_MAX; ++i) if (cod1DObjResources[i].resource &&
        CD_ExportOverlap(out, bytes, cod1DObjResources[i].resource,
                         sizeof(*cod1DObjResources[i].resource))) return 1;
    return 0;
}

/* Same original raw-name precedence as C1DT_FindBone, restricted to the
 * previously accepted models for an authored attachment admission check. */
static int CD_ExportEarlierBone(const c1dtTopology_t *topology,
                               unsigned int count, const char *name) {
    unsigned int i, j;
    for (i = 0; i < count; ++i)
        for (j = topology->models[i].boneCount; j > 0; --j)
            if (!strcmp(name, topology->models[i].resource->skeleton.boneName[j - 1]))
                return (int)(topology->models[i].firstBone + j - 1);
    return -1;
}

static int CD_ExportTopologyValid(const cod1DObjInstance_t *object) {
    const c1dtTopology_t *t = &object->topology;
    const c1dtModel_t *m;
    const c1mrSkeleton_t *s;
    const c1dtBone_t *b;
    unsigned char duplicate[COD1_DOBJ_EXPORT_DUPLICATE_BYTES];
    unsigned int i, j, global, accepted = 0, previous = 0, first = 0;
    unsigned int missing = 0, unmelded = 0, classified = 0, warnings = 0;
    unsigned int duplicates = 0, at = 16;
    int anchor, target, parent;
    if (!object->authoredCount || object->authoredCount > COD1_DOBJ_MODELS_MAX ||
        !CD_QueryResourcesValid(object) || t->modelCount > object->authoredCount) return 0;
    for (i = 0; i < object->authoredCount; ++i)
        if (!CD_ModelPath(object->authored[i].model) ||
            !CD_String(object->authored[i].tag, sizeof(object->authored[i].tag))) return 0;
    for (i = 0; i < t->modelCount; ++i) {
        m = &t->models[i];
        if (m->inputIndex >= object->authoredCount ||
            (i ? m->inputIndex <= previous : m->inputIndex != 0) ||
            !C1DT_ResourceValid(m->resource) || !C1DT_String(m->tagName) ||
            m->firstBone != first ||
            Q_stricmp(object->authored[m->inputIndex].model,
                      cod1DObjResources[object->resources[i].slot].path)) return 0;
        previous = m->inputIndex; first += m->boneCount;
        if (m->resource->skeleton.hasClassification) classified |= 1u << i;
    }
    for (i = 0; i < object->authoredCount; ++i) {
        if (accepted < t->modelCount && t->models[accepted].inputIndex == i) {
            m = &t->models[accepted];
            anchor = i && object->authored[i].tag[0] ?
                CD_ExportEarlierBone(t, accepted, object->authored[i].tag) : -1;
            if (m->anchorBone != anchor ||
                (i && object->authored[i].tag[0] ?
                    anchor < 0 || strcmp(m->tagName, object->authored[i].tag) :
                    m->tagName[0] != 0)) return 0;
            ++accepted;
        } else {
            if (!i || !object->authored[i].tag[0] ||
                CD_ExportEarlierBone(t, accepted, object->authored[i].tag) >= 0) return 0;
            missing |= 1u << i; ++warnings;
        }
    }
    memset(duplicate, 0, sizeof(duplicate));
    for (i = 0; i < t->modelCount; ++i) {
        m = &t->models[i]; s = &m->resource->skeleton;
        for (j = 0; j < m->boneCount; ++j) {
            global = m->firstBone + j; b = &t->bones[global];
            parent = s->parent[j] < 0 ? m->anchorBone :
                     (int)(m->firstBone + (unsigned int)s->parent[j]);
            target = i && m->anchorBone < 0 ? C1DT_FindBone(t, s->boneName[j]) : (int)global;
            if (target < 0 || b->modelIndex != i || b->localBone != j ||
                b->parentBone != parent ||
                b->duplicateBone != (target == (int)global ? -1 : target) ||
                b->canonicalBone != (unsigned int)target) return 0;
            if (target != (int)global) {
                if (at + 2 >= sizeof(duplicate)) return 0;
                duplicate[global / 8] |= (unsigned char)(1u << (global % 8));
                duplicate[at++] = (unsigned char)(global + 1);
                duplicate[at++] = (unsigned char)(target + 1);
                ++duplicates;
            }
        }
        if (i && m->anchorBone < 0 && t->bones[m->firstBone].duplicateBone < 0) {
            unmelded |= 1u << m->inputIndex; ++warnings;
        }
    }
    duplicate[at++] = 0;
    return first == t->boneCount && accepted == t->modelCount &&
        duplicates == t->duplicateCount && at == t->duplicateBlobSize &&
        !memcmp(duplicate, t->duplicateBlob, at) &&
        missing == t->missingTagInputMask && unmelded == t->unmeldedRootInputMask &&
        classified == t->classificationModelMask && warnings == t->warningCount;
}

int SV_Cod1DObjExport(const cod1DObjExportRequest_t *request,
                     cod1DObjExportResult_t *out, size_t outBytes) {
    cod1DObjExportRequest_t input;
    const cod1DObjInstance_t *object;
    const c1dtTopology_t *t;
    sharedEntity_t *entity;
    unsigned int i;
    int status;
    if (!request || !out || outBytes != sizeof(*out)) return COD1_DOBJ_INVALID;
    input = *request;
    if (input.version != COD1_DOBJ_EXPORT_VERSION || input.bytes != sizeof(input) ||
        !CS_KeyValid(&input.key) || input.key.slot < MAX_CLIENTS || !input.topologySerial ||
        CD_ExportAliased(request, out)) return COD1_DOBJ_INVALID;
    if (cod1DObjBuildBusy) return COD1_DOBJ_UNAVAILABLE;
    status = CD_Eligible(&input.key, &entity);
    if (status != COD1_DOBJ_OK) return status;
    object = cod1DObjInstances[input.key.slot];
    if (!object) return COD1_DOBJ_UNAVAILABLE;
    if (object->entity != entity || object->serial != input.topologySerial ||
        !CD_ExportTopologyValid(object)) return COD1_DOBJ_STALE;
    /* No fallible operation or external boundary remains. Build directly in
     * independent caller storage, avoiding a large result on the C stack. */
    t = &object->topology;
    memset(out, 0, sizeof(*out));
    out->version = COD1_DOBJ_EXPORT_VERSION; out->bytes = sizeof(*out);
    out->key = input.key; out->topologySerial = object->serial;
    out->authoredCount = object->authoredCount; out->modelCount = t->modelCount;
    out->boneCount = t->boneCount; out->duplicateCount = t->duplicateCount;
    out->warningCount = t->warningCount; out->missingTagInputMask = t->missingTagInputMask;
    out->unmeldedRootInputMask = t->unmeldedRootInputMask;
    out->classificationModelsMask = t->classificationModelMask;
    out->duplicateBlobSize = t->duplicateBlobSize;
    for (i = 0; i < t->modelCount; ++i) {
        const c1dtModel_t *m = &t->models[i];
        const char *path = cod1DObjResources[object->resources[i].slot].path;
        out->models[i].authored = object->authored[m->inputIndex];
        memcpy(out->models[i].resource, path, strlen(path) + 1);
        out->models[i].inputIndex = m->inputIndex;
        out->models[i].firstBone = m->firstBone; out->models[i].boneCount = m->boneCount;
        out->models[i].anchorBone = m->anchorBone;
    }
    for (i = 0; i < t->boneCount; ++i) {
        out->bones[i].modelIndex = t->bones[i].modelIndex;
        out->bones[i].localBone = t->bones[i].localBone;
        out->bones[i].parentBone = t->bones[i].parentBone;
        out->bones[i].duplicateBone = t->bones[i].duplicateBone;
        out->bones[i].canonicalBone = t->bones[i].canonicalBone;
    }
    memcpy(out->duplicateBlob, t->duplicateBlob, t->duplicateBlobSize);
    return COD1_DOBJ_OK;
}
