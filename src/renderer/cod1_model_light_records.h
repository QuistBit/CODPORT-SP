/* R72 proposal: decoded native model-light records and world coefficients.
 * Read-only research until integrated by the release coordinator.
 * Native image is never executed. Authored 72-byte types 1/2/3/4/5/7 are
 * distinct from synthesized runtime type 8. No instance precalc is added. */
#ifndef COD1_MODEL_LIGHT_RECORDS_H
#define COD1_MODEL_LIGHT_RECORDS_H
#include <float.h>
#include <math.h>
#include <string.h>
#include "cod1_worldspawn_light.h"

#include "cod1_model_light_types.h"

static int C1M_Finite(float v) {return v>=-FLT_MAX && v<=FLT_MAX;}
static float C1M_Store(double v) {volatile float stored=(float)v;return stored;}
static unsigned int C1M_U32(const unsigned char *p) {
    return (unsigned int)p[0]|((unsigned int)p[1]<<8)|
           ((unsigned int)p[2]<<16)|((unsigned int)p[3]<<24);
}
static float C1M_F32(const unsigned char *p) {
    unsigned int bits=C1M_U32(p);float v;memcpy(&v,&bits,4);return v;
}
static double C1M_LuminanceWide(const float rgb[3]) {
    /* Preserve the recovered blue + green + red operation order. */
    return (double)rgb[2]*0.11400000005960464+
           (double)rgb[1]*0.5870000123977661+
           (double)rgb[0]*0.29899999499320984;
}
static float C1M_Luminance(const float rgb[3]) {
    return C1M_Store(C1M_LuminanceWide(rgb));
}

/* Worldspawn parser/normalization is the retained R54 native contract. Apply
 * identityLight inside coefficient arithmetic, once, before final float store. */
static int C1M_WorldCoefficients(const c1WorldLightParams *p,float identityLight,
                                c1mWorldCoefficients *out) {
    c1mWorldCoefficients value;float ac[3],peak=0;const float *sc;double delta;int k;
    if(!p || !out || p->invalid || !C1M_Finite(identityLight) || identityLight<=0)return 0;
    sc=(p->present&C1WL_SKYCOLOR)?p->skyColor:p->sunColor;
    memcpy(ac,p->ambientColor,sizeof(ac));
    if(p->ambient!=0 && !C1WL_Normalize(ac,&peak))return 0;
    delta=((double)p->sunlight-(double)p->ambient)*identityLight;
    for(k=0;k<3;k++){
        double a=(p->ambient!=0 && peak!=0)?(double)identityLight*p->ambient*ac[k]:0;
        value.ambient[k]=C1M_Store(a);
        value.sun[k]=C1M_Store(delta*(1.0-(double)p->fraction)*p->sunColor[k]);
        value.sky[k]=C1M_Store(delta*(double)p->fraction*sc[k]);
        if(!C1M_Finite(value.ambient[k]) || !C1M_Finite(value.sun[k]) || !C1M_Finite(value.sky[k]))return 0;
    }
    value.skyLuminance=C1M_Luminance(value.sky);if(!C1M_Finite(value.skyLuminance))return 0;
    *out=value;return 1;
}

/* Supported authored records only. Valid signed finite RGB is retained.
 * Unknown source types 6/8 are rejected, never guessed as omnidirectional.
 * Invalid/nonrepresentable fields return failure without publishing a record. */
static int C1M_DecodeBspLight72(const unsigned char *source,float identityLight,
                              const c1mWorldCoefficients *world,c1mLight *out) {
    c1mLight value;float rgb[3],cosine;double luminance,reciprocal;unsigned int type;int k;
    if(!source || !out || !C1M_Finite(identityLight) || identityLight<=0)return 0;
    type=C1M_U32(source);if(type!=1 && type!=2 && type!=3 && type!=4 && type!=5 && type!=7)return 0;
    memset(&value,0,sizeof(value));value.type=type;value.spotCutoff=180;
    for(k=0;k<3;k++){
        float authored=C1M_F32(source+4+k*4);if(!C1M_Finite(authored))return 0;
        rgb[k]=C1M_Store((double)authored*identityLight);if(!C1M_Finite(rgb[k]))return 0;
    }
    luminance=C1M_LuminanceWide(rgb);value.scalar=C1M_Store(luminance);
    if(!C1M_Finite(value.scalar))return 0;
    reciprocal=luminance!=0?1.0/luminance:0;
    for(k=0;k<3;k++){
        value.normalizedRGB[k]=C1M_Store((double)rgb[k]*reciprocal);
        value.ambient[k]=C1M_Store((double)rgb[k]*0.10000000149011612);
        value.diffuse[k]=C1M_Store((double)rgb[k]*0.800000011920929);
        if(!C1M_Finite(value.normalizedRGB[k]) || !C1M_Finite(value.ambient[k]) || !C1M_Finite(value.diffuse[k]))return 0;
        value.position[k]=C1M_F32(source+(type==1?28:16)+k*4);
        if(!C1M_Finite(value.position[k]))return 0;
    }
    if(type==1){
        if(!world)return 0;value.positionW=0;value.attenuation[0]=1;
        for(k=0;k<3;k++){value.ambient[k]=0;value.diffuse[k]=world->sun[k];if(!C1M_Finite(value.diffuse[k]))return 0;}
        value.scalar=C1M_Luminance(value.diffuse);
    }else{
        value.positionW=1;
        if(type==2 || type==5)value.attenuation[2]=1;
        if(type==3)value.attenuation[1]=C1M_F32(source+40);
        if(type==4 || type==7){value.attenuation[2]=C1M_F32(source+40);value.attenuation[0]=C1M_F32(source+44);}
        if(type==5 || type==7){
            for(k=0;k<3;k++){value.spotDirection[k]=-C1M_F32(source+28+k*4);if(!C1M_Finite(value.spotDirection[k]))return 0;}
            cosine=C1M_F32(source+(type==5?40:48));
            if(!C1M_Finite(cosine) || cosine < -1 || cosine > 1)return 0;
            value.spotCutoff=C1M_Store(acos((double)cosine)*57.29577791868205);
            /* Native LittleLong followed by FILD: integer, not float exponent. */
            value.spotExponent=C1M_Store((double)(int)C1M_U32(source+(type==5?44:52)));
        }
    }
    for(k=0;k<3;k++)if(!C1M_Finite(value.attenuation[k]))return 0;
    if(!C1M_Finite(value.scalar) || !C1M_Finite(value.spotCutoff) || !C1M_Finite(value.spotExponent))return 0;
    *out=value;return 1;
}
#endif
