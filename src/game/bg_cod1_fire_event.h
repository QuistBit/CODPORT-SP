#ifndef BG_COD1_FIRE_EVENT_H
#define BG_COD1_FIRE_EVENT_H
#include "bg_cod1_inventory.h"
/* All CoD fire events carry the same registry index+1 as brass/sound.
 * Registry IDs and stable inventory slots are separate namespaces.
 * Preserve emitted projectile identity after disposal or a queued switch.
 * Legacy parm0 and AI events retain the held-weapon dispatch. */
static int CW_ProjectileFireEventWeapon(int event,int parameter,int ai) {
    const cwDef_t *d;const char *type;int root,weapon;
    event&=~EV_EVENT_BITS;
    if(ai || (event!=EV_FIRE_WEAPON && event!=EV_FIRE_WEAPON_LASTSHOT) ||
       parameter<1 || parameter>CW_MAX_DEFS)return WP_NONE;
    d=CW_Get(parameter-1);if(!d)return WP_NONE;
    type=CW_String(d,"weaponType");
    if(strcmp(type,"grenade") && strcmp(type,"projectile"))return WP_NONE;
    root=CW_FamilyRoot(parameter-1);
    for(weapon=1;weapon<WP_NUM_WEAPONS;++weapon)
        if(CW_WeaponRoot(weapon)==root)return weapon;
    return WP_NONE;
}
#endif
