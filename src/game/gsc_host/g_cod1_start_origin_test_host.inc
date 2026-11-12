/* Dedicated selected mode44: genuine VFS resource admission and public Session.
 * The normal mode43 Steam bomb scene does not select this capability. */
#include "../g_cod1_animation_start_origin_session.h"
static unsigned gscTestStartOriginQueries;
static int GscTestStartOriginScope(const GscStartOriginRequest *request) {
    GscSessionSnapshot snapshot;
    return request && (gscTestEntityMode==44 || gscTestEntityMode==45) && gscTestBusy &&
        gscTestSession && gscTestId && !gscTestCompleted &&
        GscTestModuleReadCurrent() &&
        gsc_session_snapshot(gscTestSession,&snapshot) && snapshot.busy &&
        snapshot.state==GSC_SESSION_RUNNING && snapshot.session_id==gscTestId &&
        snapshot.logical_ms==request->now_ms;
}
static int GscTestStartOriginQuery(void *user,const GscStartOriginRequest *request,
    GscStartOriginReply *out,char *error,size_t size,int (*current)(void *),void *context) {
    const GscStartOriginHost *real=G_Cod1StartOriginQueryHost();
    GscStartOriginReply staged;
    uint32_t origin[3],angles[3],result[3];
    (void)user;
    if(!out || !GscTestStartOriginScope(request) || !real || !real->query ||
       !real->query(real->user,request,&staged,error,size,current,context) ||
       !GscTestStartOriginScope(request)) return 0;
    memcpy(origin,request->origin,sizeof(origin));memcpy(angles,request->angles,sizeof(angles));
    memcpy(result,staged.vector,sizeof(result));
    ++gscTestStartOriginQueries;
    G_Printf("[R209] GETSTARTORIGIN_QUERY index=%u ordinal=%u now=%d origin=%08x,%08x,%08x angles=%08x,%08x,%08x result=%08x,%08x,%08x\n",
        gscTestStartOriginQueries,request->ordinal,request->now_ms,
        origin[0],origin[1],origin[2],angles[0],angles[1],angles[2],result[0],result[1],result[2]);
    *out=staged;return 1;
}
static int GscTestStartOriginRead(const char *path,size_t limit,void **out,
    size_t *outSize,char *error,size_t size) {
    fileHandle_t file=0;int length;void *bytes;
    *out=NULL;*outSize=0;
    if(!GscTestModuleReadCurrent())return 0;
    length=trap_FS_FOpenFile(path,&file,FS_READ);
    if(length<=0 || !file || (size_t)length>limit) {
        if(file)trap_FS_FCloseFile(file);
        GscTestError(error,size,"start-origin actual VFS resource unavailable");return 0;
    }
    bytes=COD1_GSC_VFS_CALLOC((size_t)length,1);
    if(!bytes){trap_FS_FCloseFile(file);return 0;}
    /* trap_FS_Read has no return count. Full catalog/clip parsing validates
     * these actual winning bytes; no invented read-count claim is made. */
    trap_FS_Read(bytes,length,file);trap_FS_FCloseFile(file);
    if(!GscTestModuleReadCurrent()){COD1_GSC_VFS_FREE(bytes);return 0;}
    *out=bytes;*outSize=(size_t)length;return 1;
}
static int GscTestStartOriginSelect(char *error,size_t size) {
    void *atr=NULL,*clip=NULL;size_t atrSize=0,clipSize=0;int ok=0;
    cod1StartOriginResourceInput_t input;
    GscSessionStartOriginSelection selection;
    memset(&input,0,sizeof(input));memset(&selection,0,sizeof(selection));
    gscTestStartOriginQueries=0;
    if(!GscTestStartOriginRead("animtrees/generic_human.atr",131072,&atr,&atrSize,error,size) ||
       !GscTestStartOriginRead("xanim/training_guy_practicejump",16777216,&clip,&clipSize,error,size))goto done;
    input.tree="generic_human";input.name="training_guy_practicejump";
    input.atr_bytes=atr;input.atr_size=atrSize;input.xanim_bytes=clip;input.xanim_size=clipSize;
    if(!G_Cod1SelectStartOriginResources(&input,1,error,size) ||
       !GscTestModuleReadCurrent() ||
       !G_Cod1StartOriginMapCertificate(&selection.map_certificate,error,size))goto done;
    selection.generic_human_atr=atr;selection.generic_human_atr_size=atrSize;
    selection.host.user=NULL;selection.host.query=GscTestStartOriginQuery;
    if(!gsc_session_select_r209_start_origin(gscTestSession,&selection,error,size) ||
       !GscTestModuleReadCurrent())goto done;
    G_Printf("[GSC R209] genuine resources selected ATR=%u XAnim=%u map-receipt=%llu\n",
        (unsigned)atrSize,(unsigned)clipSize,selection.map_certificate.receipt);
    ok=1;
done:
    if(atr)COD1_GSC_VFS_FREE(atr);if(clip)COD1_GSC_VFS_FREE(clip);
    if(!ok)GscTestError(error,size,"actual VFS/start-origin Session selection refused");
    return ok;
}
