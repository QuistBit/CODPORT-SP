/* GAME54DE0/10680 finite body-turn arithmetic. Fixed original 50ms AI tick.
 * Explicit float stores retain native rounding; no path target production. */
#ifndef BG_COD1_ACTOR_BODY_TURN14_H
#define BG_COD1_ACTOR_BODY_TURN14_H
#include <stdint.h>
#include <string.h>
typedef struct {float yaw;int start,duration;} c1abt14Result_t;
static int C1ABT14_Apply(float yaw,float desired,int disabled,int start,int duration,int now,c1abt14Result_t*out){
#if defined(_MSC_VER)&&defined(_M_IX86)
 unsigned char saved[108];uint32_t mx,nearest=0x1f80u,words[2],q[5],rateBits;const unsigned short cw=0x027f;
 float delta,rate,upper,lower,sum;c1abt14Result_t next;int prone=0;
 if(!out||now<0||start<0||duration==INT32_MIN)return 0;
 memcpy(words,&yaw,4);memcpy(words+1,&desired,4);
 if((words[0]&0x7fffffff)>0x49742400u||(words[1]&0x7fffffff)>0x49742400u)return 0;
 __asm fnsave saved
 __asm stmxcsr mx
 __asm fldcw cw
 __asm ldmxcsr nearest
 next.start=start;next.duration=duration;
 delta=(float)((double)desired-yaw);
 while(delta>180.f)delta=(float)((double)delta-360.0);
 while(delta<-180.f)delta=(float)((double)delta+360.0);
 if(start&&!disabled){
  if(duration<0){if((int64_t)start-duration<now)next.start=0;else prone=1;}
  else{prone=1;if(duration>0&&(int64_t)start+duration<now)next.duration=0;}
 }
 rateBits=prone?0x3d1374bcu:0x3ecccccdu;memcpy(&rate,&rateBits,4);
 upper=(float)((double)rate*50.0);lower=(float)((double)rate*-50.0);
 if(delta>upper)delta=upper;else if(delta<lower)delta=lower;
 sum=(float)((double)delta+yaw);memcpy(words,&sum,4);
 /* Same exact native float-to-short wrap used by the angle command. */
 if(!C1AI14_OrientAngle(words[0],q)){
  __asm ldmxcsr mx
  __asm frstor saved
  return 0;
 }
 memcpy(&next.yaw,q+3,4);
 __asm ldmxcsr mx
 __asm frstor saved
 *out=next;return 1;
#else
 (void)yaw;(void)desired;(void)disabled;(void)start;(void)duration;(void)now;(void)out;return 0;
#endif
}
#endif
