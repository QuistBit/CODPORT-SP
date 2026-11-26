/* Production pure lighting jobs + production dispatcher under the portable
 * Win32 synchronization adapter. No GL/engine is mocked into worker calls. */
#include "mt_test_runtime.h"
#include <fenv.h>
#include <math.h>
#include <stddef.h>
#include <stdio.h>

/* Same field layout as qfiles.h; only these vertex fields are consumed here. */
typedef struct {
    float xyz[3],st[2],lightmap[2],normal[3];
    unsigned char color[4];
} drawVert_t;
typedef struct { float normal[3],uv[2],xyz[3]; } xm14Vertex;
/* Exact scalar operation order from game/q_math.c:VectorNormalize. */
static float VectorNormalize(float v[3]) {
    float length,ilength;
    length=v[0]*v[0]+v[1]*v[1]+v[2]*v[2];
    length=sqrt(length);
    if(length){ilength=1/length;v[0]*=ilength;v[1]*=ilength;v[2]*=ilength;}
    return length;
}
#include "../../src/renderer/cod1_prepared_light_jobs.h"
#include "../../src/renderer/cod1_static_bake_jobs.h"
#include "../../src/renderer/cod1_perlight_jobs.h"
/* Minimal renderer-private entity fields consumed by the actual entity
 * kernel; no public renderer ABI or GL implementation is replaced. */
typedef unsigned char byte;
typedef float vec4_t[4];
typedef int qboolean;
enum { qfalse=0,qtrue=1 };
#define SHADER_MAX_VERTEXES 4000
typedef struct {
    struct { float origin[3],axis[3][3]; } e;
    c1mModelLightFrame cod1ModelLighting;
} trRefEntity_t;
#define DotProduct(a,b) ((a)[0]*(b)[0]+(a)[1]*(b)[1]+(a)[2]*(b)[2])
static void CrossProduct(const float a[3],const float b[3],float out[3]) {
    out[0]=a[1]*b[2]-a[2]*b[1];
    out[1]=a[2]*b[0]-a[0]*b[2];
    out[2]=a[0]*b[1]-a[1]*b[0];
}
#ifdef MT_TEST_SIMD
#define __forceinline inline __attribute__((always_inline))
#define __declspec(storage) _Thread_local
#include "../../src/renderer/cod1_light_quad347.h"
#endif
#include "../../src/renderer/cod1_entity_light_jobs.h"

enum { VERTICES=999 };
static float sineTable[1024];
static c1mModelLightFrame frame;
static const int rounding[4]={FE_TONEAREST,FE_DOWNWARD,FE_UPWARD,FE_TOWARDZERO};

static void same_fp(const c1jFp *a,const c1jFp *b) {
    assert(a->control==b->control);
    assert((a->status&63)==(b->status&63));
    assert(a->mxcsr==b->mxcsr);
}

static c1PreparedLightInput lighting(int mode,int fullbright) {
    c1PreparedLightInput input;
    input.frame=&frame;input.sine=sineTable;
    input.fullbright=fullbright;input.identity=173;input.roundMode=mode;
    return input;
}

static void make_frame(void) {
    unsigned i,k;
    memset(&frame,0,sizeof(frame));frame.valid=1;
    frame.prepared.remainingSky=.625f;frame.prepared.count=3;
    for(k=0;k<3;k++) {
        frame.world.ambient[k]=.025f*(k+1);
        frame.world.sky[k]=.08f*(k+1);
    }
    for(i=0;i<3;i++) {
        c1mLight *light=frame.prepared.lights+i;
        frame.prepared.weights[i]=.2f*(i+1);
        light->positionW=i?1:0;
        light->position[0]=i?25:0;light->position[1]=i?-7:0;light->position[2]=i?80:1;
        light->attenuation[0]=1;light->attenuation[1]=.002f;light->attenuation[2]=.00001f;
        light->spotCutoff=i==2?60:180;light->spotExponent=3;
        light->spotDirection[2]=1;
        for(k=0;k<3;k++){light->ambient[k]=.02f*(i+1);light->diffuse[k]=.1f*(k+1);}
    }
    C1M_InitShadeSinTable(sineTable);
}

static int scalar_colors(const c1PreparedLightJob *job,unsigned count) {
    unsigned i;
    for(i=0;i<count;i++) {
        const drawVert_t *v=(const drawVert_t *)(job->vertices+i*job->stride);
        unsigned char *color=job->rgba+4*i;
        if(job->light.fullbright)color[0]=color[1]=color[2]=job->light.identity;
        else if(!C1M_Shade(&frame.world,frame.prepared.remainingSky,frame.prepared.lights,
                frame.prepared.weights,frame.prepared.count,v->xyz,v->normal,
                job->light.roundMode,sineTable,color))return 0;
        color[3]=v->color[3];
    }
    return 1;
}

static void test_colors(void) {
    drawVert_t vertices[VERTICES],original[VERTICES];
    unsigned char expected[VERTICES*4+16],actual[VERTICES*4+16];
    c1PreparedLightJob job;
    c1jFp before,scalarFP,parallelFP;
    unsigned i,mode,bright;
    memset(vertices,0,sizeof(vertices));
    for(i=0;i<VERTICES;i++) {
        vertices[i].xyz[0]=(float)(i%41)-20;vertices[i].xyz[1]=(float)(i%17)-8;
        vertices[i].xyz[2]=(float)(i%7);vertices[i].normal[2]=1;
        vertices[i].color[3]=(unsigned char)i;
    }
    memcpy(original,vertices,sizeof(vertices));
    memset(&job,0,sizeof(job));job.vertices=(const unsigned char *)vertices;
    job.stride=sizeof(*vertices);job.xyzOffset=offsetof(drawVert_t,xyz);
    job.normalOffset=offsetof(drawVert_t,normal);job.alphaOffset=offsetof(drawVert_t,color)+3;
    for(mode=0;mode<4;mode++)for(bright=0;bright<2;bright++) {
        assert(!fesetround(rounding[mode]));feclearexcept(FE_ALL_EXCEPT);
        feraiseexcept(FE_INEXACT);C1J_FpGet(&before);
        job.light=lighting((int)mode,(int)bright);
        memset(expected,0xa5,sizeof(expected));memset(actual,0xa5,sizeof(actual));
        job.rgba=expected;assert(scalar_colors(&job,VERTICES));C1J_FpGet(&scalarFP);
        C1J_FpRestore(&before,0,0);job.rgba=actual;
        assert(C1J_ParallelFor(VERTICES,128,C1PreparedLight_Range,&job));
        C1J_FpGet(&parallelFP);same_fp(&scalarFP,&parallelFP);
        assert(!memcmp(expected,actual,sizeof(actual)));
        assert(!memcmp(original,vertices,sizeof(vertices)));
    }
    /* A malformed late vertex must be reported. Neither input nor guard bytes
     * can change; the owner leaves its active cache unpublished on failure. */
    assert(!fesetround(FE_TONEAREST));job.light=lighting(0,0);
    vertices[301].xyz[0]=NAN;
    memset(actual,0xa5,sizeof(actual));job.rgba=actual;C1J_FpGet(&before);
    assert(!C1J_ParallelFor(VERTICES,128,C1PreparedLight_Range,&job));
    C1J_FpRestore(&before,0,0);assert(!scalar_colors(&job,VERTICES));
    for(i=VERTICES*4;i<sizeof(actual);i++)assert(actual[i]==0xa5);
    /* Fullbright preserves its original geometry bypass and authored alpha. */
    job.light.fullbright=1;assert(C1J_ParallelFor(VERTICES,128,C1PreparedLight_Range,&job));
    for(i=0;i<VERTICES;i++){assert(actual[4*i]==173);assert(actual[4*i+3]==(unsigned char)i);}
}

static int scalar_bake(const c1StaticBakeJob *job,unsigned count) {
    unsigned j;int k,q;
    for(j=0;j<count;j++) {
        drawVert_t *v=job->vertices+j;const xm14Vertex *source=job->source+j;
        memset(v,0,sizeof(*v));
        for(k=0;k<3;k++) {
            v->xyz[k]=job->origin[k];v->normal[k]=0;
            for(q=0;q<3;q++) {
                v->xyz[k]+=source->xyz[q]*job->scale[q]*job->axis[q][k];
                v->normal[k]+=source->normal[q]/job->scale[q]*job->axis[q][k];
            }
            v->color[k]=(unsigned char)(255*(job->precalc[k]>1?1:job->precalc[k]));
        }
        VectorNormalize(v->normal);v->color[3]=255;
        if(job->light.fullbright)v->color[0]=v->color[1]=v->color[2]=job->light.identity;
        else if(!C1M_Shade(&frame.world,frame.prepared.remainingSky,frame.prepared.lights,
                frame.prepared.weights,frame.prepared.count,v->xyz,v->normal,
                job->light.roundMode,sineTable,v->color))return 0;
        if(job->debugWhite)for(k=0;k<3;k++)v->color[k]=255;
        v->st[0]=source->uv[0];v->st[1]=source->uv[1];
    }
    return 1;
}

static void test_bake(void) {
    xm14Vertex source[VERTICES],original[VERTICES];
    drawVert_t expected[VERTICES+1],actual[VERTICES+1];
    c1StaticBakeJob job;c1jFp before,a,b;unsigned i,mode,bright,white;
    memset(&job,0,sizeof(job));job.source=source;
    job.scale[0]=-2;job.scale[1]=.5f;job.scale[2]=1.25f;
    job.axis[0][1]=1;job.axis[1][0]=-1;job.axis[2][2]=1;
    job.origin[0]=123;job.origin[1]=-42;job.origin[2]=.125f;
    job.precalc[0]=.3f;job.precalc[1]=.7f;job.precalc[2]=1.2f;
    memset(source,0,sizeof(source));
    for(i=0;i<VERTICES;i++) {
        source[i].xyz[0]=(float)(i%71)-35;source[i].xyz[1]=(float)(i%19)-9;
        source[i].xyz[2]=(float)(i%13);
        source[i].normal[0]=.3f;source[i].normal[1]=.4f;source[i].normal[2]=.5f;
        source[i].uv[0]=(float)i/1024;source[i].uv[1]=(float)(i%29)/32;
    }
    memcpy(original,source,sizeof(source));
    for(mode=0;mode<4;mode++)for(bright=0;bright<2;bright++)for(white=0;white<2;white++) {
        assert(!fesetround(rounding[mode]));feclearexcept(FE_ALL_EXCEPT);C1J_FpGet(&before);
        job.light=lighting((int)mode,(int)bright);job.debugWhite=(int)white;
        memset(expected,0xa5,sizeof(expected));memset(actual,0xa5,sizeof(actual));
        job.vertices=expected;assert(scalar_bake(&job,VERTICES));C1J_FpGet(&a);
        C1J_FpRestore(&before,0,0);job.vertices=actual;
        assert(C1J_ParallelFor(VERTICES,128,C1StaticBake_Range,&job));C1J_FpGet(&b);
        same_fp(&a,&b);assert(!memcmp(expected,actual,sizeof(actual)));
        assert(!memcmp(original,source,sizeof(source)));
    }
    assert(!fesetround(FE_TONEAREST));job.light=lighting(0,0);
    source[501].normal[0]=NAN;job.vertices=actual;
    assert(!C1J_ParallelFor(VERTICES,128,C1StaticBake_Range,&job));
}

static void test_projection(void) {
    float xyz[VERTICES][4],normal[VERTICES][4];
    unsigned char clip[VERTICES+1],expectedClip[VERTICES+1];
    cod1PLProjectionVertex_t expected[VERTICES],actual[VERTICES];
    cod1PLProjectionLight_t light={80,{2,3,40},{.3f,.8f,1.25f}};
    c1PerlightJob job;c1jFp before,a,b;unsigned i,mode,basis;
    memset(xyz,0,sizeof(xyz));memset(normal,0,sizeof(normal));
    for(i=0;i<VERTICES;i++) {
        xyz[i][0]=(float)(i%101)-50;xyz[i][1]=(float)(i%51)-25;
        xyz[i][2]=(float)(i%17);normal[i][2]=1;
    }
    job.light=&light;job.xyz=xyz;job.normal=normal;
    for(mode=0;mode<4;mode++)for(basis=0;basis<2;basis++) {
        assert(!fesetround(rounding[mode]));feclearexcept(FE_ALL_EXCEPT);C1J_FpGet(&before);
        memset(expectedClip,0xa5,sizeof(expectedClip));memset(clip,0xa5,sizeof(clip));
        for(i=0;i<VERTICES;i++) {
            assert(Cod1PL_ProjectVertex(&light,xyz[i],normal[i],basis,(cod1PLRoundMode_t)mode,expected+i));
            expectedClip[i]=expected[i].clip;
        }
        C1J_FpGet(&a);C1J_FpRestore(&before,0,0);
        job.useBasis=(int)basis;job.roundMode=(cod1PLRoundMode_t)mode;
        job.projected=actual;job.clip=clip;
        assert(C1J_ParallelFor(VERTICES,128,C1Perlight_Range,&job));C1J_FpGet(&b);
        same_fp(&a,&b);assert(!memcmp(expectedClip,clip,sizeof(clip)));
        for(i=0;i<VERTICES;i++) {
            assert(!memcmp(expected[i].st,actual[i].st,sizeof(expected[i].st)));
            assert(!memcmp(expected[i].rgba,actual[i].rgba,4));assert(expected[i].clip==actual[i].clip);
        }
    }
    assert(!fesetround(FE_TONEAREST));job.roundMode=COD1_PL_ROUND_NEAREST;
    xyz[400][0]=NAN;assert(!C1J_ParallelFor(VERTICES,128,C1Perlight_Range,&job));
}

static void test_entity_lighting(void) {
    trRefEntity_t entity;
    vec4_t xyz[VERTICES],normal[VERTICES];
    unsigned char scalar[VERTICES*4+16],parallel[VERTICES*4+16];
    cod1EntityLightInput_t input;
    c1jFp before,a,b;
    unsigned i,mode,bright;
    memset(&entity,0,sizeof(entity));entity.cod1ModelLighting=frame;
    entity.e.axis[0][0]=-2;entity.e.axis[1][1]=.5f;entity.e.axis[2][2]=1.25f;
    entity.e.origin[0]=20;entity.e.origin[1]=3;entity.e.origin[2]=-6;
    memset(xyz,0,sizeof(xyz));memset(normal,0,sizeof(normal));
    for(i=0;i<VERTICES;i++) {
        xyz[i][0]=(float)(i%101)-50;xyz[i][1]=(float)(i%51)-25;
        xyz[i][2]=(float)(i%17);normal[i][2]=1;
    }
    input.valid=1;input.sine=sineTable;input.identity=173;
    for(mode=0;mode<4;mode++)for(bright=0;bright<2;bright++) {
        assert(!fesetround(rounding[mode]));feclearexcept(FE_ALL_EXCEPT);C1J_FpGet(&before);
        input.roundMode=(int)mode;input.fullbright=(int)bright;
        memset(scalar,0xa5,sizeof(scalar));memset(parallel,0xa5,sizeof(parallel));
        assert(R_Cod1ShadeEntityVerticesInput(&entity,VERTICES,xyz,normal,scalar,&input));
        C1J_FpGet(&a);C1J_FpRestore(&before,0,0);
        assert(C1EntityLight_Shade(&entity,VERTICES,xyz,normal,parallel,&input));
        C1J_FpGet(&b);same_fp(&a,&b);
        assert(!memcmp(scalar,parallel,sizeof(scalar)));
    }
    /* Failed speculative work must never leak later chunks into the caller's
     * RGBA. The production wrapper restores FP and writes the same scalar
     * first-failure prefix while leaving the output suffix untouched. */
    assert(!fesetround(FE_TONEAREST));input.roundMode=0;input.fullbright=0;
    xyz[301][0]=NAN;
    memset(scalar,0xa5,sizeof(scalar));memset(parallel,0xa5,sizeof(parallel));
    C1J_FpGet(&before);
    assert(!R_Cod1ShadeEntityVerticesInput(&entity,VERTICES,xyz,normal,scalar,&input));
    C1J_FpGet(&a);C1J_FpRestore(&before,0,0);
    assert(!C1EntityLight_Shade(&entity,VERTICES,xyz,normal,parallel,&input));
    C1J_FpGet(&b);same_fp(&a,&b);assert(!memcmp(scalar,parallel,sizeof(scalar)));
    assert(C1EntityLight_Shade(NULL,0,NULL,NULL,NULL,NULL));
}

int main(void) {
    c1jStatus status;
    make_frame();C1J_Configure(1,3);
    test_colors();test_bake();test_projection();test_entity_lighting();
    C1J_Status(&status);
    assert(status.batches>0 && status.workerChunks>0);
    C1J_Configure(0,0);test_colors();test_bake();test_projection();test_entity_lighting();
    C1J_Shutdown();assert(!atomic_load(&mt_live_handles));assert(!atomic_load(&mt_live_threads));
    printf("MT render-light parity PASS: 4 rounding modes, RGB/alpha/geometry/UV/FP, malformed inputs, disabled path; batches=%llu workerChunks=%llu\n",
           status.batches,status.workerChunks);
    return 0;
}
