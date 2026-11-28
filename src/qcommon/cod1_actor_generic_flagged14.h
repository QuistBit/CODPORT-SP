#ifndef COD1_ACTOR_GENERIC_FLAGGED14_H
#define COD1_ACTOR_GENERIC_FLAGGED14_H
#include "cod1_actor_generic_frequency14.h"
#include "cod1_actor_generic_metadata14.h"
/* ENGINE47DCF0 chooses the greatest positive direct child's goal, then
 * recurses. It does not compare cumulative weights or current weights.
 * Equal weights retain the first viable child in native definition order. */
static unsigned C1AGFL14_Descendant(const c1agf14Topology_t*t,const c1atl14Node_t*n,unsigned root){
 unsigned c,chosen=0;float best=0;
 if(t->first[root]==C1AGF14_NONE)return root;
 for(c=t->first[root];c!=C1AGF14_NONE;c=t->next[c])if(n[c].goalWeight>best){
  unsigned leaf=C1AGFL14_Descendant(t,n,c);
  if(leaf){best=n[c].goalWeight;chosen=leaf;}
 }
 return chosen;
}
static int C1AGFL14_Cursor(const c1agt14Definition_t*d,const c1atl14Node_t*n,c1afl14State_t*f,
 const c1anm14Metadata_t*const*m,unsigned i){
 unsigned leaf,cursor;
 if(!f[i].token)return 1;
 if(n[i].phase==1){f[i].cursor=-1;return 1;}
 leaf=d[i].children?f[i].descendant:i;
 if(!leaf)return 1;
 if(!m[leaf]||!C1AGM14_Valid(m[leaf]))return 0;
 cursor=C1ANC14_Cursor(m[leaf]->notes,m[leaf]->count,n[i].phase);
 if(cursor>=m[leaf]->count)return 0;
 f[i].cursor=(int16_t)cursor;return 1;
}
/* Preallocated nodes and metadata; registration only, no VM delivery.
 * Topology is privately owned and must come from C1AGF14_Build. */
static int C1AGFL14_Inner(const c1agt14Definition_t*d,c1atl14Node_t*n,c1afl14State_t*f,
 const c1agf14Topology_t*t,const c1anm14Metadata_t*const*m,
 unsigned target,unsigned root,unsigned token,float weight,float time,float rate,int force){
 unsigned count=t->count,p,i,sync,walk;int ok=0;c1atl14Node_t*stage=NULL;c1afl14State_t*flags=NULL;
 if(!count||count>C1AGF14_MAX||!target||target>=count||root>=count||target==root||token>65535||
    !C1ATL14_Finite(weight)||weight<0||(token&&weight==0)||!C1AGT14_Valid(d,n,count))return 0;
 if(token&&d[target].children&&!(d[target].flags&3))return 0;
 if(weight<.001f)weight=0;
 for(i=0;i<count;i++){
  unsigned desc=f[i].descendant;
  if(f[i].cursor<-1||f[i].cursor>255||desc>=count||
     (desc&&(!d[i].children||d[desc].children||!C1AGT14_Descendant(d,desc,i))))return 0;
  if(f[i].token){unsigned leaf=d[i].children?desc:i;
   if(leaf&&(!m[leaf]||!C1AGM14_Valid(m[leaf])||f[i].cursor>=(int)m[leaf]->count))return 0;
  }
 }
 p=d[target].parent;while(p!=root){if(!p)return 0;p=d[p].parent;}
 stage=malloc(count*sizeof(*stage));flags=malloc(count*sizeof(*flags));if(!stage||!flags)goto done;
 memcpy(stage,n,count*sizeof(*stage));memcpy(flags,f,count*sizeof(*flags));
 for(p=target;;p=d[p].parent){
  for(i=1;i<count;i++)if(i!=p&&d[i].parent==d[p].parent){flags[i].token=0;flags[i].cursor=-1;}
  flags[p].token=(uint16_t)(p==target?token:0);flags[p].cursor=-1;flags[p].descendant=0;
  if(!C1AKA14_Select(d,stage,count,p,d[p].parent,p==target?weight:1,time,p==target?rate:1,force))goto done;
  if(p==target&&token&&d[p].children){
   flags[p].descendant=(uint16_t)C1AGFL14_Descendant(t,stage,p);
   if(!flags[p].descendant)goto done;
  }
  sync=p;while(sync&&(!d[sync].children||!(d[sync].flags&4)))sync=d[sync].parent;
  if(sync){
   for(i=0;i<count;i++)if(i!=p&&C1AGT14_Descendant(d,i,p)&&!C1AGFL14_Cursor(d,stage,flags,m,i))goto done;
   for(walk=p;walk!=sync;walk=d[walk].parent)if(!C1AGFL14_Cursor(d,stage,flags,m,walk))goto done;
  }
  if(!C1AGFL14_Cursor(d,stage,flags,m,p))goto done;
  if(d[p].parent==root)break;
 }
 memcpy(n,stage,count*sizeof(*n));memcpy(f,flags,count*sizeof(*f));ok=1;
done:free(stage);free(flags);return ok;
}
static int C1AGFL14_Select(const c1agt14Definition_t*d,c1atl14Node_t*n,c1afl14State_t*f,
 const c1agf14Topology_t*t,const c1anm14Metadata_t*const*m,
 unsigned target,unsigned root,unsigned token,float weight,float time,float rate,int force){
 unsigned count,i;int ok;size_t nb,fb,db,mb;
 if(!d||!n||!f||!t||!m||!t->count||t->count>C1AGF14_MAX)return 0;
 count=t->count;nb=count*sizeof(*n);fb=count*sizeof(*f);db=count*sizeof(*d);mb=count*sizeof(*m);
 if(C1ANC14_Overlap(n,nb,f,fb)||C1ANC14_Overlap(n,nb,d,db)||C1ANC14_Overlap(f,fb,d,db)||
    C1ANC14_Overlap(n,nb,t,sizeof(*t))||C1ANC14_Overlap(f,fb,t,sizeof(*t))||
    C1ANC14_Overlap(n,nb,m,mb)||C1ANC14_Overlap(f,fb,m,mb))return 0;
 for(i=0;i<count;i++)if(m[i]&&(C1ANC14_Overlap(n,nb,m[i],sizeof(*m[i]))||C1ANC14_Overlap(f,fb,m[i],sizeof(*m[i]))))return 0;
#if defined(_MSC_VER)&&defined(_M_IX86)
 {unsigned char saved[108];uint32_t mx,nearest=0x1f80;const unsigned short cw=0x027f;
 __asm fnsave saved
 __asm stmxcsr mx
 __asm fldcw cw
 __asm ldmxcsr nearest
 ok=C1AGFL14_Inner(d,n,f,t,m,target,root,token,weight,time,rate,force);
 __asm ldmxcsr mx
 __asm frstor saved
 }
#else
 ok=C1AGFL14_Inner(d,n,f,t,m,target,root,token,weight,time,rate,force);
#endif
 return ok;
}
/* ENGINE100 and109 differ from101: only zero-goal ancestors are promoted.
 * The direct knob clears immediate siblings before the ordinary setter. */
static int C1AGFL14_SetInner(const c1agt14Definition_t*d,c1atl14Node_t*n,c1afl14State_t*f,
 const c1agf14Topology_t*t,const c1anm14Metadata_t*const*m,unsigned target,unsigned token,
 float weight,float time,float rate,int force,int knob){
 unsigned i,p,sync,count=t->count;float effective=weight,greatest=0,fade,delta;int ok=0,useMask;unsigned char active[4096];
 c1atl14Node_t*stage=NULL;c1afl14State_t*flags=NULL;
 if(!target||target>=count||token>65535||!C1AGT14_Valid(d,n,count)||(knob!=0&&knob!=1)||
    !C1ATL14_Finite(weight)||weight<0||weight>1||(token&&weight==0)||!C1ATL14_Finite(time)||time<0||
    !C1ATL14_Finite(rate)||rate<0||rate>100||(force!=0&&force!=1))return 0;
 if(token&&d[target].children&&!(d[target].flags&3))return 0;
 for(i=0;i<count;i++){
  unsigned desc=f[i].descendant,leaf=d[i].children?desc:i;
  if(desc>=count||f[i].cursor<-1||f[i].cursor>255||
     (desc&&(!d[i].children||d[desc].children||!C1AGT14_Descendant(d,desc,i))))return 0;
  if(f[i].token&&leaf&&(!m[leaf]||!C1AGM14_Valid(m[leaf])||f[i].cursor>=(int)m[leaf]->count))return 0;
 }
 stage=malloc(count*sizeof(*stage));flags=malloc(count*sizeof(*flags));if(!stage||!flags)goto done;
 memcpy(stage,n,count*sizeof(*stage));memcpy(flags,f,count*sizeof(*flags));
 if(knob){
  if(effective<.001f)effective=0;
  p=d[target].parent;
  for(i=1;i<count;i++)if(d[i].parent==p){delta=i==target?(float)((double)effective-stage[i].currentWeight):stage[i].currentWeight;if(delta<0)delta=-delta;if(delta>greatest)greatest=delta;}
  fade=(float)((double)greatest*time);if(!C1ATL14_Finite(fade))goto done;
  useMask=C1AGT49_SiblingPaths(d,stage,count,target,p,active);
  for(i=1;i<count;i++)if(i!=target&&d[i].parent==p){
   if(!C1ATL14_ClearGoal(stage+i,fade,useMask?active[i]:C1AGT14_Active(d,stage,count,i)))goto done;
   flags[i].token=0;flags[i].cursor=-1;
  }
 }
 flags[target].token=(uint16_t)token;flags[target].cursor=-1;flags[target].descendant=0;
 if(token&&d[target].children){flags[target].descendant=(uint16_t)C1AGFL14_Descendant(t,stage,target);if(!flags[target].descendant)goto done;}
 for(p=d[target].parent;;p=d[p].parent){if(!stage[p].goalWeight){flags[p].token=0;flags[p].cursor=-1;flags[p].descendant=0;}if(!p)break;}
 if(!C1AGT14_SetAnim(d,stage,count,target,effective,time,rate,force))goto done;
 sync=target;while(sync&&(!d[sync].children||!(d[sync].flags&4)))sync=d[sync].parent;
 if(sync){
  for(i=0;i<count;i++)if(i!=target&&C1AGT14_Descendant(d,i,target)&&!C1AGFL14_Cursor(d,stage,flags,m,i))goto done;
  for(p=target;p!=sync;p=d[p].parent)if(!C1AGFL14_Cursor(d,stage,flags,m,p))goto done;
 }
 if(!C1AGFL14_Cursor(d,stage,flags,m,target))goto done;
 memcpy(n,stage,count*sizeof(*n));memcpy(f,flags,count*sizeof(*f));ok=1;
done:free(stage);free(flags);return ok;
}
static int C1AGFL14_Set(const c1agt14Definition_t*d,c1atl14Node_t*n,c1afl14State_t*f,
 const c1agf14Topology_t*t,const c1anm14Metadata_t*const*m,unsigned target,unsigned token,
 float weight,float time,float rate,int force,int knob){
 unsigned count,i;int ok;size_t nb,fb,db,mb;
 if(!d||!n||!f||!t||!m||!t->count||t->count>C1AGF14_MAX)return 0;
 count=t->count;nb=count*sizeof(*n);fb=count*sizeof(*f);db=count*sizeof(*d);mb=count*sizeof(*m);
 if(C1ANC14_Overlap(n,nb,f,fb)||C1ANC14_Overlap(n,nb,d,db)||C1ANC14_Overlap(f,fb,d,db)||
    C1ANC14_Overlap(n,nb,t,sizeof(*t))||C1ANC14_Overlap(f,fb,t,sizeof(*t))||
    C1ANC14_Overlap(n,nb,m,mb)||C1ANC14_Overlap(f,fb,m,mb))return 0;
 for(i=0;i<count;i++)if(m[i]&&(C1ANC14_Overlap(n,nb,m[i],sizeof(*m[i]))||C1ANC14_Overlap(f,fb,m[i],sizeof(*m[i]))))return 0;
#if defined(_MSC_VER)&&defined(_M_IX86)
 {unsigned char saved[108];uint32_t mx,nearest=0x1f80;const unsigned short cw=0x027f;
 __asm fnsave saved
 __asm stmxcsr mx
 __asm fldcw cw
 __asm ldmxcsr nearest
 ok=C1AGFL14_SetInner(d,n,f,t,m,target,token,weight,time,rate,force,knob);
 __asm ldmxcsr mx
 __asm frstor saved
 }
#else
 ok=C1AGFL14_SetInner(d,n,f,t,m,target,token,weight,time,rate,force,knob);
#endif
 return ok;
}
#endif
