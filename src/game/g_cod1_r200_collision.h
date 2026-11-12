#ifndef G_COD1_R200_COLLISION_H
#define G_COD1_R200_COLLISION_H
#include "cod1_gsc/gsc_fixture.h"
#include <math.h>
int G_Cod1ScriptBrushNotSolid(const GscEntityRef *,int,char *,size_t,int (*)(void *),void *);
int G_Cod1ScriptBrushNonsolidOwned(const GscEntityRef *);
int G_Cod1ScriptBrushNonsolidCommit(const GscEntityRef *);
const GscEntityCollisionHost *G_Cod1R200CollisionHost(void);
/* Shared pure predicate; the sidecar remains in the entity host so its old
 * standalone users do not need to link unrelated GAME mutation helpers. */
static int R200BrushShape(const gentity_t *e) {
    int i;
    if(!e||!e->inuse||e->client||e->s.number<MAX_CLIENTS||e->s.number>=ENTITYNUM_WORLD||
       !e->classname||strcmp(e->classname,"script_brushmodel")||!e->r.bmodel||
       e->s.eType!=ET_MOVER||e->s.modelindex<=0||e->s.modelindex>=MAX_MODELS||
       (e->r.svFlags&(SVF_BOT|SVF_CASTAI)))return 0;
    for(i=0;i<3;i++)if(!isfinite(e->r.currentOrigin[i])||!isfinite(e->r.currentAngles[i])||
        !isfinite(e->r.mins[i])||!isfinite(e->r.maxs[i])||e->r.mins[i]>e->r.maxs[i])return 0;
    return e->s.solid==SOLID_BMODEL||(!e->s.solid&&!e->r.contents&&!e->clipmask);
}
#endif
