/* R139: the pure, partial native ammo initializer. This copied preparation is
 * not a world entity and is not a ready-to-publish packet. Model registration,
 * native state publication, linking, physics and pickup are still required.
 * Do not feed its item index to RTCW's ET_ITEM / bg_itemlist renderer. */
#ifndef BG_COD1_ITEM_SPAWN_H
#define BG_COD1_ITEM_SPAWN_H
#include "bg_cod1_item_precache.h"
#include "bg_cod1_ammo_pickup.h"
#include <math.h>

typedef struct cod1AmmoSpawnInit_s {
    cod1ItemDescriptor_t item;
    int32_t spawnflags;
    float origin[3], mins[3], maxs[3];
} cod1AmmoSpawnInit_t;

/* R140 borrowed map-owned server precache slot, not a renderer handle or a
 * native/RTCW network model index. Revalidate before later use. This records
 * the server v14 model/LOD0 skeleton resource only; no world entity exists. */
typedef struct cod1AmmoSpawnModel_s {
    cod1AmmoSpawnInit_t init;
    uint64_t map_epoch;
    uint32_t descriptor_fingerprint;
    int server_model_slot;
} cod1AmmoSpawnModel_t;

/* R144 raw native normal-path seeds. These are deliberately separate from
 * Q3 entity flags, contents, trajectories and ground numbers. In particular,
 * the original allocator defaults do not establish ground word +0xa == 0.
 * A caller supplies its prior native ground and flag values explicitly. */
typedef struct cod1AmmoSpawnNativeSeed_s {
    uint32_t nativeStateFlags, nativeServerFlags, nativeEntityFlags;
    uint32_t nativeGroundEntity;
    float angles[3];
} cod1AmmoSpawnNativeSeed_t;

typedef struct {
    uint32_t type, time, duration;
    float base[3], delta[3];
} cod1AmmoSpawnNativeTrajectory_t;

/* Copied, unpublished normal/static branch, bound to a verified DObj serial.
 * Resource slot is the port server bank, never the retail model byte or a
 * renderer/network model index. Native cells 1b4/1b8 retain opaque values;
 * this stage neither resolves nor calls the corresponding handlers. */
typedef struct cod1AmmoSpawnNativeState_s {
    uint64_t topologySerial;
    uint32_t descriptor_fingerprint;
    int server_model_slot;
    uint32_t nativeKind, nativeItemIndex;
    uint32_t nativeStateFlags, nativeServerFlags, nativeEntityFlags;
    uint32_t nativeContents, nativeCell1b4, nativeCell1b8, nativeGroundEntity;
    uint32_t reserved; /* explicit zero: copied records have no tail padding */
    cod1AmmoSpawnNativeTrajectory_t position, angles;
} cod1AmmoSpawnNativeState_t;

/* The native class dispatcher compares classname bytes exactly, in item-table
 * order (2004151f..200415a6). CI_Resolve is a resource API: pickup-name aliases
 * and case folding from that API are not spawn-class identity. Only confirmed
 * static type-2 rows are selected here; dynamic weapon constructors are outside
 * this partial initializer. Failure leaves the caller's output untouched. */
static int CIS_PrepareAmmoSpawn(const char *classname, const float origin[3],
                               int32_t flags, cod1AmmoSpawnInit_t *out) {
    cod1AmmoSpawnInit_t init;
    const cod1ItemStatic_t *row;
    int i, index = 0, count = CW_Count();
    if (!classname || !*classname || !origin || !out ||
        count < 0 || count > CW_MAX_DEFS) return 0;
    for (i = 0; i < 3; ++i) if (!isfinite(origin[i])) return 0;
    for (i = 1; i < COD1_ITEM_COUNT; ++i) {
        /* Loaded dynamic rows replace their static placeholders. */
        if (i <= count) continue;
        row = &cod1ItemCompiled[i];
        if (row->classname && !strcmp(row->classname, classname)) {
            if (row->type != 2) return 0;
            index = i;
            break;
        }
    }
    if (!index) return 0;
    memset(&init, 0, sizeof(init));
    if (!CI_Descriptor(index, &init.item) || init.item.dynamic ||
        init.item.type != 2 || strcmp(init.item.classname, classname) ||
        !init.item.model_present[0] || !init.item.models[0][0]) return 0;
    init.spawnflags = flags;
    for (i = 0; i < 3; ++i) init.origin[i] = origin[i];
    /* Native type-2 bounds: 2001c67c..2001c6be. They do not depend on the
     * suspended flag. No RTCW ITEM_RADIUS or drop-to-floor trace is used. */
    init.mins[0] = init.mins[1] = -1.0f;
    init.mins[2] = 0.0f;
    init.maxs[0] = init.maxs[1] = 1.0f;
    init.maxs[2] = 2.0f;
    /* item.quantity remains resource metadata, never a gameplay ammo grant. */
    *out = init;
    return 1;
}

/* Exact stores from 2001c6c4..2001c72f and the normal 2001c7af..2001c871
 * branch. The alternate [202db26c]!=0 delayed path is outside this subset.
 * No native CPU execution, Q3 projection or live-item admission is implied.
 * This pure constructor alone does not verify the server DObj instance. */
static int CIS_PrepareAmmoSpawnNativeState(const cod1AmmoSpawnModel_t *model,
    uint64_t topologySerial, const cod1AmmoSpawnNativeSeed_t *seed,
    cod1AmmoSpawnNativeState_t *out) {
    cod1ItemDescriptor_t descriptor;
    cod1AmmoSpawnInit_t checked;
    cod1AmmoSpawnNativeState_t state;
    int i;
    if (!model || !seed || !out || !topologySerial || !model->map_epoch ||
        model->server_model_slot < 1 || model->server_model_slot > COD1_PRECACHE_MODELS ||
        model->descriptor_fingerprint != CI_Fingerprint() || seed->nativeGroundEntity > 2047 ||
        !CI_Descriptor(model->init.item.index, &descriptor) ||
        memcmp(&descriptor, &model->init.item, sizeof(descriptor)) ||
        !CIS_PrepareAmmoSpawn(descriptor.classname, model->init.origin,
            model->init.spawnflags, &checked) || memcmp(&checked, &model->init, sizeof(checked))) return 0;
    for (i = 0; i < 3; ++i) if (!isfinite(seed->angles[i])) return 0;
    memset(&state, 0, sizeof(state));
    state.topologySerial = topologySerial;
    state.descriptor_fingerprint = model->descriptor_fingerprint;
    state.server_model_slot = model->server_model_slot;
    state.nativeKind = 2; state.nativeItemIndex = (uint32_t)descriptor.index;
    state.nativeStateFlags = seed->nativeStateFlags | UINT32_C(0x10);
    state.nativeServerFlags = seed->nativeServerFlags | UINT32_C(0x200);
    state.nativeEntityFlags = seed->nativeEntityFlags | UINT32_C(0x8000);
    state.nativeContents = UINT32_C(0x407c0108);
    state.nativeCell1b4 = 7; state.nativeCell1b8 = 8;
    state.nativeGroundEntity = (model->init.spawnflags & 1) ? seed->nativeGroundEntity : 2047;
    /* Type 2 has no origin-Z adjustment in either suspended branch. Native
     * trajectory type/time/duration/delta are raw zero, not Q3 enum casts. */
    memcpy(state.position.base, model->init.origin, sizeof(state.position.base));
    memcpy(state.angles.base, seed->angles, sizeof(state.angles.base));
    *out = state;
    return 1;
}
#endif
