/* R112 selected diagnostic intent. These private records acknowledge the
 * observed retail wrapper phases only: they do not run a map controller,
 * persistence serializer, RTCW intermission/fades or engine completion UI.
 * Retail g_reloading is identified; the retained port's cached field is only
 * an additional protective admission gate, never written by this producer. */
typedef struct {
    uint64_t epoch;
    int owned,requested,finished,success,failed,reload_intent,fade_intent;
    int32_t persistent;
    int fade_start,fade_duration,fade_deadline;
    size_t map_length;
    char map[1024];
} GscTestMissionIntent;
static GscTestMissionIntent gscTestMission;
typedef struct {GscSession *session;uint64_t id,epoch,map;GscEntityRef player;int phase,at,last,deadline;char target[MAX_QPATH];} GscChangeLevel;
static GscChangeLevel gscChangeLevel;
static void GscChangeCancel(void);
static int GscChangeBegin(const char *target);

#include "../bg_cod1_mission_transition.h"
static Cod1MissionTransition gscTestFinale;
static int gscTestFinaleSelected;
static GscEntityRef gscTestFinalePlayer;

static void GscTestFailureCancel(void);
static void GscTestMissionReset(void) {
    uint64_t epoch=gscTestMission.epoch;
    GscTestFailureCancel();GscChangeCancel();
    Cod1MissionTransitionReset(&gscTestFinale);gscTestFinaleSelected=0;
    memset(&gscTestFinalePlayer,0,sizeof(gscTestFinalePlayer));
    memset(&gscTestMission,0,sizeof(gscTestMission));
    gscTestMission.epoch=epoch==UINT64_MAX?UINT64_MAX:epoch+1;
}
static int GscTestMissionBegin(void) {
    if ((gscTestEntityMode!=19 && gscTestEntityMode!=20 && gscTestEntityMode!=21 && gscTestEntityMode!=22 && gscTestEntityMode!=23 && gscTestEntityMode!=24 && gscTestEntityMode!=25 && (gscTestEntityMode!=26 && gscTestEntityMode!=27 && (gscTestEntityMode!=28 && (gscTestEntityMode!=29 && (gscTestEntityMode!=30 && (gscTestEntityMode!=31 && (gscTestEntityMode!=32 && (gscTestEntityMode!=33 && (gscTestEntityMode!=34 && (gscTestEntityMode!=35 && (gscTestEntityMode!=36 && (gscTestEntityMode!=37 && (gscTestEntityMode!=38 && (gscTestEntityMode!=39 && (gscTestEntityMode!=40&&(gscTestEntityMode!=41&&(gscTestEntityMode!=42&&gscTestEntityMode!=43))))))))))))))))) || !gscTestMission.epoch ||
        gscTestMission.epoch==UINT64_MAX || gscTestMission.owned ||
        gscTestStopRequested || !G_Cod1MapEntities() ||
        g_gametype.integer!=GT_SINGLE_PLAYER || level.restarted ||
        !GscTestModuleReadCurrent()) return 0;
    gscTestMission.owned=1;
    return 1;
}
static int GscTestMissionCurrent(uint64_t id,uint64_t epoch) {
    if (!id || id!=gscTestId || !gscTestSession || gscTestCompleted ||
        epoch!=gscTestMission.epoch || !gscTestMission.owned ||
        (gscTestEntityMode!=19 && gscTestEntityMode!=20 && gscTestEntityMode!=21 && gscTestEntityMode!=22 && gscTestEntityMode!=23 && gscTestEntityMode!=24 && gscTestEntityMode!=25 && (gscTestEntityMode!=26 && gscTestEntityMode!=27 && (gscTestEntityMode!=28 && (gscTestEntityMode!=29 && (gscTestEntityMode!=30 && (gscTestEntityMode!=31 && (gscTestEntityMode!=32 && (gscTestEntityMode!=33 && (gscTestEntityMode!=34 && (gscTestEntityMode!=35 && (gscTestEntityMode!=36 && (gscTestEntityMode!=37 && (gscTestEntityMode!=38 && (gscTestEntityMode!=39 && (gscTestEntityMode!=40&&(gscTestEntityMode!=41&&(gscTestEntityMode!=42&&gscTestEntityMode!=43))))))))))))))))) || gscTestStopRequested || level.restarted ||
        !G_Cod1MapEntities() || g_gametype.integer!=GT_SINGLE_PLAYER ||
        !GscTestModuleReadCurrent()) return 0;
    return id==gscTestId && gscTestSession && !gscTestCompleted &&
        epoch==gscTestMission.epoch && gscTestMission.owned &&
        (gscTestEntityMode==19 || gscTestEntityMode==20 || gscTestEntityMode==21 || (gscTestEntityMode==22 || gscTestEntityMode==23 || gscTestEntityMode==24 || gscTestEntityMode==25 || (gscTestEntityMode==26 || (gscTestEntityMode==27 || (gscTestEntityMode==28 || (gscTestEntityMode==29 || (gscTestEntityMode==30 || (gscTestEntityMode==31 || (gscTestEntityMode==32 || (gscTestEntityMode==33 || (gscTestEntityMode==34 || gscTestEntityMode==35 || gscTestEntityMode==36 || gscTestEntityMode==37 || gscTestEntityMode==38 || gscTestEntityMode==39 || (gscTestEntityMode==40||(gscTestEntityMode==41||(gscTestEntityMode==42||gscTestEntityMode==43)))))))))))))) && !gscTestStopRequested && !level.restarted &&
        G_Cod1MapEntities() && g_gametype.integer==GT_SINGLE_PLAYER;
}
/* Slot zero attached to the active client storage is a narrow port binding.
 * It does not replace or prove retail's named-player lookup implementation. */
static gentity_t *GscTestMissionPlayer(void) {
    if (!level.clients || !g_entities[0].inuse ||
        g_entities[0].client!=&level.clients[0]) return NULL;
    return &g_entities[0];
}
static int GscTestMissionGate(void *user,uint64_t id,int logicalMs,
    int kind,int *enabled,char *error,size_t capacity) {
    uint64_t epoch=gscTestMission.epoch;
    gentity_t *player;
    (void)user;
    if (enabled) *enabled=0;
    if (!enabled || logicalMs<0 ||
        (kind!=GSC_MISSION_SUCCESS && kind!=GSC_MISSION_FAILED && kind!=GSC_MISSION_CHANGELEVEL) ||
        !GscTestMissionCurrent(id,epoch)) goto refuse;
    player=GscTestMissionPlayer();
    if (kind==GSC_MISSION_SUCCESS || kind==GSC_MISSION_CHANGELEVEL) {
        if (!player) {
            GscTestError(error,capacity,"mission success player binding unavailable");
            return 0;
        }
        /* These tests precede VM typed getters, as in the shared retail call.
         * Combining private reload intent with the retained port's cached
         * g_reloading value is a protective port policy, not frame parity. */
        *enabled=player->health>0 && !gscTestMission.reload_intent &&
            !g_reloading.integer;
    } else *enabled=player!=NULL; /* Failure deliberately has no health gate. */
    if (!GscTestMissionCurrent(id,epoch)) goto refuse;
    return 1;
refuse:
    if (enabled) *enabled=0;
    GscTestError(error,capacity,"mission gate invalid or map/session changed");
    return 0;
}
static int GscTestMissionEventValid(const GscMissionEvent *event) {
    const char *end;
    if (!event || (event->has_arg1!=0 && event->has_arg1!=1)) return 0;
    end=memchr(event->map,0,sizeof(event->map));
    if (!end || event->length!=(size_t)(end-event->map)) return 0;
    if (event->phase==GSC_MISSION_REQUEST) return 1;
    if (event->length) return 0;
    if (event->phase==GSC_MISSION_ARGUMENT) return event->has_arg1==1;
    return !event->has_arg1 && !event->arg1;
}
static int GscTestMissionSuccess(void *user,uint64_t id,int logicalMs,
    const GscMissionEvent *event,char *error,size_t capacity) {
    GscMissionEvent copy;
    uint64_t epoch=gscTestMission.epoch;
    (void)user;
    if (!event) goto refuse;
    copy=*event; /* Do not keep a borrowed event across engine services. */
    if (logicalMs<0 || (copy.kind!=GSC_MISSION_SUCCESS && copy.kind!=GSC_MISSION_CHANGELEVEL) ||
        (copy.kind==GSC_MISSION_CHANGELEVEL && copy.phase==GSC_MISSION_SUCCESS_MARK) ||
        (copy.phase!=GSC_MISSION_ARGUMENT && copy.phase!=GSC_MISSION_REQUEST &&
         copy.phase!=GSC_MISSION_SUCCESS_MARK) ||
        !GscTestMissionEventValid(&copy) ||
        !GscTestMissionCurrent(id,epoch)) goto refuse;
    if (copy.phase==GSC_MISSION_ARGUMENT) {
        gscTestMission.persistent=copy.arg1;
    } else if (copy.phase==GSC_MISSION_REQUEST) {
        if(gscTestFinaleSelected && (gscTestFinale.phase || !copy.has_arg1 ||
           copy.arg1 || strcmp(copy.map,"us_intro"))) {
            GscTestError(error,capacity,"R229 finale requires one nonpersistent us_intro request");return 0;
        }
        if ((copy.has_arg1 && copy.arg1!=gscTestMission.persistent) ||
            (!copy.has_arg1 && copy.arg1)) goto refuse;
        if (!Q_stricmp(copy.map,"gamefinished")) gscTestMission.finished=1;
        gscTestMission.requested=1;
        memcpy(gscTestMission.map,copy.map,copy.length+1);
        gscTestMission.map_length=copy.length;
        /* Retail invokes its serializer after these preceding writes. Keep
         * that partial intent observable, but never acknowledge serialization.
         * Omitted argument one preserves the earlier raw persistent integer. */
        if (gscTestMission.persistent) {
            GscTestError(error,capacity,"mission persistence serializer unavailable in selected diagnostic mode");
            return 0;
        }
        if(copy.kind==GSC_MISSION_CHANGELEVEL&&!GscChangeBegin(copy.map)){
            GscTestError(error,capacity,"changelevel map/request owner unavailable");return 0;
        }
    } else {
        gscTestMission.success=1; /* Also after a suppressed shared gate. */
        if(gscTestFinaleSelected && gscTestMission.requested &&
           (!G_Cod1EntityCapture(GscTestMissionPlayer(),&gscTestFinalePlayer) ||
           !Cod1MissionTransitionBegin(&gscTestFinale,id,epoch,level.time,
              gscTestMission.map,gscTestMission.persistent))) {
            GscTestError(error,capacity,"R229 duplicate or invalid pending finale");return 0;
        }
    }
    if (!GscTestMissionCurrent(id,epoch)) goto refuse;
    return 1;
refuse:
    GscTestError(error,capacity,"mission success phase invalid or map/session changed");
    return 0;
}
static int GscTestMissionFailed(void *user,uint64_t id,int logicalMs,
    const GscMissionEvent *event,char *error,size_t capacity) {
    GscMissionEvent copy;
    uint64_t epoch=gscTestMission.epoch;
    (void)user;
    if (!event) goto refuse;
    copy=*event;
    if (logicalMs<0 || copy.kind!=GSC_MISSION_FAILED ||
        (copy.phase!=GSC_MISSION_DEATH_FADE && copy.phase!=GSC_MISSION_FAILURE_MARK) ||
        !GscTestMissionEventValid(&copy) || !GscTestMissionPlayer() ||
        !GscTestMissionCurrent(id,epoch)) goto refuse;
    if (copy.phase==GSC_MISSION_DEATH_FADE) {
        if (!gscTestMission.reload_intent && !g_reloading.integer) {
            if (gscTestFinaleSelected) {
                if (gscTestFinale.phase || level.time < 0 || level.time > INT_MAX-2050 ||
                    !G_Cod1EntityCapture(GscTestMissionPlayer(), &gscTestFinalePlayer)) goto refuse;
                gscTestFinale.session=id;
                gscTestFinale.epoch=epoch;
                gscTestFinale.phase=COD1_FAILURE_PENDING;
                gscTestFinale.at=gscTestFinale.last=level.time;
                gscTestFinale.deadline=level.time+2000;
            }
            gscTestMission.fade_intent=1;
            gscTestMission.fade_start=(int)((unsigned)level.time+1000U);
            gscTestMission.fade_duration=2000;
            gscTestMission.fade_deadline=(int)((unsigned)level.time+2000U);
            gscTestMission.reload_intent=1;
        }
    } else gscTestMission.failed=1; /* Even when the fade itself was suppressed. */
    if (!GscTestMissionCurrent(id,epoch)) goto refuse;
    return 1;
refuse:
    GscTestError(error,capacity,"mission failure phase invalid or map/session changed");
    return 0;
}
