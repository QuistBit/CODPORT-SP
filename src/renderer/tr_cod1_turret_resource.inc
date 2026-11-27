#include "tr_cod1_turret.h"
static cod1TurretModel_t *cod1TurretResources;
void R_Cod1TurretReset(void){
 cod1TurretModel_t *r=cod1TurretResources,*next;
 while(r){next=r->next;C1TM_Free(r->resource);C1AW14_Free(r->animatedSkeleton);C1AB14_Free(r->animatedClip[0]);C1AB14_Free(r->animatedClip[1]);free(r);r=next;}
 cod1TurretResources=NULL;
}
static int R_LoadCod1Turret(model_t *mod){
 static const char *paths[5]={"xmodel/MG42_bipod","xmodelparts/MG42_bipod1","xmodelsurfs/MG42_bipod1","xanim/standMG42gun_aim_foward","xanim/standMG42gun_fire_foward"};
 void *data[5]={NULL,NULL,NULL,NULL,NULL};int bytes[5]={0,0,0,0,0},i,ok=0;
 cod1TurretModel_t *r=NULL;c1tpStaticPose_t pose;float angles[3]={0,0,0};
 if(!mod)return 0;
 for(i=0;i<5;i++){bytes[i]=ri.FS_ReadFile(paths[i],NULL);if(bytes[i]<1||bytes[i]>16*1024*1024)goto done;if(ri.FS_ReadFile(paths[i],&data[i])!=bytes[i]||!data[i])goto done;}
 r=(cod1TurretModel_t*)calloc(1,sizeof(*r));if(!r)goto done;
 if(!C1TM_Create(data[0],bytes[0],data[1],bytes[1],data[2],bytes[2],&r->resource)||!C1TP_StaticResource(&r->resource->bones,angles,&pose))goto done;
 if(r->resource->mesh.numSurfaces!=5||!C1AW14_Decode(data[1],bytes[1],&r->animatedSkeleton)||
    !C1AB14_Decode(data[3],bytes[3],&r->animatedClip[0])||!C1AB14_Decode(data[4],bytes[4],&r->animatedClip[1]))goto done;
 for(i=0;i<2;i++){unsigned char map[128];if(r->animatedClip[i]->bones!=9||r->animatedClip[i]->fps!=30||r->animatedClip[i]->flags!=(unsigned)(i?3:2)||!C1AW14_Map(r->animatedSkeleton,r->animatedClip[i],map))goto done;}
 for(i=0;i<r->resource->mesh.numSurfaces;i++){
  char path[MAX_QPATH];const char *name=r->resource->model.skin[0][i];size_t n=strlen(name),j;
  if(!n||n+6>=MAX_QPATH||strstr(name,"..")||strchr(name,':')||strchr(name,'\\')||name[0]=='/')goto done;
  for(j=0;j<n;j++)if((unsigned char)name[j]<32)goto done;
  memcpy(path,"skins/",6);memcpy(path+6,name,n+1);r->shader[i]=R_FindShader(path,LIGHTMAP_NONE,qtrue);
  if(!r->shader[i]||r->shader[i]->defaultShader)goto done;
 }
 /* Preserve validated serialized model bounds; current culling uses capture. */
 for(i=0;i<3;i++){
  float lo=r->resource->bones.model.serializedBounds.mins[i],hi=r->resource->bones.model.serializedBounds.maxs[i];
  if(!C1TP_Finite(lo,1000000)||!C1TP_Finite(hi,1000000)||lo>hi)goto done;
  r->bounds[0][i]=lo;r->bounds[1][i]=hi;
 }
 mod->cod1Turret=r;mod->type=MOD_COD1_TURRET_XMODEL;mod->numLods=1;
 mod->dataSize=(int)sizeof(*r)+(int)sizeof(*r->resource)+bytes[2];
 r->next=cod1TurretResources;cod1TurretResources=r;r=NULL;ok=1;
 done:/* Reverse acquisition order for the temporary Hunk stack. */
 for(i=5;i>0;--i)if(data[i-1])ri.FS_FreeFile(data[i-1]);
 if(r){C1TM_Free(r->resource);C1AW14_Free(r->animatedSkeleton);C1AB14_Free(r->animatedClip[0]);C1AB14_Free(r->animatedClip[1]);free(r);}return ok;
}

static int R_RegisterCod1TurretResource(model_t *mod,const char *name){
 if(!mod||!name||strcmp(name,COD1_TURRET_MODEL_ALIAS))return 0;return R_LoadCod1Turret(mod);
}
