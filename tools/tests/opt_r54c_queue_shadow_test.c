#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <limits.h>
#include "gsc_r237_queue_sync54.h"
#define TASK_DEAD 0
#define TASK_READY 1
#define TASK_SLEEP 2
#define TASK_EVENT 3
#define CAP 1030
typedef struct {int status,r237_enqueued,wake_ms;uint64_t serial,r243_time_root_incarnation,r237_insertion;} Task;
typedef struct {int task_count;Task tasks[CAP];} GscRuntime;
typedef struct {uint64_t shadow_checks,shadow_rows,shadow_skipped,shadow_matches,shadow_mismatches;} GscR237Profile53;
static Task *task_at(const GscRuntime *r,int n){return (Task *)&r->tasks[n];}
#include "gsc_vm_r237_queue_shadow54.inc"
static uint64_t rng=UINT64_C(0x9e3779b97f4a7c15);
static unsigned rnd(void){rng^=rng<<13;rng^=rng>>7;rng^=rng<<17;return (unsigned)rng;}
/* Reference selector uses the same conditions/order as the R237 scan. */
static int reference(const GscRuntime*r,uint64_t stamp,int bucket,const Task **chosen,int *next){
    int n;*chosen=NULL;*next=INT32_MAX;
    for(n=0;n<r->task_count;n++){
        Task *t=task_at(r,n);
        if(t->status!=TASK_READY&&t->status!=TASK_SLEEP)continue;
        if(t->r243_time_root_incarnation!=stamp||!t->r237_enqueued||!t->r237_insertion||t->wake_ms<bucket||t->wake_ms>INT32_C(0xffffff))return 0;
        if(t->wake_ms==bucket){
            if(*chosen&&t->r237_insertion==(*chosen)->r237_insertion)return 0;
            if(!*chosen||t->r237_insertion>(*chosen)->r237_insertion)*chosen=t;
        }else if(t->wake_ms<*next)*next=t->wake_ms;
    }
    return 1;
}
int main(void){
    static GscRuntime r,before;GscR237Profile53 p={0};const Task *chosen;int next,n,bucket;unsigned i;uint64_t serial=0,insertion=0,stamp=0,matched=0,mismatch=0,skipped=0;
    for(i=0;i<50000;i++){
        if(i%127==0)++stamp;
        if(i%251==0)stamp=0;
        bucket=(int)(rnd()%0xffff00u);r.task_count=(int)(rnd()%129);
        memset(r.tasks,0,sizeof(r.tasks));
        for(n=0;n<r.task_count;n++){
            Task *t=r.tasks+n;t->status=(int)(rnd()%4);t->serial=++serial;t->r237_insertion=++insertion;t->r243_time_root_incarnation=stamp;t->r237_enqueued=1;t->wake_ms=bucket+(int)(rnd()%8);
            if(t->status==TASK_DEAD||t->status==TASK_EVENT){t->r237_enqueued=0;t->wake_ms=-1;t->r243_time_root_incarnation=stamp+1;}
        }
        assert(reference(&r,stamp,bucket,&chosen,&next));before=r;
        if(i%17==0){if(chosen)chosen=NULL;else next=next==INT32_MAX?bucket:next+1;++mismatch;}
        else if(i%31==0){r.task_count=CAP;before=r;++skipped;}
        else ++matched;
        r237_shadow_compare54(&r,stamp,bucket,chosen,next,&p);
        assert(!memcmp(&r,&before,sizeof(r)));assert(p.shadow_checks==i+1&&p.shadow_matches==matched&&p.shadow_mismatches==mismatch&&p.shadow_skipped==skipped);
    }
    r.task_count=1;r.tasks[0]=(Task){TASK_READY,1,1,0,0,1};assert(reference(&r,0,1,&chosen,&next));r237_shadow_compare54(&r,0,1,chosen,next,&p);assert(p.shadow_skipped==++skipped);
    r.task_count=1024;for(n=0;n<r.task_count;n++)r.tasks[n]=(Task){TASK_SLEEP,1,0xffffff,(uint64_t)n+1,0,(uint64_t)n+1};
    assert(reference(&r,0,0xffffff,&chosen,&next));assert(chosen==r.tasks+1023);r237_shadow_compare54(&r,0,0xffffff,chosen,next,&p);assert(p.shadow_matches==++matched);
    printf("R54C PASS: %llu checks, %llu matches, %llu injected mismatches, %llu skips, %llu rows\n",(unsigned long long)p.shadow_checks,(unsigned long long)p.shadow_matches,(unsigned long long)p.shadow_mismatches,(unsigned long long)p.shadow_skipped,(unsigned long long)p.shadow_rows);return 0;
}
