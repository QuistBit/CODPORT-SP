#ifndef BG_COD1_VEHICLE_CONTROLLER14_H
#define BG_COD1_VEHICLE_CONTROLLER14_H
#include "bg_cod1_vehicle_path14.h"
/* GAME3DAB0 path-mode arithmetic. Notifications are an ordered result,
 * delivered only by the owner after entity/world publication. */
enum {C1VC14_WAIT_NODE=1,C1VC14_END_NODE=2,C1VC14_WAIT_SPEED=3};
typedef struct {float speed;unsigned count;unsigned events[3];} C1VC14Result;
static int C1VC14_Smooth(float target,float old,float rate,float*out){
 double t=target,d;float step,value;unsigned guard=0;
 while(t-old>180){if(++guard>65536)return 0;t-=360;}
 while(t-old< -180){if(++guard>65536)return 0;t+=360;}
 d=t-old;step=(float)(d*rate*0.050000000745058059692);
 value=fabs(d)>0.0010000000474974513&&fabs(d)>=fabs((double)step)?(float)((double)step+old):(float)t;
 return C1VG14_Wrap(value,out);
}
static int C1VC14_Difference(float a,float b,float*out){float d=(float)((double)a-b);unsigned guard=0;
 while(d>180){if(++guard>65536)return 0;d=(float)((double)d-360);}while(d< -180){if(++guard>65536)return 0;d=(float)((double)d+360);}*out=d;return 1;
}
__declspec(noinline) static int C1VC14_Body(const C1V14PathNode*n,unsigned count,const C1V14Definition*d,C1V14Record*r,float current,C1VC14Result*result){
 C1VP14State base,future;unsigned i,iterations=0;int reached=0,stepHit;float oldSpeed=current,target,ratio,diff;
 memcpy(&base,r,sizeof(base));result->speed=current;result->count=0;if(base.nodeIndex<0)return 1;
 if(r->speedMode){
  target=r->speedMode==2?r->speed:r->targetSpeed;
  if(current<target){current=(float)((double)current+(double)r->speedAccel*0.050000000745058059692);if(current>target)current=target;}
  else{current=(float)((double)current-(double)r->speedAccel*0.050000000745058059692);if(current<target)current=target;}
  if(r->speedMode==2&&current==target)r->speedMode=0;
 }else current=r->speed;
 if(r->speed>0)r->stepFraction=(float)((double)current/r->speed+r->stepFraction);
 future=base;if(!C1VP14_StepBody(n,count,&future,r->waitNode,&stepHit))return 0;
 while(r->stepFraction>1){
  if(++iterations>65536)return 0;r->stepFraction=(float)((double)r->stepFraction-1);base=future;
  if(!C1VP14_StepBody(n,count,&future,r->waitNode,&stepHit))return 0;if(stepHit)reached=1;
 }
 memcpy(r,&base,sizeof(base));if(base.flags)current=0;
 for(i=0;i<3;i++){
  r->origin[i]=(float)(((double)future.origin[i]-base.origin[i])*r->stepFraction+base.origin[i]);
  target=C1VP14_LerpAngle(base.angles[i],future.angles[i],r->stepFraction);
  if(!C1VC14_Smooth(target,r->oldAngles[i],i==1?4.0f:6.0f,r->angles+i))return 0;
  r->velocity[i]=(float)(((double)r->origin[i]-r->oldOrigin[i])*20.0);
  if(!C1VC14_Difference(r->angles[i],r->oldAngles[i],&diff))return 0;
  r->angularVelocity[i]=(float)((double)diff*20.0);
 }
 r->localVelocity[0]=current;r->localVelocity[1]=r->localVelocity[2]=0;
 if(reached&&r->waitNode> -1)result->events[result->count++]=C1VC14_WAIT_NODE;
 if(base.flags)result->events[result->count++]=C1VC14_END_NODE;
 ratio=(float)((double)current/d->engineSoundSpeed);if(ratio<0)ratio=0;else if(ratio>1)ratio=1;
 r->soundBlend[1]=ratio;r->soundBlend[0]=(float)(1.0-(double)ratio);
 if(r->waitSpeed>=0&&((oldSpeed<r->waitSpeed&&current>=r->waitSpeed)||(oldSpeed>=r->waitSpeed&&current<r->waitSpeed)))result->events[result->count++]=C1VC14_WAIT_SPEED;
 result->speed=current;return 1;
}
static int C1VC14_PathFrame(const C1V14PathNode*n,unsigned count,const C1V14Definition*d,const C1V14Record*input,float current,C1V14Record*out,C1VC14Result*result){
 C1V14Record r;C1VC14Result v;int ok;unsigned k;unsigned char env[108];uint32_t mx,nearMx=0x1f80;unsigned short cw=0x027f;
 if(!d||!input||!out||!result||!C1VP14_GraphValid(n,count)||!isfinite(current)||current<0||!isfinite(d->engineSoundSpeed)||d->engineSoundSpeed<=0||
 C1V14_Overlap(input,sizeof(*input),out,sizeof(*out))||C1V14_Overlap(input,sizeof(*input),result,sizeof(*result))||C1V14_Overlap(d,sizeof(*d),out,sizeof(*out))||C1V14_Overlap(d,sizeof(*d),result,sizeof(*result))||
 C1V14_Overlap(n,count*sizeof(*n),out,sizeof(*out))||C1V14_Overlap(n,count*sizeof(*n),result,sizeof(*result))||C1V14_Overlap(out,sizeof(*out),result,sizeof(*result)))return 0;
 r=*input;memset(&v,0,sizeof(v));
 if(r.nodeIndex< -1||r.nodeIndex>=(int)count||r.waitNode< -1||r.waitNode>=(int)count||r.flags>1||r.speedMode<0||r.speedMode>2||r.pathNodes[0].targetname||r.pathNodes[1].targetname||
 !isfinite(r.speed)||r.speed<0||!isfinite(r.lookahead)||r.lookahead<0||!isfinite(r.pathFraction)||!isfinite(r.pathState)||!isfinite(r.stepFraction)||r.stepFraction<0||
 !isfinite(r.targetSpeed)||r.targetSpeed<0||!isfinite(r.speedAccel)||r.speedAccel<0||!isfinite(r.waitSpeed))return 0;
 for(k=0;k<3;k++)if(!isfinite(r.pathOrigin[k])||!isfinite(r.pathAngles[k])||!isfinite(r.pathLookOrigin[k])||!isfinite(r.oldOrigin[k])||!isfinite(r.oldAngles[k]))return 0;
 __asm fnsave env
 __asm stmxcsr mx
 __asm fldcw cw
 __asm ldmxcsr nearMx
 ok=C1VC14_Body(n,count,d,&r,current,&v);
 __asm ldmxcsr mx
 __asm frstor env
 if(!ok||!isfinite(v.speed))return 0;
 for(k=0;k<3;k++)if(!isfinite(r.origin[k])||!isfinite(r.angles[k])||!isfinite(r.velocity[k])||!isfinite(r.angularVelocity[k]))return 0;
 *out=r;*result=v;return 1;
}
#endif
