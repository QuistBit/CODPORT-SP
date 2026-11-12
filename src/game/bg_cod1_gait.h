#ifndef BG_COD1_GAIT_H
#define BG_COD1_GAIT_H
/* CoD1 SP gait-cycle reconstruction, including shared stance-transition state.
 * Source evidence: gamex86.dll 0x200067c0. No sound event implementation. */
#include <math.h>
typedef struct {
 int cycle,msec,pm_type,flags,eflags,ground,elapsed,unused_stance,forward,right;
 float vx,vy,vz,base_speed;
 int target,transition_start,transition_down,now;
} cod1GaitInput_t;
typedef struct {int cycle,event_call;float xyspeed;} cod1GaitOutput_t;
static float CGait_Add(float x,float y){volatile float z=x+y;return z;}
static float CGait_Mul(float x,float y){volatile float z=x*y;return z;}
static float CGait_Div(float x,float y){volatile float z=x/y;return z;}
static float CGait_Transition(const cod1GaitInput_t *in,int from,int to){
 int duration;float t;
 if(!in->transition_start)return 0;
 if(to!=-1 && from!=-1){
  if(from!=in->target)return 0;
  if(from==40){
   if(to==11 && in->transition_down)return 0;
   if(to==60 && !in->transition_down)return 0;
  }
 }
 duration=200;
 if(in->target==11 && !(in->flags&4))duration=400;
 else if(in->target==40){
  if(in->transition_down)duration=(in->flags&4)?100:150;
  else duration=400;
 }
 t=CGait_Div((float)(in->now-in->transition_start),(float)duration);
 return t<0?0:t>1?1:t;
}
static int CGait_Stance(const cod1GaitInput_t *in){
 if((in->flags&1)||in->target==11||
 (in->transition_start && in->target==40 && !in->transition_down))return 1;
 if((in->flags&2)||(in->transition_start && in->target==40))return 2;
 return 0;
}
static void COD1_Gait(const cod1GaitInput_t *in,cod1GaitOutput_t *out){
 float speed,den,rate,f;int pose=CGait_Stance(in);
 out->cycle=in->cycle;out->event_call=0;out->xyspeed=-99;
 if(in->pm_type>=4)return;
 speed=(float)sqrt((double)CGait_Add(CGait_Mul(in->vx,in->vx),CGait_Mul(in->vy,in->vy)));
 out->xyspeed=speed;
 if(in->eflags&0xc000)return;
 if(in->ground==2047){
  if(!(in->flags&0x10)||in->elapsed<300)return;
  den=CGait_Mul(.5f,190.5f);den=CGait_Mul(den,(in->flags&0x80)?.4f:1.f);
  rate=CGait_Mul(CGait_Div(in->vz,den),(in->flags&0x80)?.35f:.45f);
  out->cycle=((int)CGait_Add((float)in->cycle,CGait_Mul((float)in->msec,rate)))&255;
  out->event_call=1;return;
 }
 if(speed<10){if(speed<1)out->cycle=0;return;}
 den=in->base_speed;
 if(in->forward){
  if(in->right){
   f=CGait_Add(CGait_Mul(CGait_Add(.8f,-1),.75f),1);
   den=CGait_Mul(CGait_Mul(den,CGait_Add(f,1)),.5f);
   if(in->forward<0)den=CGait_Mul(CGait_Mul(den,CGait_Add(.7f,1)),.5f);
  }else if(in->forward<0)den=CGait_Mul(den,.7f);
 }else if(in->right)den=CGait_Mul(den,CGait_Add(CGait_Mul(CGait_Add(.8f,-1),.75f),1));
 den=CGait_Mul(den,(in->flags&0x80)?.4f:1.f);
 f=CGait_Transition(in,11,40);
 if(f>0)den=CGait_Mul(den,CGait_Add(CGait_Mul(CGait_Add(1,-f),.65f),CGait_Mul(f,.15f)));
 else {
  f=CGait_Transition(in,40,11);
  if(f>0)den=CGait_Mul(den,CGait_Add(CGait_Mul(CGait_Add(1,-f),.15f),CGait_Mul(f,.65f)));
  else if(pose==1)den=CGait_Mul(den,.15f);
  else if(pose==2)den=CGait_Mul(den,.65f);
 }
 if(pose==1){rate=(in->flags&0x80)?.24f:.25f;}
 else if(pose==2){rate=(in->flags&0x80)?.315f:.34f;}
 else if(in->flags&0x40)rate=(in->flags&0x80)?.325f:.36f;
 else rate=(in->flags&0x80)?.305f:.335f;
 rate=CGait_Mul(CGait_Div(speed,den),rate);
 out->cycle=((int)CGait_Add((float)in->cycle,CGait_Mul((float)in->msec,rate)))&255;
 out->event_call=in->forward||in->right;
}

#endif
