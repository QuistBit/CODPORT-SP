#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "gsc_vm_breakdown55.h"
typedef struct {int state;} Task;
typedef struct {GscBreakdown55 *r55_breakdown_scope;int results_enabled;int result;int calls;} GscRuntime;
typedef struct {uint64_t (*clock)(void);GscBreakdown55 breakdown;} GscR237Profile53;
static uint64_t ticks;
static uint64_t clk(void){return ++ticks;}
static int source(GscRuntime*r,Task*t,int*b){++r->calls;++t->state;--*b;return r->result;}
static int r55_run_source(GscRuntime*r,Task*t,int*b,GscR237Profile53*p){(void)p;return R55B_CALL(r,R55B_EVAL,source(r,t,b));}
#include "gsc_vm_r237_breakdown55.inc"
int main(void){unsigned i;for(i=0;i<4096;i++){
 GscBreakdown55 inherited={0};GscR237Profile53 p={0};GscRuntime r={0};Task t={0};int budget=10,result;
 p.clock=clk;inherited.clock=clk;r.results_enabled=i&1;r.result=(int)((i>>1)%3);if(i&8)r.r55_breakdown_scope=&inherited;
 ticks=0;result=r55_run_source_breakdown(&r,&t,&budget,&p);
 assert(result==r.result&&r.calls==1&&t.state==1&&budget==9);
 if(i&8){assert(r.r55_breakdown_scope==&inherited&&inherited.calls[R55B_EVAL]==1&&p.breakdown.sources==0&&p.breakdown.legacy_sources==0);}
 else if(i&1){assert(!r.r55_breakdown_scope&&p.breakdown.sources==1&&p.breakdown.calls[R55B_EVAL]==1&&p.breakdown.ticks[R55B_EVAL]==1);}
 else {assert(!r.r55_breakdown_scope&&p.breakdown.legacy_sources==1&&!p.breakdown.calls[R55B_EVAL]);}
 assert(inherited.depth[R55B_EVAL]==0&&p.breakdown.depth[R55B_EVAL]==0);
 }puts("R55B scope PASS: 4096 cases, legacy/inherited scope/return paths");return 0;}
