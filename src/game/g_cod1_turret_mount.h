#ifndef G_COD1_TURRET_MOUNT_H
#define G_COD1_TURRET_MOUNT_H
#include "g_cod1_actor.h"
#if defined(COD1_GSC_TEST_WIP)&&COD1_GSC_TEST_WIP
/* startStand:1 scheduled once;0 guarantees no thread was started. notify has
 * the same accepted1/notdelivered0 contract. Call outside Session invoke. */
int G_Cod1TurretMountRequest(const GscEntityRef*,const GscEntityRef*,int,int(*)(void*),void*,int*);
int G_Cod1TurretMountFrame(int,int(*)(void*),int(*)(void*,const GscEntityRef*,const char*),int(*)(void*,const GscEntityRef*,const GscEntityRef*,const char*,const char*),void*);
int G_Cod1TurretMountRead(const GscEntityRef*,cod1ActorTurretMount_t*);
#endif
#endif
