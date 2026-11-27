/* R115 typed FX index resources and a map-owned event scene. Configstrings and
 * reliable current-SP-slot0 delivery are paired port policy. They are neither
 * retail temp-entity/PVS timing nor acknowledgment of an original engine FX
 * renderer. Raw names retain their original spelling; identity is ASCII case
 * insensitive. A true scene boundary clears only this owner's exact records. */
#include "../bg_cod1_script_fx.h"
#include <math.h>
typedef struct {
    GscSession *session;
    uint64_t id, epoch, map_epoch, incarnation, sequence, revision;
    int owned, marker_ready, dirty, mode;
    char names[MAX_COD1_SCRIPT_FX][GSC_FX_NAME_CAPACITY];
} GscTestFxScene;
static GscTestFxScene gscTestFx;
static void GscTestTagFxReset(void);
static int GscTestLoopFxReset(void);

static int GscTestFxSceneSame(const GscTestFxScene *lease) {
    return lease && lease->owned && lease->epoch && lease->epoch!=UINT64_MAX &&
        lease->incarnation && lease->incarnation!=UINT64_MAX &&
        gscTestFx.owned && lease->epoch==gscTestFx.epoch &&
        lease->incarnation==gscTestFx.incarnation && lease->map_epoch &&
        lease->map_epoch==gscTestFx.map_epoch &&
        lease->map_epoch==G_Cod1EntityMapEpoch();
}
static int GscTestFxCurrent(const GscTestFxScene *lease) {
    return GscTestFxSceneSame(lease) && lease->session &&
        lease->session==gscTestFx.session && lease->session==gscTestSession &&
        lease->id && lease->id==gscTestFx.id && lease->id==gscTestId &&
        gscTestEntityMode==lease->mode && (lease->mode==22 || lease->mode==23 || lease->mode==24 || lease->mode==25 || (lease->mode==26 || lease->mode==27 || (lease->mode==28 || (lease->mode==29 || (lease->mode==30 || (lease->mode==31 || (lease->mode==32 || (lease->mode==33 || (lease->mode==34 || lease->mode==35 || lease->mode==36 || lease->mode==37 || lease->mode==38 || lease->mode==39 || (lease->mode==40||(lease->mode==41||(lease->mode==42||lease->mode==43)))))))))))) && !gscTestCompleted && !gscTestStopRequested &&
        !level.restarted && G_Cod1MapEntities() &&
        g_gametype.integer==GT_SINGLE_PLAYER;
}
static int GscTestFxTimeCurrent(const GscTestFxScene *lease,int logical_ms) {
    GscSessionSnapshot snapshot;
    return GscTestFxCurrent(lease) && logical_ms>=0 &&
        gsc_session_snapshot(lease->session,&snapshot) &&
        snapshot.state==GSC_SESSION_RUNNING && snapshot.session_id==lease->id &&
        snapshot.logical_ms==logical_ms;
}
static int GscTestFxAdmission(const GscTestFxScene *lease) {
    static const unsigned keys[6]={C1CP47_F_ENABLE,C1CP47_F_MODE,C1CP47_K_SAVE,
        C1CP47_F_ENABLE,C1CP47_K_SAVE,C1CP47_F_MODE};
    int values[6]={1,0,0,1,0,0};
    int n,value;
    values[1]=values[5]=lease->mode;
    for(n=0;n<6;++n) {
        if(!GscTestFxCurrent(lease)) return 0;
        value=GscConfigInteger47(keys[n]);
        if(!GscTestFxCurrent(lease) || value!=values[n]) return 0;
    }
    return 1;
}
static int GscTestFxRead(const GscTestFxScene *lease,int index,char *out,
    size_t capacity,int logical_ms,int running) {
    if(!out || !capacity || capacity>MAX_STRING_CHARS ||
        !(running?GscTestFxTimeCurrent(lease,logical_ms):GscTestFxCurrent(lease))) return 0;
    memset(out,0,capacity);trap_GetConfigstring(index,out,(int)capacity);
    return memchr(out,0,capacity) &&
        (running?GscTestFxTimeCurrent(lease,logical_ms):GscTestFxCurrent(lease));
}
static int GscTestFxMarker(const GscTestFxScene *lease,int logical_ms) {
    char marker[COD1_SCRIPT_FX_METADATA_CAPACITY],actual[MAX_STRING_CHARS];
    int n;
    if(!lease->marker_ready || !CF_EncodeMetadata(marker,sizeof(marker),lease->epoch)) return 0;
    for(n=0;n<2;++n) {
        if(!GscTestFxAdmission(lease) || !GscTestFxTimeCurrent(lease,logical_ms) ||
            !GscTestFxRead(lease,CS_COD1_SCRIPT_FX_METADATA,actual,sizeof(actual),logical_ms,1) ||
            strcmp(actual,marker) || !GscTestFxAdmission(lease) ||
            !GscTestFxTimeCurrent(lease,logical_ms)) return 0;
    }
    return 1;
}
static int GscTestFxResourceText(char *out,size_t capacity,uint64_t epoch,
    int index,const char *name) {
    cod1ScriptFxResource_t resource;
    if(!name || !memchr(name,0,GSC_FX_NAME_CAPACITY) ||
        strlen(name)>=sizeof(resource.name)) return 0;
    memset(&resource,0,sizeof(resource));resource.epoch=epoch;resource.index=index;
    memcpy(resource.name,name,strlen(name)+1);
    return CF_EncodeResource(out,capacity,resource.epoch,resource.index,resource.name);
}
static int GscTestFxResetCurrent(uint64_t epoch,uint64_t incarnation,
    uint64_t map_epoch) {
    return !gscTestFx.owned && epoch==gscTestFx.epoch &&
        incarnation==gscTestFx.incarnation && map_epoch &&
        map_epoch==G_Cod1EntityMapEpoch();
}
static int GscTestFxResetMarker(uint64_t epoch,uint64_t incarnation,
    uint64_t map_epoch,const char *marker) {
    char actual[MAX_STRING_CHARS];int n;
    for(n=0;n<2;++n) {
        if(!GscTestFxResetCurrent(epoch,incarnation,map_epoch)) return 0;
        memset(actual,0,sizeof(actual));trap_GetConfigstring(CS_COD1_SCRIPT_FX_METADATA,actual,sizeof(actual));
        if(!GscTestFxResetCurrent(epoch,incarnation,map_epoch) ||
            !memchr(actual,0,sizeof(actual)) || strcmp(actual,marker)) return 0;
    }
    return 1;
}
static void GscTestFxReset(void) {
    GscTestFxScene old=gscTestFx;
    uint64_t epoch=old.epoch==UINT64_MAX?UINT64_MAX:old.epoch+1;
    uint64_t incarnation=old.incarnation==UINT64_MAX?UINT64_MAX:old.incarnation+1;
    char marker[COD1_SCRIPT_FX_METADATA_CAPACITY],actual[MAX_STRING_CHARS],expected[MAX_STRING_CHARS];
    int i,n;
    if(!GscTestLoopFxReset()){gscTestStopRequested=1;return;}
    GscTestTagFxReset();
    memset(&gscTestFx,0,sizeof(gscTestFx));gscTestFx.epoch=epoch;gscTestFx.incarnation=incarnation;
    if(!old.owned || !old.marker_ready ||
        !CF_EncodeMetadata(marker,sizeof(marker),old.epoch)) return;
    /* Invalidation precedes traps. Every clear has two exact owned-record
     * reads, and a replacement owner makes the abandoned reset stop. These
     * bounded checks do not pretend separate engine calls are atomic. */
    for(n=0;n<2;++n) {
        if(!GscTestFxResetCurrent(epoch,incarnation,old.map_epoch)) return;
        memset(actual,0,sizeof(actual));trap_GetConfigstring(CS_COD1_SCRIPT_FX_METADATA,actual,sizeof(actual));
        if(!GscTestFxResetCurrent(epoch,incarnation,old.map_epoch) ||
            !memchr(actual,0,sizeof(actual)) || strcmp(actual,marker)) return;
    }
    for(i=1;i<MAX_COD1_SCRIPT_FX;++i) if(old.names[i][0]) {
        if(!GscTestFxResourceText(expected,sizeof(expected),old.epoch,i,old.names[i])) return;
        for(n=0;n<2;++n) {
            if(!GscTestFxResetCurrent(epoch,incarnation,old.map_epoch)) return;
            memset(actual,0,sizeof(actual));trap_GetConfigstring(CS_COD1_SCRIPT_FX_TABLE+i,actual,sizeof(actual));
            if(!GscTestFxResetCurrent(epoch,incarnation,old.map_epoch) ||
                !memchr(actual,0,sizeof(actual)) || strcmp(actual,expected)) break;
        }
        if(n==2) {
            if(!GscTestFxResetMarker(epoch,incarnation,old.map_epoch,marker)) return;
            trap_SetConfigstring(CS_COD1_SCRIPT_FX_TABLE+i,"");
            if(!GscTestFxResetCurrent(epoch,incarnation,old.map_epoch)) return;
        }
    }
    for(n=0;n<2;++n) {
        if(!GscTestFxResetCurrent(epoch,incarnation,old.map_epoch)) return;
        memset(actual,0,sizeof(actual));trap_GetConfigstring(CS_COD1_SCRIPT_FX_METADATA,actual,sizeof(actual));
        if(!GscTestFxResetCurrent(epoch,incarnation,old.map_epoch) ||
            !memchr(actual,0,sizeof(actual)) || strcmp(actual,marker)) return;
    }
    trap_SetConfigstring(CS_COD1_SCRIPT_FX_METADATA,"");
}
static int GscTestFxBegin(char *error,size_t capacity) {
    GscTestFxScene lease;char marker[COD1_SCRIPT_FX_METADATA_CAPACITY],actual[MAX_STRING_CHARS];
    int i,n;
    if(gscTestFx.owned || !gscTestFx.epoch || gscTestFx.epoch==UINT64_MAX ||
        !gscTestFx.incarnation || gscTestFx.incarnation==UINT64_MAX) goto refuse;
    gscTestFx.session=gscTestSession;gscTestFx.id=gscTestId;gscTestFx.mode=gscTestEntityMode;
    gscTestFx.map_epoch=G_Cod1EntityMapEpoch();gscTestFx.owned=1;lease=gscTestFx;
    if(!GscTestFxAdmission(&lease) || !CF_EncodeMetadata(marker,sizeof(marker),lease.epoch)) goto refuse;
    for(n=0;n<2;++n) {
        if(!GscTestFxRead(&lease,CS_COD1_SCRIPT_FX_METADATA,actual,sizeof(actual),0,0) || actual[0]) goto refuse;
        for(i=1;i<MAX_COD1_SCRIPT_FX;++i)
            if(!GscTestFxRead(&lease,CS_COD1_SCRIPT_FX_TABLE+i,actual,sizeof(actual),0,0) || actual[0]) goto refuse;
        if(!GscTestFxAdmission(&lease)) goto refuse;
    }
    gscTestFx.marker_ready=1;trap_SetConfigstring(CS_COD1_SCRIPT_FX_METADATA,marker);
    if(!GscTestFxCurrent(&lease) ||
        !GscTestFxRead(&lease,CS_COD1_SCRIPT_FX_METADATA,actual,sizeof(actual),0,0) ||
        strcmp(actual,marker) || !GscTestFxAdmission(&lease)) goto refuse;
    return 1;
refuse:
    GscTestError(error,capacity,"FX scene unavailable, namespace occupied or map/session changed");return 0;
}
static int GscTestFxLoad(void *user,uint64_t id,int logical_ms,
    const GscFxLoadEvent *event,int32_t *index_out,char *error,size_t capacity) {
    GscFxLoadEvent copy;GscTestFxScene lease=gscTestFx;
    char actual[MAX_STRING_CHARS],confirm[MAX_STRING_CHARS],expected[MAX_STRING_CHARS];
    int i,n;
    (void)user;
    if(!event || !index_out) goto refuse;
    copy=*event;
    if(id!=lease.id || (copy.initial_window!=0 && copy.initial_window!=1) ||
        !memchr(copy.name,0,sizeof(copy.name)) || !GscTestFxTimeCurrent(&lease,logical_ms) ||
        !GscTestFxMarker(&lease,logical_ms)) goto refuse;
    if(!copy.name[0]) {
        if(!copy.initial_window || !gscTestResourceInitialization) goto refuse;
        *index_out=0;return 1;
    }
    if(!CF_ValidateName(copy.name)) goto refuse;
    for(i=1;i<MAX_COD1_SCRIPT_FX;++i) {
        if(gscTestFx.revision!=lease.revision ||
            !GscTestFxRead(&lease,CS_COD1_SCRIPT_FX_TABLE+i,actual,sizeof(actual),logical_ms,1) ||
            !GscTestFxMarker(&lease,logical_ms)) goto refuse;
        if(!actual[0]) {
            if(gscTestFx.names[i][0]) goto refuse;
            break; /* The original table stops at its first empty entry. */
        }
        if(!gscTestFx.names[i][0] ||
            !GscTestFxResourceText(expected,sizeof(expected),lease.epoch,i,gscTestFx.names[i]) ||
            strcmp(actual,expected)) goto refuse;
        if(!Q_stricmp(gscTestFx.names[i],copy.name)) {
            if(!GscTestFxRead(&lease,CS_COD1_SCRIPT_FX_TABLE+i,confirm,sizeof(confirm),logical_ms,1) ||
                strcmp(confirm,actual) || !GscTestFxMarker(&lease,logical_ms) ||
                gscTestFx.revision!=lease.revision) goto refuse;
            *index_out=i;return 1; /* Existing entries remain legal after waits. */
        }
    }
    if(i==MAX_COD1_SCRIPT_FX || !copy.initial_window || !gscTestResourceInitialization ||
        lease.revision==UINT64_MAX ||
        !GscTestFxResourceText(expected,sizeof(expected),lease.epoch,i,copy.name)) goto refuse;
    for(n=0;n<2;++n) {
        if(gscTestFx.revision!=lease.revision || !GscTestFxMarker(&lease,logical_ms) ||
            !GscTestFxRead(&lease,CS_COD1_SCRIPT_FX_TABLE+i,confirm,sizeof(confirm),logical_ms,1) ||
            confirm[0] || gscTestFx.names[i][0] ||
            !copy.initial_window || !gscTestResourceInitialization) goto refuse;
    }
    /* Consume resource revision and record exact ownership before publication.
     * A cancelled callback never rewinds an already visible table effect. */
    gscTestFx.revision=++lease.revision;memcpy(gscTestFx.names[i],copy.name,sizeof(copy.name));
    trap_SetConfigstring(CS_COD1_SCRIPT_FX_TABLE+i,expected);
    if(GscTestFxSceneSame(&lease) && gscTestFx.revision==lease.revision &&
        !strcmp(gscTestFx.names[i],copy.name)) gscTestFx.dirty=1;
    if(!GscTestFxTimeCurrent(&lease,logical_ms) || gscTestFx.revision!=lease.revision ||
        !GscTestFxRead(&lease,CS_COD1_SCRIPT_FX_TABLE+i,confirm,sizeof(confirm),logical_ms,1) ||
        strcmp(confirm,expected) || !GscTestFxMarker(&lease,logical_ms) ||
        gscTestFx.revision!=lease.revision) goto refuse;
    *index_out=i;return 1;
refuse:
    GscTestError(error,capacity,"loadfx invalid/full or foreign FX table, late new resource or map/session changed");return 0;
}
/* Preserve recovered native storage/order points without changing generic
 * Q3 math. Binary32 stores are explicit; arbitrary x87 precision is not claimed. */
static int GscTestFxDirection(const float raw[3],unsigned *code,float *length_out) {
    float sum,length,inverse,normalized[3],score,best_score=0.f;
    double wide;int i,k,best=0;
    if(!raw || !code || !length_out || !CQ_Finite(raw[0]) || !CQ_Finite(raw[1]) || !CQ_Finite(raw[2])) return 0;
    wide=(double)raw[0]*raw[0]+(double)raw[1]*raw[1];wide+=(double)raw[2]*raw[2];
    if(!(wide>0.0) || wide>(double)FLT_MAX) return 0;
    sum=(float)wide;
    if(!CQ_Finite(sum) || sum<=0.f) return 0;
    length=(float)sqrt((double)sum);inverse=(float)(1.0/(double)length);
    if(!CQ_Finite(length) || length<=0.f || !CQ_Finite(inverse) || inverse<=0.f) return 0;
    for(k=0;k<3;++k) {normalized[k]=(float)((double)inverse*raw[k]);if(!CQ_Finite(normalized[k])) return 0;}
    for(i=0;i<NUMVERTEXNORMALS;++i) {
        wide=(double)normalized[2]*bytedirs[i][2]+(double)normalized[0]*bytedirs[i][0];
        wide+=(double)normalized[1]*bytedirs[i][1];score=(float)wide;
        if(!CQ_Finite(score)) return 0;
        if(score>best_score) {best_score=score;best=i;}
    }
    *code=(unsigned)best;*length_out=length;return 1;
}
static int GscTestFxPlay(void *user,uint64_t id,int logical_ms,
    const GscFxPlayEvent *event,char *error,size_t capacity) {
    GscFxPlayEvent copy;GscTestFxScene lease=gscTestFx;GscEntityRef player;
    cod1ScriptFx_t fx;char command[COD1_SCRIPT_FX_COMMAND_CAPACITY],actual[MAX_STRING_CHARS],expected[MAX_STRING_CHARS];
    int n;
    (void)user;
    if(!event) goto refuse;copy=*event;
    memset(&fx,0,sizeof(fx));fx.epoch=lease.epoch;fx.sequence=lease.sequence+1;
    fx.index=(int)((uint32_t)copy.index&UINT32_C(255));fx.directed=copy.has_direction;
    memcpy(fx.origin,copy.origin,sizeof(fx.origin));
    if(id!=lease.id || lease.sequence==UINT64_MAX || !GscTestFxTimeCurrent(&lease,logical_ms) ||
        fx.index<=0 || fx.index>=MAX_COD1_SCRIPT_FX || !gscTestFx.names[fx.index][0] ||
        (copy.has_direction!=0 && copy.has_direction!=1) ||
        (copy.has_direction && !GscTestFxDirection(copy.direction,&fx.direction_code,&fx.direction_length)) ||
        !CF_EncodeCommand(command,sizeof(command),&fx) ||
        !GscTestFxResourceText(expected,sizeof(expected),lease.epoch,fx.index,gscTestFx.names[fx.index]) ||
        !G_Cod1EntityCapture(&g_entities[0],&player) || !GscTestQuakePlayerCurrent(&player)) goto refuse;
    for(n=0;n<2;++n) {
        if(!GscTestFxMarker(&lease,logical_ms) || !GscTestQuakePlayerCurrent(&player) ||
            !GscTestFxRead(&lease,CS_COD1_SCRIPT_FX_TABLE+fx.index,actual,sizeof(actual),logical_ms,1) ||
            strcmp(actual,expected) || !GscTestFxMarker(&lease,logical_ms) ||
            !GscTestQuakePlayerCurrent(&player) || gscTestFx.sequence!=lease.sequence ||
            gscTestFx.revision!=lease.revision) goto refuse;
    }
    gscTestFx.sequence=fx.sequence;trap_SendServerCommand(0,command);
    if(!GscTestFxMarker(&lease,logical_ms) || !GscTestQuakePlayerCurrent(&player) ||
        gscTestFx.sequence!=fx.sequence || gscTestFx.revision!=lease.revision ||
        !GscTestFxRead(&lease,CS_COD1_SCRIPT_FX_TABLE+fx.index,actual,sizeof(actual),logical_ms,1) ||
        strcmp(actual,expected) || !GscTestFxMarker(&lease,logical_ms)) goto refuse;
    return 1;
refuse:
    GscTestError(error,capacity,"playfx invalid/unregistered FX index or direction, recipient unavailable or map/session changed");return 0;
}
