#include "tr_cod1_turret_actor.h"
static cod1TurretActorModel_t *cod1TurretActorResources;
/* Skinning is synchronous on the renderer front end. Only the final draw
 * vertices and the animated bounds have to survive until backend completion. */
cnsArena_t cod1ActorSkinScratch;
static int R_ReserveActorSkinScratch(size_t bytes) {
 unsigned char *data;
 if (bytes <= cod1ActorSkinScratch.capacity) return 1;
 data = realloc(cod1ActorSkinScratch.data, bytes);
 if (!data) return 0;
 cod1ActorSkinScratch.data = data;
 cod1ActorSkinScratch.capacity = bytes;
 return 1;
}
static void R_FreeActorResource(cod1TurretActorModel_t*r){if(r->sealed.storage)C1AA14_Unseal(&r->sealed);else C1AA14_Free(r->resource);free(r);}
void R_ReleaseActorCapture(cod1TurretActorModel_t*r){if(r&&r->pendingFrames){r->pendingFrames--;if(r->retired&&!r->pendingFrames)R_FreeActorResource(r);}}
void R_Cod1TurretActorReset(void){cod1TurretActorModel_t*r=cod1TurretActorResources,*next;R_ClearActorCapturePins();while(r){next=r->next;r->retired=1;if(!r->pendingFrames)R_FreeActorResource(r);r=next;}cod1TurretActorResources=NULL;
 free(cod1ActorSkinScratch.data);
 memset(&cod1ActorSkinScratch,0,sizeof(cod1ActorSkinScratch));
}
static int R_Cod1TurretActorPath(char out[128],const char*prefix,const char*name){size_t i,n;if(!name||!memchr(name,0,64)||!(n=strlen(name))||n>=64||strstr(name,"..")||strchr(name,':')||strchr(name,'\\')||strchr(name,'/'))return 0;for(i=0;i<n;i++)if((unsigned char)name[i]<32)return 0;if(strlen(prefix)+n>=128)return 0;strcpy(out,prefix);strcat(out,name);return 1;}
/* Called only through the typed registration export, not a fixed actor alias. */
qhandle_t RE_RegisterCod1TurretActor(const cod1TurretActorResourceState_t*input){
 cod1TurretActorResourceState_t def;cod1TurretActorModel_t*r,*old;model_t*model;
 c1aa14PartInput_t parts[C1AA14_PARTS];void*data[C1AA14_PARTS][3];char partNames[C1AA14_PARTS][64];unsigned i,j;int ok=0;
 if(!tr.registered||!input)return 0;memcpy(&def,input,sizeof(def));
 if(def.version!=COD1_TURRET_ACTOR_RESOURCE_VERSION||def.size!=sizeof(def)||def.partCount<1||def.partCount>C1AA14_PARTS)return 0;
 for(i=0;i<C1AA14_PARTS;i++){
  char path[128];if(i<(unsigned)def.partCount){if(!R_Cod1TurretActorPath(path,"xmodel/",def.model[i])||!memchr(def.tag[i],0,64)||(def.tag[i][0]&&strncmp(def.tag[i],"tag_",4)))return 0;}
  else if(def.model[i][0]||def.tag[i][0])return 0;
 }
 /* Canonical strings prevent unused padding from creating duplicate resources. */
 for(i=0;i<C1AA14_PARTS;i++){size_t n=strlen(def.model[i]),t=strlen(def.tag[i]);memset(def.model[i]+n,0,64-n);memset(def.tag[i]+t,0,64-t);}
 for(old=cod1TurretActorResources;old;old=old->next)if(!memcmp(&old->definition,&def,sizeof(def)))return old->handle;
 memset(data,0,sizeof(data));memset(parts,0,sizeof(parts));r=(cod1TurretActorModel_t*)calloc(1,sizeof(*r));if(!r)return 0;r->definition=def;
 for(i=0;i<(unsigned)def.partCount;i++){
  char path[128];xm14Model meta;int bytes;
  if(!R_Cod1TurretActorPath(path,"xmodel/",def.model[i]))goto done;
  bytes=ri.FS_ReadFile(path,&data[i][0]);if(bytes<1||bytes>16*1024*1024||!data[i][0]||!XM14_Model(data[i][0],bytes,&meta))goto done;
  parts[i].modelName=def.model[i];parts[i].model=data[i][0];parts[i].modelBytes=bytes;
  if(!R_Cod1TurretActorPath(path,"xmodelparts/",meta.lod[0]))goto done;strcpy(partNames[i],meta.lod[0]);parts[i].partsName=partNames[i];
  bytes=ri.FS_ReadFile(path,&data[i][1]);if(bytes<1||bytes>16*1024*1024||!data[i][1])goto done;parts[i].parts=data[i][1];parts[i].partsBytes=bytes;
  if(!R_Cod1TurretActorPath(path,"xmodelsurfs/",meta.lod[0]))goto done;
  bytes=ri.FS_ReadFile(path,&data[i][2]);if(bytes<1||bytes>16*1024*1024||!data[i][2])goto done;parts[i].mesh=data[i][2];parts[i].meshBytes=bytes;parts[i].tag=def.tag[i];
 }
 if(!C1AA14_Create(parts,def.partCount,&r->resource))goto done;
 {cv14Skeleton bind=r->resource->part[0].mesh.bind;cv14Pose empty;cv14Transform identity;cnsArena_t arena;c1aa14Frame_t*captured;
  memset(&empty,0,sizeof(empty));CV14_Identity(&identity);CV14_ApplyPose(&bind,&empty,&identity);memset(&arena,0,sizeof(arena));arena.capacity=16u*1024u*1024u;arena.data=malloc(arena.capacity);if(!arena.data)goto done;
  captured=C1AA14_Capture(r->resource,bind.world,bind.n,&arena);if(captured)memcpy(r->bounds,captured->bounds,sizeof(r->bounds));free(arena.data);
  /* Bind and animated captures share the same topology and vertex counts.
   * Allow padding for a different base address in the reusable arena. */
  if(!captured || !R_ReserveActorSkinScratch(arena.used + 15))goto done;
 }
 /* Sealing is optional: allocation/protection failure preserves the strict
  * existing resource. Successful copies contain no pointer to the old owner. */
 if(C1AA14_Seal(r->resource,&r->sealed)){C1AA14_Free(r->resource);r->resource=(c1aa14Resource_t*)r->sealed.resource;C1AA575_Build(r->resource,&r->poseBounds);}
 for(i=0;i<(unsigned)def.partCount;i++)for(j=0;j<(unsigned)r->resource->part[i].mesh.numSurfaces;j++){
  char path[128];if(!R_Cod1TurretActorPath(path,"skins/",r->resource->part[i].model.skin[0][j]))goto done;
  r->shader[i][j]=R_FindShader(path,LIGHTMAP_NONE,qtrue);if(!r->shader[i][j]||r->shader[i][j]->defaultShader)goto done;
 }
 model=R_AllocModel();if(!model)goto done;
 Com_sprintf(model->name,sizeof(model->name),"@cod1_turret_actor_%d",model->index);model->type=MOD_COD1_TURRET_ACTOR;model->cod1TurretActor=r;model->numLods=1;model->dataSize=sizeof(*r);r->handle=model->index;r->next=cod1TurretActorResources;cod1TurretActorResources=r;ok=1;
 done:/* FS_ReadFile uses temporary Hunk storage; unwind our reads in LIFO
       * order on success and on any partially loaded prefix. */
 for(i=C1AA14_PARTS;i>0;--i)for(j=3;j>0;--j)if(data[i-1][j-1])ri.FS_FreeFile(data[i-1][j-1]);
 if(!ok){if(r->sealed.storage)C1AA14_Unseal(&r->sealed);else C1AA14_Free(r->resource);free(r);return 0;}return r->handle;
}
