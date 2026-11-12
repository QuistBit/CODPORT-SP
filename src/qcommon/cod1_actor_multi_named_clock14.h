#ifndef COD1_ACTOR_MULTI_NAMED_CLOCK14_H
#define COD1_ACTOR_MULTI_NAMED_CLOCK14_H
#include "cod1_actor_notetrack_metadata14.h"
/* First bounded ordinary unsynchronized two nonloop named-leaf closure.
 * Caller owns original native definition ordering, actual slot presence and
 * metadata authority. No clock prep, VM dispatch or scene admission here. */
#define C1AMN14_NODES 32
#define C1AMN14_EVENTS 4
typedef struct {unsigned nativeOrdinal;int parent,leaf,present;c1atl14Node_t clock;} c1amn14Node_t;
typedef struct {unsigned leaf;uint16_t payload;} c1amn14Event_t;
typedef struct {unsigned resume,count;float usedDt;c1amn14Event_t events[C1AMN14_EVENTS];} c1amn14Step_t;
typedef struct {const c1amn14Node_t*nodes;unsigned count;const c1afl14State_t*flags;const c1anm14Metadata_t*metadata[2];int owner,enabled,mixed;unsigned bodyAuthored;float dt;} c1amn14Call_t;
static int C1AMN14_Validate(const c1amn14Call_t*c){unsigned i,j,leaves=0;int parent;const c1amn14Node_t*n;
 if(!c->count||c->count>C1AMN14_NODES||(c->owner!=0&&c->owner!=1)||(c->enabled!=0&&c->enabled!=1)||!C1ATL14_Finite(c->dt)||c->dt<0||c->dt>60)return 0;
 for(i=0;i<2;i++){
  const c1anm14Metadata_t*m=c->metadata[i];
  if(!C1ANM14_Valid(m))return 0;
  /* The bank verifies the exact authored marks. A nonloop body may have only the implicit end. */
  if(c->bodyAuthored>=C1ANC14_MAX_NOTES)return 0;
  if((c->mixed&&i==1)||(i==0&&m->loop&&!c->bodyAuthored)){if(m->count!=1)return 0;}
  else if(m->loop||m->authoredCount!=(i==0?c->bodyAuthored:1u))return 0;
 }
 for(i=0;i<c->count;i++){
  n=c->nodes+i;parent=n->parent;if((i==0?parent!=-1:parent<0||parent>=(int)i)||(n->present!=0&&n->present!=1)||n->leaf<-1||n->leaf>1)return 0;
  for(j=0;j<i;j++)if(c->nodes[j].nativeOrdinal==n->nativeOrdinal)return 0;
  if(n->leaf>=0){c1atl14Node_t value;c1afl14State_t flags;c1anc14Step_t step;const c1anm14Metadata_t*m=c->metadata[n->leaf];
   if(leaves&(1u<<n->leaf))return 0;leaves|=1u<<n->leaf;
   for(j=i+1;j<c->count;j++)if(c->nodes[j].parent==(int)i)return 0;
   if(m->loop){
    if(!C1ANC14_LoopEndInner(&n->clock,c->flags+n->leaf,m->notes,m->frequency,c->dt,c->owner,0,&value,&flags,&step))return 0;
   }else if(!C1ANC14_Inner(&n->clock,c->flags+n->leaf,m->notes,m->count,m->frequency,0,c->dt,c->owner,0,&value,&flags,&step))return 0;
  }else{
   const c1atl14Node_t*v=&n->clock;
   unsigned child=0;for(j=i+1;j<c->count;j++)if(c->nodes[j].parent==(int)i)child=1;if(!child)return 0;
   if(!C1ATL14_Finite(v->currentWeight)||v->currentWeight<0||v->currentWeight>1||!C1ATL14_Finite(v->goalWeight)||v->goalWeight<0||v->goalWeight>1||v->rate!=1||v->phase||v->oldPhase||v->cycle||v->oldCycle)return 0;
  }
 }return leaves==3;
}
static double C1AMN14_Fraction(const c1amn14Call_t*c,unsigned index){const c1amn14Node_t*n=c->nodes+index;float best=1;unsigned i,next,last=0,have=0;double candidate;
 if(!n->present||n->clock.currentWeight==0||n->clock.goalWeight==0)return 1;
 if(n->leaf>=0){const c1atl14Node_t*v=&n->clock;const c1afl14State_t*f=c->flags+n->leaf;const c1anm14Metadata_t*m=c->metadata[n->leaf];double delta=(double)m->frequency*v->rate*c->dt,target;float roundedDelta=(float)delta,roundedTarget,note;
  if(!c->enabled||!c->owner||!f->token||v->phase==1||delta==0)return 1;
  target=(double)roundedDelta+v->oldPhase;
  if(m->loop){
   unsigned cycle=v->oldCycle;
   while(target>=1){target-=1;cycle=(cycle+1)&65535u;}
   if((double)v->phase-target<=(int)(int16_t)cycle-(int)(int16_t)v->cycle&&v->phase>(float)target)
    return ((1.0-v->oldPhase)+((int)(int16_t)v->cycle-(int)(int16_t)v->oldCycle))/roundedDelta;
   return 1;
  }
  if(target>=1)target=1;roundedTarget=(float)target;
  if(target<v->phase)return 1;note=m->notes[(unsigned)f->cursor].phase;
  if((roundedTarget>note||roundedTarget==1)&&v->phase<=note)return ((double)note-v->oldPhase)/roundedDelta;return 1;
 }
 for(;;){next=~0u;for(i=1;i<c->count;i++)if(c->nodes[i].parent==(int)index&&(!have||c->nodes[i].nativeOrdinal>last)&&(next==~0u||c->nodes[i].nativeOrdinal<c->nodes[next].nativeOrdinal))next=i;
  if(next==~0u)break;candidate=C1AMN14_Fraction(c,next);if(candidate<best)best=(float)candidate;last=c->nodes[next].nativeOrdinal;have=1;
 }return best;
}
static int C1AMN14_AdvanceNode(const c1amn14Call_t*c,unsigned index,float usedDt,int enabled,c1amn14Node_t*nodes,c1afl14State_t flags[2],c1amn14Step_t*out){
 const c1amn14Node_t*n=c->nodes+index;unsigned i,next,last=0,have=0;
 if(!n->present||n->clock.currentWeight==0)return 1;if(n->clock.goalWeight==0)enabled=0;
 if(n->leaf>=0){const c1anm14Metadata_t*m=c->metadata[n->leaf];const c1afl14State_t*f=c->flags+n->leaf;c1anc14Step_t step;c1atl14Node_t value;c1afl14State_t nf;double target;
  if(m->loop){
   unsigned end=0;
   if(!C1AFC14_Inner(&n->clock,f,m->frequency,1,usedDt,c->owner,enabled,&value,&end))return 0;
   nodes[index].clock=value;
   if(end){
    if(out->count==C1AMN14_EVENTS)return 0;
    out->events[out->count].leaf=(unsigned)n->leaf;out->events[out->count++].payload=m->notes[0].token;
    if(n->clock.phase!=1)flags[n->leaf].cursor=0;
   }
   return 1;
  }
  if(!C1ANC14_Inner(&n->clock,f,m->notes,m->count,m->frequency,0,usedDt,c->owner,0,&value,&nf,&step))return 0;
  /*47BA90's advance eligibility precedes47B6F0. A retained phase1 alone
   * must not manufacture end when the unrounded new target goes backward. */
  target=(double)m->frequency*n->clock.rate;target*=usedDt;target+=n->clock.oldPhase;if(target>=1)target=1;
  if(enabled&&c->owner&&f->token&&target>0&&n->clock.phase<=target){unsigned begin=0,end=0;
   if(n->clock.phase==1){begin=m->count-1;end=m->count;}
   else if(value.phase==1){if(m->notes[(unsigned)f->cursor].phase>=n->clock.phase){begin=(unsigned)f->cursor;end=m->count;}}
   else if(value.phase>m->notes[(unsigned)f->cursor].phase&&m->notes[(unsigned)f->cursor].phase>=n->clock.phase){begin=(unsigned)f->cursor;for(end=begin;end<m->count&&m->notes[end].phase<value.phase;end++){}nf.cursor=(int16_t)C1ANC14_Cursor(m->notes,m->count,value.phase);}
   for(i=begin;i<end;i++){if(out->count==C1AMN14_EVENTS)return 0;out->events[out->count].leaf=(unsigned)n->leaf;out->events[out->count++].payload=m->notes[i].token;}
  }nodes[index].clock=value;flags[n->leaf]=nf;return 1;
 }
 for(;;){next=~0u;for(i=1;i<c->count;i++)if(c->nodes[i].parent==(int)index&&(!have||c->nodes[i].nativeOrdinal>last)&&(next==~0u||c->nodes[i].nativeOrdinal<c->nodes[next].nativeOrdinal))next=i;
  if(next==~0u)break;if(!C1AMN14_AdvanceNode(c,next,usedDt,enabled,nodes,flags,out))return 0;last=c->nodes[next].nativeOrdinal;have=1;
 }return 1;
}
static int C1AMN14_Inner(const c1amn14Call_t*c,c1amn14Node_t*nodes,c1afl14State_t flags[2],c1amn14Step_t*out){double fraction,advance;unsigned i;
 if(!C1AMN14_Validate(c))return 0;memcpy(nodes,c->nodes,c->count*sizeof(*nodes));memcpy(flags,c->flags,2*sizeof(*flags));memset(out,0,sizeof(*out));out->usedDt=c->dt;
 /*47CCD0 bypasses event search when owner/notify is disabled. */
 fraction=c->owner&&c->enabled?C1AMN14_Fraction(c,0):1;
 if(fraction!=1){advance=fraction*c->dt+(double)0.001f;if(advance<=c->dt){out->usedDt=(float)advance;out->resume=1;}}
 i=C1AMN14_AdvanceNode(c,0,out->usedDt,c->enabled,nodes,flags,out);return (int)i;
}
static int C1AMN14_AdvanceBodyPair(c1amn14Node_t*nodes,unsigned count,c1afl14State_t flags[2],const c1anm14Metadata_t*m0,const c1anm14Metadata_t*m1,float fullDt,int owner,int enabled,int mixed,unsigned bodyAuthored,c1amn14Step_t*out){
 c1amn14Call_t call;c1amn14Node_t stage[C1AMN14_NODES];c1afl14State_t fs[2];c1amn14Step_t result;const void*ptrs[5];size_t sizes[5];unsigned i,j;int ok;
 if(!nodes||!flags||!m0||!m1||!out||!count||count>C1AMN14_NODES)return 0;
 ptrs[0]=nodes;sizes[0]=count*sizeof(*nodes);ptrs[1]=flags;sizes[1]=2*sizeof(*flags);ptrs[2]=m0;sizes[2]=sizeof(*m0);ptrs[3]=m1;sizes[3]=sizeof(*m1);ptrs[4]=out;sizes[4]=sizeof(*out);
 for(i=0;i<5;i++)for(j=i+1;j<5;j++)if(C1ANC14_Overlap(ptrs[i],sizes[i],ptrs[j],sizes[j]))return 0;
 call.nodes=nodes;call.count=count;call.flags=flags;call.metadata[0]=m0;call.metadata[1]=m1;call.dt=fullDt;call.owner=owner;call.enabled=enabled;call.mixed=mixed;call.bodyAuthored=bodyAuthored;
#if defined(_MSC_VER)&&defined(_M_IX86)
 {unsigned char saved[108];uint32_t mx,nearest=0x1f80;const unsigned short cw=0x027f;
 __asm fnsave saved
 __asm stmxcsr mx
 __asm fldcw cw
 __asm ldmxcsr nearest
 ok=C1AMN14_Inner(&call,stage,fs,&result);
 __asm ldmxcsr mx
 __asm frstor saved
 }
#else
 ok=C1AMN14_Inner(&call,stage,fs,&result);
#endif
 if(!ok)return 0;memcpy(nodes,stage,count*sizeof(*nodes));memcpy(flags,fs,sizeof(fs));*out=result;return 1;
}
static int C1AMN14_AdvancePair(c1amn14Node_t*nodes,unsigned count,c1afl14State_t flags[2],const c1anm14Metadata_t*m0,const c1anm14Metadata_t*m1,float fullDt,int owner,int enabled,int mixed,c1amn14Step_t*out){
 return C1AMN14_AdvanceBodyPair(nodes,count,flags,m0,m1,fullDt,owner,enabled,mixed,1,out);
}
static int C1AMN14_Advance(c1amn14Node_t*nodes,unsigned count,c1afl14State_t flags[2],const c1anm14Metadata_t*m0,const c1anm14Metadata_t*m1,float fullDt,int owner,int enabled,c1amn14Step_t*out){
 return C1AMN14_AdvancePair(nodes,count,flags,m0,m1,fullDt,owner,enabled,0,out);
}
#endif
