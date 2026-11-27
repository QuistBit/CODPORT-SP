/* R120 map-owned ambience publication. Success means this game host invoked
 * the actual reliable engine command for connected SP slot zero and survived
 * ownership checks. It is not a client display acknowledgment. */
#include "../bg_cod1_ambient.h"
typedef struct {
    GscSession *session;
    uint64_t id,map_epoch,epoch,incarnation,operation_id;
    uint32_t server_id;
    int owned,marker_ready,failed,mode,server_ready;
} GscTestAmbientScene;
static GscTestAmbientScene gscTestAmbient;
/* Separate fence survives a recursive scene reset during an engine trap. */
static int gscTestAmbientBusy;
static int GscTestAmbientIdentity(const GscTestAmbientScene *lease) {
    return lease && lease->owned && gscTestAmbient.owned && CA_Generation(lease->epoch) &&
        CA_Generation(lease->epoch)!=UINT32_MAX &&
        (lease->server_ready?CA_Epoch(lease->epoch):!CA_ModuleId(lease->epoch)) &&
        lease->epoch==gscTestAmbient.epoch &&
        lease->server_ready==gscTestAmbient.server_ready &&
        (!lease->server_ready || (lease->server_id==gscTestAmbient.server_id &&
          lease->server_id==CA_ModuleId(lease->epoch))) &&
        lease->incarnation && lease->incarnation!=UINT64_MAX &&
        lease->incarnation==gscTestAmbient.incarnation && lease->map_epoch &&
        lease->map_epoch==gscTestAmbient.map_epoch &&
        lease->map_epoch==G_Cod1EntityMapEpoch() && lease->session &&
        lease->session==gscTestAmbient.session && lease->session==gscTestSession &&
        lease->id && lease->id==gscTestAmbient.id && lease->id==gscTestId;
}
static int GscTestAmbientCurrent(const GscTestAmbientScene *lease) {
    GscSessionSnapshot snapshot;
    return GscTestAmbientIdentity(lease) && !gscTestAmbient.failed &&
        gscTestEntityMode==lease->mode && (lease->mode==27 || (lease->mode==28 || (lease->mode==29 || (lease->mode==30 || (lease->mode==31 || (lease->mode==32 || (lease->mode==33 || (lease->mode==34 || lease->mode==35 || lease->mode==36 || lease->mode==37 || lease->mode==38 || lease->mode==39 || (lease->mode==40||(lease->mode==41||(lease->mode==42||lease->mode==43))))))))))) && !gscTestCompleted && !gscTestStopRequested &&
        !level.restarted && G_Cod1MapEntities() && g_gametype.integer==GT_SINGLE_PLAYER &&
        gsc_session_snapshot(lease->session,&snapshot) && snapshot.session_id==lease->id &&
        !snapshot.error[0] && (snapshot.state==GSC_SESSION_PREPARED || snapshot.state==GSC_SESSION_RUNNING);
}
static int GscTestAmbientTime(const GscTestAmbientScene *lease,int ms) {
    GscSessionSnapshot snapshot;
    return ms>=0 && GscTestAmbientCurrent(lease) &&
        gsc_session_snapshot(lease->session,&snapshot) && snapshot.session_id==lease->id &&
        snapshot.state==GSC_SESSION_RUNNING && !snapshot.error[0] && snapshot.logical_ms==ms;
}
static int GscTestAmbientAdmission(const GscTestAmbientScene *lease,int ms,int running) {
    static const unsigned keys[6]={C1CP47_F_ENABLE,C1CP47_F_MODE,C1CP47_K_SAVE,
        C1CP47_F_ENABLE,C1CP47_K_SAVE,C1CP47_F_MODE};
    int values[6]={1,0,0,1,0,0};int i,value;
    values[1]=values[5]=lease->mode;
    for(i=0;i<6;++i) {
        if(!(running?GscTestAmbientTime(lease,ms):GscTestAmbientCurrent(lease))) return 0;
        value=GscConfigInteger47(keys[i]);
        if(!(running?GscTestAmbientTime(lease,ms):GscTestAmbientCurrent(lease)) || value!=values[i]) return 0;
        if(lease->server_ready && (i==0 || i==5)) {
            value=trap_Cvar_VariableIntegerValue(COD1_AMBIENT_MODULE_CVAR);
            if(!(running?GscTestAmbientTime(lease,ms):GscTestAmbientCurrent(lease)) ||
               (uint32_t)value!=lease->server_id) return 0;
        }
    }
    return 1;
}
static int GscTestAmbientMarker(const GscTestAmbientScene *lease,int ms) {
    char expected[23],actual[MAX_STRING_CHARS];int i;
    if(!lease->marker_ready || !CA_EncodeMetadata(expected,sizeof(expected),lease->epoch)) return 0;
    for(i=0;i<2;++i) {
        if(!GscTestAmbientAdmission(lease,ms,1)) return 0;
        memset(actual,0,sizeof(actual));trap_GetConfigstring(CS_COD1_AMBIENT_METADATA,actual,sizeof(actual));
        if(!GscTestAmbientAdmission(lease,ms,1) || !memchr(actual,0,sizeof(actual)) || strcmp(actual,expected)) return 0;
    }
    return 1;
}
static int GscTestAmbientResetCurrent(uint64_t epoch,uint64_t incarnation,uint64_t map_epoch) {
    return !gscTestAmbient.owned && gscTestAmbient.epoch==epoch &&
        gscTestAmbient.incarnation==incarnation && map_epoch && map_epoch==G_Cod1EntityMapEpoch();
}
static void GscTestAmbientReset(void) {
    GscTestAmbientScene old=gscTestAmbient;
    uint32_t generation=CA_Generation(old.epoch);
    uint64_t epoch=generation==UINT32_MAX?UINT32_MAX:(uint64_t)generation+1;
    uint64_t incarnation=old.incarnation==UINT64_MAX?UINT64_MAX:old.incarnation+1;
    char expected[23],actual[MAX_STRING_CHARS];int i;
    memset(&gscTestAmbient,0,sizeof(gscTestAmbient));
    gscTestAmbient.epoch=epoch;gscTestAmbient.incarnation=incarnation;
    if(!old.owned || !old.marker_ready || !CA_EncodeMetadata(expected,sizeof(expected),old.epoch)) return;
    for(i=0;i<2;++i) {
        if(!GscTestAmbientResetCurrent(epoch,incarnation,old.map_epoch)) return;
        memset(actual,0,sizeof(actual));trap_GetConfigstring(CS_COD1_AMBIENT_METADATA,actual,sizeof(actual));
        if(!GscTestAmbientResetCurrent(epoch,incarnation,old.map_epoch) ||
           !memchr(actual,0,sizeof(actual)) || strcmp(actual,expected)) return;
    }
    trap_SetConfigstring(CS_COD1_AMBIENT_METADATA,"");
}
static int GscTestAmbientBegin(char *error,size_t capacity) {
    GscTestAmbientScene lease;char expected[23],actual[MAX_STRING_CHARS];int i,server_id;
    if(gscTestAmbient.owned || !CA_Generation(gscTestAmbient.epoch) ||
       CA_Generation(gscTestAmbient.epoch)==UINT32_MAX ||
       !gscTestAmbient.incarnation || gscTestAmbient.incarnation==UINT64_MAX) goto refuse;
    gscTestAmbient.session=gscTestSession;gscTestAmbient.id=gscTestId;
    gscTestAmbient.map_epoch=G_Cod1EntityMapEpoch();gscTestAmbient.mode=gscTestEntityMode;
    gscTestAmbient.owned=1;lease=gscTestAmbient;
    if(!GscTestAmbientAdmission(&lease,0,0)) goto refuse;
    server_id=trap_Cvar_VariableIntegerValue(COD1_AMBIENT_MODULE_CVAR);
    if(!GscTestAmbientCurrent(&lease) || server_id<=0) goto refuse;
    gscTestAmbient.server_id=(uint32_t)server_id;gscTestAmbient.server_ready=1;
    gscTestAmbient.epoch=((uint64_t)(uint32_t)server_id<<32)|CA_Generation(lease.epoch);
    lease=gscTestAmbient;
    if(!GscTestAmbientAdmission(&lease,0,0)) goto refuse;
    if(!CA_EncodeMetadata(expected,sizeof(expected),lease.epoch)) goto refuse;
    for(i=0;i<2;++i) {
        if(!GscTestAmbientAdmission(&lease,0,0)) goto refuse;
        memset(actual,0,sizeof(actual));trap_GetConfigstring(CS_COD1_AMBIENT_METADATA,actual,sizeof(actual));
        if(!GscTestAmbientAdmission(&lease,0,0) || !memchr(actual,0,sizeof(actual)) || actual[0]) goto refuse;
    }
    gscTestAmbient.marker_ready=1;trap_SetConfigstring(CS_COD1_AMBIENT_METADATA,expected);
    if(!GscTestAmbientCurrent(&lease)) goto refuse;
    memset(actual,0,sizeof(actual));trap_GetConfigstring(CS_COD1_AMBIENT_METADATA,actual,sizeof(actual));
    if(!GscTestAmbientAdmission(&lease,0,0) || !memchr(actual,0,sizeof(actual)) || strcmp(actual,expected)) goto refuse;
    return 1;
refuse:
    GscTestError(error,capacity,"ambience scene unavailable, occupied or map/session changed");return 0;
}
typedef struct {
    GscTestAmbientScene lease;
    GscEntityRef player;
    int logical_ms,server_time;
} GscTestAmbientFilesLease;
static int GscTestAmbientFilesCurrent(const GscTestAmbientFilesLease *files) {
    return files && GscTestAmbientAdmission(&files->lease,files->logical_ms,1) &&
        GscTestQuakePlayerCurrent(&files->player) && level.time==files->server_time &&
        gscTestAmbient.operation_id==files->lease.operation_id;
}
static int GscTestAmbientList(void *user,char *list,int capacity) {
    GscTestAmbientFilesLease *files=(GscTestAmbientFilesLease *)user;int count;
    if(!GscTestAmbientFilesCurrent(files)) return -1;
    count=trap_FS_GetFileList("soundaliases",".csv",list,capacity);
    return GscTestAmbientFilesCurrent(files)?count:-1;
}
static int GscTestAmbientRead(void *user,const char *path,char **out,int *size) {
    GscTestAmbientFilesLease *files=(GscTestAmbientFilesLease *)user;
    fileHandle_t file=0;int length;char *data;
    *out=NULL;*size=0;
    if(!GscTestAmbientFilesCurrent(files)) return 0;
    length=trap_FS_FOpenFile(path,&file,FS_READ);
    if(!GscTestAmbientFilesCurrent(files) || !file || length<=0 || length>COD1_AMBIENT_TABLE_BYTES) {
        if(file) trap_FS_FCloseFile(file);return 0;
    }
    data=(char *)COD1_GSC_VFS_CALLOC((size_t)length+1,1);
    if(!data) {trap_FS_FCloseFile(file);return 0;}
    trap_FS_Read(data,length,file);
    if(!GscTestAmbientFilesCurrent(files)) {
        trap_FS_FCloseFile(file);COD1_GSC_VFS_FREE(data);return 0;
    }
    trap_FS_FCloseFile(file);
    if(!GscTestAmbientFilesCurrent(files) || memchr(data,0,(size_t)length)) {
        COD1_GSC_VFS_FREE(data);return 0;
    }
    *out=data;*size=length;return 1;
}
static void GscTestAmbientRelease(void *user,char *data) {(void)user;COD1_GSC_VFS_FREE(data);}
static int GscTestAmbientExists(void *user,const char *path) {
    GscTestAmbientFilesLease *files=(GscTestAmbientFilesLease *)user;
    fileHandle_t file=0;int length,current;
    if(!GscTestAmbientFilesCurrent(files)) return 0;
    length=trap_FS_FOpenFile(path,&file,FS_READ);
    current=GscTestAmbientFilesCurrent(files);
    if(file) trap_FS_FCloseFile(file);
    return current && file && length>0 && GscTestAmbientFilesCurrent(files);
}
static int GscTestAmbientPublish(void *user,uint64_t id,int logical_ms,
    const GscAmbientEvent *event,char *error,size_t capacity) {
    GscAmbientEvent copy;GscTestAmbientScene lease=gscTestAmbient;
    cod1Ambient_t wire;cod1AmbientFiles files;cod1AmbientPlan plan;
    GscTestAmbientFilesLease resource;char map[128],command[COD1_AMBIENT_COMMAND_CAPACITY];
    int accepted=0,server_time=level.time;
    (void)user;
    if(gscTestAmbientBusy) {
        gscTestStopRequested=1;gscTestAmbient.failed=1;
        GscTestError(error,capacity,"ambience callback reentry refused");return 0;
    }
    if(!event) goto refuse;
    copy=*event;
    if(copy.transition_ms<0 || server_time<0 || copy.transition_ms>INT_MAX-server_time ||
       !memchr(copy.alias,0,sizeof(copy.alias))) goto refuse;
    memset(&wire,0,sizeof(wire));wire.epoch=lease.epoch;wire.operation_id=copy.operation_id;
    wire.server_time=server_time;wire.deadline=server_time+copy.transition_ms;wire.kind=copy.kind;
    memcpy(wire.alias,copy.alias,sizeof(wire.alias));
    memset(&resource,0,sizeof(resource));resource.lease=lease;
    resource.logical_ms=logical_ms;resource.server_time=server_time;
    if(id!=lease.id || wire.operation_id<=lease.operation_id ||
       !CA_EncodeCommand(command,sizeof(command),&wire) ||
       !GscTestAmbientTime(&lease,logical_ms) ||
       !G_Cod1EntityCapture(&g_entities[0],&resource.player) ||
       !GscTestQuakePlayerCurrent(&resource.player)) goto refuse;
    gscTestAmbientBusy=1;
    if(!GscTestAmbientMarker(&lease,logical_ms) || !GscTestAmbientFilesCurrent(&resource)) goto end;
    if(wire.kind==COD1_AMBIENT_PLAY) {
        memset(map,0,sizeof(map));trap_Cvar_VariableStringBuffer("mapname",map,sizeof(map));
        if(!GscTestAmbientFilesCurrent(&resource) || !memchr(map,0,sizeof(map))) goto end;
        memset(&files,0,sizeof(files));files.user=&resource;files.list=GscTestAmbientList;
        files.read=GscTestAmbientRead;files.release=GscTestAmbientRelease;files.exists=GscTestAmbientExists;
        memset(&plan,0,sizeof(plan));
        if(!Cod1AmbientResolve(&files,wire.alias,map,&plan) || !GscTestAmbientFilesCurrent(&resource)) goto end;
    }
    if(!GscTestAmbientMarker(&lease,logical_ms) || !GscTestAmbientFilesCurrent(&resource) ||
       !GscTestAmbientMarker(&lease,logical_ms) || !GscTestAmbientFilesCurrent(&resource)) goto end;
    /* Consume before the actual reliable command prefix. A failed postcheck
     * does not rewind this token or acknowledge client/audio completion. */
    gscTestAmbient.operation_id=wire.operation_id;trap_SendServerCommand(0,command);
    if(!GscTestAmbientMarker(&lease,logical_ms) || !GscTestQuakePlayerCurrent(&resource.player) ||
       level.time!=server_time || gscTestAmbient.operation_id!=wire.operation_id) goto end;
    accepted=1;
end:
    gscTestAmbientBusy=0;
    if(accepted) return 1;
refuse:
    GscTestError(error,capacity,"ambience unsupported/stale payload, resource or recipient unavailable, or map/session changed");return 0;
}
