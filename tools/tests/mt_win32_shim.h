#ifndef MT_WIN32_SHIM_H
#define MT_WIN32_SHIM_H
/* A narrow pthread implementation of the Win32 synchronization primitives
 * used by cod1_jobs_win.inc. This is a scheduling/lifetime test adapter, not
 * Windows ABI, CPU-topology, priority, stack-reservation or driver validation. */
#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <sched.h>

#define __stdcall
#define TRUE 1
#define FALSE 0
#define INFINITE UINT32_MAX
#define WAIT_OBJECT_0 0u
#define WAIT_TIMEOUT 258u

typedef uint32_t DWORD;
typedef int32_t LONG;
typedef pthread_mutex_t CRITICAL_SECTION;
typedef struct mtHandle {
 int kind,manual,signaled,joined;
 pthread_mutex_t lock;
 pthread_cond_t condition;
 pthread_t thread;
 unsigned (*function)(void *);
 void *context;
} *HANDLE;

static atomic_uint mt_next_thread=1;
static atomic_uint mt_tick_offset;
static _Thread_local DWORD mt_thread_id;
static atomic_int mt_thread_credits=-1;
static atomic_int mt_live_handles,mt_live_threads,mt_peak_threads;
static atomic_int mt_pause_workers,mt_workers_paused;

static DWORD GetCurrentThreadId(void) {
 if (!mt_thread_id) mt_thread_id=atomic_fetch_add(&mt_next_thread,1);
 return mt_thread_id;
}
static DWORD GetTickCount(void) {
 struct timespec t;
 assert(!clock_gettime(CLOCK_MONOTONIC,&t));
 return (DWORD)((uint64_t)t.tv_sec*1000u+(unsigned)t.tv_nsec/1000000u+
                atomic_load(&mt_tick_offset));
}
static void mt_sleep_ms(unsigned milliseconds) {
 struct timespec t={(time_t)(milliseconds/1000u),(long)(milliseconds%1000u)*1000000L};
 while (nanosleep(&t,&t) && errno==EINTR) {}
}
static LONG InterlockedExchange(volatile LONG *value,LONG next) {
 return __atomic_exchange_n(value,next,__ATOMIC_SEQ_CST);
}
static LONG InterlockedCompareExchange(volatile LONG *value,LONG next,LONG previous) {
 __atomic_compare_exchange_n(value,&previous,next,0,__ATOMIC_SEQ_CST,__ATOMIC_SEQ_CST);
 return previous;
}
static void InitializeCriticalSection(CRITICAL_SECTION *lock) {
 pthread_mutexattr_t attr;
 assert(!pthread_mutexattr_init(&attr));
 assert(!pthread_mutexattr_settype(&attr,PTHREAD_MUTEX_RECURSIVE));
 assert(!pthread_mutex_init(lock,&attr));
 assert(!pthread_mutexattr_destroy(&attr));
}
static void EnterCriticalSection(CRITICAL_SECTION *lock) { assert(!pthread_mutex_lock(lock)); }
static void LeaveCriticalSection(CRITICAL_SECTION *lock) { assert(!pthread_mutex_unlock(lock)); }
static int TryEnterCriticalSection(CRITICAL_SECTION *lock) {
 int result=pthread_mutex_trylock(lock);
 assert(result==0 || result==EBUSY);
 return result==0;
}
static HANDLE CreateEventA(void *security,int manual,int signaled,const char *name) {
 HANDLE h=(HANDLE)calloc(1,sizeof(*h));
 (void)security;(void)name;
 if (!h) return NULL;
 h->kind=1;h->manual=manual;h->signaled=signaled;
 assert(!pthread_mutex_init(&h->lock,NULL));
 assert(!pthread_cond_init(&h->condition,NULL));
 atomic_fetch_add(&mt_live_handles,1);
 return h;
}
static int SetEvent(HANDLE h) {
 assert(h && h->kind==1);
 assert(!pthread_mutex_lock(&h->lock));
 h->signaled=1;
 if (h->manual) assert(!pthread_cond_broadcast(&h->condition));
 else assert(!pthread_cond_signal(&h->condition));
 assert(!pthread_mutex_unlock(&h->lock));
 return 1;
}
static int ResetEvent(HANDLE h) {
 assert(h && h->kind==1);
 assert(!pthread_mutex_lock(&h->lock));
 h->signaled=0;
 assert(!pthread_mutex_unlock(&h->lock));
 return 1;
}
static DWORD WaitForSingleObject(HANDLE h,DWORD timeout) {
 DWORD result=WAIT_OBJECT_0;
 assert(h);
 if (h->kind==2) {
  assert(timeout==INFINITE);
  if (!h->joined) {assert(!pthread_join(h->thread,NULL));h->joined=1;}
  return result;
 }
 assert(h->kind==1);
 assert(!pthread_mutex_lock(&h->lock));
 if (timeout==INFINITE) {
  while (!h->signaled) assert(!pthread_cond_wait(&h->condition,&h->lock));
 } else if (!timeout) {
  if (!h->signaled) result=WAIT_TIMEOUT;
 } else {
  struct timespec until;
  assert(!clock_gettime(CLOCK_REALTIME,&until));
  until.tv_sec+=timeout/1000u;
  until.tv_nsec+=(long)(timeout%1000u)*1000000L;
  if (until.tv_nsec>=1000000000L) {until.tv_sec++;until.tv_nsec-=1000000000L;}
  while (!h->signaled) {
   int error=pthread_cond_timedwait(&h->condition,&h->lock,&until);
   if (error==ETIMEDOUT) {result=WAIT_TIMEOUT;break;}
   assert(!error);
  }
 }
 if (result==WAIT_OBJECT_0 && !h->manual) h->signaled=0;
 assert(!pthread_mutex_unlock(&h->lock));
 return result;
}
static int CloseHandle(HANDLE h) {
 assert(h);
 if (h->kind==1) {
  assert(!pthread_cond_destroy(&h->condition));
  assert(!pthread_mutex_destroy(&h->lock));
 } else assert(h->kind==2 && h->joined);
 free(h);
 atomic_fetch_sub(&mt_live_handles,1);
 return 1;
}
static void *mt_thread_start(void *context) {
 HANDLE h=(HANDLE)context;
 (void)GetCurrentThreadId();
 h->function(h->context);
 atomic_fetch_sub(&mt_live_threads,1);
 return NULL;
}
static uintptr_t _beginthreadex(void *security,unsigned stack,
 unsigned (*function)(void *),void *context,unsigned flags,unsigned *id) {
 HANDLE h;
 int credits=atomic_load(&mt_thread_credits),live,peak;
 (void)security;(void)stack;(void)flags;(void)id;
 while (credits>=0) {
  if (!credits) return 0;
  if (atomic_compare_exchange_weak(&mt_thread_credits,&credits,credits-1)) break;
 }
 h=(HANDLE)calloc(1,sizeof(*h));
 if (!h) return 0;
 h->kind=2;h->function=function;h->context=context;
 atomic_fetch_add(&mt_live_handles,1);
 live=atomic_fetch_add(&mt_live_threads,1)+1;
 peak=atomic_load(&mt_peak_threads);
 while (live>peak && !atomic_compare_exchange_weak(&mt_peak_threads,&peak,live)) {}
 if (pthread_create(&h->thread,NULL,mt_thread_start,h)) {
  atomic_fetch_sub(&mt_live_threads,1);
  atomic_fetch_sub(&mt_live_handles,1);
  free(h);
  return 0;
 }
 return (uintptr_t)h;
}
static void mt_before_work(void) {
 if (atomic_load(&mt_pause_workers)) {
  atomic_fetch_add(&mt_workers_paused,1);
  while (atomic_load(&mt_pause_workers)) mt_sleep_ms(1);
  atomic_fetch_sub(&mt_workers_paused,1);
 }
}
#endif
