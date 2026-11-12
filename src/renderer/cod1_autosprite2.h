/* R59: indexed autosprite2, transcribed from the hash-locked CoDSP renderer
 * 0x5098a0..0x509f11. It selects directed edges of consecutive triangle pairs;
 * vertex storage order is not a quad contract. This helper only changes XYZ.
 * Native uint16 indices are widened to the port's existing uint32 tess indices.
 */
#ifndef COD1_AUTOSPRITE2_H
#define COD1_AUTOSPRITE2_H
#include <float.h>
#include <math.h>
#include <stddef.h>

static int C1AS_Finite(float value) {
    return value==value && value<=FLT_MAX && value>=-FLT_MAX;
}
static float C1AS_EdgeLength(const float *a,const float *b) {
    float x=b[0]-a[0],y=b[1]-a[1],z=b[2]-a[2];
    /* Native stores the differences and the sum to float; x87 accumulates
     * the products before that final store. */
    return (float)((double)z*z+(double)y*y+(double)x*x);
}
static void C1AS_LocalForward(const float world[3],const float axis[3][3],float local[3]) {
    int k;
    for(k=0;k<3;k++)local[k]=(float)((double)world[0]*axis[k][0]+
        (double)world[2]*axis[k][2]+(double)world[1]*axis[k][1]);
}
static void C1AS_Normalize(float v[3]) {
    float squared=(float)((double)v[0]*v[0]+(double)v[1]*v[1]+(double)v[2]*v[2]);
    float length=(float)sqrt((double)squared),inverse;
    int k;
    if(length!=0.0f){inverse=1.0f/length;for(k=0;k<3;k++)v[k]=(float)((double)v[k]*inverse);}
}
/* Valid source geometry follows the native six-index traversal, including
 * shared/reordered vertices and strict first-wins ties. Guard malformed port
 * buffers before any write; native out-of-bounds behavior is not reproduced. */
static int C1AS_Deform(float *xyz,int vertices,int stride,const unsigned int *indices,
                      int count,const float forward[3]) {
    int i,j,k;
    if(!xyz || !indices || !forward || vertices<0 || stride<3 || count<0 || count%6)return 0;
    for(k=0;k<3;k++)if(!C1AS_Finite(forward[k]))return 0;
    for(i=0;i<count;i++){
        const float *v;
        if(indices[i]>=(unsigned int)vertices)return 0;
        v=xyz+(size_t)stride*indices[i];
        for(k=0;k<3;k++)if(!C1AS_Finite(v[k]))return 0;
    }
    for(i=0;i<count;i+=6){
        float lengths[2]={FLT_MAX,FLT_MAX},mid[2][3],major[3],minor[3];
        unsigned int ends[2][2]={{0,0},{0,0}};
        for(j=0;j<6;j++){
            int base=(j/3)*3,edge=j%3;
            unsigned int a=indices[i+base+edge],b=indices[i+base+(edge+1)%3];
            float length=C1AS_EdgeLength(xyz+(size_t)stride*a,xyz+(size_t)stride*b);
            if(length<lengths[0]){
                lengths[1]=lengths[0];ends[1][0]=ends[0][0];ends[1][1]=ends[0][1];
                lengths[0]=length;ends[0][0]=a;ends[0][1]=b;
            }else if(length<lengths[1]){
                lengths[1]=length;ends[1][0]=a;ends[1][1]=b;
            }
        }
        for(j=0;j<2;j++){
            const float *a=xyz+(size_t)stride*ends[j][0],*b=xyz+(size_t)stride*ends[j][1];
            for(k=0;k<3;k++)mid[j][k]=(float)(0.5*((double)a[k]+b[k]));
        }
        for(k=0;k<3;k++)major[k]=mid[1][k]-mid[0][k];
        minor[0]=(float)((double)major[1]*forward[2]-(double)major[2]*forward[1]);
        minor[1]=(float)((double)major[2]*forward[0]-(double)major[0]*forward[2]);
        minor[2]=(float)((double)major[0]*forward[1]-(double)major[1]*forward[0]);
        C1AS_Normalize(minor);
        for(j=0;j<2;j++){
            float half=(float)(0.5*sqrt((double)lengths[j]));
            float *a=xyz+(size_t)stride*ends[j][0],*b=xyz+(size_t)stride*ends[j][1];
            float sign=j?half:-half;
            for(k=0;k<3;k++)a[k]=(float)((double)mid[j][k]+(double)sign*minor[k]);
            for(k=0;k<3;k++)b[k]=(float)((double)mid[j][k]-(double)sign*minor[k]);
        }
    }
    return 1;
}
#endif
