#ifndef COD1_ACTOR_GENERIC_FREQUENCY14_H
#define COD1_ACTOR_GENERIC_FREQUENCY14_H
#include "cod1_actor_generic_tree14.h"
/* ENGINE47A370, generalized from the retained 3x7 native oracle. Parent
 * accumulation order is the native definition order, not source ordinal.
 * The immutable topology is constructed once; it carries no mutable clock. */
#define C1AGF14_MAX 4096u
#define C1AGF14_NONE UINT32_MAX
typedef struct {
 unsigned count;
 unsigned first[C1AGF14_MAX],next[C1AGF14_MAX],order[C1AGF14_MAX];
} c1agf14Topology_t;
static int C1AGF14_Build(const c1agt14Definition_t *defs,unsigned count,
 const unsigned *nativeOrder,c1agf14Topology_t *out){
 c1agf14Topology_t *v;unsigned i,j,p,depth,children[C1AGF14_MAX];int ok=0;
 if(!defs||!nativeOrder||!out||!count||count>C1AGF14_MAX||defs[0].parent)return 0;
 v=malloc(sizeof(*v));if(!v)return 0;memset(v,0xff,sizeof(*v));v->count=count;
 memset(children,0,count*sizeof(*children));
 for(i=0;i<count;i++){
  if(defs[i].parent>=count||(i&&defs[i].parent==i)||nativeOrder[i]>=count)goto done;
  for(j=0;j<i;j++)if(nativeOrder[j]==nativeOrder[i])goto done;
  v->order[i]=nativeOrder[i];if(i)children[defs[i].parent]++;
 }
 for(i=0;i<count;i++){
  if(children[i]!=defs[i].children)goto done;
  for(p=i,depth=0;p;p=defs[p].parent)if(++depth>64)goto done;
 }
 for(i=1;i<count;i++){
  unsigned *link=&v->first[defs[i].parent];
  while(*link!=C1AGF14_NONE&&v->order[*link]<v->order[i])link=&v->next[*link];
  v->next[i]=*link;*link=i;
 }
 *out=*v;ok=1;
done:free(v);return ok;
}
/* Caller owns this Build-produced topology and the immutable resource
 * frequencies. Every mutable weight/rate is read afresh. No missing leaf
 * frequency is invented: zero is reserved for a real zero-interval clip. */
static double C1AGF14_Recurse(const c1agf14Topology_t *t,const c1atl14Node_t *n,
 const float *frequency,unsigned root){
 unsigned child=t->first[root];float weights=0,product=0;
 if(child==C1AGF14_NONE)return frequency[root];
 for(;child!=C1AGF14_NONE;child=t->next[child])if(n[child].currentWeight>0){
  double f=C1AGF14_Recurse(t,n,frequency,child);
  if(f>0){
   weights=(float)((double)weights+n[child].currentWeight);
   product=(float)(f*n[child].rate*n[child].currentWeight+product);
  }
 }
 return weights>0?(double)product/weights:0;
}
static int C1AGF14_Frequency(const c1agf14Topology_t *t,const c1atl14Node_t *n,
 const float *frequency,unsigned root,double *out){
 double value;unsigned i;
 if(!t||!n||!frequency||!out||!t->count||t->count>C1AGF14_MAX||root>=t->count)return 0;
 for(i=0;i<t->count;i++){
  if(!C1ATL14_Finite(n[i].currentWeight)||n[i].currentWeight<0||n[i].currentWeight>1||
     !C1ATL14_Finite(n[i].rate)||n[i].rate<0||n[i].rate>100||
     !C1ATL14_Finite(frequency[i])||frequency[i]<0||frequency[i]>120)return 0;
 }
#if defined(_MSC_VER)&&defined(_M_IX86)
 {unsigned char saved[108];uint32_t mx,nearest=0x1f80;const unsigned short cw=0x027f;
 __asm fnsave saved
 __asm stmxcsr mx
 __asm fldcw cw
 __asm ldmxcsr nearest
 value=C1AGF14_Recurse(t,n,frequency,root);
 __asm ldmxcsr mx
 __asm frstor saved
 }
#else
 value=C1AGF14_Recurse(t,n,frequency,root);
#endif
 if(!isfinite(value)||value<0)return 0;*out=value;return 1;
}
#endif
