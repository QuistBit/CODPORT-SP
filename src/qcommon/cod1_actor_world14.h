#ifndef COD1_ACTOR_WORLD14_H
#define COD1_ACTOR_WORLD14_H
/* Ordinary single-model current body skeleton. No synthetic pose/clock/actor
 * attachment. Source bind translation is added to animated delta translation. */
#include "cod1_actor_bone14.h"
#include "cod1_model_resource.h"
#include "cod1_native_start_origin.h"
#define C1AW14_MAGIC 0xc1a01402u
typedef struct {uint32_t magic;c1mrSkeleton_t source;float bindQ[128][4];} c1aw14Skeleton_t;
typedef struct {unsigned bones;float q[128][4],matrix[128][16];} c1aw14Pose_t;
static void C1AW14_Free(c1aw14Skeleton_t*s){free(s);}
static C1AB14_NOINLINE int C1AW14_DecodeInner(const void*data,size_t size,c1aw14Skeleton_t**out){
 c1aw14Skeleton_t*s;c1mrReader_t r;unsigned i,k;float scale;uint32_t bits=0x38000100u;
 if(!data||!out||*out)return 0;s=(c1aw14Skeleton_t*)calloc(1,sizeof(*s));if(!s)return 0;
 if(!C1MR_DecodeParts(data,size,&s->source)||s->source.boneCount>=128)goto bad;
 memcpy(&scale,&bits,4);r.data=data;r.size=size;r.at=6;r.ok=1;
 for(i=0;i<s->source.boneCount;i++){
  if(i<s->source.rootCount){s->bindQ[i][3]=1;continue;}
  {int v[4];int64_t sum=0;C1MR_U8(&r);for(k=0;k<3;k++)C1MR_F32(&r);for(k=0;k<3;k++){v[k]=C1MR_S16(&r);if(v[k]==-32768)goto bad;sum+=(int64_t)v[k]*v[k];}v[3]=(int)C1AB14_W(sum<1073676289?1073676289u-(unsigned)sum:0);for(k=0;k<4;k++)s->bindQ[i][k]=(float)v[k]*scale;}
 }
 if(!r.ok)goto bad;for(i=0;i<s->source.boneCount;i++)for(k=i+1;k<s->source.boneCount;k++)if(!strcmp(s->source.boneName[i],s->source.boneName[k]))goto bad;
 s->magic=C1AW14_MAGIC;*out=s;return 1;
 bad:free(s);return 0;
}
static int C1AW14_Decode(const void*data,size_t size,c1aw14Skeleton_t**out){int ok;
#if defined(_MSC_VER)&&defined(_M_IX86)
 unsigned char outer[108];uint32_t savedMx,nearestMx=0x1f80u;const unsigned short nearest=0x027f;
 __asm fnsave outer
 __asm stmxcsr savedMx
 __asm fldcw nearest
 __asm ldmxcsr nearestMx
 ok=C1AW14_DecodeInner(data,size,out);
 __asm ldmxcsr savedMx
 __asm frstor outer
#else
 ok=C1AW14_DecodeInner(data,size,out);
#endif
 return ok;
}
/* Map by actual original bone name. Every clip bone must exist exactly once. */
static int C1AW14_Map(const c1aw14Skeleton_t*s,const c1ab14Clip_t*c,unsigned char map[128]){
 unsigned i,j;unsigned char staged[128],used[128]={0};if(!s||!c||!map||s->magic!=C1AW14_MAGIC||c->magic!=C1AB14_MAGIC||!c->bones||c->bones>=128)return 0;
 if(C1AB14_Overlap(s,sizeof(*s),map,c->bones)||C1AB14_Overlap(c,sizeof(*c),map,c->bones))return 0;
 for(i=0;i<c->bones;i++){const c1ab14Bone_t*b=&c->bone[i];if(C1AB14_Overlap(b->qi,b->nq*2,map,c->bones)||C1AB14_Overlap(b->q,b->nq*8,map,c->bones)||C1AB14_Overlap(b->ti,b->nt*2,map,c->bones)||C1AB14_Overlap(b->t,b->nt*12,map,c->bones))return 0;if(!memchr(c->bone[i].name,0,128))return 0;for(j=0;j<s->source.boneCount;j++)if(!strcmp(c->bone[i].name,s->source.boneName[j]))break;if(j==s->source.boneCount||used[j])return 0;used[j]=1;staged[i]=(unsigned char)j;}
 memcpy(map,staged,c->bones);return 1;
}
static C1AB14_NOINLINE void C1AW14_Product(const float parent[4],const float local[4],float out[4]){
 out[0]=(float)((double)local[3]*parent[0]+(double)parent[1]*local[2]+(double)parent[3]*local[0]-(double)parent[2]*local[1]);
 out[1]=(float)((double)parent[3]*local[1]-(double)parent[0]*local[2]+(double)local[3]*parent[1]+(double)parent[2]*local[0]);
 out[2]=(float)((double)parent[3]*local[2]+(double)local[1]*parent[0]-(double)parent[1]*local[0]+(double)local[3]*parent[2]);
 out[3]=(float)((double)parent[3]*local[3]-(double)local[0]*parent[0]-(double)local[1]*parent[1]-(double)parent[2]*local[2]);
}
static C1AB14_NOINLINE void C1AW14_Axis(const float q[4],float m[16]){
 float xx=(float)((double)q[0]*q[0]),yy=(float)((double)q[1]*q[1]),zz=(float)((double)q[2]*q[2]);double n=(double)q[3]*q[3]+zz+yy+xx,s;
 memset(m,0,64);if(n==0){m[0]=m[5]=m[10]=1;return;}s=2/n;
 m[0]=(float)(1-s*(yy+zz));m[1]=(float)(s*((double)q[0]*q[1]+(double)q[3]*q[2]));m[2]=(float)(s*((double)q[0]*q[2]-(double)q[3]*q[1]));
 m[4]=(float)(s*((double)q[0]*q[1]-(double)q[3]*q[2]));m[5]=(float)(1-s*(xx+zz));m[6]=(float)(s*((double)q[1]*q[2]+(double)q[3]*q[0]));
 m[8]=(float)(s*((double)q[0]*q[2]+(double)q[3]*q[1]));m[9]=(float)(s*((double)q[1]*q[2]-(double)q[3]*q[0]));m[10]=(float)(1-s*(xx+yy));
}
static C1AB14_NOINLINE int C1AW14_BuildInner(const c1aw14Skeleton_t*s,const c1ab14Accumulator_t*a,unsigned n,c1aw14Pose_t*out){
 c1aw14Pose_t pose;unsigned i,k;int p;float local[3],q[4];
 if(!s||!a||!out||s->magic!=C1AW14_MAGIC||n!=s->source.boneCount||!n||n>=128||!s->source.rootCount||s->source.rootCount>n||C1AB14_Overlap(s,sizeof(*s),out,sizeof(*out))||C1AB14_Overlap(a,n*sizeof(*a),out,sizeof(*out)))return 0;
 memset(&pose,0,sizeof(pose));pose.bones=n;
 for(i=0;i<n;i++){
  p=s->source.parent[i];if((i<s->source.rootCount&&p!=-1)||(i>=s->source.rootCount&&(p<0||p>=(int)i)))return 0;
  for(k=0;k<8;k++)if(!(((const float*)&a[i])[k]>=-1000000&&((const float*)&a[i])[k]<=1000000))return 0;
  if(!(a[i].weight>=0&&a[i].weight<=128))return 0;
  for(k=0;k<4;k++)q[k]=a[i].weight!=0?a[i].q[k]:s->bindQ[i][k];
  for(k=0;k<3;k++){float bind=s->source.local[i].p[k];if(!(bind>=-1000000&&bind<=1000000))return 0;local[k]=a[i].weight!=0?a[i].t[k]:0;if(p>=0)local[k]=(float)((double)bind+local[k]);}
  if(p>=0)C1AW14_Product(pose.q[p],q,pose.q[i]);else memcpy(pose.q[i],q,16);
  C1AW14_Axis(pose.q[i],pose.matrix[i]);
  for(k=0;k<3;k++){float value=p<0?local[k]:(float)((double)pose.matrix[p][8+k]*local[2]+(double)pose.matrix[p][4+k]*local[1]+(double)pose.matrix[p][k]*local[0]+pose.matrix[p][12+k]);if(!(value>=-1000000&&value<=1000000))return 0;pose.matrix[i][12+k]=value;}
 }
 memcpy(out,&pose,sizeof(pose));return 1;
}
static int C1AW14_Build(const c1aw14Skeleton_t*s,const c1ab14Accumulator_t*a,unsigned n,c1aw14Pose_t*out){int ok;
#if defined(_MSC_VER)&&defined(_M_IX86)
 unsigned char outer[108];uint32_t savedMx,nearestMx=0x1f80u;const unsigned short nearest=0x027f;
 __asm fnsave outer
 __asm stmxcsr savedMx
 __asm fldcw nearest
 __asm ldmxcsr nearestMx
 ok=C1AW14_BuildInner(s,a,n,out);
 __asm ldmxcsr savedMx
 __asm frstor outer
#else
 ok=C1AW14_BuildInner(s,a,n,out);
#endif
 return ok;
}
/* Explicit current actor world placement of completed model-space matrices.
 * q remains model-space diagnostic; matrix is the actual placed world output. */
static int C1AW14_Place(const c1aw14Pose_t*model,const float origin[3],const float angles[3],c1aw14Pose_t*out){
 c1aw14Pose_t value;float zero[3]={0,0,0};unsigned i,k;
 if(!model||!origin||!angles||!out||!model->bones||model->bones>=128)return 0;value=*model;
 for(i=0;i<value.bones;i++){if(!C1NSO_Transform(origin,angles,model->matrix[i]+12,value.matrix[i]+12))return 0;for(k=0;k<3;k++)if(!C1NSO_Transform(zero,angles,model->matrix[i]+k*4,value.matrix[i]+k*4))return 0;}
 memcpy(out,&value,sizeof(value));return 1;
}
#endif
