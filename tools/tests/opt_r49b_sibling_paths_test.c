#include <assert.h>
#include <fenv.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef struct {float phase,oldPhase,remaining,goalWeight,currentWeight,rate;unsigned cycle,oldCycle;} c1atl14Node_t;
typedef struct {unsigned parent,children,flags;} c1agt14Definition_t;
#include "deps.inc"
#include "helper.inc"
static c1agt14Definition_t d[4096];static c1atl14Node_t a[4096],b[4096],initial[4096];
static unsigned seed=0x49b123u;static unsigned rnd(void){seed=1664525u*seed+1013904223u;return seed>>8;}
static float bits(uint32_t x){float v;memcpy(&v,&x,4);return v;}
static unsigned cases,masked,oldqueries,newqueries;
static void check(unsigned count,unsigned target,float fade){
 unsigned parent=d[target].parent,i;unsigned char mask[4096];int use,fx,fy;
 memcpy(a,initial,count*sizeof(*a));memcpy(b,initial,count*sizeof(*b));
 feclearexcept(FE_ALL_EXCEPT);
 for(i=1;i<count;i++)if(i!=target&&d[i].parent==parent){++oldqueries;assert(C1ATL14_ClearGoalInner(a+i,fade,C1AGT14_Active(d,a,count,i),a+i));}
 fx=fetestexcept(FE_ALL_EXCEPT);feclearexcept(FE_ALL_EXCEPT);
 use=C1AGT49_SiblingPaths(d,b,count,target,parent,mask);masked+=use;
 for(i=1;i<count;i++)if(i!=target&&d[i].parent==parent){int active;if(use)active=mask[i];else{++newqueries;active=C1AGT14_Active(d,b,count,i);}assert(C1ATL14_ClearGoalInner(b+i,fade,active,b+i));}
 fy=fetestexcept(FE_ALL_EXCEPT);if(fx!=fy||memcmp(a,b,count*sizeof(*a))){fprintf(stderr,"mismatch %u %u %u %d %d\n",cases,count,target,fx,fy);abort();}++cases;
}
static void init(unsigned count,unsigned mode,int exceptional){
 memset(d,0,sizeof(d));memset(initial,0,sizeof(initial));
 for(unsigned i=1;i<count;i++){unsigned p=mode==0?0:mode==1?(i-1)/2:rnd()%i;d[i].parent=p;d[p].children++;}
 for(unsigned i=0;i<count;i++){c1atl14Node_t*n=initial+i;n->rate=1;n->phase=(float)(rnd()%9)*.125f;n->oldPhase=(float)(rnd()%9)*.125f;n->cycle=rnd()%65536;n->oldCycle=rnd()%65536;n->currentWeight=(float)(rnd()%5)*.25f;n->goalWeight=(float)(rnd()%5)*.25f;n->remaining=(float)(rnd()%4)*.125f;if(exceptional&&rnd()%13==0)n->currentWeight=bits(rnd()%2?1:0x80000000u);}
}
int main(void){
 int modes[]={FE_TONEAREST,FE_DOWNWARD,FE_UPWARD,FE_TOWARDZERO};float fades[]={0,.25f,60.f,-0.f,bits(1)};
 init(2078,0,0);for(unsigned i=0;i<2078;i++)initial[i].currentWeight=initial[i].goalWeight=1;check(2078,1,.25f);printf("wide sibling set: %u -> %u individual activity queries\n",oldqueries,newqueries);
 for(unsigned m=0;m<4;m++){assert(fesetround(modes[m])==0);for(unsigned k=0;k<200;k++){unsigned count=k%40==0?4096:k%40==1?2078:2+rnd()%128;init(count,k%3,k%4==0);for(unsigned j=0;j<sizeof(fades)/sizeof(fades[0]);j++)check(count,1+rnd()%(count-1),fades[j]);}}
 /* Forward parents: 0 -> 4 -> 1,2,3. */
 init(5,0,0);d[0].children=1;d[4].children=3;for(unsigned i=1;i<4;i++){d[i].parent=4;initial[i].currentWeight=1;}initial[0].currentWeight=initial[4].currentWeight=1;check(5,1,.25f);
 printf("%u cases PASS, mask selected %u, individual queries %u -> %u\n",cases,masked,oldqueries,newqueries);return 0;
}
