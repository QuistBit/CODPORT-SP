#ifndef BG_COD1_LOOP_PARAMETERS_H
#define BG_COD1_LOOP_PARAMETERS_H
/* Original 443390 (fresh gain/pitch), 51B5EC (TLS rand arithmetic),
 * 443010 (refresh gain). Caller owns random state and selected alias pair.
 * This does not select weighted variants or claim the original TLS schedule. */
#include "bg_cod1_script_sound_packet.h"
typedef struct {float volume_min,volume_max,pitch_min,pitch_max;} C1LRange;
typedef struct {float gain,pitch,refresh_gain;uint32_t seed;} C1LParameters;
static unsigned C1L_Rand15(uint32_t*seed){*seed=*seed*214013u+2531011u;return (*seed>>16)&32767u;}
static int C1L_RangeValid(const C1LRange*r){return r&&CQ_Finite(r->volume_min)&&CQ_Finite(r->volume_max)&&r->volume_min>=0&&r->volume_max>=r->volume_min&&r->volume_max<=2.5f&&CQ_Finite(r->pitch_min)&&CQ_Finite(r->pitch_max)&&r->pitch_min>0&&r->pitch_max>=r->pitch_min&&r->pitch_max<=4;}
static int C1L_ResolveDraws(const C1LRange*a,const C1LRange*b,float blend,unsigned gainDraw,unsigned pitchDraw,C1LParameters*out){
 float lo,hi,plo,phi;double inv;C1LParameters p;
 if(!out||gainDraw>32767||pitchDraw>32767||!C1L_RangeValid(a)||!C1L_RangeValid(b)||!CQ_Finite(blend)||blend<0||blend>1)return 0;
 inv=1.0-(double)blend;
 lo=(float)(inv*a->volume_min+(double)blend*b->volume_min);hi=(float)(inv*a->volume_max+(double)blend*b->volume_max);
 plo=(float)(inv*a->pitch_min+(double)blend*b->pitch_min);phi=(float)(inv*a->pitch_max+(double)blend*b->pitch_max);
 p.gain=(float)(((double)lo+((double)hi-lo)*(gainDraw*(double)(1.0f/32768.0f)))*(double)0.8f);
 p.pitch=(float)((double)plo+((double)phi-plo)*(pitchDraw*(double)(1.0f/32768.0f)));
 /* Refresh has no intermediate float store before the 0.8 multiplication. */
 p.refresh_gain=(float)((inv*a->volume_min+(double)blend*b->volume_min)*(double)0.8f);p.seed=0;*out=p;return 1;
}
static int C1L_ResolveParameters(const C1LRange*a,const C1LRange*b,float blend,uint32_t seed,C1LParameters*out){unsigned g=C1L_Rand15(&seed),p=C1L_Rand15(&seed);if(!C1L_ResolveDraws(a,b,blend,g,p,out))return 0;out->seed=seed;return 1;}
/* Original443440: float difference stores, x87 squared distance comparison.
 * Supported spatial auto-channel only. Native target is MSVC x86; retain its
 * active x87 precision mode rather than changing the process control word. */
static int C1L_WithinDistance(const float*listener,const float*origin,float maximum){
 float x,y,z;int i;for(i=0;i<3;i++)if(!CQ_Finite(listener[i])||!CQ_Finite(origin[i]))return 0;
 if(!CQ_Finite(maximum)||maximum<0)return 0;
 x=listener[0]-origin[0];y=listener[1]-origin[1];z=listener[2]-origin[2];
#if defined(_MSC_VER) && defined(_M_IX86)
 {unsigned short status;
 __asm {fld z}
 __asm {fmul z}
 __asm {fld y}
 __asm {fmul y}
 __asm {faddp st(1),st(0)}
 __asm {fld x}
 __asm {fmul x}
 __asm {faddp st(1),st(0)}
 __asm {fld maximum}
 __asm {fmul maximum}
 __asm {fcompp}
 __asm {fnstsw status}
 return !(status&0x0500);
 }
#else
 return (double)x*x+(double)y*y+(double)z*z<=(double)maximum*maximum;
#endif
}
#endif
