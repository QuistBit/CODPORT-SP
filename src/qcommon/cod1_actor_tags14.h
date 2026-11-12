/* Headless actual ordered character assembly, current BODY supplied by the
 * caller. Attachment binds are local source transforms, never a BODY fallback.
 * No entity lifetime/currentness, animation clock or visibility authority. */
#ifndef COD1_ACTOR_TAGS14_H
#define COD1_ACTOR_TAGS14_H
#include "cod1_dobj_topology.h"
#include "cod1_actor_world14.h"
#include "../renderer/cod1_viewmodel14.h"
typedef struct {const char *modelName,*partsName,*tag;const void *model,*parts;size_t modelBytes,partsBytes;} c1at14Input_t;
typedef struct {unsigned count;c1mrResource_t part[8];cv14Skeleton bind[8];char tag[8][128];c1dtTopology_t topology;c1aw14Skeleton_t *body;} c1at14Resource_t;
typedef struct {unsigned bones;cv14Transform model[128];} c1at14Frame_t;
static int C1AT14_Valid(const cv14Transform*t){double n=0;unsigned k;for(k=0;k<3;k++)if(!(t->p[k]>=-1000000&&t->p[k]<=1000000))return 0;for(k=0;k<4;k++){if(!(t->q[k]>=-1.001f&&t->q[k]<=1.001f))return 0;n+=(double)t->q[k]*t->q[k];}return n>=.999&&n<=1.001;}
static void C1AT14_Free(c1at14Resource_t*r){if(r){C1AW14_Free(r->body);free(r);}}
static C1AB14_NOINLINE int C1AT14_CreateInner(const c1at14Input_t*in,unsigned count,c1at14Resource_t**out){c1at14Resource_t*r;c1dtModelInput_t inputs[8];unsigned i,j;
 if(!in||!out||*out||!count||count>8||C1AB14_Overlap(in,count*sizeof(*in),out,sizeof(*out)))return 0;
 for(i=0;i<count;i++)if(!in[i].model||!in[i].parts||!in[i].modelBytes||!in[i].partsBytes||in[i].modelBytes>16u*1024*1024||in[i].partsBytes>16u*1024*1024||C1AB14_Overlap(out,sizeof(*out),in[i].model,in[i].modelBytes)||C1AB14_Overlap(out,sizeof(*out),in[i].parts,in[i].partsBytes))return 0;
 r=(c1at14Resource_t*)calloc(1,sizeof(*r));if(!r)return 0;r->count=count;
 for(i=0;i<count;i++){
  if(!C1MR_DecodeResource(in[i].modelName,in[i].model,in[i].modelBytes,in[i].partsName,in[i].parts,in[i].partsBytes,r->part+i)||!CV14_Parts(in[i].parts,in[i].partsBytes,r->bind+i)||(in[i].tag&&!C1DT_String(in[i].tag)))goto bad;
  if(in[i].tag)strcpy(r->tag[i],in[i].tag);
  for(j=0;j<r->part[i].skeleton.boneCount;j++)if(!C1AT14_Valid(&r->bind[i].local[j])||strcmp(r->bind[i].name[j],r->part[i].skeleton.boneName[j]))goto bad;
  inputs[i].resource=r->part+i;inputs[i].tagName=r->tag[i];
 }
 if(!C1AW14_Decode(in[0].parts,in[0].partsBytes,&r->body)||C1DT_Build(inputs,count,&r->topology)!=C1DT_OK||r->topology.missingTagInputMask||r->topology.unmeldedRootInputMask)goto bad;
 *out=r;return 1;bad:C1AT14_Free(r);return 0;
}
static int C1AT14_Bone(const c1at14Resource_t*r,unsigned i,cv14Transform*world,unsigned char*state){const c1dtBone_t*b;cv14Transform t;
 if(i>=r->topology.boneCount||state[i]==1)return 0;if(state[i]==2)return 1;state[i]=1;b=r->topology.bones+i;
 if(b->modelIndex>=r->count||b->localBone>=(unsigned)r->bind[b->modelIndex].n)return 0;
 if(b->duplicateBone>=0){if(!C1AT14_Bone(r,b->duplicateBone,world,state))return 0;t=world[b->duplicateBone];}
 else {if(b->parentBone<0||!C1AT14_Bone(r,b->parentBone,world,state))return 0;CV14_Compose(world+b->parentBone,&r->bind[b->modelIndex].local[b->localBone],&t);}
 if(!C1AT14_Valid(&t))return 0;world[i]=t;state[i]=2;return 1;
}
static C1AB14_NOINLINE int C1AT14_BuildInner(const c1at14Resource_t*r,const c1aw14Pose_t*body,c1at14Frame_t*out){c1at14Frame_t f;c1dtModelInput_t inputs[8];c1dtTopology_t top;unsigned char state[128]={0};unsigned i,k;double n;
 if(!r||!body||!out||!r->count||r->count>8||body->bones!=r->part[0].skeleton.boneCount||!body->bones||body->bones>=128||C1AB14_Overlap(r,sizeof(*r),out,sizeof(*out))||C1AB14_Overlap(r->body,sizeof(*r->body),out,sizeof(*out))||C1AB14_Overlap(body,sizeof(*body),out,sizeof(*out)))return 0;
 for(i=0;i<r->count;i++){inputs[i].resource=r->part+i;inputs[i].tagName=r->tag[i];}if(C1DT_Build(inputs,r->count,&top)!=C1DT_OK||memcmp(&top,&r->topology,sizeof(top)))return 0;
 memset(&f,0,sizeof(f));f.bones=top.boneCount;
 for(i=0;i<body->bones;i++){n=0;for(k=0;k<4;k++){if(!(body->q[i][k]>=-1000000&&body->q[i][k]<=1000000))return 0;n+=(double)body->q[i][k]*body->q[i][k];}if(!(n>1e-15&&n<1e15))return 0;n=sqrt(n);for(k=0;k<4;k++)f.model[i].q[k]=(float)(body->q[i][k]/n);memcpy(f.model[i].p,body->matrix[i]+12,12);if(!C1AT14_Valid(f.model+i))return 0;state[i]=2;}
 for(i=body->bones;i<f.bones;i++)if(!C1AT14_Bone(r,i,f.model,state))return 0;*out=f;return 1;
}
/* Sys131 lowercases the requested interned bone name. Exact actual name only. */
static int C1AT14_Find(const c1at14Resource_t*r,const char*requested){char name[128];unsigned i,j;int found=-1;
 if(!r||!r->count||r->count>8||!requested||!requested[0]||!C1DT_String(requested))return -1;
 for(i=0;requested[i];i++){unsigned c=(unsigned char)requested[i];if(c>=128)return -1;name[i]=(char)(c>='A'&&c<='Z'?c+32:c);}name[i]=0;
 /* Original DObj search scans ordered models; within each model reverse bone
  * order, as C1DT named melding. Preserve that first matching model contract. */
 for(i=0;i<r->count;i++)for(j=r->part[i].skeleton.boneCount;j>0;j--)if(!strcmp(name,r->part[i].skeleton.boneName[j-1]))return (int)(r->topology.models[i].firstBone+j-1);
 return found;
}
static int C1AT14_WorldMatrix(const c1at14Frame_t*f,unsigned bone,const float origin[3],const float angles[3],float out[16]){c1aw14Pose_t local,world;unsigned k;
 if(!f||!origin||!angles||!out||bone>=f->bones||f->bones>128||!C1AT14_Valid(f->model+bone)||C1AB14_Overlap(f,sizeof(*f),out,64)||C1AB14_Overlap(origin,12,out,64)||C1AB14_Overlap(angles,12,out,64))return 0;
 memset(&local,0,sizeof(local));local.bones=1;memcpy(local.q[0],f->model[bone].q,16);C1AW14_Axis(local.q[0],local.matrix[0]);memcpy(local.matrix[0]+12,f->model[bone].p,12);
 for(k=0;k<3;k++)if(!(origin[k]>=-1000000&&origin[k]<=1000000&&angles[k]>=-360000&&angles[k]<=360000))return 0;
 if(!C1AW14_Place(&local,origin,angles,&world))return 0;memcpy(out,world.matrix[0],64);return 1;
}
/* GAME20010CD0/E910/E870 outer arithmetic. Portable atan2/sqrt/sin/cos are
 * explicit libm dependencies; the bounded original oracle uses controlled x87
 * math providers and validates angle tolerance, never full CRT bit identity. */
static C1AB14_NOINLINE int C1AT14_MatrixAnglesInner(const float m[16],float out[3]){const double pi=3.14159265358979323846;const float pif=3.1415927410125732421875f;float a[3],x,y,z,rad,s,c,first;unsigned k;
 if(!m||!out||C1AB14_Overlap(m,64,out,12))return 0;for(k=0;k<16;k++)if(!(m[k]>=-1000000&&m[k]<=1000000))return 0;
 if(m[0]==0&&m[1]==0){a[1]=0;a[0]=m[2]>0?270.f:90.f;}
 else{a[1]=(float)(atan2((double)m[1],m[0])*180/pi);if(a[1]<0)a[1]+=360;first=(float)sqrt((double)m[0]*m[0]+(double)m[1]*m[1]);a[0]=(float)(atan2((double)m[2],first)*-180/pi);if(a[0]<0)a[0]+=360;}
 rad=(float)(-(double)a[1]*pif/180);c=(float)cos(rad);s=(float)sin(rad);x=(float)((double)c*m[4]-(double)s*m[5]);y=(float)((double)c*m[5]+(double)s*m[4]);z=m[6];
 rad=(float)(-(double)a[0]*pif/180);c=(float)cos(rad);s=(float)sin(rad);first=x;x=(float)((double)first*c+(double)s*z);z=(float)((double)c*z-(double)first*s);
 if(x==0&&y==0)first=z>0?-90.f:90.f;else{float horizontal=(float)sqrt((double)x*x+(double)y*y);first=(float)(atan2((double)z,horizontal)*-180/pi);}
 a[2]=y<0?(first<0?180.f+first:-180.f+first):-first;memcpy(out,a,12);return 1;
}
#if defined(_MSC_VER)&&defined(_M_IX86)
#define C1AT14_ENV_BEGIN unsigned char saved[108];uint32_t mx,nearMx=0x1f80u;const unsigned short nearCW=0x027f; __asm { fnsave saved } __asm { stmxcsr mx } __asm { fldcw nearCW } __asm { ldmxcsr nearMx }
#define C1AT14_ENV_END __asm { ldmxcsr mx } __asm { frstor saved }
#else
#define C1AT14_ENV_BEGIN
#define C1AT14_ENV_END
#endif
static int C1AT14_Create(const c1at14Input_t*in,unsigned count,c1at14Resource_t**out){int ok;C1AT14_ENV_BEGIN ok=C1AT14_CreateInner(in,count,out);C1AT14_ENV_END return ok;}
static int C1AT14_Build(const c1at14Resource_t*r,const c1aw14Pose_t*body,c1at14Frame_t*out){int ok;C1AT14_ENV_BEGIN ok=C1AT14_BuildInner(r,body,out);C1AT14_ENV_END return ok;}
static int C1AT14_MatrixAngles(const float m[16],float out[3]){int ok;C1AT14_ENV_BEGIN ok=C1AT14_MatrixAnglesInner(m,out);C1AT14_ENV_END return ok;}
#undef C1AT14_ENV_BEGIN
#undef C1AT14_ENV_END
#endif
