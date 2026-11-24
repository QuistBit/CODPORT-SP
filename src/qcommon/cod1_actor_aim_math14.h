#ifndef COD1_ACTOR_AIM_MATH14_H
#define COD1_ACTOR_AIM_MATH14_H
#include <stdint.h>
#include <string.h>
#include <math.h>
/* GAME66DB0 + E030. Successful finite map-coordinate domain only.
 * Preserve the float stores between the original PC53 operations. */
static int C1AAM14_WeightsInner(const float target[3],const float eye[3],float out[3]){
 float x,y,z,sum,length,inverse,w[3];unsigned i;
 if(!target||!eye||!out)return 0;
 for(i=0;i<3;i++)if(!isfinite(target[i])||!isfinite(eye[i])||fabs((double)target[i])>62500||fabs((double)eye[i])>62500)return 0;
 x=(float)((double)target[0]-eye[0]);y=(float)((double)target[1]-eye[1]);z=(float)((double)target[2]-eye[2]);
 sum=(float)((double)x*x+(double)y*y+(double)z*z);length=(float)sqrt((double)sum);
 if(length!=0){inverse=(float)(1.0/(double)length);z=(float)((double)z*inverse);}
 w[0]=w[2]=0;
 if(z>0){w[2]=z;w[1]=(float)(1.0-(double)z);}else{w[0]=-z;w[1]=(float)(1.0-(double)w[0]);}
 memcpy(out,w,sizeof(w));return 1;
}
/* Keep the FP scope out of caller inlining: MSVC /O2 can otherwise
 * move the inner call beyond FRSTOR. The saved image belongs to this call. */
#if defined(_MSC_VER)
__declspec(noinline)
#endif
static int C1AAM14_Weights(const float target[3],const float eye[3],float out[3]){
 int ok;
#if defined(_MSC_VER) && defined(_M_IX86)
 unsigned char outer[108];uint32_t savedMx,nearestMx=0x1f80;const unsigned short cw=0x027f;
 __asm fnsave outer
 __asm stmxcsr savedMx
 __asm fldcw cw
 __asm ldmxcsr nearestMx
 ok=C1AAM14_WeightsInner(target,eye,out);
 __asm ldmxcsr savedMx
 __asm frstor outer
#else
 ok=C1AAM14_WeightsInner(target,eye,out);
#endif
 return ok;
}
#endif
