#include "tr_cod1_turret_actor.h"
int RE_AddCod1TurretActorToScene(const cod1TurretActorRenderState_t*input){
 cod1TurretActorRenderState_t state;model_t*model;backEndData_t*frame;cod1TurretActorSubmission_t*snapshot;
 c1aa14Frame_t*capture;cv14Transform body[COD1_TURRET_ACTOR_BONES];trRefEntity_t entity;
 size_t before,bytes;unsigned char*cursor;unsigned i,j,k;float zero[3]={0,0,0},unit[3];
 if(!tr.registered||!input)return 0;memcpy(&state,input,sizeof(state));
 if((state.version!=COD1_TURRET_ACTOR_RENDER_VERSION&&state.version!=COD1_TURRET_ACTOR_RENDER_GENERIC_VERSION)||state.size!=sizeof(state)||state.ownerNum<0||state.ownerNum>=MAX_GENTITIES||
    !(state.epochLo|state.epochHi)||!state.generation||state.sampleTick<0||state.hModel<1||state.hModel>=tr.numModels||
    state.boneCount<1||state.boneCount>COD1_TURRET_ACTOR_BONES||r_numentities>=ENTITYNUM_WORLD)return 0;
 for(i=0;i<3;i++)if(!C1TP_Finite(state.origin[i],1000000)||!C1TP_Finite(state.baseAngles[i],3600))return 0;
 for(i=0;i<(unsigned)state.boneCount;i++){memcpy(body[i].q,state.q[i],16);memcpy(body[i].p,state.p[i],12);if(!C1AA14_TransformValid(&body[i]))return 0;}
 model=R_GetModelByHandle(state.hModel);if(model->type!=MOD_COD1_TURRET_ACTOR||!model->cod1TurretActor||!model->cod1TurretActor->resource)return 0;
 frame=backEndData[tr.smpFrame];if(!frame)return 0;before=frame->cod1TurretArena.used;
 capture=state.version==COD1_TURRET_ACTOR_RENDER_GENERIC_VERSION?C1AA14_CaptureFull(model->cod1TurretActor->resource,body,state.boneCount,&frame->cod1TurretArena):C1AA14_Capture(model->cod1TurretActor->resource,body,state.boneCount,&frame->cod1TurretArena);if(!capture)return 0;
 if(!capture->surfaceCount||capture->surfaceCount>COD1_TURRET_ACTOR_SURFACES)goto bad;
 bytes=C1AA14_Align(sizeof(*snapshot))+C1AA14_Align(capture->surfaceCount*sizeof(srfTriangles_t))+C1AA14_Align(capture->surfaceCount*sizeof(shader_t*));
 for(i=0;i<capture->surfaceCount;i++)if(!C1AA14_Add(&bytes,C1AA14_Align(capture->surface[i].vertices*sizeof(drawVert_t)))||!C1AA14_Add(&bytes,C1AA14_Align(capture->surface[i].indexes*sizeof(int))))goto bad;
 snapshot=CNS_Alloc(&frame->cod1TurretArena,bytes,16);if(!snapshot)goto bad;memset(snapshot,0,bytes);snapshot->state=state;snapshot->capture=capture;snapshot->surfaces=capture->surfaceCount;
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
 entity.cod1TurretActorSubmission=snapshot;frame->entities[r_numentities]=entity;r_numentities++;return 1;
 bad:frame->cod1TurretArena.used=before;return 0;
}
