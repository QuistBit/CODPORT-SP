/* R52: packed instance colour for CoD1 rgbGen lightingPrecalc.
 * CoDSP SHA256 af73842d..., VA 4d203f..4d208f -> 4fa170..4fa181 ->
 * 4fa3d7..4fa52b. The final consumers read currentEntity+0x6c, not
 * normal-dependent diffuse vertex light. See research/material_precalc_r52.
 * Renderer storage/transport only: no family names, BSP rewrite or minlight.
 */
#ifndef COD1_PRECALC_H
#define COD1_PRECALC_H
#include <stddef.h>
static unsigned char C1Precalc_Byte(float value, float identityLight) {
    /* Original x87 keeps both multiplies before its single float store.
     * identityLight is a renderer power of two. Clamp AFTER this scaling.
     * Bounded conversion also handles malformed nonfinite inputs safely. */
    volatile float scaled = (float)((double)value * (double)identityLight * 255.0);
    if (!(scaled > 0.0f)) return 0;
    if (scaled >= 255.0f) return 255;
    return (unsigned char)scaled;
}
static void C1Precalc_Pack(const float value[3], float identityLight,
                          unsigned char rgba[4]) {
    int k;
    for (k=0; k<3; ++k) rgba[k]=C1Precalc_Byte(value[k],identityLight);
    rgba[3]=255;
}
#endif
