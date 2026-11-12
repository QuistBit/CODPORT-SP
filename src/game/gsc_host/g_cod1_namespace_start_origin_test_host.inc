/* Observes only the separately selected real mode47 query. */
static unsigned gscTestNamespaceStartOriginQueries;
static int GscTestNamespaceStartOriginScope(const GscStartOriginRequest *request) {
    GscSessionSnapshot snapshot;
    return request && (gscTestEntityMode==47 || (gscTestEntityMode==43 && trap_Cvar_VariableIntegerValue("g_cod1GscTrainingRigs")==1)) && gscTestBusy &&
        gscTestSession && gscTestId && !gscTestCompleted && GscTestModuleReadCurrent() &&
        gsc_session_snapshot(gscTestSession,&snapshot) && snapshot.busy &&
        snapshot.state==GSC_SESSION_RUNNING && snapshot.session_id==gscTestId &&
        snapshot.logical_ms==request->now_ms;
}
static int GscTestNamespaceStartOriginQuery(void *user,const GscStartOriginRequest *request,
    GscStartOriginReply *out,char *error,size_t size,int (*current)(void *),void *context) {
    const GscStartOriginHost *real=G_Cod1NamespaceStartOriginQueryHost();
    GscStartOriginRequest copied;GscStartOriginReply staged;
    uint32_t origin[3],angles[3],result[3];
    (void)user;
    if(!request || !out || !GscTestNamespaceStartOriginScope(request) || !real || !real->query)return 0;
    copied=*request;
    if(!real->query(real->user,request,&staged,error,size,current,context) ||
       memcmp(request,&copied,sizeof(copied)) || !GscTestNamespaceStartOriginScope(&copied))return 0;
    memcpy(origin,copied.origin,sizeof(origin));memcpy(angles,copied.angles,sizeof(angles));
    memcpy(result,staged.vector,sizeof(result));++gscTestNamespaceStartOriginQueries;
    G_Printf("[R212] NAMESPACE_START_ORIGIN_QUERY index=%u tree=%s name=%s ordinal=%u namespace=%llu runtime=%llu now=%d origin=%08x,%08x,%08x angles=%08x,%08x,%08x result=%08x,%08x,%08x\n",
        gscTestNamespaceStartOriginQueries,copied.tree,copied.name,copied.ordinal,
        copied.catalog_identity,copied.runtime_identity,copied.now_ms,
        origin[0],origin[1],origin[2],angles[0],angles[1],angles[2],result[0],result[1],result[2]);
    *out=staged;return 1;
}
static int GscTestNamespaceStartOriginSelect(char *error,size_t size) {
    GscStartOriginHost host;host.user=NULL;host.query=GscTestNamespaceStartOriginQuery;
    return gsc_session_select_r212_namespace_start_origin(gscTestSession,&host,error,size) && GscTestModuleReadCurrent();
}
