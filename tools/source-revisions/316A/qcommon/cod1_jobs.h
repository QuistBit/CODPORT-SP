#ifndef COD1_JOBS_H
#define COD1_JOBS_H
#include <stddef.h>
#include <stdio.h>
/* Main-thread configuration and synchronous fork/join. Callbacks own disjoint
 * outputs and may only read immutable inputs: no engine/GL/FS/GSC callbacks.
 * Contexts remain owned by the caller until ParallelFor returns. */
typedef int (*c1jJobFn)(void *context,unsigned first,unsigned end);
typedef struct {unsigned logical,available,budget,created;unsigned long long batches,workerChunks,callerChunks;} c1jStatus;
typedef struct {unsigned short control,status;unsigned mxcsr;} c1jFp;
void C1J_Configure(int enabled,int requestedWorkers);
void C1J_Shutdown(void);
int C1J_Enabled(void);
int C1J_ParallelFor(unsigned count,unsigned grain,c1jJobFn fn,void *context);
void C1J_Status(c1jStatus *out);
void C1J_FpGet(c1jFp *out);
void C1J_FpRestore(const c1jFp *saved,unsigned x87Flags,unsigned sseFlags);
/* One explicitly attached log FILE. No worker ever accesses the engine's
 * filesystem tables or reports errors through engine callbacks. */
int C1L_Attach(FILE *file);
int C1L_Matches(FILE *file);
int C1L_Write(FILE *file,const void *bytes,size_t count);
int C1L_Flush(FILE *file,int wait);
int C1L_Detach(FILE *file);
typedef struct {unsigned long long queued,written,flushed,backpressure;unsigned pending;int active,error;} c1lStatus;
void C1L_Status(c1lStatus *out);
#endif
