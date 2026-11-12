#ifndef CG_COD1_OBJECTIVES_H
#define CG_COD1_OBJECTIVES_H

/* Retail cgame 0x3000e080/0x3000e0a0 exposes the +scores objective sheet;
 * 0x3001f570..0x3001f931 draws the 16 game-driven records and skips center
 * print while visible. Checkbox materials and the title are original assets.
 * Text wrapping uses this port's font dimensions until in-game comparison. */
static qhandle_t cod1ObjectiveCheckbox[3];

/* The executed retail script helper emits ASCII 20/21 between translatable
 * and literal segments. Resolve mission StringEd packages for every key.
 * Literal %s/%% expansion is this port's bounded display bridge; the retail
 * engine's complete StringEd decoder has not yet been executed or matched. */
static const char *CG_Cod1LocalizeComposed(const char *text,int preserveLiteral) {
    static char output[4096];
    char encoded[MAX_STRING_CHARS];
    const char *parts[18],*format;
    unsigned char literal[18];
    size_t length,used=0;
    int count=1,i=0,j;
    if (!text) return "";
    length=strlen(text);
    if (length>=sizeof(encoded)) return "";
    memcpy(encoded,text,length+1);
    parts[0]=encoded;literal[0]=0;
    for (j=0;j<(int)length;++j) {
        unsigned char ch=(unsigned char)encoded[j];
        if (ch!=20 && ch!=21) continue;
        encoded[j]=0;
        if (count>=(int)(sizeof(parts)/sizeof(parts[0]))) return "";
        parts[count]=encoded+j+1;literal[count]=(unsigned char)(ch==21);++count;
    }
    for (j=0;j<count;++j) {
        const char *translated=literal[j]?NULL:CG_Cod1LocaleLookupObjective(parts[j]);
        if (translated) parts[j]=translated;
        else if (preserveLiteral) literal[j]=1;
    }
    while (i<count && used+1<sizeof(output)) {
        int expand=!preserveLiteral || !literal[i];
        format=parts[i++];
        while (*format && used+1<sizeof(output)) {
            if (expand && *format=='%' && format[1]=='s' && i<count) {
                const char *parameter=parts[i++];
                while (*parameter && used+1<sizeof(output)) {
                    unsigned char ch=(unsigned char)*parameter++;
                    if (ch>=32 || ch=='\n' || ch=='\r' || ch=='\t') output[used++]=(char)ch;
                }
                format+=2;
            } else if (expand && *format=='%' && format[1]=='%') {
                output[used++]='%';format+=2;
            } else {
                unsigned char ch=(unsigned char)*format++;
                if (ch>=32 || ch=='\n' || ch=='\r' || ch=='\t') output[used++]=(char)ch;
            }
        }
        if (preserveLiteral && *format) return "";
    }
    if (preserveLiteral && i<count) return "";
    output[used]=0;
    return output;
}

const char *CG_Cod1ObjectiveLocalize(const char *text) {
    return CG_Cod1LocalizeComposed(text,0);
}

/* gm/gmb carry the same script IString segments as objective text. Only
 * translated resource templates consume %s/%%; ordinary script bytes are
 * literal. This bounded display bridge is shared with the objective path;
 * the original engine's syscall-58 decoder and queue remain unexecuted. */
const char *CG_Cod1MessageLocalize(const char *text) {
    static const char *legacyKeys[] = {
        "GAME_OBJECTIVESUPDATED", "GAME_OBJECTIVECOMPLETED", "GAME_OBJECTIVEFAILED",
        "GAME_PICKUP_HEALTH", "GAME_PICKUP_AMMO", "GAME_PICKUP_CLIPONLY_AMMO",
        "GAME_PICKUP_CANTCARRYMOREAMMO", "GAME_CANT_GET_PRIMARY_WEAP_MESSAGE",
        "GAME_CANT_GET_PISTOL_WEAP_MESSAGE", "GAME_CANT_GET_GRENADE_WEAP_MESSAGE",
        "GAME_CANT_GET_SMOKER_WEAP_MESSAGE"
    };
    int i;
    /* Preserve the prior known GAME_* fallbacks when their assets are
     * absent. Explicit script segment markers must never take this path.
     * Alphabetic native bodies can omit a first marker, so the wire format
     * does not distinguish a bare literal key from a localized reference. */
    if (text && !strchr(text,20) && !strchr(text,21) &&
        !CG_Cod1LocaleLookupObjective(text)) {
        for (i=0;i<(int)(sizeof(legacyKeys)/sizeof(legacyKeys[0]));++i)
            if (!Q_stricmp(text,legacyKeys[i])) return CG_Cod1PromptLocalize(text);
    }
    return CG_Cod1LocalizeComposed(text,1);
}

static float CG_Cod1ObjectiveText(float x,float y,float maxWidth,const char *text,
                                    float alpha,const float *rgb) {
    char line[1024];
    int at=0, space=0;
    float height=22.0f;
    const char *p=text;
    if (!text || !*text) return height;
    while (*p && y<456.0f) {
        unsigned char ch=(unsigned char)*p++;
        if (ch=='\r') continue;
        if (ch=='\n') {
            line[at]=0;
            CG_Cod1PromptText(x,y,line,.28f,alpha,rgb);
            y+=height;at=0;space=0;
            continue;
        }
        if (at>=(int)sizeof(line)-2) break;
        line[at++]=(char)ch;
        line[at]=0;
        if (ch==' ') space=at;
        if (CG_Cod1PromptTextWidth(line,.28f)>maxWidth && at>1) {
            int tail=0;
            if (space>0) {
                tail=at-space;
                line[space-1]=0;
            } else {
                tail=1;
                line[at-1]=0;
            }
            CG_Cod1PromptText(x,y,line,.28f,alpha,rgb);
            y+=height;
            if (tail) {
                if (space>0) memmove(line,line+space,(size_t)tail);
                else line[0]=(char)ch;
            }
            at=tail;
            line[at]=0;
            space=0;
        }
    }
    if (at && y<465.0f) CG_Cod1PromptText(x,y,line,.28f,alpha,rgb);
    return y+height;
}

static qboolean CG_Cod1ObjectivesDraw(void) {
    static const float titleColor[3]={1.0f,1.0f,1.0f};
    static const float activeColor[3]={.88f,.88f,.88f};
    static const float completedColor[3]={.50f,1.0f,.50f};
    static const float failedColor[3]={1.0f,.50f,.50f};
    vec4_t divider={.6f,.6f,.6f,.7f};
    float alpha,y,width;
    qboolean native=qfalse,hasText=qfalse;
    int i;

    if (!cg.snap || cg_paused.integer || cg.snap->ps.pm_type==PM_INTERMISSION)
        return qfalse;
    if (cg.showScores) alpha=1.0f;
    else if (cg.scoreFadeTime>0 && cg.time>=cg.scoreFadeTime &&
             cg.time-cg.scoreFadeTime<100)
        alpha=1.0f-(cg.time-cg.scoreFadeTime)*.01f;
    else return qfalse;

    if (!cod1ObjectiveCheckbox[0]) {
        cod1ObjectiveCheckbox[0]=trap_R_RegisterShaderNoMip("ui/assets/checkbox_clear");
        cod1ObjectiveCheckbox[1]=trap_R_RegisterShaderNoMip("ui/assets/checkbox_checked");
        cod1ObjectiveCheckbox[2]=trap_R_RegisterShaderNoMip("ui/assets/checkbox_fail");
    }
    width=CodHud_Width();
    if (width<300.0f) width=640.0f;
    { const char *title=CG_Cod1LocaleLookup("CGAME_MISSIONOBJECTIVES");
      CG_Cod1PromptText(20,149,title?title:"Mission Objectives",.45f,alpha,titleColor); }
    divider[3]*=alpha;
    CodHud_Rect(20,150,width-40,1,divider);
    y=176;

    for (i=0;i<COD1_OBJECTIVE_COUNT;i++) {
        const cod1Objective_t *objective=CG_Cod1ObjectiveAt(i);
        if (objective) native=qtrue;
    }
    if (native) {
        for (i=0;i<COD1_OBJECTIVE_COUNT && y<452;i++) {
            const cod1Objective_t *objective=CG_Cod1ObjectiveAt(i);
            const float *color=activeColor;
            const char *translated;
            int checkbox=0;
            if (!objective || objective->state==2) continue;
            if (objective->state==3) {checkbox=1;color=completedColor;}
            else if (objective->state==5) {checkbox=2;color=failedColor;}
            if (cod1ObjectiveCheckbox[checkbox]) {
                vec4_t tint={color[0],color[1],color[2],alpha};
                trap_R_SetColor(tint);
                CodHud_Pic(20,y-17,20,20,cod1ObjectiveCheckbox[checkbox]);
                trap_R_SetColor(NULL);
            }
            translated=CG_Cod1ObjectiveLocalize(objective->text);
            if (*translated) hasText=qtrue;
            y=CG_Cod1ObjectiveText(48,y,width-75,translated,alpha,color);
            if (y<452) y+=4;
        }
    } else {
        /* RTCW's objective info is a separate six-record server format.
         * Display known descriptions when playing older script content;
         * RTCW's ownership status is not a CoD completed/failed state. */
        int count=atoi(Info_ValueForKey(CG_ConfigString(CS_MULTI_INFO),"numobjectives"));
        if (count<0) count=0;
        if (count>MAX_OBJECTIVES) count=MAX_OBJECTIVES;
        for (i=0;i<count && y<452;i++) {
            const char *raw=CG_ConfigString(CS_MULTI_OBJECTIVE1+i);
            char description[MAX_STRING_CHARS];
            const char *value=Info_ValueForKey(raw,"allied_desc");
            if (!*value) value=Info_ValueForKey(raw,"axis_desc");
            Q_strncpyz(description,value,sizeof(description));
            if (!description[0]) continue;
            hasText=qtrue;
            if (cod1ObjectiveCheckbox[0]) {
                vec4_t tint={1,1,1,alpha};
                trap_R_SetColor(tint);
                CodHud_Pic(20,y-17,20,20,cod1ObjectiveCheckbox[0]);
                trap_R_SetColor(NULL);
            }
            y=CG_Cod1ObjectiveText(48,y,width-75,description,alpha,activeColor)+4;
        }
    }
    if (!hasText)
        CG_Cod1PromptText(20,182,"No mission objectives available.",.28f,alpha,activeColor);
    return qtrue;
}

#endif /* CG_COD1_OBJECTIVES_H */
