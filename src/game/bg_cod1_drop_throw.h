#ifndef BG_COD1_DROP_THROW_H
#define BG_COD1_DROP_THROW_H
/* Copied arithmetic from original200678B0 ->2000E030. Caller must own real
 * Actor/item leases and native nearest/53-bit FP domain. No world mutation. */
#include <math.h>
typedef struct {float velocity[3];int time;} c1dtPlan;
static int C1DT_Plan(const float owner[3],float minZ,float maxZ,const float item[3],float speed,int now,c1dtPlan*out){
 c1dtPlan p;float center,delta[3],sum,length,scale;unsigned i;
 if(!out||!isfinite(speed)||now<0)return 0;p.time=now;
 if(speed==0){p.velocity[0]=p.velocity[1]=p.velocity[2]=0;*out=p;return 1;}
 if(!owner||!item||!isfinite(minZ)||!isfinite(maxZ)||maxZ<minZ||fabsf(speed)>1000000)return 0;
 for(i=0;i<3;i++)if(!isfinite(owner[i])||!isfinite(item[i])||fabsf(owner[i])>1000000||fabsf(item[i])>1000000)return 0;
 center=(float)(((double)maxZ-minZ)*0.5+owner[2]);
 delta[0]=(float)((double)item[0]-owner[0]);delta[1]=(float)((double)item[1]-owner[1]);
 delta[2]=(float)((double)item[2]-center+2.0);
 sum=(float)((double)delta[0]*delta[0]+(double)delta[1]*delta[1]+(double)delta[2]*delta[2]);
 length=(float)sqrt((double)sum);
 if(length!=0){scale=(float)(1.0/(double)length);for(i=0;i<3;i++)delta[i]=(float)((double)scale*delta[i]);}
 for(i=0;i<3;i++)p.velocity[i]=(float)((double)delta[i]*speed);*out=p;return 1;
}
#endif
