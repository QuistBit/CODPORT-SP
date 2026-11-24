#ifndef COD1_EXACT_BYTES_R242_H
#define COD1_EXACT_BYTES_R242_H
#include <stddef.h>
#include <string.h>
/* Boolean equality over valid byte spans. No cache, hash, sampling or overread. */
#if defined(_MSC_VER) && (defined(_M_X64) || (defined(_M_IX86_FP) && _M_IX86_FP >= 2))
#define C1EXACT242_VECTOR_PATH 1
#include <emmintrin.h>
__declspec(noinline) static int R242ExactSSEBytesEqual(const void *va,const void *vb,size_t n) {
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
static int R242ExactSSEBytesEqual(const void *a,const void *b,size_t n){return memcmp(a,b,n)==0;}
#endif
/* Read every byte, retaining the existing SSE2 path on unsupported systems.
 * This is boolean equality, not an ordering comparison. No data cache, hash,
 * sampled reads, alignment requirement, or access beyond either span. */
#if defined(_MSC_VER) && _MSC_VER >= 1800 && (defined(_M_X64) || defined(_M_IX86))
#include <intrin.h>
#include <immintrin.h>
static int R325_Avx2Bits(unsigned maximum,unsigned ecx,unsigned long long xcr0,unsigned ebx){
 return maximum>=7 && (ecx&0x18000000u)==0x18000000u && (xcr0&6)==6 && (ebx&32)!=0;
}
static int R325_HasAvx2(void){
 static volatile long detected;long value=detected;int info[4];unsigned maximum,ecx,ebx;unsigned long long xcr0;
 if(value)return value==2;
 __cpuid(info,0);maximum=(unsigned)info[0];value=1;
 if(maximum>=7){
  __cpuid(info,1);ecx=(unsigned)info[2];
  if((ecx&0x18000000u)==0x18000000u){
   xcr0=_xgetbv(0);__cpuidex(info,7,0);ebx=(unsigned)info[1];
   if(R325_Avx2Bits(maximum,ecx,xcr0,ebx))value=2;
  }
 }
 _InterlockedCompareExchange(&detected,value,0);return value==2;
}
__declspec(noinline) static int R325_Avx2Equal(const unsigned char*a,const unsigned char*b,size_t n){
 while(n>=256){
  __m256i d0=_mm256_xor_si256(_mm256_loadu_si256((const __m256i*)a),_mm256_loadu_si256((const __m256i*)b));
  __m256i d1=_mm256_xor_si256(_mm256_loadu_si256((const __m256i*)(a+32)),_mm256_loadu_si256((const __m256i*)(b+32)));
  __m256i d2=_mm256_xor_si256(_mm256_loadu_si256((const __m256i*)(a+64)),_mm256_loadu_si256((const __m256i*)(b+64)));
  __m256i d3=_mm256_xor_si256(_mm256_loadu_si256((const __m256i*)(a+96)),_mm256_loadu_si256((const __m256i*)(b+96)));
  d0=_mm256_or_si256(d0,_mm256_xor_si256(_mm256_loadu_si256((const __m256i*)(a+128)),_mm256_loadu_si256((const __m256i*)(b+128))));
  d1=_mm256_or_si256(d1,_mm256_xor_si256(_mm256_loadu_si256((const __m256i*)(a+160)),_mm256_loadu_si256((const __m256i*)(b+160))));
  d2=_mm256_or_si256(d2,_mm256_xor_si256(_mm256_loadu_si256((const __m256i*)(a+192)),_mm256_loadu_si256((const __m256i*)(b+192))));
  d3=_mm256_or_si256(d3,_mm256_xor_si256(_mm256_loadu_si256((const __m256i*)(a+224)),_mm256_loadu_si256((const __m256i*)(b+224))));
  d0=_mm256_or_si256(_mm256_or_si256(d0,d1),_mm256_or_si256(d2,d3));
  if(!_mm256_testz_si256(d0,d0)){_mm256_zeroupper();return 0;}
  a+=256;b+=256;n-=256;
 }
 while(n>=32){
  __m256i d=_mm256_xor_si256(_mm256_loadu_si256((const __m256i*)a),_mm256_loadu_si256((const __m256i*)b));
  if(!_mm256_testz_si256(d,d)){_mm256_zeroupper();return 0;}
  a+=32;b+=32;n-=32;
 }
 _mm256_zeroupper();return R242ExactSSEBytesEqual(a,b,n);
}
#endif
#ifdef _MSC_VER
__declspec(noinline)
#endif
static int R242ExactATRBytesEqual(const void*a,const void*b,size_t n){
#if defined(_MSC_VER) && _MSC_VER >= 1800 && (defined(_M_X64) || defined(_M_IX86))
 if(n>=1024 && R325_HasAvx2())return R325_Avx2Equal((const unsigned char*)a,(const unsigned char*)b,n);
#endif
 return R242ExactSSEBytesEqual(a,b,n);
}
#endif
