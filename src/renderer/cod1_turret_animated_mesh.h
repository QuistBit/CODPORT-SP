#ifndef COD1_TURRET_ANIMATED_MESH_H
#define COD1_TURRET_ANIMATED_MESH_H
#include "cod1_turret_mesh.h"
#include "../qcommon/cod1_turret_animated_pose14.h"
static c1tmFrame_t *C1TAM_CapturePose(const c1tmResource_t *r,const float angles[3],const c1tpStaticPose_t *inputPose,cnsArena_t *arena){
 c1tmFrame_t *frame;c1tpStaticPose_t pose;cv14Skeleton sk;float copiedAngles[3],n;size_t bytes,before;unsigned char *cursor;int i,j,k;
 if(!r||!angles||!inputPose||!arena||!arena->data||arena->used>arena->capacity||r->mesh.numSurfaces<1||r->mesh.numSurfaces>XM14_SURFS||r->mesh.numSurfaces!=r->model.ns[0]||r->mesh.bind.n!=13||
 C1TM_Overlap(arena->data,arena->capacity,r,sizeof(*r))||C1TM_Overlap(arena->data,arena->capacity,arena,sizeof(*arena)))return NULL;
 memcpy(copiedAngles,angles,12);pose=*inputPose;
 if(pose.bones!=13||pose.flash<0||pose.flash>=13||strcmp(r->bones.skeleton.boneName[pose.flash],"tag_flash")||C1TM_Overlap(arena->data,arena->capacity,inputPose,sizeof(*inputPose)))return NULL;
 for(i=0;i<3;i++)if(!C1TP_Finite(copiedAngles[i],3600))return NULL;
 for(i=0;i<13;i++)for(k=0;k<3;k++)if(!C1TP_Finite(pose.p[i][k],1000000))return NULL;
 sk=r->mesh.bind;
 for(i=0;i<13;i++){
  if(strcmp(sk.name[i],r->bones.skeleton.boneName[i])||sk.parent[i]!=r->bones.skeleton.parent[i])return NULL;
  memcpy(sk.world[i].p,pose.p[i],12);n=0;for(k=0;k<4;k++)n+=pose.q[i][k]*pose.q[i][k];if(!(n>1e-15f&&n<16))return NULL;n=sqrtf(n);for(k=0;k<4;k++)sk.world[i].q[k]=pose.q[i][k]/n;
 }
 bytes=C1TM_Align(sizeof(*frame));
 for(i=0;i<r->mesh.numSurfaces;i++){
  const cnSourceSurface_t *s=&r->mesh.surfaces[i];
  if(!s->verts||!s->indexes||s->numVerts<1||s->numVerts>CV14_VERTS||s->numIndexes<3||s->numIndexes>CV14_TRIS*3||s->numIndexes%3||!memchr(r->model.skin[0][i],0,XM14_NAME)||!r->model.skin[0][i][0]||
  C1TM_Overlap(arena->data,arena->capacity,s->verts,s->numVerts*sizeof(*s->verts))||C1TM_Overlap(arena->data,arena->capacity,s->indexes,s->numIndexes*sizeof(*s->indexes)))return NULL;
  for(j=0;j<s->numIndexes;j++)if(s->indexes[j]>=s->numVerts)return NULL;
  for(j=0;j<s->numVerts;j++)if(s->verts[j].numWeights!=1||s->verts[j].weight[0].weight!=1)return NULL;
  bytes+=C1TM_Align((size_t)s->numVerts*sizeof(c1tmVertex_t))+C1TM_Align((size_t)s->numIndexes*sizeof(unsigned short));
 }
 before=arena->used;frame=(c1tmFrame_t*)CNS_Alloc(arena,bytes,16);if(!frame)return NULL;memset(frame,0,bytes);
 frame->surfaces=r->mesh.numSurfaces;frame->pose=pose;memcpy(frame->angles,copiedAngles,12);cursor=(unsigned char*)frame+C1TM_Align(sizeof(*frame));
 for(k=0;k<3;k++){frame->bounds[0][k]=FLT_MAX;frame->bounds[1][k]=-FLT_MAX;}
 for(i=0;i<r->mesh.numSurfaces;i++){
  const cnSourceSurface_t *src=&r->mesh.surfaces[i];c1tmSurface_t *dst=&frame->surface[i];strcpy(dst->material,r->model.skin[0][i]);dst->vertices=src->numVerts;dst->indexes=src->numIndexes;
  dst->vertex=(c1tmVertex_t*)cursor;cursor+=C1TM_Align((size_t)dst->vertices*sizeof(*dst->vertex));dst->index=(unsigned short*)cursor;cursor+=C1TM_Align((size_t)dst->indexes*sizeof(*dst->index));memcpy(dst->index,src->indexes,dst->indexes*sizeof(*dst->index));
  for(j=0;j<dst->vertices;j++){
   c1tmVertex_t *v=&dst->vertex[j];if(!CN_SourceSkinVertex(&src->verts[j],&sk,v->xyz,v->normal))goto bad;
   n=0;for(k=0;k<3;k++)n+=v->normal[k]*v->normal[k];if(!(n>1e-15f&&n<1e15f))goto bad;n=sqrtf(n);for(k=0;k<3;k++){v->normal[k]/=n;if(v->xyz[k]<frame->bounds[0][k])frame->bounds[0][k]=v->xyz[k];if(v->xyz[k]>frame->bounds[1][k])frame->bounds[1][k]=v->xyz[k];}
   memcpy(v->uv,src->verts[j].uv,8);
  }
 }
 return frame;
bad:arena->used=before;return NULL;
}
#endif
