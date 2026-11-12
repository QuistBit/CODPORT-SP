/* R72 finite transcription proposal, not production integration.
 * Static CPU shader 4da420..4da9ef and ordinary cache alpha publication.
 * Inputs are decoded/prepared native lights and identity-scaled coefficients.
 * No lightingPrecalc, specular, per-model exceptions, or brightness floor. */
#ifndef COD1_MODEL_LIGHT_SHADE_H
#define COD1_MODEL_LIGHT_SHADE_H
#include <limits.h>
#include "cod1_model_light_records.h"

enum {C1M_ROUND_NEAREST=0,C1M_ROUND_DOWN=1,C1M_ROUND_UP=2,C1M_ROUND_CHOP=3};
static float C1M_ShadeStore(long double value) {
    volatile float result=(float)value;return result;
}
static int C1M_ShadeRound(long double value,int mode,int *out) {
    long double rounded,lo,frac;
    if(value<=INT_MIN || value>=INT_MAX || mode<0 || mode>3)return 0;
    if(mode==C1M_ROUND_DOWN)rounded=floorl(value);
    else if(mode==C1M_ROUND_UP)rounded=ceill(value);
    else if(mode==C1M_ROUND_CHOP)rounded=value<0?ceill(value):floorl(value);
    else {
        lo=floorl(value);frac=value-lo;
        rounded=frac<0.5L?lo:frac>0.5L?lo+1.0L:
                (fmodl(lo,2.0L)==0?lo:lo+1.0L);
    }
    *out=(int)rounded;return 1;
}
/* Native sine table: x87 FSIN of these exact float constants, stored float.
 * sinl is a finite libm transcription, not a certified Intel FSIN emulator. */
static void C1M_InitShadeSinTable(float table[1024]) {
    int i;for(i=0;i<1024;i++)table[i]=C1M_ShadeStore(sinl((long double)i*
        (long double)0.3515625f*(long double)3.1415927410125732f*
        (long double)0.0055555556900799274f));
}
static int C1M_ShadeByte(float channel,int roundMode,unsigned char *out) {
    int value;float scaled=C1M_ShadeStore((long double)channel*255.0L);
    if(!C1M_Finite(scaled) || !C1M_ShadeRound((long double)scaled+
       0.000000000931322574615478515625L,roundMode,&value))return 0;
    if(value<0)value=0;
    if(value>255)value=255;
    *out=(unsigned char)value;return 1;
}
static float C1M_ShadeNormalize(float vector[3]) {
    float squared,distance,inverse;int k;
    squared=C1M_ShadeStore((long double)vector[0]*vector[0]+
        (long double)vector[1]*vector[1]+(long double)vector[2]*vector[2]);
    if(!C1M_Finite(squared) || squared<0)return -1;
    distance=C1M_ShadeStore(sqrtl((long double)squared));
    if(distance!=0) {
        inverse=C1M_ShadeStore(1.0L/(long double)distance);
        for(k=0;k<3;k++)vector[k]=C1M_ShadeStore((long double)inverse*vector[k]);
    }
    return distance;
}
/* CPU cached-model diffuse route, finite/nonexceptional denominator domain.
 * Normal has already received the producer's matrix transform and normalization.
 * Noninteger positive exponents follow the native ceil-count loop. Authored
 * supported BSP records publish integer exponents. A 180 cutoff bypasses cone.
 * Returns 0 without output mutation on unsupported/exceptional inputs.
 * Output alpha is the ordinary cache caller's later forced-255 publication. */
static int C1M_Shade(const c1mWorldCoefficients *world,float remainingSky,
                     const c1mLight *lights,const float *weights,int count,
                     const float vertex[3],const float normal[3],
                     int roundMode,const float sinTable[1024],
                     unsigned char outRGBA[4]) {
    float rgb[3],dir[3],dot,distance,spot,phase;long double factor,denominator;
    unsigned char result[4];int i,k,index,exponent;
    if(!world || !vertex || !normal || !sinTable || !outRGBA ||
       count<0 || count>16 || (count && (!lights || !weights)) ||
       !C1M_Finite(remainingSky) || roundMode<0 || roundMode>3)return 0;
    for(k=0;k<3;k++) {
        if(!C1M_Finite(vertex[k]) || !C1M_Finite(normal[k]) ||
           !C1M_Finite(world->ambient[k]) || !C1M_Finite(world->sky[k]))return 0;
        rgb[k]=C1M_ShadeStore((long double)world->sky[k]*remainingSky+world->ambient[k]);
    }
    for(i=0;i<count;i++) {
        const c1mLight *light=&lights[i];
        if(!C1M_Finite(weights[i]) || !C1M_Finite(light->positionW))return 0;
        for(k=0;k<3;k++)if(!C1M_Finite(light->position[k]) ||
            !C1M_Finite(light->ambient[k]) || !C1M_Finite(light->diffuse[k]))return 0;
        factor=(long double)weights[i];
        if(light->positionW==0) {
            /* W=0 reads raw direction, and ignores all attenuation/spot fields. */
            dot=C1M_ShadeStore((long double)light->position[2]*normal[2]+
                (long double)light->position[0]*normal[0]+
                (long double)light->position[1]*normal[1]);
            if(dot<0)dot=0;
        } else {
            for(k=0;k<3;k++)dir[k]=C1M_ShadeStore((long double)light->position[k]-vertex[k]);
            distance=C1M_ShadeNormalize(dir);if(distance<0)return 0;
            dot=C1M_ShadeStore((long double)dir[2]*normal[2]+
                (long double)dir[1]*normal[1]+(long double)dir[0]*normal[0]);
            if(dot<0)dot=0;
            for(k=0;k<3;k++)if(!C1M_Finite(light->attenuation[k]))return 0;
            denominator=((long double)distance*light->attenuation[2]+
                          light->attenuation[1])*distance+light->attenuation[0];
            if(denominator==0)return 0;
            factor=(long double)weights[i]/denominator;
            if(!C1M_Finite(light->spotCutoff))return 0;
            if(light->spotCutoff!=180.0f) {
                /* Native cone branch explicitly reloads this float spill. */
                factor=C1M_ShadeStore(factor);
                for(k=0;k<3;k++)if(!C1M_Finite(light->spotDirection[k]))return 0;
                spot=C1M_ShadeStore((long double)dir[2]*light->spotDirection[2]+
                    (long double)dir[1]*light->spotDirection[1]+
                    (long double)dir[0]*light->spotDirection[0]);
                phase=C1M_ShadeStore(((long double)light->spotCutoff+90.0L)*
                                    (long double)2.844444513320923f);
                if(!C1M_ShadeRound((long double)phase+
                   0.000000000931322574615478515625L,roundMode,&index))return 0;
                index&=1023;if(!C1M_Finite(sinTable[index]))return 0;
                if(spot<=sinTable[index])factor=0;
                else {
                    if(!C1M_Finite(light->spotExponent) || light->spotExponent>128)return 0;
                    exponent=light->spotExponent>0?(int)ceill(light->spotExponent):0;
                    for(k=0;k<exponent;k++)factor*=spot;
                }
            }
        }
        for(k=0;k<3;k++) {
            rgb[k]=C1M_ShadeStore(((long double)dot*light->diffuse[k]+
                                 light->ambient[k])*factor+rgb[k]);
            if(!C1M_Finite(rgb[k]))return 0;
        }
    }
    for(k=0;k<3;k++)if(!C1M_ShadeByte(rgb[k],roundMode,&result[k]))return 0;
    result[3]=255;memcpy(outRGBA,result,4);return 1;
}
#endif
