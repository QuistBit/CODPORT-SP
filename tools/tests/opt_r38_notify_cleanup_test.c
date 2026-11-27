/* Differential test of the real sparse-name implementation. Minimal host
 * types reproduce the fields accessed by that implementation. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define C1GA_COUNT 2078u
typedef struct {int32_t cursor;uint16_t token,descendant;} TestFlag;
typedef struct {TestFlag flags[C1GA_COUNT];unsigned nameCount;uint64_t generation[C1GA_COUNT],pending[C1GA_COUNT],serial,ownerIdentity;int32_t pendingTick[C1GA_COUNT];} C1GANotifyState;
typedef struct {unsigned ordinal;char text[64];} C1GANameEntry;
typedef struct {C1GANotifyState *notify;C1GANameEntry *names;} C1GARecord;
static int R242ExactATRBytesEqual(const void*a,const void*b,size_t n){return memcmp(a,b,n)==0;}
static int ActorOverlap(const void*a,size_t n,const void*b,size_t m){uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;return n&&m&&(x<=y?y-x<n:x-y<m);}
#include "../../src/game/gsc_host/g_cod1_actor_notify_names.inc"
static int old_cleanup(C1GARecord*r){
 unsigned i;
 for(i=0;i<C1GA_COUNT;i++)if(!r->notify->flags[i].token){
  if(!ActorGenericNameSet(r,i,NULL,0))return 0;r->notify->generation[i]=r->notify->pending[i]=0;r->notify->pendingTick[i]=0;
 }
 return 1;
}
static uint32_t rng=0x9e3779b9u;
static uint32_t next(void){rng^=rng<<13;rng^=rng>>17;rng^=rng<<5;return rng;}
static C1GANotifyState a,b;
static C1GANameEntry x[C1GA_COUNT],y[C1GA_COUNT];
static void run(unsigned mode,unsigned density){
 C1GARecord old={&a,x},now={&b,y};unsigned i,c=0;int left,right;
 memset(&a,0,sizeof(a));memset(x,0x5a,sizeof(x));
 a.serial=next();a.ownerIdentity=next();
 for(i=0;i<C1GA_COUNT;i++){
  a.flags[i].cursor=(int32_t)(next()%100)-1;a.flags[i].descendant=(uint16_t)next();
  a.flags[i].token=(uint16_t)((next()%100<density)?i+1:0);
  a.generation[i]=next();a.pending[i]=next();a.pendingTick[i]=(int32_t)(next()%100000);
  if(mode==1||(mode!=0&&next()%100<density)){
   unsigned j;x[c].ordinal=i;for(j=0;j<64;j++)x[c].text[j]=(char)(next()%128);c++;
  }
 }
 a.nameCount=c;
 if(mode==3&&c>1)x[c/2].ordinal=x[c/2-1].ordinal;
 if(mode==4&&c>1){unsigned v=x[0].ordinal;x[0].ordinal=x[c-1].ordinal;x[c-1].ordinal=v;}
 if(mode==5&&c)x[c-1].ordinal=C1GA_COUNT+2;
 if(mode==6)a.nameCount=C1GA_COUNT+1;
 if(mode==7){old.names=NULL;now.names=NULL;}
 if(mode==8)for(i=0;i<C1GA_COUNT;i++)a.flags[i].token=(uint16_t)(i+1);
 memcpy(&b,&a,sizeof(a));memcpy(y,x,sizeof(x));
 left=old_cleanup(&old);right=ActorGenericClearUnnamed(&now);
 assert(left==right);assert(!memcmp(&a,&b,sizeof(a)));assert(!memcmp(x,y,sizeof(x)));
}
int main(void){unsigned i,j;for(i=0;i<9;i++)for(j=0;j<=100;j+=5)run(i,j);
 for(i=0;i<300;i++)run(2,next()%101);
 puts("R38 notify cleanup: 489 differential cases PASS (full bytes and status)");return 0;
}
