/* R118 map-owned print3d publication. Success means this game host invoked
 * the actual reliable engine command for connected SP slot zero and survived
 * ownership checks. It is not a client display acknowledgment. */
#include "../bg_cod1_debug_text.h"
typedef struct {
    GscSession *session;
    uint64_t id,map_epoch,epoch,incarnation,operation_id;
    int owned,marker_ready,failed,mode;
} GscTestDebugTextScene;
static GscTestDebugTextScene gscTestDebugText;
/* Separate fence survives a recursive scene reset during an engine trap. */
static int gscTestDebugTextBusy;
static int GscTestDebugTextIdentity(const GscTestDebugTextScene *lease) {
    return lease && lease->owned && gscTestDebugText.owned && lease->epoch &&
        lease->epoch!=UINT64_MAX && lease->epoch==gscTestDebugText.epoch &&
        lease->incarnation && lease->incarnation!=UINT64_MAX &&
        lease->incarnation==gscTestDebugText.incarnation && lease->map_epoch &&
        lease->map_epoch==gscTestDebugText.map_epoch &&
        lease->map_epoch==G_Cod1EntityMapEpoch() && lease->session &&
        lease->session==gscTestDebugText.session && lease->session==gscTestSession &&
        lease->id && lease->id==gscTestDebugText.id && lease->id==gscTestId;
}
static int GscTestDebugTextCurrent(const GscTestDebugTextScene *lease) {
    GscSessionSnapshot snapshot;
    return GscTestDebugTextIdentity(lease) && !gscTestDebugText.failed &&
        gscTestEntityMode==lease->mode && (lease->mode==25 || (lease->mode==26 || lease->mode==27 || (lease->mode==28 || (lease->mode==29 || (lease->mode==30 || (lease->mode==31 || (lease->mode==32 || (lease->mode==33 || (lease->mode==34 || lease->mode==35 || lease->mode==36 || lease->mode==37 || lease->mode==38 || lease->mode==39 || (lease->mode==40||(lease->mode==41||(lease->mode==42||lease->mode==43)))))))))))) && !gscTestCompleted && !gscTestStopRequested &&
        !level.restarted && G_Cod1MapEntities() && g_gametype.integer==GT_SINGLE_PLAYER &&
        gsc_session_snapshot(lease->session,&snapshot) && snapshot.session_id==lease->id &&
        !snapshot.error[0] && (snapshot.state==GSC_SESSION_PREPARED || snapshot.state==GSC_SESSION_RUNNING);
}
static int GscTestDebugTextTime(const GscTestDebugTextScene *lease,int ms) {
    GscSessionSnapshot snapshot;
    return ms>=0 && GscTestDebugTextCurrent(lease) &&
        gsc_session_snapshot(lease->session,&snapshot) && snapshot.session_id==lease->id &&
        snapshot.state==GSC_SESSION_RUNNING && !snapshot.error[0] && snapshot.logical_ms==ms;
}
static int GscTestDebugTextAdmission(const GscTestDebugTextScene *lease,int ms,int running) {
    static const unsigned keys[6]={C1CP47_F_ENABLE,C1CP47_F_MODE,C1CP47_K_SAVE,
        C1CP47_F_ENABLE,C1CP47_K_SAVE,C1CP47_F_MODE};
    int values[6]={1,0,0,1,0,0};int i,value;
    values[1]=values[5]=lease->mode;
    for(i=0;i<6;++i) {
        if(!(running?GscTestDebugTextTime(lease,ms):GscTestDebugTextCurrent(lease))) return 0;
        value=GscConfigInteger47(keys[i]);
        if(!(running?GscTestDebugTextTime(lease,ms):GscTestDebugTextCurrent(lease)) || value!=values[i]) return 0;
    }
    return 1;
}
static int GscTestDebugTextMarker(const GscTestDebugTextScene *lease,int ms) {
    char expected[23],actual[MAX_STRING_CHARS];int i;
    if(!lease->marker_ready || !CTD_EncodeMetadata(expected,sizeof(expected),lease->epoch)) return 0;
    for(i=0;i<2;++i) {
        if(!GscTestDebugTextAdmission(lease,ms,1)) return 0;
        memset(actual,0,sizeof(actual));trap_GetConfigstring(CS_COD1_DEBUG_TEXT_METADATA,actual,sizeof(actual));
        if(!GscTestDebugTextAdmission(lease,ms,1) || !memchr(actual,0,sizeof(actual)) || strcmp(actual,expected)) return 0;
    }
    return 1;
}
static int GscTestDebugTextResetCurrent(uint64_t epoch,uint64_t incarnation,uint64_t map_epoch) {
    return !gscTestDebugText.owned && gscTestDebugText.epoch==epoch &&
        gscTestDebugText.incarnation==incarnation && map_epoch && map_epoch==G_Cod1EntityMapEpoch();
}
static void GscTestDebugTextReset(void) {
    GscTestDebugTextScene old=gscTestDebugText;
    uint64_t epoch=old.epoch==UINT64_MAX?UINT64_MAX:old.epoch+1;
    uint64_t incarnation=old.incarnation==UINT64_MAX?UINT64_MAX:old.incarnation+1;
    char expected[23],actual[MAX_STRING_CHARS];int i;
    memset(&gscTestDebugText,0,sizeof(gscTestDebugText));
    gscTestDebugText.epoch=epoch;gscTestDebugText.incarnation=incarnation;
    if(!old.owned || !old.marker_ready || !CTD_EncodeMetadata(expected,sizeof(expected),old.epoch)) return;
    for(i=0;i<2;++i) {
        if(!GscTestDebugTextResetCurrent(epoch,incarnation,old.map_epoch)) return;
        memset(actual,0,sizeof(actual));trap_GetConfigstring(CS_COD1_DEBUG_TEXT_METADATA,actual,sizeof(actual));
        if(!GscTestDebugTextResetCurrent(epoch,incarnation,old.map_epoch) ||
           !memchr(actual,0,sizeof(actual)) || strcmp(actual,expected)) return;
    }
    trap_SetConfigstring(CS_COD1_DEBUG_TEXT_METADATA,"");
}
static int GscTestDebugTextBegin(char *error,size_t capacity) {
    GscTestDebugTextScene lease;char expected[23],actual[MAX_STRING_CHARS];int i;
    if(gscTestDebugText.owned || !gscTestDebugText.epoch || gscTestDebugText.epoch==UINT64_MAX ||
       !gscTestDebugText.incarnation || gscTestDebugText.incarnation==UINT64_MAX) goto refuse;
    gscTestDebugText.session=gscTestSession;gscTestDebugText.id=gscTestId;
    gscTestDebugText.map_epoch=G_Cod1EntityMapEpoch();gscTestDebugText.mode=gscTestEntityMode;
    gscTestDebugText.owned=1;lease=gscTestDebugText;
    if(!CTD_EncodeMetadata(expected,sizeof(expected),lease.epoch)) goto refuse;
    for(i=0;i<2;++i) {
        if(!GscTestDebugTextAdmission(&lease,0,0)) goto refuse;
        memset(actual,0,sizeof(actual));trap_GetConfigstring(CS_COD1_DEBUG_TEXT_METADATA,actual,sizeof(actual));
        if(!GscTestDebugTextAdmission(&lease,0,0) || !memchr(actual,0,sizeof(actual)) || actual[0]) goto refuse;
    }
    gscTestDebugText.marker_ready=1;trap_SetConfigstring(CS_COD1_DEBUG_TEXT_METADATA,expected);
    if(!GscTestDebugTextCurrent(&lease)) goto refuse;
    memset(actual,0,sizeof(actual));trap_GetConfigstring(CS_COD1_DEBUG_TEXT_METADATA,actual,sizeof(actual));
    if(!GscTestDebugTextAdmission(&lease,0,0) || !memchr(actual,0,sizeof(actual)) || strcmp(actual,expected)) goto refuse;
    return 1;
refuse:
    GscTestError(error,capacity,"debug text scene unavailable, occupied or map/session changed");return 0;
}
static int GscTestDebugTextPublish(void *user,uint64_t id,int logical_ms,
    const GscDebugTextEvent *event,char *error,size_t capacity) {
    GscDebugTextEvent copy;GscTestDebugTextScene lease=gscTestDebugText;
    cod1DebugText_t wire;GscEntityRef player;char command[COD1_DEBUG_TEXT_COMMAND_CAPACITY];
    int accepted=0,server_time=level.time;
    (void)user;
    if(gscTestDebugTextBusy) {
        gscTestStopRequested=1;gscTestDebugText.failed=1;
        GscTestError(error,capacity,"print3d callback reentry refused");return 0;
    }
    if(!event) goto refuse;
    copy=*event;
    memset(&wire,0,sizeof(wire));wire.epoch=lease.epoch;wire.operation_id=copy.operation_id;
    wire.server_time=server_time;wire.length=copy.length;wire.scale=copy.scale;
    memcpy(wire.origin,copy.origin,sizeof(wire.origin));memcpy(wire.color,copy.color,sizeof(wire.color));
    memcpy(wire.text,copy.text,sizeof(wire.text));
    if(id!=lease.id || wire.operation_id<=lease.operation_id ||
       !CTD_EncodeCommand(command,sizeof(command),&wire) ||
       !GscTestDebugTextTime(&lease,logical_ms) || !G_Cod1EntityCapture(&g_entities[0],&player) ||
       !GscTestQuakePlayerCurrent(&player)) goto refuse;
    gscTestDebugTextBusy=1;
    if(!GscTestDebugTextMarker(&lease,logical_ms) || !GscTestQuakePlayerCurrent(&player) ||
       level.time!=server_time || gscTestDebugText.operation_id!=lease.operation_id ||
       !GscTestDebugTextMarker(&lease,logical_ms) || !GscTestQuakePlayerCurrent(&player) ||
       level.time!=server_time || gscTestDebugText.operation_id!=lease.operation_id) goto end;
    /* Consume before the irreversible command prefix; a failed post-check
     * never rewinds it or permits publication of the same token twice. */
    gscTestDebugText.operation_id=wire.operation_id;trap_SendServerCommand(0,command);
    if(!GscTestDebugTextMarker(&lease,logical_ms) || !GscTestQuakePlayerCurrent(&player) ||
       level.time!=server_time || gscTestDebugText.operation_id!=wire.operation_id) goto end;
    accepted=1;
end:
    gscTestDebugTextBusy=0;
    if(accepted) return 1;
refuse:
    GscTestError(error,capacity,"print3d invalid/stale payload, recipient unavailable or map/session changed");return 0;
}
