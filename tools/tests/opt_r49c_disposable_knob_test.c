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
#include "helper.inc"
#include "valid.inc"
#define C1ATL14_NOINLINE
#include "goal.inc"
static int C1AGT14_Valid(const c1agt14Definition_t*d,const c1atl14Node_t*n,unsigned count){return C1AGT14_ValidScalar(d,n,count);}
static int C1ATL14_ClearGoal(c1atl14Node_t*n,float fade,int active){return C1ATL14_ClearGoalInner(n,fade,active,n);}
static unsigned allocations, frees, fail_at;
static void *tracked_malloc(size_t n){++allocations;if(allocations==fail_at)return NULL;return malloc(n);}
static void tracked_free(void*p){if(p)++frees;free(p);}
#define malloc tracked_malloc
#define free tracked_free
#define C1AKA14_Inner old_knob_inner
#define C1AKA14_Select old_knob
#define C1AKR14_Inner old_ancestor_inner
#define C1AKR14_Select old_ancestor
#include "knob.old"
#include "ancestor.old"
#undef C1AKA14_Inner
#undef C1AKA14_Select
#undef C1AKR14_Inner
#undef C1AKR14_Select
#undef COD1_ACTOR_KNOB_ALL14_H
#undef COD1_ACTOR_ANCESTOR_KNOB14_H
#include "knob.new"
#include "ancestor.new"
#undef malloc
#undef free
static c1agt14Definition_t d[4096];static c1atl14Node_t initial[4096],a[4096],b[4096];
static unsigned seed=0x4c49b21u,cases,success,refused;static unsigned long long alloc_old,alloc_new;
static unsigned rnd(void){seed=1664525u*seed+1013904223u;return seed>>8;}
static float bits(uint32_t x){float v;memcpy(&v,&x,4);return v;}
static void init(unsigned count,unsigned mode){
 memset(d,0,sizeof(d));memset(initial,0,sizeof(initial));
 for(unsigned i=1;i<count;i++){unsigned p=mode==0?0:mode==1?(i-1)/2:rnd()%i;d[i].parent=p;d[p].children++;}
 for(unsigned i=0;i<count;i++){c1atl14Node_t*n=initial+i;d[i].flags=d[i].children?(rnd()%3==0?4:0):0;n->rate=1;n->phase=(float)(rnd()%9)*.125f;n->oldPhase=(float)(rnd()%9)*.125f;n->cycle=rnd()%65536;n->oldCycle=rnd()%65536;n->currentWeight=(float)(rnd()%5)*.25f;n->goalWeight=(float)(rnd()%5)*.25f;n->remaining=(float)(rnd()%4)*.125f;if(rnd()%37==0)n->currentWeight=bits(1);if(rnd()%41==0)n->remaining=bits(0x80000000u);}
}
static void check(unsigned count,unsigned target,unsigned root,float weight,float time,float rate,int force,int ancestor){
 int x,y,fx,fy;unsigned old_n,new_n;
 memcpy(a,initial,count*sizeof(*a));memcpy(b,initial,count*sizeof(*b));
 fail_at=0;allocations=frees=0;feclearexcept(FE_ALL_EXCEPT);
 x=ancestor?old_ancestor(d,a,count,target,root,weight,time,rate,force):old_knob(d,a,count,target,root,weight,time,rate,force);
 fx=fetestexcept(FE_ALL_EXCEPT);old_n=allocations;assert(allocations==frees);
 allocations=frees=0;feclearexcept(FE_ALL_EXCEPT);
 y=ancestor?C1AKR14_Select(d,b,count,target,root,weight,time,rate,force):C1AKA14_Select(d,b,count,target,root,weight,time,rate,force);
 fy=fetestexcept(FE_ALL_EXCEPT);new_n=allocations;assert(allocations==frees);
 if(x!=y||fx!=fy||memcmp(a,b,count*sizeof(*a))||new_n>old_n){fprintf(stderr,"mismatch %u n=%u target=%u root=%u %d/%d fp %d/%d\n",cases,count,target,root,x,y,fx,fy);abort();}
 if(!x)assert(!memcmp(a,initial,count*sizeof(*a)));++cases;if(x)++success;else ++refused;alloc_old+=old_n;alloc_new+=new_n;
}
int main(void){
 int modes[]={FE_TONEAREST,FE_DOWNWARD,FE_UPWARD,FE_TOWARDZERO};float weights[]={0,.25f,1,-1,bits(0x7fc00000)},times[]={0,.2f,60.f,-1.f,bits(0x7f7fffff)};
 init(2078,1);check(2078,2077,0,1,.2f,1,1,1);printf("2078 nodes ancestor: allocations %llu -> %llu\n",alloc_old,alloc_new);
 for(unsigned m=0;m<4;m++){assert(!fesetround(modes[m]));for(unsigned k=0;k<200;k++){unsigned count=k%50==0?4096:k%50==1?2078:2+rnd()%128;init(count,k%3);unsigned target=1+rnd()%(count-1),p=d[target].parent,root=p;while(root&&rnd()%2)root=d[root].parent;if(k%19==0)d[0].children++;if(k%23==0)initial[count-1].phase=bits(0x7fc00000);for(unsigned j=0;j<5;j++){check(count,target,p,weights[j],times[rnd()%5],j==4?101:1,j%2,0);check(count,target,root,weights[j],times[rnd()%5],1,j%2,1);}}}
 /* Forward parents and native sync branch. */
 init(5,0);d[0].children=1;d[4].children=3;d[4].flags=4;for(unsigned i=1;i<4;i++)d[i].parent=4;check(5,1,0,1,.25f,1,1,1);
 /* The outer allocation still refuses without a write. */
 memcpy(b,initial,5*sizeof(*b));allocations=frees=0;fail_at=1;assert(!C1AKR14_Select(d,b,5,1,0,1,.25f,1,1));assert(!memcmp(b,initial,5*sizeof(*b)));assert(!frees);fail_at=0;
 printf("%u cases PASS (%u admitted, %u refused); allocations %llu -> %llu\n",cases,success,refused,alloc_old,alloc_new);return 0;
}
