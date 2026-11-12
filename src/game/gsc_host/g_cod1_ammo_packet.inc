/* R145 checked static visual proposal. Native collision/handlers and the
 * later world producer remain separate from this readonly projection. */
static int AmmoPacketCurrent( const GscEntityRef *entity,
    const cod1AmmoSpawnNativeState_t *state ) {
    const Cod1AmmoCandidate *candidate;
    if ( !AmmoDObjQueryStageValid( entity ) ) return 0;
    candidate = &cod1AmmoCandidates[entity->slot];
    return candidate->nativeStatePresent && !candidate->nativeStateBusy &&
        candidate->dobjPresent && candidate->dobjSummary.topologySerial == state->topologySerial &&
        !memcmp( &candidate->nativeState, state, sizeof( *state ) );
}

int G_Cod1ProjectAmmoSpawnPacket( const GscEntityRef *input,
    int networkModelSlot, entityState_t *out, char *error, size_t size ) {
    GscEntityRef entity;
    cod1AmmoSpawnNativeState_t state, verified;
    cod1AmmoSpawnModel_t model;
    entityState_t packet;
    char path[MAX_STRING_CHARS];
    int i;
    if ( !ActorArguments( input, sizeof( *input ), out, sizeof( *out ), error, size ) ) return 0;
    if ( !input || !out || networkModelSlot < 1 || networkModelSlot >= MAX_MODELS ) goto refuse;
    entity = *input;
    if ( !G_Cod1ReadAmmoSpawnNativeState( &entity, &state ) ||
         !G_Cod1ReadAmmoSpawnCandidate( &entity, &model ) ||
         state.nativeGroundEntity != 2047 || state.nativeKind != 2 ||
         !CAN_BuildPacket( (int)entity.slot, (int)state.nativeItemIndex, networkModelSlot,
             state.descriptor_fingerprint, state.position.base, state.angles.base, &packet ) ) goto refuse;
    /* Both domains are checked: copied GAME state against the actual native
     * bank/serial, and the caller's independent current network model path.
     * Reads have no production side effects; fixture callbacks are adversarial
     * boundaries, so repeat all checks before returning the copied proposal. */
    for ( i = 0; i < 2; ++i ) {
        if ( !AmmoPacketCurrent( &entity, &state ) ) goto refuse;
        memset( path, 0, sizeof( path ) );
        trap_GetConfigstring( CS_MODELS + networkModelSlot, path, sizeof( path ) );
        if ( !AmmoPacketCurrent( &entity, &state ) || !memchr( path, 0, sizeof( path ) ) ||
             Q_stricmp( path, model.init.item.models[0] ) ||
             !G_Cod1ReadAmmoSpawnNativeState( &entity, &verified ) ||
             memcmp( &state, &verified, sizeof( state ) ) ) goto refuse;
    }
    memset( path, 0, sizeof( path ) );
    trap_GetConfigstring( CS_MODELS + networkModelSlot, path, sizeof( path ) );
    if ( !AmmoPacketCurrent( &entity, &state ) || !memchr( path, 0, sizeof( path ) ) ||
         Q_stricmp( path, model.init.item.models[0] ) ||
         !G_Cod1ReadAmmoSpawnNativeState( &entity, &verified ) ||
         memcmp( &state, &verified, sizeof( state ) ) ) goto refuse;
    *out = packet;
    return 1;
refuse:
    EntityError( error, size, "ammo packet requires current native state, unbound ground and exact network XModel binding" );
    return 0;
}
