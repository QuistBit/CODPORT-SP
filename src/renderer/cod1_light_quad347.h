/* Four independent vertices. Binary64 expressions and explicit binary32
 * spills follow the scalar shader. Only masked exceptions may enter here.
 * A declined group is replayed from its saved FP state by the caller. */
#ifndef COD1_LIGHT_QUAD347_H
#define COD1_LIGHT_QUAD347_H
#define C1Q347_ENABLED 1
#include "cod1_model_light_shade_batch.h"
#include <emmintrin.h>
#include <stdint.h>
typedef struct {__m128d lo,hi;} c1q347;
static __forceinline c1q347 C1Q347_D(__m128 a){c1q347 r;r.lo=_mm_cvtps_pd(a);r.hi=_mm_cvtps_pd(_mm_movehl_ps(a,a));return r;}
static __forceinline __m128 C1Q347_F(c1q347 a){return _mm_movelh_ps(_mm_cvtpd_ps(a.lo),_mm_cvtpd_ps(a.hi));}
static __forceinline c1q347 C1Q347_S(double a){c1q347 r;r.lo=r.hi=_mm_set1_pd(a);return r;}
static __forceinline c1q347 C1Q347_Add(c1q347 a,c1q347 b){c1q347 r;r.lo=_mm_add_pd(a.lo,b.lo);r.hi=_mm_add_pd(a.hi,b.hi);return r;}
static __forceinline c1q347 C1Q347_Sub(c1q347 a,c1q347 b){c1q347 r;r.lo=_mm_sub_pd(a.lo,b.lo);r.hi=_mm_sub_pd(a.hi,b.hi);return r;}
static __forceinline c1q347 C1Q347_Mul(c1q347 a,c1q347 b){c1q347 r;r.lo=_mm_mul_pd(a.lo,b.lo);r.hi=_mm_mul_pd(a.hi,b.hi);return r;}
static __forceinline c1q347 C1Q347_Div(c1q347 a,c1q347 b){c1q347 r;r.lo=_mm_div_pd(a.lo,b.lo);r.hi=_mm_div_pd(a.hi,b.hi);return r;}
/* The CRT sqrt entry also updates x87 status on this Windows x86 build.
 * Keep those four actual scalar calls; SSE sqrt has different FP side effects. */
static c1q347 C1Q347_Sqrt(c1q347 a){c1q347 r;double values[4];int i;
 _mm_storeu_pd(values,a.lo);_mm_storeu_pd(values+2,a.hi);
 for(i=0;i<4;i++)values[i]=(double)sqrtl((long double)values[i]);
 r.lo=_mm_loadu_pd(values);r.hi=_mm_loadu_pd(values+2);return r;}
static __forceinline __m128 C1Q347_Select(__m128 m,__m128 yes,__m128 no){return _mm_or_ps(_mm_and_ps(m,yes),_mm_andnot_ps(m,no));}
static __forceinline c1q347 C1Q347_SelectD(__m128 mask,c1q347 yes,c1q347 no){
 c1q347 r;__m128 m0=_mm_shuffle_ps(mask,mask,_MM_SHUFFLE(1,1,0,0)),m1=_mm_shuffle_ps(mask,mask,_MM_SHUFFLE(3,3,2,2));
 r.lo=_mm_or_pd(_mm_and_pd(_mm_castps_pd(m0),yes.lo),_mm_andnot_pd(_mm_castps_pd(m0),no.lo));
 r.hi=_mm_or_pd(_mm_and_pd(_mm_castps_pd(m1),yes.hi),_mm_andnot_pd(_mm_castps_pd(m1),no.hi));return r;
}
static int C1Q347_Finite(__m128 a){
 /* These comparisons also retain the scalar denormal-operand FP flag.
  * An integer exponent test is not equivalent for subnormal results. */
 return _mm_movemask_ps(_mm_and_ps(_mm_cmpge_ps(a,_mm_set1_ps(-FLT_MAX)),_mm_cmple_ps(a,_mm_set1_ps(FLT_MAX))))==15;
}
static __m128 C1Q347_Dot(__m128 a0,__m128 b0,__m128 a1,__m128 b1,__m128 a2,__m128 b2){
 return C1Q347_F(C1Q347_Add(C1Q347_Add(C1Q347_Mul(C1Q347_D(a0),C1Q347_D(b0)),C1Q347_Mul(C1Q347_D(a1),C1Q347_D(b1))),C1Q347_Mul(C1Q347_D(a2),C1Q347_D(b2))));
}
static int C1Q347_Normalize(__m128 v[3],__m128 *distance){
 __m128 squared=C1Q347_Dot(v[0],v[0],v[1],v[1],v[2],v[2]),inverse;int k;
 if(!C1Q347_Finite(squared)||_mm_movemask_ps(_mm_cmplt_ps(squared,_mm_setzero_ps())))return 0;
 *distance=C1Q347_F(C1Q347_Sqrt(C1Q347_D(squared)));
 /* Zero length preserves the scalar no-division path, including underflow. */
 if(_mm_movemask_ps(_mm_cmpeq_ps(*distance,_mm_setzero_ps())))return 0;
 inverse=C1Q347_F(C1Q347_Div(C1Q347_S(1.0),C1Q347_D(*distance)));
 for(k=0;k<3;k++)v[k]=C1Q347_F(C1Q347_Mul(C1Q347_D(inverse),C1Q347_D(v[k])));
 return 1;
}
static int C1Q347_Separate(const void*a,size_t an,const void*b,size_t bn){
 uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
 if(x>UINTPTR_MAX-an||y>UINTPTR_MAX-bn)return 0;
 return x<=y?y-x>=an:x-y>=bn;
}
#include "cod1_light_bytes579.h"
static int C1Q347_Shade(const c1mShadeBatch *batch,__m128 vertex[3],__m128 normal[3],unsigned char out[16]){
 __m128 rgb[3],dir[3],dot,distance,spot,inside;float lanes[3][4];unsigned char result[16];int i,k,v,exponent;
 c1q347 factor,denominator;
 for(k=0;k<3;k++){
  if(!C1Q347_Finite(vertex[k])||!C1Q347_Finite(normal[k]))return 0;
  rgb[k]=_mm_set1_ps(batch->rgb[k]);
 }
 for(i=0;i<batch->count;i++){
  const c1mLight *light=batch->lights+i;factor=C1Q347_S(batch->weights[i]);
  if(light->positionW==0){
   dot=C1Q347_Dot(_mm_set1_ps(light->position[2]),normal[2],_mm_set1_ps(light->position[0]),normal[0],_mm_set1_ps(light->position[1]),normal[1]);
   dot=C1Q347_Select(_mm_cmplt_ps(dot,_mm_setzero_ps()),_mm_setzero_ps(),dot);
  }else{
   for(k=0;k<3;k++)dir[k]=C1Q347_F(C1Q347_Sub(C1Q347_S(light->position[k]),C1Q347_D(vertex[k])));
   if(!C1Q347_Normalize(dir,&distance))return 0;
   dot=C1Q347_Dot(dir[2],normal[2],dir[1],normal[1],dir[0],normal[0]);
   dot=C1Q347_Select(_mm_cmplt_ps(dot,_mm_setzero_ps()),_mm_setzero_ps(),dot);
   denominator=C1Q347_Add(C1Q347_Mul(C1Q347_Add(C1Q347_Mul(C1Q347_D(distance),C1Q347_S(light->attenuation[2])),C1Q347_S(light->attenuation[1])),C1Q347_D(distance)),C1Q347_S(light->attenuation[0]));
   if(_mm_movemask_pd(_mm_cmpeq_pd(denominator.lo,_mm_setzero_pd()))||_mm_movemask_pd(_mm_cmpeq_pd(denominator.hi,_mm_setzero_pd())))return 0;
   factor=C1Q347_Div(C1Q347_S(batch->weights[i]),denominator);
   if(light->spotCutoff!=180.0f){
    factor=C1Q347_D(C1Q347_F(factor));
    spot=C1Q347_Dot(dir[2],_mm_set1_ps(light->spotDirection[2]),dir[1],_mm_set1_ps(light->spotDirection[1]),dir[0],_mm_set1_ps(light->spotDirection[0]));
    if(!C1Q347_Finite(spot))return 0;
    inside=_mm_cmpgt_ps(spot,_mm_set1_ps(batch->cutoff[i]));
    factor=C1Q347_SelectD(inside,factor,C1Q347_S(0));
    if(_mm_movemask_ps(inside)){
     exponent=light->spotExponent>0?(int)ceill(light->spotExponent):0;
     /* Inactive cone lanes do only exact 0*1; their scalar branch is skipped. */
     spot=C1Q347_Select(inside,spot,_mm_set1_ps(1));
     for(k=0;k<exponent;k++)factor=C1Q347_Mul(factor,C1Q347_D(spot));
    }
   }
  }
  for(k=0;k<3;k++){
   rgb[k]=C1Q347_F(C1Q347_Add(C1Q347_Mul(C1Q347_Add(C1Q347_Mul(C1Q347_D(dot),C1Q347_S(light->diffuse[k])),C1Q347_S(light->ambient[k])),factor),C1Q347_D(rgb[k])));
   if(!C1Q347_Finite(rgb[k]))return 0;
  }
 }
 if(C1Q579_Available())return C1Q579_Bytes(rgb,batch->roundMode,out);
 for(k=0;k<3;k++)_mm_storeu_ps(lanes[k],rgb[k]);
 for(v=0;v<4;v++){for(k=0;k<3;k++)if(!C1M_ShadeByte(lanes[k][v],batch->roundMode,&result[v*4+k]))return 0;result[v*4+3]=255;}
 memcpy(out,result,16);return 1;
}
static int C1Q347_Entity(const c1mShadeBatch *batch,const float origin[3],const float axis[3][3],const float cross[3][3],float det,const float xyz[4][4],const float normal[4][4],unsigned char rgba[16]){
 __m128 x[4],n[4],world[3],wn[3],distance;int i,j;
 for(i=0;i<4;i++){x[i]=_mm_loadu_ps(xyz[i]);n[i]=_mm_loadu_ps(normal[i]);}
 _MM_TRANSPOSE4_PS(x[0],x[1],x[2],x[3]);_MM_TRANSPOSE4_PS(n[0],n[1],n[2],n[3]);
 for(i=0;i<3;i++){
  world[i]=_mm_set1_ps(origin[i]);wn[i]=_mm_setzero_ps();
  for(j=0;j<3;j++){
   world[i]=_mm_add_ps(world[i],_mm_mul_ps(x[j],_mm_set1_ps(axis[j][i])));
   wn[i]=_mm_add_ps(wn[i],_mm_div_ps(_mm_mul_ps(n[j],_mm_set1_ps(cross[j][i])),_mm_set1_ps(det)));
  }
 }
 if(!C1Q347_Normalize(wn,&distance))return 0;
 return C1Q347_Shade(batch,world,wn,rgba);
}
#endif
