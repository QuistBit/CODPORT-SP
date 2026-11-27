#include <assert.h>
#include <stdio.h>
#include <limits.h>
#define C1AGO14_PREPARED_CAPACITY 2078u
#include "../../src/qcommon/cod1_actor_prepared_order14.h"
static c1ago14Prepared_t cache;
static c1agt14Definition_t defs[C1AGO14_PREPARED_CAPACITY];
typedef struct {
 int first[C1AGO14_PREPARED_CAPACITY],next[C1AGO14_PREPARED_CAPACITY];
 unsigned order[C1AGO14_PREPARED_CAPACITY],frontier;
} Scratch;
static Scratch ref,out,before;
static unsigned hits,misses;
static uint32_t rng=0x5d1a917bu;
static unsigned rnd(void){rng^=rng<<13;rng^=rng>>17;rng^=rng<<5;return rng;}
static void make(unsigned count,unsigned shape){
 unsigned i,p;memset(defs,0,sizeof(defs));
 for(i=1;i<count;i++){
  p=shape==0?0:shape==1?(i-1)/2:rnd()%i;
  defs[i].parent=p;defs[p].children++;defs[i].flags=rnd()&15;
 }
}
static void miss(unsigned count,uint64_t epoch,uint64_t receipt){
 memset(&out,0xa5,sizeof(out));before=out;
 assert(!C1AGO14_Copy(&cache,count,epoch,receipt,out.first,out.next,out.order,&out.frontier));
 assert(!memcmp(&out,&before,sizeof(out)));misses++;
}
static void compare(unsigned count,uint64_t epoch,uint64_t receipt){
 memset(&ref,0x5a,sizeof(ref));memset(&out,0x5a,sizeof(out));
 assert(C1AGO14_Build(defs,count,ref.first,ref.next,ref.order,&ref.frontier));
 assert(C1AGO14_Copy(&cache,count,epoch,receipt,out.first,out.next,out.order,&out.frontier));
 assert(!memcmp(&ref,&out,sizeof(ref)));hits++;
 /* Each consumer owns scratch, and cannot mutate the cache through it. */
 out.first[0]=-123;out.order[0]=UINT_MAX;
 assert(cache.first[0]==ref.first[0]&&cache.order[0]==ref.order[0]);
}
int main(void){
 unsigned i,j,count;uint64_t receipt;
 miss(2078,1,1);
 for(i=0;i<300;i++){
  count=i%2?2078:1+rnd()%2078;receipt=(uint64_t)i+1;
  make(count,i%3);
  assert(C1AGO14_Prepare(&cache,defs,count,5,receipt));
  for(j=0;j<7;j++)compare(count,5,receipt);
  miss(count,6,receipt);miss(count,5,receipt+1);miss(count,0,receipt);
  miss(count,5,0);miss(count==2078?2077:count+1,5,receipt);
  /* Reset can keep the same map epoch; old receipt must not grant access. */
  C1AGO14_Retire(&cache);miss(count,5,receipt);
  assert(C1AGO14_Prepare(&cache,defs,count,5,receipt+1));
  miss(count,5,receipt);compare(count,5,receipt+1);
  /* A failed rebuild must never expose an earlier successful result. */
  defs[0].children=UINT_MAX;
  assert(!C1AGO14_Prepare(&cache,defs,count,5,receipt+2));
  miss(count,5,receipt+1);miss(count,5,receipt+2);
 }
 make(2078,1);assert(C1AGO14_Prepare(&cache,defs,2078,UINT64_MAX,UINT64_MAX));
 compare(2078,UINT64_MAX,UINT64_MAX);
 assert(!C1AGO14_Prepare(&cache,defs,2079,1,1));miss(2078,UINT64_MAX,UINT64_MAX);
 assert(!C1AGO14_Prepare(&cache,defs,0,1,1));miss(0,1,1);
 assert(!C1AGO14_Prepare(&cache,defs,2078,0,1));miss(2078,0,1);
 assert(!C1AGO14_Prepare(&cache,defs,2078,1,0));miss(2078,1,0);
 assert(!C1AGO14_Prepare(&cache,NULL,2078,1,1));miss(2078,1,1);
 printf("R44B1: %u exact copies, %u unchanged misses\n",hits,misses);return 0;
}
