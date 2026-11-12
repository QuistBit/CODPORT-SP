#ifndef G_COD1_PLAYER_LIFETIME_H
#define G_COD1_PLAYER_LIFETIME_H
/* GAME-private completed ClientBegin provenance. This contains no native
 * C4/134, collision, path, VM-phase or pose authority. Issuance is static
 * in g_client.c; an observer callback cannot manufacture a completion.
 * Receipts never cross GAME module/save/wire boundaries. */
#if defined(COD1_GSC_TEST_WIP) && COD1_GSC_TEST_WIP
#include "cod1_gsc/gsc_fixture.h"
typedef struct {
 GscEntityRef entity;
 uint64_t completion,activity;
 int32_t spawn_count,completed_time;
} cod1PlayerLifetime_t;
int G_Cod1PlayerLifetimeRead(const GscEntityRef*,cod1PlayerLifetime_t*);
/* Historical completed-spawn identity survives ordinary input/pregame flags.
 * g_reloading/saveGamePending do not say whether ClientBegin returned.
 * Full restore retires before copies; persistent transfer advances activity.
 * This fact never certifies load/world/native readiness. */
int G_Cod1PlayerLifetimeSameCompleted(const cod1PlayerLifetime_t*);
/* Only the explicitly hooked lifecycle/input/frame activity is tracked.
 * This is NOT universal field immutability or a native path certificate.
 * Read never refreshes this immutable completion-time activity stamp. */
int G_Cod1PlayerLifetimeUnadvanced(const cod1PlayerLifetime_t*);
void G_Cod1PlayerLifetimeActivity(gentity_t*);
void G_Cod1PlayerLifetimeRetire(gentity_t*);
void G_Cod1PlayerLifetimeMapReset(void);
void G_Cod1PlayerLifetimeBeforeBirth(gentity_t*,const GscEntityRef*);
void G_Cod1PlayerLifetimeAfterBirth(gentity_t*);
int G_Cod1PlayerLifetimePrivate(const void*,size_t);
#else
#define G_Cod1PlayerLifetimeActivity(e) ((void)0)
#define G_Cod1PlayerLifetimeRetire(e) ((void)0)
#define G_Cod1PlayerLifetimeMapReset() ((void)0)
#define G_Cod1PlayerLifetimeBeforeBirth(e,r) ((void)0)
#define G_Cod1PlayerLifetimeAfterBirth(e) ((void)0)
#endif
#endif
