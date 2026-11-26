/* Windows backend; compiled once in common.c. No game state is shared with workers. */
#include "cod1_jobs.h"
#include "cod1_cpu_win.h"
#include <process.h>
#include <stdint.h>
#include <xmmintrin.h>
#include <float.h>
#ifndef C1J_BEGINTHREAD
#define C1J_BEGINTHREAD _beginthreadex
#endif
#ifndef C1J_BEFORE_WORK
#define C1J_BEFORE_WORK() ((void)0)
#endif
#ifndef C1J_THREAD_LOCAL
#define C1J_THREAD_LOCAL __declspec(thread)
#endif

typedef struct {
 HANDLE thread,start;
 unsigned long long chunks;
} c1jWorker;

static struct {
 DWORD owner,lastQuery;
 int configured,enabled,requested;
 unsigned budget,created;
 volatile LONG ready,publishedEnabled,stop,active;
 unsigned next,completed,failed,x87,sse;
 /* Admission protects pool lifetime and the single outstanding batch. It is
  * held by its submitter through join. Gate protects short chunk operations
  * and counters; workers never acquire admission. Lock order is admission,
  * then gate. Neither lock is destroyed while late render calls may exist. */
 CRITICAL_SECTION admission,gate;
 unsigned count,grain,chunks;
 c1jJobFn fn;
 void *context;
 c1jFp fp;
 HANDLE done;
 c1jWorker worker[64];
 c1jCpuInfo cpu;
 unsigned long long batches,callerChunks,retiredWorkerChunks;
 unsigned long long nonOwnerBatches,contentionSerial,nestedSerial;
} c1jobs;
static C1J_THREAD_LOCAL unsigned c1jCallbackDepth;

static LONG C1J_AtomicRead(volatile LONG *value) {
 return InterlockedCompareExchange(value,0,0);
}

/* The native FP implementation is unchanged. The portable scheduling test
 * supplies explicit FP hooks, without pretending to validate x87/MSVC ABI. */
#ifndef C1J_EXTERNAL_FP
void C1J_FpGet(c1jFp *result){
 __asm {
  mov eax,result
  fnstcw word ptr[eax]
  fnstsw word ptr[eax+2]
 }
 result->mxcsr=_mm_getcsr();
}
void C1J_FpRestore(const c1jFp *saved,unsigned x87Flags,unsigned sseFlags){
 unsigned char env[28];unsigned short status;
 __asm {
  lea eax,env
  fnstenv [eax]
 }
 memcpy(&status,env+4,2);status=(unsigned short)((status&~63u)|((saved->status|x87Flags)&63));
 memcpy(env,&saved->control,2);memcpy(env+4,&status,2);
 __asm {
  lea eax,env
  fldenv [eax]
 }
 _mm_setcsr((saved->mxcsr&~63u)|((saved->mxcsr|sseFlags)&63));
}
static void C1J_FpBegin(const c1jFp *fp){unsigned short control=fp->control;
 __asm {
  fnclex
  fldcw control
 }
 _mm_setcsr(fp->mxcsr&~63u);
}
#endif

/* A wake is only a hint to claim work. A late worker may see an already
 * completed batch, and must not retain its context or delay its caller.
 * Claim and completion are serialized; computation is outside the lock.
 * The final completed chunk signals only after publishing FP flags/counters.
 * Therefore caller-owned data can be retired at completion, independently of
 * when idle workers get scheduled. All jobs of a batch finish in this frame. */
static void C1J_DoChunks(c1jWorker *worker) {
 for (;;) {
  unsigned first,end;
  void *context;
  c1jJobFn fn;
  c1jFp fp,result;
  int ok;

  EnterCriticalSection(&c1jobs.gate);
  if (!c1jobs.active || c1jobs.next>=c1jobs.chunks) {
   LeaveCriticalSection(&c1jobs.gate);
   return;
  }
  first=c1jobs.next++*c1jobs.grain;
  end=c1jobs.count-first<c1jobs.grain?c1jobs.count:first+c1jobs.grain;
  context=c1jobs.context;
  fn=c1jobs.fn;
  fp=c1jobs.fp;
  LeaveCriticalSection(&c1jobs.gate);

  C1J_FpBegin(&fp);
  c1jCallbackDepth++;
  ok=fn(context,first,end);
  c1jCallbackDepth--;
  C1J_FpGet(&result);

  EnterCriticalSection(&c1jobs.gate);
  if (!ok) c1jobs.failed=1;
  c1jobs.x87|=result.status&63;
  c1jobs.sse|=result.mxcsr&63;
  if (worker) worker->chunks++;
  else c1jobs.callerChunks++;
  if (++c1jobs.completed==c1jobs.chunks) SetEvent(c1jobs.done);
  LeaveCriticalSection(&c1jobs.gate);
 }
}

static unsigned __stdcall C1J_Worker(void *arg) {
 c1jWorker *worker=(c1jWorker *)arg;
 for (;;) {
  WaitForSingleObject(worker->start,INFINITE);
  if (C1J_AtomicRead(&c1jobs.stop)) break;
  C1J_BEFORE_WORK();
  C1J_DoChunks(worker);
 }
 return 0;
}

/* Admission is held and no batch is active during either pool operation. */
static void C1J_ClosePool(void) {
 unsigned i;
 if (!c1jobs.created) return;
 InterlockedExchange(&c1jobs.stop,1);
 for (i=0;i<c1jobs.created;i++) SetEvent(c1jobs.worker[i].start);
 for (i=0;i<c1jobs.created;i++) {
  WaitForSingleObject(c1jobs.worker[i].thread,INFINITE);
  c1jobs.retiredWorkerChunks+=c1jobs.worker[i].chunks;
  CloseHandle(c1jobs.worker[i].thread);
  CloseHandle(c1jobs.worker[i].start);
 }
 CloseHandle(c1jobs.done);
 c1jobs.done=NULL;
 c1jobs.created=0;
 InterlockedExchange(&c1jobs.stop,0);
}

static void C1J_OpenPool(void) {
 unsigned i;
 if (c1jobs.created || !c1jobs.budget) return;
 c1jobs.done=CreateEventA(NULL,TRUE,FALSE,NULL);
 if (!c1jobs.done) return;
 for (i=0;i<c1jobs.budget;i++) {
  c1jWorker *worker=&c1jobs.worker[i];
  memset(worker,0,sizeof(*worker));
  worker->start=CreateEventA(NULL,FALSE,FALSE,NULL);
  if (!worker->start) break;
  worker->thread=(HANDLE)C1J_BEGINTHREAD(NULL,256*1024,C1J_Worker,worker,0,NULL);
  if (!worker->thread) {
   CloseHandle(worker->start);
   worker->start=NULL;
   break;
  }
  c1jobs.created++;
 }
 if (!c1jobs.created) {
  CloseHandle(c1jobs.done);
  c1jobs.done=NULL;
 }
}

void C1J_Configure(int enabled,int requested) {
 DWORD now=GetTickCount(),thread=GetCurrentThreadId();
 c1jCpuInfo cpu;
 unsigned budget;

 /* Management remains main-thread-only, including during nested callbacks. */
 if (c1jCallbackDepth || (c1jobs.owner && c1jobs.owner!=thread)) return;
 enabled=!!enabled;
 if (!C1J_AtomicRead(&c1jobs.ready)) {
  c1jobs.owner=thread;
  InitializeCriticalSection(&c1jobs.admission);
  InitializeCriticalSection(&c1jobs.gate);
  InterlockedExchange(&c1jobs.ready,1);
 }
 if (c1jobs.configured && enabled==c1jobs.enabled &&
     requested==c1jobs.requested && (DWORD)(now-c1jobs.lastQuery)<1000) return;

 cpu=C1J_QueryCpu();
 budget=C1J_WorkerBudget(cpu.available,enabled,requested);
 /* The periodic topology refresh usually changes no execution settings.
  * Publish only its status metadata under the short gate; waiting for an
  * unrelated renderer batch here would serialize the two frame stages once
  * per second. Settings/budget changes still acquire admission and drain. */
 if (c1jobs.configured && enabled==c1jobs.enabled &&
     requested==c1jobs.requested && budget==c1jobs.budget) {
  EnterCriticalSection(&c1jobs.gate);
  c1jobs.cpu=cpu;
  LeaveCriticalSection(&c1jobs.gate);
  c1jobs.lastQuery=now;
  return;
 }
 /* Stop new admissions before draining a live renderer batch. Calls already
  * computing their own serial fallback do not touch the pool or its context. */
 if (!enabled) InterlockedExchange(&c1jobs.publishedEnabled,0);
 EnterCriticalSection(&c1jobs.admission);
 if (budget!=c1jobs.budget || !enabled) C1J_ClosePool();
 c1jobs.configured=1;
 c1jobs.enabled=enabled;
 c1jobs.requested=requested;
 EnterCriticalSection(&c1jobs.gate);
 c1jobs.cpu=cpu;
 LeaveCriticalSection(&c1jobs.gate);
 c1jobs.budget=budget;
 c1jobs.lastQuery=now;
 InterlockedExchange(&c1jobs.publishedEnabled,enabled);
 LeaveCriticalSection(&c1jobs.admission);
}

int C1J_Enabled(void) {
 return C1J_AtomicRead(&c1jobs.ready) && C1J_AtomicRead(&c1jobs.publishedEnabled);
}

static void C1J_RecordSerial(int nested) {
 EnterCriticalSection(&c1jobs.gate);
 if (nested) c1jobs.nestedSerial++;
 else c1jobs.contentionSerial++;
 LeaveCriticalSection(&c1jobs.gate);
}

int C1J_ParallelFor(unsigned count,unsigned grain,c1jJobFn fn,void *context) {
 unsigned chunks,workers,i,x87,sse;
 c1jFp initial;
 int ok;

 if (!count) return 1;
 if (!fn) return 0;
 if (!grain) grain=1;
 if (count>0x7fffffff) return 0;
 chunks=1+(count-1)/grain;
 if (chunks<2 || !C1J_Enabled()) return fn(context,0,count);

 C1J_FpGet(&initial);
 /* Preserve synchronous exception/trap order when exceptions are unmasked. */
 if ((initial.control&63)!=63 || (initial.mxcsr&0x1f80)!=0x1f80)
  return fn(context,0,count);

 /* Main/frontend and renderer share one pool. Never wait for the other
  * submitter: it may be finishing work needed by our surrounding frame. */
 if (!TryEnterCriticalSection(&c1jobs.admission)) {
  C1J_RecordSerial(0);
  return fn(context,0,count);
 }
 if (!c1jobs.configured || !c1jobs.enabled || !c1jobs.budget ||
     !C1J_AtomicRead(&c1jobs.publishedEnabled)) {
  LeaveCriticalSection(&c1jobs.admission);
  return fn(context,0,count);
 }
 /* A CRITICAL_SECTION is recursive. Without this check a callback on the
  * accepted submitter would overwrite its own outstanding batch. Workers
  * instead fail TryEnter above and run nested work serially. */
 if (c1jobs.active) {
  C1J_RecordSerial(1);
  LeaveCriticalSection(&c1jobs.admission);
  return fn(context,0,count);
 }

 C1J_OpenPool();
 workers=c1jobs.created;
 if (workers>chunks-1) workers=chunks-1;
 if (!workers) {
  LeaveCriticalSection(&c1jobs.admission);
  return fn(context,0,count);
 }

 EnterCriticalSection(&c1jobs.gate);
 c1jobs.count=count;
 c1jobs.grain=grain;
 c1jobs.chunks=chunks;
 c1jobs.fn=fn;
 c1jobs.context=context;
 c1jobs.fp=initial;
 c1jobs.next=c1jobs.completed=c1jobs.failed=c1jobs.x87=c1jobs.sse=0;
 ResetEvent(c1jobs.done);
 InterlockedExchange(&c1jobs.active,1);
 LeaveCriticalSection(&c1jobs.gate);
 for (i=0;i<workers;i++) SetEvent(c1jobs.worker[i].start);
 C1J_DoChunks(NULL);
 WaitForSingleObject(c1jobs.done,INFINITE);

 EnterCriticalSection(&c1jobs.gate);
 x87=c1jobs.x87;
 sse=c1jobs.sse;
 ok=!c1jobs.failed;
 c1jobs.batches++;
 if (GetCurrentThreadId()!=c1jobs.owner) c1jobs.nonOwnerBatches++;
 c1jobs.context=NULL;
 c1jobs.fn=NULL;
 InterlockedExchange(&c1jobs.active,0);
 LeaveCriticalSection(&c1jobs.gate);
 C1J_FpRestore(&initial,x87,sse);
 LeaveCriticalSection(&c1jobs.admission);
 return ok;
}

void C1J_Status(c1jStatus *out) {
 unsigned i;
 memset(out,0,sizeof(*out));
 if (!C1J_AtomicRead(&c1jobs.ready)) return;
 /* Normal status calls wait for a complete batch. Callback-local inspection
  * can only observe its own live pool, whose admission is already held. */
 if (!c1jCallbackDepth) EnterCriticalSection(&c1jobs.admission);
 EnterCriticalSection(&c1jobs.gate);
 out->logical=c1jobs.cpu.logical;
 out->available=c1jobs.cpu.available;
 out->budget=c1jobs.budget;
 out->created=c1jobs.created;
 out->batches=c1jobs.batches;
 out->callerChunks=c1jobs.callerChunks;
 out->workerChunks=c1jobs.retiredWorkerChunks;
 for (i=0;i<c1jobs.created;i++) out->workerChunks+=c1jobs.worker[i].chunks;
 out->nonOwnerBatches=c1jobs.nonOwnerBatches;
 out->contentionSerial=c1jobs.contentionSerial;
 out->nestedSerial=c1jobs.nestedSerial;
 LeaveCriticalSection(&c1jobs.gate);
 if (!c1jCallbackDepth) LeaveCriticalSection(&c1jobs.admission);
}

void C1J_Shutdown(void) {
 if (c1jCallbackDepth || (c1jobs.owner && GetCurrentThreadId()!=c1jobs.owner)) return;
 if (C1J_AtomicRead(&c1jobs.ready)) {
  InterlockedExchange(&c1jobs.publishedEnabled,0);
  EnterCriticalSection(&c1jobs.admission);
  C1J_ClosePool();
  c1jobs.enabled=0;
  c1jobs.configured=0;
  c1jobs.budget=0;
  LeaveCriticalSection(&c1jobs.admission);
 }
 C1L_Detach(NULL);
}
