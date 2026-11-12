#ifndef BG_COD1_INVENTORY_H
#define BG_COD1_INVENTORY_H
#include "bg_cod1_weapondef.h"
/* Stable inventory slots and mode bits; definition indices may depend on
 * which resource families are installed. Thompson retains mode bit zero. */
static int CW_WeaponRoot(int weapon) {
    const char *name;
    switch(weapon) {
    case WP_THOMPSON:name="thompson";break;
    case WP_MP40:name="mp40";break;
    case WP_STEN:name="sten";break;
    case WP_MP44:name="mp44";break;
    case WP_PPSH:name="ppsh";break;
    case WP_BAR:name="bar";break;
    case WP_BREN:name="bren";break;
    case WP_FG42:name="fg42";break;
    case WP_GARAND:name="m1garand";break;
    case WP_M1CARBINE:name="m1carbine";break;
    case WP_COLT:name="colt";break;
    case WP_LUGER:name="luger";break;
    case WP_KAR98K:name="kar98k";break;
    case WP_ENFIELD:name="enfield";break;
    case WP_MOSIN:name="mosin_nagant";break;
    case WP_KAR98KSCOPE:name="kar98k_sniper";break;
    case WP_MOSINSCOPE:name="mosin_nagant_sniper";break;
    case WP_SPRINGFIELD:name="springfield";break;
    case WP_MK1:name="mk1britishfrag";break;
    case WP_RGD33:name="rgd-33russianfrag";break;
    case WP_STALINGRAD_AMMO:name="stalingrad_ammo";break;
    case WP_PANZERFAUST:name="panzerfaust";break;
    case WP_GRENADE_PINEAPPLE:name="fraggrenade";break;
    case WP_GRENADE_LAUNCHER:name="stielhandgranate";break;
    default:return -1;
    }
    return CW_Find(name);
}
static int CW_BoltBit(int weapon) {return weapon>=WP_KAR98K && weapon<=WP_SPRINGFIELD ? 1<<(weapon-WP_KAR98K):0;}
static int CW_ModeBit(int weapon) {
    switch(weapon) {
    case WP_THOMPSON:return 1;case WP_MP44:return 2;case WP_PPSH:return 4;
    case WP_BAR:return 8;case WP_FG42:return 16;default:return 0;
    }
}
static const cwDef_t *CW_WeaponDef(int weapon) { return CW_Get(CW_WeaponRoot(weapon)); }
static const cwDef_t *CW_PSDef(const playerState_t *ps) {
    const cwDef_t *root=CW_WeaponDef(ps->weapon);
    if(root && (ps->stats[STAT_COD1_MODE]&CW_ModeBit(ps->weapon)) && CW_Get(root->altIndex))return CW_Get(root->altIndex);
    return root;
}
static void CW_SetMode(playerState_t *ps,int weapon,int definition) {
    int root=CW_WeaponRoot(weapon),bit=CW_ModeBit(weapon);
    if(root<0 || CW_FamilyRoot(definition)!=root)return;
    if(definition==root)ps->stats[STAT_COD1_MODE]&=~bit;
    else ps->stats[STAT_COD1_MODE]|=bit;
}
static int CW_Active(const playerState_t *ps) { return !ps->aiChar && CW_WeaponDef(ps->weapon)!=NULL; }
static int CW_MaxClip(const playerState_t *ps,int weapon) {
    const cwDef_t *d=ps->aiChar?NULL:CW_WeaponDef(weapon);
    return d?(int)CW_Number(d,"clipSize"):ammoTable[weapon].maxclip;
}
static int CW_MaxAmmo(const playerState_t *ps,int weapon) {
    const cwDef_t *d=ps->aiChar?NULL:CW_WeaponDef(weapon);
    return d?(int)CW_Number(d,"maxAmmo"):ammoTable[BG_FindAmmoForWeapon(weapon)].maxammo;
}
#include "bg_cod1_slots.h"
#endif
