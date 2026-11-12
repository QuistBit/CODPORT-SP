/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef V59_INTERNAL_H
#define V59_INTERNAL_H
#define V59_EMBEDDED
#include "v59_collision.h"

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <setjmp.h>
#include <stdio.h>
#include <limits.h>
static __inline float fa(float a,float b){volatile float r=a+b;return r;}
static __inline float fs(float a,float b){volatile float r=a-b;return r;}
static __inline float fm(float a,float b){volatile float r=a*b;return r;}
static __inline float fd(float a,float b){volatile float r=a/b;return r;}
static __inline float dp(const float*a,const float*b,int i,int j,int k){return fa(fa(fm(a[i],b[i]),fm(a[j],b[j])),fm(a[k],b[k]));}
static __inline float dot(const float*a,const float*b){return dp(a,b,2,1,0);}
static __inline void cp(const float*a,float*b){memcpy(b,a,12);}
static __inline void minus(const float*a,const float*b,float*c){int i;for(i=0;i<3;i++)c[i]=fs(a[i],b[i]);}
static __inline void cross(const float*a,const float*b,float*c){int i;for(i=0;i<3;i++)c[i]=fs(fm(a[(i+1)%3],b[(i+2)%3]),fm(a[(i+2)%3],b[(i+1)%3]));}
static __inline float norm(float*v){float n=sqrtf(dp(v,v,0,1,2));int i;if(n)for(i=0;i<3;i++)v[i]=fm(v[i],fd(1,n));return n;}
typedef struct {float n[3],d;int material,axis;float bound,sign;} brush_plane;
typedef struct {uint32_t flags,contents;} material;
typedef struct {uint32_t contents;float mins[3],maxs[3];int n;brush_plane *planes;} brush;
typedef struct {float plane[4],bary[2][4];int vertices[3],edges[3];} triangle;
typedef struct {float origin[3],basis[2][3],direction[3],length;} edge;
typedef struct {int nt,ne,nv,down;triangle *tri;edge *edges;float (*vertices)[3];} mesh;
typedef struct {float p[4];int signbits;} curve_plane;
typedef struct {int plane,n,planes[26],inward[26],noadjust[26];} facet;
typedef struct {int np,nf;curve_plane *planes;facet *facets;} curve;
typedef struct {int material,type;uint32_t contents;float bounds[6];mesh m;curve c;} surface;
typedef struct {int v[9];} node;
typedef struct {float bounds[6];uint32_t ranges[6];} model;
typedef struct allocation {struct allocation *next;void *ptr;} allocation;
struct v59_world {allocation *allocs;int nm,nb,ns,np,nn,nl,nbr,nsr,nmodels;
 material *materials;brush *brushes;surface *surfaces;float (*planes)[4];node *nodes,*leaves;
 uint32_t *brushrefs,*surfacerefs;model *models;int maxedges,maxvertices,maxdepth;};
typedef struct {float start[3],end[3],delta[3],delta2,size0[3],size1[3],corners[8][3],max_offset;
 float bmins[3],bmaxs[3];uint32_t mask,point;float fraction,endpos[3],normal[3];
 uint32_t flags,contents,material;unsigned char tail[6],allsolid,startsolid;
 uint32_t capsule;float radius,halfheight,offset[3],extents[3];} trace_work;
typedef char tw_size_check[sizeof(trace_work)==280?1:-1];
struct v59_context {const v59_world *w;trace_work prep,t;int stationary;
 unsigned char *brushseen,*surfaceseen,*edgeseen,*vertexseen;
 int (*events)[2],*gathered;size_t nevents,eventcap,nleaves;

};
int v59_curve_build(int w,int h,int tolerance,const float *points,curve *,float bounds[6],char *,size_t);
void v59_narrow(v59_context *,int kind,int index,int stationary);
#endif
