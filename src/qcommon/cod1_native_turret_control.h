/* Original turret local bone-control arithmetic dependency, x86/MSVC.
 * Original 200457E0 / quaternion product2000FFF0 instruction materialization.
 * Selected PC53/nearest policy, bounded finite angles +/-3600. No DObj, bind
 * composition, current/world muzzle, entity/controller or GSC admission.
 * Caller x87 state (including pending exceptions/full stack) is preserved;
 * no SSE floating instructions. Refusal leaves output unchanged; aliases OK. */
#ifndef COD1_NATIVE_TURRET_CONTROL_H
#define COD1_NATIVE_TURRET_CONTROL_H
#include <stdint.h>
#include <string.h>
#if defined(_MSC_VER) && defined(_M_IX86)
static __declspec(noinline) void C1NTC_Product(const float *a,const float *b,float *out){
 __asm {
  mov eax,a
  mov ecx,b
  mov edx,out
  fld         dword ptr [eax+0Ch]
  fmul        dword ptr [ecx]
  fld         dword ptr [eax+4]
  fmul        dword ptr [ecx+8]
  faddp       st(1),st
  fld         dword ptr [ecx+0Ch]
  fmul        dword ptr [eax]
  faddp       st(1),st
  fld         dword ptr [eax+8]
  fmul        dword ptr [ecx+4]
  fsubp       st(1),st
  fstp        dword ptr [edx]
  fld         dword ptr [eax+0Ch]
  fmul        dword ptr [ecx+4]
  fld         dword ptr [eax]
  fmul        dword ptr [ecx+8]
  fsubp       st(1),st
  fld         dword ptr [eax+8]
  fmul        dword ptr [ecx]
  faddp       st(1),st
  fld         dword ptr [ecx+0Ch]
  fmul        dword ptr [eax+4]
  faddp       st(1),st
  fstp        dword ptr [edx+4]
  fld         dword ptr [ecx+4]
  fmul        dword ptr [eax]
  fld         dword ptr [eax+0Ch]
  fmul        dword ptr [ecx+8]
  faddp       st(1),st
  fld         dword ptr [ecx]
  fmul        dword ptr [eax+4]
  fsubp       st(1),st
  fld         dword ptr [ecx+0Ch]
  fmul        dword ptr [eax+8]
  faddp       st(1),st
  fstp        dword ptr [edx+8]
  fld         dword ptr [eax+0Ch]
  fmul        dword ptr [ecx+0Ch]
  fld         dword ptr [ecx]
  fmul        dword ptr [eax]
  fsubp       st(1),st
  fld         dword ptr [ecx+4]
  fmul        dword ptr [eax+4]
  fsubp       st(1),st
  fld         dword ptr [eax+8]
  fmul        dword ptr [ecx+8]
  fsubp       st(1),st
  fstp        dword ptr [edx+0Ch]
 }
}
static __declspec(noinline) int C1NTC_Compute(const float angles[3],const float position[3],float out[8]){
 uint32_t ab[3],pb[3];float a[3],v[8],yaw[4],pitch[4],roll[4],temp[4],radians;
 const uint32_t factor=0x3c0efa35u;const unsigned short cw=0x027f;unsigned char outer[108];int i;
 if(!angles||!position||!out)return 0;
 memcpy(ab,angles,12);memcpy(pb,position,12);
 for(i=0;i<3;i++)if((ab[i]&0x7f800000u)==0x7f800000u||(pb[i]&0x7f800000u)==0x7f800000u||(ab[i]&0x7fffffffu)>0x45610000u)return 0;
 memcpy(a,ab,12);memset(yaw,0,sizeof(yaw));memset(pitch,0,sizeof(pitch));memset(roll,0,sizeof(roll));memset(v,0,sizeof(v));memcpy(v+5,pb,12);
 __asm {
  fnsave outer
  fldcw cw
  fld dword ptr a[4]
  fmul dword ptr factor
  fstp radians
  fld radians
  fsincos
  fstp dword ptr yaw[12]
  fstp dword ptr yaw[8]
  fld dword ptr a[0]
  fmul dword ptr factor
  fstp radians
  fld radians
  fsincos
  fstp dword ptr pitch[12]
  fstp dword ptr pitch[4]
  fld dword ptr a[8]
  fmul dword ptr factor
  fstp radians
  fld radians
  fsincos
  fstp dword ptr roll[12]
  fstp dword ptr roll[0]
 }
 C1NTC_Product(yaw,pitch,temp);C1NTC_Product(temp,roll,v);
 __asm frstor outer
 memcpy(out,v,sizeof(v));return 1;
}
#else
static int C1NTC_Compute(const float angles[3],const float position[3],float out[8]){(void)angles;(void)position;(void)out;return 0;}
#endif
/* Native 20022DC0 supplies pitch/yaw/0 to both aim tags and the third
 * entity angle as pitch/0/0 to tag_flash. Bone indices/readiness/DObj lease
 * and later local-to-world composition remain the resource owner's job. */
typedef struct {float aim[8],flash[8];} c1ntcTurretControl_t;
static int C1NTC_Turret(const float angles[3],c1ntcTurretControl_t *out){
 c1ntcTurretControl_t staged;float a[3],f[3],zero[3];uint32_t bits[3];
 if(!angles||!out)return 0;memcpy(bits,angles,12);memset(a,0,12);memset(f,0,12);memset(zero,0,12);memcpy(a,bits,8);memcpy(f,bits+2,4);
 if(!C1NTC_Compute(a,zero,staged.aim)||!C1NTC_Compute(f,zero,staged.flash))return 0;memcpy(out,&staged,sizeof(staged));return 1;
}
#endif
