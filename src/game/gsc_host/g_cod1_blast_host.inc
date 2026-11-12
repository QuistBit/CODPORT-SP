/* R113 separately selected script blast. The copied context is a lease on the
 * current diagnostic session and entity map, not an original context global.
 * This subset has no ignore-player immunity and refuses actor/model LOS. */
typedef struct {
    uint64_t id, epoch;
} GscTestBlastContext;

static int GscTestBlastCurrent(void *user) {
    const GscTestBlastContext *context=(const GscTestBlastContext *)user;
    if (!context || !context->id || context->id!=gscTestId ||
        !context->epoch || context->epoch!=G_Cod1EntityMapEpoch() ||
        !gscTestSession || gscTestCompleted || gscTestStopRequested ||
        (gscTestEntityMode!=20 && gscTestEntityMode!=21 && gscTestEntityMode!=22 && gscTestEntityMode!=23 && gscTestEntityMode!=24 && gscTestEntityMode!=25 && (gscTestEntityMode!=26 && gscTestEntityMode!=27 && (gscTestEntityMode!=28 && (gscTestEntityMode!=29 && (gscTestEntityMode!=30 && (gscTestEntityMode!=31 && (gscTestEntityMode!=32 && (gscTestEntityMode!=33 && (gscTestEntityMode!=34 && (gscTestEntityMode!=35 && (gscTestEntityMode!=36 && (gscTestEntityMode!=37 && (gscTestEntityMode!=38 && (gscTestEntityMode!=39 && (gscTestEntityMode!=40&&(gscTestEntityMode!=41&&(gscTestEntityMode!=42&&gscTestEntityMode!=43))))))))))))))))) || level.restarted || !G_Cod1MapEntities() ||
        g_gametype.integer!=GT_SINGLE_PLAYER || !GscTestModuleReadCurrent()) return 0;
    return context->id==gscTestId &&
        context->epoch==G_Cod1EntityMapEpoch() && gscTestSession &&
        !gscTestCompleted && !gscTestStopRequested && (gscTestEntityMode==20 || gscTestEntityMode==21 || (gscTestEntityMode==22 || gscTestEntityMode==23 || gscTestEntityMode==24 || gscTestEntityMode==25 || (gscTestEntityMode==26 || (gscTestEntityMode==27 || (gscTestEntityMode==28 || (gscTestEntityMode==29 || (gscTestEntityMode==30 || (gscTestEntityMode==31 || (gscTestEntityMode==32 || (gscTestEntityMode==33 || (gscTestEntityMode==34 || gscTestEntityMode==35 || gscTestEntityMode==36 || gscTestEntityMode==37 || gscTestEntityMode==38 || gscTestEntityMode==39 || (gscTestEntityMode==40||(gscTestEntityMode==41||(gscTestEntityMode==42||gscTestEntityMode==43)))))))))))))) &&
        !level.restarted && G_Cod1MapEntities() &&
        g_gametype.integer==GT_SINGLE_PLAYER;
}
static int GscTestBlastRadius(void *user,uint64_t id,int logicalMs,
    const GscBlastEvent *event,char *error,size_t capacity) {
    GscBlastEvent copy;
    GscTestBlastContext context;
    char endError[GSC_SESSION_ERROR_CAP];
    int accepted,ended;
    (void)user;
    if (!event) {
        GscTestError(error,capacity,"script blast event missing");return 0;
    }
    copy=*event; /* All parameters are private before any cvar/trace service. */
    context.id=id;context.epoch=G_Cod1EntityMapEpoch();
    if (logicalMs<0 || copy.kind!=GSC_BLAST_RADIUS ||
        !GscTestBlastCurrent(&context)) {
        GscTestError(error,capacity,"script blast invalid or map/session changed");return 0;
    }
    if (!GscTestBlastEventsBegin(id,context.epoch,logicalMs,error,capacity)) return 0;
    accepted=G_Cod1ScriptRadiusDamage(copy.origin,copy.radius,copy.inner_damage,
        copy.outer_damage,GscTestBlastCurrent,&context,error,capacity);
    ended=GscTestBlastEventsEnd(endError,sizeof(endError));
    if (!ended && (accepted || gscTestBlastEvents.failed)) GscTestError(error,capacity,endError);
    return accepted && ended;
}
