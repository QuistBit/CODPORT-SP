#ifndef BG_COD1_BULLET_TRACE14_H
#define BG_COD1_BULLET_TRACE14_H
#include <stdint.h>
#include <string.h>
/* GAME34830 and ENGINE4317F0. Explicit finite PC53/nearest world/miss domain.
 * A live entity hit is a separate lifetime contract, never converted to world. */
static const char*const c1bt14Surfaces[23]={"default","bark","brick","carpet","cloth","concrete","dirt","flesh","foliage","glass","grass","gravel","ice","metal","mud","paper","plaster","rock","sand","snow","water","wood","asphalt"};
static const char*C1BT14_Surface(uint32_t flags){unsigned i=(flags>>20)&31;return c1bt14Surfaces[i<23?i:0];}
static int C1BT14_Vector(const float*v){unsigned i;uint32_t u;if(!v)return 0;for(i=0;i<3;i++){memcpy(&u,v+i,4);if((u&0x7fffffffu)>0x49742400u)return 0;}return 1;}
static int C1BT14_MissNormal(const float start[3],const float end[3],float out[3]){
 float a[3],b[3],v[3],segmentLength,inverse;const float one=1;unsigned char env[108];unsigned short cw=0x027f;
 if(!out||!C1BT14_Vector(start)||!C1BT14_Vector(end))return 0;memcpy(a,start,12);memcpy(b,end,12);
 __asm {
 fnsave env
 fldcw cw
 fld dword ptr b[0]
 fsub dword ptr a[0]
 fstp dword ptr v[0]
 fld dword ptr b[4]
 fsub dword ptr a[4]
 fstp dword ptr v[4]
 fld dword ptr b[8]
 fsub dword ptr a[8]
 fstp dword ptr v[8]
 fld dword ptr v[0]
 fmul dword ptr v[0]
 fld dword ptr v[4]
 fmul dword ptr v[4]
 faddp st(1),st(0)
 fld dword ptr v[8]
 fmul dword ptr v[8]
 faddp st(1),st(0)
 fstp segmentLength
 fld segmentLength
 fsqrt
 fstp segmentLength
 }
 /* The zero test is integer-only while the caller environment is saved. */
 {uint32_t bits;memcpy(&bits,&segmentLength,4);if(bits&0x7fffffffu){
 __asm {
 fld one
 fdiv segmentLength
 fstp inverse
 fld inverse
 fmul dword ptr v[0]
 fstp dword ptr v[0]
 fld inverse
 fmul dword ptr v[4]
 fstp dword ptr v[4]
 fld inverse
 fmul dword ptr v[8]
 fstp dword ptr v[8]
 }
 }}
 __asm frstor env
 if(!C1BT14_Vector(v))return 0;memcpy(out,v,12);return 1;
}
#endif
