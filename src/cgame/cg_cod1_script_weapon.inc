/* R198 private paired reliable command. The server has already checked the
 * connected player, loaded name and ownership. Snapshot transitions process
 * reliable commands before the new inventory snapshot, so the old ownership
 * bitset is deliberately not used to reject a newly given weapon. */
#include "../game/bg_cod1_inventory.h"
static void CG_Cod1ScriptWeaponCommand(void) {
    char text[16];int i,weapon=0;
    if(!cgs.cod1MapEntities||cgs.gametype!=GT_SINGLE_PLAYER||trap_Argc()!=2)return;
    memset(text,0xa5,sizeof(text));trap_Argv(1,text,sizeof(text));
    if(!memchr(text,0,sizeof(text))||!text[0])return;
    for(i=0;text[i];++i) {
        if(text[i]<'0'||text[i]>'9'||i>2)return;
        weapon=weapon*10+(text[i]-'0');
        if(weapon>=WP_NUM_WEAPONS)return;
    }
    if(weapon!=WP_NONE&&!CW_WeaponDef(weapon))return;
    CG_FinishWeaponChange(cg.weaponSelect,weapon);
}
