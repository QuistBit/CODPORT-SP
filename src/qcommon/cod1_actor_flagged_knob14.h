#ifndef COD1_ACTOR_FLAGGED_KNOB14_H
#define COD1_ACTOR_FLAGGED_KNOB14_H
#include "cod1_actor_named_clock14.h"
/* Original ENGINE100:47D8B0 ->47D5F0 ->47E5D0 ->47DD90/47E290/47E190.
 * Bounded preallocated SERVER unsynchronized leaf selection. This operation
 * clears only immediate siblings and promotes only ancestors whose goal is
 * zero. It is distinct from ENGINE101/C1AFL14 ancestor-wide selection.
 * notes is the actual source-ordered processed TARGET resource metadata,
 * including the original loader's implicit end. No string ownership, event
 * delivery or resource admission is provided. The owner handles token leases.
 * Other leaves' positive cursors are preserved unless natively cancelled;
 * their metadata validation remains with their resource owner. */
static int C1AFK14_Inner(const c1agt14Definition_t*d,c1atl14Node_t*n,c1afl14State_t*f,unsigned count,unsigned target,unsigned token,float weight,float time,float rate,int force,const c1anc14Note_t*notes,unsigned noteCount){
 c1atl14Node_t*stage=NULL;c1afl14State_t*flags=NULL;unsigned i,p;float epsilon,greatest=0,delta,fade,effective;const uint32_t epsBits=0x3a83126fu;int ok=0,useMask;unsigned char active[4096];
 if(!d||!n||!f||!notes||!count||count>4096||!target||target>=count||!token||token>65535||!noteCount||noteCount>C1ANC14_MAX_NOTES||
    (force!=0&&force!=1)||!C1ATL14_Finite(weight)||weight<=0||weight>1||!C1ATL14_Finite(time)||time<0||!C1ATL14_Finite(rate)||rate<0||rate>100)return 0;
 if(C1ANC14_Overlap(d,count*sizeof(*d),n,count*sizeof(*n))||C1ANC14_Overlap(d,count*sizeof(*d),f,count*sizeof(*f))||
    C1ANC14_Overlap(d,count*sizeof(*d),notes,noteCount*sizeof(*notes))||C1ANC14_Overlap(n,count*sizeof(*n),f,count*sizeof(*f))||
    C1ANC14_Overlap(n,count*sizeof(*n),notes,noteCount*sizeof(*notes))||C1ANC14_Overlap(f,count*sizeof(*f),notes,noteCount*sizeof(*notes)))return 0;
 if(!C1AGT14_Valid(d,n,count)||d[target].children)return 0;
 for(i=0;i<count;i++)if(f[i].cursor<-1||f[i].cursor>255||
    (f[i].descendant&&(f[i].descendant>=count||d[f[i].descendant].children||!C1AGT14_Descendant(d,f[i].descendant,i))))return 0;
 /* Dormant or unrelated sync branches are present in the real generic_human
  * tree. Only the selected ancestry would require unsupported sync restart.
  * Immediate sibling cancellation preserves its native descendant field. */
 for(p=target;;p=d[p].parent){if((p&&d[p].children&&(d[p].flags&4))||f[p].descendant)return 0;if(!p)break;}
 for(i=0;i<noteCount;i++)if(!notes[i].token||!C1ATL14_Finite(notes[i].phase)||notes[i].phase<0||notes[i].phase>1||(i&&notes[i].phase<notes[i-1].phase))return 0;
 if(notes[noteCount-1].phase!=1)return 0;
 stage=malloc(count*sizeof(*stage));flags=malloc(count*sizeof(*flags));if(!stage||!flags)goto done;
 memcpy(stage,n,count*sizeof(*stage));memcpy(flags,f,count*sizeof(*flags));memcpy(&epsilon,&epsBits,4);effective=weight<epsilon?0:weight;
 /*47D8B0 clamps weight before47D5F0 measures sibling fade distance. */
 p=d[target].parent;
 for(i=1;i<count;i++)if(d[i].parent==p){delta=i==target?(float)((double)effective-stage[i].currentWeight):stage[i].currentWeight;if(delta<0)delta=-delta;if(delta>greatest)greatest=delta;}
 fade=(float)((double)greatest*time);if(!C1ATL14_Finite(fade))goto done;
 useMask=C1AGT49_SiblingPaths(d,stage,count,target,p,active);
 for(i=1;i<count;i++)if(i!=target&&d[i].parent==p){
  if(!C1ATL14_ClearGoal(stage+i,fade,useMask?active[i]:C1AGT14_Active(d,stage,count,i)))goto done;
  flags[i].token=0;flags[i].cursor=-1;
 }
 /*47E5D0's conditional ancestors are promoted after the target goal and before
  * restart. The shared SetAnim operation preserves this native ordering. */
 flags[target].token=(uint16_t)token;flags[target].cursor=-1;flags[target].descendant=0;
 for(p=d[target].parent;;p=d[p].parent){if(stage[p].goalWeight==0){flags[p].token=0;flags[p].cursor=-1;flags[p].descendant=0;}if(!p)break;}
 if(!C1AGT14_SetAnim(d,stage,count,target,effective,time,rate,force))goto done;
 /*47E190 handles the exact endpoint specially;47A580 retains the first note
  * in source order among equal minimum phases greater than/equal to phase. */
 flags[target].cursor=stage[target].phase==1?-1:(int16_t)C1ANC14_Cursor(notes,noteCount,stage[target].phase);
 memcpy(n,stage,count*sizeof(*n));memcpy(f,flags,count*sizeof(*f));ok=1;
 done:free(stage);free(flags);return ok;
}
static int C1AFK14_Select(const c1agt14Definition_t*d,c1atl14Node_t*n,c1afl14State_t*f,unsigned count,unsigned target,unsigned token,float weight,float time,float rate,int force,const c1anc14Note_t*notes,unsigned noteCount){
 int ok;
#if defined(_MSC_VER)&&defined(_M_IX86)
 unsigned char saved[108];uint32_t mx,nearest=0x1f80;const unsigned short cw=0x027f;
 __asm fnsave saved
 __asm stmxcsr mx
 __asm fldcw cw
 __asm ldmxcsr nearest
 ok=C1AFK14_Inner(d,n,f,count,target,token,weight,time,rate,force,notes,noteCount);
 __asm ldmxcsr mx
 __asm frstor saved
#else
 ok=C1AFK14_Inner(d,n,f,count,target,token,weight,time,rate,force,notes,noteCount);
#endif
 return ok;
}
#endif
