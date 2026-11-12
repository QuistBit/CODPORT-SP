#include "cg_cod1_bob.h"
/* PATCH35: model/idle/fire resources and sound aliases come from WeaponDef.
 * PATCH61 keeps immutable resource records per WeaponDef family. */
#ifndef CG_COD1_VIEWMODEL_H
#define CG_COD1_VIEWMODEL_H
#include "../game/bg_cod1_viewprofile.h"
#include "../game/bg_cod1_native_mode.h"
#include "../game/bg_cod1_soundalias.h"
#include "../game/bg_cod1_reload.h"
#include "../game/bg_cod1_combat.h"
#include "cg_cod1_ads.h"
#include "cg_cod1_scope.h"
static qhandle_t cg_cod1ViewModel;
static cwViewAnim cg_cod1FireAnim,cg_cod1ReloadAnim[2],cg_cod1SwitchAnim,cg_cod1AdsFireAnim;
static cwViewAnim cg_cod1StowAnim[2],cg_cod1LastAnim[2],cg_cod1EmptyIdleAnim;
static int cg_cod1LastShot;
static sfxHandle_t cg_cod1EmptyReloadSound,cg_cod1SwitchSound;
static int cg_cod1ShotTime=-10000;
#define CW_CGAME_ALIAS_VARIANTS 4
typedef struct {
    int count;
    cwSoundAliasVariant meta[CW_CGAME_ALIAS_VARIANTS];
    sfxHandle_t sound[CW_CGAME_ALIAS_VARIANTS];
} cgCod1AliasSet;
static cgCod1AliasSet cg_cod1MeleeSwingAlias;
static int cg_cod1MeleeSwingLoaded;
typedef struct {
    qhandle_t model,overlay;
    qhandle_t nativeFallback;
    int nativeAnimated,nativeKind,nativeFailed,hands;
    int muzzleFx;
    cwViewAnim fire,reload[2],selector,adsFire,stow[2],last[2],emptyIdle;
    cwViewAnim extra[CW_VIEW_EXTRA];
    cwViewProfile profile;
    sfxHandle_t fireSound[4],lastSound[4],reloadSound;
    cgCod1AliasSet fireAlias,lastAlias;
    sfxHandle_t emptyReloadSound,switchSound,extraSound[CW_VIEW_EXTRA];
    cgCod1ADSAssets ads;
} cgCod1ViewAssets;
static cgCod1ViewAssets cg_cod1Views[CW_MAX_DEFS];
static int cg_cod1ActiveRoot=-1;
static void CG_Cod1NativeReset(void);
void CG_Cod1ResetViewWeapons(void) {
    CG_Cod1ResetGrenadeFeedback();
    CG_Cod1BrassShutdown();
    CG_Cod1NativeReset();
    memset(cg_cod1Views,0,sizeof(cg_cod1Views));cg_cod1ActiveRoot=-1;
    memset(&cg_cod1MeleeSwingAlias,0,sizeof(cg_cod1MeleeSwingAlias));cg_cod1MeleeSwingLoaded=0;
    cg_cod1ViewModel=0;cg_cod1ShotTime=-10000;cg_cod1LastShot=0;
}

/* Resource preloading never resets the visible weapon. Only selecting a new
 * family (or rebuilding that family's handles) starts a fresh pose lifetime. */
static void CG_Cod1BindView(int root) {
    const cgCod1ViewAssets *v=&cg_cod1Views[root];
    int newFamily=CW_FamilyRoot(root)!=CW_FamilyRoot(cg_cod1ActiveRoot);
    cg_cod1ActiveRoot=root;cg_cod1ViewModel=v->model;
    cg_cod1FireAnim=v->fire;memcpy(cg_cod1ReloadAnim,v->reload,sizeof(v->reload));
    cg_cod1SwitchAnim=v->selector;cg_cod1AdsFireAnim=v->adsFire;
    memcpy(cg_cod1LastAnim,v->last,sizeof(v->last));cg_cod1EmptyIdleAnim=v->emptyIdle;
    memcpy(cg_weapons[cg.predictedPlayerState.weapon].flashSound,v->fireSound,sizeof(v->fireSound));
    memcpy(cg_weapons[cg.predictedPlayerState.weapon].lastShotSound,v->lastSound,sizeof(v->lastSound));
    cg_weapons[cg.predictedPlayerState.weapon].reloadSound=v->reloadSound;
    memcpy(cg_cod1StowAnim,v->stow,sizeof(v->stow));
    cg_cod1EmptyReloadSound=v->emptyReloadSound;cg_cod1SwitchSound=v->switchSound;
    memcpy(cg_cod1AdsRoots,v->ads.roots,sizeof(v->ads.roots));
    memcpy(cg_cod1AdsFrames,v->ads.frames,sizeof(v->ads.frames));
    cg_cod1HipInverse=v->ads.hipInverse;
    if(!newFamily)return;
    CG_Cod1ResetSway();memset(&cg_cod1AdsBlend,0,sizeof(cg_cod1AdsBlend));
    memset(cg_cod1GunOffset,0,sizeof(cg_cod1GunOffset));memset(cg_cod1GunSpeed,0,sizeof(cg_cod1GunSpeed));
    cg_cod1GunTime=-1;cg_cod1ShotTime=-10000;
}

static int CG_Cod1AliasSounds(const char *alias,sfxHandle_t *handles,int capacity) {
    static const char *tables[]={"soundaliases/iw_sound.csv","soundaliases/iw_sound2.csv"};
    char paths[4][CW_VIEW_PATH];int t,n,i,size;fileHandle_t file;char *data;
    memset(handles,0,capacity*sizeof(*handles));
    if(!alias || !*alias)return 0;
    for(t=0;t<2;t++) {
        size=trap_FS_FOpenFile(tables[t],&file,FS_READ);
        if(!file)continue;
        if(size<=0 || size>1048576){trap_FS_FCloseFile(file);continue;}
        data=(char*)malloc(size);if(!data){trap_FS_FCloseFile(file);return 0;}
        trap_FS_Read(data,size,file);trap_FS_FCloseFile(file);
        n=CW_SoundAliasPaths(data,size,alias,paths,capacity>4?4:capacity);free(data);
        if(!n)continue;
        for(i=0;i<n;i++) {
            handles[i]=trap_S_RegisterSound(paths[i]);
            CG_Printf("CoD1 PATCH35 sound: %s -> %s (handle %d)\n",alias,paths[i],handles[i]);
        }
        return n;
    }
    CG_Printf("CoD1 PATCH35: sound alias '%s' not found in retail weapon tables\n",alias);return 0;
}
static int CG_Cod1AliasSetLoad(const char *alias,cgCod1AliasSet *set) {
    static const char *tables[]={"soundaliases/iw_sound.csv","soundaliases/iw_sound2.csv"};
    int t,size,i;fileHandle_t file;char *data;
    memset(set,0,sizeof(*set));if(!alias||!*alias)return 0;
    for(t=0;t<2;t++) {
        size=trap_FS_FOpenFile(tables[t],&file,FS_READ);if(!file)continue;
        if(size<=0||size>1048576){trap_FS_FCloseFile(file);continue;}
        data=(char*)malloc(size);if(!data){trap_FS_FCloseFile(file);return 0;}
        trap_FS_Read(data,size,file);trap_FS_FCloseFile(file);
        set->count=CW_SoundAliasVariants(data,size,alias,set->meta,CW_CGAME_ALIAS_VARIANTS);free(data);
        if(set->count){int w=0;for(i=0;i<set->count;i++)if(CW_SoundAliasLoadSpecMatches(set->meta[i].loadspec,cgs.mapname)){if(w!=i)set->meta[w]=set->meta[i];w++;}set->count=w;}
        if(!set->count)continue;
        for(i=0;i<set->count;i++)set->sound[i]=trap_S_RegisterSound(set->meta[i].path);
        return set->count;
    }
    return 0;
}

/* WeaponDef has melee animation/timing/damage but no melee-swing sound field,
 * and the supplied retail definitions have empty noteTrackSoundA-D.  The
 * original cgame therefore cannot be reconstructed from WeaponDef alone here.
 * Prefer a retail alias named melee_swing; if that name is absent, discover
 * rows whose actual retail file contains "melee_swing".  A final direct-path
 * fallback is accepted only when those files physically exist in the mounted
 * CoD data, so RTCW's fstatck/fstmiss are never substituted silently. */
static int CG_Cod1MeleeSwingLoad(void) {
    static const char *tables[]={"soundaliases/iw_sound.csv","soundaliases/iw_sound2.csv"};
    static const char *direct[]={"sound/weapons/melee/melee_swing01.wav","sound/weapons/melee/melee_swing03.wav"};
    int t,size,i,w;fileHandle_t file;char *data;
    if(cg_cod1MeleeSwingLoaded)return cg_cod1MeleeSwingAlias.count;
    cg_cod1MeleeSwingLoaded=1;
    if(CG_Cod1AliasSetLoad("melee_swing",&cg_cod1MeleeSwingAlias)) {
        CG_Printf("CoD1 HANDHELD_FINAL_R2: melee swing alias 'melee_swing' (%d variants)\n",cg_cod1MeleeSwingAlias.count);
        return cg_cod1MeleeSwingAlias.count;
    }
    for(t=0;t<2;t++) {
        size=trap_FS_FOpenFile(tables[t],&file,FS_READ);if(!file)continue;
        if(size<=0||size>1048576){trap_FS_FCloseFile(file);continue;}
        data=(char*)malloc(size);if(!data){trap_FS_FCloseFile(file);break;}
        trap_FS_Read(data,size,file);trap_FS_FCloseFile(file);
        cg_cod1MeleeSwingAlias.count=CW_SoundAliasVariantsByFileToken(data,size,"melee_swing",
            cg_cod1MeleeSwingAlias.meta,CW_CGAME_ALIAS_VARIANTS);free(data);
        if(cg_cod1MeleeSwingAlias.count){
            w=0;for(i=0;i<cg_cod1MeleeSwingAlias.count;i++)if(CW_SoundAliasLoadSpecMatches(cg_cod1MeleeSwingAlias.meta[i].loadspec,cgs.mapname)){
                if(w!=i)cg_cod1MeleeSwingAlias.meta[w]=cg_cod1MeleeSwingAlias.meta[i];w++;
            }
            cg_cod1MeleeSwingAlias.count=w;
        }
        if(!cg_cod1MeleeSwingAlias.count)continue;
        for(i=0;i<cg_cod1MeleeSwingAlias.count;i++)cg_cod1MeleeSwingAlias.sound[i]=trap_S_RegisterSound(cg_cod1MeleeSwingAlias.meta[i].path);
        CG_Printf("CoD1 HANDHELD_FINAL_R2: discovered retail melee swing files (%d variants)\n",cg_cod1MeleeSwingAlias.count);
        return cg_cod1MeleeSwingAlias.count;
    }
    memset(&cg_cod1MeleeSwingAlias,0,sizeof(cg_cod1MeleeSwingAlias));
    for(i=0;i<2&&cg_cod1MeleeSwingAlias.count<CW_CGAME_ALIAS_VARIANTS;i++) {
        file=0;size=trap_FS_FOpenFile(direct[i],&file,FS_READ);if(!file)continue;trap_FS_FCloseFile(file);
        w=cg_cod1MeleeSwingAlias.count++;
        CW_AliasCopy(cg_cod1MeleeSwingAlias.meta[w].path,sizeof(cg_cod1MeleeSwingAlias.meta[w].path),direct[i]);
        cg_cod1MeleeSwingAlias.meta[w].volumeMin=cg_cod1MeleeSwingAlias.meta[w].volumeMax=1.0f;
        cg_cod1MeleeSwingAlias.meta[w].pitchMin=cg_cod1MeleeSwingAlias.meta[w].pitchMax=1.0f;
        cg_cod1MeleeSwingAlias.meta[w].distMin=120.0f;cg_cod1MeleeSwingAlias.meta[w].distMax=1200.0f;
        cg_cod1MeleeSwingAlias.meta[w].probability=1.0f;
        cg_cod1MeleeSwingAlias.sound[w]=trap_S_RegisterSound(direct[i]);
    }
    if(cg_cod1MeleeSwingAlias.count)CG_Printf("CoD1 HANDHELD_FINAL_R2: direct retail melee swing fallback (%d files)\n",cg_cod1MeleeSwingAlias.count);
    else CG_Printf("CoD1 HANDHELD_FINAL_R2: no retail melee swing sound found; impact sound intentionally deferred\n");
    return cg_cod1MeleeSwingAlias.count;
}

static unsigned cg_cod1AliasRng=0x6d2b79f5u;
static float CG_Cod1AliasUnit(void) { cg_cod1AliasRng=cg_cod1AliasRng*1664525u+1013904223u;return ((cg_cod1AliasRng>>8)&0xffffffu)*(1.0f/16777216.0f); }
static int CG_Cod1AliasSetPick(const cgCod1AliasSet *set) {
    float total=0,r;int i,last=-1;
    if(!set)return -1;
    for(i=0;i<set->count;i++)if(set->sound[i]&&set->meta[i].probability>0){total+=set->meta[i].probability;last=i;}
    if(last<0||total<=0)return -1;
    r=CG_Cod1AliasUnit()*total;
    for(i=0;i<set->count;i++)if(set->sound[i]&&set->meta[i].probability>0){if(r<set->meta[i].probability)return i;r-=set->meta[i].probability;}
    return last;
}
static float CG_Cod1AliasRange(float a,float b) { return a+(b-a)*CG_Cod1AliasUnit(); }
/* Retail CoDSP's alias instance builder samples gain and pitch independently.
 * The gain sample is multiplied by 0.8 before reaching the lower sound path. */
static qboolean CG_Cod1AliasSetPlay(const cgCod1AliasSet *set,const vec3_t origin,int entnum,int channel) {
    int i=CG_Cod1AliasSetPick(set);float gain,pitch;
    if(i<0)return qfalse;
    gain=0.8f*CG_Cod1AliasRange(set->meta[i].volumeMin,set->meta[i].volumeMax);
    pitch=CG_Cod1AliasRange(set->meta[i].pitchMin,set->meta[i].pitchMax);
    trap_Cod1StartAliasSound(origin,entnum,channel,set->sound[i],gain,pitch,set->meta[i].distMin,set->meta[i].distMax);
    return qtrue;
}
static qboolean CG_Cod1RegisterViewWeapon(weaponInfo_t *weapon,const cwDef_t *def) {
    cwViewProfile profile;fileHandle_t file=0;int size,i;unsigned char header[9];char model[MAX_QPATH];
    int requestedKind,nativeSetting=0,selectedHands=cg.predictedPlayerState.cod1ViewHands;
    int root=def?CW_Find(def->name):-1;
    cgCod1ViewAssets *v;
    if(root<0)return qfalse;
    v=&cg_cod1Views[root];
    if(!Cod1ViewHandsName(selectedHands)){CG_Error("Invalid selected CoD hands ID");return qfalse;}
    if(v->model&&v->hands==selectedHands)return qtrue;
    if(cg_cod1ActiveRoot==root){CG_Cod1NativeReset();cg_cod1ActiveRoot=-1;}
    memset(v,0,sizeof(*v));v->hands=selectedHands;
    CG_Printf("CoD1 PATCH37: posture camera curves and WeaponDef stance offsets enabled\n");
    if(!def || !CW_ViewProfile(def,&profile)||!CW_ViewSelectHands(&profile,Cod1ViewHandsName(selectedHands)))return qfalse;
    v->profile=profile;
    size=trap_FS_FOpenFile(profile.fire,&file,FS_READ);
    if(!file)return qfalse;
    if(size>=sizeof(header))trap_FS_Read(header,sizeof(header),file);
    trap_FS_FCloseFile(file);
    if(size<sizeof(header) || !CW_ViewAnimInfo(header,sizeof(header),&v->fire)) {
        CG_Printf("CoD1 PATCH35: unsupported fire clip %s\n",profile.fire);return qfalse;
    }
    for(i=0;i<2;i++) {
        file=0;size=trap_FS_FOpenFile(profile.reload[i],&file,FS_READ);
        if(!file)return qfalse;
        if(size>=sizeof(header))trap_FS_Read(header,sizeof(header),file);
        trap_FS_FCloseFile(file);
        if(size<sizeof(header) || !CW_ViewAnimInfo(header,sizeof(header),&v->reload[i]))return qfalse;
    }
    for(i=0;i<2;i++) {
        const char *path=i?profile.adsFire:profile.selector;
        file=0;size=trap_FS_FOpenFile(path,&file,FS_READ);if(!file)return qfalse;
        if(size>=sizeof(header))trap_FS_Read(header,sizeof(header),file);trap_FS_FCloseFile(file);
        if(size<sizeof(header) || !CW_ViewAnimInfo(header,sizeof(header),i?&v->adsFire:&v->selector))return qfalse;
    }
    for(i=0;i<2;i++) {
        const char *path=i?profile.raise:profile.drop;
        file=0;size=trap_FS_FOpenFile(path,&file,FS_READ);if(!file)return qfalse;
        if(size>=sizeof(header))trap_FS_Read(header,sizeof(header),file);trap_FS_FCloseFile(file);
        if(size<sizeof(header) || !CW_ViewAnimInfo(header,sizeof(header),&v->stow[i]))return qfalse;
    }
    for(i=0;i<3;i++) {
        const char *path=i==0?profile.lastFire:i==1?profile.lastAdsFire:profile.emptyIdle;
        cwViewAnim *a=i<2?&v->last[i]:&v->emptyIdle;
        file=0;size=trap_FS_FOpenFile(path,&file,FS_READ);if(!file)return qfalse;
        if(size>=sizeof(header))trap_FS_Read(header,sizeof(header),file);trap_FS_FCloseFile(file);
        if(size<sizeof(header) || !CW_ViewAnimInfo(header,sizeof(header),a))return qfalse;
    }
    for(i=0;i<CW_VIEW_EXTRA;i++)if(*profile.extra[i]) {
        file=0;size=trap_FS_FOpenFile(profile.extra[i],&file,FS_READ);if(!file)return qfalse;
        if(size>=sizeof(header))trap_FS_Read(header,sizeof(header),file);trap_FS_FCloseFile(file);
        if(size<sizeof(header) || !CW_ViewAnimInfo(header,sizeof(header),&v->extra[i]))return qfalse;
    }
    if(!CG_Cod1LoadADS(&profile,&v->ads)){CG_Error("CoD1 PATCH35: failed to read ADS root clips");return qfalse;}
    if(strlen(def->name)+12>=sizeof(model))return qfalse;
    {
        char nativeFlag[16];
        trap_Cvar_VariableStringBuffer("r_cod1NativeXModel",nativeFlag,sizeof(nativeFlag));
        nativeSetting=atoi(nativeFlag);
        if(!CW_ViewResourceName(model,sizeof(model),CW_NativeViewPrefix(def,nativeSetting),def->name,selectedHands)) {
            CG_Error("Selected CoD viewmodel resource name overflow");return qfalse;
        }
    }
    requestedKind=!Q_stricmpn(model,"cod1native2/",12)?2:
                  !Q_stricmpn(model,"cod1native/",11)?1:0;
    /* The legacy profile omits some Native-2 leaves (e.g. altRaiseAnim).
     * In mode 3 give each definition its own model so alternate families
     * cannot silently share another definition's XAnim resources. */
    for(i=0;CW_NativeViewMayShare(nativeSetting,requestedKind) && i<CW_Count();i++)if(cg_cod1Views[i].model &&
       cg_cod1Views[i].nativeKind==requestedKind &&
       !memcmp(&cg_cod1Views[i].profile,&profile,sizeof(profile))) {
        v->model=cg_cod1Views[i].model;break;
    }
    if(!v->model)v->model=trap_R_RegisterModel(model);
    if(!v->model && !Q_stricmpn(model,"cod1native2/",12)) {
        char detail[256];trap_Cvar_VariableStringBuffer("r_cod1ViewFail",detail,sizeof(detail));
        CG_Printf("CoD1 Native-2 unavailable (%s); trying accepted fallback%s%s\n",
                  model,detail[0]?": ":"",detail);
        if(!CW_ViewResourceName(model,sizeof(model),!Q_stricmp(def->name,"thompson")?
            "cod1native/":"cod1view64/",def->name,selectedHands))return qfalse;
        v->model=trap_R_RegisterModel(model);
    }
    if(!v->model && !Q_stricmpn(model,"cod1native/",11)) {
        char fallback[MAX_QPATH],detail[256];
        trap_Cvar_VariableStringBuffer("r_cod1ViewFail",detail,sizeof(detail));
        CG_Printf("CoD1 Native-1 unavailable (%s); falling back to accepted adapter%s%s\n",model,detail[0]?": ":"",detail);
        if(!CW_ViewResourceName(fallback,sizeof(fallback),"cod1view64/",def->name,selectedHands))return qfalse;
        v->model=trap_R_RegisterModel(fallback);
        Q_strncpyz(model,fallback,sizeof(model));
    }
    if(!v->model){
        char detail[256];trap_Cvar_VariableStringBuffer("r_cod1ViewFail",detail,sizeof(detail));
        if(detail[0])CG_Error("CoD1 viewmodel failed (%s)\nRenderer: %s",model,detail);
        else CG_Error("CoD1 PATCH35 viewmodel failed: update WolfSP.exe and check XAnim assets (%s)",model);
        return qfalse;
    } else {
        char warning[512];
        v->nativeAnimated=!Q_stricmpn(model,"cod1native2/",12);
        v->nativeKind=v->nativeAnimated?2:!Q_stricmpn(model,"cod1native/",11)?1:0;
        trap_Cvar_VariableStringBuffer("r_cod1ViewWarn",warning,sizeof(warning));
        if(warning[0])CG_Printf("CoD1 viewmodel warning: %s\n",warning);
    }
    if(*CW_String(def,"adsOverlayShader")) {
        v->overlay=trap_R_RegisterShaderNoMip(CW_String(def,"adsOverlayShader"));
        if(!v->overlay)CG_Printf("CoD1 PATCH62: missing scope shader %s; check original UI textures\n",CW_String(def,"adsOverlayShader"));
    }
    weapon->weaponModel[W_FP_MODEL]=v->model;
    v->muzzleFx=CG_Cod1EfxRegisterMuzzle(CW_String(def,"viewFlashEffect"));
    CG_Cod1BrassPrecache(def);
    CG_Cod1AliasSounds(CW_String(def,"fireSound"),v->fireSound,4);
    CG_Cod1AliasSounds(CW_String(def,"lastShotSound"),v->lastSound,4);
    CG_Cod1AliasSetLoad(CW_String(def,"fireSound"),&v->fireAlias);
    CG_Cod1AliasSetLoad(CW_String(def,"lastShotSound"),&v->lastAlias);
    memcpy(weapon->flashSound,v->fireSound,sizeof(v->fireSound));
    memcpy(weapon->lastShotSound,v->lastSound,sizeof(v->lastSound));
    CG_Cod1AliasSounds(CW_String(def,"reloadSound"),&v->reloadSound,1);weapon->reloadSound=v->reloadSound;
    CG_Cod1AliasSounds(CW_String(def,"reloadEmptySound"),&v->emptyReloadSound,1);
    CG_Cod1AliasSounds(CW_String(def,"altSwitchSound"),&v->switchSound,1);
    CG_Cod1AliasSounds(CW_String(def,"rechamberSound"),&v->extraSound[CW_VIEW_BOLT],1);
    CG_Cod1AliasSounds(CW_String(def,"reloadStartSound"),&v->extraSound[CW_VIEW_RELOAD_START],1);
    CG_Cod1AliasSounds(CW_String(def,"reloadEndSound"),&v->extraSound[CW_VIEW_RELOAD_END],1);
    CG_Cod1AliasSounds(CW_String(def,"pullbackSound"),&v->extraSound[CW_VIEW_HOLD],1);
    CG_Printf("CoD1 PATCH35 reload clips: normal=%d empty=%d frames\n",v->reload[0].frames,v->reload[1].frames);
    weapon->flashDlightColor[0]=1;weapon->flashDlightColor[1]=0.7f;weapon->flashDlightColor[2]=0.5f;
    CG_Printf("CoD1 PATCH35 view weapon %s: %s, %d fire frames at %d fps\n",def->name,profile.gun,v->fire.frames,v->fire.fps);
    if(CW_PSDef(&cg.predictedPlayerState)==def)CG_Cod1BindView(root);
    return qtrue;
}
static qboolean CG_Cod1PlayFireAlias(const entityState_t *ent,qboolean lastShot) {
    const cwDef_t *def;int root;const cgCod1AliasSet *set;
    if(!ent||(def=CW_WeaponDef(ent->weapon))==NULL)return qfalse;
    root=CW_Find(def->name);if(root<0)return qfalse;
    if(!cg_cod1Views[root].model)CG_RegisterWeapon(ent->weapon);
    if(!cg_cod1Views[root].model)return qfalse;
    set=lastShot&&cg_cod1Views[root].lastAlias.count?&cg_cod1Views[root].lastAlias:&cg_cod1Views[root].fireAlias;
    return CG_Cod1AliasSetPlay(set,NULL,ent->number,CHAN_WEAPON);
}


qboolean CG_Cod1MeleeStartEvent(centity_t *cent) {
    if(!cent)return qfalse;
    if(!CG_Cod1MeleeSwingLoad())return qfalse;
    return CG_Cod1AliasSetPlay(&cg_cod1MeleeSwingAlias,NULL,cent->currentState.number,CHAN_WEAPON);
}

static qboolean CG_Cod1SelectViewWeapon(const playerState_t *ps) {
    const cwDef_t *def=CW_PSDef(ps);
    int root=def?CW_Find(def->name):-1;
    if(root<0 || ps->aiChar) {
        cg_cod1ActiveRoot=-1;cg_cod1ViewModel=0;CG_Cod1ResetSway();
        cg_cod1GunTime=-1;cg_cod1ShotTime=-10000;
        return qfalse;
    }
    CG_RegisterWeapon(ps->weapon);
    if((!cg_cod1Views[root].model||cg_cod1Views[root].hands!=ps->cod1ViewHands) && !CG_Cod1RegisterViewWeapon(&cg_weapons[ps->weapon],def))return qfalse;
    if(cg_cod1ActiveRoot!=root)CG_Cod1BindView(root);
    return qtrue;
}
#include "cg_cod1_native_anim.h"
qhandle_t CG_Cod1ScopeShader(void) {
    if(cg_cod1ActiveRoot<0 || CW_PSDef(&cg.predictedPlayerState)!=CW_Get(cg_cod1ActiveRoot))return 0;
    return cg_cod1Views[cg_cod1ActiveRoot].overlay;
}
static qboolean CG_Cod1DrawViewWeapon(void) {
    refEntity_t gun;
    cgCod1ViewAssets *current;
    const playerState_t *ps=&cg.predictedPlayerState;
    const cwDef_t *definition;
    int key,scopeHidden,nativePose;
    if(!CG_Cod1SelectViewWeapon(&cg.predictedPlayerState))return qfalse;
    current=&cg_cod1Views[cg_cod1ActiveRoot];
    definition=CW_PSDef(ps);
    key=CW_FamilyRoot(cg_cod1ActiveRoot)+1;
    if(cg.predictedPlayerState.stats[STAT_COD1_EDGES]&CW_LADDER_HIDDEN) {
        CG_Cod1ResetSway();cg_cod1GunTime=-1;cg_cod1ShotTime=-10000;
        memset(cg_cod1GunOffset,0,sizeof(cg_cod1GunOffset));memset(cg_cod1GunSpeed,0,sizeof(cg_cod1GunSpeed));
        return qtrue; /* handled: never fall back to the RTCW weapon */
    }
    memset(&gun,0,sizeof(gun));gun.hModel=cg_cod1ViewModel;
    VectorCopy(cg.refdef.vieworg,gun.origin);VectorCopy(gun.origin,gun.oldorigin);
    VectorCopy(gun.origin,gun.lightingOrigin);AxisCopy(cg.refdef.viewaxis,gun.axis);
    gun.renderfx=RF_DEPTHHACK|RF_FIRST_PERSON|RF_MINLIGHT|RF_LIGHTING_ORIGIN;
    if(cg.predictedPlayerState.weaponstate==WEAPON_DROPPING || cg.predictedPlayerState.weaponstate==WEAPON_RAISING) {
        int raising=cg.predictedPlayerState.weaponstate==WEAPON_RAISING;
        int base=1+cg_cod1FireAnim.frames+cg_cod1ReloadAnim[0].frames+cg_cod1ReloadAnim[1].frames+
            cg_cod1SwitchAnim.frames+cg_cod1AdsFireAnim.frames+(raising?cg_cod1StowAnim[0].frames:0);
        CW_ViewReloadFrame(&cg_cod1StowAnim[raising],base,cg.predictedPlayerState.weaponTime,
            CW_StowTime(CW_PSDef(&cg.predictedPlayerState),raising),&gun.oldframe,&gun.frame,&gun.backlerp);
    } else if(cg.predictedPlayerState.weaponstate==WEAPON_RELOADING &&
       (cg.predictedPlayerState.stats[STAT_COD1_RELOAD]==COD1_RELOAD_NORMAL || cg.predictedPlayerState.stats[STAT_COD1_RELOAD]==COD1_RELOAD_EMPTY)) {
        int empty=cg.predictedPlayerState.stats[STAT_COD1_RELOAD]==COD1_RELOAD_EMPTY;
        int base=1+cg_cod1FireAnim.frames+(empty?cg_cod1ReloadAnim[0].frames:0);
        CW_ViewReloadFrame(&cg_cod1ReloadAnim[empty],base,cg.predictedPlayerState.weaponTime,CW_ReloadDuration(CW_PSDef(&cg.predictedPlayerState),empty),&gun.oldframe,&gun.frame,&gun.backlerp);
    } else if(cg.predictedPlayerState.weaponstate==WEAPON_COD1_SWITCH) {
        int base=1+cg_cod1FireAnim.frames+cg_cod1ReloadAnim[0].frames+cg_cod1ReloadAnim[1].frames;
        CW_ViewReloadFrame(&cg_cod1SwitchAnim,base,cg.predictedPlayerState.weaponTime,CW_SwitchTime(CW_PSDef(&cg.predictedPlayerState)),&gun.oldframe,&gun.frame,&gun.backlerp);
    } else {
        int ads=CW_ADS(&cg.predictedPlayerState)>=1;
        int base=0,extra=cg_cod1FireAnim.frames+cg_cod1ReloadAnim[0].frames+cg_cod1ReloadAnim[1].frames+cg_cod1SwitchAnim.frames;
        const cwViewAnim *anim=ads?&cg_cod1AdsFireAnim:&cg_cod1FireAnim;
        if(ads)base=extra;
        extra+=cg_cod1AdsFireAnim.frames+cg_cod1StowAnim[0].frames+cg_cod1StowAnim[1].frames;
        if(cg_cod1LastShot){anim=&cg_cod1LastAnim[ads];base=extra+(ads?cg_cod1LastAnim[0].frames:0);}
        CW_ViewFireFrame(anim,cg.time-cg_cod1ShotTime,&gun.oldframe,&gun.frame,&gun.backlerp);
        if(gun.oldframe){gun.oldframe+=base;gun.frame+=base;}
        else if(!cg.predictedPlayerState.ammoclip[BG_FindClipForWeapon(cg.predictedPlayerState.weapon)]) {
            gun.oldframe=gun.frame=1+extra+cg_cod1LastAnim[0].frames+cg_cod1LastAnim[1].frames;
        }
    }
    {
        const playerState_t *ps=&cg.predictedPlayerState;const cwDef_t *d=CW_PSDef(ps);
        const cgCod1ViewAssets *v=&cg_cod1Views[cg_cod1ActiveRoot];int extra=-1,total=0,i;
        int phase=ps->stats[STAT_COD1_RELOAD]&~COD1_RELOAD_STOP;
        int base=1+v->fire.frames+v->reload[0].frames+v->reload[1].frames+v->selector.frames+
            v->adsFire.frames+v->stow[0].frames+v->stow[1].frames+v->last[0].frames+v->last[1].frames+v->emptyIdle.frames;
        if(ps->weaponstate==WEAPON_COD1_RECHAMBER){extra=CW_ADS(ps)>.001f?CW_VIEW_ADSBOLT:CW_VIEW_BOLT;total=(int)CW_Number(d,"rechamberTime");}
        else if(ps->weaponstate==WEAPON_COD1_HOLD){extra=CW_VIEW_HOLD;total=(int)CW_Number(d,"holdFireTime");}
        else if(ps->weaponstate==WEAPON_COD1_MELEE){extra=CW_VIEW_MELEE;total=(int)CW_Number(d,"meleeTime");}
        else if(ps->weaponstate==WEAPON_RELOADING) {
            if(phase==COD1_RELOAD_START){extra=CW_VIEW_RELOAD_START;total=(int)CW_Number(d,"reloadStartTime");}
            else if(phase==COD1_RELOAD_END){extra=CW_VIEW_RELOAD_END;total=(int)CW_Number(d,"reloadEndTime");}
            else if(phase==COD1_RELOAD_LOOP)CW_ViewReloadFrame(&v->reload[0],1+v->fire.frames,ps->weaponTime,
                CW_ReloadDuration(d,0),&gun.oldframe,&gun.frame,&gun.backlerp);
        }
        if(extra>=0) {
            for(i=0;i<extra;i++)base+=v->extra[i].frames;
            CW_ViewReloadFrame(&v->extra[extra],base,ps->weaponTime,total,&gun.oldframe,&gun.frame,&gun.backlerp);
        }
    }
    scopeHidden=CG_Cod1ScopeShader() && CW_ADS(ps)>=1;
    nativePose=current->nativeAnimated && !current->nativeFailed;
    CG_Cod1PositionADSMode(&gun,nativePose);
    if(nativePose) {
        cod1NativeAnimState_t state;
        cod1NativeViewTags_t tags;
        int submitted=0;
        if(CG_Cod1NativeState(ps,definition,&state)) {
            state.flags=scopeHidden?0:COD1_NATIVE_DRAW_VIEW;
            submitted=trap_R_AddCod1NativeView(&gun,&state,&tags);
        }
        if(submitted) {
            CG_Cod1BrassViewPose(key,&gun,
                tags.brassIndex>=0?&tags.brass:NULL,qtrue);
            if(!scopeHidden)CG_Cod1EfxMuzzleViewPose(current->muzzleFx,
                key,&gun,tags.flashIndex>=0?&tags.flash:NULL,qtrue);
            return qtrue;
        }
        /* A runtime pose failure cannot safely draw the immutable Native-2
         * model as if it were animated. Thompson has accepted static Native-1;
         * MP40 goes to the previously playable adapter. */
        if(!current->nativeFailed) {
            char fallback[MAX_QPATH];
            if(!CW_ViewResourceName(fallback,sizeof(fallback),!Q_stricmp(definition->name,"thompson")?
                "cod1native/":"cod1view64/",definition->name,current->hands))return qfalse;
            current->nativeFallback=trap_R_RegisterModel(fallback);
            if(!current->nativeFallback && !Q_stricmp(definition->name,"thompson")) {
                if(!CW_ViewResourceName(fallback,sizeof(fallback),"cod1view64/",definition->name,current->hands))return qfalse;
                current->nativeFallback=trap_R_RegisterModel(fallback);
            }
            current->nativeFailed=1;
            CG_Printf("CoD1 Native-2 pose unavailable for %s; accepted fallback %s\n",
                definition->name,current->nativeFallback?"ready":"missing");
        }
        if(!current->nativeFallback)return qtrue;
        CG_Cod1PositionADS(&gun);
    }
    if(current->nativeAnimated) {
        if(!current->nativeFallback)return qtrue;
        gun.hModel=current->nativeFallback;
    }
    CG_Cod1BrassView(key,&gun);
    if(scopeHidden)return qtrue;
    trap_R_AddRefEntityToScene(&gun);
    CG_Cod1EfxMuzzleView(current->muzzleFx,key,&gun);
    return qtrue;
}
/* EV_FILL_CLIP retains its legacy meaning (reload begins), with an explicit
 * variant parameter. Event processing already suppresses prediction repeats. */
qboolean CG_Cod1ReloadEvent(centity_t *cent,int variant) {
    sfxHandle_t sound;
    int weapon=cent->currentState.weapon,root=CW_WeaponRoot(weapon);
    const cgCod1ViewAssets *v;
    if(root<0 || variant<1 || variant>8)return qfalse;
    CG_RegisterWeapon(weapon);
    if(cent->currentState.number==cg.predictedPlayerState.clientNum && weapon==cg.predictedPlayerState.weapon) {
        CG_Cod1SelectViewWeapon(&cg.predictedPlayerState);root=cg_cod1ActiveRoot;
    }
    v=&cg_cod1Views[root];
    sound=variant==3?v->switchSound:variant==2?v->emptyReloadSound:
        variant==4?v->extraSound[CW_VIEW_RELOAD_START]:variant==6?v->extraSound[CW_VIEW_RELOAD_END]:
        variant==7?v->extraSound[CW_VIEW_BOLT]:variant==8?v->extraSound[CW_VIEW_HOLD]:v->reloadSound;
    /* Original cgame 0x30016e3d..0x30016e7e: normal and empty cues are
     * alternatives, not mandatory independent sounds. Do not apply this to
     * segmented start/loop/end, bolt, hold or selector events. The original
     * WAV (notably Garand) already contains its leading timing silence. */
    if(!sound && variant==1) sound=v->emptyReloadSound;
    if(!sound && variant==2) sound=v->reloadSound;
    if(sound)trap_S_StartSound(NULL,cent->currentState.number,CHAN_WEAPON,sound);
    if(cent->currentState.number==cg.predictedPlayerState.clientNum && weapon==cg.predictedPlayerState.weapon)cg_cod1ShotTime=-10000;
    return qtrue;
}
#endif
