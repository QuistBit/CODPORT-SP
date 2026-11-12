#ifndef COD1_ACTOR_FLAGGED_LEAF14_H
#define COD1_ACTOR_FLAGGED_LEAF14_H
#include "cod1_actor_ancestor_knob14.h"
/* Bounded preallocated server state for implicit-end-only leaves. Tokens are
 * owner-neutral identifiers. No interned string ownership or GSC delivery is
 * provided here. Sync-branch notify descendant selection remains excluded. */
typedef struct {uint16_t descendant;int16_t cursor;uint16_t token,clientType;} c1afl14State_t;
static void C1AFL14_Cursor(const c1agt14Definition_t*d,const c1atl14Node_t*n,c1afl14State_t*f,unsigned i){
 if(f[i].token&&!d[i].children)f[i].cursor=n[i].phase==1?-1:0;
}
static int C1AFL14_Inner(const c1agt14Definition_t*d,c1atl14Node_t*n,c1afl14State_t*f,unsigned count,unsigned target,unsigned root,unsigned token,float weight,float time,float rate,int force){
 c1atl14Node_t*stage=NULL;c1afl14State_t*flags=NULL;unsigned p,i,sync,walk;int ok=0;
 if(!C1ATL14_Finite(weight)||weight<=0||!f||!token||token>65535||!target||target>=count||root>=count||target==root||!C1AGT14_Valid(d,n,count)||d[target].children)return 0;
 {uintptr_t a=(uintptr_t)f,b=(uintptr_t)n,c=(uintptr_t)d;size_t fa=count*sizeof(*f),nb=count*sizeof(*n),dc=count*sizeof(*d);if((a<=b?b-a<fa:a-b<nb)||(a<=c?c-a<fa:a-c<dc))return 0;}
 for(i=0;i<count;i++)if(f[i].descendant||f[i].cursor<-1||(f[i].cursor>0 && (!f[i].token || f[i].cursor>255 || d[i].children || C1AGT14_Descendant(d,i,root))))return 0;
 p=d[target].parent;while(p!=root){if(!p)return 0;p=d[p].parent;}
 stage=(c1atl14Node_t*)malloc(count*sizeof(*stage));flags=(c1afl14State_t*)malloc(count*sizeof(*flags));if(!stage||!flags)goto done;
 memcpy(stage,n,count*sizeof(*stage));memcpy(flags,f,count*sizeof(*flags));
 for(p=target;;p=d[p].parent){
  for(i=1;i<count;i++)if(i!=p&&d[i].parent==d[p].parent){flags[i].token=0;flags[i].cursor=-1;}
  flags[p].token=(uint16_t)(p==target?token:0);flags[p].cursor=-1;flags[p].descendant=0;
  if(!C1AKA14_Select(d,stage,count,p,d[p].parent,p==target?weight:1,time,p==target?rate:1,force))goto done;
  sync=p;while(sync&&(!d[sync].children||!(d[sync].flags&4)))sync=d[sync].parent;
  if(sync){
   for(i=0;i<count;i++)if(i!=p&&C1AGT14_Descendant(d,i,p))C1AFL14_Cursor(d,stage,flags,i);
   for(walk=p;walk!=sync;walk=d[walk].parent)C1AFL14_Cursor(d,stage,flags,walk);
  }
  C1AFL14_Cursor(d,stage,flags,p);
  if(d[p].parent==root)break;
 }
 /* Other named branches may already have delivered authored markers. */
 for(i=0;i<count;i++)if(f[i].cursor>0 && (memcmp(flags+i,f+i,sizeof(*f)) || memcmp(stage+i,n+i,sizeof(*n))))goto done;
 memcpy(n,stage,count*sizeof(*n));memcpy(f,flags,count*sizeof(*f));ok=1;
 done:free(stage);free(flags);return ok;
}
static int C1AFL14_Select(const c1agt14Definition_t*d,c1atl14Node_t*n,c1afl14State_t*f,unsigned count,unsigned target,unsigned root,unsigned token,float weight,float time,float rate,int force){
 int ok;
#if defined(_MSC_VER) && defined(_M_IX86)
 unsigned char saved[108];uint32_t mx,nearest=0x1f80;const unsigned short cw=0x027f;
 __asm fnsave saved
 __asm stmxcsr mx
 __asm fldcw cw
 __asm ldmxcsr nearest
 ok=C1AFL14_Inner(d,n,f,count,target,root,token,weight,time,rate,force);
 __asm ldmxcsr mx
 __asm frstor saved
#else
 ok=C1AFL14_Inner(d,n,f,count,target,root,token,weight,time,rate,force);
#endif
 return ok;
}
#endif
