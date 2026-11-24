#ifndef BG_COD1_PATH_DELTA_H
#define BG_COD1_PATH_DELTA_H
#include "bg_cod1_path_navigation.h"
/* GAME6160C..616AE. Retain the x87 transcendental result until the native
 * multiply/store sequence; an early double store can change the last bit. */
static void C1PNDeltaRaw(float*output,const unsigned char*path,float step){
#if defined(_MSC_VER)&&defined(_M_IX86)
 const float minimum=0.0000009999999974752427f,floorScale=0.6000000238418579f;const double exponent=0.3330000042915344;
 __asm {
 mov edx,path
 mov ecx,output
 fld dword ptr [edx+3a8h]
 fmul dword ptr [edx+39ch]
 fld dword ptr [edx+3a4h]
 fmul dword ptr [edx+398h]
 faddp st(1),st
 fcom minimum
 fnstsw ax
 test ah,5
 jp positiveDot
 fstp st(0)
 fld minimum
positiveDot:
 fldln2
 fxch st(1)
 fyl2x
 fmul exponent
 fldl2e
 fmulp st(1),st
 fld st(0)
 frndint
 fxch st(1)
 fsub st,st(1)
 f2xm1
 fld1
 faddp st(1),st
 fscale
 fstp st(1)
 fcom floorScale
 fnstsw ax
 test ah,5
 jp scaleReady
 fstp st(0)
 fld floorScale
scaleReady:
 fmul step
 fcom dword ptr [edx+3ach]
 fnstsw ax
 test ah,41h
 jne lengthReady
 fstp st(0)
 fld dword ptr [edx+3ach]
lengthReady:
 fld st(0)
 fmul dword ptr [edx+398h]
 fstp dword ptr [ecx]
 fld st(0)
 fmul dword ptr [edx+39ch]
 fstp dword ptr [ecx+4]
 fmul dword ptr [edx+3a0h]
 fstp dword ptr [ecx+8]
 }
#else
 double dot=(double)C1PFFloat(path,0x3a8)*C1PFFloat(path,0x39c)+(double)C1PFFloat(path,0x3a4)*C1PFFloat(path,0x398);
 double scale=pow(dot<(float).000001?(float).000001:dot,0.3330000042915344),length;unsigned i;
 if(scale<(float).6)scale=(float).6;length=scale*step;if(length>C1PFFloat(path,0x3ac))length=C1PFFloat(path,0x3ac);
 for(i=0;i<3;i++)output[i]=(float)(length*C1PFFloat(path,0x398+i*4));
#endif
}
static int C1PNDeltaInner(unsigned char*out,float*delta,const unsigned char*input,const C1PNGraph*g,const float*origin,int skipTrace,int passedIsActor,int now,float step,C1NRTrace sight,c1mmp14Query_t trace,C1PRContact contact,void*ctx){
 unsigned char p[C1PF_BYTES];float v[3];
 if(!out||!delta||!C1PFData(input,origin)||!isfinite(step)||step<0||step>10000||!C1PNSeparate(delta,12,g,input,origin,NULL)||C1PSOverlap(delta,12,out,C1PF_BYTES))return 0;
 if(!C1PNUpdateInner(p,input,g,origin,skipTrace,0,passedIsActor,now,sight,trace,contact,ctx))return 0;
 C1PNDeltaRaw(v,p,step);if(!C1MMP14_Vec(v))return 0;memcpy(out,p,sizeof(p));memcpy(delta,v,12);return 1;
}
static int C1PNDelta(unsigned char*out,float*delta,const unsigned char*input,const C1PNGraph*g,const float*origin,int skipTrace,int passedIsActor,int now,float step,C1NRTrace sight,c1mmp14Query_t trace,C1PRContact contact,void*ctx){
 int ok;
 if(!C1PNSeparate(out,C1PF_BYTES,g,out==input?NULL:input,origin,NULL))return 0;
#if defined(_MSC_VER)&&defined(_M_IX86)
 {unsigned char saved[108],discard[108];unsigned mx,nearMx=0x1f80;unsigned short cw=0x027f;
 __asm fnsave saved
 __asm stmxcsr mx
 __asm fldcw cw
 __asm ldmxcsr nearMx
 ok=C1PNDeltaInner(out,delta,input,g,origin,skipTrace,passedIsActor,now,step,sight,trace,contact,ctx);
 __asm fnsave discard
 __asm ldmxcsr mx
 __asm frstor saved
 }
#else
 ok=C1PNDeltaInner(out,delta,input,g,origin,skipTrace,passedIsActor,now,step,sight,trace,contact,ctx);
#endif
 return ok;
}
#endif
