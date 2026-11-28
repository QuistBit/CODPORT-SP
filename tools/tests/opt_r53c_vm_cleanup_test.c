/* R53C cleanup boundary, using extracted production functions. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#define CONFIGURED_PRECACHE_UNBORN 0
#define GSC53_ENDON 3
#define GSC53_ARRAY 4
typedef struct {uint64_t (*clock)(void);uint64_t ticks[5],calls[5],clock_failures;} GscR237Profile53;
typedef struct {int configured_precache_phase,*execution_budget,*executing_task,advancing,array_ok;char error[2];int order[3],n;} GscRuntime;
static void configured_precache_close(GscRuntime *r){r->order[r->n++]=1;r->configured_precache_phase=2;}
static void endon_collect_retired(GscRuntime *r){assert(!r->advancing&&!r->executing_task&&!r->execution_budget);r->order[r->n++]=2;}
static int array_collect_safe_point(GscRuntime *r){r->order[r->n++]=3;return r->array_ok;}
#include "finish_extracted.h"
static uint64_t tick;
static uint64_t clk(void){return ++tick;}
static uint64_t badclk(void){return 0;}
int main(void){int i,budget=5,task=1;for(i=0;i<64;i++){GscRuntime a={0},b,c;GscR237Profile53 p={0};int ok=(i>>2)&1,ra,rb,rc;unsigned calls=ok&&!(i&8);a.configured_precache_phase=i%3;a.execution_budget=&budget;a.executing_task=&task;a.advancing=1;a.error[0]=(i&8)?'e':0;a.array_ok=(i>>4)&1;b=a;c=a;p.clock=(i&32)?clk:badclk;tick=0;ra=r237_finish_operation(&a,ok);rb=r237_finish_operation53(&b,ok,NULL);rc=r237_finish_operation53(&c,ok,&p);assert(ra==rb&&ra==rc&&!memcmp(&a,&b,sizeof(a))&&!memcmp(&a,&c,sizeof(a)));assert(p.calls[GSC53_ENDON]==calls&&p.calls[GSC53_ARRAY]==calls);assert(p.clock_failures==((i&32)?0:2*calls));}puts("R53C cleanup: 64 cases PASS");return 0;}
