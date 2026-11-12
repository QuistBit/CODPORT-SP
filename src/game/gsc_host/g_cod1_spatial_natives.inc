/* R198 selected real spatial reads. Contact is the existing native capsule
 * query used by G_TouchTriggers, never an inferred AABB overlap or AI pose.
 * getEye is the server's unsnapped activation/aim eye, including stance/lean. */
#include "../bg_cod1_lean.h"
static int cod1SpatialNativeBusy,cod1SpatialNativeCancelled;
static int SpatialNativeOverlap(const void *a,size_t na,const void *b,size_t nb) {
    uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
    return a&&b&&na&&nb&&(x<=y?y-x<na:x-y<nb);
}
static int SpatialNativeFinite3(const float value[3]) {
    return isfinite(value[0])&&isfinite(value[1])&&isfinite(value[2]);
}
static int SpatialNativeBrush(const GscEntityRef *ref) {
    const gentity_t *ent;int i;
    if(!ref||!EntityValidate(NULL,ref)||!ActorOwnerAllowed(ref,0))return 0;
    ent=&g_entities[ref->slot];
    if(!ent->r.linked||!ent->r.bmodel||!ent->r.contents||
       !SpatialNativeFinite3(ent->r.currentOrigin)||!SpatialNativeFinite3(ent->r.currentAngles)||
       !SpatialNativeFinite3(ent->r.mins)||!SpatialNativeFinite3(ent->r.maxs)||
       ent->r.currentAngles[PITCH]!=0||ent->r.currentAngles[ROLL]!=0)return 0;
    for(i=0;i<3;++i)if(ent->r.maxs[i]<ent->r.mins[i])return 0;
    return (ent->classname&&!strcmp(ent->classname,"script_brushmodel")&&EntityVisibilityEligible(ent))||
        EntityTriggerOriginEligible(ent);
}
static int SpatialNativeBounds(const gentity_t *ent,const playerState_t *ps,
    float mins[3],float maxs[3]) {
    int i;volatile float extent[3];
    if(!ent->r.linked||!SpatialNativeFinite3(ps->origin)||
       !SpatialNativeFinite3(ent->r.mins)||!SpatialNativeFinite3(ent->r.maxs))return 0;
    for(i=0;i<3;++i) {
        if(ent->r.maxs[i]<=ent->r.mins[i])return 0;
        extent[i]=ent->r.maxs[i]-ent->r.mins[i];if(!isfinite(extent[i]))return 0;
        mins[i]=ps->origin[i]+ent->r.mins[i];maxs[i]=ps->origin[i]+ent->r.maxs[i];
    }
    /* Native v59 vertical-capsule admission; other shapes would silently
     * select the retained legacy collision fallback in CM_TraceV59Internal. */
    return extent[0]==extent[1]&&
        SpatialNativeFinite3(mins)&&SpatialNativeFinite3(maxs);
}
static int SpatialNativeEyeValid(const gentity_t *ent,const playerState_t *ps) {
    return SpatialNativeFinite3(ent->s.pos.trBase)&&SpatialNativeFinite3(ps->origin)&&
        SpatialNativeFinite3(ps->viewangles)&&isfinite(ps->mins[2])&&
        isfinite(ps->cod1EyeHeight)&&isfinite(ps->leanf)&&
        (BG_Cod1LeanActive(ps)?(ps->leanf>=-1&&ps->leanf<=1):ps->leanf==0);
}
static int SpatialNativeLease(uint64_t epoch,int hostTime,
    int (*current)(void *),void *lease) {
    return !cod1SpatialNativeCancelled&&current&&current(lease)&&
        !cod1SpatialNativeCancelled&&epoch&&G_Cod1EntityMapEpoch()==epoch&&
        cod1EntityReady&&!level.spawning&&!level.restarted&&level.time==hostTime&&
        G_Cod1MapEntities()&&g_gametype.integer==GT_SINGLE_PLAYER;
}
static int SpatialNativeSnapshots(const GscEntityRef *player,const gentity_t *playerBefore,
    const playerState_t *psBefore,const GscEntityRef *brush,const gentity_t *brushBefore) {
    if(!PlayerNativePlayer(player)||g_entities[player->slot].client!=playerBefore->client||
       memcmp(playerBefore,&g_entities[player->slot],sizeof(*playerBefore))||
       memcmp(psBefore,&g_entities[player->slot].client->ps,sizeof(*psBefore)))return 0;
    return !brush||(SpatialNativeBrush(brush)&&
        !memcmp(brushBefore,&g_entities[brush->slot],sizeof(*brushBefore)));
}
static int SpatialNativeInvoke(void *user,const GscSpatialNativeRequest *input,
    GscSpatialNativeResult *output,char *error,size_t size,
    int (*current)(void *),void *lease) {
    GscSpatialNativeRequest request;GscSpatialNativeResult result;
    GscEntityRef player,brush;gentity_t playerBefore,brushBefore;playerState_t psBefore;
    vec3_t mins,maxs,zero={0,0,0};uint64_t epoch;int hostTime,contact,hasBrush=0;
    (void)user;
    if(cod1SpatialNativeBusy) {
        cod1SpatialNativeCancelled=1;
        EntityError(error,size,"spatial native callback reentry refused");return 0;
    }
    if(!input||!output||!current||
       SpatialNativeOverlap(input,sizeof(*input),output,sizeof(*output))||
       SpatialNativeOverlap(input,sizeof(*input),error,size)||
       SpatialNativeOverlap(output,sizeof(*output),error,size))return 0;
    request=*input;memset(&result,0,sizeof(result));memset(&brush,0,sizeof(brush));
    if(request.now_ms<0)goto refuse;
    if(request.kind==GSC_SPATIAL_GET_EYE) {
        if(request.other.epoch||request.other.slot||request.other.generation||
           !PlayerNativePlayer(&request.receiver))goto refuse;
        player=request.receiver;
    } else if(request.kind==GSC_SPATIAL_IS_TOUCHING) {
        if(PlayerNativePlayer(&request.receiver)&&SpatialNativeBrush(&request.other)) {
            player=request.receiver;brush=request.other;
        } else if(PlayerNativePlayer(&request.other)&&SpatialNativeBrush(&request.receiver)) {
            player=request.other;brush=request.receiver;
        } else goto refuse;
        hasBrush=1;brushBefore=g_entities[brush.slot];
    } else goto refuse;
    playerBefore=g_entities[player.slot];psBefore=playerBefore.client->ps;
    if(hasBrush?!SpatialNativeBounds(&playerBefore,&psBefore,mins,maxs):
        !SpatialNativeEyeValid(&playerBefore,&psBefore))goto refuse;
    epoch=G_Cod1EntityMapEpoch();hostTime=level.time;
    cod1SpatialNativeBusy=1;cod1SpatialNativeCancelled=0;
    if(!SpatialNativeLease(epoch,hostTime,current,lease)||
       memcmp(input,&request,sizeof(request))||
       !SpatialNativeSnapshots(&player,&playerBefore,&psBefore,
            hasBrush?&brush:NULL,hasBrush?&brushBefore:NULL))goto busyRefuse;
    if(hasBrush) {
        contact=(int)trap_EntityContactCapsule(mins,maxs,&g_entities[brush.slot]);
        if(contact!=0&&contact!=1)goto busyRefuse;
        result.kind=GSC_SPATIAL_RESULT_INT;result.value=contact;
    } else {
        CalcMuzzlePointForActivate(&g_entities[player.slot],zero,zero,zero,result.vector);
        if(!SpatialNativeFinite3(result.vector))goto busyRefuse;
        result.kind=GSC_SPATIAL_RESULT_VECTOR;
    }
    if(!SpatialNativeLease(epoch,hostTime,current,lease)||
       memcmp(input,&request,sizeof(request))||
       !SpatialNativeSnapshots(&player,&playerBefore,&psBefore,
            hasBrush?&brush:NULL,hasBrush?&brushBefore:NULL))goto busyRefuse;
    *output=result;cod1SpatialNativeBusy=0;return 1;
busyRefuse:
    cod1SpatialNativeBusy=0;
refuse:
    EntityError(error,size,"spatial read requires current player eye or linked native vertical-capsule/player versus CoD brush/trigger geometry");return 0;
}
const GscSpatialNativeHost *G_Cod1SpatialNativeHost(void) {
    static const GscSpatialNativeHost host={NULL,SpatialNativeInvoke};return &host;
}
