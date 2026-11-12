/* SPDX-License-Identifier: GPL-3.0-or-later
 * BRASS_R1: small, shared cosmetic-event contract. No new ammo timer or RNG.
 * The payload is a registry index+1 (validated by the existing registry hash),
 * NOT the player's current weapon when the client later handles the event.
 */
#ifndef BG_COD1_BRASS_H
#define BG_COD1_BRASS_H
#include "bg_cod1_inventory.h"
#include "bg_cod1_reload.h"
static int CWB_WeaponParm(const playerState_t *ps) {
    const cwDef_t *d; int index;
    if(!ps || ps->aiChar || !(d=CW_PSDef(ps)))return 0;
    index=CW_Find(d->name);
    return index>=0 && index<CW_MAX_DEFS ? index+1 : 0;
}
static int CWB_ConsumeBolt(playerState_t *ps) {
    const cwDef_t *d; int bit,parm;
    if(!ps || !(d=CW_PSDef(ps)) || !CW_Number(d,"boltAction"))return 0;
    bit=CW_BoltBit(ps->weapon);
    if(!bit || !(ps->cod1Rechamber&bit))return 0;
    parm=CWB_WeaponParm(ps);ps->cod1Rechamber&=~bit;
    if(parm)BG_AddPredictableEventToPlayerstate(EV_COD1_EJECT_BRASS,parm,ps);
    return parm;
}
/* Inspected gamex86 2000bfe0/2000c056 chooses the bolt delay before the reload
 * add-ammo milestone. Observe that milestone without repurposing weaponDelay:
 * the port's existing insert-ammo/reload-sound timings stay unchanged. */
static int CWB_ReloadDelay(const playerState_t *ps,int *totalOut) {
    const cwDef_t *d=CW_PSDef(ps);int phase,total,limit,delay,empty;
    if(!d || ps->weaponstate!=WEAPON_RELOADING || !CW_Number(d,"boltAction"))return 0;
    phase=ps->stats[STAT_COD1_RELOAD]&~COD1_RELOAD_STOP;
    if(phase==COD1_RELOAD_END || phase==COD1_RELOAD_LOOP)return 0;
    empty=phase==COD1_RELOAD_EMPTY;
    if(phase==COD1_RELOAD_START) {
        total=(int)CW_Number(d,"reloadStartTime");limit=(int)CW_Number(d,"reloadStartAddTime");
    } else if(phase==COD1_RELOAD_NORMAL || empty) {
        total=CW_ReloadDuration(d,empty);limit=CW_ReloadAddDelay(d,empty);
    } else return 0;
    if(total<1)total=1;
    if(limit<=0 || limit>total)limit=total;
    delay=(int)CW_Number(d,"rechamberBoltTime");
    if(delay<=0 || delay>=limit)delay=1;
    *totalOut=total;return delay;
}
static void CWB_CheckReload(playerState_t *ps,int msec) {
    int total=0,delay;
    if(!ps || msec<=0 || !(ps->cod1Rechamber&CW_BoltBit(ps->weapon)))return;
    delay=CWB_ReloadDelay(ps,&total);
    if(delay && (double)total-ps->weaponTime+msec>=delay)CWB_ConsumeBolt(ps);
}
#endif
