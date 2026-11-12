#ifndef BG_COD1_ACTOR_LOOK14_H
#define BG_COD1_ACTOR_LOOK14_H
/* Ownership-neutral original Look state/command arithmetic. Numeric cells map
 * native actor[1dc,21c), never a complete actor or native tree-ID namespace.
 * Step is one original AI update (native fixed .05 integration), not an elapsed
 * time accumulator. Genuine eye/current yaw and three queried CURRENT weights
 * are supplied by the owner; no guessed pose, trace, target or events. */
#include <stdint.h>
#include <stddef.h>
#include <string.h>
typedef struct {uint32_t active;float target[3],yaw,velocity,speed;uint32_t ready;float limits[2];uint32_t nodes[3];int32_t stamp;float slopes[2];} c1al14State_t;
typedef struct {uint32_t kind,node;float weight,blend,rate;} c1al14Command_t;
typedef struct {c1al14State_t state;unsigned count;c1al14Command_t command[3];} c1al14Result_t;
_Static_assert(sizeof(c1al14State_t)==64&&offsetof(c1al14State_t,nodes)==40&&offsetof(c1al14State_t,stamp)==52,"Look selected numeric cells");
static int C1AL14_Finite(uint32_t u){return (u&0x7f800000u)!=0x7f800000u;}
static int C1AL14_StateValid(const c1al14State_t*s){uint32_t u[16];unsigned i;if(!s)return 0;memcpy(u,s,64);if(u[0]>1||u[7]>1)return 0;for(i=1;i<16;i++)if(i!=7&&!(i>=10&&i<=13)&&!C1AL14_Finite(u[i]))return 0;if(s->ready&&(!s->nodes[0]||!s->nodes[1]||!s->nodes[2]||s->nodes[0]>=65536||s->nodes[1]>=65536||s->nodes[2]>=65536||s->nodes[0]==s->nodes[1]||s->nodes[0]==s->nodes[2]||s->nodes[1]==s->nodes[2]))return 0;return 1;}
/* Unowned handles/interpolation cells are unread while ready==0. They become
 * owned only when Nodes writes all of them. No native zero-default assertion. */
static int C1AL14_Fresh(const uint32_t actualKnownCells[10],c1al14State_t*out){c1al14State_t s;if(!actualKnownCells||!out||actualKnownCells[7])return 0;memset(&s,0,sizeof(s));memcpy(&s,actualKnownCells,40);if(!C1AL14_StateValid(&s))return 0;*out=s;return 1;}
static int C1AL14_Nodes(const c1al14State_t*in,const uint32_t nodes[3],int now,c1al14Result_t*out){c1al14Result_t v;unsigned i;uint32_t one=0x3f800000u;if(!in||!nodes||!out||now<0||!C1AL14_StateValid(in))return 0;memset(&v,0,sizeof(v));v.state=*in;memcpy(v.state.nodes,nodes,12);v.state.ready=1;v.state.stamp=now;memset(v.state.slopes,0,sizeof(v.state.slopes));if(!C1AL14_StateValid(&v.state))return 0;v.count=3;for(i=0;i<3;i++){v.command[i].kind=i==2?109:97;v.command[i].node=nodes[i==2?0:i+1];if(i==2){memcpy(&v.command[i].weight,&one,4);memcpy(&v.command[i].rate,&one,4);}}*out=v;return 1;}
#if defined(_MSC_VER)&&defined(_M_IX86)
typedef struct {float eye[3],weights[3];uint32_t nodes[3];c1al14Result_t*out;int error;} C1AL14_Frame;
static uint32_t __cdecl C1AL14_GoalQuery(int op,C1AL14_Frame*f,unsigned node){unsigned i;uint32_t bits=0;if(op!=118||!f)return 0;for(i=0;i<3;i++)if(node==f->nodes[i]){memcpy(&bits,f->weights+i,4);return bits;}f->error=1;return 0;}
static void __cdecl C1AL14_GoalSet(C1AL14_Frame*f,uint32_t node,float weight,float blend,float rate,int notify,int force){c1al14Command_t*c;if(!f||f->out->count>=3||notify||force){if(f)f->error=1;return;}c=f->out->command+f->out->count++;c->kind=109;c->node=node;memcpy(&c->weight,&weight,4);memcpy(&c->blend,&blend,4);memcpy(&c->rate,&rate,4);}
static __declspec(naked) void C1AL14_EyeRaw(void){__asm {mov ecx,[eax]
 mov [esi],ecx
 mov ecx,[eax+4]
 mov [esi+4],ecx
 mov ecx,[eax+8]
 mov [esi+8],ecx
 ret}}
static __declspec(naked) void C1AL14_TreeRaw(void){__asm {mov eax,[eax]
 ret}}
static __declspec(naked) void C1AL14_Sqrt(void){__asm {fsqrt
 ret}}
static __declspec(naked) void C1AL14_Atan2(void){__asm {sub esp,4
 fnstcw word ptr [esp]
 mov word ptr [esp+2],133fh
 fldcw word ptr [esp+2]
 fpatan
 fldcw word ptr [esp]
 add esp,4
 ret}}
static __declspec(naked) void C1AL14_Trunc(void){__asm {sub esp,12
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
static const uint32_t C1AL14_C2009A1B0=UINT32_C(0x43340000);
static const uint32_t C1AL14_C2009A1B4=UINT32_C(0x3f000000);
static const uint32_t C1AL14_C2009A1B8=UINT32_C(0x3f800000);
static const uint32_t C1AL14_C2009A1BC=UINT32_C(0x00000000);
static const uint32_t C1AL14_C2009A1D8=UINT32_C(0x3a83126f);
static const uint32_t C1AL14_C2009A1E8=UINT32_C(0x41a00000);
static const uint32_t C1AL14_C2009A200=UINT32_C(0xc47a0000);
static const uint32_t C1AL14_C2009A208=UINT32_C(0x3d4ccccd);
static const uint64_t C1AL14_C2009A268=UINT64_C(0x3ff0000000000000);
static const uint64_t C1AL14_C2009A288=UINT64_C(0x400921fb60000000);
static const uint64_t C1AL14_C2009A290=UINT64_C(0x4066800000000000);
static const uint32_t C1AL14_C2009A298=UINT32_C(0x43b40000);
static const uint32_t C1AL14_C2009A29C=UINT32_C(0xc3340000);
static const uint64_t C1AL14_C2009A2C0=UINT64_C(0xc066800000000000);
static const uint32_t C1AL14_C2009A2D0=UINT32_C(0x3fc00000);
static const uint32_t C1AL14_C2009A308=UINT32_C(0x41200000);
static const uint32_t C1AL14_C2009A354=UINT32_C(0x3dcccccd);
static const uint32_t C1AL14_C2009A3A8=UINT32_C(0xbf800000);
static const uint32_t C1AL14_C2009A578=UINT32_C(0x3d99999a);
static const uint32_t C1AL14_C2009A57C=UINT32_C(0x3e6147ae);
static const uint32_t C1AL14_C2009A580=UINT32_C(0xc1200000);
static const uint32_t C1AL14_C2009A584=UINT32_C(0xbdcccccd);
static void C1AL14_TargetRaw(void);
static void C1AL14_Limit0(void);
static void C1AL14_Limit1(void);
static void C1AL14_LimitsRaw(void);
static void C1AL14_StopRaw(void);
static void C1AL14_StepRaw(void);
static void C1AL14_AnglesRaw(void);
static void C1AL14_DeltaRaw(void);
static __declspec(naked) void C1AL14_TargetRaw(void){__asm {
C1AL14_L2005EB30:
 mov         edx,dword ptr [eax]
C1AL14_L2005EB32:
 fld         dword ptr [esp+4]
C1AL14_L2005EB36:
 fcomp       dword ptr C1AL14_C2009A1E8
C1AL14_L2005EB3C:
 mov         dword ptr [ecx+000001E0h],edx
C1AL14_L2005EB42:
 mov         edx,dword ptr [eax+4]
C1AL14_L2005EB45:
 mov         dword ptr [ecx+000001E4h],edx
C1AL14_L2005EB4B:
 mov         eax,dword ptr [eax+8]
C1AL14_L2005EB4E:
 mov         dword ptr [ecx+000001E8h],eax
C1AL14_L2005EB54:
 fnstsw      ax
C1AL14_L2005EB56:
 test        ah,41h
C1AL14_L2005EB59:
 jne C1AL14_L2005EB65
C1AL14_L2005EB5B:
 mov         edx,dword ptr [esp+4]
C1AL14_L2005EB5F:
 mov         dword ptr [ecx+000001F4h],edx
C1AL14_L2005EB65:
 fld         dword ptr [ecx+000001F4h]
C1AL14_L2005EB6B:
 mov         dword ptr [ecx+000001DCh],1
C1AL14_L2005EB75:
 fcomp       dword ptr C1AL14_C2009A1E8
C1AL14_L2005EB7B:
 fnstsw      ax
C1AL14_L2005EB7D:
 test        ah,5
C1AL14_L2005EB80:
 jp C1AL14_L2005EB8C
C1AL14_L2005EB82:
 mov         dword ptr [ecx+000001F4h],41A00000h
C1AL14_L2005EB8C:
 ret
}}
static __declspec(naked) void C1AL14_Limit0(void){__asm {
C1AL14_L2005EB90:
 push        ecx
C1AL14_L2005EB91:
 mov         ecx,dword ptr [eax+00000210h]
C1AL14_L2005EB97:
 sub         ecx,dword ptr [eax+21Ch]
C1AL14_L2005EB9D:
 mov         dword ptr [esp],ecx
C1AL14_L2005EBA1:
 jns C1AL14_L2005EBAB
C1AL14_L2005EBA3:
 mov         dword ptr [esp],0
C1AL14_L2005EBAB:
 fild        dword ptr [esp]
C1AL14_L2005EBAF:
 fmul        dword ptr [eax+00000214h]
C1AL14_L2005EBB5:
 fsubr       dword ptr [eax+000001FCh]
C1AL14_L2005EBBB:
 fcom        dword ptr C1AL14_C2009A1BC
C1AL14_L2005EBC1:
 fnstsw      ax
C1AL14_L2005EBC3:
 test        ah,5
C1AL14_L2005EBC6:
 jp C1AL14_L2005EBD2
C1AL14_L2005EBC8:
 fstp        st(0)
C1AL14_L2005EBCA:
 fld         dword ptr C1AL14_C2009A1BC
C1AL14_L2005EBD0:
 pop         ecx
C1AL14_L2005EBD1:
 ret
C1AL14_L2005EBD2:
 fcom        dword ptr C1AL14_C2009A1B0
C1AL14_L2005EBD8:
 fnstsw      ax
C1AL14_L2005EBDA:
 test        ah,41h
C1AL14_L2005EBDD:
 jne C1AL14_L2005EBE7
C1AL14_L2005EBDF:
 fstp        st(0)
C1AL14_L2005EBE1:
 fld         dword ptr C1AL14_C2009A1B0
C1AL14_L2005EBE7:
 pop         ecx
C1AL14_L2005EBE8:
 ret
}}
static __declspec(naked) void C1AL14_Limit1(void){__asm {
C1AL14_L2005EBF0:
 push        ecx
C1AL14_L2005EBF1:
 mov         ecx,dword ptr [eax+00000210h]
C1AL14_L2005EBF7:
 sub         ecx,dword ptr [eax+21Ch]
C1AL14_L2005EBFD:
 mov         dword ptr [esp],ecx
C1AL14_L2005EC01:
 jns C1AL14_L2005EC0B
C1AL14_L2005EC03:
 mov         dword ptr [esp],0
C1AL14_L2005EC0B:
 fild        dword ptr [esp]
C1AL14_L2005EC0F:
 fmul        dword ptr [eax+00000218h]
C1AL14_L2005EC15:
 fsubr       dword ptr [eax+00000200h]
C1AL14_L2005EC1B:
 fcom        dword ptr C1AL14_C2009A1BC
C1AL14_L2005EC21:
 fnstsw      ax
C1AL14_L2005EC23:
 test        ah,5
C1AL14_L2005EC26:
 jp C1AL14_L2005EC32
C1AL14_L2005EC28:
 fstp        st(0)
C1AL14_L2005EC2A:
 fld         dword ptr C1AL14_C2009A1BC
C1AL14_L2005EC30:
 pop         ecx
C1AL14_L2005EC31:
 ret
C1AL14_L2005EC32:
 fcom        dword ptr C1AL14_C2009A1B0
C1AL14_L2005EC38:
 fnstsw      ax
C1AL14_L2005EC3A:
 test        ah,41h
C1AL14_L2005EC3D:
 jne C1AL14_L2005EC47
C1AL14_L2005EC3F:
 fstp        st(0)
C1AL14_L2005EC41:
 fld         dword ptr C1AL14_C2009A1B0
C1AL14_L2005EC47:
 pop         ecx
C1AL14_L2005EC48:
 ret
}}
static __declspec(naked) void C1AL14_LimitsRaw(void){__asm {
C1AL14_L2005EC50:
 push        ecx
C1AL14_L2005EC51:
 fld         dword ptr [esp+8]
C1AL14_L2005EC55:
 fcomp       dword ptr [esi+000001FCh]
C1AL14_L2005EC5B:
 fnstsw      ax
C1AL14_L2005EC5D:
 test        ah,44h
C1AL14_L2005EC60:
 jp C1AL14_L2005EC77
C1AL14_L2005EC62:
 fld         dword ptr [esp+0Ch]
C1AL14_L2005EC66:
 fcomp       dword ptr [esi+00000200h]
C1AL14_L2005EC6C:
 fnstsw      ax
C1AL14_L2005EC6E:
 test        ah,44h
C1AL14_L2005EC71:
 jnp C1AL14_L2005ED08
C1AL14_L2005EC77:
 mov         eax,esi
C1AL14_L2005EC79:
 call C1AL14_Limit0
C1AL14_L2005EC7E:
 fsubr       dword ptr [esp+8]
C1AL14_L2005EC82:
 mov         eax,dword ptr [esp+8]
C1AL14_L2005EC86:
 mov         dword ptr [esi+000001FCh],eax
C1AL14_L2005EC8C:
 mov         eax,esi
C1AL14_L2005EC8E:
 fstp        dword ptr [esp]
C1AL14_L2005EC92:
 call C1AL14_Limit1
C1AL14_L2005EC97:
 fsubr       dword ptr [esp+0Ch]
C1AL14_L2005EC9B:
 fld         dword ptr [esp+10h]
C1AL14_L2005EC9F:
 mov         ecx,dword ptr [esp+0Ch]
C1AL14_L2005ECA3:
 fmul        dword ptr C1AL14_C2009A200
C1AL14_L2005ECA9:
 mov         dword ptr [esi+00000200h],ecx
C1AL14_L2005ECAF:
 call C1AL14_Trunc
C1AL14_L2005ECB4:
 fld         dword ptr [esp+10h]
C1AL14_L2005ECB8:
 fcomp       dword ptr C1AL14_C2009A1BC
C1AL14_L2005ECBE:
 mov         edx,dword ptr [esi+21Ch]
C1AL14_L2005ECC4:
 sub         edx,eax
C1AL14_L2005ECC6:
 mov         dword ptr [esi+00000210h],edx
C1AL14_L2005ECCC:
 fnstsw      ax
C1AL14_L2005ECCE:
 test        ah,44h
C1AL14_L2005ECD1:
 jp C1AL14_L2005ECE8
C1AL14_L2005ECD3:
 mov         eax,461C3C00h
C1AL14_L2005ECD8:
 fstp        st(0)
C1AL14_L2005ECDA:
 mov         dword ptr [esi+00000214h],eax
C1AL14_L2005ECE0:
 mov         dword ptr [esi+00000218h],eax
C1AL14_L2005ECE6:
 pop         ecx
C1AL14_L2005ECE7:
 ret
C1AL14_L2005ECE8:
 fld         dword ptr C1AL14_C2009A1D8
C1AL14_L2005ECEE:
 fdiv        dword ptr [esp+10h]
C1AL14_L2005ECF2:
 fld         dword ptr [esp]
C1AL14_L2005ECF6:
 fmul        st,st(1)
C1AL14_L2005ECF8:
 fstp        dword ptr [esi+00000214h]
C1AL14_L2005ECFE:
 fmul        st,st(1)
C1AL14_L2005ED00:
 fstp        dword ptr [esi+00000218h]
C1AL14_L2005ED06:
 fstp        st(0)
C1AL14_L2005ED08:
 pop         ecx
C1AL14_L2005ED09:
 ret
}}
static __declspec(naked) void C1AL14_StopRaw(void){__asm {
C1AL14_L2005ED10:
 fld         dword ptr [esp+4]
C1AL14_L2005ED14:
 mov         dword ptr [ecx+000001DCh],0
C1AL14_L2005ED1E:
 fcomp       dword ptr C1AL14_C2009A1E8
C1AL14_L2005ED24:
 fnstsw      ax
C1AL14_L2005ED26:
 test        ah,41h
C1AL14_L2005ED29:
 jne C1AL14_L2005ED35
C1AL14_L2005ED2B:
 mov         eax,dword ptr [esp+4]
C1AL14_L2005ED2F:
 mov         dword ptr [ecx+000001F4h],eax
C1AL14_L2005ED35:
 ret
}}
static __declspec(naked) void C1AL14_StepRaw(void){__asm {
C1AL14_L2005ED40:
 mov         eax,dword ptr [ebx+000001F8h]
C1AL14_L2005ED46:
 sub         esp,20h
C1AL14_L2005ED49:
 push        ebp
C1AL14_L2005ED4A:
 xor         ebp,ebp
C1AL14_L2005ED4C:
 test        eax,eax
C1AL14_L2005ED4E:
 je C1AL14_L2005F1D6
C1AL14_L2005ED54:
 mov         eax,dword ptr [ebx+000001DCh]
C1AL14_L2005ED5A:
 test        eax,eax
C1AL14_L2005ED5C:
 je C1AL14_L2005ED84
C1AL14_L2005ED5E:
 fld         dword ptr [ebx+000001FCh]
C1AL14_L2005ED64:
 fcomp       dword ptr C1AL14_C2009A1BC
C1AL14_L2005ED6A:
 fnstsw      ax
C1AL14_L2005ED6C:
 test        ah,44h
C1AL14_L2005ED6F:
 jnp C1AL14_L2005ED84
C1AL14_L2005ED71:
 fld         dword ptr [ebx+00000200h]
C1AL14_L2005ED77:
 fcomp       dword ptr C1AL14_C2009A1BC
C1AL14_L2005ED7D:
 fnstsw      ax
C1AL14_L2005ED7F:
 test        ah,44h
C1AL14_L2005ED82:
 jp C1AL14_L2005ED9B
C1AL14_L2005ED84:
 fld         dword ptr [ebx+000001ECh]
C1AL14_L2005ED8A:
 fcomp       dword ptr C1AL14_C2009A1BC
C1AL14_L2005ED90:
 fnstsw      ax
C1AL14_L2005ED92:
 test        ah,44h
C1AL14_L2005ED95:
 jnp C1AL14_L2005F1D6
C1AL14_L2005ED9B:
 push        esi
C1AL14_L2005ED9C:
 push        edi
C1AL14_L2005ED9D:
 mov         eax,ebx
C1AL14_L2005ED9F:
 call C1AL14_Limit1
C1AL14_L2005EDA4:
 fstp        dword ptr [esp+10h]
C1AL14_L2005EDA8:
 mov         eax,dword ptr [ebx+4]
C1AL14_L2005EDAB:
 lea         esi,[esp+14h]
C1AL14_L2005EDAF:
 call C1AL14_EyeRaw
C1AL14_L2005EDB4:
 fld         dword ptr [ebx+000001E0h]
C1AL14_L2005EDBA:
 fsub        dword ptr [esp+14h]
C1AL14_L2005EDBE:
 lea         edi,[esp+14h]
C1AL14_L2005EDC2:
 lea         esi,[esp+20h]
C1AL14_L2005EDC6:
 fstp        dword ptr [esp+20h]
C1AL14_L2005EDCA:
 fld         dword ptr [ebx+000001E4h]
C1AL14_L2005EDD0:
 fsub        dword ptr [esp+18h]
C1AL14_L2005EDD4:
 fstp        dword ptr [esp+24h]
C1AL14_L2005EDD8:
 fld         dword ptr [ebx+000001E8h]
C1AL14_L2005EDDE:
 fsub        dword ptr [esp+1Ch]
C1AL14_L2005EDE2:
 fstp        dword ptr [esp+28h]
C1AL14_L2005EDE6:
 call C1AL14_AnglesRaw
C1AL14_L2005EDEB:
 mov         eax,dword ptr [ebx+000001DCh]
C1AL14_L2005EDF1:
 xor         edi,edi
C1AL14_L2005EDF3:
 cmp         eax,edi
C1AL14_L2005EDF5:
 je C1AL14_L2005EE66
C1AL14_L2005EDF7:
 fld         dword ptr [ebx+000001FCh]
C1AL14_L2005EDFD:
 fcomp       dword ptr C1AL14_C2009A1BC
C1AL14_L2005EE03:
 fnstsw      ax
C1AL14_L2005EE05:
 test        ah,44h
C1AL14_L2005EE08:
 jnp C1AL14_L2005EE66
C1AL14_L2005EE0A:
 fld         dword ptr [ebx+00000200h]
C1AL14_L2005EE10:
 fcomp       dword ptr C1AL14_C2009A1BC
C1AL14_L2005EE16:
 fnstsw      ax
C1AL14_L2005EE18:
 test        ah,44h
C1AL14_L2005EE1B:
 jnp C1AL14_L2005EE66
C1AL14_L2005EE1D:
 mov         eax,dword ptr [esp+18h]
C1AL14_L2005EE21:
 mov         ecx,dword ptr [ebx]
C1AL14_L2005EE23:
 fld         dword ptr [ecx+000000F0h]
C1AL14_L2005EE29:
 push        eax
C1AL14_L2005EE2A:
 push        ecx
C1AL14_L2005EE2B:
 fstp        dword ptr [esp]
C1AL14_L2005EE2E:
 call C1AL14_DeltaRaw
C1AL14_L2005EE33:
 fld         dword ptr [esp+18h]
C1AL14_L2005EE37:
 fchs
C1AL14_L2005EE39:
 add         esp,8
C1AL14_L2005EE3C:
 fstp        dword ptr [esp+0Ch]
C1AL14_L2005EE40:
 fcom        dword ptr [esp+0Ch]
C1AL14_L2005EE44:
 fnstsw      ax
C1AL14_L2005EE46:
 test        ah,5
C1AL14_L2005EE49:
 jp C1AL14_L2005EE53
C1AL14_L2005EE4B:
 fstp        st(0)
C1AL14_L2005EE4D:
 fld         dword ptr [esp+0Ch]
C1AL14_L2005EE51:
 jmp C1AL14_L2005EE6C
C1AL14_L2005EE53:
 fcom        dword ptr [esp+10h]
C1AL14_L2005EE57:
 fnstsw      ax
C1AL14_L2005EE59:
 test        ah,41h
C1AL14_L2005EE5C:
 jne C1AL14_L2005EE6C
C1AL14_L2005EE5E:
 fstp        st(0)
C1AL14_L2005EE60:
 fld         dword ptr [esp+10h]
C1AL14_L2005EE64:
 jmp C1AL14_L2005EE6C
C1AL14_L2005EE66:
 fld         dword ptr C1AL14_C2009A1BC
C1AL14_L2005EE6C:
 fld         st(0)
C1AL14_L2005EE6E:
 fsub        dword ptr [ebx+000001ECh]
C1AL14_L2005EE74:
 fst         dword ptr [esp+10h]
C1AL14_L2005EE78:
 fabs
C1AL14_L2005EE7A:
 fcomp       qword ptr C1AL14_C2009A268
C1AL14_L2005EE80:
 fnstsw      ax
C1AL14_L2005EE82:
 test        ah,5
C1AL14_L2005EE85:
 jp C1AL14_L2005EE98
C1AL14_L2005EE87:
 fstp        dword ptr [ebx+000001ECh]
C1AL14_L2005EE8D:
 mov         dword ptr [ebx+000001F0h],edi
C1AL14_L2005EE93:
 jmp C1AL14_L2005F064
C1AL14_L2005EE98:
 fld         dword ptr [ebx+000001F0h]
C1AL14_L2005EE9E:
 fld         st(1)
C1AL14_L2005EEA0:
 fcomp       dword ptr C1AL14_C2009A1BC
C1AL14_L2005EEA6:
 fnstsw      ax
C1AL14_L2005EEA8:
 test        ah,5
C1AL14_L2005EEAB:
 jnp C1AL14_L2005EED1
C1AL14_L2005EEAD:
 fld         st(1)
C1AL14_L2005EEAF:
 fcomp       dword ptr C1AL14_C2009A1BC
C1AL14_L2005EEB5:
 fnstsw      ax
C1AL14_L2005EEB7:
 test        ah,44h
C1AL14_L2005EEBA:
 jp C1AL14_L2005EEF4
C1AL14_L2005EEBC:
 fld         dword ptr [esp+10h]
C1AL14_L2005EEC0:
 fcomp       dword ptr C1AL14_C2009A1BC
C1AL14_L2005EEC6:
 fnstsw      ax
C1AL14_L2005EEC8:
 test        ah,5
C1AL14_L2005EECB:
 jp C1AL14_L2005EF73
C1AL14_L2005EED1:
 fmul        dword ptr C1AL14_C2009A3A8
C1AL14_L2005EED7:
 mov         ebp,1
C1AL14_L2005EEDC:
 fxch        st(1)
C1AL14_L2005EEDE:
 fmul        dword ptr C1AL14_C2009A3A8
C1AL14_L2005EEE4:
 fxch        st(1)
C1AL14_L2005EEE6:
 fld         dword ptr [esp+10h]
C1AL14_L2005EEEA:
 fmul        dword ptr C1AL14_C2009A3A8
C1AL14_L2005EEF0:
 fstp        dword ptr [esp+10h]
C1AL14_L2005EEF4:
 fld         dword ptr [esp+10h]
C1AL14_L2005EEF8:
 fcomp       dword ptr C1AL14_C2009A1BC
C1AL14_L2005EEFE:
 fnstsw      ax
C1AL14_L2005EF00:
 test        ah,5
C1AL14_L2005EF03:
 jp C1AL14_L2005EF73
C1AL14_L2005EF05:
 fld         dword ptr C1AL14_C2009A584
C1AL14_L2005EF0B:
 fmul        st,st(1)
C1AL14_L2005EF0D:
 fcomp       dword ptr [esp+10h]
C1AL14_L2005EF11:
 fnstsw      ax
C1AL14_L2005EF13:
 test        ah,41h
C1AL14_L2005EF16:
 jp C1AL14_L2005EF63
C1AL14_L2005EF18:
 fmul        dword ptr C1AL14_C2009A2D0
C1AL14_L2005EF1E:
 fcom        dword ptr C1AL14_C2009A580
C1AL14_L2005EF24:
 fnstsw      ax
C1AL14_L2005EF26:
 test        ah,41h
C1AL14_L2005EF29:
 jne C1AL14_L2005EF38
C1AL14_L2005EF2B:
 fstp        st(0)
C1AL14_L2005EF2D:
 fld         dword ptr C1AL14_C2009A580
C1AL14_L2005EF33:
 jmp C1AL14_L2005EFDC
C1AL14_L2005EF38:
 fld         dword ptr C1AL14_C2009A584
C1AL14_L2005EF3E:
 fmul        st,st(1)
C1AL14_L2005EF40:
 fcomp       dword ptr [esp+10h]
C1AL14_L2005EF44:
 fnstsw      ax
C1AL14_L2005EF46:
 test        ah,41h
C1AL14_L2005EF49:
 jne C1AL14_L2005EFDC
C1AL14_L2005EF4F:
 fstp        st(0)
C1AL14_L2005EF51:
 fld         dword ptr [esp+10h]
C1AL14_L2005EF55:
 fmul        dword ptr C1AL14_C2009A580
C1AL14_L2005EF5B:
 fsub        dword ptr C1AL14_C2009A354
C1AL14_L2005EF61:
 jmp C1AL14_L2005EFDC
C1AL14_L2005EF63:
 fld         dword ptr [ebx+000001F4h]
C1AL14_L2005EF69:
 fmul        dword ptr C1AL14_C2009A208
C1AL14_L2005EF6F:
 fsubp       st(1),st
C1AL14_L2005EF71:
 jmp C1AL14_L2005EFDC
C1AL14_L2005EF73:
 fld         dword ptr C1AL14_C2009A354
C1AL14_L2005EF79:
 fmul        st,st(1)
C1AL14_L2005EF7B:
 fcomp       dword ptr [esp+10h]
C1AL14_L2005EF7F:
 fnstsw      ax
C1AL14_L2005EF81:
 test        ah,1
C1AL14_L2005EF84:
 jne C1AL14_L2005EFCE
C1AL14_L2005EF86:
 fld         dword ptr C1AL14_C2009A1B4
C1AL14_L2005EF8C:
 fmul        st,st(1)
C1AL14_L2005EF8E:
 fsubp       st(1),st
C1AL14_L2005EF90:
 fcom        dword ptr C1AL14_C2009A308
C1AL14_L2005EF96:
 fnstsw      ax
C1AL14_L2005EF98:
 test        ah,5
C1AL14_L2005EF9B:
 jp C1AL14_L2005EFA7
C1AL14_L2005EF9D:
 fstp        st(0)
C1AL14_L2005EF9F:
 fld         dword ptr C1AL14_C2009A308
C1AL14_L2005EFA5:
 jmp C1AL14_L2005EFDC
C1AL14_L2005EFA7:
 fld         dword ptr C1AL14_C2009A354
C1AL14_L2005EFAD:
 fmul        st,st(1)
C1AL14_L2005EFAF:
 fcomp       dword ptr [esp+10h]
C1AL14_L2005EFB3:
 fnstsw      ax
C1AL14_L2005EFB5:
 test        ah,5
C1AL14_L2005EFB8:
 jp C1AL14_L2005EFDC
C1AL14_L2005EFBA:
 fstp        st(0)
C1AL14_L2005EFBC:
 fld         dword ptr [esp+10h]
C1AL14_L2005EFC0:
 fmul        dword ptr C1AL14_C2009A308
C1AL14_L2005EFC6:
 fadd        dword ptr C1AL14_C2009A354
C1AL14_L2005EFCC:
 jmp C1AL14_L2005EFDC
C1AL14_L2005EFCE:
 fld         dword ptr [ebx+000001F4h]
C1AL14_L2005EFD4:
 fmul        dword ptr C1AL14_C2009A208
C1AL14_L2005EFDA:
 faddp       st(1),st
C1AL14_L2005EFDC:
 fld         dword ptr [ebx+000001F4h]
C1AL14_L2005EFE2:
 fmul        dword ptr C1AL14_C2009A57C
C1AL14_L2005EFE8:
 fstp        dword ptr [esp+10h]
C1AL14_L2005EFEC:
 fcom        dword ptr [esp+10h]
C1AL14_L2005EFF0:
 fnstsw      ax
C1AL14_L2005EFF2:
 test        ah,41h
C1AL14_L2005EFF5:
 jne C1AL14_L2005EFFD
C1AL14_L2005EFF7:
 fstp        st(0)
C1AL14_L2005EFF9:
 fld         dword ptr [esp+10h]
C1AL14_L2005EFFD:
 cmp         ebp,edi
C1AL14_L2005EFFF:
 je C1AL14_L2005F011
C1AL14_L2005F001:
 fmul        dword ptr C1AL14_C2009A3A8
C1AL14_L2005F007:
 fxch        st(1)
C1AL14_L2005F009:
 fmul        dword ptr C1AL14_C2009A3A8
C1AL14_L2005F00F:
 fxch        st(1)
C1AL14_L2005F011:
 fst         dword ptr [ebx+000001F0h]
C1AL14_L2005F017:
 fld         dword ptr [ebx+000001ECh]
C1AL14_L2005F01D:
 fcomp       st(2)
C1AL14_L2005F01F:
 fmul        dword ptr C1AL14_C2009A208
C1AL14_L2005F025:
 fnstsw      ax
C1AL14_L2005F027:
 fadd        dword ptr [ebx+000001ECh]
C1AL14_L2005F02D:
 test        ah,5
C1AL14_L2005F030:
 fst         dword ptr [ebx+000001ECh]
C1AL14_L2005F036:
 fcomp       st(1)
C1AL14_L2005F038:
 fnstsw      ax
C1AL14_L2005F03A:
 jp C1AL14_L2005F04F
C1AL14_L2005F03C:
 test        ah,1
C1AL14_L2005F03F:
 jne C1AL14_L2005F062
C1AL14_L2005F041:
 fstp        dword ptr [ebx+000001ECh]
C1AL14_L2005F047:
 mov         dword ptr [ebx+000001F0h],edi
C1AL14_L2005F04D:
 jmp C1AL14_L2005F064
C1AL14_L2005F04F:
 test        ah,41h
C1AL14_L2005F052:
 jp C1AL14_L2005F062
C1AL14_L2005F054:
 fstp        dword ptr [ebx+000001ECh]
C1AL14_L2005F05A:
 mov         dword ptr [ebx+000001F0h],edi
C1AL14_L2005F060:
 jmp C1AL14_L2005F064
C1AL14_L2005F062:
 fstp        st(0)
C1AL14_L2005F064:
 mov         eax,dword ptr [ebx]
C1AL14_L2005F066:
 call C1AL14_TreeRaw
C1AL14_L2005F06B:
 mov         esi,eax
C1AL14_L2005F06D:
 mov         eax,ebx
C1AL14_L2005F06F:
 call C1AL14_Limit0
C1AL14_L2005F074:
 fdivr       dword ptr [ebx+000001ECh]
C1AL14_L2005F07A:
 fmul        dword ptr C1AL14_C2009A1B4
C1AL14_L2005F080:
 fst         dword ptr [esp+0Ch]
C1AL14_L2005F084:
 fcomp       dword ptr C1AL14_C2009A1B8
C1AL14_L2005F08A:
 fnstsw      ax
C1AL14_L2005F08C:
 test        ah,41h
C1AL14_L2005F08F:
 jne C1AL14_L2005F09E
C1AL14_L2005F091:
 mov         dword ptr [esp+0Ch],3F800000h
C1AL14_L2005F099:
 jmp C1AL14_L2005F1F0
C1AL14_L2005F09E:
 fld         dword ptr [esp+0Ch]
C1AL14_L2005F0A2:
 fcomp       dword ptr C1AL14_C2009A3A8
C1AL14_L2005F0A8:
 fnstsw      ax
C1AL14_L2005F0AA:
 test        ah,5
C1AL14_L2005F0AD:
 jp C1AL14_L2005F1DB
C1AL14_L2005F0B3:
 mov         dword ptr [esp+0Ch],0BF800000h
C1AL14_L2005F0BB:
 movzx       ecx,word ptr [ebx+00000208h]
C1AL14_L2005F0C2:
 fld         dword ptr [esp+0Ch]
C1AL14_L2005F0C6:
 fmul        dword ptr C1AL14_C2009A3A8
C1AL14_L2005F0CC:
 push        ecx
C1AL14_L2005F0CD:
 push        esi
C1AL14_L2005F0CE:
 push        76h
C1AL14_L2005F0D0:
 fstp        dword ptr [esp+18h]
C1AL14_L2005F0D4:
 call C1AL14_GoalQuery
C1AL14_L2005F0DA:
 mov         dword ptr [esp+1Ch],eax
C1AL14_L2005F0DE:
 fld         dword ptr [esp+18h]
C1AL14_L2005F0E2:
 add         esp,0Ch
C1AL14_L2005F0E5:
 fcomp       dword ptr [esp+10h]
C1AL14_L2005F0E9:
 fnstsw      ax
C1AL14_L2005F0EB:
 test        ah,44h
C1AL14_L2005F0EE:
 jnp C1AL14_L2005F120
C1AL14_L2005F0F0:
 fld         dword ptr [esp+0Ch]
C1AL14_L2005F0F4:
 mov         edx,dword ptr [esp+0Ch]
C1AL14_L2005F0F8:
 fsub        dword ptr [esp+10h]
C1AL14_L2005F0FC:
 mov         eax,dword ptr [ebx+00000208h]
C1AL14_L2005F102:
 push        edi
C1AL14_L2005F103:
 push        edi
C1AL14_L2005F104:
 fabs
C1AL14_L2005F106:
 push        3F800000h
C1AL14_L2005F10B:
 fdivr       dword ptr C1AL14_C2009A578
C1AL14_L2005F111:
 push        ecx
C1AL14_L2005F112:
 fstp        dword ptr [esp]
C1AL14_L2005F115:
 push        edx
C1AL14_L2005F116:
 push        eax
C1AL14_L2005F117:
 push        esi
C1AL14_L2005F118:
 call C1AL14_GoalSet
C1AL14_L2005F11D:
 add         esp,1Ch
C1AL14_L2005F120:
 movzx       ecx,word ptr [ebx+0000020Ch]
C1AL14_L2005F127:
 push        ecx
C1AL14_L2005F128:
 push        esi
C1AL14_L2005F129:
 push        76h
C1AL14_L2005F12B:
 call C1AL14_GoalQuery
C1AL14_L2005F131:
 mov         dword ptr [esp+1Ch],eax
C1AL14_L2005F135:
 add         esp,0Ch
C1AL14_L2005F138:
 fld         dword ptr [esp+10h]
C1AL14_L2005F13C:
 fcomp       dword ptr C1AL14_C2009A1BC
C1AL14_L2005F142:
 fnstsw      ax
C1AL14_L2005F144:
 test        ah,44h
C1AL14_L2005F147:
 jnp C1AL14_L2005F16F
C1AL14_L2005F149:
 fld         dword ptr C1AL14_C2009A578
C1AL14_L2005F14F:
 mov         edx,dword ptr [ebx+0000020Ch]
C1AL14_L2005F155:
 fdiv        dword ptr [esp+10h]
C1AL14_L2005F159:
 push        edi
C1AL14_L2005F15A:
 push        edi
C1AL14_L2005F15B:
 push        3F800000h
C1AL14_L2005F160:
 push        ecx
C1AL14_L2005F161:
 fstp        dword ptr [esp]
C1AL14_L2005F164:
 push        edi
C1AL14_L2005F165:
 push        edx
C1AL14_L2005F166:
 push        esi
C1AL14_L2005F167:
 call C1AL14_GoalSet
C1AL14_L2005F16C:
 add         esp,1Ch
C1AL14_L2005F16F:
 movzx       eax,word ptr [ebx+00000204h]
C1AL14_L2005F176:
 fld         dword ptr C1AL14_C2009A1B8
C1AL14_L2005F17C:
 fsub        dword ptr [esp+0Ch]
C1AL14_L2005F180:
 push        eax
C1AL14_L2005F181:
 push        esi
C1AL14_L2005F182:
 push        76h
C1AL14_L2005F184:
 fstp        dword ptr [esp+18h]
C1AL14_L2005F188:
 call C1AL14_GoalQuery
C1AL14_L2005F18E:
 mov         dword ptr [esp+1Ch],eax
C1AL14_L2005F192:
 fld         dword ptr [esp+18h]
C1AL14_L2005F196:
 add         esp,0Ch
C1AL14_L2005F199:
 fcomp       dword ptr [esp+10h]
C1AL14_L2005F19D:
 fnstsw      ax
C1AL14_L2005F19F:
 test        ah,44h
C1AL14_L2005F1A2:
 jnp C1AL14_L2005F1D4
C1AL14_L2005F1A4:
 fld         dword ptr [esp+0Ch]
C1AL14_L2005F1A8:
 mov         edx,dword ptr [ebx+00000204h]
C1AL14_L2005F1AE:
 fsub        dword ptr [esp+10h]
C1AL14_L2005F1B2:
 push        edi
C1AL14_L2005F1B3:
 push        edi
C1AL14_L2005F1B4:
 push        3F800000h
C1AL14_L2005F1B9:
 fabs
C1AL14_L2005F1BB:
 push        ecx
C1AL14_L2005F1BC:
 fdivr       dword ptr C1AL14_C2009A578
C1AL14_L2005F1C2:
 mov         ecx,dword ptr [esp+1Ch]
C1AL14_L2005F1C6:
 fstp        dword ptr [esp]
C1AL14_L2005F1C9:
 push        ecx
C1AL14_L2005F1CA:
 push        edx
C1AL14_L2005F1CB:
 push        esi
C1AL14_L2005F1CC:
 call C1AL14_GoalSet
C1AL14_L2005F1D1:
 add         esp,1Ch
C1AL14_L2005F1D4:
 pop         edi
C1AL14_L2005F1D5:
 pop         esi
C1AL14_L2005F1D6:
 pop         ebp
C1AL14_L2005F1D7:
 add         esp,20h
C1AL14_L2005F1DA:
 ret
C1AL14_L2005F1DB:
 fld         dword ptr [esp+0Ch]
C1AL14_L2005F1DF:
 fcomp       dword ptr C1AL14_C2009A1BC
C1AL14_L2005F1E5:
 fnstsw      ax
C1AL14_L2005F1E7:
 test        ah,41h
C1AL14_L2005F1EA:
 jne C1AL14_L2005F0BB
C1AL14_L2005F1F0:
 movzx       edx,word ptr [ebx+0000020Ch]
C1AL14_L2005F1F7:
 push        edx
C1AL14_L2005F1F8:
 push        esi
C1AL14_L2005F1F9:
 push        76h
C1AL14_L2005F1FB:
 call C1AL14_GoalQuery
C1AL14_L2005F201:
 mov         dword ptr [esp+1Ch],eax
C1AL14_L2005F205:
 fld         dword ptr [esp+18h]
C1AL14_L2005F209:
 add         esp,0Ch
C1AL14_L2005F20C:
 fcomp       dword ptr [esp+10h]
C1AL14_L2005F210:
 fnstsw      ax
C1AL14_L2005F212:
 test        ah,44h
C1AL14_L2005F215:
 jnp C1AL14_L2005F247
C1AL14_L2005F217:
 fld         dword ptr [esp+0Ch]
C1AL14_L2005F21B:
 mov         eax,dword ptr [esp+0Ch]
C1AL14_L2005F21F:
 fsub        dword ptr [esp+10h]
C1AL14_L2005F223:
 push        edi
C1AL14_L2005F224:
 push        edi
C1AL14_L2005F225:
 push        3F800000h
C1AL14_L2005F22A:
 fabs
C1AL14_L2005F22C:
 push        ecx
C1AL14_L2005F22D:
 fdivr       dword ptr C1AL14_C2009A578
C1AL14_L2005F233:
 mov         ecx,dword ptr [ebx+0000020Ch]
C1AL14_L2005F239:
 fstp        dword ptr [esp]
C1AL14_L2005F23C:
 push        eax
C1AL14_L2005F23D:
 push        ecx
C1AL14_L2005F23E:
 push        esi
C1AL14_L2005F23F:
 call C1AL14_GoalSet
C1AL14_L2005F244:
 add         esp,1Ch
C1AL14_L2005F247:
 movzx       edx,word ptr [ebx+00000208h]
C1AL14_L2005F24E:
 push        edx
C1AL14_L2005F24F:
 push        esi
C1AL14_L2005F250:
 push        76h
C1AL14_L2005F252:
 call C1AL14_GoalQuery
C1AL14_L2005F258:
 mov         dword ptr [esp+1Ch],eax
C1AL14_L2005F25C:
 add         esp,0Ch
C1AL14_L2005F25F:
 fld         dword ptr [esp+10h]
C1AL14_L2005F263:
 fcomp       dword ptr C1AL14_C2009A1BC
C1AL14_L2005F269:
 fnstsw      ax
C1AL14_L2005F26B:
 test        ah,44h
C1AL14_L2005F26E:
 jnp C1AL14_L2005F16F
C1AL14_L2005F274:
 fld         dword ptr C1AL14_C2009A578
C1AL14_L2005F27A:
 mov         eax,dword ptr [ebx+00000208h]
C1AL14_L2005F280:
 fdiv        dword ptr [esp+10h]
C1AL14_L2005F284:
 push        edi
C1AL14_L2005F285:
 push        edi
C1AL14_L2005F286:
 push        3F800000h
C1AL14_L2005F28B:
 push        ecx
C1AL14_L2005F28C:
 fstp        dword ptr [esp]
C1AL14_L2005F28F:
 push        edi
C1AL14_L2005F290:
 push        eax
C1AL14_L2005F291:
 jmp C1AL14_L2005F166
}}
static __declspec(naked) void C1AL14_AnglesRaw(void){__asm {
C1AL14_L2000E910:
 fld         dword ptr [esi+4]
C1AL14_L2000E913:
 sub         esp,0Ch
C1AL14_L2000E916:
 fcomp       dword ptr C1AL14_C2009A1BC
C1AL14_L2000E91C:
 fnstsw      ax
C1AL14_L2000E91E:
 test        ah,44h
C1AL14_L2000E921:
 jp C1AL14_L2000E97C
C1AL14_L2000E923:
 fld         dword ptr [esi]
C1AL14_L2000E925:
 fcomp       dword ptr C1AL14_C2009A1BC
C1AL14_L2000E92B:
 fnstsw      ax
C1AL14_L2000E92D:
 test        ah,44h
C1AL14_L2000E930:
 jp C1AL14_L2000E97C
C1AL14_L2000E932:
 fld         dword ptr [esi+8]
C1AL14_L2000E935:
 mov         dword ptr [esp+4],0
C1AL14_L2000E93D:
 fcomp       dword ptr C1AL14_C2009A1BC
C1AL14_L2000E943:
 fnstsw      ax
C1AL14_L2000E945:
 test        ah,41h
C1AL14_L2000E948:
 jne C1AL14_L2000E963
C1AL14_L2000E94A:
 mov         edx,dword ptr [esp+4]
C1AL14_L2000E94E:
 mov         eax,43870000h
C1AL14_L2000E953:
 mov         dword ptr [edi],eax
C1AL14_L2000E955:
 mov         dword ptr [edi+4],edx
C1AL14_L2000E958:
 mov         dword ptr [edi+8],0
C1AL14_L2000E95F:
 add         esp,0Ch
C1AL14_L2000E962:
 ret
C1AL14_L2000E963:
 mov         edx,dword ptr [esp+4]
C1AL14_L2000E967:
 mov         eax,42B40000h
C1AL14_L2000E96C:
 mov         dword ptr [edi],eax
C1AL14_L2000E96E:
 mov         dword ptr [edi+4],edx
C1AL14_L2000E971:
 mov         dword ptr [edi+8],0
C1AL14_L2000E978:
 add         esp,0Ch
C1AL14_L2000E97B:
 ret
C1AL14_L2000E97C:
 fld         dword ptr [esi+4]
C1AL14_L2000E97F:
 fld         dword ptr [esi]
C1AL14_L2000E981:
 call C1AL14_Atan2
C1AL14_L2000E986:
 fmul        qword ptr C1AL14_C2009A290
C1AL14_L2000E98C:
 fdiv        qword ptr C1AL14_C2009A288
C1AL14_L2000E992:
 fstp        dword ptr [esp+4]
C1AL14_L2000E996:
 fld         dword ptr [esp+4]
C1AL14_L2000E99A:
 fcomp       dword ptr C1AL14_C2009A1BC
C1AL14_L2000E9A0:
 fnstsw      ax
C1AL14_L2000E9A2:
 test        ah,5
C1AL14_L2000E9A5:
 jp C1AL14_L2000E9B5
C1AL14_L2000E9A7:
 fld         dword ptr [esp+4]
C1AL14_L2000E9AB:
 fadd        dword ptr C1AL14_C2009A298
C1AL14_L2000E9B1:
 fstp        dword ptr [esp+4]
C1AL14_L2000E9B5:
 mov         ecx,dword ptr [esi]
C1AL14_L2000E9B7:
 mov         eax,dword ptr [esi+4]
C1AL14_L2000E9BA:
 mov         dword ptr [esp],ecx
C1AL14_L2000E9BE:
 fld         dword ptr [esp]
C1AL14_L2000E9C2:
 mov         dword ptr [esp+8],eax
C1AL14_L2000E9C6:
 fmul        dword ptr [esp]
C1AL14_L2000E9CA:
 fld         dword ptr [esp+8]
C1AL14_L2000E9CE:
 fmul        dword ptr [esp+8]
C1AL14_L2000E9D2:
 faddp       st(1),st
C1AL14_L2000E9D4:
 call C1AL14_Sqrt
C1AL14_L2000E9D9:
 fstp        dword ptr [esp+8]
C1AL14_L2000E9DD:
 fld         dword ptr [esi+8]
C1AL14_L2000E9E0:
 fld         dword ptr [esp+8]
C1AL14_L2000E9E4:
 call C1AL14_Atan2
C1AL14_L2000E9E9:
 fmul        qword ptr C1AL14_C2009A2C0
C1AL14_L2000E9EF:
 fdiv        qword ptr C1AL14_C2009A288
C1AL14_L2000E9F5:
 fstp        dword ptr [esp]
C1AL14_L2000E9F9:
 fld         dword ptr [esp]
C1AL14_L2000E9FD:
 fcomp       dword ptr C1AL14_C2009A1BC
C1AL14_L2000EA03:
 fnstsw      ax
C1AL14_L2000EA05:
 test        ah,5
C1AL14_L2000EA08:
 jp C1AL14_L2000EA18
C1AL14_L2000EA0A:
 fld         dword ptr [esp]
C1AL14_L2000EA0E:
 fadd        dword ptr C1AL14_C2009A298
C1AL14_L2000EA14:
 fstp        dword ptr [esp]
C1AL14_L2000EA18:
 mov         eax,dword ptr [esp]
C1AL14_L2000EA1C:
 mov         edx,dword ptr [esp+4]
C1AL14_L2000EA20:
 mov         dword ptr [edi],eax
C1AL14_L2000EA22:
 mov         dword ptr [edi+4],edx
C1AL14_L2000EA25:
 mov         dword ptr [edi+8],0
C1AL14_L2000EA2C:
 add         esp,0Ch
C1AL14_L2000EA2F:
 ret
}}
static __declspec(naked) void C1AL14_DeltaRaw(void){__asm {
C1AL14_L20010680:
 fld         dword ptr [esp+4]
C1AL14_L20010684:
 fsub        dword ptr [esp+8]
C1AL14_L20010688:
 fstp        dword ptr [esp+4]
C1AL14_L2001068C:
 fld         dword ptr [esp+4]
C1AL14_L20010690:
 fcomp       dword ptr C1AL14_C2009A1B0
C1AL14_L20010696:
 fnstsw      ax
C1AL14_L20010698:
 test        ah,41h
C1AL14_L2001069B:
 jne C1AL14_L200106BF
C1AL14_L2001069D:
 lea         ecx,[ecx]
C1AL14_L200106A0:
 fld         dword ptr [esp+4]
C1AL14_L200106A4:
 fsub        dword ptr C1AL14_C2009A298
C1AL14_L200106AA:
 fstp        dword ptr [esp+4]
C1AL14_L200106AE:
 fld         dword ptr [esp+4]
C1AL14_L200106B2:
 fcomp       dword ptr C1AL14_C2009A1B0
C1AL14_L200106B8:
 fnstsw      ax
C1AL14_L200106BA:
 test        ah,41h
C1AL14_L200106BD:
 je C1AL14_L200106A0
C1AL14_L200106BF:
 fld         dword ptr [esp+4]
C1AL14_L200106C3:
 fcomp       dword ptr C1AL14_C2009A29C
C1AL14_L200106C9:
 fnstsw      ax
C1AL14_L200106CB:
 test        ah,5
C1AL14_L200106CE:
 jp C1AL14_L200106EF
C1AL14_L200106D0:
 fld         dword ptr [esp+4]
C1AL14_L200106D4:
 fadd        dword ptr C1AL14_C2009A298
C1AL14_L200106DA:
 fstp        dword ptr [esp+4]
C1AL14_L200106DE:
 fld         dword ptr [esp+4]
C1AL14_L200106E2:
 fcomp       dword ptr C1AL14_C2009A29C
C1AL14_L200106E8:
 fnstsw      ax
C1AL14_L200106EA:
 test        ah,5
C1AL14_L200106ED:
 jnp C1AL14_L200106D0
C1AL14_L200106EF:
 fld         dword ptr [esp+4]
C1AL14_L200106F3:
 ret
}}
static int C1AL14_Operation(unsigned kind,const c1al14State_t*in,const float*vector,float a,float b,float c,int now,const float*eye,float yaw,const float*weights,c1al14Result_t*out){
 unsigned char actor[0x220],entity[0xf4],saved[108];uint32_t mx,nearestMx=0x1f80u;const unsigned short cw=0x027f;c1al14Result_t result;C1AL14_Frame frame;unsigned i;int valid=1;uint32_t raw[4];
 if(!in||!out||!C1AL14_StateValid(in)||now<0||now>INT32_MAX-600000||kind<1||kind>4)return 0;
 memcpy(raw,&a,4);memcpy(raw+1,&b,4);memcpy(raw+2,&c,4);memcpy(raw+3,&yaw,4);for(i=0;i<4;i++)if(!C1AL14_Finite(raw[i]))return 0;
 if(kind==1&&(!vector))return 0;if(kind==4&&in->ready){uint32_t yb;memcpy(&yb,&in->yaw,4);if((in->active||(yb&0x7fffffffu))&&(!eye||!weights))return 0;}
 memset(&result,0,sizeof(result));result.state=*in;memset(&frame,0,sizeof(frame));frame.out=&result;memcpy(frame.nodes,in->nodes,12);
 if(vector)for(i=0;i<3;i++){memcpy(raw,vector+i,4);if(!C1AL14_Finite(raw[0])||(raw[0]&0x7fffffffu)>0x49800000u)return 0;}
 if(eye){for(i=0;i<3;i++){memcpy(raw,eye+i,4);if(!C1AL14_Finite(raw[0])||(raw[0]&0x7fffffffu)>0x49800000u)return 0;}memcpy(frame.eye,eye,12);}
 if(weights){for(i=0;i<3;i++){memcpy(raw,weights+i,4);if(((raw[0]>>31)&&(raw[0]&0x7fffffffu))||(raw[0]&0x7fffffffu)>0x3f800000u)return 0;}memcpy(frame.weights,weights,12);}
 memset(actor,0,sizeof(actor));memset(entity,0,sizeof(entity));memcpy(actor+0x1dc,in,64);memcpy(actor+0x21c,&now,4);*(void**)actor=entity;*(void**)(actor+4)=&frame;*(void**)entity=&frame;memcpy(entity+0xf0,&yaw,4);
 __asm fnsave saved
 __asm stmxcsr mx
 __asm fldcw cw
 __asm ldmxcsr nearestMx
 if(kind==1){__asm {push a
 lea ecx,actor
 mov eax,vector
 call C1AL14_TargetRaw
 add esp,4}}
 else if(kind==2){__asm {lea eax,actor
 push esi
 push c
 push b
 push a
 mov esi,eax
 call C1AL14_LimitsRaw
 add esp,12
 pop esi}}
 else if(kind==3){__asm {push a
 lea ecx,actor
 call C1AL14_StopRaw
 add esp,4}}
 else {__asm {lea eax,actor
 push ebx
 mov ebx,eax
 call C1AL14_StepRaw
 pop ebx}}
 memcpy(&result.state,actor+0x1dc,64);
 if(frame.error||!C1AL14_StateValid(&result.state))valid=0;
 for(i=0;i<result.count;i++){c1al14Command_t*q=result.command+i;uint32_t t[3];memcpy(t,&q->weight,12);if(!q->node||q->node>=65536||!C1AL14_Finite(t[0])||!C1AL14_Finite(t[1])||!C1AL14_Finite(t[2])||q->weight<0||q->weight>1||q->blend<0||q->rate<0)valid=0;}
 __asm ldmxcsr mx
 __asm frstor saved
 if(!valid)return 0;*out=result;return 1;
}
static int C1AL14_Target(const c1al14State_t*s,const float target[3],float speed,c1al14Result_t*out){return C1AL14_Operation(1,s,target,speed,0,0,0,NULL,0,NULL,out);}
static int C1AL14_Limits(const c1al14State_t*s,float a,float b,float blend,int now,c1al14Result_t*out){uint32_t u[3];memcpy(u,&a,4);memcpy(u+1,&b,4);memcpy(u+2,&blend,4);if((u[0]>>31)||(u[1]>>31)||(u[2]>>31)||u[0]>0x43340000u||u[1]>0x43340000u||u[2]>0x44160000u)return 0;return C1AL14_Operation(2,s,NULL,a,b,blend,now,NULL,0,NULL,out);}
static int C1AL14_Stop(const c1al14State_t*s,float speed,c1al14Result_t*out){return C1AL14_Operation(3,s,NULL,speed,0,0,0,NULL,0,NULL,out);}
static int C1AL14_Step(const c1al14State_t*s,int now,const float eye[3],float actualYaw,const float currentWeights[3],c1al14Result_t*out){return C1AL14_Operation(4,s,NULL,0,0,0,now,eye,actualYaw,currentWeights,out);}
#else
/* No portable-math fallback is silently selected. */
static int C1AL14_Target(const c1al14State_t*s,const float p[3],float speed,c1al14Result_t*out){(void)s;(void)p;(void)speed;(void)out;return 0;}
static int C1AL14_Limits(const c1al14State_t*s,float a,float b,float t,int now,c1al14Result_t*out){(void)s;(void)a;(void)b;(void)t;(void)now;(void)out;return 0;}
static int C1AL14_Stop(const c1al14State_t*s,float speed,c1al14Result_t*out){(void)s;(void)speed;(void)out;return 0;}
static int C1AL14_Step(const c1al14State_t*s,int now,const float e[3],float yaw,const float w[3],c1al14Result_t*out){(void)s;(void)now;(void)e;(void)yaw;(void)w;(void)out;return 0;}
#endif
#endif
