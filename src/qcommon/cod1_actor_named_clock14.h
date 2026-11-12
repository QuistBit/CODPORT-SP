#ifndef COD1_ACTOR_NAMED_CLOCK14_H
#define COD1_ACTOR_NAMED_CLOCK14_H
#include "cod1_actor_flagged_clock14.h"
/* Original47CCD0 ->47B470/47A5E0 ->47BA90/47B6F0. Bounded nonloop,
 * unsynchronized leaf under identity-rate unsynchronized ancestors only.
 * The ancestor's min-fraction float checkpoint47B6CB is retained. General
 * hierarchical rates and synchronized descendants are excluded.
 * The caller supplies original processed metadata,
 * including its implicit end record at phase1; tokens identify copied payloads,
 * not script events. resume requires the caller to run the real VM queue and
 * repeat with the SAME fullDt and oldPhase/oldCycle. No VM callback occurs here. */
#define C1ANC14_MAX_NOTES 256u
typedef struct { uint16_t token; float phase; } c1anc14Note_t;
/* The actual float duration selected by47CCD0 for its whole-tree47BA90 call.
 * Fading/unflagged siblings must use this duration too; phase inversion loses
 * native rounding and cannot recover it. Invalid calls leave all output intact. */
typedef struct { unsigned count,resume; float usedDt; uint16_t payload[C1ANC14_MAX_NOTES]; } c1anc14Step_t;
static int C1ANC14_Overlap(const void*a,size_t an,const void*b,size_t bn){
 uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;return x<=y?y-x<an:x-y<bn;
}
static unsigned C1ANC14_Cursor(const c1anc14Note_t*notes,unsigned count,float phase){
 unsigned i;for(i=0;i<count;i++)if(notes[i].phase>=phase)return i;return count;
}
static int C1ANC14_Inner(const c1atl14Node_t*n,const c1afl14State_t*f,const c1anc14Note_t*notes,unsigned count,float frequency,int loop,float fullDt,int ownerPresent,int notifyEnabled,c1atl14Node_t*next,c1afl14State_t*flags,c1anc14Step_t*step){
 double delta,target,fraction=1,advance,phase;float roundedDelta,roundedTarget,usedDt=fullDt,roundedPhase;unsigned i,cursor;
 if(loop!=0||(ownerPresent!=0&&ownerPresent!=1)||(notifyEnabled!=0&&notifyEnabled!=1)||
    !C1ATL14_Finite(frequency)||frequency<0||frequency>120||!C1ATL14_Finite(fullDt)||fullDt<0||fullDt>60||
    !C1ATL14_Finite(n->phase)||n->phase<0||n->phase>1||!C1ATL14_Finite(n->oldPhase)||n->oldPhase<0||n->oldPhase>1||n->phase<n->oldPhase||
    !C1ATL14_Finite(n->currentWeight)||n->currentWeight<0||n->currentWeight>1||!C1ATL14_Finite(n->goalWeight)||n->goalWeight<0||n->goalWeight>1||
    !C1ATL14_Finite(n->rate)||n->rate<0||n->rate>100||n->cycle>65535||n->oldCycle>65535||n->cycle!=n->oldCycle||
    f->descendant||f->cursor<-1||f->cursor>=(int)count)return 0;
 for(i=0;i<count;i++)if(!notes[i].token||!C1ATL14_Finite(notes[i].phase)||notes[i].phase<0||notes[i].phase>1||(i&&notes[i].phase<notes[i-1].phase))return 0;
 if(notes[count-1].phase!=1)return 0;
 cursor=C1ANC14_Cursor(notes,count,n->phase);
 if(f->token&&n->phase<1&&(f->cursor<0||(unsigned)f->cursor!=cursor))return 0;
 *next=*n;*flags=*f;memset(step,0,sizeof(*step));step->usedDt=fullDt;
 if(n->currentWeight==0)return 1;
 /*47B470 stores the full phase delta as float BEFORE adding oldPhase. */
 delta=(double)frequency*n->rate;delta*=fullDt;roundedDelta=(float)delta;
 target=(double)roundedDelta+n->oldPhase;if(target>=1)target=1;
 roundedTarget=(float)target;
 if(notifyEnabled&&ownerPresent&&f->token&&n->goalWeight>0&&delta!=0&&n->phase<1&&target>=n->phase){
  float note=notes[(unsigned)f->cursor].phase;
  if((roundedTarget>note||roundedTarget==1)&&n->phase<=note){
   fraction=(float)(((double)note-n->oldPhase)/roundedDelta);
   if(fraction!=1){
    /*47CD21..38: float store, but compare the unrounded x87 result. */
    advance=fraction*fullDt+(double)0.001f;usedDt=(float)advance;
    if(advance<=fullDt)step->resume=1;else usedDt=fullDt;
   }
  }
 }
 step->usedDt=usedDt;
 /*47BA90 keeps the full product in x87 until the final phase store. */
 delta=(double)frequency*n->rate;delta*=usedDt;phase=delta+n->oldPhase;
 if(phase>=1)phase=1;else if(phase<=0)return 1;
 if((double)n->phase-phase>0)return 1;
 roundedPhase=(float)phase;
 if(notifyEnabled&&ownerPresent&&f->token&&n->goalWeight>0){
  if(n->phase==1){step->payload[step->count++]=notes[count-1].token;}
  else if(roundedPhase==1){
   /* Native endpoint emits the remaining source-ordered notes, including end,
    * without updating the cursor (47B8DA..928). */
   if(notes[(unsigned)f->cursor].phase>=n->phase)
    for(i=(unsigned)f->cursor;i<count;i++)step->payload[step->count++]=notes[i].token;
  }else if(roundedPhase>notes[(unsigned)f->cursor].phase&&notes[(unsigned)f->cursor].phase>=n->phase){
   for(i=(unsigned)f->cursor;i<count&&notes[i].phase<roundedPhase;i++)step->payload[step->count++]=notes[i].token;
   flags->cursor=(int16_t)C1ANC14_Cursor(notes,count,roundedPhase);
  }
 }
 next->phase=roundedPhase;next->cycle=n->oldCycle;return 1;
}
static int C1ANC14_Advance(c1atl14Node_t*n,c1afl14State_t*f,const c1anc14Note_t*notes,unsigned count,float frequency,int loop,float fullDt,int ownerPresent,int notifyEnabled,c1anc14Step_t*out){
 c1atl14Node_t next;c1afl14State_t flags;c1anc14Step_t step;size_t bytes;int ok;
 if(!n||!f||!notes||!out||!count||count>C1ANC14_MAX_NOTES)return 0;bytes=count*sizeof(*notes);
 if(C1ANC14_Overlap(n,sizeof(*n),f,sizeof(*f))||C1ANC14_Overlap(n,sizeof(*n),notes,bytes)||C1ANC14_Overlap(n,sizeof(*n),out,sizeof(*out))||
    C1ANC14_Overlap(f,sizeof(*f),notes,bytes)||C1ANC14_Overlap(f,sizeof(*f),out,sizeof(*out))||C1ANC14_Overlap(notes,bytes,out,sizeof(*out)))return 0;
#if defined(_MSC_VER)&&defined(_M_IX86)
 {unsigned char saved[108];uint32_t mx,nearest=0x1f80;const unsigned short cw=0x027f;
 __asm fnsave saved
 __asm stmxcsr mx
 __asm fldcw cw
 __asm ldmxcsr nearest
 ok=C1ANC14_Inner(n,f,notes,count,frequency,loop,fullDt,ownerPresent,notifyEnabled,&next,&flags,&step);
 __asm ldmxcsr mx
 __asm frstor saved
 }
#else
 ok=C1ANC14_Inner(n,f,notes,count,frequency,loop,fullDt,ownerPresent,notifyEnabled,&next,&flags,&step);
#endif
 if(!ok)return 0;*n=next;*f=flags;*out=step;return 1;
}
/* Separate bounded domain: looping leaf with ZERO authored notes and exactly
 * the original processed implicit end record. Uses the already native-verified
 *47BA90/47B6F0 clock and preserves its signed-cycle/full-loop quirks. */
static int C1ANC14_LoopEndInner(const c1atl14Node_t*n,const c1afl14State_t*f,const c1anc14Note_t*end,float frequency,float fullDt,int ownerPresent,int notifyEnabled,c1atl14Node_t*next,c1afl14State_t*flags,c1anc14Step_t*step){
 double delta,target,fraction,advance;float roundedDelta,usedDt=fullDt;unsigned cycle,event=0;
 if(!end->token||!C1ATL14_Finite(end->phase)||end->phase!=1)return 0;
 /* Validate the entire clock domain before any arithmetic using it. */
 if(!C1AFC14_Inner(n,f,frequency,1,0,ownerPresent,notifyEnabled,next,&event))return 0;
 *flags=*f;memset(step,0,sizeof(*step));step->usedDt=fullDt;
 delta=(double)frequency*n->rate;delta*=fullDt;
 if(!C1ATL14_Finite(fullDt)||fullDt<0||fullDt>60||delta>65535)return 0;
 roundedDelta=(float)delta;target=(double)roundedDelta+n->oldPhase;cycle=n->oldCycle;
 while(target>=1){target-=1;cycle=(cycle+1)&65535u;}
 if(n->currentWeight>0&&n->goalWeight>0&&ownerPresent&&notifyEnabled&&f->token&&delta!=0&&n->phase<1&&
    (double)n->phase-target<=(int)(int16_t)cycle-(int)(int16_t)n->cycle&&n->phase>(float)target){
  fraction=(float)(((1.0-n->oldPhase)+((int)(int16_t)n->cycle-(int)(int16_t)n->oldCycle))/roundedDelta);
  if(fraction!=1){advance=fraction*fullDt+(double)0.001f;if(advance<=fullDt){usedDt=(float)advance;step->resume=1;}}
 }
 step->usedDt=usedDt;
 if(!C1AFC14_Inner(n,f,frequency,1,usedDt,ownerPresent,notifyEnabled,next,&event))return 0;
 if(event){step->payload[step->count++]=end->token;if(n->phase!=1)flags->cursor=0;}
 return 1;
}
static int C1ANC14_AdvanceLoopEnd(c1atl14Node_t*n,c1afl14State_t*f,const c1anc14Note_t*end,float frequency,float fullDt,int ownerPresent,int notifyEnabled,c1anc14Step_t*out){
 c1atl14Node_t next;c1afl14State_t flags;c1anc14Step_t step;int ok;
 if(!n||!f||!end||!out)return 0;
 if(C1ANC14_Overlap(n,sizeof(*n),f,sizeof(*f))||C1ANC14_Overlap(n,sizeof(*n),end,sizeof(*end))||C1ANC14_Overlap(n,sizeof(*n),out,sizeof(*out))||
    C1ANC14_Overlap(f,sizeof(*f),end,sizeof(*end))||C1ANC14_Overlap(f,sizeof(*f),out,sizeof(*out))||C1ANC14_Overlap(end,sizeof(*end),out,sizeof(*out)))return 0;
#if defined(_MSC_VER)&&defined(_M_IX86)
 {unsigned char saved[108];uint32_t mx,nearest=0x1f80;const unsigned short cw=0x027f;
 __asm fnsave saved
 __asm stmxcsr mx
 __asm fldcw cw
 __asm ldmxcsr nearest
 ok=C1ANC14_LoopEndInner(n,f,end,frequency,fullDt,ownerPresent,notifyEnabled,&next,&flags,&step);
 __asm ldmxcsr mx
 __asm frstor saved
 }
#else
 ok=C1ANC14_LoopEndInner(n,f,end,frequency,fullDt,ownerPresent,notifyEnabled,&next,&flags,&step);
#endif
 if(!ok)return 0;*n=next;*f=flags;*out=step;return 1;
}
#endif
