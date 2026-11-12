/* PATCH40. Typed bridge between RTCW Pmove and the verified CoD1 numeric core.
 * Included by bg_pmove.c after its event helpers. Entity/flag/event namespaces
 * are translated explicitly; the shared core never sees RTCW structs.
 */
#include "cod_jump_land.h"

static void PM_Cod1JumpState(cod_jump_land *s) {
    memset(s,0,sizeof(*s));
    VectorCopy(pm->ps->origin,s->origin);
    s->origin[2]+=pm->mins[2];
    VectorCopy(pm->ps->velocity,s->velocity);
    VectorCopy(pml.forward,s->forward);
    VectorCopy(pml.previous_origin,s->previous_origin);
    s->previous_origin[2]+=pm->mins[2];
    VectorCopy(pml.previous_velocity,s->previous_velocity);
    s->jump_apex=pm->ps->cod1JumpApex;
    if(pm->ps->pm_flags&PMF_LADDER) {
        s->flags|=16;VectorCopy(pm->ps->cod1LadderNormal,s->ladder_normal);
    }
    s->aim_spread=pm->ps->aimSpreadScaleFloat;
    s->gravity=pm->ps->gravity;
    s->command_time=pm->cmd.serverTime;
    s->jump_time=pm->ps->jumpTime;
    s->upmove=pm->cmd.upmove;
    s->ground_plane=pml.groundPlane;
    s->walking=pml.walking;
    s->ground_entity=pm->ps->groundEntityNum==ENTITYNUM_NONE?2047:
        pm->ps->groundEntityNum==ENTITYNUM_WORLD?2046:pm->ps->groundEntityNum;
    s->timer=pm->ps->pm_time;
    s->water_level=pm->waterlevel;
    s->prone_view=11;s->crouch_view=40;
    s->view_target=60;
    if(pm->ps->pm_flags&PMF_COD1_PRONE) {s->flags|=1;s->view_target=11;}
    else if(pm->ps->pm_flags&PMF_DUCKED) {s->flags|=2;s->view_target=40;}
    if(pm->ps->pm_flags&PMF_JUMP_HELD) s->flags|=8;
    if(pm->ps->pm_flags&PMF_RESPAWNED) s->flags|=0x800;
    if(pm->ps->pm_flags&PMF_TIME_LAND) s->flags|=0x2000;
    if(pm->ps->stats[STAT_COD1_EDGES]&COD1_LAND_SLOW) s->flags|=0x100;
    if(pm->ps->pm_flags&PMF_TIME_KNOCKBACK) s->flags|=0x200;
    /* Current engine trace flags have RTCW semantics. The core needs only
     * these shared behavioral bits here; material-specific audio stays with
     * PM_FootstepForSurface. Never feed RTCW material flags as a CoD ordinal. */
    s->surface_flags=(unsigned)pml.groundTrace.surfaceFlags & (SURF_NODAMAGE|SURF_SLICK);
    s->fall_min=256;s->fall_max=480; /* Original SP defaults, not mutable cvars. */
}

static qboolean PM_Cod1CheckJump(void) {
    cod_jump_land s;
    int result;
    PM_Cod1JumpState(&s);
    result=cod_check_jump(&s);
    pm->cmd.upmove=s.upmove;
    if(result<=0) return qfalse;
    /* PATCH58: discard surplus speed only on an accepted ordinary takeoff.
     * Do not change the native jump solver, ladder push-off, slick surfaces,
     * or active knockback. No new landing delay or jump-input latch. */
    if(!(pm->ps->pm_flags&(PMF_LADDER|PMF_TIME_KNOCKBACK)) &&
       !(pml.groundTrace.surfaceFlags&SURF_SLICK) && pm->ps->speed>0)
        BG_Cod1LimitHorizontalSpeed(s.velocity,(float)pm->ps->speed);
    VectorCopy(s.velocity,pm->ps->velocity);
    pm->ps->cod1JumpApex=s.jump_apex;
    pm->ps->aimSpreadScaleFloat=s.aim_spread;
    pm->ps->aimSpreadScale=(int)s.aim_spread;
    pm->ps->pm_flags|=PMF_JUMP_HELD;
    if(!(s.flags&16)){pm->ps->pm_flags&=~PMF_LADDER;pml.ladder=qfalse;}
    pm->ps->groundEntityNum=ENTITYNUM_NONE;
    pml.groundPlane=qfalse;pml.walking=qfalse;
    PM_AddEvent(EV_JUMP);
    if(pm->cmd.forwardmove>=0) {
        BG_AnimScriptEvent(pm->ps,ANIM_ET_JUMP,qfalse,qtrue);
        pm->ps->pm_flags&=~PMF_BACKWARDS_JUMP;
    } else {
        BG_AnimScriptEvent(pm->ps,ANIM_ET_JUMPBK,qfalse,qtrue);
        pm->ps->pm_flags|=PMF_BACKWARDS_JUMP;
    }
    return qtrue;
}

static void PM_Cod1CrashLand(void) {
    cod_jump_land s;
    float height=0;
    int result,parameter;
    PM_Cod1JumpState(&s);
    result=cod_crash_land(&s,&height);
    if(result<=0) return;
    VectorCopy(s.velocity,pm->ps->velocity);
    pm->ps->pm_time=s.timer;
    pm->ps->pm_flags&=~(PMF_TIME_LAND|PMF_TIME_KNOCKBACK);
    if(s.flags&0x2000) pm->ps->pm_flags|=PMF_TIME_LAND;
    if(s.flags&0x200) pm->ps->pm_flags|=PMF_TIME_KNOCKBACK;
    pm->ps->stats[STAT_COD1_EDGES]&=~COD1_LAND_SLOW;
    if(s.flags&0x100) pm->ps->stats[STAT_COD1_EDGES]|=COD1_LAND_SLOW;
    /* The core sees material ordinal zero. Hard-landing/damage events still
     * carry their numeric parameter; quiet contacts need no event. */
    if(s.event_sequence) {
        parameter=s.event_params[0];
        if(s.events[0]==116) BG_AddPredictableEventToPlayerstate(EV_COD1_FALL_DAMAGE,parameter,pm->ps);
        else if(s.events[0]==93) BG_AddPredictableEventToPlayerstate(EV_COD1_LAND,parameter,pm->ps);
    } else if(height>4) {
        BG_AddPredictableEventToPlayerstate(PM_FootstepForSurface(),pml.groundTrace.surfaceFlags,pm->ps);
    }
}
