/* PATCH52: CoD ladder -> lower -> unarmed -> raise sequence.
 * Native cgame 0x3000c330, 0x3000b720, 0x3000b8c0, 0x3000bab0.
 * RTCW retains the inventory weapon ID; CW_LADDER_HIDDEN represents CoD's
 * active weapon 0. All state is in existing transmitted playerState fields.
 * Included by bg_pmove.c; gameplay must stop along with first-person drawing.
 */
static qboolean PM_Cod1LadderWeapon(const cwDef_t *def) {
    playerState_t *ps=pm->ps;
    int *bits=&ps->stats[STAT_COD1_EDGES];
    int attached=!!(ps->pm_flags&PMF_LADDER);
    int hidden=!!(*bits&CW_LADDER_HIDDEN);
    int active=!!(*bits&CW_LADDER_WEAPON);
    if(!attached && !active)return qfalse;
    /* Keep the existing selector adapter until its transition completes;
     * ordinary auto/semi definitions both use the sequence below. */
    if(!active && ps->weaponstate==WEAPON_COD1_SWITCH)return qfalse;
    if(!attached && pm->cmd.weapon!=ps->weapon) {
        *bits&=~(CW_LADDER_WEAPON|CW_LADDER_HIDDEN);
        ps->weaponstate=WEAPON_READY;ps->weaponTime=ps->weaponDelay=0;
        return qfalse; /* Other inventory weapons still use their own bridge. */
    }
    if(ps->weaponTime>0) {
        ps->weaponTime-=pml.msec;
        if(ps->weaponTime<=0) {
            ps->weaponTime=0;
            /* Native timer helper 0x3000c260 also retains a 1-ms timer
             * for semi-auto while fire is held and the selected gun has
             * ammunition. This applies to drop/raise as well as firing. */
            if(!hidden && CW_Number(def,"semiAuto") &&
               (pm->cmd.buttons&BUTTON_ATTACK) && pm->cmd.weapon==ps->weapon &&
               ps->ammoclip[BG_FindClipForWeapon(ps->weapon)]>0) {
                ps->weaponTime=1;
                if(ps->weaponstate==WEAPON_FIRING)ps->weaponstate=WEAPON_READY;
            }
        }
    }
    if(ps->weaponDelay>0) {
        ps->weaponDelay-=pml.msec;
        if(ps->weaponDelay<0)ps->weaponDelay=0;
    }
    /* The current shot's recovery cannot be interrupted by the ladder.
     * Reload may be interrupted before its delayed magazine transfer. */
    if(!hidden && attached && ps->weaponstate!=WEAPON_DROPPING &&
       !(ps->weaponstate==WEAPON_FIRING && ps->weaponTime>0)) {
        *bits|=CW_LADDER_WEAPON;active=1;
        ps->weaponstate=WEAPON_DROPPING;
        ps->weaponTime=CW_StowTime(def,0);ps->weaponDelay=0;
        ps->stats[STAT_COD1_RELOAD]=ps->stats[STAT_COD1_SWITCH_TARGET]=0;
        PM_StartWeaponAnim(WEAP_DROP);
    }
    ps->eFlags&=~EF_FIRING;
    if(!active)return qtrue;
    if(hidden && !attached) {
        *bits&=~CW_LADDER_HIDDEN;
        ps->weaponstate=WEAPON_RAISING;ps->weaponTime=CW_StowTime(def,1);
        ps->weaponDelay=0;CW_StoreSpread(ps,25500);
        PM_StartWeaponAnim(WEAP_RAISE);return qtrue;
    }
    if(ps->weaponTime>0 || ps->weaponDelay>0)return qtrue;
    if(ps->weaponstate==WEAPON_DROPPING) {
        if(attached) {
            *bits|=CW_LADDER_HIDDEN;
            /* Native finish-drop switches to weapon 0 / RAISING for one
             * step; its raise duration is zero, then it becomes READY. */
            ps->weaponstate=WEAPON_RAISING;
        } else {
            ps->weaponstate=WEAPON_READY;*bits&=~CW_LADDER_WEAPON;
            PM_StartWeaponAnim(WEAP_IDLE1);
        }
        return qtrue;
    }
    if(ps->weaponstate==WEAPON_RAISING) {
        ps->weaponstate=WEAPON_READY;
        if(!hidden)*bits&=~CW_LADDER_WEAPON;
        PM_StartWeaponAnim(WEAP_IDLE1);
    }
    return qtrue;
}
