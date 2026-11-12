/* SPDX-License-Identifier: GPL-3.0-or-later
 * Shared by qagame and cgame. Do not call a static-world-only backend here:
 * pm->trace also checks movers, players, and other solid entities.
 */
#include "cod_step.h"
#include "bg_cod1_stance.h"

static int PM_ToCodEntity(int n) {
    return n == ENTITYNUM_WORLD ? 2046 : n == ENTITYNUM_NONE ? 2047 : n;
}
static int PM_FromCodEntity(int n) {
    return n == 2046 ? ENTITYNUM_WORLD : n == 2047 ? ENTITYNUM_NONE : n;
}
static void PM_Cod1Trace(void *context, cod_trace *out, const float *start,
    const float *mins, const float *maxs, const float *end, int client, unsigned mask) {
    trace_t tr;
    pmove_t *move = (pmove_t *)context;
    move->trace(&tr, start, mins, maxs, end, client, (int)mask);
    out->fraction = tr.fraction;
    VectorCopy(tr.endpos, out->end);
    VectorCopy(tr.plane.normal, out->normal);
    out->surface_flags = tr.surfaceFlags;
    /* The core's 0xc000 startsolid retry denotes CoD player/corpse classes,
     * whereas RTCW uses these bits for movers/areaportals. Never confuse them.
     * Entity retry stays disabled until its content classes are ported. */
    out->contents = (unsigned)tr.contents & ~0xc000u;
    out->allsolid = tr.allsolid;
    out->startsolid = tr.startsolid;
    out->entity = PM_ToCodEntity(tr.entityNum);
}
static void PM_Cod1NativeStep(qboolean gravity) {
    cod_move m;
    int i, n, first;
    static int announced;
    float feet = pm->mins[2];
    if (!announced) {
        announced = 1;
#ifdef GAMEDLL
        Com_Printf("PATCH55 CoD1 ground/air/ladder Step/Slide and jump state active (server)\n");
#else
        Com_Printf("PATCH55 CoD1 ground/air/ladder Step/Slide and jump state active (client prediction)\n");
#endif
    }
    memset(&m, 0, sizeof(m));
    VectorCopy(pm->ps->origin, m.origin);
    m.origin[2] += feet;
    VectorCopy(pm->ps->velocity, m.velocity);
    VectorCopy(pm->mins, m.mins);
    VectorCopy(pm->maxs, m.maxs);
    m.mins[2] = 0;
    m.maxs[2] -= feet;
    VectorCopy(pml.groundTrace.plane.normal, m.ground_normal);
    m.frame_time = pml.frametime;
    m.impact_speed = pml.impactSpeed;
    m.jump_apex = pm->ps->cod1JumpApex;
    m.gravity = pm->ps->gravity;
    m.timer = pm->ps->pm_time;
    m.ground_plane = pml.groundPlane;
    m.ground_entity = PM_ToCodEntity(pm->ps->groundEntityNum);
    m.client = pm->ps->clientNum;
    m.flags = (pm->ps->pm_flags & PMF_COD1_PRONE) ? 1 :
        (pm->ps->pm_flags & PMF_DUCKED) ? 128 : 0;
    /* PATCH55: native Step/Slide has an explicit ladder path. */
    if(pm->ps->pm_flags & PMF_LADDER)m.flags |= 16;
    m.trace_mask = (unsigned)pm->tracemask;
    m.ground_surface_flags = SURF_NOSTEPS;
    m.bob_cycle = pm->ps->bobCycle;
    m.water_level = pm->waterlevel;
    m.prone_yaw = pm->ps->viewangles[YAW];
    cod_step(&m, gravity, PM_Cod1Trace, pm);
    VectorCopy(m.origin, pm->ps->origin);
    pm->ps->origin[2] -= feet;
    VectorCopy(m.velocity, pm->ps->velocity);
    pml.impactSpeed = m.impact_speed;
    pm->ps->bobCycle = m.bob_cycle;
    for (i=0;i<m.touch_count;i++) PM_AddTouchEnt(PM_FromCodEntity(m.touches[i]));
    /* Preserve the native signed amount: negative steps need camera smoothing
     * as well. The appended event retains all existing protocol event values.
     * Footsteps stay with the existing outer Pmove. */
    first = m.event_sequence > 4 ? m.event_sequence-4 : 0;
    for (i=first;i<m.event_sequence;i++) {
        if (m.events[i&3] != 0x8f) continue;
        n = m.event_params[i&3]-128;
        if (n) BG_AddPredictableEventToPlayerstate(EV_COD1_STEP,
            m.event_params[i&3], pm->ps);
    }
}
