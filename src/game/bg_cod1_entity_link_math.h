/* Original45130/45280/45670 empty-tag row-affine semantics. Native x87/CRT
 * bit identity is not claimed by these explicit portable libm operations. */
#ifndef BG_COD1_ENTITY_LINK_MATH_H
#define BG_COD1_ENTITY_LINK_MATH_H
#include <math.h>
#include <string.h>
static int C1EL_Finite(const float*m,int n){int i;for(i=0;i<n;i++)if(!isfinite(m[i]))return 0;return 1;}
static int C1EL_Frame(const float o[3],const float a[3],float m[12]){
 float p,y,r,sp,cp,sy,cy,sr,cr;const float rad=0.01745329238474369f;
 if(!C1EL_Finite(o,3)||!C1EL_Finite(a,3)||a[0]<-360000||a[0]>360000||a[1]<-360000||a[1]>360000||a[2]<-360000||a[2]>360000)return 0;
 p=a[0]*rad;y=a[1]*rad;r=a[2]*rad;sp=(float)sin(p);cp=(float)cos(p);sy=(float)sin(y);cy=(float)cos(y);sr=(float)sin(r);cr=(float)cos(r);
 m[0]=cp*cy;m[1]=cp*sy;m[2]=-sp;
 m[3]=sr*sp*cy-cr*sy;m[4]=sr*sp*sy+cr*cy;m[5]=sr*cp;
 m[6]=cr*sp*cy+sr*sy;m[7]=cr*sp*sy-sr*cy;m[8]=cr*cp;memcpy(m+9,o,12);return C1EL_Finite(m,12);
}
static int C1EL_Multiply(const float a[12],const float b[12],float out[12]){
 float m[12];int i,j;for(i=0;i<3;i++)for(j=0;j<3;j++)m[i*3+j]=a[i*3]*b[j]+a[i*3+1]*b[3+j]+a[i*3+2]*b[6+j];
 for(j=0;j<3;j++)m[9+j]=a[9]*b[j]+a[10]*b[3+j]+a[11]*b[6+j]+b[9+j];if(!C1EL_Finite(m,12))return 0;memcpy(out,m,sizeof(m));return 1;
}
static int C1EL_Inverse(const float a[12],float out[12]){
 float m[12];int i,j;for(i=0;i<3;i++)for(j=0;j<3;j++)m[i*3+j]=a[j*3+i];
 for(j=0;j<3;j++)m[9+j]=-a[9]*m[j]-a[10]*m[3+j]-a[11]*m[6+j];if(!C1EL_Finite(m,12))return 0;memcpy(out,m,sizeof(m));return 1;
}
/* Original10B80 uses48-byte affine stride, unlike64-byte10CD0. */
static int C1EL_Angles(const float m[12],float out[3]){
 const double pi=3.1415927410125732421875;float a[3],h,rad,c,s,x,y,z,u,alpha;
 if(!C1EL_Finite(m,12))return 0;
 if(m[0]==0&&m[1]==0){a[1]=0;a[0]=m[2]>0?270.f:90.f;}
 else{a[1]=(float)(atan2((double)m[1],m[0])*180/pi);if(a[1]<0)a[1]+=360;h=(float)sqrt((double)m[0]*m[0]+(double)m[1]*m[1]);a[0]=(float)(atan2((double)m[2],h)*-180/pi);if(a[0]<0)a[0]+=360;}
 rad=(float)(-(double)a[1]*pi/180);c=(float)cos(rad);s=(float)sin(rad);x=m[3];y=m[4];z=m[5];u=(float)((double)c*x-(double)s*y);y=(float)((double)c*y+(double)s*x);
 rad=(float)(-(double)a[0]*pi/180);c=(float)cos(rad);s=(float)sin(rad);x=(float)((double)u*c+(double)s*z);z=(float)((double)c*z-(double)u*s);
 if(x==0&&y==0)alpha=z>0?-90.f:90.f;else{h=(float)sqrt((double)x*x+(double)y*y);alpha=(float)(atan2((double)z,h)*-180/pi);}
 a[2]=y<0?(alpha<0?alpha+180.f:alpha-180.f):-alpha;if(!C1EL_Finite(a,3))return 0;memcpy(out,a,12);return 1;
}
#endif
