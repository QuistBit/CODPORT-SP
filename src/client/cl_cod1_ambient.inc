/* R120 trusted gamestate authority. The reliable baseline belongs to the
 * parsed gamestate, not to the newest command at cgame creation. Older paired
 * control/state cannot revert the new realm; current initial events remain
 * eligible even when they precede this baseline. */
#include "../game/bg_cod1_ambient.h"
#include "../qcommon/cod1_item_protocol.h"
static struct {int ready,checkpoint,sequence;uint32_t server_id;} clCod1AmbientAuthority;
static void CL_Cod1AmbientCaptureAuthority(void) {
    const char *info=cl.gameState.stringData+cl.gameState.stringOffsets[CS_SYSTEMINFO];
    uint32_t id=0,previous_id=clCod1AmbientAuthority.server_id;
    int previous_ready=clCod1AmbientAuthority.ready;
    clCod1AmbientAuthority.ready=CA_ModuleIdToken(Info_ValueForKey(info,COD1_AMBIENT_MODULE_CVAR),&id) && id>0 && id<=INT_MAX;
    clCod1AmbientAuthority.server_id=id;
    if(previous_ready!=clCod1AmbientAuthority.ready || previous_id!=id)
        S_Cod1AmbientModuleReset();
}
void CL_Cod1AmbientGamestateCheckpoint(int sequence) {
    clCod1AmbientAuthority.sequence=sequence;
    clCod1AmbientAuthority.checkpoint=sequence>=0;
    if(sequence<0) {
        clCod1AmbientAuthority.ready=0;clCod1AmbientAuthority.server_id=0;
        S_Cod1AmbientModuleReset();return;
    }
    CL_Cod1AmbientCaptureAuthority();
}
unsigned int CL_Cod1AmbientServerId(void) {
    return clCod1AmbientAuthority.ready?clCod1AmbientAuthority.server_id:0;
}
int CL_Cod1AmbientAuthorityReady(void) {return clCod1AmbientAuthority.ready;}
static void CL_Cod1AmbientSystemInfoChanged(void) {CL_Cod1AmbientCaptureAuthority();}
static int CL_Cod1AmbientStaleCommand(int sequence) {
    const char *cmd=Cmd_Argv(0);cod1Ambient_t event;
    const char *arguments[COD1_AMBIENT_ARGC];char copied[COD1_AMBIENT_ARGC][257];int i,index;
    if(!strcmp(cmd,COD1_AMBIENT_COMMAND)) {
        if(!clCod1AmbientAuthority.ready || Cmd_Argc()!=COD1_AMBIENT_ARGC)return 1;
        for(i=0;i<COD1_AMBIENT_ARGC;++i) {
            Q_strncpyz(copied[i],Cmd_Argv(i),sizeof(copied[i]));arguments[i]=copied[i];
        }
        return !CA_DecodeCommand(COD1_AMBIENT_ARGC,arguments,&event) ||
            CA_ModuleId(event.epoch)!=clCod1AmbientAuthority.server_id;
    }
    if(clCod1AmbientAuthority.checkpoint && sequence<=clCod1AmbientAuthority.sequence &&
       (!strcmp(cmd,"cs") || !strcmp(cmd,"bcs0") || !strcmp(cmd,"bcs1") || !strcmp(cmd,"bcs2"))) {
        index=atoi(Cmd_Argv(1));
        return index==CS_SYSTEMINFO || index==CS_COD1_AMBIENT_METADATA || index==CS_COD1_ITEM_PRECACHE;
    }
    return 0;
}
