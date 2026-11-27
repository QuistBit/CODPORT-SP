#ifndef COD1_ACTOR_PREPARED_ORDER14_H
#define COD1_ACTOR_PREPARED_ORDER14_H
#include <stdint.h>
#include <string.h>
#include "cod1_actor_source_order14.h"
/* Main-thread private catalog storage. Capacity is fixed by the owner at
 * inclusion; no pointers escape, no callback or allocation in these helpers.
 * Prepare only from the actual admitted immutable definitions. */
typedef struct {
 uint64_t epoch,receipt;
 unsigned count,frontier;
 int ready;
 int first[C1AGO14_PREPARED_CAPACITY],next[C1AGO14_PREPARED_CAPACITY];
 unsigned order[C1AGO14_PREPARED_CAPACITY];
} c1ago14Prepared_t;
static void C1AGO14_Retire(c1ago14Prepared_t *p){
 p->ready=0;p->epoch=p->receipt=0;p->count=0;
}
static int C1AGO14_Prepare(c1ago14Prepared_t *p,const c1agt14Definition_t *d,
 unsigned count,uint64_t epoch,uint64_t receipt){
 C1AGO14_Retire(p);
 if(!epoch||!receipt||!count||count>C1AGO14_PREPARED_CAPACITY||
    !C1AGO14_Build(d,count,p->first,p->next,p->order,&p->frontier))return 0;
 p->epoch=epoch;p->receipt=receipt;p->count=count;p->ready=1;
 return 1;
}
/* Caller owns disjoint writable scratch; no output touched on a miss. */
static int C1AGO14_Copy(const c1ago14Prepared_t *p,unsigned count,
 uint64_t epoch,uint64_t receipt,int *first,int *next,unsigned *order,unsigned *frontier){
 if(!p->ready||!epoch||!receipt||!count||count>C1AGO14_PREPARED_CAPACITY||
    p->epoch!=epoch||p->receipt!=receipt||p->count!=count)return 0;
 memcpy(first,p->first,count*sizeof(*first));
 memcpy(next,p->next,count*sizeof(*next));
 memcpy(order,p->order,count*sizeof(*order));
 *frontier=p->frontier;
 return 1;
}
#endif
