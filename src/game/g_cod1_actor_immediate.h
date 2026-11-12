#ifndef G_COD1_ACTOR_IMMEDIATE_H
#define G_COD1_ACTOR_IMMEDIATE_H
#include "cod1_gsc/gsc_fixture.h"
/* Real generic tree commands plus exact fresh ExitProne and face-default/finite bounded face-angle
 * immediate branches. Active prone/other orientation modes remain refused. */
const GscActorAnimationHost *G_Cod1ActorComposedAnimationHost(void);
/* Read-only duration metadata from already decoded current generic clips.
 * Catalog membership alone does not load or certify a clip. */
const GscAnimationLengthHost *G_Cod1LoadedAnimationLengthHost(void);
/* A delivery callback must copy/queue the packet atomically, return1 only
 * when accepted, and defer script execution. Refusal retains the receipt. */
typedef struct {GscEntityRef entity;uint64_t runtime,catalog,owner_identity,generation;uint32_t ordinal;int32_t produced_ms;char event[64],payload[4];} cod1ActorAnimationNotify_t;
typedef int (*cod1ActorAnimationNotifyCallback)(void*,const cod1ActorAnimationNotify_t*);
int G_Cod1ActorGenericNotifyDrain(const GscEntityRef*,int,int(*)(void*),cod1ActorAnimationNotifyCallback,void*);
#endif
