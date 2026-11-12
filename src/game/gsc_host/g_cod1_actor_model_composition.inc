/* R156 authored composition ownership and base replacement. Included after
 * the checked DObj Build/Free stage. No engine pointer/string is retained. */
int G_Cod1ActorReadModelComposition( const cod1ActorLease_t *input,
    cod1ActorModelComposition_t *out, char *error, size_t size ) {
    cod1ActorLease_t lease;
    if ( !ActorArguments( input, sizeof( *input ), out, sizeof( *out ), error, size ) ||
         ActorOverlap( input, sizeof( *input ), out, sizeof( *out ) ) ) return 0;
    if ( !input || !out ) {
        EntityError( error, size, "actor composition read requires lease and output" ); return 0;
    }
    lease = *input;
    if ( !ActorLeaseValid( &lease ) || lease.kind != COD1_ACTOR_LEASE_ACTOR ) {
        EntityError( error, size, "actor composition read requires a current actor owner" ); return 0;
    }
    *out = cod1Actors[lease.actorIndex].modelComposition;
    return 1;
}

/* The candidate is a private pre-trap copy. Build commits its copied authored
 * request only after success; empty-base cleanup uses the same exact owner. */
static int ActorCommitModelComposition( const cod1ActorLease_t *lease,
    const cod1ActorModelComposition_t *composition, cod1DObjSummary_t *out,
    char *error, size_t size ) {
    cod1DObjSummary_t result;
    memset( &result, 0, sizeof( result ) );
    if ( composition->descriptors[0].model[0] ) {
        if ( !G_Cod1ActorBuildDObj( lease, composition->descriptors,
                 composition->descriptorCount, &result, error, size ) ) return 0;
    } else {
        if ( !ActorDObjFreeLease( lease, error, size ) ) return 0;
        if ( !ActorDObjStageValid( lease, 0 ) ) {
            EntityError( error, size, "actor state changed during empty-base cleanup" ); return 0;
        }
        cod1Actors[lease->actorIndex].modelComposition = *composition;
        memset(&cod1Actors[lease->actorIndex].assignedModel,0,sizeof(cod1ActorAssignedModel_t));
        memset(&cod1Actors[lease->actorIndex].registeredAttachments,0,sizeof(cod1ActorRegisteredAttachments_t));
    }
    *out = result;
    return 1;
}
static int ActorMutableModelComposition( const cod1ActorLease_t *lease,
    cod1ActorModelComposition_t *composition, char *error, size_t size ) {
    if ( !ActorDObjStageValid( lease, 0 ) ) {
        EntityError( error, size, "actor model mutation requires current hidden unpublished native data" ); return 0;
    }
    *composition = cod1Actors[lease->actorIndex].modelComposition;
    if ( composition->descriptorCount > COD1_DOBJ_MODELS_MAX ) {
        EntityError( error, size, "actor saved composition is invalid" ); return 0;
    }
    if ( !composition->descriptorCount ) composition->descriptorCount = 1;
    return 1;
}

int G_Cod1ActorReplaceDObjBase( const cod1ActorLease_t *input,
    const cod1DObjModel_t *base, cod1DObjSummary_t *out, char *error, size_t size ) {
    cod1ActorLease_t lease;
    cod1DObjModel_t copiedBase;
    cod1ActorModelComposition_t composition;
    if ( !ActorArguments( input, sizeof( *input ), out, sizeof( *out ), error, size ) ||
         !ActorArguments( base, sizeof( *base ), out, sizeof( *out ), error, size ) ||
         ActorOverlap( input, sizeof( *input ), base, sizeof( *base ) ) ||
         ActorOverlap( input, sizeof( *input ), out, sizeof( *out ) ) ||
         ActorOverlap( base, sizeof( *base ), out, sizeof( *out ) ) ) return 0;
    if ( !input || !base || !out ) {
        EntityError( error, size, "actor base replacement requires lease, descriptor and output" ); return 0;
    }
    lease = *input; copiedBase = *base;
    if ( copiedBase.tag[0] || !memchr( copiedBase.model, 0, sizeof( copiedBase.model ) ) ||
         !ActorDObjStageValid( &lease, 0 ) ) {
        EntityError( error, size, "actor base replacement requires a bounded untagged base and current hidden native data" ); return 0;
    }
    if ( !ActorMutableModelComposition( &lease, &composition, error, size ) ) return 0;
    composition.descriptors[0] = copiedBase;
    return ActorCommitModelComposition( &lease, &composition, out, error, size );
}

int G_Cod1ActorAppendDObjAttachment( const cod1ActorLease_t *input,
    const cod1DObjModel_t *attachment, cod1DObjSummary_t *out, char *error, size_t size ) {
    cod1ActorLease_t lease;
    cod1DObjModel_t descriptor;
    cod1ActorModelComposition_t composition;
    if ( !ActorArguments( input, sizeof( *input ), out, sizeof( *out ), error, size ) ||
         !ActorArguments( attachment, sizeof( *attachment ), out, sizeof( *out ), error, size ) ||
         ActorOverlap( input, sizeof( *input ), attachment, sizeof( *attachment ) ) ||
         ActorOverlap( input, sizeof( *input ), out, sizeof( *out ) ) ||
         ActorOverlap( attachment, sizeof( *attachment ), out, sizeof( *out ) ) ) return 0;
    if ( !input || !attachment || !out ) {
        EntityError( error, size, "actor attachment append requires lease, descriptor and output" ); return 0;
    }
    lease = *input; descriptor = *attachment;
    if ( !descriptor.model[0] || !memchr( descriptor.model, 0, sizeof( descriptor.model ) ) ||
         !memchr( descriptor.tag, 0, sizeof( descriptor.tag ) ) ) {
        EntityError( error, size, "actor attachment requires bounded model and tag strings" ); return 0;
    }
    if ( !ActorMutableModelComposition( &lease, &composition, error, size ) ) return 0;
    /* Registration with no base is still an external native dependency; this
     * raw constructor stage admits only the existing checked nonempty base. */
    if ( !composition.descriptors[0].model[0] ||
         composition.descriptorCount == COD1_DOBJ_MODELS_MAX ) {
        EntityError( error, size, "actor attachment append requires a base and a free authored slot" ); return 0;
    }
    composition.descriptors[composition.descriptorCount++] = descriptor;
    return ActorCommitModelComposition( &lease, &composition, out, error, size );
}

int G_Cod1ActorRemoveDObjAttachment( const cod1ActorLease_t *input,
    uint32_t descriptorIndex, cod1DObjSummary_t *out, char *error, size_t size ) {
    cod1ActorLease_t lease;
    cod1ActorModelComposition_t composition;
    size_t tail;
    if ( !ActorArguments( input, sizeof( *input ), out, sizeof( *out ), error, size ) ||
         ActorOverlap( input, sizeof( *input ), out, sizeof( *out ) ) ) return 0;
    if ( !input || !out ) {
        EntityError( error, size, "actor attachment removal requires lease and output" ); return 0;
    }
    lease = *input;
    if ( !ActorMutableModelComposition( &lease, &composition, error, size ) ) return 0;
    if ( !descriptorIndex || descriptorIndex >= composition.descriptorCount ) {
        EntityError( error, size, "actor removal requires a current authored attachment index" ); return 0;
    }
    tail = ( composition.descriptorCount - descriptorIndex - 1 ) * sizeof( composition.descriptors[0] );
    memmove( &composition.descriptors[descriptorIndex], &composition.descriptors[descriptorIndex + 1], tail );
    --composition.descriptorCount;
    memset( &composition.descriptors[composition.descriptorCount], 0, sizeof( composition.descriptors[0] ) );
    return ActorCommitModelComposition( &lease, &composition, out, error, size );
}

int G_Cod1ActorClearDObjAttachments( const cod1ActorLease_t *input,
    cod1DObjSummary_t *out, char *error, size_t size ) {
    cod1ActorLease_t lease;
    cod1ActorModelComposition_t composition;
    if ( !ActorArguments( input, sizeof( *input ), out, sizeof( *out ), error, size ) ||
         ActorOverlap( input, sizeof( *input ), out, sizeof( *out ) ) ) return 0;
    if ( !input || !out ) {
        EntityError( error, size, "actor attachment clear requires lease and output" ); return 0;
    }
    lease = *input;
    if ( !ActorMutableModelComposition( &lease, &composition, error, size ) ) return 0;
    composition.descriptorCount = 1;
    memset( &composition.descriptors[1], 0, sizeof( composition.descriptors ) - sizeof( composition.descriptors[0] ) );
    return ActorCommitModelComposition( &lease, &composition, out, error, size );
}
