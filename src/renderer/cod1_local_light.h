/* R57 bounded point-light path. Native decoder 4d17e0..4d1d1d,
 * attenuation 4aa8d0..4aa96e. Types 2/3/4 are omnidirectional;
 * cone types 5/7 require a separate verified angular consumer.
 * This is not the complete native light selection/GL aggregation pipeline. */
#ifndef COD1_LOCAL_LIGHT_H
#define COD1_LOCAL_LIGHT_H
#include "cod1_lightcache.h"
typedef struct {
    unsigned int type;
    float rgb[3],position[3],attenuation[3];
} c1PointLight;
static int C1L_Point(const unsigned char *record,c1PointLight *out) {
    int k;unsigned int type=C1_U32(record);
    if(type<2 || type>4)return 0;
    memset(out,0,sizeof(*out));out->type=type;
    for(k=0;k<3;k++){
        out->rgb[k]=C1_F32(record+4+k*4);
        out->position[k]=C1_F32(record+16+k*4);
        if(!(out->rgb[k]>=0 && out->rgb[k]<=FLT_MAX) ||
           !(out->position[k]>=-131072 && out->position[k]<131072))return 0;
    }
    if(type==2)out->attenuation[2]=1;
    if(type==3)out->attenuation[1]=C1_F32(record+40);
    if(type==4){out->attenuation[2]=C1_F32(record+40);out->attenuation[0]=C1_F32(record+44);}
    for(k=0;k<3;k++)if(!(out->attenuation[k]>=0 && out->attenuation[k]<=FLT_MAX))return 0;
    return 1;
}
/* Product of the native normalized color and scalar attenuation is the raw
 * authored color / (constant + linear*d + quadratic*d*d), at overbright 0.
 * Keep explicit float stores; do not invent a denominator or minimum RGB. */
static int C1L_RGB(const c1PointLight *light,const float *point,const float *normal,
                   float visibility,float *rgb) {
    double dsq=0,dot=0,denom,d;float delta[3],valueRGB[3];int k;
    for(k=0;k<3;k++){
        volatile float v=(float)((double)light->position[k]-point[k]);delta[k]=v;
        dsq+=(double)v*v;dot+=(double)v*normal[k];
    }
    d=sqrt(dsq);denom=light->attenuation[0]+light->attenuation[1]*d+light->attenuation[2]*dsq;
    if(!(denom>0 && denom<=DBL_MAX) || !(visibility>=0 && visibility<=1))return 0;
    if(d>0)dot/=d;else dot=0;
    if(dot<0)dot=0;if(dot>1)dot=1;
    for(k=0;k<3;k++){
        double value=visibility*light->rgb[k]*(0.10000000149011612+0.800000011920929*dot)/denom;
        if(!(value>=0 && value<=FLT_MAX))return 0;
        valueRGB[k]=(float)value;
    }
    memcpy(rgb,valueRGB,sizeof(valueRGB));
    return 1;
}
#endif
