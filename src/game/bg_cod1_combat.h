/* PATCH35: shared/predicted CoD weapon state; existing networked stats only. */
#ifndef BG_COD1_COMBAT_H
#define BG_COD1_COMBAT_H
#include "bg_cod1_weapondef.h"
#include "bg_cod1_stance.h"
#define CW_EDGE_ATTACK 1
#define CW_EDGE_MODE 2
#define CW_ADS_RISING 4
/* PATCH48: retain input intent while reload temporarily lowers the sights.
 * RISING remains the direction used by the last simulation step/remainder. */
#define CW_ADS_REQUESTED 8
#define CW_PRONE_LOCK 16
/* HANDHELD_FINAL_R1: CoD1 command bit 0x20 is melee. STAT_COD1_EDGES is
 * separate storage, so use free high bits rather than colliding with the
 * existing lean-active bit (32). */
#define CW_EDGE_MELEE 512
#define CW_MELEE_HIT 1024
/* RTCW already transports WEAPON_WAITING in the 4-bit weaponstate field and
 * this port has no live donor use for it. Reuse it for the native CoD melee
 * lifetime instead of widening the network protocol. */
#define WEAPON_COD1_MELEE WEAPON_WAITING
/* PATCH52: ladder stow sequence and effective native weapon 0. */
#define CW_LADDER_WEAPON 64
#define CW_LADDER_HIDDEN 128
static int CW_StowTime(const cwDef_t *d,int raising) {
    int n=(int)CW_Number(d,raising?"raiseTime":"dropTime");
    return n<0?0:(n>30000?30000:n);
}
#include "bg_cod1_inventory.h"
static float CW_ADS(const playerState_t *ps) {
    int v=ps->stats[STAT_COD1_ADS];return v<=0?0:(v>=1000?1:v*.001f);
}
/* Segmented reload keeps ADS lowered through the start/loop/end sequence.
 * Simulation calls this BEFORE decrementing weaponTime. Rendering calls it
 * on the resulting state, as retail cgame does when choosing ADS leaf goals. */
static int CW_ADSTarget(const playerState_t *ps,const cwDef_t *d) {
    if(!CW_Number(d,"aimDownSight"))return 0;
    if(CW_Number(d,"adsFire") && ps->weaponstate==WEAPON_COD1_LAUNCH)return 1;
    /* Original melee states 10/11 are outside the ADS-capable state set. */
    if(ps->weaponstate==WEAPON_COD1_MELEE)return 0;
    if((ps->stats[STAT_COD1_EDGES]&CW_LADDER_WEAPON) || !(ps->stats[STAT_COD1_EDGES]&CW_ADS_REQUESTED))return 0;
    if(ps->weaponstate!=WEAPON_RELOADING)return 1;
    return !CW_Number(d,"segmentedReload") &&
        ps->weaponTime<=(int)CW_Number(d,"adsReloadTransTime");
}
static int CW_SwitchTime(const cwDef_t *d) {
    int n=(int)(CW_Number(d,"altDropTime")+CW_Number(d,"altRaiseTime"));
    return n<1?1:(n>30000?30000:n);
}
static void CW_UpdateADS(playerState_t *ps,const cwDef_t *d,int wanted,int msec) {
    int dir=!!(ps->stats[STAT_COD1_EDGES]&CW_ADS_RISING),duration,n,step;
    if(ps->stats[STAT_COD1_EDGES]&CW_LADDER_HIDDEN) {
        ps->stats[STAT_COD1_ADS]=ps->stats[STAT_COD1_ADS_REMAINDER]=0;
        ps->stats[STAT_COD1_EDGES]&=~CW_ADS_RISING;return;
    }
    if(dir!=wanted)ps->stats[STAT_COD1_ADS_REMAINDER]=0;
    ps->stats[STAT_COD1_EDGES]=(ps->stats[STAT_COD1_EDGES]&~CW_ADS_RISING)|(wanted?CW_ADS_RISING:0);
    duration=(int)CW_Number(d,wanted?"adsTransInTime":"adsTransOutTime");
    if(duration<1)duration=1;if(duration>30000)duration=30000;
    n=msec*1000+ps->stats[STAT_COD1_ADS_REMAINDER];step=n/duration;
    ps->stats[STAT_COD1_ADS_REMAINDER]=n%duration;
    n=ps->stats[STAT_COD1_ADS]+(wanted?step:-step);
    if(n<=0){n=0;ps->stats[STAT_COD1_ADS_REMAINDER]=0;}
    if(n>=1000){n=1000;ps->stats[STAT_COD1_ADS_REMAINDER]=0;}
    ps->stats[STAT_COD1_ADS]=n;
}
static void CW_StoreSpread(playerState_t *ps,float value) {
    if(value<0)value=0;if(value>25500)value=25500;
    ps->stats[STAT_COD1_SPREAD]=(int)(value+.5f);
    ps->aimSpreadScaleFloat=ps->stats[STAT_COD1_SPREAD]*.01f;
    ps->aimSpreadScale=ps->stats[STAT_COD1_SPREAD]/100;
}
static void CW_UpdateSpread(playerState_t *ps,const cwDef_t *d,const usercmd_t *cmd,int msec) {
    float decay=(float)CW_Number(d,"hipSpreadDecayRate"),add=0;
    if(ps->groundEntityNum==ENTITYNUM_NONE)decay*=.5f;
    else if(ps->pm_flags&PMF_COD1_PRONE)decay*=(float)CW_Number(d,"hipSpreadProneDecay");
    else if(ps->pm_flags&PMF_DUCKED)decay*=(float)CW_Number(d,"hipSpreadDuckedDecay");
    if(CW_ADS(ps)<1) {
        /* Movement threshold adapter: avoid adding bloom from trace jitter. */
        if((cmd->forwardmove||cmd->rightmove) && ps->velocity[0]*ps->velocity[0]+ps->velocity[1]*ps->velocity[1]>121)
            add=(float)CW_Number(d,"hipSpreadMoveAdd");
        if(ps->groundEntityNum==ENTITYNUM_NONE)add+=2.56f;
    }
    CW_StoreSpread(ps,ps->stats[STAT_COD1_SPREAD]+(add-decay)*msec*25.5f);
}
static float CW_SpreadDegrees(const playerState_t *ps) {
    const cwDef_t *d=CW_PSDef(ps);float lo,hi,hip,a=CW_ADS(ps);
    const char *key=(ps->pm_flags&PMF_COD1_PRONE)?"hipSpreadProneMin":((ps->pm_flags&PMF_DUCKED)?"hipSpreadDuckedMin":"hipSpreadStandMin");
    lo=(float)CW_Number(d,key);hi=(float)CW_Number(d,"hipSpreadMax");
    /* SP gamex86 20049697: ADS minimum applies only at full aim.
     * Existing movement bloom still decays after entering the sights. */
    if(a>=1)lo=(float)CW_Number(d,"adsSpread");
    hip=lo+(hi-lo)*(ps->stats[STAT_COD1_SPREAD]/25500.0f);
    return hip;
}
/* Original camera impulse values are angular velocities, not Euler offsets. */
static float CW_KickRange(const cwDef_t *d,const char *lo,const char *hi,float unit) {
    float a=(float)CW_Number(d,lo);return a+((float)CW_Number(d,hi)-a)*unit;
}
static void CW_GunSpring(float *offset,float *speed,float dt,float maxAngle,float accel,float maxSpeed,float decay,float drag) {
    float v;
    if(fabs(*offset)<.25f && fabs(*speed)<1){*offset=*speed=0;return;}
    *offset+=*speed*dt;
    if(*offset>maxAngle){*offset=maxAngle;if(*speed>0)*speed=0;}
    if(*offset< -maxAngle){*offset=-maxAngle;if(*speed<0)*speed=0;}
    v=*speed+(*offset>0?-accel:(*offset<0?accel:0))*dt;
    v-=(dt*v)*decay;
    if(v>0){v-=drag*dt;if(v<0)v=0;}else{v+=drag*dt;if(v>0)v=0;}
    if(v>maxSpeed)v=maxSpeed;if(v< -maxSpeed)v=-maxSpeed;*speed=v;
}
#endif
