/* R120 authority survives game DLL reloads and server-time rollback. */
#include "../qcommon/cod1_ambient_module.h"
static int sv_cod1AmbientModuleGeneration;

static int SV_Cod1AmbientReservedCvar(const char *name) {
    return name && !Q_stricmp(name,COD1_AMBIENT_MODULE_CVAR);
}
static void SV_Cod1AmbientRegisterGameCvar(vmCvar_t *variable,const char *name,
                                         const char *value,int flags) {
    if(SV_Cod1AmbientReservedCvar(name)) {
        Cvar_Register(variable,COD1_AMBIENT_MODULE_CVAR,
            Cvar_VariableString(COD1_AMBIENT_MODULE_CVAR),CVAR_SYSTEMINFO|CVAR_ROM);
    } else Cvar_Register(variable,name,value,flags);
}
static void SV_Cod1AmbientSetGameCvar(const char *name,const char *value) {
    if(!SV_Cod1AmbientReservedCvar(name))Cvar_Set(name,value);
}
static void SV_Cod1AmbientBeginGameModule(void) {
    int authority=0;
    Cvar_Get(COD1_AMBIENT_MODULE_CVAR,"0",CVAR_SYSTEMINFO|CVAR_ROM);
    if(sv_cod1AmbientModuleGeneration<COD1_AMBIENT_MODULE_MAX)
        authority=++sv_cod1AmbientModuleGeneration;
    /* Exhaustion publishes zero forever; a retired identity is never reused. */
    Cvar_Set(COD1_AMBIENT_MODULE_CVAR,va("%i",authority));
    /* On restart this queues SYSTEMINFO before GAME_INIT marker/events. Full
     * loads carry the same current authority in their new gamestate. */
    SV_SetConfigstring(CS_SYSTEMINFO,Cvar_InfoString_Big(CVAR_SYSTEMINFO));
}
