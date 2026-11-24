#ifndef G_COD1_ACTOR_TAG_MATRIX584_H
#define G_COD1_ACTOR_TAG_MATRIX584_H
#include "../../qcommon/cod1_actor_world14.h"
/* Tag queries consume one world matrix. Model-space hierarchy has already
 * been built and validated for all bones. For conservatively bounded input,
 * every unused world transform is guaranteed finite, so only the requested
 * matrix needs arithmetic. Unusual inputs retain the complete original path,
 * including refusal caused by an unrequested bone. No pose or owner is cached.
 * The native transform retains its exact arithmetic and caller FP state. */
static int C1AW584_TagMatrix(const c1aw14Pose_t *model,const float origin[3],
 const float angles[3],unsigned bone,float out[16]){
 float matrix[16],zero[3]={0,0,0};unsigned i,j,k;uint32_t bits;
 if(!model||!origin||!angles||!out||!model->bones||model->bones>=128||bone>=model->bones)return 0;
 /* 2^60 leaves ample headroom: each bounded-angle basis component is at
  * most 2 in magnitude. Three products plus origin cannot overflow float. */
 for(j=0;j<3;j++){memcpy(&bits,origin+j,4);if((bits&0x7fffffffu)>0x5d800000u)goto full;}
 for(i=0;i<model->bones;i++)for(k=0;k<4;k++)for(j=0;j<3;j++){
  memcpy(&bits,model->matrix[i]+4*k+j,4);if((bits&0x7fffffffu)>0x5d800000u)goto full;
 }
 memcpy(matrix,model->matrix[bone],64);
 if(!C1NSO_Transform(origin,angles,model->matrix[bone]+12,matrix+12))return 0;
 for(k=0;k<3;k++)if(!C1NSO_Transform(zero,angles,model->matrix[bone]+k*4,matrix+k*4))return 0;
 memcpy(out,matrix,64);return 1;
full:
 {c1aw14Pose_t placed;if(!C1AW14_Place(model,origin,angles,&placed))return 0;memcpy(out,placed.matrix[bone],64);return 1;}
}
#endif
