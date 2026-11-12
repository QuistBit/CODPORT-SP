/* R213 private GAME inventory projection. CAP ordinals are CW index + 1,
 * never WP enum values. This present-state witness is not a grant, pickup
 * authorization, network representation or nonreviving counter ticket. */
#ifndef G_COD1_AMMO_INVENTORY_H
#define G_COD1_AMMO_INVENTORY_H
#include "cod1_gsc/gsc_fixture.h"
#include "bg_cod1_ammo_registry.h"
#define COD1_AMMO_PICKUP_INVENTORY_VERSION 1
typedef struct { int wp, ammoCell, clipCell; } cod1AmmoPickupPortMapping_t;
typedef struct {
    uint32_t version, bytes;
    GscEntityRef player;
    int spawnCount;
    uintptr_t clientOwner, clientPool;
    int maxclients;
    cod1AmmoRegistryReceipt_t registry;
    cod1AmmoPickupInventory_t inventory;
    cod1AmmoPickupPortMapping_t mapping[CW_MAX_DEFS+1];
    uint32_t portWeapons[2];
    int modeBits;
} cod1AmmoPickupInventorySnapshot_t;
/* Exactly two borrowed current callbacks on success. Refusal preserves out;
 * callback mutations are not rolled back. Ref/receipt are copied before calls.
 * Bounded engine/private aliases refuse without writing. CAR validation retains
 * its existing floating-point behavior. Pointer witnesses remain GAME-only. */
int G_Cod1ReadAmmoPickupInventory(const GscEntityRef *player,
    const cod1AmmoRegistryReceipt_t *registry,
    cod1AmmoPickupInventorySnapshot_t *out, char *error, size_t size,
    int (*current)(void *), void *context);
/* Pure relevant-state reprojection, also usable inside a Read callback.
 * Counter restoration under the same owner may restore currentness. Legitimate
 * spawn progression/map/allocation/client/registry replacement invalidates the
 * witness; foreign memory restoration is not detected as tampering. */
int G_Cod1AmmoPickupInventoryCurrent(const cod1AmmoPickupInventorySnapshot_t *snapshot);
#endif
