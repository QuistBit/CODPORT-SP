#ifndef G_COD1_ACTOR_SCRIPTED_QUEUE_H
#define G_COD1_ACTOR_SCRIPTED_QUEUE_H
#include "g_cod1_actor.h"
#if defined(COD1_GSC_TEST_WIP) && COD1_GSC_TEST_WIP
/* Actual actor-owned PROSPECTIVE commands only, shared with critical sections
 * and the turret host. Neither function consumes commands or changes actual
 * state/AI readiness. Separate original init and real controller are required.
 * Push returns host success; *admitted is the original helper's 0/1 result.
 * Original animscripted still runs init when admitted=0. Caller must not turn
 * helper admission into script completion. Pop is limited to a pending state9
 * above the root; other selected/unsafe states are refused without mutation. */
int G_Cod1ActorScriptedQueuePush(const cod1ActorLease_t *,int *admitted,char *,size_t);
int G_Cod1ActorScriptedQueuePop(const cod1ActorLease_t *,char *,size_t);
#endif
#endif
