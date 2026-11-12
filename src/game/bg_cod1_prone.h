/* PATCH49: CoD1 prone input arbitration (cgame 0x300076e4..0x300077b9,
 * 0x3000795b..0x30007970 and 0x3000ae30). Included by bg_pmove.c only.
 * The caller supplies the previous RAW usercmd, as game/cgame already do. */
static int PM_Cod1WeaponInputActive(void) {
    return BG_Cod1MovementActive(pm) && CW_Active(pm->ps) &&
        !(pm->ps->stats[STAT_COD1_EDGES]&CW_LADDER_WEAPON) &&
        pm->ps->stats[STAT_HEALTH]>0 && !(pm->ps->pm_flags&PMF_RESPAWNED) &&
        !(pm->ps->eFlags&EF_MG42_ACTIVE) && !pm->ps->persistant[PERS_HWEAPON_USE];
}
static void PM_Cod1ProneInput(void) {
    playerState_t *ps=pm->ps;
    int *bits=&ps->stats[STAT_COD1_EDGES];
    int moved=abs(pm->cmd.forwardmove)>abs(pm->oldcmd.forwardmove) ||
        abs(pm->cmd.rightmove)>abs(pm->oldcmd.rightmove);
    if(!PM_Cod1WeaponInputActive() || !(ps->pm_flags&PMF_COD1_PRONE)) {
        *bits&=~CW_PRONE_LOCK;return;
    }
    if(moved) {
        /* Retail 0x3000b030: firing and melee relaxation cannot be interrupted
         * by a new movement command. Only currently supported rifle states
         * are mapped here; RTCW numeric weapon states differ from CoD1. */
        if(ps->weaponstate!=WEAPON_FIRING && ps->weaponstate!=WEAPON_COD1_MELEE)
            *bits&=~(CW_PRONE_LOCK|CW_ADS_REQUESTED);
    } else if(!(*bits&CW_ADS_REQUESTED) &&
        (ps->weaponstate==WEAPON_READY || ps->weaponstate==WEAPON_RAISING ||
         ps->weaponstate==WEAPON_DROPPING || ps->weaponstate==WEAPON_RELOADING ||
         ps->weaponstate==WEAPON_COD1_SWITCH || ps->weaponstate==WEAPON_COD1_RECHAMBER)) *bits&=~CW_PRONE_LOCK;
    if(*bits&(CW_ADS_REQUESTED|CW_PRONE_LOCK))
        pm->cmd.forwardmove=pm->cmd.rightmove=0;
}
static void PM_Cod1ADSRequest(int wasProne) {
    playerState_t *ps=pm->ps;
    const cwDef_t *d=CW_PSDef(ps);
    int *bits=&ps->stats[STAT_COD1_EDGES];
    int prone=!!(ps->pm_flags&PMF_COD1_PRONE);
    int moving=pm->cmd.forwardmove || pm->cmd.rightmove;
    int modeEdge=(pm->cmd.buttons&BUTTON_GESTURE) && !(*bits&CW_EDGE_MODE);
    /* Native stance-entry suffix 0x300059e8: only a NEW prone stance clears
     * the old request/lock when a movement command survives the earlier gate. */
    if(prone && !wasProne && moving) *bits&=~(CW_PRONE_LOCK|CW_ADS_REQUESTED);
    if(PM_Cod1WeaponInputActive() && CW_Number(d,"adsFire") && ps->weaponstate==WEAPON_COD1_LAUNCH) {
        *bits|=CW_ADS_REQUESTED;return;
    }
    if(!PM_Cod1WeaponInputActive() || !CW_Number(d,"aimDownSight") || !(pm->cmd.wbuttons&WBUTTON_ATTACK2) ||
       pm->cmd.weapon!=ps->weapon || (modeEdge && CW_Get(d?d->altIndex:-1)) ||
       (ps->weaponstate!=WEAPON_READY && ps->weaponstate!=WEAPON_FIRING &&
        ps->weaponstate!=WEAPON_RELOADING && ps->weaponstate!=WEAPON_COD1_RECHAMBER) ||
       (prone && (!pml.groundPlane || !CW_Number(d,"aimDownSight")))) {
        *bits&=~CW_ADS_REQUESTED;return;
    }
    /* Held ADS with surviving crawl input preserves the previously resolved
     * request. A fresh ADS edge can stop an already-held movement command. */
    if(prone && (pm->oldcmd.wbuttons&WBUTTON_ATTACK2) && moving)return;
    *bits|=CW_ADS_REQUESTED;
    if(prone)*bits|=CW_PRONE_LOCK;
}
