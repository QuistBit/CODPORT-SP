#ifndef BG_COD1_TURRET_TARGET_H
#define BG_COD1_TURRET_TARGET_H
/* R232 bounded script_origin target/control dependency, original GAME
 * 20020F80/21940/213A0/20CA0 and IsFiringTurret20032200.
 * Caller owns current real muzzle, native resource/base transform, entity
 * allocation/epoch, target lifetime and 50ms think scheduling. No mount,
 * current DObj acquisition, GSC binding, shot, animation or sound admission.
 * Includes original mounted height-limit pitch flags 0x100/0x200/0x400.
 * Rejections are atomic. A rejected target clears tracking bit only when
 * explicitly requested; native state/IsFiring is NOT permission to shoot. */
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <limits.h>
typedef struct {float minPitch,maxPitch,minYaw,maxYaw,pitchRate,yawRate;} c1ttConfig_t;
typedef struct {
 float pitch,yaw,pitchCorrection,pitchLimit; /* limit = native private+0x98 */
 uint32_t flags; /* native private+8, valid tracking 0x40 */
 int trackingState; /* native private+4: 0,2,1; IsFiring means !=0 */
 int targetChangedMs,lastTickMs;
 uint64_t targetIdentity;int hasTarget;
 float trackedPoint[3];
} c1ttState_t;
typedef struct {int turretStateChange;int previousState,newState;} c1ttTransition_t;
static int C1TT_Finite(float x,float bound){return x>=-bound&&x<=bound;}
static int C1TT_Config(const c1ttConfig_t *c){return c&&C1TT_Finite(c->minPitch,180)&&C1TT_Finite(c->maxPitch,180)&&C1TT_Finite(c->minYaw,180)&&C1TT_Finite(c->maxYaw,180)&&c->minPitch<=c->maxPitch&&c->minYaw<=c->maxYaw&&c->pitchRate>=0&&c->pitchRate<=360&&c->yawRate>=0&&c->yawRate<=360;}
/* Original wraps only values >180 or <-180; both boundary signs survive. */
static float C1TT_AngleSubtract(float a,float b){float d=a-b;while(d>180)d-=360;while(d< -180)d+=360;return d;}
static int C1TT_State(const c1ttState_t *s){return s&&C1TT_Finite(s->pitch,3600)&&C1TT_Finite(s->yaw,3600)&&C1TT_Finite(s->pitchCorrection,3600)&&s->trackingState>=0&&s->trackingState<=2&&s->targetChangedMs>=0&&s->lastTickMs>=-50&&(!(s->flags&0x200u)||C1TT_Finite(s->pitchLimit,3600));}
static int C1TT_Aim(const c1ttConfig_t *c,const float muzzleWorld[3],const float targetWorld[3],const float baseAngles[3],float desiredPitchYaw[2]){
 float delta[3],horizontal,angles[2];int i;const double radiansToDegrees=57.2957795130823208768;
 if(!C1TT_Config(c)||!muzzleWorld||!targetWorld||!baseAngles||!desiredPitchYaw)return 0;
 for(i=0;i<3;i++){if(!C1TT_Finite(muzzleWorld[i],1000000)||!C1TT_Finite(targetWorld[i],1000000)||!C1TT_Finite(baseAngles[i],3600))return 0;delta[i]=targetWorld[i]-muzzleWorld[i];}
 if(delta[0]==0&&delta[1]==0){angles[0]=delta[2]>0?-90.f:90.f;angles[1]=0;}
 else {horizontal=(float)sqrt((double)delta[0]*delta[0]+(double)delta[1]*delta[1]);angles[0]=(float)(-atan2((double)delta[2],(double)horizontal)*radiansToDegrees);angles[1]=(float)(atan2((double)delta[1],(double)delta[0])*radiansToDegrees);}
 angles[0]=C1TT_AngleSubtract(angles[0],baseAngles[0]);angles[1]=C1TT_AngleSubtract(angles[1],baseAngles[1]);
 if(angles[0]<c->minPitch||angles[0]>c->maxPitch||angles[1]<c->minYaw||angles[1]>c->maxYaw)return 0;
 memcpy(desiredPitchYaw,angles,sizeof(angles));return 1;
}
/* Call for the actual allocation-fenced target, even when unchanged. Identity
 * is caller-provided and must change for slot reuse; zero is never a target. */
static int C1TT_BeginTarget(c1ttState_t *state,uint64_t identity,int now){c1ttState_t s;
 if(!C1TT_State(state)||!identity||now<0||now>INT_MAX-250||now<state->lastTickMs)return 0;s=*state;
 if(!s.hasTarget||s.targetIdentity!=identity){s.targetIdentity=identity;s.hasTarget=1;s.targetChangedMs=now;s.flags&=~8u;}
 *state=s;return 1;
}
static float C1TT_Approach(float current,float desired,float maxStep){float d=C1TT_AngleSubtract(desired,current);if(d>maxStep)d=maxStep;else if(d< -maxStep)d=-maxStep;return current+d;}
/* Exactly one native think update. Late callers get one 50ms step, never a
 * catch-up loop. Native state gate is evaluated BEFORE current-angle step.
 * Explicit 1500ms convergence is NOT used on this script_origin route. */
static int C1TT_Tick(const c1ttConfig_t *c,c1ttState_t *state,const float desiredPitchYaw[2],const float targetWorld[3],int firingRequested,int now,c1ttTransition_t *transition){
 c1ttState_t s;c1ttTransition_t t;float effective,pitchStep,yawStep,pitchRate,bounded;int i,eligible;
 if(!C1TT_Config(c)||!C1TT_State(state)||!state->hasTarget||!state->targetIdentity||!desiredPitchYaw||!targetWorld||!transition||(firingRequested!=0&&firingRequested!=1)||now<0||now>INT_MAX-250||now<state->targetChangedMs||now-state->lastTickMs<50)return 0;
 if(!C1TT_Finite(desiredPitchYaw[0],180)||!C1TT_Finite(desiredPitchYaw[1],180)||desiredPitchYaw[0]<c->minPitch||desiredPitchYaw[0]>c->maxPitch||desiredPitchYaw[1]<c->minYaw||desiredPitchYaw[1]>c->maxYaw)return 0;
 for(i=0;i<3;i++)if(!C1TT_Finite(targetWorld[i],1000000))return 0;s=*state;t.previousState=s.trackingState;t.turretStateChange=0;
 effective=s.pitch+s.pitchCorrection;
 if(!firingRequested)s.trackingState=0;
 else {
  eligible=1;
  if(s.trackingState==0){if(now-s.targetChangedMs<250&&(fabsf(C1TT_AngleSubtract(effective,desiredPitchYaw[0]))>=5||fabsf(C1TT_AngleSubtract(s.yaw,desiredPitchYaw[1]))>=5))eligible=0;else s.trackingState=2;}
  if(eligible&&s.trackingState==2&&(double)fabsf(C1TT_AngleSubtract(effective,desiredPitchYaw[0]))<(double)c->pitchRate*(double).2f&&(double)fabsf(C1TT_AngleSubtract(s.yaw,desiredPitchYaw[1]))<(double)c->yawRate*(double).2f)s.trackingState=1;
 }
 s.flags|=0x40u;memcpy(s.trackedPoint,targetWorld,12);
 pitchRate=c->pitchRate;
 /* Native20D00: mounted height correction raises slow pitch rate to360. */
 if((s.flags&0x300u)==0x300u&&pitchRate<360)pitchRate=360;
 pitchStep=pitchRate*.05f;yawStep=c->yawRate*.05f;
 effective=C1TT_Approach(effective,desiredPitchYaw[0],pitchStep);bounded=effective;
 if(s.flags&0x200u){
  if(s.flags&0x400u){if(effective<s.pitchLimit)bounded=s.pitchLimit;else s.flags&=~0x100u;}
  else {if(effective>s.pitchLimit)bounded=s.pitchLimit;else s.flags&=~0x100u;}
 }
 s.pitch=C1TT_Approach(s.pitch,bounded,pitchStep);s.pitchCorrection=effective-s.pitch;
 s.yaw=C1TT_Approach(s.yaw,desiredPitchYaw[1],yawStep);s.lastTickMs=now;
 t.newState=s.trackingState;t.turretStateChange=(t.previousState==0)!=(t.newState==0);*state=s;*transition=t;return 1;
}
static int C1TT_IsFiring(const c1ttState_t *state){return C1TT_State(state)&&state->trackingState!=0;}
/* Exact native aim refusal side effect; caller still owns stop/death cleanup. */
static int C1TT_RejectTarget(c1ttState_t *state){if(!C1TT_State(state))return 0;state->flags&=~0x40u;return 1;}
#endif
