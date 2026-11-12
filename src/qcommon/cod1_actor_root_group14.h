#ifndef COD1_ACTOR_ROOT_GROUP14_H
#define COD1_ACTOR_ROOT_GROUP14_H
/* Selected original MG42 3 rows x 7 yaw leaves. Caller supplies authoritative
 * current phases and actual weights, not elapsed-time guesses. This is the
 * ENGINE111 current-root arithmetic, not an animation tree scheduler. */
#include "cod1_actor_root14.h"
typedef struct {float q2[2],translation[3];} c1arg14Result_t;
static int C1ARG14_Finite(float x){uint32_t u;memcpy(&u,&x,4);return (u&0x7f800000u)!=0x7f800000u;}
#if defined(_MSC_VER)
#define C1ARG14_NOINLINE __declspec(noinline)
#else
#define C1ARG14_NOINLINE
#endif
/* Row query's top-level normalize flag is zero. Intermediate row groups in a
 * main-group query subsequently normalize q2 to rowWeight, not to unit before
 * leaf blending. Every accumulator store is single precision, like original. */
static void C1ARG14_AddLeaf(float accum[6],const c1ar14Sample_t*leaf,float weight){
 unsigned j;const uint32_t scaleBits=0x38000100u;float scale;double factor;memcpy(&scale,&scaleBits,4);factor=(double)weight*scale;
 for(j=0;j<2;j++)accum[j]=(float)((double)leaf->q2[j]*factor+accum[j]);
 accum[2]=(float)((double)accum[2]+weight);
 for(j=0;j<3;j++)accum[j+3]=(float)((double)leaf->translation[j]*weight+accum[j+3]);
}
static C1ARG14_NOINLINE int C1ARG14_RowAdd(const c1ar14Root_t*const bank[7],const float phases[7],const float weights[7],float inputWeight,int normalize,int initialize,float parent[6]){
 unsigned i,j,active=0,first=0;float raw[6]={0};c1ar14Sample_t leaf;double norm,factor;
 for(i=0;i<7;i++){
  if(!bank[i]||!(bank[i]->flags&2)||!C1ARG14_Finite(weights[i])||weights[i]<0||weights[i]>1||!C1ARG14_Finite(phases[i])||phases[i]<0||phases[i]>1)return 0;
  if(weights[i]>0){if(!active)first=i;active++;}
 }
 if(initialize)memset(parent,0,24);
 /* The single-child shortcut forwards inputWeight and destination directly. */
 if(active==1){if(!C1AR14_SampleShort(bank[first],phases[first],&leaf))return 0;C1ARG14_AddLeaf(parent,&leaf,inputWeight);return 1;}
 for(i=0;i<7;i++)if(weights[i]>0){if(!C1AR14_SampleShort(bank[i],phases[i],&leaf))return 0;C1ARG14_AddLeaf(raw,&leaf,weights[i]);}
 if(!active)return 1;
 if(normalize){
  norm=(double)raw[0]*raw[0]+(double)raw[1]*raw[1];if(norm>0){factor=(double)inputWeight/sqrt(norm);for(j=0;j<2;j++)parent[j]=(float)(raw[j]*factor+parent[j]);}
  if(raw[2]>0){factor=(double)inputWeight/raw[2];parent[2]=(float)((double)parent[2]+inputWeight);for(j=0;j<3;j++)parent[j+3]=(float)(raw[j+3]*factor+parent[j+3]);}
 }else{
  /* This selected public query always initializes its top destination. */
  memcpy(parent,raw,24);if(raw[2]>0){factor=1.0/raw[2];for(j=0;j<3;j++)parent[j+3]=(float)(raw[j+3]*factor);}
 }
 return 1;
}
static C1ARG14_NOINLINE int C1ARG14_BlendInner(const c1ar14Root_t*const bank[21],const float phases[21],const float leafWeights[21],const float rowWeights[3],c1arg14Result_t*out){
 float sum[6]={0};unsigned i,j,active=0,first=0,added=0;double factor;c1arg14Result_t result;
 for(i=0;i<21;i++)if(!bank[i]||bank[i]->magic!=C1AR14_MAGIC||!(bank[i]->flags&2)||!C1ARG14_Finite(phases[i])||phases[i]<0||phases[i]>1||!C1ARG14_Finite(leafWeights[i])||leafWeights[i]<0||leafWeights[i]>1)return 0;
 for(i=0;i<3;i++){if(!C1ARG14_Finite(rowWeights[i])||rowWeights[i]<0||rowWeights[i]>1)return 0;if(rowWeights[i]>0){if(!active)first=i;active++;}}
 if(active==1){if(!C1ARG14_RowAdd(bank+first*7,phases+first*7,leafWeights+first*7,1,0,1,sum))return 0;}
 else{
  for(i=0;i<3;i++)if(rowWeights[i]>0){if(!C1ARG14_RowAdd(bank+i*7,phases+i*7,leafWeights+i*7,rowWeights[i],1,!added,sum))return 0;added=1;}
  if(sum[2]>0){factor=1.0/sum[2];for(j=0;j<3;j++)sum[j+3]=(float)(sum[j+3]*factor);}
 }
 result.q2[0]=sum[0];result.q2[1]=sum[1];if(result.q2[0]==0&&result.q2[1]==0)result.q2[1]=1;
 for(j=0;j<3;j++)result.translation[j]=sum[j+3];
 for(j=0;j<2;j++)if(!C1ARG14_Finite(result.q2[j]))return 0;for(j=0;j<3;j++)if(!C1ARG14_Finite(result.translation[j]))return 0;
 memcpy(out,&result,sizeof(result));return 1;
}
static int C1ARG14_BlendCurrent(const c1ar14Root_t*const bank[21],const float phases[21],const float leafWeights[21],const float rowWeights[3],c1arg14Result_t*out){
 int ok;c1arg14Result_t result;if(!bank||!phases||!leafWeights||!rowWeights||!out)return 0;
#if defined(_MSC_VER) && defined(_M_IX86)
 {unsigned char outer[108];uint32_t savedMx,nearestMx=0x1f80u;const unsigned short cw=0x027f;
 __asm fnsave outer
 __asm stmxcsr savedMx
 __asm fldcw cw
 __asm ldmxcsr nearestMx
 ok=C1ARG14_BlendInner(bank,phases,leafWeights,rowWeights,&result);
 __asm ldmxcsr savedMx
 __asm frstor outer
 }
#else
 ok=C1ARG14_BlendInner(bank,phases,leafWeights,rowWeights,&result);
#endif
 if(!ok)return 0;memcpy(out,&result,sizeof(result));return 1;
}
#undef C1ARG14_NOINLINE
#endif
