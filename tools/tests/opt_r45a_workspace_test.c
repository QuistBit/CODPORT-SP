/* Exercise the exact production clone-pool fragment extracted by the driver.
 * Synthetic header/lease; no GAME callbacks or FaceFrame execution. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define C1GA_COUNT 2078u
typedef struct {float phase,oldPhase,remaining,goalWeight,currentWeight,rate;unsigned cycle,oldCycle;} c1atl14Node_t;
typedef struct {uint16_t descendant;int16_t cursor;uint16_t token,clientType;} c1afl14State_t;
typedef struct {c1afl14State_t flags[C1GA_COUNT];unsigned nameCount;uint64_t generation[C1GA_COUNT],pending[C1GA_COUNT],serial,ownerIdentity;int32_t pendingTick[C1GA_COUNT];} C1GANotifyState;
typedef struct {unsigned ordinal;char text[64];} C1GANameEntry;
typedef struct {uint64_t words[6];} cod1ActorLease_t;
typedef struct {
 int used,tick,detachedMG,mgTick;cod1ActorLease_t lease;uint64_t runtime,catalog,revision;
 c1atl14Node_t *nodes;C1GANotifyState *notify;C1GANameEntry *names;uint32_t lookCells[16];
} C1GARecord;
static unsigned calls,live,failAt;
static void *trackedMalloc(size_t n){void *p;++calls;if(calls==failAt)return NULL;p=malloc(n);assert(p);++live;memset(p,0xab,n);return p;}
static void trackedFree(void *p){if(p){assert(live);--live;free(p);}}
#define malloc trackedMalloc
#define free trackedFree
#include "clone.inc"
#undef malloc
#undef free
static void empty(const C1GARecord *r){assert(!r->nodes&&!r->notify&&!r->names);}
static void equivalent(const C1GARecord *a,const C1GARecord *b){
 assert(a->nodes!=b->nodes&&a->notify!=b->notify&&a->names!=b->names);
 assert(a->revision==b->revision);
 assert(!memcmp(a->nodes,b->nodes,C1GA_COUNT*sizeof(*a->nodes)));
 assert(!memcmp(a->notify,b->notify,sizeof(*a->notify)));
 assert(!memcmp(a->names,b->names,a->notify->nameCount*sizeof(*a->names)));
}
int main(void){
 C1GARecord src={0},stage={0},base={0},outer={0},expected={0},invalid;
 unsigned i,j,initial; c1atl14Node_t *first;
 src.nodes=trackedMalloc(C1GA_COUNT*sizeof(*src.nodes));src.notify=trackedMalloc(sizeof(*src.notify));src.names=trackedMalloc(C1GA_COUNT*sizeof(*src.names));
 memset(src.nodes,0,C1GA_COUNT*sizeof(*src.nodes));memset(src.notify,0,sizeof(*src.notify));memset(src.names,0,C1GA_COUNT*sizeof(*src.names));src.revision=7;
 for(i=1;i<=3;i++){initial=calls;failAt=calls+i;assert(!ActorGenericClone(&stage,&src));empty(&stage);assert(calls==initial+3&&live==3&&!actorClonePool[0].nodes&&!actorClonePool[1].nodes);}
 failAt=0;invalid=src;invalid.notify=NULL;initial=calls;assert(!ActorGenericClone(&stage,&invalid));empty(&stage);assert(calls==initial);
 src.notify->nameCount=C1GA_COUNT+1;assert(!ActorGenericClone(&stage,&src));empty(&stage);assert(calls==initial);
 for(i=0;i<64;i++){
  src.notify->nameCount=i%2?C1GA_COUNT:i;src.notify->pending[i]=i+1;src.nodes[i].cycle=i+1;src.names[i].ordinal=i;
  assert(ActorGenericClone(&outer,&src)&&ActorGenericClone(&expected,&outer));
  assert(ActorGenericClone(&stage,&src)&&ActorGenericClone(&base,&src));
  equivalent(&src,&outer);equivalent(&outer,&expected);equivalent(&src,&stage);equivalent(&src,&base);
  assert(outer.nodes!=stage.nodes&&expected.nodes!=base.nodes&&stage.nodes!=base.nodes);
  stage.nodes[i].cycle++;stage.notify->pending[i]++;assert(base.nodes[i].cycle==src.nodes[i].cycle&&outer.notify->pending[i]==src.notify->pending[i]);
  first=stage.nodes;ActorGenericFreeOwned(&stage);empty(&stage);
  ActorGenericFreeOwned(&base);empty(&base);
  assert(actorClonePool[0].nodes==first&&actorClonePool[1].nodes);
  initial=calls;failAt=calls+1;assert(ActorGenericClone(&stage,&src));assert(stage.nodes==first&&calls==initial);equivalent(&src,&stage);failAt=0;
  /* Pool clear retires only idle storage. Checked-out outer/expected/stage survive. */
  ActorClonePoolClear();assert(!actorClonePool[0].nodes&&!actorClonePool[1].nodes);
  equivalent(&src,&outer);equivalent(&outer,&expected);equivalent(&src,&stage);
  ActorGenericFreeOwned(&stage);ActorGenericFreeOwned(&outer);ActorGenericFreeOwned(&expected);empty(&stage);empty(&outer);empty(&expected);
  assert(live==9); /* source + two idle triples; third release freed */
  for(j=0;j<2;j++)assert(actorClonePool[j].nodes&&actorClonePool[j].notify&&actorClonePool[j].names);
 }
 ActorGenericFreeOwned(&src);empty(&src);ActorClonePoolClear();ActorClonePoolClear();assert(!live);
 puts("R45A: 64 nested clone/release/clear cycles, 3 allocation faults, 2 invalid inputs: PASS");return 0;
}
