#ifndef BG_COD1_PATH_SEARCH_H
#define BG_COD1_PATH_SEARCH_H
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
/* GAME 636E0, ordinary graph search. Physical node order, active link order,
 * team blocking bytes and distances are inputs, never reconstructed here.
 * Separate scratch owns the native temporary node fields 5c..70. The caller
 * must certify graph/actor lifetime before and after using this DATA result.
 * -1 is NULL, -2 is the native stack sentinel. This does not build a path,
 * choose a nearest node, notify GSC or authorize collision-free movement. */
typedef struct { unsigned first,active; float origin[3]; } C1PSNode;
typedef struct { unsigned target; unsigned char blocked[4]; float distance; } C1PSLink;
typedef struct { int seen,next,previous,parent; float cost,heuristic; } C1PSScratch;
typedef struct { int found; unsigned expanded,relaxed,reopened; } C1PSResult;
static int C1PSOverlap(const void*a,size_t an,const void*b,size_t bn){
 uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;return x<=y?y-x<an:x-y<bn;
}
static int C1PSSearch(const C1PSNode*nodes,unsigned count,const C1PSLink*links,
 unsigned linkCount,unsigned start,unsigned goal,unsigned team,const float target[3],
 C1PSScratch*out,C1PSResult*result){
 C1PSScratch*s;C1PSResult r={0};int head,current,at,before,after;unsigned i,j;
 size_t ns=(size_t)count*sizeof(*nodes),ls=(size_t)linkCount*sizeof(*links),os=(size_t)count*sizeof(*out);
 if(!nodes||!count||count>2048||(linkCount&&!links)||!out||!result||!target||
    start>=count||goal>=count||team>3||linkCount>count*32767u)return 0;
 if(C1PSOverlap(out,os,nodes,ns)||(linkCount&&C1PSOverlap(out,os,links,ls))||
    C1PSOverlap(out,os,target,12)||C1PSOverlap(out,os,result,sizeof(*result))||
    C1PSOverlap(result,sizeof(*result),nodes,ns)||
    (linkCount&&C1PSOverlap(result,sizeof(*result),links,ls))||C1PSOverlap(result,sizeof(*result),target,12))return 0;
 for(i=0;i<3;i++)if(!isfinite(target[i])||fabsf(target[i])>1000000)return 0;
 for(i=0;i<count;i++){
  if(nodes[i].first>linkCount||nodes[i].active>32767||nodes[i].active>linkCount-nodes[i].first)return 0;
  for(j=0;j<3;j++)if(!isfinite(nodes[i].origin[j])||fabsf(nodes[i].origin[j])>1000000)return 0;
  for(j=0;j<nodes[i].active;j++){const C1PSLink*l=links+nodes[i].first+j;
   if(l->target>=count||!isfinite(l->distance)||l->distance<0||l->distance>1000000)return 0;
  }
 }
 s=(C1PSScratch*)calloc(count,sizeof(*s));if(!s)return 0;
 for(i=0;i<count;i++)s[i].next=s[i].previous=s[i].parent=-1;
 s[start].seen=1;s[start].previous=s[start].parent=-2;head=(int)start;
 while(head>=0){
  current=head;if((unsigned)current==goal){r.found=1;break;}
  head=s[current].next;if(head>=0)s[head].previous=-2;r.expanded++;
  for(j=0;j<nodes[current].active;j++){
   const C1PSLink*l=links+nodes[current].first+j;double cost;float priority;
   if(l->blocked[team])continue;at=(int)l->target;cost=(double)l->distance+s[current].cost;
   if(s[at].seen){
    if(!(cost<s[at].cost))continue;
    before=s[at].previous;after=s[at].next;
    if(before!=-1){if(before==-2)head=after;else s[before].next=after;if(after>=0)s[after].previous=before;}
    else r.reopened++;
   }else{
    double dx=(double)nodes[at].origin[0]-target[0],dy=(double)nodes[at].origin[1]-target[1];
    s[at].seen=1;s[at].heuristic=(float)sqrt(dy*dy+dx*dx);
   }
   s[at].cost=(float)cost;s[at].parent=current;priority=(float)(cost+s[at].heuristic);
   before=-2;after=head;
   while(after>=0&&(double)s[after].heuristic+s[after].cost<priority){before=after;after=s[after].next;}
   s[at].previous=before;s[at].next=after;if(before==-2)head=at;else s[before].next=at;
   if(after>=0)s[after].previous=at;r.relaxed++;
  }
  s[current].previous=-1;
 }
 memcpy(out,s,os);*result=r;free(s);return 1;
}
#endif
