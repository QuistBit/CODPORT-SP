#ifndef COD1_ACTOR_TURRET_TREE14_H
#define COD1_ACTOR_TURRET_TREE14_H
/* Selected original MG42 complete 3x7 group, no notify tracks. Root bank is
 * immutable and owned by the caller for the tree lifetime. This controller
 * owns mutable goal/current weights and actual synchronized phase/cycle state.
 * Input bank is native child order, reverse of all 21 ATR text leaves:
 * up/level/down rows and right-to-left yaw children. See original head
 * insertion 45E21B..45E24D and compiler next traversal 455270..4554E6.
 * It does not own an actor, authorize mounting, or synthesize arrival/events. */
#include "cod1_actor_root14.h"
#include "cod1_actor_root_group14.h"
#include "cod1_actor_turret_goals14.h"
#define C1ATT14_MAGIC 0xc1a77314u
typedef struct {uint32_t magic;const c1ar14Root_t*bank[21];float frequency[21];c1atl14Node_t node[25];int sampleTick;unsigned loop;} c1att14Tree_t;
typedef struct {c1atg14Yaw_t yaw;unsigned queriedRows,lowerRow,upperRow;int endpoint;float rowHeight[3],heightFraction;c1arg14Result_t root;} c1att14Selection_t;
static int C1ATT14_InitInner(c1att14Tree_t*out,const c1ar14Root_t*const bank[21],const unsigned noteCounts[21],int sampleTick){
 c1att14Tree_t v;unsigned i;if(!out||!bank||!noteCounts||sampleTick<0)return 0;memset(&v,0,sizeof(v));v.sampleTick=sampleTick;
 for(i=0;i<21;i++){
  const c1ar14Root_t*r=bank[i];if(!r||r->magic!=C1AR14_MAGIC||!(r->flags&2)||noteCounts[i]||r->noteCount||!r->fps||r->fps>120||r->intervals>4095)return 0;
  if(i&&(r->intervals!=bank[0]->intervals||r->fps!=bank[0]->fps||(r->flags&1)!=(bank[0]->flags&1)))return 0;
  v.bank[i]=r;v.frequency[i]=r->intervals?(float)((double)r->fps/r->intervals):0;
 }
 for(i=0;i<25;i++)v.node[i].rate=1;v.loop=bank[0]->flags&1;v.magic=C1ATT14_MAGIC;*out=v;return 1;
}
static int C1ATT14_QueryRoot(const c1att14Tree_t*t,c1arg14Result_t*out){float phases[21],leaf[21],row[3];unsigned i;if(!t||t->magic!=C1ATT14_MAGIC||!out)return 0;for(i=0;i<21;i++){phases[i]=t->node[4+i].phase;leaf[i]=t->node[4+i].goalWeight;}for(i=0;i<3;i++)row[i]=t->node[1+i].goalWeight;return C1ARG14_BlendCurrent(t->bank,phases,leaf,row,out);}
static int C1ATT14_ActivePath(const c1att14Tree_t*t,unsigned index){unsigned r,c;if(t->node[0].currentWeight<=0)return 0;if(index==0){for(r=0;r<3;r++)if(t->node[1+r].currentWeight>0)for(c=0;c<7;c++)if(t->node[4+r*7+c].currentWeight>0)return 1;return 0;}if(index<4){r=index-1;if(t->node[index].currentWeight<=0)return 0;for(c=0;c<7;c++)if(t->node[4+r*7+c].currentWeight>0)return 1;return 0;}r=(index-4)/7;return t->node[1+r].currentWeight>0&&t->node[index].currentWeight>0;}
static int C1ATT14_ClearChildren(c1att14Tree_t*t){unsigned r,c;for(r=0;r<3;r++){unsigned n=1+r;if(!C1ATL14_ClearGoal(&t->node[n],0,C1ATT14_ActivePath(t,n)))return 0;for(c=0;c<7;c++){n=4+r*7+c;if(!C1ATL14_ClearGoal(&t->node[n],0,C1ATT14_ActivePath(t,n)))return 0;}}return 1;}
static int C1ATT14_SetNode(c1att14Tree_t*t,unsigned n,float goal,float time){if(n<1||n>=25||!C1ATL14_SetGoal(&t->node[n],goal,time,1,0))return 0;/* Complete sync ancestor owns descendant phases, including newly active leaves. */t->node[n].phase=t->node[0].phase;t->node[n].oldPhase=t->node[0].oldPhase;t->node[n].cycle=t->node[0].cycle;t->node[n].oldCycle=t->node[0].oldCycle;return 1;}
static int C1ATT14_SetTimed(c1att14Tree_t*t,unsigned n,double target){float time;if(!C1ATG14_Time(t->node[n].currentWeight,target,&time))return 0;return C1ATT14_SetNode(t,n,(float)target,time);}
static int C1ATT14_SetYaw(c1att14Tree_t*t,unsigned row,const c1atg14Yaw_t*yaw,int timed){unsigned n=4+row*7+yaw->lower;if(timed){if(!C1ATT14_SetTimed(t,n,1.0-yaw->fraction))return 0;if(yaw->fraction>0&&!C1ATT14_SetTimed(t,n+1,yaw->fraction))return 0;}else{if(!C1ATT14_SetNode(t,n,yaw->lowerWeight,1))return 0;if(yaw->fraction>0&&!C1ATT14_SetNode(t,n+1,yaw->fraction,1))return 0;}return 1;}
#if defined(_MSC_VER)
#define C1ATT14_NOINLINE __declspec(noinline)
#else
#define C1ATT14_NOINLINE
#endif
static C1ATT14_NOINLINE int C1ATT14_SelectInner(c1att14Tree_t*t,float yaw,float increment,float desired,c1att14Selection_t*out){
 c1att14Tree_t v;c1att14Selection_t result;unsigned r,last=0;float phases[21],leaf[21],rows[3];c1arg14Result_t query;
 if(!t||t->magic!=C1ATT14_MAGIC||!out||!C1ATL14_Finite(desired)||t->node[0].currentWeight<=0)return 0;v=*t;memset(&result,0,sizeof(result));if(!C1ATG14_Yaw(yaw,increment,7,&result.yaw)||!C1ATT14_ClearChildren(&v))return 0;
 for(r=0;r<3;r++){
  unsigned i;if(!C1ATT14_SetNode(&v,1+r,1,1)||!C1ATT14_SetYaw(&v,r,&result.yaw,0))return 0;
  for(i=0;i<21;i++){phases[i]=v.node[4+i].phase;leaf[i]=v.node[4+i].goalWeight;}memset(rows,0,sizeof(rows));rows[r]=1;
  if(!C1ARG14_BlendCurrent(v.bank,phases,leaf,rows,&query))return 0;result.rowHeight[r]=query.translation[2];result.queriedRows=r+1;last=r;
  if(r&&!(result.rowHeight[r]>result.rowHeight[r-1]))return 0;
  if(result.rowHeight[r]>=desired)break;
 }
 /* Native clears the entire selected group's children again after probing. */
 if(!C1ATT14_ClearChildren(&v)||!C1ATT14_SetYaw(&v,last,&result.yaw,1))return 0;
 if(last==0&&desired<=result.rowHeight[0]){result.endpoint=-1;result.lowerRow=result.upperRow=0;if(!C1ATT14_SetTimed(&v,1,1))return 0;}
 else if(result.rowHeight[last]<desired){result.endpoint=1;result.lowerRow=result.upperRow=last;if(!C1ATT14_SetTimed(&v,1+last,1))return 0;}
 else{
  result.upperRow=last;result.lowerRow=last-1;if(!C1ATG14_Height(desired,result.rowHeight[last-1],result.rowHeight[last],&result.heightFraction))return 0;
  if(!C1ATT14_SetTimed(&v,1+last,result.heightFraction)||!C1ATT14_SetTimed(&v,last,1.0-result.heightFraction)||!C1ATT14_SetYaw(&v,last-1,&result.yaw,1))return 0;
 }
 if(!C1ATT14_QueryRoot(&v,&result.root))return 0;*t=v;*out=result;return 1;
}
static int C1ATT14_Select(c1att14Tree_t*t,float yaw,float increment,float desired,c1att14Selection_t*out){int ok;c1att14Tree_t v;c1att14Selection_t selected;if(!t||!out)return 0;v=*t;
#if defined(_MSC_VER)&&defined(_M_IX86)
 {unsigned char outer[108];uint32_t mx,nmx=0x1f80;const unsigned short cw=0x027f;
 __asm fnsave outer
 __asm stmxcsr mx
 __asm fldcw cw
 __asm ldmxcsr nmx
 ok=C1ATT14_SelectInner(&v,yaw,increment,desired,&selected);
 __asm ldmxcsr mx
 __asm frstor outer
 }
#else
 ok=C1ATT14_SelectInner(&v,yaw,increment,desired,&selected);
#endif
 if(!ok)return 0;*t=v;*out=selected;return 1;}
/* Two admitted MG42 knobs; all other pose knobs must already be inactive.
 * Restart clears the sibling's goal and resets selected subtree phase/cycles,
 * then selected SetGoal starts the real blend. No fabricated completion notify. */
static C1ATT14_NOINLINE int C1ATT14_KnobRestartInner(c1att14Tree_t*selected,c1att14Tree_t*sibling,float weight,float time,float rate){
 c1att14Tree_t a,b;float difference,maximum,fade;if(!selected||!sibling||selected==sibling||selected->magic!=C1ATT14_MAGIC||sibling->magic!=C1ATT14_MAGIC||!C1ATL14_Finite(weight)||weight<0||weight>1||!C1ATL14_Finite(time)||time<0)return 0;
 a=*selected;b=*sibling;{float eps;uint32_t u=0x3a83126f;memcpy(&eps,&u,4);if(weight<eps)weight=0;}difference=(float)((double)weight-a.node[0].currentWeight);if(difference<0)difference=-difference;maximum=difference;if(b.node[0].currentWeight>maximum)maximum=b.node[0].currentWeight;fade=(float)((double)maximum*time);
 if(!C1ATL14_ClearGoal(&b.node[0],fade,C1ATT14_ActivePath(&b,0))||!C1ATL14_SetGoal(&a.node[0],weight,time,rate,0)||!C1ATL14_RestartNodes(a.node,25))return 0;*selected=a;*sibling=b;return 1;
}
static int C1ATT14_KnobRestart(c1att14Tree_t*selected,c1att14Tree_t*sibling,float weight,float time,float rate){int ok;c1att14Tree_t a,b;if(!selected||!sibling||selected==sibling)return 0;a=*selected;b=*sibling;
#if defined(_MSC_VER)&&defined(_M_IX86)
 {unsigned char outer[108];uint32_t mx,nmx=0x1f80;const unsigned short cw=0x027f;
 __asm fnsave outer
 __asm stmxcsr mx
 __asm fldcw cw
 __asm ldmxcsr nmx
 ok=C1ATT14_KnobRestartInner(&a,&b,weight,time,rate);
 __asm ldmxcsr mx
 __asm frstor outer
 }
#else
 ok=C1ATT14_KnobRestartInner(&a,&b,weight,time,rate);
#endif
 if(!ok)return 0;*selected=a;*sibling=b;return 1;}
static C1ATT14_NOINLINE int C1ATT14_AdvanceInner(c1att14Tree_t*t,int sampleTick){
 c1att14Tree_t v;double frequency;float dt,delta;unsigned i;if(!t||t->magic!=C1ATT14_MAGIC||sampleTick<=t->sampleTick||sampleTick-t->sampleTick>60000)return 0;v=*t;dt=(float)((double)(sampleTick-t->sampleTick)*.001);if(!C1ATL14_AdvanceWeight(&v.node[0],dt))return 0;
 if(v.node[0].currentWeight>0||v.node[0].goalWeight>0){if(!C1ATL14_Frequency3x7(v.node,v.frequency,&frequency)||!C1ATL14_AdvanceFrequency(&v.node[0],frequency,v.loop,0,dt,&delta))return 0;for(i=1;i<25;i++){if(!C1ATL14_AdvanceWeight(&v.node[i],dt))return 0;v.node[i].phase=v.node[0].phase;v.node[i].oldPhase=v.node[0].oldPhase;v.node[i].cycle=v.node[0].cycle;v.node[i].oldCycle=v.node[0].oldCycle;}}
 v.sampleTick=sampleTick;*t=v;return 1;
}
static int C1ATT14_Init(c1att14Tree_t*out,const c1ar14Root_t*const bank[21],const unsigned noteCounts[21],int sampleTick){int ok;c1att14Tree_t v;if(!out)return 0;
#if defined(_MSC_VER)&&defined(_M_IX86)
 {unsigned char outer[108];uint32_t mx,nmx=0x1f80;const unsigned short cw=0x027f;
 __asm fnsave outer
 __asm stmxcsr mx
 __asm fldcw cw
 __asm ldmxcsr nmx
 ok=C1ATT14_InitInner(&v,bank,noteCounts,sampleTick);
 __asm ldmxcsr mx
 __asm frstor outer
 }
#else
 ok=C1ATT14_InitInner(&v,bank,noteCounts,sampleTick);
#endif
 if(!ok)return 0;*out=v;return 1;}
static int C1ATT14_Advance(c1att14Tree_t*t,int sampleTick){int ok;c1att14Tree_t v;if(!t)return 0;v=*t;
#if defined(_MSC_VER)&&defined(_M_IX86)
 {unsigned char outer[108];uint32_t mx,nmx=0x1f80;const unsigned short cw=0x027f;
 __asm fnsave outer
 __asm stmxcsr mx
 __asm fldcw cw
 __asm ldmxcsr nmx
 ok=C1ATT14_AdvanceInner(&v,sampleTick);
 __asm ldmxcsr mx
 __asm frstor outer
 }
#else
 ok=C1ATT14_AdvanceInner(&v,sampleTick);
#endif
 if(!ok)return 0;*t=v;return 1;}
#undef C1ATT14_NOINLINE
#endif
