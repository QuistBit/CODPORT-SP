#ifndef C1NAV14_V7_DATA_H
#define C1NAV14_V7_DATA_H
#include <stddef.h>
#include <stdint.h>
#include <string.h>
/* Bounded DATA decoder for the bytes read by GAME 2006F7A0 from BSP lump31.
 * This does not create a native node pool, resolve physical node identities,
 * compute native link flags, build a tree or grant any native/Actor operation.
 * The caller owns immutable input and separate output spans for the call. */
typedef struct { uint32_t first,count; } C1NAV14_V7NodeData;
typedef struct { uint16_t target; uint32_t distanceBits; } C1NAV14_V7LinkData;
typedef struct {
 int valid;uint32_t nodes,links;size_t visibilityOffset,visibilityBytes,consumed;
} C1NAV14_V7Layout;
static uint16_t C1NAV14_V7U16(const unsigned char*p){return (uint16_t)((uint16_t)p[0]|((uint16_t)p[1]<<8));}
static uint32_t C1NAV14_V7U32(const unsigned char*p){return (uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);}
static C1NAV14_V7Layout C1NAV14_V7Inspect(const unsigned char*p,size_t n,uint32_t expectedNodes){
 C1NAV14_V7Layout z={0},r={0};size_t at=6,out=0,take;uint32_t i,j,count;unsigned u;
 if(!p||n<6||C1NAV14_V7U32(p)!=7||!expectedNodes||expectedNodes>2048||C1NAV14_V7U16(p+4)!=expectedNodes)return z;
 r.nodes=expectedNodes;r.visibilityBytes=((size_t)expectedNodes*(expectedNodes-1)+7)/8;
 for(i=0;i<expectedNodes;i++){
  if(n-at<2)return z;count=C1NAV14_V7U16(p+at);at+=2;
  /* Negative signed counts cannot denote a bounded serialized record set. */
  if(count>32767||count>(n-at)/6||UINT32_MAX-r.links<count)return z;
  for(j=0;j<count;j++,at+=6)if(C1NAV14_V7U16(p+at)>=expectedNodes)return z;
  r.links+=count;
 }
 r.visibilityOffset=at;
 while(out<r.visibilityBytes){
  if(at==n)return z;u=p[at++];take=u>=128?255-u:(size_t)u+1;
  /* FF means zero literal count: original do-loop has no bounded meaning. */
  if(!take||take>r.visibilityBytes-out)return z;
  if(u>=128){if(take>n-at)return z;at+=take;}
  else {if(at==n)return z;at++;}
  out+=take;
 }
 r.consumed=at;r.valid=1;return r;
}
static int C1NAV14_V7Overlap(const void*a,size_t an,const void*b,size_t bn){
 uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
 if(!an||!bn)return 0;
 if(!a||!b||an>UINTPTR_MAX-x||bn>UINTPTR_MAX-y)return 1;
 return x<y+bn&&y<x+an;
}
static int C1NAV14_V7Decode(const unsigned char*p,size_t n,uint32_t expectedNodes,
 C1NAV14_V7NodeData*nodes,size_t nodeCapacity,C1NAV14_V7LinkData*links,size_t linkCapacity,
 unsigned char*visibility,size_t visibilityCapacity){
 C1NAV14_V7Layout r=C1NAV14_V7Inspect(p,n,expectedNodes);size_t at=6,out=0,nb,lb,take;uint32_t i,j,k=0,count;unsigned u;
 if(!r.valid||!nodes||nodeCapacity<r.nodes||linkCapacity<r.links||visibilityCapacity<r.visibilityBytes||
    (r.links&&!links)||(r.visibilityBytes&&!visibility))return 0;
 nb=(size_t)r.nodes*sizeof(*nodes);lb=(size_t)r.links*sizeof(*links);
 if(C1NAV14_V7Overlap(p,n,nodes,nb)||C1NAV14_V7Overlap(p,n,links,lb)||C1NAV14_V7Overlap(p,n,visibility,r.visibilityBytes)||
    C1NAV14_V7Overlap(nodes,nb,links,lb)||C1NAV14_V7Overlap(nodes,nb,visibility,r.visibilityBytes)||C1NAV14_V7Overlap(links,lb,visibility,r.visibilityBytes))return 0;
 /* All malformed-input/capacity refusals precede the first output store. */
 for(i=0;i<r.nodes;i++){
  count=C1NAV14_V7U16(p+at);at+=2;nodes[i].first=k;nodes[i].count=count;
  for(j=0;j<count;j++,k++,at+=6){links[k].target=C1NAV14_V7U16(p+at);links[k].distanceBits=C1NAV14_V7U32(p+at+2);}
 }
 while(out<r.visibilityBytes){
  u=p[at++];
  if(u>=128){take=255-u;memcpy(visibility+out,p+at,take);at+=take;out+=take;}
  else {memset(visibility+out,0,u);out+=u;visibility[out++]=p[at++];}
 }
 return 1;
}
#endif
