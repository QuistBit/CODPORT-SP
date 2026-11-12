/* R114 map-owned earthquake scene. The reliable single-player command is a
 * paired port transport, not retail PVS/entity-snapshot timing. A completed or
 * disabled VM does not erase already accepted quakes in this game scene. Only
 * true bootstrap, shutdown or save-load clears the owned marker. */
#include "../bg_cod1_script_quake.h"
typedef struct {
    GscSession *session;
    uint64_t id, epoch, map_epoch, incarnation, sequence;
    int owned, marker_ready, mode;
} GscTestQuakeScene;
static GscTestQuakeScene gscTestQuake;

static int GscTestQuakeIdentity(const GscTestQuakeScene *lease) {
    return lease && lease->owned && lease->epoch &&
        lease->epoch!=UINT64_MAX && lease->incarnation &&
        lease->incarnation!=UINT64_MAX && gscTestQuake.owned &&
        lease->epoch==gscTestQuake.epoch &&
        lease->incarnation==gscTestQuake.incarnation &&
        lease->map_epoch && lease->map_epoch==gscTestQuake.map_epoch &&
        lease->map_epoch==G_Cod1EntityMapEpoch() &&
        lease->session && lease->session==gscTestQuake.session &&
        lease->session==gscTestSession && lease->id &&
        lease->id==gscTestQuake.id && lease->id==gscTestId;
}
static int GscTestQuakeCurrent(const GscTestQuakeScene *lease) {
    return GscTestQuakeIdentity(lease) && gscTestEntityMode==lease->mode && (lease->mode==21 || lease->mode==22 || lease->mode==23 || lease->mode==24 || lease->mode==25 || (lease->mode==26 || lease->mode==27 || (lease->mode==28 || (lease->mode==29 || (lease->mode==30 || (lease->mode==31 || (lease->mode==32 || (lease->mode==33 || (lease->mode==34 || lease->mode==35 || lease->mode==36 || lease->mode==37 || lease->mode==38 || lease->mode==39 || (lease->mode==40||(lease->mode==41||(lease->mode==42||lease->mode==43)))))))))))) &&
        !gscTestCompleted && !gscTestStopRequested && !level.restarted &&
        G_Cod1MapEntities() && g_gametype.integer==GT_SINGLE_PLAYER;
}
static int GscTestQuakeTimeCurrent(const GscTestQuakeScene *lease,int logical_ms) {
    GscSessionSnapshot snapshot;
    return GscTestQuakeCurrent(lease) && logical_ms>=0 &&
        gsc_session_snapshot(lease->session,&snapshot) &&
        snapshot.state==GSC_SESSION_RUNNING && snapshot.session_id==lease->id &&
        snapshot.logical_ms==logical_ms;
}
/* Each external query is bounded by the copied lease. The older module gate
 * deliberately retains its historic read order; this producer may stop before
 * any subsequent read after a callback invalidates its particular owner. */
static int GscTestQuakeAdmission(const GscTestQuakeScene *lease) {
    static const char *names[6]={"g_cod1GscTest","g_cod1GscEntityTest",
        "savegame_loading","g_cod1GscTest","savegame_loading",
        "g_cod1GscEntityTest"};
    int values[6]={1,0,0,1,0,0};
    int n,value;
    values[1]=values[5]=lease->mode;
    for(n=0;n<6;++n) {
        if(!GscTestQuakeCurrent(lease)) return 0;
        value=trap_Cvar_VariableIntegerValue(names[n]);
        if(!GscTestQuakeCurrent(lease) || value!=values[n]) return 0;
    }
    return 1;
}
static int GscTestQuakeResetCurrent(uint64_t epoch,uint64_t incarnation,
    uint64_t map_epoch) {
    return !gscTestQuake.owned && epoch==gscTestQuake.epoch &&
        incarnation==gscTestQuake.incarnation && map_epoch &&
        map_epoch==G_Cod1EntityMapEpoch();
}
static void GscTestQuakeReset(void) {
    GscTestQuakeScene old=gscTestQuake;
    uint64_t epoch=old.epoch==UINT64_MAX?UINT64_MAX:old.epoch+1;
    uint64_t incarnation=old.incarnation==UINT64_MAX?UINT64_MAX:old.incarnation+1;
    char marker[COD1_SCRIPT_QUAKE_METADATA_CAPACITY],actual[MAX_STRING_CHARS];
    /* Invalidate before the first trap. A recursive reset sees no owned marker;
     * an abandoned older reset never clears a replacement scene. */
    memset(&gscTestQuake,0,sizeof(gscTestQuake));
    gscTestQuake.epoch=epoch;gscTestQuake.incarnation=incarnation;
    if(!old.owned || !old.marker_ready ||
        !CQ_EncodeMetadata(marker,sizeof(marker),old.epoch) ||
        !GscTestQuakeResetCurrent(epoch,incarnation,old.map_epoch)) return;
    memset(actual,0,sizeof(actual));
    trap_GetConfigstring(CS_COD1_SCRIPT_EFFECTS,actual,sizeof(actual));
    if(!GscTestQuakeResetCurrent(epoch,incarnation,old.map_epoch) ||
        !memchr(actual,0,sizeof(actual)) || strcmp(actual,marker)) return;
    memset(actual,0,sizeof(actual));
    trap_GetConfigstring(CS_COD1_SCRIPT_EFFECTS,actual,sizeof(actual));
    if(!GscTestQuakeResetCurrent(epoch,incarnation,old.map_epoch) ||
        !memchr(actual,0,sizeof(actual)) || strcmp(actual,marker)) return;
    trap_SetConfigstring(CS_COD1_SCRIPT_EFFECTS,"");
    /* No old-owner writes or compensating rollback after this effect. */
}
static int GscTestQuakeBegin(char *error,size_t capacity) {
    GscTestQuakeScene lease;
    char marker[COD1_SCRIPT_QUAKE_METADATA_CAPACITY],actual[MAX_STRING_CHARS];
    if(gscTestQuake.owned || !gscTestQuake.epoch ||
        gscTestQuake.epoch==UINT64_MAX || !gscTestQuake.incarnation ||
        gscTestQuake.incarnation==UINT64_MAX) goto refuse;
    gscTestQuake.session=gscTestSession;gscTestQuake.id=gscTestId;
    gscTestQuake.map_epoch=G_Cod1EntityMapEpoch();gscTestQuake.owned=1;gscTestQuake.mode=gscTestEntityMode;
    lease=gscTestQuake;
    if(!GscTestQuakeAdmission(&lease) ||
        !CQ_EncodeMetadata(marker,sizeof(marker),lease.epoch)) goto refuse;
    memset(actual,0,sizeof(actual));
    trap_GetConfigstring(CS_COD1_SCRIPT_EFFECTS,actual,sizeof(actual));
    if(!GscTestQuakeCurrent(&lease) || !memchr(actual,0,sizeof(actual)) ||
        actual[0]) goto refuse; /* Never overwrite a foreign namespace. */
    /* Bounded confirmation catches a namespace change during the previous
     * query. These reads are not an atomic snapshot of arbitrary callbacks. */
    memset(actual,0,sizeof(actual));
    trap_GetConfigstring(CS_COD1_SCRIPT_EFFECTS,actual,sizeof(actual));
    if(!GscTestQuakeCurrent(&lease) || !memchr(actual,0,sizeof(actual)) ||
        actual[0] || !GscTestQuakeAdmission(&lease)) goto refuse;
    gscTestQuake.marker_ready=1;
    trap_SetConfigstring(CS_COD1_SCRIPT_EFFECTS,marker);
    if(!GscTestQuakeCurrent(&lease)) goto refuse;
    memset(actual,0,sizeof(actual));
    trap_GetConfigstring(CS_COD1_SCRIPT_EFFECTS,actual,sizeof(actual));
    if(!GscTestQuakeCurrent(&lease) || !memchr(actual,0,sizeof(actual)) ||
        strcmp(actual,marker) || !GscTestQuakeAdmission(&lease)) goto refuse;
    return 1;
refuse:
    GscTestError(error,capacity,"quake scene unavailable, namespace occupied or map/session changed");
    return 0;
}
static int GscTestQuakePlayerCurrent(const GscEntityRef *player) {
    return player && player->slot==0 && G_Cod1EntitySameAllocation(player) &&
        level.clients && g_entities[0].client==&level.clients[0] &&
        g_entities[0].client->pers.connected==CON_CONNECTED &&
        !g_entities[0].aiCharacter && !g_entities[0].client->ps.aiChar &&
        !(g_entities[0].r.svFlags&SVF_CASTAI) &&
        g_entities[0].client->sess.sessionTeam!=TEAM_SPECTATOR;
}
static int GscTestQuakeEmit(void *user,uint64_t id,int logical_ms,
    const GscQuakeEvent *event,char *error,size_t capacity) {
    GscQuakeEvent copy;
    GscTestQuakeScene lease=gscTestQuake;
    GscEntityRef player;
    cod1ScriptQuake_t quake;
    char marker[COD1_SCRIPT_QUAKE_METADATA_CAPACITY],actual[MAX_STRING_CHARS];
    char command[COD1_SCRIPT_QUAKE_COMMAND_CAPACITY];
    (void)user;
    if(!event) goto refuse;
    copy=*event; /* No borrowed parameters survive the first engine callback. */
    if(logical_ms<0 || id!=lease.id || !lease.marker_ready ||
        lease.sequence==UINT64_MAX || !GscTestQuakeTimeCurrent(&lease,logical_ms) ||
        !G_Cod1EntityCapture(&g_entities[0],&player) ||
        !GscTestQuakePlayerCurrent(&player)) goto refuse;
    memset(&quake,0,sizeof(quake));quake.epoch=lease.epoch;
    quake.sequence=lease.sequence+1;quake.duration_ms=copy.duration_ms;
    quake.scale=copy.scale;quake.radius=copy.radius;
    memcpy(quake.origin,copy.origin,sizeof(quake.origin));
    if(!CQ_EncodeCommand(command,sizeof(command),&quake) ||
        !CQ_EncodeMetadata(marker,sizeof(marker),lease.epoch) ||
        !GscTestQuakeAdmission(&lease) || !GscTestQuakeTimeCurrent(&lease,logical_ms) ||
        !GscTestQuakePlayerCurrent(&player)) goto refuse;
    memset(actual,0,sizeof(actual));
    trap_GetConfigstring(CS_COD1_SCRIPT_EFFECTS,actual,sizeof(actual));
    if(!GscTestQuakeTimeCurrent(&lease,logical_ms) || !GscTestQuakePlayerCurrent(&player) ||
        !memchr(actual,0,sizeof(actual)) || strcmp(actual,marker) ||
        gscTestQuake.sequence!=lease.sequence || !GscTestQuakeAdmission(&lease) ||
        !GscTestQuakeTimeCurrent(&lease,logical_ms) || !GscTestQuakePlayerCurrent(&player)) goto refuse;
    memset(actual,0,sizeof(actual));
    trap_GetConfigstring(CS_COD1_SCRIPT_EFFECTS,actual,sizeof(actual));
    if(!GscTestQuakeTimeCurrent(&lease,logical_ms) || !GscTestQuakePlayerCurrent(&player) ||
        !memchr(actual,0,sizeof(actual)) || strcmp(actual,marker) ||
        gscTestQuake.sequence!=lease.sequence || !GscTestQuakeAdmission(&lease) ||
        !GscTestQuakeTimeCurrent(&lease,logical_ms) || !GscTestQuakePlayerCurrent(&player)) goto refuse;
    /* Consume before sending. A later refusal cannot replay committed effects
     * or reset the sequence of the same surviving map-owned scene. */
    gscTestQuake.sequence=quake.sequence;
    trap_SendServerCommand(0,command);
    if(!GscTestQuakeTimeCurrent(&lease,logical_ms) || !GscTestQuakePlayerCurrent(&player) ||
        gscTestQuake.sequence!=quake.sequence) goto refuse;
    memset(actual,0,sizeof(actual));
    trap_GetConfigstring(CS_COD1_SCRIPT_EFFECTS,actual,sizeof(actual));
    if(!GscTestQuakeTimeCurrent(&lease,logical_ms) || !GscTestQuakePlayerCurrent(&player) ||
        !memchr(actual,0,sizeof(actual)) || strcmp(actual,marker) ||
        gscTestQuake.sequence!=quake.sequence || !GscTestQuakeAdmission(&lease) ||
        !GscTestQuakeTimeCurrent(&lease,logical_ms) || !GscTestQuakePlayerCurrent(&player)) goto refuse;
    return 1;
refuse:
    GscTestError(error,capacity,"quake event invalid, recipient unavailable or map/session changed");
    return 0;
}
