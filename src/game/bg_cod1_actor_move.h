#ifndef BG_COD1_ACTOR_MOVE_H
#define BG_COD1_ACTOR_MOVE_H
#include "bg_cod1_actor_step.h"
/* GAME66350/66310/669B0; caller supplies a staged native physics record. */
static int C1APWalkInner(unsigned char*p,unsigned char*l,C1APQuery query,void*ctx){
 float v[3],before[2],normal[3],speed;unsigned i;
 v[0]=(float)((1.0/C1APF(l,0))*C1APF(p,0x2c));v[1]=(float)((1.0/C1APF(l,0))*C1APF(p,0x30));v[2]=0;
 before[0]=v[0];before[1]=v[1];speed=(float)sqrt((double)v[0]*v[0]+(double)v[1]*v[1]);memcpy(normal,l+28,12);
 C1APClip(v,v,normal,1.001f);
 if((double)before[0]*v[0]+(double)before[1]*v[1]>0){C1APNormalize(v,v);for(i=0;i<3;i++)v[i]=(float)((double)speed*v[i]);}
 memcpy(p+12,v,12);if(v[0]==0&&v[1]==0&&C1APU(l,8))return 1;
 return C1APStepInner(p,l,0,query,ctx);
}
static int C1APAirInner(unsigned char*p,unsigned char*l,C1APQuery query,void*ctx){
 float v[3],normal[3];if(C1APU(l,8)){memcpy(v,p+12,12);memcpy(normal,l+28,12);C1APClip(v,v,normal,1.001f);memcpy(p+12,v,12);}
 return C1APStepInner(p,l,1,query,ctx);
}
/* Result -1 unavailable, 0/1 exactly the original result. Counter models
 * 20125634 and is incremented only in the native nonstationary branch. */
static int C1APMoveInner(unsigned char*p,unsigned char*l,unsigned*counter,C1APQuery query,void*ctx){
 unsigned i,mode;int result;float dt;
 C1APSetU(p,0xf0,2047);C1APSetU(p,0x6c,0);p[0x105]=0;
 if(C1APS(p,0x18)==2046&&C1APF(p,0x2c)==0&&C1APF(p,0x30)==0&&C1APF(p,0x34)==0){memset(p+12,0,12);return 1;}
 ++*counter;memset(l,0,C1AP_LOCAL_BYTES);C1APSetU(l,0x58,C1APU(p,0x64));memcpy(l+0x40,p,12);memcpy(l+0x4c,p+12,12);
 dt=(float)((double)(int32_t)C1APU(p,0x48)*(float).001);C1APSetF(l,0,dt);C1APSetU(p,0x20,0);C1APSetU(p,0x24,0);mode=C1APU(p,0x40);
 if(mode==3){for(i=0;i<3;i++)C1APSetF(p,i*4,(float)((double)C1APF(p,0x2c+i*4)+C1APF(p,i*4)));return 1;}
 if(mode==4){for(i=0;i<3;i++)C1APSetF(p,12+i*4,(float)((1.0/dt)*C1APF(p,0x2c+i*4)));return C1APSlideInner(p,l,0,query,ctx)<0?-1:1;}
 if(!C1APGroundInner(p,l,query,ctx))return -1;
 result=C1APU(l,4)?C1APWalkInner(p,l,query,ctx):C1APAirInner(p,l,query,ctx);if(result<0)return -1;
 if(C1APU(l,8)&&C1APF(l,36)>=(float).7){C1APSetU(p,0x20,1);C1APSetF(p,0x24,C1APF(l,36));C1APSetU(p,0x28,(C1APU(l,40)>>20)&31);}
 else C1APSetU(p,0x28,0);
 return result;
}
#endif
