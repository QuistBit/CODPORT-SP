#ifndef COD1_ACTOR_CHECK_PRONE14_H
#define COD1_ACTOR_CHECK_PRONE14_H
#include "../game/bg_cod1_actor_prone14.h"
/* GAME69D10 passes fixed radius15/height24, caller yaw/skip-first-test,
 * no pose outputs, required terrain, no supplied normal, native mask mode1.
 * The existing reconstructed1B90 geometry is shared, not approximated. */
static int C1CP14_Apply(int slot,const float origin[3],float yaw,int already,
 c1ap14TraceFn capsule,c1ap14TraceFn box,int*out){
 uint32_t bits[4],mx,nearMx=0x1f80;float local[3];int value,i;
 unsigned char saved[108];const unsigned short cw=0x027f;
 if(slot<0||slot>=2048||!origin||!capsule||!box||!out)return 0;
 memcpy(bits,origin,12);memcpy(bits+3,&yaw,4);
 for(i=0;i<4;i++)if((bits[i]&0x7fffffffu)>(i==3?0x45610000u:0x49800000u)||
    ((bits[i]&0x7fffffffu)&&(bits[i]&0x7fffffffu)<0x00800000u))return 0;
 memcpy(local,origin,12);
 __asm fnsave saved
 __asm stmxcsr mx
 __asm fldcw cw
 __asm ldmxcsr nearMx
 value=C1AP14_QueryRaw(slot,local,15,24,yaw,NULL,NULL,NULL,already,1,NULL,capsule,box,1);
 __asm ldmxcsr mx
 __asm frstor saved
 if((value!=0&&value!=1)||memcmp(local,origin,12))return 0;
 *out=value;return 1;
}
#endif
