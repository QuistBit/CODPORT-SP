#ifndef BG_COD1_DROP_SURFACE_H
#define BG_COD1_DROP_SURFACE_H
/* Original2001C890 +20010B80 surface-angle fragment. Float stages retained.
 * Standard libm trig is compared to native x87 with explicit angle tolerance.
 * Caller owes live trace/lifecycle. This is not an installed gameplay provider. */
#include <math.h>
static float C1DS_Pitch(const float v[3]){
 float h;if(v[0]==0&&v[1]==0)return v[2]>0?-90.0f:90.0f;
 h=(float)sqrt((double)v[0]*v[0]+(double)v[1]*v[1]);
 return (float)(atan2((double)v[2],h)*-180.0/(double)3.1415927410125732f);
}
static int C1DS_Surface(const float angles[3],const float normal[3],int weapon,float out[3]){
 float f[3],r[3],n[3],a[3],cy,sy,cp,sp,yaw,pitch,x,y,z,tmp,roll,h;unsigned i;
 if(!angles||!normal||!out)return 0;for(i=0;i<3;i++)if(!isfinite(angles[i])||!isfinite(normal[i]))return 0;
 yaw=(float)((double)angles[1]*0.01745329238474369f);pitch=(float)((double)angles[0]*0.01745329238474369f);
 cy=(float)cos(yaw);sy=(float)sin(yaw);cp=(float)cos(pitch);sp=(float)sin(pitch);
 f[0]=(float)((double)cp*cy);f[1]=(float)((double)cp*sy);f[2]=-sp;
 for(i=0;i<3;i++)n[i]=normal[i];
 r[0]=(float)((double)f[1]*n[2]-(double)f[2]*n[1]);r[1]=(float)((double)f[2]*n[0]-(double)f[0]*n[2]);r[2]=(float)((double)f[0]*n[1]-(double)f[1]*n[0]);
 for(i=0;i<3;i++)r[i]=-r[i];
 f[0]=(float)((double)r[1]*n[2]-(double)r[2]*n[1]);f[1]=(float)((double)r[2]*n[0]-(double)r[0]*n[2]);f[2]=(float)((double)r[0]*n[1]-(double)r[1]*n[0]);
 if(f[0]==0&&f[1]==0){a[0]=f[2]>0?270.0f:90.0f;a[1]=0;}else{
  a[1]=(float)(atan2(f[1],f[0])*180.0/(double)3.1415927410125732f);if(a[1]<0)a[1]=(float)((double)a[1]+360.0f);
  h=(float)sqrt((double)f[0]*f[0]+(double)f[1]*f[1]);a[0]=(float)(atan2(f[2],h)*-180.0/(double)3.1415927410125732f);if(a[0]<0)a[0]=(float)((double)a[0]+360.0f);
 }
 yaw=(float)(-(double)a[1]*3.1415927410125732f/180.0f);cy=(float)cos(yaw);sy=(float)sin(yaw);
 tmp=(float)((double)cy*r[0]-(double)sy*r[1]);y=(float)((double)sy*r[0]+(double)cy*r[1]);
 pitch=(float)(-(double)a[0]*3.1415927410125732f/180.0f);cp=(float)cos(pitch);sp=(float)sin(pitch);
 x=(float)((double)tmp*cp+(double)sp*r[2]);z=(float)((double)cp*r[2]-(double)tmp*sp);
 {float v[3]={x,y,z};roll=C1DS_Pitch(v);}
 a[2]=y<0?(float)((double)(roll>=0?-180.0f:180.0f)+roll):-roll;if(weapon)a[2]=(float)((double)a[2]+90.0f);
 for(i=0;i<3;i++)if(!isfinite(a[i]))return 0;for(i=0;i<3;i++)out[i]=a[i];return 1;
}
#endif
