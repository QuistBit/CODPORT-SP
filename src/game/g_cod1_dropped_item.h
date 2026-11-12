#ifndef G_COD1_DROPPED_ITEM_H
#define G_COD1_DROPPED_ITEM_H
#if defined(COD1_GSC_TEST_WIP) && COD1_GSC_TEST_WIP
#include "cod1_gsc/gsc_fixture.h"
#include "bg_cod1_drop_frame.h"
/* Low-level real ET_ITEM backend, not a VM DropWeapon binding. Native item
 * content/trajectory fields are translated explicitly to port representation.
 * Caller supplies completed native spawn math; no RNG or Actor fields are
 * manufactured by this API. Registration is acquired from the real Session. */
typedef struct {
 int cwIndex,reserve,clip,angular;
 float origin[3],angles[3],velocity[3],angularDelta[3];
} cod1DroppedItemRequest_t;
/* Native Actor-only DropWeapon factory. none returns a zero ref without
 * reading hand/speed. Other names require existing registered assets. */
int G_Cod1ActorDropWeapon(const GscEntityRef*,const char*,const char*,float,GscEntityRef*,char*,size_t);
int G_Cod1ActorDropWeaponChecked(const GscEntityRef*,const char*,const char*,float,GscEntityRef*,char*,size_t,int(*)(void*),void*);
int G_Cod1DroppedItemsStop(uint64_t);
int G_Cod1DroppedItemsSelect(uint64_t,int(*)(void*),void*);
int G_Cod1DroppedItemSpawn(const cod1DroppedItemRequest_t*,GscEntityRef*,char*,size_t);
int G_Cod1DroppedItemRead(const GscEntityRef*,c1dfState*,char*,size_t);
/* Returns1 for an owned allocation even after quarantine; never falls back
 * to RTCW item physics for a failed owned row.0 is genuinely unowned. */
int G_Cod1DroppedItemFrame(struct gentity_s*);
/* Internal real Touch_Item_Auto lifecycle.0 unowned,-1 refused,1 owned.
 * Begin/End bracket the actual pickup function; Commit follows a positive
 * acquisition and consumes exactly the captured original allocation. */
int G_Cod1DroppedItemPickupBegin(struct gentity_s*,struct gentity_s*,GscEntityRef*);
int G_Cod1DroppedItemPickupOwner(struct gentity_s*,GscEntityRef*);
int G_Cod1DroppedItemPickupCurrent(const GscEntityRef*);
int G_Cod1DroppedItemPickupCommit(const GscEntityRef*,int);
int G_Cod1DroppedItemPickupPlayerAlive(const GscEntityRef*,const struct gentity_s*);
void G_Cod1DroppedItemPickupEnd(const GscEntityRef*);
int G_Cod1DroppedItemStorageProtected(const void*,size_t);
void G_Cod1DroppedItemsReset(void);
#endif
#endif
