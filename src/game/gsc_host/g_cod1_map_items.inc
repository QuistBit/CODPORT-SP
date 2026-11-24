/* Map-owned item services reuse the qualified native item lifecycle. No
 * Training source selection, assumed BSP grenade count, or fabricated event. */
static struct {int selected,ready,constructing;GscSession*session;uint64_t epoch,id;cod1AmmoRegistryReceipt_t registry;int itemIndex[WP_NUM_WEAPONS];uintptr_t itemRow[WP_NUM_WEAPONS];}gscMapItems;
static int GscMapItemStorageProtected(const void*p,size_t n){return CAR_Overlap(p,n,&gscMapItems,sizeof(gscMapItems));}
static int GscMapItemOwner(int preparing){
 GscSessionSnapshot view;GscSession*owner=gscMapItems.session;uint64_t epoch=gscMapItems.epoch,id=gscMapItems.id;
 return gscMapItems.selected&&owner&&owner==gscTestSession&&epoch&&id&&epoch==gscTestMapEpoch&&epoch==G_Cod1EntityMapEpoch()&&id==gscTestId&&!gscTestStopRequested&&GscVehicleMapCurrent(epoch)&&GscTestModuleReadCurrent()&&
  gscMapItems.selected&&gscMapItems.session==owner&&gscTestSession==owner&&gscMapItems.epoch==epoch&&gscMapItems.id==id&&gscTestId==id&&!gscTestStopRequested&&
  gsc_session_snapshot(owner,&view)&&view.session_id==id&&view.has_runtime&&!view.error[0]&&
  (preparing?view.state==GSC_SESSION_PREPARED&&!view.busy&&!view.active_threads:view.state==GSC_SESSION_RUNNING)&&
  gscMapItems.session==owner&&gscMapItems.epoch==epoch&&gscMapItems.id==id&&gscTestSession==owner&&gscTestId==id&&epoch==G_Cod1EntityMapEpoch()&&!gscTestStopRequested&&gscMapItems.selected;
}
static int GscMapItemCurrent(void*user){(void)user;return !gscMapItems.constructing&&GscMapItemOwner(0);}
static int GscMapItemConstructionCurrent(void*user){(void)user;return GscMapItemOwner(gscMapItems.constructing);}
static int GscMapItemInteraction(void *user,int wp,int *index,uintptr_t *row) {
    int i;(void)user;
    if(!index||!row||!GscMapItemConstructionCurrent(NULL)||wp<=0||wp>=WP_NUM_WEAPONS)return 0;
    /* Resolve the genuine port interaction table independently of CAP, CI,
     * CS_MODELS and server model-bank indices. This is not a native item ID. */
    if(bg_numItems<=0||bg_numItems>MAX_ITEMS)return 0;
    if(!gscMapItems.itemRow[wp])for(i=1;i<bg_numItems;++i) {
        if(bg_itemlist[i].classname&&bg_itemlist[i].giType==IT_WEAPON&&bg_itemlist[i].giTag==wp) {
            gscMapItems.itemIndex[wp]=i;
            gscMapItems.itemRow[wp]=(uintptr_t)&bg_itemlist[i];break;
        }
    }
    if(!gscMapItems.itemRow[wp])return 0;
    *index=gscMapItems.itemIndex[wp];*row=gscMapItems.itemRow[wp];return 1;
}
static int GscMapItemRegistration(void *user,int capIndex,
    cod1WeaponRegistrationReceipt_t *out,char *error,size_t size,
    int (*current)(void *),void *context) {
    cod1AmmoRegistryReceipt_t registry=gscMapItems.registry;
    cod1WeaponRegistrationReceipt_t result;(void)user;
    if(!out||!GscMapItemConstructionCurrent(NULL)||
       !G_Cod1ReadWeaponRegistration(&registry,capIndex,&result,error,size,current,context))return 0;
    /* Trusted copied bridge: public receipt APIs never write private owner
     * storage or accept an alias to the lifecycle guard as public input. */
    memcpy(out,&result,sizeof(result));return 1;
}
static int GscMapItemFloorTrace(void *user,int slot,const float origin[3],
    const float mins[3],const float maxs[3],float placed[3],int *ground) {
    trace_t trace;vec3_t end;(void)user;
    if(!origin||!mins||!maxs||!placed||!ground||slot<MAX_CLIENTS||
       slot>=ENTITYNUM_MAX_NORMAL||!GscMapItemConstructionCurrent(NULL))return 0;
    memcpy(end,origin,sizeof(end));end[2]-=4096.f;
    trap_Trace(&trace,origin,mins,maxs,end,slot,MASK_SOLID);
    if(!GscMapItemConstructionCurrent(NULL)||trace.startsolid||trace.allsolid||
       !isfinite(trace.fraction)||trace.fraction<0.f||trace.fraction>1.f||
       !isfinite(trace.endpos[0])||!isfinite(trace.endpos[1])||!isfinite(trace.endpos[2])||
       trace.entityNum<0||trace.entityNum>=MAX_GENTITIES)return 0;
    memcpy(placed,trace.endpos,sizeof(vec3_t));*ground=trace.entityNum;return 1;
}
static int GscMapItemAcquire(void *user,const GscEntityRef *inputPlayer,
    const cod1WeaponRegistrationReceipt_t *inputRegistration,int32_t count,
    int32_t *accepted,char *error,size_t size,int (*current)(void *),void *context) {
    GscEntityRef player;cod1WeaponRegistrationReceipt_t registration;
    cod1AcquireClipAmmoResult_t result;(void)user;
    if(!inputPlayer||!inputRegistration||!accepted||!GscMapItemCurrent(NULL))return 0;
    player=*inputPlayer;registration=*inputRegistration;
    if(!G_Cod1AcquireRegisteredClipOnlyAmmo(&player,&registration,count,&result,
        error,size,current,context))return 0;
    if(result.accepted>0 && G_Cod1EntitySameAllocation(&player) &&
       g_entities[player.slot].client && result.wp>WP_NONE && result.wp<WP_NUM_WEAPONS) {
        int index=gscMapItems.itemIndex[result.wp];
        if(index>0 && index<MAX_ITEMS)
            G_AddEvent(&g_entities[player.slot],
                result.newlyOwned?EV_ITEM_PICKUP:EV_COD1_WEAPON_AMMO_PICKUP,index);
    }
    *accepted=result.accepted;return 1;
}
static int GscMapItemPickupNotify(void *user,const GscEntityRef *item,
    const GscEntityRef *player,char *error,size_t size) {
    GscEventValue values[2];GscEntityRef itemCopy,playerCopy;int awakened;
    (void)user;(void)error;(void)size;
    if(!item||!player||!GscMapItemCurrent(NULL)||
       !G_Cod1EntitySameAllocation(item)||!G_Cod1EntitySameAllocation(player))return 0;
    itemCopy=*item;playerCopy=*player;
    memset(values,0,sizeof(values));values[0].kind=GSC_EVENT_ENTITY;values[0].entity=playerCopy;
    values[1].kind=GSC_EVENT_UNDEFINED;
    /* Actual producers mark current waits ready; continuations execute on
     * the next Session advance. Native synchronous waiter timing is not claimed. */
    if(!G_Cod1GscTestEntityEventWithPayload(&g_entities[itemCopy.slot],"trigger",values,2,&awakened)||
       !GscMapItemCurrent(NULL)||!G_Cod1EntitySameAllocation(&playerCopy)||
       !G_Cod1EntitySameAllocation(&itemCopy))return 0;
    values[0].entity=itemCopy;
    if(!G_Cod1GscTestEntityEventWithPayload(&g_entities[playerCopy.slot],"pickup",values,2,&awakened)||
       !GscMapItemCurrent(NULL)||!G_Cod1EntitySameAllocation(&playerCopy))return 0;
    G_Printf("[Campaign] ITEM_PICKUP item=%u generation=%u player=%u\n",
        (unsigned)itemCopy.slot,(unsigned)itemCopy.generation,(unsigned)playerCopy.slot);
    return 1;
}
static int GscMapItemFieldClassify(void *user,const GscEntityRef *entity,
    const char *name,char *error,size_t size) {
    (void)user;(void)error;(void)size;
    if(!name)return -1;if(strcmp(name,"count"))return 0;
    return GscMapItemCurrent(NULL)&&G_Cod1ItemLifecycleOwns(entity)?1:-1;
}
static int GscMapItemFieldRead(void *user,const GscEntityRef *entity,
    const char *name,GscStringCastValue *out,char *error,size_t size) {
    int32_t count;GscStringCastValue value;(void)user;
    if(!out||!name||strcmp(name,"count")||!GscMapItemCurrent(NULL)||
       !G_Cod1ItemLifecycleGetCount(entity,&count,error,size))return 0;
    memset(&value,0,sizeof(value));value.type=5;value.integer=count;*out=value;return 1;
}
static int GscMapItemFieldWrite(void *user,const GscEntityRef *entity,
    const char *name,const GscStringCastValue *value,char *error,size_t size) {
    int32_t count;(void)user;
    if(!name||strcmp(name,"count")||!value||!GscMapItemCurrent(NULL))return 0;
    if(value->type==5)count=value->integer;
    else if(value->type==4&&isfinite(value->number)&&value->number>=-2147483648.0&&
            value->number<2147483648.0&&
            (double)(int32_t)value->number==(double)value->number)count=(int32_t)value->number;
    else return 0;
    return G_Cod1ItemLifecycleSetCount(entity,count,error,size);
}
static const GscEntityFieldHost *GscMapItemFieldHost(void) {
    static const GscEntityFieldHost host={NULL,GscMapItemFieldClassify,
        GscMapItemFieldRead,GscMapItemFieldWrite};return &host;
}

static int GscMapItemSelect(char*error,size_t cap){
 if(gscMapItems.selected||!gscTestSession||!GscCampaignProgramCurrent(G_Cod1EntityMapEpoch()))return 0;
 if(!gsc_session_select_map_items(gscTestSession,GscMapItemFieldHost(),error,cap))return 0;
 gscMapItems.selected=1;gscMapItems.session=gscTestSession;gscMapItems.epoch=G_Cod1EntityMapEpoch();return 1;
}
static int GscMapItemBegin(char*error,size_t cap){
 cod1AmmoRegistryReceipt_t registry;cod1ItemLifecycleServices_t services;int ok=0;
 if(!gscMapItems.selected)return 1;if(gscMapItems.ready||gscMapItems.constructing)return 0;
 gscMapItems.id=gscTestId;gscMapItems.constructing=1;
 if(!GscMapItemConstructionCurrent(NULL)||!CAR_Build(&registry)||!GscMapItemConstructionCurrent(NULL))goto done;
 gscMapItems.registry=registry;memset(&services,0,sizeof(services));services.current=GscMapItemConstructionCurrent;services.interaction_item=GscMapItemInteraction;services.floor_trace=GscMapItemFloorTrace;services.read_registration=GscMapItemRegistration;services.acquire=GscMapItemAcquire;services.pickup_notify=GscMapItemPickupNotify;
 if(!G_Cod1ItemLifecycleSelect(&registry,&services,error,cap)||!GscMapItemConstructionCurrent(NULL))goto done;
 gscMapItems.ready=1;ok=1;
done:gscMapItems.constructing=0;return ok;
}
static void GscMapItemReset(void){memset(&gscMapItems,0,sizeof(gscMapItems));}
