/* R117 server precacheShellshock name registration. Original game publishes
 * names independently of the client's scripts/<name>.shock acquisition. This
 * private port namespace uses actual configstring storage/readback; it is not
 * client loading, rendering, audio or shellshock application acknowledgment. */
#define CS_COD1_SCRIPT_SHELLSHOCK (CS_COD1_SCRIPT_FX_TABLE + MAX_COD1_SCRIPT_FX)
#define MAX_COD1_SCRIPT_SHELLSHOCK 16
#if CS_COD1_SCRIPT_SHELLSHOCK < CS_COD1_SCRIPT_FX_TABLE + MAX_COD1_SCRIPT_FX
#error shellshock overlaps FX storage
#endif
#if CS_COD1_SCRIPT_SHELLSHOCK + MAX_COD1_SCRIPT_SHELLSHOCK > MAX_CONFIGSTRINGS
#error shellshock exceeds configstring storage
#endif
typedef struct {
    GscSession *session;
    uint64_t id, map_epoch, incarnation, call_highwater, revision;
    int owned, marker_ready, failed, mode;
    char names[MAX_COD1_SCRIPT_SHELLSHOCK][128];
    char published[MAX_COD1_SCRIPT_SHELLSHOCK][128];
} GscTestShellshockScene;
static GscTestShellshockScene gscTestShellshock;
/* These guards are separate from the resettable scene. The initialization
 * flag spans the synchronous initial start including its ready-child drain.
 * Native flag scope supports this domain; FIFO child order is port policy. */
static int gscTestShellshockBusy, gscTestShellshockInitialization;
static int GscTestShellshockIdentity(const GscTestShellshockScene *lease) {
    return lease && lease->owned && lease->incarnation &&
        lease->incarnation!=UINT64_MAX && gscTestShellshock.owned &&
        lease->incarnation==gscTestShellshock.incarnation && lease->id &&
        lease->id==gscTestShellshock.id && lease->map_epoch &&
        lease->map_epoch==gscTestShellshock.map_epoch &&
        lease->map_epoch==G_Cod1EntityMapEpoch();
}
static int GscTestShellshockCurrent(const GscTestShellshockScene *lease) {
    GscSessionSnapshot snapshot;
    return GscTestShellshockIdentity(lease) && !gscTestShellshock.failed &&
        lease->session && lease->session==gscTestShellshock.session &&
        lease->session==gscTestSession && lease->id==gscTestId &&
        (lease->mode==24 || lease->mode==25 || (lease->mode==26 || lease->mode==27 || (lease->mode==28 || (lease->mode==29 || (lease->mode==30 || (lease->mode==31 || (lease->mode==32 || (lease->mode==33 || (lease->mode==34 || lease->mode==35 || lease->mode==36 || lease->mode==37 || lease->mode==38 || lease->mode==39 || (lease->mode==40||(lease->mode==41||(lease->mode==42||lease->mode==43)))))))))))) && gscTestEntityMode==lease->mode &&
        !gscTestCompleted && !gscTestStopRequested && !level.restarted &&
        G_Cod1MapEntities() && g_gametype.integer==GT_SINGLE_PLAYER &&
        gsc_session_snapshot(lease->session,&snapshot) &&
        snapshot.session_id==lease->id && !snapshot.error[0] &&
        (snapshot.state==GSC_SESSION_PREPARED || snapshot.state==GSC_SESSION_RUNNING);
}
static int GscTestShellshockTime(const GscTestShellshockScene *lease,int ms) {
    GscSessionSnapshot snap;
    return GscTestShellshockCurrent(lease) && gscTestShellshockInitialization &&
        ms>=0 && gsc_session_snapshot(lease->session,&snap) &&
        snap.session_id==lease->id && snap.state==GSC_SESSION_RUNNING &&
        !snap.error[0] && snap.logical_ms==ms;
}
static int GscTestShellshockAdmission(const GscTestShellshockScene *lease,
    int ms,int running) {
    static const char *names[6]={"g_cod1GscTest","g_cod1GscEntityTest",
        "savegame_loading","g_cod1GscTest","savegame_loading","g_cod1GscEntityTest"};
    int values[6]={1,0,0,1,0,0};int i,v;
    values[1]=values[5]=lease->mode;
    for(i=0;i<6;++i) {
        if(!(running?GscTestShellshockTime(lease,ms):GscTestShellshockCurrent(lease))) return 0;
        v=trap_Cvar_VariableIntegerValue(names[i]);
        if(!(running?GscTestShellshockTime(lease,ms):GscTestShellshockCurrent(lease)) || v!=values[i]) return 0;
    }
    return 1;
}
static int GscTestShellshockMarkerText(const GscTestShellshockScene *lease,
    char out[55]) {
    if(!lease || !lease->id || !lease->map_epoch || !lease->incarnation) return 0;
    memcpy(out,"c1s 1 ",6);CQ_WriteHex(out+6,16,lease->id);
    CQ_WriteHex(out+22,16,lease->map_epoch);CQ_WriteHex(out+38,16,lease->incarnation);
    out[54]=0;return 1;
}
static int GscTestShellshockRead(const GscTestShellshockScene *lease,int index,
    char out[MAX_STRING_CHARS],int ms,int running) {
    if(!GscTestShellshockAdmission(lease,ms,running)) return 0;
    memset(out,0,MAX_STRING_CHARS);trap_GetConfigstring(index,out,MAX_STRING_CHARS);
    return memchr(out,0,MAX_STRING_CHARS) && GscTestShellshockAdmission(lease,ms,running);
}
static int GscTestShellshockMarker(const GscTestShellshockScene *lease,int ms) {
    char marker[55],actual[MAX_STRING_CHARS];int n;
    if(!lease->marker_ready || !GscTestShellshockMarkerText(lease,marker)) return 0;
    for(n=0;n<2;++n)
        if(!GscTestShellshockRead(lease,CS_COD1_SCRIPT_SHELLSHOCK,actual,ms,1) ||
            strcmp(marker,actual)) return 0;
    return 1;
}
static int GscTestShellshockResetCurrent(uint64_t incarnation,uint64_t map) {
    return !gscTestShellshock.owned && incarnation==gscTestShellshock.incarnation &&
        map && map==G_Cod1EntityMapEpoch();
}
static int GscTestShellshockResetMarker(uint64_t incarnation,uint64_t map,
    const char *marker) {
    char actual[MAX_STRING_CHARS];int n;
    for(n=0;n<2;++n) {
        if(!GscTestShellshockResetCurrent(incarnation,map)) return 0;
        memset(actual,0,sizeof(actual));trap_GetConfigstring(CS_COD1_SCRIPT_SHELLSHOCK,actual,sizeof(actual));
        if(!GscTestShellshockResetCurrent(incarnation,map) ||
            !memchr(actual,0,sizeof(actual)) || strcmp(marker,actual)) return 0;
    }
    return 1;
}
static void GscTestShellshockReset(void) {
    GscTestShellshockScene old=gscTestShellshock;
    uint64_t incarnation=old.incarnation==UINT64_MAX?UINT64_MAX:old.incarnation+1;
    char marker[55],actual[MAX_STRING_CHARS];int i,n;
    memset(&gscTestShellshock,0,sizeof(gscTestShellshock));
    gscTestShellshock.incarnation=incarnation;gscTestShellshockInitialization=0;
    if(!old.owned || !old.marker_ready || !GscTestShellshockMarkerText(&old,marker) ||
        !GscTestShellshockResetMarker(incarnation,old.map_epoch,marker)) return;
    trap_SetConfigstring(CS_COD1_PLAYER_SHOCK,"");
    /* Invalidate before traps. Foreign/replacement records are never adopted
     * or erased; the independent callback busy guard survives this reset. */
    for(i=1;i<MAX_COD1_SCRIPT_SHELLSHOCK;++i) if(old.published[i][0]) {
        for(n=0;n<2;++n) {
            if(!GscTestShellshockResetMarker(incarnation,old.map_epoch,marker)) return;
            memset(actual,0,sizeof(actual));trap_GetConfigstring(CS_COD1_SCRIPT_SHELLSHOCK+i,actual,sizeof(actual));
            if(!GscTestShellshockResetCurrent(incarnation,old.map_epoch)) return;
            if(!memchr(actual,0,sizeof(actual)) || strcmp(actual,old.published[i])) break;
        }
        if(n==2) {
            if(!GscTestShellshockResetMarker(incarnation,old.map_epoch,marker)) return;
            trap_SetConfigstring(CS_COD1_SCRIPT_SHELLSHOCK+i,"");
            if(!GscTestShellshockResetCurrent(incarnation,old.map_epoch)) return;
        }
    }
    if(GscTestShellshockResetMarker(incarnation,old.map_epoch,marker))
        trap_SetConfigstring(CS_COD1_SCRIPT_SHELLSHOCK,"");
}
static int GscTestShellshockBegin(char *error,size_t capacity) {
    GscTestShellshockScene lease;char marker[55],actual[MAX_STRING_CHARS];int i,n;
    if(gscTestShellshockBusy || gscTestShellshock.owned ||
        !gscTestShellshock.incarnation || gscTestShellshock.incarnation==UINT64_MAX ||
        (gscTestEntityMode!=24 && gscTestEntityMode!=25 && (gscTestEntityMode!=26 && gscTestEntityMode!=27 && (gscTestEntityMode!=28 && (gscTestEntityMode!=29 && (gscTestEntityMode!=30 && (gscTestEntityMode!=31 && (gscTestEntityMode!=32 && (gscTestEntityMode!=33 && (gscTestEntityMode!=34 && (gscTestEntityMode!=35 && (gscTestEntityMode!=36 && (gscTestEntityMode!=37 && (gscTestEntityMode!=38 && (gscTestEntityMode!=39 && (gscTestEntityMode!=40&&(gscTestEntityMode!=41&&(gscTestEntityMode!=42&&gscTestEntityMode!=43)))))))))))))))))) goto refuse;
    gscTestShellshock.session=gscTestSession;gscTestShellshock.id=gscTestId;
    gscTestShellshock.mode=gscTestEntityMode;gscTestShellshock.map_epoch=G_Cod1EntityMapEpoch();
    gscTestShellshock.owned=1;lease=gscTestShellshock;gscTestShellshockBusy=1;
    if(!GscTestShellshockMarkerText(&lease,marker)) goto end;
    for(n=0;n<2;++n) for(i=0;i<MAX_COD1_SCRIPT_SHELLSHOCK;++i)
        if(!GscTestShellshockRead(&lease,CS_COD1_SCRIPT_SHELLSHOCK+i,actual,0,0) || actual[0]) goto end;
    if(!GscTestShellshockAdmission(&lease,0,0)) goto end;
    gscTestShellshock.marker_ready=1;trap_SetConfigstring(CS_COD1_SCRIPT_SHELLSHOCK,marker);
    if(!GscTestShellshockRead(&lease,CS_COD1_SCRIPT_SHELLSHOCK,actual,0,0) || strcmp(marker,actual)) goto end;
    gscTestShellshockBusy=0;return 1;
end:
    if(GscTestShellshockIdentity(&lease)) gscTestShellshock.failed=1;
    gscTestShellshockBusy=0;
refuse:
    GscTestError(error,capacity,"shellshock scene unavailable, occupied or map/session changed");return 0;
}
static int GscTestShellshockPayload(const GscShellshockEvent *event) {
    const char *end;size_t i;
    if(!event || !event->operation_id || !(end=(const char *)memchr(event->name,0,sizeof(event->name)))) return 0;
    /* Bounded printable ASCII is port transport policy, not retail validation.
     * The quoted cmd tokenizer preserves spaces/slashes/backslashes/semicolon;
     * quotes and controls cannot enter the unescaped CS command producer. */
    for(i=0;i<(size_t)(end-event->name);++i)
        if((unsigned char)event->name[i]<32 || (unsigned char)event->name[i]>126 || event->name[i]=='"') return 0;
    return 1;
}
static int GscTestShellshockTable(const GscTestShellshockScene *lease,int ms,
    const char *name,int *slot,int *duplicate) {
    char actual[MAX_STRING_CHARS];int i,empty=0;
    *slot=0;*duplicate=0;
    for(i=1;i<MAX_COD1_SCRIPT_SHELLSHOCK;++i) {
        if(gscTestShellshock.revision!=lease->revision ||
            !GscTestShellshockRead(lease,CS_COD1_SCRIPT_SHELLSHOCK+i,actual,ms,1) ||
            strcmp(actual,lease->names[i]) || !GscTestShellshockMarker(lease,ms)) return 0;
        if(!actual[0]) {if(!empty) empty=i;}
        else {
            if(empty) return 0;
            if(!Q_stricmp(actual,name)) {*slot=i;*duplicate=1;}
        }
    }
    if(!*duplicate) *slot=empty;
    return 1;
}
/* GAME 37DE0 and 37F30 store index, server start and duration for
 * the current player. This SP configstring carries all three atomically. */
int G_Cod1PlayerShellshock(const GscPlayerNativeRequest *input,GscPlayerNativeResult *out,
    char *error,size_t cap,int (*current)(void *),void *ctx){
    GscPlayerNativeRequest q=*input;
    GscTestShellshockScene scene=gscTestShellshock;
    char actual[MAX_STRING_CHARS],packet[96];int i,index=0,ms=0,now=level.time;
    double rounded;gentity_t *player;
    if(!current||q.player.slot>=level.maxclients||!G_Cod1EntitySameAllocation(&q.player)||
       !GscTestShellshockCurrent(&scene)||gscTestShellshockBusy)goto failed;
    player=g_entities+q.player.slot;
    if(!player->client||player->client!=level.clients+q.player.slot||
       player->client->pers.connected!=CON_CONNECTED)goto failed;
    if(q.kind==GSC_PLAYER_SHELLSHOCK){
        for(i=1;i<MAX_COD1_SCRIPT_SHELLSHOCK;i++){
            trap_GetConfigstring(CS_COD1_SCRIPT_SHELLSHOCK+i,actual,sizeof(actual));
            if(scene.names[i][0]&&!Q_stricmp(actual,q.weapon)&&!strcmp(actual,scene.names[i])){index=i;break;}
        }
        if(!index){GscTestError(error,cap,"shellshock effect was not precached");return 0;}
        /* Retail stores the product as float before x87 fistp with a positive
         * 2^-30 bias. For this bounded range this resolves exact half ties up. */
        rounded=floor((double)(float)(q.number*1000.0f)+0.5);
        if(!(rounded>=0 && rounded<=60000)){GscTestError(error,cap,"shellshock duration outside 0..60000 milliseconds");return 0;}
        ms=(int)rounded;
    }else if(q.kind!=GSC_PLAYER_STOP_SHELLSHOCK)goto failed;
    if(current(ctx)!=1||!GscTestShellshockCurrent(&scene)||level.time!=now||
       scene.revision!=gscTestShellshock.revision||!G_Cod1EntitySameAllocation(&q.player)||
       memcmp(&q,input,sizeof(q)))goto failed;
    if(index)Com_sprintf(packet,sizeof(packet),"%u %u %d %d %d",q.player.slot,q.player.generation,index,now,ms);
    else packet[0]=0;
    trap_SetConfigstring(CS_COD1_PLAYER_SHOCK,packet);
    trap_GetConfigstring(CS_COD1_PLAYER_SHOCK,actual,sizeof(actual));
    if(!G_Cod1EntitySameAllocation(&q.player)||strcmp(actual,packet))goto failed;
    memset(out,0,sizeof(*out));
    if(g_debugMove.integer>1)G_Printf("[shellshock GAME] player=%u effect=%d start=%d duration=%d\n",q.player.slot,index,index?now:0,ms);
    return 1;
failed:
    GscTestError(error,cap,"shellshock requires the current player and effect table");return 0;
}
static int GscTestShellshockPublish(void *user,uint64_t id,int ms,
    const GscShellshockEvent *event,char *error,size_t capacity) {
    GscShellshockEvent copy;GscTestShellshockScene lease;
    char actual[MAX_STRING_CHARS];int slot=0,duplicate=0,n,accepted=0;
    (void)user;
    if(gscTestShellshockBusy) {
        gscTestStopRequested=1;
        GscTestError(error,capacity,"shellshock callback reentry refused");return 0;
    }
    if(!event) goto refuse;copy=*event;lease=gscTestShellshock;
    if(!GscTestShellshockPayload(&copy) || id!=lease.id ||
        !GscTestShellshockTime(&lease,ms) || copy.operation_id<=lease.call_highwater ||
        lease.revision==UINT64_MAX) goto refuse;
    gscTestShellshockBusy=1;gscTestShellshock.call_highwater=copy.operation_id;
    if(!GscTestShellshockMarker(&lease,ms)) goto end;
    if(!copy.name[0]) {accepted=1;goto end;}
    for(n=0;n<2;++n) {
        if(!GscTestShellshockTable(&lease,ms,copy.name,&slot,&duplicate) || !slot) goto end;
    }
    if(duplicate) {accepted=1;goto end;}
    /* Expected ownership is recorded before VOID Set for bounded cleanup;
     * names[] and revision represent only an observed successful commit. */
    memcpy(gscTestShellshock.published[slot],copy.name,sizeof(copy.name));
    trap_SetConfigstring(CS_COD1_SCRIPT_SHELLSHOCK+slot,copy.name);
    if(!GscTestShellshockRead(&lease,CS_COD1_SCRIPT_SHELLSHOCK+slot,actual,ms,1) ||
        strcmp(actual,copy.name) || !GscTestShellshockMarker(&lease,ms) ||
        gscTestShellshock.revision!=lease.revision) goto end;
    memcpy(lease.names[slot],copy.name,sizeof(copy.name));
    for(n=0;n<2;++n)
        if(!GscTestShellshockTable(&lease,ms,copy.name,&slot,&duplicate) || !duplicate) goto end;
    memcpy(gscTestShellshock.names[slot],copy.name,sizeof(copy.name));
    ++gscTestShellshock.revision;accepted=1;
end:
    if(!accepted && GscTestShellshockIdentity(&lease)) gscTestShellshock.failed=1;
    gscTestShellshockBusy=0;
    if(accepted) return 1;
refuse:
    GscTestError(error,capacity,"shellshock invalid/full/foreign table, late registration or map/session changed");return 0;
}
