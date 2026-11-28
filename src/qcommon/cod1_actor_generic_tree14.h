#ifndef COD1_ACTOR_GENERIC_TREE14_H
#define COD1_ACTOR_GENERIC_TREE14_H
/* Server-context ENGINE97/109 ownership-neutral node operations. The owner
 * supplies the real parsed hierarchy, including sync flags, and current nodes.
 * No catalog/native ID equivalence, clip sampling, notify, mount or tree lease
 * is manufactured here. All changes commit together after bounded validation. */
#include "cod1_actor_tree_clock14.h"
#include <stdlib.h>
#include "cod1_actor_definition14.h"
static int C1AGT14_ValidScalar(const c1agt14Definition_t*d,const c1atl14Node_t*n,unsigned count){
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
/* Exact per-thread topology proof. Compare the complete definition bytes
 * on each call, and validate every live node on each call. Exceptional IEEE
 * encodings retain the original scalar path and its FP behavior. */
#if defined(_MSC_VER) && defined(_M_IX86) && _M_IX86_FP >= 2
#include "cod1_exact_bytes_r242.h"
static __declspec(thread) struct {unsigned count;c1agt14Definition_t definitions[4096];} c1agtTopology;
static __forceinline int C1AGT14_FourDefault(const c1atl14Node_t*n){
 const __m128i zero=_mm_setzero_si128(),last=_mm_setr_epi32(0,0x3f800000,0,0);
 __m128i first=_mm_or_si128(_mm_loadu_si128((const __m128i*)(n)),_mm_loadu_si128((const __m128i*)(n+1)));
 __m128i tail=_mm_or_si128(_mm_xor_si128(_mm_loadu_si128((const __m128i*)&n[0].currentWeight),last),_mm_xor_si128(_mm_loadu_si128((const __m128i*)&n[1].currentWeight),last));
 first=_mm_or_si128(first,_mm_or_si128(_mm_loadu_si128((const __m128i*)(n+2)),_mm_loadu_si128((const __m128i*)(n+3))));
 tail=_mm_or_si128(tail,_mm_or_si128(_mm_xor_si128(_mm_loadu_si128((const __m128i*)&n[2].currentWeight),last),_mm_xor_si128(_mm_loadu_si128((const __m128i*)&n[3].currentWeight),last)));
 return _mm_movemask_epi8(_mm_cmpeq_epi8(_mm_or_si128(first,tail),zero))==65535;
}
static int C1AGT14_Valid(const c1agt14Definition_t*d,const c1atl14Node_t*nodes,unsigned count){
 const __m128i zero=_mm_setzero_si128();
 const __m128i firstNormal=_mm_set1_epi32(0x00800000);
 const __m128i lastNormal=_mm_setr_epi32(0x00800000,0x00800000,0,0);
 const __m128i firstMax=_mm_setr_epi32(0x3f800000,0x3f800000,0x7f7fffff,0x3f800000);
 const __m128i lastMax=_mm_setr_epi32(0x3f800000,0x42c80000,65535,65535);
 unsigned i;
 _Static_assert(sizeof(c1atl14Node_t)==32 && offsetof(c1atl14Node_t,oldCycle)==28 &&
   offsetof(c1atl14Node_t,rate)==20 && offsetof(c1atl14Node_t,goalWeight)==12,"animation node layout");
 if(!d||!nodes||!count||count>4096)return 0;
 {uintptr_t a=(uintptr_t)d,b=(uintptr_t)nodes;size_t da=count*sizeof(*d),nb=count*sizeof(*nodes);if(a<=b?b-a<da:a-b<nb)return 0;}
 if(c1agtTopology.count!=count || !R242ExactATRBytesEqual(c1agtTopology.definitions,d,count*sizeof(*d))){
  if(!C1AGT14_ValidScalar(d,nodes,count))return 0;
  memcpy(c1agtTopology.definitions,d,count*sizeof(*d));
  c1agtTopology.count=count;return 1;
 }
 for(i=0;i<count;i++){
  if(i+4<=count&&C1AGT14_FourDefault(nodes+i)){i+=3;continue;}
  const __m128i first=_mm_loadu_si128((const __m128i *)&nodes[i].phase);
  const __m128i last=_mm_loadu_si128((const __m128i *)&nodes[i].currentWeight);
  const __m128i negative=_mm_cmpgt_epi32(zero,_mm_or_si128(first,last));
  const __m128i high=_mm_or_si128(_mm_cmpgt_epi32(first,firstMax),_mm_cmpgt_epi32(last,lastMax));
  /* Scalar comparisons on a subnormal may set the MXCSR denormal flag
   * or trap when unmasked. Run the original comparisons for these nodes. */
  const __m128i lowFirst=_mm_andnot_si128(_mm_cmpeq_epi32(first,zero),_mm_cmpgt_epi32(firstNormal,first));
  const __m128i lowLast=_mm_andnot_si128(_mm_cmpeq_epi32(last,zero),_mm_cmpgt_epi32(lastNormal,last));
  const __m128i exceptional=_mm_or_si128(_mm_or_si128(lowFirst,lowLast),_mm_or_si128(negative,high));
  if(_mm_movemask_epi8(exceptional))
   return C1AGT14_ValidScalar(d,nodes,count);
 }
 return 1;
}
#else
static int C1AGT14_Valid(const c1agt14Definition_t*d,const c1atl14Node_t*n,unsigned count){return C1AGT14_ValidScalar(d,n,count);}
#endif
static int C1AGT14_Descendant(const c1agt14Definition_t*d,unsigned node,unsigned root){
 for(;;){if(node==root)return 1;if(!node)return 0;node=d[node].parent;}
}
static int C1AGT14_Active(const c1agt14Definition_t*d,const c1atl14Node_t*n,unsigned count,unsigned root){
 unsigned i,p;for(p=root;;p=d[p].parent){if(n[p].currentWeight==0)return 0;if(!p)break;}
 for(i=0;i<count;i++)if(!d[i].children&&n[i].currentWeight>0&&C1AGT14_Descendant(d,i,root)){
  for(p=i;p!=root;p=d[p].parent)if(n[p].currentWeight==0)break;if(p==root)return 1;
 }return 0;
}
#include "cod1_actor_active_paths37.h"
/* Only a canonical, already-cleared inactive node is a no-op. The caller
 * has validated the entire tree. Keep signed zeros and denormals on the
 * original path, including unusual fade encodings. No phase/rate writes. */
static int C1AGT49_ClearDormant(const c1atl14Node_t*n,int ordinaryFade,unsigned active){
 uint32_t remaining,goal,current;
 if(!ordinaryFade||active)return 0;
 memcpy(&remaining,&n->remaining,4);memcpy(&goal,&n->goalWeight,4);memcpy(&current,&n->currentWeight,4);
 return (remaining|goal|current)==0;
}
static int C1AGT14_ClearInner(const c1agt14Definition_t*d,c1atl14Node_t*n,unsigned count,unsigned target,float fade){
 c1atl14Node_t*stage;unsigned i,depth,maxDepth=0,p;int ok=0;
 unsigned first[65],last[65],next[4096];unsigned char active[4096];
 uint32_t fadeBits;int ordinaryFade;
 if(target>=count||!C1ATL14_Finite(fade)||fade<0||!C1AGT14_Valid(d,n,count))return 0;
 memcpy(&fadeBits,&fade,4);ordinaryFade=fadeBits==0||(fadeBits>=0x00800000u&&fadeBits<0x7f800000u);
 stage=(c1atl14Node_t*)malloc(count*sizeof(*stage));if(!stage)return 0;memcpy(stage,n,count*sizeof(*stage));
 /* Native recursion processes the parent before its children. Current weights
  * can change on an inactive path, so preserve this order for every level. */
 /* Valid has checked every parent and the depth<=64 bound. Build only
  * integer topology here, with no callbacks or mutation. Each depth retains
  * ascending original ordinals, including trees with forward parent indices. */
 memset(first,0xff,sizeof(first));memset(last,0xff,sizeof(last));
 for(i=0;i<count;i++){
  for(p=i,depth=0;p;p=d[p].parent)depth++;if(depth>maxDepth)maxDepth=depth;
  if(!C1AGT14_Descendant(d,i,target))continue;
  next[i]=~0u;if(last[depth]!=~0u)next[last[depth]]=i;else first[depth]=i;last[depth]=i;
 }
 /* Only the current-weight reachability is precomputed. Nontrivial
  * ClearGoal calls retain the original depth/ordinal order. */
 if(d[target].children)C1AGT37_ActivePaths(d,stage,count,active);
 else active[target]=(unsigned char)C1AGT14_Active(d,stage,count,target);
 for(depth=0;depth<=maxDepth;depth++)for(i=first[depth];i!=~0u;i=next[i])
  if(!C1AGT49_ClearDormant(&stage[i],ordinaryFade,active[i])&&
     !C1ATL14_ClearGoal(&stage[i],fade,active[i]))goto done;
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
