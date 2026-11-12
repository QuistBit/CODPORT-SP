/* R144 owned normal/static native item state, kept behind the unpublished
 * candidate gate until native world/client/handler consumers are available. */
static void AmmoNativeStateInvalidate( const GscEntityRef *entity ) {
    Cod1AmmoCandidate *candidate;
    if ( !AmmoCandidateOwned( entity ) ) return;
    candidate = &cod1AmmoCandidates[entity->slot];
    candidate->nativeStatePresent = 0;
    memset( &candidate->nativeState, 0, sizeof( candidate->nativeState ) );
    /* An initializing query still owns its busy flag. DObj operations may
     * retire or rebuild its instance; the initializer rechecks afterwards. */
}

int G_Cod1InitializeAmmoSpawnNativeState( const GscEntityRef *input,
    const cod1AmmoSpawnNativeSeed_t *seedInput, cod1AmmoSpawnNativeState_t *out,
    char *error, size_t size ) {
    GscEntityRef entity;
    cod1AmmoSpawnNativeSeed_t seed;
    cod1AmmoSpawnNativeState_t state;
    cod1DObjSummary_t verified;
    Cod1AmmoCandidate *candidate;
    int accepted;
    if ( !ActorArguments( input, sizeof( *input ), out, sizeof( *out ), error, size ) ||
         !ActorArguments( seedInput, sizeof( *seedInput ), out, sizeof( *out ), error, size ) ) return 0;
    if ( !input || !seedInput || !out ) {
        EntityError( error, size, "ammo native state requires candidate, native seed and output" ); return 0;
    }
    entity = *input; seed = *seedInput;
    if ( !AmmoDObjStageValid( &entity, 0 ) ) {
        EntityError( error, size, "ammo native state requires a current unpublished candidate" ); return 0;
    }
    candidate = &cod1AmmoCandidates[entity.slot];
    if ( candidate->nativeStatePresent || candidate->nativeStateBusy ||
         !candidate->dobjPresent || !AmmoDObjSummaryValid( &candidate->dobjSummary ) ||
         !CIS_PrepareAmmoSpawnNativeState( &candidate->model,
             candidate->dobjSummary.topologySerial, &seed, &state ) ) {
        EntityError( error, size, "ammo native state already staged, DObj absent or native seed invalid" ); return 0;
    }
    candidate->nativeStateBusy = 1;
    accepted = G_Cod1ReadAmmoSpawnDObj( &entity, &verified );
    if ( !AmmoCandidateOwned( &entity ) || !G_Cod1EntitySameAllocation( &entity ) ) {
        EntityError( error, size, "ammo allocation changed during native state initialization" ); return 0;
    }
    candidate = &cod1AmmoCandidates[entity.slot];
    accepted = accepted && AmmoDObjStageValid( &entity, 0 ) &&
        candidate->nativeStateBusy == 1 && !candidate->nativeStatePresent &&
        candidate->dobjPresent && verified.topologySerial == state.topologySerial &&
        !memcmp( &verified, &candidate->dobjSummary, sizeof( verified ) );
    if ( candidate->nativeStateBusy == 1 ) candidate->nativeStateBusy = 0;
    if ( !accepted ) {
        EntityError( error, size, "ammo native state DObj bank, revision or ownership changed" ); return 0;
    }
    candidate->nativeState = state; candidate->nativeStatePresent = 1;
    *out = state;
    return 1;
}

int G_Cod1ReadAmmoSpawnNativeState( const GscEntityRef *input,
    cod1AmmoSpawnNativeState_t *out ) {
    GscEntityRef entity;
    cod1AmmoSpawnNativeState_t before;
    cod1DObjSummary_t verified;
    const Cod1AmmoCandidate *candidate;
    if ( !input || !out ) return 0;
    entity = *input;
    if ( !AmmoDObjQueryStageValid( &entity ) ) return 0;
    candidate = &cod1AmmoCandidates[entity.slot];
    if ( !candidate->nativeStatePresent || candidate->nativeStateBusy ) return 0;
    before = candidate->nativeState;
    if ( before.descriptor_fingerprint != candidate->model.descriptor_fingerprint ||
         before.server_model_slot != candidate->model.server_model_slot ||
         before.nativeItemIndex != (uint32_t)candidate->model.init.item.index ||
         !G_Cod1ReadAmmoSpawnDObj( &entity, &verified ) ||
         !AmmoDObjQueryStageValid( &entity ) ) return 0;
    candidate = &cod1AmmoCandidates[entity.slot];
    if ( !candidate->nativeStatePresent || candidate->nativeStateBusy ||
         verified.topologySerial != before.topologySerial ||
         memcmp( &verified, &candidate->dobjSummary, sizeof( verified ) ) ||
         memcmp( &before, &candidate->nativeState, sizeof( before ) ) ) return 0;
    *out = before;
    return 1;
}
