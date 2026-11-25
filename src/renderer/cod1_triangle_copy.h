/* Pure copying into already reserved, disjoint tess slots. Preserve the fourth
 * position/normal components and source alpha, including all NaN payload bits.
 * No GL, allocation, material change or floating-point arithmetic occurs here. */
#ifndef COD1_TRIANGLE_COPY_H
#define COD1_TRIANGLE_COPY_H
#include <emmintrin.h>
#include <stddef.h>
#include <string.h>
typedef char C1T_DrawVertexLayoutCheck[(sizeof(drawVert_t)==44&&offsetof(drawVert_t,st)==12&&offsetof(drawVert_t,lightmap)==20&&offsetof(drawVert_t,normal)==28&&offsetof(drawVert_t,color)==40)?1:-1];
typedef char C1T_IndexLayoutCheck[(sizeof(glIndex_t)==4)?1:-1];
static void C1T_CopyIndexes(const int *source,glIndex_t *dest,int count,unsigned base){
 int i=0;const __m128i offset=_mm_set1_epi32((int)base);
 for(;i+4<=count;i+=4)_mm_storeu_si128((__m128i*)(dest+i),_mm_add_epi32(_mm_loadu_si128((const __m128i*)(source+i)),offset));
 for(;i<count;i++)dest[i]=base+source[i];
}
static void C1T_CopyVertices(const drawVert_t *source,int count,float (*xyz)[4],float (*normal)[4],float (*tex)[2][2],unsigned char (*color)[4],int *dlight,unsigned char (*precalc)[4],int needsNormal,int dlightBits,int usePrecalc,const unsigned char constant[4],const unsigned char *modelColors){
 int i;
 for(i=0;i<count;i++){
  const drawVert_t *v=source+i;
  _mm_storel_epi64((__m128i*)xyz[i],_mm_loadl_epi64((const __m128i*)v->xyz));memcpy(xyz[i]+2,v->xyz+2,4);
  if(needsNormal){_mm_storel_epi64((__m128i*)normal[i],_mm_loadl_epi64((const __m128i*)v->normal));memcpy(normal[i]+2,v->normal+2,4);}
  _mm_storeu_si128((__m128i*)tex[i],_mm_loadu_si128((const __m128i*)v->st));
  memcpy(color[i],v->color,4);if(modelColors)memcpy(color[i],modelColors+4*i,3);
  if(usePrecalc)memcpy(precalc[i],constant,4);
  dlight[i]=dlightBits;
 }
}
#endif
