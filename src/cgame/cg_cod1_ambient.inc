/* R120 paired ambience consumer. The private transport preserves the native
 * absolute transition deadline; media admission and mixing are port policy. */
#include "../game/bg_cod1_ambient.h"
typedef struct {
    uint64_t incarnation,epoch,operation_id;
    int time;
    char map[sizeof(cgs.mapname)];
} cod1AmbientClientLease_t;
static struct {
    uint64_t incarnation,epoch,floor,operation_id;
    int last_time,have_time,have_server_id;
    uint32_t server_id;
} cgCod1Ambient;
static void CCA_Advance(uint64_t *value) {if(*value!=UINT64_MAX)++*value;}
static cod1AmbientClientLease_t CCA_Lease(void) {
    cod1AmbientClientLease_t lease;
    lease.incarnation=cgCod1Ambient.incarnation;lease.epoch=cgCod1Ambient.epoch;
    lease.operation_id=cgCod1Ambient.operation_id;lease.time=cg.time;
    memcpy(lease.map,cgs.mapname,sizeof(lease.map));return lease;
}
static int CCA_Current(const cod1AmbientClientLease_t *lease) {
    return lease && lease->incarnation && lease->incarnation!=UINT64_MAX &&
        lease->incarnation==cgCod1Ambient.incarnation &&
        lease->epoch==cgCod1Ambient.epoch &&
        lease->operation_id==cgCod1Ambient.operation_id && lease->time==cg.time &&
        !memcmp(lease->map,cgs.mapname,sizeof(lease->map));
}
static void CCA_ResetTarget(uint64_t epoch,uint64_t floor,uint64_t operation_id) {
    cod1AmbientPlayback playback;
    CCA_Advance(&cgCod1Ambient.incarnation);
    cgCod1Ambient.epoch=epoch;cgCod1Ambient.floor=floor;
    cgCod1Ambient.operation_id=operation_id;
    cgCod1Ambient.last_time=0;cgCod1Ambient.have_time=0;
    memset(&playback,0,sizeof(playback));playback.kind=COD1_AMBIENT_RESET;
    playback.epoch=epoch;playback.operation_id=operation_id;
    playback.client_incarnation=cgCod1Ambient.incarnation;playback.client_time=cg.time;
    /* State is retired before crossing the import. A nested reset remains the
     * current owner; this invocation performs no writes after the callback. */
    trap_Cod1ScriptAmbient(&playback);
}
void CG_Cod1AmbientReset(qboolean full) {
    CCA_ResetTarget(full?0:cgCod1Ambient.epoch,full?0:cgCod1Ambient.floor,
        full?0:cgCod1Ambient.operation_id);
}
void CG_Cod1AmbientFrame(void) {
    if(cg.time<0 || (cgCod1Ambient.have_time && cg.time<cgCod1Ambient.last_time)) {
        CG_Cod1AmbientReset(qfalse);return;
    }
    cgCod1Ambient.last_time=cg.time;cgCod1Ambient.have_time=1;
}
static int CCA_ModuleId(const cod1AmbientClientLease_t *lease,uint32_t *id) {
    const char *text;char copied[MAX_INFO_STRING];
    if(!CCA_Current(lease))return 0;
    text=CG_ConfigString(CS_SYSTEMINFO);
    if(!CCA_Current(lease) || !text || strlen(text)>=sizeof(copied))return 0;
    memcpy(copied,text,strlen(text)+1);
    return CA_ModuleIdToken(Info_ValueForKey(copied,COD1_AMBIENT_MODULE_CVAR),id) && *id>0 && *id<=INT_MAX && CCA_Current(lease);
}
void CG_Cod1AmbientConfigChanged(int number) {
    cod1AmbientClientLease_t lease;const char *text;uint64_t epoch;uint32_t id;
    if(number!=CS_COD1_AMBIENT_METADATA && number!=CS_SYSTEMINFO)return;
    lease=CCA_Lease();if(!CCA_ModuleId(&lease,&id))return;
    if(!cgCod1Ambient.have_server_id || id!=cgCod1Ambient.server_id) {
        cgCod1Ambient.server_id=id;cgCod1Ambient.have_server_id=1;
        CCA_ResetTarget(0,0,0);lease=CCA_Lease();
        if(!CCA_Current(&lease) || cgCod1Ambient.server_id!=id)return;
    }
    /* Re-read the marker after a verified realm transition. Ordering is also
     * enforced by the server before restarting game code. */
    text=CG_ConfigString(CS_COD1_AMBIENT_METADATA);
    if(!CCA_Current(&lease) || !text)return;
    if(!text[0]) {CCA_ResetTarget(0,cgCod1Ambient.floor,0);return;}
    if(!CA_DecodeMetadata(text,&epoch) || CA_ModuleId(epoch)!=id ||
       epoch==cgCod1Ambient.epoch || epoch<=cgCod1Ambient.floor)return;
    CCA_ResetTarget(epoch,epoch,0);
}
static int CCA_List(void *user,char *list,int capacity) {
    cod1AmbientClientLease_t *lease=(cod1AmbientClientLease_t *)user;int result;
    if(!CCA_Current(lease))return 0;
    result=trap_Cod1FS_GetFileList("soundaliases",".csv",list,capacity);
    return CCA_Current(lease)?result:0;
}
static int CCA_Read(void *user,const char *path,char **out,int *size) {
    cod1AmbientClientLease_t *lease=(cod1AmbientClientLease_t *)user;
    fileHandle_t file=0;int length,valid;char *data=NULL;
    *out=NULL;*size=0;if(!CCA_Current(lease))return 0;
    length=trap_FS_FOpenFile(path,&file,FS_READ);
    valid=CCA_Current(lease) && file && length>0 && length<=COD1_AMBIENT_TABLE_BYTES;
    if(valid)data=(char *)calloc((size_t)length+1,1);
    if(data && CCA_Current(lease))trap_FS_Read(data,length,file);
    else valid=0;
    if(!CCA_Current(lease))valid=0;
    /* Closing a locally acquired handle is mandatory even after retirement. */
    if(file)trap_FS_FCloseFile(file);
    if(!CCA_Current(lease))valid=0;
    if(!valid || !data || memchr(data,0,(size_t)length)) {free(data);return 0;}
    *out=data;*size=length;return 1;
}
static void CCA_Release(void *user,char *data) {(void)user;free(data);}
static int CCA_Exists(void *user,const char *path) {
    cod1AmbientClientLease_t *lease=(cod1AmbientClientLease_t *)user;
    fileHandle_t file=0;int length,valid;
    if(!CCA_Current(lease))return 0;
    length=trap_FS_FOpenFile(path,&file,FS_READ);valid=CCA_Current(lease);
    if(file)trap_FS_FCloseFile(file);
    return valid && CCA_Current(lease) && file && length>0;
}
void CG_Cod1AmbientCommand(void) {
    char copied[COD1_AMBIENT_ARGC][2*COD1_AMBIENT_ALIAS_BYTES+1];
    const char *arguments[COD1_AMBIENT_ARGC];
    cod1AmbientClientLease_t lease;cod1Ambient_t event;
    cod1AmbientPlayback playback;cod1AmbientFiles files;
    int i,count,result;int64_t remaining;uint32_t server_id;
    if(!cgCod1Ambient.epoch)return;
    CG_Cod1AmbientFrame();if(cg.time<0)return;
    lease=CCA_Lease();
    if(!CCA_ModuleId(&lease,&server_id) || CA_ModuleId(lease.epoch)!=server_id)return;
    count=trap_Argc();
    if(!CCA_Current(&lease) || count!=COD1_AMBIENT_ARGC)return;
    for(i=0;i<count;++i) {
        memset(copied[i],0,sizeof(copied[i]));if(!CCA_Current(&lease))return;
        trap_Argv(i,copied[i],sizeof(copied[i]));
        if(!CCA_Current(&lease))return;arguments[i]=copied[i];
    }
    if(!CA_DecodeCommand(count,arguments,&event) ||
       !CA_Follows(&event,lease.epoch,lease.operation_id) || !CCA_Current(&lease))return;
    memset(&playback,0,sizeof(playback));playback.kind=event.kind;
    memcpy(playback.alias,event.alias,sizeof(playback.alias));
    if(event.kind==COD1_AMBIENT_PLAY) {
        if(!memchr(lease.map,0,sizeof(lease.map)))return;
        memset(&files,0,sizeof(files));files.user=&lease;files.list=CCA_List;
        files.read=CCA_Read;files.release=CCA_Release;files.exists=CCA_Exists;
        if(!Cod1AmbientResolve(&files,event.alias,lease.map,&playback.plan)) {
            if(CCA_Current(&lease))CG_Printf("CoD1 ambience: alias unavailable or unsupported: %s\n",event.alias);
            return;
        }
    }
    if(!CCA_Current(&lease) || !CCA_ModuleId(&lease,&server_id) || CA_ModuleId(event.epoch)!=server_id)return;
    remaining=(int64_t)event.deadline-(int64_t)cg.time;
    if(remaining<0)remaining=0;
    if(remaining>INT_MAX)return;
    playback.transitionMs=(int)remaining;playback.epoch=event.epoch;
    playback.operation_id=event.operation_id;playback.client_incarnation=lease.incarnation;
    playback.client_time=lease.time;
    /* An import may commit before a nested lifecycle callback retires it.
     * Consume the token before entering it, and never rewind on refusal. */
    cgCod1Ambient.operation_id=event.operation_id;lease=CCA_Lease();
    result=trap_Cod1ScriptAmbient(&playback);
    if(!CCA_Current(&lease))return;
    if(result!=1)CG_Printf("CoD1 ambience: sound engine refused command\n");
}
