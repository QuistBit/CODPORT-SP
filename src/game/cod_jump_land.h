/* SPDX-License-Identifier: GPL-3.0-or-later
 * Independent numerical CoD1 SP reconstruction. Not an RTCW protocol struct.
 * Flags, event IDs, material bits and feet coordinates use the CoD1 domain.
 * The engine adapter must translate them; never memcpy this over playerState.
 */
#ifndef COD_JUMP_LAND_H
#define COD_JUMP_LAND_H
#include <stdint.h>

typedef struct {
    float origin[3], velocity[3], forward[3], ladder_normal[3];
    float previous_origin[3], previous_velocity[3];
    float jump_apex, aim_spread, fall_min, fall_max;
    int gravity, command_time, jump_time, upmove;
    int ground_plane, walking, ground_entity, timer, pm_type, water_level;
    uint32_t flags, surface_flags;
    int view_target, view_transition_time, view_transition_down;
    int prone_view, crouch_view;
    int event_sequence, events[4], event_params[4];
} cod_jump_land;

/* 1 = accepted; 0 = rejected; -1 = unsupported nonpositive gravity.
 * The caller records jump_time after its airborne move, as the original does.
 * This routine does not implement input-driven stance transitions.
 */
int cod_check_jump(cod_jump_land *state);

/* 1 = landing evaluated; 0 = submerged/no real solution; -1 = unsupported
 * nonpositive gravity. Reports the equivalent fall height when evaluated.
 * Does not apply health damage itself: the event consumer owns that action.
 */
int cod_crash_land(cod_jump_land *state, float *fall_height);
#endif
