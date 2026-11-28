#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#define TASK_DEAD 0
#define TASK_READY 1
#define TASK_SLEEP 2
#define EVENT_ENTITY 1
#define GSC53_SOURCE 2
#define GSC53_TASK_GUARD 9
typedef struct {uint64_t (*clock)(void);uint64_t ticks[10],calls[10],clock_failures;} GscR237Profile53;
typedef struct {int r237_enqueued,status,event_scope;uint64_t r237_insertion,r243_time_root_incarnation,serial;void*results;} Task;
typedef struct {int refresh,wait,owner,owner_after,run_ok,run_status,run_error,results_enabled,state_enabled,resources_enabled,resources_closed,r238_array_value_cow_enabled,array_next_scan,array_next_entry_scan,called,cleared;uint64_t resources_entry_serial;Task *executing_task,*array_dispatch_root;char error[128];} GscRuntime;
static int frees;
#define GSC_RUNTIME_FREE(p) ((void)(p),++frees)
static int refresh_state_context(GscRuntime*r,Task*t){(void)t;return r->refresh;}
static int refresh_wait_owner(GscRuntime*r,Task*t){(void)t;return r->wait;}
static int r237_owner_current(GscRuntime*r,uint64_t stamp){(void)stamp;return r->called?r->owner_after:r->owner;}
static int rfail(GscRuntime*r,int line,const char*s){(void)line;snprintf(r->error,sizeof(r->error),"%s",s);return 0;}
static void clear_wait_owner(Task*t){t->event_scope=0;}
static void configured_precache_dispatch_begin(GscRuntime*r,Task*t){assert(r->executing_task==t);}
static void configured_precache_dispatch_end(GscRuntime*r,Task*t){assert(r->executing_task==t&&!r->array_dispatch_root);r->cleared++;}
static int run_task(GscRuntime*r,Task*t,int*b){r->called++;--*b;t->status=r->run_status;if(r->run_error)r->error[0]='e';return r->run_ok;}
static int run_task_results(GscRuntime*r,Task*t,int*b){return run_task(r,t,b);}
static uint64_t r237_profile_begin53(GscR237Profile53 *p){return p&&p->clock?p->clock():0;}
static void r237_profile_end53(GscR237Profile53 *p,unsigned stage,uint64_t start){
 uint64_t end;if(!p)return;
 ++p->calls[stage];end=p->clock?p->clock():0;
 if(!start||!end||end<start){++p->clock_failures;return;}
 p->ticks[stage]+=end-start;
}
#define r237_dispatch_task53 old_dispatch
#include "dispatch_old.inc"
#undef r237_dispatch_task53
#define r237_dispatch_task53 new_dispatch
#include "dispatch_new.inc"
#undef r237_dispatch_task53
static uint64_t tick;
static uint64_t clk(void){return ++tick;}
static uint64_t badclk(void){return 0;}
int main(void){unsigned i;int storage=0;for(i=0;i<8192;i++){GscRuntime a={0},b;Task x={0},y;GscR237Profile53 p={0};int ra,rb,ba=10,bb=10,fa;unsigned calls;a.refresh=!!(i&1);a.wait=!!(i&2);a.owner=!!(i&4);a.owner_after=!!(i&8);a.run_ok=!!(i&16);a.run_error=!!(i&32);a.run_status=(i>>6)%3;a.results_enabled=!!(i&256);a.state_enabled=!!(i&512);a.resources_enabled=!!(i&1024);a.resources_entry_serial=5;a.r238_array_value_cow_enabled=!!(i&2048);x.status=(i&4096)?TASK_DEAD:TASK_SLEEP;x.event_scope=EVENT_ENTITY;x.serial=5;x.results=&storage;x.r237_enqueued=1;x.r237_insertion=x.r243_time_root_incarnation=8;b=a;y=x;calls=a.refresh&&a.wait&&a.owner&&x.status!=TASK_DEAD;p.clock=(i&64)?clk:badclk;tick=0;frees=0;ra=old_dispatch(&a,&x,&ba,8,NULL);fa=frees;frees=0;rb=new_dispatch(&b,&y,&bb,8,&p);assert(ra==rb&&ba==bb&&fa==frees&&!memcmp(&a,&b,sizeof(a))&&!memcmp(&x,&y,sizeof(x)));assert(p.calls[GSC53_SOURCE]==calls&&p.calls[GSC53_TASK_GUARD]==1&&p.clock_failures==((i&64)?0:calls+1));}puts("R53D task guard observer: 8192 on/off cases PASS");return 0;}
