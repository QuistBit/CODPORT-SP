/* PATCH64: additional handheld states. Included only by bg_pmove.c.
 * Timings/amounts come from WeaponDef. Retail reference: cgame 3000b300
 * (bolt), 3000b670/3000bcc0 (segmented reload), 3000c480 (start fire). */
static void PM_Cod1ReloadPhase(int phase) {
    const cwDef_t *d=CW_PSDef(pm->ps);int total,delay;
    if(phase==COD1_RELOAD_START){total=(int)CW_Number(d,"reloadStartTime");delay=(int)CW_Number(d,"reloadStartAddTime");}
    else if(phase==COD1_RELOAD_END){total=(int)CW_Number(d,"reloadEndTime");delay=0;}
    else {total=CW_ReloadDuration(d,0);delay=CW_ReloadAddDelay(d,0);}
    if(total<1)total=1;
    pm->ps->stats[STAT_COD1_RELOAD]=phase;
    pm->ps->weaponstate=WEAPON_RELOADING;pm->ps->weaponTime=total;
    pm->ps->weaponDelay=phase==COD1_RELOAD_END?0:(delay>0 && delay<=total?delay:total);
    PM_StartWeaponAnim(WEAP_RELOAD1);
    BG_AddPredictableEventToPlayerstate(EV_FILL_CLIP,phase,pm->ps);
}
static int PM_Cod1CanReload(const cwDef_t *d) {
    int clip=pm->ps->ammoclip[BG_FindClipForWeapon(pm->ps->weapon)];
    int add=(int)CW_Number(d,"reloadAmmoAdd"),max=(int)CW_Number(d,"clipSize");
    if(CW_Number(d,"clipOnly") || CW_Inert(d) || clip>=max || pm->ps->ammo[BG_PlayerAmmoIndex(pm->ps,pm->ps->weapon)]<=0)return 0;
    if(CW_Number(d,"noPartialReload") && clip>0 && (!add || add>=max || max-clip<add))return 0;
    return 1;
}
static void PM_Cod1Launch(const cwDef_t *d) {
    int clip=BG_FindClipForWeapon(pm->ps->weapon);
    if(pm->ps->ammoclip[clip]<=0){pm->ps->weaponstate=WEAPON_READY;pm->ps->weaponTime=pm->ps->weaponDelay=0;return;}
    pm->ps->ammoclip[clip]--;
    pm->ps->grenadeTimeLeft=CW_Grenade(d)?(int)CW_Number(d,"fuseTime"):0;
    pm->ps->stats[STAT_COD1_SHOT_CONE]=1;
    pm->ps->weaponstate=WEAPON_FIRING;pm->ps->weaponDelay=0;
    pm->ps->weaponTime=(int)CW_Number(d,"fireTime");
    pm->ps->lastFireTime=pm->cmd.serverTime;pm->ps->releasedFire=qfalse;
    PM_StartWeaponAnim(pm->ps->ammoclip[clip]?WEAP_ATTACK1:WEAP_ATTACK_LASTSHOT);
    BG_AddPredictableEventToPlayerstate(
        pm->ps->ammoclip[clip]?EV_FIRE_WEAPON:EV_FIRE_WEAPON_LASTSHOT,
        CWB_WeaponParm(pm->ps),pm->ps);
    if(pm->ps->pm_flags&PMF_COD1_PRONE)pm->ps->stats[STAT_COD1_EDGES]|=CW_PRONE_LOCK;
}
/* Original cgame 0x3000c920..0x3000ca2b: command bit 0x20 starts
 * weapon melee, meleeDelay selects the impact instant and meleeTime owns the
 * full recovery. Retail uses two weapon states (10/11); this port carries both
 * phases in WEAPON_COD1_MELEE and marks the impact phase in STAT_COD1_EDGES. */
static int PM_Cod1Melee(const cwDef_t *d,int meleeEdge) {
    playerState_t *ps=pm->ps;int total,delay,index;
    if(ps->weaponstate==WEAPON_COD1_MELEE) {
        if(ps->weaponDelay>0) {
            ps->weaponDelay-=pml.msec;
            if(ps->weaponDelay<0)ps->weaponDelay=0;
        }
        ps->weaponTime-=pml.msec;
        if(!(ps->stats[STAT_COD1_EDGES]&CW_MELEE_HIT) && ps->weaponDelay<=0) {
            index=CW_Find(d->name);
            ps->stats[STAT_COD1_EDGES]|=CW_MELEE_HIT;
            BG_AddPredictableEventToPlayerstate(EV_COD1_MELEE_HIT,index>=0?index+1:0,ps);
        }
        if(ps->weaponTime<=0) {
            ps->weaponTime=ps->weaponDelay=0;ps->weaponstate=WEAPON_READY;
            ps->stats[STAT_COD1_EDGES]&=~CW_MELEE_HIT;
            PM_StartWeaponAnim(WEAP_IDLE1);
        }
        return 1;
    }
    if(!meleeEdge || CW_Number(d,"meleeDamage")<=0 || pm->cmd.weapon!=ps->weapon)return 0;
    /* Keep the first integration deliberately narrow: the native branch can
     * interrupt several states, but READY/FIRING are the verified safe states
     * in the current port. Reload/launch/ladder transitions remain atomic. */
    if(ps->weaponstate!=WEAPON_READY && ps->weaponstate!=WEAPON_FIRING)return 0;
    total=(int)CW_Number(d,"meleeTime");delay=(int)CW_Number(d,"meleeDelay");
    if(total<1)total=1;if(total>30000)total=30000;
    if(delay<0)delay=0;if(delay>total)delay=total;
    ps->weaponstate=WEAPON_COD1_MELEE;ps->weaponTime=total;ps->weaponDelay=delay;
    ps->stats[STAT_COD1_EDGES]&=~(CW_ADS_REQUESTED|CW_MELEE_HIT);
    if(ps->pm_flags&PMF_COD1_PRONE)ps->stats[STAT_COD1_EDGES]|=CW_PRONE_LOCK;
    index=CW_Find(d->name);
    BG_AddPredictableEventToPlayerstate(EV_COD1_MELEE_START,index>=0?index+1:0,ps);
    if(!delay) {
        ps->stats[STAT_COD1_EDGES]|=CW_MELEE_HIT;
        BG_AddPredictableEventToPlayerstate(EV_COD1_MELEE_HIT,index>=0?index+1:0,ps);
    }
    return 1;
}

/* R217: run before both the ladder bridge and queued weapon changes.
 * Keep the emitted shot identity independent of subsequent current selection. */
static int PM_Cod1DiscardSpent(const cwDef_t *d) {
    playerState_t *ps=pm->ps;
    int spent=ps->weapon,clip,ammo,target=WP_NONE,slot,w;
    if((!CW_Grenade(d) && !CW_Projectile(d)) || !CW_Number(d,"clipOnly") ||
       ps->weaponstate!=WEAPON_FIRING || ps->weaponTime>pml.msec)return 0;
    clip=BG_FindClipForWeapon(spent);ammo=BG_PlayerAmmoIndex(ps,spent);
    if(clip<=0 || clip>=MAX_WEAPONS || ammo<=0 || ammo>=MAX_WEAPONS ||
       ps->ammoclip[clip]!=0 || ps->ammo[ammo]!=0)return 0;
    ps->weaponTime=ps->weaponDelay=ps->grenadeTimeLeft=0;ps->weaponstate=WEAPON_READY;
    COM_BitClear(ps->weapons,spent);CW_SyncSlots(ps);
    w=pm->cmd.weapon;
    if(w>WP_NONE && w<WP_NUM_WEAPONS && CW_WeaponDef(w) &&
       COM_BitCheck(ps->weapons,w))target=w;
    for(slot=1;target==WP_NONE && slot<CW_SLOT_COUNT;++slot) {
        w=ps->cod1Slots[slot];
        if(w>WP_NONE && w<WP_NUM_WEAPONS && CW_WeaponDef(w) &&
           COM_BitCheck(ps->weapons,w))target=w;
    }
    for(w=1;target==WP_NONE && w<WP_NUM_WEAPONS;++w)
        if(CW_WeaponDef(w) && COM_BitCheck(ps->weapons,w))target=w;
    ps->weapon=WP_NONE;ps->cod1Rechamber=0;PM_Cod1ResetRaise();
    if(target!=WP_NONE)PM_BeginWeaponChange(WP_NONE,target,qfalse);
    else {ps->weaponstate=WEAPON_READY;PM_StartWeaponAnim(WEAP_IDLE1);}
    return 1;
}

static int PM_Cod1ExtraWeapon(const cwDef_t *d,int attackEdge) {
    playerState_t *ps=pm->ps;int attack=!!(pm->cmd.buttons&BUTTON_ATTACK);
    if(ps->weaponstate==WEAPON_RELOADING && CW_Number(d,"segmentedReload") && attackEdge)
        ps->stats[STAT_COD1_RELOAD]|=COD1_RELOAD_STOP;
    if(ps->weaponstate==WEAPON_COD1_RECHAMBER) {
        if(ps->weaponDelay>0) {
            ps->weaponDelay-=pml.msec;
            if(ps->weaponDelay<=0){ps->weaponDelay=0;CWB_ConsumeBolt(ps);}
        }
        ps->weaponTime-=pml.msec;
        if(pm->cmd.weapon!=ps->weapon){PM_BeginWeaponChange(ps->weapon,pm->cmd.weapon,qfalse);return 1;}
        if(ps->weaponTime<=0){ps->weaponTime=ps->weaponDelay=0;ps->weaponstate=WEAPON_READY;PM_StartWeaponAnim(WEAP_IDLE1);}
        return 1;
    }
    if(!CW_Grenade(d) && !CW_Projectile(d) && !CW_Inert(d))return 0;
    if(PM_Cod1DiscardSpent(d))return 1;
    if(ps->weaponstate==WEAPON_FIRING && ps->weaponTime<=pml.msec) {
        ps->weaponTime=ps->weaponDelay=ps->grenadeTimeLeft=0;
        ps->weaponstate=WEAPON_READY;
        if(pm->cmd.weapon==ps->weapon)return 1;
    }
    if(ps->weaponstate!=WEAPON_COD1_HOLD && ps->weaponstate!=WEAPON_COD1_LAUNCH &&
       (ps->weaponstate!=WEAPON_FIRING || ps->weaponTime<=pml.msec) && pm->cmd.weapon!=ps->weapon) {
        ps->grenadeTimeLeft=0;PM_BeginWeaponChange(ps->weapon,pm->cmd.weapon,qfalse);
    }
    if(ps->weaponstate==WEAPON_DROPPING || ps->weaponstate==WEAPON_RAISING) {
        ps->weaponTime-=pml.msec;
        if(ps->weaponTime<=0) {
            ps->weaponTime=0;
            if(ps->weaponstate==WEAPON_DROPPING)PM_FinishWeaponChange();
            else {ps->weaponstate=WEAPON_READY;PM_StartWeaponAnim(WEAP_IDLE1);}
        }
        return 1;
    }
    if(CW_Inert(d))return 1;
    if(ps->weaponstate==WEAPON_COD1_HOLD) {
        ps->weaponTime-=pml.msec;if(ps->weaponTime<0)ps->weaponTime=0;
        /* All four stock SP grenades have cookOffHold=0: holding the pin
         * animation does not consume the fuse. Release is accepted after it. */
        if(!attack && ps->weaponTime==0)PM_Cod1Launch(d);
        return 1;
    }
    if(ps->weaponstate==WEAPON_COD1_LAUNCH) {
        ps->weaponTime-=pml.msec;
        if(ps->weaponTime<=0)PM_Cod1Launch(d);
        return 1;
    }
    if(ps->weaponstate==WEAPON_FIRING) {
        ps->weaponTime-=pml.msec;
        if(ps->weaponTime<=0){
            ps->weaponTime=0;ps->grenadeTimeLeft=0;ps->weaponstate=WEAPON_READY;
            /* R217 disposal is performed above, before queued weapon changes. */
        }
        return 1;
    }
    if(!attack || ps->weaponstate!=WEAPON_READY || ps->ammoclip[BG_FindClipForWeapon(ps->weapon)]<=0)return 1;
    if(CW_Grenade(d)) {
        ps->weaponstate=WEAPON_COD1_HOLD;ps->weaponTime=(int)CW_Number(d,"holdFireTime");ps->weaponDelay=0;
        BG_AddPredictableEventToPlayerstate(EV_FILL_CLIP,8,ps);
    } else {
        int delay=(int)CW_Number(d,"fireDelay");
        if(CW_Number(d,"adsFire")) {
            int aim=(int)((1-CW_ADS(ps))*CW_Number(d,"adsTransInTime")+.5);
            if(aim>delay)delay=aim;ps->stats[STAT_COD1_EDGES]|=CW_ADS_REQUESTED;
        }
        ps->weaponstate=WEAPON_COD1_LAUNCH;ps->weaponTime=delay;
        if(!delay)PM_Cod1Launch(d);
    }
    return 1;
}
