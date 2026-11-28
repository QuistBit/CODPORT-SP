/* Differential test of extracted production scheduler functions, with mock
 * directory, time-root membership and continuation dispatch. Not a game test. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#define CAP 16
#define TASK_DEAD 0
#define TASK_READY 1
#define TASK_SLEEP 2
#define TASK_EVENT 3
typedef struct { uint64_t incarnation,next_insertion; int bucket; } R243TimeRoot;
typedef struct { int status,r237_enqueued,wake_ms,action; uint64_t r243_time_root_incarnation,r237_insertion; } Task;
typedef struct {
 R243TimeRoot root; int present,required,physical,active,selected,started,r237_frame_open,r237_frame_target_ms,now_ms,r237_vm_bucket_ms,task_count;
 uint64_t r237_next_insertion,r237_frame_time_root,r237_frame_generation;
 char error[128]; Task tasks[CAP]; int calls,order[CAP*3];
} GscRuntime;
static uint64_t lookups;
static R243TimeRoot *r243_time_root_find(const GscRuntime *r){++lookups;return r&&r->present?(R243TimeRoot*)&r->root:NULL;}
static int r243_time_root_required_found(const GscRuntime *r,const R243TimeRoot *a){return a||r->required;}
static int r243_time_root_physical_found(const GscRuntime *r,const R243TimeRoot *a){return a&&r->physical&&a->incarnation&&a->incarnation!=UINT64_MAX;}
static int r243_time_root_active_found(const GscRuntime *r,const R243TimeRoot *a){return r243_time_root_physical_found(r,a)&&r->active;}
static int gsc_runtime_r237_frame_buckets_selected(const GscRuntime *r){return r&&r->selected&&!r->error[0];}
static Task *task_at(GscRuntime *r,int n){return &r->tasks[n];}
static int rfail(GscRuntime *r,int line,const char *msg){(void)line;snprintf(r->error,sizeof(r->error),"%s",msg);return 0;}
static int r237_dispatch_task(GscRuntime *r,Task *t,int *budget,uint64_t stamp){
 int action=t->action;assert(r->calls<CAP*3);r->order[r->calls++]=(int)(t-r->tasks);--*budget;t->status=TASK_DEAD;
 if(action==1){t->status=TASK_SLEEP;t->wake_ms++;}
 if(action==2){r->root.incarnation++;r->present=1;}
 if(action==3){r->active=0;r->required=1;}
 if(action==4)return rfail(r,0,"dispatch failure");
 if(action==5){t->status=TASK_READY;t->action=0;t->r237_insertion+=100;t->r243_time_root_incarnation=stamp;}
 if(action==6){r->present=0;r->required=1;}
 return 1;
}
static int pause_now(void){return 1;}
#include "r53b_extracted.h"
#include "profile_scan.h"
static uint64_t counter;
static uint64_t good_clock(void){return ++counter;}
static uint64_t bad_clock(void){++counter;return 0;}
static uint32_t seed=0x9e3779b9u;
static uint32_t rnd(void){seed^=seed<<13;seed^=seed>>17;seed^=seed<<5;return seed;}
int main(void){
 unsigned i,j;uint64_t old_lookups=0,new_lookups=0;unsigned completed=0,failed=0,paused=0,dispatched=0;
 assert(old_valid(NULL,0)==new_valid(NULL,0)); {GscRuntime z={0};int budget=0;assert(old_drain(&z,&budget,NULL,NULL)==1);}
 for(i=0;i<100000;i++){
  GscRuntime a={0},b,c; GscR237Profile53 p={0}; int rc, nc=-77, bc=100;int budget_a=100,budget_b=100,next_a=-77,next_b=-77,ra,rb,va,vb;uint64_t generation;
  int normal=(i%4)!=0,legacy=(i%3)==0,bucket=(int)(rnd()%200);
  a.present=!legacy;a.required=!legacy;a.physical=1;a.active=1;a.selected=1;a.started=1;a.r237_frame_open=1;
  a.root.incarnation=1+rnd()%100;a.root.next_insertion=1000;a.root.bucket=bucket;a.r237_vm_bucket_ms=bucket;a.r237_next_insertion=1000;
  a.r237_frame_generation=1+rnd()%8;a.r237_frame_time_root=legacy?0:a.root.incarnation;
  a.r237_frame_target_ms=a.now_ms=bucket+5;generation=a.r237_frame_generation;
  a.task_count=(int)(rnd()%(CAP+1));
  for(j=0;j<(unsigned)a.task_count;j++){
   Task *t=&a.tasks[j];t->status=(int)(rnd()%4);t->r237_enqueued=1;t->r237_insertion=j+1;
   t->r243_time_root_incarnation=a.r237_frame_time_root;t->wake_ms=bucket+(int)(rnd()%3);t->action=(int)(rnd()%7);
   if(normal&&i%5)t->action=(int)(rnd()%2);
   if(!normal&&rnd()%5==0)t->r237_enqueued=0;
   if(!normal&&rnd()%7==0)t->r237_insertion=1;
  }
  if(!normal)switch(rnd()%13){
   case 0:a.present=0;a.required=1;break;case 1:a.physical=0;break;case 2:a.active=0;break;
   case 3:a.root.incarnation=UINT64_MAX;break;case 4:a.root.incarnation=0;break;
   case 5:a.r237_frame_time_root++;break;case 6:a.started=0;break;case 7:a.r237_frame_open=0;break;
   case 8:generation=0;break;case 9:a.root.bucket=a.r237_vm_bucket_ms=-1;break;
   case 10:a.r237_frame_target_ms=a.now_ms=bucket-1;break;case 11:a.selected=0;break;default:break;
  }
  b=a;c=a;lookups=0;va=new_valid(&a,generation);old_lookups+=lookups;lookups=0;vb=new_valid(&b,generation);new_lookups+=lookups;assert(va==vb);
  lookups=0;ra=new_drain(&a,&budget_a,i%7==0?pause_now:NULL,&next_a);old_lookups+=lookups;
  lookups=0;rb=r237_drain_bucket_slice53(&b,&budget_b,i%7==0?pause_now:NULL,&next_b,NULL);new_lookups+=lookups;
  assert(ra==rb&&budget_a==budget_b&&next_a==next_b&&!memcmp(&a,&b,sizeof(a)));
  p.clock=(i%2)?good_clock:bad_clock;p.hz=1000;counter=0;rc=r237_drain_bucket_slice53(&c,&bc,i%7==0?pause_now:NULL,&nc,&p);
  assert(rc==ra&&bc==budget_a&&nc==next_a&&!memcmp(&a,&c,sizeof(a)));
  assert(p.scans==p.calls[GSC53_SCAN]&&p.dispatches==p.calls[GSC53_DISPATCH]&&p.dispatches==(unsigned)c.calls&&p.budget_spent==(unsigned)(100-bc));
  assert(p.examined<=p.scans*(unsigned)c.task_count&&p.max_tasks<=(unsigned)c.task_count);
  assert(counter==2*(p.scans+p.dispatches));
  if(i%2){assert(!p.clock_failures&&p.ticks[GSC53_SCAN]==p.scans&&p.ticks[GSC53_DISPATCH]==p.dispatches);}else{assert(p.clock_failures==p.scans+p.dispatches&&!p.ticks[GSC53_SCAN]&&!p.ticks[GSC53_DISPATCH]);}
  if(ra==1)completed++;else if(ra==2)paused++;else failed++;dispatched+=(unsigned)a.calls;
 }
 assert(completed&&failed&&paused&&dispatched&&new_lookups==old_lookups);
 printf("R53C: 100000 cases, profiled + unprofiled, %u complete, %u failed, %u paused, %u dispatches; lookups %llu -> %llu\n",completed,failed,paused,dispatched,(unsigned long long)old_lookups,(unsigned long long)new_lookups);
 return 0;
}
