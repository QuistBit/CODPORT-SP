#ifndef MT_TEST_INTRIN_H
#define MT_TEST_INTRIN_H
/* Only the MSVC CPUID spelling is adapted. The production SSE kernels run
 * using the compiler's real x86 intrinsics. */
#include <cpuid.h>
static inline void mt_test_cpuid(int result[4],int leaf) {
    unsigned a=0,b=0,c=0,d=0;
    __get_cpuid((unsigned)leaf,&a,&b,&c,&d);
    result[0]=(int)a;result[1]=(int)b;result[2]=(int)c;result[3]=(int)d;
}
#undef __cpuid
#define __cpuid(result,leaf) mt_test_cpuid((result),(leaf))
#endif
