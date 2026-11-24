/* Genuine synchronous GAME queries, selected by the R130 mode30 constructor.
 * GetTime returns the authoritative signed32 level.time, not session elapsed
 * now. The latter is only an ownership/clock guard. Strings use a one-byte oversize probe: a truncated
 * valid <=127 prefix cannot pass for a longer cvar value. Integer and float
 * services read actual engine cached fields; no string parser substitutes. */
static int gscTestNativeQueryBusy;
static int GscTestNativeQueryEmpty(const char *value,size_t size) {
    size_t n;for(n=0;n<size;n++)if(value[n])return 0;return 1;
}
static int GscTestNativeQuerySame(GscSession *session,uint64_t id,uint64_t epoch,int mode,int now,int hostTime) {
    GscSessionTiming view;
    return session && session==gscTestSession && id && id==gscTestId && epoch &&
        epoch==G_Cod1EntityMapEpoch() && mode==gscTestEntityMode &&
        (mode==29||(mode==30 || (mode==31 || (mode==32 || (mode==33 || (mode==34 || mode==35 || mode==36 || mode==37 || mode==38 || mode==39 || (mode==40||(mode==41||(mode==42||mode==43))))))))) && now>=0 &&
        !gscTestStopRequested && !gscTestCompleted && level.time==hostTime &&
        gsc_session_timing(session,&view) &&
        view.state==GSC_SESSION_RUNNING && view.session_id==id &&
        view.logical_ms==now && G_Cod1MapEntities() &&
        g_gametype.integer==GT_SINGLE_PLAYER && !level.restarted;
}
static int GscTestNativeQueryCurrent(GscSession *session,uint64_t id,uint64_t epoch,int mode,int now,int hostTime) {
    return GscTestNativeQuerySame(session,id,epoch,mode,now,hostTime) &&
        GscTestModuleReadCurrent() && GscTestNativeQuerySame(session,id,epoch,mode,now,hostTime);
}
static int GscTestNativeQuery(void *user,uint64_t id,int now,
    GscNativeQuery *query,char *error,size_t capacity) {
    GscNativeQuery before,result;char value[129];const char *end;size_t length;
    GscSession *session;uint64_t epoch;int mode,hostTime,difficulty=-1;uintptr_t queryAt,errorAt;
    static const char *difficultyNames[4]={"easy","medium","hard","fu"};
    (void)user;
    if(query && error && capacity) {
        queryAt=(uintptr_t)query;errorAt=(uintptr_t)error;
        if(errorAt>=queryAt?errorAt-queryAt<sizeof(*query):queryAt-errorAt<capacity)return 0;
    }
    if(gscTestNativeQueryBusy) {
        GscTestError(error,capacity,"native cvar query callback reentry refused");return 0;
    }
    gscTestNativeQueryBusy=1;session=gscTestSession;epoch=G_Cod1EntityMapEpoch();mode=gscTestEntityMode;hostTime=level.time;
    if(!query || !query->operation_id ||
       !((query->kind>=GSC_QUERY_CVAR_STRING&&query->kind<=GSC_QUERY_CVAR_FLOAT) ||
         query->kind==GSC_QUERY_LEVEL_TIME || query->kind==GSC_QUERY_DIFFICULTY) ||
       !memchr(query->name,0,sizeof(query->name)) || query->integer_value ||
       query->float_value!=0 || query->string_value[0] ||
       (query->kind==GSC_QUERY_LEVEL_TIME &&
        ((mode!=30 && (mode!=31 && (mode!=32 && (mode!=33 && (mode!=34 && mode!=35 && (mode!=36 && (mode!=37 && (mode!=38 && (mode!=39 && (mode!=40&&(mode!=41&&(mode!=42&&mode!=43)))))))))))) || !GscTestNativeQueryEmpty(query->name,sizeof(query->name)) ||
         !GscTestNativeQueryEmpty(query->string_value,sizeof(query->string_value)))) ||
       (query->kind==GSC_QUERY_DIFFICULTY &&
        ((mode!=36 && (mode!=37 && (mode!=38 && (mode!=39 && (mode!=40&&(mode!=41&&(mode!=42&&mode!=43))))))) || !GscTestNativeQueryEmpty(query->name,sizeof(query->name)) ||
         !GscTestNativeQueryEmpty(query->string_value,sizeof(query->string_value)))))goto refuse;
    before=*query;result=before;
    if(!GscTestNativeQueryCurrent(session,id,epoch,mode,now,hostTime) ||
       query->operation_id!=before.operation_id || query->kind!=before.kind ||
       memcmp(query->name,before.name,sizeof(before.name)) || query->integer_value!=before.integer_value ||
       query->float_value!=before.float_value || memcmp(query->string_value,before.string_value,sizeof(before.string_value)))goto refuse;
    if(before.kind==GSC_QUERY_DIFFICULTY) {
        if(!G_Cod1EntityCollectionReady(epoch,level.num_entities))goto refuse;
        difficulty=g_gameskill.integer;
        if(difficulty<0 || difficulty>=4)goto refuse;
        memcpy(result.string_value,difficultyNames[difficulty],strlen(difficultyNames[difficulty])+1);
        if(!GscTestNativeQueryCurrent(session,id,epoch,mode,now,hostTime) ||
           !G_Cod1EntityCollectionReady(epoch,level.num_entities) ||
           g_gameskill.integer!=difficulty)goto refuse;
    } else if(before.kind==GSC_QUERY_LEVEL_TIME) {
        result.integer_value=(int32_t)hostTime;
        if(!GscTestNativeQueryCurrent(session,id,epoch,mode,now,hostTime))goto refuse;
    } else if(before.kind==GSC_QUERY_CVAR_STRING) {
        memset(value,0xa5,sizeof(value));
        trap_Cvar_VariableStringBuffer(before.name,value,sizeof(value));
        if(!GscTestNativeQueryCurrent(session,id,epoch,mode,now,hostTime))goto refuse;
        end=memchr(value,0,sizeof(value));if(!end)goto refuse;
        length=(size_t)(end-value);if(length>=sizeof(result.string_value))goto refuse;
        memcpy(result.string_value,value,length+1);
    } else if(before.kind==GSC_QUERY_CVAR_INTEGER) {
        result.integer_value=(int32_t)trap_Cvar_VariableIntegerValue(before.name);
        if(!GscTestNativeQueryCurrent(session,id,epoch,mode,now,hostTime))goto refuse;
    } else {
        result.float_value=trap_Cod1CvarValue(before.name);
        if(!GscTestNativeQueryCurrent(session,id,epoch,mode,now,hostTime) || !isfinite(result.float_value))goto refuse;
    }
    if(query->operation_id!=before.operation_id || query->kind!=before.kind ||
       memcmp(query->name,before.name,sizeof(before.name)) || query->integer_value!=before.integer_value ||
       query->float_value!=before.float_value || memcmp(query->string_value,before.string_value,sizeof(before.string_value)))goto refuse;
    *query=result;gscTestNativeQueryBusy=0;return 1;
refuse:
    gscTestNativeQueryBusy=0;
    GscTestError(error,capacity,"native cvar query invalid, oversized or cancelled at engine read boundary");
    return 0;
}
