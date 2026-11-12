/* R131 synchronous getspawnerarray GAME adapter. Genuine registry membership
 * is copied completely; type/classname/health heuristics never manufacture it.
 * Private VM arrays are bounded to 256, so a larger full registry is refused.
 * Ascending slot order is existing restoration policy, not retail ordering. */
#include "../g_cod1_spawner.h"
enum { GSC_TEST_SPAWNER_ARRAY_CAPACITY = 256 };
static int gscTestSpawnerArrayBusy, gscTestSpawnerArrayReentered;

static int GscTestSpawnerArrayOverlap(const void *a,size_t as,const void *b,size_t bs) {
    uintptr_t aa=(uintptr_t)a,bb=(uintptr_t)b;
    if(!a || !b || !as || !bs)return 0;
    return aa<=bb ? bb-aa<as : aa-bb<bs;
}

static int GscTestSpawnerArraySame(GscSession *session,uint64_t id,uint64_t epoch,
    int now,int hostTime,int entityCount) {
    GscSessionSnapshot view;
    return !gscTestSpawnerArrayReentered && session && session==gscTestSession &&
        id && id==gscTestId && epoch && epoch==gscTestMapEpoch &&
        epoch==G_Cod1EntityMapEpoch() && (gscTestEntityMode==31 || (gscTestEntityMode==32 || (gscTestEntityMode==33 || (gscTestEntityMode==34 || gscTestEntityMode==35 || gscTestEntityMode==36 || gscTestEntityMode==37 || gscTestEntityMode==38 || gscTestEntityMode==39 || (gscTestEntityMode==40||(gscTestEntityMode==41||(gscTestEntityMode==42||gscTestEntityMode==43))))))) &&
        !gscTestStopRequested && !gscTestCompleted && !level.restarted &&
        now>=0 && level.time==hostTime && entityCount>=MAX_CLIENTS &&
        G_Cod1EntityCollectionReady(epoch,entityCount) &&
        gsc_session_snapshot(session,&view) && view.session_id==id && !view.error[0] &&
        view.state==GSC_SESSION_RUNNING && view.logical_ms==now;
}

/* Check the entire captured owner immediately before and after each trap.
 * Repeated reads remain a bounded lease check, not an atomic cvar snapshot. */
static int GscTestSpawnerArrayAdmission(GscSession *session,uint64_t id,uint64_t epoch,
    int now,int hostTime,int entityCount) {
    static const char *names[6]={"g_cod1GscTest","g_cod1GscEntityTest","savegame_loading",
        "g_cod1GscTest","savegame_loading","g_cod1GscEntityTest"};
    static const int values[6]={1,31,0,1,0,31};
    int i,value,mode=gscTestEntityMode;
    for(i=0;i<6;++i) {
        if(mode!=gscTestEntityMode || !GscTestSpawnerArraySame(session,id,epoch,now,hostTime,entityCount))return 0;
        value=trap_Cvar_VariableIntegerValue(names[i]);
        if(mode!=gscTestEntityMode || !GscTestSpawnerArraySame(session,id,epoch,now,hostTime,entityCount) ||
           value!=((i==1 || i==5)?mode:values[i]))return 0;
    }
    return 1;
}

static int GscTestSpawnerArrayMembers(const GscEntityRef *refs,size_t count,
    uint64_t epoch) {
    const GscEntityHost *entities=G_Cod1EntityHost();
    cod1SpawnerSnapshot_t typed;
    size_t i;
    if(!entities || !entities->validate || count>GSC_TEST_SPAWNER_ARRAY_CAPACITY)return 0;
    for(i=0;i<count;++i) {
        if(refs[i].epoch!=epoch || !refs[i].generation || refs[i].slot<MAX_CLIENTS ||
           refs[i].slot>=ENTITYNUM_MAX_NORMAL || (i && refs[i-1].slot>=refs[i].slot) ||
           !entities->validate(entities->user,&refs[i]) ||
           !G_Cod1SpawnerRead(&refs[i],&typed,NULL,0) ||
           typed.entity.epoch!=refs[i].epoch || typed.entity.slot!=refs[i].slot ||
           typed.entity.generation!=refs[i].generation)return 0;
    }
    return 1;
}

static int GscTestSpawnerArrayCollect(void *user,uint64_t id,int now,
    GscEntityRef *out,size_t capacity,size_t *count,char *error,size_t errorSize) {
    GscEntityRef first[GSC_TEST_SPAWNER_ARRAY_CAPACITY],second[GSC_TEST_SPAWNER_ARRAY_CAPACITY];
    GscSession *session;
    uint64_t epoch;
    size_t firstCount=0,secondCount=0,outputBytes;
    int hostTime,entityCount,accepted=0,overlappingError;
    (void)user;
    /* Alias refusal covers the whole declared range even for malformed capacity;
     * subtraction-based overlap never dereferences that range or wraps its end. */
    outputBytes=capacity>SIZE_MAX/sizeof(*out) ? SIZE_MAX : capacity*sizeof(*out);
    overlappingError=GscTestSpawnerArrayOverlap(out,outputBytes,error,errorSize) ||
        GscTestSpawnerArrayOverlap(count,sizeof(*count),error,errorSize);
    if(gscTestSpawnerArrayBusy) {
        gscTestSpawnerArrayReentered=1;
        if(!overlappingError)GscTestError(error,errorSize,"spawner array collection reentered");
        return 0;
    }
    if(overlappingError)return 0;
    if(!out || !count || capacity!=GSC_TEST_SPAWNER_ARRAY_CAPACITY ||
       GscTestSpawnerArrayOverlap(out,outputBytes,count,sizeof(*count))) {
        GscTestError(error,errorSize,"spawner array requires separate copied outputs and capacity 256");return 0;
    }
    session=gscTestSession;epoch=G_Cod1EntityMapEpoch();hostTime=level.time;entityCount=level.num_entities;
    gscTestSpawnerArrayBusy=1;gscTestSpawnerArrayReentered=0;
    memset(first,0,sizeof(first));memset(second,0,sizeof(second));
    /* Guard the capability before consulting the registry, then preserve
     * the complete first membership across the second six-read round. */
    if(!GscTestSpawnerArraySame(session,id,epoch,now,hostTime,entityCount) ||
       !GscTestSpawnerArrayAdmission(session,id,epoch,now,hostTime,entityCount) ||
       !G_Cod1SpawnerCollectAll(first,capacity,&firstCount,NULL,0) ||
       !GscTestSpawnerArrayMembers(first,firstCount,epoch) ||
       !GscTestSpawnerArrayAdmission(session,id,epoch,now,hostTime,entityCount))goto end;
    /* A callback may add/remove a member while retaining map and entity count.
     * Compare the complete second typed membership before output publication. */
    if(!G_Cod1SpawnerCollectAll(second,capacity,&secondCount,NULL,0) ||
       firstCount!=secondCount || memcmp(first,second,firstCount*sizeof(*first)) ||
       !GscTestSpawnerArrayMembers(second,secondCount,epoch) ||
       !GscTestSpawnerArraySame(session,id,epoch,now,hostTime,entityCount))goto end;
    if(firstCount)memcpy(out,first,firstCount*sizeof(*out));
    *count=firstCount;accepted=1;
end:
    gscTestSpawnerArrayBusy=0;gscTestSpawnerArrayReentered=0;
    if(!accepted)GscTestError(error,errorSize,"spawner array refused: unavailable/full registry, stale ready map, owner, time or complete membership");
    return accepted;
}

/* R149 separate named-team capability. The legacy collect_all callback and
 * host layout remain unchanged. Complete selected membership is compared
 * across the same two admission rounds, including actual typed team values. */
static int GscTestSpawnerTeamArrayMembers(const GscEntityRef *refs,size_t count,
    uint64_t epoch,unsigned int mask) {
    size_t i;cod1SpawnerSnapshot_t typed;
    if(!mask||(mask&~14u)||!GscTestSpawnerArrayMembers(refs,count,epoch))return 0;
    for(i=0;i<count;i++) {
        if(!G_Cod1SpawnerRead(&refs[i],&typed,NULL,0)||
           typed.team<1||typed.team>3||!(mask&(1u<<typed.team)))return 0;
    }
    return 1;
}
static int GscTestSpawnerTeamArrayCollect(void *user,uint64_t id,int now,
    unsigned int mask,GscEntityRef *out,size_t capacity,size_t *count,
    char *error,size_t errorSize) {
    GscEntityRef first[GSC_TEST_SPAWNER_ARRAY_CAPACITY],second[GSC_TEST_SPAWNER_ARRAY_CAPACITY];
    GscSession *session;uint64_t epoch;size_t firstCount=0,secondCount=0,outputBytes;
    int hostTime,entityCount,accepted=0,overlappingError;
    (void)user;
    outputBytes=capacity>SIZE_MAX/sizeof(*out)?SIZE_MAX:capacity*sizeof(*out);
    overlappingError=GscTestSpawnerArrayOverlap(out,outputBytes,error,errorSize)||
        GscTestSpawnerArrayOverlap(count,sizeof(*count),error,errorSize);
    if(gscTestSpawnerArrayBusy) {
        gscTestSpawnerArrayReentered=1;
        if(!overlappingError)GscTestError(error,errorSize,"spawner team array collection reentered");
        return 0;
    }
    if(overlappingError)return 0;
    if(!out||!count||capacity!=GSC_TEST_SPAWNER_ARRAY_CAPACITY||!mask||(mask&~14u)||
       GscTestSpawnerArrayOverlap(out,outputBytes,count,sizeof(*count))) {
        GscTestError(error,errorSize,"spawner team array requires valid named mask and separate capacity-256 outputs");return 0;
    }
    session=gscTestSession;epoch=G_Cod1EntityMapEpoch();hostTime=level.time;entityCount=level.num_entities;
    gscTestSpawnerArrayBusy=1;gscTestSpawnerArrayReentered=0;
    memset(first,0,sizeof(first));memset(second,0,sizeof(second));
    if((gscTestEntityMode!=37 && (gscTestEntityMode!=38 && (gscTestEntityMode!=39 && (gscTestEntityMode!=40&&(gscTestEntityMode!=41&&(gscTestEntityMode!=42&&gscTestEntityMode!=43))))))||!GscTestSpawnerArraySame(session,id,epoch,now,hostTime,entityCount)||
       !GscTestSpawnerArrayAdmission(session,id,epoch,now,hostTime,entityCount)||
       !G_Cod1SpawnerCollectTeams(mask,first,capacity,&firstCount,NULL,0)||
       !GscTestSpawnerTeamArrayMembers(first,firstCount,epoch,mask)||
       !GscTestSpawnerArrayAdmission(session,id,epoch,now,hostTime,entityCount))goto end;
    if(!G_Cod1SpawnerCollectTeams(mask,second,capacity,&secondCount,NULL,0)||
       firstCount!=secondCount||memcmp(first,second,firstCount*sizeof(*first))||
       !GscTestSpawnerTeamArrayMembers(second,secondCount,epoch,mask)||
       (gscTestEntityMode!=37 && (gscTestEntityMode!=38 && (gscTestEntityMode!=39 && (gscTestEntityMode!=40&&(gscTestEntityMode!=41&&(gscTestEntityMode!=42&&gscTestEntityMode!=43))))))||!GscTestSpawnerArraySame(session,id,epoch,now,hostTime,entityCount))goto end;
    if(firstCount)memcpy(out,first,firstCount*sizeof(*out));
    *count=firstCount;accepted=1;
end:
    gscTestSpawnerArrayBusy=0;gscTestSpawnerArrayReentered=0;
    if(!accepted)GscTestError(error,errorSize,"spawner team array refused: unavailable/full registry, stale map, owner, time or membership");
    return accepted;
}
