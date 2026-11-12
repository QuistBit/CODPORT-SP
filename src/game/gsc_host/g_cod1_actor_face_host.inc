/* Explicit map-owned face resource and current-tree publication dependency.
 * No new GSC command admission or automatic actor/frame owner is enabled here.
 * Five face resources are owned; base Look/shuffle resources remain owned by
 * the existing generic bank. Any aggregate view is borrowed and NEVER freed. */
#include "../g_cod1_actor_face.h"
#include "../../qcommon/cod1_actor_face_unmounted14.h"
#include "../../qcommon/cod1_actor_notetrack_metadata14.h"
#include "../bg_cod1_face_unmounted_actor_wire.h"
typedef struct {uint64_t epoch,revision,busy;const void*stage;size_t stageBytes;c1ar14Root_t*root[5];c1ab14Clip_t*body[5];c1anm14Metadata_t metadata[5];} C1FaceResources;
static C1FaceResources cod1FaceResources;
static int ActorFaceDataPrivate(const void*p,size_t n,const C1FaceResources*r){unsigned i;for(i=0;i<5;i++)if(TurretBodyPrivate(p,n,r->body[i])||ActorOverlap(p,n,r->root[i],r->root[i]?sizeof(*r->root[i]):0))return 1;return 0;}
static int ActorFacePrivate(const void*p,size_t n){C1FaceResources*r=&cod1FaceResources;
 if(ActorScriptedPrivate(p,n)||ActorFaceFramePrivate(p,n)||ActorFaceTagPrivate(p,n))return 1;
 if(ActorOverlap(p,n,r,sizeof(*r))||ActorOverlap(p,n,r->stage,r->stageBytes))return 1;
 if(!r->epoch&&!r->stage)return 0;
 if(ActorFaceDataPrivate(p,n,r))return 1;
 return r->stage&&r->stageBytes==sizeof(C1FaceResources)&&ActorFaceDataPrivate(p,n,(const C1FaceResources*)r->stage);
}
static void ActorFaceReset(void){unsigned i;C1FaceResources*r=&cod1FaceResources;
 ActorScriptedReset();ActorFaceTagReset();ActorFaceFrameReset();
 if(r->revision!=UINT64_MAX)r->revision++;
 for(i=0;i<5;i++){C1AR14_Free(r->root[i]);C1AB14_Free(r->body[i]);r->root[i]=NULL;r->body[i]=NULL;}memset(r->metadata,0,sizeof(r->metadata));r->epoch=0;
 /* Retain a loading callback's busy/stage lease until its owner unwinds. */
}
static int ActorFaceReady(uint64_t epoch){unsigned i;C1FaceResources*r=&cod1FaceResources;
 if(!epoch||r->epoch!=epoch||r->busy||cod1GenericLookEpoch!=epoch||!C1AUT14_Ready(&cod1GenericUnmountedBank))return 0;
 for(i=0;i<5;i++)if(!C1AFU14_ValidFace(i,r->root[i],r->body[i])||!C1ANM14_Valid(r->metadata+i))return 0;
 return 1;
}
/* Borrow existing map-owned banks for synchronous sampling only. This stack
 * aggregate owns nothing and MUST NOT be passed to C1AFU14_Free. Callers fence
 * the actual actor/DObj, epoch and source tree before and after sampling. */
static int ActorFaceSamplerBank(const c1ad14Resource_t*resource,c1ast14Bank_t*out){
 c1afu14Bank_t borrowed;unsigned i;
 if(!ActorFaceReady(cod1EntityEpoch))return 0;memset(&borrowed,0,sizeof(borrowed));borrowed.base=cod1GenericUnmountedBank;
 for(i=0;i<5;i++){borrowed.faceRoot[i]=cod1FaceResources.root[i];borrowed.faceBody[i]=cod1FaceResources.body[i];}
 return C1AFU14_Bank(resource,&borrowed,out);
}
int G_Cod1ActorFaceResourcesLoad(void){C1FaceResources stage,*r=&cod1FaceResources;uint64_t epoch=cod1EntityEpoch,revision=r->revision;unsigned i;int ok=0;char path[128];
 if(ActorFaceReady(epoch))return 1;
 if(!epoch||!level.spawning||!cod1EntityActive||r->busy||revision==UINT64_MAX||r->epoch||cod1GenericOperation.busy||cod1GenericLookEpoch!=epoch||!C1AUT14_Ready(&cod1GenericUnmountedBank))return 0;
 memset(&stage,0,sizeof(stage));r->busy=epoch;r->stage=&stage;r->stageBytes=sizeof(stage);
 for(i=0;i<5;i++){
  snprintf(path,sizeof(path),"xanim/%s",C1AFU14_Name(i+42));
  if(!TurretBankRead(path,stage.root+i,stage.body+i,epoch)||r->revision!=revision||cod1EntityEpoch!=epoch||!level.spawning||!cod1EntityActive||!C1AFU14_ValidFace(i,stage.root[i],stage.body[i])||!C1ANM14_Build(stage.root[i],stage.metadata+i))goto done;
 }
 if(r->revision!=revision||cod1EntityEpoch!=epoch||!level.spawning||!cod1EntityActive||r->busy!=epoch||r->epoch||cod1GenericLookEpoch!=epoch||!C1AUT14_Ready(&cod1GenericUnmountedBank))goto done;
 for(i=0;i<5;i++){r->root[i]=stage.root[i];r->body[i]=stage.body[i];r->metadata[i]=stage.metadata[i];stage.root[i]=NULL;stage.body[i]=NULL;}r->epoch=epoch;ok=1;
 done:for(i=0;i<5;i++){C1AR14_Free(stage.root[i]);C1AB14_Free(stage.body[i]);}r->stage=NULL;r->stageBytes=0;if(r->busy==epoch)r->busy=0;return ok;
}

int G_Cod1ActorFaceUnmountedSample(const GscEntityRef *input,int now,float output[98],float phases[2][21],int (*current)(void*),void *ctx){
 GscEntityRef owner;cod1ActorLease_t lease,after;C1GARecord saved;
 cod1ActorRecord_t *actorCopy=NULL;gentity_t *entityCopy=NULL;c1atl14Node_t *copy=NULL;
 float values[98]={0},times[2][21]={{0}};c1ast14Node_t checked[C1AFU14_NODES];uint64_t epoch;unsigned i,j;int ok=0;
 if(!input||!output||!phases||!current||now!=level.time||cod1GenericOperation.busy||
    !ActorGoalBuffers(input,sizeof(*input),output,sizeof(values),NULL,0)||!ActorGoalBuffers(input,sizeof(*input),phases,sizeof(times),NULL,0)||
    ActorOverlap(output,sizeof(values),phases,sizeof(times))||ActorGenericPrivate(input,sizeof(*input))||ActorGenericPrivate(output,sizeof(values))||ActorGenericPrivate(phases,sizeof(times)))return 0;
 owner=*input;epoch=cod1EntityEpoch;cod1GenericOperation.busy=1;cod1GenericOperation.cancelled=0;
 if(!ActorGoalLease(&owner,&lease)||G_Cod1ActorGenericPresent(&owner)!=1||cod1Actors[lease.actorIndex].turretMount.mounted||!ActorFaceReady(epoch))goto done;
 saved=cod1GenericActors[lease.actorIndex];if(saved.tick!=now)goto done;
 actorCopy=malloc(sizeof(*actorCopy));entityCopy=malloc(sizeof(*entityCopy));copy=malloc(C1GA_COUNT*sizeof(*copy));if(!actorCopy||!entityCopy||!copy)goto done;
 memcpy(actorCopy,cod1Actors+lease.actorIndex,sizeof(*actorCopy));memcpy(entityCopy,g_entities+owner.slot,sizeof(*entityCopy));memcpy(copy,saved.nodes,C1GA_COUNT*sizeof(*copy));
 cod1GenericOperation.stage=copy;cod1GenericOperation.bytes=C1GA_COUNT*sizeof(*copy);
 if(current(ctx)!=1||now!=level.time||epoch!=cod1EntityEpoch||cod1GenericOperation.cancelled||memcmp(input,&owner,sizeof(owner))||
    memcmp(&saved,cod1GenericActors+lease.actorIndex,sizeof(saved))||memcmp(actorCopy,cod1Actors+lease.actorIndex,sizeof(*actorCopy))||memcmp(entityCopy,g_entities+owner.slot,sizeof(*entityCopy)))goto done;
 for(i=1;i<C1GA_COUNT;i++)if(!cod1GenericDefinitions[i].children&&(copy[i].currentWeight||copy[i].goalWeight)){
  int covered=0;for(j=0;j<C1AFU14_NODES;j++)if(c1afu14Definitions[j].ordinal==i&&c1afu14Definitions[j].leaf>=0)covered=1;if(!covered)goto done;
 }
 for(i=0;i<C1AFU14_NODES;i++){
  const c1afu14Definition_t *d=c1afu14Definitions+i;
  if(d->ordinal>=C1GA_COUNT||(d->leaf>=0&&strcmp(C1AFU14_Name(d->leaf),cod1GenericNames[d->ordinal].name))||
     (i&&cod1GenericDefinitions[d->ordinal].parent!=c1afu14Definitions[d->parent].ordinal))goto done;
  values[i]=copy[d->ordinal].currentWeight;if(d->phase>=0)times[0][d->phase]=copy[d->ordinal].phase;
 }
 if(!C1AFU14_Nodes(values,times,checked))goto done;
 if(current(ctx)!=1||now!=level.time||epoch!=cod1EntityEpoch||cod1GenericOperation.cancelled||memcmp(input,&owner,sizeof(owner))||
    memcmp(&saved,cod1GenericActors+lease.actorIndex,sizeof(saved))||!ActorGoalLease(&owner,&after)||memcmp(&lease,&after,sizeof(lease))||
    memcmp(actorCopy,cod1Actors+lease.actorIndex,sizeof(*actorCopy))||memcmp(entityCopy,g_entities+owner.slot,sizeof(*entityCopy))||
    memcmp(copy,saved.nodes,C1GA_COUNT*sizeof(*copy))||!ActorFaceReady(epoch))goto done;
 memcpy(output,values,sizeof(values));memcpy(phases,times,sizeof(times));ok=1;
done:free(copy);free(actorCopy);free(entityCopy);cod1GenericOperation.stage=NULL;cod1GenericOperation.bytes=0;cod1GenericOperation.notifyStage=NULL;cod1GenericOperation.notifyBytes=0;cod1GenericOperation.busy=0;return ok;
}

int G_Cod1ActorFaceUnmountedPublish(const GscEntityRef*input,int now,int(*current)(void*),void*ctx){
 C1UAPCall*c=NULL;C1TActorPublished saved,next;entityState_t wire;gentity_t*e;char text[1024],actual[1024];unsigned i;int slot=-1,ok=0;
 if(!input||!current||now!=level.time||!ActorFaceReady(cod1EntityEpoch)||!cod1EntityEpoch||cod1UnmountedPublishBusy||cod1UnmountedPublishSerial==UINT64_MAX||cod1TurretActorConfigBusy||
    !ActorGoalBuffers(input,sizeof(*input),NULL,0,NULL,0)||ActorUnmountedPublisherPrivate(input,sizeof(*input)))return 0;
 c=calloc(1,sizeof(*c));if(!c)return 0;c->borrowed=input;c->actor=*input;c->epoch=cod1EntityEpoch;c->serial=++cod1UnmountedPublishSerial;c->now=now;c->current=current;c->ctx=ctx;cod1UnmountedPublishBusy=c->epoch;
 if(!ActorGoalLease(&c->actor,&c->lease)||G_Cod1ActorGenericPresent(&c->actor)!=1)goto done;
 c->record=cod1Actors[c->lease.actorIndex];c->body=g_entities[c->actor.slot];c->generic=cod1GenericActors[c->lease.actorIndex];
 if(!ActorUnmountedPublishFence(c)||!TurretActorCompositionText(&c->record.modelComposition,text))goto done;
 for(i=0;i<MAX_COD1_TURRET_ACTORS;i++)if(!strcmp(cod1TurretActorConfigs[i],text)){slot=(int)i;break;}
 if(slot<0)for(i=0;i<MAX_COD1_TURRET_ACTORS;i++)if(!cod1TurretActorConfigs[i][0]){slot=(int)i;break;}
 if(slot<0)goto done;trap_GetConfigstring(CS_COD1_TURRET_ACTORS+slot,actual,sizeof(actual));if(!ActorUnmountedPublishFence(c))goto done;
 if(cod1TurretActorConfigs[slot][0]){if(strcmp(actual,text))goto done;}
 else{
  if(actual[0]&&strcmp(actual,text))goto done;cod1TurretActorConfigBusy=c->epoch;
  if(!actual[0])trap_SetConfigstring(CS_COD1_TURRET_ACTORS+slot,text);if(!ActorUnmountedPublishFence(c))goto done;
  trap_GetConfigstring(CS_COD1_TURRET_ACTORS+slot,actual,sizeof(actual));if(!ActorUnmountedPublishFence(c)||strcmp(actual,text))goto done;
  strcpy(cod1TurretActorConfigs[slot],text);cod1TurretActorConfigBusy=0;
 }
 saved=next=cod1UnmountedPublished[c->lease.actorIndex];if(next.used&&!ActorRefEqual(&next.owner,&c->actor))goto done;
 if(!next.used){next.used=1;next.owner=c->actor;next.previous=c->body.s;next.oldFlags=c->body.flags;next.oldSvFlags=c->body.r.svFlags;}
 wire=c->body.s;wire.eType=ET_GENERAL;wire.cod1TurretVersion=0;wire.cod1TurretEpochLo=(int)(uint32_t)c->epoch;wire.cod1TurretEpochHi=(int)(uint32_t)(c->epoch>>32);
 wire.cod1TurretActorVersion=COD1_FACE_UNMOUNTED_ACTOR_WIRE_VERSION;wire.cod1TurretActorGeneration=c->actor.generation;wire.cod1TurretActorGun=-1;wire.cod1TurretActorGunGeneration=0;
 wire.cod1TurretActorTick=now;wire.cod1TurretActorConfig=CS_COD1_TURRET_ACTORS+slot;wire.cod1TurretActorFlags=1;wire.pos.trType=wire.apos.trType=TR_STATIONARY;wire.eFlags&=~EF_NODRAW;
 VectorCopy(c->body.r.currentOrigin,wire.pos.trBase);VectorCopy(c->body.r.currentAngles,wire.apos.trBase);memset(wire.cod1TurretActorCurrent,0,sizeof(wire.cod1TurretActorCurrent));
 if(!G_Cod1ActorFaceUnmountedSample(&c->actor,now,wire.cod1TurretActorGenericCurrent,wire.cod1TurretActorPhases,ActorUnmountedPublishFence,c)||
    !C1FUAW_Valid(&wire,now)||!ActorUnmountedPublishCurrent(c)||memcmp(cod1UnmountedPublished+c->lease.actorIndex,&saved,sizeof(saved)))goto done;
 e=g_entities+c->actor.slot;e->s=wire;e->flags&=~FL_NODRAW;e->r.svFlags&=~SVF_NOCLIENT;cod1UnmountedPublished[c->lease.actorIndex]=next;
 /* Origin, bounds and contents stay at the already linked actual pose.
  * Snapshot visibility reads live svFlags: no relink or callback is needed. */
 c->body=*e;ok=ActorUnmountedPublishCurrent(c)&&!memcmp(cod1UnmountedPublished+c->lease.actorIndex,&next,sizeof(next));
done:if(c){if(cod1TurretActorConfigBusy==c->epoch)cod1TurretActorConfigBusy=0;if(cod1UnmountedPublishBusy==c->epoch)cod1UnmountedPublishBusy=0;}free(c);return ok;
}
