#ifndef BG_COD1_TURRET_MOUNT_MOTION_H
#define BG_COD1_TURRET_MOUNT_MOTION_H
/* Native fixed 50ms mount-goal smoothing,13470/13500/6C8DE..6C9F4.
 * Outputs movement intent, NEVER a successful physics move or teleport.
 * Caller must run actual collision/floor and recompute root-link offsets
 * after applied movement. Nativeflags0x20 owns one-time offset initialization.
 * Caller fences actor/turret allocation and invokes once per actual50ms tick. */
#include <stdint.h>
#include <string.h>
#include <math.h>
typedef struct {float positionOffset[3],angleOffset[3];} c1tmmState_t;
typedef struct {float candidateOrigin[3],angles[3],movementDelta[3];} c1tmmMotion_t;
static int C1TMM_Finite(float x,float bound){return x>=-bound&&x<=bound;}
static float C1TMM_AngleSubtract(float a,float b){float d=a-b;while(d>180)d-=360;while(d< -180)d+=360;return d;}
static float C1TMM_Normalize360(float x){if(x<0){do{x+=360;}while(x<0);return x;}while(x>=360)x-=360;return x;}
static int C1TMM_Step50(c1tmmState_t *state,uint32_t *nativeFlags,const float currentActorOrigin[3],const float currentActorAngles[3],const float goalOrigin[3],const float goalAngles[3],c1tmmMotion_t *out){
 c1tmmState_t s;c1tmmMotion_t m;uint32_t flags;int i;double length,factor,adjusted,offsetX;
 if(!state||!nativeFlags||!currentActorOrigin||!currentActorAngles||!goalOrigin||!goalAngles||!out)return 0;
 for(i=0;i<3;i++)if(!C1TMM_Finite(currentActorOrigin[i],1000000)||!C1TMM_Finite(goalOrigin[i],1000000)||!C1TMM_Finite(currentActorAngles[i],3600)||!C1TMM_Finite(goalAngles[i],3600))return 0;
 s=*state;flags=*nativeFlags;
 if(!(flags&0x20u)){for(i=0;i<3;i++){s.positionOffset[i]=currentActorOrigin[i]-goalOrigin[i];s.angleOffset[i]=C1TMM_AngleSubtract(currentActorAngles[i],goalAngles[i]);}flags|=0x20u;}
 for(i=0;i<3;i++)if(!C1TMM_Finite(s.positionOffset[i],2000000)||!C1TMM_Finite(s.angleOffset[i],3600))return 0;
 memcpy(m.candidateOrigin,goalOrigin,12);memcpy(m.angles,goalAngles,12);
 length=sqrt((double)s.positionOffset[0]*s.positionOffset[0]+(double)s.positionOffset[1]*s.positionOffset[1]+(double)s.positionOffset[2]*s.positionOffset[2]);
 if(length>0){factor=1.0-((double).05f*300.0)/length;if(factor>0){offsetX=factor*s.positionOffset[0];for(i=0;i<3;i++)s.positionOffset[i]=(float)(factor*s.positionOffset[i]);m.candidateOrigin[0]=(float)((double)goalOrigin[0]+offsetX);m.candidateOrigin[1]=goalOrigin[1]+s.positionOffset[1];m.candidateOrigin[2]=goalOrigin[2]+s.positionOffset[2];}else memset(s.positionOffset,0,12);}
 for(i=0;i<3;i++){if(s.angleOffset[i]>27){adjusted=(double)s.angleOffset[i]-27;s.angleOffset[i]=(float)adjusted;m.angles[i]=C1TMM_Normalize360((float)((double)goalAngles[i]+adjusted));}else if(s.angleOffset[i]< -27){adjusted=(double)s.angleOffset[i]+27;s.angleOffset[i]=(float)adjusted;m.angles[i]=C1TMM_Normalize360((float)((double)goalAngles[i]+adjusted));}else s.angleOffset[i]=0;
  if(!C1TMM_Finite(m.candidateOrigin[i],3000000)||!C1TMM_Finite(m.angles[i],3600))return 0;}
 m.movementDelta[0]=m.candidateOrigin[0]-currentActorOrigin[0];m.movementDelta[1]=m.candidateOrigin[1]-currentActorOrigin[1];m.movementDelta[2]=0;
 *state=s;*nativeFlags=flags;*out=m;return 1;
}
#endif
