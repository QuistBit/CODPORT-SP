/* R110 diagnostic-only console producer. The original output-gate global is
 * not yet identified; this selected port gate admits only its live session.
 * It is not a developer/cvar rendering preference. Subsequent lease checks
 * protect map/session cancellation and do not query the retail output gate. */
static int GscTestConsoleCurrent(uint64_t id) {
    if (!id || id!=gscTestId || !gscTestSession || gscTestCompleted ||
        (gscTestEntityMode!=17 && gscTestEntityMode!=18 && (gscTestEntityMode!=19 && gscTestEntityMode!=20 && gscTestEntityMode!=21 && gscTestEntityMode!=22 && gscTestEntityMode!=23 && gscTestEntityMode!=24 && gscTestEntityMode!=25 && (gscTestEntityMode!=26 && gscTestEntityMode!=27 && (gscTestEntityMode!=28 && (gscTestEntityMode!=29 && (gscTestEntityMode!=30 && (gscTestEntityMode!=31 && (gscTestEntityMode!=32 && (gscTestEntityMode!=33 && (gscTestEntityMode!=34 && (gscTestEntityMode!=35 && (gscTestEntityMode!=36 && (gscTestEntityMode!=37 && (gscTestEntityMode!=38 && (gscTestEntityMode!=39 && (gscTestEntityMode!=40&&(gscTestEntityMode!=41&&(gscTestEntityMode!=42&&gscTestEntityMode!=43&&gscTestEntityMode!=44&&gscTestEntityMode!=45&&gscTestEntityMode!=46&&gscTestEntityMode!=47)))))))))))))))))) || gscTestStopRequested ||
        !G_Cod1MapEntities() || g_gametype.integer!=GT_SINGLE_PLAYER ||
        level.restarted || !GscTestModuleReadCurrent()) return 0;
    return id==gscTestId && gscTestSession && !gscTestCompleted &&
        (gscTestEntityMode==17 || gscTestEntityMode==18||(gscTestEntityMode==19 || gscTestEntityMode==20 || gscTestEntityMode==21 || (gscTestEntityMode==22 || gscTestEntityMode==23 || gscTestEntityMode==24 || gscTestEntityMode==25 || (gscTestEntityMode==26 || (gscTestEntityMode==27 || (gscTestEntityMode==28 || (gscTestEntityMode==29 || (gscTestEntityMode==30 || (gscTestEntityMode==31 || (gscTestEntityMode==32 || (gscTestEntityMode==33 || (gscTestEntityMode==34 || gscTestEntityMode==35 || gscTestEntityMode==36 || gscTestEntityMode==37 || gscTestEntityMode==38 || gscTestEntityMode==39 || (gscTestEntityMode==40||(gscTestEntityMode==41||(gscTestEntityMode==42||gscTestEntityMode==43||gscTestEntityMode==44||gscTestEntityMode==45||gscTestEntityMode==46||gscTestEntityMode==47))))))))))))))) && !gscTestStopRequested && !level.restarted &&
        G_Cod1MapEntities() && g_gametype.integer==GT_SINGLE_PLAYER;
}

static int GscTestConsoleGate(void *user,uint64_t id,int logicalMs,
    int kind,int phase,int *enabled,char *error,size_t capacity) {
    (void)user;
    if (enabled) *enabled=0;
    if (!enabled || logicalMs<0 ||
        (kind!=GSC_CONSOLE_PRINT && kind!=GSC_CONSOLE_PRINTLN) ||
        (phase!=GSC_CONSOLE_GATE_ARGUMENTS &&
            !(kind==GSC_CONSOLE_PRINTLN && phase==GSC_CONSOLE_GATE_ENTRY)) ||
        !GscTestConsoleCurrent(id)) {
        GscTestError(error,capacity,"console gate invalid or map/session changed");
        return 0;
    }
    *enabled=1;
    return 1;
}

static int GscTestConsole(void *user,uint64_t id,int logicalMs,
    const GscConsoleEvent *event,char *error,size_t capacity,int kind) {
    const char *end;
    (void)user;
    if (!event || event->kind!=kind || logicalMs<0 ||
        !GscTestConsoleCurrent(id)) goto refuse;
    end=memchr(event->text,0,sizeof(event->text));
    if (!end || event->length!=(size_t)(end-event->text)) goto refuse;
    if (event->phase==GSC_CONSOLE_ARGUMENT) {
        if (event->argument_index<0) goto refuse;
    } else if (event->phase==GSC_CONSOLE_NEWLINE) {
        if (kind!=GSC_CONSOLE_PRINTLN || event->argument_index!=-1 ||
            event->length!=1 || event->text[0]!='\n') goto refuse;
    } else goto refuse;
    /* Every argument, including an empty string, is a separate real game
     * print. Percent characters are data; never use authored text as fmt. */
    G_Printf("%s",event->text);
    if (!GscTestConsoleCurrent(id)) goto refuse;
    return 1;
refuse:
    GscTestError(error,capacity,"console output invalid or map/session changed");
    return 0;
}

static int GscTestConsolePrint(void *user,uint64_t id,int logicalMs,
    const GscConsoleEvent *event,char *error,size_t capacity) {
    return GscTestConsole(user,id,logicalMs,event,error,capacity,GSC_CONSOLE_PRINT);
}
static int GscTestConsolePrintln(void *user,uint64_t id,int logicalMs,
    const GscConsoleEvent *event,char *error,size_t capacity) {
    return GscTestConsole(user,id,logicalMs,event,error,capacity,GSC_CONSOLE_PRINTLN);
}
