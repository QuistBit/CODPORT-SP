/* Ordered native v14 character assembly: owned source + detached frame.
 * Current base BODY bone transforms are supplied by the caller; this does not
 * sample animation, invent a root, certify currentness, or enqueue rendering.
 * Empty-tag parts meld via the existing exact R104 C1DT named-bone map.
 * Explicit tags must exist; unmelded secondary roots fail. All weighted source
 * influences are retained. Captures deep-own pose, geometry, indices/materials
 * until the caller's frame arena resets, even after resources are freed. */
#ifndef COD1_ACTOR_ASSEMBLY14_H
#define COD1_ACTOR_ASSEMBLY14_H
#include "cod1_native_source14.h"
#include "cod1_skin_pose.h"
#include "cod1_native_snapshot_arena.h"
#include "../qcommon/cod1_dobj_topology.h"
#include <stdint.h>
#define C1AA14_PARTS C1DT_MAX_MODELS
#define C1AA14_LIMIT (64u*1024u*1024u)
typedef struct {
 const char *modelName,*partsName,*tag;
 const void *model,*parts,*mesh;size_t modelBytes,partsBytes,meshBytes;
} c1aa14PartInput_t;
typedef struct {
 c1mrResource_t source;xm14Model model;cnSourcePart_t mesh;
 char tag[C1MR_NAME_MAX];
} c1aa14Part_t;
typedef struct {unsigned partCount;c1dtTopology_t topology;c1aa14Part_t part[C1AA14_PARTS];} c1aa14Resource_t;
typedef struct {float xyz[3],normal[3],uv[2];} c1aa14Vertex_t;
typedef struct {unsigned part,localSurface;char material[XM14_NAME];int vertices,indexes;c1aa14Vertex_t *vertex;unsigned short *index;} c1aa14Surface_t;
typedef struct {
 unsigned partCount,boneCount,surfaceCount,firstBone[C1AA14_PARTS],partBones[C1AA14_PARTS];
 char modelName[C1AA14_PARTS][C1MR_NAME_MAX],boneName[C1DT_MAX_BONES][C1MR_NAME_MAX];
 cv14Transform world[C1DT_MAX_BONES];c1aa14Surface_t *surface;float bounds[2][3];
} c1aa14Frame_t;
static int C1AA14_Overlap(const void *a,size_t an,const void *b,size_t bn){uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;if(!a||!b||!an||!bn)return 0;return x<=y?y-x<an:x-y<bn;}
static size_t C1AA14_Align(size_t x){return (x+15u)&~(size_t)15u;}
static int C1AA14_Add(size_t *total,size_t bytes){if(bytes>C1AA14_LIMIT||*total>C1AA14_LIMIT-bytes)return 0;*total+=bytes;return 1;}
static int C1AA14_TransformValid(const cv14Transform *t){double n=0;int k;
 for(k=0;k<3;k++)if(!(t->p[k]>=-1000000.f&&t->p[k]<=1000000.f))return 0;
 for(k=0;k<4;k++){if(!(t->q[k]>=-1.001f&&t->q[k]<=1.001f))return 0;n+=(double)t->q[k]*t->q[k];}
 return n>=.999&&n<=1.001;
}
static void C1AA14_Free(c1aa14Resource_t *r){unsigned i;if(r){for(i=0;i<C1AA14_PARTS;i++)CN_SourceFree(&r->part[i].mesh);free(r);}}
static int C1AA14_Create(const c1aa14PartInput_t *inputs,unsigned count,c1aa14Resource_t **out){
 c1aa14Resource_t *r;c1dtModelInput_t in[C1AA14_PARTS];unsigned i,j;size_t total;int v;
 if(!inputs||!count||count>C1AA14_PARTS||!out||*out||C1AA14_Overlap(out,sizeof(*out),inputs,count*sizeof(*inputs)))return 0;
 for(i=0;i<count;i++)if(!inputs[i].model||!inputs[i].parts||!inputs[i].mesh||!inputs[i].modelBytes||!inputs[i].partsBytes||!inputs[i].meshBytes||
 inputs[i].modelBytes>16u*1024u*1024u||inputs[i].partsBytes>16u*1024u*1024u||inputs[i].meshBytes>16u*1024u*1024u||
 C1AA14_Overlap(out,sizeof(*out),inputs[i].model,inputs[i].modelBytes)||C1AA14_Overlap(out,sizeof(*out),inputs[i].parts,inputs[i].partsBytes)||C1AA14_Overlap(out,sizeof(*out),inputs[i].mesh,inputs[i].meshBytes))return 0;
 r=(c1aa14Resource_t*)calloc(1,sizeof(*r));if(!r)return 0;total=sizeof(*r);r->partCount=count;
 for(i=0;i<count;i++){
  c1aa14Part_t *p=&r->part[i];const c1aa14PartInput_t *src=inputs+i;
  if(!C1MR_DecodeResource(src->modelName,src->model,src->modelBytes,src->partsName,src->parts,src->partsBytes,&p->source)||
     !XM14_Model(src->model,src->modelBytes,&p->model)||strcmp(p->model.lod[0],p->source.partsName)||
     !CV14_Parts(src->parts,src->partsBytes,&p->mesh.bind)||p->mesh.bind.n!=(int)p->source.skeleton.boneCount||
     (src->tag&&!C1DT_String(src->tag)))goto bad;
  if(src->tag)memcpy(p->tag,src->tag,strlen(src->tag)+1);
  for(j=0;j<p->source.skeleton.boneCount;j++)if(strcmp(p->mesh.bind.name[j],p->source.skeleton.boneName[j])||p->mesh.bind.parent[j]!=p->source.skeleton.parent[j]||
     !C1AA14_TransformValid(&p->mesh.bind.local[j]))goto bad;
  if(!CN_SourceParseMesh(src->mesh,src->meshBytes,&p->mesh.bind,&p->mesh)||p->mesh.numSurfaces!=p->model.ns[0])goto bad;
  for(j=0;j<(unsigned)p->mesh.numSurfaces;j++){
   const cnSourceSurface_t *s=&p->mesh.surfaces[j];
   if(!p->model.skin[0][j][0]||!C1AA14_Add(&total,(size_t)s->numVerts*sizeof(*s->verts))||!C1AA14_Add(&total,(size_t)s->numIndexes*sizeof(*s->indexes)))goto bad;
   for(v=0;v<s->numVerts;v++)if(!s->verts[v].numWeights||s->verts[v].numWeights>CN_SOURCE_MAX_WEIGHTS)goto bad;
  }
  in[i].resource=&p->source;in[i].tagName=p->tag;
 }
 if(C1DT_Build(in,count,&r->topology)!=C1DT_OK||r->topology.modelCount!=count||r->topology.missingTagInputMask||r->topology.unmeldedRootInputMask)goto bad;
 *out=r;return 1;
bad:C1AA14_Free(r);return 0;
}
static int C1AA14_Bone(const c1aa14Resource_t *r,unsigned index,cv14Transform *world,unsigned char *state){
 const c1dtBone_t *b;const c1aa14Part_t *p;cv14Transform value;
 if(index>=r->topology.boneCount||state[index]==1)return 0;if(state[index]==2)return 1;state[index]=1;b=&r->topology.bones[index];
 if(b->modelIndex>=r->partCount)return 0;p=&r->part[b->modelIndex];if(b->localBone>=(unsigned)p->mesh.bind.n)return 0;
 if(b->duplicateBone>=0){if(!C1AA14_Bone(r,(unsigned)b->duplicateBone,world,state))return 0;value=world[b->duplicateBone];}
 else {if(b->parentBone<0||!C1AA14_Bone(r,(unsigned)b->parentBone,world,state))return 0;CV14_Compose(&world[b->parentBone],&p->mesh.bind.local[b->localBone],&value);}
 if(!C1AA14_TransformValid(&value))return 0;world[index]=value;state[index]=2;return 1;
}

#ifdef C1AA14_PARALLEL
#include "../qcommon/cod1_jobs.h"
typedef struct {const c1aa14Resource_t *source;const cnSkinPose_t *pose;c1aa14Surface_t *dest;unsigned surfaces;} c1aa14SkinJob;
static int C1AA14_SkinJob(void *context,unsigned first,unsigned end){
 c1aa14SkinJob *job=(c1aa14SkinJob*)context;unsigned j,s,base=0;int k;float norm;
 for(s=0;s<job->surfaces && base<end;s++){
 const c1aa14Surface_t *surface=job->dest+s;
 const cnSourceSurface_t *source=job->source->part[surface->part].mesh.surfaces+surface->localSurface;
 unsigned n=(unsigned)job->dest[s].vertices,lo=first>base?first-base:0,hi=end-base<n?end-base:n;
 for(j=lo;j<hi;j++){
  c1aa14Vertex_t *v=job->dest[s].vertex+j;
  if(!CN_SourceSkinPrepared(source->verts+j,job->pose+surface->part,v->xyz,v->normal))return 0;
  norm=0;for(k=0;k<3;k++)norm+=v->normal[k]*v->normal[k];if(!(norm>1e-15f&&norm<1e15f))return 0;norm=sqrtf(norm);
  for(k=0;k<3;k++)v->normal[k]/=norm;
  memcpy(v->uv,source->verts[j].uv,8);
 }base+=n;
 }return 1;
}
/* All parts of one object now share a join. Up to eight bounded skeletons
 * and prepared poses live on the capture caller's stack, never a worker's.
 * This is about 265 KiB on x86/SSE builds; the executable reserves an 8 MiB main
 * stack. No allocation or mutable source data is exposed to the workers. */
static int C1AA14_TrySkin(const c1aa14Resource_t *r,const c1dtTopology_t *topology,
                        const cv14Transform *world,c1aa14Frame_t *frame,
                        unsigned char *cursor,unsigned vertices){
 cv14Skeleton posed[C1AA14_PARTS];cnSkinPose_t pose[C1AA14_PARTS];
 c1aa14SkinJob job;c1jFp before;unsigned p,s,i,at=0;
 C1J_FpGet(&before);
 if((before.control&63)!=63||(before.mxcsr&0x1f80)!=0x1f80)return 0;
 for(p=0;p<r->partCount;p++){
  const c1aa14Part_t *part=&r->part[p];unsigned first=topology->models[p].firstBone;
  posed[p]=part->mesh.bind;frame->firstBone[p]=first;frame->partBones[p]=topology->models[p].boneCount;
  strcpy(frame->modelName[p],part->source.modelName);
  for(i=0;i<(unsigned)posed[p].n;i++){posed[p].world[i]=world[first+i];strcpy(frame->boneName[first+i],posed[p].name[i]);}
  if(!CN_SourcePrepareSkinPose(posed+p,pose+p))goto fallback;
  for(s=0;s<(unsigned)part->mesh.numSurfaces;s++,at++){
   const cnSourceSurface_t *source=&part->mesh.surfaces[s];c1aa14Surface_t *dest=frame->surface+at;
   dest->part=p;dest->localSurface=s;strcpy(dest->material,part->model.skin[0][s]);dest->vertices=source->numVerts;dest->indexes=source->numIndexes;
   dest->vertex=(c1aa14Vertex_t*)cursor;cursor+=C1AA14_Align((size_t)dest->vertices*sizeof(*dest->vertex));
   dest->index=(unsigned short*)cursor;cursor+=C1AA14_Align((size_t)dest->indexes*sizeof(*dest->index));
   memcpy(dest->index,source->indexes,(size_t)dest->indexes*sizeof(*dest->index));
  }
 }
 job.source=r;job.pose=pose;job.dest=frame->surface;job.surfaces=at;
 if(C1J_ParallelFor(vertices,512,C1AA14_SkinJob,&job))return 1;
fallback:C1J_FpRestore(&before,0,0);return 0;
}
#endif

static c1aa14Frame_t *C1AA14_CaptureCheckedMode(const c1aa14Resource_t *r,const cv14Transform *currentBodyWorld,unsigned bodyBones,cnsArena_t *arena,int full,int sealed,const unsigned char (*weightProof)[XM14_SURFS]){
 c1dtModelInput_t inputs[C1AA14_PARTS];c1dtTopology_t topology;cv14Transform world[C1DT_MAX_BONES];unsigned char state[C1DT_MAX_BONES];
 c1aa14Frame_t *frame;unsigned char *cursor;size_t bytes,before;unsigned p,s,i,ns=0,at=0,totalVertices=0;int j,k,w,parallelDone=0;float norm;cv14Skeleton posed;cnSkinPose_t skinPose;
 unsigned proofBit=0;
#if defined(_MSC_VER) && defined(_M_IX86)
 if(weightProof){unsigned short cw;__asm fnstcw cw
  proofBit=(cw&0x0300)==0?2:((cw&0x0300)==0x0200||(cw&0x0300)==0x0300?1:0);
 }
#endif
 if(!r||!currentBodyWorld||!arena||!arena->data||!r->partCount||r->partCount>C1AA14_PARTS||arena->used>arena->capacity||
    (uintptr_t)arena->data>UINTPTR_MAX-arena->capacity||C1AA14_Overlap(arena->data,arena->capacity,r,sizeof(*r))||C1AA14_Overlap(arena->data,arena->capacity,arena,sizeof(*arena))||
    bodyBones!=(full?r->topology.boneCount:r->part[0].source.skeleton.boneCount)||!bodyBones||bodyBones>=C1DT_MAX_BONES)return NULL;
 if(sealed)topology=r->topology;else {
 for(p=0;p<r->partCount;p++){inputs[p].resource=&r->part[p].source;inputs[p].tagName=r->part[p].tag;}
 if(C1DT_Build(inputs,r->partCount,&topology)!=C1DT_OK||memcmp(&topology,&r->topology,sizeof(topology)))return NULL;
 }
 memset(state,0,sizeof(state));memcpy(world,currentBodyWorld,bodyBones*sizeof(*world));for(i=0;i<bodyBones;i++){if(!C1AA14_TransformValid(&world[i]))return NULL;state[i]=2;}
 if(full)for(i=0;i<bodyBones;i++)if(topology.bones[i].duplicateBone>=0&&memcmp(world+i,world+topology.bones[i].duplicateBone,sizeof(*world)))return NULL;
 for(i=bodyBones;i<topology.boneCount;i++)if(!C1AA14_Bone(r,i,world,state))return NULL;
 bytes=C1AA14_Align(sizeof(*frame));
 for(p=0;p<r->partCount;p++){
  const c1aa14Part_t *part=&r->part[p];if(part->mesh.numSurfaces<1||part->mesh.numSurfaces>XM14_SURFS||part->mesh.numSurfaces!=part->model.ns[0]||part->mesh.bind.n!=(int)part->source.skeleton.boneCount)return NULL;
  ns+=(unsigned)part->mesh.numSurfaces;
  for(s=0;s<(unsigned)part->mesh.numSurfaces;s++){
   const cnSourceSurface_t *surface=&part->mesh.surfaces[s];
   if(!surface->verts||!surface->indexes||surface->numVerts<1||surface->numVerts>CV14_VERTS||surface->numIndexes<3||surface->numIndexes>CV14_TRIS*3||surface->numIndexes%3||
      !memchr(part->model.skin[0][s],0,XM14_NAME)||!part->model.skin[0][s][0]||C1AA14_Overlap(arena->data,arena->capacity,surface->verts,surface->numVerts*sizeof(*surface->verts))||
      C1AA14_Overlap(arena->data,arena->capacity,surface->indexes,surface->numIndexes*sizeof(*surface->indexes))||
      !C1AA14_Add(&bytes,C1AA14_Align((size_t)surface->numVerts*sizeof(c1aa14Vertex_t)))||!C1AA14_Add(&bytes,C1AA14_Align((size_t)surface->numIndexes*sizeof(unsigned short))))return NULL;
   totalVertices+=(unsigned)surface->numVerts;
   if(!sealed)for(j=0;j<surface->numIndexes;j++)if(surface->indexes[j]>=surface->numVerts)return NULL;
   if(!weightProof||!(weightProof[p][s]&proofBit))for(j=0;j<surface->numVerts;j++){
    const cnSourceVertex_t *vertex=&surface->verts[j];double sum=0;
    if(!vertex->numWeights||vertex->numWeights>CN_SOURCE_MAX_WEIGHTS||vertex->normalBone>=(unsigned)part->mesh.bind.n)return NULL;
    for(w=0;w<vertex->numWeights;w++){const cnSourceWeight_t *weight=vertex->weight+w;if(weight->bone>=(unsigned)part->mesh.bind.n||!(weight->weight>=0&&weight->weight<=1))return NULL;sum+=weight->weight;}
    if(!(sum>=.999&&sum<=1.001))return NULL;
   }
  }
 }
 if(!C1AA14_Add(&bytes,C1AA14_Align((size_t)ns*sizeof(c1aa14Surface_t))))return NULL;
 before=arena->used;frame=(c1aa14Frame_t*)CNS_Alloc(arena,bytes,16);if(!frame)return NULL;memset(frame,0,bytes);frame->partCount=r->partCount;frame->boneCount=topology.boneCount;frame->surfaceCount=ns;
 memcpy(frame->world,world,topology.boneCount*sizeof(*world));frame->surface=(c1aa14Surface_t*)((unsigned char*)frame+C1AA14_Align(sizeof(*frame)));cursor=(unsigned char*)frame+C1AA14_Align(sizeof(*frame))+C1AA14_Align((size_t)ns*sizeof(*frame->surface));
 for(k=0;k<3;k++){frame->bounds[0][k]=FLT_MAX;frame->bounds[1][k]=-FLT_MAX;}
#ifdef C1AA14_PARALLEL
 if(totalVertices>512&&C1J_Enabled())parallelDone=C1AA14_TrySkin(r,&topology,world,frame,cursor,totalVertices);
#endif
 for(p=0;p<r->partCount;p++){
  const c1aa14Part_t *part=&r->part[p];unsigned first=topology.models[p].firstBone,firstSurface=at;
  if(!parallelDone){posed=part->mesh.bind;
  frame->firstBone[p]=first;frame->partBones[p]=topology.models[p].boneCount;strcpy(frame->modelName[p],part->source.modelName);
  for(i=0;i<(unsigned)posed.n;i++){posed.world[i]=world[first+i];strcpy(frame->boneName[first+i],posed.name[i]);}
  if(!CN_SourcePrepareSkinPose(&posed,&skinPose))goto bad;
  for(s=0;s<(unsigned)part->mesh.numSurfaces;s++,at++){
   const cnSourceSurface_t *source=&part->mesh.surfaces[s];c1aa14Surface_t *dest=frame->surface+at;dest->part=p;dest->localSurface=s;strcpy(dest->material,part->model.skin[0][s]);dest->vertices=source->numVerts;dest->indexes=source->numIndexes;
   dest->vertex=(c1aa14Vertex_t*)cursor;cursor+=C1AA14_Align((size_t)dest->vertices*sizeof(*dest->vertex));dest->index=(unsigned short*)cursor;cursor+=C1AA14_Align((size_t)dest->indexes*sizeof(*dest->index));memcpy(dest->index,source->indexes,(size_t)dest->indexes*sizeof(*dest->index));
  }
  }else at+=(unsigned)part->mesh.numSurfaces;
   /* Immutable source/pose and disjoint vertex slots until join. Bounds retain
    * the original vertex order, including the tie behaviour of signed zero. */
  for(s=0;s<(unsigned)part->mesh.numSurfaces;s++){
   const cnSourceSurface_t *source=&part->mesh.surfaces[s];c1aa14Surface_t *dest=frame->surface+firstSurface+s;
   for(j=0;j<dest->vertices;j++){
    c1aa14Vertex_t *v=dest->vertex+j;if(!parallelDone){if(!CN_SourceSkinPrepared(source->verts+j,&skinPose,v->xyz,v->normal))goto bad;
    norm=0;for(k=0;k<3;k++)norm+=v->normal[k]*v->normal[k];if(!(norm>1e-15f&&norm<1e15f))goto bad;norm=sqrtf(norm);
    for(k=0;k<3;k++)v->normal[k]/=norm;memcpy(v->uv,source->verts[j].uv,8);}
    for(k=0;k<3;k++){if(v->xyz[k]<frame->bounds[0][k])frame->bounds[0][k]=v->xyz[k];if(v->xyz[k]>frame->bounds[1][k])frame->bounds[1][k]=v->xyz[k];}
   }
  }
 }
 return frame;
bad:arena->used=before;return NULL;
}
/* The raw API retains every topology/index check. The registered path is
 * reserved for separately owned OS-protected source allocations. All current
 * pose, arena, skinning and rollback checks remain common. Weight arithmetic
 * can only be elided by the separate read-only exact-arithmetic proof. */
static c1aa14Frame_t *C1AA14_CaptureMode(const c1aa14Resource_t*r,const cv14Transform*pose,unsigned bones,cnsArena_t*arena,int full){return C1AA14_CaptureCheckedMode(r,pose,bones,arena,full,0,NULL);}
static c1aa14Frame_t *C1AA14_Capture(const c1aa14Resource_t*r,const cv14Transform*body,unsigned bones,cnsArena_t*arena){return C1AA14_CaptureMode(r,body,bones,arena,0);}
/* Version2 supplies every actual ordered DObj bone, including animated HEAD.
 * No secondary bind composition is performed a second time. */
static c1aa14Frame_t *C1AA14_CaptureFull(const c1aa14Resource_t*r,const cv14Transform*pose,unsigned bones,cnsArena_t*arena){return C1AA14_CaptureMode(r,pose,bones,arena,1);}
static int C1AA14_FindTag(const c1aa14Frame_t *frame,unsigned part,const char *name,cv14Transform *out){unsigned i,first;
 if(!frame||!name||!out||part>=frame->partCount||part>=C1AA14_PARTS||frame->boneCount>=C1DT_MAX_BONES||strncmp(name,"tag_",4)||!C1DT_String(name))return 0;first=frame->firstBone[part];
 if(first>frame->boneCount||frame->partBones[part]>frame->boneCount-first)return 0;
 for(i=frame->partBones[part];i>0;i--)if(!strcmp(name,frame->boneName[first+i-1])){*out=frame->world[first+i-1];return 1;}return 0;
}
#endif
