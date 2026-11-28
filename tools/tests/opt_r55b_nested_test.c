#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "gsc_vm_breakdown55.h"
typedef struct {GscBreakdown55 *r55_breakdown_scope; unsigned events; int mode;} Runtime;
static uint64_t now, clock_calls;
static int clock_mode;
static uint64_t fakeclock(void){++clock_calls; ++now; return clock_mode==1?0:clock_mode==2?1000000-now:now;}
static int leaf(Runtime*r,int result){r->events++;return result;}
static int guard(Runtime*r,int result){return R55B_CALL(r,R55B_GUARD,leaf(r,result));}
static int native(Runtime*r,int result){return R55B_CALL(r,R55B_NATIVE_EXPR,guard(r,result));}
static int eval_inner(Runtime*r,int result){return R55B_CALL(r,R55B_EVAL, native(r,result));}
static int eval(Runtime*r,int result){return R55B_CALL(r,R55B_EVAL, eval_inner(r,result));}
static int stmt(Runtime*r,int result){return R55B_CALL(r,R55B_NATIVE_STMT,eval(r,result));}
int main(void){unsigned i,j;for(i=0;i<65536;i++){
 GscBreakdown55 s={0};Runtime a={0,0,(int)i},b={&s,0,(int)i};int result=(int)(i%3),x,y;
 s.clock=fakeclock;now=100;clock_calls=0;clock_mode=i%3;
 x=stmt(&a,result);assert(!clock_calls);y=stmt(&b,result);
 assert(x==y&&y==result&&a.events==1&&b.events==1&&clock_calls==8);
 assert(s.calls[R55B_EVAL]==2&&s.outer[R55B_EVAL]==1);
 for(j=0;j<R55B_COUNT;j++){assert(s.depth[j]==0&&s.outer[j]==1);assert(s.calls[j]==(j==R55B_EVAL?2:1));}
 if(clock_mode==0){assert(!s.failures&&s.ticks[R55B_GUARD]==1&&s.ticks[R55B_NATIVE_EXPR]==3&&s.ticks[R55B_EVAL]==5&&s.ticks[R55B_NATIVE_STMT]==7);}else{assert(s.failures==4);for(j=0;j<R55B_COUNT;j++)assert(!s.ticks[j]);}
 /* short circuit: skipped expression has no extra calls or clock reads */
 clock_calls=0;assert(!(0&&R55B_CALL(&b,R55B_NATIVE_EXPR,leaf(&b,1))));assert(!clock_calls&&b.events==1);
 }
 puts("R55B PASS: 65536 on/off nested timing cases, good/zero/reversed clocks, short circuit");return 0;}
