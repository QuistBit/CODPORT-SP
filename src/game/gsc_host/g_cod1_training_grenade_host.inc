/* R216 selected original grenade subsection. Original function bytes come
 * only from the user's mounted training sources and a checked SHA selector.
 * Full training/main and unrelated actors remain outside this profile. */
#include "../g_cod1_item_lifecycle.h"
#include "../g_cod1_grenade_player.h"
#include "../g_cod1_yaw_motion.h"
#include "g_cod1_training_grenade_source.h"
static vmCvar_t gscTestTrainingGrenades;
static struct {
    int selected,ready,startTime;
    uint64_t epoch,id;
    cod1AmmoRegistryReceipt_t registry;
    int itemIndex[WP_NUM_WEAPONS];
    uintptr_t itemRow[WP_NUM_WEAPONS];
} gscTestGrenades;
static cod1ItemPreparedReceipt_t gscTestGrenadePrepared;
static const GscEntityFieldHost *GscTestGrenadeFieldHost(void);
static const GscYawMotionHost *GscTestGrenadeYawHost(void);
static struct {GscSession*session;uint64_t id,epoch;int active;} gscTestGrenadePrepare;
static int GscTestGrenadeStorageProtected(const void *p,size_t n) {
    return CAR_Overlap(p,n,&gscTestGrenadePrepared,sizeof(gscTestGrenadePrepared)) ||
        CAR_Overlap(p,n,&gscTestGrenadePrepare,sizeof(gscTestGrenadePrepare)) ||
        CAR_Overlap(p,n,&gscTestGrenades,sizeof(gscTestGrenades)) ||
        CAR_Overlap(p,n,&gscTestTrainingGrenades,sizeof(gscTestTrainingGrenades)) ||
        CAR_Overlap(p,n,G_Cod1GrenadePlayerNativeHost(),sizeof(GscPlayerNativeHost)) ||
        CAR_Overlap(p,n,GscTestGrenadeFieldHost(),sizeof(GscEntityFieldHost)) ||
        CAR_Overlap(p,n,GscTestGrenadeYawHost(),sizeof(GscYawMotionHost));
}
static int GscTestGrenadeCurrent(void *user) {
    GscSessionSnapshot view;GscSession *owner=gscTestSession;
    uint64_t epoch=gscTestGrenades.epoch,id=gscTestGrenades.id;(void)user;
    return gscTestGrenades.selected && owner && !gscTestStopRequested &&
        !level.spawning && !level.restarted && gscTestEntityMode==43 &&
        gscTestGrenades.epoch==G_Cod1EntityMapEpoch() &&
        gscTestGrenades.id==gscTestId &&
        G_Cod1GscFeature47(C1CP47_F_GRENADES)==1 &&
        GscTestModuleReadCurrent() && gscTestSession==owner && !gscTestStopRequested &&
        gscTestGrenades.selected && gscTestGrenades.epoch==epoch && gscTestGrenades.id==id &&
        gscTestId==id && epoch==G_Cod1EntityMapEpoch() &&
        gsc_session_snapshot(owner,&view) && view.session_id==id &&
        view.state==GSC_SESSION_RUNNING && view.has_runtime;
}
/* Synchronous construction lease only: preparation never authorizes pickup,
 * events, ammo acquisition, script fields or yaw. Those retain Current(). */
static int GscTestGrenadeConstructionCurrent(void*user){
 GscSessionSnapshot view;GscSession*owner=gscTestGrenadePrepare.session;
 uint64_t id=gscTestGrenadePrepare.id,epoch=gscTestGrenadePrepare.epoch;(void)user;
 if(!gscTestGrenadePrepare.active)return GscTestGrenadeCurrent(NULL);
 return gscTestGrenadePrepare.session==gscTestSession&&gscTestSession&&
  gscTestGrenadePrepare.id==gscTestId&&gscTestGrenades.id==gscTestId&&
  gscTestGrenadePrepare.epoch==G_Cod1EntityMapEpoch()&&gscTestGrenades.epoch==gscTestGrenadePrepare.epoch&&
  gscTestGrenades.selected&&!gscTestStopRequested&&!level.spawning&&!level.restarted&&gscTestEntityMode==43&&
  G_Cod1GscFeature47(C1CP47_F_GRENADES)==1&&
  GscTestModuleReadCurrent()&&GscTestEarlyCurrent()&&!gscTestStopRequested&&
  gscTestGrenadePrepare.active&&gscTestGrenadePrepare.session==owner&&gscTestSession==owner&&
  gscTestGrenadePrepare.id==id&&gscTestId==id&&gscTestGrenades.id==id&&
  gscTestGrenadePrepare.epoch==epoch&&G_Cod1EntityMapEpoch()==epoch&&gscTestGrenades.epoch==epoch&&
  gsc_session_snapshot(owner,&view)&&view.session_id==id&&
  view.state==GSC_SESSION_PREPARED&&!view.busy&&!view.active_threads&&!view.error[0]&&view.has_runtime&&
  gscTestGrenadePrepare.active&&gscTestGrenadePrepare.session==owner&&gscTestSession==owner&&
  gscTestGrenadePrepare.id==id&&gscTestId==id&&gscTestGrenades.id==id&&
  gscTestGrenadePrepare.epoch==epoch&&G_Cod1EntityMapEpoch()==epoch&&gscTestGrenades.epoch==epoch&&
  gscTestGrenades.selected&&!gscTestStopRequested;
}
static int GscTestGrenadeInteraction(void *user,int wp,int *index,uintptr_t *row) {
    int i;(void)user;
    if(!index||!row||!GscTestGrenadeConstructionCurrent(NULL)||wp<=0||wp>=WP_NUM_WEAPONS)return 0;
    /* Resolve the genuine port interaction table independently of CAP, CI,
     * CS_MODELS and server model-bank indices. This is not a native item ID. */
    if(bg_numItems<=0||bg_numItems>MAX_ITEMS)return 0;
    if(!gscTestGrenades.itemRow[wp])for(i=1;i<bg_numItems;++i) {
        if(bg_itemlist[i].classname&&bg_itemlist[i].giType==IT_WEAPON&&bg_itemlist[i].giTag==wp) {
            gscTestGrenades.itemIndex[wp]=i;
            gscTestGrenades.itemRow[wp]=(uintptr_t)&bg_itemlist[i];break;
        }
    }
    if(!gscTestGrenades.itemRow[wp])return 0;
    *index=gscTestGrenades.itemIndex[wp];*row=gscTestGrenades.itemRow[wp];return 1;
}
static int GscTestGrenadeRegistration(void *user,int capIndex,
    cod1WeaponRegistrationReceipt_t *out,char *error,size_t size,
    int (*current)(void *),void *context) {
    cod1AmmoRegistryReceipt_t registry=gscTestGrenades.registry;
    cod1WeaponRegistrationReceipt_t result;(void)user;
    if(!out||!GscTestGrenadeConstructionCurrent(NULL)||
       !G_Cod1ReadWeaponRegistration(&registry,capIndex,&result,error,size,current,context))return 0;
    /* Trusted copied bridge: public receipt APIs never write private owner
     * storage or accept an alias to the lifecycle guard as public input. */
    memcpy(out,&result,sizeof(result));return 1;
}
static int GscTestGrenadeFloorTrace(void *user,int slot,const float origin[3],
    const float mins[3],const float maxs[3],float placed[3],int *ground) {
    trace_t trace;vec3_t end;(void)user;
    if(!origin||!mins||!maxs||!placed||!ground||slot<MAX_CLIENTS||
       slot>=ENTITYNUM_MAX_NORMAL||!GscTestGrenadeConstructionCurrent(NULL))return 0;
    memcpy(end,origin,sizeof(end));end[2]-=4096.f;
    trap_Trace(&trace,origin,mins,maxs,end,slot,MASK_SOLID);
    if(!GscTestGrenadeConstructionCurrent(NULL)||trace.startsolid||trace.allsolid||
       !isfinite(trace.fraction)||trace.fraction<0.f||trace.fraction>1.f||
       !isfinite(trace.endpos[0])||!isfinite(trace.endpos[1])||!isfinite(trace.endpos[2])||
       trace.entityNum<0||trace.entityNum>=MAX_GENTITIES)return 0;
    memcpy(placed,trace.endpos,sizeof(vec3_t));*ground=trace.entityNum;return 1;
}
static int GscTestGrenadeAcquire(void *user,const GscEntityRef *inputPlayer,
    const cod1WeaponRegistrationReceipt_t *inputRegistration,int32_t count,
    int32_t *accepted,char *error,size_t size,int (*current)(void *),void *context) {
    GscEntityRef player;cod1WeaponRegistrationReceipt_t registration;
    cod1AcquireClipAmmoResult_t result;(void)user;
    if(!inputPlayer||!inputRegistration||!accepted||!GscTestGrenadeCurrent(NULL))return 0;
    player=*inputPlayer;registration=*inputRegistration;
    if(!G_Cod1AcquireRegisteredClipOnlyAmmo(&player,&registration,count,&result,
        error,size,current,context))return 0;
    if(result.accepted>0 && G_Cod1EntitySameAllocation(&player) &&
       g_entities[player.slot].client && result.wp>WP_NONE && result.wp<WP_NUM_WEAPONS) {
        int index=gscTestGrenades.itemIndex[result.wp];
        if(index>0 && index<MAX_ITEMS)
            G_AddEvent(&g_entities[player.slot],
                result.newlyOwned?EV_ITEM_PICKUP:EV_COD1_WEAPON_AMMO_PICKUP,index);
    }
    *accepted=result.accepted;return 1;
}
static int GscTestGrenadePickupNotify(void *user,const GscEntityRef *item,
    const GscEntityRef *player,char *error,size_t size) {
    GscEventValue values[2];GscEntityRef itemCopy,playerCopy;int awakened;
    (void)user;(void)error;(void)size;
    if(!item||!player||!GscTestGrenadeCurrent(NULL)||
       !G_Cod1EntitySameAllocation(item)||!G_Cod1EntitySameAllocation(player))return 0;
    itemCopy=*item;playerCopy=*player;
    memset(values,0,sizeof(values));values[0].kind=GSC_EVENT_ENTITY;values[0].entity=playerCopy;
    values[1].kind=GSC_EVENT_UNDEFINED;
    /* Actual producers mark current waits ready; continuations execute on
     * the next Session advance. Native synchronous waiter timing is not claimed. */
    if(!G_Cod1GscTestEntityEventWithPayload(&g_entities[itemCopy.slot],"trigger",values,2,&awakened)||
       !GscTestGrenadeCurrent(NULL)||!G_Cod1EntitySameAllocation(&playerCopy)||
       !G_Cod1EntitySameAllocation(&itemCopy))return 0;
    values[0].entity=itemCopy;
    if(!G_Cod1GscTestEntityEventWithPayload(&g_entities[playerCopy.slot],"pickup",values,2,&awakened)||
       !GscTestGrenadeCurrent(NULL)||!G_Cod1EntitySameAllocation(&playerCopy))return 0;
    G_Printf("[R216] GRENADE_PICKUP item=%u generation=%u player=%u\n",
        (unsigned)itemCopy.slot,(unsigned)itemCopy.generation,(unsigned)playerCopy.slot);
    return 1;
}
static int GscTestGrenadeFieldClassify(void *user,const GscEntityRef *entity,
    const char *name,char *error,size_t size) {
    (void)user;(void)error;(void)size;
    if(!name)return -1;if(strcmp(name,"count"))return 0;
    return GscTestGrenadeCurrent(NULL)&&G_Cod1ItemLifecycleOwns(entity)?1:-1;
}
static int GscTestGrenadeFieldRead(void *user,const GscEntityRef *entity,
    const char *name,GscStringCastValue *out,char *error,size_t size) {
    int32_t count;GscStringCastValue value;(void)user;
    if(!out||!name||strcmp(name,"count")||!GscTestGrenadeCurrent(NULL)||
       !G_Cod1ItemLifecycleGetCount(entity,&count,error,size))return 0;
    memset(&value,0,sizeof(value));value.type=5;value.integer=count;*out=value;return 1;
}
static int GscTestGrenadeFieldWrite(void *user,const GscEntityRef *entity,
    const char *name,const GscStringCastValue *value,char *error,size_t size) {
    int32_t count;(void)user;
    if(!name||strcmp(name,"count")||!value||!GscTestGrenadeCurrent(NULL))return 0;
    if(value->type==5)count=value->integer;
    else if(value->type==4&&isfinite(value->number)&&value->number>=-2147483648.0&&
            value->number<2147483648.0&&
            (double)(int32_t)value->number==(double)value->number)count=(int32_t)value->number;
    else return 0;
    return G_Cod1ItemLifecycleSetCount(entity,count,error,size);
}
static const GscEntityFieldHost *GscTestGrenadeFieldHost(void) {
    static const GscEntityFieldHost host={NULL,GscTestGrenadeFieldClassify,
        GscTestGrenadeFieldRead,GscTestGrenadeFieldWrite};return &host;
}
typedef struct {int (*current)(void *);void *context;} GscTestGrenadeYawLease;
static int GscTestGrenadeYawCurrent(void *context) {
    GscTestGrenadeYawLease *lease=context;
    return GscTestGrenadeCurrent(NULL)&&lease->current&&lease->current(lease->context)==1&&
        GscTestGrenadeCurrent(NULL);
}
static int GscTestGrenadeYawInvoke(void *user,const GscYawMotionRequest *request,
    char *error,size_t size,int (*current)(void *),void *context) {
    GscTestGrenadeYawLease lease;const GscYawMotionHost *host=G_Cod1YawMotionHost();(void)user;
    lease.current=current;lease.context=context;
    if(!request||!host||!host->invoke||!GscTestGrenadeCurrent(NULL))return 0;
    return host->invoke(host->user,request,error,size,GscTestGrenadeYawCurrent,&lease);
}
static const GscYawMotionHost *GscTestGrenadeYawHost(void) {
    static const GscYawMotionHost host={NULL,GscTestGrenadeYawInvoke};return &host;
}
static void GscTestGrenadeReset(void) {
    memset(&gscTestGrenadePrepare,0,sizeof(gscTestGrenadePrepare));G_Cod1ItemLifecycleReset();G_Cod1YawMotionReset();memset(&gscTestGrenades,0,sizeof(gscTestGrenades));
}
static int GscTestGrenadeBootstrap(char *error,size_t size) {
    cod1AmmoRegistryReceipt_t registry;cod1ItemLifecycleServices_t services;
    GscEntityRef refs[5];cod1ItemPreparedReceipt_t prepared;int i,count=0;
    if(!GscTestGrenadeConstructionCurrent(NULL))return 0;
    if(gscTestGrenades.ready)return 1;
    /* Adopt the original allocation and untouched BSP pose before the legacy
     * deferred constructor. The selected constructor cancels that old think. */
    if(!CAR_Build(&registry))return 0;
    gscTestGrenades.registry=registry;
    memset(&services,0,sizeof(services));services.current=GscTestGrenadeConstructionCurrent;
    services.interaction_item=GscTestGrenadeInteraction;
    services.floor_trace=GscTestGrenadeFloorTrace;
    services.read_registration=GscTestGrenadeRegistration;
    services.acquire=GscTestGrenadeAcquire;services.pickup_notify=GscTestGrenadePickupNotify;
    if(!G_Cod1ItemLifecycleSelect(&registry,&services,error,size))return 0;
    for(i=MAX_CLIENTS;i<level.num_entities;++i) {
        if(!g_entities[i].inuse||!g_entities[i].classname||
           strcmp(g_entities[i].classname,"weapon_fraggrenade"))continue;
        if(count>=5||!G_Cod1EntityCapture(&g_entities[i],&refs[count]))return 0;
        ++count;
    }
    if(count!=5){GscTestError(error,size,"original training requires exactly five BSP frag grenades");return 0;}
    if(gscTestGrenadePrepare.active){
        if(gscTestGrenadePrepared.transaction||!G_Cod1ItemLifecyclePreparedBegin(refs,&prepared,error,size))return 0;
        gscTestGrenadePrepared=prepared;
    }
    for(i=0;i<5;++i)if(!G_Cod1ItemLifecycleAdopt(&refs[i],error,size))return 0;
    gscTestGrenades.ready=1;
    G_Printf("[R216] ORIGINAL_GRENADE_ITEMS_READY count=5 genuine_BSP=1\n");return 1;
}
