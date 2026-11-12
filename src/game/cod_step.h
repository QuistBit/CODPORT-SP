/* Independent CoD1 SP step/slide reconstruction. Research adapter, not a patch.
 * No PE addresses, DLL calls, or engine-global state. See STEP_PORT_RU.md.
 */
#ifndef COD_STEP_H
#define COD_STEP_H
#include <stdint.h>

typedef struct {
    float fraction, end[3], normal[3];
    uint32_t surface_flags, contents;
    int entity, allsolid, startsolid;
} cod_trace;

typedef struct {
    float origin[3], velocity[3], mins[3], maxs[3], ground_normal[3];
    float frame_time, impact_speed, jump_apex;
    int gravity, timer, ground_plane, ground_entity, pm_type, client;
    uint32_t flags, trace_mask, ground_surface_flags;
    int bob_cycle, water_level, touch_count, touches[32];
    int event_sequence, events[4], event_params[4];
    float prone_yaw, prone_height, prone_torso_pitch, prone_waist_pitch;
} cod_move;

typedef void (*cod_trace_fn)(void *context, cod_trace *result,
    const float start[3], const float mins[3], const float maxs[3],
    const float end[3], int client, uint32_t mask);

/* slide: returns whether movement was clipped. Supports capsule stances.
 * step: returns 0 after movement, including the original prone slope sampling.
 * These routines do not perform stance transitions or the outer Pmove loop.
 */
int cod_slide(cod_move *, int gravity, cod_trace_fn, void *);
int cod_step(cod_move *, int gravity, cod_trace_fn, void *);
#endif
