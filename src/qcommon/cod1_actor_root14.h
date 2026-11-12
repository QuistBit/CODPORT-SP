#ifndef COD1_ACTOR_ROOT14_H
#define COD1_ACTOR_ROOT14_H
/* Owned v14 root delta tracks, distinct from body-bone sampling. Source clip
 * is completely structurally validated, then no input pointer is retained.
 * SampleRaw takes explicit normalized leaf phase [0,1]; it never chooses a
 * tree phase, creates an animation event or admits a live actor. Rotation is
 * the native shortscaled q(z,w), deliberately not normalized before blending.
 * Caller owns immutable record; free it with C1AR14_Free. */
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#define C1AR14_KEYS 4096
#define C1AR14_MAGIC 0xc1a01401u
#define C1AR14_CLIP_LIMIT (16u*1024u*1024u)
/* Retain authored note names/frames in source order; phase and delivery remain
 * separate native clock responsibilities. No input pointer is retained. */
typedef struct {char name[128];uint16_t frame;} c1ar14Note_t;
#define C1AR14_NOTES 255u
typedef struct {
 uint32_t magic;unsigned frames,intervals,serializedFrames,bones,flags,fps;
 unsigned rotationKeys,translationKeys,noteCount;size_t prefixBytes;
 uint16_t rotationIndex[C1AR14_KEYS],translationIndex[C1AR14_KEYS];
 int16_t rotation[C1AR14_KEYS][2];float translation[C1AR14_KEYS][3];
 c1ar14Note_t notes[C1AR14_NOTES];
} c1ar14Root_t;
typedef struct {float q2[2],translation[3];} c1ar14Sample_t;
typedef struct {const unsigned char *p;size_t n,at;int ok;} c1ar14Reader_t;
static int C1AR14_Take(c1ar14Reader_t *r,size_t n){if(!r->ok||r->at>r->n||n>r->n-r->at){r->ok=0;return 0;}r->at+=n;return 1;}
static unsigned C1AR14_U8(c1ar14Reader_t*r){return C1AR14_Take(r,1)?r->p[r->at-1]:0;}
static unsigned C1AR14_U16(c1ar14Reader_t*r){unsigned a=C1AR14_U8(r);return a|(C1AR14_U8(r)<<8);}
static int C1AR14_S16(c1ar14Reader_t*r){unsigned u=C1AR14_U16(r);return u<32768?(int)u:(int)u-65536;}
static int C1AR14_Float(c1ar14Reader_t*r,float*out){uint32_t u;const unsigned char*p;if(!C1AR14_Take(r,4))return 0;p=r->p+r->at-4;u=(uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);if((u&0x7f800000u)==0x7f800000u){r->ok=0;return 0;}memcpy(out,&u,4);return 1;}
static int C1AR14_NoteName(c1ar14Reader_t*r,char out[128]){unsigned n,v;for(n=0;n<128;n++){v=C1AR14_U8(r);out[n]=(char)v;if(!v)return r->ok;}r->ok=0;return 0;}
static int C1AR14_String(c1ar14Reader_t*r){unsigned n;for(n=0;n<128;n++)if(!C1AR14_U8(r))return r->ok;r->ok=0;return 0;}
static int C1AR14_Indices(c1ar14Reader_t*r,unsigned count,unsigned frames,uint16_t*out){unsigned i,v,previous=0;if(count>frames)return 0;for(i=0;i<count;i++){v=count>1&&count<frames?(frames>256?C1AR14_U16(r):C1AR14_U8(r)):i;if(!r->ok||v>=frames||(i&&v<=previous)||(!i&&v))return 0;if(out)out[i]=(uint16_t)v;previous=v;}return 1;}
static int C1AR14_RoundedW(int z){
 unsigned residual,lo=0,hi=32768,mid;if(z==-32768)return 0;residual=32767u*32767u-(unsigned)(z*z);
 while(hi-lo>1){mid=(lo+hi)/2;if(mid*mid<=residual)lo=mid;else hi=mid;}
 return (int)(lo+(residual-lo*lo>lo));
}
static void C1AR14_Free(c1ar14Root_t*r){free(r);}
static int C1AR14_Decode(const void*data,size_t size,c1ar14Root_t**out){
 c1ar14Reader_t r;c1ar14Root_t *root=NULL;unsigned i,j,n,simpleBytes,notes,count;unsigned char simple[16];float ignored;
 if(!data||!out||*out||size<9||size>C1AR14_CLIP_LIMIT)return 0;r.p=data;r.n=size;r.at=0;r.ok=1;
 if(C1AR14_U16(&r)!=14)return 0;root=(c1ar14Root_t*)calloc(1,sizeof(*root));if(!root)return 0;
 root->frames=C1AR14_U16(&r);root->bones=C1AR14_U16(&r);root->flags=C1AR14_U8(&r);root->fps=C1AR14_U16(&r);
 if(!r.ok||!root->frames||root->frames>4095||!root->bones||root->bones>128||(root->flags&~3u)||!root->fps||root->fps>120)goto bad;
 root->serializedFrames=root->frames+(root->flags&1u);root->intervals=root->serializedFrames-1;
 if(root->flags&2u){
  root->rotationKeys=C1AR14_U16(&r);n=root->rotationKeys;if(!C1AR14_Indices(&r,n,root->serializedFrames,root->rotationIndex)||(n>1&&root->rotationIndex[n-1]!=root->intervals))goto bad;
  for(i=0;i<n;i++){int z=C1AR14_S16(&r);if(z==-32768)goto bad;root->rotation[i][0]=(int16_t)z;root->rotation[i][1]=(int16_t)C1AR14_RoundedW(z);}
  root->translationKeys=C1AR14_U16(&r);n=root->translationKeys;if(!C1AR14_Indices(&r,n,root->serializedFrames,root->translationIndex)||(n>1&&root->translationIndex[n-1]!=root->intervals))goto bad;
  for(i=0;i<n;i++)for(j=0;j<3;j++)if(!C1AR14_Float(&r,&root->translation[i][j]))goto bad;
 }
 root->prefixBytes=r.at;simpleBytes=(root->bones+7)/8;if(!C1AR14_Take(&r,simpleBytes))goto bad;for(i=0;i<simpleBytes;i++)simple[i]=(unsigned char)C1AR14_U8(&r);
 for(i=0;i<root->bones;i++)if(!C1AR14_String(&r))goto bad;
 for(i=0;i<root->bones;i++){
  count=C1AR14_U16(&r);if(!C1AR14_Indices(&r,count,root->serializedFrames,NULL)||!C1AR14_Take(&r,count*((simple[i/8]&(1u<<(i%8)))?2u:6u)))goto bad;
  count=C1AR14_U16(&r);if(!C1AR14_Indices(&r,count,root->serializedFrames,NULL))goto bad;for(j=0;j<count*3;j++)if(!C1AR14_Float(&r,&ignored))goto bad;
 }
 notes=C1AR14_U8(&r);root->noteCount=notes;for(i=0;i<notes;i++){if(!C1AR14_NoteName(&r,root->notes[i].name))goto bad;root->notes[i].frame=(uint16_t)C1AR14_U16(&r);if(root->notes[i].frame>root->frames)goto bad;}
 if(!r.ok||r.at!=r.n)goto bad;root->magic=C1AR14_MAGIC;*out=root;return 1;
 bad:C1AR14_Free(root);return 0;
}
static int C1AR14_ValidTrack(const uint16_t*index,unsigned count,unsigned intervals){unsigned i;if(count>C1AR14_KEYS||count>intervals+1)return 0;for(i=0;i<count;i++)if((!i&&index[i])||(i&&index[i]<=index[i-1])||index[i]>intervals)return 0;return count<2||index[count-1]==intervals;}
static unsigned C1AR14_Interval(const uint16_t*index,unsigned count,unsigned frame){unsigned low=0,high=count-1,mid;while(high-low>1){mid=(low+high)/2;if(index[mid]<=frame)low=mid;else high=mid;}return low;}
#if defined(_MSC_VER)
#define C1AR14_NOINLINE __declspec(noinline)
#else
#define C1AR14_NOINLINE
#endif
static C1AR14_NOINLINE int C1AR14_SampleInner(const c1ar14Root_t*root,float phase,c1ar14Sample_t*out,int shortMode){
 c1ar14Sample_t value;uint32_t bits;unsigned frame,i,j,key;float frameValue,raw[2],scale;double fraction;const uint32_t scaleBits=0x38000100u;
 if(!root||!out||root->magic!=C1AR14_MAGIC||!root->frames||root->frames>4095||root->serializedFrames!=root->frames+(root->flags&1u)||root->intervals+1!=root->serializedFrames||root->flags&~3u||!C1AR14_ValidTrack(root->rotationIndex,root->rotationKeys,root->intervals)||!C1AR14_ValidTrack(root->translationIndex,root->translationKeys,root->intervals))return 0;
 memcpy(&bits,&phase,4);if((bits&0x7f800000u)==0x7f800000u||!(phase>=0&&phase<=1))return 0;memcpy(&scale,&scaleBits,4);if(shortMode)scale=1;
 for(i=0;i<root->translationKeys;i++)for(j=0;j<3;j++){memcpy(&bits,&root->translation[i][j],4);if((bits&0x7f800000u)==0x7f800000u)return 0;}
 memset(&value,0,sizeof(value));value.q2[1]=shortMode?32767.0f:1.0f;frameValue=(float)((double)root->intervals*phase);frame=(unsigned)((double)root->intervals*phase);
 if(root->rotationKeys){
  if(root->rotationKeys==1||phase==1||!root->intervals){key=phase==1?root->rotationKeys-1:0;raw[0]=(float)root->rotation[key][0];raw[1]=(float)root->rotation[key][1];}
  else{key=C1AR14_Interval(root->rotationIndex,root->rotationKeys,frame);fraction=((double)frameValue-root->rotationIndex[key])/(root->rotationIndex[key+1]-root->rotationIndex[key]);for(j=0;j<2;j++)raw[j]=(float)((root->rotation[key+1][j]-root->rotation[key][j])*fraction+root->rotation[key][j]);}
  value.q2[0]=raw[0]*scale;value.q2[1]=raw[1]*scale;
 }
 if(root->translationKeys){
  if(root->translationKeys==1||phase==1||!root->intervals){key=phase==1?root->translationKeys-1:0;memcpy(value.translation,root->translation[key],12);}
  else{key=C1AR14_Interval(root->translationIndex,root->translationKeys,frame);fraction=((double)frameValue-root->translationIndex[key])/(root->translationIndex[key+1]-root->translationIndex[key]);for(j=0;j<3;j++)value.translation[j]=(float)(((double)root->translation[key+1][j]-root->translation[key][j])*fraction+root->translation[key][j]);}
 }
 memcpy(out,&value,sizeof(value));return 1;
}
/* Controlled arithmetic policy; integer parsing/metadata validation is outside
 * this fence. Both x87 and MXCSR state are restored byte-for-byte, including
 * pending exceptions. No caller FP policy is inherited for interpolation. */
static int C1AR14_SampleControlled(const c1ar14Root_t*root,float phase,c1ar14Sample_t*out,int shortMode){
 c1ar14Sample_t value;int ok;
 if(!root||!out)return 0;
#if defined(_MSC_VER) && defined(_M_IX86)
 {unsigned char outer[108];uint32_t savedMx,nearestMx=0x1f80u;const unsigned short nearest=0x027f;
 __asm fnsave outer
 __asm stmxcsr savedMx
 __asm fldcw nearest
 __asm ldmxcsr nearestMx
 ok=C1AR14_SampleInner(root,phase,&value,shortMode);
 __asm ldmxcsr savedMx
 __asm frstor outer
 }
#else
 ok=C1AR14_SampleInner(root,phase,&value,shortMode);
#endif
 if(!ok)return 0;memcpy(out,&value,sizeof(value));return 1;
}
static int C1AR14_SampleRaw(const c1ar14Root_t*r,float phase,c1ar14Sample_t*out){return C1AR14_SampleControlled(r,phase,out,0);}
/* Exact leaf helper output before the native accumulator applies weight/32767. */
static int C1AR14_SampleShort(const c1ar14Root_t*r,float phase,c1ar14Sample_t*out){return C1AR14_SampleControlled(r,phase,out,1);}
#undef C1AR14_NOINLINE
#endif
