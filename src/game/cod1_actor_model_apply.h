/* R161 checked, unpublished actor model assignment. This paired port import
 * joins a real registered base resource, staged topology and native area link.
 * No current animated pose, DObj narrow phase or actor/AI publication. */
#ifndef COD1_ACTOR_MODEL_APPLY_H
#define COD1_ACTOR_MODEL_APPLY_H
#include "cod1_dobj.h"
#include "cod1_precache.h"
#define COD1_ACTOR_MODEL_APPLY_VERSION 1u
typedef struct {
    uint32_t version, bytes;
    cod1DObjBuild_t build;
    cod1ModelResourceReference_t base;
    uint64_t previousTopologySerial;
    uint32_t modelIndex, nativeContents, nativeServerFlags, reserved;
    char nativeName[MAX_QPATH];
} cod1ActorModelApply_t;
typedef struct {
    uint32_t version, bytes;
    cod1DObjSummary_t topology;
    uint32_t linked, reserved;
} cod1ActorModelApplyResult_t;
/* Nonzero modelIndex requires a current base reference and exact CS spelling.
 * Zero clears topology while retaining GAME's authored attachments. The actor
 * must already have a checked native placement/link. Ordinary failure keeps
 * the previous topology and area registration, and leaves output unchanged.
 * The first resource is copied from the registered decoded bank, not reloaded
 * by filename. Separate DObj ownership keeps that copy alive through a reset.
 * Native bit8 means fixed XY64 area bounds; bit10 means first-model XY bounds;
 * PVS and Z still use the ordinary entity hull. Bit8 narrow-phase queries
 * return UNAVAILABLE until a real animated DObj collision provider exists.
 * EXE/GAME ABI58. Existing records and imports1000..1016 are unchanged. */
#endif
