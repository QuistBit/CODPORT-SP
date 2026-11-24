/* Registered renderer resources own this allocation until reset. Their complete
 * topology and mesh arrays are protected read-only before publication. Raw
 * resources keep the original validation path, including mutation refusal. */
#ifndef COD1_ACTOR_SEALED14_H
#define COD1_ACTOR_SEALED14_H
#include "cod1_actor_assembly14.h"
typedef struct {void *storage;size_t bytes;const c1aa14Resource_t *resource;} c1aa14Seal_t;
#if defined(_WIN32)
#include <windows.h>
#ifdef small
#undef small
#endif
#endif

/* A conservative sufficient proof, not a replacement validator. Every admitted
 * weight is positive normal or signed zero and <= 1. Its exact integer value
 * in units of 2^-49 fits here, as do all nine partial sums. Thus binary64 adds
 * are exact under every rounding mode. Flag 2 additionally proves each partial
 * sum representable at 24-bit precision. No FP instruction runs in this proof.
 * The narrower final interval lies strictly inside the raw [.999,1.001]. */
static unsigned char C1AA14_WeightProof(const cnSourceSurface_t *surface,unsigned bones){
 int j,w;unsigned char flags=3;
 for(j=0;j<surface->numVerts;j++){
  const cnSourceVertex_t *vertex=surface->verts+j;uint64_t sum=0;
  if(!vertex->numWeights||vertex->numWeights>CN_SOURCE_MAX_WEIGHTS||vertex->normalBone>=bones)return 0;
  for(w=0;w<vertex->numWeights;w++){
   const cnSourceWeight_t *weight=vertex->weight+w;uint32_t bits,absolute,exponent;uint64_t reduced;
   if(weight->bone>=bones)return 0;memcpy(&bits,&weight->weight,4);absolute=bits&0x7fffffffu;
   if(absolute){
    if(bits>0x3f800000u)return 0;exponent=bits>>23;
    if(exponent<101)return 0;
    sum+=(uint64_t)((bits&0x7fffffu)|0x800000u)<<(exponent-101);
   }
   reduced=sum;if(reduced){while(!(reduced&1))reduced>>=1;if(reduced>0xffffffu)flags&=1;}
  }
  if(sum<((UINT64_C(1)<<49)-(UINT64_C(1)<<37))||sum>((UINT64_C(1)<<49)+(UINT64_C(1)<<37)))return 0;
 }return flags;
}

static void C1AA14_Unseal(c1aa14Seal_t *seal){
 if(!seal)return;
#if defined(_WIN32)
 if(seal->storage)VirtualFree(seal->storage,0,0x8000);
#endif
 memset(seal,0,sizeof(*seal));
}
static int C1AA14_Seal(const c1aa14Resource_t *source,c1aa14Seal_t *out){
#if defined(_WIN32)
 c1aa14Resource_t *copy;unsigned char *cursor;unsigned char (*proofs)[XM14_SURFS];size_t bytes=C1AA14_Align(sizeof(*copy))+C1AA14_Align(C1AA14_PARTS*XM14_SURFS);
 c1dtModelInput_t inputs[C1AA14_PARTS];c1dtTopology_t topology;unsigned p,s;int j;unsigned long old;
 if(!source||!out||out->storage||out->resource||out->bytes||!source->partCount||source->partCount>C1AA14_PARTS||C1AA14_Overlap(out,sizeof(*out),source,sizeof(*source)))return 0;
 for(p=0;p<source->partCount;p++){inputs[p].resource=&source->part[p].source;inputs[p].tagName=source->part[p].tag;}
 if(C1DT_Build(inputs,source->partCount,&topology)!=C1DT_OK||memcmp(&topology,&source->topology,sizeof(topology)))return 0;
 for(p=0;p<source->partCount;p++){
  const cnSourcePart_t *part=&source->part[p].mesh;
  if(part->numSurfaces<1||part->numSurfaces>XM14_SURFS)return 0;
  for(s=0;s<(unsigned)part->numSurfaces;s++){
   const cnSourceSurface_t *surface=part->surfaces+s;
   if(!surface->verts||!surface->indexes||surface->numVerts<1||surface->numVerts>CV14_VERTS||surface->numIndexes<3||surface->numIndexes>CV14_TRIS*3||surface->numIndexes%3)return 0;
   if(C1AA14_Overlap(out,sizeof(*out),surface->verts,(size_t)surface->numVerts*sizeof(*surface->verts))||C1AA14_Overlap(out,sizeof(*out),surface->indexes,(size_t)surface->numIndexes*sizeof(*surface->indexes)))return 0;
   if(!C1AA14_Add(&bytes,C1AA14_Align((size_t)surface->numVerts*sizeof(*surface->verts)))||!C1AA14_Add(&bytes,C1AA14_Align((size_t)surface->numIndexes*sizeof(*surface->indexes))))return 0;
   for(j=0;j<surface->numIndexes;j++)if(surface->indexes[j]>=surface->numVerts)return 0;
  }
 }
 copy=VirtualAlloc(NULL,bytes,0x3000,0x04);if(!copy)return 0;
 memcpy(copy,source,sizeof(*copy));proofs=(unsigned char (*)[XM14_SURFS])((unsigned char*)copy+C1AA14_Align(sizeof(*copy)));memset(proofs,0,C1AA14_PARTS*XM14_SURFS);cursor=(unsigned char*)proofs+C1AA14_Align(C1AA14_PARTS*XM14_SURFS);
 for(p=0;p<source->partCount;p++){
  copy->topology.models[p].resource=&copy->part[p].source;
  for(s=0;s<(unsigned)source->part[p].mesh.numSurfaces;s++){
   const cnSourceSurface_t *src=&source->part[p].mesh.surfaces[s];cnSourceSurface_t *dst=&copy->part[p].mesh.surfaces[s];
   proofs[p][s]=C1AA14_WeightProof(src,(unsigned)source->part[p].mesh.bind.n);
   dst->verts=(cnSourceVertex_t*)cursor;memcpy(cursor,src->verts,(size_t)src->numVerts*sizeof(*src->verts));cursor+=C1AA14_Align((size_t)src->numVerts*sizeof(*src->verts));
   dst->indexes=(unsigned short*)cursor;memcpy(cursor,src->indexes,(size_t)src->numIndexes*sizeof(*src->indexes));cursor+=C1AA14_Align((size_t)src->numIndexes*sizeof(*src->indexes));
  }
  /* Unused surface slots must not retain pointers to the source owner. */
  for(;s<XM14_SURFS;s++){copy->part[p].mesh.surfaces[s].verts=NULL;copy->part[p].mesh.surfaces[s].indexes=NULL;}
 }
 /* Unused parts are never inspected; discard their borrowed pointer slots. */
 for(;p<C1AA14_PARTS;p++){copy->topology.models[p].resource=NULL;for(s=0;s<XM14_SURFS;s++){copy->part[p].mesh.surfaces[s].verts=NULL;copy->part[p].mesh.surfaces[s].indexes=NULL;}}
 if((size_t)(cursor-(unsigned char*)copy)!=bytes||!VirtualProtect(copy,bytes,0x02,&old)){VirtualFree(copy,0,0x8000);return 0;}
 out->storage=copy;out->bytes=bytes;out->resource=copy;return 1;
#else
 (void)source;(void)out;return 0;
#endif
}
static c1aa14Frame_t *C1AA14_CaptureRegistered(const c1aa14Seal_t *seal,const c1aa14Resource_t *resource,const cv14Transform *pose,unsigned bones,cnsArena_t *arena,int full){
 if(!seal||!seal->storage)return C1AA14_CaptureMode(resource,pose,bones,arena,full);
 if(seal->storage!=seal->resource||seal->resource!=resource||seal->bytes<C1AA14_Align(sizeof(*resource))+C1AA14_Align(C1AA14_PARTS*XM14_SURFS)||!arena||C1AA14_Overlap(arena->data,arena->capacity,seal,sizeof(*seal))||C1AA14_Overlap(arena->data,arena->capacity,seal->storage,seal->bytes))return NULL;
 return C1AA14_CaptureCheckedMode(resource,pose,bones,arena,full,1,(const unsigned char (*)[XM14_SURFS])((const unsigned char*)seal->storage+C1AA14_Align(sizeof(*resource))));
}
#endif
