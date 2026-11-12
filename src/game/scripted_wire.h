#ifndef BG_COD1_SCRIPTED_ACTOR_WIRE_H
#define BG_COD1_SCRIPTED_ACTOR_WIRE_H
#include "bg_cod1_turret_actor_wire.h"
#define COD1_SCRIPTED_ACTOR_WIRE_VERSION 25
#define COD1_FRAME_RIFLE_CROUCHRUN_1 441
#define COD1_CROUCHRUN1_FORWARD 92.3618469f
#define COD1_FRAME_RIFLE_CROUCHRUN_2 424
#define COD1_FRAME_WALL_DIVE 425
#define COD1_FRAME_PRONE_CRAWL 426
#define COD1_FRAME_PRECOMBAT_RUN 427
#define COD1_FRAME_LADDER_UP 428
#define COD1_FRAME_LADDER_OFF 429
#define COD1_FRAME_LADDER_ON 430
#define COD1_FRAME_LADDER_DOWN 431
#define COD1_FRAME_RIFLE_CROUCHRUN_3 436
#define COD1_CROUCHRUN3_FORWARD 64.8591156f
#define COD1_FRAME_RIFLE_CROUCHRUN_4 437
#define COD1_CROUCHRUN4_FORWARD 92.3618469f
/* Rifle branch of animscripts/init.gsc, indexed by the live entity number. */
static inline int Cod1CourseCrouchFrame(unsigned slot){
    if(slot%3==2)return COD1_FRAME_RIFLE_CROUCHRUN_3;
    if(slot%3==1 && slot%9>=3)return COD1_FRAME_RIFLE_CROUCHRUN_2;
    if(slot%3==1 || slot%9>=6)return COD1_FRAME_RIFLE_CROUCHRUN_4;
    return COD1_FRAME_RIFLE_CROUCHRUN_1;
}
#define COD1_IS_RIFLE_CROUCHRUN(f) ((f)==COD1_FRAME_RIFLE_CROUCHRUN_1 || (f)==COD1_FRAME_RIFLE_CROUCHRUN_2 || (f)==COD1_FRAME_RIFLE_CROUCHRUN_3 || (f)==COD1_FRAME_RIFLE_CROUCHRUN_4)
/* A tagged stop layer keeps the outgoing course gait and its phase. */
#define COD1_COURSE_STOP_FLAGS 7
#define COD1_COURSE_APPROACH_BIT 8
#define COD1_COURSE_ENTRY_BIT 16
#define COD1_ENTRY_SHUFFLE_B 32
#define COD1_COURSE_EXTRA_FLAGS (COD1_COURSE_APPROACH_BIT|COD1_COURSE_ENTRY_BIT|COD1_ENTRY_SHUFFLE_B)
#define COD1_CLIP_ENTRY_RIGHT 442
#define COD1_CLIP_ENTRY_LEFT 443
#define COD1_CLIP_ENTRY_BACK 444
#define COD1_CLIP_ENTRY_SHUFFLE_A 445
#define COD1_CLIP_ENTRY_SHUFFLE_B 446
#define COD1_CLIP_CROUCH_STOP 440
#define COD1_FRAME_CROUCH_WALK_START 438
#define COD1_FRAME_CROUCH_WALK_LOOP 439
#define COD1_IS_COURSE_WALK_FRAME(f) ((f)==COD1_FRAME_CROUCH_WALK_START || (f)==COD1_FRAME_CROUCH_WALK_LOOP)
#define COD1_FRAME_REST_A 432
#define COD1_IS_REST_FRAME(f) ((f)>=COD1_FRAME_REST_A && (f)<COD1_FRAME_REST_A+4)
#define COD1_IS_LADDER_FRAME(f) ((f)>=COD1_FRAME_LADDER_UP && (f)<=COD1_FRAME_LADDER_DOWN)
#define COD1_PRECOMBAT_RUN_FORWARD 120.409309f
/* Net forward displacement in the original 67-frame prone_crawl loop. */
#define COD1_PRONE_CRAWL_FORWARD 44.294125f
/* Separate semantic profile: never fabricates a gun owner or mounted receipt. */
static int C1ScriptedWireValid(const entityState_t*s,int snapshotTime){unsigned i,g;int baseFlags,approach,entry;
 if(!s)return 0;
 approach=s->cod1TurretActorFlags & COD1_COURSE_APPROACH_BIT;
 entry=s->cod1TurretActorFlags & COD1_COURSE_ENTRY_BIT;
 baseFlags=s->cod1TurretActorFlags & ~COD1_COURSE_EXTRA_FLAGS;
 if(!entry && (s->cod1TurretActorFlags & COD1_ENTRY_SHUFFLE_B))return 0;
 if(entry){
    const float *weights=s->cod1TurretActorPhases[1];
    if(approach || baseFlags!=3 || s->frame!=COD1_FRAME_PRECOMBAT_RUN ||
       !(weights[14]+weights[15]+weights[16]+weights[17]>0) || !(weights[18]>0) ||
       (weights[19]==0 && weights[20]!=0))return 0;
 }
 if(approach && ((baseFlags!=3 && baseFlags!=COD1_COURSE_STOP_FLAGS) ||
    !COD1_IS_RIFLE_CROUCHRUN(s->frame) || s->frame!=Cod1CourseCrouchFrame(s->number) ||
    !(s->cod1TurretActorPhases[1][13]>0 && s->cod1TurretActorPhases[1][13]<1)))return 0;
 if(s->cod1TurretActorVersion!=COD1_SCRIPTED_ACTOR_WIRE_VERSION||s->number<0||s->number>=MAX_GENTITIES||s->eType!=ET_GENERAL||s->pos.trType!=TR_STATIONARY||s->apos.trType!=TR_STATIONARY||!(s->cod1TurretEpochLo|s->cod1TurretEpochHi)||!s->cod1TurretActorGeneration||s->cod1TurretActorGun!=-1||s->cod1TurretActorGunGeneration||s->cod1TurretActorTick<0||s->cod1TurretActorTick>snapshotTime||s->cod1TurretActorConfig<CS_COD1_TURRET_ACTORS||(s->cod1TurretActorConfig>=CS_COD1_TURRET_ACTORS+MAX_COD1_TURRET_ACTORS && !(s->cod1TurretActorConfig==CS_COD1_JUMPER && s->effect1Time==0x4a5202))||(baseFlags!=1&&baseFlags!=3&&baseFlags!=COD1_COURSE_STOP_FLAGS))return 0;
 if(baseFlags==COD1_COURSE_STOP_FLAGS &&
    (!(COD1_IS_RIFLE_CROUCHRUN(s->frame)||COD1_IS_COURSE_WALK_FRAME(s->frame)||COD1_IS_REST_FRAME(s->frame)) ||
     !(s->cod1TurretActorPhases[1][11]>0 && s->cod1TurretActorPhases[1][12]>0)))return 0;
 for(i=0;i<3;i++)if(!(s->pos.trBase[i]>=-1000000&&s->pos.trBase[i]<=1000000)||!(s->apos.trBase[i]>=-3600&&s->apos.trBase[i]<=3600))return 0;for(i=0;i<98;i++)if(!(s->cod1TurretActorGenericCurrent[i]>=0&&s->cod1TurretActorGenericCurrent[i]<=1)||(i>=87&&s->cod1TurretActorGenericCurrent[i]!=0))return 0;if(s->cod1TurretActorGenericCurrent[0]!=1)return 0;for(g=0;g<2;g++){for(i=0;i<25;i++)if(!(s->cod1TurretActorCurrent[g][i]>=0 && s->cod1TurretActorCurrent[g][i]<=1) ||
    (!s->cod1TurretActorCurrent[0][i] && s->cod1TurretActorCurrent[1][i]))return 0;for(i=0;i<21;i++)if(!(s->cod1TurretActorPhases[g][i]>=0&&s->cod1TurretActorPhases[g][i]<=1)||((g*21+i>=(baseFlags==COD1_COURSE_STOP_FLAGS?34:(baseFlags==3?32:31)))&& !((baseFlags==1 || (baseFlags==3 && s->frame==0 && !entry && !approach)) && g==1 && i>=11 && i<=15 && s->cod1TurretActorGenericCurrent[82+i-11]>0) && !(approach && g==1 && i==13) && !(entry && g==1 && i>=14) &&s->cod1TurretActorPhases[g][i]!=0))return 0;}return 1;}
#endif
