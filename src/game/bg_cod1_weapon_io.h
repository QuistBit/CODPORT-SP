#ifndef BG_COD1_WEAPON_IO_H
#define BG_COD1_WEAPON_IO_H
#include <limits.h>
#include <stdint.h>
#include "bg_cod1_weapondef.h"
#include "bg_cod1_reload.h"
#include "bg_cod1_inventory.h"
/* Cold map initialization uses the same ordered mounted weapon files in
 * GAME and CGAME. Inventory slots remain independent of resource ordinals.
 * This does not claim the original engine's cross-DLL warm WeaponDef cache. */
typedef struct {
    unsigned serial;
#ifdef GAMEDLL
    uint64_t epoch;
#else
    int levelStartTime,serverCommandSequence;
    char mapname[MAX_QPATH];
    gameState_t gameState;
#endif
} cwEngineOwner_t;
static unsigned cwEngineSerial;
static int CW_EngineCurrent(void*user) {
    const cwEngineOwner_t*o=(const cwEngineOwner_t*)user;
    if(!o||!o->serial||o->serial!=cwEngineSerial)return 0;
#ifdef GAMEDLL
    return o->epoch && o->epoch==G_Cod1EntityMapEpoch();
#else
    return o->levelStartTime==cgs.levelStartTime &&
        o->serverCommandSequence==cgs.serverCommandSequence &&
        !memcmp(o->mapname,cgs.mapname,sizeof(o->mapname)) &&
        !memcmp(&o->gameState,&cgs.gameState,sizeof(o->gameState));
#endif
}
static int CW_EngineList(void*user,char*data,int capacity) {
    int count;if(!CW_EngineCurrent(user))return -1;
#ifdef GAMEDLL
    count=trap_FS_GetFileList("weapons/sp","",data,capacity);
#else
    count=trap_Cod1FS_GetFileList("weapons/sp","",data,capacity);
#endif
    return CW_EngineCurrent(user)?count:-1;
}
static int CW_EngineTableRead(void*user,const char*path,char*data,int capacity) {
    fileHandle_t handle=0;int size,read;
    if(!CW_EngineCurrent(user))return -1;
    size=trap_FS_FOpenFile(path,&handle,FS_READ);
    if(!handle)return -1;
    if(!CW_EngineCurrent(user)||size<0||size>capacity){trap_FS_FCloseFile(handle);return -2;}
    read=trap_FS_ReadCount(data,size,handle);
    trap_FS_FCloseFile(handle);
    return CW_EngineCurrent(user)&&read==size?size:-1;
}
static int CW_PlayerFamily(int family) {
    int weapon,root;
    for(weapon=1;weapon<WP_NUM_WEAPONS;weapon++){
        root=CW_WeaponRoot(weapon);
        if(root>=0&&CW_FamilyRoot(root)==family)return 1;
    }
    return 0;
}
static void CW_InitEngineWeapons(void) {
    cwEngineOwner_t owner;cwTableHost_t host;
    int result,i,families=0;unsigned hash=2166136261u;
    char fingerprint[16];const cwDef_t*d;
#ifdef GAMEDLL
    static vmCvar_t checksumCvar;
#else
    const char*serverHash;
#endif
    memset(&owner,0,sizeof(owner));
    if(cwEngineSerial==UINT_MAX){Com_Error(ERR_DROP,"CoD1 weapon initialization serial exhausted");return;}
    owner.serial=++cwEngineSerial;
#ifdef GAMEDLL
    owner.epoch=G_Cod1EntityMapEpoch();
#else
    owner.levelStartTime=cgs.levelStartTime;owner.serverCommandSequence=cgs.serverCommandSequence;
    memcpy(owner.mapname,cgs.mapname,sizeof(owner.mapname));owner.gameState=cgs.gameState;
#endif
    host.user=&owner;host.current=CW_EngineCurrent;host.list=CW_EngineList;host.read=CW_EngineTableRead;
    result=CW_LoadOriginalTable(&host);
    if(result<1){Com_Error(ERR_DROP,"CoD1 original weapon table: %s",CW_Error());return;}
    d=CW_Get(CW_Find("thompson"));
    if(!d||!CW_ReloadSupported(d)||BG_Cod1WeaponShotMsec()<1||BG_Cod1WeaponShotMsec()>60000||BG_Cod1WeaponDelayMsec()<0||BG_Cod1WeaponDelayMsec()>60000){Com_Error(ERR_DROP,"CoD1 invalid Thompson timing/reload resource");return;}
    for(i=0;i<CW_Count();i++) {
        int family=CW_FamilyRoot(i);const cwDef_t*root=CW_Get(family);
        d=CW_Get(i);hash=(hash^d->hash)*16777619u;hash=(hash^(unsigned)(family+1))*16777619u;
        if(family==i)families++;
        /* These bounds belong to the retained player-inventory lane, not to
         * turret/non-player resources, which may legitimately have clip0. */
        if(CW_PlayerFamily(family)) {
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

        }
        Com_Printf("CoD1 WeaponDef %d %s fields=%d unknown=%d alt=%s hash=%08x\n",i+1,d->name,d->count,d->unknown,CW_String(d,"altWeapon"),d->hash);
    }
    Com_sprintf(fingerprint,sizeof(fingerprint),"443-%08x",hash);
#ifdef GAMEDLL
    trap_Cvar_Register(&checksumCvar,"cod1_weaponHash","none",CVAR_ROM|CVAR_SERVERINFO);
    trap_Cvar_Set("cod1_weaponHash",fingerprint);
#else
    serverHash=Info_ValueForKey(CG_ConfigString(CS_SERVERINFO),"cod1_weaponHash");
    if(strcmp(serverHash,fingerprint)){Com_Error(ERR_DROP,"CoD1 weapon table mismatch game=%s cgame=%s",serverHash,fingerprint);return;}
#endif
    Com_Printf("CoD1 original ordered weapon bank: %d definitions, %d families, registry %s\n",CW_Count(),families,fingerprint);
}
#endif
