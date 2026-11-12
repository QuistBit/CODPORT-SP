#ifndef COD1_TURRET_ANIMATED_POSE14_H
#define COD1_TURRET_ANIMATED_POSE14_H
/* Current real animation accumulators plus original MG42 bone controls.
 * Model-local output only. Caller owns the exact model/tree/tick/allocation. */
#include "cod1_actor_world14.h"
#include "cod1_native_turret_control.h"
static C1AB14_NOINLINE int C1TAP14_BuildInner(const c1aw14Skeleton_t*s,const c1ab14Accumulator_t*a,const float angles[3],c1aw14Pose_t*out){
 c1aw14Pose_t pose,checked;c1ntcTurretControl_t control;int indices[3]={-1,-1,-1};unsigned i,k;int p,which;
 if(!s||!a||!angles||!out||s->source.boneCount!=13||s->source.rootCount!=1||
    !C1AW14_Build(s,a,13,&checked)||!C1NTC_Turret(angles,&control)||
    C1AB14_Overlap(s,sizeof(*s),out,sizeof(*out))||C1AB14_Overlap(a,13*sizeof(*a),out,sizeof(*out))||C1AB14_Overlap(angles,12,out,sizeof(*out)))return 0;
 for(i=0;i<13;i++){
  const char*n=s->source.boneName[i];which=!strcmp(n,"tag_aim")?0:!strcmp(n,"tag_aim_animated")?1:!strcmp(n,"tag_flash")?2:-1;
  if(which>=0){if(indices[which]>=0||i==0)return 0;indices[which]=(int)i;}
 }
 for(i=0;i<3;i++)if(indices[i]<0)return 0;
 memset(&pose,0,sizeof(pose));pose.bones=13;
 for(i=0;i<13;i++){
  float q[4],local[3];p=s->source.parent[i];which=(int)i==indices[0]||(int)i==indices[1]?0:(int)i==indices[2]?1:-1;
  if(which>=0){memcpy(q,which?control.flash:control.aim,16);memset(local,0,12);}
  else{memcpy(q,a[i].weight!=0?a[i].q:s->bindQ[i],16);for(k=0;k<3;k++)local[k]=a[i].weight!=0?a[i].t[k]:0;}
  if(p>=0){for(k=0;k<3;k++)local[k]=(float)((double)s->source.local[i].p[k]+local[k]);
   if(which>=0)C1NTC_Product(q,pose.q[p],pose.q[i]);else C1AW14_Product(pose.q[p],q,pose.q[i]);
  }else memcpy(pose.q[i],q,16);
  C1AW14_Axis(pose.q[i],pose.matrix[i]);
  for(k=0;k<3;k++){float value=p<0?local[k]:(float)((double)pose.matrix[p][8+k]*local[2]+(double)pose.matrix[p][4+k]*local[1]+(double)pose.matrix[p][k]*local[0]+pose.matrix[p][12+k]);if(!(value>=-1000000&&value<=1000000))return 0;pose.matrix[i][12+k]=value;}
 }
 *out=pose;return 1;
}
static int C1TAP14_Build(const c1aw14Skeleton_t*s,const c1ab14Accumulator_t*a,const float angles[3],c1aw14Pose_t*out){int ok;
#if defined(_MSC_VER)&&defined(_M_IX86)
 unsigned char outer[108];uint32_t savedMx,nearestMx=0x1f80u;const unsigned short nearest=0x027f;
 __asm fnsave outer
 __asm stmxcsr savedMx
 __asm fldcw nearest
 __asm ldmxcsr nearestMx
 ok=C1TAP14_BuildInner(s,a,angles,out);
 __asm ldmxcsr savedMx
 __asm frstor outer
#else
 ok=0;
#endif
 return ok;
}
/* Original two-leaf mg42 tree body blend, mapped by actual clip bone names.
 * weights are real current node weights; a zero tree is not a posed gun. */
static C1AB14_NOINLINE int C1TAP14_SampleGunInner(const c1aw14Skeleton_t*s,const c1ab14Clip_t*const clips[2],const float phases[2],const float weights[2],const float angles[3],c1aw14Pose_t*out){
 c1ab14Accumulator_t local[128]={{0}},mapped[128]={{0}};unsigned char map[2][128];unsigned i,j;int any=0;
 if(!s||!clips||!phases||!weights||!angles||!out||s->source.boneCount!=13||
  C1AB14_Overlap(out,sizeof(*out),s,sizeof(*s))||C1AB14_Overlap(out,sizeof(*out),clips,2*sizeof(*clips))||
  C1AB14_Overlap(out,sizeof(*out),phases,8)||C1AB14_Overlap(out,sizeof(*out),weights,8)||C1AB14_Overlap(out,sizeof(*out),angles,12))return 0;
 for(i=0;i<2;i++){
  if(!clips[i]||clips[i]->bones!=9||!(phases[i]>=0&&phases[i]<=1)||!(weights[i]>=0&&weights[i]<=1)||!C1AW14_Map(s,clips[i],map[i]))return 0;
  if(C1AB14_Overlap(out,sizeof(*out),clips[i],sizeof(*clips[i])))return 0;
  for(j=0;j<9;j++){const c1ab14Bone_t*b=&clips[i]->bone[j];if(C1AB14_Overlap(out,sizeof(*out),b->qi,b->nq*sizeof(*b->qi))||C1AB14_Overlap(out,sizeof(*out),b->q,b->nq*sizeof(*b->q))||C1AB14_Overlap(out,sizeof(*out),b->ti,b->nt*sizeof(*b->ti))||C1AB14_Overlap(out,sizeof(*out),b->t,b->nt*sizeof(*b->t)))return 0;}
  if(i)for(j=0;j<9;j++)if(map[i][j]!=map[0][j])return 0;
  if(weights[i]>0){if(!C1AB14_Accumulate(clips[i],phases[i],weights[i],local,128))return 0;any=1;}
 }
 if(!any||!C1AB14_NormalizeGroup(local,9,1))return 0;
 for(j=0;j<9;j++)mapped[map[0][j]]=local[j];
 return C1TAP14_Build(s,mapped,angles,out);
}
static int C1TAP14_SampleGun(const c1aw14Skeleton_t*s,const c1ab14Clip_t*const clips[2],const float phases[2],const float weights[2],const float angles[3],c1aw14Pose_t*out){int ok;
#if defined(_MSC_VER)&&defined(_M_IX86)
 unsigned char outer[108];uint32_t savedMx,nearestMx=0x1f80u;const unsigned short nearest=0x027f;
 __asm fnsave outer
 __asm stmxcsr savedMx
 __asm fldcw nearest
 __asm ldmxcsr nearestMx
 ok=C1TAP14_SampleGunInner(s,clips,phases,weights,angles,out);
 __asm ldmxcsr savedMx
 __asm frstor outer
#else
 ok=0;
#endif
 return ok;
}
#endif
