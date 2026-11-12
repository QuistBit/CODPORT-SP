#ifndef CG_COD1_TURRET_ACTOR_POSE14_H
#define CG_COD1_TURRET_ACTOR_POSE14_H
#include "../qcommon/cod1_actor_world14.h"
/* Selected ordinary current tree. Every row/aim/fire boundary is preserved;
 * a sole active child bypasses its local weight exactly as47BD40. */
typedef struct {const c1ab14Clip_t*clip[2][21];unsigned char map[2][21][128];unsigned bones;} cgta14Bank_t;
static C1AB14_NOINLINE int CGTA14_Merge(const c1ab14Accumulator_t*src,c1ab14Accumulator_t*dst,unsigned n,float weight,int clear,int top){unsigned i,k;double norm,scale;if(clear)memset(dst,0,n*sizeof(*dst));for(i=0;i<n;i++){
 norm=(double)src[i].q[3]*src[i].q[3];norm+=(double)src[i].q[1]*src[i].q[1];norm+=(double)src[i].q[0]*src[i].q[0];norm+=(double)src[i].q[2]*src[i].q[2];
 if(top){if(src[i].weight>0){scale=1.0/src[i].weight;for(k=0;k<4;k++)dst[i].q[k]=(float)(scale*src[i].q[k]);for(k=0;k<3;k++)dst[i].t[k]=(float)(scale*src[i].t[k]);dst[i].weight=src[i].weight;}}
 else{if(norm>0){scale=(double)weight/sqrt(norm);for(k=0;k<4;k++)dst[i].q[k]=(float)(scale*src[i].q[k]+dst[i].q[k]);}if(src[i].weight>0){scale=(double)weight/src[i].weight;for(k=0;k<3;k++)dst[i].t[k]=(float)(scale*src[i].t[k]+dst[i].t[k]);dst[i].weight=(float)((double)weight+dst[i].weight);}}
 }return 1;}
static int CGTA14_Leaf(const cgta14Bank_t*b,unsigned g,unsigned leaf,float phase,float weight,c1ab14Accumulator_t*out,int clear){c1ab14Accumulator_t mapped[128];const c1ab14Clip_t*c=b->clip[g][leaf];unsigned i;if(clear)memset(out,0,b->bones*sizeof(*out));for(i=0;i<c->bones;i++)mapped[i]=out[b->map[g][leaf][i]];if(!C1AB14_Accumulate(c,phase,weight,mapped,c->bones))return 0;for(i=0;i<c->bones;i++)out[b->map[g][leaf][i]]=mapped[i];return 1;}
/* level0 =two top groups; level1=three rows; level2=seven leaf tracks. */
static int CGTA14_Node(const cgta14Bank_t*b,const float current[2][25],const float phases[2][21],unsigned level,unsigned group,unsigned row,float weight,c1ab14Accumulator_t*out,int clear,int top){unsigned child[7],count=0,i,n=level==0?2:level==1?3:7;c1ab14Accumulator_t local[128];
 for(i=0;i<n;i++){float w=level==0?current[i][0]:level==1?current[group][1+i]:current[group][4+row*7+i];if(w>0)child[count++]=i;}
 if(!count){if(clear)memset(out,0,b->bones*sizeof(*out));return 1;}
 if(count==1){i=child[0];if(level==2)return CGTA14_Leaf(b,group,row*7+i,phases[group][row*7+i],weight,out,clear);return CGTA14_Node(b,current,phases,level+1,level==0?i:group,level==1?i:row,weight,out,clear,top);}
 memset(local,0,b->bones*sizeof(*local));for(i=0;i<count;i++){unsigned c=child[i];float w=level==0?current[c][0]:level==1?current[group][1+c]:current[group][4+row*7+c];if(level==2){if(!CGTA14_Leaf(b,group,row*7+c,phases[group][row*7+c],w,local,i==0))return 0;}else if(!CGTA14_Node(b,current,phases,level+1,level==0?c:group,level==1?c:row,w,local,i==0,0))return 0;}
 return CGTA14_Merge(local,out,b->bones,weight,clear,top);
}
static C1AB14_NOINLINE int CGTA14_SampleInner(const cgta14Bank_t*b,const float current[2][25],const float phases[2][21],c1ab14Accumulator_t*out,unsigned capacity){c1ab14Accumulator_t staged[128];unsigned g,i,k;size_t bytes;if(!b||!current||!phases||!out||!b->bones||b->bones>=128||capacity<b->bones)return 0;bytes=b->bones*sizeof(*out);
 if(C1AB14_Overlap(b,sizeof(*b),out,bytes)||C1AB14_Overlap(current,50*sizeof(float),out,bytes)||C1AB14_Overlap(phases,42*sizeof(float),out,bytes))return 0;
 for(g=0;g<2;g++){for(i=0;i<25;i++)if(!(current[g][i]>=0&&current[g][i]<=1))return 0;for(i=0;i<21;i++){const c1ab14Clip_t*c=b->clip[g][i];unsigned char seen[128]={0};if(!c||c->magic!=C1AB14_MAGIC||!c->bones||c->bones>=128||!(phases[g][i]>=0&&phases[g][i]<=1)||C1AB14_Overlap(c,sizeof(*c),out,bytes))return 0;for(k=0;k<c->bones;k++){const c1ab14Bone_t*track=c->bone+k;unsigned mapped=b->map[g][i][k];if(mapped>=b->bones||seen[mapped]||C1AB14_Overlap(track->qi,track->nq*2,out,bytes)||C1AB14_Overlap(track->q,track->nq*8,out,bytes)||C1AB14_Overlap(track->ti,track->nt*2,out,bytes)||C1AB14_Overlap(track->t,track->nt*12,out,bytes))return 0;seen[mapped]=1;}}}
 if(!CGTA14_Node(b,current,phases,0,0,0,1,staged,1,1))return 0;memcpy(out,staged,b->bones*sizeof(*out));return 1;
}
static int CGTA14_Sample(const cgta14Bank_t*b,const float current[2][25],const float phases[2][21],c1ab14Accumulator_t*out,unsigned capacity){int ok;
#if defined(_MSC_VER)&&defined(_M_IX86)
 unsigned char outer[108];uint32_t savedMx,nearestMx=0x1f80u;const unsigned short nearest=0x027f;
 __asm fnsave outer
 __asm stmxcsr savedMx
 __asm fldcw nearest
 __asm ldmxcsr nearestMx
 ok=CGTA14_SampleInner(b,current,phases,out,capacity);
 __asm ldmxcsr savedMx
 __asm frstor outer
#else
 ok=CGTA14_SampleInner(b,current,phases,out,capacity);
#endif
 return ok;}
#endif
