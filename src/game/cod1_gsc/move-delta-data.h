#ifndef R243_MOVE_DELTA_DATA_H
#define R243_MOVE_DELTA_DATA_H
#include "../../qcommon/cod1_actor_root_interval14.h"
/* DATA query for an already owned immutable decoded leaf. It supplies neither
 * a typed VM animation nor resource/catalog/Session authority. Caller resolves
 * those separately. The declared finite domain is the original1..3 arguments;
 * native extra-argument and NaN error semantics remain outside this adapter. */
static int R243MoveDeltaData(const c1ar14Root_t*root,unsigned argc,float start,float end,float out[3]){
 const c1ar14Root_t*bank[1];c1ari14Node_t nodes[2];c1ari14Result_t result;
 if(!root||!out||argc<1||argc>3||C1ARI14_Overlap(out,12,root,sizeof(*root)))return 0;
 if(argc==1)start=0;if(argc<3)end=1;
 if(!C1ARI14_Finite(start)||!C1ARI14_Finite(end)||start<0||start>1||end<0||end>1)return 0;
 memset(nodes,0,sizeof(nodes));bank[0]=root;
 nodes[0].parent=-1;nodes[0].leaf=-1;
 nodes[1].ordinal=1;nodes[1].parent=0;nodes[1].leaf=0;
 nodes[1].currentWeight=nodes[1].goalWeight=1;nodes[1].oldPhase=start;nodes[1].phase=end;
 if(!C1ARI14_Interval(bank,1,nodes,2,0,&result))return 0;
 memcpy(out,result.translation,12);return 1;
}
#endif
