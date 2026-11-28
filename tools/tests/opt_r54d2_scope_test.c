#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "gsc_r237_queue_sync54.h"
#define CAP 32
#define TASK_DEAD 0
#define TASK_READY 1
#define TASK_SLEEP 2
#define TASK_EVENT 3
enum {GSC53_SCAN,GSC53_DISPATCH,GSC53_QUEUE_SHADOW,GSC53_QUEUE_DIRTY,GSC53_STAGES};
typedef struct {int status,r237_enqueued,wake_ms,action;uint64_t serial,r243_time_root_incarnation,r237_insertion;} Task;
typedef struct {uint64_t stamp,insertion;int bucket,task_count,calls;int order[CAP*3];Task tasks[CAP];char error[128];void *r54_dirty_scope;} GscRuntime;
typedef struct {uint64_t stamp;int bucket;} R237QueueView39;
typedef struct {uint64_t calls[GSC53_STAGES],ticks[GSC53_STAGES],scans,examined,dispatches,budget_spent;unsigned max_tasks;int queue_shadow,queue_dirty;uint64_t shadow_checks,shadow_rows,shadow_skipped,shadow_matches,shadow_mismatches,dirty_checks,dirty_matches,dirty_mismatches,dirty_skipped,dirty_feeds,dirty_audit_rows,dirty_reseeds;} GscR237Profile53;
static Task *task_at(const GscRuntime*r,int n){return (Task*)&r->tasks[n];}
static uint64_t r237_owner_stamp(const GscRuntime*r){return r->stamp;}
static int r237_queue_view39(const GscRuntime*r,R237QueueView39*v){v->stamp=r->stamp;v->bucket=r->bucket;return 1;}
static int rfail(GscRuntime*r,int line,const char*s){(void)line;snprintf(r->error,sizeof(r->error),"%s",s);return 0;}
static uint64_t ticks;
static uint64_t r237_profile_begin53(GscR237Profile53*p){return p?++ticks:0;}
static void r237_profile_end53(GscR237Profile53*p,unsigned stage,uint64_t t){if(p){p->calls[stage]++;p->ticks[stage]+=++ticks-t;}}
#include "gsc_vm_r237_dirty_scope54.inc"
static int omit;
static int r237_dispatch_task53(GscRuntime*r,Task*t,int *budget,uint64_t stamp,GscR237Profile53*p){
    int action=t->action,slot=(int)(t-r->tasks),other=(slot+1)%r->task_count;Task*u=r->tasks+other;(void)p;
    r54_dirty_mark_task(r,t);
    if(!omit&&(action==3||action==4))r54_dirty_mark_task(r,u);
    assert(r->calls<CAP*3);r->order[r->calls++]=slot;--*budget;t->status=TASK_DEAD;t->r237_enqueued=0;
    if(action==1){t->status=TASK_SLEEP;t->r237_enqueued=1;t->wake_ms++;t->r237_insertion=++r->insertion;}
    if(action==2){t->status=TASK_READY;t->r237_enqueued=1;t->r237_insertion=++r->insertion;t->action=0;}
    if(action==3&&u!=t){u->status=TASK_DEAD;}
    if(action==4&&u!=t){u->status=TASK_READY;u->r237_enqueued=1;u->r243_time_root_incarnation=stamp;u->wake_ms=r->bucket;u->r237_insertion=++r->insertion;u->serial+=100;u->action=0;}
    if(action==5)++r->stamp;
    if(action==6)return rfail(r,0,"dispatch failure");
    return 1;
}
#include "gsc_vm_r237_queue_shadow54.inc"
#define r237_drain_bucket_slice53 old_drain
#include "new_drain.inc"
#undef r237_drain_bucket_slice53
#include "gsc_vm_r237_dirty_audit54.inc"
#include "dirty_core.inc"
#define r237_drain_bucket_slice53 new_drain
#include "gsc_vm_r237_dirty_drain54.inc"
#undef r237_drain_bucket_slice53
static int pause_now(void){return 1;}
static uint64_t rng=UINT64_C(0xb1946fc731bc3215);
static unsigned rnd(void){rng^=rng<<13;rng^=rng>>7;rng^=rng<<17;return (unsigned)rng;}
int main(void){
    unsigned i;uint64_t checks=0,dispatches=0,feeds=0,rows=0,reseeds=0;unsigned complete=0,paused=0,failed=0;
    for(i=0;i<100000;i++){
        GscRuntime a={0},b,c;GscR237Profile53 off={0},on={0};int n,na=-1,nb=-1,nc=-1,ba=100,bb=100,bc=100,ra,rb,rc;int(*expire)(void)=i%9==0?pause_now:NULL;
        a.stamp=i%2?7:0;a.bucket=(int)(rnd()%1000);a.task_count=(int)(rnd()%(CAP+1));
        for(n=0;n<a.task_count;n++){Task*t=a.tasks+n;t->status=(int)(rnd()%4);t->r237_enqueued=1;t->wake_ms=a.bucket+(int)(rnd()%4);t->serial=(uint64_t)n+1;t->r243_time_root_incarnation=a.stamp;t->r237_insertion=++a.insertion;t->action=(int)(rnd()%7);if(i%3)t->action%=5;}
        if(a.task_count&&i%19==0){a.tasks[0].status=TASK_READY;a.tasks[0].r237_enqueued=0;}
        if(a.task_count>=2&&i%23==0){a.tasks[0].status=a.tasks[1].status=TASK_READY;a.tasks[0].wake_ms=a.tasks[1].wake_ms=a.bucket;a.tasks[0].r237_insertion=a.tasks[1].r237_insertion=1;}
        b=a;c=a;on.queue_shadow=1;on.queue_dirty=1;
        ticks=0;ra=old_drain(&a,&ba,expire,&na,NULL);assert(!ticks);
        ticks=0;rb=new_drain(&b,&bb,expire,&nb,&off);assert(!off.shadow_checks&&!off.calls[GSC53_QUEUE_SHADOW]);
        ticks=0;rc=new_drain(&c,&bc,expire,&nc,&on);
        assert(ra==rb&&ra==rc&&na==nb&&na==nc&&ba==bb&&ba==bc&&!memcmp(&a,&b,sizeof(a))&&!memcmp(&a,&c,sizeof(a)));
        assert(on.shadow_checks==on.shadow_matches&&!on.shadow_mismatches&&!on.shadow_skipped&&on.calls[GSC53_QUEUE_SHADOW]==on.shadow_checks);
        assert(on.dirty_checks==on.dirty_matches&&!on.dirty_mismatches&&!on.dirty_skipped&&on.dirty_checks==on.shadow_checks);
        assert(!c.r54_dirty_scope);feeds+=on.dirty_feeds;rows+=on.dirty_audit_rows;reseeds+=on.dirty_reseeds;
        checks+=on.shadow_checks;dispatches+=(unsigned)a.calls;if(ra==1)complete++;else if(ra==2)paused++;else failed++;
    }
    assert(checks&&dispatches&&complete&&paused&&failed);
    printf("R54D2 drain PASS: 100000 cases, %llu successful shadow comparisons, %llu dispatches, complete=%u paused=%u failed=%u\n",(unsigned long long)checks,(unsigned long long)dispatches,complete,paused,failed);printf("dirty feeds=%llu audit rows=%llu reseeds=%llu\n",(unsigned long long)feeds,(unsigned long long)rows,(unsigned long long)reseeds);
    {GscRuntime a={0};GscR237Profile53 p={0};int budget=10,next=-1;
     a.task_count=2;a.stamp=7;a.bucket=0;a.insertion=2;
     a.tasks[0]=(Task){TASK_READY,1,0,3,1,7,2};a.tasks[1]=(Task){TASK_SLEEP,1,1,0,2,7,1};
     p.queue_dirty=1;omit=1;assert(new_drain(&a,&budget,NULL,&next,&p)==1);omit=0;
     assert(!a.r54_dirty_scope&&p.dirty_checks==2&&p.dirty_matches==1&&p.dirty_mismatches==1&&next==INT32_MAX);
    }
    puts("omitted non-head mutation detected; scope cleared");return 0;
}
