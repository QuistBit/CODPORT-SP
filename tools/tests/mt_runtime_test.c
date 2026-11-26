/* gcc -std=c11 -D_POSIX_C_SOURCE=200809L -pthread -Itools/tests/mt_include
 *     tools/tests/mt_runtime_test.c -lm -o /tmp/mt_runtime_test */
#include "mt_test_runtime.h"
#include <stdio.h>

#define MT_ITEMS 2048u

typedef struct {
 atomic_uint visits[MT_ITEMS],calls;
 unsigned value[MT_ITEMS];
 atomic_int retired,finished;
 DWORD submitter;
 HANDLE started,release,callerSeen,workerSeen;
 int nested,flags,fail;
} mtJob;

typedef struct {unsigned visits[16];} mtNested;

static void mt_prepare(mtJob *job,int paired) {
 unsigned i;
 memset(job,0,sizeof(*job));
 for (i=0;i<MT_ITEMS;i++) atomic_init(&job->visits[i],0);
 atomic_init(&job->calls,0);
 atomic_init(&job->retired,0);
 atomic_init(&job->finished,0);
 job->submitter=GetCurrentThreadId();
 if (paired) {
  job->callerSeen=CreateEventA(NULL,TRUE,FALSE,NULL);
  job->workerSeen=CreateEventA(NULL,TRUE,FALSE,NULL);
  assert(job->callerSeen && job->workerSeen);
 }
}
static void mt_check(const mtJob *job) {
 unsigned i;
 for (i=0;i<MT_ITEMS;i++) {
  assert(atomic_load(&job->visits[i])==1);
  assert(job->value[i]==(i*2654435761u^0x5aa55aa5u));
 }
}
static void mt_retire(mtJob *job) {
 atomic_store(&job->retired,1);
 if (job->started) CloseHandle(job->started);
 if (job->release) CloseHandle(job->release);
 if (job->callerSeen) CloseHandle(job->callerSeen);
 if (job->workerSeen) CloseHandle(job->workerSeen);
}
static int mt_inner(void *context,unsigned first,unsigned end) {
 mtNested *job=(mtNested *)context;
 unsigned i;
 assert(end<=16);
 for (i=first;i<end;i++) job->visits[i]++;
 return 1;
}
static int mt_work(void *context,unsigned first,unsigned end) {
 mtJob *job=(mtJob *)context;
 unsigned i;
 assert(first<end && end<=MT_ITEMS);
 assert(!atomic_load(&job->retired));
 atomic_fetch_add(&job->calls,1);
 if (job->started) SetEvent(job->started);
 if (job->release) assert(WaitForSingleObject(job->release,5000)==WAIT_OBJECT_0);
 if (job->callerSeen) {
  if (GetCurrentThreadId()==job->submitter) {
   SetEvent(job->callerSeen);
   assert(WaitForSingleObject(job->workerSeen,5000)==WAIT_OBJECT_0);
  } else {
   SetEvent(job->workerSeen);
   assert(WaitForSingleObject(job->callerSeen,5000)==WAIT_OBJECT_0);
  }
 }
 if (job->nested) {
  mtNested inner={{0}};
  assert(C1J_ParallelFor(16,4,mt_inner,&inner));
  for (i=0;i<16;i++) assert(inner.visits[i]==1);
 }
 if (job->flags) {
  c1jFp fp;
  C1J_FpGet(&fp);
  C1J_FpRestore(&fp,1,32);
 }
 for (i=first;i<end;i++) {
  assert(atomic_fetch_add(&job->visits[i],1)==0);
  job->value[i]=i*2654435761u^0x5aa55aa5u;
 }
 return !(job->fail && first==0);
}
static void *mt_submit(void *context) {
 mtJob *job=(mtJob *)context;
 job->submitter=GetCurrentThreadId();
 assert(C1J_ParallelFor(MT_ITEMS,64,mt_work,job));
 atomic_store(&job->finished,1);
 return NULL;
}
static void mt_start_held(mtJob *job,pthread_t *thread) {
 mt_prepare(job,0);
 job->started=CreateEventA(NULL,TRUE,FALSE,NULL);
 job->release=CreateEventA(NULL,TRUE,FALSE,NULL);
 assert(job->started && job->release);
 assert(!pthread_create(thread,NULL,mt_submit,job));
 assert(WaitForSingleObject(job->started,5000)==WAIT_OBJECT_0);
}
static void mt_finish_held(mtJob *job,pthread_t thread) {
 SetEvent(job->release);
 assert(!pthread_join(thread,NULL));
 mt_check(job);
 mt_retire(job);
}
static void mt_wait_paused(int expected) {
 DWORD start=GetTickCount();
 while ((atomic_load(&mt_workers_paused)>0)!=expected) {
  assert((DWORD)(GetTickCount()-start)<5000);
  mt_sleep_ms(1);
 }
}

static void test_disabled_and_validation(void) {
 mtJob job;
 c1jStatus status;
 assert(!C1J_Enabled());
 mt_prepare(&job,0);
 assert(C1J_ParallelFor(MT_ITEMS,64,mt_work,&job));
 mt_check(&job);
 assert(atomic_load(&job.calls)==1);
 assert(C1J_ParallelFor(0,0,NULL,NULL));
 assert(!C1J_ParallelFor(1,1,NULL,NULL));
 assert(!C1J_ParallelFor(0x80000000u,1,mt_work,&job));
 C1J_Status(&status);
 assert(!status.created && !status.batches);
 mt_retire(&job);
 puts("PASS disabled/pre-init and argument validation");
}
static void test_frontend_and_backend(void) {
 mtJob job;
 c1jStatus before,after;
 pthread_t render;
 C1J_Configure(1,3);
 mt_prepare(&job,1);
 assert(C1J_ParallelFor(MT_ITEMS,64,mt_work,&job));
 mt_check(&job);
 C1J_Status(&before);
 assert(before.created==3 && before.workerChunks && before.callerChunks);
 mt_retire(&job);
 mt_prepare(&job,1);
 assert(!pthread_create(&render,NULL,mt_submit,&job));
 assert(!pthread_join(render,NULL));
 mt_check(&job);
 C1J_Status(&after);
 assert(after.nonOwnerBatches==before.nonOwnerBatches+1);
 assert(after.batches==before.batches+1);
 assert(after.workerChunks>before.workerChunks);
 assert(atomic_load(&mt_peak_threads)<=3);
 mt_retire(&job);
 puts("PASS frontend and backend submissions use one real pool");
}
static void test_contending_submitters(void) {
 mtJob renderJob,mainJob;
 pthread_t render;
 c1jStatus before,after;
 C1J_Status(&before);
 mt_start_held(&renderJob,&render);
 mt_prepare(&mainJob,0);
 assert(C1J_ParallelFor(MT_ITEMS,64,mt_work,&mainJob));
 mt_check(&mainJob);
 assert(atomic_load(&mainJob.calls)==1);
 assert(!atomic_load(&renderJob.finished));
 mt_finish_held(&renderJob,render);
 C1J_Status(&after);
 assert(after.nonOwnerBatches==before.nonOwnerBatches+1);
 assert(after.contentionSerial==before.contentionSerial+1);
 assert(atomic_load(&mt_peak_threads)<=3);
 mt_retire(&mainJob);
 puts("PASS contending caller completes serially while backend holds pool");
}
static void test_nested_submissions(void) {
 mtJob job;
 c1jStatus before,after;
 C1J_Status(&before);
 mt_prepare(&job,1);
 job.nested=1;
 assert(C1J_ParallelFor(MT_ITEMS,64,mt_work,&job));
 mt_check(&job);
 C1J_Status(&after);
 assert(after.nestedSerial>before.nestedSerial);
 assert(after.contentionSerial>before.contentionSerial);
 assert(after.batches==before.batches+1);
 mt_retire(&job);
 puts("PASS recursive submitter and nested workers preserve parent batch");
}
static void test_fp_and_failure(void) {
 mtJob job;
 c1jFp saved,wanted,after;
 c1jStatus before,status;
 C1J_FpGet(&saved);
 wanted=saved;
 wanted.control=(unsigned short)((wanted.control&~0x0c00u)|0x0400u|63u);
 wanted.status=4;
 wanted.mxcsr=(wanted.mxcsr&~(0x6000u|63u))|0x2000u|0x1f80u|4u;
 C1J_FpRestore(&wanted,0,0);
 mt_prepare(&job,1);
 job.flags=1;
 assert(C1J_ParallelFor(MT_ITEMS,64,mt_work,&job));
 C1J_FpGet(&after);
 assert(after.control==wanted.control);
 assert((after.status&63u)==5u);
 assert((after.mxcsr&~63u)==(wanted.mxcsr&~63u));
 assert((after.mxcsr&63u)==36u);
 mt_check(&job);
 mt_retire(&job);
 C1J_FpRestore(&saved,0,0);

 C1J_Status(&before);
 wanted=saved;
 wanted.status=0;
 wanted.mxcsr=(wanted.mxcsr&~63u)|0x1f80u;
 wanted.control=(unsigned short)((wanted.control|63u)&~1u);
 C1J_FpRestore(&wanted,0,0);
 mt_prepare(&job,0);
 assert(C1J_ParallelFor(MT_ITEMS,64,mt_work,&job));
 C1J_FpRestore(&saved,0,0);
 assert(atomic_load(&job.calls)==1);
 mt_check(&job);
 mt_retire(&job);
 C1J_Status(&status);
 assert(status.batches==before.batches);

 mt_prepare(&job,1);
 job.fail=1;
 assert(!C1J_ParallelFor(MT_ITEMS,64,mt_work,&job));
 mt_check(&job);
 mt_retire(&job);
 puts("PASS real FP controls/flag merge, exception fallback and failure join");
}
static void test_late_wakes(void) {
 mtJob retired,next;
 atomic_store(&mt_pause_workers,1);
 mt_prepare(&retired,0);
 assert(C1J_ParallelFor(MT_ITEMS,64,mt_work,&retired));
 mt_check(&retired);
 mt_wait_paused(1);
 atomic_store(&retired.retired,1);
 atomic_store(&mt_pause_workers,0);
 mt_wait_paused(0);
 mt_prepare(&next,1);
 assert(C1J_ParallelFor(MT_ITEMS,64,mt_work,&next));
 mt_check(&next);
 mt_check(&retired);
 mt_retire(&next);
 mt_retire(&retired);
 puts("PASS delayed workers never retain a completed context");
}
static void *mt_release_soon(void *context) {
 mt_sleep_ms(10);
 SetEvent((HANDLE)context);
 return NULL;
}
static void test_unchanged_topology_refresh(void) {
 mtJob job;
 pthread_t render;
 c1jStatus status;
 mt_start_held(&job,&render);
 /* Force a periodic CPU query while preserving an explicit budget of three.
  * Waiting for admission here would deadlock until the held job times out. */
 atomic_store(&mt_available_cpus,8);
 atomic_fetch_add(&mt_tick_offset,1001);
 C1J_Configure(1,3);
 assert(!atomic_load(&job.finished));
 mt_finish_held(&job,render);
 C1J_Status(&status);
 assert(status.available==8 && status.budget==3 && status.created==3);
 atomic_store(&mt_available_cpus,4);
 atomic_fetch_add(&mt_tick_offset,1001);
 C1J_Configure(1,3);
 puts("PASS unchanged-budget topology refresh never waits for renderer batch");
}
static void *mt_release_on_shutdown(void *context) {
 DWORD start=GetTickCount();
 while (C1J_Enabled()) {
  assert((DWORD)(GetTickCount()-start)<5000);
  mt_sleep_ms(1);
 }
 SetEvent((HANDLE)context);
 return NULL;
}
static void test_reconfigure_and_shutdown(void) {
 mtJob job,serial;
 pthread_t render,release;
 c1jStatus before,after;
 unsigned logDetaches;
 mt_start_held(&job,&render);
 assert(!pthread_create(&release,NULL,mt_release_soon,job.release));
 C1J_Configure(1,1);
 assert(!pthread_join(release,NULL));
 assert(!pthread_join(render,NULL));
 mt_check(&job);
 mt_retire(&job);
 C1J_Status(&after);
 assert(after.budget==1 && !after.created);
 mt_prepare(&job,1);
 assert(C1J_ParallelFor(MT_ITEMS,64,mt_work,&job));
 mt_check(&job);
 mt_retire(&job);
 C1J_Status(&before);
 assert(before.created==1);

 logDetaches=atomic_load(&mt_log_detaches);
 C1J_Configure(0,0);
 assert(!C1J_Enabled());
 assert(atomic_load(&mt_log_detaches)==logDetaches);
 mt_prepare(&serial,0);
 assert(C1J_ParallelFor(MT_ITEMS,64,mt_work,&serial));
 assert(atomic_load(&serial.calls)==1);
 mt_check(&serial);
 mt_retire(&serial);
 C1J_Status(&after);
 assert(!after.created && !after.budget && after.batches==before.batches);
 C1J_Configure(1,0);
 assert(C1J_Enabled());
 mt_prepare(&serial,0);
 assert(C1J_ParallelFor(MT_ITEMS,64,mt_work,&serial));
 assert(atomic_load(&serial.calls)==1);
 mt_check(&serial);
 mt_retire(&serial);
 C1J_Status(&after);
 assert(!after.created && !after.budget && after.batches==before.batches);

 C1J_Configure(1,3);
 mt_start_held(&job,&render);
 assert(!pthread_create(&release,NULL,mt_release_on_shutdown,job.release));
 C1J_Shutdown();
 assert(!pthread_join(release,NULL));
 assert(!pthread_join(render,NULL));
 mt_check(&job);
 mt_retire(&job);
 C1J_Status(&after);
 assert(!C1J_Enabled() && !after.created && !after.budget);
 assert(!atomic_load(&mt_live_handles) && !atomic_load(&mt_live_threads));
 puts("PASS reconfigure/drain, zero workers, independent log and shutdown");
}
static void test_creation_failure(void) {
 mtJob job;
 c1jStatus before,after;
 atomic_store(&mt_thread_credits,1);
 C1J_Configure(1,3);
 mt_prepare(&job,1);
 assert(C1J_ParallelFor(MT_ITEMS,64,mt_work,&job));
 mt_check(&job);
 mt_retire(&job);
 C1J_Status(&after);
 assert(after.created==1 && after.budget==3);
 C1J_Shutdown();
 atomic_store(&mt_thread_credits,0);
 C1J_Configure(1,3);
 C1J_Status(&before);
 mt_prepare(&job,0);
 assert(C1J_ParallelFor(MT_ITEMS,64,mt_work,&job));
 assert(atomic_load(&job.calls)==1);
 mt_check(&job);
 mt_retire(&job);
 C1J_Status(&after);
 assert(!after.created && after.batches==before.batches);
 C1J_Shutdown();
 atomic_store(&mt_thread_credits,-1);
 assert(!atomic_load(&mt_live_handles) && !atomic_load(&mt_live_threads));
 puts("PASS partial and total thread-creation failure retain correct results");
}
static void *mt_stress_submit(void *unused) {
 unsigned iteration;
 (void)unused;
 for (iteration=0;iteration<80;iteration++) {
  mtJob job;
  mt_prepare(&job,0);
  job.nested=iteration%3==0;
  assert(C1J_ParallelFor(MT_ITEMS,64,mt_work,&job));
  mt_check(&job);
  mt_retire(&job);
  sched_yield();
 }
 return NULL;
}
static void test_reconfiguration_stress(void) {
 pthread_t first,second;
 unsigned iteration;
 C1J_Configure(1,3);
 assert(!pthread_create(&first,NULL,mt_stress_submit,NULL));
 assert(!pthread_create(&second,NULL,mt_stress_submit,NULL));
 for (iteration=0;iteration<100;iteration++) {
  c1jStatus status;
  C1J_Configure(iteration%5!=0,(int)(iteration%3)+1);
  C1J_Status(&status);
  assert(status.created<=status.budget && status.budget<=3);
  sched_yield();
 }
 assert(!pthread_join(first,NULL));
 assert(!pthread_join(second,NULL));
 C1J_Shutdown();
 assert(atomic_load(&mt_peak_threads)<=3);
 assert(!atomic_load(&mt_live_handles) && !atomic_load(&mt_live_threads));
 puts("PASS simultaneous submitters with live configuration/status stress");
}
int main(void) {
 test_disabled_and_validation();
 test_frontend_and_backend();
 test_contending_submitters();
 test_nested_submissions();
 test_fp_and_failure();
 test_late_wakes();
 test_unchanged_topology_refresh();
 test_reconfigure_and_shutdown();
 test_creation_failure();
 test_reconfiguration_stress();
 puts("All portable worker-runtime checks passed (not native Windows validation).");
 return 0;
}
