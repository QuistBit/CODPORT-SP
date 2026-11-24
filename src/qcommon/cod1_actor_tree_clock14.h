#ifndef COD1_ACTOR_TREE_CLOCK14_H
#define COD1_ACTOR_TREE_CLOCK14_H
/* Selected server-context animation node arithmetic. Native record's goal
 * weight and interpolated current weight are distinct; ENGINE111 queries the
 * former, while render pose blending uses the latter. No tree clock invented. */
#include <stdint.h>
#include <string.h>
#include <math.h>
typedef struct {float phase,oldPhase,remaining,goalWeight,currentWeight,rate;unsigned cycle,oldCycle;} c1atl14Node_t;
static int C1ATL14_Finite(float f){uint32_t u;memcpy(&u,&f,4);return (u&0x7f800000u)!=0x7f800000u;}
#if defined(_MSC_VER)
#define C1ATL14_NOINLINE __declspec(noinline)
#else
#define C1ATL14_NOINLINE
#endif
static C1ATL14_NOINLINE int C1ATL14_SetGoalInner(const c1atl14Node_t*node,float weight,float timePerWeight,float rate,int isRoot,c1atl14Node_t*out){
 c1atl14Node_t value;float epsilon;const uint32_t epsBits=0x3a83126fu;float delta;
 if(!node||!out||!C1ATL14_Finite(weight)||weight<0||weight>1||!C1ATL14_Finite(timePerWeight)||timePerWeight<0||!C1ATL14_Finite(rate)||rate<0||!C1ATL14_Finite(node->currentWeight)||node->currentWeight<0||node->currentWeight>1)return 0;
 memcpy(&epsilon,&epsBits,4);value=*node;if(weight<epsilon)weight=0;if(isRoot){weight=1;timePerWeight=0;}
 value.goalWeight=weight;delta=(float)((double)weight-value.currentWeight);if(delta<0)delta=-delta;value.remaining=(float)((double)delta*timePerWeight);
 if(!C1ATL14_Finite(value.remaining))return 0;
 if(value.remaining<epsilon){value.remaining=0;value.currentWeight=weight;}else if(value.currentWeight==0)value.currentWeight=(float)((double)weight*epsilon);
 value.rate=rate;*out=value;return 1;
}
static int C1ATL14_SetGoal(c1atl14Node_t*node,float weight,float timePerWeight,float rate,int isRoot){
 int ok;c1atl14Node_t value;if(!node)return 0;
#if defined(_MSC_VER) && defined(_M_IX86)
 {unsigned char outer[108];uint32_t savedMx,nearestMx=0x1f80;const unsigned short cw=0x027f;
 __asm fnsave outer
 __asm stmxcsr savedMx
 __asm fldcw cw
 __asm ldmxcsr nearestMx
 ok=C1ATL14_SetGoalInner(node,weight,timePerWeight,rate,isRoot,&value);
 __asm ldmxcsr savedMx
 __asm frstor outer
 }
#else
 ok=C1ATL14_SetGoalInner(node,weight,timePerWeight,rate,isRoot,&value);
#endif
 if(!ok)return 0;*node=value;return 1;
}
static C1ATL14_NOINLINE int C1ATL14_AdvanceWeightInner(const c1atl14Node_t*node,float dt,c1atl14Node_t*out){
 c1atl14Node_t v;float epsilon,tiny;uint32_t u=0x3a83126fu;double next;if(!node||!out||!C1ATL14_Finite(dt)||dt<0||!C1ATL14_Finite(node->remaining)||node->remaining<0||!C1ATL14_Finite(node->goalWeight)||node->goalWeight<0||node->goalWeight>1||!C1ATL14_Finite(node->currentWeight)||node->currentWeight<0||node->currentWeight>1)return 0;
 memcpy(&epsilon,&u,4);u=0x358637beu;memcpy(&tiny,&u,4);v=*node;
 /* ENGINE47B1E0/47ACC7: equality stays in interpolation. */
 if((double)dt+epsilon>v.remaining){v.currentWeight=v.goalWeight;v.remaining=0;}
 else{next=((double)v.goalWeight-v.currentWeight)/v.remaining*dt+v.currentWeight;v.currentWeight=next<tiny?(float)((double)v.goalWeight*epsilon):(float)next;v.remaining=(float)((double)v.remaining-dt);}
 *out=v;return 1;
}
static int C1ATL14_AdvanceWeight(c1atl14Node_t*node,float dt){
 int ok;c1atl14Node_t value;if(!node)return 0;
 /* For a dormant node the reference assigns positive zero to two fields
  * that already contain positive zero. Check bits so that signed zero,
  * denormals, NaNs and a caller's FP environment retain their old behavior. */
 uint32_t remaining,goal,current,step;
 memcpy(&remaining,&node->remaining,4);
 memcpy(&goal,&node->goalWeight,4);
 memcpy(&current,&node->currentWeight,4);
 memcpy(&step,&dt,4);
 if((remaining|goal|current)==0 && step<0x7f800000u)return 1;
#if defined(_MSC_VER) && defined(_M_IX86)
 {unsigned char outer[108];uint32_t savedMx,nearestMx=0x1f80;const unsigned short cw=0x027f;
 __asm fnsave outer
 __asm stmxcsr savedMx
 __asm fldcw cw
 __asm ldmxcsr nearestMx
 ok=C1ATL14_AdvanceWeightInner(node,dt,&value);
 __asm ldmxcsr savedMx
 __asm frstor outer
 }
#else
 ok=C1ATL14_AdvanceWeightInner(node,dt,&value);
#endif
 if(!ok)return 0;*node=value;return 1;
}
/* No-notetrack selected leaf clock. The owning resource must supply its real
 * note count; a clip with notes is refused rather than silently losing events.
 * fps/intervals/loop are native v14 metadata, not body/gun guessed durations. */
static C1ATL14_NOINLINE int C1ATL14_PhaseInner(const c1atl14Node_t*node,double frequency,int loop,unsigned noteCount,float dt,c1atl14Node_t*out,float*deltaOut){
 c1atl14Node_t v;double delta,phase;unsigned cycle;
 if(!node||!out||!deltaOut||noteCount||!(frequency>=0&&frequency<=120)||(loop!=0&&loop!=1)||!C1ATL14_Finite(dt)||dt<0||!C1ATL14_Finite(node->phase)||node->phase<0||node->phase>1||!C1ATL14_Finite(node->rate)||node->rate<0||node->rate>100||node->cycle>65535)return 0;
 delta=frequency*node->rate*dt;if(delta>65535)return 0;phase=delta+node->phase;cycle=node->cycle;
 if(phase>=1){if(loop){do{phase-=1;cycle++;}while(phase>=1);}else phase=1;}
 v=*node;v.oldPhase=v.phase;v.oldCycle=v.cycle;v.phase=(float)phase;v.cycle=cycle&65535u;*deltaOut=(float)delta;*out=v;return 1;
}
static C1ATL14_NOINLINE int C1ATL14_AdvanceLeafInner(const c1atl14Node_t*node,unsigned intervals,unsigned fps,int loop,unsigned noteCount,float dt,c1atl14Node_t*out,float*deltaOut){
 float frequency;if(intervals>4095||!fps||fps>120)return 0;frequency=intervals?(float)((double)fps/intervals):0;return C1ATL14_PhaseInner(node,frequency,loop,noteCount,dt,out,deltaOut);
}
static int C1ATL14_AdvanceLeaf(c1atl14Node_t*node,unsigned intervals,unsigned fps,int loop,unsigned noteCount,float dt,float*deltaOut){
 int ok;c1atl14Node_t value;float delta;if(!node||!deltaOut)return 0;
#if defined(_MSC_VER) && defined(_M_IX86)
 {unsigned char outer[108];uint32_t savedMx,nearestMx=0x1f80;const unsigned short cw=0x027f;
 __asm fnsave outer
 __asm stmxcsr savedMx
 __asm fldcw cw
 __asm ldmxcsr nearestMx
 ok=C1ATL14_AdvanceLeafInner(node,intervals,fps,loop,noteCount,dt,&value,&delta);
 __asm ldmxcsr savedMx
 __asm frstor outer
 }
#else
 ok=C1ATL14_AdvanceLeafInner(node,intervals,fps,loop,noteCount,dt,&value,&delta);
#endif
 if(!ok)return 0;*node=value;*deltaOut=delta;return 1;
}
/* Native47A370 recursive frequency query: each weighted accumulation is stored
 * as float; each group returns an unrounded double quotient to its parent. */
static C1ATL14_NOINLINE int C1ATL14_FrequencyInner(const c1atl14Node_t nodes[25],const float leafFrequency[21],double*out){
 unsigned r,c,i;float mainWeights=0,mainProduct=0;double rowFrequency;
 for(i=0;i<25;i++)if(!C1ATL14_Finite(nodes[i].currentWeight)||nodes[i].currentWeight<0||nodes[i].currentWeight>1||!C1ATL14_Finite(nodes[i].rate)||nodes[i].rate<0||nodes[i].rate>100)return 0;
 for(i=0;i<21;i++)if(!C1ATL14_Finite(leafFrequency[i])||leafFrequency[i]<0||leafFrequency[i]>120)return 0;
 for(r=0;r<3;r++)if(nodes[1+r].currentWeight>0){
  float rowWeights=0,rowProduct=0;
  for(c=0;c<7;c++){i=r*7+c;if(nodes[4+i].currentWeight>0&&leafFrequency[i]>0){rowWeights=(float)((double)rowWeights+nodes[4+i].currentWeight);rowProduct=(float)((double)leafFrequency[i]*nodes[4+i].rate*nodes[4+i].currentWeight+rowProduct);}}
  rowFrequency=rowWeights>0?(double)rowProduct/rowWeights:0;
  if(rowFrequency>0){mainWeights=(float)((double)mainWeights+nodes[1+r].currentWeight);mainProduct=(float)(rowFrequency*nodes[1+r].rate*nodes[1+r].currentWeight+mainProduct);}
 }
 *out=mainWeights>0?(double)mainProduct/mainWeights:0;return 1;
}
static int C1ATL14_Frequency3x7(const c1atl14Node_t nodes[25],const float leafFrequency[21],double*out){
 int ok;double value;if(!nodes||!leafFrequency||!out)return 0;
#if defined(_MSC_VER) && defined(_M_IX86)
 {unsigned char outer[108];uint32_t savedMx,nearestMx=0x1f80;const unsigned short cw=0x027f;
 __asm fnsave outer
 __asm stmxcsr savedMx
 __asm fldcw cw
 __asm ldmxcsr nearestMx
 ok=C1ATL14_FrequencyInner(nodes,leafFrequency,&value);
 __asm ldmxcsr savedMx
 __asm frstor outer
 }
#else
 ok=C1ATL14_FrequencyInner(nodes,leafFrequency,&value);
#endif
 if(!ok)return 0;memcpy(out,&value,8);return 1;
}
static int C1ATL14_AdvanceFrequency(c1atl14Node_t*node,double frequency,int loop,unsigned noteCount,float dt,float*deltaOut){
 int ok;c1atl14Node_t value;float delta;if(!node||!deltaOut)return 0;
#if defined(_MSC_VER) && defined(_M_IX86)
 {unsigned char outer[108];uint32_t savedMx,nearestMx=0x1f80;const unsigned short cw=0x027f;
 __asm fnsave outer
 __asm stmxcsr savedMx
 __asm fldcw cw
 __asm ldmxcsr nearestMx
 ok=C1ATL14_PhaseInner(node,frequency,loop,noteCount,dt,&value,&delta);
 __asm ldmxcsr savedMx
 __asm frstor outer
 }
#else
 ok=C1ATL14_PhaseInner(node,frequency,loop,noteCount,dt,&value,&delta);
#endif
 if(!ok)return 0;*node=value;*deltaOut=delta;return 1;
}
/* Clear goal differs from SetGoal(0): on an active path it preserves the
 * interpolated current weight until AdvanceWeight, even for zero fade time. */
static C1ATL14_NOINLINE int C1ATL14_ClearGoalInner(const c1atl14Node_t*node,float fade,int activePath,c1atl14Node_t*out){
 c1atl14Node_t v;float eps;uint32_t u=0x3a83126fu;if(!node||!out||!C1ATL14_Finite(fade)||fade<0||!C1ATL14_Finite(node->remaining)||node->remaining<0||!C1ATL14_Finite(node->goalWeight)||node->goalWeight<0||node->goalWeight>1||!C1ATL14_Finite(node->currentWeight)||node->currentWeight<0||node->currentWeight>1)return 0;
 memcpy(&eps,&u,4);if(fade<eps)fade=0;v=*node;
 if(!activePath){v.goalWeight=0;v.currentWeight=0;v.remaining=0;}
 else{if(v.goalWeight>0){v.goalWeight=0;v.remaining=fade;}else if(v.remaining>fade)v.remaining=fade;}
 *out=v;return 1;
}
static int C1ATL14_ClearGoal(c1atl14Node_t*node,float fade,int activePath){
 int ok;c1atl14Node_t value;if(!node)return 0;
#if defined(_MSC_VER) && defined(_M_IX86)
 {unsigned char outer[108];uint32_t savedMx,nearestMx=0x1f80;const unsigned short cw=0x027f;
 __asm fnsave outer
 __asm stmxcsr savedMx
 __asm fldcw cw
 __asm ldmxcsr nearestMx
 ok=C1ATL14_ClearGoalInner(node,fade,activePath,&value);
 __asm ldmxcsr savedMx
 __asm frstor outer
 }
#else
 ok=C1ATL14_ClearGoalInner(node,fade,activePath,&value);
#endif
 if(!ok)return 0;*node=value;return 1;
}
static int C1ATL14_RestartNodes(c1atl14Node_t*nodes,unsigned count){unsigned i;if(!nodes||!count||count>128)return 0;for(i=0;i<count;i++){nodes[i].phase=0;nodes[i].oldPhase=0;nodes[i].cycle=0;nodes[i].oldCycle=0;}return 1;}
#undef C1ATL14_NOINLINE
#endif


