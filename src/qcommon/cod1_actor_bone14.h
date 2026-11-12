#ifndef COD1_ACTOR_BONE14_H
#define COD1_ACTOR_BONE14_H
/* Owned original v14 body tracks. Explicit leaf phase and weight; no clock,
 * fabricated animation event, root displacement or actor/world authority. */
#include "cod1_actor_root14.h"
#define C1AB14_BONES 128
#define C1AB14_MAGIC 0xc1ab1401u
typedef struct {char name[128];unsigned simple,nq,nt;uint16_t *qi,*ti;int16_t (*q)[4];float (*t)[3];} c1ab14Bone_t;
typedef struct {uint32_t magic;unsigned frames,intervals,serializedFrames,flags,fps,bones;c1ab14Bone_t bone[C1AB14_BONES];size_t packedBytes;} c1ab14Clip_t;
typedef struct {float q[4],weight,t[3];} c1ab14Accumulator_t;
static int C1AB14_Overlap(const void*a,size_t an,const void*b,size_t bn){uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;if(!a||!b||!an||!bn)return 0;return x<=y?y-x<an:x-y<bn;}
static void C1AB14_Free(c1ab14Clip_t*c){unsigned i;if(!c)return;if(!c->packedBytes)for(i=0;i<C1AB14_BONES;i++){free(c->bone[i].qi);free(c->bone[i].ti);free(c->bone[i].q);free(c->bone[i].t);}free(c);}
static unsigned C1AB14_W(unsigned residual){unsigned lo=0,hi=32768,mid;while(hi-lo>1){mid=(hi+lo)/2;if(mid*mid<=residual)lo=mid;else hi=mid;}return lo+(residual-lo*lo>lo);}
static int C1AB14_Decode(const void*data,size_t size,c1ab14Clip_t**out){
 c1ar14Root_t*root=NULL;c1ar14Reader_t r;c1ab14Clip_t*c=NULL;unsigned i,j,k,n,bytes;unsigned char sign[16],simple[16];
 if(!out||*out||!C1AR14_Decode(data,size,&root))return 0;
 c=(c1ab14Clip_t*)calloc(1,sizeof(*c));if(!c)goto fail;c->frames=root->frames;c->intervals=root->intervals;c->serializedFrames=root->serializedFrames;c->flags=root->flags;c->fps=root->fps;c->bones=root->bones;
 r.p=(const unsigned char*)data;r.n=size;r.at=root->prefixBytes;r.ok=1;bytes=(c->bones+7)/8;
 for(i=0;i<bytes;i++)sign[i]=(unsigned char)C1AR14_U8(&r);for(i=0;i<bytes;i++)simple[i]=(unsigned char)C1AR14_U8(&r);
 for(i=0;i<c->bones;i++){for(j=0;j<128;j++){c->bone[i].name[j]=(char)C1AR14_U8(&r);if(!c->bone[i].name[j])break;}if(j==128)goto fail;}
 for(i=0;i<c->bones;i++){
  c1ab14Bone_t*b=&c->bone[i];b->simple=!!(simple[i/8]&(1u<<(i%8)));b->nq=n=C1AR14_U16(&r);
  if(n){b->qi=(uint16_t*)malloc(n*sizeof(uint16_t));b->q=(int16_t(*)[4])calloc(n,sizeof(*b->q));if(!b->qi||!b->q)goto fail;}
  if(!C1AR14_Indices(&r,n,c->serializedFrames,b->qi))goto fail;
  for(j=0;j<n;j++){int64_t sum=0,dot=0;for(k=b->simple?2:0;k<3;k++){int v=C1AR14_S16(&r);if(v==-32768)goto fail;b->q[j][k]=(int16_t)v;sum+=(int64_t)v*v;}b->q[j][3]=(int16_t)C1AB14_W(sum<1073676289?1073676289u-(unsigned)sum:0);
   if(j){for(k=0;k<4;k++)dot+=(int64_t)b->q[j-1][k]*b->q[j][k];}else if(sign[i/8]&(1u<<(i%8)))dot=-1;
   if(dot<0)for(k=0;k<4;k++)b->q[j][k]=(int16_t)-b->q[j][k];
  }
  b->nt=n=C1AR14_U16(&r);if(n){b->ti=(uint16_t*)malloc(n*sizeof(uint16_t));b->t=(float(*)[3])malloc(n*sizeof(*b->t));if(!b->ti||!b->t)goto fail;}
  if(!C1AR14_Indices(&r,n,c->serializedFrames,b->ti))goto fail;for(j=0;j<n;j++)for(k=0;k<3;k++)if(!C1AR14_Float(&r,&b->t[j][k]))goto fail;
 }
 if(!r.ok)goto fail;c->magic=C1AB14_MAGIC;C1AR14_Free(root);*out=c;return 1;
 fail:C1AR14_Free(root);C1AB14_Free(c);return 0;
}
/* Native4791C0/4795B0/4799A0 accumulator layout. Integer reconstructed
 * quaternions are interpolated BEFORE normalization, unlike old CA14. */
#if defined(_MSC_VER)
#define C1AB14_NOINLINE __declspec(noinline)
#else
#define C1AB14_NOINLINE
#endif
static C1AB14_NOINLINE int C1AB14_AccumulateInner(const c1ab14Clip_t*c,float phase,float weight,c1ab14Accumulator_t*out,unsigned capacity){
 c1ab14Accumulator_t staged[C1AB14_BONES];unsigned i,k,j,frame;float frameValue,qscale;uint32_t u;const uint32_t scaleBits=0x38000100u;
 if(!c||!out||c->magic!=C1AB14_MAGIC||!c->bones||c->bones>C1AB14_BONES||capacity<c->bones||!(phase>=0&&phase<=1)||!(weight>=0&&weight<=1))return 0;
 memcpy(&qscale,&scaleBits,4);qscale=(float)((double)weight*qscale);frameValue=(float)((double)c->intervals*phase);frame=(unsigned)((double)c->intervals*phase);
 if(C1AB14_Overlap(c,sizeof(*c),out,c->bones*sizeof(*out)))return 0;
 for(i=0;i<c->bones;i++){const c1ab14Bone_t*b=&c->bone[i];size_t n=c->bones*sizeof(*out);if(C1AB14_Overlap(b->qi,b->nq*2,out,n)||C1AB14_Overlap(b->q,b->nq*8,out,n)||C1AB14_Overlap(b->ti,b->nt*2,out,n)||C1AB14_Overlap(b->t,b->nt*12,out,n))return 0;}
 memcpy(staged,out,c->bones*sizeof(*out));
 for(i=0;i<c->bones;i++){
  const c1ab14Bone_t*b=&c->bone[i];double f;unsigned last;
  if(!C1AR14_ValidTrack(b->qi,b->nq,c->intervals)||!C1AR14_ValidTrack(b->ti,b->nt,c->intervals))return 0;
  for(k=0;k<8;k++){memcpy(&u,((float*)&staged[i])+k,4);if((u&0x7f800000u)==0x7f800000u)return 0;}
  if(b->nq){last=b->nq-1;j=(phase==1||!c->intervals)?last:((b->nq>1)?C1AR14_Interval(b->qi,b->nq,frame):0);f=j<last?((double)frameValue-b->qi[j])/(b->qi[j+1]-b->qi[j]):0;
   for(k=b->simple?2:0;k<4;k++){double value=b->q[j][k];if(j<last)value+=(b->q[j+1][k]-b->q[j][k])*f;staged[i].q[k]=(float)(value*qscale+staged[i].q[k]);}
  }else {if(!b->simple)return 0;staged[i].q[3]=(float)((double)weight+staged[i].q[3]);}
  if(b->nt){last=b->nt-1;j=(phase==1||!c->intervals)?last:((b->nt>1)?C1AR14_Interval(b->ti,b->nt,frame):0);f=j<last?((double)frameValue-b->ti[j])/(b->ti[j+1]-b->ti[j]):0;
   for(k=0;k<3;k++){double value=b->t[j][k];if(j<last)value+=((double)b->t[j+1][k]-b->t[j][k])*f;staged[i].t[k]=(float)(value*weight+staged[i].t[k]);}
  }
  staged[i].weight=(float)((double)weight+staged[i].weight);
 }
 memcpy(out,staged,c->bones*sizeof(*out));return 1;
}
static int C1AB14_Accumulate(const c1ab14Clip_t*c,float phase,float weight,c1ab14Accumulator_t*out,unsigned capacity){int ok;
#if defined(_MSC_VER)&&defined(_M_IX86)
 unsigned char outer[108];uint32_t savedMx,nearestMx=0x1f80u;const unsigned short nearest=0x027f;
 __asm fnsave outer
 __asm stmxcsr savedMx
 __asm fldcw nearest
 __asm ldmxcsr nearestMx
 ok=C1AB14_AccumulateInner(c,phase,weight,out,capacity);
 __asm ldmxcsr savedMx
 __asm frstor outer
#else
 ok=C1AB14_AccumulateInner(c,phase,weight,out,capacity);
#endif
 return ok;}

/* Original ordinary child-group normalization47C276..47C301. Separate from
 * additive/override tree branches. Per-bone output remains an accumulator. */
static C1AB14_NOINLINE int C1AB14_NormalizeGroupInner(c1ab14Accumulator_t*a,unsigned n,float inputWeight){
 c1ab14Accumulator_t staged[C1AB14_BONES];unsigned i,k;double sum,scale;uint32_t u;
 if(!a||!n||n>C1AB14_BONES||!(inputWeight>=0&&inputWeight<=1))return 0;memcpy(staged,a,n*sizeof(*a));
 for(i=0;i<n;i++){
  for(k=0;k<8;k++){memcpy(&u,((float*)&staged[i])+k,4);if((u&0x7f800000u)==0x7f800000u)return 0;}
  sum=(double)staged[i].q[3]*staged[i].q[3];sum+=(double)staged[i].q[1]*staged[i].q[1];sum+=(double)staged[i].q[0]*staged[i].q[0];sum+=(double)staged[i].q[2]*staged[i].q[2];
  if(sum!=0){scale=(double)inputWeight/sqrt(sum);for(k=0;k<4;k++)staged[i].q[k]=(float)(scale*staged[i].q[k]);}
  if(staged[i].weight!=0){scale=(double)inputWeight/staged[i].weight;staged[i].weight=inputWeight;for(k=0;k<3;k++)staged[i].t[k]=(float)(scale*staged[i].t[k]);}
 }
 memcpy(a,staged,n*sizeof(*a));return 1;
}
static int C1AB14_NormalizeGroup(c1ab14Accumulator_t*a,unsigned n,float inputWeight){int ok;
#if defined(_MSC_VER)&&defined(_M_IX86)
 unsigned char outer[108];uint32_t savedMx,nearestMx=0x1f80u;const unsigned short nearest=0x027f;
 __asm fnsave outer
 __asm stmxcsr savedMx
 __asm fldcw nearest
 __asm ldmxcsr nearestMx
 ok=C1AB14_NormalizeGroupInner(a,n,inputWeight);
 __asm ldmxcsr savedMx
 __asm frstor outer
#else
 ok=C1AB14_NormalizeGroupInner(a,n,inputWeight);
#endif
 return ok;
}

#endif
