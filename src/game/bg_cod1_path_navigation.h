#ifndef BG_COD1_PATH_NAVIGATION_H
#define BG_COD1_PATH_NAVIGATION_H
#include "bg_cod1_path_build.h"
#include "bg_cod1_node_reachable.h"
typedef struct {const unsigned char*records;unsigned count;const C1NTree*tree;const C1PSNode*nodes;const C1PSLink*links;unsigned linkCount;} C1PNGraph;
static int C1PNSeparate(const void*out,size_t size,const C1PNGraph*g,const unsigned char*input,const float*origin,const float*goal){
 if(!out||!g||!g->records||!g->nodes||!g->tree||!g->count||g->count>2048||g->tree->count!=g->count||g->tree->used>4095||g->linkCount>g->count*32767u||(g->linkCount&&!g->links))return 0;
 return !C1PSOverlap(out,size,g,sizeof(*g))&&!C1PSOverlap(out,size,g->records,g->count*116)&&!C1PSOverlap(out,size,g->nodes,g->count*sizeof(*g->nodes))&&
  !(g->linkCount&&C1PSOverlap(out,size,g->links,g->linkCount*sizeof(*g->links)))&&!C1PSOverlap(out,size,g->tree,sizeof(*g->tree))&&
  !(g->tree->nodes&&C1PSOverlap(out,size,g->tree->nodes,g->tree->used*sizeof(*g->tree->nodes)))&&!(g->tree->order&&C1PSOverlap(out,size,g->tree->order,g->count*sizeof(*g->tree->order)))&&
  (!input||!C1PSOverlap(out,size,input,C1PF_BYTES))&&(!origin||!C1PSOverlap(out,size,origin,12))&&(!goal||!C1PSOverlap(out,size,goal,12));
}
/* Native5FE80: goal nearest first, origin nearest second, then ordinary636E0
 * and5FA70. Caller owns a frozen physical graph and live spatial transaction. */
static int C1PNRouteInner(unsigned char*out,int*nativeResult,const unsigned char*input,const C1PNGraph*g,const float*origin,const float*goal,int team,int allowTraverse,int now,C1NRTrace sight,c1mmp14Query_t trace,C1PRContact contact,void*ctx){
 C1NRResult start,end;C1PSResult search;C1PSScratch*scratch=0;int*parents=0;unsigned i;int ok=0,value=0;unsigned char p[C1PF_BYTES];
 if(!out||!nativeResult||!input||!g||!g->records||!g->nodes||!g->tree||g->count!=g->tree->count||!g->count||g->count>2048||!sight||!trace||!C1MMP14_Vec(origin)||!C1MMP14_Vec(goal)||team<0||team>3||(allowTraverse!=0&&allowTraverse!=1)||
    (out!=input&&C1PSOverlap(out,C1PF_BYTES,input,C1PF_BYTES))||C1PSOverlap(out,C1PF_BYTES,nativeResult,sizeof(int)))return 0;
 for(i=0;i<g->count;i++)if(g->nodes[i].active!=C1NQU16(g->records+i*116+0x20)||memcmp(g->nodes[i].origin,g->records+i*116+0x38,12))return 0;
 if(!C1PNSeparate(out,C1PF_BYTES,g,out==input?0:input,origin,goal)||!C1PNSeparate(nativeResult,sizeof(*nativeResult),g,input,origin,goal))return 0;
 memcpy(p,input,sizeof(p));
 if(!C1NRNearest(g->records,g->count*116,g->tree,goal,192,0xfffffffeu,64,0,0,0,sight,ctx,&end))goto done;
 if(end.node<0){ok=1;goto done;}
 if(!C1NRNearest(g->records,g->count*116,g->tree,origin,192,0xfffffffeu,64,0,0,0,sight,ctx,&start))goto done;
 if(start.node<0){ok=1;goto done;}
 scratch=calloc(g->count,sizeof(*scratch));parents=malloc(g->count*sizeof(*parents));if(!scratch||!parents)goto done;
 if(!C1PSSearch(g->nodes,g->count,g->links,g->linkCount,start.node,end.node,team,goal,scratch,&search))goto done;
 if(!search.found){ok=1;goto done;}
 for(i=0;i<g->count;i++)parents[i]=scratch[i].parent;
 if(!C1PBBuild(p,&value,p,g->records,g->count,parents,end.node,start.node,team,origin,goal,1,allowTraverse,now,trace,contact,ctx))goto done;
 ok=1;
done:free(scratch);free(parents);if(ok){memcpy(out,p,sizeof(p));*nativeResult=value;}return ok;
}
static int C1PNRoute(unsigned char*out,int*nativeResult,const unsigned char*input,const C1PNGraph*g,const float*origin,const float*goal,int team,int allowTraverse,int now,C1NRTrace sight,c1mmp14Query_t trace,C1PRContact contact,void*ctx){
 int ok;
#if defined(_MSC_VER)&&defined(_M_IX86)
 {unsigned char saved[108];uint32_t mx,nearMx=0x1f80;const unsigned short cw=0x027f;
 __asm fnsave saved
 __asm stmxcsr mx
 __asm fldcw cw
 __asm ldmxcsr nearMx
 ok=C1PNRouteInner(out,nativeResult,input,g,origin,goal,team,allowTraverse,now,sight,trace,contact,ctx);
 __asm ldmxcsr mx
 __asm frstor saved
 }
#else
 ok=C1PNRouteInner(out,nativeResult,input,g,origin,goal,team,allowTraverse,now,sight,trace,contact,ctx);
#endif
 return ok;
}
/* Complete629C0 including the real native5FE80 repath boundary. A completed
 * repath search may return native false and retain the path, as in GAME. */
static int C1PNUpdateInner(unsigned char*out,const unsigned char*input,const C1PNGraph*g,const float*origin,int skipTrace,int retireBudget,int passedIsActor,int now,C1NRTrace sight,c1mmp14Query_t trace,C1PRContact contact,void*ctx){
 unsigned char p[C1PF_BYTES];float goal[3];int status,nativeResult;
 if(!out||!input||!C1PNSeparate(out,C1PF_BYTES,g,out==input?0:input,origin,0))return 0;
 memcpy(p,input,sizeof(p));status=C1PFUpdate(p,p,origin,skipTrace,retireBudget,passedIsActor,trace,contact,ctx);
 if(!status)return 0;
 if(status==2){
  unsigned flags=C1PFWord(p,0x3b8)&~0x20u;C1PFSetWord(p,0x3b8,flags);memcpy(goal,p+0x38c,12);
  if(!C1PNRoute(p,&nativeResult,p,g,origin,goal,C1PFWord(p,0x3c0),(flags>>4)&1,now,sight,trace,contact,ctx))return 0;
 }
 memcpy(out,p,sizeof(p));return 1;
}
static int C1PNUpdate(unsigned char*out,const unsigned char*input,const C1PNGraph*g,const float*origin,int skipTrace,int retireBudget,int passedIsActor,int now,C1NRTrace sight,c1mmp14Query_t trace,C1PRContact contact,void*ctx){
 int ok;
#if defined(_MSC_VER)&&defined(_M_IX86)
 {unsigned char saved[108];uint32_t mx,nearMx=0x1f80;const unsigned short cw=0x027f;
 __asm fnsave saved
 __asm stmxcsr mx
 __asm fldcw cw
 __asm ldmxcsr nearMx
 ok=C1PNUpdateInner(out,input,g,origin,skipTrace,retireBudget,passedIsActor,now,sight,trace,contact,ctx);
 __asm ldmxcsr mx
 __asm frstor saved
 }
#else
 ok=C1PNUpdateInner(out,input,g,origin,skipTrace,retireBudget,passedIsActor,now,sight,trace,contact,ctx);
#endif
 return ok;
}
#endif
