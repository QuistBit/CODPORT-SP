/* PATCH47: two retail ADS leaves over the existing rigid tag_torso adapter. */
#ifndef CG_COD1_ADS_H
#define CG_COD1_ADS_H
#include "../renderer/cod1_viewmodel14.h"
#include "cg_cod1_sway.h"
#include "cg_cod1_ads_math.h"
#include "cg_cod1_ads_tracks.h"
#include "cg_cod1_aim_math.h"
#include "cg_cod1_landing.h"
typedef struct {
    cv14Transform roots[2][128],hipInverse;
    int frames[2];
} cgCod1ADSAssets;
static qboolean CG_Cod1SelectViewWeapon(const playerState_t *ps);
static cv14Transform cg_cod1AdsRoots[2][128],cg_cod1HipInverse;
static int cg_cod1AdsFrames[2],cg_cod1GunTime=-1;
static float cg_cod1GunOffset[2],cg_cod1GunSpeed[2];
static struct {
    float weight[2];
    int valid,time,client,teleport,spawnCount;
} cg_cod1AdsBlend;
static int CG_Cod1LoadADS(const cwViewProfile *profile,cgCod1ADSAssets *assets) {
    int i,n,index,ok;fileHandle_t f;unsigned char *data;cv14Pose *pose;
    if(!*profile->hip) {
        memset(assets,0,sizeof(*assets));CV14_Identity(&assets->hipInverse);
        for(i=0;i<2;i++){assets->frames[i]=1;CV14_Identity(&assets->roots[i][0]);}
        return 1;
    }
    for(i=0;i<2;i++) {
        const char *path=i?profile->adsDown:profile->hip;
        n=trap_FS_FOpenFile(path,&f,FS_READ);if(!f)return 0;
        if(n<=0 || n>65536){trap_FS_FCloseFile(f);return 0;}
        data=(unsigned char*)malloc(n);pose=(cv14Pose*)malloc(sizeof(*pose));
        if(!data || !pose){free(data);free(pose);trap_FS_FCloseFile(f);return 0;}
        trap_FS_Read(data,n,f);trap_FS_FCloseFile(f);
        ok=COD1_ADSReadRoots(data,n,assets->roots[i],&assets->frames[i]);
        /* Remove exactly the hip root baked by the existing renderer reader. */
        if(ok && !i) {
            ok=CV14_FirstPose(data,n,pose) && pose->n==1 && (index=CV14_Find(pose,"tag_torso"))>=0;
            if(ok)assets->hipInverse=pose->t[index];
        }
        free(data);free(pose);if(!ok)return 0;
    }
    {
        cv14Transform hip=assets->hipInverse;
        for(i=0;i<3;i++)assets->hipInverse.q[i]=-hip.q[i];
        CV14_Rotate(assets->hipInverse.q,hip.p,assets->hipInverse.p);
        for(i=0;i<3;i++)assets->hipInverse.p[i]=-assets->hipInverse.p[i];
    }
    return 1;
}
static cv14Transform CG_Cod1ADSRoot(const playerState_t *ps) {
    cv14Transform up,down;float a=CW_ADS(ps);int dt;
    /* Retail leaf goals use retained intent and the current reload timer,
     * independently of the direction used before timers in the last Pmove. */
    int rising=CW_ADSTarget(ps,CW_PSDef(ps)),teleport=ps->eFlags&EF_TELEPORT_BIT;
    if(!cg_cod1AdsBlend.valid || !cg_cod1Sway.valid || cg.time<cg_cod1AdsBlend.time ||
       ps->clientNum!=cg_cod1AdsBlend.client || teleport!=cg_cod1AdsBlend.teleport ||
       ps->persistant[PERS_SPAWN_COUNT]!=cg_cod1AdsBlend.spawnCount) {
        /* Adapter lifetime reset: no previous visible animation to blend from. */
        cg_cod1AdsBlend.valid=1;cg_cod1AdsBlend.time=cg.time;
        cg_cod1AdsBlend.client=ps->clientNum;cg_cod1AdsBlend.teleport=teleport;
        cg_cod1AdsBlend.spawnCount=ps->persistant[PERS_SPAWN_COUNT];
        cg_cod1AdsBlend.weight[0]=(float)rising;cg_cod1AdsBlend.weight[1]=(float)!rising;
    }
    dt=cg.time-cg_cod1AdsBlend.time;
    if(dt>0) {
        cg_cod1AdsBlend.weight[0]=COD1_ADSWeight(cg_cod1AdsBlend.weight[0],(float)rising,dt*.001f);
        cg_cod1AdsBlend.weight[1]=COD1_ADSWeight(cg_cod1AdsBlend.weight[1],(float)!rising,dt*.001f);
        cg_cod1AdsBlend.time=cg.time;
    }
    up=COD1_ADSSampleRoot(cg_cod1AdsRoots[0],cg_cod1AdsFrames[0],a);
    down=COD1_ADSSampleRoot(cg_cod1AdsRoots[1],cg_cod1AdsFrames[1],1-a);
    return COD1_ADSBlendRoots(&up,&down,cg_cod1AdsBlend.weight[0],cg_cod1AdsBlend.weight[1]);
}
void CG_Cod1FireRecoil(void) {
    playerState_t *ps=&cg.predictedPlayerState;const cwDef_t *d=CW_PSDef(ps);
    float a=(rand()&32767)*(1.f/32768.f),b=(rand()&32767)*(1.f/32768.f);
    int aimed=CW_ADS(ps)>=1;
    if(!CG_Cod1SelectViewWeapon(ps))return;
    cg.kickAVel[PITCH]=-CW_KickRange(d,aimed?"adsViewKickPitchMin":"hipViewKickPitchMin",aimed?"adsViewKickPitchMax":"hipViewKickPitchMax",a);
    cg.kickAVel[YAW]=CW_KickRange(d,aimed?"adsViewKickYawMin":"hipViewKickYawMin",aimed?"adsViewKickYawMax":"hipViewKickYawMax",b);
    cg.kickAVel[ROLL]=-.5f*cg.kickAVel[YAW];
    aimed=CW_ADS(ps)>0;
    a=(rand()&32767)*(1.f/32768.f);b=(rand()&32767)*(1.f/32768.f);
    cg_cod1GunSpeed[0]+=CW_KickRange(d,aimed?"adsGunKickPitchMin":"hipGunKickPitchMin",aimed?"adsGunKickPitchMax":"hipGunKickPitchMax",a);
    cg_cod1GunSpeed[1]+=CW_KickRange(d,aimed?"adsGunKickYawMin":"hipGunKickYawMin",aimed?"adsGunKickYawMax":"hipGunKickYawMax",b);
}
static void CG_Cod1PositionADSMode(refEntity_t *gun,qboolean nativePose) {
    cv14Transform target,delta;
    const cwDef_t *d=CW_PSDef(&cg.predictedPlayerState);int i,j,k,dt=cg.time-cg_cod1GunTime;
    float ads=CW_ADS(&cg.predictedPlayerState),local[3][3];
    float hip,accel,maximum,decay,drag,limits[2];
    if(!nativePose)target=CG_Cod1ADSRoot(&cg.predictedPlayerState);
    hip=(float)CW_Number(d,"hipGunKickAccel");accel=hip+((float)CW_Number(d,"adsGunKickAccel")-hip)*ads;
    hip=(float)CW_Number(d,"hipGunKickSpeedMax");maximum=hip+((float)CW_Number(d,"adsGunKickSpeedMax")-hip)*ads;
    hip=(float)CW_Number(d,"hipGunKickSpeedDecay");decay=hip+((float)CW_Number(d,"adsGunKickSpeedDecay")-hip)*ads;
    hip=(float)CW_Number(d,"hipGunKickStaticDecay");drag=hip+((float)CW_Number(d,"adsGunKickStaticDecay")-hip)*ads;
    limits[0]=(float)CW_Number(d,"gunMaxPitch");limits[1]=(float)CW_Number(d,"gunMaxYaw");
    CG_Cod1UpdateSway();
    /* Resource load already clears state. Do not discard a first-frame shot. */
    if(cg_cod1GunTime<0)dt=0;
    else if(dt<0 || dt>200){dt=0;memset(cg_cod1GunOffset,0,sizeof(cg_cod1GunOffset));memset(cg_cod1GunSpeed,0,sizeof(cg_cod1GunSpeed));}
    cg_cod1GunTime=cg.time;
    if(CW_Number(d,"aimDownSight")) {
        while(dt>0){int step=dt>5?5:dt;for(i=0;i<2;i++)CW_GunSpring(&cg_cod1GunOffset[i],&cg_cod1GunSpeed[i],step*.001f,limits[i],accel,maximum,decay,drag);dt-=step;}
        for(i=0;i<2;i++)cg_cod1Sway.gunAngles[i]=cg_cod1GunOffset[i];
    } else cg_cod1Sway.gunAngles[0]=cg_cod1Sway.gunAngles[1]=0;
    /* Native-2 puts ADS-up/down in tag_torso, inside the submitted DObj.
     * Keep the existing procedural camera, recoil and sway on the entity,
     * but never apply that ADS root a second time to its world transform. */
    if(nativePose)CV14_Identity(&delta);
    else CV14_Compose(&target,&cg_cod1HipInverse,&delta);
    AxisClear(local);
    for(i=0;i<3;i++){vec3_t basis;VectorCopy(local[i],basis);CV14_Rotate(delta.q,basis,local[i]);}
    CG_Cod1ApplySway(delta.p,local);
    for(i=0;i<3;i++) {
        gun->origin[i]=cg.refdef.vieworg[i];
        for(j=0;j<3;j++)gun->origin[i]+=cg.refdef.viewaxis[j][i]*delta.p[j];
    }
    for(i=0;i<3;i++) {
        for(j=0;j<3;j++){gun->axis[i][j]=0;for(k=0;k<3;k++)gun->axis[i][j]+=cg.refdef.viewaxis[k][j]*local[i][k];}
    }
    /* Native suffix applies in world Z after the camera transform, also in ADS. */
    gun->origin[2]+=COD1_LandWeaponOffset(cg.time-cg.landTime,cg.landChange);
    VectorCopy(gun->origin,gun->oldorigin);
}
static void CG_Cod1PositionADS(refEntity_t *gun) {CG_Cod1PositionADSMode(gun,qfalse);}
/* PATCH56: pose state feeds the shot, excluding the animation root. At hip
 * the retail producer selects camera angles; at ADS it selects weapon angles.
 * Existing procedural pose/recoil adapters supply the state, not server RNG. */
static vec3_t cg_cod1ScopeAim;
static int cg_cod1ScopeAimValid;
void CG_Cod1ScopeCenter(float *x,float *y) {
    vec3_t forward;float z;
    *x=cg.refdef.x+cg.refdef.width*.5f;*y=cg.refdef.y+cg.refdef.height*.5f;
    if(!cg_cod1ScopeAimValid || cg.refdef.fov_x<=0 || cg.refdef.fov_y<=0)return;
    AngleVectors(cg_cod1ScopeAim,forward,NULL,NULL);z=DotProduct(forward,cg.refdef.viewaxis[0]);
    if(z<=0)return;
    *x-=cg.refdef.width*.5f*DotProduct(forward,cg.refdef.viewaxis[1])/(z*tan(cg.refdef.fov_x*M_PI/360));
    *y-=cg.refdef.height*.5f*DotProduct(forward,cg.refdef.viewaxis[2])/(z*tan(cg.refdef.fov_y*M_PI/360));
}
void CG_Cod1SendWeaponAim(void) {
    const playerState_t *ps=&cg.predictedPlayerState;
    const cwDef_t *d=CW_PSDef(ps);vec3_t local;float aim[2];
    if(!CG_Cod1SelectViewWeapon(ps) || !CW_Active(ps) || ps->pm_type!=PM_NORMAL || ps->stats[STAT_HEALTH]<=0 ||
       ps->persistant[PERS_TEAM]==TEAM_SPECTATOR || cg.renderingThirdPerson || cg.testGun ||
       (ps->eFlags&EF_MG42_ACTIVE) || ps->persistant[PERS_HWEAPON_USE] ||
       (ps->stats[STAT_COD1_EDGES]&CW_LADDER_HIDDEN)) {
        cg_cod1ScopeAimValid=0;trap_Cod1SetWeaponAngles(0,0,0);return;
    }
    if(cg_cod1GunTime!=cg.time && cg_cod1AdsFrames[0]>0 && !cg.testGun) {
        refEntity_t unused;
        int draw=cg_drawGun.integer;
        /* Native calculates procedural pose even when submission is hidden. */
        cg_drawGun.integer=1;CG_Cod1PositionADS(&unused);cg_drawGun.integer=draw;
    }
    CG_Cod1ProceduralAngles(local);
    COD1_WeaponAimAngles(cg.refdefViewAngles,local,CW_ADS(ps),
        CW_Number(d,"aimDownSight")!=0,aim);
    VectorSet(cg_cod1ScopeAim,aim[0],aim[1],0);cg_cod1ScopeAimValid=1;
    trap_Cod1SetWeaponAngles(1,aim[0],aim[1]);
}
#endif
