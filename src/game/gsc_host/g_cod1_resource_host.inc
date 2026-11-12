/* R108 original precache table effects. Client registration is asynchronous;
 * successful publication is not an acknowledgment of a renderer handle. */
static int gscTestResourceInitialization;
static char gscTestResourceValidated[MAX_MODELS][MAX_QPATH];
static int GscTestResourceCurrent(uint64_t id) {
    return gscTestResourceInitialization && id && id==gscTestId &&
        (gscTestEntityMode==15 || gscTestEntityMode==16 || gscTestEntityMode==17 || gscTestEntityMode==18||(gscTestEntityMode==19 || gscTestEntityMode==20 || gscTestEntityMode==21 || (gscTestEntityMode==22 || gscTestEntityMode==23 || gscTestEntityMode==24 || gscTestEntityMode==25 || (gscTestEntityMode==26 || (gscTestEntityMode==27 || (gscTestEntityMode==28 || (gscTestEntityMode==29 || (gscTestEntityMode==30 || (gscTestEntityMode==31 || (gscTestEntityMode==32 || (gscTestEntityMode==33 || (gscTestEntityMode==34 || gscTestEntityMode==35 || gscTestEntityMode==36 || gscTestEntityMode==37 || gscTestEntityMode==38 || gscTestEntityMode==39 || (gscTestEntityMode==40||(gscTestEntityMode==41||(gscTestEntityMode==42||gscTestEntityMode==43))))))))))))))) && !gscTestStopRequested && G_Cod1MapEntities() &&
        g_gametype.integer==GT_SINGLE_PLAYER && !level.restarted &&
        GscTestModuleReadCurrent();
}
static int GscTestResource(void *user,uint64_t id,int logicalMs,
    const GscResourceEvent *event,char *error,size_t capacity,int model) {
    char name[sizeof(event->name)],registered[MAX_STRING_CHARS];
    int i,base=model?CS_MODELS:CS_COD1_SHADERS;
    int slots=model?MAX_MODELS:MAX_COD1_SHADERS;
    (void)user;
    if (!event || logicalMs<0 || event->kind!=(model?GSC_RESOURCE_MODEL:GSC_RESOURCE_SHADER) ||
        !GscTestResourceCurrent(id)) goto refuse;
    if(model && !event->name[0])return 1;
    if(!Cod1PrecacheNameBytes(event->name,model,model?MAX_QPATH:sizeof(event->name)))goto refuse;
    strcpy(name,event->name);
    for(i=1;i<slots;++i) {
        trap_GetConfigstring(base+i,registered,sizeof(registered));
        if(!GscTestResourceCurrent(id))goto refuse;
        if(!registered[0])break;
        if(!Q_stricmp(registered,name)) {
            if(!model || !Q_stricmp(gscTestResourceValidated[i],name))return 1;
            /* Older port producers can publish a model CS without loading a
             * server resource. Verify/acquire such an inherited entry once;
             * a CS name alone is never accepted as a genuine loaded model. */
            break;
        }
    }
    if(i==slots)goto refuse;
    if(model) {
        cod1ModelPrecache_t request;
        memset(&request,0,sizeof(request));request.version=COD1_PRECACHE_VERSION;
        request.bytes=sizeof(request);strcpy(request.name,name);
        if(trap_Cod1ModelPrecache(&request)<=0 || !GscTestResourceCurrent(id))goto refuse;
        trap_GetConfigstring(base+i,registered,sizeof(registered));
        if(!GscTestResourceCurrent(id))goto refuse;
        if(registered[0]) {
            if(Q_stricmp(registered,name))goto refuse;
            strcpy(gscTestResourceValidated[i],name);return 1;
        }
    }
    trap_SetConfigstring(base+i,name);
    if(!GscTestResourceCurrent(id))goto refuse;
    if(model)strcpy(gscTestResourceValidated[i],name);
    return 1;
refuse:
    GscTestError(error,capacity,model?
        "precachemodel resource unavailable, invalid/full table or initialization changed":
        "precacheshader invalid/full table or initialization changed");return 0;
}
static int GscTestResourceShader(void *user,uint64_t id,int logicalMs,
    const GscResourceEvent *event,char *error,size_t capacity) {
    return GscTestResource(user,id,logicalMs,event,error,capacity,0);
}
static int GscTestResourceModel(void *user,uint64_t id,int logicalMs,
    const GscResourceEvent *event,char *error,size_t capacity) {
    return GscTestResource(user,id,logicalMs,event,error,capacity,1);
}
