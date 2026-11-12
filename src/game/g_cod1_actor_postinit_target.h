#ifndef G_COD1_ACTOR_POSTINIT_TARGET_H
#define G_COD1_ACTOR_POSTINIT_TARGET_H
#include "g_cod1_actor_postinit_position.h"
#include "g_cod1_native_numeric.h"
/* 736A0 numeric branch ->739A0, empty path/no node/no enemy.
 * Native classification is on the FIRST byte, including explicit minus/dot, before whitespace or plus.
 * Decimal finite subset only; named targets and other CRT formats stay open.
 * Successful parsing is not an Actor ownership or script-entry certificate. */
static int C1TargetSpace(unsigned char c){return c==' '||c=='\t'||c=='\r'||c=='\n'||c=='\v'||c=='\f';}
static int C1TargetDigit(unsigned char c){return c>='0'&&c<='9';}
static int C1TargetParse(const char*text,float out[3]){
 unsigned i,n=0;const char*p;float v[3];const char*start[3],*end[3];
 if(!text||!out||(!C1TargetDigit((unsigned char)text[0])&&text[0]!='-'&&text[0]!='.'))return 0;
 while(n<1024&&text[n])++n;if(n==1024)return 0;
 p=text;
 for(i=0;i<3;i++){
  int digits=0;while(C1TargetSpace((unsigned char)*p))++p;
  start[i]=p;if(*p=='+'||*p=='-')++p;
  while(C1TargetDigit((unsigned char)*p)){++p;++digits;}
  if(*p=='.'){++p;while(C1TargetDigit((unsigned char)*p)){++p;++digits;}}
  if(!digits||*p=='x'||*p=='X')return 0;
  if(*p=='e'||*p=='E'){++p;if(*p=='+'||*p=='-')++p;
   if(!C1TargetDigit((unsigned char)*p))return 0;
   while(C1TargetDigit((unsigned char)*p))++p;}
  end[i]=p;
 }
 for(i=0;i<3;i++){
  unsigned char native[12];uint32_t bits;
  if(!C1NDecimal(start[i],end[i],native))return 0;
  C1LD12Float(native,&bits);memcpy(v+i,&bits,4);
 }
 for(i=0;i<3;i++)if(!isfinite(v[i]))return 0;
 memcpy(out,v,sizeof(v));return 1;
}
#if defined(_MSC_VER) && defined(_M_IX86)
/* Exact saved 20051EB0 x87 sequence; no movement or completion event. */
static const uint32_t c1TargetZLimit=UINT32_C(0x45c80000);
__declspec(naked) static int C1TargetRange(const float*current,const float*goal,float radius){
 __asm {
  fld dword ptr [esp+12]
  mov eax,[esp+4]
  mov ecx,[esp+8]
  fld dword ptr [eax]
  fsub dword ptr [ecx]
  fld dword ptr [eax+4]
  fsub dword ptr [ecx+4]
  fld dword ptr [eax+8]
  fsub dword ptr [ecx+8]
  fld st(0)
  fmul st,st(1)
  fcomp dword ptr [c1TargetZLimit]
  fnstsw ax
  fstp st(0)
  test ah,41h
  je zOutside
  fld st(0)
  fmul st,st(1)
  fld st(2)
  fmul st,st(3)
  faddp st(1),st
  fld st(3)
  fmul st,st(4)
  fcompp
  fstp st(0)
  fnstsw ax
  fstp st(0)
  test ah,5
  fstp st(0)
  jnp outside
  mov eax,1
  ret
 zOutside:
  fstp st(0)
  fstp st(0)
  fstp st(0)
 outside:
  xor eax,eax
  ret
 }
}

#endif
static int C1PostinitTargetApply(C1PostinitPosition*s,const float target[3]){
 C1PostinitPosition v;int changed=0;unsigned i;
 if(!s||!target)return 0;v=*s;
 if(v.node40||v.node4c||v.enemy||(int16_t)v.path6f4||v.cursor6fa||
    !isfinite(v.radius)||v.radius<0)return 0;
 for(i=0;i<3;i++){
  if(!isfinite(v.position[i])||!isfinite(v.goal[i])||!isfinite(target[i]))return 0;
  if(v.goal[i]!=target[i])changed=1;
 }
 v.cell20=-1.0f;
 if(changed){
#if defined(_MSC_VER) && defined(_M_IX86)
  memcpy(v.goal,target,12);v.node8=0;v.path6f4=0;v.cell774=0;v.cell7e4=0;
  v.cell78f=(unsigned char)C1TargetRange(v.position,target,v.radius);
#else
  return 0;
#endif
 }
 *s=v;return 1;
}
/* 73810/73A80 non-Sentient target, after typed owner admission. Repeated
 * identity is the native immediate no-store path. cell20/78F are untouched. */
static int C1PostinitEntityTargetApply(C1PostinitPosition*s,const float target[3],int repeated){
 C1PostinitPosition v;unsigned i;
 if(!s)return 0;if(repeated)return 1;if(!target)return 0;v=*s;
 if(v.node40||v.node4c||v.enemy||(int16_t)v.path6f4||v.cursor6fa)return 0;
 for(i=0;i<3;i++)if(!isfinite(target[i]))return 0;
 memcpy(v.goal,target,12);v.node8=1;v.path6f4=0;v.cell774=0;v.cell7e4=0;*s=v;return 1;
}
#endif
