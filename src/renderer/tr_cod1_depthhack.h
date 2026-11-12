#ifndef TR_COD1_DEPTHHACK_H
#define TR_COD1_DEPTHHACK_H
/* PATCH45: ordinary CoD1 view-weapon projection (CoDSP 0x4c9c08).
 * Preserve the current horizontal/vertical projection, including any offset.
 * Use the original infinite-far coefficients with the weapon's own near plane;
 * native BSP59 world views also use infinite far in the ordinary projection. */
static void R_Cod1DepthHackProjection(const float world[16],float nearPlane,
                                    float weapon[16]) {
    int i;
    for(i=0;i<16;i++)weapon[i]=world[i];
    weapon[10]=-1.0f;weapon[11]=-1.0f;
    weapon[14]=-2.0f*nearPlane;weapon[15]=0.0f;
}
#endif
