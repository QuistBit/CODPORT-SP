#ifndef G_COD1_ACTOR_FACE_FRAME_H
#define G_COD1_ACTOR_FACE_FRAME_H
#include "cod1_gsc/gsc_fixture.h"
/* Explicit bounded unmounted owner. Selection enables no script command and
 * does not initialize an actor or manufacture an animation completion. */
typedef struct {
 GscEntityRef entity;
 uint64_t runtime,catalog,owner_identity,generation,frame_generation;
 uint32_t ordinal;
 int32_t produced_ms;
 char event[64],payload[128];
} cod1ActorFaceNote_t;
typedef int (*cod1ActorFaceNoteCallback)(void*,const cod1ActorFaceNote_t*);
const GscActorAnimationHost*G_Cod1ActorFacialAnimationHost(void);
int G_Cod1ActorFaceClockSelect(const GscEntityRef*,int,int(*)(void*),void*);
int G_Cod1ActorFaceFrameBegin(const GscEntityRef*,int,uint64_t,int(*)(void*),void*);
int G_Cod1ActorFaceFrameStep(const GscEntityRef*,int,uint64_t,int*,int(*)(void*),void*);
int G_Cod1ActorFaceFrameDrain(const GscEntityRef*,int,uint64_t,int(*)(void*),cod1ActorFaceNoteCallback,void*);
#endif
