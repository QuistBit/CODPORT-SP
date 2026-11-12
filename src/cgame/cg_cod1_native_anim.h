/* Native CoD1 first-person XAnim scheduler. Include after cg_cod1_viewmodel.h.
 * Indices match the original cgame's 20 independent XAnimTree registrations;
 * a missing WeaponDef animation FIELD aliases idleAnim's resource, not its
 * runtime node. This state is local to cgame and never enters the protocol.
 */
#ifndef CG_COD1_NATIVE_ANIM_H
#define CG_COD1_NATIVE_ANIM_H

#include "../game/bg_cod1_native_runtime.h"

enum {
    CN_IDLE=1, CN_EMPTY_IDLE, CN_FIRE, CN_HOLD, CN_LAST_FIRE,
    CN_RECHAMBER, CN_MELEE, CN_RELOAD, CN_RELOAD_EMPTY,
    CN_RELOAD_START, CN_RELOAD_END, CN_RAISE, CN_DROP,
    CN_ALT_RAISE, CN_ALT_DROP, CN_ADS_FIRE, CN_ADS_LAST,
    CN_ADS_RECHAMBER, CN_ADS_UP, CN_ADS_DOWN
};

typedef struct {
    int ready;
    unsigned int hash;
    int lastFrame[COD1_NATIVE_LEAVES], fps[COD1_NATIVE_LEAVES];
    int duration[COD1_NATIVE_LEAVES], loop[COD1_NATIVE_LEAVES];
} cgCod1NativeDefinition;
typedef struct {
    int valid, time, client, teleport, spawn, definition, action, shotTime;
    int remaining, actionStart;
    float weight[COD1_NATIVE_LEAVES], leafTime[COD1_NATIVE_LEAVES];
} cgCod1NativeInstance;

static cgCod1NativeDefinition cg_cod1NativeDef[CW_MAX_DEFS];
static cgCod1NativeInstance cg_cod1NativeView[CW_MAX_DEFS];
static int cg_cod1NativeSelected=-1;

static void CG_Cod1NativeReset(void) {
    memset(cg_cod1NativeDef,0,sizeof(cg_cod1NativeDef));
    memset(cg_cod1NativeView,0,sizeof(cg_cod1NativeView));
    cg_cod1NativeSelected=-1;
}

static float CG_Cod1NativeClamp(float t) {
    return t<=0?0:t>=1?1:t;
}
static int CG_Cod1NativeIsShot(int action) {
    return action==CN_FIRE || action==CN_LAST_FIRE ||
        action==CN_ADS_FIRE || action==CN_ADS_LAST;
}

static int CG_Cod1NativeLoadDefinition(int index,const cwDef_t *def) {
    static const char *fields[COD1_NATIVE_LEAVES]={
        NULL,"idleAnim","emptyIdleAnim","fireAnim","holdFireAnim",
        "lastShotAnim","rechamberAnim","meleeAnim","reloadAnim",
        "reloadEmptyAnim","reloadStartAnim","reloadEndAnim","raiseAnim",
        "dropAnim","altRaiseAnim","altDropAnim","adsFireAnim",
        "adsLastShotAnim","adsRechamberAnim","adsUpAnim","adsDownAnim"
    };
    cgCod1NativeDefinition *c=&cg_cod1NativeDef[index];
    char path[CW_VIEW_PATH];fileHandle_t f;unsigned char header[9];
    int i,size,frames,bones,fps;
    if(c->ready && c->hash==def->hash)return c->ready>0;
    memset(c,0,sizeof(*c));c->hash=def->hash;
    for(i=1;i<COD1_NATIVE_LEAVES;i++) {
        const char *name=CW_String(def,fields[i]);
        /* The empty field gets an independent leaf backed by idle's bytes.
         * A nonempty but absent resource is an error, not an idle fallback. */
        if(!*name)name=CW_String(def,"idleAnim");
        if(!CW_ViewPath(path,name,"xanim/",0))goto fail;
        f=0;size=trap_FS_FOpenFile(path,&f,FS_READ);
        if(!f)goto fail;
        if(size<9 || size>8*1024*1024){trap_FS_FCloseFile(f);goto fail;}
        trap_FS_Read(header,sizeof(header),f);trap_FS_FCloseFile(f);
        frames=header[2]+256*header[3];bones=header[4]+256*header[5];
        fps=header[7]+256*header[8];
        if(header[0]!=14 || header[1] || (header[6]&~1) ||
           frames<1 || frames-1+(header[6]&1)>=4096 ||
           bones<1 || bones>128 || fps<1 || fps>1000)goto fail;
        c->lastFrame[i]=frames-1+(header[6]&1);
        c->fps[i]=fps;
        c->duration[i]=c->lastFrame[i]*1000/fps;
        c->loop[i]=header[6]&1;
    }
    c->ready=1;return 1;
fail:
    CG_Printf("CoD1 native XAnim: missing/unsupported leaf %d for %s (%s)\n",i,def->name,path);
    c->ready=-1;return 0;
}

/* When the original overrides an XAnim playback rate, its effective clip
 * duration is the weapon timing; a zero timing freezes that leaf. Its
 * reloadEndAnim uses reloadTime, not reloadEndTime (original 0x30027AE7). */
static int CG_Cod1NativeDuration(const cwDef_t *def,const cgCod1NativeDefinition *c,int leaf) {
    const char *timing=NULL;
    int n;
    switch(leaf) {
    case CN_HOLD:timing="holdFireTime";break;
    case CN_MELEE:timing="meleeTime";break;
    case CN_RELOAD:case CN_RELOAD_END:timing="reloadTime";break;
    case CN_RELOAD_EMPTY:timing="reloadEmptyTime";break;
    case CN_RELOAD_START:timing="reloadStartTime";break;
    case CN_RAISE:timing="raiseTime";break;
    case CN_DROP:timing="dropTime";break;
    case CN_ALT_RAISE:timing="altRaiseTime";break;
    case CN_ALT_DROP:timing="altDropTime";break;
    default:break;
    }
    if(timing) {
        n=(int)CW_Number(def,timing);
        return n>0?n:0;
    }
    return c->duration[leaf];
}

/* Length of the simulation state may differ from the XAnim duration. This
 * matters for rechamber and for reload-end (whose registered rate is based
 * on the complete reloadTime). */
static int CG_Cod1NativeStateDuration(const cwDef_t *def,int leaf,int fallback) {
    switch(leaf) {
    case CN_RECHAMBER:case CN_ADS_RECHAMBER:return (int)CW_Number(def,"rechamberTime");
    case CN_RELOAD_END:return (int)CW_Number(def,"reloadEndTime");
    case CN_RELOAD_START:return (int)CW_Number(def,"reloadStartTime");
    case CN_RAISE:return CW_StowTime(def,1);
    case CN_DROP:return CW_StowTime(def,0);
    default:return fallback;
    }
}

/* Leaf indices and per-leaf resource/rate registration were traced in the
 * original. The mapping from this port's WEAPON_* and STAT_COD1_RELOAD states
 * to those leaves is provisional for segmented reload and alternate switch;
 * live original action timing/notetracks have not yet been captured. */
static int CG_Cod1NativeAction(const playerState_t *ps,const cwDef_t *def,
                               const cgCod1NativeDefinition *c,int *remaining) {
    int phase=ps->stats[STAT_COD1_RELOAD]&~COD1_RELOAD_STOP;
    int aimed=CW_ADS(ps)>=1;
    int clip=BG_FindClipForWeapon(ps->weapon);
    int empty=clip>=0 && !ps->ammoclip[clip];
    *remaining=ps->weaponTime;
    switch(ps->weaponstate) {
    case WEAPON_DROPPING:
    case WEAPON_DROPPING_TORELOAD:return CN_DROP;
    case WEAPON_RAISING:
    case WEAPON_RAISING_TORELOAD:return CN_RAISE;
    case WEAPON_COD1_SWITCH: {
        int drop=(int)CW_Number(def,"altDropTime");
        int elapsed=CW_SwitchTime(def)-ps->weaponTime;
        if(drop<0)drop=0;
        if(elapsed<drop || (int)CW_Number(def,"altRaiseTime")<=0) {
            *remaining=elapsed<drop?drop-elapsed:0;return CN_ALT_DROP;
        }
        *remaining=ps->weaponTime;return CN_ALT_RAISE;
    }
    case WEAPON_COD1_RECHAMBER:return CW_ADS(ps)>.001f?CN_ADS_RECHAMBER:CN_RECHAMBER;
    case WEAPON_COD1_MELEE:return CN_MELEE;
    case WEAPON_COD1_HOLD:return CN_HOLD;
    case WEAPON_RELOADING:
        if(phase==COD1_RELOAD_START)return CN_RELOAD_START;
        if(phase==COD1_RELOAD_END)return CN_RELOAD_END;
        if(phase==COD1_RELOAD_LOOP)return CN_RELOAD;
        return phase==COD1_RELOAD_EMPTY?CN_RELOAD_EMPTY:CN_RELOAD;
    default:break;
    }
    /* A predicted shot has its own event clock and can outlive WEAPON_FIRING.
     * Its XAnim plays at the file's FPS; weaponTime is the refire lockout. */
    if(cg_cod1ShotTime>=0 && cg.time>=cg_cod1ShotTime &&
       cg.time-cg_cod1ShotTime<CG_Cod1NativeDuration(def,c,
           aimed?(cg_cod1LastShot?CN_ADS_LAST:CN_ADS_FIRE):
                 (cg_cod1LastShot?CN_LAST_FIRE:CN_FIRE))) {
        *remaining=-1;
        if(aimed)return cg_cod1LastShot?CN_ADS_LAST:CN_ADS_FIRE;
        return cg_cod1LastShot?CN_LAST_FIRE:CN_FIRE;
    }
    *remaining=-1;
    return empty?CN_EMPTY_IDLE:CN_IDLE;
}

/* A snapshot for one first-person DObj. Returns false so the caller can use
 * the existing adapter if any of this definition's clips cannot be read. */
static qboolean CG_Cod1NativeState(const playerState_t *ps,const cwDef_t *def,
                                   cod1NativeAnimState_t *out) {
    int index,root,reset,action,remaining,dt,i,elapsed,duration,adsGoal;
    cgCod1NativeInstance *v;const cgCod1NativeDefinition *c;
    float aim;
    if(!out)return qfalse;
    memset(out,0,sizeof(*out));out->version=COD1_NATIVE_STATE_VERSION;out->size=sizeof(*out);
    if(!ps || !def || (index=CW_Find(def->name))<0 || index>=CW_MAX_DEFS)return qfalse;
    root=CW_FamilyRoot(index);if(root<0 || root>=CW_MAX_DEFS)return qfalse;
    if(!CG_Cod1NativeLoadDefinition(index,def))return qfalse;
    c=&cg_cod1NativeDef[index];v=&cg_cod1NativeView[root];
    reset=!v->valid || cg.time<v->time || cg.time-v->time>1000 ||
        v->client!=ps->clientNum || v->teleport!=(ps->eFlags&EF_TELEPORT_BIT) ||
        v->spawn!=ps->persistant[PERS_SPAWN_COUNT] ||
        v->definition!=index || cg_cod1NativeSelected!=root;
    if(reset) {
        memset(v,0,sizeof(*v));v->valid=1;v->action=-1;v->shotTime=-1;
        v->client=ps->clientNum;v->teleport=ps->eFlags&EF_TELEPORT_BIT;
        v->spawn=ps->persistant[PERS_SPAWN_COUNT];v->definition=index;
        v->weight[CW_ADSTarget(ps,def)?CN_ADS_UP:CN_ADS_DOWN]=1;
    }
    dt=cg.time-v->time;if(reset || dt<0)dt=0;
    if(dt>200)dt=200;
    cg_cod1NativeSelected=root;
    action=CG_Cod1NativeAction(ps,def,c,&remaining);
    if(action<1 || action>CN_ADS_RECHAMBER)return qfalse;
    if(action!=v->action || (remaining>=0 && remaining>v->remaining+20) ||
       (CG_Cod1NativeIsShot(action) && cg_cod1ShotTime!=v->shotTime)) {
        v->action=action;v->actionStart=cg.time;
        v->leafTime[action]=0;
    }
    v->shotTime=cg_cod1ShotTime;v->remaining=remaining;
    /* An action changes immediately; resource aliases still have distinct
     * array entries and clocks. Preserve inactive clocks for a future restart. */
    for(i=1;i<=CN_ADS_RECHAMBER;i++)v->weight[i]=i==action?1:0;
    duration=CG_Cod1NativeDuration(def,c,action);
    if(duration>0) {
        if(remaining>=0) {
            int total=CG_Cod1NativeStateDuration(def,action,duration);
            /* Countdown owns the state lifetime; reload-end deliberately
             * retains the original reloadTime XAnim rate instead of forcing
             * a full clip into this shorter simulation phase. */
            if(total<1)total=duration;
            elapsed=total-remaining;
        } else elapsed=cg.time-(CG_Cod1NativeIsShot(action)?cg_cod1ShotTime:v->actionStart);
        if(elapsed<0)elapsed=0;
        v->leafTime[action]=CG_Cod1NativeClamp(elapsed/(float)duration);
        if(c->loop[action] && elapsed>=duration && duration>0)
            v->leafTime[action]=(elapsed%duration)/(float)duration;
    } else {
        /* One-frame idle is a fixed pose; a zero-speed action stays at its
         * first sample, as in the original timing override. */
        v->leafTime[action]=(action==CN_IDLE || action==CN_EMPTY_IDLE)?1:0;
    }
    aim=CW_ADS(ps);adsGoal=CW_ADSTarget(ps,def);
    if(CW_Number(def,"aimDownSight")) {
        if(!reset && dt>0) {
            v->weight[CN_ADS_UP]=COD1_ADSWeight(v->weight[CN_ADS_UP],(float)adsGoal,dt*.001f);
            v->weight[CN_ADS_DOWN]=COD1_ADSWeight(v->weight[CN_ADS_DOWN],(float)!adsGoal,dt*.001f);
        }
        v->leafTime[CN_ADS_UP]=aim;
        v->leafTime[CN_ADS_DOWN]=1-aim;
    } else v->weight[CN_ADS_UP]=v->weight[CN_ADS_DOWN]=0;
    v->time=cg.time;
    for(i=1;i<COD1_NATIVE_LEAVES;i++) {
        out->weight[i]=v->weight[i];out->time[i]=v->leafTime[i];
    }
    return qtrue;
}
#endif
