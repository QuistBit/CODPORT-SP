/* Private, disjoint vertex output for one static-model surface. The caller
 * has already allocated it and prepared the model's lighting. This header
 * uses the renderer's existing xm14Vertex/drawVert_t and VectorNormalize;
 * it performs no visibility query, allocation, logging or GL operation. */
#ifndef COD1_STATIC_BAKE_JOBS_H
#define COD1_STATIC_BAKE_JOBS_H
#include "cod1_prepared_light_jobs.h"

typedef struct {
    const xm14Vertex *source;
    drawVert_t *vertices;
    float origin[3], scale[3], precalc[3], axis[3][3];
    c1PreparedLightInput light;
    int debugWhite;
} c1StaticBakeJob;

static int C1StaticBake_Range(void *context, unsigned first, unsigned end) {
    const c1StaticBakeJob *job = (const c1StaticBakeJob *)context;
    unsigned j;
    int k, q;
    for (j = first; j < end; ++j) {
        const xm14Vertex *source = job->source + j;
        drawVert_t *v = job->vertices + j;
        memset(v, 0, sizeof(*v));
        for (k = 0; k < 3; ++k) {
            v->xyz[k] = job->origin[k];
            v->normal[k] = 0;
            for (q = 0; q < 3; ++q) {
                v->xyz[k] += source->xyz[q] * job->scale[q] * job->axis[q][k];
                v->normal[k] += source->normal[q] / job->scale[q] * job->axis[q][k];
            }
            v->color[k] = (unsigned char)(255 * (job->precalc[k] > 1 ? 1 : job->precalc[k]));
        }
        VectorNormalize(v->normal);
        v->color[3] = 255;
        if (!C1PreparedLight_Shade(&job->light, v->xyz, v->normal, v->color)) return 0;
        if (job->debugWhite) for (k = 0; k < 3; ++k) v->color[k] = 255;
        v->st[0] = source->uv[0];
        v->st[1] = source->uv[1];
    }
    return 1;
}
#endif
