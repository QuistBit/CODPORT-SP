/* R135 actual GAME-to-CL/CG copied query bridge. Cache effects are owned by
 * CGAME; refusal after QUERY never refreshes/rolls back that cache. */
static int gscTestKeyBindingBusy,gscTestKeyBindingReentered;
static int GscTestKeyBindingSame(GscSession *session,uint64_t id,uint64_t epoch,int now,int hostTime) {
    GscSessionSnapshot view;
    return !gscTestKeyBindingReentered&&session&&session==gscTestSession&&id&&id==gscTestId&&epoch&&
        epoch==gscTestMapEpoch&&epoch==G_Cod1EntityMapEpoch()&&(gscTestEntityMode==34 || gscTestEntityMode==35 || gscTestEntityMode==36 || gscTestEntityMode==37 || gscTestEntityMode==38 || gscTestEntityMode==39 || (gscTestEntityMode==40||(gscTestEntityMode==41||(gscTestEntityMode==42||gscTestEntityMode==43))))&&
        !gscTestStopRequested&&!gscTestCompleted&&!level.restarted&&now>=0&&level.time==hostTime&&
        G_Cod1MapEntities()&&g_gametype.integer==GT_SINGLE_PLAYER&&
        gsc_session_snapshot(session,&view)&&view.session_id==id&&!view.error[0]&&
        view.state==GSC_SESSION_RUNNING&&view.logical_ms==now;
}
static int GscTestKeyBindingQuery(void *user,uint64_t id,int now,GscKeyBindingRequest *query,char *error,size_t capacity) {
    GscKeyBindingRequest before,result;GscSession *session;uint64_t epoch;int hostTime,accepted;
    (void)user;
    if(!keybinding_no_alias(query,error,capacity))return 0;
    if(gscTestKeyBindingBusy){gscTestKeyBindingReentered=1;GscTestError(error,capacity,"getkeybinding query reentry refused");return 0;}
    gscTestKeyBindingBusy=1;gscTestKeyBindingReentered=0;
    session=gscTestSession;epoch=G_Cod1EntityMapEpoch();hostTime=level.time;
    if(!keybinding_request_valid(query))goto refuse;
    before=*query;result=before;
    if(!GscTestKeyBindingSame(session,id,epoch,now,hostTime)||!GscTestModuleReadCurrent()||
       !GscTestKeyBindingSame(session,id,epoch,now,hostTime)||!keybinding_filled_same(&before,query))goto refuse;
    accepted=trap_Cod1KeyBindingQuery(&result);
    if(!GscTestKeyBindingSame(session,id,epoch,now,hostTime)||accepted!=1||
       !keybinding_filled_same(&before,query)||!keybinding_reply_valid(&before,&result))goto refuse;
    /* Cvar/map/session fences can themselves invoke external callbacks. Check
     * them before an immutable phase2 CG lease validation; never re-query. */
    if(!GscTestModuleReadCurrent()||!GscTestKeyBindingSame(session,id,epoch,now,hostTime)||
       !keybinding_filled_same(&before,query))goto refuse;
    {GscKeyBindingRequest lease=result,validated;lease.phase=COD1_KEYBINDING_PHASE_VALIDATE;validated=lease;
     if(trap_Cod1KeyBindingQuery(&validated)!=1||!GscTestKeyBindingSame(session,id,epoch,now,hostTime)||
        !keybinding_filled_same(&lease,&validated)||!keybinding_filled_same(&before,query))goto refuse;}
    *query=result;gscTestKeyBindingBusy=0;return 1;
refuse:
    gscTestKeyBindingBusy=0;GscTestError(error,capacity,"getkeybinding query invalid, stale or cancelled outside current fresh GAME owner");return 0;
}
