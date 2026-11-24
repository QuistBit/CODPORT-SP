#ifndef BG_COD1_TURRET_INITIAL_POSE14_H
#define BG_COD1_TURRET_INITIAL_POSE14_H
/* GAME22630: model-local tag_aim/tag_butt -> up to31 world traces.
 * This component prepares geometry only. The owner must obtain actual current
 * tags, run mask0x11 locational traces in order, stop on first fraction<1,
 * and fence the entity/map/pose around every callback. A missing actual tag
 * skips this geometry; it does not invent a tag at the origin.
 * Source authority: retained R231 native-manual-turret-tick-authority.json. */
#include "bg_cod1_turret_constructor14.h"
#include "../qcommon/cod1_actor_scripted_math14.h"
typedef struct {float start[3],end[31][3],pitch[31];} C1TI14Plan;
/* Native due dispatcher1E8F0 and22630/22960: one update at the current
 * server time, never a catch-up loop. Return0 leaves outputs untouched. */
static int C1TI14_Schedule(int now,int due,int handler,int*next,int*nextHandler){
 if(!next||!nextHandler||next==nextHandler||now<0||now>2147483597||(handler!=12&&handler!=13))return -1;
 if(due<=0||due>now)return 0;*next=now+50;*nextHandler=12;return 1;
}
static int C1TI14_Vector(const float*v,uint32_t bound){unsigned i;uint32_t bits;if(!v)return 0;for(i=0;i<3;i++){memcpy(&bits,v+i,4);if((bits&0x7fffffffu)>bound)return 0;}return 1;}
static int C1TI14_Prepare(const float origin[3],const float angles[3],const float aim[3],const float butt[3],C1TI14Plan*out){
 C1TI14Plan p;float matrix[12],delta[3],rotation[9],turn[3]={0,0,0},local[3];unsigned i,j;
 unsigned char env[108];uint32_t mx,nearMx=0x1f80;const unsigned short cw=0x027f;
 if(!out||!C1TI14_Vector(origin,0x49742400)||!C1TI14_Vector(angles,0x45610000)||!C1TI14_Vector(aim,0x49742400)||!C1TI14_Vector(butt,0x49742400))return 0;
 if(C1TS14_Overlap(out,sizeof(*out),origin,12)||C1TS14_Overlap(out,sizeof(*out),angles,12)||C1TS14_Overlap(out,sizeof(*out),aim,12)||C1TS14_Overlap(out,sizeof(*out),butt,12))return 0;
 __asm fnsave env
 __asm stmxcsr mx
 __asm fldcw cw
 __asm ldmxcsr nearMx
 C1ASM14_Matrix(angles,matrix);memcpy(matrix+9,origin,12);
 C1ASM14_Point(matrix,aim,0,p.start);
 for(j=0;j<3;j++)delta[j]=(float)((double)butt[j]-aim[j]);
 for(i=0;i<31;i++){
  turn[0]=p.pitch[i]=(float)((double)(float)i*-3.0);
  C1ASM14_Matrix(turn,rotation);
  local[0]=(float)(((double)rotation[6]*delta[2]+(double)rotation[3]*delta[1])+(double)rotation[0]*delta[0]);
  local[1]=(float)(((double)rotation[7]*delta[2]+(double)rotation[1]*delta[0])+(double)rotation[4]*delta[1]);
  local[2]=(float)(((double)rotation[8]*delta[2]+(double)rotation[2]*delta[0])+(double)rotation[5]*delta[1]);
  for(j=0;j<3;j++)local[j]=(float)((double)local[j]+aim[j]);
  for(j=0;j<3;j++)p.end[i][j]=(float)((((double)matrix[6+j]*local[2]+(double)matrix[3+j]*local[1])+(double)matrix[j]*local[0])+(double)origin[j]);
 }
 __asm ldmxcsr mx
 __asm frstor env
 *out=p;return 1;
}
#endif
