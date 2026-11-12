/* R214 is a separately callable internal operation. Reuse the R213 protected
 * serial/guard so Read and Add cancel each other's synchronous reentry without
 * introducing another owner or changing inherited Read/Current behavior. */
#include "../g_cod1_owned_clip_ammo.h"

static int OwnedClipAmmoBuffers(const GscEntityRef *ref,
    const cod1AmmoRegistryReceipt_t *registry,int32_t *out,char *error,size_t size) {
    /* Scalar output has exactly four bytes; treating it as a snapshot would
     * incorrectly classify unrelated adjacent caller storage as an alias. */
    return AmmoInventoryPoolSafe() && !AmmoInventoryPrivate(ref,sizeof(*ref)) &&
        !AmmoInventoryPrivate(registry,sizeof(*registry)) &&
        !AmmoInventoryPrivate(out,sizeof(*out)) && !AmmoInventoryPrivate(error,size) &&
        !CAR_Overlap(ref,sizeof(*ref),registry,sizeof(*registry)) &&
        !CAR_Overlap(ref,sizeof(*ref),out,sizeof(*out)) &&
        !CAR_Overlap(registry,sizeof(*registry),out,sizeof(*out)) &&
        !CAR_Overlap(ref,sizeof(*ref),error,size) &&
        !CAR_Overlap(registry,sizeof(*registry),error,size) &&
        !CAR_Overlap(out,sizeof(*out),error,size);
}
static int OwnedClipAmmoFence(uint64_t serial,const GscEntityRef *input,
    const cod1AmmoRegistryReceipt_t *inputRegistry,const GscEntityRef *ref,
    const cod1AmmoRegistryReceipt_t *registry,const cod1AmmoPickupInventorySnapshot_t *staged,
    int32_t *out,char *error,size_t size) {
    cod1AmmoPickupInventorySnapshot_t fresh;
    return cod1AmmoInventoryGuard.active && !cod1AmmoInventoryGuard.cancelled &&
        cod1AmmoInventoryGuard.serial==serial &&
        OwnedClipAmmoBuffers(input,inputRegistry,out,error,size) &&
        !memcmp(input,ref,sizeof(*ref)) && !memcmp(inputRegistry,registry,sizeof(*registry)) &&
        AmmoInventoryProject(ref,registry,&fresh) && AmmoInventoryEqual(&fresh,staged);
}
static int OwnedClipAmmoStage(const cod1AmmoPickupInventorySnapshot_t *snapshot,
    int target,int32_t quantity,int *physicalClip,int32_t *nextClip,int32_t *delta) {
    const cod1AmmoPickupInventory_t *inventory=&snapshot->inventory;
    const cod1AmmoPickupWeapon_t *weapon;
    int32_t old,maximum,headroom,change,room;
    if(target<1 || target>inventory->weaponCount || quantity<=0 ||
       !CAP_Owned(inventory,target))return 0;
    weapon=&inventory->weapons[target];
    if(weapon->clipOnly!=1 || weapon->sharedCapIndex < -1 ||
       inventory->ammo[weapon->ammoIndex]!=0)return 0;
    old=inventory->clip[weapon->clipIndex];maximum=inventory->maxClip[weapon->clipIndex];
    if(old<0 || maximum<1 || old>maximum ||
       !CAP_AmmoHeadroom(inventory,target,&headroom) || headroom<0)return 0;
    room=maximum-old;change=quantity<room?quantity:room;
    if(change>headroom)change=headroom;
    /* change<=maximum-old: no unchecked old+requestedQuantity overflow.
     * Positive final prevents native RemoveWeapon/ownership mutation paths. */
    if(old+change<=0)return 0;
    *physicalClip=snapshot->mapping[target].clipCell;
    if(*physicalClip<=0 || *physicalClip>=MAX_WEAPONS)return 0;
    *nextClip=old+change;*delta=change;return 1;
}
int G_Cod1AddOwnedClipOnlyAmmo(const GscEntityRef *input,
    const cod1AmmoRegistryReceipt_t *inputRegistry,int selectedCAPIndex,int32_t quantity,
    int32_t *outDelta,char *error,size_t size,int (*current)(void *),void *context) {
    GscEntityRef ref;
    cod1AmmoRegistryReceipt_t registry;
    cod1AmmoPickupInventorySnapshot_t staged;
    uint64_t serial;
    int physicalClip,phase,ok=0;
    int32_t nextClip,delta;
    if(!OwnedClipAmmoBuffers(input,inputRegistry,outDelta,error,size))return 0;
    if(!input || !inputRegistry || !outDelta || !current)return 0;
    if(cod1AmmoInventoryGuard.active) {
        cod1AmmoInventoryGuard.cancelled=1;
        EntityError(error,size,"owned clip ammo operation reentered");return 0;
    }
    if(cod1AmmoInventorySerial==UINT64_MAX) {
        EntityError(error,size,"ammo inventory operation serial exhausted");return 0;
    }
    memcpy(&ref,input,sizeof(ref));memcpy(&registry,inputRegistry,sizeof(registry));
    serial=++cod1AmmoInventorySerial;
    cod1AmmoInventoryGuard.active=1;cod1AmmoInventoryGuard.cancelled=0;
    cod1AmmoInventoryGuard.serial=serial;
    /* Public Read would cancel this same guard and add extra callbacks. */
    if(!AmmoInventoryProject(&ref,&registry,&staged) ||
       !OwnedClipAmmoStage(&staged,selectedCAPIndex,quantity,&physicalClip,&nextClip,&delta))goto done;
    for(phase=0;phase<2;++phase) {
        if(!OwnedClipAmmoFence(serial,input,inputRegistry,&ref,&registry,&staged,outDelta,error,size) ||
           current(context)!=1 ||
           !OwnedClipAmmoFence(serial,input,inputRegistry,&ref,&registry,&staged,outDelta,error,size))goto done;
    }
    /* Exact captured allocation/client/pool/mapping and every relevant counter
     * were proved by the last pure fence. No call or fallible work follows the
     * first store. Only the mapped clip and caller scalar are published. */
    g_entities[ref.slot].client->ps.ammoclip[physicalClip]=nextClip;
    *outDelta=delta;ok=1;
done:
    cod1AmmoInventoryGuard.active=0;cod1AmmoInventoryGuard.cancelled=0;
    if(!ok && OwnedClipAmmoBuffers(input,inputRegistry,outDelta,error,size))
        EntityError(error,size,"owned clip ammo lost current owner, registry or admitted state");
    return ok;
}
