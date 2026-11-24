#ifndef BG_COD1_VEHICLE_PATH14_H
#define BG_COD1_VEHICLE_PATH14_H
#include "bg_cod1_vehicle_graph14.h"
/* Native path prefix of vehicle_t, GAME48360/483F0. Authored paths with
 * no temporary switch-node overrides. This does not publish entities or VM
 * events; the owning controller must consume the reached-node result. */
typedef struct {
 int16_t nodeIndex;uint16_t flags;float fraction,speed,lookahead,rotateBlend;
 float origin[3],angles[3],lookOrigin[3];C1V14PathNode temporary[2];
} C1VP14State;
typedef char C1VP14StateSize[(sizeof(C1VP14State)==0xb8)?1:-1];
static int C1VP14_GraphValid(const C1V14PathNode*n,unsigned count){
 unsigned i,k;if(!n||!count||count>1500)return 0;
 for(i=0;i<count;i++){
  if(n[i].id!=(int)i||n[i].next< -1||n[i].next>=(int)count||!isfinite(n[i].speed)||n[i].speed<0||!isfinite(n[i].lookahead)||n[i].lookahead<0||!isfinite(n[i].distance)||n[i].distance<0)return 0;
  for(k=0;k<3;k++)if(!isfinite(n[i].origin[k])||!isfinite(n[i].angles[k])||!isfinite(n[i].direction[k]))return 0;
 }return 1;
}
static int C1VP14_Init(const C1V14PathNode*n,unsigned count,unsigned id,const C1VP14State*before,C1VP14State*out){
 C1VP14State s;unsigned i;if(!before||!out||id>=count||!C1VP14_GraphValid(n,count)||C1V14_Overlap(n,count*sizeof(*n),out,sizeof(*out))||C1V14_Overlap(before,sizeof(*before),out,sizeof(*out)))return 0;
 s=*before;s.nodeIndex=(int16_t)id;s.flags=0;s.fraction=0;s.speed=n[id].speed;s.lookahead=n[id].lookahead;s.rotateBlend=n[id].rotate?1.0f:0.0f;
 memcpy(s.origin,n[id].origin,12);memcpy(s.angles,n[id].angles,12);memcpy(s.lookOrigin,n[id].origin,12);
 for(i=0;i<2;i++){uint16_t padding=s.temporary[i].reserved6;C1V14_NodeInit(s.temporary+i);s.temporary[i].reserved6=padding;}
 *out=s;return 1;
}
static float C1VP14_LerpAngle(float a,float b,float fraction){
 if((double)b-a>180)b=(float)((double)b-360);if((double)b-a< -180)b=(float)((double)b+360);
 return (float)(((double)b-a)*fraction+a);
}
static void C1VP14_Look(const C1V14PathNode*n,unsigned count,C1VP14State*s){
 unsigned steps,k;int at=s->nodeIndex;double distance=(double)n[at].distance*s->fraction+(double)s->lookahead*s->speed;
 for(steps=0;steps<count;steps++){
  if(n[at].next<0||n[at].distance==0){distance=0;break;}
  if(distance<n[at].distance)break;distance-=n[at].distance;at=n[at].next;
 }
 for(k=0;k<3;k++)s->lookOrigin[k]=(float)(distance*n[at].direction[k]+n[at].origin[k]);
}
static int C1VP14_Advance(const C1V14PathNode*n,unsigned count,C1VP14State*s,const float*dir,int watched){
 unsigned steps;int at=s->nodeIndex,last=at,reached=0,next;double fraction=s->fraction;float a,b;
 for(steps=0;steps<count;steps++){
  const C1V14PathNode*node=n+at;last=at;if(at==watched)reached=1;next=node->next;
  if(next<0||node->distance==0){fraction=0;break;}
  /* Original x87 adds z+y before x and stores each projection to float. */
  a=(float)(((double)s->origin[2]-node->origin[2])*dir[2]+((double)s->origin[1]-node->origin[1])*dir[1]+((double)s->origin[0]-node->origin[0])*dir[0]);
  b=(float)(((double)n[next].origin[2]-s->origin[2])*dir[2]+((double)n[next].origin[1]-s->origin[1])*dir[1]+((double)n[next].origin[0]-s->origin[0])*dir[0]);
  if(a==0&&b==0){fraction=0;break;}
  if(a>=0&&b>=0){fraction=(double)a/((double)b+a);break;}
  at=next;
 }
 s->nodeIndex=(int16_t)at;s->flags=n[last].next<0;s->fraction=(float)fraction;next=n[at].next;
 if(next<0){s->speed=n[at].speed;s->lookahead=n[at].lookahead;s->rotateBlend=n[at].rotate?1.0f:0.0f;}
 else{
  s->speed=(float)(((double)n[next].speed-n[at].speed)*fraction+n[at].speed);
  s->lookahead=(float)(((double)n[next].lookahead-n[at].lookahead)*fraction+n[at].lookahead);
  s->rotateBlend=n[at].rotate?(n[next].rotate?1.0f:(float)(1.0-(double)s->fraction)):(n[next].rotate?s->fraction:0.0f);
 }return reached;
}
static int C1VP14_Rotate(const C1V14PathNode*n,C1VP14State*s){
 const C1V14PathNode*a=n+s->nodeIndex,*b;unsigned k;float left[3],right[3],v;
 if(a->next<0){if(a->rotate)memcpy(s->angles,a->angles,12);return 1;}b=n+a->next;
 if(!a->rotate&&!b->rotate)return 1;
 memcpy(left,a->rotate?a->angles:s->angles,12);memcpy(right,b->rotate?b->angles:s->angles,12);
 for(k=0;k<3;k++){v=C1VP14_LerpAngle(left[k],right[k],s->fraction);if(!C1VG14_Wrap(v,s->angles+k))return 0;}return 1;
}
__declspec(noinline) static int C1VP14_StepBody(const C1V14PathNode*n,unsigned count,C1VP14State*s,int watched,int*reached){
 float dir[3],distance;double movement;unsigned k;*reached=0;if(s->flags)return 1;
 C1VP14_Look(n,count,s);for(k=0;k<3;k++)dir[k]=(float)((double)s->lookOrigin[k]-s->origin[k]);distance=C1VG14_Normalize(dir);
 if(!isfinite(distance))return 0;if(distance<=0){s->flags=1;return 1;}
 C1VG14_VectorAngles(dir,s->angles);for(k=0;k<3;k++)if(!C1VG14_Wrap(s->angles[k],s->angles+k))return 0;
 movement=(double)s->speed*0.050000000745058059692;
 for(k=0;k<3;k++)s->origin[k]=(float)(movement*dir[k]+s->origin[k]);
 *reached=C1VP14_Advance(n,count,s,dir,watched);return C1VP14_Rotate(n,s);
}
static int C1VP14_Step(const C1V14PathNode*n,unsigned count,const C1VP14State*input,int watched,C1VP14State*out,int*reached){
 C1VP14State s;unsigned k;int ok,hit;unsigned char env[108];uint32_t mx,nearMx=0x1f80;unsigned short cw=0x027f;
 if(!input||!out||!reached||!C1VP14_GraphValid(n,count)||watched< -1||watched>=(int)count||
 C1V14_Overlap(input,sizeof(*input),out,sizeof(*out))||C1V14_Overlap(n,count*sizeof(*n),out,sizeof(*out))||C1V14_Overlap(input,sizeof(*input),reached,sizeof(*reached))||
 C1V14_Overlap(n,count*sizeof(*n),reached,sizeof(*reached))||C1V14_Overlap(out,sizeof(*out),reached,sizeof(*reached)))return 0;
 s=*input;if(s.nodeIndex<0||s.nodeIndex>=(int)count||s.flags>1||!isfinite(s.fraction)||!isfinite(s.speed)||s.speed<0||!isfinite(s.lookahead)||s.lookahead<0||!isfinite(s.rotateBlend))return 0;
 for(k=0;k<3;k++)if(!isfinite(s.origin[k])||!isfinite(s.angles[k])||!isfinite(s.lookOrigin[k]))return 0;
 if(s.temporary[0].targetname||s.temporary[1].targetname)return 0;
 __asm fnsave env
 __asm stmxcsr mx
 __asm fldcw cw
 __asm ldmxcsr nearMx
 ok=C1VP14_StepBody(n,count,&s,watched,&hit);
 __asm ldmxcsr mx
 __asm frstor env
 if(!ok)return 0;for(k=0;k<3;k++)if(!isfinite(s.origin[k])||!isfinite(s.angles[k])||!isfinite(s.lookOrigin[k]))return 0;
 if(!isfinite(s.speed)||!isfinite(s.lookahead)||!isfinite(s.fraction))return 0;*out=s;*reached=hit;return 1;
}
#endif
