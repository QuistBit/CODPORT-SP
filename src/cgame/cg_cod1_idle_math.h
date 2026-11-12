#ifndef CG_COD1_IDLE_MATH_H
#define CG_COD1_IDLE_MATH_H
#include <math.h>
/* PATCH45: cgame 0x30029550. Pure local-angle contribution; time is ms.
 * Stance: 0 standing, 1 crouched, 2 prone. No movement/bob dependency. */
typedef struct {
    float hipAmount,adsAmount,crouchFactor,proneFactor;
    int aimDownSight;
} CodIdleParams;
static float COD1_IdleFloat(float x) {volatile float v=x;return v;}
static float COD1_IdleTarget(const CodIdleParams *p,int stance) {
    return stance==2?p->proneFactor:stance==1?p->crouchFactor:1.0f;
}
static void COD1_IdleAngles(float *stanceFactor,const CodIdleParams *p,
    int stance,float ads,int dt,int time,float angles[3]) {
    static const float rate[3]={0.0010000000474974513f,
        0.000699999975040555f,0.0005000000237487257f};
    static const float scale[3]={0.009999999776482582f,
        0.009999999776482582f,0.03999999910593033f};
    float target=COD1_IdleTarget(p,stance),step=COD1_IdleFloat((float)dt*rate[2]);
    float amount,amplitude,wave;int i;
    if(*stanceFactor<target) {
        *stanceFactor=COD1_IdleFloat(*stanceFactor+step);
        if(*stanceFactor>target)*stanceFactor=target;
    } else {
        *stanceFactor=COD1_IdleFloat(*stanceFactor-step);
        if(*stanceFactor<target)*stanceFactor=target;
    }
    if(p->aimDownSight)
        amount=COD1_IdleFloat(p->hipAmount+COD1_IdleFloat(ads*
            COD1_IdleFloat(p->adsAmount-p->hipAmount)));
    else amount=p->hipAmount!=0?p->hipAmount:80.0f;
    amplitude=COD1_IdleFloat(amount * (*stanceFactor));
    for(i=0;i<3;i++) {
        wave=COD1_IdleFloat((float)sin(COD1_IdleFloat((float)time*rate[i])));
        angles[i]=COD1_IdleFloat(angles[i]+COD1_IdleFloat(
            COD1_IdleFloat(wave*amplitude)*scale[i]));
    }
}
#endif
