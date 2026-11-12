#ifndef CG_COD1_OBJECTIVE_STATE_H
#define CG_COD1_OBJECTIVE_STATE_H

/* The dedicated port range preserves RTCW's metadata/fog configstrings.
 * Native records can contain only a state (objective_current on empty). */
#define COD1_OBJECTIVE_CONFIGSTRING_BASE CS_COD1_OBJECTIVES

static cod1Objective_t cod1Objectives[COD1_OBJECTIVE_COUNT];
static char cod1ObjectiveRaw[COD1_OBJECTIVE_COUNT][MAX_STRING_CHARS];

/* The Q3_VM bg_lib sscanf implementation always returns zero even when it
 * fills its outputs. Validate three numeric tokens independently. */
static qboolean CG_Cod1ObjectiveNumber(const char *token) {
    qboolean digits=qfalse;
    if (*token=='+' || *token=='-') ++token;
    while (*token>='0' && *token<='9') {digits=qtrue;++token;}
    if (*token=='.') {
        ++token;
        while (*token>='0' && *token<='9') {digits=qtrue;++token;}
    }
    if (!digits) return qfalse;
    /* The VM atof deliberately has no scientific-notation support; the
     * producer serializes integer coordinates, so reject unsupported forms. */
    return *token==0;
}

static qboolean CG_Cod1ObjectiveOrigin(const char *text,vec3_t result) {
    char buffer[128], *cursor, *token;
    int i;
    if (!*text || strlen(text)>=sizeof(buffer)) return qfalse;
    Q_strncpyz(buffer,text,sizeof(buffer));
    cursor=buffer;
    for (i=0;i<3;i++) {
        token=COM_Parse(&cursor);
        if (!token[0] || !CG_Cod1ObjectiveNumber(token)) return qfalse;
        result[i]=(float)atof(token); /* consume before COM_Parse overwrites token */
    }
    return !COM_Parse(&cursor)[0];
}

void CG_Cod1ObjectivesReset(void) {
    int i;
    memset(cod1Objectives,0,sizeof(cod1Objectives));
    memset(cod1ObjectiveRaw,0,sizeof(cod1ObjectiveRaw));
    for (i=0; i<COD1_OBJECTIVE_COUNT; ++i)
        cod1Objectives[i].changeTime=-1;
}

static void CG_Cod1ObjectiveParse(int index,const char *raw) {
    cod1Objective_t *objective=&cod1Objectives[index];
    const char *value;
    int state;
    int oldState=objective->state;
    int oldRing=objective->ring;

    if (!raw || strlen(raw)>=MAX_STRING_CHARS) {
        memset(objective,0,sizeof(*objective));
        objective->changeTime=-1;
        objective->ring=0;
        return;
    }
    value=Info_ValueForKey(raw,"state");
    /* gamex86 publishes only the single-digit numeric states 0..5. */
    state=(value[0]>='0' && value[0]<='5' && !value[1]) ? value[0]-'0' : 0;
    objective->state=state;
    if (objective->state==0) {
        objective->text[0]=0;
        VectorClear(objective->origin);
        objective->hasOrigin=qfalse;
        objective->changeTime=-1;
        objective->ring=0;
        return;
    }
    value=Info_ValueForKey(raw,"str");
    Q_strncpyz(objective->text,value,sizeof(objective->text));
    value=Info_ValueForKey(raw,"org");
    objective->hasOrigin=CG_Cod1ObjectiveOrigin(value,objective->origin);
    if (!objective->hasOrigin) VectorClear(objective->origin);
    objective->ring=atoi(Info_ValueForKey(raw,"ring"));
    if ((oldState!=4 && objective->state==4) || oldRing!=objective->ring)
        objective->changeTime=cg.time;
}

void CG_Cod1ObjectivesConfigStringModified(int number) {
    int index=number-COD1_OBJECTIVE_CONFIGSTRING_BASE;
    const char *raw;
    if (index<0 || index>=COD1_OBJECTIVE_COUNT) return;
    raw=CG_ConfigString(number);
    if (!raw) raw="";
    if (strcmp(cod1ObjectiveRaw[index],raw)) {
        Q_strncpyz(cod1ObjectiveRaw[index],raw,sizeof(cod1ObjectiveRaw[index]));
        CG_Cod1ObjectiveParse(index,raw);
    }
}

void CG_Cod1ObjectivesInit(void) {
    int i;
    for (i=0;i<COD1_OBJECTIVE_COUNT;++i)
        CG_Cod1ObjectivesConfigStringModified(COD1_OBJECTIVE_CONFIGSTRING_BASE+i);
}

const cod1Objective_t *CG_Cod1ObjectiveAt(int index) {
    if (index<0 || index>=COD1_OBJECTIVE_COUNT) return NULL;
    /* Lazy initial gamestate population shares the update route. */
    CG_Cod1ObjectivesConfigStringModified(COD1_OBJECTIVE_CONFIGSTRING_BASE+index);
    return cod1Objectives[index].state ? &cod1Objectives[index] : NULL;
}

#endif /* CG_COD1_OBJECTIVE_STATE_H */
