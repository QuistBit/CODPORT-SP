/* Selected port weapon acquisition using genuine completed registration. */
#ifndef G_COD1_ACQUIRE_CLIP_AMMO_H
#define G_COD1_ACQUIRE_CLIP_AMMO_H
#include "g_cod1_ammo_inventory.h"
#include "g_cod1_weapon_registration.h"
typedef struct {
    int newlyOwned;
    int32_t accepted;
    int wp,slot;
} cod1AcquireClipAmmoResult_t;

/* A successful owned/full operation may return accepted=0; the item remains.
 * First acquisition requires positive accepted rounds. Two exact1 caller
 * fences on success; ownership, target counters and slot publish together.
 * The live registration Session must remain RUNNING. No native registration
 * table equivalence, autoswitch or weapon replacement is claimed. */
int G_Cod1AcquireRegisteredClipOnlyAmmo(const GscEntityRef *player,
    const cod1WeaponRegistrationReceipt_t *registration,int32_t quantity,
    cod1AcquireClipAmmoResult_t *out,char *error,size_t size,
    int (*current)(void *),void *context);

/* Internal dependency inversion between the two real owners. The only
 * production caller is the registration wrapper, holding its exact lease.
 * current may issue read-only engine requests. storage_protected is pure.
 * Both facade bytes and every borrowed source stay immutable for the call. */
typedef struct {
    void *context;
    int (*current)(void *);
    int (*storage_protected)(void *,const void *,size_t);
} cod1AcquireClipAmmoAuthority_t;
int G_Cod1AcquireClipOnlyAmmo(const GscEntityRef *player,
    const cod1AmmoRegistryReceipt_t *registry,int selectedCAPIndex,int32_t quantity,
    const cod1AcquireClipAmmoAuthority_t *authority,cod1AcquireClipAmmoResult_t *out,
    char *error,size_t size,int (*current)(void *),void *context);
/* Delegates entity-host private classification. Do not feed this accessor
 * back into G_Cod1EntityStorageProtected: that would recurse. */
int G_Cod1AcquireClipAmmoStorageProtected(const void *,size_t);
#endif
