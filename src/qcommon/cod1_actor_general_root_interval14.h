#ifndef COD1_ACTOR_GENERAL_ROOT_INTERVAL14_H
#define COD1_ACTOR_GENERAL_ROOT_INTERVAL14_H
#include "cod1_actor_generic_tree14.h"
#include "cod1_actor_root_interval14.h"
/* Exact active ancestor closure for ENGINE112. Textual source siblings are
 * inserted at the native head; descending source order is native child order.
 * Omit only zero selected weights, retaining every visited empty branch and
 * singleton normalization boundary. No missing active resource is replaced. */
typedef struct {
 c1ari14Node_t nodes[128];const c1ar14Root_t*roots[82];
 unsigned source[128],leafSource[82],count,leaves;
} c1agr14Projection_t;
static int C1AGR14_Visit(const c1agt14Definition_t*d,const c1atl14Node_t*n,
 const c1ar14Root_t*const*roots,unsigned count,int goal,unsigned source,int parent,unsigned depth,c1agr14Projection_t*v){
 unsigned here,i;const c1atl14Node_t*live=n+source;c1ari14Node_t*p;
 if(depth>32||v->count>=128)return 0;here=v->count++;p=v->nodes+here;v->source[here]=source;
 p->ordinal=source;p->parent=parent;p->leaf=-1;p->currentWeight=live->currentWeight;p->goalWeight=live->goalWeight;p->oldPhase=live->oldPhase;p->phase=live->phase;
 if(!d[source].children){
  if(v->leaves>=82||!roots[source]||roots[source]->magic!=C1AR14_MAGIC)return 0;
  p->leaf=(int)v->leaves;v->leafSource[v->leaves]=source;v->roots[v->leaves++]=roots[source];return 1;
 }
 for(i=1;i<count;i++)if(d[i].parent==source&&(goal?n[i].goalWeight:n[i].currentWeight)>0&&
    !C1AGR14_Visit(d,n,roots,count,goal,i,(int)here,depth+1,v))return 0;
 return 1;
}
static int C1AGR14_Project(const c1agt14Definition_t*d,const c1atl14Node_t*n,
 const c1ar14Root_t*const*roots,unsigned count,int goal,c1agr14Projection_t*out){
 c1agr14Projection_t value;unsigned i;
 if(!out||!roots||!count||count>4096||(goal!=0&&goal!=1)||
    C1ARI14_Overlap(out,sizeof(*out),d,count*sizeof(*d))||C1ARI14_Overlap(out,sizeof(*out),n,count*sizeof(*n))||
    C1ARI14_Overlap(out,sizeof(*out),roots,count*sizeof(*roots))||!C1AGT14_Valid(d,n,count)||!d[0].children)return 0;
 for(i=0;i<count;i++)if(roots[i]&&C1ARI14_Overlap(out,sizeof(*out),roots[i],sizeof(*roots[i])))return 0;
 memset(&value,0,sizeof(value));
 if(!C1AGR14_Visit(d,n,roots,count,goal,0,-1,0,&value)||!value.leaves)return 0;
 *out=value;return 1;
}
#endif
