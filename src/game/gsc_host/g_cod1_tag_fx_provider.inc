/* R126 existing actor topology only. Included beside the ownership registry.
 * No implicit reservation, model loading, dictionary insertion, entity event,
 * actor admission or pose substitute. VM/session fences remain in R124. */
#include "../cod1_gsc/gsc_tag_fx_payload.inc"

/* Missing ownership has a native MODEL-absent result. A known actor must
 * still have a valid lease and eligible stage before absence can be reported.
 * Busy, retired, admitted or malformed records never masquerade as absence. */
static int EntityTagFxAbsent(const GscEntityRef *entity) {
    cod1ActorLease_t lease;
    const cod1ActorRecord_t *actor;
    uint32_t index;
    int i;
    if (!ActorMapAvailable() || !G_Cod1EntitySameAllocation(entity)) return 0;
    /* A registered RTCW model, brush/player/cast entity or inconsistent
     * reservation has no proved provider here. Do not label it model-less. */
    if (g_entities[entity->slot].s.modelindex || g_entities[entity->slot].s.modelindex2)
        return 0;
    if (!cod1ActorByEntity[entity->slot]) {
        if (cod1SentientByEntity[entity->slot] || g_entities[entity->slot].client ||
            g_entities[entity->slot].r.bmodel ||
            (g_entities[entity->slot].r.svFlags & (SVF_CASTAI | SVF_BOT))) return 0;
        for (i = 0; i < COD1_ACTOR_CAPACITY; ++i)
            if (cod1Actors[i].activation >= 0 &&
                ActorRefEqual(&cod1Actors[i].owner, entity)) return 0;
        for (i = 0; i < COD1_SENTIENT_CAPACITY; ++i)
            if (cod1Sentients[i].activation >= 0 &&
                ActorRefEqual(&cod1Sentients[i].owner, entity)) return 0;
        return 1;
    }
    index = cod1ActorByEntity[entity->slot] - 1;
    if (index >= COD1_ACTOR_CAPACITY) return 0;
    actor = &cod1Actors[index];
    memset(&lease, 0, sizeof(lease));
    lease.entity = *entity; lease.serial = actor->serial; lease.actorIndex = index;
    lease.sentientIndex = actor->sentientIndex; lease.kind = COD1_ACTOR_LEASE_ACTOR;
    return ActorDObjStageValid(&lease, 0) && !actor->dobj.present;
}

int G_Cod1EntityTagFxQuery(void *user, int logicalMs, GscTagFxQuery *query,
    char *error, size_t size) {
    GscTagFxQuery before, copy;
    cod1ActorDObjLease_t captured;
    cod1DObjQueryResult_t result;
    (void)user;
    if (!ActorArguments(query, sizeof(*query), query, sizeof(*query), error, size)) return 0;
    if (!query || logicalMs < 0) goto refuse;
    before = copy = *query;
    if (!tag_fx_query_valid(&copy)) goto refuse;
    if (!G_Cod1ActorCaptureDObj(&copy.entity, &captured, NULL, 0)) {
        if (copy.phase != GSC_TAG_FX_MODEL || !EntityTagFxAbsent(&copy.entity)) goto refuse;
        /* All response fields remain the initial absent/-1 values. */
        return 1;
    }
    /* R124 cookies reserve MAX; the lower-level R125 query itself does not. */
    if (captured.topologySerial == UINT64_MAX ||
        (copy.phase == GSC_TAG_FX_BONE && copy.model_identity != captured.topologySerial) ||
        !G_Cod1ActorQueryDObj(&captured.lease, captured.topologySerial,
            copy.phase == GSC_TAG_FX_MODEL ? COD1_DOBJ_QUERY_MODEL : COD1_DOBJ_QUERY_BONE,
            copy.tag, &result, NULL, 0) ||
        memcmp(query, &before, sizeof(before))) goto refuse;
    if (copy.phase == GSC_TAG_FX_MODEL) {
        copy.model_present = 1; copy.model_identity = captured.topologySerial;
        memset(copy.model_name, 0, sizeof(copy.model_name));
        memcpy(copy.model_name, result.model, strlen(result.model) + 1);
    } else {
        if (strcmp(copy.model_name, result.model)) goto refuse;
        copy.bone_index = result.boneIndex;
    }
    if (!tag_fx_reply_valid(&before, &copy)) goto refuse;
    *query = copy;
    return 1;
refuse:
    EntityError(error, size, "tag FX query refused: unavailable entity/model, stale topology or copied payload");
    return 0;
}

int G_Cod1EntityTagFxValidate(const GscTagFxEvent *event, char *error, size_t size) {
    GscTagFxEvent before, copy;
    cod1ActorDObjLease_t captured;
    cod1DObjQueryResult_t result;
    if (!ActorArguments(event, sizeof(*event), NULL, 0, error, size)) return 0;
    if (!event) goto refuse;
    before = copy = *event;
    if (!tag_fx_event_valid(&copy) ||
        !G_Cod1ActorCaptureDObj(&copy.entity, &captured, NULL, 0) ||
        captured.topologySerial != copy.model_identity ||
        !G_Cod1ActorQueryDObj(&captured.lease, captured.topologySerial,
            COD1_DOBJ_QUERY_BONE, copy.tag, &result, NULL, 0) ||
        result.boneIndex < 0 || memcmp(event, &before, sizeof(before))) goto refuse;
    return 1;
refuse:
    EntityError(error, size, "tag FX preflight refused: invalid token, retired model or missing current bone");
    return 0;
}
