/* R109 map-owned objective records. Dedicated configstrings are the server
 * authority; the private copy records only what this producer published.
 * Completing/stopping the diagnostic VM does not delete mission objectives.
 * A real map/load/shutdown boundary invalidates callbacks before clearing. */
static char gscTestObjectiveRecords[MAX_COD1_OBJECTIVES][MAX_STRING_CHARS];
static uint64_t gscTestObjectiveEpoch;
static int gscTestObjectiveOwned, gscTestObjectiveMessageDeadline;

static void GscTestObjectiveReset(void) {
    int i, owned=gscTestObjectiveOwned;
    uint64_t epoch;
    if (gscTestObjectiveEpoch!=UINT64_MAX) ++gscTestObjectiveEpoch;
    epoch=gscTestObjectiveEpoch;
    gscTestObjectiveOwned=0;
    gscTestObjectiveMessageDeadline=0;
    memset(gscTestObjectiveRecords,0,sizeof(gscTestObjectiveRecords));
    if (!owned) return; /* Older modes retain their trap sequence. */
    for (i=0;i<MAX_COD1_OBJECTIVES;++i) {
        trap_SetConfigstring(CS_COD1_OBJECTIVES+i,"");
        if (epoch!=gscTestObjectiveEpoch || gscTestObjectiveOwned) return;
    }
}

static int GscTestObjectiveBegin(void) {
    int i;
    uint64_t epoch,previous;
    if ((gscTestEntityMode!=16 && gscTestEntityMode!=17 && gscTestEntityMode!=18 && (gscTestEntityMode!=19 && gscTestEntityMode!=20 && gscTestEntityMode!=21 && gscTestEntityMode!=22 && gscTestEntityMode!=23 && gscTestEntityMode!=24 && gscTestEntityMode!=25 && (gscTestEntityMode!=26 && gscTestEntityMode!=27 && (gscTestEntityMode!=28 && (gscTestEntityMode!=29 && (gscTestEntityMode!=30 && (gscTestEntityMode!=31 && (gscTestEntityMode!=32 && (gscTestEntityMode!=33 && (gscTestEntityMode!=34 && (gscTestEntityMode!=35 && (gscTestEntityMode!=36 && (gscTestEntityMode!=37 && (gscTestEntityMode!=38 && (gscTestEntityMode!=39 && (gscTestEntityMode!=40&&(gscTestEntityMode!=41&&(gscTestEntityMode!=42&&gscTestEntityMode!=43)))))))))))))))))) || !G_Cod1MapEntities() ||
        g_gametype.integer!=GT_SINGLE_PLAYER || level.restarted ||
        !GscTestModuleReadCurrent()) return 0;
    previous=gscTestObjectiveEpoch;
    GscTestObjectiveReset();
    if (previous==UINT64_MAX || gscTestObjectiveEpoch!=previous+1 ||
        gscTestObjectiveOwned) return 0;
    if (gscTestStopRequested || !GscTestModuleReadCurrent()) return 0;
    if (gscTestObjectiveEpoch==UINT64_MAX) return 0;
    ++gscTestObjectiveEpoch;
    if (gscTestObjectiveEpoch==UINT64_MAX) return 0;
    epoch=gscTestObjectiveEpoch;
    gscTestObjectiveOwned=1;
    for (i=0;i<MAX_COD1_OBJECTIVES;++i) {
        if (epoch!=gscTestObjectiveEpoch || !gscTestObjectiveOwned ||
            !GscTestModuleReadCurrent()) return 0;
        trap_SetConfigstring(CS_COD1_OBJECTIVES+i,"");
    }
    return epoch==gscTestObjectiveEpoch && gscTestObjectiveOwned &&
        GscTestModuleReadCurrent();
}

static int GscTestObjectiveCurrent(uint64_t id,uint64_t epoch) {
    return id && id==gscTestId && epoch==gscTestObjectiveEpoch &&
        gscTestObjectiveOwned && (gscTestEntityMode==16 || gscTestEntityMode==17 || gscTestEntityMode==18||(gscTestEntityMode==19 || gscTestEntityMode==20 || gscTestEntityMode==21 || (gscTestEntityMode==22 || gscTestEntityMode==23 || gscTestEntityMode==24 || gscTestEntityMode==25 || (gscTestEntityMode==26 || (gscTestEntityMode==27 || (gscTestEntityMode==28 || (gscTestEntityMode==29 || (gscTestEntityMode==30 || (gscTestEntityMode==31 || (gscTestEntityMode==32 || (gscTestEntityMode==33 || (gscTestEntityMode==34 || gscTestEntityMode==35 || gscTestEntityMode==36 || gscTestEntityMode==37 || gscTestEntityMode==38 || gscTestEntityMode==39 || (gscTestEntityMode==40||(gscTestEntityMode==41||(gscTestEntityMode==42||gscTestEntityMode==43))))))))))))))) &&
        !gscTestStopRequested && G_Cod1MapEntities() &&
        g_gametype.integer==GT_SINGLE_PLAYER && !level.restarted &&
        GscTestModuleReadCurrent();
}

/* Retail objective wrappers reject these value characters before InfoSet.
 * Match key removal and append order, while bounding the terminator as well
 * as the body. Retail's exactly-1024-body edge is refused in this port. */
static void GscTestObjectiveRemoveKey(char *raw,const char *key) {
    char *cursor=raw,*start,*name;
    size_t length=strlen(key);
    while (*cursor) {
        start=cursor;
        if (*cursor=='\\') ++cursor;
        name=cursor;
        while (*cursor && *cursor!='\\') ++cursor;
        if (!*cursor) return;
        if ((size_t)(cursor-name)==length && !memcmp(name,key,length)) {
            ++cursor;
            while (*cursor && *cursor!='\\') ++cursor;
            /* Native removal compares exact bytes and removes the first
             * matching pair. The retained shared helper used strcpy here. */
            memmove(start,cursor,strlen(cursor)+1);
            return;
        }
        ++cursor;
        while (*cursor && *cursor!='\\') ++cursor;
    }
}

static int GscTestObjectiveSet(char *raw,const char *key,const char *value) {
    size_t n,k,v;
    if (strchr(value,'\\') || strchr(value,';') || strchr(value,'"')) return 0;
    GscTestObjectiveRemoveKey(raw,key);
    if (!value[0]) return 1;
    n=strlen(raw);k=strlen(key);v=strlen(value);
    if (n+k+v+2>=MAX_STRING_CHARS) return 0;
    raw[n++]='\\';memcpy(raw+n,key,k);n+=k;
    raw[n++]='\\';memcpy(raw+n,value,v);raw[n+v]=0;
    return 1;
}

static int GscTestObjectiveRead(uint64_t id,uint64_t epoch,int index,char *raw) {
    trap_GetConfigstring(CS_COD1_OBJECTIVES+index,raw,MAX_STRING_CHARS);
    raw[MAX_STRING_CHARS-1]=0;
    return GscTestObjectiveCurrent(id,epoch);
}

static int GscTestObjectivePublish(uint64_t id,uint64_t epoch,int index,
                                    const char *raw) {
    if (!GscTestObjectiveCurrent(id,epoch)) return 0;
    Q_strncpyz(gscTestObjectiveRecords[index],raw,MAX_STRING_CHARS);
    trap_SetConfigstring(CS_COD1_OBJECTIVES+index,raw);
    return GscTestObjectiveCurrent(id,epoch);
}

static int GscTestObjectiveMessage(uint64_t id,uint64_t epoch,int state) {
    const char *command;
    if (state==1 || state==4)
        command="obj_update \"GAME_OBJECTIVESUPDATED\"";
    else if (state==3)
        command="obj_complete \"GAME_OBJECTIVECOMPLETED\"";
    else if (state==5)
        command="obj_update \"GAME_OBJECTIVEFAILED\"";
    else return 1;
    if (!GscTestObjectiveCurrent(id,epoch)) return 0;
    if (gscTestObjectiveMessageDeadline>level.time) return 1;
    /* Retail stores a wrapping signed int deadline. Avoid C signed overflow. */
    gscTestObjectiveMessageDeadline=(int)((unsigned)level.time+100U);
    trap_SendServerCommand(-1,command);
    return GscTestObjectiveCurrent(id,epoch);
}

static int GscTestObjective(void *user,uint64_t id,int logicalMs,
    const GscObjectiveEvent *event,char *error,size_t capacity,int kind) {
    char raw[MAX_STRING_CHARS],value[96];
    uint64_t epoch=gscTestObjectiveEpoch;
    int i,state;
    (void)user;
    if (!event || event->kind!=kind || logicalMs<0 ||
        !GscTestObjectiveCurrent(id,epoch)) goto refuse;
    if (kind==GSC_OBJECTIVE_CURRENT) {
        if (event->current_mask&~0xffffU) goto refuse;
        for (i=0;i<MAX_COD1_OBJECTIVES;++i) {
            if (!GscTestObjectiveRead(id,epoch,i,raw)) goto refuse;
            state=atoi(Info_ValueForKey(raw,"state"));
            if (event->current_mask&(1U<<i)) {
                if (state==4) continue;
                if (!GscTestObjectiveSet(raw,"state","4")) goto refuse;
            } else {
                if (state!=4) continue;
                if (!GscTestObjectiveSet(raw,"state","1")) goto refuse;
            }
            if (!GscTestObjectivePublish(id,epoch,i,raw)) goto refuse;
        }
        if (!GscTestObjectiveMessage(id,epoch,1)) goto refuse;
        return 1;
    }
    if (event->index<0 || event->index>=MAX_COD1_OBJECTIVES ||
        !GscTestObjectiveRead(id,epoch,event->index,raw)) goto refuse;
    if (kind==GSC_OBJECTIVE_DELETE) {
        if (!GscTestObjectivePublish(id,epoch,event->index,"")) goto refuse;
        return 1;
    }
    if (kind==GSC_OBJECTIVE_ADD || kind==GSC_OBJECTIVE_STATE) {
        if (event->state<0 || event->state>5) goto refuse;
        value[0]=(char)('0'+event->state);value[1]=0;
        if (!GscTestObjectiveSet(raw,"state",value)) goto refuse;
    }
    if (kind==GSC_OBJECTIVE_ADD) {
        if ((event->has_text!=0 && event->has_text!=1) ||
            (event->has_origin!=0 && event->has_origin!=1)) goto refuse;
    }
    if ((kind==GSC_OBJECTIVE_ADD && event->has_text) ||
        kind==GSC_OBJECTIVE_STRING || kind==GSC_OBJECTIVE_STRING_NOMESSAGE) {
        if (!memchr(event->text,0,sizeof(event->text))) goto refuse;
        if (!GscTestObjectiveSet(raw,"str",event->text)) goto refuse;
    }
    if ((kind==GSC_OBJECTIVE_ADD && event->has_origin) ||
        kind==GSC_OBJECTIVE_POSITION) {
        snprintf(value,sizeof(value),"%i %i %i",event->origin[0],
            event->origin[1],event->origin[2]);
        if (!GscTestObjectiveSet(raw,"org",value)) goto refuse;
    }
    if (kind==GSC_OBJECTIVE_RING) {
        if (!GscTestObjectiveSet(raw,"ring",atoi(Info_ValueForKey(raw,"ring"))?"0":"1")) goto refuse;
    }
    if (!GscTestObjectivePublish(id,epoch,event->index,raw)) goto refuse;
    if (kind==GSC_OBJECTIVE_ADD || kind==GSC_OBJECTIVE_STATE ||
        kind==GSC_OBJECTIVE_STRING) {
        /* Add/state announce the supplied enum even when a foreign differently
         * cased key wins the native case-folded lookup in the resulting CS. */
        state=kind==GSC_OBJECTIVE_STRING ? atoi(Info_ValueForKey(raw,"state")) : event->state;
        if (!GscTestObjectiveMessage(id,epoch,state)) goto refuse;
    }
    return 1;
refuse:
    GscTestError(error,capacity,"objective event invalid or map/session changed");
    return 0;
}

#define GSC_OBJECTIVE_ADAPTER(name,kind) \
static int name(void *user,uint64_t id,int logicalMs,const GscObjectiveEvent *event, \
                char *error,size_t capacity) { \
    return GscTestObjective(user,id,logicalMs,event,error,capacity,kind); \
}
GSC_OBJECTIVE_ADAPTER(GscTestObjectiveAdd,GSC_OBJECTIVE_ADD)
GSC_OBJECTIVE_ADAPTER(GscTestObjectiveDelete,GSC_OBJECTIVE_DELETE)
GSC_OBJECTIVE_ADAPTER(GscTestObjectiveState,GSC_OBJECTIVE_STATE)
GSC_OBJECTIVE_ADAPTER(GscTestObjectiveString,GSC_OBJECTIVE_STRING)
GSC_OBJECTIVE_ADAPTER(GscTestObjectiveStringNoMessage,GSC_OBJECTIVE_STRING_NOMESSAGE)
GSC_OBJECTIVE_ADAPTER(GscTestObjectivePosition,GSC_OBJECTIVE_POSITION)
GSC_OBJECTIVE_ADAPTER(GscTestObjectiveSelect,GSC_OBJECTIVE_CURRENT)
GSC_OBJECTIVE_ADAPTER(GscTestObjectiveRing,GSC_OBJECTIVE_RING)
#undef GSC_OBJECTIVE_ADAPTER
