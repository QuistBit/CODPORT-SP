/* R103 checked provisional placement. Included after actor_registry.inc.
 * Engine calls are ownership boundaries: no borrowed record/entity pointer is
 * trusted afterward without checking the full allocation AND lease again. */
static uint64_t cod1ActorSceneBusyEpoch;

static cod1SpatialActorKey_t ActorSpatialKey( const cod1ActorLease_t *lease ) {
    cod1SpatialActorKey_t key;
    memset( &key, 0, sizeof( key ) );
    key.epoch = lease->entity.epoch; key.lease = lease->serial;
    key.slot = lease->entity.slot; key.generation = lease->entity.generation;
    return key;
}
static int ActorSpatialCancel( const cod1ActorLease_t *lease ) {
    cod1SpatialActorUnlink_t request;
    memset( &request, 0, sizeof( request ) );
    request.version = COD1_SPATIAL_VERSION; request.bytes = sizeof( request );
    request.key = ActorSpatialKey( lease );
    return trap_Cod1SpatialActorUnlink( &request );
}
static int ActorSpatialUnlink( const cod1ActorLease_t *lease, int pending,
    char *error, size_t size ) {
    cod1ActorRecord_t *a;
    int status, oldBusy;
    if ( !ActorLeaseValid( lease ) || lease->kind != COD1_ACTOR_LEASE_ACTOR ) {
        EntityError( error, size, "spatial cleanup received a stale actor lease" ); return 0;
    }
    a = &cod1Actors[lease->actorIndex];
    if ( !a->spatial.linked && !a->spatial.busy ) return 1;
    if ( a->spatial.busy == 2 || ( a->spatial.busy && !pending ) ) {
        EntityError( error, size, "actor spatial operation is already in progress" ); return 0;
    }
    oldBusy = a->spatial.busy;
    a->spatial.busy = 2;
    status = ActorSpatialCancel( lease );
    if ( !ActorLeaseValid( lease ) ) {
        EntityError( error, size, "actor allocation changed during spatial cleanup" ); return 0;
    }
    a = &cod1Actors[lease->actorIndex];
    a->spatial.busy = oldBusy;
    if ( status != COD1_SPATIAL_OK ) {
        EntityError( error, size, "checked actor unlink failed; reservation retained" ); return 0;
    }
    a->spatial.linked = 0;
    return 1;
}
static int ActorSpatialUnlinkLease( const cod1ActorLease_t *lease,
    char *error, size_t size ) {
    return ActorSpatialUnlink( lease, 0, error, size );
}
/* Born may encounter a still-pending link callback. Its exact old key is
 * cleaned here, and the outer placement independently cancels that old key
 * after noticing the changed generation. No replacement is recaptured. */
static int ActorSpatialUnlinkOwner( const GscEntityRef *owner ) {
    cod1ActorLease_t lease;
    const cod1ActorRecord_t *a;
    uint32_t index;
    if ( owner->slot >= MAX_GENTITIES || !cod1ActorByEntity[owner->slot] ) return 1;
    index = cod1ActorByEntity[owner->slot] - 1;
    a = &cod1Actors[index];
    if ( !ActorRefEqual( owner, &a->owner ) ) return 1;
    memset( &lease, 0, sizeof( lease ) );
    lease.entity = *owner; lease.serial = a->serial; lease.actorIndex = index;
    lease.sentientIndex = a->sentientIndex; lease.kind = COD1_ACTOR_LEASE_ACTOR;
    return ActorSpatialUnlink( &lease, 1, NULL, 0 );
}
static int ActorSpatialSceneCleanup( void ) {
    cod1SpatialSceneReset_t request;
    uint64_t epoch = cod1EntityEpoch;
    int i, present = 0, status;
    for ( i = 0; i < COD1_ACTOR_CAPACITY; ++i )
        if ( cod1Actors[i].activation >= 0 &&
             ( cod1Actors[i].spatial.linked || cod1Actors[i].spatial.busy ) ) present = 1;
    if ( !present ) return 1;
    if ( !epoch || cod1ActorSceneBusyEpoch == epoch ) return 0;
    memset( &request, 0, sizeof( request ) );
    request.version = COD1_SPATIAL_VERSION; request.bytes = sizeof( request );
    request.epoch = epoch;
    cod1ActorSceneBusyEpoch = epoch;
    status = trap_Cod1SpatialSceneReset( &request );
    if ( cod1ActorSceneBusyEpoch == epoch ) cod1ActorSceneBusyEpoch = 0;
    return status == COD1_SPATIAL_OK;
}
int G_Cod1ActorProbeFloor( const GscEntityRef *input,
    cod1ActorFloorPlacement_t *out, char *error, size_t size ) {
    GscEntityRef ref;
    cod1SpatialTraceQuery_t request;
    cod1ActorFloorPlacement_t result;
    trace_t trace;
    int i, status;
    if ( !ActorArguments( input, sizeof( *input ), out, sizeof( *out ), error, size ) ) return 0;
    if ( !input || !out ) { EntityError( error, size, "actor floor query requires identity and output" ); return 0; }
    ref = *input;
    if ( !ActorMapAvailable() || !G_Cod1EntitySameAllocation( &ref ) ) {
        EntityError( error, size, "actor floor query requires a current fresh CoD SP allocation" ); return 0;
    }
    memset( &request, 0, sizeof( request ) ); memset( &result, 0, sizeof( result ) );
    request.version = COD1_SPATIAL_VERSION; request.bytes = sizeof( request );
    request.contentsMask = COD1_SPATIAL_FLOOR_MASK;
    request.passEntity[0] = request.passEntity[1] = ENTITYNUM_NONE;
    result.entity = ref;
    for ( i = 0; i < 3; ++i ) {
        float value = g_entities[ref.slot].r.currentOrigin[i];
        if ( !isfinite( value ) ) { EntityError( error, size, "actor floor origin must be finite" ); return 0; }
        result.originalOrigin[i] = request.start[i] = request.end[i] = value;
    }
    request.start[2] += 1.0f; request.end[2] -= 128.0f;
    request.mins[0] = request.mins[1] = -15.0f;
    request.maxs[0] = request.maxs[1] = 15.0f; request.maxs[2] = 30.0f;
    if ( !isfinite( request.start[2] ) || !isfinite( request.end[2] ) ) {
        EntityError( error, size, "actor floor trace bounds must be finite" ); return 0;
    }
    memset( &trace, 0, sizeof( trace ) );
    status = trap_Cod1SpatialCapsuleTrace( &request, &trace );
    if ( !ActorMapAvailable() || !G_Cod1EntitySameAllocation( &ref ) ) {
        EntityError( error, size, "actor allocation changed during floor query" ); return 0;
    }
    if ( !VectorCompare( result.originalOrigin, g_entities[ref.slot].r.currentOrigin ) ) {
        EntityError( error, size, "actor origin changed during floor query" ); return 0;
    }
    if ( status != COD1_SPATIAL_OK || !isfinite( trace.fraction ) ||
         trace.fraction < 0.0f || trace.fraction > 1.0f ||
         ( trace.startsolid != qfalse && trace.startsolid != qtrue ) ||
         ( trace.allsolid != qfalse && trace.allsolid != qtrue ) ||
         trace.entityNum < 0 || trace.entityNum >= MAX_GENTITIES ) {
        EntityError( error, size, "actor floor trace is unavailable or returned an invalid result" ); return 0;
    }
    for ( i = 0; i < 3; ++i ) {
        if ( !isfinite( trace.endpos[i] ) ) {
            EntityError( error, size, "actor floor trace returned a nonfinite endpoint" ); return 0;
        }
        /* Original byte2f is STARTSOLID, not an allsolid/no-floor gate. */
        result.placedOrigin[i] = trace.startsolid ? request.start[i] : trace.endpos[i];
    }
    result.fraction = trace.fraction; result.startsolid = trace.startsolid;
    result.allsolid = trace.allsolid; result.hitSlot = trace.entityNum;
    *out = result;
    return 1;
}
static int ActorSpatialStageValid( const cod1ActorLease_t *lease, int busy ) {
    const cod1ActorRecord_t *a;
    const gentity_t *e;
    if ( !ActorLeaseValid( lease ) || lease->kind != COD1_ACTOR_LEASE_ACTOR ) return 0;
    a = &cod1Actors[lease->actorIndex]; e = &g_entities[lease->entity.slot];
    return a->alive && a->nativeDataInitialized && !a->initialized &&
        cod1ActorEngineSceneBusyEpoch != lease->entity.epoch &&
        !a->spatial.linked && a->spatial.busy == busy && !a->dobj.busy && !a->modelBusy && !a->modelMethodBusy &&
        !e->client && !e->r.bmodel && !e->r.linked && !e->r.contents &&
        !e->neverFree && ( e->r.svFlags & SVF_NOCLIENT ) &&
        !( e->r.svFlags & ( SVF_CASTAI | SVF_BOT ) );
}
int G_Cod1ActorPlaceAndLink( const cod1ActorLease_t *input,
    char *error, size_t size ) {
    cod1ActorLease_t lease;
    cod1ActorFloorPlacement_t placement;
    cod1ActorNativeEntityDefaults_t oldNative;
    cod1SpatialActorLink_t request;
    trajectory_t oldTrajectory;
    trajectory_t placedTrajectory;
    vec3_t oldOrigin, oldMins, oldMaxs, linkAngles;
    cod1ActorRecord_t *a;
    gentity_t *e;
    int i, status, canceled;
    if ( !ActorArguments( input, sizeof( *input ), NULL, 0, error, size ) ) return 0;
    if ( !input ) { EntityError( error, size, "actor placement requires a lease" ); return 0; }
    lease = *input;
    if ( !ActorSpatialStageValid( &lease, 0 ) ) {
        EntityError( error, size, "actor placement requires an alive native-data lease and already hidden nonsolid unlinked entity" ); return 0;
    }
    cod1Actors[lease.actorIndex].spatial.busy = 1;
    status = G_Cod1ActorProbeFloor( &lease.entity, &placement, error, size );
    if ( !ActorSpatialStageValid( &lease, 1 ) ) {
        if ( ActorLeaseValid( &lease ) ) cod1Actors[lease.actorIndex].spatial.busy = 0;
        EntityError( error, size, "actor placement ownership or unpublished state changed during floor query" ); return 0;
    }
    a = &cod1Actors[lease.actorIndex];
    if ( !status ) { a->spatial.busy = 0; return 0; }
    e = &g_entities[lease.entity.slot];
    oldTrajectory = e->s.pos; VectorCopy( e->r.currentOrigin, oldOrigin );
    VectorCopy( e->r.mins, oldMins ); VectorCopy( e->r.maxs, oldMaxs );
    oldNative = a->nativeData.entity;
    G_SetOrigin( e, placement.placedOrigin );
    VectorCopy( a->nativeData.entity.mins, e->r.mins );
    VectorCopy( a->nativeData.entity.maxs, e->r.maxs );
    placedTrajectory = e->s.pos; VectorCopy( e->r.currentAngles, linkAngles );
    for ( i = 0; i < 3; ++i ) {
        a->nativeData.entity.origin[i] = a->nativeData.entity.originTrajectory[i] = placement.placedOrigin[i];
        a->nativeData.entity.originDelta[i] = 0;
    }
    a->nativeData.entity.originType = a->nativeData.entity.originTime = a->nativeData.entity.originDuration = 0;
    memset( &request, 0, sizeof( request ) );
    request.version = COD1_SPATIAL_VERSION; request.bytes = sizeof( request );
    request.key = ActorSpatialKey( &lease );
    request.nativeContents = a->nativeData.entity.nativeContents;
    request.nativeServerFlags = a->nativeData.entity.nativeServerFlags;
    status = trap_Cod1SpatialActorLink( &request );
    /* A successful checked link changes r.linked, so validate ownership and
     * unpublished eligibility separately from the pre-link unlinked gate. */
    if ( !ActorLeaseValid( &lease ) ) {
        canceled = ActorSpatialCancel( &lease );
        if ( canceled != COD1_SPATIAL_OK && canceled != COD1_SPATIAL_STALE )
            G_Error( "R103: cannot cancel orphaned provisional actor link" );
        EntityError( error, size, "actor allocation changed during native link" ); return 0;
    }
    a = &cod1Actors[lease.actorIndex]; e = &g_entities[lease.entity.slot];
    if ( status == COD1_SPATIAL_OK && a->alive && !a->initialized &&
         a->nativeDataInitialized && a->spatial.busy == 1 &&
         ( e->r.svFlags & SVF_NOCLIENT ) && !e->client && !e->r.bmodel &&
         e->r.linked && !e->neverFree &&
         VectorCompare( placement.placedOrigin, e->r.currentOrigin ) &&
         VectorCompare( a->nativeData.entity.mins, e->r.mins ) &&
         VectorCompare( a->nativeData.entity.maxs, e->r.maxs ) &&
         VectorCompare( linkAngles, e->r.currentAngles ) &&
         !memcmp( &placedTrajectory, &e->s.pos, sizeof( placedTrajectory ) ) &&
         !( e->r.svFlags & ( SVF_BOT | SVF_CASTAI ) ) && !e->r.contents ) {
        a->spatial.floor = placement; a->spatial.linked = 1; a->spatial.busy = 0;
        return 1;
    }
    if ( status == COD1_SPATIAL_OK ) {
        canceled = ActorSpatialCancel( &lease );
        if ( !ActorLeaseValid( &lease ) ) {
            EntityError( error, size, "actor allocation changed during placement cancellation" ); return 0;
        }
        a = &cod1Actors[lease.actorIndex]; e = &g_entities[lease.entity.slot];
        if ( canceled != COD1_SPATIAL_OK ) {
            a->spatial.linked = 1; a->spatial.busy = 0;
            EntityError( error, size, "actor link cancellation failed; reservation and placed pose retained" ); return 0;
        }
    }
    /* Checked failure publishes no private registration. Ordinary engine
     * link diagnostics (abs bounds/PVS etc.) are outside this stage's rollback.
     * Restore only owned pose fields; preserve death, flags, angles and all
     * unrelated callback game/script state. */
    e->s.pos = oldTrajectory; VectorCopy( oldOrigin, e->r.currentOrigin );
    VectorCopy( oldMins, e->r.mins ); VectorCopy( oldMaxs, e->r.maxs );
    for ( i = 0; i < 3; ++i ) {
        a->nativeData.entity.origin[i] = oldNative.origin[i];
        a->nativeData.entity.originTrajectory[i] = oldNative.originTrajectory[i];
        a->nativeData.entity.originDelta[i] = oldNative.originDelta[i];
    }
    a->nativeData.entity.originType = oldNative.originType;
    a->nativeData.entity.originTime = oldNative.originTime;
    a->nativeData.entity.originDuration = oldNative.originDuration;
    a->spatial.busy = 0;
    EntityError( error, size, "checked actor link failed or unpublished state changed; owned pose restored" );
    return 0;
}
