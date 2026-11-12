#ifndef COD1_NORMAL_HEALTH14_H
#define COD1_NORMAL_HEALTH14_H
#include <stdint.h>
#include <math.h>
/* Retail GAME 20032D90 / 20032CE0, player branch only. The original setter
 * rounds through binary32 before adding double 2^-30 and FISTP under the
 * caller's x87 rounding mode. Do not replace with (int)(x+0.5) or truncation.
 * Finite inputs and positive client maxHealth are the admitted domain.
 * No entity/owner capability is conferred by these numeric helpers. */
static int Cod1NormalHealth14Set(int32_t maximum,float fraction,int32_t *out) {
    if(!out||maximum<=0||!isfinite(fraction)||fraction<=0.0f)return 0;
    if(fraction>1.0f)fraction=1.0f;
#if defined(_MSC_VER) && defined(_M_IX86)
    {
        float product;double epsilon=9.31322574615478515625e-10;
        int32_t health;
        __asm {
            fild maximum
            fmul fraction
            fstp product
            fld product
            fadd epsilon
            fistp health
        }
        *out=health<1?1:health;return 1;
    }
#else
    /* This port's current GAME target is MSVC x86. Refuse rather than guess
     * a different floating-point environment on an unverified platform. */
    return 0;
#endif
}
static int Cod1NormalHealth14Get(int32_t health,int32_t maximum,float *out) {
    if(!out||maximum<=0)return 0;
    if(health==0){*out=0.0f;return 1;}
#if defined(_MSC_VER) && defined(_M_IX86)
    {
        float ratio;
        __asm {
            fild health
            fidiv maximum
            fstp ratio
        }
        if(!isfinite(ratio))return 0;
        *out=ratio;return 1;
    }
#else
    return 0;
#endif
}
#endif
