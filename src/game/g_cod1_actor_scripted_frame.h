#ifndef G_COD1_ACTOR_SCRIPTED_FRAME_H
#define G_COD1_ACTOR_SCRIPTED_FRAME_H
#include "g_cod1_actor_face_frame.h"
/* Explicit normal Foley two-clip clock dependency. No GSC native admission,
 * actor constructor/FSM, normal-record pose or automatic profile selection.
 * Existing FaceFrame Begin/Step/Drain APIs drive the selected staged owner. */
int G_Cod1ActorScriptedClockSelect(const GscEntityRef*,int,int(*)(void*),void*);
const GscActorAnimationHost*G_Cod1ActorScriptedAnimationHost(void);
int G_Cod1ActorScriptedSampleFull(const GscEntityRef*,int,float[98],float[2][21],float[2][25],int(*)(void*),void*);
int G_Cod1ActorScriptedSample(const GscEntityRef*,int,float[98],float[2][21],int(*)(void*),void*);
#endif
