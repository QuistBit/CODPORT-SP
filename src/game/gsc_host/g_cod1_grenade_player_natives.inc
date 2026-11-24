#include "../g_cod1_grenade_player.h"
static int GrenadePlayerBuffers(const GscPlayerNativeRequest *input,
    GscPlayerNativeResult *out,char *error,size_t size) {
    const void *p[3];size_t z[3];int i,j;
    if(!AmmoInventoryPoolSafe()||!input||!out)return 0;
    p[0]=input;z[0]=sizeof(*input);p[1]=out;z[1]=sizeof(*out);p[2]=error;z[2]=size;
    for(i=0;i<3;++i) {
        if(AmmoInventoryPrivate(p[i],z[i])||
           CAR_Overlap(p[i],z[i],G_Cod1GrenadePlayerNativeHost(),sizeof(GscPlayerNativeHost)))return 0;
        for(j=0;j<i;++j)if(CAR_Overlap(p[i],z[i],p[j],z[j]))return 0;
    }
    return 1;
}
static int GrenadePlayerNativeInvoke(void *user,const GscPlayerNativeRequest *input,
    GscPlayerNativeResult *out,char *error,size_t size,int (*current)(void *),void *context) {
    GscPlayerNativeRequest request;GscPlayerNativeResult result;
    cod1AmmoRegistryReceipt_t registry;cod1AmmoPickupInventorySnapshot_t staged,fresh;
    playerState_t before;int target,amount,maximum,now,ok=0;
    uint64_t serial;
    (void)user;
    if(!GrenadePlayerBuffers(input,out,error,size))return 0;
    if(input->kind!=GSC_PLAYER_SET_ANGLES&&(input->angles_bits[0]||input->angles_bits[1]||input->angles_bits[2]))return 0;
    if(input->kind!=GSC_PLAYER_GET_FRACTION_MAX_AMMO)
        return PlayerNativeInvoke(NULL,input,out,error,size,current,context);
    if(!current)return 0;
    if(cod1AmmoInventoryGuard.active){cod1AmmoInventoryGuard.cancelled=1;return 0;}
    if(cod1AmmoInventorySerial==UINT64_MAX)return 0;
    request=*input;memset(&result,0,sizeof(result));
    serial=++cod1AmmoInventorySerial;cod1AmmoInventoryGuard.active=1;
    cod1AmmoInventoryGuard.cancelled=0;cod1AmmoInventoryGuard.serial=serial;now=level.time;
    if(request.now_ms<0||request.value||!memchr(request.weapon,0,sizeof(request.weapon))||
       !request.weapon[0]||!CAR_Build(&registry)||!CAR_Current(&registry)||
       !AmmoInventoryProject(&request.player,&registry,&staged))goto done;
    before=g_entities[request.player.slot].client->ps;
    result.kind=GSC_PLAYER_RESULT_FLOAT;
    /* Original gamex86 20015740: a non-owned index (including none) returns
     * 1; owned clipOnly uses clip/maxClip, other weapons stock/maxAmmo.
     * Unknown names and malformed mapped counters still refuse in this port. */
    if(!strcmp(request.weapon,"none"))result.number=1.0f;
    else {
        const cod1AmmoPickupWeapon_t *weapon;
        target=CW_Find(request.weapon)+1;
        if(target<1||target>registry.weaponCount)goto done;
        weapon=&staged.inventory.weapons[target];
        if(!CAP_Owned(&staged.inventory,target))result.number=1.0f;
        else {
            if(weapon->clipOnly) {
                amount=staged.inventory.clip[weapon->clipIndex];
                maximum=staged.inventory.maxClip[weapon->clipIndex];
                if(staged.inventory.ammo[weapon->ammoIndex]!=0)goto done;
            } else {
                amount=staged.inventory.ammo[weapon->ammoIndex];
                maximum=staged.inventory.maxAmmo[weapon->ammoIndex];
            }
            if(maximum<=0||amount<0||amount>maximum)goto done;
            result.number=(float)amount/(float)maximum;
        }
    }
    if(!isfinite(result.number)||result.number<0.0f||result.number>1.0f||
       current(context)!=1||!cod1AmmoInventoryGuard.active||cod1AmmoInventoryGuard.cancelled||
       cod1AmmoInventoryGuard.serial!=serial||level.time!=now||
       !GrenadePlayerBuffers(input,out,error,size)||memcmp(input,&request,sizeof(request))||
       !AmmoInventoryProject(&request.player,&registry,&fresh)||!AmmoInventoryEqual(&staged,&fresh)||
       memcmp(&before,&g_entities[request.player.slot].client->ps,sizeof(before)))goto done;
    memcpy(out,&result,sizeof(result));ok=1;
done:
    cod1AmmoInventoryGuard.active=0;cod1AmmoInventoryGuard.cancelled=0;
    if(!ok&&GrenadePlayerBuffers(input,out,error,size))
        EntityError(error,size,"ammo fraction requires current mapped inventory and finite supported capacity");
    return ok;
}
const GscPlayerNativeHost *G_Cod1GrenadePlayerNativeHost(void) {
    static const GscPlayerNativeHost host={NULL,GrenadePlayerNativeInvoke};return &host;
}
