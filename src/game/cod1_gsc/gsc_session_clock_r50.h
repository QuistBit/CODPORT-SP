#ifndef GSC_SESSION_CLOCK_R50_H
#define GSC_SESSION_CLOCK_R50_H
#include <stdint.h>
struct GscSession;
/* Internal synchronous value query. No thread census, diagnostics, callback,
 * lifetime extension or execution permission. Keep the full snapshot API. */
typedef struct { int valid,state,logical_ms; } GscSessionClockR50;
GscSessionClockR50 gsc_session_clock_r50(const struct GscSession *,uint64_t);
#endif
