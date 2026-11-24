/* R103 paired native engine/game queries. Original collision bit lanes are
 * data in this interface; they are never RTCW entity types or AI flags.
 * This is a private port ABI, not a retail CoD or QVM syscall definition. */
#ifndef COD1_SPATIAL_H
#define COD1_SPATIAL_H
#include <stdint.h>

#define COD1_SPATIAL_VERSION 1u
#define G_COD1_PATH_WORLD_SIGHT 1030 /* private paired GAME/engine ABI77 */
#define COD1_SPATIAL_ACTOR_CONTENTS UINT32_C(0x00004000)
#define COD1_SPATIAL_NATIVE_DOBJ_FLAG UINT32_C(0x00000008)
#define COD1_SPATIAL_NATIVE_CAPSULE_FLAG UINT32_C(0x00000200)
#define COD1_SPATIAL_FLOOR_MASK UINT32_C(0x02820011)
#define COD1_SPATIAL_OCCUPANCY_MASK UINT32_C(0x0200c000)
#define COD1_SPATIAL_SIGHT_MASK UINT32_C(0x00000001)

enum {
    COD1_SPATIAL_OK = 1,
    COD1_SPATIAL_INVALID = -1,
    COD1_SPATIAL_UNAVAILABLE = -2,
    COD1_SPATIAL_OVERFLOW = -3,
    COD1_SPATIAL_STALE = -4
};

typedef struct {
    uint32_t version, bytes;
    float start[3], end[3], mins[3], maxs[3];
    int32_t passEntity[2];
    uint32_t contentsMask;
} cod1SpatialTraceQuery_t;

typedef struct {
    uint32_t version, bytes;
    float mins[3], maxs[3];
    uint32_t contentsMask;
} cod1SpatialAreaQuery_t;

typedef struct {
    uint32_t version, bytes;
    float start[3], end[3];
    int32_t passEntity[2];
    uint32_t contentsMask;
} cod1SpatialSightQuery_t;

/* The epoch/generation/lease are opaque game-owned identities. The engine
 * checks exact registered keys, never infers a new generation from a slot.
 * The game must validate its allocation and lease before AND after a trap.
 * The engine cannot independently inspect the private game lease table. */
typedef struct {
    uint64_t epoch, lease;
    uint32_t slot, generation;
} cod1SpatialActorKey_t;

typedef struct {
    uint32_t version, bytes;
    cod1SpatialActorKey_t key;
    uint32_t nativeContents, nativeServerFlags;
    uint32_t hasDObj;
} cod1SpatialActorLink_t;

typedef struct {
    uint32_t version, bytes;
    cod1SpatialActorKey_t key;
} cod1SpatialActorUnlink_t;

/* Exact registered actor contents compare/exchange. No callbacks, relink,
 * pose/model/flag mutation or ownership creation. Absence is STALE, including
 * expected==replacement. Cleanup can restore contents after a pose changed:
 * association is required; query eligibility is checked by the next query. */
typedef struct {
    uint32_t version, bytes;
    cod1SpatialActorKey_t key;
    uint32_t expectedContents, replacementContents;
} cod1SpatialActorContents_t;

typedef struct {
    uint32_t version, bytes;
    uint64_t epoch;
} cod1SpatialSceneReset_t;

/* Fixed native path hull (-15,-15,18)..(15,15,72), mask02820011.
 * Zero actor key means ordinary typed entity; native actor requires exact
 * registered key. Status is separate from original result 0/-1. */
typedef struct {
    uint32_t version,bytes,slot,expectedContents;
    cod1SpatialActorKey_t actor;
    float start[3],end[3];
} cod1PathSightQuery_t;

/* Checked query outputs are committed only on OK. Area OVERFLOW publishes
 * the required matching count and leaves the slot buffer unchanged: filtering
 * happens before capacity, never after a truncated unmasked query.
 * Entity exclusions use port ENTITYNUM_NONE; real slots retain native direct
 * slot/child/same-owner rules for both exclusions. Queries do not edit shared
 * contents, flags or owners. Native link is the explicitly supported no-DObj
 * bbox/capsule subset. hasDObj!=0 is UNAVAILABLE until a real backend exists.
 * Ordinary server unlink and world clear remove private lane registrations.
 * A provisional actor must already be hidden with SVF_NOCLIENT and must not
 * carry legacy BOT/CASTAI identity. Linking does not add these flags.
 * Native link consumes the actual current shared pose/bbox. It does not turn
 * a provisional lease into an initialized actor, snapshot or spawned event.
 * Reset with a different active epoch is STALE and cannot clear that scene. */
#endif
