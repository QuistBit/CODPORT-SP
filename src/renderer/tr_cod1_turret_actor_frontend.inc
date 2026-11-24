#include "tr_cod1_turret_actor.h"
void R_AddCod1TurretActorSurfaces(trRefEntity_t*ent){
 cod1TurretActorSubmission_t*snapshot;unsigned i;
 if(!ent||!tr.currentModel||tr.currentModel->type!=MOD_COD1_TURRET_ACTOR||!(snapshot=ent->cod1TurretActorSubmission)||snapshot->state.hModel!=ent->e.hModel)return;
 if(R_CullLocalBox(snapshot->bounds)==CULL_OUT)return;
 if(snapshot->pendingModel){
  trRefEntity_t captured;cod1TurretActorModel_t*resource=snapshot->pendingModel;
  if(tr.currentModel->cod1TurretActor!=resource||resource->retired||!R_CaptureCod1TurretActor(&snapshot->state,&captured))return;
  /* Portal/multiple views can hold another copy of the entity. Publish
   * detached geometry into their shared scene snapshot, preserving the pin
   * chain until arena reset. Every view then observes the same capture. */
  {cod1TurretActorSubmission_t*next=snapshot->nextPending;*snapshot=*captured.cod1TurretActorSubmission;snapshot->nextPending=next;}
  R_ReleaseActorCapture(resource);
  if(R_CullLocalBox(snapshot->bounds)==CULL_OUT)return;
 }
 if(!snapshot->surfaces||snapshot->surfaces>COD1_TURRET_ACTOR_SURFACES)return;
 R_SetupEntityLighting(&tr.refdef,ent);
 for(i=0;i<snapshot->surfaces;i++)R_AddDrawSurf((surfaceType_t*)(snapshot->geometry+i),snapshot->shader[i],0,qfalse,qfalse);
}
