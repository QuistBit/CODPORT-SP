/* R145 private port projection for a static native type-2 ammo model.
 * This packet is not the original CoD entity layout and never uses RTCW's
 * ET_ITEM/bg_itemlist index. Native raw flags/contents/handler cells remain
 * in GAME's sidecar. Existing wire fields carry this explicitly marked lane;
 * no world producer is selected by this header. */
#ifndef BG_COD1_AMMO_ENTITY_H
#define BG_COD1_AMMO_ENTITY_H
/* Include after g_local.h/cg_local.h (or bg_public.h in a standalone test).
 * The legacy bg_public.h has no guard and must not be included twice. */
#include "bg_cod1_item_precache.h"
#include <math.h>

#define COD1_AMMO_NET_MARKER 679
#define COD1_AMMO_NET_VERSION 1

typedef struct {
    int slot, itemIndex, networkModelSlot;
    uint32_t fingerprint;
    float origin[3], angles[3];
} cod1AmmoEntityView_t;

/* networkModelSlot is an independently checked CS_MODELS index. It is never
 * inferred from the server resource bank slot. Fingerprint bits are copied
 * without a signed integer conversion; MSG's time field transports all32. */
static int CAN_BuildPacket(int slot, int itemIndex, int networkModelSlot,
    uint32_t fingerprint, const float origin[3], const float angles[3],
    entityState_t *out) {
    cod1ItemDescriptor_t descriptor;
    entityState_t packet;
    int i;
    if (!out || !origin || !angles || slot < MAX_CLIENTS || slot >= ENTITYNUM_MAX_NORMAL ||
        networkModelSlot < 1 || networkModelSlot >= MAX_MODELS || !fingerprint ||
        fingerprint != CI_Fingerprint() || !CI_Descriptor(itemIndex, &descriptor) ||
        descriptor.dynamic || descriptor.type != 2 || !descriptor.model_present[0] ||
        !descriptor.models[0][0]) return 0;
    for (i = 0; i < 3; ++i) if (!isfinite(origin[i]) || !isfinite(angles[i])) return 0;
    memset(&packet, 0, sizeof(packet));
    packet.number = slot; packet.eType = ET_GENERAL;
    packet.density = COD1_AMMO_NET_MARKER;
    packet.modelindex = networkModelSlot; packet.modelindex2 = itemIndex;
    memcpy(&packet.time, &fingerprint, sizeof(fingerprint));
    packet.time2 = COD1_AMMO_NET_VERSION;
    packet.groundEntityNum = ENTITYNUM_NONE;
    packet.pos.trType = packet.apos.trType = TR_STATIONARY;
    memcpy(packet.pos.trBase, origin, sizeof(packet.pos.trBase));
    memcpy(packet.origin, origin, sizeof(packet.origin));
    memcpy(packet.apos.trBase, angles, sizeof(packet.apos.trBase));
    memcpy(packet.angles, angles, sizeof(packet.angles));
    *out = packet;
    return 1;
}

/* Validate the whole selected static packet, including unused zero fields.
 * A malformed marked packet must not fall through to generic item/model
 * rendering. Failure preserves output. The caller still checks the current
 * map, CS model path and positive renderer handle before submission. */
static int CAN_ReadPacket(const entityState_t *input, cod1AmmoEntityView_t *out) {
    entityState_t packet, expected;
    cod1AmmoEntityView_t view;
    uint32_t fingerprint;
    if (!input || !out) return 0;
    packet = *input;
    memcpy(&fingerprint, &packet.time, sizeof(fingerprint));
    if (!CAN_BuildPacket(packet.number, packet.modelindex2, packet.modelindex,
            fingerprint, packet.pos.trBase, packet.apos.trBase, &expected) ||
        memcmp(&packet, &expected, sizeof(packet))) return 0;
    memset(&view, 0, sizeof(view));
    view.slot = packet.number; view.itemIndex = packet.modelindex2;
    view.networkModelSlot = packet.modelindex; view.fingerprint = fingerprint;
    memcpy(view.origin, packet.pos.trBase, sizeof(view.origin));
    memcpy(view.angles, packet.apos.trBase, sizeof(view.angles));
    *out = view;
    return 1;
}
#endif
