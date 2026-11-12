/* Valid only for a privately initialized pair of equal byte spans. Windows
 * tracks writes to both copies for the rest of the allocation's lifetime.
 * After any write (or an unavailable query), use the exact byte comparison.
 * Never reset the dirty state after publishing addresses to callers. */
#ifndef G_COD1_ATR_WRITE_WATCH_H
#define G_COD1_ATR_WRITE_WATCH_H
#include <stdint.h>
#include <stddef.h>
#include <string.h>
typedef struct { unsigned char *base; size_t length; } Cod1ATRWatch;

#if defined(_WIN32) && !defined(Q3_VM)
#ifndef _WINDOWS_
__declspec(dllimport) void *__stdcall VirtualAlloc(void *,size_t,unsigned long,unsigned long);
__declspec(dllimport) int __stdcall VirtualFree(void *,size_t,unsigned long);
__declspec(dllimport) unsigned int __stdcall ResetWriteWatch(void *,size_t);
__declspec(dllimport) unsigned int __stdcall GetWriteWatch(unsigned long,void *,size_t,void **,uintptr_t *,unsigned long *);
#endif

enum { C1_WATCH_COMMIT=0x1000, C1_WATCH_RESERVE=0x2000, C1_WATCH_RELEASE=0x8000,
       C1_WRITE_WATCH=0x200000, C1_PAGE_READWRITE=4 };

static int Cod1ATRWatchCreate(Cod1ATRWatch *watch,const unsigned char *bytes,size_t length) {
    /* The 526-byte MG42 catalog is cheaper to compare directly. */
    unsigned char *base;
    if(!bytes || length<4096 || length>SIZE_MAX/2) return 0;
    base=(unsigned char *)VirtualAlloc(NULL,2*length,C1_WRITE_WATCH|C1_WATCH_RESERVE|C1_WATCH_COMMIT,C1_PAGE_READWRITE);
    if(!base) return 0;
    memcpy(base,bytes,length);
    memcpy(base+length,bytes,length);
    if(!R242ExactATRBytesEqual(base,base+length,length) || ResetWriteWatch(base,2*length)){
        VirtualFree(base,0,C1_WATCH_RELEASE);
        return 0;
    }
    watch->base=base;
    watch->length=length;
    return 1;
}

static void Cod1ATRWatchFree(Cod1ATRWatch *watch) {
    if(watch->base) VirtualFree(watch->base,0,C1_WATCH_RELEASE);
    memset(watch,0,sizeof(*watch));
}

static int Cod1ATRWatchClean(const Cod1ATRWatch *watch,const unsigned char *bytes,
                             const unsigned char *reference,size_t length) {
    void *changed;
    uintptr_t count=1;
    unsigned long granularity;
    if(!watch->base || watch->base!=bytes || watch->length!=length ||
       watch->base+length!=reference) return 0;
    return GetWriteWatch(0,watch->base,2*length,&changed,&count,&granularity)==0 && count==0;
}
#else
static int Cod1ATRWatchCreate(Cod1ATRWatch *watch,const unsigned char *bytes,size_t length) {
    (void)watch;(void)bytes;(void)length;return 0;
}
static void Cod1ATRWatchFree(Cod1ATRWatch *watch) {(void)watch;}
static int Cod1ATRWatchClean(const Cod1ATRWatch *watch,const unsigned char *bytes,
                             const unsigned char *reference,size_t length) {
    (void)watch;(void)bytes;(void)reference;(void)length;return 0;
}
#endif
#endif
