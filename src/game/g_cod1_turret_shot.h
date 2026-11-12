#ifndef G_COD1_TURRET_SHOT_H
#define G_COD1_TURRET_SHOT_H
#if defined(COD1_GSC_TEST_WIP)&&COD1_GSC_TEST_WIP
#include "g_cod1_actor.h"
/* Actual selected SP ClientSpawn only, after full capture/connected ownership,
 * MapReady and before Session. No running-Session requirement. Slot0 only. */
int G_Cod1TurretShotPlayerBegin(const GscEntityRef*);
#endif
#endif
