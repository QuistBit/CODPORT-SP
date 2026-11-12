#ifndef BG_COD1_LEAN_H
#define BG_COD1_LEAN_H
#include "bg_cod1_stance.h"
/* PATCH50: normalized CoD lean uses the existing networked stats/leanf fields.
 * The mode bit prevents RTCW consumers treating a fraction as world units. */
#define COD1_LEAN_ACTIVE 32
static int BG_Cod1LeanActive(const playerState_t *ps) {
    return !!(ps->stats[STAT_COD1_EDGES]&COD1_LEAN_ACTIVE);
}
static float COD1_LeanAdd(float a,float b) {volatile float f=a+b;return f;}
static float COD1_LeanMul(float a,float b) {volatile float f=a*b;return f;}
static float COD1_LeanCurve(float lean) {
    return COD1_LeanMul(COD1_LeanAdd(2,-(float)fabs(lean)),lean);
}
/* cgame 0x300331b0 / game 0x2004b710. A yaw-only right vector with
 * curve*rollDegrees roll, scaled by curve*distance. No camera angle change. */
static void COD1_LeanOffset(float point[3],float yaw,float lean,float rollDegrees,float distance) {
    float curve,angle,sy,cy,sr,cr,amount;
    if(!lean)return;
    curve=COD1_LeanCurve(lean);
    angle=COD1_LeanMul(yaw,0.01745329238474369f);
    sy=(float)sin(angle);cy=(float)cos(angle);
    angle=COD1_LeanMul(COD1_LeanMul(curve,rollDegrees),0.01745329238474369f);
    sr=(float)sin(angle);cr=(float)cos(angle);amount=COD1_LeanMul(curve,distance);
    point[0]=COD1_LeanAdd(point[0],COD1_LeanMul(COD1_LeanMul(cr,sy),amount));
    point[1]=COD1_LeanAdd(point[1],COD1_LeanMul(COD1_LeanMul(-cr,cy),amount));
    point[2]=COD1_LeanAdd(point[2],COD1_LeanMul(-sr,amount));
}
static float COD1_LeanStep(float lean,int direction,float limit,int msec) {
    float step=COD1_LeanMul(COD1_LeanMul((float)msec,direction?0.0028571428265422583f:0.0035714285913854837f),limit);
    if(direction>0){if(lean<limit)lean=COD1_LeanAdd(lean,step);if(lean>limit)lean=limit;}
    else if(direction<0){if(lean>-limit)lean=COD1_LeanAdd(lean,-step);if(lean<-limit)lean=-limit;}
    else if(lean>0){lean=COD1_LeanAdd(lean,-step);if(lean<0)lean=0;}
    else if(lean<0){lean=COD1_LeanAdd(lean,step);if(lean>0)lean=0;}
    return lean;
}
static float COD1_LeanClip(float lean,float fraction) {
    float allowed;
    if(fraction<0)fraction=0;if(fraction>1)fraction=1;
    allowed=COD1_LeanAdd(1,-(float)sqrt(COD1_LeanAdd(1,-fraction)));
    if((float)fabs(lean)>allowed)lean=lean<0?-allowed:allowed;
    return lean;
}
/* Retail stance classifier 0x30002c10 retains prone during rise-to-crouch. */
static float BG_Cod1LeanLimit(const playerState_t *ps) {
    int target=ps->cod1EyeTarget?ps->cod1EyeTarget:ps->viewheight-(int)ps->mins[2];
    if((ps->pm_flags&PMF_COD1_PRONE) || target==11 ||
       (ps->cod1EyeStart && target==40 && !ps->cod1EyeDown))return .25f;
    if((ps->pm_flags&PMF_DUCKED) || (ps->cod1EyeStart && target==40))return .5f;
    return 1;
}
static float BG_Cod1LeanEyeHeight(const playerState_t *ps) {
    return ps->cod1EyeTarget?ps->cod1EyeHeight+ps->mins[2]:(float)ps->viewheight;
}
#endif
