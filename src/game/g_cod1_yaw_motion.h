/* R216 selected real inline-brush angular mover. This is a port trajectory,
 * not an original CoD trajectory enum or original controller/VM ABI. */
#ifndef G_COD1_YAW_MOTION_H
#define G_COD1_YAW_MOTION_H
#include "cod1_gsc/gsc_fixture.h"
struct gentity_s;
/* Four finite script scalars are quantized once to nearest millisecond.
 * Total1..600000ms; nonnegative accel/decel sum <=total; |yaw|<=3600deg.
 * The selected real script_brushmodel is an independent, stationary linked
 * inline mover. Existing movement, teams, actors and physics roles refuse.
 * Success starts actual G_RunMover/G_MoverPush physics and returns at once. */
const GscYawMotionHost *G_Cod1YawMotionHost(void);
void G_Cod1YawMotionReset(void);
/* Called by the explicitly enabled actual mover after G_MoverTeam. Accounts
 * for its blocked-frame time shift, then settles an exact completed endpoint.
 * No trap or callback occurs here, and a changed owner is never overwritten. */
void G_Cod1YawMotionFinish(struct gentity_s *);
/* Pure own-storage classifier, safe in the general entity storage union. */
int G_Cod1YawMotionStorageProtected(const void *,size_t);
#endif
