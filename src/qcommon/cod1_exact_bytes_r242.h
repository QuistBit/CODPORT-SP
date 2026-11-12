#ifndef COD1_EXACT_BYTES_R242_H
#define COD1_EXACT_BYTES_R242_H
#include <stddef.h>
#include <string.h>
/* Boolean equality over valid byte spans. No cache, hash, sampling or overread. */
#if defined(_MSC_VER) && (defined(_M_X64) || (defined(_M_IX86_FP) && _M_IX86_FP >= 2))
#define C1EXACT242_VECTOR_PATH 1
#include <emmintrin.h>
__declspec(noinline) static int R242ExactATRBytesEqual(const void *va,const void *vb,size_t n) {
    const unsigned char *a=(const unsigned char *)va,*b=(const unsigned char *)vb;
    const __m128i zero=_mm_setzero_si128();
    while(n>=64) {
        __m128i d0=_mm_xor_si128(_mm_loadu_si128((const __m128i *)(a)),_mm_loadu_si128((const __m128i *)(b)));
        __m128i d1=_mm_xor_si128(_mm_loadu_si128((const __m128i *)(a+16)),_mm_loadu_si128((const __m128i *)(b+16)));
        __m128i d2=_mm_xor_si128(_mm_loadu_si128((const __m128i *)(a+32)),_mm_loadu_si128((const __m128i *)(b+32)));
        __m128i d3=_mm_xor_si128(_mm_loadu_si128((const __m128i *)(a+48)),_mm_loadu_si128((const __m128i *)(b+48)));
        __m128i different=_mm_or_si128(_mm_or_si128(d0,d1),_mm_or_si128(d2,d3));
        if(_mm_movemask_epi8(_mm_cmpeq_epi8(different,zero))!=65535)return 0;
        a+=64;b+=64;n-=64;
    }
    while(n>=16) {
        __m128i different=_mm_xor_si128(_mm_loadu_si128((const __m128i *)a),_mm_loadu_si128((const __m128i *)b));
        if(_mm_movemask_epi8(_mm_cmpeq_epi8(different,zero))!=65535)return 0;
        a+=16;b+=16;n-=16;
    }
    while(n){if(*a++!=*b++)return 0;n--;}
    return 1;
}

#else
#define C1EXACT242_VECTOR_PATH 0
static int R242ExactATRBytesEqual(const void *a,const void *b,size_t n){return memcmp(a,b,n)==0;}
#endif
#endif
