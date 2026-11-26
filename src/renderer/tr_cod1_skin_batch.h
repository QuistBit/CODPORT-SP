/* Private synchronous skinning batches for drawVert_t snapshots. Resources,
 * poses and span descriptors remain owned by the capture caller until join.
 * Workers touch only their vertex range and call the same pure skin/normal
 * math as the serial capture. Bounds, tags, allocation and publication stay
 * on the caller. A failed batch restores FP state before serial replay. */
#ifndef TR_COD1_SKIN_BATCH_H
#define TR_COD1_SKIN_BATCH_H
#include "cod1_native_source14.h"
#include "../qcommon/cod1_jobs.h"

typedef struct {
    const cnSourceVertex_t *source;
    const cv14Skeleton *pose;
    drawVert_t *vertices;
    unsigned count;
    int white;
} c1DrawSkinSpan;
typedef struct {
    const c1DrawSkinSpan *span;
    unsigned spans;
} c1DrawSkinJob;

static int R_Cod1DrawSkinJob(void *context, unsigned first, unsigned end) {
    const c1DrawSkinJob *job = (const c1DrawSkinJob *)context;
    unsigned s, base = 0, j;
    int k;
    for (s = 0; s < job->spans && base < end; s++) {
        const c1DrawSkinSpan *span = job->span + s;
        unsigned lo = first > base ? first - base : 0;
        unsigned hi = end - base < span->count ? end - base : span->count;
        for (j = lo; j < hi; j++) {
            drawVert_t *v = span->vertices + j;
            if (!CN_SourceSkinVertex(span->source + j, span->pose, v->xyz, v->normal))
                return 0;
            /* VectorNormalize is pure q_math; keep its existing sqrt/divide
             * and multiply sequence, including its zero-normal behavior. */
            VectorNormalize(v->normal);
            for (k = 0; k < 2; k++) v->st[k] = span->source[j].uv[k];
            if (span->white) v->color[0] = v->color[1] = v->color[2] = v->color[3] = 255;
        }
        base += span->count;
    }
    return 1;
}

static int R_Cod1TryDrawSkin(const c1DrawSkinSpan *span, unsigned spans,
                           unsigned vertices) {
    c1DrawSkinJob job;
    c1jFp before;
    if (vertices <= 512 || !C1J_Enabled()) return 0;
    C1J_FpGet(&before);
    /* Keep synchronous trap/first-failure order when exceptions are enabled. */
    if ((before.control & 63) != 63 || (before.mxcsr & 0x1f80) != 0x1f80) return 0;
    job.span = span;
    job.spans = spans;
    if (C1J_ParallelFor(vertices, 512, R_Cod1DrawSkinJob, &job)) return 1;
    C1J_FpRestore(&before, 0, 0);
    return 0;
}
#endif
