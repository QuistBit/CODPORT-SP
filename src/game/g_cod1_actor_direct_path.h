#ifndef G_COD1_ACTOR_DIRECT_PATH_H
#define G_COD1_ACTOR_DIRECT_PATH_H
#include "g_cod1_actor.h"
#include "bg_cod1_actor_direct_path.h"
typedef struct {int initialized,blocked,consumed,tick;uint64_t assignment;GscEntityRef actor;c1dpRecord_t record;} cod1ActorDirectPathView_t;
/* One direct, grounded, capsule-clear route only. A blocked route is pending;
 * Frame returns 1 after recording that fact, but Read refuses its distance. */
int G_Cod1ActorDirectPathFrame(const GscEntityRef*,int,int(*)(void*),void*);
int G_Cod1ActorDirectPathRead(const GscEntityRef*,int,float*,int(*)(void*),void*);
int G_Cod1ActorDirectPathInspect(const GscEntityRef*,cod1ActorDirectPathView_t*);
#endif
