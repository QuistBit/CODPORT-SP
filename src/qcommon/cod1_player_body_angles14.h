#ifndef COD1_PLAYER_BODY_ANGLES14_H
#define COD1_PLAYER_BODY_ANGLES14_H
#include <stdint.h>
#include <string.h>
/* GAME12861..128B2, common post-move body publication: positive zero pitch
 * and roll, bit-preserved playerState yaw. Body .angles is not viewangles.
 * Stage before writes so even overlapping copied buffers are well-defined. */
static void C1PBA14_Angles(const float view[3],float body[3]){
 uint32_t bits[3]={0,0,0};memcpy(bits+1,view+1,4);memcpy(body,bits,12);
}
#endif
