#ifndef G_COD1_ACTOR_POSTINIT_POSITION_H
#define G_COD1_ACTOR_POSTINIT_POSITION_H
#include <stdint.h>
#include <string.h>
#include "../qcommon/cod1_angle_normalize14.h"
/* Typed projection for739A0(position==entity E0) followed by107D0 and the
 * 4D84C sentient20 store. Empty path/no enemy/no node claims only. This is
 * neither736A0 target assignment nor INIT/controller/publication authority. */
typedef struct {
 float position[3],goal[3],yaw,radius,cell20;
 uint32_t node8,node40,node4c,enemy,path6f4,cell774,cell7e4;
 int16_t cursor6fa;unsigned char cell78f;
} C1PostinitPosition;
static int C1PostinitPositionApply(C1PostinitPosition*s){
 C1PostinitPosition v;float yaw;unsigned i;int changed=0;
 if(!s)return 0;v=*s;
 if(v.node8||v.node40||v.node4c||v.enemy||(int16_t)v.path6f4||v.cursor6fa||
    !isfinite(v.radius)||v.radius<0||!C1AngleNormalize14(v.yaw,&yaw))return 0;
 for(i=0;i<3;i++){
  if(!isfinite(v.position[i])||!isfinite(v.goal[i]))return 0;
  if(v.position[i]!=v.goal[i])changed=1;
 }
 /* Equal signed-zero positions take the original no-change path; preserve
  * raw goal bits and all Actor cells in that branch. */
 v.cell20=-1.0f;
 if(changed){memcpy(v.goal,v.position,sizeof(v.goal));v.node8=0;
  v.path6f4=0;v.cell774=0;v.cell7e4=0;v.cell78f=1;}
 v.cell20=yaw;*s=v;return 1;
}
#endif
