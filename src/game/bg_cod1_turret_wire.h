#ifndef BG_COD1_TURRET_WIRE_H
#define BG_COD1_TURRET_WIRE_H
/* Shared sample validation. A wire sample is not server permission to shoot. */
#define COD1_TURRET_WIRE_VERSION 2
#define COD1_TURRET_MODEL_ALIAS "cod1turret/mg42_bipod"
static int C1TW_Valid(const entityState_t *s,int snapshotTime) {
 int k;
 if(!s||s->cod1TurretVersion!=COD1_TURRET_WIRE_VERSION||s->number<0||s->number>=MAX_GENTITIES||
    (!s->cod1TurretEpochLo&&!s->cod1TurretEpochHi)||!s->cod1TurretGeneration||
    s->cod1TurretTick<0||s->cod1TurretTick>snapshotTime||s->eType!=ET_GENERAL||
    s->modelindex<1||s->modelindex>=MAX_MODELS||s->pos.trType!=TR_STATIONARY||s->apos.trType!=TR_STATIONARY)return 0;
 for(k=0;k<3;k++)if(!(s->cod1TurretAngles[k]>=-3600&&s->cod1TurretAngles[k]<=3600)||
    !(s->pos.trBase[k]>=-1000000&&s->pos.trBase[k]<=1000000)||
    !(s->apos.trBase[k]>=-3600&&s->apos.trBase[k]<=3600))return 0;
 if((s->loopSound!=0&&s->loopSound!=1)||(s->cod1TurretAnimated!=0&&s->cod1TurretAnimated!=1))return 0;
 for(k=0;k<2;k++)if(!(s->cod1TurretPhases[k]>=0&&s->cod1TurretPhases[k]<=1)||!(s->cod1TurretWeights[k]>=0&&s->cod1TurretWeights[k]<=1)||(!s->cod1TurretAnimated&&(s->cod1TurretPhases[k]!=0||s->cod1TurretWeights[k]!=0)))return 0;
 if(s->cod1TurretAnimated&&!(s->cod1TurretWeights[0]>0||s->cod1TurretWeights[1]>0))return 0;
 return 1;
}
#endif
