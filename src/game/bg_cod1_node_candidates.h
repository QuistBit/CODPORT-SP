#ifndef BG_COD1_NODE_CANDIDATES_H
#define BG_COD1_NODE_CANDIDATES_H
#include "g_cod1_node_tree.h"
#include "bg_cod1_path_search.h"
/* Native6F0B0/6EF70. Query the already-qualified physical XY tree in its
 * native traversal/leaf order. A candidate is not a reachable node:71240
 * subsequently sorts and traces. No path or graph authority is created. */
typedef struct {unsigned node;float distanceSquared;} C1NQCandidate;
typedef struct {
 const unsigned char*records;const C1NTree*tree;const float*origin;
 float radius,radiusSquared,heightSquared;unsigned mask,capacity,count;
 C1NQCandidate*output;
} C1NQWalk;
static unsigned C1NQU32(const unsigned char*p){unsigned v;memcpy(&v,p,4);return v;}
static unsigned C1NQU16(const unsigned char*p){uint16_t v;memcpy(&v,p,2);return v;}
static float C1NQFloat(const unsigned char*p){float v;memcpy(&v,p,4);return v;}
static void C1NQVisit(C1NQWalk*q,unsigned id){
 const C1NTreeNode*n=q->tree->nodes+id;unsigned i;
 if(n->axis>=0){
  double delta=(double)q->origin[n->axis]-n->split;float saved=(float)delta;
  if(delta<=q->radius)C1NQVisit(q,n->a);
  if(-(double)q->radius<=saved)C1NQVisit(q,n->b);
  return;
 }
 for(i=0;i<n->b;i++){
  unsigned index=q->tree->order[n->a+i];const unsigned char*r=q->records+116*index;
  double dx=(double)C1NQFloat(r+0x38)-q->origin[0],dy=(double)C1NQFloat(r+0x3c)-q->origin[1],dz;
  float distance=(float)(dy*dy+dx*dx);
  if(distance>q->radiusSquared||!(q->mask&(1u<<C1NQU32(r+0x24))))continue;
  if((C1NQU16(r+0x2a)&1)&&!(q->mask&1))continue;
  dz=(double)C1NQFloat(r+0x40)-q->origin[2];if(dz*dz>q->heightSquared)continue;
  if(q->count==q->capacity)return;
  q->output[q->count].node=index;q->output[q->count].distanceSquared=distance;++q->count;
 }
}
static int C1NQQuery(const unsigned char*records,size_t bytes,const C1NTree*tree,
 const float origin[3],float radius,float height,unsigned mask,C1NQCandidate*out,unsigned capacity,unsigned*count){
 C1NQWalk q;C1NQCandidate*next;unsigned i,j;unsigned char seen[2048]={0};
 if(!records||!tree||!tree->count||tree->count>2048||!tree->used||tree->used>tree->count*2-1||
    !tree->nodes||!tree->order||!origin||!out||!count||capacity>2048||
    bytes<(size_t)tree->count*116||!isfinite(radius)||radius<0||radius>1000000||!isfinite(height)||height<0||height>1000000)return 0;
 if(C1PSOverlap(out,capacity*sizeof(*out),records,bytes)||C1PSOverlap(out,capacity*sizeof(*out),tree,sizeof(*tree))||
    C1PSOverlap(out,capacity*sizeof(*out),tree->nodes,tree->used*sizeof(*tree->nodes))||
    C1PSOverlap(out,capacity*sizeof(*out),tree->order,tree->count*sizeof(*tree->order))||
    C1PSOverlap(out,capacity*sizeof(*out),origin,12)||C1PSOverlap(out,capacity*sizeof(*out),count,sizeof(*count))||
    C1PSOverlap(count,sizeof(*count),records,bytes)||C1PSOverlap(count,sizeof(*count),tree,sizeof(*tree))||
    C1PSOverlap(count,sizeof(*count),tree->nodes,tree->used*sizeof(*tree->nodes))||
    C1PSOverlap(count,sizeof(*count),tree->order,tree->count*sizeof(*tree->order))||C1PSOverlap(count,sizeof(*count),origin,12))return 0;
 for(j=0;j<3;j++)if(!isfinite(origin[j])||fabsf(origin[j])>1000000)return 0;
 for(i=0;i<tree->count;i++){
  if(tree->order[i]>=tree->count||seen[tree->order[i]])return 0;seen[tree->order[i]]=1;
  if(C1NQU32(records+i*116+0x24)>31)return 0;
  for(j=0;j<3;j++){float v=C1NQFloat(records+i*116+0x38+j*4);if(!isfinite(v)||fabsf(v)>1000000)return 0;}
 }
 memset(seen,0,sizeof(seen));
 /* A tree has one incoming edge per non-root record and disjoint leaf ranges.
  * Forward child indices prohibit cycles before recursive traversal. */
 {unsigned char parents[4095]={0};parents[0]=1;
  for(i=0;i<tree->used;i++){const C1NTreeNode*n=tree->nodes+i;
   if(parents[i]!=1)return 0;
   if(n->axis>=0){if(n->axis>1||!isfinite(n->split)||n->a<=i||n->b<=i||n->a>=tree->used||n->b>=tree->used||++parents[n->a]!=1||++parents[n->b]!=1)return 0;}
   else{if(n->axis!=-1||!n->b||n->a>=tree->count||n->b>tree->count-n->a)return 0;for(j=0;j<n->b;j++)if(seen[n->a+j]++)return 0;}
  }
  for(i=0;i<tree->count;i++)if(seen[i]!=1)return 0;
 }
 next=calloc(capacity?capacity:1,sizeof(*next));if(!next)return 0;
 memset(&q,0,sizeof(q));q.records=records;q.tree=tree;q.origin=origin;q.radius=radius;q.radiusSquared=(float)((double)radius*radius);
 q.heightSquared=(float)((double)height*height);q.mask=mask;q.capacity=capacity;q.output=next;C1NQVisit(&q,0);
 memcpy(out,next,q.count*sizeof(*out));*count=q.count;free(next);return 1;
}
#endif
