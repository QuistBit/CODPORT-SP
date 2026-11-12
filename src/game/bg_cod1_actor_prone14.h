/* R232 actor prone geometry, selected nondebug GAME profile. Authored source
 * reconstruction of the original x86 operation/materialization order. No DLL,
 * engine image, fabricated floor or debug-draw callback is used at runtime.
 * Query profile matches native EnterProne: first capsule check enabled, terrain
 * support required, no supplied ground normal, native mask variant enabled.
 * The caller supplies genuine current capsule/box traces. This header owns no
 * entity, admission, lifetime, animation node, trajectory or notification.
 * Windows/MSVC x86 arithmetic is selected PC53/nearest; whole caller x87/MXCSR
 * preserved. Callbacks must honor the documented trace ABI and be synchronous.
 */
#ifndef BG_COD1_ACTOR_PRONE14_H
#define BG_COD1_ACTOR_PRONE14_H
#include <stdint.h>
#include <stddef.h>
#include <string.h>
typedef struct {float fraction,end[3],normal[3];uint32_t surface_flags,contents;unsigned char opaque[10],allsolid,startsolid;} c1ap14Trace_t;
typedef void (__cdecl *c1ap14TraceFn)(c1ap14Trace_t*,const float*,const float*,const float*,const float*,int,uint32_t);
typedef struct {int fits;float height,torso,waist;} c1ap14Pose_t;
_Static_assert(sizeof(c1ap14Trace_t)==48&&offsetof(c1ap14Trace_t,end)==4&&offsetof(c1ap14Trace_t,normal)==16&&offsetof(c1ap14Trace_t,allsolid)==46&&offsetof(c1ap14Trace_t,startsolid)==47,"native trace witness layout");
#if defined(_MSC_VER)&&defined(_M_IX86)
static const uint32_t C1AP14_DebugDisabled=0;
static const uint32_t C1AP14_C2009A1B0=UINT32_C(0x43340000);
static const uint32_t C1AP14_C2009A1B4=UINT32_C(0x3f000000);
static const uint32_t C1AP14_C2009A1B8=UINT32_C(0x3f800000);
static const uint32_t C1AP14_C2009A1BC=UINT32_C(0x00000000);
static const uint32_t C1AP14_C2009A1C4=UINT32_C(0x3f333333);
static const uint32_t C1AP14_C2009A23C=UINT32_C(0x41c00000);
static const uint64_t C1AP14_C2009A288=UINT64_C(0x400921fb60000000);
static const uint32_t C1AP14_C2009A298=UINT32_C(0x43b40000);
static const uint32_t C1AP14_C2009A29C=UINT32_C(0xc3340000);
static const uint32_t C1AP14_C2009A2A0=UINT32_C(0x3bb40000);
static const uint32_t C1AP14_C2009A2A4=UINT32_C(0x43360b61);
static const uint32_t C1AP14_C2009A2AC=UINT32_C(0x40000000);
static const uint32_t C1AP14_C2009A2B8=UINT32_C(0x3c8efa35);
static const uint64_t C1AP14_C2009A2C0=UINT64_C(0xc066800000000000);
static const uint32_t C1AP14_C2009A2D0=UINT32_C(0x3fc00000);
static const uint32_t C1AP14_C2009A2DC=UINT32_C(0x40c00000);
static const uint32_t C1AP14_C2009A308=UINT32_C(0x41200000);
static const uint32_t C1AP14_C2009A370=UINT32_C(0x41f00000);
static const uint32_t C1AP14_C2009A3A8=UINT32_C(0xbf800000);
static const uint32_t C1AP14_C2009A3F0=UINT32_C(0xc2480000);
static const uint32_t C1AP14_C2009A448=UINT32_C(0x40a00000);
static const uint32_t C1AP14_C2009A44C=UINT32_C(0x428c0000);
static const uint32_t C1AP14_C2009A450=UINT32_C(0x42580000);
static const uint32_t C1AP14_C2009A454=UINT32_C(0xbf400000);
static const uint32_t C1AP14_C2009A458=UINT32_C(0x40200000);
static const uint32_t C1AP14_C2009A45C=UINT32_C(0x41b00000);
static __declspec(naked) void C1AP14_Sqrt(void){__asm {fsqrt
 ret}}
static __declspec(naked) void C1AP14_Atan2(void){__asm {sub esp,4
 fnstcw word ptr [esp]
 mov word ptr [esp+2],133fh
 fldcw word ptr [esp+2]
 fpatan
 fldcw word ptr [esp]
 add esp,4
 ret}}
static __declspec(naked) void C1AP14_Trunc(void){__asm {sub esp,12
 fnstcw word ptr [esp+8]
 mov ax,word ptr [esp+8]
 or ax,0c00h
 mov word ptr [esp+10],ax
 fldcw word ptr [esp+10]
 fistp qword ptr [esp]
 fldcw word ptr [esp+8]
 mov eax,dword ptr [esp]
 mov edx,dword ptr [esp+4]
 add esp,12
 ret}}
static __declspec(naked) void C1AP14_Normalize(void){__asm {
C1AP14_L2000E030:
 sub         esp,0Ch
C1AP14_L2000E033:
 mov         edx,dword ptr [esi]
C1AP14_L2000E035:
 mov         ecx,dword ptr [esi+4]
C1AP14_L2000E038:
 mov         eax,dword ptr [esi+8]
C1AP14_L2000E03B:
 mov         dword ptr [esp],edx
C1AP14_L2000E03F:
 fld         dword ptr [esp]
C1AP14_L2000E043:
 mov         dword ptr [esp+4],ecx
C1AP14_L2000E047:
 fmul        dword ptr [esp]
C1AP14_L2000E04B:
 mov         dword ptr [esp+8],eax
C1AP14_L2000E04F:
 fld         dword ptr [esp+4]
C1AP14_L2000E053:
 fmul        dword ptr [esp+4]
C1AP14_L2000E057:
 faddp       st(1),st
C1AP14_L2000E059:
 fld         dword ptr [esp+8]
C1AP14_L2000E05D:
 fmul        dword ptr [esp+8]
C1AP14_L2000E061:
 faddp       st(1),st
C1AP14_L2000E063:
 fstp        dword ptr [esp]
C1AP14_L2000E067:
 fld         dword ptr [esp]
C1AP14_L2000E06B:
 call C1AP14_Sqrt
C1AP14_L2000E070:
 fstp        dword ptr [esp]
C1AP14_L2000E074:
 fld         dword ptr [esp]
C1AP14_L2000E078:
 fcom        dword ptr C1AP14_C2009A1BC
C1AP14_L2000E07E:
 fnstsw      ax
C1AP14_L2000E080:
 test        ah,44h
C1AP14_L2000E083:
 jnp C1AP14_L2000E0AD
C1AP14_L2000E085:
 fld         dword ptr C1AP14_C2009A1B8
C1AP14_L2000E08B:
 fdiv        st,st(1)
C1AP14_L2000E08D:
 fstp        dword ptr [esp]
C1AP14_L2000E091:
 fld         dword ptr [esp]
C1AP14_L2000E095:
 fmul        dword ptr [esi]
C1AP14_L2000E097:
 fstp        dword ptr [esi]
C1AP14_L2000E099:
 fld         dword ptr [esp]
C1AP14_L2000E09D:
 fmul        dword ptr [esi+4]
C1AP14_L2000E0A0:
 fstp        dword ptr [esi+4]
C1AP14_L2000E0A3:
 fld         dword ptr [esp]
C1AP14_L2000E0A7:
 fmul        dword ptr [esi+8]
C1AP14_L2000E0AA:
 fstp        dword ptr [esi+8]
C1AP14_L2000E0AD:
 add         esp,0Ch
C1AP14_L2000E0B0:
 ret
}}
static __declspec(naked) void C1AP14_NormalizeTo(void){__asm {
C1AP14_L2000E260:
 sub         esp,0Ch
C1AP14_L2000E263:
 mov         edx,dword ptr [edi]
C1AP14_L2000E265:
 mov         ecx,dword ptr [edi+4]
C1AP14_L2000E268:
 mov         eax,dword ptr [edi+8]
C1AP14_L2000E26B:
 mov         dword ptr [esp],edx
C1AP14_L2000E26F:
 fld         dword ptr [esp]
C1AP14_L2000E273:
 mov         dword ptr [esp+4],ecx
C1AP14_L2000E277:
 fmul        dword ptr [esp]
C1AP14_L2000E27B:
 mov         dword ptr [esp+8],eax
C1AP14_L2000E27F:
 fld         dword ptr [esp+4]
C1AP14_L2000E283:
 fmul        dword ptr [esp+4]
C1AP14_L2000E287:
 faddp       st(1),st
C1AP14_L2000E289:
 fld         dword ptr [esp+8]
C1AP14_L2000E28D:
 fmul        dword ptr [esp+8]
C1AP14_L2000E291:
 faddp       st(1),st
C1AP14_L2000E293:
 fstp        dword ptr [esp]
C1AP14_L2000E297:
 fld         dword ptr [esp]
C1AP14_L2000E29B:
 call C1AP14_Sqrt
C1AP14_L2000E2A0:
 fstp        dword ptr [esp]
C1AP14_L2000E2A4:
 fld         dword ptr [esp]
C1AP14_L2000E2A8:
 fcom        dword ptr C1AP14_C2009A1BC
C1AP14_L2000E2AE:
 fnstsw      ax
C1AP14_L2000E2B0:
 test        ah,44h
C1AP14_L2000E2B3:
 jnp C1AP14_L2000E2E1
C1AP14_L2000E2B5:
 fld         dword ptr C1AP14_C2009A1B8
C1AP14_L2000E2BB:
 fdiv        st,st(1)
C1AP14_L2000E2BD:
 fstp        dword ptr [esp]
C1AP14_L2000E2C1:
 fld         dword ptr [esp]
C1AP14_L2000E2C5:
 fmul        dword ptr [edi]
C1AP14_L2000E2C7:
 fstp        dword ptr [esi]
C1AP14_L2000E2C9:
 fld         dword ptr [esp]
C1AP14_L2000E2CD:
 fmul        dword ptr [edi+4]
C1AP14_L2000E2D0:
 fstp        dword ptr [esi+4]
C1AP14_L2000E2D3:
 fld         dword ptr [esp]
C1AP14_L2000E2D7:
 fmul        dword ptr [edi+8]
C1AP14_L2000E2DA:
 fstp        dword ptr [esi+8]
C1AP14_L2000E2DD:
 add         esp,0Ch
C1AP14_L2000E2E0:
 ret
C1AP14_L2000E2E1:
 xor         eax,eax
C1AP14_L2000E2E3:
 mov         dword ptr [esi+8],eax
C1AP14_L2000E2E6:
 mov         dword ptr [esi+4],eax
C1AP14_L2000E2E9:
 mov         dword ptr [esi],eax
C1AP14_L2000E2EB:
 add         esp,0Ch
C1AP14_L2000E2EE:
 ret
}}
static __declspec(naked) void C1AP14_Pitch(void){__asm {
C1AP14_L2000E7B0:
 fld         dword ptr [esi+4]
C1AP14_L2000E7B3:
 sub         esp,8
C1AP14_L2000E7B6:
 fcomp       dword ptr C1AP14_C2009A1BC
C1AP14_L2000E7BC:
 fnstsw      ax
C1AP14_L2000E7BE:
 test        ah,44h
C1AP14_L2000E7C1:
 jp C1AP14_L2000E802
C1AP14_L2000E7C3:
 fld         dword ptr [esi]
C1AP14_L2000E7C5:
 fcomp       dword ptr C1AP14_C2009A1BC
C1AP14_L2000E7CB:
 fnstsw      ax
C1AP14_L2000E7CD:
 test        ah,44h
C1AP14_L2000E7D0:
 jp C1AP14_L2000E802
C1AP14_L2000E7D2:
 fld         dword ptr [esi+8]
C1AP14_L2000E7D5:
 fcomp       dword ptr C1AP14_C2009A1BC
C1AP14_L2000E7DB:
 fnstsw      ax
C1AP14_L2000E7DD:
 test        ah,41h
C1AP14_L2000E7E0:
 jne C1AP14_L2000E7F2
C1AP14_L2000E7E2:
 mov         dword ptr [esp],43870000h
C1AP14_L2000E7EA:
 fld         dword ptr [esp]
C1AP14_L2000E7EE:
 add         esp,8
C1AP14_L2000E7F1:
 ret
C1AP14_L2000E7F2:
 mov         dword ptr [esp],42B40000h
C1AP14_L2000E7FA:
 fld         dword ptr [esp]
C1AP14_L2000E7FE:
 add         esp,8
C1AP14_L2000E801:
 ret
C1AP14_L2000E802:
 mov         ecx,dword ptr [esi]
C1AP14_L2000E804:
 mov         eax,dword ptr [esi+4]
C1AP14_L2000E807:
 mov         dword ptr [esp],ecx
C1AP14_L2000E80B:
 fld         dword ptr [esp]
C1AP14_L2000E80F:
 mov         dword ptr [esp+4],eax
C1AP14_L2000E813:
 fmul        dword ptr [esp]
C1AP14_L2000E817:
 fld         dword ptr [esp+4]
C1AP14_L2000E81B:
 fmul        dword ptr [esp+4]
C1AP14_L2000E81F:
 faddp       st(1),st
C1AP14_L2000E821:
 call C1AP14_Sqrt
C1AP14_L2000E826:
 fstp        dword ptr [esp+4]
C1AP14_L2000E82A:
 fld         dword ptr [esi+8]
C1AP14_L2000E82D:
 fld         dword ptr [esp+4]
C1AP14_L2000E831:
 call C1AP14_Atan2
C1AP14_L2000E836:
 fmul        qword ptr C1AP14_C2009A2C0
C1AP14_L2000E83C:
 fdiv        qword ptr C1AP14_C2009A288
C1AP14_L2000E842:
 fstp        dword ptr [esp]
C1AP14_L2000E846:
 fld         dword ptr [esp]
C1AP14_L2000E84A:
 fcomp       dword ptr C1AP14_C2009A1BC
C1AP14_L2000E850:
 fnstsw      ax
C1AP14_L2000E852:
 test        ah,5
C1AP14_L2000E855:
 jp C1AP14_L2000E865
C1AP14_L2000E857:
 fld         dword ptr [esp]
C1AP14_L2000E85B:
 fadd        dword ptr C1AP14_C2009A298
C1AP14_L2000E861:
 fstp        dword ptr [esp]
C1AP14_L2000E865:
 fld         dword ptr [esp]
C1AP14_L2000E869:
 add         esp,8
C1AP14_L2000E86C:
 ret
}}
static __declspec(naked) void C1AP14_Axes(void){__asm {
C1AP14_L2000EB00:
 sub         esp,24h
C1AP14_L2000EB03:
 fld         dword ptr [edx+4]
C1AP14_L2000EB06:
 lea         eax,[esp+4]
C1AP14_L2000EB0A:
 fmul        dword ptr C1AP14_C2009A2B8
C1AP14_L2000EB10:
 lea         ecx,[esp]
C1AP14_L2000EB14:
 mov         dword ptr [esp+20h],eax
C1AP14_L2000EB18:
 mov         dword ptr [esp+1Ch],ecx
C1AP14_L2000EB1C:
 fstp        dword ptr [esp+18h]
C1AP14_L2000EB20:
 push        eax
C1AP14_L2000EB21:
 push        ecx
C1AP14_L2000EB22:
 fld         dword ptr [esp+20h]
C1AP14_L2000EB26:
 fsincos
C1AP14_L2000EB28:
 mov         ecx,dword ptr [esp+28h]
C1AP14_L2000EB2C:
 mov         eax,dword ptr [esp+24h]
C1AP14_L2000EB30:
 fstp        dword ptr [ecx]
C1AP14_L2000EB32:
 fstp        dword ptr [eax]
C1AP14_L2000EB34:
 pop         ecx
C1AP14_L2000EB35:
 pop         eax
C1AP14_L2000EB36:
 fld         dword ptr [edx]
C1AP14_L2000EB38:
 lea         eax,[esp+10h]
C1AP14_L2000EB3C:
 fmul        dword ptr C1AP14_C2009A2B8
C1AP14_L2000EB42:
 lea         ecx,[esp+14h]
C1AP14_L2000EB46:
 mov         dword ptr [esp+1Ch],eax
C1AP14_L2000EB4A:
 mov         dword ptr [esp+20h],ecx
C1AP14_L2000EB4E:
 fstp        dword ptr [esp+18h]
C1AP14_L2000EB52:
 push        eax
C1AP14_L2000EB53:
 push        ecx
C1AP14_L2000EB54:
 fld         dword ptr [esp+20h]
C1AP14_L2000EB58:
 fsincos
C1AP14_L2000EB5A:
 mov         ecx,dword ptr [esp+24h]
C1AP14_L2000EB5E:
 mov         eax,dword ptr [esp+28h]
C1AP14_L2000EB62:
 fstp        dword ptr [ecx]
C1AP14_L2000EB64:
 fstp        dword ptr [eax]
C1AP14_L2000EB66:
 pop         ecx
C1AP14_L2000EB67:
 pop         eax
C1AP14_L2000EB68:
 test        esi,esi
C1AP14_L2000EB6A:
 je C1AP14_L2000EB8A
C1AP14_L2000EB6C:
 fld         dword ptr [esp+10h]
C1AP14_L2000EB70:
 fmul        dword ptr [esp+4]
C1AP14_L2000EB74:
 fstp        dword ptr [esi]
C1AP14_L2000EB76:
 fld         dword ptr [esp+10h]
C1AP14_L2000EB7A:
 fmul        dword ptr [esp]
C1AP14_L2000EB7E:
 fstp        dword ptr [esi+4]
C1AP14_L2000EB81:
 fld         dword ptr [esp+14h]
C1AP14_L2000EB85:
 fchs
C1AP14_L2000EB87:
 fstp        dword ptr [esi+8]
C1AP14_L2000EB8A:
 test        edi,edi
C1AP14_L2000EB8C:
 jne C1AP14_L2000EB96
C1AP14_L2000EB8E:
 test        ebx,ebx
C1AP14_L2000EB90:
 je C1AP14_L2000EC5D
C1AP14_L2000EB96:
 fld         dword ptr [edx+8]
C1AP14_L2000EB99:
 lea         edx,[esp+0Ch]
C1AP14_L2000EB9D:
 fmul        dword ptr C1AP14_C2009A2B8
C1AP14_L2000EBA3:
 lea         eax,[esp+8]
C1AP14_L2000EBA7:
 mov         dword ptr [esp+1Ch],edx
C1AP14_L2000EBAB:
 mov         dword ptr [esp+20h],eax
C1AP14_L2000EBAF:
 fstp        dword ptr [esp+18h]
C1AP14_L2000EBB3:
 push        eax
C1AP14_L2000EBB4:
 push        ecx
C1AP14_L2000EBB5:
 fld         dword ptr [esp+20h]
C1AP14_L2000EBB9:
 fsincos
C1AP14_L2000EBBB:
 mov         ecx,dword ptr [esp+24h]
C1AP14_L2000EBBF:
 mov         eax,dword ptr [esp+28h]
C1AP14_L2000EBC3:
 fstp        dword ptr [ecx]
C1AP14_L2000EBC5:
 fstp        dword ptr [eax]
C1AP14_L2000EBC7:
 pop         ecx
C1AP14_L2000EBC8:
 pop         eax
C1AP14_L2000EBC9:
 test        edi,edi
C1AP14_L2000EBCB:
 je C1AP14_L2000EC19
C1AP14_L2000EBCD:
 fld         dword ptr [esp+8]
C1AP14_L2000EBD1:
 fmul        dword ptr [esp+14h]
C1AP14_L2000EBD5:
 fstp        dword ptr [esp+18h]
C1AP14_L2000EBD9:
 fld         dword ptr [esp+0Ch]
C1AP14_L2000EBDD:
 fmul        dword ptr [esp]
C1AP14_L2000EBE1:
 fld         dword ptr [esp+18h]
C1AP14_L2000EBE5:
 fmul        dword ptr [esp+4]
C1AP14_L2000EBE9:
 fsubp       st(1),st
C1AP14_L2000EBEB:
 fstp        dword ptr [edi]
C1AP14_L2000EBED:
 fld         dword ptr [esp+0Ch]
C1AP14_L2000EBF1:
 fmul        dword ptr [esp+4]
C1AP14_L2000EBF5:
 fmul        dword ptr C1AP14_C2009A3A8
C1AP14_L2000EBFB:
 fld         dword ptr [esp+18h]
C1AP14_L2000EBFF:
 fmul        dword ptr [esp]
C1AP14_L2000EC03:
 fsubp       st(1),st
C1AP14_L2000EC05:
 fstp        dword ptr [edi+4]
C1AP14_L2000EC08:
 fld         dword ptr [esp+8]
C1AP14_L2000EC0C:
 fmul        dword ptr [esp+10h]
C1AP14_L2000EC10:
 fmul        dword ptr C1AP14_C2009A3A8
C1AP14_L2000EC16:
 fstp        dword ptr [edi+8]
C1AP14_L2000EC19:
 test        ebx,ebx
C1AP14_L2000EC1B:
 je C1AP14_L2000EC5D
C1AP14_L2000EC1D:
 fld         dword ptr [esp+0Ch]
C1AP14_L2000EC21:
 fmul        dword ptr [esp+14h]
C1AP14_L2000EC25:
 fstp        dword ptr [esp+18h]
C1AP14_L2000EC29:
 fld         dword ptr [esp+18h]
C1AP14_L2000EC2D:
 fmul        dword ptr [esp+4]
C1AP14_L2000EC31:
 fld         dword ptr [esp+8]
C1AP14_L2000EC35:
 fmul        dword ptr [esp]
C1AP14_L2000EC39:
 faddp       st(1),st
C1AP14_L2000EC3B:
 fstp        dword ptr [ebx]
C1AP14_L2000EC3D:
 fld         dword ptr [esp+18h]
C1AP14_L2000EC41:
 fmul        dword ptr [esp]
C1AP14_L2000EC45:
 fld         dword ptr [esp+8]
C1AP14_L2000EC49:
 fmul        dword ptr [esp+4]
C1AP14_L2000EC4D:
 fsubp       st(1),st
C1AP14_L2000EC4F:
 fstp        dword ptr [ebx+4]
C1AP14_L2000EC52:
 fld         dword ptr [esp+0Ch]
C1AP14_L2000EC56:
 fmul        dword ptr [esp+10h]
C1AP14_L2000EC5A:
 fstp        dword ptr [ebx+8]
C1AP14_L2000EC5D:
 add         esp,24h
C1AP14_L2000EC60:
 ret
}}
static __declspec(naked) void C1AP14_AngleDelta(void){__asm {
C1AP14_L20010680:
 fld         dword ptr [esp+4]
C1AP14_L20010684:
 fsub        dword ptr [esp+8]
C1AP14_L20010688:
 fstp        dword ptr [esp+4]
C1AP14_L2001068C:
 fld         dword ptr [esp+4]
C1AP14_L20010690:
 fcomp       dword ptr C1AP14_C2009A1B0
C1AP14_L20010696:
 fnstsw      ax
C1AP14_L20010698:
 test        ah,41h
C1AP14_L2001069B:
 jne C1AP14_L200106BF
C1AP14_L2001069D:
 lea         ecx,[ecx]
C1AP14_L200106A0:
 fld         dword ptr [esp+4]
C1AP14_L200106A4:
 fsub        dword ptr C1AP14_C2009A298
C1AP14_L200106AA:
 fstp        dword ptr [esp+4]
C1AP14_L200106AE:
 fld         dword ptr [esp+4]
C1AP14_L200106B2:
 fcomp       dword ptr C1AP14_C2009A1B0
C1AP14_L200106B8:
 fnstsw      ax
C1AP14_L200106BA:
 test        ah,41h
C1AP14_L200106BD:
 je C1AP14_L200106A0
C1AP14_L200106BF:
 fld         dword ptr [esp+4]
C1AP14_L200106C3:
 fcomp       dword ptr C1AP14_C2009A29C
C1AP14_L200106C9:
 fnstsw      ax
C1AP14_L200106CB:
 test        ah,5
C1AP14_L200106CE:
 jp C1AP14_L200106EF
C1AP14_L200106D0:
 fld         dword ptr [esp+4]
C1AP14_L200106D4:
 fadd        dword ptr C1AP14_C2009A298
C1AP14_L200106DA:
 fstp        dword ptr [esp+4]
C1AP14_L200106DE:
 fld         dword ptr [esp+4]
C1AP14_L200106E2:
 fcomp       dword ptr C1AP14_C2009A29C
C1AP14_L200106E8:
 fnstsw      ax
C1AP14_L200106EA:
 test        ah,5
C1AP14_L200106ED:
 jnp C1AP14_L200106D0
C1AP14_L200106EF:
 fld         dword ptr [esp+4]
C1AP14_L200106F3:
 ret
}}
static __declspec(naked) void C1AP14_AngleNormalize(void){__asm {
C1AP14_L20010770:
 fld         dword ptr [esp+4]
C1AP14_L20010774:
 fmul        dword ptr C1AP14_C2009A2A4
C1AP14_L2001077A:
 call C1AP14_Trunc
C1AP14_L2001077F:
 and         eax,0FFFFh
C1AP14_L20010784:
 mov         dword ptr [esp+4],eax
C1AP14_L20010788:
 fild        dword ptr [esp+4]
C1AP14_L2001078C:
 fstp        dword ptr [esp+4]
C1AP14_L20010790:
 fld         dword ptr [esp+4]
C1AP14_L20010794:
 fmul        dword ptr C1AP14_C2009A2A0
C1AP14_L2001079A:
 fstp        dword ptr [esp+4]
C1AP14_L2001079E:
 fld         dword ptr [esp+4]
C1AP14_L200107A2:
 fcomp       dword ptr C1AP14_C2009A1B0
C1AP14_L200107A8:
 fnstsw      ax
C1AP14_L200107AA:
 test        ah,41h
C1AP14_L200107AD:
 jne C1AP14_L200107BD
C1AP14_L200107AF:
 fld         dword ptr [esp+4]
C1AP14_L200107B3:
 fsub        dword ptr C1AP14_C2009A298
C1AP14_L200107B9:
 fstp        dword ptr [esp+4]
C1AP14_L200107BD:
 fld         dword ptr [esp+4]
C1AP14_L200107C1:
 ret
}}
static __declspec(naked) int C1AP14_QueryRaw(int slot,const float *origin,float radius,float height,float yaw,float *outHeight,float *outTorso,float *outWaist,int skip,int terrain,const float *normal,c1ap14TraceFn capsule,c1ap14TraceFn box,int maskVariant){__asm {
C1AP14_L20001B90:
 sub         esp,0CCh
C1AP14_L20001B96:
 fld         dword ptr [esp+000000D8h]
C1AP14_L20001B9D:
 push        ebx
C1AP14_L20001B9E:
 fchs
C1AP14_L20001BA0:
 push        ebp
C1AP14_L20001BA1:
 mov         ebp,dword ptr [esp+000000DCh]
C1AP14_L20001BA8:
 fst         dword ptr [esp+2Ch]
C1AP14_L20001BAC:
 fld         dword ptr [esp+2Ch]
C1AP14_L20001BB0:
 mov         eax,dword ptr [ebp+8]
C1AP14_L20001BB3:
 fxch        st(1)
C1AP14_L20001BB5:
 mov         dword ptr [esp+60h],eax
C1AP14_L20001BB9:
 fadd        dword ptr [ebp]
C1AP14_L20001BBC:
 mov         eax,dword ptr C1AP14_DebugDisabled
C1AP14_L20001BC1:
 test        eax,eax
C1AP14_L20001BC3:
 push        esi
C1AP14_L20001BC4:
 fstp        dword ptr [esp+5Ch]
C1AP14_L20001BC8:
 mov         esi,dword ptr [esp+00000110h]
C1AP14_L20001BCF:
 push        edi
C1AP14_L20001BD0:
 fadd        dword ptr [ebp+4]
C1AP14_L20001BD3:
 mov         dword ptr [esp+74h],0
C1AP14_L20001BDB:
 fstp        dword ptr [esp+64h]
C1AP14_L20001BDF:
 fld         dword ptr [esp+000000E8h]
C1AP14_L20001BE6:
 fadd        dword ptr [ebp]
C1AP14_L20001BE9:
 fstp        dword ptr [esp+54h]
C1AP14_L20001BED:
 fld         dword ptr [esp+000000E8h]
C1AP14_L20001BF4:
 fadd        dword ptr [ebp+4]
C1AP14_L20001BF7:
 fstp        dword ptr [esp+58h]
C1AP14_L20001BFB:
 fld         dword ptr [esp+000000ECh]
C1AP14_L20001C02:
 fadd        dword ptr [ebp+8]
C1AP14_L20001C05:
 fstp        dword ptr [esp+5Ch]
C1AP14_L20001C09:
 je C1AP14_L20001C2B
C1AP14_L20001C0B:
 mov         ebx,dword ptr C1AP14_DebugDisabled
C1AP14_L20001C11:
 push 0
C1AP14_L20001C16:
 mov         edi,1
C1AP14_L20001C1B:
 lea         edx,[esp+58h]
C1AP14_L20001C1F:
 lea         eax,[esp+64h]
C1AP14_L20001C23:
 int 3 /* debug drawing is not selected or exposed */
C1AP14_L20001C28:
 add         esp,4
C1AP14_L20001C2B:
 mov         eax,dword ptr [esp+00000100h]
C1AP14_L20001C32:
 neg         esi
C1AP14_L20001C34:
 sbb         esi,esi
C1AP14_L20001C36:
 and         esi,10000h
C1AP14_L20001C3C:
 add         esi,810011h
C1AP14_L20001C42:
 test        eax,eax
C1AP14_L20001C44:
 mov         dword ptr [esp+70h],esi
C1AP14_L20001C48:
 jne C1AP14_L20001CEB
C1AP14_L20001C4E:
 mov         ecx,dword ptr [esp+34h]
C1AP14_L20001C52:
 fld         dword ptr [ebp+8]
C1AP14_L20001C55:
 mov         eax,dword ptr [esp+000000E8h]
C1AP14_L20001C5C:
 fadd        dword ptr C1AP14_C2009A308
C1AP14_L20001C62:
 mov         edx,ecx
C1AP14_L20001C64:
 mov         dword ptr [esp+64h],edx
C1AP14_L20001C68:
 mov         edx,dword ptr [esp+000000ECh]
C1AP14_L20001C6F:
 fstp        dword ptr [esp+24h]
C1AP14_L20001C73:
 mov         dword ptr [esp+60h],ecx
C1AP14_L20001C77:
 mov         ecx,eax
C1AP14_L20001C79:
 mov         dword ptr [esp+5Ch],edx
C1AP14_L20001C7D:
 mov         edx,dword ptr [ebp+8]
C1AP14_L20001C80:
 mov         dword ptr [esp+54h],eax
C1AP14_L20001C84:
 mov         eax,dword ptr [ebp]
C1AP14_L20001C87:
 push        esi
C1AP14_L20001C88:
 mov         dword ptr [esp+5Ch],ecx
C1AP14_L20001C8C:
 mov         ecx,dword ptr [ebp+4]
C1AP14_L20001C8F:
 mov         dword ptr [esp+1Ch],edx
C1AP14_L20001C93:
 mov         edx,dword ptr [esp+000000E4h]
C1AP14_L20001C9A:
 push        edx
C1AP14_L20001C9B:
 mov         dword ptr [esp+18h],eax
C1AP14_L20001C9F:
 mov         dword ptr [esp+24h],eax
C1AP14_L20001CA3:
 mov         dword ptr [esp+1Ch],ecx
C1AP14_L20001CA7:
 mov         dword ptr [esp+28h],ecx
C1AP14_L20001CAB:
 lea         eax,[esp+24h]
C1AP14_L20001CAF:
 push        eax
C1AP14_L20001CB0:
 lea         ecx,[esp+60h]
C1AP14_L20001CB4:
 push        ecx
C1AP14_L20001CB5:
 lea         edx,[esp+70h]
C1AP14_L20001CB9:
 push        edx
C1AP14_L20001CBA:
 lea         eax,[esp+24h]
C1AP14_L20001CBE:
 push        eax
C1AP14_L20001CBF:
 lea         ecx,[esp+00000090h]
C1AP14_L20001CC6:
 push        ecx
C1AP14_L20001CC7:
 mov         dword ptr [esp+00000084h],0
C1AP14_L20001CD2:
 call        dword ptr [esp+0000012Ch]
C1AP14_L20001CD9:
 mov         al,byte ptr [esp+000000C2h]
C1AP14_L20001CE0:
 add         esp,1Ch
C1AP14_L20001CE3:
 test        al,al
C1AP14_L20001CE5:
 jne C1AP14_L20002B4A
C1AP14_L20001CEB:
 mov         eax,dword ptr [esp+00000104h]
C1AP14_L20001CF2:
 test        eax,eax
C1AP14_L20001CF4:
 je C1AP14_L20001D15
C1AP14_L20001CF6:
 mov         eax,dword ptr [esp+00000108h]
C1AP14_L20001CFD:
 test        eax,eax
C1AP14_L20001CFF:
 je C1AP14_L20001D15
C1AP14_L20001D01:
 fld         dword ptr [eax+8]
C1AP14_L20001D04:
 fcomp       dword ptr C1AP14_C2009A1C4
C1AP14_L20001D0A:
 fnstsw      ax
C1AP14_L20001D0C:
 test        ah,5
C1AP14_L20001D0F:
 jnp C1AP14_L20002B4A
C1AP14_L20001D15:
 fld         dword ptr [esp+000000F0h]
C1AP14_L20001D1C:
 lea         ebx,[esp+000000D0h]
C1AP14_L20001D23:
 fsub        dword ptr C1AP14_C2009A1B0
C1AP14_L20001D29:
 lea         edi,[esp+000000C4h]
C1AP14_L20001D30:
 lea         esi,[esp+000000A8h]
C1AP14_L20001D37:
 lea         edx,[esp+1Ch]
C1AP14_L20001D3B:
 fstp        dword ptr [esp+20h]
C1AP14_L20001D3F:
 mov         dword ptr [esp+60h],0C0C00000h
C1AP14_L20001D47:
 mov         dword ptr [esp+64h],0C0C00000h
C1AP14_L20001D4F:
 mov         dword ptr [esp+68h],0C0C00000h
C1AP14_L20001D57:
 mov         dword ptr [esp+54h],40C00000h
C1AP14_L20001D5F:
 mov         dword ptr [esp+58h],40C00000h
C1AP14_L20001D67:
 mov         dword ptr [esp+5Ch],40C00000h
C1AP14_L20001D6F:
 mov         dword ptr [esp+1Ch],0
C1AP14_L20001D77:
 mov         dword ptr [esp+24h],0
C1AP14_L20001D7F:
 call C1AP14_Axes
C1AP14_L20001D84:
 fld         dword ptr [esp+000000ECh]
C1AP14_L20001D8B:
 fsub        dword ptr C1AP14_C2009A2DC
C1AP14_L20001D91:
 mov         edx,dword ptr [ebp]
C1AP14_L20001D94:
 mov         eax,dword ptr [ebp+4]
C1AP14_L20001D97:
 mov         ecx,dword ptr [esp+70h]
C1AP14_L20001D9B:
 fstp        dword ptr [esp+34h]
C1AP14_L20001D9F:
 mov         dword ptr [esp+10h],edx
C1AP14_L20001DA3:
 fld         dword ptr [esp+34h]
C1AP14_L20001DA7:
 mov         edx,dword ptr [esp+000000E0h]
C1AP14_L20001DAE:
 fadd        dword ptr [ebp+8]
C1AP14_L20001DB1:
 push        ecx
C1AP14_L20001DB2:
 mov         dword ptr [esp+18h],eax
C1AP14_L20001DB6:
 push        edx
C1AP14_L20001DB7:
 fstp        dword ptr [esp+20h]
C1AP14_L20001DBB:
 lea         eax,[esp+24h]
C1AP14_L20001DBF:
 fld         dword ptr [esp+000000B0h]
C1AP14_L20001DC6:
 push        eax
C1AP14_L20001DC7:
 fmul        dword ptr C1AP14_C2009A450
C1AP14_L20001DCD:
 lea         ecx,[esp+60h]
C1AP14_L20001DD1:
 push        ecx
C1AP14_L20001DD2:
 lea         edx,[esp+70h]
C1AP14_L20001DD6:
 fadd        dword ptr [esp+20h]
C1AP14_L20001DDA:
 push        edx
C1AP14_L20001DDB:
 lea         eax,[esp+24h]
C1AP14_L20001DDF:
 push        eax
C1AP14_L20001DE0:
 fstp        dword ptr [esp+34h]
C1AP14_L20001DE4:
 lea         ecx,[esp+00000090h]
C1AP14_L20001DEB:
 fld         dword ptr [esp+000000C4h]
C1AP14_L20001DF2:
 push        ecx
C1AP14_L20001DF3:
 fmul        dword ptr C1AP14_C2009A450
C1AP14_L20001DF9:
 fadd        dword ptr [esp+30h]
C1AP14_L20001DFD:
 fstp        dword ptr [esp+3Ch]
C1AP14_L20001E01:
 fld         dword ptr [esp+000000CCh]
C1AP14_L20001E08:
 fmul        dword ptr C1AP14_C2009A450
C1AP14_L20001E0E:
 fadd        dword ptr [esp+34h]
C1AP14_L20001E12:
 fstp        dword ptr [esp+40h]
C1AP14_L20001E16:
 call        dword ptr [esp+00000128h]
C1AP14_L20001E1D:
 mov         ecx,dword ptr C1AP14_DebugDisabled
C1AP14_L20001E23:
 add         esp,1Ch
C1AP14_L20001E26:
 test        ecx,ecx
C1AP14_L20001E28:
 je C1AP14_L20001E88
C1AP14_L20001E2A:
 mov         edx,dword ptr C1AP14_DebugDisabled
C1AP14_L20001E30:
 push        1
C1AP14_L20001E32:
 push        edx
C1AP14_L20001E33:
 push 0
C1AP14_L20001E38:
 lea         eax,[esp+1Ch]
C1AP14_L20001E3C:
 push        40C00000h
C1AP14_L20001E41:
 push        eax
C1AP14_L20001E42:
 lea         eax,[esp+000000D8h]
C1AP14_L20001E49:
 int 3 /* debug drawing is not selected or exposed */
C1AP14_L20001E4E:
 mov         ecx,dword ptr C1AP14_DebugDisabled
C1AP14_L20001E54:
 add         esp,14h
C1AP14_L20001E57:
 test        ecx,ecx
C1AP14_L20001E59:
 je C1AP14_L20001E88
C1AP14_L20001E5B:
 mov         ecx,dword ptr C1AP14_DebugDisabled
C1AP14_L20001E61:
 push        1
C1AP14_L20001E63:
 push        ecx
C1AP14_L20001E64:
 push 0
C1AP14_L20001E69:
 lea         edx,[esp+1Ch]
C1AP14_L20001E6D:
 push        40C00000h
C1AP14_L20001E72:
 push        edx
C1AP14_L20001E73:
 lea         eax,[esp+000000E4h]
C1AP14_L20001E7A:
 int 3 /* debug drawing is not selected or exposed */
C1AP14_L20001E7F:
 mov         ecx,dword ptr C1AP14_DebugDisabled
C1AP14_L20001E85:
 add         esp,14h
C1AP14_L20001E88:
 fld         dword ptr [esp+78h]
C1AP14_L20001E8C:
 fcomp       dword ptr C1AP14_C2009A1B8
C1AP14_L20001E92:
 fnstsw      ax
C1AP14_L20001E94:
 test        ah,5
C1AP14_L20001E97:
 jp C1AP14_L2000209A
C1AP14_L20001E9D:
 mov         eax,dword ptr [esp+00000104h]
C1AP14_L20001EA4:
 test        eax,eax
C1AP14_L20001EA6:
 jne C1AP14_L20001EAF
C1AP14_L20001EA8:
 test        ecx,ecx
C1AP14_L20001EAA:
 jmp C1AP14_L200021FE
C1AP14_L20001EAF:
 fld         dword ptr [esp+78h]
C1AP14_L20001EB3:
 mov         dword ptr [esp+74h],1
C1AP14_L20001EBB:
 fmul        dword ptr C1AP14_C2009A450
C1AP14_L20001EC1:
 fadd        dword ptr C1AP14_C2009A2DC
C1AP14_L20001EC7:
 fstp        dword ptr [esp+50h]
C1AP14_L20001ECB:
 fld         dword ptr [esp+000000E8h]
C1AP14_L20001ED2:
 fadd        dword ptr C1AP14_C2009A2AC
C1AP14_L20001ED8:
 fcomp       dword ptr [esp+50h]
C1AP14_L20001EDC:
 fnstsw      ax
C1AP14_L20001EDE:
 test        ah,41h
C1AP14_L20001EE1:
 jne C1AP14_L20001EEA
C1AP14_L20001EE3:
 test        ecx,ecx
C1AP14_L20001EE5:
 jmp C1AP14_L200021FE
C1AP14_L20001EEA:
 fld         dword ptr [esp+34h]
C1AP14_L20001EEE:
 fmul        dword ptr C1AP14_C2009A1C4
C1AP14_L20001EF4:
 fadd        dword ptr C1AP14_C2009A23C
C1AP14_L20001EFA:
 fstp        dword ptr [esp+6Ch]
C1AP14_L20001EFE:
 fld         dword ptr [esp+50h]
C1AP14_L20001F02:
 fcomp       dword ptr [esp+6Ch]
C1AP14_L20001F06:
 fnstsw      ax
C1AP14_L20001F08:
 test        ah,5
C1AP14_L20001F0B:
 jp C1AP14_L2000206F
C1AP14_L20001F11:
 test        ecx,ecx
C1AP14_L20001F13:
 je C1AP14_L20001F3A
C1AP14_L20001F15:
 mov         eax,dword ptr C1AP14_DebugDisabled
C1AP14_L20001F1A:
 push        1
C1AP14_L20001F1C:
 push        eax
C1AP14_L20001F1D:
 push 0
C1AP14_L20001F22:
 lea         ecx,[esp+00000088h]
C1AP14_L20001F29:
 push        ecx
C1AP14_L20001F2A:
 lea         edx,[esp+20h]
C1AP14_L20001F2E:
 push        edx
C1AP14_L20001F2F:
 push        52h
C1AP14_L20001F31:
 int 3 /* debug drawing is not selected or exposed */
C1AP14_L20001F37:
 add         esp,18h
C1AP14_L20001F3A:
 fld         dword ptr [esp+24h]
C1AP14_L20001F3E:
 lea         esi,[esp+000000A8h]
C1AP14_L20001F45:
 fadd        dword ptr C1AP14_C2009A45C
C1AP14_L20001F4B:
 lea         edi,[esp+44h]
C1AP14_L20001F4F:
 mov         dword ptr [esp+74h],0
C1AP14_L20001F57:
 fstp        dword ptr [esp+24h]
C1AP14_L20001F5B:
 fld         dword ptr [esp+1Ch]
C1AP14_L20001F5F:
 fsub        dword ptr [esp+10h]
C1AP14_L20001F63:
 fstp        dword ptr [esp+44h]
C1AP14_L20001F67:
 fld         dword ptr [esp+20h]
C1AP14_L20001F6B:
 fsub        dword ptr [esp+14h]
C1AP14_L20001F6F:
 fstp        dword ptr [esp+48h]
C1AP14_L20001F73:
 fld         dword ptr [esp+24h]
C1AP14_L20001F77:
 fsub        dword ptr [esp+18h]
C1AP14_L20001F7B:
 fstp        dword ptr [esp+4Ch]
C1AP14_L20001F7F:
 call C1AP14_NormalizeTo
C1AP14_L20001F84:
 mov         edi,dword ptr [esp+70h]
C1AP14_L20001F88:
 fstp        dword ptr [esp+000000B4h]
C1AP14_L20001F8F:
 mov         eax,dword ptr [esp+000000E0h]
C1AP14_L20001F96:
 mov         ebx,dword ptr [esp+0000010Ch]
C1AP14_L20001F9D:
 push        edi
C1AP14_L20001F9E:
 push        eax
C1AP14_L20001F9F:
 lea         ecx,[esp+24h]
C1AP14_L20001FA3:
 push        ecx
C1AP14_L20001FA4:
 lea         edx,[esp+60h]
C1AP14_L20001FA8:
 push        edx
C1AP14_L20001FA9:
 lea         eax,[esp+70h]
C1AP14_L20001FAD:
 push        eax
C1AP14_L20001FAE:
 lea         ecx,[esp+24h]
C1AP14_L20001FB2:
 push        ecx
C1AP14_L20001FB3:
 lea         edx,[esp+00000090h]
C1AP14_L20001FBA:
 push        edx
C1AP14_L20001FBB:
 call        ebx
C1AP14_L20001FBD:
 fld         dword ptr [esp+00000094h]
C1AP14_L20001FC4:
 fcomp       dword ptr C1AP14_C2009A1B8
C1AP14_L20001FCA:
 add         esp,1Ch
C1AP14_L20001FCD:
 fnstsw      ax
C1AP14_L20001FCF:
 test        ah,5
C1AP14_L20001FD2:
 jp C1AP14_L20002037
C1AP14_L20001FD4:
 fld         dword ptr [esp+000000B4h]
C1AP14_L20001FDB:
 mov         dword ptr [esp+74h],1
C1AP14_L20001FE3:
 fmul        dword ptr [esp+78h]
C1AP14_L20001FE7:
 fadd        dword ptr C1AP14_C2009A2DC
C1AP14_L20001FED:
 fst         dword ptr [esp+50h]
C1AP14_L20001FF1:
 fcomp       dword ptr [esp+6Ch]
C1AP14_L20001FF5:
 fnstsw      ax
C1AP14_L20001FF7:
 test        ah,5
C1AP14_L20001FFA:
 mov         eax,dword ptr C1AP14_DebugDisabled
C1AP14_L20001FFF:
 jnp C1AP14_L200021FC
C1AP14_L20002005:
 test        eax,eax
C1AP14_L20002007:
 je C1AP14_L200020D6
C1AP14_L2000200D:
 mov         eax,dword ptr C1AP14_DebugDisabled
C1AP14_L20002012:
 push        1
C1AP14_L20002014:
 push        eax
C1AP14_L20002015:
 push 0
C1AP14_L2000201A:
 lea         ecx,[esp+00000088h]
C1AP14_L20002021:
 push        ecx
C1AP14_L20002022:
 lea         edx,[esp+20h]
C1AP14_L20002026:
 push        edx
C1AP14_L20002027:
 push        52h
C1AP14_L20002029:
 int 3 /* debug drawing is not selected or exposed */
C1AP14_L2000202F:
 add         esp,18h
C1AP14_L20002032:
 jmp C1AP14_L200020D6
C1AP14_L20002037:
 mov         eax,dword ptr C1AP14_DebugDisabled
C1AP14_L2000203C:
 test        eax,eax
C1AP14_L2000203E:
 je C1AP14_L20002065
C1AP14_L20002040:
 mov         eax,dword ptr C1AP14_DebugDisabled
C1AP14_L20002045:
 push        1
C1AP14_L20002047:
 push        eax
C1AP14_L20002048:
 push 0
C1AP14_L2000204D:
 lea         ecx,[esp+00000088h]
C1AP14_L20002054:
 push        ecx
C1AP14_L20002055:
 lea         edx,[esp+20h]
C1AP14_L20002059:
 push        edx
C1AP14_L2000205A:
 push        52h
C1AP14_L2000205C:
 int 3 /* debug drawing is not selected or exposed */
C1AP14_L20002062:
 add         esp,18h
C1AP14_L20002065:
 mov         dword ptr [esp+50h],42700000h
C1AP14_L2000206D:
 jmp C1AP14_L200020D6
C1AP14_L2000206F:
 test        ecx,ecx
C1AP14_L20002071:
 je C1AP14_L200020CB
C1AP14_L20002073:
 mov         eax,dword ptr C1AP14_DebugDisabled
C1AP14_L20002078:
 push        1
C1AP14_L2000207A:
 push        eax
C1AP14_L2000207B:
 push 0
C1AP14_L20002080:
 lea         ecx,[esp+00000088h]
C1AP14_L20002087:
 push        ecx
C1AP14_L20002088:
 lea         edx,[esp+20h]
C1AP14_L2000208C:
 push        edx
C1AP14_L2000208D:
 push        52h
C1AP14_L2000208F:
 int 3 /* debug drawing is not selected or exposed */
C1AP14_L20002095:
 add         esp,18h
C1AP14_L20002098:
 jmp C1AP14_L200020CB
C1AP14_L2000209A:
 test        ecx,ecx
C1AP14_L2000209C:
 je C1AP14_L200020C3
C1AP14_L2000209E:
 mov         eax,dword ptr C1AP14_DebugDisabled
C1AP14_L200020A3:
 push        1
C1AP14_L200020A5:
 push        eax
C1AP14_L200020A6:
 push 0
C1AP14_L200020AB:
 lea         ecx,[esp+00000088h]
C1AP14_L200020B2:
 push        ecx
C1AP14_L200020B3:
 lea         edx,[esp+20h]
C1AP14_L200020B7:
 push        edx
C1AP14_L200020B8:
 push        52h
C1AP14_L200020BA:
 int 3 /* debug drawing is not selected or exposed */
C1AP14_L200020C0:
 add         esp,18h
C1AP14_L200020C3:
 mov         dword ptr [esp+50h],42700000h
C1AP14_L200020CB:
 mov         edi,dword ptr [esp+70h]
C1AP14_L200020CF:
 mov         ebx,dword ptr [esp+0000010Ch]
C1AP14_L200020D6:
 mov         eax,dword ptr [esp+7Ch]
C1AP14_L200020DA:
 mov         ecx,dword ptr [esp+00000080h]
C1AP14_L200020E1:
 mov         edx,dword ptr [esp+00000084h]
C1AP14_L200020E8:
 mov         dword ptr [esp+28h],eax
C1AP14_L200020EC:
 mov         eax,dword ptr [esp+00000104h]
C1AP14_L200020F3:
 test        eax,eax
C1AP14_L200020F5:
 mov         dword ptr [esp+2Ch],ecx
C1AP14_L200020F9:
 mov         dword ptr [esp+30h],edx
C1AP14_L200020FD:
 jne C1AP14_L20002142
C1AP14_L200020FF:
 mov         eax,dword ptr [esp+000000F4h]
C1AP14_L20002106:
 test        eax,eax
C1AP14_L20002108:
 je C1AP14_L20002110
C1AP14_L2000210A:
 mov         dword ptr [eax],0
C1AP14_L20002110:
 mov         eax,dword ptr [esp+000000F8h]
C1AP14_L20002117:
 test        eax,eax
C1AP14_L20002119:
 je C1AP14_L20002121
C1AP14_L2000211B:
 mov         dword ptr [eax],0
C1AP14_L20002121:
 mov         eax,dword ptr [esp+000000FCh]
C1AP14_L20002128:
 test        eax,eax
C1AP14_L2000212A:
 je C1AP14_L20002132
C1AP14_L2000212C:
 mov         dword ptr [eax],0
C1AP14_L20002132:
 pop         edi
C1AP14_L20002133:
 pop         esi
C1AP14_L20002134:
 pop         ebp
C1AP14_L20002135:
 mov         eax,1
C1AP14_L2000213A:
 pop         ebx
C1AP14_L2000213B:
 add         esp,0CCh
C1AP14_L20002141:
 ret
C1AP14_L20002142:
 fld         dword ptr [esp+000000A8h]
C1AP14_L20002149:
 mov         eax,dword ptr [esp+000000E0h]
C1AP14_L20002150:
 fmul        dword ptr C1AP14_C2009A23C
C1AP14_L20002156:
 push        edi
C1AP14_L20002157:
 push        eax
C1AP14_L20002158:
 lea         ecx,[esp+24h]
C1AP14_L2000215C:
 fadd        dword ptr [ebp]
C1AP14_L2000215F:
 push        ecx
C1AP14_L20002160:
 lea         edx,[esp+60h]
C1AP14_L20002164:
 push        edx
C1AP14_L20002165:
 fst         dword ptr [esp+20h]
C1AP14_L20002169:
 lea         eax,[esp+70h]
C1AP14_L2000216D:
 fld         dword ptr [esp+000000BCh]
C1AP14_L20002174:
 push        eax
C1AP14_L20002175:
 fmul        dword ptr C1AP14_C2009A23C
C1AP14_L2000217B:
 lea         ecx,[esp+24h]
C1AP14_L2000217F:
 push        ecx
C1AP14_L20002180:
 lea         edx,[esp+00000090h]
C1AP14_L20002187:
 fadd        dword ptr [ebp+4]
C1AP14_L2000218A:
 push        edx
C1AP14_L2000218B:
 fst         dword ptr [esp+30h]
C1AP14_L2000218F:
 fld         dword ptr [esp+000000CCh]
C1AP14_L20002196:
 fmul        dword ptr C1AP14_C2009A23C
C1AP14_L2000219C:
 fadd        dword ptr [ebp+8]
C1AP14_L2000219F:
 fadd        dword ptr [esp+50h]
C1AP14_L200021A3:
 fstp        dword ptr [esp+34h]
C1AP14_L200021A7:
 fxch        st(1)
C1AP14_L200021A9:
 fstp        dword ptr [esp+38h]
C1AP14_L200021AD:
 fstp        dword ptr [esp+3Ch]
C1AP14_L200021B1:
 fld         dword ptr [esp+00000104h]
C1AP14_L200021B8:
 fmul        dword ptr C1AP14_C2009A458
C1AP14_L200021BE:
 fadd        dword ptr [esp+50h]
C1AP14_L200021C2:
 fsub        dword ptr C1AP14_C2009A2DC
C1AP14_L200021C8:
 fstp        dword ptr [esp+00000088h]
C1AP14_L200021CF:
 fld         dword ptr [esp+34h]
C1AP14_L200021D3:
 fsub        dword ptr [esp+00000088h]
C1AP14_L200021DA:
 fstp        dword ptr [esp+40h]
C1AP14_L200021DE:
 call        ebx
C1AP14_L200021E0:
 fld         dword ptr [esp+00000094h]
C1AP14_L200021E7:
 add         esp,1Ch
C1AP14_L200021EA:
 fcomp       dword ptr C1AP14_C2009A1B8
C1AP14_L200021F0:
 fnstsw      ax
C1AP14_L200021F2:
 test        ah,44h
C1AP14_L200021F5:
 jp C1AP14_L20002223
C1AP14_L200021F7:
 mov         eax,dword ptr C1AP14_DebugDisabled
C1AP14_L200021FC:
 test        eax,eax
C1AP14_L200021FE:
 je C1AP14_L20002B4A
C1AP14_L20002204:
 mov         eax,dword ptr C1AP14_DebugDisabled
C1AP14_L20002209:
 push        1
C1AP14_L2000220B:
 push        eax
C1AP14_L2000220C:
 push 0
C1AP14_L20002211:
 lea         ecx,[esp+00000088h]
C1AP14_L20002218:
 push        ecx
C1AP14_L20002219:
 lea         edx,[esp+20h]
C1AP14_L2000221D:
 push        edx
C1AP14_L2000221E:
 jmp C1AP14_L20002B3F
C1AP14_L20002223:
 mov         ecx,dword ptr C1AP14_DebugDisabled
C1AP14_L20002229:
 test        ecx,ecx
C1AP14_L2000222B:
 je C1AP14_L20002258
C1AP14_L2000222D:
 mov         eax,dword ptr C1AP14_DebugDisabled
C1AP14_L20002232:
 push        1
C1AP14_L20002234:
 push        eax
C1AP14_L20002235:
 push 0
C1AP14_L2000223A:
 lea         ecx,[esp+00000088h]
C1AP14_L20002241:
 push        ecx
C1AP14_L20002242:
 lea         edx,[esp+20h]
C1AP14_L20002246:
 push        edx
C1AP14_L20002247:
 push        52h
C1AP14_L20002249:
 int 3 /* debug drawing is not selected or exposed */
C1AP14_L2000224F:
 mov         ecx,dword ptr C1AP14_DebugDisabled
C1AP14_L20002255:
 add         esp,18h
C1AP14_L20002258:
 mov         eax,dword ptr [esp+7Ch]
C1AP14_L2000225C:
 fld         dword ptr [esp+6Ch]
C1AP14_L20002260:
 fmul        dword ptr [esp+78h]
C1AP14_L20002264:
 mov         edx,dword ptr [esp+00000080h]
C1AP14_L2000226B:
 mov         dword ptr [esp+38h],eax
C1AP14_L2000226F:
 mov         eax,dword ptr [esp+00000084h]
C1AP14_L20002276:
 fadd        dword ptr C1AP14_C2009A2DC
C1AP14_L2000227C:
 mov         dword ptr [esp+40h],eax
C1AP14_L20002280:
 mov         eax,dword ptr [esp+74h]
C1AP14_L20002284:
 test        eax,eax
C1AP14_L20002286:
 mov         dword ptr [esp+3Ch],edx
C1AP14_L2000228A:
 je C1AP14_L2000247A
C1AP14_L20002290:
 fld         dword ptr [esp+50h]
C1AP14_L20002294:
 fsub        st,st(1)
C1AP14_L20002296:
 fxch        st(1)
C1AP14_L20002298:
 fmul        dword ptr C1AP14_C2009A454
C1AP14_L2000229E:
 fcompp
C1AP14_L200022A0:
 fnstsw      ax
C1AP14_L200022A2:
 test        ah,41h
C1AP14_L200022A5:
 jne C1AP14_L200022CA
C1AP14_L200022A7:
 test        ecx,ecx
C1AP14_L200022A9:
 je C1AP14_L20002B4A
C1AP14_L200022AF:
 mov         ecx,dword ptr C1AP14_DebugDisabled
C1AP14_L200022B5:
 push        1
C1AP14_L200022B7:
 push        ecx
C1AP14_L200022B8:
 push 0
C1AP14_L200022BD:
 lea         edx,[esp+44h]
C1AP14_L200022C1:
 lea         eax,[esp+34h]
C1AP14_L200022C5:
 jmp C1AP14_L20002B3D
C1AP14_L200022CA:
 test        ecx,ecx
C1AP14_L200022CC:
 je C1AP14_L200022F1
C1AP14_L200022CE:
 mov         ecx,dword ptr C1AP14_DebugDisabled
C1AP14_L200022D4:
 push        1
C1AP14_L200022D6:
 push        ecx
C1AP14_L200022D7:
 push 0
C1AP14_L200022DC:
 lea         edx,[esp+44h]
C1AP14_L200022E0:
 push        edx
C1AP14_L200022E1:
 lea         eax,[esp+38h]
C1AP14_L200022E5:
 push        eax
C1AP14_L200022E6:
 push        52h
C1AP14_L200022E8:
 int 3 /* debug drawing is not selected or exposed */
C1AP14_L200022EE:
 add         esp,18h
C1AP14_L200022F1:
 fld         dword ptr [esp+28h]
C1AP14_L200022F5:
 lea         esi,[esp+44h]
C1AP14_L200022F9:
 fsub        dword ptr [esp+38h]
C1AP14_L200022FD:
 fld         dword ptr [esp+2Ch]
C1AP14_L20002301:
 fsub        dword ptr [esp+3Ch]
C1AP14_L20002305:
 fld         dword ptr [esp+30h]
C1AP14_L20002309:
 fsub        dword ptr [esp+40h]
C1AP14_L2000230D:
 fstp        dword ptr [esp+4Ch]
C1AP14_L20002311:
 fld         dword ptr [esp+000000A8h]
C1AP14_L20002318:
 fmul        dword ptr C1AP14_C2009A2DC
C1AP14_L2000231E:
 fadd        st,st(2)
C1AP14_L20002320:
 fstp        dword ptr [esp+44h]
C1AP14_L20002324:
 fld         dword ptr [esp+000000ACh]
C1AP14_L2000232B:
 fmul        dword ptr C1AP14_C2009A2DC
C1AP14_L20002331:
 fadd        st,st(1)
C1AP14_L20002333:
 fstp        dword ptr [esp+48h]
C1AP14_L20002337:
 fstp        st(0)
C1AP14_L20002339:
 fstp        st(0)
C1AP14_L2000233B:
 fld         dword ptr [esp+000000B0h]
C1AP14_L20002342:
 fmul        dword ptr C1AP14_C2009A2DC
C1AP14_L20002348:
 fadd        dword ptr [esp+4Ch]
C1AP14_L2000234C:
 fadd        dword ptr C1AP14_C2009A2DC
C1AP14_L20002352:
 fstp        dword ptr [esp+4Ch]
C1AP14_L20002356:
 call C1AP14_Normalize
C1AP14_L2000235B:
 fstp        st(0)
C1AP14_L2000235D:
 mov         ecx,dword ptr [esp+000000E0h]
C1AP14_L20002364:
 fld         dword ptr [esp+44h]
C1AP14_L20002368:
 push        edi
C1AP14_L20002369:
 fmul        dword ptr C1AP14_C2009A370
C1AP14_L2000236F:
 push        ecx
C1AP14_L20002370:
 lea         edx,[esp+24h]
C1AP14_L20002374:
 push        edx
C1AP14_L20002375:
 fadd        dword ptr [esp+1Ch]
C1AP14_L20002379:
 lea         eax,[esp+60h]
C1AP14_L2000237D:
 fld         dword ptr [esp+54h]
C1AP14_L20002381:
 push        eax
C1AP14_L20002382:
 fmul        dword ptr C1AP14_C2009A370
C1AP14_L20002388:
 lea         ecx,[esp+70h]
C1AP14_L2000238C:
 push        ecx
C1AP14_L2000238D:
 lea         edx,[esp+24h]
C1AP14_L20002391:
 fadd        dword ptr [esp+28h]
C1AP14_L20002395:
 push        edx
C1AP14_L20002396:
 fld         dword ptr [esp+64h]
C1AP14_L2000239A:
 lea         eax,[esp+00000090h]
C1AP14_L200023A1:
 fmul        dword ptr C1AP14_C2009A370
C1AP14_L200023A7:
 push        eax
C1AP14_L200023A8:
 fadd        dword ptr [esp+34h]
C1AP14_L200023AC:
 fstp        dword ptr [esp+40h]
C1AP14_L200023B0:
 fld         dword ptr [esp+000000C4h]
C1AP14_L200023B7:
 fmul        dword ptr C1AP14_C2009A450
C1AP14_L200023BD:
 fadd        dword ptr [ebp]
C1AP14_L200023C0:
 fadd        st,st(2)
C1AP14_L200023C2:
 fmul        dword ptr C1AP14_C2009A1B4
C1AP14_L200023C8:
 fstp        dword ptr [esp+38h]
C1AP14_L200023CC:
 fld         dword ptr [esp+000000C8h]
C1AP14_L200023D3:
 fmul        dword ptr C1AP14_C2009A450
C1AP14_L200023D9:
 fadd        dword ptr [ebp+4]
C1AP14_L200023DC:
 fadd        st,st(1)
C1AP14_L200023DE:
 fmul        dword ptr C1AP14_C2009A1B4
C1AP14_L200023E4:
 fstp        dword ptr [esp+3Ch]
C1AP14_L200023E8:
 fstp        st(0)
C1AP14_L200023EA:
 fstp        st(0)
C1AP14_L200023EC:
 call        ebx
C1AP14_L200023EE:
 fld         dword ptr [esp+00000094h]
C1AP14_L200023F5:
 add         esp,1Ch
C1AP14_L200023F8:
 fcomp       dword ptr C1AP14_C2009A1B8
C1AP14_L200023FE:
 fnstsw      ax
C1AP14_L20002400:
 test        ah,5
C1AP14_L20002403:
 mov         eax,dword ptr C1AP14_DebugDisabled
C1AP14_L20002408:
 jp C1AP14_L20002430
C1AP14_L2000240A:
 test        eax,eax
C1AP14_L2000240C:
 je C1AP14_L20002B4A
C1AP14_L20002412:
 mov         ecx,dword ptr C1AP14_DebugDisabled
C1AP14_L20002418:
 push        1
C1AP14_L2000241A:
 push        ecx
C1AP14_L2000241B:
 push 0
C1AP14_L20002420:
 lea         edx,[esp+00000088h]
C1AP14_L20002427:
 lea         eax,[esp+1Ch]
C1AP14_L2000242B:
 jmp C1AP14_L20002B3D
C1AP14_L20002430:
 test        eax,eax
C1AP14_L20002432:
 je C1AP14_L2000245A
C1AP14_L20002434:
 mov         ecx,dword ptr C1AP14_DebugDisabled
C1AP14_L2000243A:
 push        1
C1AP14_L2000243C:
 push        ecx
C1AP14_L2000243D:
 push 0
C1AP14_L20002442:
 lea         edx,[esp+00000088h]
C1AP14_L20002449:
 push        edx
C1AP14_L2000244A:
 lea         eax,[esp+20h]
C1AP14_L2000244E:
 push        eax
C1AP14_L2000244F:
 push        52h
C1AP14_L20002451:
 int 3 /* debug drawing is not selected or exposed */
C1AP14_L20002457:
 add         esp,18h
C1AP14_L2000245A:
 mov         ecx,dword ptr [esp+7Ch]
C1AP14_L2000245E:
 mov         edx,dword ptr [esp+00000080h]
C1AP14_L20002465:
 mov         eax,dword ptr [esp+00000084h]
C1AP14_L2000246C:
 mov         dword ptr [esp+28h],ecx
C1AP14_L20002470:
 mov         dword ptr [esp+2Ch],edx
C1AP14_L20002474:
 mov         dword ptr [esp+30h],eax
C1AP14_L20002478:
 jmp C1AP14_L2000247C
C1AP14_L2000247A:
 fstp        st(0)
C1AP14_L2000247C:
 fld         dword ptr [esp+30h]
C1AP14_L20002480:
 mov         eax,dword ptr [esp+30h]
C1AP14_L20002484:
 fsub        dword ptr [esp+40h]
C1AP14_L20002488:
 mov         ecx,dword ptr [esp+28h]
C1AP14_L2000248C:
 mov         edx,dword ptr [esp+2Ch]
C1AP14_L20002490:
 mov         esi,dword ptr [esp+000000E0h]
C1AP14_L20002497:
 push        edi
C1AP14_L20002498:
 push        esi
C1AP14_L20002499:
 mov         dword ptr [esp+20h],eax
C1AP14_L2000249D:
 fadd        st(0),st
C1AP14_L2000249F:
 mov         dword ptr [esp+18h],ecx
C1AP14_L200024A3:
 mov         dword ptr [esp+24h],ecx
C1AP14_L200024A7:
 fadd        dword ptr [esp+000000F0h]
C1AP14_L200024AE:
 lea         eax,[esp+24h]
C1AP14_L200024B2:
 push        eax
C1AP14_L200024B3:
 lea         ecx,[esp+60h]
C1AP14_L200024B7:
 push        ecx
C1AP14_L200024B8:
 mov         dword ptr [esp+24h],edx
C1AP14_L200024BC:
 mov         dword ptr [esp+30h],edx
C1AP14_L200024C0:
 lea         edx,[esp+70h]
C1AP14_L200024C4:
 push        edx
C1AP14_L200024C5:
 lea         eax,[esp+24h]
C1AP14_L200024C9:
 fsubr       dword ptr [esp+44h]
C1AP14_L200024CD:
 push        eax
C1AP14_L200024CE:
 lea         ecx,[esp+00000090h]
C1AP14_L200024D5:
 push        ecx
C1AP14_L200024D6:
 fstp        dword ptr [esp+40h]
C1AP14_L200024DA:
 call        ebx
C1AP14_L200024DC:
 fld         dword ptr [esp+00000094h]
C1AP14_L200024E3:
 fcomp       dword ptr C1AP14_C2009A1B8
C1AP14_L200024E9:
 add         esp,1Ch
C1AP14_L200024EC:
 fnstsw      ax
C1AP14_L200024EE:
 test        ah,44h
C1AP14_L200024F1:
 mov         eax,dword ptr C1AP14_DebugDisabled
C1AP14_L200024F6:
 jnp C1AP14_L200025B9
C1AP14_L200024FC:
 test        eax,eax
C1AP14_L200024FE:
 je C1AP14_L20002526
C1AP14_L20002500:
 mov         edx,dword ptr C1AP14_DebugDisabled
C1AP14_L20002506:
 push        1
C1AP14_L20002508:
 push        edx
C1AP14_L20002509:
 push 0
C1AP14_L2000250E:
 lea         eax,[esp+00000088h]
C1AP14_L20002515:
 push        eax
C1AP14_L20002516:
 lea         ecx,[esp+20h]
C1AP14_L2000251A:
 push        ecx
C1AP14_L2000251B:
 push        52h
C1AP14_L2000251D:
 int 3 /* debug drawing is not selected or exposed */
C1AP14_L20002523:
 add         esp,18h
C1AP14_L20002526:
 mov         edx,dword ptr [esp+7Ch]
C1AP14_L2000252A:
 fld         dword ptr [esp+34h]
C1AP14_L2000252E:
 fadd        dword ptr [ebp+8]
C1AP14_L20002531:
 mov         eax,dword ptr [esp+00000080h]
C1AP14_L20002538:
 mov         ecx,dword ptr [esp+00000084h]
C1AP14_L2000253F:
 mov         dword ptr [esp+28h],edx
C1AP14_L20002543:
 mov         edx,dword ptr [ebp]
C1AP14_L20002546:
 fstp        dword ptr [esp+18h]
C1AP14_L2000254A:
 fld         dword ptr [esp+000000E8h]
C1AP14_L20002551:
 mov         dword ptr [esp+2Ch],eax
C1AP14_L20002555:
 mov         eax,dword ptr [ebp+4]
C1AP14_L20002558:
 fmul        dword ptr C1AP14_C2009A2D0
C1AP14_L2000255E:
 push        edi
C1AP14_L2000255F:
 mov         dword ptr [esp+34h],ecx
C1AP14_L20002563:
 mov         ecx,edx
C1AP14_L20002565:
 mov         dword ptr [esp+14h],edx
C1AP14_L20002569:
 mov         edx,eax
C1AP14_L2000256B:
 push        esi
C1AP14_L2000256C:
 mov         dword ptr [esp+1Ch],eax
C1AP14_L20002570:
 mov         dword ptr [esp+24h],ecx
C1AP14_L20002574:
 lea         eax,[esp+24h]
C1AP14_L20002578:
 push        eax
C1AP14_L20002579:
 lea         ecx,[esp+60h]
C1AP14_L2000257D:
 push        ecx
C1AP14_L2000257E:
 mov         dword ptr [esp+30h],edx
C1AP14_L20002582:
 lea         edx,[esp+70h]
C1AP14_L20002586:
 push        edx
C1AP14_L20002587:
 lea         eax,[esp+24h]
C1AP14_L2000258B:
 fsubr       dword ptr [ebp+8]
C1AP14_L2000258E:
 push        eax
C1AP14_L2000258F:
 lea         ecx,[esp+00000090h]
C1AP14_L20002596:
 push        ecx
C1AP14_L20002597:
 fstp        dword ptr [esp+40h]
C1AP14_L2000259B:
 call        ebx
C1AP14_L2000259D:
 fld         dword ptr [esp+00000094h]
C1AP14_L200025A4:
 fcomp       dword ptr C1AP14_C2009A1B8
C1AP14_L200025AA:
 add         esp,1Ch
C1AP14_L200025AD:
 fnstsw      ax
C1AP14_L200025AF:
 test        ah,44h
C1AP14_L200025B2:
 mov         eax,dword ptr C1AP14_DebugDisabled
C1AP14_L200025B7:
 jp C1AP14_L200025E1
C1AP14_L200025B9:
 test        eax,eax
C1AP14_L200025BB:
 je C1AP14_L20002B4A
C1AP14_L200025C1:
 mov         edx,dword ptr C1AP14_DebugDisabled
C1AP14_L200025C7:
 push        1
C1AP14_L200025C9:
 push        edx
C1AP14_L200025CA:
 push 0
C1AP14_L200025CF:
 lea         eax,[esp+00000088h]
C1AP14_L200025D6:
 push        eax
C1AP14_L200025D7:
 lea         ecx,[esp+20h]
C1AP14_L200025DB:
 push        ecx
C1AP14_L200025DC:
 jmp C1AP14_L20002B3F
C1AP14_L200025E1:
 test        eax,eax
C1AP14_L200025E3:
 je C1AP14_L2000260B
C1AP14_L200025E5:
 mov         edx,dword ptr C1AP14_DebugDisabled
C1AP14_L200025EB:
 push        1
C1AP14_L200025ED:
 push        edx
C1AP14_L200025EE:
 push 0
C1AP14_L200025F3:
 lea         eax,[esp+00000088h]
C1AP14_L200025FA:
 push        eax
C1AP14_L200025FB:
 lea         ecx,[esp+20h]
C1AP14_L200025FF:
 push        ecx
C1AP14_L20002600:
 push        52h
C1AP14_L20002602:
 int 3 /* debug drawing is not selected or exposed */
C1AP14_L20002608:
 add         esp,18h
C1AP14_L2000260B:
 fld         dword ptr [esp+38h]
C1AP14_L2000260F:
 mov         edx,dword ptr [esp+7Ch]
C1AP14_L20002613:
 fsub        dword ptr [esp+7Ch]
C1AP14_L20002617:
 mov         eax,dword ptr [esp+00000080h]
C1AP14_L2000261E:
 mov         ecx,dword ptr [esp+00000084h]
C1AP14_L20002625:
 lea         esi,[esp+44h]
C1AP14_L20002629:
 fstp        dword ptr [esp+44h]
C1AP14_L2000262D:
 mov         dword ptr [esp+000000B8h],edx
C1AP14_L20002634:
 fld         dword ptr [esp+3Ch]
C1AP14_L20002638:
 mov         dword ptr [esp+000000BCh],eax
C1AP14_L2000263F:
 fsub        dword ptr [esp+00000080h]
C1AP14_L20002646:
 mov         dword ptr [esp+000000C0h],ecx
C1AP14_L2000264D:
 fstp        dword ptr [esp+48h]
C1AP14_L20002651:
 fld         dword ptr [esp+40h]
C1AP14_L20002655:
 fsub        dword ptr [esp+00000084h]
C1AP14_L2000265C:
 fstp        dword ptr [esp+4Ch]
C1AP14_L20002660:
 call C1AP14_Pitch
C1AP14_L20002665:
 fstp        dword ptr [esp+000000B4h]
C1AP14_L2000266C:
 lea         esi,[esp+44h]
C1AP14_L20002670:
 fld         dword ptr [esp+28h]
C1AP14_L20002674:
 mov         edx,dword ptr [esp+000000B4h]
C1AP14_L2000267B:
 fsub        dword ptr [esp+38h]
C1AP14_L2000267F:
 push        edx
C1AP14_L20002680:
 fstp        dword ptr [esp+48h]
C1AP14_L20002684:
 fld         dword ptr [esp+30h]
C1AP14_L20002688:
 fsub        dword ptr [esp+40h]
C1AP14_L2000268C:
 fstp        dword ptr [esp+4Ch]
C1AP14_L20002690:
 fld         dword ptr [esp+34h]
C1AP14_L20002694:
 fsub        dword ptr [esp+44h]
C1AP14_L20002698:
 fstp        dword ptr [esp+50h]
C1AP14_L2000269C:
 call C1AP14_Pitch
C1AP14_L200026A1:
 push        ecx
C1AP14_L200026A2:
 fstp        dword ptr [esp]
C1AP14_L200026A5:
 call C1AP14_AngleDelta
C1AP14_L200026AA:
 fcom        dword ptr C1AP14_C2009A3F0
C1AP14_L200026B0:
 add         esp,8
C1AP14_L200026B3:
 fnstsw      ax
C1AP14_L200026B5:
 test        ah,5
C1AP14_L200026B8:
 jnp C1AP14_L20002AED
C1AP14_L200026BE:
 fcomp       dword ptr C1AP14_C2009A44C
C1AP14_L200026C4:
 fnstsw      ax
C1AP14_L200026C6:
 test        ah,41h
C1AP14_L200026C9:
 je C1AP14_L20002AEF
C1AP14_L200026CF:
 mov         ecx,dword ptr [esp+00000080h]
C1AP14_L200026D6:
 fld         dword ptr [esp+00000084h]
C1AP14_L200026DD:
 mov         eax,dword ptr [esp+7Ch]
C1AP14_L200026E1:
 fadd        dword ptr C1AP14_C2009A448
C1AP14_L200026E7:
 mov         edx,dword ptr [esp+38h]
C1AP14_L200026EB:
 mov         esi,dword ptr [esp+000000E0h]
C1AP14_L200026F2:
 push        edi
C1AP14_L200026F3:
 fstp        dword ptr [esp+1Ch]
C1AP14_L200026F7:
 fld         dword ptr [esp+44h]
C1AP14_L200026FB:
 push        esi
C1AP14_L200026FC:
 fadd        dword ptr C1AP14_C2009A448
C1AP14_L20002702:
 mov         dword ptr [esp+1Ch],ecx
C1AP14_L20002706:
 mov         dword ptr [esp+18h],eax
C1AP14_L2000270A:
 mov         eax,dword ptr [esp+44h]
C1AP14_L2000270E:
 mov         dword ptr [esp+24h],edx
C1AP14_L20002712:
 fstp        dword ptr [esp+2Ch]
C1AP14_L20002716:
 lea         ecx,[esp+24h]
C1AP14_L2000271A:
 push        ecx
C1AP14_L2000271B:
 lea         edx,[esp+60h]
C1AP14_L2000271F:
 push        edx
C1AP14_L20002720:
 mov         dword ptr [esp+30h],eax
C1AP14_L20002724:
 lea         eax,[esp+70h]
C1AP14_L20002728:
 push        eax
C1AP14_L20002729:
 lea         ecx,[esp+24h]
C1AP14_L2000272D:
 push        ecx
C1AP14_L2000272E:
 lea         edx,[esp+00000090h]
C1AP14_L20002735:
 push        edx
C1AP14_L20002736:
 mov         dword ptr [esp+7Ch],0
C1AP14_L2000273E:
 mov         dword ptr [esp+00000080h],0
C1AP14_L20002749:
 mov         dword ptr [esp+00000084h],0
C1AP14_L20002754:
 mov         dword ptr [esp+70h],0
C1AP14_L2000275C:
 mov         dword ptr [esp+74h],0
C1AP14_L20002764:
 mov         dword ptr [esp+78h],0
C1AP14_L2000276C:
 call        ebx
C1AP14_L2000276E:
 fld         dword ptr [esp+00000094h]
C1AP14_L20002775:
 fcomp       dword ptr C1AP14_C2009A1B8
C1AP14_L2000277B:
 add         esp,1Ch
C1AP14_L2000277E:
 fnstsw      ax
C1AP14_L20002780:
 test        ah,5
C1AP14_L20002783:
 mov         eax,dword ptr C1AP14_DebugDisabled
C1AP14_L20002788:
 jp C1AP14_L200027AE
C1AP14_L2000278A:
 test        eax,eax
C1AP14_L2000278C:
 je C1AP14_L20002B4A
C1AP14_L20002792:
 mov         eax,dword ptr C1AP14_DebugDisabled
C1AP14_L20002797:
 push        1
C1AP14_L20002799:
 push        eax
C1AP14_L2000279A:
 push 0
C1AP14_L2000279F:
 lea         ecx,[esp+28h]
C1AP14_L200027A3:
 push        ecx
C1AP14_L200027A4:
 lea         edx,[esp+20h]
C1AP14_L200027A8:
 push        edx
C1AP14_L200027A9:
 jmp C1AP14_L20002B3F
C1AP14_L200027AE:
 test        eax,eax
C1AP14_L200027B0:
 je C1AP14_L200027D4
C1AP14_L200027B2:
 mov         eax,dword ptr C1AP14_DebugDisabled
C1AP14_L200027B7:
 push        1
C1AP14_L200027B9:
 push        eax
C1AP14_L200027BA:
 push 0
C1AP14_L200027BF:
 lea         ecx,[esp+28h]
C1AP14_L200027C3:
 push        ecx
C1AP14_L200027C4:
 lea         edx,[esp+20h]
C1AP14_L200027C8:
 push        edx
C1AP14_L200027C9:
 push        52h
C1AP14_L200027CB:
 int 3 /* debug drawing is not selected or exposed */
C1AP14_L200027D1:
 add         esp,18h
C1AP14_L200027D4:
 mov         eax,dword ptr [esp+1Ch]
C1AP14_L200027D8:
 fld         dword ptr [esp+30h]
C1AP14_L200027DC:
 mov         edx,dword ptr [esp+24h]
C1AP14_L200027E0:
 fadd        dword ptr C1AP14_C2009A448
C1AP14_L200027E6:
 mov         ecx,dword ptr [esp+20h]
C1AP14_L200027EA:
 push        edi
C1AP14_L200027EB:
 mov         dword ptr [esp+14h],eax
C1AP14_L200027EF:
 fstp        dword ptr [esp+28h]
C1AP14_L200027F3:
 mov         eax,dword ptr [esp+2Ch]
C1AP14_L200027F7:
 push        esi
C1AP14_L200027F8:
 mov         dword ptr [esp+20h],edx
C1AP14_L200027FC:
 mov         dword ptr [esp+1Ch],ecx
C1AP14_L20002800:
 mov         ecx,dword ptr [esp+34h]
C1AP14_L20002804:
 mov         dword ptr [esp+24h],eax
C1AP14_L20002808:
 lea         edx,[esp+24h]
C1AP14_L2000280C:
 push        edx
C1AP14_L2000280D:
 lea         eax,[esp+60h]
C1AP14_L20002811:
 push        eax
C1AP14_L20002812:
 mov         dword ptr [esp+30h],ecx
C1AP14_L20002816:
 lea         ecx,[esp+70h]
C1AP14_L2000281A:
 push        ecx
C1AP14_L2000281B:
 lea         edx,[esp+24h]
C1AP14_L2000281F:
 push        edx
C1AP14_L20002820:
 lea         eax,[esp+00000090h]
C1AP14_L20002827:
 push        eax
C1AP14_L20002828:
 call        ebx
C1AP14_L2000282A:
 fld         dword ptr [esp+00000094h]
C1AP14_L20002831:
 fcomp       dword ptr C1AP14_C2009A1B8
C1AP14_L20002837:
 add         esp,1Ch
C1AP14_L2000283A:
 fnstsw      ax
C1AP14_L2000283C:
 test        ah,5
C1AP14_L2000283F:
 mov         eax,dword ptr C1AP14_DebugDisabled
C1AP14_L20002844:
 jp C1AP14_L20002869
C1AP14_L20002846:
 test        eax,eax
C1AP14_L20002848:
 je C1AP14_L20002B4A
C1AP14_L2000284E:
 mov         ecx,dword ptr C1AP14_DebugDisabled
C1AP14_L20002854:
 push        1
C1AP14_L20002856:
 push        ecx
C1AP14_L20002857:
 push 0
C1AP14_L2000285C:
 lea         edx,[esp+28h]
C1AP14_L20002860:
 lea         eax,[esp+1Ch]
C1AP14_L20002864:
 jmp C1AP14_L20002B3D
C1AP14_L20002869:
 test        eax,eax
C1AP14_L2000286B:
 je C1AP14_L20002A2B
C1AP14_L20002871:
 mov         ecx,dword ptr C1AP14_DebugDisabled
C1AP14_L20002877:
 push        1
C1AP14_L20002879:
 push        ecx
C1AP14_L2000287A:
 push 0
C1AP14_L2000287F:
 lea         edx,[esp+28h]
C1AP14_L20002883:
 push        edx
C1AP14_L20002884:
 lea         eax,[esp+20h]
C1AP14_L20002888:
 push        eax
C1AP14_L20002889:
 push        52h
C1AP14_L2000288B:
 int 3 /* debug drawing is not selected or exposed */
C1AP14_L20002891:
 mov         eax,dword ptr C1AP14_DebugDisabled
C1AP14_L20002896:
 add         esp,18h
C1AP14_L20002899:
 test        eax,eax
C1AP14_L2000289B:
 je C1AP14_L20002A2B
C1AP14_L200028A1:
 mov         ecx,dword ptr C1AP14_DebugDisabled
C1AP14_L200028A7:
 push        1
C1AP14_L200028A9:
 push        ecx
C1AP14_L200028AA:
 push 0
C1AP14_L200028AF:
 lea         edx,[esp+000000C4h]
C1AP14_L200028B6:
 push        40C00000h
C1AP14_L200028BB:
 push        edx
C1AP14_L200028BC:
 lea         eax,[esp+000000D8h]
C1AP14_L200028C3:
 int 3 /* debug drawing is not selected or exposed */
C1AP14_L200028C8:
 mov         eax,dword ptr C1AP14_DebugDisabled
C1AP14_L200028CD:
 add         esp,14h
C1AP14_L200028D0:
 test        eax,eax
C1AP14_L200028D2:
 je C1AP14_L20002A2B
C1AP14_L200028D8:
 mov         eax,dword ptr C1AP14_DebugDisabled
C1AP14_L200028DD:
 push        1
C1AP14_L200028DF:
 push        eax
C1AP14_L200028E0:
 push 0
C1AP14_L200028E5:
 lea         ecx,[esp+000000C4h]
C1AP14_L200028EC:
 push        40C00000h
C1AP14_L200028F1:
 push        ecx
C1AP14_L200028F2:
 lea         eax,[esp+000000E4h]
C1AP14_L200028F9:
 int 3 /* debug drawing is not selected or exposed */
C1AP14_L200028FE:
 mov         eax,dword ptr C1AP14_DebugDisabled
C1AP14_L20002903:
 add         esp,14h
C1AP14_L20002906:
 test        eax,eax
C1AP14_L20002908:
 je C1AP14_L20002A2B
C1AP14_L2000290E:
 mov         edx,dword ptr C1AP14_DebugDisabled
C1AP14_L20002914:
 push        1
C1AP14_L20002916:
 push        edx
C1AP14_L20002917:
 push 0
C1AP14_L2000291C:
 lea         eax,[esp+44h]
C1AP14_L20002920:
 push        40C00000h
C1AP14_L20002925:
 push        eax
C1AP14_L20002926:
 lea         eax,[esp+000000D8h]
C1AP14_L2000292D:
 int 3 /* debug drawing is not selected or exposed */
C1AP14_L20002932:
 mov         eax,dword ptr C1AP14_DebugDisabled
C1AP14_L20002937:
 add         esp,14h
C1AP14_L2000293A:
 test        eax,eax
C1AP14_L2000293C:
 je C1AP14_L20002A2B
C1AP14_L20002942:
 mov         ecx,dword ptr C1AP14_DebugDisabled
C1AP14_L20002948:
 push        1
C1AP14_L2000294A:
 push        ecx
C1AP14_L2000294B:
 push 0
C1AP14_L20002950:
 lea         edx,[esp+44h]
C1AP14_L20002954:
 push        40C00000h
C1AP14_L20002959:
 push        edx
C1AP14_L2000295A:
 lea         eax,[esp+000000E4h]
C1AP14_L20002961:
 int 3 /* debug drawing is not selected or exposed */
C1AP14_L20002966:
 mov         eax,dword ptr C1AP14_DebugDisabled
C1AP14_L2000296B:
 add         esp,14h
C1AP14_L2000296E:
 test        eax,eax
C1AP14_L20002970:
 je C1AP14_L20002A2B
C1AP14_L20002976:
 mov         eax,dword ptr C1AP14_DebugDisabled
C1AP14_L2000297B:
 push        1
C1AP14_L2000297D:
 push        eax
C1AP14_L2000297E:
 push 0
C1AP14_L20002983:
 lea         ecx,[esp+34h]
C1AP14_L20002987:
 push        40C00000h
C1AP14_L2000298C:
 push        ecx
C1AP14_L2000298D:
 lea         eax,[esp+000000D8h]
C1AP14_L20002994:
 int 3 /* debug drawing is not selected or exposed */
C1AP14_L20002999:
 mov         eax,dword ptr C1AP14_DebugDisabled
C1AP14_L2000299E:
 add         esp,14h
C1AP14_L200029A1:
 test        eax,eax
C1AP14_L200029A3:
 je C1AP14_L20002A2B
C1AP14_L200029A9:
 mov         edx,dword ptr C1AP14_DebugDisabled
C1AP14_L200029AF:
 push        1
C1AP14_L200029B1:
 push        edx
C1AP14_L200029B2:
 push 0
C1AP14_L200029B7:
 lea         eax,[esp+34h]
C1AP14_L200029BB:
 push        40C00000h
C1AP14_L200029C0:
 push        eax
C1AP14_L200029C1:
 lea         eax,[esp+000000E4h]
C1AP14_L200029C8:
 int 3 /* debug drawing is not selected or exposed */
C1AP14_L200029CD:
 mov         eax,dword ptr C1AP14_DebugDisabled
C1AP14_L200029D2:
 add         esp,14h
C1AP14_L200029D5:
 test        eax,eax
C1AP14_L200029D7:
 je C1AP14_L20002A2B
C1AP14_L200029D9:
 mov         ecx,dword ptr C1AP14_DebugDisabled
C1AP14_L200029DF:
 push        1
C1AP14_L200029E1:
 push        ecx
C1AP14_L200029E2:
 push 0
C1AP14_L200029E7:
 lea         edx,[esp+44h]
C1AP14_L200029EB:
 push        edx
C1AP14_L200029EC:
 lea         eax,[esp+000000C8h]
C1AP14_L200029F3:
 push        eax
C1AP14_L200029F4:
 push        52h
C1AP14_L200029F6:
 int 3 /* debug drawing is not selected or exposed */
C1AP14_L200029FC:
 mov         eax,dword ptr C1AP14_DebugDisabled
C1AP14_L20002A01:
 add         esp,18h
C1AP14_L20002A04:
 test        eax,eax
C1AP14_L20002A06:
 je C1AP14_L20002A2B
C1AP14_L20002A08:
 mov         ecx,dword ptr C1AP14_DebugDisabled
C1AP14_L20002A0E:
 push        1
C1AP14_L20002A10:
 push        ecx
C1AP14_L20002A11:
 push 0
C1AP14_L20002A16:
 lea         edx,[esp+34h]
C1AP14_L20002A1A:
 push        edx
C1AP14_L20002A1B:
 lea         eax,[esp+48h]
C1AP14_L20002A1F:
 push        eax
C1AP14_L20002A20:
 push        52h
C1AP14_L20002A22:
 int 3 /* debug drawing is not selected or exposed */
C1AP14_L20002A28:
 add         esp,18h
C1AP14_L20002A2B:
 mov         eax,dword ptr [esp+000000F4h]
C1AP14_L20002A32:
 test        eax,eax
C1AP14_L20002A34:
 je C1AP14_L20002A48
C1AP14_L20002A36:
 fld         dword ptr [esp+000000C0h]
C1AP14_L20002A3D:
 fsub        dword ptr [ebp+8]
C1AP14_L20002A40:
 fsub        dword ptr C1AP14_C2009A2DC
C1AP14_L20002A46:
 fstp        dword ptr [eax]
C1AP14_L20002A48:
 mov         edi,dword ptr [esp+000000F8h]
C1AP14_L20002A4F:
 test        edi,edi
C1AP14_L20002A51:
 je C1AP14_L20002A97
C1AP14_L20002A53:
 fld         dword ptr [esp+000000B8h]
C1AP14_L20002A5A:
 lea         esi,[esp+44h]
C1AP14_L20002A5E:
 fsub        dword ptr [esp+38h]
C1AP14_L20002A62:
 fstp        dword ptr [esp+44h]
C1AP14_L20002A66:
 fld         dword ptr [esp+000000BCh]
C1AP14_L20002A6D:
 fsub        dword ptr [esp+3Ch]
C1AP14_L20002A71:
 fstp        dword ptr [esp+48h]
C1AP14_L20002A75:
 fld         dword ptr [esp+000000C0h]
C1AP14_L20002A7C:
 fsub        dword ptr [esp+40h]
C1AP14_L20002A80:
 fstp        dword ptr [esp+4Ch]
C1AP14_L20002A84:
 call C1AP14_Pitch
C1AP14_L20002A89:
 push        ecx
C1AP14_L20002A8A:
 fstp        dword ptr [esp]
C1AP14_L20002A8D:
 call C1AP14_AngleNormalize
C1AP14_L20002A92:
 fstp        dword ptr [edi]
C1AP14_L20002A94:
 add         esp,4
C1AP14_L20002A97:
 mov         edi,dword ptr [esp+000000FCh]
C1AP14_L20002A9E:
 test        edi,edi
C1AP14_L20002AA0:
 je C1AP14_L20002ADD
C1AP14_L20002AA2:
 fld         dword ptr [esp+38h]
C1AP14_L20002AA6:
 lea         esi,[esp+44h]
C1AP14_L20002AAA:
 fsub        dword ptr [esp+28h]
C1AP14_L20002AAE:
 fstp        dword ptr [esp+44h]
C1AP14_L20002AB2:
 fld         dword ptr [esp+3Ch]
C1AP14_L20002AB6:
 fsub        dword ptr [esp+2Ch]
C1AP14_L20002ABA:
 fstp        dword ptr [esp+48h]
C1AP14_L20002ABE:
 fld         dword ptr [esp+40h]
C1AP14_L20002AC2:
 fsub        dword ptr [esp+30h]
C1AP14_L20002AC6:
 fstp        dword ptr [esp+4Ch]
C1AP14_L20002ACA:
 call C1AP14_Pitch
C1AP14_L20002ACF:
 push        ecx
C1AP14_L20002AD0:
 fstp        dword ptr [esp]
C1AP14_L20002AD3:
 call C1AP14_AngleNormalize
C1AP14_L20002AD8:
 fstp        dword ptr [edi]
C1AP14_L20002ADA:
 add         esp,4
C1AP14_L20002ADD:
 pop         edi
C1AP14_L20002ADE:
 pop         esi
C1AP14_L20002ADF:
 pop         ebp
C1AP14_L20002AE0:
 mov         eax,1
C1AP14_L20002AE5:
 pop         ebx
C1AP14_L20002AE6:
 add         esp,0CCh
C1AP14_L20002AEC:
 ret
C1AP14_L20002AED:
 fstp        st(0)
C1AP14_L20002AEF:
 mov         eax,dword ptr C1AP14_DebugDisabled
C1AP14_L20002AF4:
 test        eax,eax
C1AP14_L20002AF6:
 je C1AP14_L20002B4A
C1AP14_L20002AF8:
 mov         ecx,dword ptr C1AP14_DebugDisabled
C1AP14_L20002AFE:
 push        1
C1AP14_L20002B00:
 push        ecx
C1AP14_L20002B01:
 push 0
C1AP14_L20002B06:
 lea         edx,[esp+44h]
C1AP14_L20002B0A:
 push        edx
C1AP14_L20002B0B:
 lea         eax,[esp+000000C8h]
C1AP14_L20002B12:
 push        eax
C1AP14_L20002B13:
 push        52h
C1AP14_L20002B15:
 int 3 /* debug drawing is not selected or exposed */
C1AP14_L20002B1B:
 mov         eax,dword ptr C1AP14_DebugDisabled
C1AP14_L20002B20:
 add         esp,18h
C1AP14_L20002B23:
 test        eax,eax
C1AP14_L20002B25:
 je C1AP14_L20002B4A
C1AP14_L20002B27:
 mov         ecx,dword ptr C1AP14_DebugDisabled
C1AP14_L20002B2D:
 push        1
C1AP14_L20002B2F:
 push        ecx
C1AP14_L20002B30:
 push 0
C1AP14_L20002B35:
 lea         edx,[esp+34h]
C1AP14_L20002B39:
 lea         eax,[esp+44h]
C1AP14_L20002B3D:
 push        edx
C1AP14_L20002B3E:
 push        eax
C1AP14_L20002B3F:
 push        52h
C1AP14_L20002B41:
 int 3 /* debug drawing is not selected or exposed */
C1AP14_L20002B47:
 add         esp,18h
C1AP14_L20002B4A:
 pop         edi
C1AP14_L20002B4B:
 pop         esi
C1AP14_L20002B4C:
 pop         ebp
C1AP14_L20002B4D:
 xor         eax,eax
C1AP14_L20002B4F:
 pop         ebx
C1AP14_L20002B50:
 add         esp,0CCh
C1AP14_L20002B56:
 ret
}}
static __declspec(noinline) int C1AP14_Query(int slot,const float origin[3],float radius,float height,float yaw,c1ap14TraceFn capsule,c1ap14TraceFn box,c1ap14Pose_t *out){
 uint32_t raw[6];float o[3];c1ap14Pose_t stage;unsigned char saved[108];unsigned short cw=0x027f;uint32_t mx;int i,fit;
 if(slot<0||slot>=2048||!origin||!capsule||!box||!out)return 0;
 memcpy(raw,origin,12);memcpy(raw+3,&radius,4);memcpy(raw+4,&height,4);memcpy(raw+5,&yaw,4);
 for(i=0;i<6;i++)if((raw[i]&0x7f800000u)==0x7f800000u)return 0;
 if((raw[3]>>31)||(raw[4]>>31)||raw[3]<0x3f800000u||raw[3]>0x42800000u||raw[4]<0x3f800000u||raw[4]>0x43000000u||(raw[5]&0x7fffffffu)>0x45610000u)return 0;
 for(i=0;i<3;i++)if((raw[i]&0x7fffffffu)>0x49800000u)return 0;
 memcpy(o,raw,12);stage=*out;
 __asm fnsave saved
 __asm stmxcsr mx
 __asm fldcw cw
 fit=C1AP14_QueryRaw(slot,o,radius,height,yaw,&stage.height,&stage.torso,&stage.waist,0,1,NULL,capsule,box,1);
 __asm frstor saved
 __asm ldmxcsr mx
 stage.fits=fit;
 {uint32_t bits[3];memcpy(bits,&stage.height,12);for(i=0;i<3;i++)if((bits[i]&0x7f800000u)==0x7f800000u)return 0;}
 *out=stage;return 1;
}
#else
#error R232 actor prone geometry requires the selected MSVC x86 arithmetic profile
#endif
#endif
