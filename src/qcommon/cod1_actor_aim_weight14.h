#ifndef COD1_ACTOR_AIM_WEIGHT14_H
#define COD1_ACTOR_AIM_WEIGHT14_H
/* ENGINE110/47E4E0: preallocated server tree, notify0, force0.
 * Unlike ENGINE109, this does not raise zero-weight ancestors. Full sparse
 * allocation/client notify are not represented by this owned node kernel. */
#include "cod1_actor_generic_tree14.h"
static int C1AAW14_SetWeightInner(const c1agt14Definition_t*d,c1atl14Node_t*n,unsigned count,unsigned target,float weight,float time,float rate){
 c1atl14Node_t*stage;unsigned p,sync,targetParent,i;int ok=0;
 if(target>=count||!C1AGT14_Valid(d,n,count))return 0;
 stage=(c1atl14Node_t*)malloc(count*sizeof(*stage));if(!stage)return 0;memcpy(stage,n,count*sizeof(*stage));
 if(!C1ATL14_SetGoal(&stage[target],weight,time,target==0?1:rate,target==0))goto done;
 /* ENGINE110 allocates missing ancestors with zero goal; all nodes in this
  * owned dense representation already exist. Existing parent goals stay. */
 sync=target;targetParent=d[target].parent;
 while(sync&&(!d[sync].children||!(d[sync].flags&4)))sync=d[sync].parent;
 if(!sync){
  if(!d[target].children&&(!C1AGT14_Active(d,stage,count,target))){stage[target].phase=stage[target].oldPhase=0;stage[target].cycle=stage[target].oldCycle=0;}
 }else{
  if(!C1AGT14_Active(d,stage,count,sync)){stage[sync].phase=stage[sync].oldPhase=0;stage[sync].cycle=stage[sync].oldCycle=0;}
  for(i=0;i<count;i++)if(C1AGT14_Descendant(d,i,target)){stage[i].phase=stage[sync].phase;stage[i].oldPhase=stage[sync].oldPhase;stage[i].cycle=stage[sync].cycle;stage[i].oldCycle=stage[sync].oldCycle;}
  targetParent=d[sync].parent;for(p=target;p!=targetParent;p=d[p].parent){stage[p].phase=stage[sync].phase;stage[p].oldPhase=stage[sync].oldPhase;stage[p].cycle=stage[sync].cycle;stage[p].oldCycle=stage[sync].oldCycle;}
 }
 memcpy(n,stage,count*sizeof(*n));ok=1;
done:free(stage);return ok;
}
/* Keep the FP scope out of caller inlining: MSVC /O2 can otherwise
 * move the inner call beyond FRSTOR. The saved image belongs to this call. */
#if defined(_MSC_VER)
__declspec(noinline)
#endif
static int C1AAW14_SetWeight(const c1agt14Definition_t*d,c1atl14Node_t*n,unsigned count,unsigned target,float weight,float time,float rate){
 int ok;
#if defined(_MSC_VER) && defined(_M_IX86)
 unsigned char outer[108];uint32_t savedMx,nearestMx=0x1f80;const unsigned short cw=0x027f;
 __asm fnsave outer
 __asm stmxcsr savedMx
 __asm fldcw cw
 __asm ldmxcsr nearestMx
 ok=C1AAW14_SetWeightInner(d,n,count,target,weight,time,rate);
 __asm ldmxcsr savedMx
 __asm frstor outer
#else
 ok=C1AAW14_SetWeightInner(d,n,count,target,weight,time,rate);
#endif
 return ok;
}
#endif
