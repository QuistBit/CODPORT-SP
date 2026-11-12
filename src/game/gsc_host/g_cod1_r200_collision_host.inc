/* Typed R200 service; no entity-slot cast, visibility or synthetic success. */
#include "../g_cod1_r200_collision.h"
static int R200CollisionNotSolid(void *user,const GscEntityRef *owner,int now,
    char *error,size_t cap,int (*current)(void *),void *lease) {
    (void)user;
    return G_Cod1ScriptBrushNotSolid(owner,now,error,cap,current,lease);
}
const GscEntityCollisionHost *G_Cod1R200CollisionHost(void) {
    static const GscEntityCollisionHost host={NULL,R200CollisionNotSolid};
    return &host;
}
