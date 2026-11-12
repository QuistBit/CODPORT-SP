/* Current snapshot owns all inputs. Never use client interpolation or renderer
 * pose as server fire authority. Bad typed packets have no generic fallback. */
#include "../game/bg_cod1_turret_wire.h"
static int CG_Cod1TurretEntity(centity_t *cent) {
 const entityState_t *s=&cent->currentState;cod1TurretRenderState_t state;
 if(!s->cod1TurretVersion)return 0;
 if(!cgs.cod1MapEntities||!cg.snap||!C1TW_Valid(s,cg.snap->serverTime)||
    (s->eFlags&EF_NODRAW)||strcmp(CG_ConfigString(CS_MODELS+s->modelindex),COD1_TURRET_MODEL_ALIAS)||
    !cgs.gameModels[s->modelindex])return 1;
 memset(&state,0,sizeof(state));state.version=COD1_TURRET_RENDER_VERSION;state.size=sizeof(state);
 state.hModel=cgs.gameModels[s->modelindex];state.ownerNum=s->number;
 state.epochLo=s->cod1TurretEpochLo;state.epochHi=s->cod1TurretEpochHi;
 state.generation=s->cod1TurretGeneration;state.sampleTick=s->cod1TurretTick;
 VectorCopy(s->pos.trBase,state.origin);VectorCopy(s->apos.trBase,state.baseAngles);
 VectorCopy(s->cod1TurretAngles,state.controlAngles);
 state.animated=s->cod1TurretAnimated;memcpy(state.phases,s->cod1TurretPhases,8);memcpy(state.weights,s->cod1TurretWeights,8);
 trap_R_AddCod1TurretToScene(&state);return 1;
}
