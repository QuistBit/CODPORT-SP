#ifndef COD1_ACTOR_DAMAGE_YAW14_H
#define COD1_ACTOR_DAMAGE_YAW14_H
#include <stdint.h>
#include <math.h>
/* Native E6F0 -> float delta ->10770 ->84140. Constants retain the original
 * float representations. Finite direction is not normalized; zero XY is0.
 * The native84140 uses a signed64-bit FISTP. Out-of-range yields integer-indefinite, whose low16
 * bits are zero. Avoid an undefined C float-to-int overflow. */
static int C1ADY14_Compute(const float*direction,float heading,int32_t*out){
 float yaw,delta,angle;double scaled;int64_t turns;
 if(!direction||!out||!isfinite(heading)||!isfinite(direction[0])||
    !isfinite(direction[1])||!isfinite(direction[2]))return 0;
 if(direction[0]==0&&direction[1]==0)yaw=0;
 else{yaw=(float)(atan2((double)direction[1],(double)direction[0])*180.0/(double)3.1415927410125732f);
  if(yaw<0)yaw=(float)(yaw+360.0f);}
 delta=(float)(yaw-heading);
 scaled=(double)delta*(double)182.04444885253906f;
 turns=scaled>=9223372036854775808.0||scaled<=-9223372036854775808.0?INT64_MIN:(int64_t)scaled;
 angle=(float)(((uint32_t)turns&65535u)*0.0054931640625f);
 if(angle>180)angle=(float)(angle-360.0f);
 *out=(int32_t)angle;return 1;
}
#endif
