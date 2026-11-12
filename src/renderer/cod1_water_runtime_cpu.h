/* R50 resource lifecycle and native water consumers. No FS/GL/engine globals.
 * Cache tolerances: CoDSP 0x4fcc20; update guard: 0x4fce80. The RNG is supplied
 * by the caller; this never claims a recovered original session RNG state. */
#ifndef COD1_WATER_RUNTIME_CPU_R50_H
#define COD1_WATER_RUNTIME_CPU_R50_H
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <fenv.h>
#include <limits.h>
#include "cod1_water_program_types.h"
#define C1WR_RESOURCES 16
#define C1WR_CUBE_SIZE 64

typedef struct {
    c1w_desc desc; size_t count;
    c1w_complex *initial,*field; float *omega,*magnitude;unsigned char *rgba;
    unsigned frame;int hasFrame;unsigned updates;
} c1wr_resource;
typedef struct { c1w_tables tables;int tablesReady,count;c1wr_resource item[C1WR_RESOURCES]; } c1wr_pool;
static int C1WR_DescMatch(const c1w_desc *a,const c1w_desc *b) {
    double dot=(double)a->wind_x*b->wind_x+(double)a->wind_y*b->wind_y;
    double aa=(double)a->wind_x*a->wind_x+(double)a->wind_y*a->wind_y;
    double bb=(double)b->wind_x*b->wind_x+(double)b->wind_y*b->wind_y;
    return a->width==b->width&&a->height==b->height&&a->world_width==b->world_width&&a->world_height==b->world_height&&
       fabs((double)a->amplitude-b->amplitude)<(double)0.0010000000474974513&&
       fabs((double)a->gravity-b->gravity)<0.1&&fabs((double)a->wind_velocity-b->wind_velocity)<0.1&&
       fabs(dot/sqrt(aa*bb+1e-10)-1.0)<(double)0.0010000000474974513;
}
static void C1WR_FreeItem(c1wr_resource *r) {
    free(r->initial);free(r->field);free(r->omega);free(r->magnitude);free(r->rgba);memset(r,0,sizeof(*r));
}
static void C1WR_Reset(c1wr_pool *p) {int i;for(i=0;i<p->count;i++)C1WR_FreeItem(&p->item[i]);memset(p,0,sizeof(*p));}
static int C1WR_Get(c1wr_pool *p,const c1w_desc *d,c1w_rand15 rng,void *user) {
    int i;c1wr_resource fresh;size_t n;
    if(!p||C1W_Count(d,&n)!=C1W_OK||!rng)return -1;
    for(i=0;i<p->count;i++)if(C1WR_DescMatch(&p->item[i].desc,d))return i;
    if(p->count==C1WR_RESOURCES)return -1;
    memset(&fresh,0,sizeof(fresh));fresh.desc=*d;fresh.count=n;
    fresh.initial=(c1w_complex*)calloc(n,sizeof(c1w_complex));fresh.field=(c1w_complex*)calloc(n,sizeof(c1w_complex));
    fresh.omega=(float*)calloc(n,sizeof(float));fresh.magnitude=(float*)calloc(n,sizeof(float));fresh.rgba=(unsigned char*)malloc(n*4);
    if(!fresh.initial||!fresh.field||!fresh.omega||!fresh.magnitude||!fresh.rgba||
       C1W_Spectrum(d,rng,user,fresh.initial,fresh.omega,n)!=C1W_OK){C1WR_FreeItem(&fresh);return -1;}
    memset(fresh.rgba,255,n*4); /* native creation contents; first draw updates */
    if(!p->tablesReady){C1W_Tables(&p->tables);p->tablesReady=1;}
    i=p->count++;p->item[i]=fresh;return i;
}
/* Returns 1 for a new successful update, 0 for same-frame reuse, -1 on failure.
 * Caller uploads ONLY after return1. Frame IDs are compared for equality, so
 * wraparound and time rewinds do not suppress a genuinely new frame. */
static int C1WR_Update(c1wr_pool *p,int id,unsigned frame,float seconds) {
    c1wr_resource *r;size_t n;
    if(!p||id<0||id>=p->count)return -1;r=&p->item[id];n=r->count;
    if(r->hasFrame&&r->frame==frame)return 0;
    if(C1W_AtTime(&r->desc,&p->tables,seconds,r->initial,r->omega,r->field,n)!=C1W_OK||
       C1W_FFT2(&r->desc,&p->tables,r->field,n)!=C1W_OK||
       C1W_Normals(&r->desc,r->field,r->magnitude,n,r->rgba,n*4)!=C1W_OK)return -1;
    r->frame=frame;r->hasFrame=1;r->updates++;return 1;
}
static void C1WR_Normalize(float v[3]) {
    long double n=sqrtl((long double)v[0]*v[0]+(long double)v[1]*v[1]+(long double)v[2]*v[2]);
    if(n>0){v[0]=(float)(v[0]/n);v[1]=(float)(v[1]/n);v[2]=(float)(v[2]/n);}
}
/* CoDSP 0x50a720: normalized local view vector PLUS sun vector, NOT a final
 * normalized half vector. The authored renormalization cubemap does that. */
static void C1WR_SunHalf(const float eye[3],const float xyz[3],const float sun[3],float out[3]) {
    int k;for(k=0;k<3;k++)out[k]=eye[k]-xyz[k];C1WR_Normalize(out);for(k=0;k<3;k++)out[k]+=sun[k];
}
/* CoDSP 0x4f547c: N dot normalized local view, negative clamped, *255 + .5,
 * truncation to byte. Clamp the upper edge safely for non-unit/corrupt normals. */
static unsigned char C1WR_AlphaDot(const float eye[3],const float xyz[3],const float normal[3]) {
    float v[3],d;int k;for(k=0;k<3;k++)v[k]=eye[k]-xyz[k];C1WR_Normalize(v);
    d=(float)((long double)v[0]*normal[0]+(long double)v[1]*normal[1]+(long double)v[2]*normal[2]);
    if(!(d>0))return 0;if(d>=1)return 255;return (unsigned char)(d*255.0f+0.5f);
}
/* CoDSP 0x508ae0 ordinary wave, using its sine table (R49). Normals are NOT
 * rewritten. Negative frequency is not RTCW's special fire-rise convention.
 * Return0 before writing for unsafe/non-finite phases; not native overflow UB. */
static int C1WR_Wave(const c1wp_plan *p,const c1w_tables *t,float seconds,float *xyz,const float *normal,int count,int stride) {
    int i,k;long double phase;float scaled,amount,off;long index;
    if(!p||!t||count<0||stride<3||!xyz||!normal||fegetround()!=FE_TONEAREST)return 0;
    if(!p->hasWave)return 1;
    for(i=0;i<count;i++) {
        const float *v=xyz+i*stride;
        off=p->waveFrequency==0?0:(float)(((long double)v[2]+v[1]+v[0])*p->waveSpread);
        phase=(long double)off+p->wavePhase+(long double)seconds*p->waveFrequency;
        scaled=(float)(phase*1024.0L);
        if(!isfinite(scaled)||scaled>2147482624.0f||scaled< -2147482624.0f)return 0;
        for(k=0;k<3;k++)if(!isfinite(normal[i*stride+k])||!isfinite(v[k]))return 0;
    }
    for(i=0;i<count;i++) {
        float *v=xyz+i*stride;const float *n=normal+i*stride;
        off=p->waveFrequency==0?0:(float)(((long double)v[2]+v[1]+v[0])*p->waveSpread);
        phase=(long double)off+p->wavePhase+(long double)seconds*p->waveFrequency;
        scaled=(float)(phase*1024.0L);index=lrintf(scaled);
        amount=(float)((long double)t->phase_sine[index&1023]*p->waveAmplitude+p->waveBase);
        for(k=0;k<3;k++)v[k]=(float)((long double)v[k]+(long double)n[k]*amount);
    }return 1;
}
/* Native six faces at 64x64, texel CENTERS and signed face bases. Face order
 * +X,-X,+Y,-Y,+Z,-Z. Native constant multiplication affects RGB only, not A. */
static int C1WR_Renormalize(unsigned char *rgba,size_t bytes) {
    static const int axis[6][3]={{0,2,1},{0,2,1},{1,0,2},{1,0,2},{2,0,1},{2,0,1}};
    static const int sign[6][3]={{1,-1,-1},{-1,1,-1},{1,1,1},{-1,1,-1},{1,1,-1},{-1,-1,-1}};
    int f,x,y,k;float v[3],q;size_t at;
    if(!rgba||bytes<6u*64u*64u*4u||fegetround()!=FE_TONEAREST)return 0;
    for(f=0;f<6;f++)for(y=0;y<64;y++)for(x=0;x<64;x++) {
        v[axis[f][0]]=(float)sign[f][0];
        v[axis[f][1]]=((2*x+1)*(1.0f/64.0f)-1.0f)*sign[f][1];
        v[axis[f][2]]=((2*y+1)*(1.0f/64.0f)-1.0f)*sign[f][2];C1WR_Normalize(v);
        at=((size_t)f*4096+(size_t)y*64+x)*4;
        for(k=0;k<3;k++){q=(float)(((long double)v[k]+1)*127.5f);rgba[at+k]=(unsigned char)lrintl((long double)q+9.31322574615478515625e-10L);}rgba[at+3]=255;
    }return 1;
}
#endif
