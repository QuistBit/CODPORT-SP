/* R206 exact owned static-ammo angular mutation. Native setter 20038a20
 * clears angular trajectory metadata/delta and changes base/current angles,
 * with no link. Portable stationary projection remains a selected port lane.
 * The dedicated guard survives map callbacks until its stack frame exits;
 * native/world readers remain usable while the angle operation is guarded. */
static struct {
    int active, cancelled;
    uint64_t serial;
    GscEntityRef entity;
} cod1AmmoAnglesGuard;
static uint64_t cod1AmmoAnglesSerial;

static int AmmoAnglesPrivate( const void *p, size_t n ) {
    return ActorOverlap( p, n, g_entities, sizeof( g_entities ) ) ||
        ( level.clients && level.maxclients > 0 && level.maxclients <= MAX_CLIENTS &&
          ActorOverlap( p, n, level.clients, sizeof( gclient_t ) * (size_t)level.maxclients ) ) ||
        ActorOverlap( p, n, &level, sizeof( level ) ) ||
        ActorOverlap( p, n, &cod1EntityEpoch, sizeof( cod1EntityEpoch ) ) ||
        ActorOverlap( p, n, cod1EntityGeneration, sizeof( cod1EntityGeneration ) ) ||
        ActorOverlap( p, n, cod1EntityRetired, sizeof( cod1EntityRetired ) ) ||
        ActorOverlap( p, n, &cod1EntityReady, sizeof( cod1EntityReady ) ) ||
        ActorOverlap( p, n, &cod1EntityActive, sizeof( cod1EntityActive ) ) ||
        ActorOverlap( p, n, &cod1ActorEngineSceneBusyEpoch, sizeof( cod1ActorEngineSceneBusyEpoch ) ) ||
        ActorOverlap( p, n, &cod1AmmoCandidateBusy, sizeof( cod1AmmoCandidateBusy ) ) ||
        ActorOverlap( p, n, &cod1AmmoCandidateCancelled, sizeof( cod1AmmoCandidateCancelled ) ) ||
        ActorOverlap( p, n, &cod1AmmoSpawnModelBusy, sizeof( cod1AmmoSpawnModelBusy ) ) ||
        ActorOverlap( p, n, &cod1AmmoSpawnModelCancelled, sizeof( cod1AmmoSpawnModelCancelled ) ) ||
        ActorOverlap( p, n, cod1AmmoCandidates, sizeof( cod1AmmoCandidates ) ) ||
        ActorOverlap( p, n, cod1Actors, sizeof( cod1Actors ) ) ||
        ActorOverlap( p, n, cod1Sentients, sizeof( cod1Sentients ) ) ||
        ActorOverlap( p, n, cod1ActorByEntity, sizeof( cod1ActorByEntity ) ) ||
        ActorOverlap( p, n, cod1SentientByEntity, sizeof( cod1SentientByEntity ) ) ||
        ActorOverlap( p, n, &cod1ActorCounter, sizeof( cod1ActorCounter ) ) ||
        ActorOverlap( p, n, &cod1SentientCounter, sizeof( cod1SentientCounter ) ) ||
        ActorOverlap( p, n, &cod1ActorSerial, sizeof( cod1ActorSerial ) ) ||
        ActorOverlap( p, n, &cod1ActorCriticalDisabled, sizeof( cod1ActorCriticalDisabled ) ) ||
        ActorOverlap( p, n, &cod1ActorCriticalBusy, sizeof( cod1ActorCriticalBusy ) ) ||
        ActorOverlap( p, n, cod1EntityNativeStatic, sizeof( cod1EntityNativeStatic ) ) ||
        ActorOverlap( p, n, &cod1AmmoAnglesGuard, sizeof( cod1AmmoAnglesGuard ) ) ||
        ActorOverlap( p, n, &cod1AmmoAnglesSerial, sizeof( cod1AmmoAnglesSerial ) );
}

/* Pure, deliberately bounded ownership predicate. This is not an admission
 * of native item handlers or a relaxation of generic script/Actor writers. */
static int AmmoAnglesEligible( const GscEntityRef *entity ) {
    const Cod1AmmoCandidate *c;
    const gentity_t *e;
    int i;
    if ( !entity || !cod1EntityReady || level.spawning || level.restarted ||
         entity->slot < MAX_CLIENTS || entity->slot >= ENTITYNUM_MAX_NORMAL ||
         level.num_entities < MAX_CLIENTS || level.num_entities > ENTITYNUM_MAX_NORMAL ||
         !AmmoDObjQueryStageValid( entity ) ) return 0;
    c = &cod1AmmoCandidates[entity->slot]; e = &g_entities[entity->slot];
    if ( c->dobjBusy || c->nativeStateBusy || c->worldBusy ||
         !c->dobjPresent || !AmmoDObjSummaryValid( &c->dobjSummary ) || !c->nativeStatePresent ||
         c->nativeState.nativeKind != 2 ||
         ( c->worldPresent && c->nativeState.nativeGroundEntity != 2047 ) ||
         ActorRefEqual( entity, &cod1EntityNativeStatic[entity->slot].owner ) ||
         e->teamchain || e->teammaster || ( e->team && e->team[0] ) || e->tagParent || e->tagName ||
         e->aiName || e->aiCharacter || e->aiAttributes || e->AIScript_AlertEntity || e->aiInactive ||
         e->touch || e->use || e->think || e->nextthink ) return 0;
    for ( i = 0; i < level.num_entities; ++i )
        if ( g_entities[i].inuse && g_entities[i].tagParent == e ) return 0;
    return 1;
}

static int AmmoAnglesCurrent( const GscEntityRef *entity, uint64_t serial, int now,
    const Cod1AmmoCandidate *candidate, const gentity_t *body ) {
    return cod1AmmoAnglesGuard.active && !cod1AmmoAnglesGuard.cancelled &&
        cod1AmmoAnglesGuard.serial == serial && ActorRefEqual( entity, &cod1AmmoAnglesGuard.entity ) &&
        level.time == now && AmmoAnglesEligible( entity ) &&
        !memcmp( candidate, &cod1AmmoCandidates[entity->slot], sizeof( *candidate ) ) &&
        !memcmp( body, &g_entities[entity->slot], sizeof( *body ) );
}

static void AmmoAnglesPortable( entityState_t *packet, const float angles[3] ) {
    memset( &packet->apos, 0, sizeof( packet->apos ) );
    packet->apos.trType = TR_STATIONARY;
    memcpy( packet->apos.trBase, angles, sizeof( packet->apos.trBase ) );
    memcpy( packet->angles, angles, sizeof( packet->angles ) );
}

int G_Cod1SetAmmoSpawnAngles( const GscEntityRef *input, const float angles[3],
    char *error, size_t size ) {
    GscEntityRef entity;
    vec3_t value;
    Cod1AmmoCandidate before, *candidate;
    gentity_t body, *ent;
    cod1AmmoSpawnNativeState_t verified, next;
    entityState_t checked, packet, restored;
    uint64_t serial;
    int now, i, accepted = 0;
    /* A refusal must not use an aliased error buffer to damage its inputs or
     * an engine/private owner. Input values themselves are copied, not held. */
    if ( AmmoAnglesPrivate( error, size ) ||
         ActorOverlap( input, sizeof( *input ), error, size ) ||
         ActorOverlap( angles, sizeof( value ), error, size ) ) return 0;
    if ( cod1AmmoAnglesGuard.active ) {
        cod1AmmoAnglesGuard.cancelled = 1;
        EntityError( error, size, "ammo angular mutation reentered" ); return 0;
    }
    if ( !input || !angles ) {
        EntityError( error, size, "ammo angular mutation requires owner and finite angles" ); return 0;
    }
    entity = *input; memcpy( value, angles, sizeof( value ) );
    for ( i = 0; i < 3; ++i ) if ( !isfinite( value[i] ) ) {
        EntityError( error, size, "ammo angular mutation requires finite angles" ); return 0;
    }
    if ( cod1AmmoAnglesSerial == UINT64_MAX ) {
        EntityError( error, size, "ammo angular operation serial exhausted" ); return 0;
    }
    serial = ++cod1AmmoAnglesSerial;
    cod1AmmoAnglesGuard.active = 1; cod1AmmoAnglesGuard.cancelled = 0;
    cod1AmmoAnglesGuard.serial = serial; cod1AmmoAnglesGuard.entity = entity;
    /* Even the first native/world proof calls real DObj/configstring readers.
     * Claim the dedicated operation before those callbacks can reenter. */
    if ( !AmmoAnglesEligible( &entity ) ) goto done;
    now = level.time; before = cod1AmmoCandidates[entity.slot]; body = g_entities[entity.slot];
    if ( !G_Cod1ReadAmmoSpawnNativeState( &entity, &verified ) ||
         !AmmoAnglesCurrent( &entity, serial, now, &before, &body ) ||
         memcmp( &verified, &before.nativeState, sizeof( verified ) ) ) goto done;
    if ( before.worldPresent &&
         ( !G_Cod1ReadAmmoSpawnWorld( &entity, &checked ) ||
           !AmmoAnglesCurrent( &entity, serial, now, &before, &body ) ||
           memcmp( &checked, &before.worldPacket, sizeof( checked ) ) ) ) goto done;
    next = before.nativeState;
    memset( &next.angles, 0, sizeof( next.angles ) );
    memcpy( next.angles.base, value, sizeof( value ) );
    packet = body.s; AmmoAnglesPortable( &packet, value );
    restored = before.stagedState;
    if ( before.worldPresent ) {
        if ( !CAN_BuildPacket( (int)entity.slot, (int)next.nativeItemIndex,
                 before.worldPacket.modelindex, next.descriptor_fingerprint,
                 next.position.base, value, &packet ) ) goto done;
        AmmoAnglesPortable( &restored, value );
    }
    if ( !AmmoAnglesCurrent( &entity, serial, now, &before, &body ) ) goto done;
    /* All external boundaries are over. Commit copied angular state and every
     * portable/restoration image together; no trap runs after the first store. */
    candidate = &cod1AmmoCandidates[entity.slot]; ent = &g_entities[entity.slot];
    candidate->nativeState = next;
    ent->s.apos = packet.apos; memcpy( ent->s.angles, packet.angles, sizeof( ent->s.angles ) );
    memcpy( ent->r.currentAngles, value, sizeof( value ) );
    if ( before.worldPresent ) {
        candidate->worldPacket = packet;
        candidate->stagedState = restored;
        memcpy( candidate->stagedAngles, value, sizeof( value ) );
    }
    accepted = 1;
done:
    if ( cod1AmmoAnglesGuard.active && cod1AmmoAnglesGuard.serial == serial ) {
        cod1AmmoAnglesGuard.active = 0; cod1AmmoAnglesGuard.cancelled = 0;
    }
    if ( !accepted ) EntityError( error, size,
        "ammo angular mutation lost exact idle owner, topology, native state, binding or map" );
    return accepted;
}
