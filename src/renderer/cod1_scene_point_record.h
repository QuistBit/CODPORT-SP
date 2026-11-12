/* R72 research proposal. Scene points use a different +04/+10 policy from
 * BSP lights, even though they share the decoded runtime light structure. */
#ifndef COD1_SCENE_POINT_RECORD_H
#define COD1_SCENE_POINT_RECORD_H
#include "cod1_model_light_records.h"
static int C1M_DecodeScenePoint(const float *origin,float radius,const float *rgb,
                              float identityLight,c1mLight *out) {
    c1mLight value;double scalar;int k;
    if(!origin || !rgb || !out || !C1M_Finite(radius) || radius<=0 ||
       !C1M_Finite(identityLight) || identityLight<=0)return 0;
    memset(&value,0,sizeof(value));value.type=2;value.positionW=1;
    value.spotCutoff=180;value.radius=radius;
    value.attenuation[0]=C1M_F32((const unsigned char *)"\x6f\x12\x83\x3a");
    value.attenuation[2]=1;
    scalar=(double)radius*radius*0.03125;
    value.scalar=C1M_Store(scalar);if(!C1M_Finite(value.scalar))return 0;
    for(k=0;k<3;k++){
        if(!C1M_Finite(origin[k]) || !C1M_Finite(rgb[k]))return 0;
        value.position[k]=origin[k];
        /* +04..0c are raw caller bits on this producer path. */
        value.normalizedRGB[k]=rgb[k];
        value.diffuse[k]=C1M_Store((double)identityLight*scalar*rgb[k]);
        if(!C1M_Finite(value.diffuse[k]))return 0;
    }
    *out=value;return 1;
}
#endif
