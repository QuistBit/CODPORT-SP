/* Standalone host projection differential: generate includes with the shell
 * driver. Resource lookup is a deterministic stub, not native GAME. */
#include <assert.h>
#include <stdio.h>
#include <limits.h>
#define C1GA_COUNT 2078u
#define C1AGO14_PREPARED_CAPACITY C1GA_COUNT
#include "cod1_actor_prepared_order14.h"
typedef struct {float phase,goalWeight,currentWeight;} c1atl14Node_t;
typedef struct {unsigned ordinal,definition_children;int parent,leaf,runtime_present;float goalWeight,currentWeight,phase;} c1arc14Node_t;
typedef struct {unsigned id;} c1ar14Root_t;
typedef struct {unsigned id;} c1ab14Clip_t;
#define PROJECTION_FIELDS \
 c1atl14Node_t working[C1GA_COUNT]; \
 unsigned source[128],leafSource[82],count,leaves; \
 c1arc14Node_t nodes[128];const c1ar14Root_t *roots[82];const c1ab14Clip_t *bodies[82];
typedef struct {int first[C1GA_COUNT],next[C1GA_COUNT];unsigned order[C1GA_COUNT],frontier;} C1ActorRootOrderScratch;
typedef struct {PROJECTION_FIELDS int first[C1GA_COUNT],next[C1GA_COUNT];unsigned order[C1GA_COUNT],frontier;} LegacyRoot;
typedef struct {PROJECTION_FIELDS C1ActorRootOrderScratch scratch;} C1ActorCurrentRoot;
static c1agt14Definition_t cod1GenericDefinitions[C1GA_COUNT];
static c1ago14Prepared_t cod1GenericSourceOrder,saved;
static uint64_t cod1EntityEpoch,cod1GenericEpoch,cod1TurretAnimationReceipt,cod1GenericReceipt;
static int cod1EntityActive;
static c1ar14Root_t roots[C1GA_COUNT];static c1ab14Clip_t bodies[C1GA_COUNT];
static unsigned trace[128],oldTrace[128],traceCount,missing;
static int ActorCurrentRootResource(const void *v,unsigned source,const c1ar14Root_t **root,const c1ab14Clip_t **body){
 (void)v;assert(traceCount<128);trace[traceCount++]=source;
 if(source==missing)return 0;
 *root=roots+source;*body=bodies+source;return 1;
}
#include "r44b2_old.inc"
#include "r44b2_new.inc"
static LegacyRoot old;
static C1ActorCurrentRoot now;
static C1ActorRootOrderScratch untouched;
static uint32_t rng=0xf16a378du;
static unsigned rnd(void){rng^=rng<<13;rng^=rng>>17;rng^=rng<<5;return rng;}
static unsigned comparisons,hits,misses,successes,refusals;
static void make(unsigned shape){
 unsigned i,p;memset(cod1GenericDefinitions,0,sizeof(cod1GenericDefinitions));
 for(i=1;i<C1GA_COUNT;i++){
  p=shape==0?0:shape==1?(i-1)/2:shape==2?rnd()%i:i<64?i-1:0;
  cod1GenericDefinitions[i].parent=p;cod1GenericDefinitions[p].children++;
 }
}
static void compare(unsigned selected,unsigned density){
 unsigned i,n;int a,b,hit;
 memset(&old,0,sizeof(old));memset(&now,0,sizeof(now));
 memset(old.first,0xa5,sizeof(old.first));memset(old.next,0xa5,sizeof(old.next));
 memset(old.order,0xa5,sizeof(old.order));old.frontier=0xa5a5a5a5u;
 memset(&now.scratch,0xa5,sizeof(now.scratch));untouched=now.scratch;
 for(i=0;i<C1GA_COUNT;i++){
  old.working[i].goalWeight=(rnd()%100<density)?(float)(1+rnd()%8)/8:0;
  old.working[i].currentWeight=(float)(rnd()%9)/8;old.working[i].phase=(float)(rnd()%9)/8;
 }
 memcpy(now.working,old.working,sizeof(old.working));
 saved=cod1GenericSourceOrder;
 hit=cod1EntityActive&&cod1EntityEpoch==cod1GenericEpoch&&cod1TurretAnimationReceipt==cod1GenericReceipt&&
  saved.ready&&cod1GenericEpoch&&cod1GenericReceipt&&saved.epoch==cod1GenericEpoch&&saved.receipt==cod1GenericReceipt&&saved.count==C1GA_COUNT;
 assert((ActorCurrentRootPrepared()!=NULL)==hit);
 traceCount=0;a=LegacyOrder(&old)&&LegacyCollect(&old,selected,-1,0);n=traceCount;memcpy(oldTrace,trace,n*sizeof(*trace));
 traceCount=0;b=ActorCurrentRootProject(&now,selected);
 assert(a==b&&old.count==now.count&&old.leaves==now.leaves&&n==traceCount);
 assert(!memcmp(oldTrace,trace,n*sizeof(*trace)));
 assert(!memcmp(old.nodes,now.nodes,sizeof(old.nodes))&&!memcmp(old.source,now.source,sizeof(old.source)));
 assert(!memcmp(old.roots,now.roots,sizeof(old.roots))&&!memcmp(old.bodies,now.bodies,sizeof(old.bodies)));
 assert(!memcmp(old.leafSource,now.leafSource,sizeof(old.leafSource))&&!memcmp(old.working,now.working,sizeof(old.working)));
 assert(!memcmp(&saved,&cod1GenericSourceOrder,sizeof(saved)));
 if(hit){assert(!memcmp(&untouched,&now.scratch,sizeof(untouched)));hits++;}
 else{assert(!memcmp(old.first,now.scratch.first,sizeof(old.first))&&!memcmp(old.next,now.scratch.next,sizeof(old.next))&&!memcmp(old.order,now.scratch.order,sizeof(old.order))&&old.frontier==now.scratch.frontier);misses++;}
 comparisons++;if(a)successes++;else refusals++;
}
int main(void){
 unsigned i,j;cod1EntityActive=1;cod1EntityEpoch=cod1GenericEpoch=5;
 for(i=0;i<120;i++){
  make(i%4);cod1GenericReceipt=cod1TurretAnimationReceipt=i+1;
  assert(C1AGO14_Prepare(&cod1GenericSourceOrder,cod1GenericDefinitions,C1GA_COUNT,5,i+1));
  missing=UINT_MAX;
  compare(0,0);compare(0,100);compare(1,100);compare(C1GA_COUNT,100);
  for(j=0;j<12;j++){missing=j%3?UINT_MAX:1+rnd()%(C1GA_COUNT-1);compare(j%2?rnd()%C1GA_COUNT:0,rnd()%101);}
  cod1EntityActive=0;compare(0,25);cod1EntityActive=1;
  cod1EntityEpoch=6;compare(0,25);cod1EntityEpoch=5;
  cod1TurretAnimationReceipt++;compare(0,25);cod1TurretAnimationReceipt--;
  cod1GenericSourceOrder.count--;compare(0,25);cod1GenericSourceOrder.count++;
  C1AGO14_Retire(&cod1GenericSourceOrder);compare(0,25);
  cod1GenericDefinitions[0].children=UINT_MAX;
  assert(!C1AGO14_Prepare(&cod1GenericSourceOrder,cod1GenericDefinitions,C1GA_COUNT,5,i+1));compare(0,25);
 }
 printf("R44B2 projection: %u comparisons, %u hits, %u misses, %u success, %u refusal; scratch=%zu bytes\n",comparisons,hits,misses,successes,refusals,sizeof(C1ActorRootOrderScratch));
 return 0;
}
