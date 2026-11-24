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
typedef struct {HANDLE thread,start;unsigned long long chunks;} c1jWorker;
static struct {
 DWORD owner,lastQuery;int configured,enabled,requested,lockReady;unsigned budget,created;volatile LONG stop,active;
 unsigned next,completed,failed,x87,sse;CRITICAL_SECTION gate;
 unsigned count,grain,chunks;c1jJobFn fn;void *context;c1jFp fp;
 HANDLE done;c1jWorker worker[64];c1jCpuInfo cpu;
 unsigned long long batches,callerChunks,retiredWorkerChunks;
} c1jobs;
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
/* A wake is only a hint to claim work. A late worker may see an already
 * completed batch, and must not retain its context or delay its caller.
 * Claim and completion are serialized; computation is outside the lock.
 * The final completed chunk signals only after publishing FP flags/counters.
 * Therefore caller-owned data can be retired at completion, independently of
 * when idle workers get scheduled. All jobs of a batch finish in this frame. */
static void C1J_DoChunks(c1jWorker *worker){
 for(;;){unsigned first,end;void *context;c1jJobFn fn;c1jFp fp,result;int ok;
  EnterCriticalSection(&c1jobs.gate);
  if(!c1jobs.active || c1jobs.next>=c1jobs.chunks){LeaveCriticalSection(&c1jobs.gate);return;}
  first=c1jobs.next++*c1jobs.grain;end=c1jobs.count-first<c1jobs.grain?c1jobs.count:first+c1jobs.grain;
  context=c1jobs.context;fn=c1jobs.fn;fp=c1jobs.fp;
  LeaveCriticalSection(&c1jobs.gate);
  C1J_FpBegin(&fp);ok=fn(context,first,end);C1J_FpGet(&result);
  EnterCriticalSection(&c1jobs.gate);
  if(!ok)c1jobs.failed=1;c1jobs.x87|=result.status&63;c1jobs.sse|=result.mxcsr&63;
  if(worker)worker->chunks++;else c1jobs.callerChunks++;
  if(++c1jobs.completed==c1jobs.chunks)SetEvent(c1jobs.done);
  LeaveCriticalSection(&c1jobs.gate);
 }
}
static unsigned __stdcall C1J_Worker(void *arg){c1jWorker *w=(c1jWorker*)arg;
 for(;;){WaitForSingleObject(w->start,INFINITE);if(c1jobs.stop)break;
  C1J_BEFORE_WORK();C1J_DoChunks(w);
 }return 0;
}
static void C1J_ClosePool(void){unsigned i;if(!c1jobs.created)return;
 InterlockedExchange(&c1jobs.stop,1);
 for(i=0;i<c1jobs.created;i++)SetEvent(c1jobs.worker[i].start);
 for(i=0;i<c1jobs.created;i++){WaitForSingleObject(c1jobs.worker[i].thread,INFINITE);c1jobs.retiredWorkerChunks+=c1jobs.worker[i].chunks;CloseHandle(c1jobs.worker[i].thread);CloseHandle(c1jobs.worker[i].start);}
 CloseHandle(c1jobs.done);c1jobs.done=NULL;c1jobs.created=0;InterlockedExchange(&c1jobs.stop,0);
}
static void C1J_OpenPool(void){unsigned i;if(c1jobs.created||!c1jobs.budget)return;
 if(!c1jobs.lockReady){InitializeCriticalSection(&c1jobs.gate);c1jobs.lockReady=1;}
 c1jobs.done=CreateEventA(NULL,TRUE,FALSE,NULL);if(!c1jobs.done)return;
 for(i=0;i<c1jobs.budget;i++){
  c1jWorker*w=&c1jobs.worker[i];memset(w,0,sizeof(*w));
  w->start=CreateEventA(NULL,FALSE,FALSE,NULL);if(!w->start)break;
  w->thread=(HANDLE)C1J_BEGINTHREAD(NULL,256*1024,C1J_Worker,w,0,NULL);
  if(!w->thread){CloseHandle(w->start);w->start=NULL;break;}c1jobs.created++;
 }
 if(!c1jobs.created){CloseHandle(c1jobs.done);c1jobs.done=NULL;}
}
void C1J_Configure(int enabled,int requested){DWORD now=GetTickCount();c1jCpuInfo cpu;unsigned budget;
 if(c1jobs.owner&&c1jobs.owner!=GetCurrentThreadId())return;
 if(c1jobs.active)return;
 if(c1jobs.configured&&enabled==c1jobs.enabled&&requested==c1jobs.requested&&(DWORD)(now-c1jobs.lastQuery)<1000)return;
 cpu=C1J_QueryCpu();budget=C1J_WorkerBudget(cpu.available,enabled,requested);
 if(budget!=c1jobs.budget||!enabled)C1J_ClosePool();
 if(!enabled)C1L_Detach(NULL);
 c1jobs.owner=GetCurrentThreadId();c1jobs.configured=1;c1jobs.enabled=!!enabled;c1jobs.requested=requested;c1jobs.cpu=cpu;c1jobs.budget=budget;c1jobs.lastQuery=now;
}
int C1J_Enabled(void){return c1jobs.configured&&c1jobs.enabled;}
int C1J_ParallelFor(unsigned count,unsigned grain,c1jJobFn fn,void *context){
 unsigned chunks,workers,i,x87,sse;c1jFp initial;int ok;
 if(!count)return 1;if(!fn)return 0;if(!grain)grain=1;
 if(count>0x7fffffff)return 0;chunks=1+(count-1)/grain;
 if(GetCurrentThreadId()!=c1jobs.owner||c1jobs.active||!c1jobs.enabled||!c1jobs.budget||chunks<2)return fn(context,0,count);
 C1J_FpGet(&initial);
 /* Preserve synchronous exception/trap order when exceptions are unmasked. */
 if((initial.control&63)!=63||(initial.mxcsr&0x1f80)!=0x1f80)return fn(context,0,count);
 C1J_OpenPool();workers=c1jobs.created;if(workers>chunks-1)workers=chunks-1;
 if(!workers)return fn(context,0,count);
 EnterCriticalSection(&c1jobs.gate);
 c1jobs.count=count;c1jobs.grain=grain;c1jobs.chunks=chunks;c1jobs.fn=fn;c1jobs.context=context;c1jobs.fp=initial;
 c1jobs.next=c1jobs.completed=c1jobs.failed=c1jobs.x87=c1jobs.sse=0;ResetEvent(c1jobs.done);InterlockedExchange(&c1jobs.active,1);
 LeaveCriticalSection(&c1jobs.gate);
 for(i=0;i<workers;i++)SetEvent(c1jobs.worker[i].start);
 C1J_DoChunks(NULL);
 WaitForSingleObject(c1jobs.done,INFINITE);
 EnterCriticalSection(&c1jobs.gate);
 x87=c1jobs.x87;sse=c1jobs.sse;ok=!c1jobs.failed;c1jobs.batches++;c1jobs.context=NULL;c1jobs.fn=NULL;InterlockedExchange(&c1jobs.active,0);
 LeaveCriticalSection(&c1jobs.gate);
 C1J_FpRestore(&initial,x87,sse);return ok;
}
void C1J_Status(c1jStatus *out){unsigned i;memset(out,0,sizeof(*out));out->logical=c1jobs.cpu.logical;out->available=c1jobs.cpu.available;out->budget=c1jobs.budget;out->created=c1jobs.created;out->batches=c1jobs.batches;out->callerChunks=c1jobs.callerChunks;out->workerChunks=c1jobs.retiredWorkerChunks;for(i=0;i<c1jobs.created;i++)out->workerChunks+=c1jobs.worker[i].chunks;}
void C1J_Shutdown(void){if(c1jobs.owner&&GetCurrentThreadId()!=c1jobs.owner)return;if(c1jobs.active)return;C1J_ClosePool();C1L_Detach(NULL);c1jobs.enabled=0;c1jobs.configured=0;}
