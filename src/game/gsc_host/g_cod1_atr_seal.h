/* Map-owned catalog bytes, copied and made read-only before any address is
 * published. This is a separate owner from the writable WriteWatch fallback.
 * The epoch/receipt/resource checks remain at every certificate boundary. */
#ifndef COD1_ATR_SEAL_H
#define COD1_ATR_SEAL_H
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "g_cod1_atr_write_watch.h"
#if defined(_WIN32) && !defined(Q3_VM) && !defined(_WINDOWS_)
__declspec(dllimport) int __stdcall VirtualProtect(void *,size_t,unsigned long,unsigned long *);
#endif
typedef struct {const unsigned char *base;size_t length;} Cod1ATRSeal;
static int Cod1ATRSealCreate(Cod1ATRSeal *seal,const unsigned char *bytes,size_t length){
#if defined(_WIN32) && !defined(Q3_VM)
 unsigned char *base;unsigned long old;
 if(!seal||seal->base||seal->length||!bytes||!length||length>SIZE_MAX/2)return 0;
 base=(unsigned char*)VirtualAlloc(NULL,2*length,0x3000,0x04);
 if(!base)return 0;
 memcpy(base,bytes,length);memcpy(base+length,bytes,length);
 if(!R242ExactATRBytesEqual(base,base+length,length)||!VirtualProtect(base,2*length,0x02,&old)){
  VirtualFree(base,0,0x8000);return 0;
 }
 seal->base=base;seal->length=length;return 1;
#else
 (void)seal;(void)bytes;(void)length;return 0;
#endif
}
static void Cod1ATRSealFree(Cod1ATRSeal *seal){
 if(!seal)return;
#if defined(_WIN32) && !defined(Q3_VM)
 if(seal->base)VirtualFree((void*)seal->base,0,0x8000);
#endif
 memset(seal,0,sizeof(*seal));
}
static int Cod1ATRSealCurrent(const Cod1ATRSeal *seal,const unsigned char *bytes,const unsigned char *reference,size_t length){
 return seal&&seal->base&&seal->base==bytes&&length&&length<=SIZE_MAX/2&&seal->length==length&&
  (uintptr_t)seal->base<=UINTPTR_MAX-2*length&&seal->base+length==reference;
}
#endif
