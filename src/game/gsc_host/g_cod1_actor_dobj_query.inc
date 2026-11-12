/* R125: one copied request, no side effects; engine callbacks in fixtures are
 * adversarial ownership boundaries. Do not set dobj.busy: that existing flag
 * belongs to Build/Free and must not prevent genuine retirement by a callback. */
static int ActorDObjQueryCurrent(const cod1ActorLease_t *lease, uint64_t serial) {
    const cod1ActorRecord_t *a;
    if (!serial || !ActorDObjStageValid(lease, 0)) return 0;
    a = &cod1Actors[lease->actorIndex];
    return a->dobj.present && a->dobj.summary.topologySerial == serial &&
        a->dobj.summary.modelCount <= COD1_DOBJ_MODELS_MAX &&
        a->dobj.summary.skippedCount <= COD1_DOBJ_MODELS_MAX - a->dobj.summary.modelCount &&
        ActorDObjSummaryValid(&a->dobj.summary,
            a->dobj.summary.modelCount + a->dobj.summary.skippedCount);
}

int G_Cod1ActorQueryDObj(const cod1ActorLease_t *input, uint64_t serial,
    uint32_t phase, const char *tag, cod1DObjQueryResult_t *out,
    char *error, size_t size) {
    cod1ActorLease_t lease;
    cod1DObjQuery_t request, original;
    cod1DObjQueryResult_t result;
    cod1DObjSummary_t before;
    size_t length;
    int status;
    if (!ActorArguments(input, sizeof(*input), out, sizeof(*out), error, size) ||
        !ActorArguments(tag, COD1_DOBJ_NAME_MAX, out, sizeof(*out), error, size)) return 0;
    if (!input || !out || !tag ||
        (phase != COD1_DOBJ_QUERY_MODEL && phase != COD1_DOBJ_QUERY_BONE)) goto refuse;
    lease = *input;
    for (length = 0; length < COD1_DOBJ_NAME_MAX && tag[length]; ++length) {}
    if (length == COD1_DOBJ_NAME_MAX ||
        (phase == COD1_DOBJ_QUERY_MODEL && length) ||
        !ActorDObjQueryCurrent(&lease, serial)) goto refuse;
    memset(&request, 0, sizeof(request)); memset(&result, 0, sizeof(result));
    request.version = COD1_DOBJ_QUERY_VERSION; request.bytes = sizeof(request);
    request.key = ActorSpatialKey(&lease); request.topologySerial = serial;
    request.phase = phase; memcpy(request.tag, tag, length + 1);
    original = request; before = cod1Actors[lease.actorIndex].dobj.summary;
    status = trap_Cod1DObjQuery(&request, &result);
    if (status != COD1_DOBJ_OK || memcmp(&request, &original, sizeof(request)) ||
        !ActorDObjQueryCurrent(&lease, serial) ||
        memcmp(&before, &cod1Actors[lease.actorIndex].dobj.summary, sizeof(before)) ||
        result.version != COD1_DOBJ_QUERY_VERSION || result.bytes != sizeof(result) ||
        result.topologySerial != serial || result.modelCount != before.modelCount ||
        result.boneCount != before.boneCount ||
        !memchr(result.model, 0, sizeof(result.model)) ||
        Q_stricmpn(result.model, "xmodel/", 7) || !result.model[7] ||
        result.boneIndex < -1 || result.boneIndex >= (int32_t)before.boneCount ||
        (phase == COD1_DOBJ_QUERY_MODEL && result.boneIndex != -1)) goto refuse;
    *out = result;
    return 1;
refuse:
    EntityError(error, size, "DObj query unavailable, stale lease/revision or invalid copied reply");
    return 0;
}

int G_Cod1ActorCaptureDObj(const GscEntityRef *input,
    cod1ActorDObjLease_t *out, char *error, size_t size) {
    GscEntityRef entity;
    cod1ActorDObjLease_t copy;
    const cod1ActorRecord_t *actor;
    uint32_t index;
    if (!ActorArguments(input, sizeof(*input), out, sizeof(*out), error, size)) return 0;
    if (!input || !out) goto refuse;
    entity = *input;
    if (!ActorMapAvailable() || !G_Cod1EntitySameAllocation(&entity) ||
        entity.slot < MAX_CLIENTS || entity.slot >= ENTITYNUM_MAX_NORMAL ||
        !cod1ActorByEntity[entity.slot]) goto refuse;
    index = cod1ActorByEntity[entity.slot] - 1;
    if (index >= COD1_ACTOR_CAPACITY) goto refuse;
    actor = &cod1Actors[index];
    memset(&copy, 0, sizeof(copy));
    copy.lease.entity = entity; copy.lease.serial = actor->serial;
    copy.lease.actorIndex = index; copy.lease.sentientIndex = actor->sentientIndex;
    copy.lease.kind = COD1_ACTOR_LEASE_ACTOR;
    copy.topologySerial = actor->dobj.summary.topologySerial;
    if (!ActorDObjQueryCurrent(&copy.lease, copy.topologySerial)) goto refuse;
    *out = copy;
    return 1;
refuse:
    EntityError(error, size, "DObj capture requires current owned provisional topology");
    return 0;
}
