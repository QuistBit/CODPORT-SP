#ifndef SND_COD1_ALIAS_H
#define SND_COD1_ALIAS_H
/* Small deterministic helpers shared by the mixer and its regression test.
 * Pitch is source samples consumed per output sample: 2.0 shortens a sound,
 * 0.5 lengthens it.  They deliberately know nothing about alias selection or
 * channel allocation. */
static ID_INLINE float S_Cod1AliasSourcePosition(int sampleOffset,float pitch) {
    return (float)sampleOffset*pitch;
}
static ID_INLINE int S_Cod1AliasSamplesRemaining(int soundLength,int sampleOffset,float pitch) {
    float src;
    if(soundLength<=0||sampleOffset<0||!isfinite(pitch)||pitch<=0)return 0;
    src=S_Cod1AliasSourcePosition(sampleOffset,pitch);
    if(src>=(float)soundLength)return 0;
    return (int)ceilf(((float)soundLength-src)/pitch);
}
static ID_INLINE qboolean S_Cod1AliasStartAllowed(float distance,float distMax) {
    if(!isfinite(distance)||distance<0)return qfalse;
    if(!isfinite(distMax)||distMax<=0)return qtrue;
    return distance<=distMax ? qtrue : qfalse;
}
/* RTCW's spatializer reaches zero at range + 6.4% of range.
 * Until the exact CoD1 attenuation curve below distMax is recovered, scale
 * that already-established curve so its zero point matches the retail alias
 * maximum distance instead of silently falling back to the fixed 1250-unit
 * RTCW range.  distMin is retained by the channel for future native recovery. */
static ID_INLINE float S_Cod1AliasRtcwRange(float distMin,float distMax,float fallbackRange) {
    float range;
    (void)distMin;
    if(!isfinite(fallbackRange)||fallbackRange<=0)fallbackRange=1250.0f;
    if(!isfinite(distMax)||distMax<=0)return fallbackRange;
    range=distMax/1.064f;
    return (isfinite(range)&&range>0)?range:fallbackRange;
}
static ID_INLINE int S_Cod1AliasLerp16(short a,short b,float fraction) {
    if(fraction<0)fraction=0;if(fraction>1)fraction=1;
    return (int)((1.0f-fraction)*(float)a+fraction*(float)b);
}
#endif
