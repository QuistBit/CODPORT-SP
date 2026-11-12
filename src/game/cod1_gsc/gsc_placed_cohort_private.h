/* Internal VM/Session bridge. Not a public fixture/facade admission API. */
#ifndef GSC_PLACED_COHORT_PRIVATE_H
#define GSC_PLACED_COHORT_PRIVATE_H
#include "gsc_fixture.h"
/* Selected cohort: post-entry callback is synchronous and borrowed. VM owns
 * genuine-pop provenance and the exact continuation witness; run DATA cannot
 * mint or replace it. owner is the actual Session id; row is ordered index. */
typedef struct {void*user;int(*invoke)(void*,const GscPlacedInitRun*,char*,size_t,int(*)(void*),void*);} GscPlacedCohortPostHost;
int gsc_execute_placed_cohort_init(GscRuntime*,const void*,uint64_t,uint32_t,const GscEntityRef*,const GscEntityExecutionCertificate*,const GscMapExecutionCertificate*,const GscPlacedInitReturnHost*,const GscPlacedCohortPostHost*,GscPlacedInitRun*,char*,size_t);
int gsc_execute_placed_cohort_aitype(GscRuntime*,const void*,uint64_t,uint32_t,const GscEntityScriptEntry*,const GscEntityExecutionCertificate*,GscEntityScriptRun*,char*,size_t);
int gsc_placed_cohort_current(GscRuntime*,uint64_t);

/* Fixed implementation in Session owns the live, pending, exact row grant.
 * A supplied callback, copied result, guessed owner or forged context cannot
 * replace it. No fallback provider exists. */
int gsc_session_placed_cohort_admit(const void*,GscRuntime*,uint64_t,uint32_t,int,const GscEntityScriptEntry*,const GscEntityExecutionCertificate*,const GscMapExecutionCertificate*,const GscPlacedInitReturnHost*,const GscPlacedCohortPostHost*);
int gsc_session_placed_cohort_scope(const void*,GscRuntime*,uint64_t,uint32_t,int);
#endif
