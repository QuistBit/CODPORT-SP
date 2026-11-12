#ifndef G_COD1_TURRET_MOUNT_FIT_H
#define G_COD1_TURRET_MOUNT_FIT_H
#include "g_cod1_turret.h"
typedef struct {
 GscEntityRef gun,actor;int tick,endpoint;uint64_t revision;
 float tagWeapon[16],tagAim[16],rootRotation[2],rootTranslation[3],endpointRootZ;
} cod1TurretMountFitSample_t;
/* Provider returns actual current model-space tags/current tree-selected root,
 * with a nonzero receipt revision. It must not mutate GAME state. */
int G_Cod1TurretMountFit(const GscEntityRef*,int,
 int(*sample)(void*,const GscEntityRef*,cod1TurretMountFitSample_t*),
 int(*current)(void*),void*);
#endif
