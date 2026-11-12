#ifndef BG_COD1_ACTOR_TELEPORT_ANGLES14_H
#define BG_COD1_ACTOR_TELEPORT_ANGLES14_H
/* Optional-vector branch of GAME67D20, using original54AA0/54B10/0EB00.
 * Private computation only; no allocation/Session/spatial authority here.
 * Entity keeps raw pitch/yaw/roll. Actor orientation uses quantized pitch/yaw
 * and its look basis uses roll0. Existing allocation.cell0120 is unchanged.
 * All buffers must be separate private storage. No live publication until
 * the actual GAME owner can retain every dependency through its callbacks.
 * Selected angle domain: normal/zero finite float32, absolute value<=1e6.
 * Quantized cells are bit-exact; libm look basis has absolute tolerance2e-6
 * against the original x87 helper. Exceptional inputs remain unsupported. */
#include "bg_cod1_actor_teleport14.h"
#include "bg_cod1_actor_immediate14.h"
typedef struct {
 float angles[3],trajectory[3],delta[3];
 uint32_t type,time,duration;
 float pitch,yaw,yawCopy,lookPitch,lookYaw;
 float forward[3],right[3],up[3];
} C1TP14Angles;
static int C1TP14PrepareAngles(C1TP14Angles*out,const float*angles){
 C1TP14Angles next;uint32_t bits[3],q[5],radBits=0x3c8efa35u;
 float rad,p,y,sp,cp,sy,cy;unsigned i;
 if(!out||!angles)return 0;
 memcpy(bits,angles,12);
 for(i=0;i<3;i++)if(!C1AI14_OrientAngle(bits[i],q))return 0;
 memset(&next,0,sizeof(next));memcpy(next.angles,angles,12);memcpy(next.trajectory,angles,12);
 C1AI14_OrientAngle(bits[0],q);memcpy(&next.pitch,q+2,4);
 C1AI14_OrientAngle(bits[1],q);memcpy(&next.yaw,q+2,4);
 next.yawCopy=next.lookYaw=next.yaw;next.lookPitch=next.pitch;
 memcpy(&rad,&radBits,4);p=(float)((double)next.pitch*rad);y=(float)((double)next.yaw*rad);
 sp=(float)sin((double)p);cp=(float)cos((double)p);sy=(float)sin((double)y);cy=(float)cos((double)y);
 next.forward[0]=(float)((double)cp*cy);next.forward[1]=(float)((double)cp*sy);next.forward[2]=-sp;
 next.right[0]=sy;next.right[1]=-cy;next.right[2]=0;
 next.up[0]=(float)((double)sp*cy);next.up[1]=(float)((double)sp*sy);next.up[2]=cp;
 *out=next;return 1;
}
/* Native getters have already completed. hasAngles is exactly argc>1, not a
 * replacement arity check: the original still attempts getter0 for argc0.
 * Native refusal leaves both records untouched. Failed query is a separate
 * host failure and never returns a successful script boolean. */
static int C1TP14ApplyOptional(C1TP14State*state,C1TP14Angles*orientation,
 const float*goal,int hasAngles,const float*angles,uint32_t phase,
 const float*eye,float limit,C1TP14Trace trace,void*ctx,int*result){
 C1TP14State before,next;C1TP14Angles oldAngles,newAngles;float savedGoal[3],savedAngles[3];int value=-1;
 if(!state||!orientation||!result||!goal||(hasAngles!=0&&hasAngles!=1))return 0;
 before=next=*state;oldAngles=*orientation;memcpy(savedGoal,goal,12);
 if(hasAngles){if(!C1TP14PrepareAngles(&newAngles,angles))return 0;memcpy(savedAngles,angles,12);}
 if(!C1TP14Apply(&next,goal,phase,eye,limit,trace,ctx,&value))return 0;
 if(memcmp(state,&before,sizeof(before))||memcmp(orientation,&oldAngles,sizeof(oldAngles))||
    memcmp(goal,savedGoal,12)||(hasAngles&&memcmp(angles,savedAngles,12)))return 0;
 if(value){*state=next;if(hasAngles)*orientation=newAngles;}
 *result=value;return 1;
}
#endif
