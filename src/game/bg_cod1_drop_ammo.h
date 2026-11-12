#ifndef BG_COD1_DROP_AMMO_H
#define BG_COD1_DROP_AMMO_H
/* Actor-only ammo portion of original2001BCA0. The world/drop/registry/tag
 * transaction is separate. Native Windows rand supplies integers0..32767.
 * Default distribution below preserves the native nearest/53-bit domain. */
#include <stdint.h>
#include <limits.h>
#include <math.h>
typedef struct {int reserve,clip,draws;} c1daPlan;
static int C1DA_Draw(int(*draw)(void*,int*),void*u,int*out,int*n){int v;if(!draw||draw(u,&v)!=1||v<0||v>32767)return 0;*out=v;(*n)++;return 1;}
static int C1DA_Plan(int minimum,int maximum,int clipSize,int(*draw)(void*,int*),void*u,c1daPlan*out){
 c1daPlan p;int lo=minimum,hi=maximum,total,loaded=0,r,t;float staged;
 p.reserve=p.clip=p.draws=0;if(!out||clipSize<0||clipSize>1024)return 0;
 if(lo>hi){t=lo;lo=hi;hi=t;}
 if((int64_t)hi-(int64_t)lo>INT_MAX)return 0;
 if(!hi&&!lo){
  if(!clipSize||!C1DA_Draw(draw,u,&r,&p.draws))return 0;
  staged=(float)(((double)r*(1.0/32768.0)+1.0)*(clipSize-1)*0.5);
  total=(int)floor((double)staged+0x1p-30+0.5)+1;
  if(!C1DA_Draw(draw,u,&r,&p.draws))return 0;
  staged=(float)(((double)r*(1.0/32768.0)*0.5+0.25)*total);
  loaded=(int)floor((double)staged+0x1p-30+0.5);
 }else{
  if(hi<0){total=0;loaded=0;}
  else{
   if(lo!=hi){if(!C1DA_Draw(draw,u,&r,&p.draws))return 0;total=r%(hi-lo)+lo;}
   else total=lo;
   if(total<=0){total=0;loaded=0;}
   else if(clipSize){if(!C1DA_Draw(draw,u,&r,&p.draws))return 0;loaded=r%clipSize;if(loaded>=total)loaded=total;}
  }
 }
 p.reserve=total-loaded;p.clip=loaded;if(!p.reserve)p.reserve=-1;if(!p.clip)p.clip=-1;*out=p;return 1;
}
#endif
