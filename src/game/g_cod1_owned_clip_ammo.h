/* R214 selected internal owned clipOnly AddAmmo effect. No gameplay/VM/item
 * caller or first unowned acquisition. Selected indices are CAR CAP i+1. */
#ifndef G_COD1_OWNED_CLIP_AMMO_H
#define G_COD1_OWNED_CLIP_AMMO_H
#include "g_cod1_ammo_inventory.h"
/* Current canonical owner/CAR, target already owned and clipOnly, reserve0,
 * bounded old clip and nonnegative capacity. Positive quantity may yield0;
 * zero delta is completed validation, never item-pickup success. Final clip0
 * and ownership/removal paths are held. Two exact-1 borrowed current callbacks
 * precede the single physical clip-cell/output commit. Refusal preserves out;
 * foreign callback effects are not rolled back. Ref/receipt/context/output
 * remain valid synchronous borrowed memory. Existing CAR FP policy retained. */
int G_Cod1AddOwnedClipOnlyAmmo(const GscEntityRef *player,
    const cod1AmmoRegistryReceipt_t *registry,int selectedCAPIndex,int32_t quantity,
    int32_t *outDelta,char *error,size_t size,int (*current)(void *),void *context);
#endif
