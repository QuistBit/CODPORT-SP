#include "g_cod1_actor_tag_matrix584.h"
/* Include after the real turret animation host/mounted-frame implementation.
 * Map owned immutable resource cache; only current actual mounted owner trees.
 * Reset/private hooks must be included by the owning entity host. */
#include "../../qcommon/cod1_actor_tags14.h"
#include "../../qcommon/cod1_actor_look_tree14.h"
#include "../../cgame/cod1_turret_actor_pose14.h"
#include "../g_cod1_turret_actor_tags.h"
typedef struct {uint64_t epoch;cod1ActorModelComposition_t composition;c1at14Resource_t*resource;c1ad14Resource_t*full;cgta14Bank_t bank;} C1TActorTagResource;
static C1TActorTagResource cod1TurretActorTagResources[8];
static uint64_t cod1TurretActorTagBusy,cod1TurretActorTagSerial;
static int TurretActorTagPrivate(const void*p,size_t n){unsigned i,j;if(ActorOverlap(p,n,cod1TurretActorTagResources,sizeof(cod1TurretActorTagResources))||ActorOverlap(p,n,&cod1TurretActorTagBusy,sizeof(cod1TurretActorTagBusy))||ActorOverlap(p,n,&cod1TurretActorTagSerial,sizeof(cod1TurretActorTagSerial)))return 1;for(i=0;i<8;i++){c1ad14Resource_t*full=cod1TurretActorTagResources[i].full;c1at14Resource_t*r=cod1TurretActorTagResources[i].resource;if(r&&(ActorOverlap(p,n,r,sizeof(*r))||ActorOverlap(p,n,r->body,sizeof(*r->body))))return 1;if(full){if(ActorOverlap(p,n,full,sizeof(*full)))return 1;for(j=0;j<8;j++)if(full->raw[j]&&ActorOverlap(p,n,full->raw[j],sizeof(*full->raw[j])))return 1;}}return 0;}
static void TurretActorTagReset(void){unsigned i;++cod1TurretActorTagSerial;if(!cod1TurretActorTagSerial)++cod1TurretActorTagSerial;for(i=0;i<8;i++){C1AD14_Free(cod1TurretActorTagResources[i].full);memset(cod1TurretActorTagResources+i,0,sizeof(cod1TurretActorTagResources[i]));}/* A callback may reset while loading: retain its busy lease until unwind. */}
typedef struct {const GscEntityRef*borrowed;const char*name;char copiedName[128];uint64_t serial;GscEntityRef actor;C1TRecord*record,gun;cod1ActorLease_t lease;C1TARecord animation;cod1ActorRecord_t actorBefore;gentity_t entityBefore;cod1ActorModelComposition_t composition;float origin[3],angles[3],genericCurrent[98];int generic,now;int(*current)(void*);void*context;} C1TActorTagCall;
static int TurretActorTagUninterrupted(void*unused){(void)unused;return 1;}
static int TurretActorTagPrepareCurrent(C1TActorTagCall*c){GscEntityRef actor;
 if(!c||c->serial!=cod1TurretActorTagSerial||!c->borrowed||!c->name||!ActorGoalBuffers(c->borrowed,sizeof(*c->borrowed),NULL,0,NULL,0)||!ActorGoalBuffers(c->name,strlen(c->copiedName)+1,NULL,0,NULL,0)||memcmp(c->borrowed,&c->actor,sizeof(c->actor))||memcmp(c->name,c->copiedName,strlen(c->copiedName)+1)||!ActorLeaseValid(&c->lease)||!cod1EntityReady||level.spawning||level.restarted||c->now!=level.time||!TurretMountOwnerCurrent(c->record,c->now,&actor)||!ActorRefEqual(&actor,&c->actor)||memcmp(c->record,&c->gun,sizeof(c->gun)))return 0;
 return !memcmp(cod1Actors+c->lease.actorIndex,&c->actorBefore,sizeof(c->actorBefore))&&!memcmp(g_entities+c->actor.slot,&c->entityBefore,sizeof(c->entityBefore));
}
static int TurretActorTagPrepareFence(C1TActorTagCall*c){return TurretActorTagPrepareCurrent(c)&&c->current(c->context)==1&&TurretActorTagPrepareCurrent(c);}
static int TurretActorTagCurrent(C1TActorTagCall*c){cod1ActorRecord_t*a;gentity_t*e;
 if(!TurretActorTagPrepareCurrent(c)||G_Cod1ActorGenericPresent(&c->actor)!=c->generic)return 0;if(c->generic){float weights[98];if(!G_Cod1ActorGenericSample(&c->actor,c->now,weights,TurretActorTagUninterrupted,NULL)||memcmp(weights,c->genericCurrent,sizeof(weights)))return 0;}
 a=cod1Actors+c->lease.actorIndex;e=g_entities+c->actor.slot;
 return !memcmp(&cod1TurretAnimations[c->record-cod1Turrets],&c->animation,sizeof(c->animation))&&!memcmp(&a->modelComposition,&c->composition,sizeof(c->composition))&&a->dobj.present&&!a->dobj.busy&&a->dobj.summary.skippedCount==0&&a->dobj.summary.modelCount==c->composition.descriptorCount&&!memcmp(e->r.currentOrigin,c->origin,12)&&!memcmp(e->r.currentAngles,c->angles,12);
}
static int TurretActorTagFence(C1TActorTagCall*c){return TurretActorTagCurrent(c)&&c->current(c->context)==1&&TurretActorTagCurrent(c);}
static void*TurretActorTagRead(C1TActorTagCall*c,const char*path,size_t*size){fileHandle_t f=0;void*p=NULL;int n=trap_FS_FOpenFile(path,&f,FS_READ);if(!TurretActorTagFence(c))goto done;if(n<1||n>16*1024*1024||!f)goto done;p=malloc(n);if(!p)goto done;memset(p,0,n);trap_FS_Read(p,n,f);if(!TurretActorTagFence(c)){free(p);p=NULL;}else *size=(size_t)n;done:if(f)trap_FS_FCloseFile(f);if(!TurretActorTagFence(c)){free(p);p=NULL;}return p;}
static C1TActorTagResource*TurretActorTagResource(C1TActorTagCall*c){C1TActorTagResource staged,*slot=NULL;c1at14Input_t inputs[8];void*models[8]={0},*parts[8]={0};char partNames[8][128];unsigned i,g,j;int ok=0;char path[256];c1mrModelHeader_t header;
 for(i=0;i<8;i++){if(cod1TurretActorTagResources[i].epoch==c->actor.epoch&&!memcmp(&cod1TurretActorTagResources[i].composition,&c->composition,sizeof(c->composition)))return cod1TurretActorTagResources+i;if(!cod1TurretActorTagResources[i].resource&&!slot)slot=cod1TurretActorTagResources+i;}if(!slot)return NULL;
 memset(&staged,0,sizeof(staged));memset(inputs,0,sizeof(inputs));staged.epoch=c->actor.epoch;staged.composition=c->composition;
 if(!c->composition.descriptorCount||c->composition.descriptorCount>8)goto done;
 for(i=0;i<c->composition.descriptorCount;i++){const char*model=c->composition.descriptors[i].model;const char*tag=c->composition.descriptors[i].tag;
  if(!memchr(model,0,128)||strncmp(model,"xmodel/",7)||!C1DT_String(tag))goto done;models[i]=TurretActorTagRead(c,model,&inputs[i].modelBytes);if(!models[i]||!C1MR_DecodeModel(models[i],inputs[i].modelBytes,&header)||!C1MR_CopyName(header.lodName[0],partNames[i]))goto done;
  Com_sprintf(path,sizeof(path),"xmodelparts/%s",partNames[i]);parts[i]=TurretActorTagRead(c,path,&inputs[i].partsBytes);if(!parts[i])goto done;inputs[i].modelName=model+7;inputs[i].partsName=partNames[i];inputs[i].tag=tag;inputs[i].model=models[i];inputs[i].parts=parts[i];
 }
 if(!C1AD14_Create(inputs,c->composition.descriptorCount,&staged.full))goto done;staged.resource=staged.full->assembly;staged.bank.bones=staged.resource->body->source.boneCount;
 for(g=0;g<2;g++)for(j=0;j<21;j++){staged.bank.clip[g][j]=cod1TurretBodies[g][20-j];if(!C1AW14_Map(staged.resource->body,staged.bank.clip[g][j],staged.bank.map[g][j]))goto done;}
 if(!TurretActorTagFence(c)||slot->resource)goto done;*slot=staged;staged.resource=NULL;staged.full=NULL;ok=1;
 done:for(i=0;i<8;i++){free(models[i]);free(parts[i]);}C1AD14_Free(staged.full);return ok?slot:NULL;
}
int G_Cod1TurretActorTagMatrix(const GscEntityRef*actor,const char*name,int now,float out[16],int(*current)(void*),void*context){C1TActorTagCall c;C1TActorTagResource*resource;c1ab14Accumulator_t accum[128];c1aw14Pose_t body;c1at14Frame_t frame;float weights[2][25],phases[2][21],matrix[16];unsigned i,g;int bone,ok=0;
 if(!actor||!name||!out||!current||cod1TurretActorTagBusy||now!=level.time||!C1DT_String(name)||!name[0]||!ActorGoalBuffers(actor,sizeof(*actor),out,64,NULL,0)||!ActorGoalBuffers(name,strlen(name)+1,out,64,NULL,0)||TurretActorTagPrivate(out,64))return 0;
 memset(&c,0,sizeof(c));c.serial=cod1TurretActorTagSerial;c.borrowed=actor;c.actor=*actor;c.name=name;strcpy(c.copiedName,name);c.now=now;c.current=current;c.context=context;
 if(!TurretMountLease(actor,&c.lease))return 0;for(i=0;i<C1T_CAPACITY;i++)if(cod1Turrets[i].used&&ActorRefEqual(&cod1Turrets[i].actor,actor)){c.record=cod1Turrets+i;break;}if(!c.record)return 0;
 c.gun=*c.record;c.actorBefore=cod1Actors[c.lease.actorIndex];c.entityBefore=g_entities[actor->slot];c.composition=c.actorBefore.modelComposition;memcpy(c.origin,c.entityBefore.r.currentOrigin,12);memcpy(c.angles,c.entityBefore.r.currentAngles,12);
 if(!TurretActorTagPrepareFence(&c))return 0;
 c.generic=G_Cod1ActorGenericPresent(actor);if(c.generic<0)return 0;
 if(c.generic){if(!G_Cod1ActorGenericSample(actor,now,c.genericCurrent,TurretActorTagUninterrupted,NULL))return 0;}else{c1att14Tree_t prepared[2];int selected;if(TurretAnimationActorRead(c.record,now,prepared,&selected)!=1)return 0;}
 if(!TurretActorTagPrepareFence(&c))return 0;c.animation=cod1TurretAnimations[i];
 if(!TurretAnimationLive(&c.animation,c.record,now)||c.animation.actorTree[0].sampleTick!=now||c.animation.actorTree[1].sampleTick!=now||!TurretActorTagFence(&c))return 0;cod1TurretActorTagBusy=c.actor.epoch;
 resource=TurretActorTagResource(&c);if(!resource||!TurretActorTagFence(&c)||(bone=C1AT14_Find(resource->resource,c.copiedName))<0)goto done;
 for(g=0;g<2;g++){for(i=0;i<25;i++)weights[g][i]=c.animation.actorTree[g].node[i].currentWeight;for(i=0;i<21;i++)phases[g][i]=c.animation.actorTree[g].node[4+i].phase;}
 if(c.generic){const c1al14Bank_t*look=ActorGenericLookBankView(&c.actor,now);const c1ab14Clip_t*mg[2][21];c1ast14Bank_t bank;c1ast14Node_t nodes[98];c1aw14Pose_t placed;
  for(g=0;g<2;g++)for(i=0;i<21;i++)mg[g][i]=cod1TurretBodies[g][20-i];
  if(!look||!C1ALT14_Bank(resource->full,mg,look,&bank)||!C1ALT14_Nodes(c.genericCurrent,phases,nodes)||!C1AST14_Sample(&bank,nodes,98,accum,128)||!C1AD14_Build(resource->full,accum,bank.bones,&body)||!C1AW584_TagMatrix(&body,c.origin,c.angles,(unsigned)bone,matrix))goto done;
 }else if(!CGTA14_Sample(&resource->bank,weights,phases,accum,128)||!C1AW14_Build(resource->resource->body,accum,resource->bank.bones,&body)||!C1AT14_Build(resource->resource,&body,&frame)||!C1AT14_WorldMatrix(&frame,(unsigned)bone,c.origin,c.angles,matrix))goto done;
 if(!TurretActorTagFence(&c)||!ActorGoalBuffers(actor,sizeof(*actor),out,64,NULL,0)||!ActorGoalBuffers(name,strlen(c.copiedName)+1,out,64,NULL,0)||TurretActorTagPrivate(out,64))goto done;
 memcpy(out,matrix,64);ok=1;
 done:if(cod1TurretActorTagBusy==c.actor.epoch)cod1TurretActorTagBusy=0;return ok;
}
int G_Cod1TurretActorTagWorld(const GscEntityRef*actor,const char*name,int now,float origin[3],float angles[3],int(*current)(void*),void*context){float m[16],a[3];size_t n;
 if(!actor||!name||!origin||!angles||!C1DT_String(name))return 0;n=strlen(name)+1;
 if(!ActorGoalBuffers(actor,sizeof(*actor),origin,12,NULL,0)||!ActorGoalBuffers(actor,sizeof(*actor),angles,12,NULL,0)||!ActorGoalBuffers(name,n,origin,12,NULL,0)||!ActorGoalBuffers(name,n,angles,12,NULL,0)||ActorOverlap(origin,12,angles,12)||TurretActorTagPrivate(origin,12)||TurretActorTagPrivate(angles,12))return 0;
 if(!G_Cod1TurretActorTagMatrix(actor,name,now,m,current,context)||!C1AT14_MatrixAngles(m,a))return 0;memcpy(origin,m+12,12);memcpy(angles,a,12);return 1;
}
