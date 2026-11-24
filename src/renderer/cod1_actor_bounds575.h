#ifndef COD1_ACTOR_BOUNDS575_H
#define COD1_ACTOR_BOUNDS575_H
#include "cod1_actor_assembly14.h"
/* Bounding only: the normal capture remains the authority for geometry.
 * Nonnegative skin weights with sum in [.999,1.001] put each vertex inside
 * the union of transformed influence boxes, expanded for the weight sum
 * and FP rounding. Oversized/unsupported inputs retain eager capture. */
typedef struct {const c1aa14Resource_t *resource;unsigned bones;unsigned char used[C1DT_MAX_BONES];float offset[C1DT_MAX_BONES][2][3];} c1aaBounds575_t;
static int C1AA575_Build(const c1aa14Resource_t*r,c1aaBounds575_t*out){
 unsigned p,s,i,w,b,k;double sum;
 if(!r||!out||!r->topology.boneCount||r->topology.boneCount>=C1DT_MAX_BONES)return 0;
 memset(out,0,sizeof(*out));out->bones=r->topology.boneCount;
 for(p=0;p<r->partCount;p++)for(s=0;s<(unsigned)r->part[p].mesh.numSurfaces;s++){
  const cnSourceSurface_t*sf=r->part[p].mesh.surfaces+s;
  for(i=0;i<(unsigned)sf->numVerts;i++){
   const cnSourceVertex_t*v=sf->verts+i;sum=0;
   if(!v->numWeights||v->numWeights>CN_SOURCE_MAX_WEIGHTS)return 0;
   for(w=0;w<v->numWeights;w++){
    const cnSourceWeight_t*t=v->weight+w;
    if(t->bone>=r->topology.models[p].boneCount||!(t->weight>=0&&t->weight<=1))return 0;
    sum+=t->weight;b=r->topology.models[p].firstBone+t->bone;if(b>=out->bones)return 0;
    for(k=0;k<3;k++){
     if(!(t->offset[k]>=-1000000&&t->offset[k]<=1000000))return 0;
     if(!out->used[b]||t->offset[k]<out->offset[b][0][k])out->offset[b][0][k]=t->offset[k];
     if(!out->used[b]||t->offset[k]>out->offset[b][1][k])out->offset[b][1][k]=t->offset[k];
    }out->used[b]=1;
   }if(!(sum>=.999&&sum<=1.001))return 0;
  }
 }
 out->resource=r;return 1;
}
static int C1AA575_PoseBounds(const c1aaBounds575_t*c,const c1aa14Resource_t*r,const cv14Transform*body,unsigned bones,int full,float bounds[2][3]){
 cv14Transform world[C1DT_MAX_BONES];unsigned char state[C1DT_MAX_BONES]={0};unsigned i,j,k,any=0;float v[3],point[3],margin,magnitude;
 if(!c||c->resource!=r||!r||c->bones!=r->topology.boneCount||!body||!bones||bones!=(full?c->bones:r->part[0].source.skeleton.boneCount))return 0;
 memcpy(world,body,bones*sizeof(*body));
 for(i=0;i<bones;i++){if(!C1AA14_TransformValid(world+i))return 0;state[i]=2;}
 if(full)for(i=0;i<bones;i++)if(r->topology.bones[i].duplicateBone>=0&&memcmp(world+i,world+r->topology.bones[i].duplicateBone,sizeof(*world)))return 0;
 for(i=bones;i<c->bones;i++)if(!C1AA14_Bone(r,i,world,state))return 0;
 for(k=0;k<3;k++){bounds[0][k]=FLT_MAX;bounds[1][k]=-FLT_MAX;}
 for(i=0;i<c->bones;i++)if(c->used[i]){
  any=1;for(j=0;j<8;j++){
   for(k=0;k<3;k++)v[k]=c->offset[i][(j>>k)&1][k];
   CN_SourceRotate3(world[i].q,v,point);
   for(k=0;k<3;k++){
    point[k]+=world[i].p[k];if(!(point[k]>=-100000000&&point[k]<=100000000))return 0;
    if(point[k]<bounds[0][k])bounds[0][k]=point[k];if(point[k]>bounds[1][k])bounds[1][k]=point[k];
   }
  }
 }
 if(!any)return 0;
 for(k=0;k<3;k++){magnitude=fmaxf(fabsf(bounds[0][k]),fabsf(bounds[1][k]));margin=1+magnitude*.01f;bounds[0][k]-=margin;bounds[1][k]+=margin;}
 return 1;
}
#endif
