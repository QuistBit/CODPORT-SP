#ifndef BG_COD1_ACTOR_STEP_H
#define BG_COD1_ACTOR_STEP_H
#include "bg_cod1_actor_slide.h"
/* GAME660E0. Negative means provider failure; zero/one are native results.
 * The outer physics transaction owns staging and FP state. */
static int C1APStepInner(unsigned char*p,unsigned char*l,int gravity,C1APQuery query,void*ctx){
 float origin[3],velocity[3],end[3],start[3],upZ;C1APTrace tr;int result;unsigned mask=C1APU(l,0x58);
 memcpy(origin,p,12);memcpy(velocity,p+12,12);
 result=C1APSlideInner(p,l,gravity,query,ctx);if(result<0)return -1;if(!result)return 1;if(result==2)return 0;
 if(C1APF(p,0x14)>0&&!C1APU(l,4)){
  memcpy(end,origin,12);end[2]=(float)((double)origin[2]-18);
  if(!C1APQueryCall(query,ctx,origin,end,p,mask,&tr))return -1;
  if(tr.fraction==1||tr.normal[2]<(float).7)return 1;
 }
 memcpy(end,origin,12);end[2]=(float)((double)origin[2]+18);
 if(!C1APQueryCall(query,ctx,origin,end,p,mask,&tr))return -1;
 if(tr.allsolid)return 1;
 C1APSetU(p,0xf0,2047);memcpy(p,tr.end,12);upZ=tr.end[2];memcpy(p+12,velocity,12);
 result=C1APSlideInner(p,l,gravity,query,ctx);if(result<0)return -1;if(result==2)return 0;
 memcpy(start,p,12);memcpy(end,start,12);end[2]=(float)(((double)origin[2]-upZ)+start[2]);
 if(!C1APQueryCall(query,ctx,start,end,p,mask,&tr))return -1;
 if(!tr.startsolid)memcpy(p,tr.end,12);
 if(tr.fraction<1){memcpy(velocity,p+12,12);C1APClip(velocity,velocity,tr.normal,1.001f);memcpy(p+12,velocity,12);}
 return 1;
}
#endif
