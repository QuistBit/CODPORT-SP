/* R129/R130 localized-string host; SERVER now owns exact map receipts across
 * GAME reload, while this record owns only the Session publication lease.
 * Native ordinals1..511, ASCII folding and first-empty stop are kept.
 * Loading-only publication and bounded printable names are port policies. */
typedef struct {
    GscSession *session;
    uint64_t id,map_epoch,incarnation,operation_id;
    int owned,marker_ready,mode,host_time;
    char marker[128];
} GscTestPrecacheStringScene;
static GscTestPrecacheStringScene gscTestPrecacheString;
static int gscTestPrecacheStringBusy;
static int GscTestPrecacheStringIdentity(const GscTestPrecacheStringScene *p) {
    return p && p->owned && gscTestPrecacheString.owned && p->session &&
        p->session==gscTestSession && p->session==gscTestPrecacheString.session &&
        p->id && p->id==gscTestId && p->id==gscTestPrecacheString.id &&
        p->incarnation && p->incarnation==gscTestPrecacheString.incarnation &&
        p->map_epoch && p->map_epoch==G_Cod1EntityMapEpoch() && p->map_epoch==gscTestPrecacheString.map_epoch &&
        (p->mode==29 || (p->mode==30 || (p->mode==31 || (p->mode==32 || (p->mode==33 || (p->mode==34 || p->mode==35 || p->mode==36 || p->mode==37 || p->mode==38 || p->mode==39 || p->mode==40 || (p->mode==41||(p->mode==42||p->mode==43)))))))) && p->mode==gscTestEntityMode && p->mode==gscTestPrecacheString.mode &&
        !gscTestStopRequested && !gscTestCompleted && !level.restarted &&
        G_Cod1MapEntities() && g_gametype.integer==GT_SINGLE_PLAYER;
}
static int GscTestPrecacheStringCurrent(const GscTestPrecacheStringScene *p,int now,int running) {
    GscSessionSnapshot view;
    if(!GscTestPrecacheStringIdentity(p) || !gsc_session_snapshot(p->session,&view) ||
       view.session_id!=p->id || view.error[0])return 0;
    if(running)return now>=0 && level.time==p->host_time && gsc_session_precache_string_startup(p->session,p->id,now);
    return view.state==GSC_SESSION_PREPARED;
}
static int GscTestPrecacheStringAdmission(const GscTestPrecacheStringScene *p,int now,int running) {
    static const char *names[6]={"g_cod1GscTest","g_cod1GscEntityTest","savegame_loading",
        "g_cod1GscTest","savegame_loading","g_cod1GscEntityTest"};
    int values[6]={1,p->mode,0,1,0,p->mode},i,value;
    for(i=0;i<6;++i) {
        if(!GscTestPrecacheStringCurrent(p,now,running))return 0;
        value=trap_Cvar_VariableIntegerValue(names[i]);
        if(!GscTestPrecacheStringCurrent(p,now,running) || value!=values[i])return 0;
        if((i==0 || i==5) && (!GscTestModuleReadCurrent() || !GscTestPrecacheStringCurrent(p,now,running)))return 0;
    }
    return 1;
}
static int GscTestPrecacheStringRead(const GscTestPrecacheStringScene *p,int slot,char text[129],int now,int running) {
    if(slot<0 || slot>=MAX_COD1_LOCALIZED_STRINGS || !GscTestPrecacheStringAdmission(p,now,running))return 0;
    memset(text,0,129);trap_GetConfigstring(CS_COD1_LOCALIZED_STRINGS+slot,text,129);
    return GscTestPrecacheStringAdmission(p,now,running) && memchr(text,0,129) && strlen(text)<=127;
}
static int GscTestPrecacheStringMarker(const GscTestPrecacheStringScene *p,int now,int running) {
    char text[129];
    return p->marker_ready && GscTestPrecacheStringRead(p,0,text,now,running) &&
        !strcmp(text,p->marker) && !strcmp(p->marker,gscTestPrecacheString.marker);
}
/* Pure VM pre-argument gate: no engine getter or publication callback. */
static int GscTestPrecacheStringStartup(void *user,uint64_t id,int now) {
    GscTestPrecacheStringScene p=gscTestPrecacheString;(void)user;
    return !gscTestPrecacheStringBusy && id==p.id && p.marker_ready && GscTestPrecacheStringCurrent(&p,now,1);
}
static int GscTestPrecacheStringBegin(char *error,size_t capacity) {
    GscTestPrecacheStringScene p;char marker[128];int accepted=0;
    if(gscTestPrecacheStringBusy || gscTestPrecacheString.owned || gscTestPrecacheString.incarnation==UINT64_MAX)goto refuse;
    ++gscTestPrecacheString.incarnation;gscTestPrecacheString.owned=1;
    gscTestPrecacheString.session=gscTestSession;gscTestPrecacheString.id=gscTestId;
    gscTestPrecacheString.map_epoch=G_Cod1EntityMapEpoch();gscTestPrecacheString.mode=gscTestEntityMode;
    gscTestPrecacheString.host_time=level.time;p=gscTestPrecacheString;gscTestPrecacheStringBusy=1;
    if(!GscTestPrecacheStringAdmission(&p,0,0))goto end;
    /* SERVER verifies the complete retained map table against its own receipts,
     * and publishes a new per-Session lease atomically to restart clients.
     * Public marker text alone never authorizes adoption. */
    memset(marker,0,sizeof(marker));
    if(!trap_Cod1LocalizedAcquire(marker,sizeof(marker)))goto end;
    strcpy(gscTestPrecacheString.marker,marker);gscTestPrecacheString.marker_ready=1;
    p=gscTestPrecacheString;
    if(!GscTestPrecacheStringAdmission(&p,0,0)||!GscTestPrecacheStringMarker(&p,0,0))goto end;
    accepted=1;
end:
    gscTestPrecacheStringBusy=0;if(accepted)return 1;
refuse:
    GscTestError(error,capacity,"localized-string public namespace occupied, unavailable or owner changed");return 0;
}
static int GscTestPrecacheStringAsciiEqual(const char *a,const char *b) {
    unsigned char x,y;
    do {x=(unsigned char)*a++;y=(unsigned char)*b++;if(x>='A'&&x<='Z')x+=32;if(y>='A'&&y<='Z')y+=32;
        if(x!=y)return 0;}while(x);return 1;
}
static int GscTestPrecacheStringPublish(void *user,uint64_t id,int now,
    const GscPrecacheStringEvent *event,char *error,size_t capacity) {
    GscPrecacheStringEvent copy;GscTestPrecacheStringScene p;char text[129];
    const unsigned char *at;int slot,accepted=0;uintptr_t eventAt,errorAt;
    (void)user;
    if(event && error && capacity) {
        eventAt=(uintptr_t)event;errorAt=(uintptr_t)error;
        if(errorAt>=eventAt?errorAt-eventAt<sizeof(*event):eventAt-errorAt<capacity)return 0;
    }
    if(gscTestPrecacheStringBusy || !event)goto refuse;
    copy=*event;p=gscTestPrecacheString;
    if(!copy.operation_id || copy.operation_id<=p.operation_id || id!=p.id ||
       !memchr(copy.name,0,sizeof(copy.name)) || !copy.name[0])goto refuse;
    for(at=(const unsigned char *)copy.name;*at;++at)
        if(*at<32 || *at>126 || *at=='"' || *at=='\\' || *at=='%')goto refuse;
    gscTestPrecacheStringBusy=1;
    if(!GscTestPrecacheStringMarker(&p,now,1))goto end;
    for(slot=1;slot<MAX_COD1_LOCALIZED_STRINGS;++slot) {
        if(!GscTestPrecacheStringRead(&p,slot,text,now,1) || !GscTestPrecacheStringMarker(&p,now,1))goto end;
        if(!text[0])break;
        if(GscTestPrecacheStringAsciiEqual(text,copy.name)) {
            /* Atomic unchanged-store validation preserves the first spelling
             * and closes a target replacement during the marker getter. */
            if(!trap_Cod1TrySetLocalizedConfigstring(CS_COD1_LOCALIZED_STRINGS+slot,text,text,p.marker))goto end;
            accepted=1;break;
        }
    }
    if(slot==MAX_COD1_LOCALIZED_STRINGS)goto end;
    if(!accepted) {
        if(!GscTestPrecacheStringRead(&p,slot,text,now,1) || text[0] || !GscTestPrecacheStringMarker(&p,now,1))goto end;
        if(!trap_Cod1TrySetLocalizedConfigstring(CS_COD1_LOCALIZED_STRINGS+slot,copy.name,"",p.marker))goto end;
        /* Preserve a visible committed prefix on later refusal; no rollback
         * can clear a replacement owner or retract a public gamestate. */
        /* The accepted ordinal now belongs to the SERVER map namespace. */
        if(!GscTestPrecacheStringRead(&p,slot,text,now,1) || strcmp(text,copy.name) ||
           !GscTestPrecacheStringMarker(&p,now,1))goto end;
        accepted=1;
    }
    if(accepted && GscTestPrecacheStringIdentity(&p))gscTestPrecacheString.operation_id=copy.operation_id;
end:
    gscTestPrecacheStringBusy=0;if(accepted)return 1;
refuse:
    GscTestError(error,capacity,"localized string unavailable, pool full, loading phase closed or owner changed");return 0;
}
static void GscTestPrecacheStringReset(void) {
    GscTestPrecacheStringScene old=gscTestPrecacheString;
    if(gscTestPrecacheStringBusy)return;
    gscTestPrecacheStringBusy=1;memset(&gscTestPrecacheString,0,sizeof(gscTestPrecacheString));
    gscTestPrecacheString.incarnation=old.incarnation;
    /* Release only this Session capability. Public ordinals survive until the
     * genuine SERVER map clear, even after a failed/finished Session. A stale
     * lease cannot release the current GAME's lease or erase replacement data. */
    if(old.owned&&old.marker_ready)(void)trap_Cod1LocalizedRelease(old.marker);
    gscTestPrecacheStringBusy=0;
}
