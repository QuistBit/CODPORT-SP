#ifndef COD1_LIGHT_BYTES579_H
#define COD1_LIGHT_BYTES579_H
#include <smmintrin.h>
#include <intrin.h>
/* The parent quad path requires masked FP exceptions. CPU selection is
 * thread-local, with no shared mutable initialization between worker jobs. */
static int C1Q579_Available(void){
 static __declspec(thread) int available=-1;
 if(available<0){int cpu[4];__cpuid(cpu,1);available=(cpu[2]&(1<<19))!=0;}
 return available;
}
static __m128d C1Q579_Round(__m128d value,int mode){
 switch(mode){
 case 0:return _mm_round_pd(value,_MM_FROUND_TO_NEAREST_INT|_MM_FROUND_NO_EXC);
 case 1:return _mm_round_pd(value,_MM_FROUND_TO_NEG_INF|_MM_FROUND_NO_EXC);
 case 2:return _mm_round_pd(value,_MM_FROUND_TO_POS_INF|_MM_FROUND_NO_EXC);
 default:return _mm_round_pd(value,_MM_FROUND_TO_ZERO|_MM_FROUND_NO_EXC);
 }
}
static int C1Q579_Bytes(const __m128 rgb[3],int mode,unsigned char out[16]){
 unsigned char result[16];int k,v;
 if(mode<0||mode>3)return 0;
 for(k=0;k<3;k++){
  __m128 scaled=C1Q347_F(C1Q347_Mul(C1Q347_D(rgb[k]),C1Q347_S(255)));
  c1q347 biased;__m128i integer,words,bytes;unsigned packed;
  if(!C1Q347_Finite(scaled))return 0;
  biased=C1Q347_Add(C1Q347_D(scaled),C1Q347_S(0.000000000931322574615478515625));
  if(_mm_movemask_pd(_mm_or_pd(_mm_cmple_pd(biased.lo,_mm_set1_pd(INT_MIN)),_mm_cmpge_pd(biased.lo,_mm_set1_pd(INT_MAX))))||
     _mm_movemask_pd(_mm_or_pd(_mm_cmple_pd(biased.hi,_mm_set1_pd(INT_MIN)),_mm_cmpge_pd(biased.hi,_mm_set1_pd(INT_MAX)))))return 0;
  integer=_mm_unpacklo_epi64(_mm_cvttpd_epi32(C1Q579_Round(biased.lo,mode)),_mm_cvttpd_epi32(C1Q579_Round(biased.hi,mode)));
  words=_mm_packs_epi32(integer,integer);bytes=_mm_packus_epi16(words,words);packed=(unsigned)_mm_cvtsi128_si32(bytes);
  for(v=0;v<4;v++)result[v*4+k]=(unsigned char)(packed>>(8*v));
 }
 for(v=0;v<4;v++)result[v*4+3]=255;
 memcpy(out,result,16);return 1;
}
#endif
