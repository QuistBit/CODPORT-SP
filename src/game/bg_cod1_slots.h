#ifndef BG_COD1_SLOTS_H
#define BG_COD1_SLOTS_H
/* Native category IDs, including the reserved smoke grenade category.
 * The array is separate from ownership bits and keeps primary positions stable. */
enum { CW_SLOT_NONE, CW_SLOT_PRIMARY, CW_SLOT_PRIMARYB, CW_SLOT_PISTOL,
       CW_SLOT_GRENADE, CW_SLOT_SMOKE, CW_SLOT_COUNT };
static int CW_SlotKind(int weapon) {
    static const char *names[CW_SLOT_COUNT]={"none","primary","primaryb","pistol","grenade","smokegrenade"};
    const cwDef_t *d=CW_WeaponDef(weapon);const char *name;int i;
    if(!d)return 0;name=CW_String(d,"weaponSlot");
    for(i=1;i<CW_SLOT_COUNT;i++)if(!Q_stricmp(name,names[i]))return i;
    return 0;
}
static int CW_SlotStackable(int weapon) {return CW_Number(CW_WeaponDef(weapon),"slotStackable")!=0;}
static int CW_SlotCompatible(int slot,int weapon) {
    int kind=CW_SlotKind(weapon);
    return kind && slot>0 && slot<CW_SLOT_COUNT &&
        (kind==slot || ((kind==1 || kind==2) && (slot==1 || slot==2)));
}
static int CW_OccupiedSlot(const playerState_t *ps,int weapon) {
    int i;if(weapon<=WP_NONE || weapon>=WP_NUM_WEAPONS || !COM_BitCheck(ps->weapons,weapon))return 0;
    for(i=1;i<CW_SLOT_COUNT;i++)if(ps->cod1Slots[i]==weapon && CW_SlotCompatible(i,weapon))return i;
    return 0; /* alternate definitions already share a root WP identifier */
}
static int CW_VacantSlot(const playerState_t *ps,int weapon) {
    int kind=CW_SlotKind(weapon);
    if(kind==1 || kind==2){if(!ps->cod1Slots[1])return 1;if(!ps->cod1Slots[2])return 2;return 0;}
    return kind>=3 && kind<CW_SLOT_COUNT && !ps->cod1Slots[kind]?kind:0;
}
static int CW_StackSlot(const playerState_t *ps,int weapon,int preferred) {
    int kind=CW_SlotKind(weapon),i;
    if(!CW_SlotStackable(weapon))return 0;
    if(kind==1 || kind==2){
        if(preferred==1 || preferred==2){i=ps->cod1Slots[preferred];if(!i || CW_SlotStackable(i))return preferred;}
        i=ps->cod1Slots[1];if(!i || CW_SlotStackable(i))return 1;
        i=ps->cod1Slots[2];if(!i || CW_SlotStackable(i))return 2;
        return 0;
    }
    if(kind<3 || kind>=CW_SLOT_COUNT)return 0;
    i=ps->cod1Slots[kind];return !i || CW_SlotStackable(i)?kind:0;
}
/* Reconcile spawn/cheat/legacy ownership without deleting weapons. Capacity
 * enforcement belongs in pickup/give transactions, not movement prediction. */
static void CW_SyncSlots(playerState_t *ps) {
    int i,j,w,slot;if(ps->aiChar)return;ps->cod1Slots[0]=0;
    for(i=1;i<CW_SLOT_COUNT;i++){
        w=ps->cod1Slots[i];
        if(w<=WP_NONE || w>=WP_NUM_WEAPONS || !COM_BitCheck(ps->weapons,w) || !CW_SlotCompatible(i,w)){ps->cod1Slots[i]=0;continue;}
        for(j=1;j<i;j++)if(ps->cod1Slots[j]==w){ps->cod1Slots[i]=0;break;}
    }
    /* Prefer the active stack member, preserving fixed primary positions. */
    w=ps->weapon;
    if(w>WP_NONE && w<WP_NUM_WEAPONS && COM_BitCheck(ps->weapons,w) && !CW_OccupiedSlot(ps,w)){
        slot=CW_VacantSlot(ps,w);if(!slot)slot=CW_StackSlot(ps,w,0);
        if(slot)ps->cod1Slots[slot]=w;
    }
    for(w=1;w<WP_NUM_WEAPONS;w++)if(COM_BitCheck(ps->weapons,w) && !CW_OccupiedSlot(ps,w)){
        slot=CW_VacantSlot(ps,w);if(slot)ps->cod1Slots[slot]=w;
    }
}
/* Resolve a pickup without changing state. Replacements require explicit use.
 * Retail fallback checks the incoming ammo/clip indices (gamex86 0x2001af12),
 * not the ammunition of the primary it is about to drop. */
static int CW_PickupSlot(const playerState_t *ps,int weapon,int replace) {
    int slot,kind=CW_SlotKind(weapon);
    slot=CW_OccupiedSlot(ps,weapon);if(slot)return slot;
    slot=CW_VacantSlot(ps,weapon);if(slot)return slot;
    slot=CW_StackSlot(ps,weapon,CW_OccupiedSlot(ps,ps->weapon));if(slot)return slot;
    if(!replace)return 0;
    if(kind==CW_SLOT_PRIMARY || kind==CW_SLOT_PRIMARYB){
        slot=CW_OccupiedSlot(ps,ps->weapon);
        if(slot==CW_SLOT_PRIMARY || slot==CW_SLOT_PRIMARYB)return slot;
        if(!ps->ammo[BG_PlayerAmmoIndex(ps,weapon)] && !ps->ammoclip[BG_FindClipForWeapon(weapon)])return CW_SLOT_PRIMARY;
        return 0;
    }
    return kind>=CW_SLOT_PISTOL && kind<CW_SLOT_COUNT?kind:0;
}
/* Recover a removed client selection from the authoritative occupied place.
 * This also works if a pickup event was lost or autoswitch is disabled. */
static int CW_ReconcileSelection(const playerState_t *ps,const playerState_t *old,int selected) {
    int slot,w;
    if(ps->aiChar || !CW_WeaponDef(selected) || COM_BitCheck(ps->weapons,selected))return selected;
    slot=CW_OccupiedSlot(old,selected);
    if(slot){w=ps->cod1Slots[slot];if(w>0 && w<WP_NUM_WEAPONS && COM_BitCheck(ps->weapons,w))return w;}
    w=ps->weapon;
    if(w>0 && w<WP_NUM_WEAPONS && COM_BitCheck(ps->weapons,w))return w;
    for(slot=1;slot<CW_SLOT_COUNT;slot++){
        w=ps->cod1Slots[slot];if(w>0 && w<WP_NUM_WEAPONS && COM_BitCheck(ps->weapons,w))return w;
    }
    return WP_NONE;
}
#endif
