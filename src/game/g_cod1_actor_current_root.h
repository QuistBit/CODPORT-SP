#ifndef G_COD1_ACTOR_CURRENT_ROOT_H
#define G_COD1_ACTOR_CURRENT_ROOT_H
#include "g_cod1_actor.h"
#include "../qcommon/cod1_actor_current_root14.h"
#if defined(COD1_GSC_TEST_WIP) && COD1_GSC_TEST_WIP
/* Original111 arithmetic over this actor's ALREADY ADVANCED owned tree.
 * sourceOrdinal is a declared non-root composite or currently positive-goal
 * registered leaf in generic_human, not an
 * original packed animation handle. GAME maps to real native definition order
 * and resolves every visited leaf from current resource owners. No caller
 * supplied weights/phase/tree, lazy asset load, clock advance, event or AI
 * readiness. Unknown active resources, zero-goal selected leaves (unknown native
 * presence), and implicit root queries are held. A positive selected leaf is
 * required by original scripted-record111, which samples its clip, not the
 * root used by CLEAR63. This does not admit a new script native or controller. */
int G_Cod1ActorRootCurrent(const GscEntityRef *,int now,unsigned sourceOrdinal,
    c1arc14Result_t *,int (*current)(void *),void *);
#endif
#endif
