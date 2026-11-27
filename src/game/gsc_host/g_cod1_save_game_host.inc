/* R123 genuine GAME request boundary. Original wrapper and engine parser
 * retain a pending request, latest accepted request wins before consumption.
 * This private opt-in queue copies all metadata and cancels on map/session
 * retirement. Frame flush submits EXEC_APPEND only. Existing RTCW save24
 * consumer still does not serialize this GSC VM or consume argv2/argv3; no
 * file-writing/restoration success is reported by this lane. */
#include "../cod1_gsc/gsc_save_game_payload.inc"
typedef struct {
    GscSession *session;
    uint64_t id,map_epoch,generation,operation;
    int owned,phase,logical_ms,server_time,health,pending,mode;
    GscSaveGameEvent event;
} GscTestSaveScene;
static GscTestSaveScene gscTestSave;
static int gscTestSaveBusy;
static void GscTestSaveReset(void) {
    uint64_t generation=gscTestSave.generation;
    memset(&gscTestSave,0,sizeof(gscTestSave));
    gscTestSave.generation=generation==UINT64_MAX?UINT64_MAX:generation+1;
}
static int GscTestSaveCurrent(const GscTestSaveScene *lease,int logical_ms,int completed) {
    GscSessionSnapshot snapshot;
    return lease && lease->owned && gscTestSave.owned && lease->generation && lease->generation!=UINT64_MAX &&
        lease->generation==gscTestSave.generation && lease->session && lease->session==gscTestSave.session &&
        lease->session==gscTestSession && lease->id && lease->id==gscTestSave.id && lease->id==gscTestId &&
        lease->map_epoch && lease->map_epoch==gscTestSave.map_epoch && lease->map_epoch==G_Cod1EntityMapEpoch() &&
        (lease->mode==29 || (lease->mode==30 || (lease->mode==31 || (lease->mode==32 || (lease->mode==33 || (lease->mode==34 || lease->mode==35 || lease->mode==36 || lease->mode==37 || lease->mode==38 || lease->mode==39 || (lease->mode==40||(lease->mode==41||(lease->mode==42||lease->mode==43))))))))) && lease->mode==gscTestSave.mode &&
        gscTestEntityMode==lease->mode && !gscTestStopRequested && !level.restarted && G_Cod1MapEntities() &&
        g_gametype.integer==GT_SINGLE_PLAYER && (completed || !gscTestCompleted) &&
        gsc_session_snapshot(lease->session,&snapshot) && snapshot.session_id==lease->id && !snapshot.error[0] &&
        snapshot.logical_ms==logical_ms && (snapshot.state==GSC_SESSION_RUNNING ||
            (completed && snapshot.state==GSC_SESSION_COMPLETED));
}
static int GscTestSaveBegin(char *error,size_t capacity) {
    GscSessionSnapshot snapshot;
    if(gscTestSaveBusy || gscTestSave.owned || !gscTestSave.generation || gscTestSave.generation==UINT64_MAX ||
       !gscTestSession || !gscTestId || (gscTestEntityMode!=29 && (gscTestEntityMode!=30 && (gscTestEntityMode!=31 && (gscTestEntityMode!=32 && (gscTestEntityMode!=33 && (gscTestEntityMode!=34 && (gscTestEntityMode!=35 && (gscTestEntityMode!=36 && (gscTestEntityMode!=37 && (gscTestEntityMode!=38 && (gscTestEntityMode!=39 && (gscTestEntityMode!=40&&(gscTestEntityMode!=41&&(gscTestEntityMode!=42&&gscTestEntityMode!=43)))))))))))))) || gscTestStopRequested || level.restarted ||
       !G_Cod1MapEntities() || g_gametype.integer!=GT_SINGLE_PLAYER ||
       !gsc_session_snapshot(gscTestSession,&snapshot) || snapshot.session_id!=gscTestId ||
       snapshot.state!=GSC_SESSION_PREPARED || snapshot.error[0]) {
        GscTestError(error,capacity,"saveGame scene unavailable or map/session changed");return 0;
    }
    gscTestSave.session=gscTestSession;gscTestSave.id=gscTestId;
    gscTestSave.mode=gscTestEntityMode;
    gscTestSave.map_epoch=G_Cod1EntityMapEpoch();
    if(!gscTestSave.map_epoch)return 0;
    gscTestSave.owned=1;return 1;
}
/* Six sequential gate observations, not an atomic snapshot of arbitrary
 * unrelated cvar mutations after their final read. Owner/time fences are
 * checked around each engine service. */
static int GscTestSaveAdmission(const GscTestSaveScene *lease,int logical_ms,int completed,int server_time) {
    static const unsigned keys[6]={C1CP47_F_ENABLE,C1CP47_F_MODE,C1CP47_K_SAVE,C1CP47_F_ENABLE,C1CP47_K_SAVE,C1CP47_F_MODE};
    int values[6]={1,lease->mode,0,1,0,lease->mode};int i,value;
    for(i=0;i<6;i++) {
        if(!GscTestSaveCurrent(lease,logical_ms,completed)||level.time!=server_time)return 0;
        value=GscConfigInteger47(keys[i]);
        if(!GscTestSaveCurrent(lease,logical_ms,completed)||level.time!=server_time||value!=values[i])return 0;
    }
    return 1;
}
static int GscTestSaveQuery(void *user,uint64_t id,int logical_ms,
    GscSaveGameQuery *query,char *error,size_t capacity) {
    GscTestSaveScene lease;GscSaveGameQuery copy;vmCvar_t cvar;int accepted=0,server_time;
    (void)user;
    if(gscTestSaveBusy){gscTestStopRequested=1;goto refuse;}
    if(!query)goto refuse;
    copy=*query;lease=gscTestSave;server_time=level.time;
    if(!copy.operation_id||copy.operation_id==UINT64_MAX||id!=lease.id||
       !GscTestSaveCurrent(&lease,logical_ms,0))goto refuse;
    gscTestSaveBusy=1;
    if(copy.kind==GSC_SAVE_GAME_PLAYER_HEALTH) {
        if(copy.operation_id<=gscTestSave.operation)goto leave;
        gscTestSave.operation=copy.operation_id;gscTestSave.phase=1;
        gscTestSave.logical_ms=logical_ms;gscTestSave.server_time=server_time;
        copy.player_health=g_entities[0].health;gscTestSave.health=copy.player_health;
        /* Original dead-player gate makes no cvar/getter/queue call. */
        accepted=GscTestSaveCurrent(&lease,logical_ms,0)&&level.time==server_time;
    } else {
        if(copy.operation_id!=gscTestSave.operation||logical_ms!=gscTestSave.logical_ms||
           server_time!=gscTestSave.server_time||gscTestSave.health<=0||
           !((copy.kind==GSC_SAVE_GAME_MAPNAME&&gscTestSave.phase==1)||
             (copy.kind==GSC_SAVE_GAME_CAMPAIGN&&gscTestSave.phase==2)))goto leave;
        gscTestSave.phase=copy.kind==GSC_SAVE_GAME_MAPNAME?2:3;
        if(!GscTestSaveAdmission(&lease,logical_ms,0,server_time))goto leave;
        memset(&cvar,0,sizeof(cvar));
        if(copy.kind==GSC_SAVE_GAME_MAPNAME)trap_Cvar_Register(&cvar,"mapname","",0x44);
        else trap_Cvar_Register(&cvar,"ui_campaign","american",1);
        if(!GscTestSaveAdmission(&lease,logical_ms,0,server_time)||
           !memchr(cvar.string,0,sizeof(cvar.string))||strlen(cvar.string)>=sizeof(copy.text))goto leave;
        strcpy(copy.text,cvar.string);accepted=1;
    }
leave:
    gscTestSaveBusy=0;
    if(accepted){*query=copy;return 1;}
refuse:
    GscTestError(error,capacity,"saveGame query refused: stale owner/time, order, cvar or payload domain");return 0;
}
static int GscTestSaveCommand(const GscSaveGameEvent *event,char *out,size_t capacity) {
    const unsigned char *s;int n;
    if(!save_game_event_valid(event)||strncmp(event->filename,"autosave\\",9))return 0;
    /* Selected command transport domain: no filename token splitting,
     * dot-stripping ambiguity or quoted/control/semicolon command injection. */
    for(s=(const unsigned char *)event->filename;*s;s++)
        if(!((*s>='a'&&*s<='z')||(*s>='A'&&*s<='Z')||(*s>='0'&&*s<='9')||
             *s=='_'||*s=='-'||*s=='\\'))return 0;
    for(n=0;n<2;n++)for(s=(const unsigned char *)(n?event->image:event->description);*s;s++)
        if(*s<32||*s>126||*s=='"'||*s==';')return 0;
    n=snprintf(out,capacity,"savegame %s \"%s\" \"%s\"\n",event->filename,event->description,event->image);
    return n>=0&&(size_t)n<capacity;
}
static int GscTestSavePublish(void *user,uint64_t id,int logical_ms,
    const GscSaveGameEvent *event,char *error,size_t capacity) {
    GscTestSaveScene lease;GscSaveGameEvent copy;char command[384];int accepted=0;
    (void)user;
    if(gscTestSaveBusy){gscTestStopRequested=1;goto refuse;}
    if(!event)goto refuse;
    copy=*event;lease=gscTestSave;
    if(id!=lease.id||!GscTestSaveCurrent(&lease,logical_ms,0)||copy.operation_id!=gscTestSave.operation||
       logical_ms!=gscTestSave.logical_ms||level.time!=gscTestSave.server_time||gscTestSave.health<=0||
       (gscTestSave.phase!=2&&gscTestSave.phase!=3)||!GscTestSaveCommand(&copy,command,sizeof(command)))goto refuse;
    gscTestSaveBusy=1;
    if(GscTestSaveAdmission(&lease,logical_ms,0,lease.server_time)) {
        gscTestSave.event=copy;gscTestSave.pending=1;gscTestSave.phase=4;accepted=1;
    }
    gscTestSaveBusy=0;
    if(accepted)return 1;
refuse:
    GscTestError(error,capacity,"saveGame request refused: stale owner/time, order or command transport domain");return 0;
}
static int GscTestSaveFlush(char *error,size_t capacity) {
    GscTestSaveScene lease;GscSessionSnapshot snapshot;GscSaveGameEvent event;
    char command[384];int accepted=0,server_time;
    if(!gscTestSave.pending)return 1;
    if(gscTestSaveBusy){gscTestStopRequested=1;goto refuse;}
    lease=gscTestSave;event=lease.event;server_time=level.time;
    /* Check pointer/owner identity before asking the session to dereference
     * its opaque object. A replacement session cannot lend its lifetime. */
    if(!lease.owned||!lease.session||lease.session!=gscTestSession||lease.id!=gscTestId||
       !lease.id||!lease.map_epoch||lease.map_epoch!=G_Cod1EntityMapEpoch())goto refuse;
    if(!gsc_session_snapshot(lease.session,&snapshot)||snapshot.session_id!=lease.id||
       !GscTestSaveCommand(&event,command,sizeof(command)))goto refuse;
    gscTestSaveBusy=1;
    if(!GscTestSaveAdmission(&lease,snapshot.logical_ms,1,server_time))goto leave;
    /* Consume before the irreversible append. No retry or successful-save
     * claim follows a callback mutation; earlier append cannot be recalled. */
    gscTestSave.pending=0;
    trap_SendConsoleCommand(EXEC_APPEND,command);
    accepted=GscTestSaveAdmission(&lease,snapshot.logical_ms,1,server_time);
leave:
    gscTestSaveBusy=0;
    if(accepted)return 1;
refuse:
    GscTestError(error,capacity,"saveGame frame request dispatch cancelled");return 0;
}
