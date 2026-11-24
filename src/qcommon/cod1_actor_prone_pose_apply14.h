#ifndef COD1_ACTOR_PRONE_POSE_APPLY14_H
#define COD1_ACTOR_PRONE_POSE_APPLY14_H
#include "cod1_actor_source_pose14.h"
#include "cod1_actor_dobj_pose14.h"
/* The original controller claims tag_origin before ordinary bone sampling.
 * This root has no parent/attachment/duplicate. Its owned numeric projection
 * replaces that local channel before world composition. weight1 here selects
 * an explicit local channel in C1AD14; native controller's unused word is0. */
static int C1APP14_Apply(const c1ad14Resource_t*r,const float v[6],c1ab14Accumulator_t*a,unsigned count){
 c1ab14Accumulator_t root;int bone;
 if(!r||!r->assembly||!a||count!=r->assembly->topology.boneCount||!C1ASP14_ControllerValid(v))return 0;
 if(!C1ASP14_Bits(v[0]))return 1;
 bone=C1AT14_Find(r->assembly,"tag_origin");
 if(bone!=0||r->assembly->topology.bones[0].parentBone!=-1||r->assembly->topology.bones[0].duplicateBone>=0)return 0;
 memset(&root,0,sizeof(root));memcpy(root.q,v+1,16);root.weight=1;root.t[2]=v[5];a[0]=root;return 1;
}
#endif
