/* SPDX-License-Identifier: GPL-3.0-or-later
 * Reuses the verified RTCW-derived facet builder with CoD1 subdivision.
 * Only construction uses legacy globals; query state is separate. */
#include "v59_internal.h"
#include "v59_prefix.h"
#include "geom/qcommon/cm_patch.c"
#include <stdarg.h>
static jmp_buf failure;
static char *failure_message;
static size_t failure_size;
static allocation *temporary;
void QDECL Com_Error(int code,const char *fmt,...) {
    va_list ap;(void)code;va_start(ap,fmt);
    if(failure_message && failure_size) vsnprintf(failure_message,failure_size,fmt,ap);
    va_end(ap);longjmp(failure,1);
}
void QDECL Com_Printf(const char *fmt,...) {(void)fmt;}
void QDECL Com_DPrintf(const char *fmt,...) {(void)fmt;}
void *Z_Malloc(int n) {
    allocation *a=malloc(sizeof(*a));void *p;
    if(!a) Com_Error(0,"facet allocation failed");
    p=calloc(1,n?n:1);if(!p){free(a);Com_Error(0,"facet allocation failed");}
    a->ptr=p;a->next=temporary;temporary=a;return p;
}
void Z_Free(void *p) {
    allocation **a=&temporary,*item;
    while(*a && (*a)->ptr!=p)a=&(*a)->next;
    if(*a){item=*a;*a=item->next;free(item);}free(p);
}
void *Hunk_Alloc(int n,ha_pref pref){(void)pref;return Z_Malloc(n);}
void Com_Memset(void*p,const int v,const size_t n){memset(p,v,n);}
void Com_Memcpy(void*p,const void*s,const size_t n){memcpy(p,s,n);}
static void release_except(void *p,void *q){
    allocation *a;while(temporary){a=temporary;temporary=a->next;
    if(a->ptr!=p && a->ptr!=q)free(a->ptr);free(a);}
}
static qboolean needs_split(const vec3_t a,const vec3_t b,const vec3_t c,float tolerance) {
    vec3_t d; float length2; int k;
    for(k=0;k<3;k++) d[k]=(c[k]+a[k])-(b[k]+b[k]);
    length2=(d[2]*d[2]+d[1]*d[1])+d[0]*d[0];
    return sqrtf(length2)*0.25f > tolerance;
}

static void subdivide(cGrid_t *g,int tolerance) {
    int i,j,k,n; float t=(float)tolerance;
    for(i=0;i<g->width-2;) {
        for(j=0;j<g->height;j++)
            if(needs_split(g->points[i][j],g->points[i+1][j],g->points[i+2][j],t)) break;
        /* CoD retains the intermediate control column when the test passes. */
        if(j==g->height) { i+=2; continue; }
        if(g->width+2>MAX_GRID_SIZE) Com_Error(ERR_DROP,"v59 subdivision exceeds 129 columns");
        for(j=0;j<g->height;j++) {
            vec3_t a,b,c,left,right,center;
            VectorCopy(g->points[i][j],a); VectorCopy(g->points[i+1][j],b);
            VectorCopy(g->points[i+2][j],c);
            for(n=0;n<3;n++) {
                left[n]=(b[n]+a[n])*0.5f;
                right[n]=(c[n]+b[n])*0.5f;
                center[n]=(right[n]+left[n])*0.5f;
            }
            for(k=g->width-1;k>i+1;k--) VectorCopy(g->points[k][j],g->points[k+2][j]);
            VectorCopy(left,g->points[i+1][j]); VectorCopy(center,g->points[i+2][j]);
            VectorCopy(right,g->points[i+3][j]);
        }
        g->width+=2;
    }
}

int v59_curve_build(int w,int h,int tolerance,const float *points,curve *out,float bounds[6],char *err,size_t errsize) {
    cGrid_t *g;patchCollide_t pf;int x,y,k;
    failure_message=err;failure_size=errsize;temporary=NULL;
    if(setjmp(failure)){release_except(NULL,NULL);return -1;}
    if(w<3||h<3||w>129||h>129||!(w&1)||!(h&1)||w*h>8192||tolerance<1)
        Com_Error(0,"invalid curve control grid");
    memset(&pf,0,sizeof(pf));g=Z_Malloc(sizeof(*g));g->width=w;g->height=h;
    for(y=0;y<h;y++)for(x=0;x<w;x++)for(k=0;k<3;k++)g->points[x][y][k]=points[(y*w+x)*3+k];
    CM_SetGridWrapWidth(g);subdivide(g,tolerance);CM_RemoveDegenerateColumns(g);CM_TransposeGrid(g);
    CM_SetGridWrapWidth(g);subdivide(g,tolerance);CM_RemoveDegenerateColumns(g);
    for(k=0;k<3;k++){pf.bounds[0][k]=262144.f;pf.bounds[1][k]=-262144.f;}
    for(x=0;x<g->width;x++)for(y=0;y<g->height;y++)AddPointToBounds(g->points[x][y],pf.bounds[0],pf.bounds[1]);
    CM_PatchCollideFromGrid(g,&pf);
    for(k=0;k<3;k++){bounds[k]=pf.bounds[0][k]-1.f;bounds[k+3]=pf.bounds[1][k]+1.f;}
    out->np=pf.numPlanes;out->nf=pf.numFacets;out->planes=(curve_plane*)pf.planes;out->facets=(facet*)pf.facets;
    release_except(pf.planes,pf.facets);return 0;
}
