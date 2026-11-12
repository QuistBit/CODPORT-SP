/* First-person native viewmodel opt-in. Keep the accepted cvar values 0/1/2
 * unchanged; value 3 enables eligible WeaponDefs found by the game registry. */
#ifndef BG_COD1_NATIVE_MODE_H
#define BG_COD1_NATIVE_MODE_H
#include "bg_cod1_weapondef.h"

static int CW_NativeViewMode(const cwDef_t *def,int setting) {
    const char *type;
    if(!def)return 0;
    if(setting>=3) {
        type=CW_String(def,"weaponType");
        return ((!Q_stricmp(type,"bullet") || !Q_stricmp(type,"projectile")) &&
            *CW_String(def,"gunModel") && *CW_String(def,"handModel") &&
            *CW_String(def,"idleAnim"))?2:0;
    }
    return !Q_stricmp(def->name,"thompson")?setting:
        (!Q_stricmp(def->name,"mp40") && setting>=2)?2:0;
}

static const char *CW_NativeViewPrefix(const cwDef_t *def,int setting) {
    int mode=CW_NativeViewMode(def,setting);
    return mode>=2?"cod1native2/":mode==1?"cod1native/":"cod1view64/";
}

/* cwViewProfile intentionally omits some of the twenty XAnim leaves.
 * A mode-3 model therefore belongs to one WeaponDef, even when an alternate
 * has an identical legacy view profile. */
static int CW_NativeViewMayShare(int setting,int requestedKind) {
    return !(setting>=3 && requestedKind==2);
}
#endif
