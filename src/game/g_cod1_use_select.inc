/* R90 native USE acquisition reconstructed from gamex86 0x2001d270.
 * Only fresh isolated entity modes5/6/7/8/9/10/11/12 enter this path; modes0..4 retain R87.
 * Hints and keydown share bbox-center reach <=72 and forward cosine >=.76.
 * Native items and USE triggers share distance/angle ranking; triggers get
 * the original -144 bonus. Center visibility does not require a crosshair
 * brush hit. Allocation guards and legacy actor/door priority remain local
 * port policies; this does not implement retail hold timing. */
typedef struct {
    GscEntityRef player;
    GscEntityRef refs[MAX_GENTITIES];
    int count;
    int checkGameplay;
    int rejected;
    int geometryReady;
    int watchedSlot;
    vec3_t eye, viewangles, watchedMin, watchedMax;
} Cod1UseSelection;

static Cod1UseSelection *cod1UseSelection;
static uint64_t cod1UseDiagnosticEpoch;
static int cod1UseDiagnosticCount;

static int G_Cod1UsePlayerEligible(const gentity_t *player);

static int G_Cod1UseFiniteVector(const vec3_t vector) {
    int i;
    for (i = 0; i < 3; ++i)
        if (!(vector[i] >= -3.402823466e38f && vector[i] <= 3.402823466e38f)) return 0;
    return 1;
}

static int G_Cod1UsePlayerCurrent(const Cod1UseSelection *selection, const gentity_t *player) {
    return selection && player == &g_entities[selection->player.slot] &&
        G_Cod1EntitySameAllocation(&selection->player) && player->client;
}

/* Pure borrowed-context guard for the existing nested pickup queries. Outside
 * this selector it changes no legacy query policy or call sequence. */
static int G_Cod1UseSelectionStableRun(gentity_t *player) {
    int i;
    if (!cod1UseSelection) return 1;
    if (cod1UseSelection->rejected) return 0;
    if (!G_Cod1UsePlayerCurrent(cod1UseSelection, player) ||
        level.num_entities != cod1UseSelection->count) return 0;
    if (cod1UseSelection->checkGameplay &&
        (!G_Cod1UsePlayerEligible(player) || player->active || player->melee ||
         player->client->ps.persistant[PERS_HWEAPON_USE])) return 0;
    for (i = 0; i < cod1UseSelection->count; ++i) {
        if (cod1UseSelection->refs[i].epoch) {
            if (!G_Cod1EntitySameAllocation(&cod1UseSelection->refs[i])) return 0;
        } else if (g_entities[i].inuse) return 0;
    }
    if (cod1UseSelection->geometryReady) {
        vec3_t forward, right, up, eye;
        for (i = 0; i < 3; ++i)
            if (player->client->ps.viewangles[i] != cod1UseSelection->viewangles[i]) return 0;
        AngleVectors(player->client->ps.viewangles, forward, right, up);
        CalcMuzzlePointForActivate(player, forward, right, up, eye);
        for (i = 0; i < 3; ++i)
            if (eye[i] != cod1UseSelection->eye[i]) return 0;
    }
    if (cod1UseSelection->watchedSlot >= 0) {
        const gentity_t *target = &g_entities[cod1UseSelection->watchedSlot];
        if (!target->r.linked) return 0;
        for (i = 0; i < 3; ++i)
            if (target->r.absmin[i] != cod1UseSelection->watchedMin[i] ||
                target->r.absmax[i] != cod1UseSelection->watchedMax[i]) return 0;
    }
    return 1;
}

static void G_Cod1UseWatchBounds(Cod1UseSelection *selection, const gentity_t *target) {
    selection->watchedSlot = target->s.number;
    VectorCopy(target->r.absmin, selection->watchedMin);
    VectorCopy(target->r.absmax, selection->watchedMax);
}

/* Invalid nested query data cannot turn a failed pickup query into USE. */
static void G_Cod1UseSelectionRejectRun(void) {
    if (cod1UseSelection) cod1UseSelection->rejected = 1;
}

static int G_Cod1UsePlayerEligible(const gentity_t *player) {
    return player && player->client && player->health > 0 &&
        player->client->ps.stats[STAT_HEALTH] > 0 && !player->aiCharacter &&
        !player->client->ps.aiChar && !(player->r.svFlags & SVF_CASTAI) &&
        !player->client->noclip && player->client->sess.sessionTeam != TEAM_SPECTATOR &&
        !(player->client->ps.pm_flags & PMF_LIMBO);
}

/* Every selector trap is followed by this check before reading entities.
 * The final mode read also closes a mode change inside the event predicate. */
static int G_Cod1UseSelectionCurrent(gentity_t *player) {
    int mode, enabled;
    if (!G_Cod1UseSelectionStable(player) || !G_Cod1UsePlayerEligible(player) ||
        !G_Cod1GscTestUseSelectionActive() || !G_Cod1MapEntities() ||
        g_gametype.integer != GT_SINGLE_PLAYER) return 0;
    mode = G_Cod1GscFeature47(C1CP47_F_MODE);
    if (!G_Cod1UseSelectionStable(player) ||
        !G_Cod1GscTestUseSelectionModeCurrent(mode)) return 0;
    enabled = G_Cod1GscTestEntityEventsEnabled();
    if (!G_Cod1UseSelectionStable(player) || !enabled) return 0;
    mode = G_Cod1GscFeature47(C1CP47_F_MODE);
    return G_Cod1UseSelectionStable(player) && G_Cod1UsePlayerEligible(player) &&
        G_Cod1GscTestUseSelectionModeCurrent(mode);
}

static int G_Cod1UseSelectionReadyRun(gentity_t *player) {
    if (!cod1UseSelection) return 1;
    if (G_Cod1UseSelectionCurrent(player)) return 1;
    cod1UseSelection->rejected = 1;
    return 0;
}

static int G_Cod1UseTraceValid(const trace_t *trace) {
    if (!trace || !(trace->fraction >= 0.0f && trace->fraction <= 1.0f) ||
        trace->entityNum < 0 || trace->entityNum >= MAX_GENTITIES ||
        !G_Cod1UseFiniteVector(trace->endpos) ||
        !G_Cod1UseFiniteVector(trace->plane.normal)) return 0;
    if (trace->entityNum == ENTITYNUM_WORLD || trace->entityNum == ENTITYNUM_NONE) return 1;
    return cod1UseSelection && trace->entityNum < cod1UseSelection->count &&
        cod1UseSelection->refs[trace->entityNum].epoch &&
        G_Cod1EntitySameAllocation(&cod1UseSelection->refs[trace->entityNum]);
}

static int G_Cod1UseLegacyHit(const trace_t *trace) {
    const gentity_t *hit;
    if (trace->entityNum == ENTITYNUM_WORLD || trace->entityNum == ENTITYNUM_NONE ||
        (!(trace->fraction < 1.0f) && !trace->startsolid && !trace->allsolid)) return 0;
    hit = &g_entities[trace->entityNum];
    if (!hit->inuse) return 0;
    /* Native items are ranked with USE, including an unavailable full-health
     * kit. Such a kit must not recover direct-hit priority over the winner. */
    if (hit->s.eType == ET_ITEM && hit->item &&
        (G_Cod1NativeHealthItem(hit->item) ||
         (hit->item->giType == IT_WEAPON && CW_WeaponDef(hit->item->giTag)))) return 0;
    if (hit->classname && (!strcmp(hit->classname, "trigger_use") ||
        !strcmp(hit->classname, "trigger_multiple"))) return 0;
    return 1;
}

static void G_Cod1UseDiagnostic(const char *reason, const trace_t *first,
                              int candidates, int selected) {
    const char *classname = "<none>";
    int model = 0;
    uint64_t epoch = G_Cod1EntityMapEpoch();
    if (cod1UseDiagnosticEpoch != epoch) {
        cod1UseDiagnosticEpoch = epoch;
        cod1UseDiagnosticCount = 0;
    }
    if (cod1UseDiagnosticCount >= 15) return;
    ++cod1UseDiagnosticCount;
    if (first && cod1UseSelection && first->entityNum >= 0 &&
        first->entityNum < cod1UseSelection->count &&
        cod1UseSelection->refs[first->entityNum].epoch &&
        G_Cod1EntitySameAllocation(&cod1UseSelection->refs[first->entityNum])) {
        classname = g_entities[first->entityNum].classname;
        model = g_entities[first->entityNum].s.modelindex;
    }
    G_Printf("[CoD1 R90 USE] %s first=%d class=%.64s inline=%d fraction=%.6g startsolid=%d allsolid=%d flags=%d candidates=%d selected=%d\n",
        reason, first ? first->entityNum : ENTITYNUM_NONE,
        classname ? classname : "<null>", model, first ? first->fraction : 1.0f,
        first ? first->startsolid : 0, first ? first->allsolid : 0,
        first ? first->surfaceFlags : 0, candidates, selected);
}

static int G_Cod1UseSelectRun(gentity_t *player, qboolean hint) {
    Cod1UseSelection selection;
    trace_t first, bestTrace;
    vec3_t forward, right, up, eye, end;
    gentity_t *item = NULL;
    const char *reason = "refused: stale selection";
    float bestScore = 1.0e30f, bestDistance = 1.0e30f, itemScore;
    int i, axis, best = ENTITYNUM_NONE, candidates = 0, result = 1;
    int retainHint = 0, bestIsItem = 0;
    /* A trap may synchronously reenter. It cannot replace the outer borrowed
     * snapshot, clear its hint, or produce a second activation. */
    if (cod1UseSelection) return 1;
    if (!G_Cod1GscTestUseSelectionActive()) return 0;
    memset(&selection, 0, sizeof(selection));
    selection.watchedSlot = -1;
    memset(&first, 0, sizeof(first));
    first.fraction = 1.0f;
    first.entityNum = ENTITYNUM_NONE;
    selection.count = level.num_entities;
    if (selection.count < 0 || selection.count > MAX_GENTITIES ||
        !G_Cod1EntityCapture(player, &selection.player)) return 1;
    for (i = 0; i < selection.count; ++i)
        G_Cod1EntityCapture(&g_entities[i], &selection.refs[i]);
    cod1UseSelection = &selection;
    if (hint && G_Cod1UsePlayerCurrent(&selection, player)) {
        player->client->ps.serverCursorHint = HINT_NONE;
        player->client->ps.serverCursorHintVal = 0;
    }
    if (!G_Cod1UseSelectionCurrent(player)) goto done;
    /* The retained command tail owns dismounting and throwing carried props. */
    if (player->active || player->melee || player->client->ps.persistant[PERS_HWEAPON_USE]) {
        reason = "legacy: mounted or carried";
        result = 0;
        goto done;
    }
    selection.checkGameplay = 1;
    AngleVectors(player->client->ps.viewangles, forward, right, up);
    CalcMuzzlePointForActivate(player, forward, right, up, eye);
    VectorMA(eye, 96.0f, forward, end);
    if (!G_Cod1UseFiniteVector(eye) || !G_Cod1UseFiniteVector(end)) {
        reason = "refused: nonfinite ray";
        goto done;
    }
    VectorCopy(eye, selection.eye);
    VectorCopy(player->client->ps.viewangles, selection.viewangles);
    selection.geometryReady = 1;
    trap_Trace(&first, eye, NULL, NULL, end, player->s.number,
        CONTENTS_SOLID | CONTENTS_BODY | CONTENTS_CORPSE | CONTENTS_TRIGGER);
    if (!G_Cod1UseSelectionCurrent(player)) goto done;
    if (!G_Cod1UseTraceValid(&first)) {
        reason = "refused: malformed first trace";
        goto done;
    }
    /* Native item visibility queries borrow the same allocation snapshot.
     * Do not activate an item before comparing the original USE bonus. */
    item = G_Cod1AimedPickupCandidate(player, &itemScore);
    /* Publish the returned geometry before Current performs cvar callbacks:
     * those callbacks must not move an item between its LOS and watch setup. */
    if (!G_Cod1UseSelectionStable(player)) goto done;
    if (item) {
        vec3_t center, direction;
        if (item->s.number < 0 || item->s.number >= selection.count ||
            item != &g_entities[item->s.number] ||
            !selection.refs[item->s.number].epoch ||
            !G_Cod1EntitySameAllocation(&selection.refs[item->s.number]) ||
            !(itemScore >= 0.0f && itemScore <= 1.0e30f)) {
            reason = "refused: malformed pickup candidate";
            goto done;
        }
        for (axis = 0; axis < 3; ++axis)
            center[axis] = 0.5f * item->r.absmin[axis] + 0.5f * item->r.absmax[axis];
        VectorSubtract(center, eye, direction);
        bestDistance = VectorLength(direction);
        if (!G_Cod1UseFiniteVector(center) || !G_Cod1UseFiniteVector(direction) ||
            !(bestDistance > 0.0f && bestDistance <= 72.0f)) {
            reason = "refused: malformed pickup center";
            goto done;
        }
        best = item->s.number;
        bestScore = itemScore;
        bestIsItem = 1;
        G_Cod1UseWatchBounds(&selection, item);
    }
    if (!G_Cod1UseSelectionCurrent(player)) goto done;
    for (i = 0; i < selection.count; ++i) {
        gentity_t *target = &g_entities[i];
        trace_t targetTrace;
        vec3_t center, direction, boundsMin, boundsMax;
        float distance, cosine, score;
        if (!selection.refs[i].epoch || !G_Cod1TriggerUseEligible(target)) continue;
        if (!G_Cod1UseFiniteVector(target->r.absmin) ||
            !G_Cod1UseFiniteVector(target->r.absmax)) {
            reason = "refused: nonfinite use bounds";
            goto done;
        }
        for (axis = 0; axis < 3; ++axis) {
            if (target->r.absmin[axis] > target->r.absmax[axis]) {
                reason = "refused: inverted use bounds";
                goto done;
            }
        }
        VectorCopy(target->r.absmin, boundsMin);
        VectorCopy(target->r.absmax, boundsMax);
        for (axis = 0; axis < 3; ++axis)
            center[axis] = 0.5f * boundsMin[axis] + 0.5f * boundsMax[axis];
        VectorSubtract(center, eye, direction);
        if (!G_Cod1UseFiniteVector(center) || !G_Cod1UseFiniteVector(direction)) {
            reason = "refused: nonfinite use center";
            goto done;
        }
        distance = VectorNormalize(direction);
        if (!(distance > 0.0f && distance <= 72.0f)) continue;
        cosine = DotProduct(direction, forward);
        if (!(cosine >= 0.76f && cosine <= 1.000001f)) continue;
        score = distance + 144.0f * (1.0f - (cosine - 0.76f) / 0.24f) - 144.0f;
        ++candidates;
        /* Retail's integer score comparator leaves sub-unit ties unspecified.
         * Keep an exact-score lower-slot tie for deterministic port behavior. */
        if (score > bestScore || (score == bestScore && i >= best)) continue;
        trap_Trace(&targetTrace, eye, NULL, NULL, center, player->s.number,
            CONTENTS_SOLID | CONTENTS_BODY | CONTENTS_CORPSE);
        if (!G_Cod1UseSelectionCurrent(player)) goto done;
        if (!G_Cod1TriggerUseEligible(target) || !G_Cod1UseTraceValid(&targetTrace)) {
            reason = "refused: changed or malformed use trace";
            goto done;
        }
        for (axis = 0; axis < 3; ++axis)
            if (target->r.absmin[axis] != boundsMin[axis] || target->r.absmax[axis] != boundsMax[axis]) {
                reason = "refused: moved use candidate";
                goto done;
            }
        if ((targetTrace.startsolid || targetTrace.allsolid || targetTrace.fraction < 1.0f) &&
            targetTrace.entityNum != i) continue;
        best = i;
        bestScore = score;
        bestDistance = distance;
        bestIsItem = 0;
        G_Cod1UseWatchBounds(&selection, target);
        bestTrace = targetTrace;
        /* A clear center LOS has ENTITYNUM_NONE; retain the selected entity
         * explicitly for the existing hint trace/diagnostic consumer. */
        bestTrace.entityNum = i;
    }
    if (best == ENTITYNUM_NONE) {
        reason = "refused: no visible native candidate";
        if (G_Cod1UseLegacyHit(&first)) result = 0;
        goto done;
    }
    if (G_Cod1UseLegacyHit(&first) &&
        (first.startsolid || first.allsolid || 96.0f * first.fraction <= bestDistance)) {
        reason = "legacy: direct target priority";
        result = 0;
        goto done;
    }
    if (!G_Cod1UseSelectionCurrent(player)) goto done;
    if (bestIsItem) {
        retainHint = G_Cod1ApplyAimedPickupCandidate(player, &g_entities[best], hint);
        reason = retainHint ? "selected: native pickup" : "refused: unavailable pickup";
        goto done;
    }
    if (!G_Cod1TriggerUseEligible(&g_entities[best])) goto done;
    if (hint) {
        player->client->ps.serverCursorHint = HINT_ACTIVATE;
        {int scriptHint=G_Cod1HintIndex(&g_entities[best]);player->client->ps.serverCursorHintVal = scriptHint?scriptHint:COD1_HINT_USE_VALUE;}
        player->client->ps.serverCursorHintTrace = bestTrace;
    } else {
        G_Cod1UseDiagnostic("selected", &first, candidates, best);
        if (!G_Cod1UseSelectionCurrent(player) || !G_Cod1TriggerUseEligible(&g_entities[best])) goto done;
        g_entities[best].use(&g_entities[best], player, player);
    }
    cod1UseSelection = NULL;
    return 1;
done:
    if (!hint) G_Cod1UseDiagnostic(reason, &first, candidates, best);
    /* Reentry from diagnostic output cannot preserve a stale legacy fallthrough. */
    if (!G_Cod1UseSelectionStable(player) ||
        (result == 0 && !G_Cod1UseSelectionCurrent(player))) result = 1;
    if (hint && result != 0 && !retainHint && G_Cod1UsePlayerCurrent(&selection, player)) {
        player->client->ps.serverCursorHint = HINT_NONE;
        player->client->ps.serverCursorHintVal = 0;
    }
    cod1UseSelection = NULL;
    return result;
}
