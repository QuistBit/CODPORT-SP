/* Compare the pinned R40 clone allocator with the production R45C header.
 * Minimal record/lease fixtures; no real GAME callbacks. */
#include <assert.h>
#include <stddef.h>
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
static void *trackedMalloc(size_t n){void*p;++calls;if(calls==failAt)return NULL;p=malloc(n);assert(p);++live;memset(p,0xab,n);return p;}
static void trackedFree(void*p){if(p){assert(live);--live;free(p);}}
#define malloc trackedMalloc
#define free trackedFree
#define ActorCloneStorage RefStorage
#define actorClonePool refPool
#define ActorClonePoolClear RefClear
#define ActorGenericFreeOwned RefFree
#define ActorGenericClone RefClone
#include "reference.inc"
#undef ActorCloneStorage
#undef actorClonePool
#undef ActorClonePoolClear
#undef ActorGenericFreeOwned
#undef ActorGenericClone
#include "g_cod1_actor_clone_storage45.h"
#undef malloc
#undef free
static void empty(const C1GARecord*r){assert(!r->nodes&&!r->notify&&!r->names);}
static void shape(const C1GARecord*r){
 assert(r->nodes&&r->notify&&r->names);
 assert((uintptr_t)r->notify==(uintptr_t)r->nodes+C1GA_COUNT*sizeof(*r->nodes));
 assert((uintptr_t)r->names==(uintptr_t)r->notify+sizeof(*r->notify));
}
static void compare(const C1GARecord*a,const C1GARecord*b){
 C1GARecord h=*a;h.nodes=b->nodes;h.notify=b->notify;h.names=b->names;
 assert(!memcmp(&h,b,sizeof(h))&&a->nodes!=b->nodes&&a->notify!=b->notify&&a->names!=b->names);
 assert(!memcmp(a->nodes,b->nodes,C1GA_COUNT*sizeof(*a->nodes)));
 assert(!memcmp(a->notify,b->notify,sizeof(*a->notify)));
 assert(!memcmp(a->names,b->names,a->notify->nameCount*sizeof(*a->names)));
}
static void transfer(C1GARecord*dst,C1GARecord*src,void(*release)(C1GARecord*)){
 release(dst);*dst=*src;src->nodes=NULL;src->notify=NULL;src->names=NULL;
}
int main(void){
 C1GARecord old={0},now={0},a[4],b[4],x={0},y={0},invalid;unsigned i,j,k,before,oldCold,newCold;size_t bytes=sizeof(ActorCloneBlock45);
 old.nodes=trackedMalloc(C1GA_COUNT*sizeof(*old.nodes));old.notify=trackedMalloc(sizeof(*old.notify));old.names=trackedMalloc(C1GA_COUNT*sizeof(*old.names));
 memset(old.nodes,0,C1GA_COUNT*sizeof(*old.nodes));memset(old.notify,0,sizeof(*old.notify));
 before=calls;assert(ActorGenericAllocateBlock45(&now,1)&&calls-before==1);shape(&now);compare(&old,&now);
 for(i=0;i<C1GA_COUNT;i++){
  old.nodes[i].rate=now.nodes[i].rate=1;old.notify->flags[i].cursor=now.notify->flags[i].cursor=-1;
 }
 for(i=0;i<3;i++){before=calls;failAt=calls+i+1;assert(!RefClone(&x,&old));assert(calls-before==3);empty(&x);assert(live==4);}
 before=calls;failAt=calls+1;assert(!ActorGenericClone(&y,&now));assert(calls-before==1);empty(&y);assert(live==4);
 before=calls;failAt=calls+1;assert(!ActorGenericAllocateBlock45(&y,1));assert(calls-before==1);empty(&y);failAt=0;
 invalid=now;invalid.notify=NULL;before=calls;assert(!ActorGenericClone(&y,&invalid));empty(&y);assert(calls==before);
 old.notify->nameCount=now.notify->nameCount=C1GA_COUNT+1;assert(!ActorGenericClone(&y,&now));assert(!RefClone(&x,&old));empty(&x);empty(&y);assert(calls==before);
 old.notify->nameCount=now.notify->nameCount=0;
 before=calls;for(i=0;i<4;i++)assert(RefClone(&a[i],&old));oldCold=calls-before;
 before=calls;for(i=0;i<4;i++)assert(ActorGenericClone(&b[i],&now));newCold=calls-before;assert(oldCold==12&&newCold==4);
 for(i=0;i<4;i++){compare(&a[i],&b[i]);RefFree(&a[i]);ActorGenericFreeOwned(&b[i]);}
 RefClear();ActorClonePoolClear();assert(live==4);
 for(i=0;i<128;i++){
  unsigned count=(i%4==0)?C1GA_COUNT:i*13u%C1GA_COUNT;
  old.notify->nameCount=now.notify->nameCount=count;old.revision=now.revision=i+1;
  for(j=0;j<C1GA_COUNT;j++){
   old.nodes[j].cycle=now.nodes[j].cycle=i*123u+j;old.notify->pending[j]=now.notify->pending[j]=i+j;
   if(j<count){old.names[j].ordinal=now.names[j].ordinal=j;memset(old.names[j].text,(int)((i+j)&255),64);memset(now.names[j].text,(int)((i+j)&255),64);}
  }
  for(j=0;j<4;j++){assert(RefClone(&a[j],&old)&&ActorGenericClone(&b[j],&now));shape(&b[j]);compare(&a[j],&b[j]);for(k=0;k<j;k++)assert(b[j].nodes!=b[k].nodes&&b[j].notify!=b[k].notify&&b[j].names!=b[k].names);}
  a[0].nodes[i].cycle++;b[0].nodes[i].cycle++;a[0].notify->pending[i]++;b[0].notify->pending[i]++;compare(&a[0],&b[0]);compare(&old,&now);assert(now.nodes[i].cycle==b[1].nodes[i].cycle);
  /* Commit publishes whole stage and clears donor; controller rebase does the same. */
  transfer(&old,&a[0],RefFree);transfer(&now,&b[0],ActorGenericFreeOwned);empty(&a[0]);empty(&b[0]);compare(&old,&now);
  transfer(&a[1],&a[2],RefFree);transfer(&b[1],&b[2],ActorGenericFreeOwned);compare(&a[1],&b[1]);empty(&a[2]);empty(&b[2]);
  RefClear();ActorClonePoolClear();compare(&a[1],&b[1]);compare(&a[3],&b[3]);compare(&old,&now);
  for(j=0;j<4;j++){RefFree(&a[j]);ActorGenericFreeOwned(&b[j]);empty(&a[j]);empty(&b[j]);}
  assert(refPool[0].nodes&&refPool[1].nodes&&actorClonePool[0].nodes&&actorClonePool[1].nodes);
  before=calls;failAt=calls+1;assert(RefClone(&x,&old)&&ActorGenericClone(&y,&now));assert(calls==before);failAt=0;compare(&x,&y);RefFree(&x);ActorGenericFreeOwned(&y);
 }
 RefFree(&old);ActorGenericFreeOwned(&now);RefClear();ActorClonePoolClear();RefClear();ActorClonePoolClear();assert(!live);
 printf("R45C: 128 nested transfer/reset cycles, 4 cold clones: %u vs %u malloc calls, block bytes=%zu; PASS\n",oldCold,newCold,bytes);return 0;
}
