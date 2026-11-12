#ifndef COD1_IMAGE_MULTIPLY_R65_H
#define COD1_IMAGE_MULTIPLY_R65_H

/* Native multiplyImage identityLighting, bounded to the actual native
 * identity-light producer: 0x4e59d0 clamps overbright bits to 0..2 and
 * 0x4e5a32..0x4e5a55 computes 1 / (1 << bits).  The parser expands its
 * single argument into RGB={identity,identity,identity}, alpha=1 at
 * 0x4ee711..0x4ee779.  Upload wrapper 0x4e16f0..0x4e1866 multiplies,
 * clamps and truncates each decoded byte before mip/gamma/intensity.
 * For these powers of two, truncation is exactly an integer right shift.
 * This helper operates on decoded private image scratch, never on a live
 * image or a framebuffer.  Decode/upload/cache ownership belongs to caller.
 */
#include <stddef.h>

static int Cod1IM_IdentityShift(float identityLight, int *shift) {
    int value;
    if (identityLight == 1.0f) value = 0;
    else if (identityLight == 0.5f) value = 1;
    else if (identityLight == 0.25f) value = 2;
    else return 0;
    if (shift) *shift = value;
    return 1;
}

static int Cod1IM_MultiplyIdentity(unsigned char *rgba, size_t pixelCount,
                                 float identityLight) {
    size_t i;
    int shift;
    if (!rgba || !pixelCount || pixelCount > ((size_t)-1) / 4 ||
        !Cod1IM_IdentityShift(identityLight, &shift)) return 0;
    if (!shift) return 1;
    for (i = 0; i < pixelCount; ++i, rgba += 4) {
        rgba[0] = (unsigned char)(rgba[0] >> shift);
        rgba[1] = (unsigned char)(rgba[1] >> shift);
        rgba[2] = (unsigned char)(rgba[2] >> shift);
    }
    return 1;
}

#endif
