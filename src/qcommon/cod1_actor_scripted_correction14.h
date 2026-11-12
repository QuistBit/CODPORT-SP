#ifndef COD1_ACTOR_SCRIPTED_CORRECTION14_H
#define COD1_ACTOR_SCRIPTED_CORRECTION14_H
#include <math.h>
#include <stdint.h>
#include <string.h>
/* Pure normal-record correction arithmetic: original GAME13470/13500 only.
 * No record ownership, phase/notify completion, body clock or actor movement. */
static float C1ASC14_Angle360(float x){
    /* Native107D0 has distinct branches. A tiny negative may round to360
     * after addition; that NEGATIVE path returns360 without rechecking upper
     * bound. Do not replace it with canonical modulo normalization. */
    if(x<0){do{x=(float)((double)x+360.);}while(x<0);}
    else while(x>=360)x=(float)((double)x-360.);
    return x;
}
static void C1ASC14_Position(float correction[3],float world[3],float seconds){
    double x=correction[0],y=correction[1],z=correction[2];
    double length2=(x*x+y*y)+z*z, factor;
    if(length2==0)return;
    factor=1.-((double)seconds*300.)/sqrt(length2);
    if(factor>0){
        x=factor*x;correction[0]=(float)x;
        correction[1]=(float)(factor*y);correction[2]=(float)(factor*z);
        /* Original FST keeps X unrounded on x87; Y/Z reload stored floats. */
        world[0]=(float)(x+world[0]);
        world[1]=(float)((double)world[1]+correction[1]);
        world[2]=(float)((double)world[2]+correction[2]);
    }else correction[0]=correction[1]=correction[2]=0;
}
static void C1ASC14_Angle(float correction[3],float world[3],float seconds,float rate){
    float step=(float)((double)seconds*rate);unsigned i;double value;
    for(i=0;i<3;i++)if(correction[i]!=0){
        if(correction[i]>step)value=(double)correction[i]-step;
        else if(correction[i]<-step)value=(double)correction[i]+step;
        else {correction[i]=0;continue;}
        correction[i]=(float)value;
        world[i]=C1ASC14_Angle360((float)(value+world[i]));
    }
}
static void C1ASC14_Corrections(float position[3],float origin[3],float angle[3],float rotation[3],float seconds,float rate){
#if defined(_MSC_VER)&&defined(_M_IX86)
    unsigned char outer[108];uint32_t mx,nmx=0x1f80;unsigned short cw=0x027f;
    __asm fnsave outer
    __asm stmxcsr mx
    __asm fldcw cw
    __asm ldmxcsr nmx
#endif
    C1ASC14_Position(position,origin,seconds);
    C1ASC14_Angle(angle,rotation,seconds,rate);
#if defined(_MSC_VER)&&defined(_M_IX86)
    __asm ldmxcsr mx
    __asm frstor outer
#endif
}
#endif
