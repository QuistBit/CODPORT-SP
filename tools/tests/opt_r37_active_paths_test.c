/* Differential test of the production active-path proof and clear path.
 * Compile without fast-math. The reference is the R36 algorithm. */
#include <assert.h>
#include <stdio.h>
#include <time.h>
#include "../../src/qcommon/cod1_actor_generic_tree14.h"
#define N 4096u
static c1agt14Definition_t d[N];
static c1atl14Node_t original[N],reference[N],candidate[N];
static unsigned char mask[N];
static unsigned perm[N];
static uint32_t rng=0x9e3779b9u;
static uint32_t random32(void){rng^=rng<<13;rng^=rng>>17;rng^=rng<<5;return rng;}
static float bits(uint32_t u){float f;memcpy(&f,&u,4);return f;}
/* Old full-tree predicate, including its nonzero ancestors and positive leaf. */
static int RefActive(const c1agt14Definition_t*defs,const c1atl14Node_t*nodes,unsigned count,unsigned root){
 unsigned i,p;for(p=root;;p=defs[p].parent){if(nodes[p].currentWeight==0)return 0;if(!p)break;}
 for(i=0;i<count;i++)if(!defs[i].children&&nodes[i].currentWeight>0&&C1AGT14_Descendant(defs,i,root)){
  for(p=i;p!=root;p=defs[p].parent)if(nodes[p].currentWeight==0)break;
  if(p==root)return 1;
 }return 0;
}
/* Same depth/ordinal ordering and same ClearGoal arithmetic as R36. */
static int RefClear(const c1agt14Definition_t*defs,c1atl14Node_t*nodes,unsigned count,unsigned target,float fade){
 unsigned i,p,depth,maxDepth=0,first[65],last[65],next[N];
 if(target>=count||!C1ATL14_Finite(fade)||fade<0||!C1AGT14_Valid(defs,nodes,count))return 0;
 memset(first,0xff,sizeof(first));memset(last,0xff,sizeof(last));
 for(i=0;i<count;i++){
  for(p=i,depth=0;p;p=defs[p].parent)depth++;
  if(depth>maxDepth)maxDepth=depth;
  if(!C1AGT14_Descendant(defs,i,target))continue;
  next[i]=~0u;if(last[depth]!=~0u)next[last[depth]]=i;else first[depth]=i;last[depth]=i;
 }
 for(depth=0;depth<=maxDepth;depth++)for(i=first[depth];i!=~0u;i=next[i])
  if(!C1ATL14_ClearGoal(nodes+i,fade,RefActive(defs,nodes,count,i)))return 0;
 return 1;
}
static void MakeTree(unsigned count,int forward,int mode){
 unsigned i,j,p;
 memset(d,0,count*sizeof(*d));memset(original,0,count*sizeof(*original));
 for(i=0;i<count;i++)perm[i]=i;
 if(forward)for(i=count-1;i>1;i--){j=1+random32()%i;p=perm[i];perm[i]=perm[j];perm[j]=p;}
 for(i=1;i<count;i++){
  p=mode==0?(i-1)/3:(mode==1?0:(i<64?i-1:(i-1)/3));
  d[perm[i]].parent=perm[p];d[perm[p]].children++;
 }
 for(i=0;i<count;i++){
  static const uint32_t w[]={0,0x80000000u,1,0x00800000u,0x3a83126fu,0x3e800000u,0x3f800000u};
  original[i].currentWeight=bits(w[random32()%7]);original[i].goalWeight=bits(w[random32()%7]);
  original[i].remaining=bits(w[random32()%7]);original[i].phase=bits(w[random32()%7]);
  original[i].oldPhase=bits(w[random32()%7]);original[i].rate=1;original[i].cycle=random32()%65536;original[i].oldCycle=random32()%65536;
  d[i].flags=random32()%16;
 }
 if(mode==1)original[0].currentWeight=1;
 assert(C1AGT14_Valid(d,original,count));
}
static void Check(unsigned count,unsigned target,float fade){
 unsigned i;
 C1AGT37_ActivePaths(d,original,count,mask);
 for(i=0;i<count;i++)assert(mask[i]==RefActive(d,original,count,i));
 memcpy(reference,original,count*sizeof(*original));memcpy(candidate,original,count*sizeof(*original));
 assert(RefClear(d,reference,count,target,fade));assert(C1AGT14_Clear(d,candidate,count,target,fade));
 assert(!memcmp(reference,candidate,count*sizeof(*candidate)));
 /* The original proof still holds after each completed clear. */
 for(i=0;i<count;i++)assert(mask[i]==RefActive(d,candidate,count,i));
}
int main(void){
 unsigned i,j,counts[]={1,2,7,31,64,127,2078,4096};
 for(i=0;i<sizeof(counts)/sizeof(counts[0]);i++)for(j=0;j<20;j++){
  unsigned count=counts[i];
  /* Mode 2 has a bounded 63-deep prefix only for small trees. */
  MakeTree(count,j&1,(j%3==2&&count>64)?0:j%3);
  Check(count,0,0);Check(count,random32()%count,.2f);Check(count,count-1,2.f);
 }
 /* All active, all dormant, and a single live final leaf. */
 MakeTree(2078,0,0);
 for(i=0;i<2078;i++)original[i].currentWeight=1;
 Check(2078,0,.2f);Check(2078,1,0);
 for(i=0;i<2078;i++)original[i].currentWeight=0;
 Check(2078,0,.2f);
 for(i=2077;;i=d[i].parent){original[i].currentWeight=1;if(!i)break;}
 Check(2078,0,.2f);
 memcpy(candidate,original,2078*sizeof(*candidate));
 assert(!C1AGT14_Clear(d,candidate,2078,2078,0));
 assert(!C1AGT14_Clear(d,candidate,2078,0,-1));
 assert(!memcmp(candidate,original,2078*sizeof(*candidate)));
 d[1].parent=2078;
 assert(!C1AGT14_Clear(d,candidate,2078,0,0));
 assert(!memcmp(candidate,original,2078*sizeof(*candidate)));
 puts("R37 active-path and clear differential: PASS");return 0;
}
