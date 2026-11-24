#include "tr_cod1_turret_actor.h"
int R_CaptureCod1TurretActor(const cod1TurretActorRenderState_t*input,trRefEntity_t*out){
 cod1TurretActorRenderState_t state;model_t*model;backEndData_t*frame;cod1TurretActorSubmission_t*snapshot;
 c1aa14Frame_t*capture;cv14Transform body[COD1_TURRET_ACTOR_BONES];trRefEntity_t entity;
 size_t before,bytes;unsigned char*cursor;unsigned i,j,k;float zero[3]={0,0,0},unit[3];
 if(!tr.registered||!input)return 0;memcpy(&state,input,sizeof(state));
 if((state.version!=COD1_TURRET_ACTOR_RENDER_VERSION&&state.version!=COD1_TURRET_ACTOR_RENDER_GENERIC_VERSION)||state.size!=sizeof(state)||state.ownerNum<0||state.ownerNum>=MAX_GENTITIES||
    !(state.epochLo|state.epochHi)||!state.generation||state.sampleTick<0||state.hModel<1||state.hModel>=tr.numModels||
    state.boneCount<1||state.boneCount>COD1_TURRET_ACTOR_BONES)return 0;
 for(i=0;i<3;i++)if(!C1TP_Finite(state.origin[i],1000000)||!C1TP_Finite(state.baseAngles[i],3600))return 0;
 for(i=0;i<(unsigned)state.boneCount;i++){memcpy(body[i].q,state.q[i],16);memcpy(body[i].p,state.p[i],12);if(!C1AA14_TransformValid(&body[i]))return 0;}
 model=R_GetModelByHandle(state.hModel);if(model->type!=MOD_COD1_TURRET_ACTOR||!model->cod1TurretActor||!model->cod1TurretActor->resource)return 0;
 frame=backEndData[tr.smpFrame];if(!frame)return 0;before=frame->cod1TurretArena.used;
 CNS_Reset(&cod1ActorSkinScratch);
 capture=C1AA14_CaptureRegistered(&model->cod1TurretActor->sealed,model->cod1TurretActor->resource,body,state.boneCount,&cod1ActorSkinScratch,state.version==COD1_TURRET_ACTOR_RENDER_GENERIC_VERSION);if(!capture)return 0;
 if(!capture->surfaceCount||capture->surfaceCount>COD1_TURRET_ACTOR_SURFACES)goto bad;
 bytes=C1AA14_Align(sizeof(*snapshot))+C1AA14_Align(capture->surfaceCount*sizeof(srfTriangles_t))+C1AA14_Align(capture->surfaceCount*sizeof(shader_t*));
 for(i=0;i<capture->surfaceCount;i++)if(!C1AA14_Add(&bytes,C1AA14_Align(capture->surface[i].vertices*sizeof(drawVert_t)))||!C1AA14_Add(&bytes,C1AA14_Align(capture->surface[i].indexes*sizeof(int))))goto bad;
 snapshot=CNS_Alloc(&frame->cod1TurretArena,bytes,16);if(!snapshot)goto bad;memset(snapshot,0,bytes);snapshot->state=state;memcpy(snapshot->bounds,capture->bounds,sizeof(snapshot->bounds));snapshot->surfaces=capture->surfaceCount;
 cursor=(unsigned char*)snapshot+C1AA14_Align(sizeof(*snapshot));snapshot->geometry=(srfTriangles_t*)cursor;cursor+=C1AA14_Align(capture->surfaceCount*sizeof(srfTriangles_t));snapshot->shader=(shader_t**)cursor;cursor+=C1AA14_Align(capture->surfaceCount*sizeof(shader_t*));
 for(i=0;i<capture->surfaceCount;i++){
  const c1aa14Surface_t*src=&capture->surface[i];srfTriangles_t*dst=snapshot->geometry+i;
  if(src->part>=C1AA14_PARTS||src->localSurface>=XM14_SURFS||!(snapshot->shader[i]=model->cod1TurretActor->shader[src->part][src->localSurface]))goto bad;
  dst->surfaceType=SF_TRIANGLES;dst->numVerts=src->vertices;dst->numIndexes=src->indexes;dst->verts=(drawVert_t*)cursor;cursor+=C1AA14_Align(src->vertices*sizeof(drawVert_t));dst->indexes=(int*)cursor;cursor+=C1AA14_Align(src->indexes*sizeof(int));
  VectorCopy(capture->bounds[0],dst->bounds[0]);VectorCopy(capture->bounds[1],dst->bounds[1]);
  for(j=0;j<(unsigned)dst->numVerts;j++){drawVert_t*v=dst->verts+j;VectorCopy(src->vertex[j].xyz,v->xyz);VectorCopy(src->vertex[j].normal,v->normal);for(k=0;k<2;k++)v->st[k]=src->vertex[j].uv[k];memset(v->color,255,4);}
  for(j=0;j<(unsigned)dst->numIndexes;j++)dst->indexes[j]=src->index[j];
 }
 memset(&entity,0,sizeof(entity));entity.e.reType=RT_MODEL;entity.e.hModel=state.hModel;VectorCopy(state.origin,entity.e.origin);VectorCopy(state.origin,entity.e.oldorigin);VectorCopy(state.origin,entity.e.lightingOrigin);memset(entity.e.shaderRGBA,255,4);
 for(i=0;i<3;i++){memset(unit,0,12);unit[i]=1;if(!C1NSO_Transform(zero,state.baseAngles,unit,entity.e.axis[i]))goto bad;}
 entity.cod1TurretActorSubmission=snapshot;*out=entity;return 1;
 bad:frame->cod1TurretArena.used=before;return 0;
}

/* Scene-owned immutable resource pins last until capture or scene reset.
 * Geometry remains detached before it reaches the asynchronous backend. */
static cod1TurretActorSubmission_t *cod1ActorPending[2];
static void R_ClearPendingActorCaptures(int frame){
 cod1TurretActorSubmission_t*s=cod1ActorPending[frame],*next;
 while(s){next=s->nextPending;if(s->pendingModel)R_ReleaseActorCapture(s->pendingModel);s->pendingModel=NULL;s=next;}
 cod1ActorPending[frame]=NULL;
}
void R_ClearActorCapturePins(void){R_ClearPendingActorCaptures(0);R_ClearPendingActorCaptures(1);}
int RE_AddCod1TurretActorToScene(const cod1TurretActorRenderState_t*input){
 static cvar_t *lazy;
 cod1TurretActorRenderState_t state;model_t*model;cod1TurretActorModel_t*resource;
 trRefEntity_t entity;cod1TurretActorSubmission_t*snapshot;backEndData_t*frame;
 cv14Transform body[COD1_TURRET_ACTOR_BONES];float bounds[2][3],zero[3]={0},unit[3];unsigned i;
 if(!tr.registered||!input)return 0;state=*input;
 if((state.version!=COD1_TURRET_ACTOR_RENDER_VERSION&&state.version!=COD1_TURRET_ACTOR_RENDER_GENERIC_VERSION)||state.size!=sizeof(state)||state.ownerNum<0||state.ownerNum>=MAX_GENTITIES||
    !(state.epochLo|state.epochHi)||!state.generation||state.sampleTick<0||state.hModel<1||state.hModel>=tr.numModels||
    state.boneCount<1||state.boneCount>COD1_TURRET_ACTOR_BONES||r_numentities>=ENTITYNUM_WORLD)return 0;
 for(i=0;i<3;i++)if(!C1TP_Finite(state.origin[i],1000000)||!C1TP_Finite(state.baseAngles[i],3600))return 0;
 for(i=0;i<(unsigned)state.boneCount;i++){memcpy(body[i].q,state.q[i],16);memcpy(body[i].p,state.p[i],12);if(!C1AA14_TransformValid(body+i))return 0;}
 model=R_GetModelByHandle(state.hModel);if(model->type!=MOD_COD1_TURRET_ACTOR||!(resource=model->cod1TurretActor)||!resource->resource)return 0;
 frame=backEndData[tr.smpFrame];if(!frame)return 0;
 if(!lazy)lazy=ri.Cvar_Get("r_cod1ActorLazyCapture","1",0);
 if(!lazy->integer||resource->retired||!resource->sealed.storage||!C1AA575_PoseBounds(&resource->poseBounds,resource->resource,body,state.boneCount,state.version==COD1_TURRET_ACTOR_RENDER_GENERIC_VERSION,bounds)){
  if(!R_CaptureCod1TurretActor(&state,&entity))return 0;
 }else{
  memset(&entity,0,sizeof(entity));entity.e.reType=RT_MODEL;entity.e.hModel=state.hModel;VectorCopy(state.origin,entity.e.origin);VectorCopy(state.origin,entity.e.oldorigin);VectorCopy(state.origin,entity.e.lightingOrigin);memset(entity.e.shaderRGBA,255,4);
  for(i=0;i<3;i++){memset(unit,0,12);unit[i]=1;if(!C1NSO_Transform(zero,state.baseAngles,unit,entity.e.axis[i]))return 0;}
  snapshot=CNS_Alloc(&frame->cod1TurretArena,sizeof(*snapshot),16);if(!snapshot)return 0;memset(snapshot,0,sizeof(*snapshot));snapshot->state=state;memcpy(snapshot->bounds,bounds,sizeof(bounds));
  snapshot->pendingModel=resource;resource->pendingFrames++;snapshot->nextPending=cod1ActorPending[tr.smpFrame];cod1ActorPending[tr.smpFrame]=snapshot;entity.cod1TurretActorSubmission=snapshot;
 }
 frame->entities[r_numentities++]=entity;return 1;
}
