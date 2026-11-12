#ifndef BG_COD1_TURRET_POSE_H
#define BG_COD1_TURRET_POSE_H
/* Detached static MG42 resource pose dependency. It is not a current DObj,
 * registered/live turret, snapshot or permission to fire. The eventual live
 * owner must fence model identity, allocation, tick and prepared-pose lifetime.
 * Native ENGINE authority: normal parent*local, controlled local*parent;
 * controlled quaternion replaces bind rotation, never bind translation.
 * C1MR bind quaternions are normalized; this returns the same geometric pose
 * as original shortscaled quaternions, not native raw quaternion bit layout. */
#include "bg_cod1_turret_resources.h"
#include "../qcommon/cod1_native_turret_control.h"
typedef struct {unsigned bones;int flash;float q[C1MR_BONES_MAX][4],p[C1MR_BONES_MAX][3];} c1tpStaticPose_t;
static int C1TP_Finite(float v,float bound){return v>=-bound&&v<=bound;}
static void C1TP_Product(const float a[4],const float b[4],float out[4]){
 out[0]=a[3]*b[0]+a[1]*b[2]+a[0]*b[3]-a[2]*b[1];
 out[1]=a[3]*b[1]-a[0]*b[2]+a[2]*b[0]+a[1]*b[3];
 out[2]=a[0]*b[1]+a[3]*b[2]-a[1]*b[0]+a[2]*b[3];
 out[3]=a[3]*b[3]-a[0]*b[0]-a[1]*b[1]-a[2]*b[2];
}
static int C1TP_Axis(const float q[4],float axis[3][3]){
 float n=q[0]*q[0]+q[1]*q[1]+q[2]*q[2]+q[3]*q[3],s;
 if(!(n>1e-15f&&n<16))return 0;s=2/n;
 axis[0][0]=1-s*(q[1]*q[1]+q[2]*q[2]);axis[0][1]=s*(q[0]*q[1]+q[3]*q[2]);axis[0][2]=s*(q[0]*q[2]-q[3]*q[1]);
 axis[1][0]=s*(q[0]*q[1]-q[3]*q[2]);axis[1][1]=1-s*(q[0]*q[0]+q[2]*q[2]);axis[1][2]=s*(q[1]*q[2]+q[3]*q[0]);
 axis[2][0]=s*(q[0]*q[2]+q[3]*q[1]);axis[2][1]=s*(q[1]*q[2]-q[3]*q[0]);axis[2][2]=1-s*(q[0]*q[0]+q[1]*q[1]);return 1;
}
static int C1TP_StaticResource(const c1mrResource_t *resource,const float angles[3],c1tpStaticPose_t *out){
 C1TRModel model;c1ntcTurretControl_t controls;c1tpStaticPose_t pose;unsigned i;int k,parent,controlled;float local[4],axis[3][3];
 if(!resource||!angles||!out||resource->skeleton.rootCount!=1||!C1TR_Model(resource,&model)||!C1NTC_Turret(angles,&controls))return 0;
 memset(&pose,0,sizeof(pose));pose.bones=model.bones;pose.flash=model.flash;
 for(i=0;i<pose.bones;i++){
  const c1mrTransform_t *bind=&resource->skeleton.local[i];parent=resource->skeleton.parent[i];controlled=(int)i==model.aim||(int)i==model.animatedAim||(int)i==model.flash;
  if((i==0&&parent!=-1)||(i>0&&(parent<0||parent>=(int)i)))return 0;
  for(k=0;k<3;k++)if(!C1TP_Finite(bind->p[k],1000000))return 0;
  for(k=0;k<4;k++)if(!C1TP_Finite(bind->q[k],2))return 0;
  if(controlled)memcpy(local,(int)i==model.flash?controls.flash:controls.aim,16);else memcpy(local,bind->q,16);
  if(parent<0){/* Unattached no-animation source root has identity/zero. */
   if(controlled)return 0;for(k=0;k<3;k++)if(bind->p[k]!=0||bind->q[k]!=0)return 0;if(bind->q[3]!=1)return 0;pose.q[i][3]=1;continue;
  }
  if(controlled)C1TP_Product(local,pose.q[parent],pose.q[i]);else C1TP_Product(pose.q[parent],local,pose.q[i]);
  if(!C1TP_Axis(pose.q[parent],axis))return 0;
  for(k=0;k<3;k++){pose.p[i][k]=axis[2][k]*bind->p[2]+axis[1][k]*bind->p[1]+axis[0][k]*bind->p[0]+pose.p[parent][k];if(!C1TP_Finite(pose.p[i][k],1000000))return 0;}
  if(!C1TP_Axis(pose.q[i],axis))return 0;
 }
 memcpy(out,&pose,sizeof(pose));return 1;
}

#include "../qcommon/cod1_native_start_origin.h"
typedef struct {float position[3],forward[3];} c1tpWorldFlash_t;
/* A detached geometric transform of an already supplied model-local pose.
 * The live caller must still prove that pose belongs to this allocation,
 * model and authoritative simulation tick. This is never a fire permit. */
static int C1TP_WorldFlash(const c1tpStaticPose_t *pose,const float origin[3],const float baseAngles[3],c1tpWorldFlash_t *out){
 c1tpWorldFlash_t value;float axis[3][3],zero[3]={0,0,0};
 if(!pose||!origin||!baseAngles||!out||pose->bones!=13||pose->flash<0||pose->flash>=(int)pose->bones||!C1TP_Axis(pose->q[pose->flash],axis))return 0;
 if(!C1NSO_Transform(origin,baseAngles,pose->p[pose->flash],value.position)||!C1NSO_Transform(zero,baseAngles,axis[0],value.forward))return 0;
 memcpy(out,&value,sizeof(value));return 1;
}
#endif
