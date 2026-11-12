#ifndef BG_COD1_DROP_PLACE_H
#define BG_COD1_DROP_PLACE_H
/* Copied pure fragments of original2001BCA0. A future caller owes actual
 * current tag sampling/CM trace and atomic item/Actor lifecycle publication.
 * Native precision53/nearest domain; no gameplay provider is installed here. */
#include "bg_cod1_drop_ammo.h"
static int C1DP_Center(const float origin[3],const float mins[3],const float maxs[3],float out[3]){
 float staged[3];unsigned i;if(!origin||!mins||!maxs||!out)return 0;
 for(i=0;i<3;i++){
  if(!isfinite(origin[i])||!isfinite(mins[i])||!isfinite(maxs[i])||maxs[i]<mins[i])return 0;
  staged[i]=(float)(((double)maxs[i]+mins[i])*0.5+origin[i]);if(!isfinite(staged[i]))return 0;
 }
 for(i=0;i<3;i++)out[i]=staged[i];return 1;
}
static int C1DP_Angular(int(*draw)(void*,int*),void*u,float out[3]){
 static const float scales[3]={50,40,60};float staged[3];int n=0,v;unsigned i;if(!out)return 0;
 for(i=0;i<3;i++){if(!C1DA_Draw(draw,u,&v,&n))return 0;staged[i]=(float)(((double)v*(1.0/32768.0)*2.0-1.0)*scales[i]);}
 for(i=0;i<3;i++)out[i]=staged[i];return 1;
}
#endif
