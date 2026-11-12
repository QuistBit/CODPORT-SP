#ifndef G_COD1_GSC_COMBAT_H
#define G_COD1_GSC_COMBAT_H

#include "cod1_gsc/gsc_fixture.h"

struct gentity_s;

/* Capture once before native callbacks. Attacker is an opaque value snapshot;
 * owner lifetime still guards the native continuation after every dispatch.
 * The caller keeps this ticket private and stable across its combat hooks. */
typedef struct GscCombatEventTicket {
    GscEntityRef owner;
    GscEventValue attacker;
    struct gentity_s *ownerAddress;
    struct gentity_s *attackerAddress;
    int tracked;
} GscCombatEventTicket;

#if defined(COD1_GSC_TEST_WIP) && COD1_GSC_TEST_WIP
void G_Cod1GscTestCombatCapture(struct gentity_s *owner,
    struct gentity_s *attacker, GscCombatEventTicket *ticket);
int G_Cod1GscTestCombatCurrent(const GscCombatEventTicket *ticket);
/* Returns native continuation safety, independently of event acceptance.
 * Mode 12 queues damage(amount,attacker) or ordinary combat death(attacker).
 * Zero damage is a no-op. Inputs are copied before engine callbacks.
 * Unrepresentable world/null attackers are undefined in this port subset. */
int G_Cod1GscTestCombatDamageEvent(const GscCombatEventTicket *ticket, int take);
int G_Cod1GscTestCombatDeathEvent(const GscCombatEventTicket *ticket);
#else
static __inline void G_Cod1GscCombatDisabledCapture(GscCombatEventTicket *ticket) {
    if (ticket) ticket->tracked = 0;
}
#define G_Cod1GscTestCombatCapture(owner,attacker,ticket) \
    G_Cod1GscCombatDisabledCapture((GscCombatEventTicket *)(ticket))
#define G_Cod1GscTestCombatCurrent(ticket) 1
#define G_Cod1GscTestCombatDamageEvent(ticket,take) 1
#define G_Cod1GscTestCombatDeathEvent(ticket) 1
#endif

#endif
