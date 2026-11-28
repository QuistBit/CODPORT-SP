/* Session phase differential: mocked callbacks, no native engine claim. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#define GSC_SESSION_RUNNING 1
#define GSC_SESSION_COMPLETED 2
enum { GSC53_SCAN,GSC53_DISPATCH,GSC53_SOURCE,GSC53_ENDON,GSC53_ARRAY,GSC53_SESSION_ADMIT,GSC53_SESSION_PRE,GSC53_SESSION_POST,GSC53_SESSION_FINISH,GSC53_TASK_GUARD,GSC53_STAGES };
typedef struct {uint64_t (*clock)(void);uint64_t ticks[GSC53_STAGES],calls[GSC53_STAGES],clock_failures;} GscR237Profile53;
typedef struct {int frame_open,host_now_ms;uint64_t frame_generation;} GscR237FrameView;
typedef struct {int state,logical_ms;} View;
typedef struct GscSession {
 int busy,abort_requested,r237_frame_buckets_enabled,r237_suspended_phase;
 int private_error,view_ok,pre_ok,post_ok,vm_result,vm_abort,final_ok,resource_calls;
 struct GscSession *runtime;View view;GscR237FrameView frame;uint64_t id;
} GscSession;
static unsigned order[16],count;
static void record(unsigned v){assert(count<16);order[count++]=v;}
static void msg(char *e,size_t c,const char *s){if(e&&c)snprintf(e,c,"%s",s);}
static int session_r237_frame_private(GscSession*s,char*e,size_t c){(void)e;(void)c;record(1);return s&&s->private_error;}
static int session_enter(GscSession*s,char*e,size_t c){record(2);msg(e,c,"");if(!s)return 0;if(s->busy){s->abort_requested=1;msg(e,c,"busy");return 0;}s->busy=1;s->abort_requested=0;return 1;}
static int session_id_matches(GscSession*s,uint64_t id){record(3);return id&&s->id==id;}
static int gsc_runtime_r237_frame_view(GscSession*s,GscR237FrameView*out){record(4);if(!s->view_ok)return 0;*out=s->frame;return 1;}
static int session_leave_rejected(GscSession*s,const char*m,char*e,size_t c){record(5);s->busy=0;msg(e,c,m);return 0;}
static int session_failed(GscSession*s,const char*m,char*e,size_t c){record(6);s->runtime=NULL;s->view.state=3;s->busy=0;msg(e,c,m);return 0;}
static int session_r237_frame_resources_current(GscSession*s){record(7);return s->resource_calls++?s->post_ok:s->pre_ok;}
static int gsc_r237_frame_phase_slice(GscSession*s,uint64_t g,int x,int(*expired)(void)){(void)g;(void)x;record(8);if(expired)expired();s->abort_requested=s->vm_abort;return s->vm_result;}
static int gsc_r237_frame_phase_profile53(GscSession*s,uint64_t g,int x,int(*expired)(void),GscR237Profile53*p){assert(p);return gsc_r237_frame_phase_slice(s,g,x,expired);}
static int session_after_vm(GscSession*s,int ok,char*e,size_t c){record(9);s->busy=0;if(!ok||s->abort_requested||!s->final_ok)return session_failed(s,"vm failed",e,c);return 1;}
static int expire(void){record(10);return 0;}
#define session_r237_phase53 old_phase
#include "old.inc"
#undef session_r237_phase53
#define session_r237_phase53 new_phase
#include "new.inc"
#undef session_r237_phase53
static uint64_t ticks;
static uint64_t good(void){return ++ticks;}
static uint64_t bad(void){++ticks;return 0;}
static uint64_t reverse(void){return 1000-(++ticks);}
static void setup(GscSession*s,unsigned i){
 memset(s,0,sizeof(*s));s->id=7;s->runtime=(i&1)?s:NULL;s->busy=!!(i&2);s->private_error=!!(i&4);s->r237_frame_buckets_enabled=!!(i&8);
 s->view.state=(i&16)?1:2;s->view.logical_ms=500;s->view_ok=!!(i&32);s->frame.frame_open=!!(i&64);s->frame.frame_generation=(i&128)?8:9;
 s->frame.host_now_ms=(i&256)?500:550;s->r237_suspended_phase=(i>>9)%3;s->pre_ok=!!(i&2048);s->post_ok=!!(i&4096);s->vm_result=(i>>13)%3;s->vm_abort=!!(i&32768);s->final_ok=!!(i&65536);
}
static void normalise(GscSession*s){if(s->runtime==s)s->runtime=NULL;}
int main(void){
 unsigned i,success=0,paused=0,failed=0,stage_hits[GSC53_STAGES]={0};
 for(i=0;i<262144;i++){
  GscSession a,b,c;GscR237Profile53 p={0};char ea[100]={0},eb[100]={0},ec[100]={0};unsigned expected[16],n,j,pcalls=0;int ra,rb,rc;int x=(i>>17)%3;uint64_t id=(i%11)?7:6,g=(i%13)?8:0;
  setup(&a,i);if(i&131072){a.runtime=&a;a.busy=0;a.private_error=0;a.r237_frame_buckets_enabled=1;a.view_ok=1;a.frame.frame_open=1;a.frame.frame_generation=8;a.frame.host_now_ms=500;}
  b=a;c=a;if(a.runtime){b.runtime=&b;c.runtime=&c;} count=0;
  ra=old_phase(&a,id,g,x,expire,ea,sizeof(ea),NULL);n=count;memcpy(expected,order,n*sizeof(*order));
  count=0;rb=new_phase(&b,id,g,x,expire,eb,sizeof(eb),NULL);assert(count==n&&!memcmp(expected,order,n*sizeof(*order)));
  count=0;ticks=0;p.clock=(i%3==0)?good:(i%3==1)?bad:reverse;
  rc=new_phase(&c,id,g,x,expire,ec,sizeof(ec),&p);assert(count==n&&!memcmp(expected,order,n*sizeof(*order)));
  assert(ra==rb&&ra==rc&&!strcmp(ea,eb)&&!strcmp(ea,ec));
  assert(!!a.runtime==!!b.runtime&&!!a.runtime==!!c.runtime);normalise(&a);normalise(&b);normalise(&c);
  assert(!memcmp(&a,&b,sizeof(a))&&!memcmp(&a,&c,sizeof(a)));
  assert(p.calls[GSC53_SESSION_ADMIT]==1);
  for(j=GSC53_SESSION_ADMIT;j<=GSC53_SESSION_FINISH;j++){assert(p.calls[j]<=1);pcalls+=(unsigned)p.calls[j];stage_hits[j]+=(unsigned)p.calls[j];assert(p.ticks[j]==(i%3==0?p.calls[j]:0));}
  assert(ticks==2*pcalls&&p.clock_failures==(i%3==0?0:pcalls));
  if(ra==1)success++;else if(ra==2)paused++;else failed++;
 }
 assert(success&&paused&&failed&&stage_hits[GSC53_SESSION_PRE]&&stage_hits[GSC53_SESSION_POST]&&stage_hits[GSC53_SESSION_FINISH]);
 printf("R53D Session: 262144 cases, complete=%u paused=%u rejected/failed=%u; admit/pre/post/finish=%u/%u/%u/%u\n",success,paused,failed,stage_hits[GSC53_SESSION_ADMIT],stage_hits[GSC53_SESSION_PRE],stage_hits[GSC53_SESSION_POST],stage_hits[GSC53_SESSION_FINISH]);return 0;
}
