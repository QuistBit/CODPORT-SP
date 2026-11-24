/* GAME51A30 ->103D0: selected actor tag_origin controller.
 * Writes native raw [quaternion, unused weight=0, translation] cells.
 * Caller owns lookup, desired-bone claim and current actor allocation. */
#ifndef COD1_ACTOR_PRONE_CONTROLLER14_H
#define COD1_ACTOR_PRONE_CONTROLLER14_H
#include "../game/bg_cod1_active_prone14.h"
static __declspec(noinline) int C1APR14_Controller(c1apr14State_t*s,int now,float out[8]){
 unsigned char saved[108];uint32_t mx,nearMx=0x1f80;const unsigned short cw=0x027f;
 uint32_t angleBits=0x3c0efa35;float fraction,pitch,radians,factor,raw[8]={0};int active;
 __asm fnsave saved
 __asm stmxcsr mx
 __asm fldcw cw
 __asm ldmxcsr nearMx
 active=C1APR14_Active(s,now);
 if(active){
  fraction=(float)C1APR14_Fraction(s,now);
  pitch=(float)((double)fraction*s->bodyPitch);memcpy(&factor,&angleBits,4);
  radians=(float)((double)pitch*factor);
  __asm {
   fld radians
   fsincos
   fstp dword ptr raw[12]
   fstp dword ptr raw[4]
  }
  raw[7]=(float)((double)fraction*s->height);memcpy(out,raw,sizeof(raw));
 }
 __asm ldmxcsr mx
 __asm frstor saved
 return active;
}

#endif
