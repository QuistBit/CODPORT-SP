/* R129 internal server read of a currently owned DObj topology. This record
 * contains copied source associations, never a current pose or client actor
 * admission. It adds no GAME/cgame syscall and no serialized network field. */
#ifndef COD1_DOBJ_EXPORT_H
#define COD1_DOBJ_EXPORT_H
#include "cod1_dobj.h"

#define COD1_DOBJ_EXPORT_VERSION 1u
#define COD1_DOBJ_EXPORT_BONES_MAX 128u
#define COD1_DOBJ_EXPORT_DUPLICATE_BYTES (16u + 2u * (128u - 1u) + 1u)

typedef struct {
    uint32_t version, bytes;
    cod1SpatialActorKey_t key;
    uint64_t topologySerial;
} cod1DObjExportRequest_t;
typedef struct {
    cod1DObjModel_t authored; /* original accepted input spelling and tag */
    char resource[COD1_DOBJ_NAME_MAX]; /* exact leased resource-bank spelling */
    uint32_t inputIndex, firstBone, boneCount;
    int32_t anchorBone; /* -1 means common object root; no pose is returned */
} cod1DObjExportModel_t;
typedef struct {
    uint32_t modelIndex, localBone;
    int32_t parentBone, duplicateBone;
    uint32_t canonicalBone;
} cod1DObjExportBone_t;
typedef struct {
    uint32_t version, bytes;
    cod1SpatialActorKey_t key;
    uint64_t topologySerial;
    uint32_t authoredCount, modelCount, boneCount, duplicateCount;
    uint32_t warningCount, missingTagInputMask, unmeldedRootInputMask;
    uint32_t classificationModelsMask, duplicateBlobSize;
    cod1DObjExportModel_t models[COD1_DOBJ_MODELS_MAX];
    cod1DObjExportBone_t bones[COD1_DOBJ_EXPORT_BONES_MAX];
    unsigned char duplicateBlob[COD1_DOBJ_EXPORT_DUPLICATE_BYTES];
} cod1DObjExportResult_t;

/* outBytes must equal sizeof(cod1DObjExportResult_t). Request and output are
 * valid independent caller storage; overlap with the request, any published
 * DObj instance/resource bank, or its associated shared entity is refused.
 * The server validates key, exact revision, current entity association, every
 * accepted resource lease and ordered source topology before its only write.
 * Failure leaves all output bytes unchanged. Success copies data and owns no
 * resource/pointer: free/rebuild/reuse/map reset invalidates this association.
 * No FS, allocator, GAME callback, snapshot, pose or renderer call occurs.
 * This internal API cannot independently validate GAME's private lease table
 * and must not be used to infer actor admission from a slot/resource name. */
#endif
