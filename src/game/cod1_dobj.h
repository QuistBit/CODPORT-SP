/* R104: private native engine/game DObj TOPOLOGY boundary. This preserves
 * resource identities and skeletal composition; it does not prepare a current
 * animated pose, admit an actor, render it, or enable locational collision.
 * It is a paired port ABI, not the retail CoD/QVM syscall layout. */
#ifndef COD1_DOBJ_H
#define COD1_DOBJ_H
#include "cod1_spatial.h"
#include "../qcommon/cod1_model_metadata.h"

#define COD1_DOBJ_VERSION 1u
#define COD1_DOBJ_MODELS_MAX 8u
#define COD1_DOBJ_NAME_MAX 128u

enum {
    COD1_DOBJ_OK = 1,
    COD1_DOBJ_INVALID = -1,
    COD1_DOBJ_UNAVAILABLE = -2,
    COD1_DOBJ_OVERFLOW = -3,
    COD1_DOBJ_STALE = -4
};

typedef struct {
    char model[COD1_DOBJ_NAME_MAX]; /* exact full xmodel/ resource path */
    char tag[COD1_DOBJ_NAME_MAX];   /* empty requests native named-bone melding */
} cod1DObjModel_t;

typedef struct {
    uint32_t version, bytes;
    cod1SpatialActorKey_t key;
    uint32_t modelCount;
    uint32_t animationTree; /* nonzero explicitly UNAVAILABLE in this stage */
    cod1DObjModel_t models[COD1_DOBJ_MODELS_MAX];
} cod1DObjBuild_t;

typedef struct {
    uint32_t version, bytes;
    uint64_t topologySerial;
    uint32_t modelCount, boneCount, duplicateCount, skippedCount;
    uint32_t classificationModelsMask;
    c1mBounds_t firstModelBounds; /* original first XModel; not posed/union bounds */
} cod1DObjSummary_t;

typedef struct {
    uint32_t version, bytes;
    cod1SpatialActorKey_t key;
} cod1DObjFree_t;

typedef struct {
    uint32_t version, bytes;
    uint64_t epoch;
} cod1DObjSceneReset_t;

/* R125: read-only query of an already owned topology. This separate version
 * leaves Build/Free/SceneReset and their records unchanged. MODEL requires an
 * empty tag; BONE uses exact byte-sensitive named lookup (empty is legal).
 * A missing bone is a successful query with boneIndex=-1. No instance/model
 * is created, no source bind transform is returned as an animated pose. */
#define COD1_DOBJ_QUERY_VERSION 1u
enum { COD1_DOBJ_QUERY_MODEL = 1, COD1_DOBJ_QUERY_BONE = 2 };
typedef struct {
    uint32_t version, bytes;
    cod1SpatialActorKey_t key;
    uint64_t topologySerial; /* exact nonzero revision from a successful Build */
    uint32_t phase;
    char tag[COD1_DOBJ_NAME_MAX];
} cod1DObjQuery_t;
typedef struct {
    uint32_t version, bytes;
    uint64_t topologySerial;
    uint32_t modelCount, boneCount;
    int32_t boneIndex;
    char model[COD1_DOBJ_NAME_MAX]; /* copied first accepted XModel resource */
} cod1DObjQueryResult_t;
/* Query checks the exact key, serial, current entity-table association and
 * resource-bank leases. Failure leaves output unchanged. Ordinary unlink
 * keeps the independent topology queryable; free/reset/rebuild invalidate its
 * old key or revision. The native DLL import requires game/executable ABI53. */

/* The key is the game's exact opaque epoch/lease/allocation identity. The
 * engine cannot inspect the private game registry: validate before AND after
 * each trap. Build copies paths/tags, uses headless map-owned resources and
 * publishes only a completely checked topology, leaving output unchanged on
 * failure. Native missing explicit tags skip their descriptor; returned model
 * count can therefore differ from the authored descriptor count. Accepted
 * total bones must be strictly less than 128, even if an individual decoder
 * has capacity 128. Resource bind transforms are not a current server pose.
 * Ordinary unlink preserves this separate instance until explicit keyed free.
 * Absent-key cancellation is idempotent, including an unloaded/smaller scene;
 * it never frees a replacement key. Reset cannot erase another active epoch.
 * Fresh-map sidecars are not serialized into SAVE24. Existing spatial queries
 * retain their no-DObj subset and hasDObj refusal until pose/collision exist. */
#endif
