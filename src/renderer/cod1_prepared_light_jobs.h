/* Immutable inputs for vertex lighting jobs. Cache ownership, visibility
 * queries, counters and publication remain on the submitting thread. */
#ifndef COD1_PREPARED_LIGHT_JOBS_H
#define COD1_PREPARED_LIGHT_JOBS_H
#include <stddef.h>
#include "cod1_model_light_shade.h"

typedef struct {
    const c1mModelLightFrame *frame;
    const float *sine;
    int fullbright, roundMode;
    unsigned char identity;
} c1PreparedLightInput;

static int C1PreparedLight_Shade(const c1PreparedLightInput *input,
        const float *xyz, const float *normal, unsigned char *rgba) {
    const c1mModelLightFrame *frame = input->frame;
    if (input->fullbright) {
        rgba[0] = rgba[1] = rgba[2] = input->identity;
        rgba[3] = 255;
        return 1;
    }
    return C1M_Shade(&frame->world, frame->prepared.remainingSky,
        frame->prepared.lights, frame->prepared.weights, frame->prepared.count,
        xyz, normal, input->roundMode, input->sine, rgba);
}

/* The caller supplies aligned float fields from its immutable vertex array.
 * Alpha is copied from the source only after RGB succeeds, as in the scalar
 * cache builder. Each range owns a disjoint segment of the unpublished work
 * buffer; a failed batch must be replayed before publishing that buffer. */
typedef struct {
    c1PreparedLightInput light;
    const unsigned char *vertices;
    size_t stride, xyzOffset, normalOffset, alphaOffset;
    unsigned char *rgba;
} c1PreparedLightJob;

static int C1PreparedLight_Range(void *context, unsigned first, unsigned end) {
    const c1PreparedLightJob *job = (const c1PreparedLightJob *)context;
    unsigned i;
    for (i = first; i < end; ++i) {
        const unsigned char *vertex = job->vertices + (size_t)i * job->stride;
        unsigned char *color = job->rgba + (size_t)i * 4;
        if (!C1PreparedLight_Shade(&job->light,
                (const float *)(vertex + job->xyzOffset),
                (const float *)(vertex + job->normalOffset), color)) return 0;
        color[3] = vertex[job->alphaOffset];
    }
    return 1;
}
#endif
