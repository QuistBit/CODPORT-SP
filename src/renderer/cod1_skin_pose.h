/* Immutable per-part quaternion permutations. Preparation uses bitwise
 * shuffles only; every vertex retains the original arithmetic and checks.
 * Pose and prepared storage belong to the caller until synchronous join. */
#ifndef COD1_SKIN_POSE_H
#define COD1_SKIN_POSE_H
#include "cod1_native_source14.h"
#if defined(_MSC_VER) && (defined(_M_X64) || (defined(_M_IX86_FP) && _M_IX86_FP >= 2))
typedef struct {__m128 yzx,zxy,w;} cnSkinRotation_t;
typedef struct {const cv14Skeleton *source;cnSkinRotation_t rotation[CV14_BONES];} cnSkinPose_t;
static int CN_SourcePrepareSkinPose(const cv14Skeleton *source,cnSkinPose_t *out){
 int i;if(!source||!out||source->n<=0||source->n>CV14_BONES)return 0;
 out->source=source;
 for(i=0;i<source->n;i++){
  __m128 q=_mm_loadu_ps(source->world[i].q);
  out->rotation[i].yzx=_mm_shuffle_ps(q,q,_MM_SHUFFLE(3,0,2,1));
  out->rotation[i].zxy=_mm_shuffle_ps(q,q,_MM_SHUFFLE(3,1,0,2));
  out->rotation[i].w=_mm_shuffle_ps(q,q,_MM_SHUFFLE(3,3,3,3));
 }return 1;
}
static void CN_SourceRotatePrepared(const cnSkinRotation_t *rotation,const float *v,float *out) {
    __m128 vertex=_mm_set_ps(0,v[2],v[1],v[0]);
    __m128 qyzx=rotation->yzx,qzxy=rotation->zxy;
    __m128 vyzx=_mm_shuffle_ps(vertex,vertex,_MM_SHUFFLE(3,0,2,1));
    __m128 vzxy=_mm_shuffle_ps(vertex,vertex,_MM_SHUFFLE(3,1,0,2));
    __m128 t=_mm_mul_ps(_mm_set1_ps(2),_mm_sub_ps(_mm_mul_ps(qyzx,vzxy),_mm_mul_ps(qzxy,vyzx)));
    __m128 tyzx=_mm_shuffle_ps(t,t,_MM_SHUFFLE(3,0,2,1));
    __m128 tzxy=_mm_shuffle_ps(t,t,_MM_SHUFFLE(3,1,0,2));
    __m128 cross=_mm_sub_ps(_mm_mul_ps(qyzx,tzxy),_mm_mul_ps(qzxy,tyzx));
    __m128 qw=rotation->w;
    float result[4];
    _mm_storeu_ps(result,_mm_add_ps(cross,_mm_add_ps(vertex,_mm_mul_ps(qw,t))));
    out[0]=result[0];out[1]=result[1];out[2]=result[2];
}
static int CN_SourceSkinPrepared(const cnSourceVertex_t *src,
                               const cnSkinPose_t *pose,
                               float xyz[3], float normal[3]) {
    const cv14Skeleton *sk=pose?pose->source:NULL;
    const cnSourceWeight_t *w;
    float p[3];
    int i, k;
    if (!src || !sk || !xyz || !normal || sk->n <= 0 ||
        sk->n > CV14_BONES || !src->numWeights ||
        src->numWeights > CN_SOURCE_MAX_WEIGHTS || src->normalBone >= sk->n)
        return 0;
    CN_SourceRotatePrepared(pose->rotation+src->normalBone, src->normal, normal);
    xyz[0] = xyz[1] = xyz[2] = 0;
    for (i = 0; i < src->numWeights; i++) {
        w = &src->weight[i];
        if (w->bone >= sk->n) return 0;
        CN_SourceRotatePrepared(pose->rotation+w->bone,w->offset,p);
        for(k=0;k<3;k++)p[k]+=sk->world[w->bone].p[k];
        for (k = 0; k < 3; k++) xyz[k] += w->weight * p[k];
    }
    for (k = 0; k < 3; k++)
        if (!(xyz[k] > -100000000 && xyz[k] < 100000000) ||
            !(normal[k] > -100000000 && normal[k] < 100000000)) return 0;
    return 1;
}

#else
typedef struct {const cv14Skeleton *source;} cnSkinPose_t;
static int CN_SourcePrepareSkinPose(const cv14Skeleton *source,cnSkinPose_t *out){if(!source||!out||source->n<=0||source->n>CV14_BONES)return 0;out->source=source;return 1;}
static int CN_SourceSkinPrepared(const cnSourceVertex_t *v,const cnSkinPose_t *p,float xyz[3],float normal[3]){return CN_SourceSkinVertex(v,p?p->source:NULL,xyz,normal);}
#endif
#endif
