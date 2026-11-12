#ifndef G_COD1_SPAWNER_POSE_H
#define G_COD1_SPAWNER_POSE_H
#if defined(COD1_GSC_TEST_WIP) && COD1_GSC_TEST_WIP
#include "cod1_gsc/gsc_fixture.h"
/* Explicit origin-only provider for current hidden typed BSP spawners.
 * The base EntityHost and its ordinary script-entity domain are unchanged.
 * A spawner write changes current stationary pose, including its copied
 * snapshot, while raw BSP text/intake identity/native extras remain intact.
 * No Actor/Sentient, link, model, controller, spawn or playback is supplied. */
const GscEntityHost *G_Cod1SpawnerPoseEntityHost(void);
/* Stateful/controls profiles dispatch origin writes through this table first.
 * Its ordinary trigger/script fallback remains the base ControlHost's own. */
const GscEntityControlHost *G_Cod1SpawnerPoseControlHost(void);
#endif
#endif
