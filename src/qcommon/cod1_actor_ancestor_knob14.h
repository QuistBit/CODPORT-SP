#ifndef COD1_ACTOR_ANCESTOR_KNOB14_H
#define COD1_ACTOR_ANCESTOR_KNOB14_H
#include "cod1_actor_knob_all14.h"
/* Original ENGINE101 preallocated, notify-free clock slice. The stop root is
 * exclusive. This does not register or deliver flagged animation events. */
static int C1AKR14_Inner(const c1agt14Definition_t*d,c1atl14Node_t*n,unsigned count,unsigned target,unsigned root,float weight,float time,float rate,int force){
 c1atl14Node_t*stage;unsigned p;int ok=0;
 if(!target||target>=count||root>=count||target==root||!C1AGT14_Valid(d,n,count))return 0;
 p=d[target].parent;while(p!=root){if(!p)return 0;p=d[p].parent;}
 stage=(c1atl14Node_t*)malloc(count*sizeof(*stage));if(!stage)return 0;memcpy(stage,n,count*sizeof(*stage));
 if(!C1AKA14_Select(d,stage,count,target,d[target].parent,weight,time,rate,force))goto done;
 for(p=d[target].parent;p!=root;p=d[p].parent)if(!C1AKA14_Select(d,stage,count,p,d[p].parent,1,time,1,force))goto done;
 memcpy(n,stage,count*sizeof(*n));ok=1;
 done:free(stage);return ok;
}
static int C1AKR14_Select(const c1agt14Definition_t*d,c1atl14Node_t*n,unsigned count,unsigned target,unsigned root,float weight,float time,float rate,int force){
 int ok;
#if defined(_MSC_VER) && defined(_M_IX86)
 unsigned char saved[108];uint32_t mx,nearest=0x1f80;const unsigned short cw=0x027f;
 __asm fnsave saved
 __asm stmxcsr mx
 __asm fldcw cw
 __asm ldmxcsr nearest
 ok=C1AKR14_Inner(d,n,count,target,root,weight,time,rate,force);
 __asm ldmxcsr mx
 __asm frstor saved
#else
 ok=C1AKR14_Inner(d,n,count,target,root,weight,time,rate,force);
#endif
 return ok;
}
#endif
