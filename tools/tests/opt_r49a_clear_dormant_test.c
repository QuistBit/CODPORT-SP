#include <assert.h>
#include <fenv.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef struct {float phase,oldPhase,remaining,goalWeight,currentWeight,rate;unsigned cycle,oldCycle;} c1atl14Node_t;
typedef struct {unsigned parent,children,flags;} c1agt14Definition_t;
#include "deps.inc"
static unsigned calls;
static int C1ATL14_ClearGoal(c1atl14Node_t*node,float fade,int activePath){
 int ok;c1atl14Node_t value;if(!node)return 0;++calls;
 ok=C1ATL14_ClearGoalInner(node,fade,activePath,&value);
 if(!ok)return 0;*node=value;return 1;
}
#define C1AGT14_ClearInner old_clear
#include "clear_old.inc"
#undef C1AGT14_ClearInner
#include "clear_new.inc"
static c1agt14Definition_t d[4096];static c1atl14Node_t a[4096],b[4096],initial[4096];
static unsigned seed=0x91ce49u;static unsigned rnd(void){seed=1664525u*seed+1013904223u;return seed>>8;}
static float bits(uint32_t x){float v;memcpy(&v,&x,4);return v;}
static unsigned cases,success,refused;static unsigned long long oldcalls,newcalls;
static void check(unsigned count,unsigned target,float fade){
 int x,y,fx,fy;unsigned c1,c2;memcpy(a,initial,count*sizeof(*a));memcpy(b,initial,count*sizeof(*b));
 feclearexcept(FE_ALL_EXCEPT);calls=0;x=old_clear(d,a,count,target,fade);fx=fetestexcept(FE_ALL_EXCEPT);c1=calls;
 feclearexcept(FE_ALL_EXCEPT);calls=0;y=C1AGT14_ClearInner(d,b,count,target,fade);fy=fetestexcept(FE_ALL_EXCEPT);c2=calls;
 if(x!=y||fx!=fy||memcmp(a,b,count*sizeof(*a))||c2>c1){fprintf(stderr,"mismatch case %u count %u target %u %d/%d fp %d/%d\n",cases,count,target,x,y,fx,fy);abort();}
 ++cases;if(x)++success;else ++refused;oldcalls+=c1;newcalls+=c2;
}
static void init(unsigned count,unsigned mode){
 memset(d,0,sizeof(d));memset(initial,0,sizeof(initial));
 for(unsigned i=1;i<count;i++){unsigned p=mode==0?0:mode==1?(i-1)/2:rnd()%i;d[i].parent=p;d[p].children++;}
 for(unsigned i=0;i<count;i++){c1atl14Node_t *n=initial+i;n->rate=1;n->phase=(float)(rnd()%9)*.125f;n->oldPhase=(float)(rnd()%9)*.125f;n->cycle=rnd()%65536;n->oldCycle=rnd()%65536;if(rnd()%8==0){n->currentWeight=.5f;n->goalWeight=.25f;n->remaining=.2f;}if(rnd()%31==0)n->remaining=bits(0x80000000u);if(rnd()%41==0)n->currentWeight=bits(1);if(rnd()%43==0)n->goalWeight=bits(0x80000000u);}
 initial[0].currentWeight=initial[0].goalWeight=1;
}
int main(void){
 int modes[]={FE_TONEAREST,FE_DOWNWARD,FE_UPWARD,FE_TOWARDZERO};float fades[]={0,.001f,.25f,60.f,-0.f,-1.f,bits(1),bits(0x7fc00000u),bits(0x7f800000u)};
 init(2078,1);memset(initial,0,sizeof(initial));for(unsigned i=0;i<2078;i++)initial[i].rate=1;check(2078,0,.2f);printf("dormant 2078-node branch: %llu -> %llu ClearGoal calls\n",oldcalls,newcalls);
 for(unsigned m=0;m<4;m++){assert(fesetround(modes[m])==0);for(unsigned k=0;k<200;k++){unsigned count=k%20==0?4096:k%20==1?2078:1+rnd()%128;init(count,k%3);if(k%9==0&&count>1){unsigned p=d[1].parent;d[p].children--;d[1].parent=1;}if(k%13==0)initial[count-1].phase=bits(0x7fc00000u);for(unsigned j=0;j<sizeof(fades)/sizeof(fades[0]);j++)check(count,j%3==0?0:rnd()%count,fades[j]);}}
 /* A valid tree containing forward parent ordinals, with signed zero and tiny weights. */
 init(4,0);d[0].children=1;d[1].parent=3;d[2].parent=3;d[3].children=2;for(unsigned i=0;i<4;i++)check(4,i,.2f);
 printf("%u cases PASS (%u admitted, %u refused); ClearGoal calls %llu -> %llu\n",cases,success,refused,oldcalls,newcalls);
 return 0;
}
