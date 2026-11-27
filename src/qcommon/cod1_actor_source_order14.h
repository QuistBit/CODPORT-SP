#ifndef COD1_ACTOR_SOURCE_ORDER14_H
#define COD1_ACTOR_SOURCE_ORDER14_H
#include "cod1_actor_definition14.h"
/* Private, callback-free builder for the parsed parent-before-child source
 * catalog. Caller owns disjoint arrays of count entries and the frontier.
 * Failure may leave partial output, exactly as the old root scratch builder.
 * No claim of support for forward parents or a general mutable-tree proof. */
#define C1AGO14_MAX 4096u
static int C1AGO14_Visit(const c1agt14Definition_t *d,unsigned count,
 const int *first,const int *next,unsigned *order,unsigned *frontier,
 unsigned source,unsigned depth){
 unsigned start=*frontier,n=0;int child;
 if(depth>64||source>=count||d[source].children>count-*frontier)return 0;
 *frontier+=d[source].children;
 for(child=first[source];child>=0;child=next[child]){
  if(n>=d[source].children)return 0;
  order[child]=start+n++;
  if(!C1AGO14_Visit(d,count,first,next,order,frontier,(unsigned)child,depth+1))return 0;
 }
 return n==d[source].children;
}
static int C1AGO14_Build(const c1agt14Definition_t *d,unsigned count,
 int *first,int *next,unsigned *order,unsigned *frontier){
 unsigned i,parent;
 if(!d||!first||!next||!order||!frontier||!count||count>C1AGO14_MAX)return 0;
 for(i=0;i<count;i++){first[i]=next[i]=-1;order[i]=~0u;}
 if(d[0].parent)return 0;
 for(i=1;i<count;i++){
  parent=d[i].parent;
  if(parent>=i)return 0;
  next[i]=first[parent];first[parent]=(int)i;
 }
 order[0]=0;*frontier=1;
 if(!C1AGO14_Visit(d,count,first,next,order,frontier,0,0)||*frontier!=count)return 0;
 for(i=0;i<count;i++)if(order[i]>=count)return 0;
 return 1;
}
#endif
