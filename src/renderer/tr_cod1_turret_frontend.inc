#include "tr_cod1_turret.h"
void R_AddCod1TurretSurfaces(trRefEntity_t *ent){
 cod1TurretSubmission_t *snapshot;int i;
 if(!ent||!tr.currentModel||tr.currentModel->type!=MOD_COD1_TURRET_XMODEL||!(snapshot=ent->cod1TurretSubmission)||snapshot->state.hModel!=ent->e.hModel)return;
 if(snapshot->surfaces<1||snapshot->surfaces>XM14_SURFS||!snapshot->capture)return;
 if(R_CullLocalBox(snapshot->capture->bounds)==CULL_OUT)return;
 R_SetupEntityLighting(&tr.refdef,ent);
 for(i=0;i<snapshot->surfaces;i++)R_AddDrawSurf((surfaceType_t*)&snapshot->geometry[i],snapshot->shader[i],0,qfalse,qfalse);
}
