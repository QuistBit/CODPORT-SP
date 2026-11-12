#include "../g_cod1_acquire_clip_ammo.h"
/* This wrapper is beside the actual R121/R215 owner. Entity-host code never
 * acquires a new link dependency on the optional public Session test TU. */
typedef struct {
    WeaponRegistrationLease registration;
    const GscEntityRef *inputPlayer;
    GscEntityRef player;
    const cod1WeaponRegistrationReceipt_t *inputRegistration;
    cod1AcquireClipAmmoResult_t *out;
    char *error;size_t size;
} RegisteredClipAmmoLease;
static int RegisteredClipAmmoBuffers(const GscEntityRef *player,
    const cod1WeaponRegistrationReceipt_t *registration,cod1AcquireClipAmmoResult_t *out,
    char *error,size_t size) {
    const void *p[4];size_t z[4];int i,j;
    if(!player || !registration || !out)return 0;
    p[0]=player;z[0]=sizeof(*player);p[1]=registration;z[1]=sizeof(*registration);
    p[2]=out;z[2]=sizeof(*out);p[3]=error;z[3]=size;
    for(i=0;i<4;++i) {
        if(WeaponRegistrationPrivate(p[i],z[i]) || G_Cod1AcquireClipAmmoStorageProtected(p[i],z[i]))return 0;
        for(j=0;j<i;++j)if(CAR_Overlap(p[i],z[i],p[j],z[j]))return 0;
    }
    return 1;
}
static int RegisteredClipAmmoOwnerProtected(void *context,const void *p,size_t n) {
    (void)context;return WeaponRegistrationPrivate(p,n);
}
static int RegisteredClipAmmoCurrent(void *context) {
    RegisteredClipAmmoLease *lease=(RegisteredClipAmmoLease *)context;
    return RegisteredClipAmmoBuffers(lease->inputPlayer,lease->inputRegistration,
               lease->out,lease->error,lease->size) &&
        !memcmp(lease->inputPlayer,&lease->player,sizeof(lease->player)) &&
        !memcmp(lease->inputRegistration,&weaponRegistrationStage,sizeof(weaponRegistrationStage)) &&
        WeaponRegistrationProject(&lease->registration,weaponRegistrationStage.selectedCAPIndex,
            &weaponRegistrationFresh) &&
        !memcmp(&weaponRegistrationStage,&weaponRegistrationFresh,sizeof(weaponRegistrationStage)) &&
        RegisteredClipAmmoBuffers(lease->inputPlayer,lease->inputRegistration,
               lease->out,lease->error,lease->size) &&
        !memcmp(lease->inputPlayer,&lease->player,sizeof(lease->player)) &&
        !memcmp(lease->inputRegistration,&weaponRegistrationStage,sizeof(weaponRegistrationStage)) &&
        WeaponRegistrationScope(&lease->registration);
}
int G_Cod1AcquireRegisteredClipOnlyAmmo(const GscEntityRef *player,
    const cod1WeaponRegistrationReceipt_t *registration,int32_t quantity,
    cod1AcquireClipAmmoResult_t *out,char *error,size_t size,
    int (*current)(void *),void *context) {
    RegisteredClipAmmoLease lease;cod1AcquireClipAmmoAuthority_t authority;
    cod1AmmoRegistryReceipt_t registry;int ok=0;
    if(!RegisteredClipAmmoBuffers(player,registration,out,error,size) || !current)return 0;
    if(weaponRegistrationGuard.active){weaponRegistrationGuard.cancelled=1;return 0;}
    memset(&lease,0,sizeof(lease));lease.inputPlayer=player;lease.player=*player;
    lease.inputRegistration=registration;lease.out=out;lease.error=error;lease.size=size;
    weaponRegistrationStage=*registration;weaponRegistrationRegistry=registration->registry;
    if(!WeaponRegistrationBegin(&lease.registration))goto done;
    if(!RegisteredClipAmmoCurrent(&lease))goto done;
    registry=weaponRegistrationRegistry;
    authority.context=&lease;authority.current=RegisteredClipAmmoCurrent;
    authority.storage_protected=RegisteredClipAmmoOwnerProtected;
    ok=G_Cod1AcquireClipOnlyAmmo(player,&registry,weaponRegistrationStage.selectedCAPIndex,
        quantity,&authority,out,error,size,current,context);
    /* The inner commit already proved both owners. No trap/check follows a
     * successful commit. End only releases this synchronous private guard. */
done:
    WeaponRegistrationEnd();
    if(!ok && RegisteredClipAmmoBuffers(player,registration,out,error,size))
        GscTestError(error,size,"registered weapon acquisition lost completed resources or player");
    return ok;
}
