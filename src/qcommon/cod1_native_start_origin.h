/* R208 private arithmetic dependency, Windows/MSVC x86 only.
 * Replays original GetStartOrigin/AngleVectors instruction materialization under
 * the selected PC53/nearest profile. This is a port policy, not a claim about
 * every original runtime thread's floating-point state. Translation is already
 * the resource owner's derived start sample; this helper does no sampling.
 *
 * Valid float3 objects are required. Inputs may alias output. Finite angles are
 * restricted to +/-3600 degrees (a selected bound well within FSINCOS range).
 * Refusal leaves output unchanged. No callbacks, allocation or original DLL
 * dependency. The entire caller x87 environment/register stack is preserved;
 * integer checks and copies do not alter MXCSR, including NaN refusal paths.
 */
#ifndef COD1_NATIVE_START_ORIGIN_H
#define COD1_NATIVE_START_ORIGIN_H
#include <stdint.h>
#include <string.h>

#if defined(_MSC_VER) && defined(_M_IX86)
static __declspec(noinline) int C1NSO_Transform(const float origin[3],
 const float angles[3],const float translation[3],float out[3]) {
 uint32_t ob[3],ab[3],tb[3],vb[3];int i;
 float o[3],a[3],t[3],f[3],r[3],u[3],n[3],v[3];
 float radians,sy,cy,sinPitch,cp,sr,cr,temp;
 const uint32_t degreeFactor=0x3c8efa35u,negativeOne=0xbf800000u,zero=0u;
 unsigned char callerX87[108];const unsigned short selectedCW=0x027f;
 if(!origin||!angles||!translation||!out)return 0;
 memcpy(ob,origin,sizeof(ob));memcpy(ab,angles,sizeof(ab));memcpy(tb,translation,sizeof(tb));
 for(i=0;i<3;++i) {
  if((ob[i]&0x7f800000u)==0x7f800000u||(ab[i]&0x7f800000u)==0x7f800000u||
     (tb[i]&0x7f800000u)==0x7f800000u||(ab[i]&0x7fffffffu)>0x45610000u)return 0;
 }
 memcpy(o,ob,sizeof(o));memcpy(a,ab,sizeof(a));memcpy(t,tb,sizeof(t));
 /* FNSAVE is deliberately nonwaiting: an inherited pending unmasked exception
  * must not fire before saving the caller state. It also provides a fresh empty
  * stack, so even a caller with eight occupied x87 registers is supported.
  * No C floating arithmetic or calls occur between FNSAVE and FRSTOR. */
 __asm {
  fnsave callerX87
  fldcw selectedCW
  fld dword ptr a[4]
  fmul dword ptr degreeFactor
  fstp radians
  fld radians
  fsincos
  fstp cy
  fstp sy
  fld dword ptr a[0]
  fmul dword ptr degreeFactor
  fstp radians
  fld radians
  fsincos
  fstp cp
  fstp sinPitch
  fld cp
  fmul cy
  fstp dword ptr f[0]
  fld cp
  fmul sy
  fstp dword ptr f[4]
  fld sinPitch
  fchs
  fstp dword ptr f[8]
  fld dword ptr a[8]
  fmul dword ptr degreeFactor
  fstp radians
  fld radians
  fsincos
  fstp cr
  fstp sr
  fld sr
  fmul sinPitch
  fstp temp
  fld cr
  fmul sy
  fld temp
  fmul cy
  fsubp st(1),st(0)
  fstp dword ptr r[0]
  fld cr
  fmul cy
  fmul dword ptr negativeOne
  fld temp
  fmul sy
  fsubp st(1),st(0)
  fstp dword ptr r[4]
  fld sr
  fmul cp
  fmul dword ptr negativeOne
  fstp dword ptr r[8]
  fld cr
  fmul sinPitch
  fstp temp
  fld temp
  fmul cy
  fld sr
  fmul sy
  faddp st(1),st(0)
  fstp dword ptr u[0]
  fld temp
  fmul sy
  fld sr
  fmul cy
  fsubp st(1),st(0)
  fstp dword ptr u[4]
  fld cr
  fmul cp
  fstp dword ptr u[8]
  /* Native wrapper uses zero-minus-right, each materialized as binary32. */
  fld dword ptr zero
  fsub dword ptr r[0]
  fstp dword ptr n[0]
  fld dword ptr zero
  fsub dword ptr r[4]
  fstp dword ptr n[4]
  fld dword ptr zero
  fsub dword ptr r[8]
  fstp dword ptr n[8]
  /* Original X/Z and Y addition orders differ; no reassociation. */
  fld dword ptr u[0]
  fmul dword ptr t[8]
  fld dword ptr t[4]
  fmul dword ptr n[0]
  faddp st(1),st(0)
  fld dword ptr f[0]
  fmul dword ptr t[0]
  faddp st(1),st(0)
  fadd dword ptr o[0]
  fstp dword ptr v[0]
  fld dword ptr u[4]
  fmul dword ptr t[8]
  fld dword ptr f[4]
  fmul dword ptr t[0]
  faddp st(1),st(0)
  fld dword ptr t[4]
  fmul dword ptr n[4]
  faddp st(1),st(0)
  fadd dword ptr o[4]
  fstp dword ptr v[4]
  fld dword ptr u[8]
  fmul dword ptr t[8]
  fld dword ptr t[4]
  fmul dword ptr n[8]
  faddp st(1),st(0)
  fld dword ptr f[8]
  fmul dword ptr t[0]
  faddp st(1),st(0)
  fadd dword ptr o[8]
  fstp dword ptr v[8]
  frstor callerX87
 }
 memcpy(vb,v,sizeof(vb));
 for(i=0;i<3;++i)if((vb[i]&0x7f800000u)==0x7f800000u)return 0;
 memcpy(out,vb,sizeof(vb));return 1;
}
#else
/* Unsupported arithmetic targets fail closed instead of substituting a libm
 * approximation under the same capability name. */
static int C1NSO_Transform(const float origin[3],const float angles[3],
 const float translation[3],float out[3]) {
 (void)origin;(void)angles;(void)translation;(void)out;return 0;
}
#endif
#endif
