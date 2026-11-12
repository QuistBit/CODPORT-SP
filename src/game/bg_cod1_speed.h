/* PATCH36: ordinary-ground movement, shared by server and prediction.
 * CoD1 SP cgame 0x30002f10: largest WEIGHTED command magnitude,
 * divided by the UNWEIGHTED command length. Do not independently scale
 * wishdir axes: that changes diagonal direction as well as speed.
 * PATCH43 supplies the pre-movement ADS mode and current weapon scale.
 * Stance-transition interpolation and landing modifiers remain separate. */
#define COD1_RUN_SPEED 190.0f
#define COD1_CROUCH_SCALE 0.65f
#define COD1_ADS_MOVE_SCALE 0.4f
#define COD1_LEAN_MOVE_SCALE 0.4f

/* PATCH59: native pm_flags 0x100 is landing recovery, distinct from 0x200
 * knockback. Store its marker in an unused bit of the existing replicated
 * STAT_COD1_EDGES slot; pm_time remains the shared movement timer. */
#define COD1_LAND_SLOW 256

/* PATCH54: posture and gait also apply on ladders.
 * Keep the ordinary movement-scale gate separate from player eligibility. */
static qboolean BG_Cod1PlayerMovementActive(const pmove_t *m) {
    return (m->cmd.wbuttons & WBUTTON_COD1_STANCE) &&
        m->ps->pm_type == PM_NORMAL && !m->ps->aiChar && m->waterlevel <= 1;
}
static qboolean BG_Cod1MovementActive(const pmove_t *m) {
    return BG_Cod1PlayerMovementActive(m) && !(m->ps->pm_flags & PMF_LADDER);
}

static float BG_Cod1MoveScale(const playerState_t *ps, const usercmd_t *cmd,
                             qboolean adsMove, float weaponMoveScale, qboolean groundMove) {
    int forward = abs(cmd->forwardmove);
    int right = abs(cmd->rightmove);
    float f = forward * (cmd->forwardmove < 0 ? 0.7f : 1.0f);
    float r = right * 0.8f;
    float largest = f > r ? f : r;
    float length, scale;
    if (!largest) return 0.0f;
    length = sqrt((float)(forward * forward + right * right));
    scale = COD1_RUN_SPEED * largest / (127.0f * length);
    /* Native 0x30002fb3: mode 0x80 selects ps+0x334 (stock 0.4).
     * It is independent of visual ADS fraction and its direction of change. */
    if (adsMove) scale *= COD1_ADS_MOVE_SCALE;
    /* PATCH53: native cgame 0x30002fc5 / game 0x20003815.
     * Any remaining lean uses ps+0x34c (stock 0.4), including return.
     * ADS takes the other branch, so the two penalties never multiply. */
    else if (groundMove && ps->leanf != 0.0f) scale *= COD1_LEAN_MOVE_SCALE;
    if (ps->pm_flags & PMF_COD1_PRONE) scale *= COD1_CRAWL_SCALE;
    else if (ps->pm_flags & PMF_DUCKED) scale *= COD1_CROUCH_SCALE;
    scale *= weaponMoveScale;
    /* Walking is already represented by reduced command axes in CL_KeyMove.
     * upmove carries stance/jump intent, not horizontal speed.
     * No RTCW run, sprint or heavy-weapon multiplier applies here. */
    return scale;
}

/* CoD1 SP cgame 0x30003944: effective settled stance. */
static float BG_Cod1GroundAcceleration(const playerState_t *ps) {
    if (ps->pm_flags & PMF_COD1_PRONE) return 19.0f;
    if (ps->pm_flags & PMF_DUCKED) return 12.0f;
    return 9.0f;
}

/* PATCH57: native air command scale 0x30002e50 / 0x200036a0.
 * Unlike ground, all three command axes participate without direction,
 * stance or weapon weights. Stock non-ADS multiplier is 1.
 */
static float BG_Cod1AirScale(const playerState_t *ps,const usercmd_t *cmd,qboolean adsMove) {
    int f=abs(cmd->forwardmove),r=abs(cmd->rightmove),u=abs(cmd->upmove),largest=f;
    float length,scale;
    if(r>largest)largest=r;if(u>largest)largest=u;
    if(!largest)return 0;
    length=(float)sqrt((float)(f*f+r*r+u*u));
    scale=(float)ps->speed*(float)largest/(127.f*length);
    if(adsMove)scale*=COD1_ADS_MOVE_SCALE;
    return scale;
}

/* PATCH58: requested anti-bunny-hop rule, intentionally stricter than the
 * native CoD1/Q3 projection-based air acceleration. Limit only horizontal
 * player input; gravity and pre-existing airborne impulses stay independent.
 * Shared by server movement and client prediction. */
static float BG_Cod1HorizontalSpeed(const vec3_t velocity) {
    return (float)sqrt(velocity[0]*velocity[0]+velocity[1]*velocity[1]);
}
static void BG_Cod1LimitHorizontalSpeed(vec3_t velocity,float limit) {
    float speed=BG_Cod1HorizontalSpeed(velocity),scale;
    if(speed>limit && speed>0) {
        scale=limit/speed;
        velocity[0]*=scale;
        velocity[1]*=scale;
    }
}
