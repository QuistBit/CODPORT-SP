#ifndef COD1_COLLISION_BLOCKS583_H
#define COD1_COLLISION_BLOCKS583_H
#include <stdlib.h>
#include <stdint.h>
/* Explicit CM world ownership. Registration borrows immutable triangles until
 * CM_ClearMap; arbitrary/unregistered arrays always retain the full scan.
 * Contiguous blocks preserve every original triangle and tie-breaking order. */
#define C1CB583_SIZE 64
#define C1CB583_GROUP_BLOCKS 16
#define C1CB583_GROUP_SIZE (C1CB583_SIZE * C1CB583_GROUP_BLOCKS)
typedef struct {float low[3],high[3];} c1cb583Bounds;
static struct {const cod1CollisionTri_t *owner;int count,blocks;c1cb583Bounds *bounds,*groups;} c1cb583;
static int C1CB583_NormalOrZero(const float *f){
 uint32_t bits;memcpy(&bits,f,4);bits&=0x7fffffffu;
 return !bits||(bits>=0x00800000u&&bits<0x7f800000u);
}
void COD1_CollisionIndexReset(void){
 free(c1cb583.groups);free(c1cb583.bounds);memset(&c1cb583,0,sizeof(c1cb583));
}
int COD1_CollisionIndexBuild(const cod1CollisionTri_t *triangles,int count){
 c1cb583Bounds *bounds;int blocks,i,j,b;
 COD1_CollisionIndexReset();
 if(!triangles||count<1)return 0;
 blocks=count/C1CB583_SIZE+(count%C1CB583_SIZE!=0);
 if((size_t)blocks>SIZE_MAX/sizeof(*bounds))return 0;
 bounds=malloc((size_t)blocks*sizeof(*bounds));if(!bounds)return 0;
 for(i=0;i<count;i++){
  b=i/C1CB583_SIZE;
  for(j=0;j<3;j++){
   float lo,hi;
   /* Unusual encodings keep the original FP comparison/exception behaviour. */
   if(!C1CB583_NormalOrZero(&triangles[i].mins[j])||!C1CB583_NormalOrZero(&triangles[i].maxs[j])){free(bounds);return 0;}
   lo=triangles[i].mins[j];hi=triangles[i].maxs[j];if(lo>hi){free(bounds);return 0;}
   if(!(i%C1CB583_SIZE)){bounds[b].low[j]=lo;bounds[b].high[j]=hi;}
   else {if(lo<bounds[b].low[j])bounds[b].low[j]=lo;if(hi>bounds[b].high[j])bounds[b].high[j]=hi;}
  }
 }
 /* Optional second level, over contiguous blocks. If allocation fails,
  * preserve the original block index rather than disabling acceleration. */
 if(blocks>=C1CB583_GROUP_BLOCKS){
  int groups=blocks/C1CB583_GROUP_BLOCKS+(blocks%C1CB583_GROUP_BLOCKS!=0);
  if((size_t)groups<=SIZE_MAX/sizeof(*bounds))
   c1cb583.groups=malloc((size_t)groups*sizeof(*bounds));
  if(c1cb583.groups)for(i=0;i<blocks;i++){
   c1cb583Bounds *g=&c1cb583.groups[i/C1CB583_GROUP_BLOCKS];
   if(!(i%C1CB583_GROUP_BLOCKS))*g=bounds[i];
   else for(j=0;j<3;j++){
    if(bounds[i].low[j]<g->low[j])g->low[j]=bounds[i].low[j];
    if(bounds[i].high[j]>g->high[j])g->high[j]=bounds[i].high[j];
   }
  }
 }
 c1cb583.owner=triangles;c1cb583.count=count;c1cb583.blocks=blocks;c1cb583.bounds=bounds;return 1;
}
static const c1cb583Bounds*C1CB583_Select(const cod1CollisionTri_t *triangles,int count,const float low[3],const float high[3]){
 int j;
 if(!c1cb583.bounds||triangles!=c1cb583.owner||count!=c1cb583.count)return NULL;
 for(j=0;j<3;j++)if(!C1CB583_NormalOrZero(low+j)||!C1CB583_NormalOrZero(high+j)||low[j]>high[j])return NULL;
 return c1cb583.bounds;
}
static int C1CB583_Outside(const c1cb583Bounds *b,const float low[3],const float high[3]){
 return high[0]<b->low[0]||low[0]>b->high[0]||high[1]<b->low[1]||low[1]>b->high[1]||high[2]<b->low[2]||low[2]>b->high[2];
}
/* Called only on aligned block boundaries with a selected, valid index.
 * Return the number of original-order triangles proven disjoint. */
static int C1CB583_Skip(const c1cb583Bounds *bounds,int index,int count,const float low[3],const float high[3]){
 int span=0;
 if(c1cb583.groups && !(index%C1CB583_GROUP_SIZE) &&
    C1CB583_Outside(c1cb583.groups+index/C1CB583_GROUP_SIZE,low,high))span=C1CB583_GROUP_SIZE;
 else if(C1CB583_Outside(bounds+index/C1CB583_SIZE,low,high))span=C1CB583_SIZE;
 return span>count-index?count-index:span;
}
#endif
