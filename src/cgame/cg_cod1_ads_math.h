/* PATCH47: retail ADS leaf weights and FOV curve lifetime.
 * Verified against CoDSP 0x47dd90/0x47ac70 and cgame 0x3002a107/0x30025200.
 * A clip's position, its weight and the zoom curve are separate state. */
#ifndef CG_COD1_ADS_MATH_H
#define CG_COD1_ADS_MATH_H
#include <math.h>
static float COD1_ADSWeight(float weight,float goal,float dt) {
    float time=(float)fabs(goal-weight)*.5f;
    if(time<.001f)return goal;
    if(weight==0)weight=goal*.001f;
    if(dt+.001f>time)return goal;
    weight+=(goal-weight)/time*dt;
    if(weight<1.0000001111620804e-6f)weight=goal*.001f;
    return weight;
}
typedef struct {float previous;int rising;} CodADSZoom;
static void COD1_ADSZoomUpdate(CodADSZoom *s,float ads) {
    int direction=s->rising;
    if(ads>0 && ads<1) {
        if(ads!=s->previous)direction=ads>s->previous;
        if(s->previous==0 || s->previous==1)s->rising=direction;
    }
    s->previous=ads;
}
static float COD1_ADSZoomFov(const CodADSZoom *s,float ads,float base,
                            float aimed,float inFrac,float outFrac) {
    float span,t;
    if(ads<=0)return base;
    if(ads>=1)return aimed;
    span=s->rising?inFrac:outFrac;
    if(span<=0)return base;
    t=(ads-(1-span))/span;
    if(t<=0)return base;
    return base+(aimed-base)*t;
}
#endif
