#ifndef COD1_ACTOR_FLAGGED_CLOCK14_H
#define COD1_ACTOR_FLAGGED_CLOCK14_H
#include "cod1_actor_flagged_leaf14.h"
/* Native47BA90 server leaf phase and47B6F0 implicit-end crossing domain.
 * Input includes the caller's actual old/current phase and signed16bit cycles.
 * This routine emits an owned arithmetic receipt only, never a VM event. */
static int C1AFC14_Inner(const c1atl14Node_t*n,const c1afl14State_t*f,float frequency,int loop,float dt,int ownerPresent,int notifyEnabled,c1atl14Node_t*out,unsigned*endEvent){
 c1atl14Node_t value;double phase,delta;unsigned cycle;float rounded;
 if(!n||!f||!out||!endEvent||!C1ATL14_Finite(frequency)||frequency<0||frequency>120||
    !C1ATL14_Finite(dt)||dt<0||dt>60||(loop!=0&&loop!=1)||(ownerPresent!=0&&ownerPresent!=1)||(notifyEnabled!=0&&notifyEnabled!=1)||
    !C1ATL14_Finite(n->phase)||n->phase<0||n->phase>1||!C1ATL14_Finite(n->oldPhase)||n->oldPhase<0||n->oldPhase>1||
    !C1ATL14_Finite(n->currentWeight)||n->currentWeight<0||n->currentWeight>1||!C1ATL14_Finite(n->goalWeight)||n->goalWeight<0||n->goalWeight>1||
    !C1ATL14_Finite(n->rate)||n->rate<0||n->rate>100||n->cycle>65535||n->oldCycle>65535||f->descendant||f->cursor<-1||f->cursor>0||
    (f->token&&n->phase<1&&f->cursor!=0))return 0;
 value=*n;*endEvent=0;if(n->currentWeight==0){*out=value;return 1;}
 delta=(double)frequency*n->rate;delta*=dt;if(delta>65535)return 0;
 phase=delta+n->oldPhase;cycle=n->oldCycle;
 if(loop){while(phase<0){phase+=1;cycle=(cycle-1)&65535u;}while(phase>=1){phase-=1;cycle=(cycle+1)&65535u;}}
 else{if(phase>=1)phase=1;else if(phase<=0){*out=value;return 1;}}
 if((double)n->phase-phase>(int)(int16_t)cycle-(int)(int16_t)n->cycle){*out=value;return 1;}
 rounded=(float)phase;value.phase=rounded;value.cycle=cycle;
 if(notifyEnabled&&ownerPresent&&f->token&&n->goalWeight>0&&(n->phase==1||rounded==1||n->phase>rounded))*endEvent=1;
 *out=value;return 1;
}
static int C1AFC14_Advance(c1atl14Node_t*n,const c1afl14State_t*f,float frequency,int loop,float dt,int ownerPresent,int notifyEnabled,unsigned*endEvent){
 c1atl14Node_t value;unsigned event;int ok;if(!n||!f||!endEvent)return 0;
 {uintptr_t a=(uintptr_t)n,b=(uintptr_t)f,c=(uintptr_t)endEvent;if((a<=b?b-a<sizeof(*n):a-b<sizeof(*f))||(a<=c?c-a<sizeof(*n):a-c<sizeof(*endEvent))||(b<=c?c-b<sizeof(*f):b-c<sizeof(*endEvent)))return 0;}
#if defined(_MSC_VER) && defined(_M_IX86)
 unsigned char saved[108];uint32_t mx,nearest=0x1f80;const unsigned short cw=0x027f;
 __asm fnsave saved
 __asm stmxcsr mx
 __asm fldcw cw
 __asm ldmxcsr nearest
 ok=C1AFC14_Inner(n,f,frequency,loop,dt,ownerPresent,notifyEnabled,&value,&event);
 __asm ldmxcsr mx
 __asm frstor saved
#else
 ok=C1AFC14_Inner(n,f,frequency,loop,dt,ownerPresent,notifyEnabled,&value,&event);
#endif
 if(!ok)return 0;*n=value;*endEvent=event;return 1;
}
#endif
