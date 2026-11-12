/* R216 explicitly selected real port grenade items. ET_ITEM uses a genuine
 * bg_itemlist interaction row; the exact original visual uses an independent
 * CS_MODELS slot. Neither index is a retail/native entity ABI. */
#ifndef G_COD1_ITEM_LIFECYCLE_H
#define G_COD1_ITEM_LIFECYCLE_H
#include "g_cod1_weapon_registration.h"

typedef struct {
    void *user;
    int (*current)(void *user);
    /* Exact genuine bg_itemlist interaction row; no borrowed row contents are
     * read by this module. This keeps unselected thin hosts independent of BG. */
    int (*interaction_item)(void *user,int wp,int *index,uintptr_t *row);
    /* Actual port engine floor trace with these exact bounds, refusing
     * start/all-solid and malformed replies. Suspended items never call it. */
    int (*floor_trace)(void *user,int slot,const float origin[3],
        const float mins[3],const float maxs[3],float placed[3],int *ground);
    int (*read_registration)(void *user,int selectedCAPIndex,
        cod1WeaponRegistrationReceipt_t *out,char *error,size_t size,
        int (*current)(void *),void *context);
    int (*acquire)(void *user,const GscEntityRef *player,
        const cod1WeaponRegistrationReceipt_t *registration,int32_t quantity,
        int32_t *accepted,char *error,size_t size,
        int (*current)(void *),void *context);
    /* Called only after a positive committed acquisition and world removal.
     * A notification refusal cannot roll the inventory back or revive item. */
    int (*pickup_notify)(void *user,const GscEntityRef *item,
        const GscEntityRef *player,char *error,size_t size);
} cod1ItemLifecycleServices_t;

/* Map-owned, copied registry/provider selection. Provider's genuine registrar
 * must remain current (normally a RUNNING Session) for every later pickup. */
int G_Cod1ItemLifecycleSelect(const cod1AmmoRegistryReceipt_t *,
    const cod1ItemLifecycleServices_t *,char *,size_t);
/* Exact selected classes: weapon_fraggrenade, weapon_stielhandgranate,
 * item_ammo_stielhandgranate_open/closed. Flags are exactly0 or suspended1.
 * Adopt finishes an existing real BSP allocation; no replacement is adopted. */
int G_Cod1ItemLifecycleAdopt(const GscEntityRef *,char *,size_t);
int G_Cod1ItemLifecycleSpawn(const char *,const float origin[3],int32_t flags,
    GscEntityRef *,char *,size_t);
/* Pure routing predicate includes a known but no-longer-valid owned row, so
 * selected wrappers must refuse that row rather than fall back to old hosts. */
int G_Cod1ItemLifecycleOwns(const GscEntityRef *);
/* Read-only current selected frag owner for fixed-mask filtering. Requires
 * the caller's independent typed BSP/CW binding; no geometry/graph permit. */
int G_Cod1ItemLifecycleMaskCurrent(const GscEntityRef *,int nativeItemIndex);
int G_Cod1ItemLifecycleSetOrigin(const GscEntityRef *,const float[3],char *,size_t);
int G_Cod1ItemLifecycleSetAngles(const GscEntityRef *,const float[3],char *,size_t);
int G_Cod1ItemLifecycleSetCount(const GscEntityRef *,int32_t,char *,size_t);
int G_Cod1ItemLifecycleGetCount(const GscEntityRef *,int32_t *,char *,size_t);
/* Same production function used by the actual entity touch callback. Failure
 * and zero accepted quantity preserve item; positive acquisition consumes once. */
int G_Cod1ItemLifecyclePickup(const GscEntityRef *,const GscEntityRef *,
    int activated,int32_t *accepted,char *,size_t);
int G_Cod1ItemLifecycleStorageProtected(const void *,size_t);
/* Called only by entity host's map lifecycle; an in-flight guard is cancelled
 * and retained until its own return. No foreign callback occurs here. */
void G_Cod1ItemLifecycleReset(void);
/* Exact five-BSP-item construction journal. Execute transfers action routing
 * at the real script-start boundary; Commit retires this journal only after
 * that runtime start succeeds. Abort never restores an initial mask/body. */
typedef struct {uint64_t epoch,selection,transaction;} cod1ItemPreparedReceipt_t;
int G_Cod1ItemLifecyclePreparedBegin(const GscEntityRef[5],cod1ItemPreparedReceipt_t*,char*,size_t);
int G_Cod1ItemLifecyclePreparedExecute(const cod1ItemPreparedReceipt_t*,char*,size_t);
int G_Cod1ItemLifecyclePreparedCommit(const cod1ItemPreparedReceipt_t*,char*,size_t);
int G_Cod1ItemLifecyclePreparedAbort(const cod1ItemPreparedReceipt_t*,char*,size_t);
/* Pure guard, normally1; only the exact in-flight prepared abort narrows the
 * native free boundary. A callback may replace the body without reusing slot. */
int G_Cod1ItemLifecyclePreparedFreeGuard(const GscEntityRef*);

#endif
