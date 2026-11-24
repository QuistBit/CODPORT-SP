/* GAME 54CF0 approach and 54B10 setter, with original0EB00 basis.
 * Closed arithmetic only. Target production/body rotation are separate.
 * Fixed retail1.4 rates .8 degrees/ms and50ms AI step; no rate guessing. */
#ifndef BG_COD1_ACTOR_ORIENTATION14_H
#define BG_COD1_ACTOR_ORIENTATION14_H
#include "bg_cod1_actor_look14.h"
typedef struct {float look[2],forward[3],right[3],up[3];} c1ao14Result_t;
_Static_assert(sizeof(c1ao14Result_t)==44,"Native look cells F4..120");
#if defined(_MSC_VER)&&defined(_M_IX86)
static const uint32_t C1AO14_C2009A2B8=UINT32_C(0x3c8efa35);
static const uint32_t C1AO14_C2009A3A8=UINT32_C(0xbf800000);
static const uint32_t C1AO14_C2009A2A4=UINT32_C(0x43360b61);
static const uint32_t C1AO14_C2009A2A0=UINT32_C(0x3bb40000);
static const uint32_t C1AO14_C20089D4C=UINT32_C(0x3f4ccccd);
static const uint32_t C1AO14_C2009A3F4=UINT32_C(0x42480000);
static const uint32_t C1AO14_C2009A3F0=UINT32_C(0xc2480000);
static const uint32_t C1AO14_C20089D50=UINT32_C(0x3f4ccccd);
static void C1AO14_VectorsRaw(void);
static void C1AO14_SetRaw(void);
static void C1AO14_ApproachRaw(void);
static __declspec(naked) void C1AO14_VectorsRaw(void){__asm {
C1AO14_L2000EB00:
 sub         esp,24h
C1AO14_L2000EB03:
 fld         dword ptr [edx+4]
C1AO14_L2000EB06:
 lea         eax,[esp+4]
C1AO14_L2000EB0A:
 fmul        dword ptr C1AO14_C2009A2B8
C1AO14_L2000EB10:
 lea         ecx,[esp]
C1AO14_L2000EB14:
 mov         dword ptr [esp+20h],eax
C1AO14_L2000EB18:
 mov         dword ptr [esp+1Ch],ecx
C1AO14_L2000EB1C:
 fstp        dword ptr [esp+18h]
C1AO14_L2000EB20:
 push        eax
C1AO14_L2000EB21:
 push        ecx
C1AO14_L2000EB22:
 fld         dword ptr [esp+20h]
C1AO14_L2000EB26:
 fsincos
C1AO14_L2000EB28:
 mov         ecx,dword ptr [esp+28h]
C1AO14_L2000EB2C:
 mov         eax,dword ptr [esp+24h]
C1AO14_L2000EB30:
 fstp        dword ptr [ecx]
C1AO14_L2000EB32:
 fstp        dword ptr [eax]
C1AO14_L2000EB34:
 pop         ecx
C1AO14_L2000EB35:
 pop         eax
C1AO14_L2000EB36:
 fld         dword ptr [edx]
C1AO14_L2000EB38:
 lea         eax,[esp+10h]
C1AO14_L2000EB3C:
 fmul        dword ptr C1AO14_C2009A2B8
C1AO14_L2000EB42:
 lea         ecx,[esp+14h]
C1AO14_L2000EB46:
 mov         dword ptr [esp+1Ch],eax
C1AO14_L2000EB4A:
 mov         dword ptr [esp+20h],ecx
C1AO14_L2000EB4E:
 fstp        dword ptr [esp+18h]
C1AO14_L2000EB52:
 push        eax
C1AO14_L2000EB53:
 push        ecx
C1AO14_L2000EB54:
 fld         dword ptr [esp+20h]
C1AO14_L2000EB58:
 fsincos
C1AO14_L2000EB5A:
 mov         ecx,dword ptr [esp+24h]
C1AO14_L2000EB5E:
 mov         eax,dword ptr [esp+28h]
C1AO14_L2000EB62:
 fstp        dword ptr [ecx]
C1AO14_L2000EB64:
 fstp        dword ptr [eax]
C1AO14_L2000EB66:
 pop         ecx
C1AO14_L2000EB67:
 pop         eax
C1AO14_L2000EB68:
 test        esi,esi
C1AO14_L2000EB6A:
 je C1AO14_L2000EB8A
C1AO14_L2000EB6C:
 fld         dword ptr [esp+10h]
C1AO14_L2000EB70:
 fmul        dword ptr [esp+4]
C1AO14_L2000EB74:
 fstp        dword ptr [esi]
C1AO14_L2000EB76:
 fld         dword ptr [esp+10h]
C1AO14_L2000EB7A:
 fmul        dword ptr [esp]
C1AO14_L2000EB7E:
 fstp        dword ptr [esi+4]
C1AO14_L2000EB81:
 fld         dword ptr [esp+14h]
C1AO14_L2000EB85:
 fchs
C1AO14_L2000EB87:
 fstp        dword ptr [esi+8]
C1AO14_L2000EB8A:
 test        edi,edi
C1AO14_L2000EB8C:
 jne C1AO14_L2000EB96
C1AO14_L2000EB8E:
 test        ebx,ebx
C1AO14_L2000EB90:
 je C1AO14_L2000EC5D
C1AO14_L2000EB96:
 fld         dword ptr [edx+8]
C1AO14_L2000EB99:
 lea         edx,[esp+0Ch]
C1AO14_L2000EB9D:
 fmul        dword ptr C1AO14_C2009A2B8
C1AO14_L2000EBA3:
 lea         eax,[esp+8]
C1AO14_L2000EBA7:
 mov         dword ptr [esp+1Ch],edx
C1AO14_L2000EBAB:
 mov         dword ptr [esp+20h],eax
C1AO14_L2000EBAF:
 fstp        dword ptr [esp+18h]
C1AO14_L2000EBB3:
 push        eax
C1AO14_L2000EBB4:
 push        ecx
C1AO14_L2000EBB5:
 fld         dword ptr [esp+20h]
C1AO14_L2000EBB9:
 fsincos
C1AO14_L2000EBBB:
 mov         ecx,dword ptr [esp+24h]
C1AO14_L2000EBBF:
 mov         eax,dword ptr [esp+28h]
C1AO14_L2000EBC3:
 fstp        dword ptr [ecx]
C1AO14_L2000EBC5:
 fstp        dword ptr [eax]
C1AO14_L2000EBC7:
 pop         ecx
C1AO14_L2000EBC8:
 pop         eax
C1AO14_L2000EBC9:
 test        edi,edi
C1AO14_L2000EBCB:
 je C1AO14_L2000EC19
C1AO14_L2000EBCD:
 fld         dword ptr [esp+8]
C1AO14_L2000EBD1:
 fmul        dword ptr [esp+14h]
C1AO14_L2000EBD5:
 fstp        dword ptr [esp+18h]
C1AO14_L2000EBD9:
 fld         dword ptr [esp+0Ch]
C1AO14_L2000EBDD:
 fmul        dword ptr [esp]
C1AO14_L2000EBE1:
 fld         dword ptr [esp+18h]
C1AO14_L2000EBE5:
 fmul        dword ptr [esp+4]
C1AO14_L2000EBE9:
 fsubp       st(1),st
C1AO14_L2000EBEB:
 fstp        dword ptr [edi]
C1AO14_L2000EBED:
 fld         dword ptr [esp+0Ch]
C1AO14_L2000EBF1:
 fmul        dword ptr [esp+4]
C1AO14_L2000EBF5:
 fmul        dword ptr C1AO14_C2009A3A8
C1AO14_L2000EBFB:
 fld         dword ptr [esp+18h]
C1AO14_L2000EBFF:
 fmul        dword ptr [esp]
C1AO14_L2000EC03:
 fsubp       st(1),st
C1AO14_L2000EC05:
 fstp        dword ptr [edi+4]
C1AO14_L2000EC08:
 fld         dword ptr [esp+8]
C1AO14_L2000EC0C:
 fmul        dword ptr [esp+10h]
C1AO14_L2000EC10:
 fmul        dword ptr C1AO14_C2009A3A8
C1AO14_L2000EC16:
 fstp        dword ptr [edi+8]
C1AO14_L2000EC19:
 test        ebx,ebx
C1AO14_L2000EC1B:
 je C1AO14_L2000EC5D
C1AO14_L2000EC1D:
 fld         dword ptr [esp+0Ch]
C1AO14_L2000EC21:
 fmul        dword ptr [esp+14h]
C1AO14_L2000EC25:
 fstp        dword ptr [esp+18h]
C1AO14_L2000EC29:
 fld         dword ptr [esp+18h]
C1AO14_L2000EC2D:
 fmul        dword ptr [esp+4]
C1AO14_L2000EC31:
 fld         dword ptr [esp+8]
C1AO14_L2000EC35:
 fmul        dword ptr [esp]
C1AO14_L2000EC39:
 faddp       st(1),st
C1AO14_L2000EC3B:
 fstp        dword ptr [ebx]
C1AO14_L2000EC3D:
 fld         dword ptr [esp+18h]
C1AO14_L2000EC41:
 fmul        dword ptr [esp]
C1AO14_L2000EC45:
 fld         dword ptr [esp+8]
C1AO14_L2000EC49:
 fmul        dword ptr [esp+4]
C1AO14_L2000EC4D:
 fsubp       st(1),st
C1AO14_L2000EC4F:
 fstp        dword ptr [ebx+4]
C1AO14_L2000EC52:
 fld         dword ptr [esp+0Ch]
C1AO14_L2000EC56:
 fmul        dword ptr [esp+10h]
C1AO14_L2000EC5A:
 fstp        dword ptr [ebx+8]
C1AO14_L2000EC5D:
 add         esp,24h
C1AO14_L2000EC60:
 ret
}}
static __declspec(naked) void C1AO14_SetRaw(void){__asm {
C1AO14_L20054B10:
 sub         esp,0Ch
C1AO14_L20054B13:
 fld         dword ptr [esp+10h]
C1AO14_L20054B17:
 push        ebx
C1AO14_L20054B18:
 fmul        dword ptr C1AO14_C2009A2A4
C1AO14_L20054B1E:
 push        esi
C1AO14_L20054B1F:
 push        edi
C1AO14_L20054B20:
 mov         esi,eax
C1AO14_L20054B22:
 call C1AL14_Trunc
C1AO14_L20054B27:
 and         eax,0FFFFh
C1AO14_L20054B2C:
 mov         dword ptr [esp+1Ch],eax
C1AO14_L20054B30:
 fild        dword ptr [esp+1Ch]
C1AO14_L20054B34:
 fstp        dword ptr [esp+1Ch]
C1AO14_L20054B38:
 fld         dword ptr [esp+1Ch]
C1AO14_L20054B3C:
 fmul        dword ptr C1AO14_C2009A2A0
C1AO14_L20054B42:
 fstp        dword ptr [esp+1Ch]
C1AO14_L20054B46:
 fld         dword ptr [esp+20h]
C1AO14_L20054B4A:
 mov         eax,dword ptr [esp+1Ch]
C1AO14_L20054B4E:
 fmul        dword ptr C1AO14_C2009A2A4
C1AO14_L20054B54:
 mov         dword ptr [esi+000000F4h],eax
C1AO14_L20054B5A:
 mov         dword ptr [esp+0Ch],eax
C1AO14_L20054B5E:
 call C1AL14_Trunc
C1AO14_L20054B63:
 and         eax,0FFFFh
C1AO14_L20054B68:
 mov         dword ptr [esp+1Ch],eax
C1AO14_L20054B6C:
 fild        dword ptr [esp+1Ch]
C1AO14_L20054B70:
 lea         ebx,[esi+00000114h]
C1AO14_L20054B76:
 lea         edi,[esi+00000108h]
C1AO14_L20054B7C:
 add         esi,0FCh
C1AO14_L20054B82:
 fstp        dword ptr [esp+1Ch]
C1AO14_L20054B86:
 lea         edx,[esp+0Ch]
C1AO14_L20054B8A:
 fld         dword ptr [esp+1Ch]
C1AO14_L20054B8E:
 mov         dword ptr [esp+14h],0
C1AO14_L20054B96:
 fmul        dword ptr C1AO14_C2009A2A0
C1AO14_L20054B9C:
 fstp        dword ptr [esp+1Ch]
C1AO14_L20054BA0:
 mov         eax,dword ptr [esp+1Ch]
C1AO14_L20054BA4:
 mov         dword ptr [esi-4],eax
C1AO14_L20054BA7:
 mov         dword ptr [esp+10h],eax
C1AO14_L20054BAB:
 call C1AO14_VectorsRaw
C1AO14_L20054BB0:
 pop         edi
C1AO14_L20054BB1:
 pop         esi
C1AO14_L20054BB2:
 pop         ebx
C1AO14_L20054BB3:
 add         esp,0Ch
C1AO14_L20054BB6:
 ret         8
}}
static __declspec(naked) void C1AO14_ApproachRaw(void){__asm {
C1AO14_L20054CF0:
 mov         eax,dword ptr [ecx+00000134h]
C1AO14_L20054CF6:
 lea         edx,[ecx+00000134h]
C1AO14_L20054CFC:
 sub         esp,8
C1AO14_L20054CFF:
 test        eax,eax
C1AO14_L20054D01:
 jne C1AO14_L20054D09
C1AO14_L20054D03:
 lea         edx,[ecx+00000120h]
C1AO14_L20054D09:
 mov         eax,dword ptr [ecx+000000F4h]
C1AO14_L20054D0F:
 push        eax
C1AO14_L20054D10:
 mov         eax,dword ptr [edx+4]
C1AO14_L20054D13:
 push        eax
C1AO14_L20054D14:
 call C1AL14_DeltaRaw
C1AO14_L20054D19:
 fld         dword ptr C1AO14_C20089D4C
C1AO14_L20054D1F:
 fmul        dword ptr C1AO14_C2009A3F4
C1AO14_L20054D25:
 add         esp,8
C1AO14_L20054D28:
 fstp        dword ptr [esp]
C1AO14_L20054D2C:
 fcom        dword ptr [esp]
C1AO14_L20054D30:
 fnstsw      ax
C1AO14_L20054D32:
 test        ah,41h
C1AO14_L20054D35:
 jne C1AO14_L20054D3F
C1AO14_L20054D37:
 fstp        st(0)
C1AO14_L20054D39:
 fld         dword ptr [esp]
C1AO14_L20054D3D:
 jmp C1AO14_L20054D60
C1AO14_L20054D3F:
 fld         dword ptr C1AO14_C20089D4C
C1AO14_L20054D45:
 fmul        dword ptr C1AO14_C2009A3F0
C1AO14_L20054D4B:
 fstp        dword ptr [esp]
C1AO14_L20054D4F:
 fcom        dword ptr [esp]
C1AO14_L20054D53:
 fnstsw      ax
C1AO14_L20054D55:
 test        ah,5
C1AO14_L20054D58:
 jp C1AO14_L20054D60
C1AO14_L20054D5A:
 fstp        st(0)
C1AO14_L20054D5C:
 fld         dword ptr [esp]
C1AO14_L20054D60:
 mov         eax,dword ptr [ecx+000000F8h]
C1AO14_L20054D66:
 fadd        dword ptr [ecx+000000F4h]
C1AO14_L20054D6C:
 mov         edx,dword ptr [edx+8]
C1AO14_L20054D6F:
 push        eax
C1AO14_L20054D70:
 push        edx
C1AO14_L20054D71:
 fstp        dword ptr [esp+0Ch]
C1AO14_L20054D75:
 call C1AL14_DeltaRaw
C1AO14_L20054D7A:
 fld         dword ptr C1AO14_C20089D50
C1AO14_L20054D80:
 fmul        dword ptr C1AO14_C2009A3F4
C1AO14_L20054D86:
 add         esp,8
C1AO14_L20054D89:
 fstp        dword ptr [esp]
C1AO14_L20054D8D:
 fcom        dword ptr [esp]
C1AO14_L20054D91:
 fnstsw      ax
C1AO14_L20054D93:
 test        ah,41h
C1AO14_L20054D96:
 jne C1AO14_L20054DA0
C1AO14_L20054D98:
 fstp        st(0)
C1AO14_L20054D9A:
 fld         dword ptr [esp]
C1AO14_L20054D9E:
 jmp C1AO14_L20054DC1
C1AO14_L20054DA0:
 fld         dword ptr C1AO14_C20089D50
C1AO14_L20054DA6:
 fmul        dword ptr C1AO14_C2009A3F0
C1AO14_L20054DAC:
 fstp        dword ptr [esp]
C1AO14_L20054DB0:
 fcom        dword ptr [esp]
C1AO14_L20054DB4:
 fnstsw      ax
C1AO14_L20054DB6:
 test        ah,5
C1AO14_L20054DB9:
 jp C1AO14_L20054DC1
C1AO14_L20054DBB:
 fstp        st(0)
C1AO14_L20054DBD:
 fld         dword ptr [esp]
C1AO14_L20054DC1:
 fadd        dword ptr [ecx+000000F8h]
C1AO14_L20054DC7:
 mov         eax,dword ptr [esp+4]
C1AO14_L20054DCB:
 push        ecx
C1AO14_L20054DCC:
 fstp        dword ptr [esp]
C1AO14_L20054DCF:
 push        eax
C1AO14_L20054DD0:
 mov         eax,ecx
C1AO14_L20054DD2:
 call C1AO14_SetRaw
C1AO14_L20054DD7:
 add         esp,8
C1AO14_L20054DDA:
 ret
}}
static int C1AO14_Apply(const float current[2],const float target[2],int approach,c1ao14Result_t*out){
 unsigned char actor[0x148]={0},saved[108];uint32_t bits,mx,nearestMx=0x1f80u;const unsigned short cw=0x027f;unsigned i;
 if(!current||!out||(approach&&!target))return 0;
 for(i=0;i<2;i++){memcpy(&bits,current+i,4);if((bits&0x7fffffffu)>0x45610000u)return 0;
  if(approach){memcpy(&bits,target+i,4);if((bits&0x7fffffffu)>0x45610000u)return 0;}}
 memcpy(actor+0xf4,current,8);if(approach)memcpy(actor+0x124,target,8);
 __asm fnsave saved
 __asm stmxcsr mx
 __asm fldcw cw
 __asm ldmxcsr nearestMx
 if(approach){__asm {lea ecx,actor
 call C1AO14_ApproachRaw}}
 else{__asm {mov edx,current
 push dword ptr [edx+4]
 push dword ptr [edx]
 lea eax,actor
 call C1AO14_SetRaw}}
 __asm ldmxcsr mx
 __asm frstor saved
 memcpy(out,actor+0xf4,sizeof(*out));return 1;
}
#else
static int C1AO14_Apply(const float current[2],const float target[2],int approach,c1ao14Result_t*out){(void)current;(void)target;(void)approach;(void)out;return 0;}
#endif
#endif
