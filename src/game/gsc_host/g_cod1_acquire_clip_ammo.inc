#include "../g_cod1_acquire_clip_ammo.h"

int G_Cod1AcquireClipAmmoStorageProtected(const void *p,size_t n) {
    return !AmmoInventoryPoolSafe() || AmmoInventoryPrivate(p,n);
}
static int AcquireClipAmmoBuffers(const GscEntityRef *ref,
    const cod1AmmoRegistryReceipt_t *registry,const cod1AcquireClipAmmoAuthority_t *authority,
    cod1AcquireClipAmmoResult_t *out,char *error,size_t size) {
    const void *p[5];size_t z[5];int i,j;
    if(!AmmoInventoryPoolSafe() || !ref || !registry || !authority || !out ||
       !AmmoInventoryRange(authority,sizeof(*authority)) ||
       AmmoInventoryPrivate(authority,sizeof(*authority)))return 0;
    if(!authority->current || !authority->storage_protected)return 0;
    p[0]=ref;z[0]=sizeof(*ref);p[1]=registry;z[1]=sizeof(*registry);
    p[2]=authority;z[2]=sizeof(*authority);p[3]=out;z[3]=sizeof(*out);
    p[4]=error;z[4]=size;
    for(i=0;i<5;++i) {
        if(AmmoInventoryPrivate(p[i],z[i]) || authority->storage_protected(authority->context,p[i],z[i]))return 0;
        for(j=0;j<i;++j)if(CAR_Overlap(p[i],z[i],p[j],z[j]))return 0;
    }
    return 1;
}
static int AcquireClipAmmoStage(const cod1AmmoPickupInventorySnapshot_t *snapshot,
    int target,int32_t quantity,const playerState_t *before,playerState_t *after,
    cod1AcquireClipAmmoResult_t *result) {
    cod1AmmoPickupInventory_t effective;
    const cod1AmmoPickupWeapon_t *weapon;
    int i,wp,ac,cc,slot,physicalClip;
    int32_t nextClip,accepted,headroom,maximum;
    if(quantity<=0 || target<1 || target>snapshot->inventory.weaponCount)return 0;
    weapon=&snapshot->inventory.weapons[target];wp=snapshot->mapping[target].wp;
    ac=snapshot->mapping[target].ammoCell;cc=snapshot->mapping[target].clipCell;
    if(weapon->clipOnly!=1 || weapon->sharedCapIndex < -1 ||
       wp<=WP_NONE || wp>=WP_NUM_WEAPONS || ac<=0 || ac>=MAX_WEAPONS ||
       cc<=0 || cc>=MAX_WEAPONS)return 0;
    *after=*before;memset(result,0,sizeof(*result));result->wp=wp;
    if(CAP_Owned(&snapshot->inventory,target)) {
        if(!OwnedClipAmmoStage(snapshot,target,quantity,&physicalClip,&nextClip,&accepted))return 0;
        after->ammoclip[physicalClip]=nextClip;result->accepted=accepted;
        result->slot=CW_OccupiedSlot(before,wp);return 1;
    }
    slot=CW_PickupSlot(before,wp,0);
    if(slot<=CW_SLOT_NONE || slot>=CW_SLOT_COUNT)return 0;
    /* First acquisition starts from exact empty target cells. Stale counters
     * are refused, never reset or converted into a free initial clip. */
    if(before->ammo[ac]!=0 || before->ammoclip[cc]!=0)return 0;
    for(i=1;i<=snapshot->inventory.weaponCount;++i)if(CAP_Owned(&snapshot->inventory,i) &&
       (snapshot->mapping[i].ammoCell==ac || snapshot->mapping[i].clipCell==cc))return 0;
    if(before->cod1Slots[slot] &&
       (before->cod1Slots[slot]<=WP_NONE || before->cod1Slots[slot]>=WP_NUM_WEAPONS ||
        !COM_BitCheck(before->weapons,before->cod1Slots[slot]) ||
        !CW_SlotCompatible(slot,before->cod1Slots[slot]) ||
        !CW_SlotStackable(wp) || !CW_SlotStackable(before->cod1Slots[slot])))return 0;
    effective=snapshot->inventory;
    effective.ammo[weapon->ammoIndex]=0;effective.clip[weapon->clipIndex]=0;
    maximum=effective.maxClip[weapon->clipIndex];
    if(maximum<1 || !CAP_AmmoHeadroom(&effective,target,&headroom) || headroom<=0)return 0;
    accepted=quantity<maximum?quantity:maximum;if(accepted>headroom)accepted=headroom;
    if(accepted<=0)return 0;
    COM_BitSet(after->weapons,wp);after->ammo[ac]=0;after->ammoclip[cc]=accepted;
    if(!after->cod1Slots[slot])after->cod1Slots[slot]=wp;
    CW_SetMode(after,wp,snapshot->registry.weapons[target].cwIndex);
    result->newlyOwned=1;result->accepted=accepted;result->slot=slot;return 1;
}
static int AcquireClipAmmoFence(uint64_t serial,const GscEntityRef *input,
    const cod1AmmoRegistryReceipt_t *inputRegistry,const cod1AcquireClipAmmoAuthority_t *inputAuthority,
    const GscEntityRef *ref,const cod1AmmoRegistryReceipt_t *registry,
    const cod1AcquireClipAmmoAuthority_t *authority,
    const cod1AmmoPickupInventorySnapshot_t *staged,const playerState_t *before,
    cod1AcquireClipAmmoResult_t *out,char *error,size_t size) {
    cod1AmmoPickupInventorySnapshot_t fresh;
    if(!cod1AmmoInventoryGuard.active || cod1AmmoInventoryGuard.cancelled ||
       cod1AmmoInventoryGuard.serial!=serial ||
       memcmp(inputAuthority,authority,sizeof(*authority)) ||
       !AcquireClipAmmoBuffers(input,inputRegistry,inputAuthority,out,error,size) ||
       memcmp(input,ref,sizeof(*ref)) || memcmp(inputRegistry,registry,sizeof(*registry)) ||
       memcmp(inputAuthority,authority,sizeof(*authority)))return 0;
    /* Resource reads can invoke instrumented boundaries. Prove the player
     * after them, immediately before any later publication. */
    if(authority->current(authority->context)!=1 ||
       !cod1AmmoInventoryGuard.active || cod1AmmoInventoryGuard.cancelled ||
       cod1AmmoInventoryGuard.serial!=serial ||
       memcmp(inputAuthority,authority,sizeof(*authority)) ||
       !AcquireClipAmmoBuffers(input,inputRegistry,inputAuthority,out,error,size) ||
       memcmp(input,ref,sizeof(*ref)) || memcmp(inputRegistry,registry,sizeof(*registry)) ||
       memcmp(inputAuthority,authority,sizeof(*authority)) ||
       !AmmoInventoryProject(ref,registry,&fresh) || !AmmoInventoryEqual(staged,&fresh))return 0;
    return !memcmp(&g_entities[ref->slot].client->ps,before,sizeof(*before));
}
int G_Cod1AcquireClipOnlyAmmo(const GscEntityRef *input,
    const cod1AmmoRegistryReceipt_t *inputRegistry,int target,int32_t quantity,
    const cod1AcquireClipAmmoAuthority_t *inputAuthority,cod1AcquireClipAmmoResult_t *out,
    char *error,size_t size,int (*current)(void *),void *context) {
    GscEntityRef ref;cod1AmmoRegistryReceipt_t registry;cod1AcquireClipAmmoAuthority_t authority;
    cod1AmmoPickupInventorySnapshot_t staged;playerState_t before,after;
    cod1AcquireClipAmmoResult_t result;uint64_t serial;int phase,ok=0;
    if(!AcquireClipAmmoBuffers(input,inputRegistry,inputAuthority,out,error,size) || !current)return 0;
    if(cod1AmmoInventoryGuard.active){cod1AmmoInventoryGuard.cancelled=1;return 0;}
    if(cod1AmmoInventorySerial==UINT64_MAX)return 0;
    ref=*input;registry=*inputRegistry;authority=*inputAuthority;
    serial=++cod1AmmoInventorySerial;cod1AmmoInventoryGuard.active=1;
    cod1AmmoInventoryGuard.cancelled=0;cod1AmmoInventoryGuard.serial=serial;
    if(!AmmoInventoryProject(&ref,&registry,&staged))goto done;
    before=g_entities[ref.slot].client->ps;
    if(!AcquireClipAmmoStage(&staged,target,quantity,&before,&after,&result))goto done;
    for(phase=0;phase<2;++phase) {
        if(!AcquireClipAmmoFence(serial,input,inputRegistry,inputAuthority,&ref,&registry,&authority,
             &staged,&before,out,error,size) || current(context)!=1 ||
           !AcquireClipAmmoFence(serial,input,inputRegistry,inputAuthority,&ref,&registry,&authority,
             &staged,&before,out,error,size))goto done;
    }
    /* The last fence ends with pure player/registry proof. All callbacks are
     * over. Ownership, counters and slot publish atomically to this engine. */
    memcpy(&g_entities[ref.slot].client->ps,&after,sizeof(after));
    memcpy(out,&result,sizeof(result));ok=1;
done:
    cod1AmmoInventoryGuard.active=0;cod1AmmoInventoryGuard.cancelled=0;
    if(!ok && !memcmp(inputAuthority,&authority,sizeof(authority)) &&
       AcquireClipAmmoBuffers(input,inputRegistry,inputAuthority,out,error,size))
        EntityError(error,size,"registered clip ammo lost player, resources or admitted capacity");
    return ok;
}
