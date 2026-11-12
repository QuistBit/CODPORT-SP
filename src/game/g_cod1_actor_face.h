#ifndef G_COD1_ACTOR_FACE_H
#define G_COD1_ACTOR_FACE_H
#include "cod1_gsc/gsc_fixture.h"
/* Resource/publication dependency only. These calls neither select a facial
 * animation nor manufacture its clock, named notes, completion, or AI state. */
int G_Cod1ActorFaceResourcesLoad(void);
int G_Cod1ActorFaceUnmountedSample(const GscEntityRef*,int,float[98],float[2][21],int(*)(void*),void*);
int G_Cod1ActorFaceUnmountedPublish(const GscEntityRef*,int,int(*)(void*),void*);
#endif
