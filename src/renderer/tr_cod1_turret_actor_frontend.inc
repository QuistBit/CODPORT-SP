#include "tr_cod1_turret_actor.h"
void R_AddCod1TurretActorSurfaces(trRefEntity_t*ent){
 cod1TurretActorSubmission_t*snapshot;unsigned i;
 if(!ent||!tr.currentModel||tr.currentModel->type!=MOD_COD1_TURRET_ACTOR||!(snapshot=ent->cod1TurretActorSubmission)||snapshot->state.hModel!=ent->e.hModel||!snapshot->capture||!snapshot->surfaces||snapshot->surfaces>COD1_TURRET_ACTOR_SURFACES)return;
 if(R_CullLocalBox(snapshot->capture->bounds)==CULL_OUT)return;R_SetupEntityLighting(&tr.refdef,ent);
 for(i=0;i<snapshot->surfaces;i++)R_AddDrawSurf((surfaceType_t*)(snapshot->geometry+i),snapshot->shader[i],0,qfalse,qfalse);
}
