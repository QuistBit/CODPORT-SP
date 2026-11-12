/* R143: internal static topology for an R141 owned, unpublished ammo candidate.
 * The native DObj key is opaque: no actor/sentient reservation is required.
 * A single base and zero animation tree are a selected subset, not inferred
 * from the retail item kind. Every trap is an allocation/revision boundary. */
static cod1SpatialActorKey_t AmmoDObjKey( const GscEntityRef *entity ) {
    cod1SpatialActorKey_t key;
    memset( &key, 0, sizeof( key ) );
    key.epoch = entity->epoch; key.slot = entity->slot;
    key.generation = entity->generation; key.lease = entity->generation;
    return key;
}

static int AmmoDObjCancel( const GscEntityRef *entity ) {
    cod1DObjFree_t request;
    memset( &request, 0, sizeof( request ) );
    request.version = COD1_DOBJ_VERSION; request.bytes = sizeof( request );
    request.key = AmmoDObjKey( entity );
    return trap_Cod1DObjFree( &request );
}

static int AmmoDObjFreeOwner( const GscEntityRef *entity, int pending,
    char *error, size_t size ) {
    Cod1AmmoCandidate *candidate;
    int oldBusy, status;
    if ( !AmmoCandidateOwned( entity ) ) return 1;
    /* Birth has already overwritten classname; cleanup uses only the old
     * allocation/sidecar identity, never the initialized candidate body. */
    if ( !G_Cod1EntitySameAllocation( entity ) ) {
        EntityError( error, size, "ammo DObj cleanup allocation is stale" ); return 0;
    }
    if ( !AmmoWorldUnpublishOwner( entity, pending, error, size ) ||
         !AmmoCandidateOwned( entity ) || !G_Cod1EntitySameAllocation( entity ) ) return 0;
    candidate = &cod1AmmoCandidates[entity->slot];
    if ( !candidate->dobjPresent && !candidate->dobjBusy ) return 1;
    if ( candidate->dobjBusy == 2 || ( candidate->dobjBusy && !pending ) ) {
        EntityError( error, size, "ammo DObj operation is already in progress" ); return 0;
    }
    oldBusy = candidate->dobjBusy; candidate->dobjBusy = 2;
    status = AmmoDObjCancel( entity );
    if ( !AmmoCandidateOwned( entity ) || !G_Cod1EntitySameAllocation( entity ) ) {
        EntityError( error, size, "ammo allocation changed during DObj cleanup" ); return 0;
    }
    candidate = &cod1AmmoCandidates[entity->slot]; candidate->dobjBusy = oldBusy;
    if ( status != COD1_DOBJ_OK ) {
        EntityError( error, size, "checked ammo DObj free failed; ownership retained" ); return 0;
    }
    candidate->dobjPresent = 0;
    memset( &candidate->dobjSummary, 0, sizeof( candidate->dobjSummary ) );
    AmmoNativeStateInvalidate( entity );
    return 1;
}

static int AmmoDObjStageValid( const GscEntityRef *entity, int busy ) {
    cod1AmmoSpawnModel_t model;
    const gentity_t *ent;
    if ( !G_Cod1ReadAmmoSpawnCandidate( entity, &model ) ||
         cod1ActorEngineSceneBusyEpoch == entity->epoch ||
         cod1AmmoCandidates[entity->slot].dobjBusy != busy ||
         cod1AmmoCandidates[entity->slot].worldPresent || cod1AmmoCandidates[entity->slot].worldBusy ) return 0;
    ent = &g_entities[entity->slot];
    return !ent->s.solid && !( ent->r.svFlags & ( SVF_CASTAI | SVF_BOT ) );
}

/* Native Query owns topology independently of an ordinary world link. GAME
 * permits only its exact staged or owned published body; rebuild remains a
 * hidden-stage operation. A pending publication may read, never rebuild. */
static int AmmoDObjQueryStageValid( const GscEntityRef *entity ) {
    cod1AmmoSpawnModel_t model;
    const gentity_t *ent;
    if ( !G_Cod1ReadAmmoSpawnCandidate( entity, &model ) ||
         cod1ActorEngineSceneBusyEpoch == entity->epoch ||
         cod1AmmoCandidates[entity->slot].dobjBusy ||
         cod1AmmoCandidates[entity->slot].worldBusy == 2 ) return 0;
    ent = &g_entities[entity->slot];
    return !ent->s.solid && !( ent->r.svFlags & ( SVF_CASTAI | SVF_BOT ) );
}

static int AmmoDObjSummaryValid( const cod1DObjSummary_t *summary ) {
    return ActorDObjSummaryValid( summary, 1 ) && !summary->duplicateCount;
}

int G_Cod1BuildAmmoSpawnDObj( const GscEntityRef *input,
    cod1DObjSummary_t *out, char *error, size_t size ) {
    GscEntityRef entity;
    cod1DObjBuild_t request;
    cod1DObjSummary_t result;
    Cod1AmmoCandidate *candidate;
    uint64_t previousSerial;
    int status, canceled;
    if ( !ActorArguments( input, sizeof( *input ), out, sizeof( *out ), error, size ) ) return 0;
    if ( !input || !out ) {
        EntityError( error, size, "ammo DObj build requires candidate and output" ); return 0;
    }
    entity = *input;
    if ( !AmmoDObjStageValid( &entity, 0 ) ) {
        EntityError( error, size, "ammo DObj requires a current hidden unpublished candidate" ); return 0;
    }
    memset( &request, 0, sizeof( request ) ); memset( &result, 0, sizeof( result ) );
    request.version = COD1_DOBJ_VERSION; request.bytes = sizeof( request );
    request.key = AmmoDObjKey( &entity ); request.modelCount = 1;
    candidate = &cod1AmmoCandidates[entity.slot];
    Q_strncpyz( request.models[0].model, candidate->model.init.item.models[0],
        sizeof( request.models[0].model ) );
    previousSerial = candidate->dobjSummary.topologySerial;
    candidate->dobjBusy = 1;
    status = trap_Cod1DObjBuild( &request, &result );
    if ( !AmmoDObjStageValid( &entity, 1 ) ||
         ( status == COD1_DOBJ_OK && ( !AmmoDObjSummaryValid( &result ) ||
           result.topologySerial <= previousSerial ) ) ) {
        /* Failed native builds preserve prior topology. A successful build
         * with invalid post-state must cancel only its original copied key. */
        canceled = status == COD1_DOBJ_OK ? AmmoDObjCancel( &entity ) : COD1_DOBJ_OK;
        if ( AmmoCandidateOwned( &entity ) && G_Cod1EntitySameAllocation( &entity ) ) {
            candidate = &cod1AmmoCandidates[entity.slot]; candidate->dobjBusy = 0;
            if ( status == COD1_DOBJ_OK ) {
                candidate->dobjPresent = canceled != COD1_DOBJ_OK;
                AmmoNativeStateInvalidate( &entity );
                /* A failed cancel retains cleanup ownership, but an old
                 * summary cannot describe the newly committed topology. */
                memset( &candidate->dobjSummary, 0, sizeof( candidate->dobjSummary ) );
            }
        } else if ( canceled != COD1_DOBJ_OK && canceled != COD1_DOBJ_STALE ) {
            G_Error( "R143: orphan ammo DObj could not be canceled by its original key" );
        }
        EntityError( error, size, "ammo ownership, unpublished state or DObj result changed during build" ); return 0;
    }
    candidate = &cod1AmmoCandidates[entity.slot]; candidate->dobjBusy = 0;
    if ( status != COD1_DOBJ_OK ) {
        EntityError( error, size, "engine ammo DObj build is unavailable or rejected" ); return 0;
    }
    candidate->dobjSummary = result; candidate->dobjPresent = 1;
    AmmoNativeStateInvalidate( &entity );
    *out = result;
    return 1;
}

int G_Cod1ReadAmmoSpawnDObj( const GscEntityRef *input, cod1DObjSummary_t *out ) {
    GscEntityRef entity;
    cod1DObjSummary_t before;
    cod1DObjQuery_t request, original;
    cod1DObjQueryResult_t result;
    cod1AmmoSpawnModel_t model;
    int status;
    if ( !input || !out ) return 0;
    entity = *input;
    if ( !AmmoDObjQueryStageValid( &entity ) ||
         !cod1AmmoCandidates[entity.slot].dobjPresent ) return 0;
    before = cod1AmmoCandidates[entity.slot].dobjSummary;
    if ( !AmmoDObjSummaryValid( &before ) ||
         !G_Cod1ReadAmmoSpawnCandidate( &entity, &model ) ) return 0;
    memset( &request, 0, sizeof( request ) ); memset( &result, 0, sizeof( result ) );
    request.version = COD1_DOBJ_QUERY_VERSION; request.bytes = sizeof( request );
    request.key = AmmoDObjKey( &entity ); request.topologySerial = before.topologySerial;
    request.phase = COD1_DOBJ_QUERY_MODEL; original = request;
    /* No busy flag for reads: a callback may really free or rebuild the
     * instance. Recheck both allocation and copied summary after the query. */
    status = trap_Cod1DObjQuery( &request, &result );
    if ( status != COD1_DOBJ_OK || memcmp( &request, &original, sizeof( request ) ) ||
         !AmmoDObjQueryStageValid( &entity ) || !cod1AmmoCandidates[entity.slot].dobjPresent ||
         memcmp( &before, &cod1AmmoCandidates[entity.slot].dobjSummary, sizeof( before ) ) ||
         result.version != COD1_DOBJ_QUERY_VERSION || result.bytes != sizeof( result ) ||
         result.topologySerial != before.topologySerial || result.modelCount != before.modelCount ||
         result.boneCount != before.boneCount || result.boneIndex != -1 ||
         !memchr( result.model, 0, sizeof( result.model ) ) ||
         Q_stricmp( result.model, model.init.item.models[0] ) ) return 0;
    *out = before;
    return 1;
}

int G_Cod1FreeAmmoSpawnDObj( const GscEntityRef *input, char *error, size_t size ) {
    GscEntityRef entity;
    if ( !ActorArguments( input, sizeof( *input ), NULL, 0, error, size ) ) return 0;
    if ( !input ) {
        EntityError( error, size, "ammo DObj free requires an owned candidate" ); return 0;
    }
    entity = *input;
    if ( !AmmoCandidateOwned( &entity ) || !G_Cod1EntitySameAllocation( &entity ) ) {
        EntityError( error, size, "ammo DObj free candidate is stale or unowned" ); return 0;
    }
    return AmmoDObjFreeOwner( &entity, 0, error, size );
}
