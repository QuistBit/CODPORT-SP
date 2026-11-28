#ifndef GSC_VM_BREAKDOWN55_H
#define GSC_VM_BREAKDOWN55_H
#include <stdint.h>
/* Inclusive outermost time per category. Different categories can overlap;
 * recursion in one category is timed once. Counts include nested calls. */
enum { R55B_GUARD, R55B_EVAL, R55B_NATIVE_EXPR, R55B_NATIVE_STMT, R56A_SOUND, R55B_COUNT };
typedef struct {
    uint64_t (*clock)(void);
    uint64_t ticks[R55B_COUNT],calls[R55B_COUNT],outer[R55B_COUNT],starts[R55B_COUNT];
    unsigned depth[R55B_COUNT];
    uint64_t failures,sources,legacy_sources;
} GscBreakdown55;
static inline void R55B_Enter(GscBreakdown55 *s,unsigned stage){
    ++s->calls[stage];
    if(s->depth[stage]++==0){++s->outer[stage];s->starts[stage]=s->clock?s->clock():0;}
}
static inline int R55B_Leave(GscBreakdown55 *s,unsigned stage,int result){
    uint64_t end,start;
    if(--s->depth[stage]==0){
        start=s->starts[stage];end=s->clock?s->clock():0;
        if(!start||!end||end<start)++s->failures;
        else s->ticks[stage]+=end-start;
    }
    return result;
}
/* r and stage are stable expressions (VM pointer and enum constant).
 * call is an int-returning expression, evaluated exactly once. */
#define R55B_CALL(r,stage,call) ((r)->r55_breakdown_scope ? \
    (R55B_Enter((r)->r55_breakdown_scope,(stage)), \
     R55B_Leave((r)->r55_breakdown_scope,(stage),(call))) : (call))
#endif
