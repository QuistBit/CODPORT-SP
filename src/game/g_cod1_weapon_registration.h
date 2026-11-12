/* Internal completed dynamic resource registration, not a weapon grant. */
#ifndef G_COD1_WEAPON_REGISTRATION_H
#define G_COD1_WEAPON_REGISTRATION_H
#include "bg_cod1_ammo_registry.h"
#include "cod1_precache.h"
#define COD1_WEAPON_REGISTRATION_VERSION 1
typedef struct {
    int configstringSlot;
    cod1ModelResourceReference_t resource;
} cod1WeaponRegistrationModel_t;
typedef struct {
    int capIndex,cwIndex,itemIndex;
    cod1ItemDescriptor_t descriptor;
    cod1WeaponRegistrationModel_t models[2];
} cod1WeaponRegistrationRow_t;
typedef struct {
    uint32_t version,bytes;
    uintptr_t sessionOwner;
    uint64_t sessionId,mapEpoch,itemEpoch,incarnation,operationId;
    uint32_t registryRevision,moduleId,fingerprint;
    int selectedCAPIndex,count;
    cod1AmmoRegistryReceipt_t registry;
    char marker[COD1_ITEM_STATE_CAPACITY];
    cod1WeaponRegistrationRow_t rows[CW_MAX_DEFS];
} cod1WeaponRegistrationReceipt_t;
/* Two exact1 callbacks on success. Only completed R121 dynamic registrations
 * in the originating live Session qualify. Output remains unchanged on failure.
 * CS_MODELS and server-bank slots are separate namespaces. A fresh Read may
 * reacquire genuine same-name resources after bank replacement; old receipts
 * retain their old scene witness and fail Current. Neither is a grant ticket.
 * Normal RUNNING time advances preserve receipt identity; time is fenced only
 * within an operation. Exact marker/operation changes retire older receipts.
 * Existing CAR floating-point behavior and bounded borrowed-memory rules apply. */
int G_Cod1ReadWeaponRegistration(const cod1AmmoRegistryReceipt_t *registry,
    int selectedCAPIndex,cod1WeaponRegistrationReceipt_t *out,char *error,size_t size,
    int (*current)(void *),void *context);
/* Read-only engine calls, not callback-free. Valid Read/Current reentry cancels
 * the outer operation. Source receipt must stay valid and immutable throughout. */
int G_Cod1WeaponRegistrationCurrent(const cod1WeaponRegistrationReceipt_t *receipt);
#endif
