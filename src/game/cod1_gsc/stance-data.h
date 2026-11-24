#ifndef R243_STANCE_DATA_H
#define R243_STANCE_DATA_H
/* Exact proved branch only. Missing physical path data is NOT empty input.
 * Caller must establish owned canonical path count and goal-node value. */
static int R243StanceNoPathData(uint32_t actorMask,int16_t pathCount,uint32_t goalNode,uint32_t bit,int*out){
 if(!out||pathCount!=0||goalNode!=0||(bit!=1&&bit!=2&&bit!=4))return 0;
 *out=(actorMask&bit)!=0;return 1;
}
/* GAME 200684AB: use actorMask & pathMask if nonempty, else actorMask.
 * An excluded bit stays excluded. A sole enabled bit always survives. */
static int Cod1StanceIndependentOfPath(uint32_t actorMask,uint32_t bit,int *out){
 if(!out || (actorMask & ~7u) || (bit!=1 && bit!=2 && bit!=4))return 0;
 if((actorMask & bit) && actorMask!=bit)return 0;
 *out=(actorMask & bit)!=0;
 return 1;
}
#endif
