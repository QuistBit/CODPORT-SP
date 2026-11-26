/* Pure projection jobs. Triangle filtering and all renderer/GL state stay
 * on the backend thread after these disjoint ranges have joined. */
#ifndef COD1_PERLIGHT_JOBS_H
#define COD1_PERLIGHT_JOBS_H
#include "cod1_perlight_projection.h"

typedef struct {
    const cod1PLProjectionLight_t *light;
    const float (*xyz)[4], (*normal)[4];
    cod1PLProjectionVertex_t *projected;
    unsigned char *clip;
    int useBasis;
    cod1PLRoundMode_t roundMode;
} c1PerlightJob;

static int C1Perlight_Range(void *context, unsigned first, unsigned end) {
    const c1PerlightJob *job = (const c1PerlightJob *)context;
    unsigned i;
    for (i = first; i < end; ++i) {
        if (!Cod1PL_ProjectVertex(job->light, job->xyz[i], job->normal[i],
                job->useBasis, job->roundMode, job->projected + i)) return 0;
        job->clip[i] = job->projected[i].clip;
    }
    return 1;
}
#endif
