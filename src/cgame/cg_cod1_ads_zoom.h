#ifndef CG_COD1_ADS_ZOOM_H
#define CG_COD1_ADS_ZOOM_H
#include "cg_cod1_ads_math.h"
/* Called before both FOV and sensitivity; independent of drawing the gun. */
static struct {
    CodADSZoom curve;
    int valid,time,client,teleport,spawnCount,weapon;
} cg_cod1AdsZoom;
static void CG_Cod1UpdateADSZoom(void) {
    const playerState_t *ps=&cg.predictedPlayerState;float ads=CW_ADS(ps);
    int teleport=ps->eFlags&EF_TELEPORT_BIT;
    if(!CW_Active(ps) || ps->pm_type!=PM_NORMAL || ps->stats[STAT_HEALTH]<=0 ||
       ps->persistant[PERS_HWEAPON_USE]) {cg_cod1AdsZoom.valid=0;return;}
    if(!cg_cod1AdsZoom.valid || ps->weapon!=cg_cod1AdsZoom.weapon || cg.time<cg_cod1AdsZoom.time ||
       ps->clientNum!=cg_cod1AdsZoom.client || teleport!=cg_cod1AdsZoom.teleport ||
       ps->persistant[PERS_SPAWN_COUNT]!=cg_cod1AdsZoom.spawnCount) {
        cg_cod1AdsZoom.valid=1;cg_cod1AdsZoom.weapon=ps->weapon;cg_cod1AdsZoom.curve.previous=ads;
        cg_cod1AdsZoom.curve.rising=!!(ps->stats[STAT_COD1_EDGES]&CW_ADS_RISING);
        cg_cod1AdsZoom.client=ps->clientNum;cg_cod1AdsZoom.teleport=teleport;
        cg_cod1AdsZoom.spawnCount=ps->persistant[PERS_SPAWN_COUNT];
    } else COD1_ADSZoomUpdate(&cg_cod1AdsZoom.curve,ads);
    cg_cod1AdsZoom.time=cg.time;
}
static float CG_Cod1ADSFov(const playerState_t *ps,float base) {
    const cwDef_t *d=CW_PSDef(ps);
    return COD1_ADSZoomFov(&cg_cod1AdsZoom.curve,CW_ADS(ps),base,
        (float)CW_Number(d,"adsZoomFov"),(float)CW_Number(d,"adsZoomInFrac"),
        (float)CW_Number(d,"adsZoomOutFrac"));
}
#endif
