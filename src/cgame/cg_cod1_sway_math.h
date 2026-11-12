#ifndef CG_COD1_SWAY_MATH_H
#define CG_COD1_SWAY_MATH_H
/* PATCH44: reconstructed CoD1 SP 0x30028ff0, ordinary non-scope/non-shellshock
 * path. Input is the presentation camera, not mouse deltas. */
#include <math.h>
typedef struct {float previous[3], angle[2], offset[2];} CodSwayState;
typedef struct {float maxAngle,lerpSpeed,pitchScale,yawScale,horizScale,vertScale;} CodSwayParams;
static float CSway_Add(float a,float b){volatile float x=a+b;return x;}
static float CSway_Mul(float a,float b){volatile float x=a*b;return x;}
static float CSway_Delta(float a,float b){
    float d=CSway_Add(a,-b);
    while(d>180)d=CSway_Add(d,-360);
    while(d< -180)d=CSway_Add(d,360);
    return d;
}
static float CSway_Normalize(float a){
    float n=(float)(((int)CSway_Mul(a,65536.0f/360.0f))&65535);
    n=CSway_Mul(n,360.0f/65536.0f);return n>180?CSway_Add(n,-360):n;
}
static float CSway_Approach(float target,float old,float seconds,float rate){
    float d=CSway_Add(target,-old),step=CSway_Mul(CSway_Mul(seconds,d),rate);
    return fabs((double)d)>0.0010000000474974513 && fabs((double)step)<fabs((double)d)?CSway_Add(old,step):target;
}
static void COD1_SwayUpdate(CodSwayState *s,const CodSwayParams *hip,const CodSwayParams *aim,
                            float ads,const float angles[3],int milliseconds){
    float h[6]={hip->maxAngle,hip->lerpSpeed,hip->pitchScale,hip->yawScale,hip->horizScale,hip->vertScale};
    float a[6]={aim->maxAngle,aim->lerpSpeed,aim->pitchScale,aim->yawScale,aim->horizScale,aim->vertScale};
    float p[6],d[2],target[2],seconds=CSway_Mul((float)milliseconds,.001f);int i;
    for(i=0;i<6;i++)p[i]=CSway_Add(CSway_Mul(CSway_Add(a[i],-h[i]),ads),h[i]);
    for(i=0;i<2;i++){
        d[i]=CSway_Delta(angles[i],s->previous[i]);
        if(d[i]< -p[0])d[i]=-p[0];else if(d[i]>p[0])d[i]=p[0];
    }
    s->offset[0]=CSway_Approach(CSway_Mul(d[1],p[4]),s->offset[0],seconds,p[1]);
    s->offset[1]=CSway_Approach(CSway_Mul(d[0],p[5]),s->offset[1],seconds,p[1]);
    target[0]=CSway_Mul(d[0],p[2]);target[1]=CSway_Mul(d[1],p[3]);
    for(i=0;i<2;i++){
        while(CSway_Add(target[i],-s->angle[i])>180)target[i]=CSway_Add(target[i],-360);
        s->angle[i]=CSway_Normalize(CSway_Approach(target[i],s->angle[i],seconds,p[1]));
    }
    for(i=0;i<3;i++)s->previous[i]=angles[i];
}
#endif
