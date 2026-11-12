#ifndef COD1_ACTOR_SCRIPTED_MATH14_H
#define COD1_ACTOR_SCRIPTED_MATH14_H
#include <math.h>
#include <stdint.h>
#include <string.h>
/* Pure numeric inner functions; owner must establish PC53/nearest and validate
 * finite bounded inputs. No actual actor/runtime/catalog authority here.
 * atan2/sqrt are explicit platform libm providers, compared with full actual
 * embedded CRT with an ANGULAR tolerance, never claimed CRT bit-identical. */
static void C1ASM14_SinCos(float radians,float *s,float*c){
#if defined(_MSC_VER)&&defined(_M_IX86)
    float sv,cv;
    __asm fld radians
    __asm fsincos
    __asm fstp cv
    __asm fstp sv
    *s=sv;*c=cv;
#else
    *s=(float)sin(radians);*c=(float)cos(radians);
#endif
}
static void C1ASM14_Matrix(const float a[3],float m[9]){
    float sy,cy,sp,cp,sr,cr,tmp,right;uint32_t bits=0x3c8efa35u;float factor;memcpy(&factor,&bits,4);
    C1ASM14_SinCos((float)((double)a[1]*factor),&sy,&cy);
    C1ASM14_SinCos((float)((double)a[0]*factor),&sp,&cp);
    C1ASM14_SinCos((float)((double)a[2]*factor),&sr,&cr);
    m[0]=(float)((double)cp*cy);m[1]=(float)((double)cp*sy);m[2]=-sp;
    tmp=(float)((double)sr*sp);
    right=(float)((double)cr*sy-(double)tmp*cy);m[3]=(float)(0.-right);
    right=(float)(-(double)cr*cy-(double)tmp*sy);m[4]=(float)(0.-right);
    right=(float)(-(double)sr*cp);m[5]=(float)(0.-right);
    tmp=(float)((double)cr*sp);
    m[6]=(float)((double)tmp*cy+(double)sr*sy);m[7]=(float)((double)tmp*sy-(double)sr*cy);m[8]=(float)((double)cr*cp);
}
static void C1ASM14_Point(const float m[12],const float p[3],int constructorOrder,float out[3]){
    unsigned i;double sum;
    for(i=0;i<3;i++){
        if(constructorOrder)sum=((double)p[2]*m[6+i]+(double)p[0]*m[i])+(double)p[1]*m[3+i];
        else if(!i)sum=((double)p[1]*m[3]+(double)p[2]*m[6])+(double)p[0]*m[0];
        else sum=((double)p[0]*m[i]+(double)p[1]*m[3+i])+(double)p[2]*m[6+i];
        out[i]=(float)(sum+m[9+i]);
    }
}
static float C1ASM14_Yaw(const float q[2]){
    float xx=(float)((double)q[0]*q[0]);float n=(float)((double)q[1]*q[1]+xx);float k=(float)(2./n);
    return (float)(atan2((double)q[1]*q[0]*k,1.-(double)k*xx)*57.2957763671875);
}
static void C1ASM14_YawMatrix(float yaw,float m[9]){
    float s,c,factor;uint32_t bits=0x3c8efa35u;memcpy(&factor,&bits,4);C1ASM14_SinCos((float)((double)yaw*factor),&s,&c);
    m[0]=c;m[1]=s;m[2]=0;
    /* EC70 right=(s,-c,0); 10B30 stores zero-minus-right. */
    m[3]=(float)(0.-s);m[4]=(float)(0.-(-c));m[5]=0;m[6]=m[7]=0;m[8]=1;
}
static void C1ASM14_Product(const float a[9],const float b[9],float out[9]){
    static const unsigned order[9][3]={{0,1,2},{1,2,0},{1,2,0},{1,0,2},{2,1,0},{2,1,0},{0,2,1},{2,1,0},{2,1,0}};
    unsigned row,col,i,j,n;double sum;
    for(row=0;row<3;row++)for(col=0;col<3;col++){
        n=row*3+col;i=order[n][0];sum=(double)b[row*3+i]*a[i*3+col];
        for(j=1;j<3;j++){i=order[n][j];sum+=(double)b[row*3+i]*a[i*3+col];}out[n]=(float)sum;
    }
}
static float C1ASM14_Pitch(const float p[3]){
    float horizontal;if(p[0]==0&&p[1]==0)return p[2]>0?-90.f:90.f;
    horizontal=(float)sqrt((double)p[0]*p[0]+(double)p[1]*p[1]);
    return (float)(atan2((double)p[2],horizontal)*-180./3.1415927410125732421875);
}
static void C1ASM14_Angles(const float m[9],float a[3]){
    const double pi=3.1415927410125732421875;float horizontal,s,c,x,y,z,previous,first;
    if(m[0]==0&&m[1]==0){a[1]=0;a[0]=m[2]>0?270.f:90.f;}
    else{
        a[1]=(float)(atan2((double)m[1],m[0])*180./pi);if(a[1]<0)a[1]=(float)((double)a[1]+360.);
        horizontal=(float)sqrt((double)m[0]*m[0]+(double)m[1]*m[1]);a[0]=(float)(atan2((double)m[2],horizontal)*-180./pi);if(a[0]<0)a[0]=(float)((double)a[0]+360.);
    }
    C1ASM14_SinCos((float)(-(double)a[1]*pi/180.),&s,&c);
    x=(float)((double)c*m[3]-(double)s*m[4]);y=(float)((double)c*m[4]+(double)s*m[3]);z=m[5];
    C1ASM14_SinCos((float)(-(double)a[0]*pi/180.),&s,&c);previous=x;
    x=(float)((double)x*c+(double)s*z);z=(float)((double)c*z-(double)previous*s);
    {float v[3]={x,y,z};first=C1ASM14_Pitch(v);}
    a[2]=y<0?(float)((first<0?180.:-180.)+first):-first;
}
static float C1ASM14_Difference(float a,float b){float value=(float)((double)a-b);while(value>180)value=(float)((double)value-360.);while(value< -180)value=(float)((double)value+360.);return value;}
static void C1ASM14_Pose(const float record[12],const float q[2],const float translation[3],int constructorOrder,float origin[3],float angles[3]){
    float rotation[9],combined[9];C1ASM14_Point(record,translation,constructorOrder,origin);C1ASM14_YawMatrix(C1ASM14_Yaw(q),rotation);C1ASM14_Product(record,rotation,combined);C1ASM14_Angles(combined,angles);
}
#endif
