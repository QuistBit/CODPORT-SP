#ifndef BG_COD1_WEAPON_IO_H
#define BG_COD1_WEAPON_IO_H
#include "bg_cod1_weapondef.h"
#include "bg_cod1_reload.h"
/* Include after g_local.h / cg_local.h. Uses the existing PK3 search order. */
static int CW_EngineRead(const char *path,char *data,int capacity) {
    fileHandle_t handle;
    int size=trap_FS_FOpenFile(path,&handle,FS_READ);
    if(!handle) return -1;
    if(size<0 || size>capacity) { trap_FS_FCloseFile(handle); return -2; }
    trap_FS_Read(data,size,handle); trap_FS_FCloseFile(handle); return size;
}
static void CW_InitEngineWeapons(void) {
    int result,i,loaded;
    static const char *roots[]={"mp40","sten","mp44","ppsh","bar","bren","fg42","m1garand","m1carbine","colt","luger","kar98k","enfield","mosin_nagant","kar98k_sniper","mosin_nagant_sniper","springfield","mk1britishfrag","rgd-33russianfrag","stalingrad_ammo","panzerfaust","fraggrenade","stielhandgranate"};
    unsigned int hash=2166136261u;
    char fingerprint[16];
    const cwDef_t *d;
#ifdef GAMEDLL
    static vmCvar_t checksumCvar;
#else
    const char *serverHash;
#endif
    result=CW_LoadRegistry(CW_EngineRead,"thompson");
    if(result<0) { Com_Error(ERR_DROP,"CoD1 PATCH35 WeaponDef: %s",CW_Error()); return; }
    /* Fixed root order on both DLLs preserves Thompson IDs 0/1. Missing
     * optional MP40 data leaves the existing Thompson-only installation valid. */
    loaded=0;
    for(i=0;i<sizeof(roots)/sizeof(roots[0]);i++) {
        int r=CW_AddFamily(CW_EngineRead,roots[i]);
        if(r==-1){Com_Error(ERR_DROP,"CoD1 PATCH64 %s: %s",roots[i],CW_Error());return;}
        if(r>=0)loaded++;
    }
    result=CW_Count()>0;
    if(result) {
        d=CW_Get(0);
        if(!CW_ReloadSupported(d)) { Com_Error(ERR_DROP,"CoD1 PATCH35: unsupported Thompson reload settings"); return; }
        if(!*CW_String(d,"fireTime") || BG_Cod1WeaponShotMsec()<1 || BG_Cod1WeaponShotMsec()>60000 ||
           BG_Cod1WeaponDelayMsec()<0 || BG_Cod1WeaponDelayMsec()>60000) {
            Com_Error(ERR_DROP,"CoD1 PATCH35: invalid Thompson fireTime/fireDelay"); return;
        }
        for(i=0;i<CW_Count();i++) {
            const cwDef_t *root=CW_Get(CW_FamilyRoot(i));
            d=CW_Get(i); hash=(hash^d->hash)*16777619u;
            hash=(hash^(unsigned int)(CW_FamilyRoot(i)+1))*16777619u;
            if(!CW_ReloadSupported(d) || CW_Number(d,"fireTime")<1 || CW_Number(d,"fireTime")>30000 ||
               CW_Number(d,"fireDelay")<0 || CW_Number(d,"fireDelay")>30000 ||
               (CW_Number(d,"aimDownSight") && (CW_Number(d,"adsTransInTime")<1 || CW_Number(d,"adsTransOutTime")<1)) ||
               CW_Number(d,"hipSpreadMax")<0 || CW_Number(d,"hipSpreadMax")>30 || CW_Number(d,"adsSpread")<0 || CW_Number(d,"adsSpread")>30 ||
               (CW_Number(d,"aimDownSight") && (CW_Number(d,"adsZoomFov")<1 || CW_Number(d,"adsZoomFov")>=160)) ||
               CW_Number(d,"clipSize")<1 || CW_Number(d,"clipSize")>999 ||
               CW_Number(d,"maxAmmo")<0 || CW_Number(d,"maxAmmo")>9999 ||
               (strcmp(CW_String(d,"weaponType"),"bullet") && strcmp(CW_String(d,"weaponType"),"grenade") && strcmp(CW_String(d,"weaponType"),"projectile")) ||
               strcmp(CW_String(d,"gunModel"),CW_String(root,"gunModel")) ||
               CW_Number(d,"clipSize")!=CW_Number(root,"clipSize") ||
               strcmp(CW_String(d,"ammoName"),CW_String(root,"ammoName")) ||
               strcmp(CW_String(d,"clipName"),CW_String(root,"clipName"))) {
                Com_Error(ERR_DROP,"CoD1 PATCH35: unsupported alternate weapon parameters: %s",d->name);return;
            }
            Com_Printf("CoD1 PATCH35 WeaponDef %s: %d fields, %d unknown, fireTime=%d ms, alt=%s, hash=%08x\n",
                d->name,d->count,d->unknown,(int)CW_Number(d,"fireTime"),CW_String(d,"altWeapon"),d->hash);
        }
        Com_sprintf(fingerprint,sizeof(fingerprint),"64-%08x",hash);
    } else strcpy(fingerprint,"none");
#ifdef GAMEDLL
    trap_Cvar_Register(&checksumCvar,"cod1_weaponHash","none",CVAR_ROM|CVAR_SERVERINFO);
    trap_Cvar_Set("cod1_weaponHash",fingerprint);
#else
    serverHash=Info_ValueForKey(CG_ConfigString(CS_SERVERINFO),"cod1_weaponHash");
    if((result || *serverHash) && strcmp(serverHash,fingerprint)) {
        Com_Error(ERR_DROP,"CoD1 PATCH38 DLL compatibility mismatch: game=%s cgame=%s. Update both DLLs and use identical weapon files.",serverHash,fingerprint);
        return;
    }
#endif
    if(result) Com_Printf("CoD1 PATCH64: %d weapon definitions in %d families\n",CW_Count(),loaded+1);
    if(result) Com_Printf("CoD1 PATCH36 movement: directional speeds, stance acceleration, ground friction, wall-step guard enabled\n");
    if(result) Com_Printf("CoD1 PATCH35 reload: normal %d/%d ms, empty %d/%d ms (add/finish)\n",CW_ReloadAddDelay(CW_Get(0),0),CW_ReloadDuration(CW_Get(0),0),CW_ReloadAddDelay(CW_Get(0),1),CW_ReloadDuration(CW_Get(0),1));
    if(result) Com_Printf("CoD1 PATCH35: Thompson timings loaded from PK3 (shot %d ms, delay %d ms), registry %s\n",
        BG_Cod1WeaponShotMsec(),BG_Cod1WeaponDelayMsec(),fingerprint);
}
#endif
