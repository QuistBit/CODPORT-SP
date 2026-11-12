#ifndef G_COD1_GRENADE_PLAYER_H
#define G_COD1_GRENADE_PLAYER_H
#include "cod1_gsc/gsc_fixture.h"
/* R217 selected training host. All legacy operations retain their provider.
 * Fraction: owned clipOnly -> clip/maxClip; owned other -> stock/maxAmmo.
 * Native non-owned/none returns1 (gamex86 20015740..200158B6).
 * Current registry, player, finite counters and callback fences remain required.
 * Unknown names and unsupported zero-capacity owned domains refuse.
 * This is a port projection, not a proprietary inventory/FPU ABI claim. */
const GscPlayerNativeHost *G_Cod1GrenadePlayerNativeHost(void);
#endif
