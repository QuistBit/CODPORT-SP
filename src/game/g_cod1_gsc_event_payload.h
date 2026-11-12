#ifndef G_COD1_GSC_EVENT_PAYLOAD_H
#define G_COD1_GSC_EVENT_PAYLOAD_H

/* Native producers explicitly opt in. Modes 11/12 USE/TOUCH supply one actor;
 * older constructors retain event-only delivery.
 * Inputs are borrowed only during this call; matching tasks run on advance. */
#if defined(COD1_GSC_TEST_WIP) && COD1_GSC_TEST_WIP
#include "cod1_gsc/gsc_fixture.h"
struct gentity_s;
int G_Cod1GscTestEntityEventWithPayload(struct gentity_s *owner,
    const char *event, const GscEventValue *values, size_t count,
    int *out_awakened);
#else
static __inline int G_Cod1GscPayloadDisabled(int *out_awakened) {
    if (out_awakened) *out_awakened = 0;
    return 0;
}
#define G_Cod1GscTestEntityEventWithPayload(owner,event,values,count,out_awakened) \
    G_Cod1GscPayloadDisabled((int *)(out_awakened))
#endif
#endif
