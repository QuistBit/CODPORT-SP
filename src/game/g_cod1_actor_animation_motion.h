#ifndef G_COD1_ACTOR_ANIMATION_MOTION_H
#define G_COD1_ACTOR_ANIMATION_MOTION_H
#include "g_cod1_actor.h"
/* Called by the genuine committed MG42 script queue, before its original
 * main. Same actor/gun/queue receipt retries are idempotent. No fresh default
 * goal/current-weight choice is inferred. Only the proved mounted queue0 is
 * currently accepted. This records the queue, not successful GSC execution. */
int G_Cod1ActorAnimationMotionScriptBegin(const GscEntityRef*,int,int,int(*)(void*),void*);
/* 2: no explicit animation movement override (use the ordinary goal owner);
 * 1: current actual interval/capsule frame completed; 0: refusal/cancellation.
 * Explicit gravity/nogravity movement is bounded to an unmounted actor,
 * 50ms ticks and zero interval root yaw. It never runs alongside ROOT fit. */
int G_Cod1ActorAnimationMotionFrame(const GscEntityRef*,int,int(*)(void*),void*);
#endif
