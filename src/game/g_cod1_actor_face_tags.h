#ifndef G_COD1_ACTOR_FACE_TAGS_H
#define G_COD1_ACTOR_FACE_TAGS_H
#include "cod1_gsc/gsc_fixture.h"
/* Real already-clocked v4 unmounted full DObj, including five authored facial
 * leaves. No animation selection, time advancement, note or fallback pose. */
int G_Cod1ActorFaceTagMatrix(const GscEntityRef*,const char*,int,float[16],int(*)(void*),void*);
int G_Cod1ActorFaceTagWorld(const GscEntityRef*,const char*,int,float[3],float[3],int(*)(void*),void*);
#endif
