#ifndef COD1_ANGLE_NORMALIZE14_H
#define COD1_ANGLE_NORMALIZE14_H
#include <math.h>
/* Original GAME107D0 has separate negative and nonnegative returns.
 * In particular, a tiny negative rounds to360 and returns360. Every loop
 * iteration rounds through float memory. The finite bound is port admission,
 * not a claim about original NaN/infinite/nonprogressing inputs. */
static int C1AngleNormalize14(float input,float*out){
 volatile float v=input;
 if(!out||!isfinite(input)||input < -360000.0f||input > 360000.0f)return 0;
 if(v<0.0f){do{v=v+360.0f;}while(v<0.0f);}
 else{while(v>=360.0f)v=v-360.0f;}
 *out=v;return 1;
}
#endif
