/* Shared Pmove state for the original prone view cone. Camera interpolation
 * reads this state through PM_UpdateViewAngles; only a movement command may
 * initialise or advance its terrain-relative centre.
 */
#include "bg_cod1_prone_pitch_math.h"

static int BG_Cod1PronePitchActive(const playerState_t *ps, const usercmd_t *cmd) {
    return (cmd->wbuttons & WBUTTON_COD1_STANCE) && !ps->aiChar &&
        ps->pm_type == PM_NORMAL && ps->stats[STAT_HEALTH] > 0 &&
        (ps->pm_flags & PMF_COD1_PRONE) && !(ps->eFlags & EF_MG42_ACTIVE) &&
        !ps->persistant[PERS_HWEAPON_USE];
}

static void BG_Cod1PronePitchReset(playerState_t *ps) {
    ps->cod1PronePitchCentre = 0;
    ps->cod1PronePitchValid = 0;
}

static void PM_Cod1PronePitchInitialise(int wasProne) {
    playerState_t *ps = pm->ps;
    float groundPitch = 0;
    vec3_t end;
    trace_t tr;
    if (!BG_Cod1PronePitchActive(ps, &pm->cmd)) {
        BG_Cod1PronePitchReset(ps);
        return;
    }
    if (!wasProne || !ps->cod1PronePitchValid) {
        /* Native stance entry probes support itself before PM_GroundTrace.
         * An upward-moving player can touch a slope here while the subsequent
         * ground-plane classification is airborne; those inputs differ. */
        VectorCopy(ps->origin, end);
        end[2] -= 0.25f;
        pm->trace(&tr, ps->origin, pm->mins, pm->maxs, end,
            ps->clientNum, pm->tracemask);
        if (!tr.startsolid && tr.fraction < 1)
            groundPitch = COD1_PP_GroundPitch(ps->viewangles[YAW], tr.plane.normal);
        ps->cod1PronePitchCentre = COD1_PP_Initialise(ps->viewangles[PITCH], groundPitch);
        ps->cod1PronePitchValid = 1;
    }
}

static void PM_Cod1PronePitchAdvance(void) {
    playerState_t *ps = pm->ps;
    float groundPitch;
    if (!BG_Cod1PronePitchActive(ps, &pm->cmd)) {
        BG_Cod1PronePitchReset(ps);
        return;
    }
    if (!ps->cod1PronePitchValid) return;
    groundPitch = pml.groundPlane ? COD1_PP_GroundPitch(ps->viewangles[YAW],
        pml.groundTrace.plane.normal) : 0;
    ps->cod1PronePitchCentre = COD1_PP_Tick(ps->cod1PronePitchCentre,
        groundPitch, pml.frametime);
}

static void BG_Cod1PronePitchClamp(playerState_t *ps, const usercmd_t *cmd) {
    if (BG_Cod1PronePitchActive(ps, cmd) && ps->cod1PronePitchValid)
        COD1_PP_Clamp(ps->cod1PronePitchCentre, &ps->viewangles[PITCH],
            &ps->delta_angles[PITCH]);
}
