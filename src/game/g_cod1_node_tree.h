#ifndef G_COD1_NODE_TREE_H
#define G_COD1_NODE_TREE_H
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
/* Original6EBB0, pointer-vector producer6F270. Private DATA indices replace
 * native pointers; leaf order and equal-midpoint balancing are preserved.
 * Finite bounded map coordinates only. No publication or query authority. */
typedef struct {int32_t axis;float split;uint32_t a,b;} C1NTreeNode;
typedef struct {unsigned count,used;uint32_t*order;C1NTreeNode*nodes;} C1NTree;
static void C1NTreeFree(C1NTree*t){if(!t)return;free(t->order);free(t->nodes);free(t);}
static float C1NTreeCoord(const unsigned char*records,unsigned i,unsigned axis){float v;memcpy(&v,records+(size_t)i*116+0x38+axis*4,4);return v;}
static int C1NTreePart(C1NTree*t,const unsigned char*records,unsigned first,unsigned count,unsigned depth,uint32_t*out){
 unsigned id=t->used++,i,axis,l,r;float lo[2],hi[2];volatile float dx,dy,sum,mid;C1NTreeNode*n;
 if(id>=t->count*2-1||depth>64)return 0;*out=id;n=&t->nodes[id];
 if(count<4)goto leaf;
 for(axis=0;axis<2;axis++){
  lo[axis]=hi[axis]=C1NTreeCoord(records,t->order[first],axis);
  for(i=1;i<count;i++){float x=C1NTreeCoord(records,t->order[first+i],axis);if(lo[axis]>x)lo[axis]=x;else if(hi[axis]<x)hi[axis]=x;}
 }
 dx=hi[0]-lo[0];dy=hi[1]-lo[1];axis=dx<dy?1:0;
 if((axis?dy:dx)<=192.0f)goto leaf;
 sum=hi[axis]+lo[axis];mid=sum*0.5f;n->axis=(int32_t)axis;n->split=mid;l=0;r=count-1;
 for(;;){
  while(l<count&&C1NTreeCoord(records,t->order[first+l],axis)<mid)l++;
  while(r>0&&C1NTreeCoord(records,t->order[first+r],axis)>mid)r--;
  if(l>=r)break;
  {uint32_t tmp=t->order[first+l];t->order[first+l]=t->order[first+r];t->order[first+r]=tmp;}l++;r--;
 }
 while(2*l<count&&C1NTreeCoord(records,t->order[first+l],axis)==mid)l++;
 while(2*l>count&&C1NTreeCoord(records,t->order[first+l-1],axis)==mid)l--;
 if(!l||l==count)return 0;
 if(!C1NTreePart(t,records,first,l,depth+1,&n->a)||!C1NTreePart(t,records,first+l,count-l,depth+1,&n->b))return 0;
 return 1;
leaf:n->axis=-1;n->a=first;n->b=count;return 1;
}
static C1NTree*C1NTreeBuild(const unsigned char*records,size_t bytes,unsigned count){
 C1NTree*t;unsigned i,axis;uint32_t root;
 if(!records||!count||count>2048||bytes<(size_t)count*116)return NULL;
 for(i=0;i<count;i++)for(axis=0;axis<2;axis++){float v=C1NTreeCoord(records,i,axis);if(!isfinite(v)||v < -10000000.0f||v > 10000000.0f)return NULL;}
 t=(C1NTree*)calloc(1,sizeof(*t));if(!t)return NULL;t->count=count;
 t->order=(uint32_t*)malloc((size_t)count*sizeof(*t->order));t->nodes=(C1NTreeNode*)calloc((size_t)count*2-1,sizeof(*t->nodes));
 if(!t->order||!t->nodes)goto fail;
 for(i=0;i<count;i++)t->order[i]=i;
 if(!C1NTreePart(t,records,0,count,0,&root)||root)goto fail;return t;
fail:C1NTreeFree(t);return NULL;
}
#endif
