/* Two exact bounded immediate native command branches, no physics/event.
 * actor+134..144 are numeric cells, not an original allocation overlay. */
#ifndef BG_COD1_ACTOR_IMMEDIATE14_H
#define BG_COD1_ACTOR_IMMEDIATE14_H
#include <stdint.h>
#include <string.h>
static int C1AI14_OrientDefault(const uint32_t before[5],uint32_t after[5]) {
    uint32_t copy[5];if(!before||!after)return 0;memcpy(copy,before,sizeof(copy));
    copy[4]=0;copy[0]=0;memcpy(after,copy,sizeof(copy));return 1;
}
/* GAME68D30 face-angle ->54AA0. Selected finite normal/zero input,
 * magnitude <=1e6; exact binary product/truncate/16-bit wrap at native53-bit
 * precision. Integer-only implementation preserves the caller's FP state. */
static int C1AI14_OrientAngle(uint32_t angle,uint32_t after[5]) {
 uint32_t a=angle&0x7fffffffu,e=a>>23,m=a&0x7fffffu,n,word=0;uint64_t product,whole;int shift,top;
 if(!after||a>0x49742400u||(a&&!e))return 0;
 m=a?(m|0x800000u):0;product=(uint64_t)m*UINT64_C(11930465);
 shift=166-(int)e;whole=shift>=64?0:product>>shift;
 n=(uint32_t)whole;if(angle>>31)n=0u-n;n=(n&65535u)*45u;
 if(n){for(top=31;!(n&(1u<<top));top--){}word=((uint32_t)(127+top-13)<<23)|((n<<(23-top))&0x7fffffu);}
 after[0]=1;after[1]=0;after[2]=after[3]=word;after[4]=0;return 1;
}
/* Original6D580 returns before reading/changing any other prone cell whenever
 * +190 ==0. No claim that an active transition can use this branch. */
static int C1AI14_ExitFresh(const uint32_t cells[9]) {return cells&&cells[4]==0;}
#endif
