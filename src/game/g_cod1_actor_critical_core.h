#ifndef G_COD1_ACTOR_CRITICAL_CORE_H
#define G_COD1_ACTOR_CRITICAL_CORE_H
/* R189 selected Actor state kernel. Not a VM registration, scheduler lock,
 * Actor constructor, SAVE overlay or execution of queued AI behavior.
 *
 * Primary authority: gamex86.dll SHA256
 * 27930aa3f663d0af39a90e6956c980be1e599e0f0c98191a9fb7eaba61481577.
 * Native +38 commands, +a0 count, +a4 prospective stack, +bc top,
 * +2a4 opaque reduction flag; exact windows and CPU cases accompany R189.
 * Semantic copied storage, NOT the native Actor layout.
 *
 * Caller owns the entity/Actor lease and debug string. Begin commits the
 * native state change BEFORE the caller reads/assigns the optional label.
 * A later script argument error MUST NOT roll back this successful commit.
 * Missing labels are supplied by the current native script's WORD+4, not an
 * invented empty string. End does NOT clear that label.
 *
 * Capacity/invalid-domain refusals below are portable safety boundaries.
 * They do not claim that the original unchecked paths reject the same input.
 * This kernel is intentionally not yet bound to the GAME owner or VM.
 */
#include <stdint.h>

enum {
    COD1_CRITICAL_COMMAND_CAPACITY = 13,
    COD1_CRITICAL_STACK_CAPACITY = 6,
    COD1_CRITICAL_STATE = 8,
    COD1_CRITICAL_HELD = -1,
    COD1_CRITICAL_DUPLICATE = 0,
    COD1_CRITICAL_READ_LABEL = 1,
    COD1_CRITICAL_DISABLED = 2,
    COD1_CRITICAL_LEAVE_WARNING = 0,
    COD1_CRITICAL_LEFT = 1
};
typedef struct { int32_t operation, state; } cod1ActorStateCommand_t;
typedef struct {
    cod1ActorStateCommand_t commands[COD1_CRITICAL_COMMAND_CAPACITY];
    int32_t command_count;
    int32_t stack[COD1_CRITICAL_STACK_CAPACITY];
    int32_t top;
    int32_t native_cell02a4;
} cod1ActorCriticalState_t;

static int Cod1ActorCriticalStackValid(const cod1ActorCriticalState_t *s) {
    return s && s->top >= -1 && s->top < COD1_CRITICAL_STACK_CAPACITY;
}
static int Cod1ActorCriticalContains(const cod1ActorCriticalState_t *s) {
    int i;
    if (!Cod1ActorCriticalStackValid(s)) return COD1_CRITICAL_HELD;
    for (i=0; i<=s->top; ++i)
        if (s->stack[i] == COD1_CRITICAL_STATE) return 1;
    return 0;
}
/* Actual 4x4 DWORD table at native 0x20089f40. -1 stops reduction,
 * 0 removes two commands, other values replace the earlier operation. */
static int Cod1ActorCriticalReduceInPlace(cod1ActorCriticalState_t *s) {
    static const int32_t table[4][4] = {
        {-1,-1,-1,-1}, {-1,1,-1,3}, {-1,1,-1,0}, {-1,-1,1,-1}
    };
    int n, a, b, result;
    if (s->command_count < 0 || s->command_count > COD1_CRITICAL_COMMAND_CAPACITY) return 0;
    while ((n=s->command_count) >= 2) {
        if (!s->native_cell02a4) { s->command_count=1; break; }
        a=s->commands[n-2].operation; b=s->commands[n-1].operation;
        if (a<0 || a>3 || b<0 || b>3) return 0;
        result=table[a][b];
        if (result == -1) break;
        if (!result) s->command_count=n-2;
        else {
            s->commands[n-2].operation=result;
            s->commands[n-2].state=s->commands[n-1].state;
            s->command_count=n-1;
        }
    }
    return 1;
}
static int Cod1ActorCriticalReduce(cod1ActorCriticalState_t *s) {
    cod1ActorCriticalState_t staged;
    if (!s) return 0;
    staged=*s;
    if (!Cod1ActorCriticalReduceInPlace(&staged)) return 0;
    *s=staged; return 1;
}
static int Cod1ActorCriticalAppend(cod1ActorCriticalState_t *s, int32_t operation, int32_t state) {
    int n=s->command_count;
    if (n<0 || n>=COD1_CRITICAL_COMMAND_CAPACITY) return 0;
    s->commands[n].operation=operation;
    s->commands[n].state=state;
    s->command_count=n+1;
    return Cod1ActorCriticalReduceInPlace(s);
}
/* Selected native 0x2004e060 with requested state8. The helper, unlike the
 * public enter wrapper, may prune an existing state8 before pushing it.
 * Return 0 is the real admission-table refusal, not a script error. */
static int Cod1ActorCriticalPushState(cod1ActorCriticalState_t *s) {
    cod1ActorCriticalState_t staged;
    int i, first=-1;
    if (!Cod1ActorCriticalStackValid(s)) return COD1_CRITICAL_HELD;
    /* state8 admission row at 0x20089f94: {11,0,0,0,0}. */
    for (i=1; i<=s->top; ++i)
        if (s->stack[i]==11 || s->stack[i]==0) return 0;
    staged=*s;
    /* state8 pruning row at 0x20089ffc: {8,0,0,0,0}. */
    for (i=1; i<=staged.top; ++i)
        if (staged.stack[i]==8 || staged.stack[i]==0) { first=i; break; }
    if (first>=0) {
        while (staged.top>=first) {
            if (!Cod1ActorCriticalAppend(&staged,3,0)) return COD1_CRITICAL_HELD;
            --staged.top;
        }
    }
    if (staged.top>=COD1_CRITICAL_STACK_CAPACITY-1 ||
        !Cod1ActorCriticalAppend(&staged,2,COD1_CRITICAL_STATE)) return COD1_CRITICAL_HELD;
    ++staged.top;
    staged.stack[staged.top]=COD1_CRITICAL_STATE;
    *s=staged; return 1;
}
/* The original wrapper ignores the push helper's 0 return and still reads
 * and assigns the label. Only an unsafe/unsupported portable domain is held.
 * Actor validation is the caller's job and precedes the global disable flag. */
static int Cod1ActorCriticalBegin(cod1ActorCriticalState_t *s, int32_t disable_flag) {
    int contains, pushed;
    if (!s) return COD1_CRITICAL_HELD;
    if (disable_flag>0) return COD1_CRITICAL_DISABLED;
    contains=Cod1ActorCriticalContains(s);
    if (contains<0) return COD1_CRITICAL_HELD;
    if (contains) return COD1_CRITICAL_DUPLICATE;
    pushed=Cod1ActorCriticalPushState(s);
    return pushed<0 ? COD1_CRITICAL_HELD : COD1_CRITICAL_READ_LABEL;
}
static int Cod1ActorCriticalEnd(cod1ActorCriticalState_t *s, int32_t disable_flag) {
    cod1ActorCriticalState_t staged;
    if (!s) return COD1_CRITICAL_HELD;
    if (disable_flag>0) return COD1_CRITICAL_DISABLED;
    /* Original leave reads stack[top] without the query's negative-top guard.
     * The selected portable lane explicitly refuses this unsafe domain. */
    if (!Cod1ActorCriticalStackValid(s) || s->top<0) return COD1_CRITICAL_HELD;
    if (s->stack[s->top]!=COD1_CRITICAL_STATE) return COD1_CRITICAL_LEAVE_WARNING;
    staged=*s;
    if (!Cod1ActorCriticalAppend(&staged,3,0)) return COD1_CRITICAL_HELD;
    --staged.top;
    *s=staged; return COD1_CRITICAL_LEFT;
}
#endif
