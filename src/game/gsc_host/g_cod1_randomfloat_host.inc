/* R134 synchronous typed GAME draw host. The existing R132 map owner is
 * the only RNG state and seed. Constructor/session/mode operations never seed.
 * External capability reads precede the draw; the final check/draw/publication
 * path has no callbacks. Save/load, legacy maps and stale Fresh owners refuse. */
static int gscTestRandomFloatBusy, gscTestRandomFloatReentered;

static int GscTestRandomFloatSame(GscSession *session,uint64_t id,uint64_t epoch,
    int now,int hostTime) {
    GscSessionSnapshot view;
    return !gscTestRandomFloatReentered && session && session==gscTestSession &&
        id && id==gscTestId && epoch && epoch==gscTestMapEpoch &&
        epoch==G_Cod1EntityMapEpoch() && (gscTestEntityMode==33 || (gscTestEntityMode==34 || gscTestEntityMode==35 || gscTestEntityMode==36 || gscTestEntityMode==37 || gscTestEntityMode==38 || gscTestEntityMode==39 || (gscTestEntityMode==40||(gscTestEntityMode==41||(gscTestEntityMode==42||gscTestEntityMode==43))))) &&
        !gscTestStopRequested && !gscTestCompleted && !level.restarted &&
        now>=0 && level.time==hostTime && G_Cod1MapEntities() &&
        g_gametype.integer==GT_SINGLE_PLAYER && G_Cod1RngOwnerReady() &&
        gsc_session_snapshot(session,&view) && view.session_id==id && !view.error[0] &&
        view.state==GSC_SESSION_RUNNING && view.logical_ms==now;
}

static int GscTestRandomFloatDraw(void *user,uint64_t id,int now,
    GscRandomFloatRequest *query,char *error,size_t capacity) {
    GscRandomFloatRequest before,result;GscSession *session;uint64_t epoch;
    uint32_t draw;int hostTime;uintptr_t queryAt,errorAt;
    (void)user;
    if(query && error && capacity) {
        queryAt=(uintptr_t)query;errorAt=(uintptr_t)error;
        if(errorAt>=queryAt?errorAt-queryAt<sizeof(*query):queryAt-errorAt<capacity)return 0;
    }
    if(gscTestRandomFloatBusy) {
        gscTestRandomFloatReentered=1;
        GscTestError(error,capacity,"randomfloat draw callback reentry refused");return 0;
    }
    gscTestRandomFloatBusy=1;gscTestRandomFloatReentered=0;
    session=gscTestSession;epoch=G_Cod1EntityMapEpoch();hostTime=level.time;
    if(!randomfloat_request_valid(query))goto refuse;
    before=*query;result=before;
    if(!GscTestRandomFloatSame(session,id,epoch,now,hostTime) ||
       !GscTestModuleReadCurrent() ||
       !GscTestRandomFloatSame(session,id,epoch,now,hostTime) ||
       query->operation_id!=before.operation_id || query->bound_bits!=before.bound_bits ||
       query->draw15!=before.draw15)goto refuse;
    /* No external callback follows the final owner check. Actual GAME state
     * commits once here; any later wrapper failure must retain that commit. */
    if(!G_Cod1RngDraw15(&draw))goto refuse;
    result.draw15=draw;
    if(!GscTestRandomFloatSame(session,id,epoch,now,hostTime))goto refuse;
    *query=result;gscTestRandomFloatBusy=0;return 1;
refuse:
    gscTestRandomFloatBusy=0;
    GscTestError(error,capacity,"randomfloat draw invalid or cancelled outside current fresh GAME owner");
    return 0;
}
