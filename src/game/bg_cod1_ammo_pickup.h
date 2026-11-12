/* R147 copied native ammo capacity and type-2 pickup eligibility.
 * Original gamex86.dll 2000b540..2000b673 and 200011f8..20001246.
 * Native weapon/ammo/clip indices are separate from RTCW WP/ET_ITEM indices.
 * No live player-state acquisition, ammo transfer, handler or VM admission. */
#ifndef BG_COD1_AMMO_PICKUP_H
#define BG_COD1_AMMO_PICKUP_H
#include "bg_cod1_weapondef.h"
#include <stdint.h>
#include <limits.h>
#include <string.h>

#define COD1_AMMO_PICKUP_CELLS 64
typedef struct {
    int ammoIndex, clipIndex, sharedCapIndex, clipOnly;
} cod1AmmoPickupWeapon_t;

/* A bounded copy of the original indexed domains. Weapon zero is unused;
 * selected count is the port's CW_MAX_DEFS, not a retail registry-size claim.
 * Owned bits retain the original one-based weapon numbering, including bit32.
 * This snapshot is not an authoritative player lease or pickup authorization. */
typedef struct {
    int weaponCount;
    cod1AmmoPickupWeapon_t weapons[CW_MAX_DEFS + 1];
    uint32_t owned[(CW_MAX_DEFS + 32) / 32];
    int32_t ammo[COD1_AMMO_PICKUP_CELLS], clip[COD1_AMMO_PICKUP_CELLS];
    int32_t maxAmmo[COD1_AMMO_PICKUP_CELLS], maxClip[COD1_AMMO_PICKUP_CELLS];
    int32_t sharedCap[COD1_AMMO_PICKUP_CELLS];
} cod1AmmoPickupInventory_t;

static int CAP_WeaponValid(const cod1AmmoPickupWeapon_t *weapon) {
    return weapon->ammoIndex >= 0 && weapon->ammoIndex < COD1_AMMO_PICKUP_CELLS &&
        weapon->clipIndex >= 0 && weapon->clipIndex < COD1_AMMO_PICKUP_CELLS &&
        weapon->sharedCapIndex < COD1_AMMO_PICKUP_CELLS &&
        (weapon->clipOnly == 0 || weapon->clipOnly == 1);
}

static int CAP_Owned(const cod1AmmoPickupInventory_t *inventory, int weaponIndex) {
    return (inventory->owned[weaponIndex / 32] &
        (UINT32_C(1) << (weaponIndex % 32))) != 0;
}

/* Signed headroom is intentionally not clamped. Over-capacity states yield
 * zero/negative admission, as the native helper's caller tests result > 0.
 * Nonnegative counters/caps and a representable signed result are the chosen
 * arithmetic domain; x86 overflow and malformed registry data are not guessed.
 * Failure preserves out. Only cells actually read by this branch are used. */
static int CAP_AmmoHeadroom(const cod1AmmoPickupInventory_t *inventory,
    int weaponIndex, int32_t *out) {
    const cod1AmmoPickupWeapon_t *target, *weapon;
    unsigned char ammoSeen[COD1_AMMO_PICKUP_CELLS], clipSeen[COD1_AMMO_PICKUP_CELLS];
    int64_t room;
    int i, index;
    if (!inventory || !out || inventory->weaponCount < 1 ||
        inventory->weaponCount > CW_MAX_DEFS || weaponIndex < 1 ||
        weaponIndex > inventory->weaponCount) return 0;
    target = &inventory->weapons[weaponIndex];
    if (!CAP_WeaponValid(target)) return 0;
    if (target->sharedCapIndex < 0) {
        index = target->clipOnly ? target->clipIndex : target->ammoIndex;
        if (target->clipOnly) {
            if (inventory->maxClip[index] < 0 || inventory->clip[index] < 0) return 0;
            room = (int64_t)inventory->maxClip[index] - inventory->clip[index];
        } else {
            if (inventory->maxAmmo[index] < 0 || inventory->ammo[index] < 0) return 0;
            room = (int64_t)inventory->maxAmmo[index] - inventory->ammo[index];
        }
    } else {
        if (inventory->sharedCap[target->sharedCapIndex] < 0) return 0;
        room = inventory->sharedCap[target->sharedCapIndex];
        memset(ammoSeen, 0, sizeof(ammoSeen)); memset(clipSeen, 0, sizeof(clipSeen));
        for (i = 1; i <= inventory->weaponCount; ++i) {
            if (!CAP_Owned(inventory, i)) continue;
            weapon = &inventory->weapons[i];
            /* The native scan reads each owned weapon's group before indices. */
            if (weapon->sharedCapIndex != target->sharedCapIndex) continue;
            if (!CAP_WeaponValid(weapon)) return 0;
            if (weapon->clipOnly) {
                index = weapon->clipIndex;
                if (clipSeen[index]) continue;
                if (inventory->clip[index] < 0) return 0;
                clipSeen[index] = 1; room -= inventory->clip[index];
            } else {
                index = weapon->ammoIndex;
                if (ammoSeen[index]) continue;
                if (inventory->ammo[index] < 0) return 0;
                ammoSeen[index] = 1; room -= inventory->ammo[index];
            }
        }
    }
    if (room < INT32_MIN || room > INT32_MAX) return 0;
    *out = (int32_t)room;
    return 1;
}

/* Exact type-2 branch: an unowned target needs clipOnly; either admitted
 * ownership branch needs positive capacity. No grant/ownership mutation. */
static int CAP_CanPickupAmmo(const cod1AmmoPickupInventory_t *inventory,
    int weaponIndex, int *out) {
    int32_t room;
    if (!inventory || !out || inventory->weaponCount < 1 ||
        inventory->weaponCount > CW_MAX_DEFS || weaponIndex < 1 ||
        weaponIndex > inventory->weaponCount || !CAP_WeaponValid(&inventory->weapons[weaponIndex])) return 0;
    if (!CAP_Owned(inventory, weaponIndex) && !inventory->weapons[weaponIndex].clipOnly) {
        *out = 0; return 1;
    }
    if (!CAP_AmmoHeadroom(inventory, weaponIndex, &room)) return 0;
    *out = room > 0;
    return 1;
}
#endif
