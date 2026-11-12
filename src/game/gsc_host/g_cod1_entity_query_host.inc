/* R149 GAME adapter: capture one ready map/session and retain both presence
 * and full allocation identity across guarded engine gate reads. */
static int gscTestEntityNumberBusy,gscTestEntityNumberReentered;
static int GscTestEntityNumberOverlap(const void *a,size_t as,const void *b,size_t bs) {
    uintptr_t aa=(uintptr_t)a,bb=(uintptr_t)b;
    if(!a||!b||!as||!bs)return 0;
    return aa<=bb?bb-aa<as:aa-bb<bs;
}
static int GscTestEntityNumberSame(GscSession *session,uint64_t id,uint64_t epoch,
    int now,int hostTime,int entityCount) {
    GscSessionSnapshot view;
    return !gscTestEntityNumberReentered&&session&&session==gscTestSession&&
        id&&id==gscTestId&&epoch&&epoch==gscTestMapEpoch&&epoch==G_Cod1EntityMapEpoch()&&
        (gscTestEntityMode==37||gscTestEntityMode==38||gscTestEntityMode==39||(gscTestEntityMode==40||(gscTestEntityMode==41||(gscTestEntityMode==42||gscTestEntityMode==43))))&&!gscTestStopRequested&&!gscTestCompleted&&!level.restarted&&
        now>=0&&level.time==hostTime&&entityCount==level.num_entities&&
        G_Cod1EntityCollectionReady(epoch,entityCount)&&
        gsc_session_snapshot(session,&view)&&view.session_id==id&&!view.error[0]&&
        view.state==GSC_SESSION_RUNNING&&view.logical_ms==now;
}
static int GscTestEntityNumberCurrent(GscSession *session,uint64_t id,uint64_t epoch,
    int now,int hostTime,int entityCount) {
    return GscTestEntityNumberSame(session,id,epoch,now,hostTime,entityCount)&&
        GscTestModuleReadCurrent()&&
        GscTestEntityNumberSame(session,id,epoch,now,hostTime,entityCount);
}
static int GscTestEntityByNumber(void *user,uint64_t id,int now,int32_t slot,
    GscEntityRef *out,int *found,char *error,size_t size) {
    GscSession *session;uint64_t epoch;GscEntityRef first,second;
    int firstFound=-1,secondFound=-1,hostTime,entityCount,accepted=0;
    (void)user;
    if(GscTestEntityNumberOverlap(out,sizeof(*out),error,size)||
       GscTestEntityNumberOverlap(found,sizeof(*found),error,size))return 0;
    if(gscTestEntityNumberBusy) {
        gscTestEntityNumberReentered=1;
        GscTestError(error,size,"entity number query reentered");return 0;
    }
    if(!out||!found||(uint32_t)slot>=2048||
       GscTestEntityNumberOverlap(out,sizeof(*out),found,sizeof(*found))) {
        GscTestError(error,size,"entity number query requires bounded slot and separate outputs");return 0;
    }
    session=gscTestSession;epoch=G_Cod1EntityMapEpoch();hostTime=level.time;entityCount=level.num_entities;
    gscTestEntityNumberBusy=1;gscTestEntityNumberReentered=0;
    memset(&first,0,sizeof(first));memset(&second,0,sizeof(second));
    if(!GscTestEntityNumberCurrent(session,id,epoch,now,hostTime,entityCount)||
       !G_Cod1EntityByNumber(slot,&first,&firstFound,NULL,0)||
       !GscTestEntityNumberCurrent(session,id,epoch,now,hostTime,entityCount)||
       !G_Cod1EntityByNumber(slot,&second,&secondFound,NULL,0)||
       firstFound!=secondFound||memcmp(&first,&second,sizeof(first))||
       !GscTestEntityNumberSame(session,id,epoch,now,hostTime,entityCount))goto end;
    *out=first;*found=firstFound;accepted=1;
end:
    gscTestEntityNumberBusy=0;gscTestEntityNumberReentered=0;
    if(!accepted)GscTestError(error,size,"entity number query refused: changed map, session, time or allocation");
    return accepted;
}
