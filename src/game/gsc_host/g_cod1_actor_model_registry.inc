/* R160 actual server resource -> owned map CS registration, separate from
 * native actor model-byte assignment and DObj/animation/publication. */
typedef struct {
    uint64_t epoch;
    cod1ModelResourceReference_t resource;
    char name[MAX_QPATH];
} cod1ActorRegisteredModel_t;
static cod1ActorRegisteredModel_t cod1ActorRegisteredModels[MAX_MODELS];
/* A new map releases the old lock, including an abandoned fatal-error stack.
 * Every old operation checks its full lease/epoch before touching this table. */
static uint64_t cod1ActorModelRegistryBusyEpoch;
static void ActorModelRegistryReset( void ) {
    memset( cod1ActorRegisteredModels, 0, sizeof( cod1ActorRegisteredModels ) );
    cod1ActorModelRegistryBusyEpoch = 0;
}
static int ActorModelStageBusy( const cod1ActorLease_t *lease, int spawning, int dobjBusy ) {
    return ActorModelTopologyStageValid( lease, dobjBusy ) && !level.restarted &&
        cod1Actors[lease->actorIndex].modelBusy == 1 &&
        cod1ActorModelRegistryBusyEpoch == lease->entity.epoch &&
        !!level.spawning == spawning;
}
static int ActorModelStage( const cod1ActorLease_t *lease, int spawning ) {
    return ActorModelStageBusy( lease, spawning, 0 );
}
static int ActorModelBeginOwned( const cod1ActorLease_t *lease, int *spawning, int methodOwned ) {
    if ( cod1ActorModelRegistryBusyEpoch || level.restarted ||
         !ActorModelTopologyStageValid( lease, 0 ) || cod1Actors[lease->actorIndex].modelBusy ||
         cod1Actors[lease->actorIndex].modelMethodBusy != methodOwned ) return 0;
    *spawning = !!level.spawning;
    cod1Actors[lease->actorIndex].modelBusy = 1;
    cod1ActorModelRegistryBusyEpoch = lease->entity.epoch;
    return 1;
}
static int ActorModelBegin( const cod1ActorLease_t *lease, int *spawning ) {
    return ActorModelBeginOwned(lease,spawning,0);
}
static void ActorModelEnd( const cod1ActorLease_t *lease ) {
    if ( ActorLeaseValid( lease ) && lease->kind == COD1_ACTOR_LEASE_ACTOR )
        cod1Actors[lease->actorIndex].modelBusy = 0;
    if ( cod1ActorModelRegistryBusyEpoch == lease->entity.epoch )
        cod1ActorModelRegistryBusyEpoch = 0;
}
static int ActorModelReadCSBusy( const cod1ActorLease_t *lease, int spawning,
    int busy, uint32_t index, char *text ) {
    memset( text, 0xa5, MAX_STRING_CHARS );
    trap_GetConfigstring( CS_MODELS + index, text, MAX_STRING_CHARS );
    return ActorModelStageBusy( lease, spawning, busy ) && memchr( text, 0, MAX_STRING_CHARS ) != NULL;
}
static int ActorModelReadCS( const cod1ActorLease_t *lease, int spawning,
    uint32_t index, char *text ) {
    return ActorModelReadCSBusy( lease, spawning, 0, index, text );
}
static int ActorModelQuery( uint32_t slot, const char *name,
    cod1ModelResourceReference_t *out ) {
    cod1ModelResourceReference_t request, result;
    int status;
    memset( &request, 0, sizeof( request ) );
    memset( &result, 0, sizeof( result ) );
    request.version = COD1_MODEL_RESOURCE_REFERENCE_VERSION;
    request.bytes = sizeof( request ); request.operation = COD1_MODEL_RESOURCE_QUERY;
    request.slot = slot; strcpy( request.name, name );
    status = trap_Cod1ModelResourceReference( &request, &result );
    if ( status != 1 ) return status;
    if ( result.version != request.version || result.bytes != sizeof( result ) ||
         result.operation != request.operation || result.reserved || !result.scene ||
         !result.slot || result.slot > COD1_PRECACHE_MODELS ||
         ( slot && result.slot != slot ) || !memchr( result.name, 0, sizeof( result.name ) ) ||
         !Cod1PrecacheName( result.name, 1 ) || Q_stricmp( result.name, name ) ) return 0;
    *out = result; return 1;
}
static int ActorModelValidateResource( const cod1ModelResourceReference_t *resource ) {
    cod1ModelResourceReference_t request = *resource, result;
    memset( &result, 0, sizeof( result ) );
    request.operation = COD1_MODEL_RESOURCE_VALIDATE;
    return trap_Cod1ModelResourceReference( &request, &result ) == 1 &&
        !memcmp( &request, &result, sizeof( request ) );
}
static int ActorAcquireModelRegistration( const cod1ActorLease_t *input,
    const cod1ModelPrecache_t *model, cod1ActorModelRegistration_t *out,
    char *error, size_t size, int methodOwned ) {
    cod1ActorLease_t lease;
    cod1ModelPrecache_t request;
    cod1ActorModelRegistration_t result;
    cod1ActorRegisteredModel_t entry;
    char text[MAX_STRING_CHARS];
    uint32_t index;
    int spawning, found = 0, slot, status, accepted = 0;
    if ( !ActorArguments( input, sizeof( *input ), out, sizeof( *out ), error, size ) ||
         !ActorArguments( model, sizeof( *model ), out, sizeof( *out ), error, size ) ||
         ActorOverlap( input, sizeof( *input ), model, sizeof( *model ) ) ||
         ActorOverlap( input, sizeof( *input ), out, sizeof( *out ) ) ||
         ActorOverlap( model, sizeof( *model ), out, sizeof( *out ) ) ) return 0;
    if ( !input || !model || !out ) {
        EntityError( error, size, "model registration requires lease, typed request and output" ); return 0;
    }
    lease = *input; request = *model;
    if ( request.version != COD1_PRECACHE_VERSION || request.bytes != sizeof( request ) ||
         !memchr( request.name, 0, sizeof( request.name ) ) ||
         !Cod1PrecacheName( request.name, 1 ) || !ActorModelBeginOwned( &lease, &spawning, methodOwned ) ) {
        EntityError( error, size, "model registration requires bounded admitted name and idle hidden native data" ); return 0;
    }
    memset( &result, 0, sizeof( result ) ); result.lease = lease;
    for ( index = 1; index < MAX_MODELS; ++index ) {
        if ( !ActorModelReadCS( &lease, spawning, index, text ) ) goto done;
        if ( !text[0] ) break;
        if ( !Q_stricmp( text, request.name ) ) { found = 1; break; }
    }
    if ( index == MAX_MODELS || ( !found && !spawning ) ) goto done;
    /* A CS string is not proof of a loaded model. A pure bank query admits
     * genuine reuse outside registration; missing acquisition needs the gate. */
    status = ActorModelQuery( 0, request.name, &result.resource );
    if ( !ActorModelStage( &lease, spawning ) ) goto done;
    if ( status != 1 ) {
        if ( status != COD1_PRECACHE_UNAVAILABLE || !spawning ) goto done;
        slot = trap_Cod1ModelPrecache( &request );
        if ( !ActorModelStage( &lease, spawning ) || slot < 1 || slot > COD1_PRECACHE_MODELS ) goto done;
        if ( ActorModelQuery( (uint32_t)slot, request.name, &result.resource ) != 1 ||
             !ActorModelStage( &lease, spawning ) ) goto done;
    }
    if ( !ActorModelReadCS( &lease, spawning, index, text ) ) goto done;
    if ( text[0] && Q_stricmp( text, request.name ) ) goto done;
    if ( found && !text[0] ) goto done;
    if ( !ActorModelValidateResource( &result.resource ) || !ActorModelStage( &lease, spawning ) ) goto done;
    if ( !text[0] ) {
        trap_SetConfigstring( CS_MODELS + index, request.name );
        if ( !ActorModelStage( &lease, spawning ) ||
             !ActorModelReadCS( &lease, spawning, index, text ) || strcmp( text, request.name ) ) goto done;
    }
    /* Last resource check is pure in production: no FS/zone/GAME callback can
     * invalidate the captured bank after its identity check within this call. */
    if ( !Cod1PrecacheNameBytes( text, 1, MAX_QPATH ) ||
         !ActorModelValidateResource( &result.resource ) || !ActorModelStage( &lease, spawning ) ) goto done;
    result.modelIndex = index; strcpy( result.nativeName, text );
    memset( &entry, 0, sizeof( entry ) ); entry.epoch = lease.entity.epoch;
    entry.resource = result.resource; strcpy( entry.name, text );
    cod1ActorRegisteredModels[index] = entry;
    *out = result; accepted = 1;
done:
    ActorModelEnd( &lease );
    if ( !accepted ) EntityError( error, size, "model registration resource/table/phase or actor owner changed" );
    return accepted;
}
int G_Cod1ActorAcquireModelRegistration( const cod1ActorLease_t *input,
    const cod1ModelPrecache_t *model, cod1ActorModelRegistration_t *out,
    char *error, size_t size ) {
    return ActorAcquireModelRegistration(input,model,out,error,size,0);
}
int G_Cod1ActorModelRegistrationCurrent( const cod1ActorModelRegistration_t *input,
    char *error, size_t size ) {
    cod1ActorModelRegistration_t model;
    cod1ActorRegisteredModel_t saved;
    char text[MAX_STRING_CHARS];
    int spawning, accepted;
    if ( !ActorArguments( input, sizeof( *input ), NULL, 0, error, size ) ) return 0;
    if ( !input ) { EntityError( error, size, "model registration check requires a snapshot" ); return 0; }
    model = *input;
    if ( !model.modelIndex || model.modelIndex >= MAX_MODELS || model.reserved ||
         !memchr( model.nativeName, 0, sizeof( model.nativeName ) ) ||
         !Cod1PrecacheName( model.nativeName, 1 ) ) return 0;
    saved = cod1ActorRegisteredModels[model.modelIndex];
    if ( saved.epoch != model.lease.entity.epoch || strcmp( saved.name, model.nativeName ) ||
         memcmp( &saved.resource, &model.resource, sizeof( saved.resource ) ) ||
         !ActorModelBegin( &model.lease, &spawning ) ) return 0;
    accepted = ActorModelReadCS( &model.lease, spawning, model.modelIndex, text ) &&
        !strcmp( text, model.nativeName ) &&
        ActorModelValidateResource( &model.resource ) && ActorModelStage( &model.lease, spawning );
    ActorModelEnd( &model.lease );
    if ( !accepted ) EntityError( error, size, "model registration snapshot is no longer current" );
    return accepted;
}
