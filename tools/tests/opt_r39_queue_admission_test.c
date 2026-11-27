#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
/* Minimal VM ownership fixtures. The queue admission include is production
 * code; time-root predicates and claim mirror R38, with lookup accounting. */
typedef struct GscRuntime GscRuntime;
typedef struct {int r237_enqueued,wake_ms;uint64_t r243_time_root_incarnation,r237_insertion;} Task;
typedef struct R243TimeRoot {struct R243TimeRoot *registry_next;GscRuntime *runtime;const void *program;uint64_t incarnation,next_insertion;int bucket,closing;} R243TimeRoot;
struct GscRuntime {const void *program;R243TimeRoot *r243_time_root;int selected,r243_time_root_teardown,r237_vm_bucket_ms;uint64_t r237_next_insertion;Task *member;int line;char error[128];};
static R243TimeRoot *registry;
static unsigned finds;
static int rfail(GscRuntime*r,int line,const char*error){if(r){r->line=line;strcpy(r->error,error);}return 0;}
static R243TimeRoot *r243_time_root_find(const GscRuntime *r){R243TimeRoot*a;finds++;if(!r)return NULL;for(a=registry;a;a=a->registry_next)if((const void*)a->runtime==(const void*)r)return a;return NULL;}
static int r243_time_root_required_found(const GscRuntime*r,const R243TimeRoot*a){return a||r->selected||r->r243_time_root!=NULL||r->r243_time_root_teardown;}
static int r243_time_root_required(const GscRuntime*r){return r&&r243_time_root_required_found(r,r243_time_root_find(r));}
static int r243_time_root_physical_found(const GscRuntime*r,const R243TimeRoot*a){return a&&r->r243_time_root==a&&a->runtime==r&&a->program&&a->program==r->program&&a->incarnation&&a->incarnation!=UINT64_MAX;}
static int r243_time_root_active_found(const GscRuntime*r,const R243TimeRoot*a){return r243_time_root_physical_found(r,a)&&r->selected&&!r->r243_time_root_teardown&&!a->closing;}
static uint64_t r237_owner_stamp(const GscRuntime*r){R243TimeRoot*a=r243_time_root_find(r);return r243_time_root_physical_found(r,a)?a->incarnation:0;}
static int r237_owner_current(const GscRuntime*r,uint64_t seal){R243TimeRoot*a;if(!r)return 0;a=r243_time_root_find(r);if(!r243_time_root_required_found(r,a))return seal==0;return r243_time_root_active_found(r,a)&&seal!=0&&a->incarnation==seal;}
static int r237_bucket(const GscRuntime*r){R243TimeRoot*a;if(!r)return -1;a=r243_time_root_find(r);if(!r243_time_root_required_found(r,a))return r->r237_vm_bucket_ms;return r243_time_root_active_found(r,a)?a->bucket:-1;}
static uint64_t r237_insertion_next(const GscRuntime*r){R243TimeRoot*a;if(!r)return 0;a=r243_time_root_find(r);if(!r243_time_root_required_found(r,a))return r->r237_next_insertion;return r243_time_root_active_found(r,a)?a->next_insertion:0;}
static uint64_t r237_insertion_claim(GscRuntime*r,int line){R243TimeRoot*a;uint64_t next;int required;if(!r)return 0;a=r243_time_root_find(r);required=r243_time_root_required_found(r,a);if(required&&!r243_time_root_active_found(r,a)){rfail(r,line,"R243 time-root allocation is missing or retired");return 0;}next=required?a->next_insertion:r->r237_next_insertion;if(!next||next==UINT64_MAX){rfail(r,line,"R237 bucket insertion identity exhausted");return 0;}if(required)a->next_insertion=next+1;else r->r237_next_insertion=next+1;return next;}
static int task_member(const GscRuntime*r,const Task*t){return r->member==t;}
static int old_available(GscRuntime *r,int count,int line){
 if(!r237_owner_current(r,r237_owner_stamp(r)))return rfail(r,line,"R237 required time-root unavailable");
 if(count<0||!r237_insertion_next(r)||r237_bucket(r)<0||r237_bucket(r)>INT32_C(0xffffff)||
 (uint64_t)count>UINT64_MAX-r237_insertion_next(r))return rfail(r,line,"R237 bucket insertion identity/domain exhausted");
 return 1;
}
static int old_offer(GscRuntime *r,Task *t,int key,int line){
 uint64_t stamp=r237_owner_stamp(r),insertion;int member=!r243_time_root_required(r);
 if(!r237_owner_current(r,stamp))return rfail(r,line,"R237 queue time-root retired");
 if(!member)member=task_member(r,t);
 if(!member)return rfail(r,line,"R237 queue Task belongs to another namespace");
 if(key<r237_bucket(r)||key>INT32_C(0xffffff))return rfail(r,line,"R237 queue key outside nonwrapping cursor domain");
 if(t->r237_enqueued){
  if(t->r243_time_root_incarnation!=stamp||t->wake_ms!=key||!t->r237_insertion)return rfail(r,line,"R237 task already owns another queue key");
  return 1;
 }
 if(!old_available(r,1,line))return 0;
 insertion=r237_insertion_claim(r,line);if(!insertion)return 0;
 t->wake_ms=key;t->r237_insertion=insertion;t->r243_time_root_incarnation=stamp;t->r237_enqueued=1;return 1;
}
#include "../../src/game/cod1_gsc/gsc_vm_r237_queue_admission.inc"
static int p1,p2;
static void setup(GscRuntime*r,R243TimeRoot*a,R243TimeRoot*foreign,Task*t,int mode,int bucket,uint64_t next,int enqueued,int owner,int wake,int insertion){
 memset(r,0,sizeof(*r));memset(a,0,sizeof(*a));memset(foreign,0,sizeof(*foreign));memset(t,0,sizeof(*t));
 r->program=&p1;r->r237_vm_bucket_ms=bucket;r->r237_next_insertion=next;r->member=owner? t:NULL;
 a->runtime=r;a->program=&p1;a->incarnation=7;a->next_insertion=next;a->bucket=bucket;
 foreign->registry_next=mode? a:NULL;registry=foreign;
 if(mode){r->selected=1;r->r243_time_root=a;}
 switch(mode){case 2:foreign->registry_next=NULL;break;case 3:r->r243_time_root=foreign;break;case 4:a->program=&p2;break;case 5:a->incarnation=0;break;case 6:a->incarnation=UINT64_MAX;break;case 7:r->selected=0;break;case 8:a->closing=1;break;case 9:r->r243_time_root_teardown=1;break;case 10:r->r243_time_root=NULL;break;case 11:a->runtime=NULL;break;default:break;}
 t->r237_enqueued=enqueued;t->wake_ms=wake;t->r237_insertion=insertion?42:0;t->r243_time_root_incarnation=mode?7:0;
 if(enqueued==2)t->r243_time_root_incarnation=99;
}
int main(void){
 const int buckets[]={-1,0,50,0xffffff,0x1000000};const uint64_t nexts[]={0,1,7,UINT64_MAX-1,UINT64_MAX};
 unsigned cases=0,old_success_lookups=0,new_success_lookups=0;
 for(int mode=0;mode<12;mode++)for(unsigned b=0;b<5;b++)for(unsigned n=0;n<5;n++)for(int count=-1;count<=2;count++)for(int enq=0;enq<3;enq++)for(int owner=0;owner<2;owner++)for(int w=0;w<2;w++)for(int ins=0;ins<2;ins++){
  GscRuntime r,s;R243TimeRoot a,z,f,g;Task t,u;int x,y,key=buckets[b]+w;
  setup(&r,&a,&f,&t,mode,buckets[b],nexts[n],enq,owner,buckets[b],ins);finds=0;x=old_available(&r,count,17);
  setup(&s,&z,&g,&u,mode,buckets[b],nexts[n],enq,owner,buckets[b],ins);finds=0;y=r237_queue_available(&s,count,17);
  assert(x==y&&r.line==s.line&&!strcmp(r.error,s.error)&&r.r237_next_insertion==s.r237_next_insertion&&a.next_insertion==z.next_insertion);assert(finds==1);
  setup(&r,&a,&f,&t,mode,buckets[b],nexts[n],enq,owner,buckets[b],ins);finds=0;x=old_offer(&r,&t,key,19);unsigned before=finds;
  setup(&s,&z,&g,&u,mode,buckets[b],nexts[n],enq,owner,buckets[b],ins);finds=0;y=r237_queue_offer(&s,&u,key,19);
  assert(x==y&&r.line==s.line&&!strcmp(r.error,s.error)&&r.r237_next_insertion==s.r237_next_insertion&&a.next_insertion==z.next_insertion&&!memcmp(&t,&u,sizeof(t)));
  assert(finds<=3);if(mode==1&&b==2&&n==1&&enq==0&&owner&&w==0&&x){old_success_lookups=before;new_success_lookups=finds;}
  cases++;
 }
 assert(old_success_lookups==11&&new_success_lookups==3);
 printf("R39: %u configurations, %u differential calls; selected fresh offer root lookups %u -> %u\n",cases,cases*2,old_success_lookups,new_success_lookups);return 0;
}
