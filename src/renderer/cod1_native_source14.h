/* Retained bind-space CoD1 v14 mesh data for native viewmodel poses.
 *
 * CV14_Mesh evaluates and discards the input weights. This reader keeps the
 * same bounded surface stream, local offsets and influences so a DObj pose
 * can skin each scene submission without altering the shared model resource.
 * Source part and surface arrays are owned by the caller; initialize a part
 * to zero before parsing and release it with CN_SourceFree. A successful
 * parse does not take ownership of the input file bytes.
 */
#ifndef COD1_NATIVE_SOURCE14_H
#define COD1_NATIVE_SOURCE14_H

#include "cod1_viewmodel14.h"

/* One implicit weight followed by at most eight explicit v14 influences. */
#define CN_SOURCE_MAX_WEIGHTS 9

typedef struct {
    unsigned short bone;
    float offset[3];
    float weight;
} cnSourceWeight_t;

typedef struct {
    float normal[3], uv[2];
    unsigned short normalBone;
    unsigned char numWeights;
    cnSourceWeight_t weight[CN_SOURCE_MAX_WEIGHTS];
} cnSourceVertex_t;

typedef struct {
    int numVerts, numIndexes;
    cnSourceVertex_t *verts;
    unsigned short *indexes;
} cnSourceSurface_t;

typedef struct {
    cv14Skeleton bind;
    int numSurfaces;
    cnSourceSurface_t surfaces[XM14_SURFS];
} cnSourcePart_t;

static void CN_SourceFree(cnSourcePart_t *part) {
    int i;
    if (!part) return;
    for (i = 0; i < XM14_SURFS; i++) {
        free(part->surfaces[i].verts);
        free(part->surfaces[i].indexes);
    }
    memset(part, 0, sizeof(*part));
}

/* The bind argument may be &part->bind. On success the destination owns the
 * source vertices and triangle indices; on failure it is reset to zero.
 * As with CV14_Mesh, part must be zero-initialized before its first parse. */
static int CN_SourceParseMesh(const void *data, size_t size,
                              const cv14Skeleton *bind, cnSourcePart_t *part) {
    xm14Reader r;
    int i, j, k, c, a, b, d, faces, bone, bi;
    int totalWeights, expectedWeights, unweighted, expectedUnweighted;
    unsigned short counts[CV14_VERTS];
    float sums[CV14_VERTS], normal[3], pos[3], w;
    cnSourceSurface_t *surf;
    cnSourceVertex_t *v;

    if (!data || !part || !bind || bind->n <= 0 || bind->n > CV14_BONES)
        return 0;
    if (bind != &part->bind) part->bind = *bind;
    r.p = (const unsigned char *)data;
    r.n = size;
    r.at = 0;
    r.ok = 1;
    if (XM14_U16(&r) != 14) goto bad;
    part->numSurfaces = (int)XM14_U16(&r);
    if (!part->numSurfaces || part->numSurfaces > XM14_SURFS) goto bad;

    for (i = 0; i < part->numSurfaces; i++) {
        surf = &part->surfaces[i];
        XM14_U8(&r);
        surf->numVerts = (int)XM14_U16(&r);
        faces = (int)XM14_U16(&r);
        XM14_U16(&r);
        bone = (int)XM14_U16(&r);
        if (!r.ok || !surf->numVerts || surf->numVerts > CV14_VERTS ||
            !faces || faces > CV14_TRIS || (bone != 65535 && bone >= bind->n))
            goto bad;
        surf->numIndexes = faces * 3;
        expectedWeights = expectedUnweighted = -1;
        if (bone == 65535) {
            expectedWeights = (int)XM14_U16(&r);
            expectedUnweighted = (int)XM14_U16(&r);
        }
        surf->verts = (cnSourceVertex_t *)calloc((size_t)surf->numVerts,
                                                 sizeof(*surf->verts));
        surf->indexes = (unsigned short *)calloc((size_t)surf->numIndexes,
                                                  sizeof(*surf->indexes));
        if (!surf->verts || !surf->indexes) goto bad;

        faces = 0;
        while (faces < surf->numIndexes / 3) {
            c = (int)XM14_U8(&r);
            if (c < 3) goto bad;
            a = (int)XM14_U16(&r);
            b = (int)XM14_U16(&r);
            if (a >= surf->numVerts || b >= surf->numVerts) goto bad;
            for (j = 2; j < c; j++) {
                d = (int)XM14_U16(&r);
                if (!r.ok || d >= surf->numVerts) goto bad;
                if (a != b && a != d && b != d) {
                    if (faces >= surf->numIndexes / 3) goto bad;
                    surf->indexes[faces * 3] = (unsigned short)d;
                    surf->indexes[faces * 3 + 1] = (unsigned short)((j & 1) ? b : a);
                    surf->indexes[faces * 3 + 2] = (unsigned short)((j & 1) ? a : b);
                    faces++;
                }
                a = b;
                b = d;
            }
        }

        totalWeights = unweighted = 0;
        for (j = 0; j < surf->numVerts; j++) {
            v = &surf->verts[j];
            for (k = 0; k < 3; k++) normal[k] = XM14_F32(&r);
            for (k = 0; k < 3; k++) v->normal[k] = normal[k];
            for (k = 0; k < 2; k++) v->uv[k] = XM14_F32(&r);
            counts[j] = (unsigned short)(bone == 65535 ? XM14_U16(&r) : 0);
            bi = bone == 65535 ? (int)XM14_U16(&r) : bone;
            if (!r.ok || bi >= bind->n ||
                counts[j] > CN_SOURCE_MAX_WEIGHTS - 1) goto bad;
            v->normalBone = (unsigned short)bi;
            v->numWeights = (unsigned char)(counts[j] + 1);
            for (k = 0; k < 3; k++) pos[k] = XM14_F32(&r);
            w = counts[j] ? XM14_F32(&r) : 1.0f;
            if (!r.ok || w < 0 || w > 1) goto bad;
            sums[j] = w;
            v->weight[0].bone = (unsigned short)bi;
            v->weight[0].weight = w;
            for (k = 0; k < 3; k++) v->weight[0].offset[k] = pos[k];
            totalWeights += counts[j];
            if (!counts[j]) unweighted++;
        }
        if (bone == 65535 && (totalWeights != expectedWeights ||
                              unweighted != expectedUnweighted)) goto bad;
        for (j = 0; j < surf->numVerts; j++) {
            v = &surf->verts[j];
            for (c = 1; c < v->numWeights; c++) {
                bi = (int)XM14_U16(&r);
                if (!r.ok || bi >= bind->n) goto bad;
                for (k = 0; k < 3; k++) pos[k] = XM14_F32(&r);
                w = XM14_F32(&r);
                if (!r.ok || w < 0 || w > 1) goto bad;
                sums[j] += w;
                v->weight[c].bone = (unsigned short)bi;
                v->weight[c].weight = w;
                for (k = 0; k < 3; k++) v->weight[c].offset[k] = pos[k];
            }
            if (fabsf(sums[j] - 1.0f) > 0.001f) goto bad;
        }
    }
    if (r.ok && r.at == r.n) return 1;
bad:
    CN_SourceFree(part);
    return 0;
}

#if defined(_MSC_VER) && (defined(_M_X64) || (defined(_M_IX86_FP) && _M_IX86_FP >= 2))
#include <emmintrin.h>
/* Same component operations as CV14_Rotate, with three independent lanes.
 * The source contains exactly three floats: do not read a fourth component.
 * Skin callers write a distinct frame/stack vertex, never the source mesh. */
static void CN_SourceRotate3(const float *q,const float *v,float *out) {
    __m128 quat=_mm_loadu_ps(q),vertex=_mm_set_ps(0,v[2],v[1],v[0]);
    __m128 qyzx=_mm_shuffle_ps(quat,quat,_MM_SHUFFLE(3,0,2,1));
    __m128 qzxy=_mm_shuffle_ps(quat,quat,_MM_SHUFFLE(3,1,0,2));
    __m128 vyzx=_mm_shuffle_ps(vertex,vertex,_MM_SHUFFLE(3,0,2,1));
    __m128 vzxy=_mm_shuffle_ps(vertex,vertex,_MM_SHUFFLE(3,1,0,2));
    __m128 t=_mm_mul_ps(_mm_set1_ps(2),_mm_sub_ps(_mm_mul_ps(qyzx,vzxy),_mm_mul_ps(qzxy,vyzx)));
    __m128 tyzx=_mm_shuffle_ps(t,t,_MM_SHUFFLE(3,0,2,1));
    __m128 tzxy=_mm_shuffle_ps(t,t,_MM_SHUFFLE(3,1,0,2));
    __m128 cross=_mm_sub_ps(_mm_mul_ps(qyzx,tzxy),_mm_mul_ps(qzxy,tyzx));
    __m128 qw=_mm_shuffle_ps(quat,quat,_MM_SHUFFLE(3,3,3,3));
    float result[4];
    _mm_storeu_ps(result,_mm_add_ps(cross,_mm_add_ps(vertex,_mm_mul_ps(qw,t))));
    out[0]=result[0];out[1]=result[1];out[2]=result[2];
}
#else
#define CN_SourceRotate3 CV14_Rotate
#endif
/* Skin one retained vertex through the current local part skeleton. The
 * caller handles normal normalization after any additional pose blending. */
static int CN_SourceSkinVertex(const cnSourceVertex_t *src,
                               const cv14Skeleton *sk,
                               float xyz[3], float normal[3]) {
    const cnSourceWeight_t *w;
    float p[3];
    int i, k;
    if (!src || !sk || !xyz || !normal || sk->n <= 0 ||
        sk->n > CV14_BONES || !src->numWeights ||
        src->numWeights > CN_SOURCE_MAX_WEIGHTS || src->normalBone >= sk->n)
        return 0;
    CN_SourceRotate3(sk->world[src->normalBone].q, src->normal, normal);
    xyz[0] = xyz[1] = xyz[2] = 0;
    for (i = 0; i < src->numWeights; i++) {
        w = &src->weight[i];
        if (w->bone >= sk->n) return 0;
        CN_SourceRotate3(sk->world[w->bone].q,w->offset,p);
        for(k=0;k<3;k++)p[k]+=sk->world[w->bone].p[k];
        for (k = 0; k < 3; k++) xyz[k] += w->weight * p[k];
    }
    for (k = 0; k < 3; k++)
        if (!(xyz[k] > -100000000 && xyz[k] < 100000000) ||
            !(normal[k] > -100000000 && normal[k] < 100000000)) return 0;
    return 1;
}

#endif /* COD1_NATIVE_SOURCE14_H */
