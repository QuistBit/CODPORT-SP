/* Native CPU soup normal availability: 0x50343f..0x503493 and
 * dynamic projection choice: 0x4f4ce0..0x4f4cf8. Pure C89.
 * This selects already supplied normals; it does not manufacture UV tangents
 * or replace the separate optimized native cached TBN route.
 */
#ifndef COD1_PERLIGHT_BASIS_R64_H
#define COD1_PERLIGHT_BASIS_R64_H

/* Native checks the aggregate batch mask after ORing this CPU soup's mask.
 * Literal material bit 2 and normal-array copying are separate producers. */
static int Cod1PLR64_TriangleNormalsRequired(int materialNeedsNormals,
    int quality, unsigned int batchLightMask) {
    return materialNeedsNormals != 0 ||
        (quality != 0 && batchLightMask != 0U);
}

/* The projector consumes tess normal availability, rather than re-reading
 * the material's requirement bit. Quality zero retains native plain XY. */
static int Cod1PLR64_ProjectionUsesBasis(int quality, int normalsAvailable) {
    return quality != 0 && normalsAvailable != 0;
}

#endif
