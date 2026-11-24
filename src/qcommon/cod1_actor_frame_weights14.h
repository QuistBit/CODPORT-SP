#ifndef COD1_ACTOR_FRAME_WEIGHTS14_H
#define COD1_ACTOR_FRAME_WEIGHTS14_H
/* Server numeric projection of ENGINE47B190/47B090. An inactive ancestor
 * snaps descendant current weights to goals without copying their old phases.
 * This does NOT allocate/free native slots, clear notify ownership, or prove a
 * runtime node absent. Opposite-half lifetime remains with its actual owner. */
#include "cod1_actor_generic_tree14.h"
typedef struct {c1atl14Node_t*n;const c1agt14Definition_t*d;unsigned*first,*next;float dt;} c1afw14Context_t;
static int C1AFW14_Node(c1afw14Context_t*c,unsigned i,int prune){
 c1atl14Node_t*n=c->n+i,value;unsigned child;
 if(prune){n->remaining=0;n->currentWeight=n->goalWeight;}
 else{
  n->oldPhase=n->phase;n->oldCycle=n->cycle;
  if(!C1ATL14_AdvanceWeightInner(n,c->dt,&value))return 0;*n=value;
 }
 if(!n->currentWeight&&!n->goalWeight)prune=1;
 for(child=c->first[i];child!=~0u;child=c->next[child])if(!C1AFW14_Node(c,child,prune))return 0;
 return 1;
}
#if defined(_MSC_VER)
__declspec(noinline)
#endif
static int C1AFW14_Inner(const c1agt14Definition_t*d,c1atl14Node_t*n,unsigned count,float dt){
 c1afw14Context_t c;c1atl14Node_t*stage=NULL;unsigned*topology=NULL,*last,i;int ok=0;
 if(!C1AGT14_Valid(d,n,count)||!C1ATL14_Finite(dt)||dt<0||dt>60)return 0;
 stage=malloc(count*sizeof(*stage));topology=malloc(3*count*sizeof(*topology));if(!stage||!topology)goto done;
 memcpy(stage,n,count*sizeof(*stage));memset(topology,0xff,3*count*sizeof(*topology));
 c.n=stage;c.d=d;c.first=topology;c.next=topology+count;c.dt=dt;last=topology+count*2;
 for(i=1;i<count;i++){unsigned p=d[i].parent;if(last[p]==~0u)c.first[p]=i;else c.next[last[p]]=i;last[p]=i;}
 if(!C1AFW14_Node(&c,0,0))goto done;
 memcpy(n,stage,count*sizeof(*stage));ok=1;
done:free(stage);free(topology);return ok;
}
static int C1AFW14_Advance(const c1agt14Definition_t*d,c1atl14Node_t*n,unsigned count,float dt){
 int ok;
#if defined(_MSC_VER)&&defined(_M_IX86)
 unsigned char saved[108],discard[108];uint32_t mx,nearMx=0x1f80;const unsigned short cw=0x027f;
 __asm fnsave saved
 __asm stmxcsr mx
 __asm fldcw cw
 __asm ldmxcsr nearMx
 ok=C1AFW14_Inner(d,n,count,dt);
 __asm fnsave discard
 __asm ldmxcsr mx
 __asm frstor saved
#else
 ok=C1AFW14_Inner(d,n,count,dt);
#endif
 return ok;
}
/* Only for a disposable transaction clone: the caller discards the entire
 * clone on failure. Keep the public atomic API for every other consumer.
 * In parent-first storage each node needs only its parent's final prune bit.
 * Siblings share no state; retain the same FP wrapper and node operation. */
static int C1AFW601_PrivateInner(const c1agt14Definition_t*d,c1atl14Node_t*n,unsigned count,float dt){
 unsigned char prune[4096];unsigned i;
 if(!count||count>4096||!C1AGT14_Valid(d,n,count)||!C1ATL14_Finite(dt)||dt<0||dt>60)return 0;
 for(i=1;i<count;i++)if(d[i].parent>=i)return C1AFW14_Inner(d,n,count,dt);
 for(i=0;i<count;i++){
  c1atl14Node_t *node=n+i,value;
  if(i && prune[d[i].parent]){node->remaining=0;node->currentWeight=node->goalWeight;prune[i]=1;}
  else {
   node->oldPhase=node->phase;node->oldCycle=node->cycle;
   if(!C1ATL14_AdvanceWeightInner(node,dt,&value))return 0;
   *node=value;prune[i]=!node->currentWeight&&!node->goalWeight;
  }
 }
 return 1;
}
static int C1AFW601_Private(const c1agt14Definition_t*d,c1atl14Node_t*n,unsigned count,float dt){
 int ok;
#if defined(_MSC_VER)&&defined(_M_IX86)
 unsigned char saved[108],discard[108];uint32_t mx,nearMx=0x1f80;const unsigned short cw=0x027f;
 __asm fnsave saved
 __asm stmxcsr mx
 __asm fldcw cw
 __asm ldmxcsr nearMx
 ok=C1AFW601_PrivateInner(d,n,count,dt);
 __asm fnsave discard
 __asm ldmxcsr mx
 __asm frstor saved
#else
 ok=C1AFW601_PrivateInner(d,n,count,dt);
#endif
 return ok;
}
#endif
