#ifndef BG_COD1_POSTURE_STATE_H
#define BG_COD1_POSTURE_STATE_H
/* CoD1 SP 0x200054e0: stock eye-height transition state machine.
 * No RTCW adapter yet. Stock heights 11/40/60 and zero table movement offsets.
 * Nonzero movement offset state requires the native StepSlideMove side branch;
 * it is deliberately rejected, not silently treated as camera-only movement. */
#include <math.h>
typedef struct {int desired;float height;int start,target,down;float offset;} cod1PostureState_t;
typedef struct {int now,flags,prone_time,rise_time;float frametime;} cod1PostureInput_t;
typedef struct {int percent;float height;} cod1PosturePoint_t;
#include "bg_cod1_posture_tables.h"
static float CP_Add(float x,float y){volatile float z=x+y;return z;}
static float CP_Mul(float x,float y){volatile float z=x*y;return z;}
static float CP_Div(float x,float y){volatile float z=x/y;return z;}
static int CP_Duration(const cod1PostureState_t*s,const cod1PostureInput_t*i){
 if(s->target==11 && !(i->flags&4))return i->prone_time;
 if(s->target==40)return s->down?((i->flags&4)?100:150):i->rise_time;
 return 200;
}
static float CP_Sample(const cod1PosturePoint_t*p,int n,int percent){
 int j;float f;
 if(!percent)return p[0].height;
 for(j=1;j<n;j++){
  if(percent==p[j].percent)return p[j].height;
  if(percent<p[j].percent){
   f=CP_Div((float)(percent-p[j-1].percent),(float)(p[j].percent-p[j-1].percent));
   return CP_Add(CP_Mul(CP_Add(p[j].height,-p[j-1].height),f),p[j-1].height);
  }
 }
 return p[0].height;
}
#define CURVE(x,p) CP_Sample(x,sizeof(x)/sizeof(*x),p)
static float CP_Curve(const cod1PostureState_t*s,const cod1PostureInput_t*i,int p){
 if(s->target==11)return (i->flags&4)?CURVE(fastprone,p):CURVE(prone,p);
 if(s->target==40)return s->down?CURVE(duck,p):CURVE(rise,p);
 return CURVE(stand,p);
}
static int COD1_PostureTick(cod1PostureState_t*s,const cod1PostureInput_t*i){
 int p=0,dt;
 if(s->offset!=0 || i->prone_time<=0 || i->rise_time<=0)return 0;
 if(!s->desired || s->height==0){s->height=(float)s->desired;return 1;}
 if(s->height==(float)s->desired && !s->start)return 1;
 if(s->desired!=11 && s->desired!=40 && s->desired!=60){
  s->start=0;
  if(s->height<s->desired){s->height=CP_Add(s->height,CP_Mul(i->frametime,180));if(s->height>s->desired)s->height=(float)s->desired;}
  else {s->height=CP_Add(s->height,-CP_Mul(i->frametime,180));if(s->height<s->desired)s->height=(float)s->desired;}
  return 1;
 }
 if(s->start){
  dt=CP_Duration(s,i);p=((i->now-s->start)*100)/dt;
  if(p<0)p=0;
  if(p>=100){p=100;s->height=(float)s->target;s->start=0;s->offset=0;}
  else s->height=CP_Curve(s,i,p);
 }
 if(s->start){
  if(s->desired==s->target)return 1;
  if(!((s->desired<s->target && !s->down)||(s->desired>s->target && s->down)))return 1;
  s->down^=1;p=100-p;
  if(s->down){if(s->target==60)s->target=40;else if(s->target==40)s->target=11;}
  else {if(s->target==11)s->target=40;else if(s->target==40)s->target=60;}
  if(p==100){s->height=(float)s->target;s->start=0;s->offset=0;return 1;}
  s->start=i->now-(int)CP_Mul(CP_Mul((float)CP_Duration(s,i),(float)p),.01f);
  s->offset=0;return 1;
 }
 if(s->height==(float)s->desired)return 1;
 s->start=i->now;
 if(s->desired==11){s->down=1;s->target=s->height<=40?11:40;}
 else if(s->desired==40){s->down=s->height>40;s->target=40;}
 else {s->down=0;s->target=s->height<40?40:60;}
 return 1;
}

#undef CURVE
#endif
