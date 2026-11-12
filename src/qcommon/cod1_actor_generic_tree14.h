#ifndef COD1_ACTOR_GENERIC_TREE14_H
#define COD1_ACTOR_GENERIC_TREE14_H
/* Server-context ENGINE97/109 ownership-neutral node operations. The owner
 * supplies the real parsed hierarchy, including sync flags, and current nodes.
 * No catalog/native ID equivalence, clip sampling, notify, mount or tree lease
 * is manufactured here. All changes commit together after bounded validation. */
#include "cod1_actor_tree_clock14.h"
#include <stdlib.h>
typedef struct {unsigned parent,children,flags;} c1agt14Definition_t;
static int C1AGT14_Valid(const c1agt14Definition_t*d,const c1atl14Node_t*n,unsigned count){
 unsigned i,k,depth,children[4096];
 if(!d||!n||!count||count>4096||d[0].parent)return 0;
 {uintptr_t a=(uintptr_t)d,b=(uintptr_t)n;size_t da=count*sizeof(*d),nb=count*sizeof(*n);if(a<=b?b-a<da:a-b<nb)return 0;}
 memset(children,0,count*sizeof(*children));
 /* Validate every parent before indexing a count or following any ancestry. */
 for(i=0;i<count;i++){if(d[i].parent>=count||(i&&d[i].parent==i)||d[i].flags>15)return 0;if(i)children[d[i].parent]++;}
 for(i=0;i<count;i++){
  if(children[i]!=d[i].children)return 0;
  /* A future ancestor has not necessarily passed its own row validation yet. */
  for(k=i,depth=0;k;){if(k>=count||++depth>64)return 0;k=d[k].parent;}
  if(!C1ATL14_Finite(n[i].phase)||n[i].phase<0||n[i].phase>1||!C1ATL14_Finite(n[i].oldPhase)||n[i].oldPhase<0||n[i].oldPhase>1||
     !C1ATL14_Finite(n[i].remaining)||n[i].remaining<0||!C1ATL14_Finite(n[i].goalWeight)||n[i].goalWeight<0||n[i].goalWeight>1||
     !C1ATL14_Finite(n[i].currentWeight)||n[i].currentWeight<0||n[i].currentWeight>1||!C1ATL14_Finite(n[i].rate)||n[i].rate<0||n[i].rate>100||n[i].cycle>65535||n[i].oldCycle>65535)return 0;
 }return 1;
}
static int C1AGT14_Descendant(const c1agt14Definition_t*d,unsigned node,unsigned root){
 for(;;){if(node==root)return 1;if(!node)return 0;node=d[node].parent;}
}
static int C1AGT14_Active(const c1agt14Definition_t*d,const c1atl14Node_t*n,unsigned count,unsigned root){
 unsigned i,p;for(p=root;;p=d[p].parent){if(n[p].currentWeight==0)return 0;if(!p)break;}
 for(i=0;i<count;i++)if(!d[i].children&&n[i].currentWeight>0&&C1AGT14_Descendant(d,i,root)){
  for(p=i;p!=root;p=d[p].parent)if(n[p].currentWeight==0)break;if(p==root)return 1;
 }return 0;
}
static int C1AGT14_ClearInner(const c1agt14Definition_t*d,c1atl14Node_t*n,unsigned count,unsigned target,float fade){
 c1atl14Node_t*stage;unsigned i,depth,maxDepth=0,p;int ok=0;
 if(target>=count||!C1ATL14_Finite(fade)||fade<0||!C1AGT14_Valid(d,n,count))return 0;
 stage=(c1atl14Node_t*)malloc(count*sizeof(*stage));if(!stage)return 0;memcpy(stage,n,count*sizeof(*stage));
 /* Native recursion processes the parent before its children. Current weights
  * can change on an inactive path, so preserve this order for every level. */
 for(i=0;i<count;i++){for(p=i,depth=0;p;p=d[p].parent)depth++;if(depth>maxDepth)maxDepth=depth;}
 for(depth=0;depth<=maxDepth;depth++)for(i=0;i<count;i++)if(C1AGT14_Descendant(d,i,target)){
  unsigned z=0;for(p=i;p;p=d[p].parent)z++;if(z==depth&&!C1ATL14_ClearGoal(&stage[i],fade,C1AGT14_Active(d,stage,count,i)))goto done;
 }
 memcpy(n,stage,count*sizeof(*n));ok=1;
done:free(stage);return ok;
}
static int C1AGT14_SetAnimInner(const c1agt14Definition_t*d,c1atl14Node_t*n,unsigned count,unsigned target,float weight,float time,float rate,int forceRestart){
 c1atl14Node_t*stage;unsigned p,sync,targetParent,i;int ok=0;
 if(target>=count||(forceRestart!=0&&forceRestart!=1)||!C1AGT14_Valid(d,n,count))return 0;
 stage=(c1atl14Node_t*)malloc(count*sizeof(*stage));if(!stage)return 0;memcpy(stage,n,count*sizeof(*stage));
 if(!C1ATL14_SetGoal(&stage[target],weight,time,target==0?1:rate,target==0))goto done;
 for(p=target;p;){p=d[p].parent;if(stage[p].goalWeight==0&&!C1ATL14_SetGoal(&stage[p],1,time,1,p==0))goto done;}
 sync=target;targetParent=d[target].parent;
 while(sync&&(!d[sync].children||!(d[sync].flags&4)))sync=d[sync].parent;
 if(!sync){
  if(!d[target].children&&(forceRestart||!C1AGT14_Active(d,stage,count,target))){stage[target].phase=stage[target].oldPhase=0;stage[target].cycle=stage[target].oldCycle=0;}
 }else{
  if(forceRestart||!C1AGT14_Active(d,stage,count,sync)){stage[sync].phase=stage[sync].oldPhase=0;stage[sync].cycle=stage[sync].oldCycle=0;}
  for(i=0;i<count;i++)if(C1AGT14_Descendant(d,i,target)){stage[i].phase=stage[sync].phase;stage[i].oldPhase=stage[sync].oldPhase;stage[i].cycle=stage[sync].cycle;stage[i].oldCycle=stage[sync].oldCycle;}
  targetParent=d[sync].parent;for(p=target;p!=targetParent;p=d[p].parent){stage[p].phase=stage[sync].phase;stage[p].oldPhase=stage[sync].oldPhase;stage[p].cycle=stage[sync].cycle;stage[p].oldCycle=stage[sync].oldCycle;}
 }
 memcpy(n,stage,count*sizeof(*n));ok=1;
done:free(stage);return ok;
}
static int C1AGT14_Clear(const c1agt14Definition_t*d,c1atl14Node_t*n,unsigned count,unsigned target,float fade){
 int ok;
#if defined(_MSC_VER) && defined(_M_IX86)
 unsigned char outer[108];uint32_t savedMx,nearestMx=0x1f80;const unsigned short cw=0x027f;
 __asm fnsave outer
 __asm stmxcsr savedMx
 __asm fldcw cw
 __asm ldmxcsr nearestMx
 ok=C1AGT14_ClearInner(d,n,count,target,fade);
 __asm ldmxcsr savedMx
 __asm frstor outer
#else
 ok=C1AGT14_ClearInner(d,n,count,target,fade);
#endif
 return ok;
}
static int C1AGT14_SetAnim(const c1agt14Definition_t*d,c1atl14Node_t*n,unsigned count,unsigned target,float weight,float time,float rate,int forceRestart){
 int ok;
#if defined(_MSC_VER) && defined(_M_IX86)
 unsigned char outer[108];uint32_t savedMx,nearestMx=0x1f80;const unsigned short cw=0x027f;
 __asm fnsave outer
 __asm stmxcsr savedMx
 __asm fldcw cw
 __asm ldmxcsr nearestMx
 ok=C1AGT14_SetAnimInner(d,n,count,target,weight,time,rate,forceRestart);
 __asm ldmxcsr savedMx
 __asm frstor outer
#else
 ok=C1AGT14_SetAnimInner(d,n,count,target,weight,time,rate,forceRestart);
#endif
 return ok;
}
#endif
