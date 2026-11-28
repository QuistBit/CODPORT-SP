#ifndef GSC_SOURCE_PROFILE55_H
#define GSC_SOURCE_PROFILE55_H
#include <stdint.h>
/* Entry identity of the slowest timed source continuation in a phase.
 * Elapsed time is inclusive of nested script/native work, not self time.
 * The PC is the entry frame's cursor, not necessarily a pending statement. */
typedef struct {
    uint64_t ticks,serial,budget_spent;
    int known,depth,fn,pc,line;
    char module[128],function[64];
} GscSourceSample55;
#endif
