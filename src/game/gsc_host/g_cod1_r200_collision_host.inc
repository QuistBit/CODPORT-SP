/* Typed R200 service; no entity-slot cast, visibility or synthetic success. */
#include "../g_cod1_r200_collision.h"
static int R200CollisionNotSolid(void *user,const GscEntityRef *owner,int now,
    char *error,size_t cap,int (*current)(void *),void *lease) {
    (void)user;
    return G_Cod1ScriptBrushNotSolid(owner,now,error,cap,current,lease);
}
static int R200CollisionSolid(void *user,const GscEntityRef *owner,int now,
    char *error,size_t cap,int (*current)(void *),void *lease) {
    (void)user;
    return G_Cod1ScriptBrushSolid(owner,now,error,cap,current,lease);
}
static int EnableGrenadeTouch(void *user,const GscEntityRef *input,int now,
    char *error,size_t cap,int (*current)(void *),void *lease){
    GscEntityRef ref;
    (void)user;
    if(!input||!current||now<0)return 0;
    ref=*input;
    if(current(lease)!=1||memcmp(&ref,input,sizeof(ref))||
       !G_Cod1EnableGrenadeTouchDamage(&ref)){
        if(error&&cap)snprintf(error,cap,"enableGrenadeTouchDamage requires a current damage trigger");
        return 0;
    }
    return 1;
}
static int DisableGrenadeBounce(void *user,const GscEntityRef *input,int now,
    char *error,size_t cap,int (*current)(void *),void *lease){
    GscEntityRef ref;
    (void)user;
    if(!input||!current||now<0)return 0;
    ref=*input;
    if(current(lease)!=1||memcmp(&ref,input,sizeof(ref))||
       !G_Cod1DisableGrenadeBounce(&ref)){
        if(error&&cap)snprintf(error,cap,"disableGrenadeBounce requires a current entity");
        return 0;
    }
    return 1;
}
const GscEntityCollisionHost *G_Cod1R200CollisionHost(void) {
    static const GscEntityCollisionHost host={NULL,R200CollisionNotSolid,R200CollisionSolid,EnableGrenadeTouch,DisableGrenadeBounce};
    return &host;
}

static int GscTestConnectPaths(void *user,const GscEntityRef *entity,int now,char *error,size_t capacity,int (*current)(void *),void *lease){
    (void)user;(void)now;
    return G_Cod1NodeGraphConnectPaths(gscTestSession,gscTestId,entity,error,capacity,current,lease);
}

static int GscTestDisconnectPaths(void*user,const GscEntityRef*entity,int now,char*error,size_t cap,int(*current)(void*),void*lease){
 /* VM time is Session-relative; native owner.time is GAME level.time. */
 (void)user;(void)now;return G_Cod1NodeGraphDisconnectPaths(gscTestSession,gscTestId,entity,level.time,error,cap,current,lease);
}
