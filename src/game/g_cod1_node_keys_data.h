#ifndef C1NAV14_NODE_KEYS_DATA_H
#define C1NAV14_NODE_KEYS_DATA_H
#include <string.h>
#include "g_cod1_node_sort_data.h"
/* Fresh DATA records only; no native pool, string, map or runtime authority.
 * GAME 6FF50/6FE80: failures preserve the exact completed mutation prefix.
 * Fresh keys and authored IDs are preconditions, never reset to admit reuse. */
enum {C1NAV14_KEYS_OK=0,C1NAV14_KEYS_MULTIPLE=1,C1NAV14_KEYS_SELF=2,
 C1NAV14_KEYS_CYCLE=3,C1NAV14_KEYS_RECURSIVE_SELF=4,C1NAV14_KEYS_INPUT=5,
 C1NAV14_KEYS_REMAP_HOLD=6};
typedef struct {unsigned status,phase,index,other,token,activePrefix,invalidInPrefix;uint32_t invalidFlag;} C1NAV14_KeysResult;
static void C1NAV14_KeysStore(unsigned char*p,unsigned x){p[0]=(unsigned char)x;p[1]=(unsigned char)(x>>8);}
static unsigned C1NAV14_Key(const unsigned char*p,unsigned i,unsigned off){return C1NAV14_SortU16(p+(size_t)i*116+off);}
static void C1NAV14_KeyPut(unsigned char*p,unsigned i,unsigned off,unsigned v){C1NAV14_KeysStore(p+(size_t)i*116+off,v);}
static int C1NAV14_KeyFailure(C1NAV14_KeysResult*r,unsigned status,unsigned phase,unsigned i,unsigned other,unsigned token){r->status=status;r->phase=phase;r->index=i;r->other=other;r->token=token;return 0;}
/* Explicit DFS stack keeps the native ascending scan/store order without
 * relying on C recursion depth. Count<=2048 bounds both depth and WORD keys. */
static int C1NAV14_KeyPropagate(unsigned char*p,unsigned count,unsigned root,unsigned group,unsigned depth,C1NAV14_KeysResult*r){
 struct {unsigned index,next,depth;} stack[2048];unsigned used=1,i,j,d,target;
 stack[0].index=root;stack[0].next=0;stack[0].depth=depth;
 C1NAV14_KeyPut(p,root,0x50,group);C1NAV14_KeyPut(p,root,0x52,depth);
 while(used){
  i=stack[used-1].index;target=C1NAV14_Key(p,i,0x30);
  if(!target||stack[used-1].next==count){--used;continue;}
  j=stack[used-1].next++;
  if(C1NAV14_Key(p,j,0x2c)!=target)continue;
  if(i==j)return C1NAV14_KeyFailure(r,C1NAV14_KEYS_RECURSIVE_SELF,3,i,j,C1NAV14_Key(p,i,0x2c));
  if(C1NAV14_Key(p,j,0x2a)&2)continue;
  d=stack[used-1].depth;d=d?d+1:0;
  if(used>=count)return C1NAV14_KeyFailure(r,C1NAV14_KEYS_REMAP_HOLD,3,i,j,target);
  stack[used].index=j;stack[used].next=0;stack[used++].depth=d;
  C1NAV14_KeyPut(p,j,0x50,group);C1NAV14_KeyPut(p,j,0x52,d);
 }
 return 1;
}
static int C1NAV14_KeysOverlap(const void*a,size_t an,const void*b,size_t bn){
 uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
 return an&&bn&&(x<=y?y-x<an:x-y<bn);
}
static int C1NAV14_NodeKeys(unsigned char*p,size_t bytes,unsigned count,unsigned char*map,size_t mapBytes,uint32_t priorInvalidFlag,C1NAV14_KeysResult*r){
 unsigned i,j,cursor,steps,pre,group=1,prefix=0,invalid=0;unsigned char seen[2048]={0};size_t need=(size_t)count*116;
 /* Refusal does not modify records/map. Result is independent output. */
 if(!r)return 0;
 if(count>2048||bytes<need||mapBytes<(size_t)count*2||
    (need&&(!p||!map||(uintptr_t)p>UINTPTR_MAX-need||(uintptr_t)map>UINTPTR_MAX-(size_t)count*2))||
    C1NAV14_KeysOverlap(p,need,map,(size_t)count*2)||
    C1NAV14_KeysOverlap(p,need,r,sizeof(*r))||C1NAV14_KeysOverlap(map,(size_t)count*2,r,sizeof(*r)))return 0;
 memset(r,0,sizeof(*r));r->invalidFlag=priorInvalidFlag;
 for(i=0;i<count;i++){
  j=C1NAV14_Key(p,i,0x28);
  if(j>=count||seen[j]||C1NAV14_Key(p,i,0x50)||C1NAV14_Key(p,i,0x52)||C1NAV14_Key(p,i,0x54))return C1NAV14_KeyFailure(r,C1NAV14_KEYS_INPUT,0,i,j,0);
  seen[j]=1;
 }
 for(i=0;i<count;i++){
  C1NAV14_KeyPut(p,i,0x54,65535);
  if(!C1NAV14_Key(p,i,0x2c)||(C1NAV14_Key(p,i,0x2a)&2))continue;
  for(j=0;j<count;j++){
   if(C1NAV14_Key(p,j,0x30)!=C1NAV14_Key(p,i,0x2c))continue;
   if(C1NAV14_SortS16(C1NAV14_Key(p,i,0x54))>=0)return C1NAV14_KeyFailure(r,C1NAV14_KEYS_MULTIPLE,1,i,j,C1NAV14_Key(p,i,0x2c));
   if(i==j)return C1NAV14_KeyFailure(r,C1NAV14_KEYS_SELF,1,i,j,C1NAV14_Key(p,i,0x2c));
   C1NAV14_KeyPut(p,i,0x54,j);C1NAV14_KeyPut(p,j,0x50,65535);
  }
 }
 for(i=0;i<count;i++){
  cursor=i;steps=0;
  while(C1NAV14_SortS16(C1NAV14_Key(p,cursor,0x54))>=0&&steps<count){
   pre=C1NAV14_Key(p,cursor,0x54);
   if(pre==i)return C1NAV14_KeyFailure(r,C1NAV14_KEYS_CYCLE,2,cursor,i,C1NAV14_Key(p,cursor,0x2c));
   cursor=pre;steps++;
  }
 }
 for(i=0;i<count;i++){
  if(C1NAV14_SortS16(C1NAV14_Key(p,i,0x50))>=0||C1NAV14_SortS16(C1NAV14_Key(p,i,0x54))>=0)continue;
  if(C1NAV14_Key(p,i,0x2a)&2){if(!C1NAV14_KeyPropagate(p,count,i,0,0,r))return 0;}
  else {if(!C1NAV14_KeyPropagate(p,count,i,group,1,r))return 0;group++;}
 }
 if(!C1NAV14_NodeSort(p,bytes,count))return C1NAV14_KeyFailure(r,C1NAV14_KEYS_REMAP_HOLD,4,0,0,0);
 for(i=0;i<count;i++)C1NAV14_KeysStore(map+(size_t)C1NAV14_Key(p,i,0x28)*2,i);
 for(i=0;i<count&&C1NAV14_Key(p,i,0x50)!=0;i++){
  if(C1NAV14_SortS16(C1NAV14_Key(p,i,0x54))>=0){
   j=i;
   while(j&&C1NAV14_Key(p,j-1,0x30)!=C1NAV14_Key(p,i,0x2c))--j;
   /* Native scans without a lower bound; a generated valid key chain must
    * supply this predecessor. Refuse as HOLD instead of reading before pool. */
   if(!j)return C1NAV14_KeyFailure(r,C1NAV14_KEYS_REMAP_HOLD,5,i,0,C1NAV14_Key(p,i,0x2c));
   C1NAV14_KeyPut(p,i,0x54,j-1);
  }
  if(!C1NAV14_SortU32(p+(size_t)i*116+0x24))invalid++;
  prefix++;
 }
 r->activePrefix=prefix;r->invalidInPrefix=invalid;r->invalidFlag=invalid?1:priorInvalidFlag;
 return 1;
}
#endif
