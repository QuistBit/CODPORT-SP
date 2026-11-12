/* CoDSP af73842d..., VA 4ee0b5..4ee159:
 * constLighting first stores float32(identityLight * authored float), then
 * truncates that value times 255 and stores the low byte. The ordinary const
 * command skips the identityLight multiply. No normal/instance light input.
 * Reject nonfinite/overflow inputs instead of invoking undefined C casts.
 */
#ifndef COD1_CONST_LIGHTING_H
#define COD1_CONST_LIGHTING_H
#include <float.h>
#include <limits.h>

static int C1CL_Pack(const float color[3],float identityLight,
                     unsigned char out[3]) {
    unsigned char candidate[3];
    int i;
    if (!(identityLight>=0.0f && identityLight<=FLT_MAX)) return 0;
    for (i=0;i<3;i++) {
        volatile float scaled;
        double product;
        int packed;
        if (!(color[i]>=-FLT_MAX && color[i]<=FLT_MAX)) return 0;
        /* Native fstp dword is an observable rounding boundary. */
        product=(double)identityLight*(double)color[i];
        if (!(product>=-FLT_MAX && product<=FLT_MAX)) return 0;
        scaled=(float)product;
        if (!(scaled>=-FLT_MAX && scaled<=FLT_MAX)) return 0;
        product=(double)scaled*255.0;
        if (!(product>=(double)INT_MIN && product<=(double)INT_MAX)) return 0;
        packed=(int)product;
        candidate[i]=(unsigned char)((unsigned int)packed&255u);
    }
    for (i=0;i<3;i++) out[i]=candidate[i];
    return 1;
}
#endif
