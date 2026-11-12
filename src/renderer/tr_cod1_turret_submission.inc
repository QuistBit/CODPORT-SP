#include "tr_cod1_turret.h"
/* Read-only current supplied pose query; owner admission belongs to CGAME.
 * Shares the exact registered skeleton/clips and sampler with submission. */
int RE_Cod1TurretTagFlash(const cod1TurretRenderState_t*input,float out[16]){
 cod1TurretRenderState_t s;model_t*m;cod1TurretModel_t*r;C1TRModel tags;c1aw14Pose_t pose,world;int i;
 if(!tr.registered||!input||!out||C1TM_Overlap(input,sizeof(*input),out,64))return 0;s=*input;
 if(s.version!=COD1_TURRET_RENDER_VERSION||s.size!=sizeof(s)||s.ownerNum<0||s.ownerNum>=MAX_GENTITIES||!(s.epochLo|s.epochHi)||!s.generation||s.sampleTick<0||s.hModel<1||s.hModel>=tr.numModels||s.animated!=1)return 0;
 for(i=0;i<3;i++)if(!C1TP_Finite(s.origin[i],1000000)||!C1TP_Finite(s.baseAngles[i],3600)||!C1TP_Finite(s.controlAngles[i],3600))return 0;
 for(i=0;i<2;i++)if(!(s.phases[i]>=0&&s.phases[i]<=1)||!(s.weights[i]>=0&&s.weights[i]<=1))return 0;
 if(!(s.weights[0]>0||s.weights[1]>0))return 0;m=R_GetModelByHandle(s.hModel);if(m->type!=MOD_COD1_TURRET_XMODEL||!(r=m->cod1Turret)||!r->resource||C1TM_Overlap(out,64,r,sizeof(*r))||C1TM_Overlap(out,64,r->resource,sizeof(*r->resource)))return 0;
 if(!C1TR_Model(&r->resource->bones,&tags)||tags.flash<0||tags.flash>=13||!C1TAP14_SampleGun(r->animatedSkeleton,(const c1ab14Clip_t*const*)r->animatedClip,s.phases,s.weights,s.controlAngles,&pose)||!C1AW14_Place(&pose,s.origin,s.baseAngles,&world))return 0;
 memcpy(out,world.matrix[tags.flash],64);return 1;
}
/* Typed submission is the only admission path. A generation/tick is carried,
 * never manufactured from hModel or global renderer time. CGAME validates its
 * actual snapshot owner; renderer validates shape and captures it immutably. */
int RE_AddCod1TurretToScene(const cod1TurretRenderState_t *input){
 cod1TurretRenderState_t state;model_t *model;backEndData_t *frame;
 cod1TurretSubmission_t *snapshot;c1tmFrame_t *capture;trRefEntity_t entity;
 size_t before,bytes;unsigned char *cursor;float zero[3]={0,0,0},unit[3];int i,j,k;
 if(!tr.registered||!input)return 0;memcpy(&state,input,sizeof(state));
 if(state.version!=COD1_TURRET_RENDER_VERSION||state.size!=sizeof(state)||state.ownerNum<0||state.ownerNum>=MAX_GENTITIES||
    !(state.epochLo|state.epochHi)||!state.generation||state.sampleTick<0||state.hModel<1||state.hModel>=tr.numModels||r_numentities>=ENTITYNUM_WORLD)return 0;
 for(i=0;i<3;i++)if(!C1TP_Finite(state.origin[i],1000000)||!C1TP_Finite(state.baseAngles[i],3600)||!C1TP_Finite(state.controlAngles[i],3600))return 0;
 model=R_GetModelByHandle(state.hModel);if(model->type!=MOD_COD1_TURRET_XMODEL||!model->cod1Turret||!model->cod1Turret->resource)return 0;
 frame=backEndData[tr.smpFrame];if(!frame)return 0;before=frame->cod1TurretArena.used;
 if(state.animated!=0&&state.animated!=1)return 0;
 for(i=0;i<2;i++)if(!(state.phases[i]>=0&&state.phases[i]<=1)||!(state.weights[i]>=0&&state.weights[i]<=1)||(!state.animated&&(state.phases[i]!=0||state.weights[i]!=0)))return 0;
 if(state.animated){
  c1aw14Pose_t current;c1tpStaticPose_t posed;C1TRModel tags;cod1TurretModel_t *r=model->cod1Turret;
  if(!C1TR_Model(&r->resource->bones,&tags)||!C1TAP14_SampleGun(r->animatedSkeleton,(const c1ab14Clip_t*const*)r->animatedClip,state.phases,state.weights,state.controlAngles,&current))return 0;
  memset(&posed,0,sizeof(posed));posed.bones=current.bones;posed.flash=tags.flash;
  for(i=0;i<13;i++){memcpy(posed.q[i],current.q[i],16);memcpy(posed.p[i],current.matrix[i]+12,12);}
  capture=C1TAM_CapturePose(r->resource,state.controlAngles,&posed,&frame->cod1TurretArena);
 }else capture=C1TM_Capture(model->cod1Turret->resource,state.controlAngles,&frame->cod1TurretArena);
 if(!capture)return 0;
 bytes=C1TM_Align(sizeof(*snapshot));
 for(i=0;i<(int)capture->surfaces;i++)bytes+=C1TM_Align(capture->surface[i].vertices*sizeof(drawVert_t))+C1TM_Align(capture->surface[i].indexes*sizeof(int));
 snapshot=CNS_Alloc(&frame->cod1TurretArena,bytes,16);if(!snapshot)goto bad;
 memset(snapshot,0,bytes);snapshot->state=state;snapshot->capture=capture;snapshot->surfaces=capture->surfaces;
 if(!C1TP_WorldFlash(&capture->pose,state.origin,state.baseAngles,&snapshot->flash))goto bad;
 cursor=(unsigned char*)snapshot+C1TM_Align(sizeof(*snapshot));
 for(i=0;i<snapshot->surfaces;i++){
  const c1tmSurface_t *src=&capture->surface[i];srfTriangles_t *dst=&snapshot->geometry[i];
  snapshot->shader[i]=model->cod1Turret->shader[i];if(!snapshot->shader[i])goto bad;
  dst->surfaceType=SF_TRIANGLES;dst->numVerts=src->vertices;dst->numIndexes=src->indexes;
  dst->verts=(drawVert_t*)cursor;cursor+=C1TM_Align(src->vertices*sizeof(drawVert_t));
  dst->indexes=(int*)cursor;cursor+=C1TM_Align(src->indexes*sizeof(int));
  VectorCopy(capture->bounds[0],dst->bounds[0]);VectorCopy(capture->bounds[1],dst->bounds[1]);
  for(j=0;j<dst->numVerts;j++){
   drawVert_t *v=&dst->verts[j];VectorCopy(src->vertex[j].xyz,v->xyz);VectorCopy(src->vertex[j].normal,v->normal);
   for(k=0;k<2;k++)v->st[k]=src->vertex[j].uv[k];memset(v->color,255,4);
  }
  for(j=0;j<dst->numIndexes;j++)dst->indexes[j]=src->index[j];
 }
 memset(&entity,0,sizeof(entity));entity.e.reType=RT_MODEL;entity.e.hModel=state.hModel;
 VectorCopy(state.origin,entity.e.origin);VectorCopy(state.origin,entity.e.oldorigin);VectorCopy(state.origin,entity.e.lightingOrigin);memset(entity.e.shaderRGBA,255,4);
 for(i=0;i<3;i++){memset(unit,0,12);unit[i]=1;if(!C1NSO_Transform(zero,state.baseAngles,unit,entity.e.axis[i]))goto bad;}
 entity.cod1TurretSubmission=snapshot;frame->entities[r_numentities]=entity;r_numentities++;return 1;
 bad:frame->cod1TurretArena.used=before;return 0;
}
