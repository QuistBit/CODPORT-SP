/* Original CoD1 GAME 20036390 / CGAME 300161D0: bounded loop-FX value and
 * cadence contract. This header supplies no entity ownership or authority.
 * Callers must prove a current entity and resource before publishing effects.
 * Positive finite intervals only: the original client loops indefinitely for
 * zero/negative intervals when time advances. Nonfinite/overflow inputs refuse.
 * Default round-to-nearest native FPU domain; this code never changes FPU state. */
#ifndef BG_COD1_LOOP_FX_H
#define BG_COD1_LOOP_FX_H
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <limits.h>
#include "bg_cod1_script_quake.h"
typedef struct {
 int index;
 float origin[3],direction[3],distance,period_ms;
} cod1LoopFxValue_t;
static int CLF_Finite3(const float *v){return v&&isfinite(v[0])&&isfinite(v[1])&&isfinite(v[2]);}
static int CLF_ValueValid(const cod1LoopFxValue_t *v){
 return v&&v->index>0&&v->index<64&&CLF_Finite3(v->origin)&&CLF_Finite3(v->direction)&&
  isfinite(v->distance)&&isfinite(v->period_ms)&&v->period_ms>=1.0f&&(double)v->period_ms<=INT_MAX;
}
/* Native getter order is implemented by the VM adapter, not this value helper.
 * 'directed' means a supplied fifth argument; zero direction is an error then. */
static int CLF_Prepare(int index,float seconds,const float origin[3],int directed,
 const float direction[3],float distance,cod1LoopFxValue_t *out){
 cod1LoopFxValue_t v;float scaled,squared,length,inverse;double x,whole,fraction;int i,period;
 if(!out||!CLF_Finite3(origin)||!isfinite(seconds)||!isfinite(distance)||(directed!=0&&directed!=1)||
  (directed&&!CLF_Finite3(direction)))return 0;
 memset(&v,0,sizeof(v));v.index=index&255;memcpy(v.origin,origin,sizeof(v.origin));v.distance=distance;
 if(directed){
  squared=(float)(((double)direction[0]*direction[0]+(double)direction[1]*direction[1])+(double)direction[2]*direction[2]);
  if(!isfinite(squared)||squared<=0.0f)return 0;
  length=(float)sqrt((double)squared);if(!isfinite(length)||length<=0.0f)return 0;
  inverse=1.0f/length;for(i=0;i<3;i++)v.direction[i]=inverse*direction[i];
 }
 scaled=seconds*1000.0f;if(!isfinite(scaled))return 0;
 x=(double)scaled+0x1p-30;whole=floor(x);fraction=x-whole;
 if(fraction>0.5||(fraction==0.5&&fmod(whole,2.0)!=0.0))whole+=1.0;
 if(whole<1.0||whole>INT_MAX)return 0;period=(int)whole;v.period_ms=(float)period;
 if(!CLF_ValueValid(&v))return 0;*out=v;return 1;
}
/* Returns -1 for a refused domain (no mutation), 0 for no emission, 1 for one
 * emission. 'last' is native centity+174. It advances BEFORE distance culling.
 * A frame gap skips intervals; it does not create a catch-up burst. */
static int CLF_Step(const cod1LoopFxValue_t *v,int now,int *last,const float eye[3]){
 int next,period;int64_t delta,steps;float dx,dy,dz;double squared,range;
 if(!last||!CLF_ValueValid(v)||!CLF_Finite3(eye)||now<0||*last<0)return -1;
 period=(int)v->period_ms;next=*last;
 if(now>next){
  delta=(int64_t)now-next;if(delta<period)return 0;
  steps=(delta-1)/period;if(steps<1)steps=1;
  next=(int)((int64_t)next+steps*period);
 }else next=now;
 *last=next;
 if(v->distance!=0.0f){
  dx=v->origin[0]-eye[0];dy=v->origin[1]-eye[1];dz=v->origin[2]-eye[2];
  squared=((double)dz*dz+(double)dy*dy)+(double)dx*dx;range=(double)v->distance*v->distance;
  if(!(range>squared))return 0;
 }
 return 1;
}
/* Private paired port transport; not retail snapshot/ABI parity. GAME must
 * provide an actual current allocation, never derive authority from a slot.
 * Sequence is independent of one-shot FX sequence, shared resource epoch is not.
 * Tombstones carry canonical zero value bytes, preserving the entity key. */
#define COD1_LOOP_FX_COMMAND "c1l"
#define COD1_LOOP_FX_ARGC 17
#define COD1_LOOP_FX_CAPACITY 160
enum {COD1_LOOP_FX_RETIRED=0,COD1_LOOP_FX_VISIBLE=1,COD1_LOOP_FX_HIDDEN=2};
typedef struct {
 uint64_t epoch,sequence,entity_epoch;
 uint32_t slot,generation;
 int live;
 cod1LoopFxValue_t value;
} cod1LoopFxSnapshot_t;
static int CLF_SnapshotValid(const cod1LoopFxSnapshot_t *s){
 cod1LoopFxValue_t zero;
 if(!s||!s->epoch||!s->sequence||!s->entity_epoch||!s->generation||s->slot>=1024||s->live<0||s->live>2)return 0;
 if(s->live==COD1_LOOP_FX_VISIBLE)return CLF_ValueValid(&s->value);
 memset(&zero,0,sizeof(zero));return !memcmp(&zero,&s->value,sizeof(zero));
}
static int CLF_Encode(char *out,size_t capacity,const cod1LoopFxSnapshot_t *s){
 static const int widths[15]={16,16,16,8,8,2,2,8,8,8,8,8,8,8,8};
 uint64_t fields[15];char text[COD1_LOOP_FX_CAPACITY];int n,at=5;
 if(!out||capacity<COD1_LOOP_FX_CAPACITY||!CLF_SnapshotValid(s))return 0;
 fields[0]=s->epoch;fields[1]=s->sequence;fields[2]=s->entity_epoch;fields[3]=s->slot;fields[4]=s->generation;fields[5]=s->live;fields[6]=s->value.index;
 fields[7]=CQ_FloatBits(s->value.period_ms);fields[8]=CQ_FloatBits(s->value.distance);
 for(n=0;n<3;n++){fields[9+n]=CQ_FloatBits(s->value.origin[n]);fields[12+n]=CQ_FloatBits(s->value.direction[n]);}
 memcpy(text,"c1l 1",5);for(n=0;n<15;n++){text[at++]=' ';CQ_WriteHex(text+at,widths[n],fields[n]);at+=widths[n];}text[at]=0;
 memcpy(out,text,at+1);return 1;
}
static int CLF_Decode(int argc,const char *const *argv,cod1LoopFxSnapshot_t *out){
 static const int widths[15]={16,16,16,8,8,2,2,8,8,8,8,8,8,8,8};
 uint64_t f[15];cod1LoopFxSnapshot_t s;int n;
 if(!out||!argv||argc!=COD1_LOOP_FX_ARGC||!argv[0]||!argv[1]||strcmp(argv[0],"c1l")||strcmp(argv[1],"1"))return 0;
 for(n=0;n<15;n++)if(!CQ_ParseHex(argv[n+2],widths[n],&f[n]))return 0;
 memset(&s,0,sizeof(s));s.epoch=f[0];s.sequence=f[1];s.entity_epoch=f[2];s.slot=(uint32_t)f[3];s.generation=(uint32_t)f[4];s.live=(int)f[5];s.value.index=(int)f[6];s.value.period_ms=CQ_FloatFromBits((uint32_t)f[7]);s.value.distance=CQ_FloatFromBits((uint32_t)f[8]);
 for(n=0;n<3;n++){s.value.origin[n]=CQ_FloatFromBits((uint32_t)f[9+n]);s.value.direction[n]=CQ_FloatFromBits((uint32_t)f[12+n]);}
 if(!CLF_SnapshotValid(&s))return 0;*out=s;return 1;
}
#endif
