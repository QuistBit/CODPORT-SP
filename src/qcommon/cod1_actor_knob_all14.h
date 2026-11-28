#ifndef COD1_ACTOR_KNOB_ALL14_H
#define COD1_ACTOR_KNOB_ALL14_H
#include "cod1_actor_generic_tree14.h"
/* Actual ENGINE101 direct-child domain used by original SetLookAnims. The
 * supplied root remains controlled by the Look owner; this operation selects
 * its real child and clears sibling goals. Deeper stop-ancestor paths are held. */
/* Disposable mode is only for an unpublished outer transaction that discards
 * its entire stage on refusal. The public entry retains atomic staging. */
static int C1AKA49_InnerStorage(const c1agt14Definition_t*d,c1atl14Node_t*n,unsigned count,unsigned target,unsigned root,float weight,float time,float rate,int force,int disposable){
 c1atl14Node_t*stage;unsigned i,p,sync;float greatest=0,delta,fade;int ok=0,useMask;unsigned char active[4096];
 if(!target||target>=count||root>=count||(force!=0&&force!=1)||!C1AGT14_Valid(d,n,count)||d[target].parent!=root||
    !C1ATL14_Finite(weight)||weight<0||weight>1||!C1ATL14_Finite(time)||time<0||!C1ATL14_Finite(rate)||rate<0||rate>100)return 0;
 if(disposable)stage=n;
 else{stage=(c1atl14Node_t*)malloc(count*sizeof(*stage));if(!stage)return 0;memcpy(stage,n,count*sizeof(*stage));}
 for(i=1;i<count;i++)if(d[i].parent==root){delta=i==target?(float)((double)weight-stage[i].currentWeight):stage[i].currentWeight;if(delta<0)delta=-delta;if(delta>greatest)greatest=delta;}
 fade=(float)((double)greatest*time);if(!C1ATL14_Finite(fade))goto done;
 useMask=C1AGT49_SiblingPaths(d,stage,count,target,root,active);
 for(i=1;i<count;i++)if(i!=target&&d[i].parent==root&&!C1ATL14_ClearGoal(&stage[i],fade,useMask?active[i]:C1AGT14_Active(d,stage,count,i)))goto done;
 if(!C1ATL14_SetGoal(&stage[target],weight,time,rate,0))goto done;
 /* Every descriptor already owns a node; native E490's absent-node allocation
  * branch is outside this preallocated owner. Existing ancestor goals persist. */
 sync=target;while(sync&&(!d[sync].children||!(d[sync].flags&4)))sync=d[sync].parent;
 if(!sync){if(!d[target].children&&(force||!C1AGT14_Active(d,stage,count,target))){stage[target].phase=stage[target].oldPhase=0;stage[target].cycle=stage[target].oldCycle=0;}}
 else{
  if(force||!C1AGT14_Active(d,stage,count,sync)){stage[sync].phase=stage[sync].oldPhase=0;stage[sync].cycle=stage[sync].oldCycle=0;}
  for(i=0;i<count;i++)if(C1AGT14_Descendant(d,i,target)){stage[i].phase=stage[sync].phase;stage[i].oldPhase=stage[sync].oldPhase;stage[i].cycle=stage[sync].cycle;stage[i].oldCycle=stage[sync].oldCycle;}
  for(p=target;p!=d[sync].parent;p=d[p].parent){stage[p].phase=stage[sync].phase;stage[p].oldPhase=stage[sync].oldPhase;stage[p].cycle=stage[sync].cycle;stage[p].oldCycle=stage[sync].oldCycle;}
 }
 if(!disposable)memcpy(n,stage,count*sizeof(*n));ok=1;
done:if(!disposable)free(stage);return ok;
}
static int C1AKA14_Inner(const c1agt14Definition_t*d,c1atl14Node_t*n,unsigned count,unsigned target,unsigned root,float weight,float time,float rate,int force){
 return C1AKA49_InnerStorage(d,n,count,target,root,weight,time,rate,force,0);
}
static int C1AKA14_Select(const c1agt14Definition_t*d,c1atl14Node_t*n,unsigned count,unsigned target,unsigned root,float weight,float time,float rate,int force){
 int ok;
#if defined(_MSC_VER) && defined(_M_IX86)
 unsigned char saved[108];uint32_t mx,nearest=0x1f80;const unsigned short cw=0x027f;
 __asm fnsave saved
 __asm stmxcsr mx
 __asm fldcw cw
 __asm ldmxcsr nearest
 ok=C1AKA14_Inner(d,n,count,target,root,weight,time,rate,force);
 __asm ldmxcsr mx
 __asm frstor saved
#else
 ok=C1AKA14_Inner(d,n,count,target,root,weight,time,rate,force);
#endif
 return ok;
}
static int C1AKA49_SelectDisposable(const c1agt14Definition_t*d,c1atl14Node_t*n,unsigned count,unsigned target,unsigned root,float weight,float time,float rate,int force){
 int ok;
#if defined(_MSC_VER) && defined(_M_IX86)
 unsigned char saved[108];uint32_t mx,nearest=0x1f80;const unsigned short cw=0x027f;
 __asm fnsave saved
 __asm stmxcsr mx
 __asm fldcw cw
 __asm ldmxcsr nearest
 ok=C1AKA49_InnerStorage(d,n,count,target,root,weight,time,rate,force,1);
 __asm ldmxcsr mx
 __asm frstor saved
#else
 ok=C1AKA49_InnerStorage(d,n,count,target,root,weight,time,rate,force,1);
#endif
 return ok;
}
#endif
