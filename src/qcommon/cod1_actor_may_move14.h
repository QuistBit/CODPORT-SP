#ifndef COD1_ACTOR_MAY_MOVE14_H
#define COD1_ACTOR_MAY_MOVE14_H
/* GAME67AE0/61B30, selected no-friendly-fire-segment domain. The owner must
 * prove the actual complete same-team sentient list has no present enemies.
 * Query failure is NOT native false. Sentient collision carries the actual
 * native25C/778 policy from its separately fenced allocation owner.
 * No position/physics update: these are private read-only query computations. */
#include <stdint.h>
#include <string.h>
#include <math.h>
typedef struct {float fraction,end[3],normal[3];int allsolid,startsolid,sentient,sentientPass;} c1mmp14Trace_t;
typedef int (*c1mmp14Query_t)(void*,int,const float*,const float*,const float*,const float*,int,uint32_t,c1mmp14Trace_t*);
static int C1MMP14_Vec(const float*v){unsigned i;uint32_t u;if(!v)return 0;for(i=0;i<3;i++){memcpy(&u,v+i,4);u&=0x7fffffffu;if(u>0x49742400u||(u&&u<0x00800000u))return 0;}return 1;}
/* Query providers cannot leak their FP state into the native PC53 arithmetic. */
static int C1MMP14_Query(c1mmp14Query_t query,void*ctx,int capsule,const float*start,const float*end,const float*mins,const float*maxs,int pass,uint32_t mask,c1mmp14Trace_t*out){
 int status;
#if defined(_MSC_VER)&&defined(_M_IX86)
 unsigned char saved[108],discard[108];uint32_t mx,nearMx=0x1f80;const unsigned short cw=0x027f;
 __asm fnsave saved
 __asm stmxcsr mx
 __asm fldcw cw
 __asm ldmxcsr nearMx
 status=query(ctx,capsule,start,end,mins,maxs,pass,mask,out);
 __asm fnsave discard
 __asm ldmxcsr mx
 __asm frstor saved
#else
 status=query(ctx,capsule,start,end,mins,maxs,pass,mask,out);
#endif
 return status;
}
static int C1MMP14_Trace(c1mmp14Query_t query,void*ctx,int capsule,const float*start,const float*end,const float*mins,const float*maxs,int pass,uint32_t mask,c1mmp14Trace_t*out){
 c1mmp14Trace_t v;float s[3],e[3],lo[3],hi[3];int ok;
 if(!query||!C1MMP14_Vec(start)||!C1MMP14_Vec(end))return 0;
 memcpy(s,start,12);memcpy(e,end,12);memcpy(lo,mins,12);memcpy(hi,maxs,12);memset(&v,0xff,sizeof(v));
 ok=C1MMP14_Query(query,ctx,capsule,s,e,lo,hi,pass,mask,&v);
 if(ok!=1||memcmp(s,start,12)||memcmp(e,end,12)||memcmp(lo,mins,12)||memcmp(hi,maxs,12)||
    !(v.fraction>=0&&v.fraction<=1)||!C1MMP14_Vec(v.end)||!C1MMP14_Vec(v.normal)||
    (v.allsolid!=0&&v.allsolid!=1)||(v.startsolid!=0&&v.startsolid!=1)||(v.sentient!=0&&v.sentient!=1)||(v.sentientPass!=0&&v.sentientPass!=1)||(!v.sentient&&v.sentientPass))return 0;
 *out=v;return 1;
}
static int C1MMP14_RangeInner(const float*goal,float radiusSquared,const float*target){
 double x=(double)target[0]-goal[0],y=(double)target[1]-goal[1],z=(double)target[2]-goal[2];
 return z*z<=6400.0 && y*y+x*x<=(double)radiusSquared;
}
/* Called under the fixed PC53 environment below. 128 iterations is a port
 * resource bound: exhaustion refuses the request without publishing a bool. */
static int C1MMP14_ApplyInner(const float*origin,const float*goal,float radiusSquared,const float*target,int pass,uint32_t mask,c1mmp14Query_t query,void*ctx,int*out){
 static const float zero[3]={0,0,0},mins[3]={-15,-15,18},maxs[3]={15,15,72};
 float start[3],end[3],downStart[3],downEnd[3];c1mmp14Trace_t tr;unsigned iteration;int clear;
 if(!C1MMP14_RangeInner(goal,radiusSquared,target)){*out=0;return 1;}
 memcpy(start,target,12);memcpy(end,target,12);start[2]=(float)((double)target[2]+128);end[2]=(float)((double)target[2]-128);
 if(!C1MMP14_Trace(query,ctx,0,start,end,zero,zero,pass,mask|0x6000u,&tr))return 0;
 if(tr.allsolid){*out=0;return 1;}
 memcpy(start,origin,12);memcpy(end,target,12);end[2]=origin[2];mask|=0x4004u;
 for(iteration=0;iteration<128;iteration++){
  if(!C1MMP14_Trace(query,ctx,1,start,end,mins,maxs,pass,mask,&tr))return 0;
  clear=!tr.allsolid&&tr.fraction==1;
  if(!clear){
   float dx,dy;
   if(tr.sentient){
    if(!tr.sentientPass){*out=0;return 1;}
    mask&=~0x4000u;
   }else{
    dx=(float)((double)start[0]-tr.end[0]);dy=(float)((double)start[1]-tr.end[1]);
    if((double)dx*dx+(double)dy*dy<225){*out=0;return 1;}
   }
  }
  downStart[0]=tr.end[0];downStart[1]=tr.end[1];downStart[2]=start[2];
  memcpy(downEnd,downStart,12);downEnd[2]=(float)((double)start[2]-72);
  if(!C1MMP14_Trace(query,ctx,1,downStart,downEnd,mins,maxs,pass,mask,&tr))return 0;
  if(tr.end[2]>=start[2]&&tr.fraction!=1&&tr.normal[2]<(float).7){*out=0;return 1;}
  start[0]=tr.end[0];start[1]=tr.end[1];start[2]=(float)((double)tr.end[2]+18);
  if(clear){*out=fabs((double)start[2]-target[2])<72.0;return 1;}
  end[2]=start[2];
 }
 return 0;
}
static int C1MMP14_Apply(const float*origin,const float*goal,float radiusSquared,const float*target,int pass,uint32_t mask,c1mmp14Query_t query,void*ctx,int*out){
 int ok,value;uint32_t rb;float savedOrigin[3],savedGoal[3],savedTarget[3];
 memcpy(&rb,&radiusSquared,4);
 if(!out||!query||!C1MMP14_Vec(origin)||!C1MMP14_Vec(goal)||!C1MMP14_Vec(target)||
    (rb&0x7fffffffu)>0x5368d4a5u||((rb&0x80000000u)&&(rb&0x7fffffffu))||
    ((rb&0x7fffffffu)&&(rb&0x7fffffffu)<0x00800000u)||pass<0||pass>=2046)return 0;
 memcpy(savedOrigin,origin,12);memcpy(savedGoal,goal,12);memcpy(savedTarget,target,12);
#if defined(_MSC_VER)&&defined(_M_IX86)
 {unsigned char outer[108];uint32_t mx,nearMx=0x1f80;const unsigned short cw=0x027f;
 __asm fnsave outer
 __asm stmxcsr mx
 __asm fldcw cw
 __asm ldmxcsr nearMx
 ok=C1MMP14_ApplyInner(savedOrigin,savedGoal,radiusSquared,savedTarget,pass,mask,query,ctx,&value);
 __asm ldmxcsr mx
 __asm frstor outer
 }
#else
 ok=C1MMP14_ApplyInner(savedOrigin,savedGoal,radiusSquared,savedTarget,pass,mask,query,ctx,&value);
#endif
 if(!ok||memcmp(savedOrigin,origin,12)||memcmp(savedGoal,goal,12)||memcmp(savedTarget,target,12))return 0;
 *out=value;return 1;
}
#endif
