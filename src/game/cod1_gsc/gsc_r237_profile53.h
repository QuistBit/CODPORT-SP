#ifndef GSC_R237_PROFILE53_H
#define GSC_R237_PROFILE53_H
#include <stdint.h>
#include <stddef.h>
struct GscRuntime;
struct GscSession;
enum { GSC53_SCAN, GSC53_DISPATCH, GSC53_SOURCE, GSC53_ENDON, GSC53_ARRAY,
 GSC53_SESSION_ADMIT, GSC53_SESSION_PRE, GSC53_SESSION_POST, GSC53_SESSION_FINISH,
 GSC53_TASK_GUARD, GSC53_QUEUE_SHADOW, GSC53_QUEUE_DIRTY, GSC53_STAGES };
/* Opt-in, synchronous, same owner thread. Caller zero-initializes this record
 * and supplies a pure monotonic clock (0 on failure), with no VM or GAME
 * calls, allocations or reentry. Never retained beyond the call.
 * SOURCE is inclusive of native hosts and nested script calls, within DISPATCH.
 * Times are raw clock ticks; scan/dispatch counts are actual completed attempts.
 * No scheduler decision depends on any of these fields. */
typedef struct {
 uint64_t (*clock)(void);
 uint64_t hz, ticks[GSC53_STAGES], calls[GSC53_STAGES];
 uint64_t examined, scans, dispatches, budget_spent, clock_failures;
 unsigned max_tasks;
 int queue_shadow,queue_dirty,queue_indexed;
 uint64_t shadow_checks,shadow_rows,shadow_matches,shadow_mismatches,shadow_skipped;
 uint64_t dirty_checks,dirty_matches,dirty_mismatches,dirty_skipped,dirty_feeds,dirty_audit_rows,dirty_reseeds;
 uint64_t dirty_mark_lookups,dirty_mark_probes,dirty_map_rows,dirty_mark_misses;
 uint64_t indexed_choices,indexed_fallbacks;
} GscR237Profile53;
int gsc_r237_frame_phase_profile53(struct GscRuntime *,uint64_t,int,int (*)(void),GscR237Profile53 *);
int gsc_session_r237_frame_phase_profile53(struct GscSession *,uint64_t,uint64_t,int,int (*)(void),char *,size_t,GscR237Profile53 *);
#endif
