/* CoD1 script-only shake. Stored values follow the recovered four-slot
 * consumer; this does not change the legacy weapon/rumble queue. */
#ifndef CG_COD1_SCRIPT_QUAKE_H
#define CG_COD1_SCRIPT_QUAKE_H
#include <stdint.h>
#include <limits.h>
#include <float.h>
#include <math.h>
#include <string.h>
#include "../game/bg_cod1_script_quake.h"

#define CQS_SLOT_COUNT 4
typedef struct {
    int start_ms;
    float scale, duration_ms, radius, origin[3], score, factor;
} cod1ScriptQuakeSlot_t;
typedef struct {
    cod1ScriptQuakeSlot_t slots[CQS_SLOT_COUNT];
    float phase;
} cod1ScriptQuakeQueue_t;

static int CQS_Finite(float value) {
    return value == value && value >= -FLT_MAX && value <= FLT_MAX;
}

/* A return of zero is an inactive slot; -1 is an unsupported arithmetic
 * sample. Inactive samples deliberately retain the old cached values. */
static int CQS_Evaluate(cod1ScriptQuakeSlot_t *slot, int now,
                        const float view[3]) {
    int axis;
    int64_t age;
    double sum, factor_wide;
    float delta[3], distance, age_float, distance_factor, factor, score;
    age = (int64_t)now - slot->start_ms;
    if (age < 0 || (double)age >= slot->duration_ms) return 0;
    if (!(slot->duration_ms > 0.0f) || !(slot->radius > 0.0f) ||
        !(slot->scale > 0.0f)) return -1;
    for (axis = 0; axis < 3; ++axis) {
        if (!CQS_Finite(view[axis]) || !CQS_Finite(slot->origin[axis])) return -1;
        delta[axis] = (float)((double)view[axis] - slot->origin[axis]);
        if (!CQS_Finite(delta[axis])) return -1;
    }
    /* The original distance helper stores each delta and its sqrt result as
     * float32. The squared sum itself remains in extended intermediates. */
    sum = (double)delta[2] * delta[2] + (double)delta[1] * delta[1];
    sum += (double)delta[0] * delta[0];
    distance = (float)sqrt(sum);
    if (!CQS_Finite(distance)) return -1;
    distance_factor = (float)(1.0 - (double)distance / slot->radius);
    age_float = (float)age;
    factor_wide = (1.0 - (double)age_float / slot->duration_ms) * slot->scale;
    factor = (float)factor_wide;
    if (!CQS_Finite(distance_factor) || !CQS_Finite(factor) || factor_wide <= 0.0)
        return -1;
    /* Native score is computed before the factor is stored/rounded. */
    score = distance_factor >= 0.0f
        ? (float)((double)distance_factor * factor_wide)
        : (float)((double)distance_factor / factor_wide);
    if (!CQS_Finite(score)) return -1;
    slot->score = score;
    slot->factor = factor;
    return 1;
}

/* Accepted commands can be native no-ops or weaker than the four cached
 * scores. A zero return is a port admission failure, with no queue mutation. */
static int CQS_Start(cod1ScriptQuakeQueue_t *queue,
                     const cod1ScriptQuake_t *event, int now,
                     const float view[3]) {
    cod1ScriptQuakeSlot_t incoming;
    int slot, weakest;
    if (!CQ_Validate(event)) return 0;
    if (!CQ_IsActive(event)) return 1;
    if (now < 0 || (int64_t)now + event->duration_ms > INT_MAX) return 0;
    memset(&incoming, 0, sizeof(incoming));
    incoming.start_ms = now;
    incoming.scale = event->scale;
    incoming.duration_ms = (float)event->duration_ms;
    incoming.radius = event->radius;
    memcpy(incoming.origin, event->origin, sizeof(incoming.origin));
    if (CQS_Evaluate(&incoming, now, view) != 1) return 0;
    for (slot = 0; slot < CQS_SLOT_COUNT; ++slot) {
        if (queue->slots[slot].start_ms > now ||
            (double)queue->slots[slot].start_ms + queue->slots[slot].duration_ms <= now)
            break;
    }
    if (slot == CQS_SLOT_COUNT) {
        weakest = 0;
        for (slot = 1; slot < CQS_SLOT_COUNT; ++slot)
            if (queue->slots[slot].score < queue->slots[weakest].score) weakest = slot;
        if (incoming.score <= queue->slots[weakest].score) return 1;
        slot = weakest;
    }
    queue->slots[slot] = incoming;
    return 1;
}

/* The rand value is a declared bounded service, not a recovered PRNG state.
 * Double intermediates model the original x87 operations around the stored
 * float32 clock/constants; sampled angle comparison uses numeric tolerance. */
static int CQS_Frame(cod1ScriptQuakeQueue_t *queue, int now,
                      const float view[3], unsigned rand15, float angles[3]) {
    static const float frequency[3] = {
        25.132741928100586f, 47.1238899230957f, 37.69911193847656f
    };
    static const float amplitude[3] = {18.0f, 16.0f, 10.0f};
    float selected = 0.0f, factor = 0.0f, clock;
    int slot, axis;
    for (slot = 0; slot < CQS_SLOT_COUNT; ++slot) {
        if (CQS_Evaluate(&queue->slots[slot], now, view) == 1 &&
            queue->slots[slot].score > selected) {
            selected = queue->slots[slot].score;
            factor = queue->slots[slot].factor;
        }
    }
    if (selected <= 0.0f) {
        queue->phase = (float)(((double)(rand15 & 32767u) *
                      0.000030517578125 * 2.0 - 1.0) * 3.1415927410125732f);
        return 1;
    }
    if (selected > 1.0f) selected = 1.0f;
    clock = (float)((double)now * 0.0016666667070239782f);
    for (axis = 0; axis < 3; ++axis) {
        double value = (double)angles[axis] +
            sin((double)clock * frequency[axis] + queue->phase) *
            selected * factor * amplitude[axis];
        if (value >= -FLT_MAX && value <= FLT_MAX) angles[axis] = (float)value;
    }
    return 0;
}
#endif
