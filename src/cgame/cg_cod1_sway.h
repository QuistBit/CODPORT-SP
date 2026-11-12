#ifndef CG_COD1_SWAY_H
#define CG_COD1_SWAY_H
#include "cg_cod1_sway_math.h"
#include "cg_cod1_idle_math.h"
#include "cg_cod1_pose_math.h"
#include "cg_cod1_bob.h"
#include "../game/bg_cod1_lean.h"
/* PATCH44: lifetime and coordinate bridge to the current posed-mesh adapter.
 * One sample per cg.time, shared by stereo passes. Registering new ADS roots,
 * hiding/changing the weapon, teleporting or rewinding time starts a new sample.
 * This clock is independent of cg.frametime, updated later in CG_DrawActiveFrame.
 */
static struct {
    CodSwayState motion;
    float idleFactor,idleAngles[3],gunAngles[2];
    float posePosition[3],poseRotation[3],poseOffset[3],poseAngles[3],bobAngles[3];
    int valid,time,client,teleport,spawnCount;
} cg_cod1Sway;
static void CG_Cod1ResetSway(void) {memset(&cg_cod1Sway,0,sizeof(cg_cod1Sway));}
static int CG_Cod1SwayVisible(const playerState_t *ps) {
    return CW_Active(ps) && ps->pm_type==PM_NORMAL && ps->stats[STAT_HEALTH]>0 &&
        ps->persistant[PERS_TEAM]!=TEAM_SPECTATOR && !cg.renderingThirdPerson &&
        cg_drawGun.integer && !cg.testGun && !(ps->eFlags&EF_MG42_ACTIVE) &&
        !ps->persistant[PERS_HWEAPON_USE] && !(ps->stats[STAT_COD1_EDGES]&CW_LADDER_HIDDEN);
}
static CodSwayParams CG_Cod1SwayParams(const cwDef_t *d,int ads) {
    CodSwayParams p;
    p.maxAngle=(float)CW_Number(d,ads?"adsSwayMaxAngle":"swayMaxAngle");
    p.lerpSpeed=(float)CW_Number(d,ads?"adsSwayLerpSpeed":"swayLerpSpeed");
    p.pitchScale=(float)CW_Number(d,ads?"adsSwayPitchScale":"swayPitchScale");
    p.yawScale=(float)CW_Number(d,ads?"adsSwayYawScale":"swayYawScale");
    p.horizScale=(float)CW_Number(d,ads?"adsSwayHorizScale":"swayHorizScale");
    p.vertScale=(float)CW_Number(d,ads?"adsSwayVertScale":"swayVertScale");
    return p;
}
/* Idle shares the frame/lifetime guard with sway, but keeps absolute time phase.
 * On a fresh visible sample seed the current stance; ordinary stance changes
 * use the native 0.0005/ms filter. This reset policy belongs to the adapter. */
static void CG_Cod1UpdateIdle(const playerState_t *ps,const cwDef_t *d,int dt,int seed) {
    CodIdleParams p;int stance;
    p.hipAmount=(float)CW_Number(d,"hipIdleAmount");
    p.adsAmount=(float)CW_Number(d,"adsIdleAmount");
    p.crouchFactor=(float)CW_Number(d,"idleCrouchFactor");
    p.proneFactor=(float)CW_Number(d,"idleProneFactor");
    p.aimDownSight=CW_Number(d,"aimDownSight")!=0;
    stance=(ps->pm_flags&PMF_COD1_PRONE)?2:(ps->pm_flags&PMF_DUCKED)?1:0;
    if(seed)cg_cod1Sway.idleFactor=COD1_IdleTarget(&p,stance);
    VectorClear(cg_cod1Sway.idleAngles);
    COD1_IdleAngles(&cg_cod1Sway.idleFactor,&p,stance,CW_ADS(ps),dt,cg.time,cg_cod1Sway.idleAngles);
}
static CodPoseDef CG_Cod1PoseParams(const cwDef_t *d,int angular) {
    static const char *move[2][3][3]={
        {{"standMoveF","standMoveR","standMoveU"},{"duckedMoveF","duckedMoveR","duckedMoveU"},{"proneMoveF","proneMoveR","proneMoveU"}},
        {{"standRotP","standRotY","standRotR"},{"duckedRotP","duckedRotY","duckedRotR"},{"proneRotP","proneRotY","proneRotR"}}};
    static const char *minimum[2][3]={{"standMoveMinSpeed","duckedMoveMinSpeed","proneMoveMinSpeed"},
        {"standRotMinSpeed","duckedRotMinSpeed","proneRotMinSpeed"}};
    static const char *offset[2][3]={{"duckedOfsF","duckedOfsR","duckedOfsU"},{"proneOfsF","proneOfsR","proneOfsU"}};
    CodPoseDef p;int i,j;memset(&p,0,sizeof(p));
    for(i=0;i<3;i++) {
        p.minSpeed[i]=(float)CW_Number(d,minimum[angular][i]);
        for(j=0;j<3;j++)p.move[i][j]=(float)CW_Number(d,move[angular][i][j]);
    }
    if(!angular)for(i=0;i<2;i++)for(j=0;j<3;j++)p.offset[i][j]=(float)CW_Number(d,offset[i][j]);
    p.rate=(float)CW_Number(d,angular?"posRotRate":"posMoveRate");
    p.proneRate=(float)CW_Number(d,angular?"posProneRotRate":"posProneMoveRate");
    return p;
}
static void CG_Cod1UpdatePose(const playerState_t *ps,const cwDef_t *d,int dt) {
    CodPoseDef position=CG_Cod1PoseParams(d,0),rotation=CG_Cod1PoseParams(d,1);
    CodPoseModifiers mods;int stance=CG_Cod1BobStance(ps),flags=stance==2?64:stance==1?32:0;
    int desired=stance==2?11:stance==1?40:60,ws=ps->weaponstate==WEAPON_RELOADING?5:0;
    float actual=cg.cod1Posture.valid?cg.cod1Posture.height:(float)desired;
    /* Retail ground path uses horizontal velocity length, not held commands.
     * Modifiers are stock zero (confirmed by the original console dump).
     * Developer override cvars and the ladder-specific speed path are separate. */
    memset(&mods,0,sizeof(mods));VectorClear(cg_cod1Sway.poseOffset);VectorClear(cg_cod1Sway.poseAngles);
    COD1_PosePosition(cg_cod1Sway.posePosition,cg_cod1Sway.poseOffset,&position,&mods,
        flags,desired,actual,40,11,cg.xyspeed,ps->speed,ws,dt,CW_ADS(ps));
    COD1_PoseRotation(cg_cod1Sway.poseRotation,cg_cod1Sway.poseAngles,&rotation,&mods,
        flags,desired,actual,40,11,cg.xyspeed,ps->speed,ws,dt,CW_ADS(ps));
    VectorClear(cg_cod1Sway.bobAngles);
    if(CG_Cod1BobActive(ps))cod_bob_weapon(stance,cod_bob_phase(ps->bobCycle),cg.xyspeed,
        CW_ADS(ps),(float)CW_Number(d,"adsBobFactor"),cg_cod1Sway.bobAngles);
}
static void CG_Cod1UpdateSway(void) {
    const playerState_t *ps=&cg.predictedPlayerState;
    const cwDef_t *d=CW_PSDef(ps);CodSwayParams hip,aim;
    int teleport=ps->eFlags&EF_TELEPORT_BIT,dt;
    if(!CG_Cod1SwayVisible(ps)){CG_Cod1ResetSway();return;}
    if(!cg_cod1Sway.valid || cg.time<cg_cod1Sway.time ||
        ps->clientNum!=cg_cod1Sway.client || teleport!=cg_cod1Sway.teleport ||
        ps->persistant[PERS_SPAWN_COUNT]!=cg_cod1Sway.spawnCount) {
        CG_Cod1ResetSway();cg_cod1Sway.valid=1;cg_cod1Sway.time=cg.time;
        cg_cod1Sway.client=ps->clientNum;cg_cod1Sway.teleport=teleport;
        cg_cod1Sway.spawnCount=ps->persistant[PERS_SPAWN_COUNT];
        VectorCopy(cg.refdefViewAngles,cg_cod1Sway.motion.previous);
        CG_Cod1UpdateIdle(ps,d,0,1);CG_Cod1UpdatePose(ps,d,0);return;
    }
    dt=cg.time-cg_cod1Sway.time;if(!dt)return;
    hip=CG_Cod1SwayParams(d,0);aim=CG_Cod1SwayParams(d,1);
    COD1_SwayUpdate(&cg_cod1Sway.motion,&hip,&aim,CW_ADS(ps),cg.refdefViewAngles,dt);
    CG_Cod1UpdateIdle(ps,d,dt,0);
    CG_Cod1UpdatePose(ps,d,dt);
    cg_cod1Sway.time=cg.time;
}
/* Local coordinates are forward/LEFT/up. Rotate the extracted ADS root
 * position as well as its basis: applying angles only would choose a false
 * pivot at that root. Translate after rotation, before the camera transform.
 * PATCH49: stance/movement offsets are rotated by bob, then translated;
 * movement angles, idle and bob are summed before the final Euler matrix.
 */
/* PATCH57: one Euler sum drives both the rendered root and shot angles. */
static void CG_Cod1ProceduralAngles(vec3_t angles) {
    const playerState_t *ps=&cg.predictedPlayerState;
    int i;
    for(i=0;i<3;i++)angles[i]=cg_cod1Sway.idleAngles[i]+cg_cod1Sway.poseAngles[i]+cg_cod1Sway.bobAngles[i];
    for(i=0;i<2;i++)angles[i]+=cg_cod1Sway.gunAngles[i];
    angles[0]-=cg_cod1Sway.motion.angle[0];angles[1]-=cg_cod1Sway.motion.angle[1];
    if(!CW_Number(CW_PSDef(ps),"aimDownSight"))for(i=0;i<3;i++)angles[i]-=cg.kickAngles[i];
    if(BG_Cod1LeanActive(ps))angles[2]+=COD1_LeanMul(COD1_LeanCurve(ps->leanf),-2);
}
static void CG_Cod1ApplySway(float position[3],float axis[3][3]) {
    const CodSwayState *s=&cg_cod1Sway.motion;
    vec3_t angles,swayAxis[3],bobAxis[3],result[3],origin,leanPosition={0,0,0};int i,j,k;
    const playerState_t *ps=&cg.predictedPlayerState;
    float lean=BG_Cod1LeanActive(ps)?ps->leanf:0;
    CG_Cod1ProceduralAngles(angles);
    /* Original local translation fades with 1-ADS before bob rotation. */
    if(CW_ADS(ps)<1)COD1_LeanOffset(leanPosition,0,lean,-2,COD1_LeanMul(1-CW_ADS(ps),1.6f));
    if(!angles[0] && !angles[1] && !angles[2] && !s->offset[0] && !s->offset[1] &&
       !cg_cod1Sway.poseOffset[0] && !cg_cod1Sway.poseOffset[1] && !cg_cod1Sway.poseOffset[2])return;
    AnglesToAxis(angles,swayAxis);
    AnglesToAxis(cg_cod1Sway.bobAngles,bobAxis);
    for(i=0;i<3;i++) {
        origin[i]=0;for(k=0;k<3;k++)origin[i]+=swayAxis[k][i]*position[k];
        /* Original accumulator outputs already use forward/LEFT/up.
         * Do not negate the fields named OfsR/MoveR a second time. */
        for(k=0;k<3;k++)origin[i]+=bobAxis[k][i]*(cg_cod1Sway.poseOffset[k]+leanPosition[k]);
        for(j=0;j<3;j++) {
            result[i][j]=0;for(k=0;k<3;k++)result[i][j]+=swayAxis[k][j]*axis[i][k];
        }
    }
    origin[1]-=s->offset[0];origin[2]+=s->offset[1];
    VectorCopy(origin,position);AxisCopy(result,axis);
}
#endif
