#ifndef COD1_ACTOR_FORGET_TARGET14_H
#define COD1_ACTOR_FORGET_TARGET14_H
#include "cod1_actor_perception14.h"

/* Numeric projection of GAME2004D9D0..2004DA65. NOT a native memory layout,
 * lease, public mutator or execution receipt. The caller must establish the
 * real target slot and ALL pointer associations before applying these data
 * stores. No caller is enabled by this header.
 *
 * Native pointers are represented only as equality tokens in this projection.
 * They must never be dereferenced or converted to authority by a port adapter.
 */
typedef struct {
    c1per14Row_t observation;
    /* Four records at actor10FC + i*18: pointer is word1. */
    uint32_t attention[4][6];
    uint32_t enemyTarget, enemyAux38, targetEnemyReferences;
    uint32_t observerTeam, observerEntityActor, observerActorTimer34;
    uint32_t selectedTarget10f8;
} c1per14ForgetData_t;

/* A successful return means only that output DATA was computed. It is not
 * SentientFree, enemy release, target-generation rebinding or Think success.
 * Untouched row bytes (including timestamps and origins) stay untouched.
 */
static int C1PER14_ForgetTargetData(const c1per14ForgetData_t *input,
    uint32_t targetToken, uint32_t nativeNow, c1per14ForgetData_t *output) {
    c1per14ForgetData_t next;
    unsigned i;
    uint32_t deadline;
    if (!input || !output || !targetToken) return 0;
    memcpy(&next, input, sizeof(next));
    /* 2004D9F7: only DWORD row+0, not the entire 0x40-byte row. */
    next.observation.word[0] = 0;
    for (i = 0; i < 4; ++i) {
        if (next.attention[i][1] == targetToken) {
            next.attention[i][0] = 0; /* actor10FC+i*18 */
            next.attention[i][1] = 0; /* actor1100+i*18 */
            next.attention[i][5] = 0; /* actor1110+i*18 */
        }
    }
    if (next.enemyTarget == targetToken) {
        /* Native DEC wraps in DWORD width; no invented saturation. */
        next.targetEnemyReferences--;
        next.enemyAux38 = 0;
        next.enemyTarget = 0;
        if (next.observerTeam == 2 && next.observerEntityActor) {
            deadline = nativeNow + UINT32_C(500);
            /* 2004DA4F/JGE is signed, including DWORD addition wrap. */
            if ((next.observerActorTimer34 ^ UINT32_C(0x80000000)) <
                (deadline ^ UINT32_C(0x80000000)))
                next.observerActorTimer34 = deadline;
        }
    }
    if (next.selectedTarget10f8 == targetToken)
        next.selectedTarget10f8 = 0;
    memcpy(output, &next, sizeof(next));
    return 1;
}
#endif
