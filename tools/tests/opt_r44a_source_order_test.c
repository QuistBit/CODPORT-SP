#include <assert.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "../../src/qcommon/cod1_actor_source_order14.h"

/* R40 ActorCurrentRootOrderVisit/Order, unchanged bodies. Fixture substitutes
 * only the unrelated host struct fields and fixed catalog count (testCount). */
static unsigned testCount;
#define C1GA_COUNT testCount
typedef struct {int first[C1AGO14_MAX],next[C1AGO14_MAX];unsigned order[C1AGO14_MAX],frontier;} C1ActorCurrentRoot;
static c1agt14Definition_t cod1GenericDefinitions[C1AGO14_MAX];
static int ActorCurrentRootOrderVisit(C1ActorCurrentRoot *v,unsigned source,unsigned depth) {
    unsigned first=v->frontier,n=0;int child;
    if(depth>64||source>=C1GA_COUNT||cod1GenericDefinitions[source].children>C1GA_COUNT-v->frontier)return 0;
    v->frontier+=cod1GenericDefinitions[source].children;
    for(child=v->first[source];child>=0;child=v->next[child]){
        if(n>=cod1GenericDefinitions[source].children)return 0;
        v->order[child]=first+n++;
        if(!ActorCurrentRootOrderVisit(v,(unsigned)child,depth+1))return 0;
    }
    return n==cod1GenericDefinitions[source].children;
}
static int ActorCurrentRootOrder(C1ActorCurrentRoot *v) {
    unsigned i,parent;
    for(i=0;i<C1GA_COUNT;i++){v->first[i]=v->next[i]=-1;v->order[i]=~0u;}
    if(cod1GenericDefinitions[0].parent)return 0;
    for(i=1;i<C1GA_COUNT;i++){
        parent=cod1GenericDefinitions[i].parent;
        /* Actual catalog parser publishes names in textual preorder. */
        if(parent>=i)return 0;
        v->next[i]=v->first[parent];v->first[parent]=(int)i;
    }
    v->order[0]=0;v->frontier=1;
    if(!ActorCurrentRootOrderVisit(v,0,0)||v->frontier!=C1GA_COUNT)return 0;
    for(i=0;i<C1GA_COUNT;i++)if(v->order[i]>=C1GA_COUNT)return 0;
    return 1;
}
#undef C1GA_COUNT

static C1ActorCurrentRoot old,newer;
static c1agt14Definition_t before[C1AGO14_MAX];
static unsigned passed,refused;
static uint32_t rng=0x8c33172du;
static unsigned random_u(void){rng^=rng<<13;rng^=rng>>17;rng^=rng<<5;return rng;}
static void compare(unsigned count,int expected){
 unsigned i,seen[C1AGO14_MAX]={0};int a,b,child;
 testCount=count;memcpy(before,cod1GenericDefinitions,sizeof(before));
 memset(&old,0xa5,sizeof(old));memset(&newer,0xa5,sizeof(newer));
 a=ActorCurrentRootOrder(&old);
 b=C1AGO14_Build(cod1GenericDefinitions,count,newer.first,newer.next,newer.order,&newer.frontier);
 assert(a==b);if(expected>=0)assert(a==expected);
 assert(!memcmp(&old,&newer,sizeof(old)));assert(!memcmp(before,cod1GenericDefinitions,sizeof(before)));
 if(!a){refused++;return;}passed++;
 assert(newer.frontier==count&&newer.order[0]==0);
 for(i=0;i<count;i++){
  unsigned n=0,last=count,prev=0;assert(newer.order[i]<count&&!seen[newer.order[i]]++);
  for(child=newer.first[i];child>=0;child=newer.next[child]){
   assert((unsigned)child<last&&cod1GenericDefinitions[child].parent==i);
   if(n)assert(newer.order[child]==prev+1);
   prev=newer.order[child];last=(unsigned)child;n++;
  }
  assert(n==cod1GenericDefinitions[i].children);
 }
}
static void make(unsigned count,unsigned shape){
 unsigned i,p;memset(cod1GenericDefinitions,0,sizeof(cod1GenericDefinitions));
 for(i=1;i<count;i++){
  p=shape==0?0:shape==1?i-1:shape==2?(i-1)/2:random_u()%i;
  cod1GenericDefinitions[i].parent=p;cod1GenericDefinitions[p].children++;
  cod1GenericDefinitions[i].flags=random_u()&15;
 }
}
int main(void){
 unsigned k,count,p;static const unsigned sizes[]={1,2,7,64,65,66,128,2078,4096};
 for(k=0;k<sizeof(sizes)/sizeof(sizes[0]);k++){
  count=sizes[k];make(count,0);compare(count,1);make(count,1);compare(count,count<=65);
  make(count,2);compare(count,1);
 }
 /* Native child blocks differ from global reverse-source numbering. */
 make(6,0);cod1GenericDefinitions[0].children=2;
 cod1GenericDefinitions[1].children=2;cod1GenericDefinitions[4].children=1;
 cod1GenericDefinitions[2].parent=1;cod1GenericDefinitions[3].parent=1;cod1GenericDefinitions[5].parent=4;
 compare(6,1);assert(newer.order[4]==1&&newer.order[1]==2&&newer.order[5]==3&&newer.order[3]==4&&newer.order[2]==5);
 for(k=0;k<400;k++){
  count=k%8==0?2078:k%8==1?4096:2+random_u()%4095;
  make(count,3);compare(count,1);
  p=1+random_u()%(count-1);cod1GenericDefinitions[p].parent=p;compare(count,0);
  make(count,3);cod1GenericDefinitions[random_u()%count].children++;compare(count,0);
  make(count,3);cod1GenericDefinitions[0].parent=1;compare(count,0);
  make(count,3);cod1GenericDefinitions[0].children=UINT_MAX;compare(count,0);
 }
 make(4,0);cod1GenericDefinitions[1].parent=3;compare(4,0);
 memset(&newer,0xa5,sizeof(newer));old=newer;
 assert(!C1AGO14_Build(cod1GenericDefinitions,0,newer.first,newer.next,newer.order,&newer.frontier));
 assert(!C1AGO14_Build(cod1GenericDefinitions,C1AGO14_MAX+1,newer.first,newer.next,newer.order,&newer.frontier));
 assert(!C1AGO14_Build(NULL,4,newer.first,newer.next,newer.order,&newer.frontier));
 assert(!C1AGO14_Build(cod1GenericDefinitions,4,NULL,newer.next,newer.order,&newer.frontier));
 assert(!C1AGO14_Build(cod1GenericDefinitions,4,newer.first,NULL,newer.order,&newer.frontier));
 assert(!C1AGO14_Build(cod1GenericDefinitions,4,newer.first,newer.next,NULL,&newer.frontier));
 assert(!C1AGO14_Build(cod1GenericDefinitions,4,newer.first,newer.next,newer.order,NULL));
 assert(!memcmp(&old,&newer,sizeof(old)));
 printf("R44A differential: %u successful, %u refused, 7 argument guards\n",passed,refused);
 return 0;
}
