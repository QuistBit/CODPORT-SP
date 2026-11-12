/* R116 selected setcvar: original REGISTER then SET phases, copied bytes.
 * Original raw0x1000 is evidence only, with unproved engine semantics. The
 * port registers flags0 without a vmCvar_t snapshot, then uses the existing
 * forced trap_Cvar_Set import. No original protection/latch/ownership mapping
 * is claimed. Accepted registration is a committed prefix, never rolled back. */
#include "../../qcommon/cod1_ambient_module.h"
typedef struct {
    GscSession *session;
    uint64_t id, map_epoch, incarnation, operation, call_highwater;
    int active, pending, logical_ms, mode;
    GscCvarEvent payload;
} GscTestCvarOperation;
static GscTestCvarOperation gscTestCvar;
/* Separate from resettable operation state: a boundary during a trap cannot
 * reopen this callback and permit a second nested external publication. */
static int gscTestCvarBusy;

static void GscTestCvarReset(void) {
    uint64_t incarnation=gscTestCvar.incarnation;
    if(incarnation!=UINT64_MAX) ++incarnation;
    memset(&gscTestCvar,0,sizeof(gscTestCvar));
    gscTestCvar.incarnation=incarnation;
}
static int GscTestCvarSame(const GscTestCvarOperation *lease) {
    return lease && lease->active && gscTestCvar.active &&
        lease->incarnation && lease->incarnation!=UINT64_MAX &&
        lease->incarnation==gscTestCvar.incarnation && lease->operation &&
        lease->operation==gscTestCvar.operation && lease->map_epoch &&
        lease->map_epoch==gscTestCvar.map_epoch &&
        lease->map_epoch==G_Cod1EntityMapEpoch() && lease->session &&
        lease->session==gscTestCvar.session && lease->session==gscTestSession &&
        lease->mode==gscTestCvar.mode && (lease->mode==23 || lease->mode==24 || lease->mode==25 || (lease->mode==26 || lease->mode==27 || (lease->mode==28 || (lease->mode==29 || (lease->mode==30 || (lease->mode==31 || (lease->mode==32 || (lease->mode==33 || (lease->mode==34 || lease->mode==35 || lease->mode==36 || lease->mode==37 || lease->mode==38 || lease->mode==39 || (lease->mode==40||(lease->mode==41||(lease->mode==42||lease->mode==43)))))))))))) &&
        lease->id && lease->id==gscTestCvar.id && lease->id==gscTestId;
}
static int GscTestCvarCurrent(const GscTestCvarOperation *lease,int logical_ms) {
    GscSessionSnapshot snapshot;
    return GscTestCvarSame(lease) && gscTestEntityMode==lease->mode &&
        !gscTestCompleted && !gscTestStopRequested && !level.restarted &&
        G_Cod1MapEntities() && g_gametype.integer==GT_SINGLE_PLAYER &&
        logical_ms>=0 && lease->logical_ms==logical_ms &&
        gsc_session_snapshot(lease->session,&snapshot) &&
        snapshot.session_id==lease->id && snapshot.state==GSC_SESSION_RUNNING &&
        !snapshot.error[0] && snapshot.logical_ms==logical_ms;
}
static int GscTestCvarAdmission(const GscTestCvarOperation *lease,int logical_ms) {
    static const char *names[6]={"g_cod1GscTest","g_cod1GscEntityTest",
        "savegame_loading","g_cod1GscTest","savegame_loading","g_cod1GscEntityTest"};
    int values[6]={1,0,0,1,0,0};
    int n,value;
    values[1]=values[5]=lease->mode;
    for(n=0;n<6;++n) {
        if(!GscTestCvarCurrent(lease,logical_ms)) return 0;
        value=trap_Cvar_VariableIntegerValue(names[n]);
        if(!GscTestCvarCurrent(lease,logical_ms) || value!=values[n]) return 0;
    }
    return 1;
}
static int GscTestCvarProtected(const char *name) {
    const unsigned char *a=(const unsigned char *)name;
    const unsigned char *b=(const unsigned char *)COD1_AMBIENT_MODULE_CVAR;
    unsigned int x,y;
    do {
        x=*a++;y=*b++;
        if(x>='A' && x<='Z') x+='a'-'A';
        if(y>='A' && y<='Z') y+='a'-'A';
        if(x!=y) return 0;
    } while(x);
    return 1;
}
static int GscTestCvarPayload(const GscCvarEvent *event) {
    const char *end;size_t n;
    if(!event || (event->phase!=GSC_CVAR_REGISTER && event->phase!=GSC_CVAR_SET) ||
        event->native_flags!=UINT32_C(0x1000) || !event->operation_id ||
        !(end=(const char *)memchr(event->name,0,sizeof(event->name))) ||
        end==event->name || event->length>=sizeof(event->value) ||
        memchr(event->value,0,event->length) || event->value[event->length] ||
        GscTestCvarProtected(event->name)) return 0;
    /* Generic printable ASCII name subset, not a list of particular UI names.
     * Refuse malformed RTCW names before either import can substitute BADNAME.
     * Values are literal bytes: quotes, @, controls and backslash stay intact. */
    for(n=0;n<(size_t)(end-event->name);++n) {
        unsigned ch=(unsigned char)event->name[n];
        if(ch<33 || ch>126 || ch=='\\' || ch=='"' || ch==';') return 0;
    }
    return 1;
}
static int GscTestCvarPayloadSame(const GscCvarEvent *a,const GscCvarEvent *b) {
    return a->operation_id==b->operation_id && a->native_flags==b->native_flags && a->length==b->length &&
        !strcmp(a->name,b->name) && !memcmp(a->value,b->value,a->length+1);
}
static void GscTestCvarAbandon(const GscTestCvarOperation *lease) {
    if(GscTestCvarSame(lease)) {
        gscTestCvar.active=0;gscTestCvar.pending=0;
        memset(&gscTestCvar.payload,0,sizeof(gscTestCvar.payload));
    }
}
static int GscTestCvarPublish(void *user,uint64_t id,int logical_ms,
    const GscCvarEvent *event,char *error,size_t capacity) {
    GscCvarEvent copy;GscTestCvarOperation lease;GscSessionSnapshot snapshot;
    int accepted=0;
    (void)user;
    if(gscTestCvarBusy) {
        gscTestStopRequested=1;
        GscTestError(error,capacity,"setcvar callback reentry refused");return 0;
    }
    if(!event) goto refuse;
    copy=*event;
    if(!GscTestCvarPayload(&copy) || (gscTestEntityMode!=23 && gscTestEntityMode!=24 && gscTestEntityMode!=25 && (gscTestEntityMode!=26 && gscTestEntityMode!=27 && (gscTestEntityMode!=28 && (gscTestEntityMode!=29 && (gscTestEntityMode!=30 && (gscTestEntityMode!=31 && (gscTestEntityMode!=32 && (gscTestEntityMode!=33 && (gscTestEntityMode!=34 && (gscTestEntityMode!=35 && (gscTestEntityMode!=36 && (gscTestEntityMode!=37 && (gscTestEntityMode!=38 && (gscTestEntityMode!=39 && (gscTestEntityMode!=40&&(gscTestEntityMode!=41&&(gscTestEntityMode!=42&&gscTestEntityMode!=43))))))))))))))))) || !gscTestSession ||
        !id || id!=gscTestId || gscTestCompleted || gscTestStopRequested ||
        logical_ms<0 || !gsc_session_snapshot(gscTestSession,&snapshot) ||
        snapshot.session_id!=id || snapshot.state!=GSC_SESSION_RUNNING ||
        snapshot.logical_ms!=logical_ms) goto refuse;
    if(copy.phase==GSC_CVAR_REGISTER) {
        if(copy.operation_id<=gscTestCvar.call_highwater || !gscTestCvar.incarnation ||
            gscTestCvar.incarnation==UINT64_MAX || gscTestCvar.operation==UINT64_MAX) goto refuse;
        /* A receiver can retire after REGISTER returns while other tasks keep
         * running. A newer copied call token supersedes its abandoned internal
         * reservation, preserving the committed engine prefix. Exact SET token
         * correlation refuses old phases even with identical bytes and clock. */
        gscTestCvar.session=gscTestSession;gscTestCvar.id=id;gscTestCvar.mode=gscTestEntityMode;
        gscTestCvar.map_epoch=G_Cod1EntityMapEpoch();
        ++gscTestCvar.operation;gscTestCvar.logical_ms=logical_ms;
        gscTestCvar.call_highwater=copy.operation_id;
        gscTestCvar.active=gscTestCvar.pending=1;gscTestCvar.payload=copy;
    } else if(!gscTestCvar.active || !gscTestCvar.pending ||
        gscTestCvar.id!=id || gscTestCvar.logical_ms!=logical_ms ||
        !GscTestCvarPayloadSame(&copy,&gscTestCvar.payload)) goto refuse;
    lease=gscTestCvar;gscTestCvarBusy=1;
    if(!GscTestCvarAdmission(&lease,logical_ms) ||
        !GscTestCvarAdmission(&lease,logical_ms)) goto end;
    if(copy.phase==GSC_CVAR_REGISTER) {
        trap_Cvar_Register(NULL,copy.name,copy.value,0);
    } else {
        /* Consume the pending phase before the effect. A repeated SET, failed
         * post-check or callback cancellation never replays this publication. */
        gscTestCvar.pending=0;
        trap_Cvar_Set(copy.name,copy.value);
    }
    if(!GscTestCvarAdmission(&lease,logical_ms)) goto end;
    accepted=1;
end:
    if(!accepted || copy.phase==GSC_CVAR_SET) GscTestCvarAbandon(&lease);
    gscTestCvarBusy=0;
    if(accepted) return 1;
refuse:
    GscTestError(error,capacity,"setcvar malformed payload, stale phase or map/session changed");return 0;
}
