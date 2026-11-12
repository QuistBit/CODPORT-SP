/* R113 mode-20/21-only native combat deferral. A radius callback executes while
 * the session scheduler is busy; native G_Damage still commits its real
 * adjusted damage and produces damage/death in its original order. Only that
 * explicit producer scope may copy notifications here. Earlier constructors
 * and every unrelated busy notification retain their existing abort path.
 *
 * This is a bounded port scheduling policy, not original frame timing parity.
 * Notification delivery happens after start/advance returns, at exactly the
 * producer's captured logical millisecond, before another scheduler advance.
 * Queue or lifetime failure cancels the session without undoing committed
 * damage. There is no atomic-damage or retired-owner delivery promise. */
#define GSC_TEST_BLAST_EVENT_CAPACITY 128
typedef struct {
    GscCombatEventTicket ticket;
    GscEventValue values[2];
    GscBlastWaiterSet waiters;
    GscSession *session;
    uint64_t id, epoch, incarnation;
    int logical_ms, damage;
    size_t count;
} GscTestBlastPendingEvent;
typedef struct {
    GscTestBlastPendingEvent entries[GSC_TEST_BLAST_EVENT_CAPACITY];
    GscSession *session;
    uint64_t id, epoch, incarnation;
    int logical_ms, active, flushing, failed;
    size_t count, delivered;
    char error[GSC_SESSION_ERROR_CAP];
} GscTestBlastEventQueue;
static GscTestBlastEventQueue gscTestBlastEvents;

static void GscTestBlastEventsReset(void) {
    uint64_t incarnation = gscTestBlastEvents.incarnation;
    /* A nonwrapping incarnation prevents a reset callback from lending its
     * freshly cleared queue to a pending drain of the previous incarnation. */
    if (incarnation != UINT64_MAX) ++incarnation;
    memset(&gscTestBlastEvents, 0, sizeof(gscTestBlastEvents));
    gscTestBlastEvents.incarnation = incarnation;
}
static int GscTestBlastEventsFail(const char *message,
    char *error, size_t capacity) {
    if (!gscTestBlastEvents.failed) {
        gscTestBlastEvents.failed = 1;
        GscTestError(gscTestBlastEvents.error,
            sizeof(gscTestBlastEvents.error), message);
    }
    gscTestStopRequested = 1;
    GscTestError(error, capacity, gscTestBlastEvents.error);
    return 0;
}
static int GscTestBlastEventsLeaseIdentity(GscSession *session, uint64_t id,
    uint64_t epoch, uint64_t incarnation) {
    return incarnation && incarnation != UINT64_MAX &&
        incarnation == gscTestBlastEvents.incarnation &&
        session && session == gscTestSession && id && id == gscTestId &&
        epoch && epoch == G_Cod1EntityMapEpoch();
}
static int GscTestBlastEventsFailLease(GscSession *session, uint64_t id,
    uint64_t epoch, uint64_t incarnation,
    const char *message, char *error, size_t capacity) {
    /* Cancellation/reset is allowed during an engine callback. Report the
     * abandoned old drain without poisoning a new queue incarnation. */
    if (!GscTestBlastEventsLeaseIdentity(session, id, epoch, incarnation)) {
        GscTestError(error, capacity, message); return 0;
    }
    return GscTestBlastEventsFail(message, error, capacity);
}
static int GscTestBlastEventsLeaseCurrent(GscSession *session,
    uint64_t id, uint64_t epoch, uint64_t incarnation) {
    return GscTestBlastEventsLeaseIdentity(session, id, epoch, incarnation) &&
        (gscTestEntityMode == 20 || gscTestEntityMode == 21 || (gscTestEntityMode==22 || gscTestEntityMode==23 || gscTestEntityMode==24 || gscTestEntityMode==25 || (gscTestEntityMode==26 || (gscTestEntityMode==27 || (gscTestEntityMode==28 || (gscTestEntityMode==29 || (gscTestEntityMode==30 || (gscTestEntityMode==31 || (gscTestEntityMode==32 || (gscTestEntityMode==33 || (gscTestEntityMode==34 || gscTestEntityMode==35 || gscTestEntityMode==36 || gscTestEntityMode==37 || gscTestEntityMode==38 || gscTestEntityMode==39 || (gscTestEntityMode==40||(gscTestEntityMode==41||(gscTestEntityMode==42||gscTestEntityMode==43)))))))))))))) && !gscTestCompleted &&
        !gscTestStopRequested && !level.restarted &&
        G_Cod1MapEntities() && g_gametype.integer == GT_SINGLE_PLAYER;
}
static int GscTestBlastEventsRefsCurrent(const GscTestBlastPendingEvent *entry) {
    if (!entry || !entry->ticket.tracked ||
        !G_Cod1EntitySameAllocation(&entry->ticket.owner)) return 0;
    if (entry->ticket.attacker.kind == GSC_EVENT_ENTITY)
        return G_Cod1EntitySameAllocation(&entry->ticket.attacker.entity);
    /* Reserved world/null attackers remain explicitly undefined in the
     * accepted native combat payload subset. No engine pointer is delivered. */
    return entry->ticket.attacker.kind == GSC_EVENT_UNDEFINED;
}
static int GscTestBlastEventsSnapshotCurrent(const GscTestBlastPendingEvent *entry) {
    GscSessionSnapshot snapshot;
    return entry && gsc_session_snapshot(entry->session, &snapshot) &&
        !snapshot.busy && snapshot.session_id == entry->id &&
        snapshot.logical_ms == entry->logical_ms &&
        (snapshot.state == GSC_SESSION_RUNNING ||
         (snapshot.state == GSC_SESSION_COMPLETED && snapshot.active_threads == 0));
}
static int GscTestBlastEventsProducerActive(void) {
    return gscTestBlastEvents.active && !gscTestBlastEvents.flushing;
}
static int GscTestBlastEventsBegin(uint64_t id, uint64_t epoch,
    int logical_ms, char *error, size_t capacity) {
    GscSessionSnapshot snapshot;
    if (!gscTestBusy || logical_ms < 0 ||
        !GscTestBlastEventsLeaseCurrent(gscTestSession, id, epoch,
            gscTestBlastEvents.incarnation) ||
        gscTestBlastEvents.active || gscTestBlastEvents.flushing ||
        gscTestBlastEvents.failed ||
        !gsc_session_snapshot(gscTestSession, &snapshot) ||
        !snapshot.busy || snapshot.state != GSC_SESSION_RUNNING ||
        snapshot.session_id != id || snapshot.logical_ms != logical_ms)
        return GscTestBlastEventsFailLease(gscTestSession, id, epoch,
            gscTestBlastEvents.incarnation,
            "script blast combat queue scope unavailable", error, capacity);
    if (gscTestBlastEvents.count &&
        (gscTestBlastEvents.session != gscTestSession ||
         gscTestBlastEvents.id != id || gscTestBlastEvents.epoch != epoch ||
         gscTestBlastEvents.logical_ms != logical_ms))
        return GscTestBlastEventsFail(
            "script blast combat queue crosses a session or time boundary",
            error, capacity);
    gscTestBlastEvents.session = gscTestSession;
    gscTestBlastEvents.id = id; gscTestBlastEvents.epoch = epoch;
    gscTestBlastEvents.logical_ms = logical_ms;
    gscTestBlastEvents.active = 1;
    return 1;
}
/* Return native continuation safety, independently of queue acceptance.
 * Failure stops later blast targets via the producer's existing lease guard;
 * this target's already committed health/death tail remains native. */
static int GscTestBlastEventsCapture(const GscCombatEventTicket *ticket,
    int take, int damage) {
    GscTestBlastPendingEvent copy;
    char error[GSC_SESSION_ERROR_CAP];
    if (!ticket || !ticket->tracked) return 1;
    memset(&copy, 0, sizeof(copy));
    copy.ticket = *ticket;
    /* The continuation uses identity-only validation. Native borrowed entity
     * addresses serve no purpose in the deferred queue and are not retained. */
    copy.ticket.ownerAddress = NULL; copy.ticket.attackerAddress = NULL;
    copy.session = gscTestBlastEvents.session;
    copy.id = gscTestBlastEvents.id; copy.epoch = gscTestBlastEvents.epoch;
    copy.incarnation = gscTestBlastEvents.incarnation;
    copy.logical_ms = gscTestBlastEvents.logical_ms;
    copy.damage = damage != 0; copy.count = damage ? 2 : 1;
    if (damage) {
        copy.values[0].kind = GSC_EVENT_NUMBER;
        copy.values[0].number = (float)take;
        copy.values[1] = copy.ticket.attacker;
    } else copy.values[0] = copy.ticket.attacker;
    /* All owner/attacker/payload and lease bytes are private before checking
     * even the pure host identity gate. No trap, allocation or borrowed event
     * name is retained. Damage zero keeps the existing native no-op rule. */
    if (damage && !take) return G_Cod1GscTestCombatCurrent(&copy.ticket);
    if (!GscTestBlastEventsProducerActive() || !gscTestBusy ||
        gscTestBlastEvents.failed ||
        !GscTestBlastEventsLeaseCurrent(copy.session, copy.id, copy.epoch,
            copy.incarnation) || !GscTestBlastEventsRefsCurrent(&copy)) {
        (void)GscTestBlastEventsFailLease(copy.session, copy.id, copy.epoch,
            copy.incarnation,
            "script blast combat queue captured a stale operation", NULL, 0);
    } else if (gscTestBlastEvents.count == GSC_TEST_BLAST_EVENT_CAPACITY) {
        (void)GscTestBlastEventsFail(
            "script blast combat queue capacity exceeded after native damage",
            NULL, 0);
    } else {
        /* This separately selected API performs a readonly, busy-safe capture
         * of existing waiter registrations. No entity host or engine service
         * is called. Later waits cannot become recipients of this operation. */
        error[0] = '\0';
        if (!gsc_session_blast_capture_waiters(copy.session, copy.id,
            &copy.ticket.owner, damage ? "damage" : "death", &copy.waiters,
            error, sizeof(error))) {
            (void)GscTestBlastEventsFailLease(copy.session, copy.id, copy.epoch,
                copy.incarnation, error[0] ? error :
                "script blast combat waiter capture refused after native damage",
                NULL, 0);
        } else gscTestBlastEvents.entries[gscTestBlastEvents.count++] = copy;
    }
    return G_Cod1GscTestCombatCurrent(&copy.ticket);
}
static int GscTestBlastEventsEnd(char *error, size_t capacity) {
    size_t n;
    if (!gscTestBlastEvents.active) {
        GscTestError(error, capacity,
            "script blast combat queue scope was reset or is not active");
        return 0;
    }
    if (!GscTestBlastEventsLeaseIdentity(gscTestBlastEvents.session,
        gscTestBlastEvents.id, gscTestBlastEvents.epoch,
        gscTestBlastEvents.incarnation))
        return GscTestBlastEventsFailLease(gscTestBlastEvents.session,
            gscTestBlastEvents.id, gscTestBlastEvents.epoch,
            gscTestBlastEvents.incarnation,
            "script blast combat queue scope lost its session or map",
            error, capacity);
    gscTestBlastEvents.active = 0;
    if (gscTestBlastEvents.failed) {
        GscTestError(error, capacity, gscTestBlastEvents.error); return 0;
    }
    if (!GscTestBlastEventsLeaseCurrent(gscTestBlastEvents.session,
        gscTestBlastEvents.id, gscTestBlastEvents.epoch,
        gscTestBlastEvents.incarnation))
        return GscTestBlastEventsFailLease(gscTestBlastEvents.session,
            gscTestBlastEvents.id, gscTestBlastEvents.epoch,
            gscTestBlastEvents.incarnation,
            "script blast combat queue changed during native damage", error, capacity);
    for (n = gscTestBlastEvents.delivered; n < gscTestBlastEvents.count; ++n)
        if (!GscTestBlastEventsRefsCurrent(&gscTestBlastEvents.entries[n]))
            return GscTestBlastEventsFail(
                "script blast combat queue owner or attacker retired before delivery",
                error, capacity);
    return 1;
}
static int GscTestBlastEventsFlush(char *error, size_t capacity) {
    GscTestBlastPendingEvent copy;
    int accepted, awakened;
    if (!gscTestBlastEvents.count && !gscTestBlastEvents.failed) return 1;
    if (gscTestBlastEvents.failed) {
        GscTestError(error, capacity, gscTestBlastEvents.error); return 0;
    }
    copy = gscTestBlastEvents.entries[gscTestBlastEvents.delivered];
    if (!GscTestBlastEventsLeaseIdentity(copy.session, copy.id, copy.epoch,
        copy.incarnation) || gscTestBlastEvents.active || gscTestBlastEvents.flushing)
        return GscTestBlastEventsFailLease(copy.session, copy.id, copy.epoch,
            copy.incarnation,
            "script blast combat queue drain reentered or producer is active",
            error, capacity);
    gscTestBlastEvents.flushing = 1;
    while (gscTestBlastEvents.delivered < gscTestBlastEvents.count) {
        /* Copy the whole entry before the first cvar callback. A reset of the
         * map/session during a callback cannot substitute another owner. */
        copy = gscTestBlastEvents.entries[gscTestBlastEvents.delivered];
        if (!GscTestBlastEventsLeaseCurrent(copy.session, copy.id, copy.epoch,
            copy.incarnation) || !GscTestBlastEventsRefsCurrent(&copy) ||
            !GscTestBlastEventsSnapshotCurrent(&copy) ||
            !GscTestModuleReadCurrent() ||
            !GscTestBlastEventsLeaseCurrent(copy.session, copy.id, copy.epoch,
                copy.incarnation) || !GscTestBlastEventsRefsCurrent(&copy) ||
            !GscTestBlastEventsSnapshotCurrent(&copy)) {
            if (GscTestBlastEventsLeaseIdentity(copy.session, copy.id, copy.epoch,
                copy.incarnation))
                gscTestBlastEvents.flushing = 0;
            return GscTestBlastEventsFailLease(copy.session, copy.id, copy.epoch,
                copy.incarnation,
                "script blast combat queue cannot deliver in its captured session/time",
                error, capacity);
        }
        awakened = 0;
        accepted = gsc_session_blast_notify_captured(copy.session, copy.id,
            &copy.waiters,
            copy.values, copy.count, &awakened, error, capacity);
        if (!accepted ||
            !GscTestBlastEventsLeaseCurrent(copy.session, copy.id, copy.epoch,
                copy.incarnation) || !GscTestBlastEventsRefsCurrent(&copy) ||
            !GscTestBlastEventsSnapshotCurrent(&copy)) {
            if (GscTestBlastEventsLeaseIdentity(copy.session, copy.id, copy.epoch,
                copy.incarnation))
                gscTestBlastEvents.flushing = 0;
            return GscTestBlastEventsFailLease(copy.session, copy.id, copy.epoch,
                copy.incarnation,
                "script blast combat queue notification refused or lifetime changed",
                error, capacity);
        }
        ++gscTestBlastEvents.delivered;
        /* No task is advanced here. All queued events are posted in producer
         * order at the captured logical time before the next ordinary frame. */
    }
    gscTestBlastEvents.flushing = 0;
    gscTestBlastEvents.count = gscTestBlastEvents.delivered = 0;
    return 1;
}
