/* R119 map-owned line publication. Success means this game host invoked
 * the actual reliable engine command for connected SP slot zero and survived
 * ownership checks. It is not a client display acknowledgment. */
#include "../bg_cod1_debug_line.h"
typedef struct {
    GscSession *session;
    uint64_t id,map_epoch,epoch,incarnation,operation_id;
    int owned,marker_ready,failed,mode;
} GscTestDebugLineScene;
static GscTestDebugLineScene gscTestDebugLine;
/* Separate fence survives a recursive scene reset during an engine trap. */
static int gscTestDebugLineBusy;
static int GscTestDebugLineIdentity(const GscTestDebugLineScene *lease) {
    return lease && lease->owned && gscTestDebugLine.owned && lease->epoch &&
        lease->epoch!=UINT64_MAX && lease->epoch==gscTestDebugLine.epoch &&
        lease->incarnation && lease->incarnation!=UINT64_MAX &&
        lease->incarnation==gscTestDebugLine.incarnation && lease->map_epoch &&
        lease->map_epoch==gscTestDebugLine.map_epoch &&
        lease->map_epoch==G_Cod1EntityMapEpoch() && lease->session &&
        lease->session==gscTestDebugLine.session && lease->session==gscTestSession &&
        lease->id && lease->id==gscTestDebugLine.id && lease->id==gscTestId;
}
static int GscTestDebugLineCurrent(const GscTestDebugLineScene *lease) {
    GscSessionSnapshot snapshot;
    return GscTestDebugLineIdentity(lease) && !gscTestDebugLine.failed &&
        gscTestEntityMode==lease->mode && (lease->mode==26 || lease->mode==27 || (lease->mode==28 || (lease->mode==29 || (lease->mode==30 || (lease->mode==31 || (lease->mode==32 || (lease->mode==33 || (lease->mode==34 || lease->mode==35 || lease->mode==36 || lease->mode==37 || lease->mode==38 || lease->mode==39 || (lease->mode==40||(lease->mode==41||(lease->mode==42||lease->mode==43))))))))))) && !gscTestCompleted && !gscTestStopRequested &&
        !level.restarted && G_Cod1MapEntities() && g_gametype.integer==GT_SINGLE_PLAYER &&
        gsc_session_snapshot(lease->session,&snapshot) && snapshot.session_id==lease->id &&
        !snapshot.error[0] && (snapshot.state==GSC_SESSION_PREPARED || snapshot.state==GSC_SESSION_RUNNING);
}
static int GscTestDebugLineTime(const GscTestDebugLineScene *lease,int ms) {
    GscSessionSnapshot snapshot;
    return ms>=0 && GscTestDebugLineCurrent(lease) &&
        gsc_session_snapshot(lease->session,&snapshot) && snapshot.session_id==lease->id &&
        snapshot.state==GSC_SESSION_RUNNING && !snapshot.error[0] && snapshot.logical_ms==ms;
}
static int GscTestDebugLineAdmission(const GscTestDebugLineScene *lease,int ms,int running) {
    static const char *names[6]={"g_cod1GscTest","g_cod1GscEntityTest","savegame_loading",
        "g_cod1GscTest","savegame_loading","g_cod1GscEntityTest"};
    int values[6]={1,0,0,1,0,0};int i,value;
    values[1]=values[5]=lease->mode;
    for(i=0;i<6;++i) {
        if(!(running?GscTestDebugLineTime(lease,ms):GscTestDebugLineCurrent(lease))) return 0;
        value=trap_Cvar_VariableIntegerValue(names[i]);
        if(!(running?GscTestDebugLineTime(lease,ms):GscTestDebugLineCurrent(lease)) || value!=values[i]) return 0;
    }
    return 1;
}
static int GscTestDebugLineMarker(const GscTestDebugLineScene *lease,int ms) {
    char expected[23],actual[MAX_STRING_CHARS];int i;
    if(!lease->marker_ready || !CDL_EncodeMetadata(expected,sizeof(expected),lease->epoch)) return 0;
    for(i=0;i<2;++i) {
        if(!GscTestDebugLineAdmission(lease,ms,1)) return 0;
        memset(actual,0,sizeof(actual));trap_GetConfigstring(CS_COD1_DEBUG_LINE_METADATA,actual,sizeof(actual));
        if(!GscTestDebugLineAdmission(lease,ms,1) || !memchr(actual,0,sizeof(actual)) || strcmp(actual,expected)) return 0;
    }
    return 1;
}
static int GscTestDebugLineResetCurrent(uint64_t epoch,uint64_t incarnation,uint64_t map_epoch) {
    return !gscTestDebugLine.owned && gscTestDebugLine.epoch==epoch &&
        gscTestDebugLine.incarnation==incarnation && map_epoch && map_epoch==G_Cod1EntityMapEpoch();
}
static void GscTestDebugLineReset(void) {
    GscTestDebugLineScene old=gscTestDebugLine;
    uint64_t epoch=old.epoch==UINT64_MAX?UINT64_MAX:old.epoch+1;
    uint64_t incarnation=old.incarnation==UINT64_MAX?UINT64_MAX:old.incarnation+1;
    char expected[23],actual[MAX_STRING_CHARS];int i;
    memset(&gscTestDebugLine,0,sizeof(gscTestDebugLine));
    gscTestDebugLine.epoch=epoch;gscTestDebugLine.incarnation=incarnation;
    if(!old.owned || !old.marker_ready || !CDL_EncodeMetadata(expected,sizeof(expected),old.epoch)) return;
    for(i=0;i<2;++i) {
        if(!GscTestDebugLineResetCurrent(epoch,incarnation,old.map_epoch)) return;
        memset(actual,0,sizeof(actual));trap_GetConfigstring(CS_COD1_DEBUG_LINE_METADATA,actual,sizeof(actual));
        if(!GscTestDebugLineResetCurrent(epoch,incarnation,old.map_epoch) ||
           !memchr(actual,0,sizeof(actual)) || strcmp(actual,expected)) return;
    }
    trap_SetConfigstring(CS_COD1_DEBUG_LINE_METADATA,"");
}
static int GscTestDebugLineBegin(char *error,size_t capacity) {
    GscTestDebugLineScene lease;char expected[23],actual[MAX_STRING_CHARS];int i;
    if(gscTestDebugLine.owned || !gscTestDebugLine.epoch || gscTestDebugLine.epoch==UINT64_MAX ||
       !gscTestDebugLine.incarnation || gscTestDebugLine.incarnation==UINT64_MAX) goto refuse;
    gscTestDebugLine.session=gscTestSession;gscTestDebugLine.id=gscTestId;
    gscTestDebugLine.map_epoch=G_Cod1EntityMapEpoch();gscTestDebugLine.mode=gscTestEntityMode;
    gscTestDebugLine.owned=1;lease=gscTestDebugLine;
    if(!CDL_EncodeMetadata(expected,sizeof(expected),lease.epoch)) goto refuse;
    for(i=0;i<2;++i) {
        if(!GscTestDebugLineAdmission(&lease,0,0)) goto refuse;
        memset(actual,0,sizeof(actual));trap_GetConfigstring(CS_COD1_DEBUG_LINE_METADATA,actual,sizeof(actual));
        if(!GscTestDebugLineAdmission(&lease,0,0) || !memchr(actual,0,sizeof(actual)) || actual[0]) goto refuse;
    }
    gscTestDebugLine.marker_ready=1;trap_SetConfigstring(CS_COD1_DEBUG_LINE_METADATA,expected);
    if(!GscTestDebugLineCurrent(&lease)) goto refuse;
    memset(actual,0,sizeof(actual));trap_GetConfigstring(CS_COD1_DEBUG_LINE_METADATA,actual,sizeof(actual));
    if(!GscTestDebugLineAdmission(&lease,0,0) || !memchr(actual,0,sizeof(actual)) || strcmp(actual,expected)) goto refuse;
    return 1;
refuse:
    GscTestError(error,capacity,"debug line scene unavailable, occupied or map/session changed");return 0;
}
static int GscTestDebugLinePublish(void *user,uint64_t id,int logical_ms,
    const GscDebugLineEvent *event,char *error,size_t capacity) {
    GscDebugLineEvent copy;GscTestDebugLineScene lease=gscTestDebugLine;
    cod1DebugLine_t wire;GscEntityRef player;char command[COD1_DEBUG_LINE_COMMAND_CAPACITY];
    int accepted=0,queue_refused=0,server_time=level.time;
    (void)user;
    if(gscTestDebugLineBusy) {
        gscTestStopRequested=1;gscTestDebugLine.failed=1;
        GscTestError(error,capacity,"line callback reentry refused");return 0;
    }
    if(!event) goto refuse;
    copy=*event;
    memset(&wire,0,sizeof(wire));wire.epoch=lease.epoch;wire.operation_id=copy.operation_id;
    wire.server_time=server_time;wire.optional_integer=copy.optional_integer;
    memcpy(wire.start,copy.start,sizeof(wire.start));memcpy(wire.end,copy.end,sizeof(wire.end));
    memcpy(wire.color,copy.color,sizeof(wire.color));
    /* Raw optional integer is retained in the structural transport, but its
     * engine meaning is unavailable. Refuse nonzero before consuming a token
     * or emitting an apparent successful effect that ignores this parameter. */
    if(copy.optional_integer!=0 || id!=lease.id || wire.operation_id<=lease.operation_id ||
       !CDL_EncodeCommand(command,sizeof(command),&wire) ||
       !GscTestDebugLineTime(&lease,logical_ms) || !G_Cod1EntityCapture(&g_entities[0],&player) ||
       !GscTestQuakePlayerCurrent(&player)) goto refuse;
    gscTestDebugLineBusy=1;
    if(!GscTestDebugLineMarker(&lease,logical_ms) || !GscTestQuakePlayerCurrent(&player) ||
       level.time!=server_time || gscTestDebugLine.operation_id!=lease.operation_id ||
       !GscTestDebugLineMarker(&lease,logical_ms) || !GscTestQuakePlayerCurrent(&player) ||
       level.time!=server_time || gscTestDebugLine.operation_id!=lease.operation_id) goto end;
    /* Consume before the irreversible command prefix; a failed post-check
     * never rewinds it or permits publication of the same token twice. */
    gscTestDebugLine.operation_id=wire.operation_id;
    if(!trap_Cod1TrySendServerCommand(0,command)) {
        queue_refused=1;goto end;
    }
    if(!GscTestDebugLineMarker(&lease,logical_ms) || !GscTestQuakePlayerCurrent(&player) ||
       level.time!=server_time || gscTestDebugLine.operation_id!=wire.operation_id) goto end;
    accepted=1;
end:
    gscTestDebugLineBusy=0;
    if(accepted) return 1;
    if(queue_refused) {
        GscTestError(error,capacity,"line reliable queue admission refused");return 0;
    }
refuse:
    GscTestError(error,capacity,"line invalid/stale payload, recipient unavailable or map/session changed");return 0;
}
