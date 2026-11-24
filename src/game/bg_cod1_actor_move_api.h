#ifndef BG_COD1_ACTOR_MOVE_API_H
#define BG_COD1_ACTOR_MOVE_API_H
#include "bg_cod1_actor_move.h"
typedef struct {unsigned char physics[C1AP_BYTES],local[C1AP_LOCAL_BYTES];unsigned counter;int result;} C1APMovement;
/* Private synchronous calculation. No output is published on unavailable
 * collision, malformed input or input mutation. World/actor leases belong
 * to the caller and must remain current across every query and publication. */
static int C1APMove(C1APMovement*out,const unsigned char*input,const unsigned char*local,unsigned counter,C1APQuery query,void*ctx){
 C1APMovement next;unsigned char saved[C1AP_BYTES],savedLocal[C1AP_LOCAL_BYTES];float v[3];unsigned i;
 if(!out||!input||!local||!query||sizeof(C1APTrace)!=48||
  C1APOverlap(out,sizeof(*out),input,C1AP_BYTES)||C1APOverlap(out,sizeof(*out),local,C1AP_LOCAL_BYTES)||C1APOverlap(input,C1AP_BYTES,local,C1AP_LOCAL_BYTES)||
  C1APU(input,0x3c)>2047||C1APS(input,0x18)>2047||C1APU(input,0x6c)>32||C1APU(input,0x40)>4||
  (int32_t)C1APU(input,0x48)<=0||C1APU(input,0x48)>1000||!isfinite(C1APF(input,0x44))||C1APF(input,0x44)<0||C1APF(input,0x44)>10000)return 0;
 for(i=0;i<3;i++)if(C1APF(input,0x4c+i*4)>C1APF(input,0x58+i*4))return 0;
 for(i=0;i<24;i+=12){memcpy(v,input+i,12);if(!C1APVec(v))return 0;}
 for(i=0x4c;i<=0x58;i+=12){memcpy(v,input+i,12);if(!C1APVec(v))return 0;}
 memcpy(v,input+0x2c,12);if(!C1APVec(v))return 0;
 memset(&next,0,sizeof(next));memcpy(saved,input,sizeof(saved));memcpy(savedLocal,local,sizeof(savedLocal));memcpy(next.physics,input,sizeof(saved));memcpy(next.local,local,sizeof(savedLocal));next.counter=counter;
#if defined(_MSC_VER)&&defined(_M_IX86)
 {unsigned char fp[108],discard[108];unsigned mx,nearMx=0x1f80;unsigned short cw=0x027f;
 __asm fnsave fp
 __asm stmxcsr mx
 __asm fldcw cw
 __asm ldmxcsr nearMx
 next.result=C1APMoveInner(next.physics,next.local,&next.counter,query,ctx);
 __asm fnsave discard
 __asm ldmxcsr mx
 __asm frstor fp
 }
#else
 next.result=C1APMoveInner(next.physics,next.local,&next.counter,query,ctx);
#endif
 if(next.result<0||memcmp(saved,input,sizeof(saved))||memcmp(savedLocal,local,sizeof(savedLocal)))return 0;
 *out=next;return 1;
}
#endif
