#ifndef BG_COD1_DROP_BOUNCE_H
#define BG_COD1_DROP_BOUNCE_H
/* Original2001C890 reflection fragment. Trajectory, trace, placement and
 * surface alignment remain caller responsibilities; no active provider. */
#include <math.h>
static int C1DB_Reflect(const float velocity[3],const float normal[3],float bounce,float out[3]){
 float dot,twice,stage[3];unsigned i;if(!velocity||!normal||!out||!isfinite(bounce))return 0;
 for(i=0;i<3;i++)if(!isfinite(velocity[i])||!isfinite(normal[i]))return 0;
 dot=(float)((double)velocity[0]*normal[0]+(double)velocity[2]*normal[2]+(double)velocity[1]*normal[1]);
 twice=(float)((double)dot*-2.0);
 for(i=0;i<3;i++){float reflected=(float)((double)twice*normal[i]+velocity[i]);stage[i]=(float)((double)reflected*bounce);if(!isfinite(stage[i]))return 0;}
 for(i=0;i<3;i++)out[i]=stage[i];return 1;
}
#endif
