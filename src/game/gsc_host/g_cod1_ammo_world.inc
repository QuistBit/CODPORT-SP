/* R146 static visual publication, with generation-keyed world cleanup.
 * Collision/handlers/pickup and VM ammo constructors remain unselected. */
static int AmmoWorldBodyCurrent( const GscEntityRef *entity ) {
    const Cod1AmmoCandidate *candidate;
    const gentity_t *ent;
    if ( !AmmoCandidateOwned( entity ) || !G_Cod1EntitySameAllocation( entity ) ) return 0;
    candidate = &cod1AmmoCandidates[entity->slot]; ent = &g_entities[entity->slot];
    return candidate->worldPresent && candidate->worldBusy != 2 && ent->r.linked &&
        ent->r.svFlags == SVF_USE_CURRENT_ORIGIN && !ent->r.bmodel && !ent->r.contents &&
        !memcmp( &ent->s, &candidate->worldPacket, sizeof( ent->s ) ) &&
        !memcmp( ent->r.currentOrigin, candidate->worldPacket.origin, sizeof( ent->r.currentOrigin ) ) &&
        !memcmp( ent->r.currentAngles, candidate->worldPacket.angles, sizeof( ent->r.currentAngles ) );
}

static int AmmoWorldCurrent( const GscEntityRef *entity, int busy ) {
    return AmmoWorldBodyCurrent( entity ) && cod1AmmoCandidates[entity->slot].worldBusy == busy;
}

static int AmmoWorldUnpublishOwner( const GscEntityRef *entity, int pending,
    char *error, size_t size ) {
    Cod1AmmoCandidate *candidate;
    gentity_t *ent;
    int oldBusy;
    if ( !AmmoCandidateOwned( entity ) ) return 1;
    if ( !G_Cod1EntitySameAllocation( entity ) ) {
        EntityError( error, size, "ammo world cleanup allocation is stale" ); return 0;
    }
    candidate = &cod1AmmoCandidates[entity->slot];
    if ( candidate->worldBusy == 2 || ( candidate->worldBusy && !pending ) ) {
        EntityError( error, size, "ammo world operation is already in progress" ); return 0;
    }
    if ( !candidate->worldPresent ) return 1;
    oldBusy = candidate->worldBusy; candidate->worldBusy = 2;
    ent = &g_entities[entity->slot];
    /* Hide before the boundary. G_FreeEntity already unlinked; Born may have
     * changed classname to noclass. Cleanup depends only on the old identity. */
    ent->r.svFlags |= SVF_NOCLIENT;
    if ( ent->r.linked ) trap_UnlinkEntity( ent );
    if ( !AmmoCandidateOwned( entity ) || !G_Cod1EntitySameAllocation( entity ) ) {
        EntityError( error, size, "ammo allocation changed during world cleanup" ); return 0;
    }
    candidate = &cod1AmmoCandidates[entity->slot];
    candidate->worldBusy = oldBusy;
    ent = &g_entities[entity->slot];
    if ( ent->r.linked ) {
        ent->r.svFlags |= SVF_NOCLIENT;
        EntityError( error, size, "ammo unlink failed; hidden world ownership retained" ); return 0;
    }
    ent->s = candidate->stagedState;
    ent->r.svFlags = candidate->stagedServerFlags | SVF_NOCLIENT;
    VectorCopy( candidate->stagedState.origin, ent->r.currentOrigin );
    VectorCopy( candidate->stagedAngles, ent->r.currentAngles );
    candidate->worldPresent = 0;
    memset( &candidate->worldPacket, 0, sizeof( candidate->worldPacket ) );
    memset( &candidate->stagedState, 0, sizeof( candidate->stagedState ) );
    candidate->stagedServerFlags = 0; VectorClear( candidate->stagedAngles );
    return 1;
}

static int AmmoWorldSceneCleanup( void ) {
    uint64_t epoch = cod1EntityEpoch;
    int active = cod1EntityActive, i, status;
    GscEntityRef entity;
    for ( i = 0; i < MAX_GENTITIES; ++i ) {
        if ( cod1AmmoCandidates[i].entity.epoch != epoch ||
             ( !cod1AmmoCandidates[i].worldPresent && !cod1AmmoCandidates[i].worldBusy ) ) continue;
        entity = cod1AmmoCandidates[i].entity;
        status = AmmoWorldUnpublishOwner( &entity, 1, NULL, 0 );
        if ( cod1EntityEpoch != epoch || cod1EntityActive != active || !status ) return 0;
    }
    return 1;
}

int G_Cod1PublishAmmoSpawn( const GscEntityRef *input,
    int networkModelSlot, char *error, size_t size ) {
    GscEntityRef entity;
    entityState_t packet, verified;
    Cod1AmmoCandidate *candidate;
    gentity_t *ent;
    if ( !ActorArguments( input, sizeof( *input ), NULL, 0, error, size ) ) return 0;
    if ( !input ) { EntityError( error, size, "ammo publication requires an owned candidate" ); return 0; }
    entity = *input;
    if ( !AmmoDObjStageValid( &entity, 0 ) ) goto refuse;
    candidate = &cod1AmmoCandidates[entity.slot]; candidate->worldBusy = 1;
    if ( !G_Cod1ProjectAmmoSpawnPacket( &entity, networkModelSlot, &packet, error, size ) ||
         !AmmoCandidateOwned( &entity ) || !G_Cod1EntitySameAllocation( &entity ) ) goto rollback;
    candidate = &cod1AmmoCandidates[entity.slot];
    if ( candidate->worldBusy != 1 || candidate->worldPresent ||
         cod1ActorEngineSceneBusyEpoch == entity.epoch ) goto rollback;
    ent = &g_entities[entity.slot];
    candidate->stagedState = ent->s; candidate->stagedServerFlags = ent->r.svFlags;
    VectorCopy( ent->r.currentAngles, candidate->stagedAngles );
    candidate->worldPacket = packet; candidate->worldPresent = 1;
    ent->s = packet; ent->r.svFlags = SVF_USE_CURRENT_ORIGIN;
    VectorCopy( packet.origin, ent->r.currentOrigin ); VectorCopy( packet.angles, ent->r.currentAngles );
    trap_LinkEntity( ent );
    /* Real SV_LinkEntity may return unlinked when no BSP leaf is inside the
     * world. A callback may retire/reuse this slot or replace the map. */
    if ( !AmmoWorldCurrent( &entity, 1 ) ||
         !G_Cod1ProjectAmmoSpawnPacket( &entity, networkModelSlot, &verified, error, size ) ||
         !AmmoWorldCurrent( &entity, 1 ) || memcmp( &packet, &verified, sizeof( packet ) ) ) goto rollback;
    cod1AmmoCandidates[entity.slot].worldBusy = 0;
    return 1;
rollback:
    if ( AmmoCandidateOwned( &entity ) && G_Cod1EntitySameAllocation( &entity ) ) {
        if ( !AmmoWorldUnpublishOwner( &entity, 1, NULL, 0 ) ) {
            if ( AmmoCandidateOwned( &entity ) && G_Cod1EntitySameAllocation( &entity ) &&
                 cod1AmmoCandidates[entity.slot].worldBusy == 1 ) cod1AmmoCandidates[entity.slot].worldBusy = 0;
            EntityError( error, size, "ammo publication refused; hidden world cleanup ownership retained" ); return 0;
        }
        if ( AmmoCandidateOwned( &entity ) && G_Cod1EntitySameAllocation( &entity ) &&
             cod1AmmoCandidates[entity.slot].worldBusy == 1 ) cod1AmmoCandidates[entity.slot].worldBusy = 0;
    }
refuse:
    EntityError( error, size, "ammo world publication lost its staged body, bank, binding or allocation" ); return 0;
}

int G_Cod1UnpublishAmmoSpawn( const GscEntityRef *input, char *error, size_t size ) {
    GscEntityRef entity;
    if ( !ActorArguments( input, sizeof( *input ), NULL, 0, error, size ) ) return 0;
    if ( !input ) { EntityError( error, size, "ammo unpublish requires an owned candidate" ); return 0; }
    entity = *input;
    if ( !AmmoCandidateOwned( &entity ) || !G_Cod1EntitySameAllocation( &entity ) ) {
        EntityError( error, size, "ammo unpublish allocation is stale or unowned" ); return 0;
    }
    return AmmoWorldUnpublishOwner( &entity, 0, error, size );
}

int G_Cod1ReadAmmoSpawnWorld( const GscEntityRef *input, entityState_t *out ) {
    GscEntityRef entity;
    entityState_t before, verified;
    if ( !input || !out ) return 0;
    entity = *input;
    if ( !AmmoWorldCurrent( &entity, 0 ) ) return 0;
    before = cod1AmmoCandidates[entity.slot].worldPacket;
    if ( !G_Cod1ProjectAmmoSpawnPacket( &entity, before.modelindex, &verified, NULL, 0 ) ||
         !AmmoWorldCurrent( &entity, 0 ) ||
         memcmp( &before, &cod1AmmoCandidates[entity.slot].worldPacket, sizeof( before ) ) ||
         memcmp( &before, &verified, sizeof( before ) ) ) return 0;
    *out = before;
    return 1;
}
