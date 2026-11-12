#ifndef COD1_ACTOR_ROOT_INTERVAL14_H
#define COD1_ACTOR_ROOT_INTERVAL14_H
/* ENGINE112 interval arithmetic. No clock advancement or actor admission.
 * Actual native leaf ignores cycle counters: only phase<oldPhase adds one
 * end translation on looped clips. Never infer multiple-cycle displacement.
 * Node ordinal is original source ordinal; children run decreasing ordinal.
 * Goal/current choice is the actual syscall byte, not a guessed blend mode. */
#include "cod1_actor_root14.h"
typedef struct {unsigned ordinal;int parent,leaf;float currentWeight,goalWeight,oldPhase,phase;} c1ari14Node_t;
typedef struct {float q2[2],translation[3];} c1ari14Result_t;
static int C1ARI14_Finite(float x){uint32_t u;memcpy(&u,&x,4);return (u&0x7f800000u)!=0x7f800000u;}
static int C1ARI14_Overlap(const void*a,size_t n,const void*b,size_t m){uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;return x<=y?y-x<n:x-y<m;}
static int C1ARI14_Leaf(const c1ar14Root_t*r,float oldPhase,float phase,float weight,float out[6]){
 c1ar14Sample_t old,current;float d[3],a,scale;uint32_t bits=0x30800200;double factor,n,b,z;unsigned j;
 if(!(r->flags&2))return 1;
 if(!C1AR14_SampleInner(r,oldPhase,&old,1)||!C1AR14_SampleInner(r,phase,&current,1))return 0;
 z=current.translation[2];if((r->flags&1)&&phase<oldPhase&&r->translationKeys){for(j=0;j<2;j++)current.translation[j]=(float)((double)current.translation[j]+r->translation[r->translationKeys-1][j]);z+=r->translation[r->translationKeys-1][2];}
 memcpy(&scale,&bits,4);factor=(double)weight*scale;
 out[0]=(float)(((double)old.q2[1]*current.q2[0]-(double)old.q2[0]*current.q2[1])*factor+out[0]);
 out[1]=(float)(((double)old.q2[1]*current.q2[1]+(double)old.q2[0]*current.q2[0])*factor+out[1]);
 for(j=0;j<2;j++)d[j]=(float)((double)current.translation[j]-old.translation[j]);d[2]=(float)(z-old.translation[2]);
 n=(double)current.q2[0]*current.q2[0]+(double)current.q2[1]*current.q2[1];
 if(n>0){factor=2./n;a=(float)(factor*((double)current.q2[0]*current.q2[0]));b=(double)current.q2[1]*current.q2[0]*factor;{float x=(float)((1.-a)*d[0]+b*d[1]);d[1]=(float)((double)d[1]-(b*d[0]+(double)a*d[1]));d[0]=x;}}
 out[2]=(float)((double)weight+out[2]);for(j=0;j<3;j++)out[j+3]=(float)((double)d[j]*weight+out[j+3]);return 1;
}
static int C1ARI14_Recurse(const c1ar14Root_t*const*bank,const c1ari14Node_t*nodes,unsigned count,unsigned index,int goal,float input,int initialize,int normalize,float out[6],unsigned depth){
 unsigned i,j,active=0,first=0,last=~0u,chosen;float local[6]={0},*sum;double norm,factor;
 if(depth>32)return 0;if(initialize)memset(out,0,24);
 if(nodes[index].leaf>=0)return C1ARI14_Leaf(bank[nodes[index].leaf],nodes[index].oldPhase,nodes[index].phase,input,out);
 for(i=0;i<count;i++)if(nodes[i].parent==(int)index&&(goal?nodes[i].goalWeight:nodes[i].currentWeight)>0){active++;first=i;}
 if(!active)return 1;if(active==1)return C1ARI14_Recurse(bank,nodes,count,first,goal,input,initialize,normalize,out,depth+1);
 sum=initialize?out:local;
 for(j=0;j<active;j++){chosen=~0u;for(i=0;i<count;i++)if(nodes[i].parent==(int)index&&nodes[i].ordinal<last&&(goal?nodes[i].goalWeight:nodes[i].currentWeight)>0&&(chosen==~0u||nodes[i].ordinal>nodes[chosen].ordinal))chosen=i;if(chosen==~0u)return 0;last=nodes[chosen].ordinal;if(!C1ARI14_Recurse(bank,nodes,count,chosen,goal,goal?nodes[chosen].goalWeight:nodes[chosen].currentWeight,j==0,1,sum,depth+1))return 0;}
 if(!normalize){if(sum[2]>0){factor=1./sum[2];for(i=0;i<3;i++)sum[i+3]=(float)(sum[i+3]*factor);}if(!initialize)memcpy(out,sum,24);return 1;}
 norm=(double)sum[0]*sum[0]+(double)sum[1]*sum[1];if(norm>0){factor=input/sqrt(norm);for(i=0;i<2;i++)if(initialize)out[i]=(float)(sum[i]*factor);else out[i]=(float)(sum[i]*factor+out[i]);}
 if(sum[2]>0){factor=(double)input/sum[2];out[2]=initialize?input:(float)((double)out[2]+input);for(i=0;i<3;i++)if(initialize)out[i+3]=(float)(sum[i+3]*factor);else out[i+3]=(float)(sum[i+3]*factor+out[i+3]);}return 1;
}
static int C1ARI14_Inner(const c1ar14Root_t*const*bank,unsigned leaves,const c1ari14Node_t*nodes,unsigned count,int goal,c1ari14Result_t*out){
 unsigned i,j,walk;float sum[6]={0};c1ari14Result_t value;
 if(!bank||!nodes||!out||!leaves||leaves>82||!count||count>128||(goal!=0&&goal!=1)||nodes[0].parent!=-1||nodes[0].leaf!=-1||C1ARI14_Overlap(out,sizeof(*out),bank,leaves*sizeof(*bank))||C1ARI14_Overlap(out,sizeof(*out),nodes,count*sizeof(*nodes)))return 0;
 for(i=0;i<leaves;i++)if(!bank[i]||bank[i]->magic!=C1AR14_MAGIC||C1ARI14_Overlap(out,sizeof(*out),bank[i],sizeof(*bank[i])))return 0;
 for(i=0;i<leaves;i++){c1ar14Sample_t probe;if(!C1AR14_SampleInner(bank[i],0,&probe,1))return 0;}
 for(i=0;i<count;i++)if(nodes[i].parent< -1||nodes[i].parent>=(int)count)return 0;
 for(i=0;i<count;i++){const c1ari14Node_t*n=nodes+i;if(n->parent< -1||n->parent>=(int)count||(!i?n->parent!=-1:n->parent<0)||n->leaf< -1||n->leaf>=(int)leaves||!C1ARI14_Finite(n->phase)||n->phase<0||n->phase>1||!C1ARI14_Finite(n->oldPhase)||n->oldPhase<0||n->oldPhase>1||!C1ARI14_Finite(n->currentWeight)||n->currentWeight<0||n->currentWeight>1||!C1ARI14_Finite(n->goalWeight)||n->goalWeight<0||n->goalWeight>1)return 0;for(j=0;j<i;j++)if(n->ordinal==nodes[j].ordinal)return 0;walk=i;for(j=0;walk;j++){if(j>=32||nodes[walk].parent<0)return 0;walk=(unsigned)nodes[walk].parent;}if(n->leaf>=0)for(j=0;j<count;j++)if(nodes[j].parent==(int)i)return 0;}
 if(!C1ARI14_Recurse(bank,nodes,count,0,goal,1,1,0,sum,0))return 0;/* Exact47D03A/47D04B: identity if either component is zero. */value.q2[0]=sum[0];value.q2[1]=sum[1];if(value.q2[0]==0||value.q2[1]==0){value.q2[0]=0;value.q2[1]=1;}for(i=0;i<3;i++)value.translation[i]=sum[i+3];for(i=0;i<5;i++)if(!C1ARI14_Finite(((float*)&value)[i]))return 0;memcpy(out,&value,sizeof(value));return 1;
}
static int C1ARI14_Interval(const c1ar14Root_t*const*bank,unsigned leaves,const c1ari14Node_t*nodes,unsigned count,int goal,c1ari14Result_t*out){int ok;unsigned i;c1ari14Result_t result;if(!out||!bank||!nodes||!leaves||leaves>82||!count||count>128||C1ARI14_Overlap(out,sizeof(*out),bank,leaves*sizeof(*bank))||C1ARI14_Overlap(out,sizeof(*out),nodes,count*sizeof(*nodes)))return 0;for(i=0;i<leaves;i++)if(!bank[i]||C1ARI14_Overlap(out,sizeof(*out),bank[i],sizeof(*bank[i])))return 0;
#if defined(_MSC_VER)&&defined(_M_IX86)
 {unsigned char outer[108];uint32_t mx,nmx=0x1f80;unsigned short cw=0x027f;
 __asm fnsave outer
 __asm stmxcsr mx
 __asm fldcw cw
 __asm ldmxcsr nmx
 ok=C1ARI14_Inner(bank,leaves,nodes,count,goal,&result);
 __asm ldmxcsr mx
 __asm frstor outer
 }
#else
 ok=C1ARI14_Inner(bank,leaves,nodes,count,goal,&result);
#endif
 if(ok)memcpy(out,&result,sizeof(result));return ok;}
#endif
