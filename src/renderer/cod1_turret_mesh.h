#ifndef COD1_TURRET_MESH_H
#define COD1_TURRET_MESH_H
/* Detached native MG42 weighted-resource/frame-capture dependency.
 * No renderer handle, GAME entity, snapshot, current-DObj certificate or fire
 * admission. Resources own decoded input; captured geometry owns its indices,
 * materials, pose and vertices in one caller frame-arena reservation. A frame
 * remains valid after resource/input release, until that arena is reset.
 * Single owner/capture thread; caller keeps resource alive during this call.
 * Admitted original LOD0 vertices each have one influence of weight1; general
 * multiweight blending is deliberately outside this narrow resource adapter. */
#include "cod1_native_source14.h"
#include "cod1_native_snapshot_arena.h"
#include "../game/bg_cod1_turret_pose.h"
#include "../qcommon/cod1_jobs.h"
typedef struct {c1mrResource_t bones;xm14Model model;cnSourcePart_t mesh;} c1tmResource_t;
typedef struct {float xyz[3],normal[3],uv[2];} c1tmVertex_t;
typedef struct {char material[XM14_NAME];int vertices,indexes;c1tmVertex_t *vertex;unsigned short *index;} c1tmSurface_t;
typedef struct {unsigned surfaces;c1tmSurface_t surface[XM14_SURFS];c1tpStaticPose_t pose;float angles[3],bounds[2][3];} c1tmFrame_t;
static void C1TM_Free(c1tmResource_t *r){if(r){CN_SourceFree(&r->mesh);free(r);}}
static int C1TM_Create(const void *model,size_t mn,const void *parts,size_t pn,const void *surfaces,size_t sn,c1tmResource_t **out){
 c1tmResource_t *r;C1TRModel tags;unsigned i;int j;
 if(!model||!parts||!surfaces||!out||*out)return 0;
 r=(c1tmResource_t*)calloc(1,sizeof(*r));if(!r)return 0;
 if(!C1MR_DecodeResource("MG42_bipod",model,mn,"MG42_bipod1",parts,pn,&r->bones)||r->bones.skeleton.boneCount!=13||r->bones.skeleton.rootCount!=1||!C1TR_Model(&r->bones,&tags)||
 !XM14_Model(model,mn,&r->model)||strcmp(r->model.lod[0],"MG42_bipod1")||!CV14_Parts(parts,pn,&r->mesh.bind)||r->mesh.bind.n!=13)goto bad;
 for(i=0;i<13;i++)if(strcmp(r->mesh.bind.name[i],r->bones.skeleton.boneName[i])||r->mesh.bind.parent[i]!=r->bones.skeleton.parent[i])goto bad;
 if(!CN_SourceParseMesh(surfaces,sn,&r->mesh.bind,&r->mesh)||r->mesh.numSurfaces!=r->model.ns[0])goto bad;
 for(i=0;i<(unsigned)r->mesh.numSurfaces;i++)for(j=0;j<r->mesh.surfaces[i].numVerts;j++)if(r->mesh.surfaces[i].verts[j].numWeights!=1||r->mesh.surfaces[i].verts[j].weight[0].weight!=1)goto bad;
 *out=r;return 1;
bad:C1TM_Free(r);return 0;
}
static int C1TM_Overlap(const void *a,size_t an,const void *b,size_t bn){uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;if(!a||!b||!an||!bn)return 0;return x<=y?y-x<an:x-y<bn;}
static size_t C1TM_Align(size_t n){return (n+15u)&~(size_t)15u;}
typedef struct {const c1tmResource_t *source;const cv14Skeleton *pose;c1tmFrame_t *dest;} c1tmSkinJob;
static int C1TM_SkinJob(void *context,unsigned first,unsigned end){
 const c1tmSkinJob *job=(const c1tmSkinJob*)context;unsigned s,j,base=0;int k;float n;
 for(s=0;s<job->dest->surfaces&&base<end;s++){
  const cnSourceSurface_t *src=&job->source->mesh.surfaces[s];c1tmSurface_t *dst=&job->dest->surface[s];
  unsigned lo=first>base?first-base:0,hi=end-base<(unsigned)dst->vertices?end-base:(unsigned)dst->vertices;
  for(j=lo;j<hi;j++){
   c1tmVertex_t *v=dst->vertex+j;
   if(!CN_SourceSkinVertex(src->verts+j,job->pose,v->xyz,v->normal))return 0;
   n=0;for(k=0;k<3;k++)n+=v->normal[k]*v->normal[k];if(!(n>1e-15f&&n<1e15f))return 0;
   n=sqrtf(n);for(k=0;k<3;k++)v->normal[k]/=n;
   memcpy(v->uv,src->verts[j].uv,8);
  }
  base+=(unsigned)dst->vertices;
 }
 return 1;
}
/* One batch covers every MG42 surface. Pose and destination reservation are
 * private to this synchronous capture. Bounds retain their serial order. */
static int C1TM_TrySkin(const c1tmResource_t *r,const cv14Skeleton *pose,
                       c1tmFrame_t *frame,unsigned char *cursor){
 c1tmSkinJob job;c1jFp before;unsigned total=0;int i;
 if(!C1J_Enabled())return 0;
 for(i=0;i<r->mesh.numSurfaces;i++)total+=(unsigned)r->mesh.surfaces[i].numVerts;
 if(total<=512)return 0;
 C1J_FpGet(&before);
 if((before.control&63)!=63||(before.mxcsr&0x1f80)!=0x1f80)return 0;
 for(i=0;i<r->mesh.numSurfaces;i++){
  const cnSourceSurface_t *src=&r->mesh.surfaces[i];c1tmSurface_t *dst=&frame->surface[i];
  strcpy(dst->material,r->model.skin[0][i]);dst->vertices=src->numVerts;dst->indexes=src->numIndexes;
  dst->vertex=(c1tmVertex_t*)cursor;cursor+=C1TM_Align((size_t)dst->vertices*sizeof(*dst->vertex));
  dst->index=(unsigned short*)cursor;cursor+=C1TM_Align((size_t)dst->indexes*sizeof(*dst->index));
  memcpy(dst->index,src->indexes,(size_t)dst->indexes*sizeof(*dst->index));
 }
 job.source=r;job.pose=pose;job.dest=frame;
 if(C1J_ParallelFor(total,512,C1TM_SkinJob,&job))return 1;
 C1J_FpRestore(&before,0,0);return 0;
}
static c1tmFrame_t *C1TM_Capture(const c1tmResource_t *r,const float angles[3],cnsArena_t *arena){
 c1tmFrame_t *frame;c1tpStaticPose_t pose;cv14Skeleton sk;float copiedAngles[3],n;size_t bytes,before;unsigned char *cursor;int i,j,k,parallelDone;
 if(!r||!angles||!arena||!arena->data||arena->used>arena->capacity||r->mesh.numSurfaces<1||r->mesh.numSurfaces>XM14_SURFS||r->mesh.numSurfaces!=r->model.ns[0]||r->mesh.bind.n!=13||
 C1TM_Overlap(arena->data,arena->capacity,r,sizeof(*r))||C1TM_Overlap(arena->data,arena->capacity,arena,sizeof(*arena)))return NULL;
 memcpy(copiedAngles,angles,12);if(!C1TP_StaticResource(&r->bones,copiedAngles,&pose))return NULL;
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
 parallelDone=C1TM_TrySkin(r,&sk,frame,cursor);
 for(i=0;i<r->mesh.numSurfaces;i++){
  const cnSourceSurface_t *src=&r->mesh.surfaces[i];c1tmSurface_t *dst=&frame->surface[i];strcpy(dst->material,r->model.skin[0][i]);dst->vertices=src->numVerts;dst->indexes=src->numIndexes;
  dst->vertex=(c1tmVertex_t*)cursor;cursor+=C1TM_Align((size_t)dst->vertices*sizeof(*dst->vertex));dst->index=(unsigned short*)cursor;cursor+=C1TM_Align((size_t)dst->indexes*sizeof(*dst->index));memcpy(dst->index,src->indexes,dst->indexes*sizeof(*dst->index));
  for(j=0;j<dst->vertices;j++){
   c1tmVertex_t *v=&dst->vertex[j];
   if(!parallelDone){
    if(!CN_SourceSkinVertex(&src->verts[j],&sk,v->xyz,v->normal))goto bad;
    n=0;for(k=0;k<3;k++)n+=v->normal[k]*v->normal[k];if(!(n>1e-15f&&n<1e15f))goto bad;n=sqrtf(n);for(k=0;k<3;k++){v->normal[k]/=n;if(v->xyz[k]<frame->bounds[0][k])frame->bounds[0][k]=v->xyz[k];if(v->xyz[k]>frame->bounds[1][k])frame->bounds[1][k]=v->xyz[k];}
    memcpy(v->uv,src->verts[j].uv,8);
   }else for(k=0;k<3;k++){if(v->xyz[k]<frame->bounds[0][k])frame->bounds[0][k]=v->xyz[k];if(v->xyz[k]>frame->bounds[1][k])frame->bounds[1][k]=v->xyz[k];}
  }
 }
 return frame;
bad:arena->used=before;return NULL;
}
#endif
