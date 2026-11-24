/* Native51A30 controller is sampled from the same fenced actor as its tree.
 * A read-only tag/snapshot consumer never changes the simulation timer. */
#include "../../qcommon/cod1_actor_prone_controller14.h"
static int ActorPronePosePacket(const cod1ActorRecord_t*a,int now,float out[6]){
 c1apr14State_t s;const uint32_t*z=a->nativeData.actor.zeroCells;float raw[8]={0},v[6]={0};int active;
 memset(&s,0,sizeof(s));memcpy(s.nodes,a->nativeData.actor.proneNodes16c,12);
 memcpy(s.inverse,z,8);memcpy(&s.angle,z+2,4);s.ground=a->nativeData.actor.proneGround188;memcpy(&s.disabled,z+3,24);
 active=C1APR14_Controller(&s,now,raw);
 if(active){v[0]=1;memcpy(v+1,raw,16);v[5]=raw[7];}
 if(!C1ASP14_ControllerValid(v))return 0;memcpy(out,v,sizeof(v));return 1;
}
