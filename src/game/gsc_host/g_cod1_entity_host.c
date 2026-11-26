/* R84/R85 bounded engine host for script entity references and access.
 * References are copies of an epoch, slot and allocation generation, never
 * engine pointers. This is an internal restoration API, not retail VM ABI.
 * Transform reads copy the current engine pose of any live entity. Writes
 * are limited to nonsolid script_model/script_origin; model assignment is
 * limited to script_model. Brush pushing, actor movement and mission native sets
 * require their own confirmed contracts. */
#include "../g_local.h"
#include "../g_cod1_entity_host.h"
#include "../g_cod1_vehicle.h"
#include "../g_cod1_turret_factory.h"
#include "../g_cod1_player_lifetime.h"
#include "../g_cod1_path_entity_host.h"
#include "../g_cod1_node_host.h"
#include "../g_cod1_actor_thread_authority.h"
#include "../g_cod1_yaw_motion.h"
#include "../bg_cod1_script_entity.h"
#if defined(COD1_GSC_TEST_WIP) && COD1_GSC_TEST_WIP
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "../bg_cod1_item_spawn.h"
#include "../bg_cod1_ammo_entity.h"
#include "../bg_cod1_ammo_registry.h" /* R207 source dependency; no gameplay admission. */
#include "../g_cod1_ammo_inventory.h"
#include "../g_cod1_item_lifecycle.h"
#include "../g_cod1_dropped_item.h"
#include "../g_cod1_grenade_player.h"

static uint64_t cod1EntityEpoch;
#include "g_cod1_anim_profile31.inc"
static uint32_t cod1EntityGeneration[MAX_GENTITIES];
static unsigned char cod1EntityRetired[MAX_GENTITIES];
static int cod1EntityReady, cod1EntityActive;
static int WeaponClassnamePrivate(const void*,size_t);
static int EntityLinkPrivate(const void*,size_t);
static void EntityLinksReset(void);
static void ModelPartReset(void);
static int EntityLinksRetire(const GscEntityRef*,int);
static int ActorFactoryPrivate(const void*,size_t);
static void ActorFactoryReset(void);
static int ActorFactoryCompletedTarget(const cod1ActorLease_t*,uint64_t,uint64_t);
static uint64_t cod1ActorEngineSceneBusyEpoch;
/* R204 selected native-free fence. Ordinary free outside the synchronous
 * transaction retains its existing contract. Implemented with the host below. */
static int DeleteTransactionFreeGuard(const GscEntityRef *);
static void DeleteTransactionOwnerReset(void);
static int RuntimeActorDeletePrivate(const void*,size_t);
static int DeleteStoragePrivate(const void*,size_t);
static void RuntimeActorDeleteReset(void);
static int RuntimeActorDeleteEntry(const GscEntityRef*,const cod1ActorLease_t*);
static int RuntimeActorDeleteQuiesce(const GscEntityRef*,const cod1ActorLease_t*,int(*)(void*),void*);
static void StartOriginResourcesReset(void);
static void TurretReset(void);
static int TurretActorPublisherPrivate(const void *,size_t);
static int ActorUnmountedPublisherPrivate(const void *,size_t);
static int ActorUnmountedTagPrivate(const void *,size_t);
static void ActorUnmountedTagReset(void);
static void ActorUnmountedPublisherReset(void);
static void ActorUnmountedPublisherRetire(const GscEntityRef *);
static int TurretShotPrivate(const void *,size_t);
static void TurretShotReset(void);
static int TurretActorTagPrivate(const void *,size_t);
static void TurretActorTagReset(void);
static int ActorFacePrivate(const void *,size_t);
static void ActorFaceReset(void);
static int ActorScriptedPrivate(const void *,size_t);
static void ActorScriptedReset(void);
static int ActorFaceFramePrivate(const void *,size_t);
static void ActorFaceFrameReset(void);
static void ActorFaceFrameRetire(uint32_t,const GscEntityRef*);
static int ActorFaceClockSelected(unsigned);
static int ActorFacePoseRead(const GscEntityRef*,GscStringCastValue*,char*,size_t);
static int ActorFaceTagPrivate(const void *,size_t);
static void ActorFaceTagReset(void);
static int ActorGenericPrivate(const void *,size_t);
static void ActorGenericReset(void);
static void ActorLocomotionReset(void);
static int ActorLocomotionPrivate(const void*,size_t);
static int FreshStoragePrivate(const void*,size_t);
static int ActorNormalPrivate(const void*,size_t);
static void ActorGenericRetire(uint32_t,const GscEntityRef*);
static int ActorGenericLookLoad(void);
static int ActorLookPrivate(const void *,size_t);
static void ActorLookReset(void);
static int ActorImmediatePrivate(const void *,size_t);
static int ActorStancePrivate(const void*,size_t);
static int ActorFloatPrivate(const void*,size_t);
static int MoveDeltaGamePrivate(const void*,size_t);
static void MoveDeltaBankReset(void);
#include "../cod1_gsc/stance-profile.h"
static void ActorImmediateReset(void);
static void TurretActorPublisherReset(void);
static void TurretActorPublisherRetire(const GscEntityRef *);
static int TurretActorPublish(const GscEntityRef *,int,int(*)(void*),void*);
static int TurretMountedPrivate(const void *,size_t);
static void TurretMountedReset(void);
static void TurretMountedRetire(const GscEntityRef *);
static int TurretFitPrivate(const void *,size_t);
static void TurretFitReset(void);
static void TurretFitRetire(const GscEntityRef *);
static int TurretAnimationPrivate(const void *,size_t);
static void TurretAnimationReset(void);
static void TurretAnimationRetire(const GscEntityRef *);
static int TurretMountPrivate(const void *,size_t);
static void TurretMountReset(void);
static void TurretMountFresh(uint32_t,uint64_t);
static void TurretMountRetire(const GscEntityRef *);
static int TurretStoragePrivate(const void *,size_t);

typedef struct {
    GscEntityRef entity;
    cod1AmmoSpawnModel_t model;
    int dobjPresent, dobjBusy;
    cod1DObjSummary_t dobjSummary;
    int nativeStatePresent, nativeStateBusy;
    cod1AmmoSpawnNativeState_t nativeState;
    int worldPresent, worldBusy;
    entityState_t worldPacket, stagedState;
    int stagedServerFlags;
    vec3_t stagedAngles;
} Cod1AmmoCandidate;
static Cod1AmmoCandidate cod1AmmoCandidates[MAX_GENTITIES];
static int cod1AmmoCandidateBusy, cod1AmmoCandidateCancelled;
static int AmmoDObjFreeOwner( const GscEntityRef *entity, int pending,
    char *error, size_t size );
static void AmmoNativeStateInvalidate( const GscEntityRef *entity );
static int AmmoWorldBodyCurrent( const GscEntityRef *entity );
static int AmmoWorldUnpublishOwner( const GscEntityRef *entity, int pending,
    char *error, size_t size );
static int AmmoWorldSceneCleanup( void );

static int AmmoCandidateOwned( const GscEntityRef *entity ) {
    const GscEntityRef *owner;
    if ( !entity || !entity->epoch || !entity->generation || entity->slot >= MAX_GENTITIES ) return 0;
    owner = &cod1AmmoCandidates[entity->slot].entity;
    return owner->epoch == entity->epoch && owner->slot == entity->slot &&
        owner->generation == entity->generation;
}

static void AmmoCandidateRetire( const GscEntityRef *entity ) {
    if ( AmmoCandidateOwned( entity ) )
        memset( &cod1AmmoCandidates[entity->slot], 0, sizeof( cod1AmmoCandidates[0] ) );
}

static int EntityDiagnosticsEnabled( void ) {
    int mode = trap_Cvar_VariableIntegerValue( "g_cod1GscEntityTest" );
    return mode == 1 || mode == 2 || mode == 3 || mode == 5 || mode == 6 || mode == 7 || mode == 8 || mode == 9 || mode == 10 || mode == 11 || mode == 12 || mode == 13 || mode == 15 || mode == 16 || mode == 17 || mode == 18 || (mode == 19 || mode == 20);
}

static void EntityError( char *error, size_t size, const char *text ) {
    if ( error && size ) {
        size_t n = strlen( text );
        if ( n >= size ) n = size - 1;
        memcpy( error, text, n );
        error[n] = '\0';
    }
}

#include "g_cod1_actor_registry.inc"
#include "g_cod1_script_model_thread.inc"
#include "g_cod1_actor_critical.inc"
#include "g_cod1_actor_scripted_queue.inc"
#include "g_cod1_actor_presence.inc"
#include "g_cod1_actor_goal.inc"
#include "g_cod1_actor_postinit_position.inc"
#include "g_cod1_actor_postinit_placed_target.inc"
static int EntityValidate(void *user,const GscEntityRef *ref);
#include "g_cod1_entity_native_static.inc"
#include "g_cod1_placed_actor_intake.inc"
#include "g_cod1_actor_spatial.inc"
#include "g_cod1_actor_path_contents.inc"
#include "g_cod1_actor_dobj.inc"
#include "g_cod1_actor_motion.inc"
#include "g_cod1_actor_model_registry.inc"
#include "g_cod1_actor_model_apply.inc"
#include "g_cod1_actor_registered_attachments.inc"
#include "g_cod1_model_string.inc"
#include "g_cod1_actor_model_methods.inc"
#include "g_cod1_tag_fx_provider.inc"

static int ActorEngineSceneCleanup( void ) {
    uint64_t epoch = cod1EntityEpoch;
    int active = cod1EntityActive;
    int status, i;
    if ( epoch && cod1ActorEngineSceneBusyEpoch == epoch ) {
        /* A reset callback may have explicitly freed every old lease first.
         * No old engine ownership then remains, so a nested map incarnation
         * needs no second engine reset. Otherwise preserve the busy guard. */
        for ( i = 0; i < COD1_ACTOR_CAPACITY; ++i )
            if ( cod1Actors[i].activation >= 0 &&
                 ( cod1Actors[i].dobj.present || cod1Actors[i].dobj.busy ||
                   cod1Actors[i].spatial.linked || cod1Actors[i].spatial.busy ) ) return 0;
        for ( i = 0; i < MAX_GENTITIES; ++i )
            if ( cod1AmmoCandidates[i].entity.epoch == epoch &&
                 ( cod1AmmoCandidates[i].dobjPresent || cod1AmmoCandidates[i].dobjBusy ||
                   cod1AmmoCandidates[i].worldPresent || cod1AmmoCandidates[i].worldBusy ) ) return 0;
        return 1;
    }
    cod1ActorEngineSceneBusyEpoch = epoch;
    status = AmmoWorldSceneCleanup();
    if ( cod1EntityEpoch == epoch && cod1EntityActive == active && status )
        status = ActorDObjSceneCleanup();
    else status = 0;
    if ( cod1EntityEpoch == epoch && cod1EntityActive == active && status )
        status = ActorSpatialSceneCleanup();
    else status = 0;
    if ( cod1ActorEngineSceneBusyEpoch == epoch ) cod1ActorEngineSceneBusyEpoch = 0;
    return status;
}

void G_Cod1EntityMapBegin( void ) {
    if(!G_Cod1TurretFactoryMapCleanup()||!G_Cod1VehicleMapCleanup()){G_Error("vehicle cleanup refused at map boundary");return;}
    MoveDeltaBankReset();
    G_Cod1PlayerLifetimeMapReset();
    if(!G_Cod1LoopSoundMapCleanup()||!G_Cod1LoopFxMapCleanup()){G_Error("R243 loop FX owner cleanup refused before map boundary");return;}
    uint64_t previousEpoch = cod1EntityEpoch;
    int previousActive = cod1EntityActive;
    int cleaned = ActorEngineSceneCleanup();
    if ( cod1EntityEpoch != previousEpoch || cod1EntityActive != previousActive ) return;
    if ( !cleaned ) { G_Error( "R146: native entity scene cleanup failed before map begin" ); return; }
    EntityLinksReset();ModelPartReset();
    memset(scriptModelThreads,0,sizeof(scriptModelThreads));
    ActorPoolReset();
    TurretReset();
    StartOriginResourcesReset();
    DeleteTransactionOwnerReset();
    G_Cod1ItemLifecycleReset();
    G_Cod1DroppedItemsReset();
    G_Cod1YawMotionReset();
    NativeStaticMapReset();
    memset( cod1AmmoCandidates, 0, sizeof( cod1AmmoCandidates ) );
    cod1EntityReady = 0;
    cod1EntityActive = 0;
    if ( cod1EntityEpoch == UINT64_MAX ) {
        /* Never wrap: no reference may alias an earlier map incarnation. */
        return;
    }
    ++cod1EntityEpoch;
    cod1EntityActive = cod1EntityEpoch != UINT64_MAX;
    memset( cod1EntityGeneration, 0, sizeof( cod1EntityGeneration ) );
    memset( cod1EntityRetired, 0, sizeof( cod1EntityRetired ) );
}

void G_Cod1EntityMapReady( void ) {
    cod1EntityReady = cod1EntityActive && cod1EntityEpoch && cod1EntityEpoch != UINT64_MAX &&
        G_Cod1MapEntities() && g_gametype.integer == GT_SINGLE_PLAYER;
}

void G_Cod1EntityMapEnd( void ) {
    if(!G_Cod1TurretFactoryMapCleanup()||!G_Cod1VehicleMapCleanup()){G_Error("vehicle cleanup refused at map boundary");return;}
    MoveDeltaBankReset();
    G_Cod1PlayerLifetimeMapReset();
    if(!G_Cod1LoopSoundMapCleanup()||!G_Cod1LoopFxMapCleanup()){G_Error("R243 loop FX owner cleanup refused before map boundary");return;}
    uint64_t previousEpoch = cod1EntityEpoch;
    int previousActive = cod1EntityActive;
    int cleaned = ActorEngineSceneCleanup();
    if ( cod1EntityEpoch != previousEpoch || cod1EntityActive != previousActive ) return;
    if ( !cleaned ) { G_Error( "R146: native entity scene cleanup failed before map end" ); return; }
    G_Cod1NodeRegistryClear();
    EntityLinksReset();ModelPartReset();
    memset(scriptModelThreads,0,sizeof(scriptModelThreads));
    ActorPoolReset();
    TurretReset();
    StartOriginResourcesReset();
    DeleteTransactionOwnerReset();
    G_Cod1ItemLifecycleReset();
    G_Cod1DroppedItemsReset();
    G_Cod1YawMotionReset();
    NativeStaticMapReset();
    memset( cod1AmmoCandidates, 0, sizeof( cod1AmmoCandidates ) );
    cod1EntityReady = 0;
    cod1EntityActive = 0;
}

void G_Cod1EntityBorn( gentity_t *ent ) {
    int slot;
    GscEntityRef previous;
    if ( !ent ) return;
    slot = ent->s.number;
    if ( slot < 0 || slot >= MAX_GENTITIES || ent != &g_entities[slot] ) return;
    previous.epoch = cod1EntityEpoch;
    previous.slot = (uint32_t)slot;
    previous.generation = cod1EntityGeneration[slot];
    if(!G_Cod1TurretFactoryBeforeFree(&previous)||!G_Cod1VehicleBeforeFree(&previous)){G_Error("vehicle cleanup refused before allocation birth");return;}
    G_Cod1PlayerLifetimeBeforeBirth(ent,&previous);
    {
        int previousActive = cod1EntityActive;
        int cleaned = ActorDObjFreeOwner( &previous );
        if ( cod1EntityEpoch != previous.epoch || cod1EntityActive != previousActive ||
             cod1EntityGeneration[slot] != previous.generation ) return;
        if ( !cleaned ) { G_Error( "R104: native actor DObj cleanup failed before allocation birth" ); return; }
        cleaned = ActorSpatialUnlinkOwner( &previous );
        if ( cod1EntityEpoch != previous.epoch || cod1EntityActive != previousActive ||
             cod1EntityGeneration[slot] != previous.generation ) return;
        if ( !cleaned ) { G_Error( "R103: native actor cleanup failed before allocation birth" ); return; }
        cleaned = AmmoDObjFreeOwner( &previous, 1, NULL, 0 );
        if ( cod1EntityEpoch != previous.epoch || cod1EntityActive != previousActive ||
             cod1EntityGeneration[slot] != previous.generation ) return;
        if ( !cleaned ) { G_Error( "R143: ammo DObj cleanup failed before allocation birth" ); return; }
    }
    if(!EntityLinksRetire(&previous,1)){G_Error("CoD structural link cleanup refused before birth");return;}
    ActorRetireEntity( &previous );
    AmmoCandidateRetire( &previous );
    NativeStaticDetachAndRetire(&previous);
    if ( cod1EntityGeneration[slot] == UINT32_MAX ) {
        /* An exhausted slot stays retired until a different map epoch. */
        cod1EntityRetired[slot] = 1;
        return;
    }
    ++cod1EntityGeneration[slot];
    G_Cod1PlayerLifetimeAfterBirth(ent);
}

int G_Cod1EntityFree( const GscEntityRef *input ) {
    GscEntityRef entity;
    if ( !input ) return 0;
    entity = *input;
    if(G_Cod1EntitySameAllocation(&entity))G_Cod1PlayerLifetimeRetire(g_entities+entity.slot);
    if ( !G_Cod1EntitySameAllocation( &entity ) || !DeleteTransactionFreeGuard(&entity) ||
         !G_Cod1TurretFactoryBeforeFree(&entity) || !G_Cod1VehicleBeforeFree(&entity) ||
         !G_Cod1EntityLinksBeforeFree(&entity) ||
         !ActorDObjFreeOwner( &entity ) ||
         !G_Cod1EntitySameAllocation( &entity ) ||
         g_entities[entity.slot].neverFree ||
         !AmmoDObjFreeOwner( &entity, 1, NULL, 0 ) ||
         !G_Cod1EntitySameAllocation( &entity ) ||
         g_entities[entity.slot].neverFree || !DeleteTransactionFreeGuard(&entity) ) return 0;
    ActorMotionStopOwner(&entity);
    if(!G_Cod1EntitySameAllocation(&entity))return 0;
    ActorRetireEntity( &entity );
    AmmoCandidateRetire( &entity );
    NativeStaticDetachAndRetire(&entity);
    return 1;
}

uint64_t G_Cod1EntityMapEpoch( void ) {
    return cod1EntityActive ? cod1EntityEpoch : 0;
}

/* Trigger initialization happens before MapReady. These identity-only helpers
 * protect that phase too; VM access continues to require a ready map. */
int G_Cod1EntitySameAllocation( const GscEntityRef *ref ) {
    const gentity_t *ent;
    if ( !cod1EntityActive || !ref || !ref->epoch || !ref->generation ||
         ref->epoch != cod1EntityEpoch || ref->slot >= MAX_GENTITIES ||
         cod1EntityRetired[ref->slot] ||
         ref->generation != cod1EntityGeneration[ref->slot] ||
         ref->slot >= (uint32_t)level.num_entities ) return 0;
    ent = &g_entities[ref->slot];
    return ent->inuse && ent->s.number == (int)ref->slot;
}

int G_Cod1EntityCapture( const gentity_t *ent, GscEntityRef *out ) {
    GscEntityRef ref;
    if ( !out ) return 0;
    memset( out, 0, sizeof( *out ) );
    if ( !ent || ent->s.number < 0 || ent->s.number >= MAX_GENTITIES ||
         ent != &g_entities[ent->s.number] ) return 0;
    ref.epoch = cod1EntityEpoch;
    ref.slot = (uint32_t)ent->s.number;
    ref.generation = cod1EntityGeneration[ref.slot];
    if ( !G_Cod1EntitySameAllocation( &ref ) ) return 0;
    *out = ref;
    return 1;
}


/* Pure admission for the separately selected R113 blast helper. This is an
 * intentionally narrower port domain, not retail entity+0x10c classification.
 * A native actor/sentient reservation cannot be treated as an ordinary model.
 * Client eye/head-tag/capsule semantics require their own confirmed provider. */
int G_Cod1ScriptBlastNonActor(const gentity_t *entity) {
    GscEntityRef ref;
    if (!G_Cod1EntityCapture(entity,&ref)) return 0;
    return !entity->client && entity->s.eType!=ET_PLAYER &&
        !(entity->r.svFlags&SVF_CASTAI) &&
        !cod1ActorByEntity[ref.slot] && !cod1SentientByEntity[ref.slot];
}

static int EntityValidate( void *user, const GscEntityRef *ref ) {
    (void)user;
    return cod1EntityReady && G_Cod1EntitySameAllocation( ref );
}

static int EntityLookup( void *user, const char *name, const char *field,
                         GscEntityRef *out, int *found,
                         char *error, size_t size ) {
    int i, match = -1, classname;
    GscEntityRef ref;
    (void)user;
    if ( !cod1EntityReady || !name || !field || !out || !found ) {
        EntityError( error, size, "entity host requires a ready fresh CoD SP map" );
        return 0;
    }
    classname=!strcmp(field,"classname");
    if ( !classname && strcmp( field, "targetname" ) ) {
        EntityError( error, size, "getent field is outside the selected native string-field domain" );
        return 0;
    }
    /* Exact author spelling, unique match. Do not silently choose one of a
     * collection that needs getentarray; that API is not part of this stage. */
    for ( i = 0; i < level.num_entities && i < MAX_GENTITIES; ++i ) {
        const gentity_t *ent = &g_entities[i];
        const char *text=classname?ent->classname:ent->targetname;
        if ( !ent->inuse || !text || strcmp( text, name ) ) continue;
        if ( match >= 0 ) {
            EntityError( error, size, "getent string-field match is ambiguous" );
            return 0;
        }
        match = i;
    }
    if ( match < 0 ) {
        memset( out, 0, sizeof( *out ) );
        *found = 0;
        return 1;
    }
    ref.epoch = cod1EntityEpoch;
    ref.slot = (uint32_t)match;
    ref.generation = cod1EntityGeneration[match];
    if ( !EntityValidate( NULL, &ref ) ) {
        EntityError( error, size, "entity has no current allocation identity" );
        return 0;
    }
    *out = ref;
    *found = 1;
    return 1;
}

static int EntitySetOrigin( void *user, const GscEntityRef *ref,
                            const float origin[3], char *error, size_t size ) {
    gentity_t *ent;
    vec3_t position;
    int i;
    (void)user;
    if ( G_Cod1ItemLifecycleOwns(ref) )
        return G_Cod1ItemLifecycleSetOrigin(ref,origin,error,size);
    if ( !EntityValidate( NULL, ref ) ) {
        EntityError( error, size, "setorigin received a stale entity reference" );
        return 0;
    }
    ent = &g_entities[ref->slot];
    if ( !origin || !ent->classname || ent->r.contents || ent->r.bmodel ||
         ent->s.eType != ET_GENERAL ||
         ( strcmp( ent->classname, "script_origin" ) &&
           strcmp( ent->classname, "script_model" ) ) ) {
        EntityError( error, size, "R84 setorigin requires a nonsolid script entity" );
        return 0;
    }
    for ( i = 0; i < 3; ++i ) {
        if ( !isfinite( origin[i] ) ) {
            EntityError( error, size, "setorigin requires three finite coordinates" );
            return 0;
        }
        position[i] = origin[i];
    }
    G_SetOrigin( ent, position );
    VectorCopy( position, ent->s.origin );
    trap_LinkEntity( ent );
    if ( !EntityValidate( NULL, ref ) ) {
        EntityError( error, size, "entity retired during position update" );
        return 0;
    }
    if ( EntityDiagnosticsEnabled() ) {
        G_Printf( "[GSC R84] setorigin entity=%u generation=%u xyz=%.9g,%.9g,%.9g\n",
            (unsigned)ref->slot, (unsigned)ref->generation,
            (double)position[0], (double)position[1], (double)position[2] );
    }
    return 1;
}

int G_Cod1PrepareAmmoSpawn( const char *classname, const float origin[3],
                          int32_t flags, cod1AmmoSpawnInit_t *out,
                          char *error, size_t size ) {
    if ( !cod1EntityReady || level.spawning || !out ) {
        EntityError( error, size, "ammo preparation requires a ready CoD SP map" );
        return 0;
    }
    if ( !CIS_PrepareAmmoSpawn( classname, origin, flags, out ) ) {
        EntityError( error, size, "ammo preparation requires an exact native ammo class and finite origin" );
        return 0;
    }
    return 1;
}

static int cod1AmmoSpawnModelBusy, cod1AmmoSpawnModelCancelled;

static int AmmoSpawnModelMapCurrent( const cod1AmmoSpawnModel_t *model ) {
    cod1ItemDescriptor_t current;
    return model && cod1EntityReady && !level.spawning && !level.restarted &&
        G_Cod1MapEntities() && g_gametype.integer == GT_SINGLE_PLAYER &&
        model->map_epoch && model->map_epoch == G_Cod1EntityMapEpoch() &&
        model->descriptor_fingerprint && model->descriptor_fingerprint == CI_Fingerprint() &&
        CI_Descriptor( model->init.item.index, &current ) && !current.dynamic &&
        current.type == 2 && !memcmp( &current, &model->init.item, sizeof( current ) );
}

int G_Cod1AmmoSpawnModelCurrent( const cod1AmmoSpawnModel_t *model ) {
    return model && model->server_model_slot > 0 &&
        model->server_model_slot <= COD1_PRECACHE_MODELS && AmmoSpawnModelMapCurrent( model );
}

int G_Cod1RegisterAmmoSpawnModel( const char *classname, const float origin[3],
                                int32_t flags, cod1AmmoSpawnModel_t *out,
                                char *error, size_t size ) {
    cod1AmmoSpawnModel_t model;
    cod1ModelPrecache_t request;
    int accepted;
    if ( cod1AmmoSpawnModelBusy ) {
        cod1AmmoSpawnModelCancelled = 1;
        EntityError( error, size, "ammo model registration reentered" );
        return 0;
    }
    if ( !out ) {
        EntityError( error, size, "ammo model registration requires output" );
        return 0;
    }
    memset( &model, 0, sizeof( model ) );
    if ( !G_Cod1PrepareAmmoSpawn( classname, origin, flags, &model.init, error, size ) ) return 0;
    model.map_epoch = G_Cod1EntityMapEpoch();
    model.descriptor_fingerprint = CI_Fingerprint();
    if ( !AmmoSpawnModelMapCurrent( &model ) ) {
        EntityError( error, size, "ammo model registration requires a current native descriptor" );
        return 0;
    }
    memset( &request, 0, sizeof( request ) );
    request.version = COD1_PRECACHE_VERSION;
    request.bytes = sizeof( request );
    strcpy( request.name, model.init.item.models[0] );
    /* The descriptor and pose are already independent copies. The server
     * owns decoding/cache acquisition and may reject, reset or reenter GAME. */
    cod1AmmoSpawnModelBusy = 1;
    cod1AmmoSpawnModelCancelled = 0;
    model.server_model_slot = trap_Cod1ModelPrecache( &request );
    accepted = !cod1AmmoSpawnModelCancelled && G_Cod1AmmoSpawnModelCurrent( &model );
    cod1AmmoSpawnModelBusy = 0;
    if ( !accepted ) {
        EntityError( error, size, "ammo XModel missing/invalid, cache unavailable or map/descriptor changed" );
        return 0;
    }
    *out = model;
    return 1;
}

int G_Cod1ReadAmmoSpawnCandidate( const GscEntityRef *input, cod1AmmoSpawnModel_t *out ) {
    GscEntityRef entity;
    const Cod1AmmoCandidate *candidate;
    const gentity_t *ent;
    if ( !input || !out ) return 0;
    entity = *input;
    if ( !AmmoCandidateOwned( &entity ) || !G_Cod1EntitySameAllocation( &entity ) ) return 0;
    if ( cod1ActorByEntity[entity.slot] || cod1SentientByEntity[entity.slot] ) return 0;
    candidate = &cod1AmmoCandidates[entity.slot];
    ent = &g_entities[entity.slot];
    if ( !G_Cod1AmmoSpawnModelCurrent( &candidate->model ) ||
         !ent->classname || strcmp( ent->classname, candidate->model.init.item.classname ) ||
         ent->spawnflags != candidate->model.init.spawnflags || ent->r.contents || ent->r.bmodel ||
         ent->client || ent->item || ent->physicsObject || ent->neverFree ||
         ent->s.eType != ET_GENERAL ||
         memcmp( ent->s.origin, candidate->model.init.origin, sizeof( ent->s.origin ) ) ||
         memcmp( ent->r.currentOrigin, candidate->model.init.origin, sizeof( ent->r.currentOrigin ) ) ||
         memcmp( ent->r.mins, candidate->model.init.mins, sizeof( ent->r.mins ) ) ||
         memcmp( ent->r.maxs, candidate->model.init.maxs, sizeof( ent->r.maxs ) ) ) return 0;
    if ( candidate->worldPresent ) {
        if ( !AmmoWorldBodyCurrent( &entity ) ) return 0;
    } else if ( ent->r.linked || !( ent->r.svFlags & SVF_NOCLIENT ) || ent->s.modelindex ) return 0;
    *out = candidate->model;
    return 1;
}

#include "g_cod1_ammo_dobj.inc"
#include "g_cod1_ammo_native_state.inc"
#include "g_cod1_ammo_packet.inc"
#include "g_cod1_ammo_world.inc"
#include "g_cod1_ammo_angles.inc"

int G_Cod1DiscardAmmoSpawn( const GscEntityRef *input, char *error, size_t size ) {
    GscEntityRef entity;
    if ( !input ) {
        EntityError( error, size, "ammo discard requires an owned candidate" ); return 0;
    }
    entity = *input;
    if ( !AmmoCandidateOwned( &entity ) || !G_Cod1EntitySameAllocation( &entity ) ||
         g_entities[entity.slot].neverFree ) {
        EntityError( error, size, "ammo discard candidate stale, unowned or protected" ); return 0;
    }
    G_FreeEntity( &g_entities[entity.slot] );
    if ( G_Cod1EntitySameAllocation( &entity ) ) {
        EntityError( error, size, "ammo discard did not retire its allocation" ); return 0;
    }
    return 1;
}

int G_Cod1ReserveAmmoSpawn( const char *classname, const float origin[3],
                          int32_t flags, GscEntityRef *out, char *error, size_t size ) {
    cod1AmmoSpawnModel_t model, checked;
    GscEntityRef entity;
    gentity_t *ent;
    char *stableClass;
    int accepted = 0;
    memset( &entity, 0, sizeof( entity ) );
    if ( cod1AmmoCandidateBusy ) {
        cod1AmmoCandidateCancelled = 1;
        EntityError( error, size, "ammo allocation candidate reentered" ); return 0;
    }
    if ( !out ) { EntityError( error, size, "ammo candidate requires output" ); return 0; }
    cod1AmmoCandidateBusy = 1; cod1AmmoCandidateCancelled = 0;
    if ( !G_Cod1RegisterAmmoSpawnModel( classname, origin, flags, &model, error, size ) ) goto end;
    if ( cod1AmmoCandidateCancelled || !G_Cod1AmmoSpawnModelCurrent( &model ) ||
         level.num_entities < MAX_CLIENTS || level.num_entities > ENTITYNUM_MAX_NORMAL ||
         ( level.num_entities == ENTITYNUM_MAX_NORMAL && !G_EntitiesFree() ) ) {
        EntityError( error, size, "ammo candidate map changed or no normal entity slot" ); goto end;
    }
    ent = G_Spawn();
    /* The production fresh allocator checks its original owner across Locate.
     * Capture that identity before any initialization callback; never adopt a
     * new map's returned slot as the owner of this older registered resource. */
    if ( !ent || !G_Cod1EntityCapture( ent, &entity ) || entity.epoch != model.map_epoch ||
         entity.slot < MAX_CLIENTS || entity.slot >= ENTITYNUM_MAX_NORMAL ) {
        memset( &entity, 0, sizeof( entity ) );
        EntityError( error, size, "ammo allocation lost the registered map" ); goto end;
    }
    cod1AmmoCandidates[entity.slot].entity = entity;
    cod1AmmoCandidates[entity.slot].model = model;
    ent->r.svFlags |= SVF_NOCLIENT;
    stableClass = G_NewString( model.init.item.classname );
    /* A replacement must not receive even the first classname write. */
    if ( !stableClass || cod1AmmoCandidateCancelled || !AmmoCandidateOwned( &entity ) ||
         !G_Cod1EntitySameAllocation( &entity ) || !G_Cod1AmmoSpawnModelCurrent( &model ) ) {
        EntityError( error, size, "ammo candidate retired during classname initialization" ); goto end;
    }
    ent->classname = stableClass;
    ent->spawnflags = model.init.spawnflags;
    VectorCopy( model.init.origin, ent->s.origin );
    VectorCopy( model.init.origin, ent->s.pos.trBase );
    VectorCopy( model.init.origin, ent->r.currentOrigin );
    VectorCopy( model.init.mins, ent->r.mins );
    VectorCopy( model.init.maxs, ent->r.maxs );
    if ( !G_Cod1ReadAmmoSpawnCandidate( &entity, &checked ) ) {
        EntityError( error, size, "ammo candidate initialization invalid" ); goto end;
    }
    *out = entity; accepted = 1;
end:
    if ( !accepted && AmmoCandidateOwned( &entity ) && G_Cod1EntitySameAllocation( &entity ) ) {
        char cleanupError[128];
        if ( !G_Cod1DiscardAmmoSpawn( &entity, cleanupError, sizeof( cleanupError ) ) )
            EntityError( error, size, "ammo candidate refused; owned allocation could not be retired" );
    }
    cod1AmmoCandidateBusy = 0;
    return accepted;
}

static cod1CarbineSpawnCallback_t cod1CarbineSpawn;
static uint64_t cod1CarbineSpawnEpoch;
int G_Cod1EntitySelectCarbineSpawn(cod1CarbineSpawnCallback_t host) {
 if(!host||!cod1EntityReady||!ActorMapAvailable()||level.spawning||level.restarted)return 0;
 if(cod1CarbineSpawnEpoch==cod1EntityEpoch&&cod1CarbineSpawn&&cod1CarbineSpawn!=host)return 0;
 cod1CarbineSpawn=host;cod1CarbineSpawnEpoch=cod1EntityEpoch;return 1;
}
static int EntitySpawnWithFlags( void *user, const char *classname,
                        const float origin[3], int32_t flags, GscEntityRef *out,
                        char *error, size_t size ) {
    gentity_t *ent;
    vec3_t position;
    GscEntityRef ref;
    int i;
    (void)user;
    if(classname&&(!strcmp(classname,"weapon_m1carbine")||!strcmp(classname,"weapon_springfield")||!strcmp(classname,"weapon_thompson")))return cod1CarbineSpawn&&cod1CarbineSpawnEpoch==cod1EntityEpoch?cod1CarbineSpawn(classname,origin,flags,out,error,size):0;
    if ( classname && (!strcmp(classname,"weapon_fraggrenade") ||
         !strcmp(classname,"weapon_stielhandgranate") ||
         !strcmp(classname,"weapon_mk1britishfrag") ||
         !strcmp(classname,"weapon_rgd-33russianfrag") ||
         !strcmp(classname,"weapon_panzerfaust") ||
         !strcmp(classname,"item_ammo_stielhandgranate_open") ||
         !strcmp(classname,"item_ammo_stielhandgranate_closed")) )
        return G_Cod1ItemLifecycleSpawn(classname,origin,flags,out,error,size);
    if ( !cod1EntityReady || !classname || !origin || !out || level.spawning ||
         ( strcmp( classname, "script_origin" ) &&
           strcmp( classname, "script_model" ) ) ) {
        EntityError( error, size, "R84 spawn requires a ready map and a supported script class" );
        return 0;
    }
    for ( i = 0; i < 3; ++i ) {
        if ( !isfinite( origin[i] ) ) {
            EntityError( error, size, "spawn requires three finite coordinates" );
            return 0;
        }
        position[i] = origin[i];
    }
    if ( level.num_entities < MAX_CLIENTS ||
         level.num_entities > ENTITYNUM_MAX_NORMAL ||
         ( level.num_entities == ENTITYNUM_MAX_NORMAL && !G_EntitiesFree() ) ) {
        EntityError( error, size, "spawn has no available normal entity slot" );
        return 0;
    }
    ent = G_Spawn();
    /* Capture ownership before any initialization/link callback can retire
     * this slot, reuse it, or start another map. Never adopt its replacement. */
    ref.epoch = cod1EntityEpoch;
    ref.slot = (uint32_t)ent->s.number;
    ref.generation = cod1EntityGeneration[ref.slot];
    ent->classname = G_NewString( classname );
    VectorCopy( position, ent->s.origin );
    /* Retail spawn 20030f90 stores +0x130 before its class initializer. */
    ent->spawnflags = flags;
    if ( !G_CallSpawn( ent ) ) {
        if ( ref.epoch == cod1EntityEpoch &&
             ref.generation == cod1EntityGeneration[ref.slot] && ent->inuse ) {
            G_FreeEntity( ent );
        }
        EntityError( error, size, "supported script class could not be initialized" );
        return 0;
    }
    if ( !EntityValidate( NULL, &ref ) ) {
        EntityError( error, size, "spawned entity retired during initialization" );
        return 0;
    }
    ScriptModelThreadBorn(&ref); /* both real supported script class allocations */
    *out = ref;
    if ( EntityDiagnosticsEnabled() ) {
        G_Printf( "[GSC R84] spawn class=%s entity=%u generation=%u xyz=%.9g,%.9g,%.9g\n",
            classname, (unsigned)ref.slot, (unsigned)ref.generation,
            (double)position[0], (double)position[1], (double)position[2] );
    }
    return 1;
}

static int EntitySpawn( void *user, const char *classname,
                        const float origin[3], GscEntityRef *out,
                        char *error, size_t size ) {
    return EntitySpawnWithFlags( user, classname, origin, 0, out, error, size );
}

const GscEntityHost *G_Cod1EntityHost( void ) {
    static const GscEntityHost host = {
        NULL, EntityLookup, EntitySetOrigin, EntityValidate, EntitySpawn, EntitySpawnWithFlags
    };
    return &host;
}

/* Read-only pose access has no engine callback and retains no entity pointer.
 * These are restoration properties, not proof of the retail field ABI. */
static int EntityGetPose( const GscEntityRef *ref, float out[3], int angles,
                          char *error, size_t size ) {
    const gentity_t *ent;
    vec3_t copy;
    int i;
    if ( !out || !EntityValidate( NULL, ref ) ) {
        EntityError( error, size, "pose read received a stale entity reference or missing output" );
        return 0;
    }
    ent = &g_entities[ref->slot];
    VectorCopy( angles ? ent->r.currentAngles : ent->r.currentOrigin, copy );
    for ( i = 0; i < 3; ++i ) {
        if ( !isfinite( copy[i] ) ) {
            EntityError( error, size, "entity pose contains nonfinite coordinates" );
            return 0;
        }
    }
    VectorCopy( copy, out );
    return 1;
}

static int EntityGetOrigin( void *user, const GscEntityRef *ref, float out[3],
                            char *error, size_t size ) {
    (void)user;
    return EntityGetPose( ref, out, 0, error, size );
}

/* Native GetOrigin is a method, distinct from raw .origin. Original A8
 * selects the committed inline-brush bounds center; ordinary entities copy
 * currentOrigin. XY keeps a wide sum until half, while original Z explicitly
 * stores binary32 before half. No world/pose writes or callbacks occur here.
 * Current finite domain only; full native FP/overflow/CM parity remains OPEN. */
static int EntityOriginMethod(void *user,const GscEntityRef *ref,float out[3],char *error,size_t size) {
    const gentity_t *ent;float copy[3];int i;(void)user;
    if(!out||!EntityValidate(NULL,ref)){EntityError(error,size,"GetOrigin method received stale owner or missing output");return 0;}
    ent=&g_entities[ref->slot];
    if(!ent->r.bmodel)VectorCopy(ent->r.currentOrigin,copy);
    else {
        volatile float zsum;
        for(i=0;i<3;i++)if(!isfinite(ent->r.absmin[i])||!isfinite(ent->r.absmax[i])){
            EntityError(error,size,"GetOrigin brush bounds are nonfinite");return 0;
        }
        copy[0]=(float)(((double)ent->r.absmax[0]+(double)ent->r.absmin[0])*0.5);
        copy[1]=(float)(((double)ent->r.absmax[1]+(double)ent->r.absmin[1])*0.5);
        zsum=(float)((double)ent->r.absmax[2]+(double)ent->r.absmin[2]);
        copy[2]=(float)((double)zsum*0.5);
    }
    for(i=0;i<3;i++)if(!isfinite(copy[i])){EntityError(error,size,"GetOrigin method result is nonfinite");return 0;}
    VectorCopy(copy,out);return 1;
}
const GscEntityOriginMethodHost *G_Cod1OriginMethodHost(void) {
    static const GscEntityOriginMethodHost host={NULL,EntityOriginMethod};return &host;
}

static int EntityGetAngles( void *user, const GscEntityRef *ref, float out[3],
                            char *error, size_t size ) {
    (void)user;
    return EntityGetPose( ref, out, 1, error, size );
}

static int EntitySetAngles( void *user, const GscEntityRef *ref,
                            const float angles[3], char *error, size_t size ) {
    gentity_t *ent;
    vec3_t copy;
    int i;
    (void)user;
    if ( G_Cod1ItemLifecycleOwns(ref) )
        return G_Cod1ItemLifecycleSetAngles(ref,angles,error,size);
    if ( !EntityValidate( NULL, ref ) ) {
        EntityError( error, size, "angles assignment received a stale entity reference" );
        return 0;
    }
    ent = &g_entities[ref->slot];
    if(!angles||!ent->classname||ent->r.bmodel||
       !((!ent->r.contents&&ent->s.eType==ET_GENERAL&&
           (!strcmp(ent->classname,"script_origin")||!strcmp(ent->classname,"script_model")))||
         (cod1CarbineSpawn&&cod1CarbineSpawnEpoch==cod1EntityEpoch&&ent->s.eType==ET_ITEM&&
          G_Cod1GscTestCarbineSpawnAllowed(ent->classname)&&ent->item&&ent->item->giType==IT_WEAPON&&ent->r.linked))) {
        EntityError(error,size,"angles require a selected script entity or native carbine item");return 0;
    }
    for ( i = 0; i < 3; ++i ) {
        if ( !isfinite( angles[i] ) ) {
            EntityError( error, size, "angles assignment requires three finite coordinates" );
            return 0;
        }
        copy[i] = angles[i];
    }
    G_SetAngle( ent, copy );
    VectorCopy( copy, ent->s.angles );
    trap_LinkEntity( ent );
    if ( !EntityValidate( NULL, ref ) ) {
        EntityError( error, size, "entity retired during angles assignment" );
        return 0;
    }
    if ( EntityDiagnosticsEnabled() ) {
        G_Printf( "[GSC R85] angles entity=%u generation=%u xyz=%.9g,%.9g,%.9g\n",
            (unsigned)ref->slot, (unsigned)ref->generation,
            (double)copy[0], (double)copy[1], (double)copy[2] );
    }
    return 1;
}

static int EntitySetModel( void *user, const GscEntityRef *ref,
                           const char *model, char *error, size_t size ) {
    gentity_t *ent;
    const char *stablePath = NULL;
    int index;
    (void)user;
    if ( !EntityValidate( NULL, ref ) ) {
        EntityError( error, size, "setmodel received a stale entity reference" );
        return 0;
    }
    ent = &g_entities[ref->slot];
    if ( !model || !ent->classname || strcmp( ent->classname, "script_model" ) ||
         ent->r.contents || ent->r.bmodel || ent->s.eType != ET_GENERAL ||
         ent->s.density != COD1_SCRIPT_MODEL_NET_MARKER ) {
        EntityError( error, size, "setmodel requires a nonsolid script_model" );
        return 0;
    }
    index = G_Cod1ScriptModelResolve( model, &stablePath );
    /* Configstring reads/publication can cross a retirement or map boundary.
     * Never adopt the new occupant of the referenced slot, even after success. */
    if ( !EntityValidate( NULL, ref ) ) {
        EntityError( error, size, "entity retired during model registration" );
        return 0;
    }
    if ( index <= 0 || index >= MAX_MODELS || !stablePath ) {
        EntityError( error, size, "setmodel rejected the model path or full model table" );
        return 0;
    }
    ent = &g_entities[ref->slot];
    if ( !ent->classname || strcmp( ent->classname, "script_model" ) ||
         ent->r.contents || ent->r.bmodel || ent->s.eType != ET_GENERAL ||
         ent->s.density != COD1_SCRIPT_MODEL_NET_MARKER ) {
        EntityError( error, size, "script_model changed eligibility during model registration" );
        return 0;
    }
    /* Zero collision bounds/contents are unchanged, so no unlink/link callback
     * is needed. Model name and snapshot index are committed together. */
    ActorNativeStaticInvalidateSource(ref);
    ent->model = (char *)stablePath;
    ent->s.modelindex = index;
    if ( EntityDiagnosticsEnabled() ) {
        G_Printf( "[GSC R85] setmodel entity=%u generation=%u index=%d model=%s\n",
            (unsigned)ref->slot, (unsigned)ref->generation, index, stablePath );
    }
    return 1;
}

const GscEntityAccessHost *G_Cod1EntityAccessHost( void ) {
    static const GscEntityAccessHost host = {
        NULL, EntitySetModel, EntityGetOrigin, EntityGetAngles, EntitySetAngles
    };
    return &host;
}

#include "g_cod1_actor_script_state.inc"
#include "g_cod1_actor_desired_angle.inc"
#include "g_cod1_actor_damage_direction.inc"
#include "g_cod1_actor_script_fields.inc"
#include "g_cod1_actor_attachment_queries.inc"
#include "g_cod1_actor_model_read.inc"
#include "g_cod1_actor_weapon_fields.inc"
#include "g_cod1_weapon_model.inc"
#include "g_cod1_anim_data.inc"
#include "g_cod1_animation_refs.inc"
#include "g_cod1_animation_start_origin.inc"
static int EntityHintStoragePrivate(const void *,size_t);
/* Pure late flags-reader output separation. No native/path authority. */
int G_Cod1LateDisconnectInputsPrivate(const void*p,size_t n){
 return ActorOverlap(p,n,&level,sizeof(level))||ActorOverlap(p,n,g_entities,sizeof(g_entities))||
  ActorOverlap(p,n,g_clients,sizeof(g_clients))||ActorOverlap(p,n,&cod1EntityEpoch,sizeof(cod1EntityEpoch))||
  ActorOverlap(p,n,cod1EntityGeneration,sizeof(cod1EntityGeneration))||ActorOverlap(p,n,cod1EntityRetired,sizeof(cod1EntityRetired))||
  ActorOverlap(p,n,&cod1EntityReady,sizeof(cod1EntityReady))||ActorOverlap(p,n,&cod1EntityActive,sizeof(cod1EntityActive))||
  ActorOverlap(p,n,cod1Actors,sizeof(cod1Actors))||ActorOverlap(p,n,cod1Sentients,sizeof(cod1Sentients))||
  ActorOverlap(p,n,cod1ActorByEntity,sizeof(cod1ActorByEntity))||ActorOverlap(p,n,cod1SentientByEntity,sizeof(cod1SentientByEntity))||
  ActorOverlap(p,n,&cod1ActorSerial,sizeof(cod1ActorSerial))||ActorOverlap(p,n,&cod1ActorCounter,sizeof(cod1ActorCounter))||
  ActorOverlap(p,n,&cod1SentientCounter,sizeof(cod1SentientCounter));
}
int G_Cod1EntityStorageProtected(const void *pointer,size_t size) {
    if (!size) return 0;
    if(PlacedFieldProofPrivate(pointer,size)||G_Cod1Bsp592SealPrivate(pointer,size))return 1;
    if(ActorOverlap(pointer,size,scriptModelThreads,sizeof(scriptModelThreads)))return 1;
    if(G_Cod1TurretFactoryPrivate(pointer,size)||G_Cod1LateDisconnectPrivate(pointer,size)||G_Cod1PlayerLifetimePrivate(pointer,size)||WeaponClassnamePrivate(pointer,size)||EntityLinkPrivate(pointer,size)||ActorFactoryPrivate(pointer,size))return 1;
    if(ActorModelReadStoragePrivate(pointer,size)||ActorAttachmentStoragePrivate(pointer,size)||DeleteStoragePrivate(pointer,size)||RuntimeActorDeletePrivate(pointer,size)||TurretStoragePrivate(pointer,size)||G_Cod1DroppedItemStorageProtected(pointer,size))return 1;
    if(ActorControllerPrivate(pointer,size)||ActorNormalPrivate(pointer,size)||ActorScriptStatePrivate(pointer,size)||EntityHintStoragePrivate(pointer,size)||FreshStoragePrivate(pointer,size))return 1;
    return ActorOverlap(pointer,size,&cod1CarbineSpawn,sizeof(cod1CarbineSpawn)) ||
        ActorOverlap(pointer,size,&cod1CarbineSpawnEpoch,sizeof(cod1CarbineSpawnEpoch)) ||
        G_Cod1ItemLifecycleStorageProtected(pointer,size) ||
        G_Cod1YawMotionStorageProtected(pointer,size) ||
        ActorOverlap(pointer,size,G_Cod1GrenadePlayerNativeHost(),sizeof(GscPlayerNativeHost)) ||
        StartOriginPrivate(pointer,size) ||
        ActorOverlap(pointer,size,G_Cod1EntityHost(),sizeof(GscEntityHost)) ||
        ActorOverlap(pointer,size,G_Cod1EntityAccessHost(),sizeof(GscEntityAccessHost)) ||
        ActorOverlap(pointer,size,G_Cod1EntityControlHost(),sizeof(GscEntityControlHost)) ||
        ActorOverlap(pointer,size,G_Cod1ActorThreadAuthorityHost(),sizeof(GscThreadEntityAuthorityHost)) ||
        ActorOverlap(pointer,size,G_Cod1ScriptThreadAuthorityHost(),sizeof(GscThreadEntityAuthorityHost)) ||
        ActorOverlap(pointer,size,&g_gametype,sizeof(g_gametype));
}

#include "g_cod1_actor_inherited_entry.inc"
#include "g_cod1_actor_thread_authority.inc"
#include "g_cod1_actor_projected_entry.inc"
#include "g_cod1_actor_bootstrap_services.inc"
#include "g_cod1_actor_placed_entry.inc"
#include "g_cod1_actor_bsp_construction.inc"
#include "g_cod1_actor_aitype_resources.inc"
#include "g_cod1_actor_entity_hooks.inc"

/* R91 controls are an explicitly selected superset, never a relaxation of
 * the earlier access host. Metadata is copied and visibility retains the
 * entity's collision, type, model, link and event/sound transmission. */
static int EntityGetString( void *user, const GscEntityRef *ref,
                            const char *field, char *out, size_t outSize,
                            int *found, char *error, size_t size ) {
    const char *value;
    size_t length;
    (void)user;
    if ( !field || !out || !outSize || !found ||
         !EntityValidate( NULL, ref ) ) {
        EntityError( error, size, "metadata read received a stale reference or missing output" );
        return 0;
    }
    if ( !strcmp( field, "target" ) ) {
        value = g_entities[ref->slot].target;
    } else if ( !strcmp( field, "targetname" ) ) {
        value = g_entities[ref->slot].targetname;
    } else {
        EntityError( error, size, "metadata read supports only target and targetname" );
        return 0;
    }
    if ( !value ) {
        out[0] = '\0';
        *found = 0;
        return 1;
    }
    /* Do not silently truncate an authored target into a different entity's
     * name. An explicit empty string remains a defined string, unlike NULL. */
    for ( length = 0; length < outSize && value[length]; ++length ) {}
    if ( length == outSize ) {
        EntityError( error, size, "metadata string exceeds the output capacity" );
        return 0;
    }
    memcpy( out, value, length + 1 );
    *found = 1;
    return 1;
}

/* CS_MODELS has 256 slots, but BSP inline indices use the separate existing
 * 9-bit namespace. Handles 510/511 are temporary collision shapes. Check
 * the authored positive inline spelling against the stored numeric model;
 * the spawn-time CM binding already validated the current world's count. */
static int EntityControlInlineModel( const gentity_t *ent ) {
    const char *digit;
    int index = 0;
    if ( !ent || !ent->model || ent->model[0] != '*' || !ent->model[1] ||
         ent->s.modelindex <= 0 || ent->s.modelindex >= MAX_INLINE_MODELS ) {
        return 0;
    }
    for ( digit = ent->model + 1; *digit; ++digit ) {
        int value = *digit - '0';
        if ( value < 0 || value > 9 ||
             index > ( MAX_INLINE_MODELS - 1 - value ) / 10 ) {
            return 0;
        }
        index = index * 10 + value;
    }
    return index > 0 && index == ent->s.modelindex;
}

#include "g_cod1_r200_collision_owner.inc"
static int EntityVisibilityEligible( const gentity_t *ent ) {
    if ( !ent || !ent->classname || ent->client ||
         ent->s.number < MAX_CLIENTS || ent->s.number >= ENTITYNUM_MAX_NORMAL ) {
        return 0;
    }
    if ( !strcmp( ent->classname, "script_model" ) ) {
        return ent->s.eType == ET_GENERAL &&
            ent->s.density == COD1_SCRIPT_MODEL_NET_MARKER &&
            !ent->r.bmodel && !ent->r.contents;
    }
    if ( !strcmp( ent->classname, "script_origin" ) ) {
        return ent->s.eType == ET_GENERAL && !ent->s.modelindex &&
            !ent->r.bmodel && !ent->r.contents;
    }
    if ( !strcmp( ent->classname, "script_brushmodel" ) ) {
        return ent->s.eType == ET_MOVER && ent->r.bmodel &&
            ent->s.solid == SOLID_BMODEL &&
            EntityControlInlineModel( ent );
    }
    return 0;
}

int G_Cod1EntityReadModelField(const GscEntityRef *input,GscStringCastValue *out,char *error,size_t size) {
    GscEntityRef ref;
    GscStringCastValue result = {0};
    char text[MAX_STRING_CHARS];
    gentity_t *ent;
    int index;
    if (!input || !out || !ActorArguments(input,sizeof(*input),out,sizeof(*out),error,size) ||
        ActorOverlap(input,sizeof(*input),out,sizeof(*out))) return 0;
    ref = *input;
    if (!EntityValidate(NULL,&ref)) return 0;
    ent = &g_entities[ref.slot];
    if (!ent->classname || strcmp(ent->classname,"script_model"))
        return G_Cod1ActorReadModelField(input,out,error,size);
    if (!EntityVisibilityEligible(ent)) return 0;
    index = ent->s.modelindex;
    if (index < 0 || index >= MAX_MODELS) return 0;
    /* GAME 41C42 reads the current model configstring, including slot zero. */
    memset(text,0xa5,sizeof(text));
    trap_GetConfigstring(CS_MODELS+index,text,sizeof(text));
    if (!EntityValidate(NULL,&ref) || memcmp(input,&ref,sizeof(ref)) ||
        !ent->classname || strcmp(ent->classname,"script_model") ||
        !EntityVisibilityEligible(ent) || ent->s.modelindex != index ||
        !memchr(text,0,sizeof(text)) || strlen(text) >= sizeof(result.string)) return 0;
    result.type = 1;
    strcpy(result.string,text);
    *out = result;
    return 1;
}

#include "g_cod1_entity_hints.inc"
static int EntitySetVisible( void *user, const GscEntityRef *ref,
                             int visible, char *error, size_t size ) {
    gentity_t *ent;
    (void)user;
    if ( ( visible != 0 && visible != 1 ) ||
         !EntityValidate( NULL, ref ) ) {
        EntityError( error, size, "visibility requires a live reference and a boolean value" );
        return 0;
    }
    ent = &g_entities[ref->slot];
    if ( !EntityVisibilityEligible( ent ) ) {
        EntityError( error, size, "visibility requires a supported CoD script entity" );
        return 0;
    }
    /* R201: the normal GAME frame derives EF_NODRAW from FL_NODRAW for
     * nonclients. Commit both so that the next frame cannot undo hide/show.
     * These existing port flags change visibility only; collision, linking,
     * sound/event transmission and every other flag remain untouched.
     * No traps occur between validation and this commit. */
    if ( visible ) {
        ent->flags &= ~FL_NODRAW;
        ent->s.eFlags &= ~EF_NODRAW;
    } else {
        ent->flags |= FL_NODRAW;
        ent->s.eFlags |= EF_NODRAW;
    }
    return 1;
}

static int EntityTriggerOriginEligible( const gentity_t *ent ) {
    if ( !ent || !ent->classname || ent->client ||
         ent->s.number < MAX_CLIENTS || ent->s.number >= ENTITYNUM_MAX_NORMAL ||
         ent->spawnflags || !ent->r.linked || !ent->r.bmodel ||
         ent->r.contents != CONTENTS_TRIGGER || !EntityControlInlineModel( ent ) ||
         ent->s.eType != ET_INVISIBLE || !( ent->r.svFlags & SVF_NOCLIENT ) ) {
        return 0;
    }
    if ( !strcmp( ent->classname, "trigger_use" ) ) {
        return ent->use == G_Cod1TriggerUse && !ent->touch;
    }
    if ( !strcmp( ent->classname, "trigger_multiple" ) ) {
        return ent->touch == G_Cod1TriggerMultipleTouch && !ent->use;
    }
    return 0;
}

static int EntityControlSetOrigin( void *user, const GscEntityRef *ref,
                                   const float origin[3], char *error, size_t size ) {
    gentity_t *ent;
    vec3_t position;
    int i;
    if ( !EntityValidate( NULL, ref ) ) {
        EntityError( error, size, "control origin received a stale entity reference" );
        return 0;
    }
    ent = &g_entities[ref->slot];
    if ( !EntityTriggerOriginEligible( ent ) ) {
        /* Existing nonsolid script entity semantics remain owned by the
         * accepted R84 path. Solid brushes and actors still reach its refusal. */
        return EntitySetOrigin( user, ref, origin, error, size );
    }
    if ( !origin ) {
        EntityError( error, size, "control origin requires three finite coordinates" );
        return 0;
    }
    for ( i = 0; i < 3; ++i ) {
        if ( !isfinite( origin[i] ) ) {
            EntityError( error, size, "control origin requires three finite coordinates" );
            return 0;
        }
        position[i] = origin[i];
    }
    /* This changes an admitted invisible nonsolid trigger's query position,
     * not a solid mover's pushing behavior. Link computes its current bounds.
     * No entity pointer is used after a callback until identity is checked. */
    G_SetOrigin( ent, position );
    VectorCopy( position, ent->s.origin );
    trap_LinkEntity( ent );
    if ( !EntityValidate( NULL, ref ) ) {
        EntityError( error, size, "trigger retired during position update" );
        return 0;
    }
    if ( !EntityTriggerOriginEligible( &g_entities[ref->slot] ) ) {
        EntityError( error, size, "trigger changed eligibility during position update" );
        return 0;
    }
    return 1;
}

const GscEntityControlHost *G_Cod1EntityControlHost( void ) {
    static const GscEntityControlHost host = {
        NULL, EntityGetString, EntitySetVisible, EntityControlSetOrigin
    };
    return &host;
}

/* R93 getentarray is a separate capability. The original scripts require
 * targetname and classname; this does not broaden the legacy getent lookup.
 * Exact comparison and ascending slot order are bounded restoration policy,
 * not a recovered retail VM ordering/field ABI. All pointers stay local. */
static int cod1EntityCollectionBusy, cod1EntityCollectionReentered;

static int EntityCollectionOverlap( const void *a, size_t aSize,
                                     const void *b, size_t bSize ) {
    uintptr_t startA = (uintptr_t)a, startB = (uintptr_t)b;
    if ( !a || !b || !aSize || !bSize ) return 0;
    /* Subtraction avoids wrapping an end address for caller-provided sizes. */
    return startA <= startB ? startB - startA < aSize : startA - startB < bSize;
}

static int EntityCollectionContext( uint64_t epoch, int entityCount ) {
    return !cod1EntityCollectionReentered && cod1EntityReady &&
        cod1EntityActive && epoch && epoch == cod1EntityEpoch &&
        entityCount >= 0 && entityCount <= MAX_GENTITIES &&
        entityCount == level.num_entities && !level.spawning &&
        G_Cod1MapEntities() && g_gametype.integer == GT_SINGLE_PLAYER;
}

/* R131 pure ready-map lease; empty spawner arrays still require MapReady. */
int G_Cod1EntityCollectionReady( uint64_t epoch, int entityCount ) {
    return EntityCollectionContext( epoch, entityCount );
}

#include "g_cod1_entity_number_host.inc"

static int EntityCollectionMode( uint64_t epoch, int entityCount, int *capturedMode ) {
    int mode;
    /* Every trap is a possible lifecycle boundary. The final collection scan
     * runs after these callbacks, so no engine pointer crosses a callback. */
    if ( !EntityCollectionContext( epoch, entityCount ) ||
         trap_Cvar_VariableIntegerValue( "g_cod1GscTest" ) != 1 ||
         !EntityCollectionContext( epoch, entityCount ) ||
         trap_Cvar_VariableIntegerValue( "savegame_loading" ) != 0 ||
         !EntityCollectionContext( epoch, entityCount ) ) {
        return 0;
    }
    /* R193 FIX1: the shipped mode40 Session already supplies collections.
     * Admit that selected capability here too; keep every lifecycle check
     * and the two-sample mode fence. No blanket future-mode admission. */
    mode = trap_Cvar_VariableIntegerValue( "g_cod1GscEntityTest" );
    if ( !EntityCollectionContext( epoch, entityCount ) ||
         ( mode != 8 && mode != 9 && mode != 10 && mode != 11 && mode != 12 && mode != 13 && mode != 15 && mode != 16 && mode != 17 && mode != 18 && (mode != 19 && mode != 20) && mode != 30 && mode != 31 && mode != 40 && (mode!=41&&mode!=42&&mode!=43) ) ||
         ( *capturedMode && mode != *capturedMode ) ) return 0;
    /* The second sampling round must preserve the first admitted capability.
     * Later selected supersets include arrays, but switching modes cannot continue one lookup. */
    *capturedMode = mode;
    return 1;
}

static int EntityLookupArray( void *user, const char *name, const char *field,
                              GscEntityRef *out, size_t capacity, size_t *count,
                              char *error, size_t size ) {
    GscEntityRef snapshot[MAX_GENTITIES];
    uint64_t epoch;
    char copiedName[128];
    size_t fieldOffset;
    int entityCount, i, success = 0, overlappingError, capturedMode = 0;
    size_t length, matches = 0, seen = 0, outputBytes;
    const char *failure = "collection requires a ready selected collection mode on a CoD SP map";
    (void)user;
    /* Check error aliases before every early refusal, including reentry. The
     * cap is bounded here even for a malformed capacity, avoiding overflow. */
    outputBytes = ( capacity > MAX_GENTITIES ? MAX_GENTITIES : capacity ) * sizeof( *out );
    overlappingError = EntityCollectionOverlap( out, outputBytes, error, size ) ||
        EntityCollectionOverlap( count, sizeof( *count ), error, size );
    if ( cod1EntityCollectionBusy ) {
        /* Refuse the nested call and poison its outer operation too: a trap
         * must not let a second lookup publish a different map incarnation. */
        cod1EntityCollectionReentered = 1;
        if ( !overlappingError ) EntityError( error, size, "entity collection lookup reentered" );
        return 0;
    }
    if ( overlappingError ) return 0;
    if ( !name || !field || !out || !count || !capacity ||
         capacity > MAX_GENTITIES ) {
        EntityError( error, size, "collection requires bounded output and count" );
        return 0;
    }
    /* Distinct output regions are necessary for atomic count/ref publication. */
    if ( EntityCollectionOverlap( out, capacity * sizeof( *out ), count, sizeof( *count ) ) ) {
        EntityError( error, size, "collection output and count overlap" );
        return 0;
    }
    if ( !strcmp( field, "classname" ) ) {
        fieldOffset = offsetof(gentity_t,classname);
    } else if ( !strcmp( field, "targetname" ) ) {
        fieldOffset = offsetof(gentity_t,targetname);
    } else if ( !strcmp( field, "target" ) ) {
        fieldOffset = offsetof(gentity_t,target);
    } else {
        EntityError( error, size, "getentarray supports classname, targetname and target" );
        return 0;
    }
    for ( length = 0; length < sizeof( copiedName ) && name[length]; ++length ) {}
    if ( length == sizeof( copiedName ) ) {
        EntityError( error, size, "collection name exceeds 127 bytes" );
        return 0;
    }
    memcpy( copiedName, name, length + 1 );
    epoch = cod1EntityEpoch;
    entityCount = level.num_entities;
    cod1EntityCollectionBusy = 1;
    cod1EntityCollectionReentered = 0;
    if ( !EntityCollectionContext( epoch, entityCount ) ) goto done;
    for ( i = 0; i < entityCount; ++i ) {
        const gentity_t *ent = &g_entities[i];
        const char *value = *(char * const *)((const char *)ent + fieldOffset);
        if ( !ent->inuse || !value || strcmp( value, copiedName ) ) continue;
        if ( matches == capacity ) {
            failure = "entity collection exceeds output capacity";
            goto done;
        }
        snapshot[matches].epoch = epoch;
        snapshot[matches].slot = (uint32_t)i;
        snapshot[matches].generation = cod1EntityGeneration[i];
        if ( !EntityValidate( NULL, &snapshot[matches] ) ) {
            failure = "collection member has no current allocation identity";
            goto done;
        }
        ++matches;
    }
    if ( !EntityCollectionMode( epoch, entityCount, &capturedMode ) ||
         !EntityCollectionMode( epoch, entityCount, &capturedMode ) ) goto done;
    /* Recheck the complete membership, not merely the old references. A trap
     * may rename/add/remove a member without changing level.num_entities.
     * Validation and publication below have no engine callbacks between them. */
    for ( i = 0; i < entityCount; ++i ) {
        const gentity_t *ent = &g_entities[i];
        const char *value = *(char * const *)((const char *)ent + fieldOffset);
        if ( !ent->inuse || !value || strcmp( value, copiedName ) ) continue;
        if ( seen >= matches || snapshot[seen].slot != (uint32_t)i ||
             !EntityValidate( NULL, &snapshot[seen] ) ) {
            failure = "entity collection changed during lookup";
            goto done;
        }
        ++seen;
    }
    if ( seen != matches || !EntityCollectionContext( epoch, entityCount ) ) {
        failure = "entity collection changed during lookup";
        goto done;
    }
    if ( matches ) memcpy( out, snapshot, matches * sizeof( *out ) );
    *count = matches;
    success = 1;
done:
    cod1EntityCollectionBusy = 0;
    cod1EntityCollectionReentered = 0;
    if ( !success ) EntityError( error, size, failure );
    return success;
}

const GscEntityCollectionHost *G_Cod1EntityCollectionHost( void ) {
    static const GscEntityCollectionHost host = { NULL, EntityLookupArray };
    return &host;
}

/* The original isalive tests an actor state word when an actor exists, and
 * otherwise tests entity health > 0. Its issentient tests a separate sentient
 * pointer, which player initialization allocates independently of the client.
 * The constructor now owns a separate serial-checked Sentient component.
 * Original200308C0 tests entity+10c, independently of actor health/AI state.
 * Admit that genuine allocation plus ordinary nonactor entities and the
 * canonical native SP player; reject
 * authoritative retained cast/bot state instead of inventing CoD AI meaning.
 * No classname, network eType, positive health or arbitrary client pointer
 * is accepted as evidence of an actor or player. */
static int EntityPredicateRead( const GscEntityRef *entity, int sentient,
                                int *out, char *error, size_t size ) {
    GscEntityRef ref;
    const gentity_t *ent;
    int value, player = 0;
    /* A refused read must not clobber its output or borrowed identity through
     * an aliased diagnostic buffer. The separate output/ref alias is copied
     * safely below and is allowed. */
    if ( EntityCollectionOverlap( out, sizeof( *out ), error, size ) ||
         EntityCollectionOverlap( entity, sizeof( *entity ), error, size ) ) return 0;
    /* A selected entity predicate is pure even for an expired input.
     * Refuse private destinations before the stale-reference false result or
     * any diagnostic can publish there. A local ref/output alias stays valid. */
    if(ActorGoalPrivate(entity,sizeof(*entity))||ActorGoalPrivate(out,sizeof(*out))||ActorGoalPrivate(error,size)||
       ActorGenericPrivate(entity,sizeof(*entity))||ActorGenericPrivate(out,sizeof(*out))||ActorGenericPrivate(error,size))return 0;
    if ( !entity || !out ) {
        EntityError( error, size, "entity predicate requires a reference and output" );
        return 0;
    }
    /* An output may alias the borrowed reference. Snapshot it before any
     * output/error publication, and retain no caller or engine pointer. */
    ref = *entity;
    if ( !cod1EntityReady || !cod1EntityActive || level.spawning ||
         level.num_entities < MAX_CLIENTS || level.num_entities > MAX_GENTITIES ||
         !G_Cod1MapEntities() || g_gametype.integer != GT_SINGLE_PLAYER ) {
        EntityError( error, size, "entity predicate requires a ready fresh CoD SP map" );
        return 0;
    }
    if ( !EntityValidate( NULL, &ref ) ) {
        /* This false result is the bounded port lifetime policy, not proof of
         * how the unavailable original VM treats a freed entity value. */
        *out = 0;
        return 1;
    }
    ent = &g_entities[ref.slot];
    if ( ent->r.svFlags & ( SVF_CASTAI | SVF_BOT ) || ent->aiCharacter ||
         ent->aiAttributes || ent->AIScript_AlertEntity || ent->aiInactive ) {
        EntityError( error, size, "entity predicate requires a confirmed CoD nonactor or player; retained AI is unsupported" );
        return 0;
    }
    if ( ent->client ) {
        /* Check the complete native binding before dereferencing a client.
         * Reserved empty client slots and arbitrary/foreign pointers are not
         * player objects. A dead connected player remains sentient. */
        if ( !level.clients || level.maxclients <= 0 ||
             level.maxclients > MAX_CLIENTS || ref.slot >= MAX_CLIENTS ||
             ref.slot >= (uint32_t)level.maxclients ||
             ent->client != &level.clients[ref.slot] ||
             ent->client->pers.connected != CON_CONNECTED ||
             ent->client->ps.aiChar ||
             ( ent->aiName && strcmp( ent->aiName, "player" ) ) ) {
            EntityError( error, size, "entity predicate has no confirmed native SP player binding" );
            return 0;
        }
        player = 1;
    } else {
        if ( ref.slot < MAX_CLIENTS ) {
            EntityError( error, size, "entity predicate has no native binding for a reserved client slot" );
            return 0;
        }
        if ( ent->aiName ) {
            EntityError( error, size, "entity predicate cannot classify retained named AI as a CoD nonactor" );
            return 0;
        }
    }
    if(cod1SentientByEntity[ref.slot]||cod1ActorByEntity[ref.slot]){
        cod1ActorLease_t lease;const cod1SentientRecord_t *owned;uint32_t index;
        if(!cod1SentientByEntity[ref.slot]||
           (index=cod1SentientByEntity[ref.slot]-1)>=COD1_SENTIENT_CAPACITY){
            EntityError(error,size,"issentient owned component index is inconsistent");return 0;}
        owned=cod1Sentients+index;
        if(owned->kind!=(uint32_t)(player?COD1_ACTOR_LEASE_PLAYER:COD1_ACTOR_LEASE_ACTOR)||
           (player&&cod1ActorByEntity[ref.slot])){
            EntityError(error,size,"issentient owned component kind is inconsistent");return 0;}
        memset(&lease,0,sizeof(lease));
        lease.entity=ref;lease.serial=owned->serial;lease.actorIndex=owned->actorIndex;
        lease.sentientIndex=index;lease.kind=owned->kind;
        if(!ActorLeaseValid(&lease)){
            EntityError(error,size,"issentient owned component identity is inconsistent");return 0;}
        /* Sentient presence is independent of Actor life. For isalive the
         * original Actor branch reads +2A4, represented by the existing owned
         * local-defaults cell. Do not substitute entity health or a->alive.
         * The same reciprocal allocation/serial check protects both reads;
         * inconsistent component ownership never falls back to health. */
        if(sentient){*out=1;return 1;}
        if(lease.kind==COD1_ACTOR_LEASE_ACTOR){
            *out=cod1Actors[lease.actorIndex].defaults.lifeWords[(0x2a4-0x26c)/4]!=0;
            return 1;
        }
    }
    value = sentient ? player : ent->health > 0;
    /* Everything from identity validation through publication is pure. No
     * trap, callback, pointer retention, recapture or slot adoption occurs. */
    *out = value;
    return 1;
}

static int EntityIsAlive( void *user, const GscEntityRef *entity,
                          int *out, char *error, size_t size ) {
    (void)user;
    return EntityPredicateRead( entity, 0, out, error, size );
}

static int EntityIsSentient( void *user, const GscEntityRef *entity,
                             int *out, char *error, size_t size ) {
    (void)user;
    return EntityPredicateRead( entity, 1, out, error, size );
}

const GscEntityPredicateHost *G_Cod1EntityPredicateHost( void ) {
    static const GscEntityPredicateHost host = { NULL, EntityIsAlive, EntityIsSentient };
    return &host;
}
#include "g_cod1_player_natives.inc"
#include "g_cod1_script_thread_authority.inc"
#include "g_cod1_ammo_inventory.inc"
#include "g_cod1_owned_clip_ammo.inc"
#include "g_cod1_acquire_clip_ammo.inc"
#include "g_cod1_grenade_player_natives.inc"
#include "g_cod1_item_lifecycle.inc"
#include "g_cod1_yaw_motion.inc"
#include "g_cod1_entity_delete.inc"
#include "g_cod1_entity_links.inc"
#include "g_cod1_weapon_classname.inc"
#include "g_cod1_model_part.inc"
#include "g_cod1_entity_delete_transaction.inc"
#include "g_cod1_spatial_natives.inc"
#include "g_cod1_turret_host.inc"
/* Spawn registers this allocation only during level.spawning. Reuse requires
 * a different map epoch or entity birth generation. Gun controls and the
 * mounted shooter's lease are independent of the gun's identity as self. */
static int ScriptThreadTurretCurrent(const GscEntityRef *ref,uint64_t birth){
    const C1TRecord *turret;
    const gentity_t *ent;
    if(!ref || !birth || birth!=ref->generation ||
       !cod1EntityReady || level.spawning || level.restarted ||
       !ActorMapAvailable() || !G_Cod1EntitySameAllocation(ref) ||
       cod1TurretResourceEpoch!=ref->epoch || !cod1TurretAnimatedSkeleton ||
       cod1ActorByEntity[ref->slot] || cod1SentientByEntity[ref->slot])return 0;
    turret=TurretFind(ref);
    ent=g_entities+ref->slot;
    return turret && !ent->client && ent->classname &&
           !strcmp(ent->classname,"misc_mg42") && turret->modelIndex>0 &&
           ent->s.modelindex==turret->modelIndex &&
           ent->s.cod1TurretVersion==COD1_TURRET_WIRE_VERSION &&
           (uint32_t)ent->s.cod1TurretGeneration==ref->generation &&
           (uint32_t)ent->s.cod1TurretEpochLo==(uint32_t)ref->epoch &&
           (uint32_t)ent->s.cod1TurretEpochHi==(uint32_t)(ref->epoch>>32);
}

#include "g_cod1_actor_placed_init.inc"
#include "g_cod1_actor_look_host.inc"
#include "g_cod1_actor_active_prone.inc"
#include "g_cod1_actor_immediate_host.inc"
#include "g_cod1_entity_delete_runtime_actor.inc"
#include "g_cod1_dropped_item.inc"
#include "g_cod1_actor_spawn_factory.inc"
#include "g_cod1_move_delta.inc"
#include "g_cod1_navigation_actor_input.inc"
#else
typedef int cod1_entity_host_disabled_translation_unit;
#endif

#include "g_cod1_r243_stance.inc"
#include "g_cod1_actor_float.inc"
#include "g_cod1_r243_animtime.inc"
