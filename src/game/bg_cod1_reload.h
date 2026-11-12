#ifndef BG_COD1_RELOAD_H
#define BG_COD1_RELOAD_H
#include "bg_cod1_weapondef.h"
/* CoD1 SP magazine and segmented reload timing from WeaponDef/native probes.
 * Explicit state values live in the existing networked stats[] array. */
#define COD1_RELOAD_NORMAL 1
#define COD1_RELOAD_EMPTY 2
#define COD1_RELOAD_START 4
#define COD1_RELOAD_LOOP 5
#define COD1_RELOAD_END 6
#define COD1_RELOAD_STOP 8
static int CW_Grenade(const cwDef_t *d) {return !strcmp(CW_String(d,"weaponType"),"grenade");}
static int CW_Projectile(const cwDef_t *d) {return !strcmp(CW_String(d,"weaponType"),"projectile");}
static int CW_Inert(const cwDef_t *d) {return d && !strcmp(d->name,"stalingrad_ammo");}

static int CW_ReloadDuration(const cwDef_t *d,int empty) {
    return (int)CW_Number(d,empty?"reloadEmptyTime":"reloadTime");
}
static int CW_ReloadAddDelay(const cwDef_t *d,int empty) {
    int total=CW_ReloadDuration(d,empty),add=(int)CW_Number(d,"reloadAddTime");
    return add>0 && add<total ? add : total;
}
static int CW_ReloadSupported(const cwDef_t *d) {
    int normal=CW_ReloadDuration(d,0),empty=CW_ReloadDuration(d,1);
    double add=CW_Number(d,"reloadAddTime");
    if(d && CW_Number(d,"clipOnly"))return CW_Number(d,"fireTime")>0 && (!CW_Grenade(d) || (CW_Number(d,"holdFireTime")>0 && CW_Number(d,"fuseTime")>0));
    return d && normal>0 && normal<=30000 && empty>0 && empty<=30000 &&
        add>=0 && add<=30000 && (!CW_Number(d,"boltAction") || (CW_Number(d,"rechamberTime")>0 && CW_Number(d,"rechamberTime")<=30000)) &&
        (!CW_Number(d,"segmentedReload") || (CW_Number(d,"reloadAmmoAdd")>0 && CW_Number(d,"reloadStartTime")>0 && CW_Number(d,"reloadEndTime")>0));
}
#endif
