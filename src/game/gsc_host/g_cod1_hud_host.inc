/* R111 diagnostic-only HUD producer. The retail game formats one gm/gmb
 * command for a whole composed message. This stage admits the confirmed
 * broadcast wrappers only. A successful callback acknowledges transport,
 * not rendering or the unavailable retail engine's message queue/timing.
 * Retail leaves quotes unescaped; this bounded producer refuses them so the
 * command remains exactly one quoted payload through the real tokenizer. */
static int GscTestHudCurrent(uint64_t id) {
    if (!id || id!=gscTestId || !gscTestSession || gscTestCompleted ||
        (gscTestEntityMode!=18 && (gscTestEntityMode!=19 && gscTestEntityMode!=20 && gscTestEntityMode!=21 && gscTestEntityMode!=22 && gscTestEntityMode!=23 && gscTestEntityMode!=24 && gscTestEntityMode!=25 && (gscTestEntityMode!=26 && gscTestEntityMode!=27 && (gscTestEntityMode!=28 && (gscTestEntityMode!=29 && (gscTestEntityMode!=30 && (gscTestEntityMode!=31 && (gscTestEntityMode!=32 && (gscTestEntityMode!=33 && (gscTestEntityMode!=34 && (gscTestEntityMode!=35 && (gscTestEntityMode!=36 && (gscTestEntityMode!=37 && (gscTestEntityMode!=38 && (gscTestEntityMode!=39 && (gscTestEntityMode!=40&&(gscTestEntityMode!=41&&(gscTestEntityMode!=42&&gscTestEntityMode!=43)))))))))))))))))) || gscTestStopRequested ||
        !G_Cod1MapEntities() || g_gametype.integer!=GT_SINGLE_PLAYER ||
        level.restarted || !GscTestModuleReadCurrent()) return 0;
    return id==gscTestId && gscTestSession && !gscTestCompleted &&
        (gscTestEntityMode==18 || (gscTestEntityMode==19 || gscTestEntityMode==20 || gscTestEntityMode==21 || (gscTestEntityMode==22 || gscTestEntityMode==23 || gscTestEntityMode==24 || gscTestEntityMode==25 || (gscTestEntityMode==26 || (gscTestEntityMode==27 || (gscTestEntityMode==28 || (gscTestEntityMode==29 || (gscTestEntityMode==30 || (gscTestEntityMode==31 || (gscTestEntityMode==32 || (gscTestEntityMode==33 || (gscTestEntityMode==34 || gscTestEntityMode==35 || gscTestEntityMode==36 || gscTestEntityMode==37 || gscTestEntityMode==38 || gscTestEntityMode==39 || (gscTestEntityMode==40||(gscTestEntityMode==41||(gscTestEntityMode==42||gscTestEntityMode==43))))))))))))))) && !gscTestStopRequested && !level.restarted &&
        G_Cod1MapEntities() && g_gametype.integer==GT_SINGLE_PLAYER;
}

static int GscTestHud(void *user,uint64_t id,int logicalMs,
    const GscHudEvent *event,char *error,size_t capacity,int kind) {
    GscHudEvent copy;
    char command[MAX_STRING_CHARS];
    const char *end,*route;
    size_t n;
    int count;
    (void)user;
    if (!event) goto refuse;
    /* Do not retain a borrowed callback event across an engine service. */
    copy=*event;
    if (copy.kind!=kind || logicalMs<0 ||
        copy.audience!=GSC_HUD_GLOBAL || copy.receiver.epoch ||
        copy.receiver.slot || copy.receiver.generation ||
        !GscTestHudCurrent(id)) goto refuse;
    end=memchr(copy.text,0,sizeof(copy.text));
    if (!end || copy.length!=(size_t)(end-copy.text)) goto refuse;
    for (n=0;n<copy.length;++n) {
        unsigned char c=(unsigned char)copy.text[n];
        /* 20/21 are generated StringEd composition delimiters. Ordinary
         * authored controls must already have been rejected by the VM. */
        if (c=='"' || (c<32 && c!=0x14 && c!=0x15)) goto refuse;
    }
    route=kind==GSC_HUD_PRINT?"gm":"gmb";
    count=snprintf(command,sizeof(command),"%s \"%s\"",route,copy.text);
    /* Retail's builder limit is 1023 payload bytes. The retained engine's
     * client reliable-command slot is 1024 including NUL: independently
     * bound the full command, hence gm<=1018 and gmb<=1017 payload bytes. */
    if (count<0 || count>=(int)sizeof(command) || !GscTestHudCurrent(id))
        goto refuse;
    trap_SendServerCommand(-1,command);
    if (!GscTestHudCurrent(id)) goto refuse;
    return 1;
refuse:
    GscTestError(error,capacity,"HUD message invalid, command exceeds 1023 bytes or map/session changed");
    return 0;
}

static int GscTestHudPrint(void *user,uint64_t id,int logicalMs,
    const GscHudEvent *event,char *error,size_t capacity) {
    return GscTestHud(user,id,logicalMs,event,error,capacity,GSC_HUD_PRINT);
}
static int GscTestHudBold(void *user,uint64_t id,int logicalMs,
    const GscHudEvent *event,char *error,size_t capacity) {
    return GscTestHud(user,id,logicalMs,event,error,capacity,GSC_HUD_BOLD);
}
