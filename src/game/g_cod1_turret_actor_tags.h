#ifndef G_COD1_TURRET_ACTOR_TAGS_H
#define G_COD1_TURRET_ACTOR_TAGS_H
/* Actual mounted owner/current tree only. Returns a column-axis float16 world
 * matrix; failure leaves output untouched. No unmounted bind fallback. */
int G_Cod1TurretActorTagMatrix(const GscEntityRef*,const char*,int,float[16],int(*)(void*),void*);
int G_Cod1TurretActorTagWorld(const GscEntityRef*,const char*,int,float[3],float[3],int(*)(void*),void*);
#endif
