#ifndef BG_COD1_TURRET_UNOCCUPIED14_H
#define BG_COD1_TURRET_UNOCCUPIED14_H
/* Original22960 unlinked, no client/Actor owner, no target branch. Mode2
 * automatic enemy selection is deliberately outside this value component.
 * The caller supplies actual owner/target absence, native think scheduling,
 * and publishes returned sound/state effects under the original lifetime.
 * It is not firing permission or a replacement for the actual world owner. */
#include "bg_cod1_turret_constructor14.h"
#include "../qcommon/cod1_actor_scripted_math14.h"
typedef struct {float controls[3];int32_t nextThink;uint8_t loopSound,stopEventSound,notifyStateChange,reserved;} C1TU14Projection;
static double C1TU14_Approach(float current,float desired,float step){float d=C1ASM14_Difference(desired,current);if(d>step)d=step;else if(d< -step)d=-step;return (double)current+d;}
static int C1TU14_Update(const C1TS14Record*input,const float controls[3],int now,C1TS14Record*out,C1TU14Projection*projection){
 C1TS14Record r;C1TU14Projection p;unsigned i;float effective,bounded,step,originalPitch;double nextPitch;
 unsigned char env[108];uint32_t mx,nearMx=0x1f80;const unsigned short cw=0x027f;
 if(!input||!controls||!out||!projection||now<0||now>2147483597||input->inuse!=1||input->targetEntity||((input->flags&3)==2)||input->state<0||input->state>2||!C1TS14_Finite(input->restPitch)||!C1TS14_Finite(input->pitchLimit))return 0;
 if(C1TS14_Overlap(out,sizeof(*out),projection,sizeof(*projection))||C1TS14_Overlap(projection,sizeof(*projection),input,sizeof(*input))||C1TS14_Overlap(out,sizeof(*out),controls,12)||C1TS14_Overlap(projection,sizeof(*projection),controls,12))return 0;
 for(i=0;i<3;i++){uint32_t bits;memcpy(&bits,controls+i,4);if((bits&0x7fffffffu)>0x45610000u)return 0;}
 r=*input;memset(&p,0,sizeof(p));memcpy(p.controls,controls,12);p.nextThink=now+50;
 if(r.soundMsec>0){p.loopSound=r.loopSound;r.soundMsec-=50;if(r.soundMsec<=0&&r.stopSound){p.loopSound=0;p.stopEventSound=r.stopSound;}}
 __asm fnsave env
 __asm stmxcsr mx
 __asm fldcw cw
 __asm ldmxcsr nearMx
 if(r.flags&1){
  originalPitch=p.controls[0];effective=(float)((double)originalPitch+p.controls[2]);
  step=(float)((double)((r.flags&0x300)==0x300?360.0f:200.0f)*(double).05f);
  effective=(float)C1TU14_Approach(effective,r.restPitch,step);
  p.controls[1]=(float)C1TU14_Approach(p.controls[1],0,10.0f);bounded=effective;
  if(r.flags&0x200){if(r.flags&0x400){if(effective<r.pitchLimit)bounded=r.pitchLimit;else r.flags&=~0x100;}else{if(effective>r.pitchLimit)bounded=r.pitchLimit;else r.flags&=~0x100;}}
  nextPitch=C1TU14_Approach(originalPitch,bounded,step);p.controls[0]=(float)nextPitch;p.controls[2]=(float)((double)effective-nextPitch);
 }else{
  r.flags&=~0x48;r.previousTarget=0;p.notifyStateChange=r.state!=0;r.state=0;
 }
 __asm ldmxcsr mx
 __asm frstor env
 *out=r;*projection=p;return 1;
}
#endif
