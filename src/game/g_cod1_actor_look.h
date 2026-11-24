#ifndef G_COD1_ACTOR_LOOK_H
#define G_COD1_ACTOR_LOOK_H
#include "cod1_gsc/gsc_fixture.h"
const GscActorLookHost *G_Cod1ActorLookHost(void);
const GscActorLookHost *G_Cod1ActorSourceLookHost(void);
/* Reuses the existing typed spatial query lane. Actor GetEye is admitted only
 * with a genuine current mounted tag_eye pose; player/contact policy is the
 * unchanged spatial host. No cached or bind-pose actor eye is substituted. */
const GscSpatialNativeHost *G_Cod1ActorSpatialNativeHost(void);
/* One real original AI update. Active interpolation requires exact50ms owner
 * cadence and a genuine current mounted eye pose; no cached render frame. */
int G_Cod1ActorLookFrame(const GscEntityRef *,int,int (*)(void *),void *);
#endif
