#ifndef COD1_ACTOR_DOBJ_POSE14_H
#define COD1_ACTOR_DOBJ_POSE14_H
#include "cod1_actor_tags14.h"
/* Actual full ordered DObj raw binds plus current accumulated bone channels.
 * Duplicate bones copy their canonical current pose. No BODY-only attachment
 * fallback: head/eyes accumulators participate in the same current hierarchy. */
typedef struct {c1at14Resource_t*assembly;c1aw14Skeleton_t*raw[8];} c1ad14Resource_t;
static void C1AD14_Free(c1ad14Resource_t*r){unsigned i;if(r){for(i=1;i<8;i++)C1AW14_Free(r->raw[i]);C1AT14_Free(r->assembly);free(r);}}
static int C1AD14_Create(const c1at14Input_t*in,unsigned count,c1ad14Resource_t**out){c1ad14Resource_t*r;unsigned i;if(!in||!out||*out||!count||count>8||C1AB14_Overlap(in,count*sizeof(*in),out,sizeof(*out)))return 0;for(i=0;i<count;i++)if(C1AB14_Overlap(out,sizeof(*out),in[i].model,in[i].modelBytes)||C1AB14_Overlap(out,sizeof(*out),in[i].parts,in[i].partsBytes))return 0;r=calloc(1,sizeof(*r));if(!r)return 0;if(!C1AT14_Create(in,count,&r->assembly))goto bad;r->raw[0]=r->assembly->body;for(i=1;i<count;i++)if(!C1AW14_Decode(in[i].parts,in[i].partsBytes,&r->raw[i]))goto bad;*out=r;return 1;bad:C1AD14_Free(r);return 0;}
static int C1AD14_Map(const c1ad14Resource_t*r,const c1ab14Clip_t*c,unsigned char map[128]){unsigned char staged[128],used[128]={0};unsigned i;int index;if(!r||!r->assembly||!r->assembly->count||r->assembly->count>8||!c||!map||c->magic!=C1AB14_MAGIC||!c->bones||c->bones>=128||C1AB14_Overlap(r,sizeof(*r),map,c->bones)||C1AB14_Overlap(r->assembly,sizeof(*r->assembly),map,c->bones)||C1AB14_Overlap(c,sizeof(*c),map,c->bones))return 0;
 for(i=0;i<r->assembly->count;i++)if(r->raw[i]&&C1AB14_Overlap(r->raw[i],sizeof(*r->raw[i]),map,c->bones))return 0;
 for(i=0;i<c->bones;i++){const c1ab14Bone_t*t=c->bone+i;if(C1AB14_Overlap(t->qi,t->nq*2,map,c->bones)||C1AB14_Overlap(t->q,t->nq*8,map,c->bones)||C1AB14_Overlap(t->ti,t->nt*2,map,c->bones)||C1AB14_Overlap(t->t,t->nt*12,map,c->bones)||!C1DT_String(c->bone[i].name)||(index=C1AT14_Find(r->assembly,c->bone[i].name))<0||index>=128||used[index])return 0;used[index]=1;staged[i]=(unsigned char)index;}memcpy(map,staged,c->bones);return 1;}
static C1AB14_NOINLINE int C1AD14_BuildInner(const c1ad14Resource_t*r,const c1ab14Accumulator_t*a,unsigned n,c1aw14Pose_t*out){c1aw14Pose_t pose;c1dtTopology_t topology;c1dtModelInput_t inputs[8];unsigned i,k;int p;float local[3],q[4];
 if(!r||!r->assembly||!a||!out||!n||n>=128||n!=r->assembly->topology.boneCount||C1AB14_Overlap(r,sizeof(*r),out,sizeof(*out))||C1AB14_Overlap(r->assembly,sizeof(*r->assembly),out,sizeof(*out))||C1AB14_Overlap(a,n*sizeof(*a),out,sizeof(*out)))return 0;
 if(!r->assembly->count||r->assembly->count>8)return 0;for(i=0;i<r->assembly->count;i++){if(!r->raw[i]||r->raw[i]->magic!=C1AW14_MAGIC||C1AB14_Overlap(r->raw[i],sizeof(*r->raw[i]),out,sizeof(*out)))return 0;inputs[i].resource=r->assembly->part+i;inputs[i].tagName=r->assembly->tag[i];}
 if(C1DT_Build(inputs,r->assembly->count,&topology)!=C1DT_OK||memcmp(&topology,&r->assembly->topology,sizeof(topology)))return 0;memset(&pose,0,sizeof(pose));pose.bones=n;
 for(i=0;i<n;i++){const c1dtBone_t*b=topology.bones+i;const c1aw14Skeleton_t*s=r->raw[b->modelIndex];unsigned localBone=b->localBone;p=b->parentBone;
  if(localBone>=s->source.boneCount||p>=(int)i||p< -1)return 0;for(k=0;k<8;k++)if(!(((const float*)&a[i])[k]>=-1000000&&((const float*)&a[i])[k]<=1000000))return 0;if(!(a[i].weight>=0&&a[i].weight<=128))return 0;
  if(b->duplicateBone>=0){if(b->duplicateBone>=(int)i)return 0;memcpy(pose.q[i],pose.q[b->duplicateBone],16);memcpy(pose.matrix[i],pose.matrix[b->duplicateBone],64);continue;}
  for(k=0;k<4;k++)q[k]=a[i].weight!=0?a[i].q[k]:s->bindQ[localBone][k];for(k=0;k<3;k++){local[k]=a[i].weight!=0?a[i].t[k]:0;if(p>=0)local[k]=(float)((double)s->source.local[localBone].p[k]+local[k]);}
  if(p>=0)C1AW14_Product(pose.q[p],q,pose.q[i]);else memcpy(pose.q[i],q,16);C1AW14_Axis(pose.q[i],pose.matrix[i]);
  for(k=0;k<3;k++){float value=p<0?local[k]:(float)((double)pose.matrix[p][8+k]*local[2]+(double)pose.matrix[p][4+k]*local[1]+(double)pose.matrix[p][k]*local[0]+pose.matrix[p][12+k]);if(!(value>=-1000000&&value<=1000000))return 0;pose.matrix[i][12+k]=value;}
 }
 memcpy(out,&pose,sizeof(pose));return 1;
}
static int C1AD14_Build(const c1ad14Resource_t*r,const c1ab14Accumulator_t*a,unsigned n,c1aw14Pose_t*out){int ok;
#if defined(_MSC_VER)&&defined(_M_IX86)
 unsigned char saved[108];uint32_t mx,nearMx=0x1f80u;const unsigned short cw=0x027f;
 __asm fnsave saved
 __asm stmxcsr mx
 __asm fldcw cw
 __asm ldmxcsr nearMx
 ok=C1AD14_BuildInner(r,a,n,out);
 __asm ldmxcsr mx
 __asm frstor saved
#else
 ok=C1AD14_BuildInner(r,a,n,out);
#endif
 return ok;}
#endif
