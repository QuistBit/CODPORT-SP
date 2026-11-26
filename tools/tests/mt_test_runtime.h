#ifndef MT_TEST_RUNTIME_H
#define MT_TEST_RUNTIME_H
/* Include once in a test translation unit. The dispatcher, admission and
 * lifecycle below are production cod1_jobs_win.inc, not a second scheduler.
 * Only OS primitives, CPU discovery and native MSVC FP syntax are adapted. */
#include "mt_win32_shim.h"
#include "../../src/qcommon/cod1_jobs.h"
#include <xmmintrin.h>

/* CPU discovery is intentionally outside this portable test's claims. */
#define COD1_CPU_WIN_H
static atomic_uint mt_available_cpus=4;
typedef struct {unsigned logical,available;int cpuSets,restricted,conservative;} c1jCpuInfo;
static c1jCpuInfo C1J_QueryCpu(void) {
 c1jCpuInfo cpu={0};
 cpu.logical=cpu.available=atomic_load(&mt_available_cpus);
 return cpu;
}
static unsigned C1J_WorkerBudget(unsigned available,int enabled,int requested) {
 unsigned count;
 if (!enabled || available<2 || !requested) return 0;
 count=available-1;
 if (requested>0 && (unsigned)requested<count) count=(unsigned)requested;
 return count>64?64:count;
}

/* GNU x86 hooks use the real thread-local x87/MXCSR environment. This lets
 * numerical kernel tests inspect controls/flags, but does not compile or
 * validate the production MSVC inline assembly or Win32 x86 ABI. */
#define C1J_EXTERNAL_FP 1
void C1J_FpGet(c1jFp *out) {
 __asm__ volatile("fnstcw %0" : "=m"(out->control));
 __asm__ volatile("fnstsw %0" : "=m"(out->status));
 out->mxcsr=_mm_getcsr();
}
void C1J_FpRestore(const c1jFp *saved,unsigned x87Flags,unsigned sseFlags) {
 unsigned char environment[28];
 unsigned short status;
 __asm__ volatile("fnstenv %0" : "=m"(environment));
 memcpy(&status,environment+4,2);
 status=(unsigned short)((status&~63u)|((saved->status|x87Flags)&63u));
 memcpy(environment,&saved->control,2);
 memcpy(environment+4,&status,2);
 __asm__ volatile("fldenv %0" : : "m"(environment));
 _mm_setcsr((saved->mxcsr&~63u)|((saved->mxcsr|sseFlags)&63u));
}
static void C1J_FpBegin(const c1jFp *fp) {
 __asm__ volatile("fnclex\n\tfldcw %0" : : "m"(fp->control));
 _mm_setcsr(fp->mxcsr&~63u);
}
#define C1J_THREAD_LOCAL _Thread_local
#define C1J_BEFORE_WORK() mt_before_work()
#include "../../src/qcommon/cod1_jobs_win.inc"

/* Logging has its own production backend; it is not under test here. */
static atomic_uint mt_log_detaches;
int C1L_Detach(FILE *file) {
 (void)file;
 atomic_fetch_add(&mt_log_detaches,1);
 return 1;
}
#endif
