#ifndef COD1_ACTOR_TURRET_GOALS14_H
#define COD1_ACTOR_TURRET_GOALS14_H
#include "cod1_actor_tree_clock14.h"
typedef struct {unsigned lower;float fraction,lowerWeight;} c1atg14Yaw_t;
#if defined(_MSC_VER)
#define C1ATG14_NOINLINE __declspec(noinline)
#else
#define C1ATG14_NOINLINE
#endif
static C1ATG14_NOINLINE int C1ATG14_YawInner(float yaw,float increment,unsigned count,c1atg14Yaw_t*out){double coordinate;c1atg14Yaw_t v;if(!out||!count||count>64||!C1ATL14_Finite(yaw)||!C1ATL14_Finite(increment)||increment<=0)return 0;coordinate=(double)count*.5-(double)yaw/increment;if(coordinate<0)coordinate=0;if(coordinate>count-1)coordinate=count-1;v.lower=(unsigned)coordinate;v.fraction=(float)(coordinate-v.lower);v.lowerWeight=(float)(1.0-v.fraction);*out=v;return 1;}
static C1ATG14_NOINLINE int C1ATG14_HeightInner(float desired,float previous,float current,float*out){if(!out||!C1ATL14_Finite(desired)||!C1ATL14_Finite(previous)||!C1ATL14_Finite(current)||!(current>previous)||desired<previous||desired>current)return 0;*out=(float)(((double)desired-previous)/((double)current-previous));return 1;}
static C1ATG14_NOINLINE int C1ATG14_TimeInner(float current,double target,float*out){double delta;float value;if(!out||!C1ATL14_Finite(current)||current<0||current>1||!(target>=0&&target<=1))return 0;delta=(double)current-target;if(delta<0)delta=-delta;delta*=20;value=delta>0?(float)(1.0/delta):0;if(!C1ATL14_Finite(value))return 0;*out=value;return 1;}
/* Each public arithmetic entry selects the native PC53 policy and restores the
 * complete caller x87/MXCSR state; outputs are staged until successful return. */
static int C1ATG14_Yaw(float yaw,float increment,unsigned count,c1atg14Yaw_t*out){int ok;c1atg14Yaw_t v;if(!out)return 0;
#if defined(_MSC_VER)&&defined(_M_IX86)
 {unsigned char outer[108];uint32_t mx,nmx=0x1f80;const unsigned short cw=0x027f;
 __asm fnsave outer
 __asm stmxcsr mx
 __asm fldcw cw
 __asm ldmxcsr nmx
 ok=C1ATG14_YawInner(yaw,increment,count,&v);
 __asm ldmxcsr mx
 __asm frstor outer
 }
#else
 ok=C1ATG14_YawInner(yaw,increment,count,&v);
#endif
 if(!ok)return 0;*out=v;return 1;}
static int C1ATG14_Height(float desired,float previous,float current,float*out){int ok;float v;if(!out)return 0;
#if defined(_MSC_VER)&&defined(_M_IX86)
 {unsigned char outer[108];uint32_t mx,nmx=0x1f80;const unsigned short cw=0x027f;
 __asm fnsave outer
 __asm stmxcsr mx
 __asm fldcw cw
 __asm ldmxcsr nmx
 ok=C1ATG14_HeightInner(desired,previous,current,&v);
 __asm ldmxcsr mx
 __asm frstor outer
 }
#else
 ok=C1ATG14_HeightInner(desired,previous,current,&v);
#endif
 if(!ok)return 0;memcpy(out,&v,4);return 1;}
static int C1ATG14_Time(float current,double target,float*out){int ok;float v;if(!out)return 0;
#if defined(_MSC_VER)&&defined(_M_IX86)
 {unsigned char outer[108];uint32_t mx,nmx=0x1f80;const unsigned short cw=0x027f;
 __asm fnsave outer
 __asm stmxcsr mx
 __asm fldcw cw
 __asm ldmxcsr nmx
 ok=C1ATG14_TimeInner(current,target,&v);
 __asm ldmxcsr mx
 __asm frstor outer
 }
#else
 ok=C1ATG14_TimeInner(current,target,&v);
#endif
 if(!ok)return 0;memcpy(out,&v,4);return 1;}
#undef C1ATG14_NOINLINE
#endif
