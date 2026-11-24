#ifndef BG_COD1_UNMOUNTED_ACTOR_WIRE_H
#define BG_COD1_UNMOUNTED_ACTOR_WIRE_H
#include "bg_cod1_turret_actor_wire.h"
#define COD1_UNMOUNTED_ACTOR_WIRE_VERSION 3
#define COD1_UNMOUNTED_MG_FLAGS 2
/* Separate semantic profile: never fabricates a gun owner or mounted receipt. */
static int C1UAW_Valid(const entityState_t*s,int snapshotTime){unsigned i,g;if(!s||s->cod1TurretActorVersion!=3||s->number<0||s->number>=MAX_GENTITIES||s->eType!=ET_GENERAL||s->pos.trType!=TR_STATIONARY||s->apos.trType!=TR_STATIONARY||!(s->cod1TurretEpochLo|s->cod1TurretEpochHi)||!s->cod1TurretActorGeneration||s->cod1TurretActorGun!=-1||s->cod1TurretActorGunGeneration||s->cod1TurretActorTick<0||s->cod1TurretActorTick>snapshotTime||s->cod1TurretActorConfig<CS_COD1_TURRET_ACTORS||s->cod1TurretActorConfig>=CS_COD1_TURRET_ACTORS+MAX_COD1_TURRET_ACTORS||(s->cod1TurretActorFlags!=1 && s->cod1TurretActorFlags!=COD1_UNMOUNTED_MG_FLAGS))return 0;for(i=0;i<3;i++)if(!(s->pos.trBase[i]>=-1000000&&s->pos.trBase[i]<=1000000)||!(s->apos.trBase[i]>=-3600&&s->apos.trBase[i]<=3600))return 0;for(i=0;i<98;i++)if(!(s->cod1TurretActorGenericCurrent[i]>=0&&s->cod1TurretActorGenericCurrent[i]<=1)||(s->cod1TurretActorFlags==1 && i>=50&&s->cod1TurretActorGenericCurrent[i]!=0))return 0;if(s->cod1TurretActorGenericCurrent[0]!=1)return 0;for(g=0;g<2;g++){for(i=0;i<25;i++)if(s->cod1TurretActorCurrent[g][i]!=0)return 0;for(i=0;i<21;i++)if(!(s->cod1TurretActorPhases[g][i]>=0&&s->cod1TurretActorPhases[g][i]<=1)||(s->cod1TurretActorFlags==1 && (g||i>=2)&&s->cod1TurretActorPhases[g][i]!=0))return 0;}return 1;}
#endif
