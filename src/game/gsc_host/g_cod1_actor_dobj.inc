/* R104 explicit provisional skeletal topology. Included after the actor
 * registry/spatial helpers. Every engine call crosses an ownership boundary.
 * No renderer, current pose, script model method or AI admission is implied. */
static uint64_t cod1ActorDObjSceneBusyEpoch;

static int ActorDObjCancel( const cod1ActorLease_t *lease ) {
    cod1DObjFree_t request;
    memset( &request, 0, sizeof( request ) );
    request.version = COD1_DOBJ_VERSION; request.bytes = sizeof( request );
    request.key = ActorSpatialKey( lease );
    return trap_Cod1DObjFree( &request );
}
static int ActorDObjFree( const cod1ActorLease_t *lease, int pending,
    char *error, size_t size ) {
    cod1ActorRecord_t *a;
    int status, oldBusy;
    if ( !ActorLeaseValid( lease ) || lease->kind != COD1_ACTOR_LEASE_ACTOR ) {
        EntityError( error, size, "DObj cleanup received a stale actor lease" ); return 0;
    }
    a = &cod1Actors[lease->actorIndex];
    /* R162: the model lock begins before CS/resource validation, before
     * dobj.busy is raised. Explicit free must respect that earlier interval;
     * genuine owner retirement has the separate pending1 route. */
    if ( (a->modelBusy || a->modelMethodBusy) && !pending ) {
        EntityError(error,size,"actor model operation is already in progress");return 0;
    }
    if ( !a->dobj.present && !a->dobj.busy ) {
        memset(&a->assignedModel,0,sizeof(a->assignedModel));
        memset(&a->registeredAttachments,0,sizeof(a->registeredAttachments));
        return 1;
    }
    if ( a->dobj.busy == 2 || ( a->dobj.busy && !pending ) ) {
        EntityError( error, size, "actor DObj operation is already in progress" ); return 0;
    }
    oldBusy = a->dobj.busy;
    a->dobj.busy = 2;
    status = ActorDObjCancel( lease );
    if ( !ActorLeaseValid( lease ) ) {
        EntityError( error, size, "actor allocation changed during DObj cleanup" ); return 0;
    }
    a = &cod1Actors[lease->actorIndex];
    a->dobj.busy = oldBusy;
    if ( status != COD1_DOBJ_OK ) {
        EntityError( error, size, "checked DObj free failed; reservation retained" ); return 0;
    }
    a->dobj.present = 0;
    memset( &a->dobj.summary, 0, sizeof( a->dobj.summary ) );
    memset( &a->assignedModel, 0, sizeof( a->assignedModel ) );
    memset( &a->registeredAttachments, 0, sizeof( a->registeredAttachments ) );
    return 1;
}
static int ActorDObjFreeLease( const cod1ActorLease_t *lease,
    char *error, size_t size ) {
    return ActorDObjFree( lease, 0, error, size );
}
static int ActorDObjFreeOwner( const GscEntityRef *owner ) {
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
    return ActorDObjFree( &lease, 1, NULL, 0 );
}
static int ActorDObjSceneCleanup( void ) {
    cod1DObjSceneReset_t request;
    uint64_t epoch = cod1EntityEpoch;
    int i, present = 0, status;
    for ( i = 0; i < COD1_ACTOR_CAPACITY; ++i )
        if ( cod1Actors[i].activation >= 0 &&
             ( cod1Actors[i].dobj.present || cod1Actors[i].dobj.busy ) ) present = 1;
    /* Actors and ammo use the same native instance bank. Reset it once for
     * this epoch; a second reset could cross a callback's new incarnation. */
    for ( i = 0; i < MAX_GENTITIES; ++i )
        if ( cod1AmmoCandidates[i].entity.epoch == epoch &&
             ( cod1AmmoCandidates[i].dobjPresent || cod1AmmoCandidates[i].dobjBusy ) ) present = 1;
    if ( !present ) return 1;
    if ( !epoch || cod1ActorDObjSceneBusyEpoch == epoch ) return 0;
    memset( &request, 0, sizeof( request ) );
    request.version = COD1_DOBJ_VERSION; request.bytes = sizeof( request );
    request.epoch = epoch;
    cod1ActorDObjSceneBusyEpoch = epoch;
    status = trap_Cod1DObjSceneReset( &request );
    if ( cod1ActorDObjSceneBusyEpoch == epoch ) cod1ActorDObjSceneBusyEpoch = 0;
    return status == COD1_DOBJ_OK;
}
static int ActorTopologyStageValid( const cod1ActorLease_t *lease, int busy ) {
    const cod1ActorRecord_t *a;
    const gentity_t *e;
    if ( !ActorLeaseValid( lease ) || lease->kind != COD1_ACTOR_LEASE_ACTOR ) return 0;
    a = &cod1Actors[lease->actorIndex]; e = &g_entities[lease->entity.slot];
    return a->alive && a->nativeDataInitialized && !a->initialized &&
        cod1ActorEngineSceneBusyEpoch != lease->entity.epoch &&
        !a->spatial.busy && a->dobj.busy == busy &&
        !e->client && !e->r.bmodel && !e->r.contents && !e->s.solid &&
        !e->neverFree && ( e->r.svFlags & SVF_NOCLIENT ) &&
        !( e->r.svFlags & ( SVF_CASTAI | SVF_BOT ) ) &&
        ( !e->r.linked || a->spatial.linked );
}
/* R234 retail attachment wrappers have no nonsolid constructor restriction.
 * Runtime scope is only our current unpublished, lease-owned motion body;
 * ordinary model/DObj construction retains the old stricter gate. */
static int ActorModelTopologyStageValid(const cod1ActorLease_t *lease,int busy) {
    const cod1ActorRecord_t *a;const gentity_t *e;int x,zmin,zmax,solid;
    if(ActorTopologyStageValid(lease,busy))return 1;
    if(!ActorLeaseValid(lease)||lease->kind!=COD1_ACTOR_LEASE_ACTOR)return 0;
    a=cod1Actors+lease->actorIndex;e=g_entities+lease->entity.slot;
    if(!a->alive||!a->nativeDataInitialized||a->initialized||
       cod1ActorEngineSceneBusyEpoch==lease->entity.epoch||a->spatial.busy||a->dobj.busy!=busy||
       !a->motion.active||a->motion.model||a->motion.busy||!a->spatial.linked||!e->r.linked||
       e->client||e->r.bmodel||e->physicsObject||e->neverFree||
       !(e->r.svFlags&SVF_NOCLIENT)||(e->r.svFlags&(SVF_BOT|SVF_CASTAI))||
       e->r.contents!=CONTENTS_BODY||e->clipmask!=MASK_PLAYERSOLID)return 0;
    if(!isfinite(e->r.maxs[0])||!isfinite(e->r.mins[2])||!isfinite(e->r.maxs[2])||
       e->r.maxs[0]<-1000000||e->r.maxs[0]>1000000||
       e->r.mins[2]<-1000000||e->r.mins[2]>1000000||e->r.maxs[2]<-1000000||e->r.maxs[2]>1000000)return 0;
    x=(int)e->r.maxs[0];zmin=(int)-e->r.mins[2];zmax=(int)(e->r.maxs[2]+32);
    if(x<1)x=1;if(x>255)x=255;if(zmin<1)zmin=1;if(zmin>255)zmin=255;if(zmax<1)zmax=1;if(zmax>255)zmax=255;
    solid=(zmax<<16)|(zmin<<8)|x;return e->s.solid==solid;
}
static int ActorDObjStageValid( const cod1ActorLease_t *lease, int busy ) {
    return ActorTopologyStageValid( lease, busy ) &&
        !cod1Actors[lease->actorIndex].modelBusy &&
        !cod1Actors[lease->actorIndex].modelMethodBusy;
}
static int ActorDObjSummaryValid( const cod1DObjSummary_t *out, uint32_t count ) {
    return out->version == COD1_DOBJ_VERSION && out->bytes == sizeof( *out ) &&
        out->topologySerial && out->modelCount && out->modelCount <= count &&
        out->boneCount >= out->modelCount && out->boneCount < 128 &&
        out->duplicateCount < out->boneCount &&
        out->skippedCount == count - out->modelCount &&
        !( out->classificationModelsMask >> out->modelCount );
}
int G_Cod1ActorBuildDObj( const cod1ActorLease_t *input,
    const cod1DObjModel_t *models, uint32_t count, cod1DObjSummary_t *out,
    char *error, size_t size ) {
    cod1ActorLease_t lease;
    cod1DObjBuild_t request;
    cod1DObjSummary_t result;
    cod1ActorModelComposition_t composition;
    cod1ActorRecord_t *a;
    int status, canceled;
    size_t modelBytes;
    uint64_t previousSerial;
    if ( count < 1 || count > COD1_DOBJ_MODELS_MAX ) {
        EntityError( error, size, "actor DObj requires a base and at most seven attachments" ); return 0;
    }
    modelBytes = count * sizeof( *models );
    if ( !ActorArguments( input, sizeof( *input ), out, sizeof( *out ), error, size ) ||
         !ActorArguments( models, modelBytes, out, sizeof( *out ), error, size ) ) return 0;
    if ( !input || !models || !out ) {
        EntityError( error, size, "actor DObj build requires lease, descriptors and output" ); return 0;
    }
    lease = *input;
    if ( models[0].tag[0] || !ActorDObjStageValid( &lease, 0 ) ) {
        EntityError( error, size, "actor DObj requires a current unpublished hidden native-data lease and untagged base" ); return 0;
    }
    /* Saved strings remain bounded even across an invalid engine reply. Path
     * syntax/resource/topology admission remains the production SV contract. */
    {
        uint32_t i;
        for ( i = 0; i < count; ++i ) {
            if ( !models[i].model[0] ||
                 !memchr( models[i].model, 0, sizeof( models[i].model ) ) ||
                 !memchr( models[i].tag, 0, sizeof( models[i].tag ) ) ) {
                EntityError( error, size, "actor DObj requires bounded model and tag strings" ); return 0;
            }
        }
    }
    memset( &request, 0, sizeof( request ) ); memset( &result, 0, sizeof( result ) );
    request.version = COD1_DOBJ_VERSION; request.bytes = sizeof( request );
    request.key = ActorSpatialKey( &lease ); request.modelCount = count;
    memcpy( request.models, models, modelBytes );
    memset( &composition, 0, sizeof( composition ) );
    composition.descriptorCount = count;
    memcpy( composition.descriptors, request.models, modelBytes );
    previousSerial = cod1Actors[lease.actorIndex].dobj.summary.topologySerial;
    cod1Actors[lease.actorIndex].dobj.busy = 1;
    status = trap_Cod1DObjBuild( &request, &result );
    if ( !ActorDObjStageValid( &lease, 1 ) ||
         ( status == COD1_DOBJ_OK && ( !ActorDObjSummaryValid( &result, count ) ||
             result.topologySerial <= previousSerial ) ) ) {
        /* A successful trap may have published topology before a callback
         * invalidated the game lease/state. Cancel that old key only. Failed
         * engine builds retain an existing instance and need no rollback. */
        canceled = status == COD1_DOBJ_OK ? ActorDObjCancel( &lease ) : COD1_DOBJ_OK;
        if ( ActorLeaseValid( &lease ) ) {
            a = &cod1Actors[lease.actorIndex]; a->dobj.busy = 0;
            if ( status == COD1_DOBJ_OK && canceled == COD1_DOBJ_OK ) {
                a->dobj.present = 0; memset( &a->dobj.summary, 0, sizeof( a->dobj.summary ) );
            } else if ( status == COD1_DOBJ_OK ) {
                a->dobj.present = 1;
                /* Publication could have replaced the old topology. A failed
                 * cancellation retains cleanup ownership, not its old summary. */
                memset( &a->dobj.summary, 0, sizeof( a->dobj.summary ) );
            }
        } else if ( canceled != COD1_DOBJ_OK && canceled != COD1_DOBJ_STALE ) {
            G_Error( "R104: orphan actor DObj could not be canceled by its original key" );
        }
        EntityError( error, size, "actor ownership, unpublished state or DObj result changed during build" ); return 0;
    }
    a = &cod1Actors[lease.actorIndex]; a->dobj.busy = 0;
    if ( status != COD1_DOBJ_OK ) {
        EntityError( error, size, "engine DObj topology build is unavailable or rejected" ); return 0;
    }
    a->dobj.summary = result; a->dobj.present = 1;
    a->modelComposition = composition;
    /* Raw topology probes do not retain a prior registered base assignment.
     * Their native area link must be refreshed by the composite stage. */
    memset( &a->assignedModel, 0, sizeof( a->assignedModel ) );
    memset( &a->registeredAttachments, 0, sizeof( a->registeredAttachments ) );
    *out = result;
    return 1;
}
int G_Cod1ActorFreeDObj( const cod1ActorLease_t *input, char *error, size_t size ) {
    cod1ActorLease_t lease;
    if ( !ActorArguments( input, sizeof( *input ), NULL, 0, error, size ) ) return 0;
    if ( !input ) { EntityError( error, size, "actor DObj free requires a lease" ); return 0; }
    lease = *input;
    return ActorDObjFreeLease( &lease, error, size );
}

#include "g_cod1_actor_model_composition.inc"
#include "g_cod1_actor_dobj_query.inc"
