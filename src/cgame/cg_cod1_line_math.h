/* SPDX-License-Identifier: GPL-3.0-or-later
 * MUZZLE_R3: independently expressed Line geometry, CoDSP 0x502350/0x502110.
 * Size is HALF width; Line connects two explicit points, unlike Tail.
 * Existing RT_POLY uses perimeter order, so map original 0,1,2,3 to 0,1,3,2.
 * No renderer ABI/GL-state change. No original binary or assets embedded.
 */
#ifndef CG_COD1_LINE_MATH_H
#define CG_COD1_LINE_MATH_H
#include <math.h>
#include <float.h>
static int ceMuzzleLineQuad(const float first[3],const float second[3],
    const float camera[3],float halfWidth,float center[3],float left[3],float up[3]) {
    float a[3],b[3],side[3],squared,scale;int i;
    if(!(halfWidth>0.0f) || !(halfWidth<=FLT_MAX))return 0;
    for(i=0;i<3;i++) {a[i]=first[i]-camera[i];b[i]=second[i]-camera[i];}
    side[0]=a[1]*b[2]-a[2]*b[1];
    side[1]=a[2]*b[0]-a[0]*b[2];
    side[2]=a[0]*b[1]-a[1]*b[0];
    squared=side[0]*side[0]+side[1]*side[1]+side[2]*side[2];
    /* The original zero-vector normalizer invents no fallback direction.
     * A degenerate strip has zero area: omit submission rather than emit NaN
     * or manufacture a bright camera-facing streak. */
    if(!(squared>0.0f) || !(squared<=FLT_MAX))return 0;
    scale=halfWidth/sqrtf(squared);
    for(i=0;i<3;i++) {
        center[i]=(first[i]+second[i])*0.5f;
        left[i]=side[i]*scale;
        up[i]=(first[i]-second[i])*0.5f;
    }
    return 1;
}
#endif
