#ifndef BG_COD1_ACTOR_SLIDE_H
#define BG_COD1_ACTOR_SLIDE_H
#include "bg_cod1_actor_physics.h"
static void C1APNormalize(float*out,const float*in){
 float x=in[0],y=in[1],z=in[2],square=(float)(((double)x*x+(double)y*y)+(double)z*z),length=(float)sqrt((double)square),inverse;
 if(length==0){memset(out,0,12);return;}inverse=(float)(1.0/(double)length);out[0]=(float)((double)x*inverse);out[1]=(float)((double)y*inverse);out[2]=(float)((double)z*inverse);
}
static void C1APCross(float*out,const float*a,const float*b){out[0]=(float)((double)a[1]*b[2]-(double)a[2]*b[1]);out[1]=(float)((double)a[2]*b[0]-(double)a[0]*b[2]);out[2]=(float)((double)a[0]*b[1]-(double)a[1]*b[0]);}
static double C1APDot021(const float*a,const float*b){return ((double)a[0]*b[0]+(double)a[2]*b[2])+(double)a[1]*b[1];}
static double C1APDot210(const float*a,const float*b){return ((double)a[2]*b[2]+(double)a[1]*b[1])+(double)a[0]*b[0];}
/* GAME65970. -1 provider failure;0/1/2 are original native results.
 * Caller stages the complete physics/local state and pins world ownership. */
static int C1APSlideInner(unsigned char*p,unsigned char*l,int gravity,C1APQuery query,void*ctx){
 float velocity[3],endVelocity[3]={0},planes[5][3],end[3],clipped[3],endClipped[3],dir[3],time=C1APF(l,0),dot,delta[2];
 C1APTrace tr;unsigned count=0,bump,i,j,k;float ground[3];
 memcpy(velocity,p+12,12);memcpy(ground,l+28,12);
 if(gravity){memcpy(endVelocity,velocity,12);endVelocity[2]=(float)((double)velocity[2]-(double)time*C1APF(p,0x44));velocity[2]=(float)(((double)endVelocity[2]+velocity[2])*.5);
  if(C1APU(l,8))C1APClip(velocity,velocity,ground,1.001f);memcpy(p+12,velocity,12);
 }
 if(C1APU(l,8)){memcpy(planes[0],ground,12);count=1;}C1APNormalize(planes[count++],velocity);
 for(bump=0;bump<4;bump++){
  for(i=0;i<3;i++)end[i]=(float)((double)time*velocity[i]+C1APF(p,i*4));
  if(!C1APQueryCall(query,ctx,(const float*)p,end,p,C1APU(l,0x58),&tr))return -1;
  if(tr.fraction>0){memcpy(p,tr.end,12);if(tr.fraction==1)break;}
  else if(tr.startsolid){
   C1APSetF(p,0x14,0);if(bump==0&&l[12+47])return 2;
   if(C1APU(p,0xf0)==2047){C1APSetU(p,0xf0,tr.entity);memcpy(p+0xf4,p,8);p[0x104]=1;}return 1;
  }
  p[0x105]=1;
  if(tr.normal[2]<(float).7&&C1APU(p,0xf0)==2047&&(tr.normal[0]!=0||tr.normal[1]!=0)){
   C1APSetU(p,0xf0,tr.entity);memcpy(p+0xf4,p,8);memcpy(p+0xfc,tr.normal,8);p[0x104]=0;
  }
  if(!C1APTouch(p,tr.entity))return -1;
  time=(float)((double)time-(double)tr.fraction*time);
  if(count>=5){memset(p+12,0,12);return 1;}
  for(i=0;i<count;i++)if(C1APDot021(tr.normal,planes[i])>.99)break;
  if(i<count){for(j=0;j<3;j++)velocity[j]=(float)((double)tr.normal[j]+velocity[j]);memcpy(p+12,velocity,12);continue;}
  memcpy(planes[count++],tr.normal,12);
  for(i=0;i<count;i++){dot=(float)C1APDot021(planes[i],velocity);if(dot<.1)break;}
  if(i==count)continue;
  dot=-dot;if(dot>C1APF(l,0x3c))C1APSetF(l,0x3c,dot);
  C1APClip(clipped,velocity,planes[i],1.001f);C1APClip(endClipped,endVelocity,planes[i],1.001f);
  for(j=0;j<count;j++){
   if(j==i||C1APDot210(clipped,planes[j])>=.1)continue;
   C1APClip(clipped,clipped,planes[j],1.001f);C1APClip(endClipped,endClipped,planes[j],1.001f);
   if(C1APDot021(clipped,planes[i])>=0)continue;
   C1APCross(dir,planes[i],planes[j]);C1APNormalize(dir,dir);
   dot=(float)(((double)dir[2]*velocity[2]+(double)dir[0]*velocity[0])+(double)dir[1]*velocity[1]);
   for(k=0;k<3;k++)clipped[k]=(float)((double)dir[k]*dot);
   C1APCross(dir,planes[i],planes[j]);C1APNormalize(dir,dir);dot=(float)C1APDot210(dir,endVelocity);
   for(k=0;k<3;k++)endClipped[k]=(float)((double)dir[k]*dot);
   for(k=0;k<count;k++)if(k!=i&&k!=j&&(((double)clipped[2]*planes[k][2]+(double)clipped[0]*planes[k][0])+(double)clipped[1]*planes[k][1])<.1){memset(p+12,0,12);return 1;}
  }
  if(C1APU(p,0xf0)==2047){delta[0]=(float)((double)clipped[0]-velocity[0]);delta[1]=(float)((double)clipped[1]-velocity[1]);if(delta[0]!=0||delta[1]!=0){C1APSetU(p,0xf0,tr.entity);memcpy(p+0xf4,p,8);memcpy(p+0xfc,delta,8);p[0x104]=0;}}
  memcpy(velocity,clipped,12);memcpy(endVelocity,endClipped,12);memcpy(p+12,velocity,12);
 }
 if(gravity)memcpy(p+12,endVelocity,12);return bump!=0;
}
#endif
