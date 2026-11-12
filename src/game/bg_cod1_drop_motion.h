#ifndef BG_COD1_DROP_MOTION_H
#define BG_COD1_DROP_MOTION_H
/* Original CoD TR_GRAVITY(5) and TR_LINEAR(2) fragments. These numbers are
 * native input lanes and MUST NOT be confused with port trajectory enums. */
#include <math.h>
#include <stdint.h>
static int C1DM_Evaluate(int gravity,int32_t start,int32_t now,const float base[3],const float delta[3],float position[3],float velocity[3]){
 double t;float p[3],v[3];int64_t elapsed=(int64_t)now-start;unsigned i;
 if((gravity!=0&&gravity!=1)||elapsed<INT32_MIN||elapsed>INT32_MAX||!base||!delta||!position||!velocity)return 0;
 t=(double)(int32_t)elapsed*0.0010000000474974513f;
 for(i=0;i<3;i++){if(!isfinite(base[i])||!isfinite(delta[i]))return 0;p[i]=(float)((double)t*delta[i]+base[i]);v[i]=delta[i];}
 if(gravity){p[2]=(float)((double)t*delta[2]+base[2]-(double)t*t*400.0f);v[2]=(float)((double)delta[2]-(double)t*800.0f);}
 for(i=0;i<3;i++)if(!isfinite(p[i])||!isfinite(v[i]))return 0;
 for(i=0;i<3;i++){position[i]=p[i];velocity[i]=v[i];}return 1;
}
#endif
