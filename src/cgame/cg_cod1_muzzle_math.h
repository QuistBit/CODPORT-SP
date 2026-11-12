/* SPDX-License-Identifier: GPL-3.0-or-later
 * MUZZLE_R2. Independently expressed, bounded subset of the inspected CoDSP
 * update rules. See docs/reverse/MUZZLE_R2.md for addresses and limitations.
 * Only the weapon EFX mode calls these; accepted grenade playback is unchanged.
 */
#ifndef CG_COD1_MUZZLE_MATH_H
#define CG_COD1_MUZZLE_MATH_H
static float ceMuzzleRandomUnit(unsigned seed,unsigned time,unsigned salt) {
    unsigned x=seed^time*0x9e3779b9U^salt;
    x^=x>>16;x*=0x7feb352dU;x^=x>>15;x*=0x846ca68bU;x^=x>>16;
    /* Retain our deterministic redraw policy, NOT the original CRT RNG. */
    return (x>>8)*(1.0f/16777216.0f);
}
static float ceMuzzleCurveBias(int linear,int random,float t,float unit) {
    float remaining=linear?1.0f-t:1.0f;
    if(random)remaining*=unit;
    return 1.0f-remaining;
}
static float ceMuzzleAlpha(float start,float end,int linear,int random,float t,float unit) {
    float alpha=start+(end-start)*(linear?t:0.0f);
    if(alpha<0.0f)alpha=0.0f;else if(alpha>1.0f)alpha=1.0f;
    /* Alpha randomness modulates the CLAMPED result, not the blend weight. */
    if(random)alpha*=unit;
    return alpha;
}
static void ceMuzzleRotate(float *angle,float *delta,float milliseconds) {
    if(milliseconds<=0.0f)return;
    *angle+=milliseconds*(*delta)*0.01f;
    *delta*=1.0f-milliseconds*0.00065f;
}
#endif
