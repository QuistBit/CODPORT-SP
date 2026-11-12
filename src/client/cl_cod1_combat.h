#ifndef CL_COD1_COMBAT_H
#define CL_COD1_COMBAT_H
static kbutton_t cod1AdsKey,cod1AdsHoldKey,cod1AdsToggleKey,cod1ModeKey;
static int cod1CombatBinds;
static int cod1ObjectiveHudChecked,cod1ObjectiveHudAvailable;
static cvar_t *cl_cod1AdsToggle;
static int cod1AdsLatched,cod1AdsMenuLatched,cod1AdsMode=-1,cod1AdsSpawn=-1,cod1AdsTime=-1,cod1AdsServer;
/* PATCH47: the request may reverse at any fraction; no animation lock here.
 * Keep cl_run as RTCW's movement option. The CoD input choice has its own cvar. */
static void CL_Cod1AdsState(void) {
    int mode=cl_cod1AdsToggle && cl_cod1AdsToggle->integer!=0;
    if(mode!=cod1AdsMode) {
        cod1AdsMode=mode;cod1AdsLatched=0;cod1AdsKey.wasPressed=qfalse;
    }
    if(!cl.snap.valid) {cod1AdsLatched=cod1AdsMenuLatched=0;cod1AdsSpawn=-1;return;}
    if(cod1AdsSpawn>=0 && (cod1AdsSpawn!=cl.snap.ps.persistant[PERS_SPAWN_COUNT] ||
       cod1AdsServer!=cl.serverId || cl.snap.serverTime<cod1AdsTime)) {
        cod1AdsLatched=cod1AdsMenuLatched=0;
        cod1AdsKey.wasPressed=cod1AdsHoldKey.wasPressed=cod1AdsToggleKey.wasPressed=qfalse;
    }
    if(cl.snap.ps.stats[STAT_HEALTH]<=0 || cl.snap.ps.pm_type!=PM_NORMAL)
        cod1AdsLatched=cod1AdsMenuLatched=0;
    cod1AdsSpawn=cl.snap.ps.persistant[PERS_SPAWN_COUNT];
    cod1AdsTime=cl.snap.serverTime;cod1AdsServer=cl.serverId;
}
static void IN_Cod1AdsDown(void){
    qboolean wasActive;
    CL_Cod1AdsState();wasActive=cod1AdsKey.active;IN_KeyDown(&cod1AdsKey);
    if(cod1AdsMode && !wasActive && cod1AdsKey.active)cod1AdsLatched=!cod1AdsLatched;
}
static void IN_Cod1AdsUp(void){IN_KeyUp(&cod1AdsKey);}
/* Retail options_shoot.menu exposes toggle and hold as distinct bindable
 * actions. Keep +cod1ads and cl_cod1AdsToggle for existing configurations. */
static void IN_Cod1AdsHoldDown(void){IN_KeyDown(&cod1AdsHoldKey);}
static void IN_Cod1AdsHoldUp(void){IN_KeyUp(&cod1AdsHoldKey);}
static void IN_Cod1AdsToggleDown(void){
    qboolean wasActive=cod1AdsToggleKey.active;
    CL_Cod1AdsState();IN_KeyDown(&cod1AdsToggleKey);
    if(!wasActive && cod1AdsToggleKey.active)cod1AdsMenuLatched=!cod1AdsMenuLatched;
}
static void IN_Cod1AdsToggleUp(void){IN_KeyUp(&cod1AdsToggleKey);}
static void IN_Cod1ModeDown(void){IN_KeyDown(&cod1ModeKey);}
static void IN_Cod1ModeUp(void){IN_KeyUp(&cod1ModeKey);}
static void CL_Cod1CombatCommands(void) {
    cl_cod1AdsToggle=Cvar_Get("cl_cod1AdsToggle","1",CVAR_ARCHIVE);
    Cmd_AddCommand("+cod1ads",IN_Cod1AdsDown);Cmd_AddCommand("-cod1ads",IN_Cod1AdsUp);
    Cmd_AddCommand("+cod1adshold",IN_Cod1AdsHoldDown);Cmd_AddCommand("-cod1adshold",IN_Cod1AdsHoldUp);
    Cmd_AddCommand("+cod1adstoggle",IN_Cod1AdsToggleDown);Cmd_AddCommand("-cod1adstoggle",IN_Cod1AdsToggleUp);
    Cmd_AddCommand("+cod1mode",IN_Cod1ModeDown);Cmd_AddCommand("-cod1mode",IN_Cod1ModeUp);
}
/* A pre-existing RTCW profile binds TAB to the notebook menu.  The CoD
 * cgame uses +scores/-scores for the hold-to-view mission objectives sheet.
 * Probe the same retail HUD resources as the cgame and migrate only that
 * stock binding, leaving explicit custom bindings untouched. */
static void CL_Cod1ObjectiveTabBinding(void) {
    fileHandle_t file=0;
    int bytes;
    const char *binding;
    if(!cl.snap.valid)return;
    if(!cod1ObjectiveHudChecked) {
        bytes=FS_FOpenFileRead("fonts/fontImage_16.dat",&file,qfalse);
        if(file)FS_FCloseFile(file);
        cod1ObjectiveHudAvailable=(bytes==20552);
        if(!cod1ObjectiveHudAvailable) {
            file=0;
            bytes=FS_FOpenFileRead("gfx/hud/hud@compassback.tga",&file,qfalse);
            if(file)FS_FCloseFile(file);
            cod1ObjectiveHudAvailable=(bytes>0);
        }
        cod1ObjectiveHudChecked=1;
    }
    if(!cod1ObjectiveHudAvailable)return;
    binding=Key_GetBinding(K_TAB);
    if(binding && !Q_stricmp(binding,"notebook"))
        Key_SetBinding(K_TAB,"+scores");
}
static void CL_Cod1CombatMove(usercmd_t *cmd) {
    CL_Cod1AdsState();
    CL_Cod1ObjectiveTabBinding();
    if(!cod1CombatBinds && cl.snap.valid) {
        const char *b=Key_GetBinding(K_MOUSE2);
        if(!b || !*b || !Q_stricmp(b,"toggle cl_run"))Key_SetBinding(K_MOUSE2,"+cod1adstoggle");
        else if(!Q_stricmp(b,"+speed"))Key_SetBinding(K_MOUSE2,"+cod1adshold");
        else if(!Q_stricmp(b,"+attack2"))Key_SetBinding(K_MOUSE2,"+cod1ads");
        b=Key_GetBinding('m');
        if(!b || !*b || !Q_stricmp(b,"weapalt"))Key_SetBinding('m',"+cod1mode");
        cod1CombatBinds=1;
        Com_Printf("CoD1 PATCH47: +cod1ads uses cl_cod1AdsToggle (1=press, 0=hold); +cod1mode fire selector\n");
    }
    if(cod1AdsMenuLatched || cod1AdsHoldKey.active || cod1AdsHoldKey.wasPressed ||
       (cod1AdsMode?cod1AdsLatched:(cod1AdsKey.active || cod1AdsKey.wasPressed)))
        cmd->wbuttons|=WBUTTON_ATTACK2;
    if(cod1ModeKey.active || cod1ModeKey.wasPressed)cmd->buttons|=BUTTON_GESTURE;
    cod1AdsKey.wasPressed=cod1AdsHoldKey.wasPressed=cod1AdsToggleKey.wasPressed=cod1ModeKey.wasPressed=qfalse;
}
#endif
