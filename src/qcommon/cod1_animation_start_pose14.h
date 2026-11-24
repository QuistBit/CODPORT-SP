#ifndef COD1_ANIMATION_START_POSE14_H
#define COD1_ANIMATION_START_POSE14_H
#include "cod1_native_start_origin.h"
#include "cod1_actor_scripted_math14.h"
/* GAME138D0/13A30, sampled at normalized zero by the resource owner.
 * Explicit PC53/nearest; original CRT angular comparison is tolerance-based.
 * Origin is validated even for the angle query, as native GetVector(0) is real. */
static int C1ASP14_Vector(const float*v,unsigned n,uint32_t bound){unsigned i;uint32_t b;if(!v)return 0;for(i=0;i<n;i++){memcpy(&b,v+i,4);if((b&0x7fffffffu)>bound)return 0;}return 1;}
static int C1ASP14_Query(unsigned kind,const float origin[3],const float angles[3],const float q[2],const float translation[3],float out[3]){
 float result[3],matrix[9],rotation[9],combined[9];int valid;
 unsigned char env[108];uint32_t mx,selectedMx=0x1f80;unsigned short cw=0x027f;
 if(!out||(kind!=1&&kind!=2)||!C1ASP14_Vector(origin,3,0x49742400u)||!C1ASP14_Vector(angles,3,0x45610000u)||!C1ASP14_Vector(q,2,0x43000000u)||!C1ASP14_Vector(translation,3,0x49742400u))return 0;
 if(kind==1)return C1NSO_Transform(origin,angles,translation,out);
 __asm fnsave env
 __asm stmxcsr mx
 __asm fldcw cw
 __asm ldmxcsr selectedMx
 valid=(double)q[0]*q[0]+(double)q[1]*q[1]>1.e-20;
 if(valid){C1ASM14_Matrix(angles,matrix);C1ASM14_YawMatrix(C1ASM14_Yaw(q),rotation);C1ASM14_Product(matrix,rotation,combined);C1ASM14_Angles(combined,result);}
 __asm ldmxcsr mx
 __asm frstor env
 if(!valid||!C1ASP14_Vector(result,3,0x45610000u))return 0;
 memcpy(out,result,sizeof(result));return 1;
}
#endif
