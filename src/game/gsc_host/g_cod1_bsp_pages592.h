/* Private page-aligned BSP fields: immutable after construction, writable only
 * during owner reset/replacement. Failed protection keeps full comparisons. */
#ifndef COD1_BSP_PAGES592_H
#define COD1_BSP_PAGES592_H
#include <stdint.h>
#include <stddef.h>
#ifdef _MSC_VER
#define C1BSP592_ALIGN __declspec(align(4096))
#else
#define C1BSP592_ALIGN _Alignas(4096)
#endif
#if defined(_WIN32) && defined(_M_IX86) && !defined(Q3_VM)
#if !defined(_WINDOWS_)
__declspec(dllimport) int __stdcall VirtualProtect(void *,size_t,unsigned long,unsigned long *);
#endif
static int C1Bsp592Pages(void *p,size_t n,int writable){
 unsigned long old;
 if(!p||!n||((uintptr_t)p&4095)||(n&4095)||n>UINTPTR_MAX-(uintptr_t)p)return 0;
 return VirtualProtect(p,n,writable?0x04:0x02,&old)!=0;
}
#else
static int C1Bsp592Pages(void *p,size_t n,int writable){(void)p;(void)n;(void)writable;return 0;}
#endif
uint64_t G_Cod1Bsp592FieldsSealed(const char *,size_t,size_t,const unsigned short *,const unsigned short *);
int G_Cod1Bsp592SealPrivate(const void *,size_t);
#endif
