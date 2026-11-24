#ifndef COD1_PLAYER_VIEW_ANGLES14_H
#define COD1_PLAYER_VIEW_ANGLES14_H
#include <stdint.h>
#include <string.h>
/* GAME142F0 non-prone branch: binary32 scale43360B61, native84140 signed64
 * truncate, low16, then wrapping subtraction of the actual input command.
 * Integer arithmetic is exact for finite normal/zero |angle|<=1e6 and retains
 * every caller FP bit. No RTCW ANGLE2SHORT expression or angle normalization. */
static int C1PVA14_Delta(const uint32_t angles[3],const int32_t command[3],int32_t out[3]){
 uint32_t words[3];unsigned i;
 if(!angles||!command||!out)return 0;
 for(i=0;i<3;i++){
  uint32_t bits=angles[i],a=bits&0x7fffffffu,e=a>>23,m=a&0x7fffffu,n;uint64_t product,whole;int shift;
  if(a>0x49742400u||(a&&!e))return 0;
  m=a?(m|0x800000u):0;product=(uint64_t)m*UINT64_C(11930465);shift=166-(int)e;
  whole=shift>=64?0:product>>shift;n=(uint32_t)whole;if(bits>>31)n=0u-n;
  words[i]=(n&65535u)-(uint32_t)command[i];
 }
 memcpy(out,words,sizeof(words));return 1;
}
#endif
