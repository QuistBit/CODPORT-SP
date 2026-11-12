#ifndef G_COD1_DYNAMIC_PATH_PLACE_H
#define G_COD1_DYNAMIC_PATH_PLACE_H
#include "g_cod1_dynamic_path_bind.h"
/* Original 6D9F0 single-node placement. Candidate DATA only: the future live
 * producer must provide actual trace33 and the entity contents bracket6DBB0,
 * map lifetime, Begin-function cache and finalized native sort/remapping.
 * A valid callback result does not mean a valid node: native failure sets type0
 * but still publishes the returned endpoint. Refused callbacks publish nothing. */
typedef struct {float fraction,end[3];uint8_t allsolid,startsolid;} C1DPPlaceTrace;
typedef int (*C1DPPlaceQuery)(void*,const float*,const float*,const float*,const float*,unsigned,unsigned,C1DPPlaceTrace*);
enum {C1DP_PLACE_VALID=0,C1DP_PLACE_NO_FLOOR=1,C1DP_PLACE_SOLID=2,C1DP_PLACE_NO_CLEARANCE=3};
static int C1DP_PlaceCandidate(C1DPProjection*node,C1DPPlaceQuery trace,void*ctx,unsigned*reason){
 C1DPProjection value;C1DPPlaceTrace result;float start[3],end[3];unsigned why=0;
 static const float mins[3]={-15,-15,0},shortMax[3]={15,15,30},tallMax[3]={15,15,72};
 if(!node||!trace||!reason)return 0;value=*node;memcpy(start,node->origin,12);memcpy(end,node->origin,12);start[2]=node->origin[2]+1.0f;end[2]=node->origin[2]-256.0f;
 memset(&result,0,sizeof(result));if(!trace(ctx,start,mins,shortMax,end,0x7ff,0x00820011u,&result))return 0;
 if(result.allsolid||result.startsolid)why=C1DP_PLACE_SOLID;
 else if(result.fraction==1.0f)why=C1DP_PLACE_NO_FLOOR;
 else {memcpy(start,result.end,12);memcpy(end,result.end,12);memset(&result,0,sizeof(result));if(!trace(ctx,start,mins,tallMax,end,0x7ff,0x00820011u,&result))return 0;if(result.allsolid||result.startsolid)why=C1DP_PLACE_NO_CLEARANCE;}
 if(why)value.type=0;memcpy(value.origin,result.end,12);*node=value;*reason=why;return 1;
}
#endif
