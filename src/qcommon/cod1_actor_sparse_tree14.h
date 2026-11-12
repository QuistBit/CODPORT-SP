#ifndef COD1_ACTOR_SPARSE_TREE14_H
#define COD1_ACTOR_SPARSE_TREE14_H
#include "cod1_actor_bone14.h"
/* Exact ordinary 47BD40 current tree over a supplied sparse ancestor closure.
 * Owner must prove every active original leaf is represented. No additive
 * overlay, missing-track success, clock, goal-weight or lifetime authority. */
#define C1AST14_NODES 128
#define C1AST14_LEAVES 105
typedef struct {unsigned ordinal;int parent,leaf;float currentWeight,phase;} c1ast14Node_t;
typedef struct {unsigned bones,count;const c1ab14Clip_t*clip[C1AST14_LEAVES];unsigned char map[C1AST14_LEAVES][128];} c1ast14Bank_t;
static C1AB14_NOINLINE int C1AST14_Merge(const c1ab14Accumulator_t*src,c1ab14Accumulator_t*dst,unsigned n,float weight,int clear,int top){unsigned i,k;double norm,scale;if(clear)memset(dst,0,n*sizeof(*dst));for(i=0;i<n;i++){
 norm=(double)src[i].q[3]*src[i].q[3];norm+=(double)src[i].q[1]*src[i].q[1];norm+=(double)src[i].q[0]*src[i].q[0];norm+=(double)src[i].q[2]*src[i].q[2];
 if(top){if(src[i].weight>0){scale=1.0/src[i].weight;for(k=0;k<4;k++)dst[i].q[k]=(float)(scale*src[i].q[k]);for(k=0;k<3;k++)dst[i].t[k]=(float)(scale*src[i].t[k]);dst[i].weight=src[i].weight;}}
 else{if(norm>0){scale=(double)weight/sqrt(norm);for(k=0;k<4;k++)dst[i].q[k]=(float)(scale*src[i].q[k]+dst[i].q[k]);}if(src[i].weight>0){scale=(double)weight/src[i].weight;for(k=0;k<3;k++)dst[i].t[k]=(float)(scale*src[i].t[k]+dst[i].t[k]);dst[i].weight=(float)((double)weight+dst[i].weight);}}
 }return 1;}
static int C1AST14_Leaf(const c1ast14Bank_t*b,unsigned leaf,float phase,float weight,c1ab14Accumulator_t*out,int clear){c1ab14Accumulator_t mapped[128];const c1ab14Clip_t*c=b->clip[leaf];unsigned i;if(clear)memset(out,0,b->bones*sizeof(*out));for(i=0;i<c->bones;i++)mapped[i]=out[b->map[leaf][i]];if(!C1AB14_Accumulate(c,phase,weight,mapped,c->bones))return 0;for(i=0;i<c->bones;i++)out[b->map[leaf][i]]=mapped[i];return 1;}
static int C1AST14_Node(const c1ast14Bank_t*b,const c1ast14Node_t*nodes,unsigned n,unsigned index,float weight,c1ab14Accumulator_t*out,int clear,int top,unsigned depth){unsigned child[C1AST14_NODES],count=0,i,j;c1ab14Accumulator_t local[128];const c1ast14Node_t*node=nodes+index;
 if(depth>32)return 0;if(node->leaf>=0)return C1AST14_Leaf(b,(unsigned)node->leaf,node->phase,weight,out,clear);
 for(i=1;i<n;i++)if(nodes[i].parent==(int)index&&nodes[i].currentWeight>0){j=count;while(j&&nodes[child[j-1]].ordinal<nodes[i].ordinal){child[j]=child[j-1];j--;}child[j]=i;count++;}
 if(!count){if(clear)memset(out,0,b->bones*sizeof(*out));return 1;}
 if(count==1)return C1AST14_Node(b,nodes,n,child[0],weight,out,clear,top,depth+1);
 memset(local,0,b->bones*sizeof(*local));for(i=0;i<count;i++)if(!C1AST14_Node(b,nodes,n,child[i],nodes[child[i]].currentWeight,local,i==0,0,depth+1))return 0;
 return C1AST14_Merge(local,out,b->bones,weight,clear,top);
}
static C1AB14_NOINLINE int C1AST14_SampleInner(const c1ast14Bank_t*b,const c1ast14Node_t*nodes,unsigned n,c1ab14Accumulator_t*out,unsigned capacity){c1ab14Accumulator_t staged[128];unsigned i,j,k,depth;size_t bytes;
 if(!b||!nodes||!out||!n||n>C1AST14_NODES||!b->bones||b->bones>=128||capacity<b->bones||!b->count||b->count>C1AST14_LEAVES||nodes[0].ordinal||nodes[0].parent!=-1||nodes[0].leaf!=-1)return 0;bytes=b->bones*sizeof(*out);
 if(C1AB14_Overlap(b,sizeof(*b),out,bytes)||C1AB14_Overlap(nodes,n*sizeof(*nodes),out,bytes))return 0;
 for(i=0;i<n;i++){
  if(!(nodes[i].currentWeight>=0&&nodes[i].currentWeight<=1&&nodes[i].phase>=0&&nodes[i].phase<=1)||nodes[i].leaf< -1||nodes[i].leaf>=(int)b->count||(i&&(!nodes[i].ordinal||nodes[i].parent<0||nodes[i].parent>=(int)n)))return 0;
  for(j=0;j<i;j++)if(nodes[i].ordinal==nodes[j].ordinal)return 0;
  if(i){j=i;depth=0;while(j){if(++depth>32||nodes[j].parent<0||nodes[j].parent>=(int)n||nodes[nodes[j].parent].leaf!=-1)return 0;j=(unsigned)nodes[j].parent;}}
 }
 for(i=0;i<b->count;i++){const c1ab14Clip_t*c=b->clip[i];unsigned char seen[128]={0};if(!c||c->magic!=C1AB14_MAGIC||!c->bones||c->bones>=128||C1AB14_Overlap(c,sizeof(*c),out,bytes))return 0;for(k=0;k<c->bones;k++){const c1ab14Bone_t*t=c->bone+k;unsigned mapped=b->map[i][k];if(mapped>=b->bones||seen[mapped]||C1AB14_Overlap(t->qi,t->nq*2,out,bytes)||C1AB14_Overlap(t->q,t->nq*8,out,bytes)||C1AB14_Overlap(t->ti,t->nt*2,out,bytes)||C1AB14_Overlap(t->t,t->nt*12,out,bytes))return 0;seen[mapped]=1;}}
 if(!C1AST14_Node(b,nodes,n,0,1,staged,1,1,0))return 0;memcpy(out,staged,bytes);return 1;
}
static int C1AST14_Sample(const c1ast14Bank_t*b,const c1ast14Node_t*nodes,unsigned n,c1ab14Accumulator_t*out,unsigned capacity){int ok;
#if defined(_MSC_VER)&&defined(_M_IX86)
 unsigned char saved[108];uint32_t mx,nearMx=0x1f80u;const unsigned short cw=0x027f;
 __asm fnsave saved
 __asm stmxcsr mx
 __asm fldcw cw
 __asm ldmxcsr nearMx
 ok=C1AST14_SampleInner(b,nodes,n,out,capacity);
 __asm ldmxcsr mx
 __asm frstor saved
#else
 ok=C1AST14_SampleInner(b,nodes,n,out,capacity);
#endif
 return ok;}
#endif
