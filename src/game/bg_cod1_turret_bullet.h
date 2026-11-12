/* Native CoD1 20048AC0 endpoint + 2000DB80 disk spread + 20074D05
 * CRT random recurrence. Explicit caller-owned seed: no process rand state.
 * Input must be CURRENT shot basis/muzzle, never a render-frame cache.
 * Range is8192; selected finite spread0..45degrees. No trace/damage/event.
 * PC53 nearest arithmetic matches the original materialized x87 sequence;
 * full caller x87 image and MXCSR are preserved. Refusal preserves outputs. */
#ifndef BG_COD1_TURRET_BULLET_H
#define BG_COD1_TURRET_BULLET_H
#include <stdint.h>
#include <string.h>
static int C1TB_Overlap(const void *a,size_t an,const void *b,size_t bn){
 uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
 return x<=y?y-x<an:x-y<bn;
}
typedef struct {float forward[3],candidate[3];int hit_branch,applied;} c1tbTrackedDirection_t;
#if defined(_MSC_VER) && defined(_M_IX86)
static __declspec(noinline) int C1TB_EndPoint(const float forward[3],
 const float right[3],const float up[3],const float muzzle[3],float spread_degrees,
 uint32_t *seed,float end[3]){
 float basis[12],v[3],angle,radius,radians,scale,cosine,sine;
 uint32_t raw[12],sb,next,draw0,draw1;int i;
 const uint32_t piBits=0x40490fdbu,inv180Bits=0x3bb60b61u;
 const uint32_t degreesBits=0x43b40000u,one80Bits=0x43340000u,divisorBits=0x47000000u,rangeBits=0x46000000u;
 const unsigned short cw=0x027f;unsigned char outer[108];
 if(!forward||!right||!up||!muzzle||!seed||!end||
    C1TB_Overlap(seed,4,end,12))return 0;
 memcpy(raw,forward,12);memcpy(raw+3,right,12);memcpy(raw+6,up,12);memcpy(raw+9,muzzle,12);
 memcpy(&sb,&spread_degrees,4);
 for(i=0;i<12;i++)if((raw[i]&0x7f800000u)==0x7f800000u)return 0;
 if((sb&0x7fffffffu)>0x42340000u||(sb&0x80000000u&&sb!=0x80000000u))return 0;
 memcpy(basis,raw,48);memcpy(&next,seed,4);
 next=next*214013u+2531011u;draw0=(next>>16)&32767u;
 next=next*214013u+2531011u;draw1=(next>>16)&32767u;
 __asm {
  fnsave outer
  fldcw cw
  fld spread_degrees
  fmul dword ptr piBits
  fmul dword ptr inv180Bits
  fptan
  fstp st(0)
  fmul dword ptr rangeBits
  fstp scale
  fild draw0
  fstp angle
  fld angle
  fdiv dword ptr divisorBits
  fmul dword ptr degreesBits
  fstp angle
  fild draw1
  fstp radius
  fld radius
  fdiv dword ptr divisorBits
  fstp radius
  fld angle
  fmul dword ptr piBits
  fdiv dword ptr one80Bits
  fstp radians
  fld radians
  fsincos
  fstp cosine
  fstp sine
  fld cosine
  fmul radius
  fstp cosine
  fld sine
  fmul radius
  fstp sine
  fld cosine
  fmul scale
  fld sine
  fmul scale
  fld dword ptr basis[0]
  fmul dword ptr rangeBits
  fadd dword ptr basis[36]
  fstp dword ptr v[0]
  fld dword ptr basis[4]
  fmul dword ptr rangeBits
  fadd dword ptr basis[40]
  fstp dword ptr v[4]
  fld dword ptr basis[8]
  fmul dword ptr rangeBits
  fadd dword ptr basis[44]
  fstp dword ptr v[8]
  fld st(1)
  fmul dword ptr basis[12]
  fadd dword ptr v[0]
  fstp dword ptr v[0]
  fld st(1)
  fmul dword ptr basis[16]
  fadd dword ptr v[4]
  fstp dword ptr v[4]
  fxch st(1)
  fmul dword ptr basis[20]
  fadd dword ptr v[8]
  fstp dword ptr v[8]
  fld st(0)
  fmul dword ptr basis[24]
  fadd dword ptr v[0]
  fstp dword ptr v[0]
  fld st(0)
  fmul dword ptr basis[28]
  fadd dword ptr v[4]
  fstp dword ptr v[4]
  fmul dword ptr basis[32]
  fadd dword ptr v[8]
  fstp dword ptr v[8]
  frstor outer
 }
 memcpy(raw,v,12);
 for(i=0;i<3;i++)if((raw[i]&0x7f800000u)==0x7f800000u)return 0;
 memcpy(end,v,12);memcpy(seed,&next,4);return 1;
}
/* Tracking portion of205B0 with genuine aim point from current GAME target
 * resolution. No owner/enemy/sentient is inspected by native56410/56990.
 * miss_half_width is actual selected body-profile halfwidth (native mutable
 * player-bounds global); constant lateral radius still derives from15. */
static uint32_t C1TB_Draw(uint32_t *seed){*seed=*seed*214013u+2531011u;return (*seed>>16)&32767u;}
static __declspec(noinline) void C1TB_Normalize(float v[3]){
 float magnitude,reciprocal;uint32_t bits;
 __asm {
  mov edx,v
  fld dword ptr [edx]
  fmul dword ptr [edx]
  fld dword ptr [edx+4]
  fmul dword ptr [edx+4]
  faddp st(1),st
  fld dword ptr [edx+8]
  fmul dword ptr [edx+8]
  faddp st(1),st
  fstp magnitude
  fld magnitude
  fsqrt
  fstp magnitude
 }
 memcpy(&bits,&magnitude,4);if(!(bits&0x7fffffffu))return;
 __asm {
  fld1
  fdiv magnitude
  fstp reciprocal
  mov edx,v
  fld reciprocal
  fmul dword ptr [edx]
  fstp dword ptr [edx]
  fld reciprocal
  fmul dword ptr [edx+4]
  fstp dword ptr [edx+4]
  fld reciprocal
  fmul dword ptr [edx+8]
  fstp dword ptr [edx+8]
 }
}
static __declspec(noinline) int C1TB_TrackedForward(const float forward[3],
 const float up[3],const float muzzle[3],const float target[3],float accuracy,
 float miss_half_width,uint32_t *seed,uint32_t *qseed,c1tbTrackedDirection_t *out){
 float u[3],m[3],t[3],direction[3],cross[3],point[3],candidate[3];const float *front;
 float drawFraction,lateral,back,jitter;uint32_t raw[12],a,w,next,qn,draw;int sign,i;unsigned short status;
 const uint32_t divisor=0x47000000u,eight=0x41000000u,twelve=0x41400000u;
 const uint32_t limit=0x3f7eb852u;const unsigned short cw=0x027f;unsigned char outer[108];
 c1tbTrackedDirection_t staged;
 if(!forward||!up||!muzzle||!target||!seed||!qseed||!out||
    C1TB_Overlap(seed,4,qseed,4)||C1TB_Overlap(seed,4,out,sizeof(*out))||C1TB_Overlap(qseed,4,out,sizeof(*out)))return 0;
 memcpy(raw,forward,12);memcpy(raw+3,up,12);memcpy(raw+6,muzzle,12);memcpy(raw+9,target,12);
 for(i=0;i<12;i++)if((raw[i]&0x7fffffffu)>0x49742400u)return 0;/* finite+/-1,000,000 */
 memcpy(&a,&accuracy,4);memcpy(&w,&miss_half_width,4);
 if((a&0x7fffffffu)>0x3f800000u||(a&0x80000000u&&a!=0x80000000u)||
    (w&0x7fffffffu)>0x447a0000u||(w&0x80000000u&&w!=0x80000000u))return 0;
 memcpy(u,raw+3,12);memcpy(m,raw+6,12);memcpy(t,raw+9,12);
 memcpy(&next,seed,4);memcpy(&qn,qseed,4);memset(&staged,0,sizeof(staged));memcpy(staged.forward,raw,12);front=staged.forward;
 __asm {
  fnsave outer 
  fldcw cw
 }
 draw=C1TB_Draw(&next);
 __asm {
  fild draw 
  fdiv dword ptr divisor 
  fstp drawFraction 
  fld drawFraction 
  fcomp accuracy 
  fnstsw ax 
  mov status,ax
 }
 staged.hit_branch=(status&0x0100u)!=0;
 for(i=0;i<3;i++){
  const float *tp=t+i,*mp=m+i;float *dp=direction+i;
  __asm {
  mov eax,tp 
  mov ecx,mp 
  mov edx,dp 
  fld dword ptr [eax]
  fsub dword ptr [ecx] 
  fstp dword ptr [edx]
 }
 }
 C1TB_Normalize(direction);
 __asm {
  fld dword ptr direction[4]
  fmul dword ptr u[8]
  fld dword ptr direction[8]
  fmul dword ptr u[4]
  fsubp st(1),st
  fstp dword ptr cross[0]
  fld dword ptr direction[8]
  fmul dword ptr u[0]
  fld dword ptr direction[0]
  fmul dword ptr u[8]
  fsubp st(1),st
  fstp dword ptr cross[4]
  fld dword ptr direction[0]
  fmul dword ptr u[4]
  fld dword ptr direction[4]
  fmul dword ptr u[0]
  fsubp st(1),st
  fstp dword ptr cross[8]
 }
 C1TB_Normalize(cross);
 if(staged.hit_branch){
  for(i=0;i<6;i++){
   const float *axis=i<3?cross+i:u+i-3,*base=i<3?t+i:point+i-3;float *dest=point+i%3;
   draw=C1TB_Draw(&next);
   __asm {
    fild draw
    fstp drawFraction
    fld drawFraction
    fdiv dword ptr divisor
    fadd st(0),st
    fld1
    fsubp st(1),st
    fmul dword ptr eight
    mov eax,axis
    fmul dword ptr [eax]
    mov eax,base
    fadd dword ptr [eax]
    mov eax,dest
    fstp dword ptr [eax]
   }
  }
 }else{
  qn=qn*69069u+1u;sign=(qn&65535u)>32768u?1:-1;
  /* Native cached sqrt((15*15)*2*3), materialized binary32 once. */
  {const uint32_t fifteen=0x41700000u,three=0x40400000u;
   __asm {
  fld dword ptr fifteen 
  fmul dword ptr fifteen 
  fadd st(0),st 
  fmul dword ptr three 
  fsqrt 
  fstp lateral
 }
  }
  __asm {
  fild sign 
  fmul lateral 
  fstp lateral 
  fld miss_half_width 
  fld1 
  faddp st(1),st 
  fchs 
  fstp back
 }
  draw=C1TB_Draw(&next);
  __asm {
  fild draw 
  fstp drawFraction 
  fld drawFraction 
  fdiv dword ptr divisor 
  fadd st(0),st 
  fld1 
  fsubp st(1),st 
  fmul dword ptr twelve 
  fstp jitter
 }
  for(i=0;i<3;i++){
   const float *d=direction+i,*tp=t+i,*c=cross+i,*upv=u+i;float *dest=point+i;
   __asm {
    mov edx,dest
    fld back
    mov eax,d
    fmul dword ptr [eax]
    mov eax,tp
    fadd dword ptr [eax]
    fstp dword ptr [edx]
    mov eax,c
    fld dword ptr [eax]
    fmul lateral
    fadd dword ptr [edx]
    fstp dword ptr [edx]
    fld jitter
    mov eax,upv
    fmul dword ptr [eax]
    fadd dword ptr [edx]
    fstp dword ptr [edx]
   }
  }
 }
 for(i=0;i<3;i++){
  const float *p=point+i,*mp=m+i;float *dest=candidate+i;
  __asm {
  mov eax,p 
  mov ecx,mp 
  mov edx,dest 
  fld dword ptr [eax]
  fsub dword ptr [ecx] 
  fstp dword ptr [edx]
 }
 }
 C1TB_Normalize(candidate);memcpy(staged.candidate,candidate,12);
 __asm {
  mov edx,front
  fld dword ptr candidate[8]
  fmul dword ptr [edx+8]
  fld dword ptr candidate[4]
  fmul dword ptr [edx+4]
  faddp st(1),st
  fld dword ptr candidate[0]
  fmul dword ptr [edx]
  faddp st(1),st
  fcomp dword ptr limit
  fnstsw ax
  mov status,ax
  frstor outer
 }
 staged.applied=(status&0x4100u)==0;if(staged.applied)memcpy(staged.forward,candidate,12);
 memcpy(raw,&staged,24);for(i=0;i<6;i++)if((raw[i]&0x7f800000u)==0x7f800000u)return 0;
 memcpy(out,&staged,sizeof(staged));memcpy(seed,&next,4);memcpy(qseed,&qn,4);return 1;
}

#else
static int C1TB_EndPoint(const float f[3],const float r[3],const float u[3],const float m[3],float spread,uint32_t *seed,float end[3]){
 (void)f;(void)r;(void)u;(void)m;(void)spread;(void)seed;(void)end;return 0;
}
static int C1TB_TrackedForward(const float f[3],const float u[3],const float m[3],const float t[3],float a,float w,uint32_t *s,uint32_t *q,c1tbTrackedDirection_t *o){
 (void)f;(void)u;(void)m;(void)t;(void)a;(void)w;(void)s;(void)q;(void)o;return 0;
}
#endif
#endif
