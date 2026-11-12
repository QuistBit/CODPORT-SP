/* Fixed VM/Session/GAME bridge for the actual late6FAD0 phase only. */
#ifndef GSC_POSTCOHORT_PRIVATE_H
#define GSC_POSTCOHORT_PRIVATE_H
#include "gsc_placed_cohort_private.h"
struct GscSession;
int gsc_execute_placed_postcohort(GscRuntime*,const void*,uint64_t,uint32_t,char*,size_t);
int gsc_placed_postcohort_current(GscRuntime*,uint64_t);
int gsc_placed_postcohort_call_scope(GscRuntime*,uint64_t,int);
int gsc_session_postcohort_admit(const void*,GscRuntime*,uint64_t,uint32_t);
int gsc_session_postcohort_scope(const void*,GscRuntime*,uint64_t,uint32_t);
int gsc_session_postcohort_native_begin(const void*,GscRuntime*,uint64_t,uint32_t,int(*)(void*),void*,char*,size_t);
int gsc_session_postcohort_native_check(const void*,GscRuntime*,uint64_t,uint32_t);
int gsc_session_postcohort_native_finish(const void*,GscRuntime*,uint64_t,uint32_t,int);
int gsc_session_postcohort_native_scope(struct GscSession*,uint64_t,uint64_t,const void*,int);
int gsc_session_postcohort_graph_current(struct GscSession*,uint64_t,uint64_t);
int gsc_session_postcohort_graph_map_matches(struct GscSession*,uint64_t,uint64_t,const GscMapExecutionCertificate*);
int gsc_session_postcohort_graph_script_current(struct GscSession*,uint64_t,uint64_t,GscNodeScriptRef);
/* These are real fixed GAME implementations, never selected success hooks. */
int G_Cod1ActorPostCohortNative(struct GscSession*,uint64_t,uint64_t,const void*,char*,size_t);
int G_Cod1ActorPostCohortNativeCheck(struct GscSession*,uint64_t,uint64_t,const void*);
int G_Cod1ActorPostCohortNativeFinish(struct GscSession*,uint64_t,uint64_t,const void*,int);
#endif
