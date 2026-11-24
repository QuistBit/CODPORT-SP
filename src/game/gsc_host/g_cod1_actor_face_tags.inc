#include "g_cod1_actor_tag_matrix584.h"
#include "../../qcommon/cod1_actor_source_pose_profile14.h"
/* Include after face resource/current-tree owner. Resource bank is borrowed
 * synchronously from ActorFaceSamplerBank and must NEVER be freed here. */
#include "../g_cod1_actor_face_tags.h"
#include "../g_cod1_actor_scripted_frame.h"
#include "../../qcommon/cod1_actor_tags14.h"

/* Binding a clip's bone names to one model depends only on registered
 * resources. The animation weights, phases and actor transform are per query. */
typedef struct {
 c1ast14Bank_t bank;
 uint64_t lookRevision,faceRevision,scriptedRevision;
 int valid;
} C1FaceTagBinding;
typedef struct C1FActorTagResource {
 struct C1FActorTagResource *next;
 uint64_t epoch;
 cod1ActorModelComposition_t composition;
 c1ad14Resource_t *full;
 C1FaceTagBinding binding[2];
 uint64_t sourceRevision;unsigned char sourceMapped[C1ASP14_RESOURCES],sourceMap[C1ASP14_RESOURCES][128];
} C1FActorTagResource;
typedef struct {
 const GscEntityRef *borrowed;
 const char *name;
 char copiedName[128];
 uint64_t serial,faceRevision,scriptedRevision,sourceRevision,jumperRevision;int scripted,sourcePose,jumper;
 GscEntityRef actor;
 cod1ActorLease_t lease;
 cod1ActorRecord_t actorBefore;
 gentity_t entityBefore;
 C1GARecord genericBefore;
 c1atl14Node_t *nodesBefore;
 cod1ActorModelComposition_t composition;
 float origin[3],angles[3],weights[98],phases[2][21];
 int now;
 int (*current)(void*);
 void *context;
 /* All staged allocations remain private across FS callbacks. */
 void *models[8],*parts[8],*readBuffer;
 size_t modelBytes[8],partBytes[8],readBytes;
 c1ad14Resource_t *stageFull;
 C1FActorTagResource *stageResource;
} C1FActorTagCall;
static uint64_t ActorSourceTagRevision(uint64_t);
static int ActorSourceTagSample(C1FActorTagCall*,C1FActorTagResource*,c1ab14Accumulator_t*,unsigned*);
static C1FActorTagResource *cod1FaceTagResources;
static uint64_t cod1FaceTagBusy,cod1FaceTagSerial;
static int cod1FaceTagExhausted;
static C1FActorTagCall *cod1FaceTagOperation;

static int ActorFaceTagResourcePrivate(const void *p,size_t n,const c1ad14Resource_t *r){
 unsigned j;
 if(!r)return 0;
 if(ActorOverlap(p,n,r,sizeof(*r))||ActorOverlap(p,n,r->assembly,sizeof(*r->assembly)))return 1;
 for(j=0;j<8;j++)if(r->raw[j]&&ActorOverlap(p,n,r->raw[j],sizeof(*r->raw[j])))return 1;
 return 0;
}
static int ActorFaceTagPrivate(const void *p,size_t n){
 C1FActorTagCall *c=cod1FaceTagOperation;C1FActorTagResource*r;unsigned i;
 if(ActorOverlap(p,n,&cod1FaceTagResources,sizeof(cod1FaceTagResources))||
 ActorOverlap(p,n,&cod1FaceTagBusy,sizeof(cod1FaceTagBusy))||
 ActorOverlap(p,n,&cod1FaceTagSerial,sizeof(cod1FaceTagSerial))||
 ActorOverlap(p,n,&cod1FaceTagExhausted,sizeof(cod1FaceTagExhausted))||
 ActorOverlap(p,n,&cod1FaceTagOperation,sizeof(cod1FaceTagOperation)))return 1;
 for(r=cod1FaceTagResources;r;r=r->next)if(ActorOverlap(p,n,r,sizeof(*r))||ActorFaceTagResourcePrivate(p,n,r->full))return 1;
 if(!c)return 0;
 if(ActorOverlap(p,n,c,sizeof(*c))||ActorOverlap(p,n,c->stageResource,sizeof(*c->stageResource))||ActorOverlap(p,n,c->nodesBefore,C1GA_COUNT*sizeof(*c->nodesBefore))||
 ActorOverlap(p,n,c->readBuffer,c->readBytes)||ActorFaceTagResourcePrivate(p,n,c->stageFull))return 1;
 for(i=0;i<8;i++)if(ActorOverlap(p,n,c->models[i],c->modelBytes[i])||ActorOverlap(p,n,c->parts[i],c->partBytes[i]))return 1;
 return 0;
}
static void ActorFaceTagReset(void){
 C1FActorTagResource*r,*next;
 if(cod1FaceTagSerial==UINT64_MAX)cod1FaceTagExhausted=1;
 else cod1FaceTagSerial++;
 for(r=cod1FaceTagResources;r;r=next){next=r->next;C1AD14_Free(r->full);free(r);}cod1FaceTagResources=NULL;
 /* Keep an active caller's busy/private staging until that caller unwinds. */
}
static int ActorFaceTagUninterrupted(void *ctx){(void)ctx;return 1;}
/* GetEye can run inside initialize before SetPoseMovement installs run.
 * Admit the actual current jump/fade tree through its existing closure;
 * the mover's later render selector is deliberately not the query gate. */
static int ActorJumperSourceTagSelected(const C1GARecord*r){
 if(!r||!r->nodes||!TrainingJumperAllocation(&r->lease.entity)||
    memcmp(&r->lease,&trainingJumperOwner,sizeof(r->lease))||!ActorFaceFrameClosure(r))return -1;
 return ActorSourcePoseSelection(r);
}
static int ActorFaceTagJumperCurrent(const C1FActorTagCall*c){
 return c&&c->jumperRevision&&TrainingJumperAllocation(&c->actor)&&
  !memcmp(&c->lease,&trainingJumperOwner,sizeof(c->lease))&&
  ActorJumperReady(c->actor.epoch)&&c->jumperRevision==cod1JumperResources.revision;
}
static int ActorFaceTagCurrent(const C1FActorTagCall *c){
 const cod1ActorRecord_t *a;const C1GARecord *g;
 if(!c||cod1FaceTagOperation!=c||cod1FaceTagExhausted||c->serial!=cod1FaceTagSerial||
 (c->sourcePose&&(!c->sourceRevision||c->sourceRevision!=ActorSourceTagRevision(c->actor.epoch)))||
 (c->jumper&&!ActorFaceTagJumperCurrent(c))||
 c->faceRevision!=cod1FaceResources.revision||!ActorFaceReady(c->actor.epoch)||c->scriptedRevision!=cod1ScriptedResources.revision||c->scripted!=ActorScriptedClockSelected(c->lease.actorIndex)||(c->scripted&&!ActorScriptedReady(c->actor.epoch))||
 !ActorGoalBuffers(c->borrowed,sizeof(*c->borrowed),NULL,0,NULL,0)||
 !ActorGoalBuffers(c->name,strlen(c->copiedName)+1,NULL,0,NULL,0)||
 memcmp(c->borrowed,&c->actor,sizeof(c->actor))||memcmp(c->name,c->copiedName,strlen(c->copiedName)+1)||
 !ActorLeaseValid(&c->lease)||!cod1EntityReady||level.spawning||level.restarted||c->now!=level.time)return 0;
 a=cod1Actors+c->lease.actorIndex;g=cod1GenericActors+c->lease.actorIndex;
 return !a->turretMount.mounted&&a->dobj.present&&!a->dobj.busy&&!a->dobj.summary.skippedCount&&
 a->dobj.summary.modelCount==c->composition.descriptorCount&&
 R242ExactATRBytesEqual(a,&c->actorBefore,sizeof(*a))&&R242ExactATRBytesEqual(g_entities+c->actor.slot,&c->entityBefore,sizeof(c->entityBefore))&&
 !memcmp(g,&c->genericBefore,sizeof(*g))&&c->nodesBefore&&g->nodes&&
 R242ExactATRBytesEqual(g->nodes,c->nodesBefore,C1GA_COUNT*sizeof(*g->nodes));
}
static int ActorFaceTagFence(C1FActorTagCall *c){
 return ActorFaceTagCurrent(c)&&c->current(c->context)==1&&ActorFaceTagCurrent(c);
}
static void *ActorFaceTagRead(C1FActorTagCall *c,const char *path,size_t *size){
 fileHandle_t f=0;void *p=NULL;int n=trap_FS_FOpenFile(path,&f,FS_READ);
 if(!ActorFaceTagFence(c)||n<1||n>16*1024*1024||!f)goto done;
 p=calloc(1,(size_t)n);if(!p)goto done;c->readBuffer=p;c->readBytes=(size_t)n;
 trap_FS_Read(p,n,f);
 if(!ActorFaceTagFence(c)){free(p);p=NULL;}else *size=(size_t)n;
done:
 if(f)trap_FS_FCloseFile(f);
 if(!ActorFaceTagFence(c)){free(p);p=NULL;}
 c->readBuffer=NULL;c->readBytes=0;return p;
}
static C1FActorTagResource *ActorFaceTagResource(C1FActorTagCall *c){
 C1FActorTagResource *slot=NULL; c1at14Input_t input[8];
 char partNames[8][128],path[256];c1mrModelHeader_t header;unsigned i;
 for(slot=cod1FaceTagResources;slot;slot=slot->next){
  if(slot->epoch==c->actor.epoch&&slot->full&&
  !memcmp(&slot->composition,&c->composition,sizeof(c->composition)))return slot;
 }
 if(!c->composition.descriptorCount||c->composition.descriptorCount>8||c->stageResource)return NULL;
 /* Private until all resource reads and owner fences have completed. The
  * per-composition descriptor limit remains eight; cache entries are not
  * limited by that unrelated number. All entries retire with the map. */
 c->stageResource=calloc(1,sizeof(*c->stageResource));if(!c->stageResource)return NULL;
 slot=c->stageResource;
 memset(input,0,sizeof(input));
 for(i=0;i<c->composition.descriptorCount;i++){
  const char *model=c->composition.descriptors[i].model,*tag=c->composition.descriptors[i].tag;
  if(!memchr(model,0,128)||strncmp(model,"xmodel/",7)||!C1DT_String(tag))return NULL;
  c->models[i]=ActorFaceTagRead(c,model,c->modelBytes+i);
  if(!c->models[i]||!C1MR_DecodeModel(c->models[i],c->modelBytes[i],&header)||!C1MR_CopyName(header.lodName[0],partNames[i]))return NULL;
  Com_sprintf(path,sizeof(path),"xmodelparts/%s",partNames[i]);c->parts[i]=ActorFaceTagRead(c,path,c->partBytes+i);
  if(!c->parts[i])return NULL;
  input[i].model=c->models[i];input[i].modelBytes=c->modelBytes[i];input[i].parts=c->parts[i];input[i].partsBytes=c->partBytes[i];
  input[i].modelName=model+7;input[i].partsName=partNames[i];input[i].tag=tag;
 }
 if(!C1AD14_Create(input,c->composition.descriptorCount,&c->stageFull)||!ActorFaceTagFence(c)||slot->full)return NULL;
 slot->epoch=c->actor.epoch;slot->composition=c->composition;slot->full=c->stageFull;c->stageFull=NULL;
 slot->next=cod1FaceTagResources;cod1FaceTagResources=slot;c->stageResource=NULL;return slot;
}
static const c1ast14Bank_t *ActorFaceTagBank(C1FActorTagResource *model,int scripted){
 C1FaceTagBinding *binding=&model->binding[scripted!=0];
 uint64_t epoch=cod1EntityEpoch;
 if(!epoch||model->epoch!=epoch||!model->full)return NULL;
 if(scripted?!ActorScriptedReady(epoch):!ActorFaceReady(epoch))return NULL;
 if(!binding->valid||binding->lookRevision!=cod1GenericLookRevision||
    binding->faceRevision!=cod1FaceResources.revision||
    (scripted&&binding->scriptedRevision!=cod1ScriptedResources.revision)){
  c1ast14Bank_t built;
  if(!(scripted?ActorScriptedSamplerBank(model->full,&built):ActorFaceSamplerBank(model->full,&built)))return NULL;
  binding->bank=built;
  binding->lookRevision=cod1GenericLookRevision;
  binding->faceRevision=cod1FaceResources.revision;
  binding->scriptedRevision=cod1ScriptedResources.revision;
  binding->valid=1;
 }
 return &binding->bank;
}
int G_Cod1ActorFaceTagMatrix(const GscEntityRef *actor,const char *name,int now,float out[16],int(*current)(void*),void *context){
 C1FActorTagCall *c=NULL;C1FActorTagResource *r;const c1ast14Bank_t *bank;c1ast14Node_t nodes[C1ASB14_NODES];
 c1ab14Accumulator_t accum[128];c1aw14Pose_t pose;float matrix[16],weights[98],phases[2][21],facial[2][25];int bone,ok=0;unsigned i,bones=0;
 if(!actor||!name||!out||!current||cod1FaceTagBusy||cod1FaceTagOperation||cod1FaceTagExhausted||now!=level.time||
 !C1DT_String(name)||!name[0]||!ActorGoalBuffers(actor,sizeof(*actor),out,64,NULL,0)||
 !ActorGoalBuffers(name,strlen(name)+1,out,64,NULL,0)||ActorFaceTagPrivate(actor,sizeof(*actor))||
 ActorFaceTagPrivate(name,strlen(name)+1)||ActorFaceTagPrivate(out,64))return 0;
 c=calloc(1,sizeof(*c));if(!c)return 0;c->serial=cod1FaceTagSerial;c->faceRevision=cod1FaceResources.revision;c->scriptedRevision=cod1ScriptedResources.revision;
 c->borrowed=actor;c->actor=*actor;c->name=name;strcpy(c->copiedName,name);c->now=now;c->current=current;c->context=context;
 if(!ActorGoalLease(actor,&c->lease)||cod1Actors[c->lease.actorIndex].turretMount.mounted)goto done;
 c->scripted=ActorScriptedClockSelected(c->lease.actorIndex);
 c->actorBefore=cod1Actors[c->lease.actorIndex];c->entityBefore=g_entities[actor->slot];c->genericBefore=cod1GenericActors[c->lease.actorIndex];
 if(!c->genericBefore.used||!c->genericBefore.nodes)goto done;
 c->jumper=c->scripted&&TrainingJumperAllocation(actor);
 if(c->jumper)c->jumperRevision=cod1JumperResources.revision;
 c->sourcePose=c->scripted?(c->jumper?ActorJumperSourceTagSelected(&c->genericBefore):ActorSourcePoseSelection(&c->genericBefore)):0;
 if(c->sourcePose<0)goto done;
 if(c->sourcePose){c->sourceRevision=ActorSourceTagRevision(actor->epoch);if(!c->sourceRevision||c->genericBefore.tick!=now)goto done;}
 c->nodesBefore=malloc(C1GA_COUNT*sizeof(*c->nodesBefore));if(!c->nodesBefore)goto done;
 memcpy(c->nodesBefore,c->genericBefore.nodes,C1GA_COUNT*sizeof(*c->nodesBefore));
 c->composition=c->actorBefore.modelComposition;memcpy(c->origin,c->entityBefore.r.currentOrigin,12);memcpy(c->angles,c->entityBefore.r.currentAngles,12);
 cod1FaceTagOperation=c;cod1FaceTagBusy=c->actor.epoch;
 if(!c->sourcePose){
 if(!(c->scripted?G_Cod1ActorScriptedSampleFull(actor,now,weights,phases,facial,ActorFaceTagUninterrupted,NULL):G_Cod1ActorFaceUnmountedSample(actor,now,weights,phases,ActorFaceTagUninterrupted,NULL)))goto done;
 memcpy(c->weights,weights,sizeof(weights));memcpy(c->phases,phases,sizeof(phases));
 }
 if(!ActorFaceTagFence(c))goto done;
 r=ActorFaceTagResource(c);
 if(!r||!ActorFaceTagFence(c)||(bone=C1AT14_Find(r->full->assembly,c->copiedName))<0)goto done;
 if(c->sourcePose){if(!ActorSourceTagSample(c,r,accum,&bones))goto done;}
 else{
  if(!(bank=ActorFaceTagBank(r,c->scripted))||!(c->scripted?C1ASB14_NodesFull(c->weights,c->phases,facial,nodes):C1AFU14_Nodes(c->weights,c->phases,nodes))||
     !C1AST14_Sample(bank,nodes,c->scripted?C1ASB14_NODES:C1AFU14_NODES,accum,128))goto done;
  bones=bank->bones;
 }
 if(!C1AD14_Build(r->full,accum,bones,&pose)||!C1AW584_TagMatrix(&pose,c->origin,c->angles,(unsigned)bone,matrix))goto done;
 
 if(!ActorFaceTagFence(c)||!ActorGoalBuffers(actor,sizeof(*actor),out,64,NULL,0)||
 !ActorGoalBuffers(name,strlen(c->copiedName)+1,out,64,NULL,0)||ActorFaceTagPrivate(out,64))goto done;
 memcpy(out,matrix,64);ok=1;
 if(c->sourcePose&&c->jumper&&g_debugMove.integer>1)G_Printf("[386 jumper eye] owner=%u/%u tick=%d tag=%s phase=%.6f jumpWeight=%.6f runWeight=%.6f xyz=%.6f %.6f %.6f\n",c->actor.slot,c->actor.generation,now,c->copiedName,c->nodesBefore[892].phase,c->nodesBefore[892].currentWeight,c->nodesBefore[164].currentWeight,matrix[12],matrix[13],matrix[14]);
 if(c->sourcePose&&g_debugMove.integer>1)G_Printf("[379 source tag] owner=%u tick=%d tag=%s source_clock=1 xyz=%.6f %.6f %.6f\n",c->actor.slot,now,c->copiedName,matrix[12],matrix[13],matrix[14]);
done:
 if(c){for(i=0;i<8;i++){free(c->models[i]);free(c->parts[i]);}C1AD14_Free(c->stageFull);free(c->stageResource);free(c->nodesBefore);
 if(cod1FaceTagOperation==c){cod1FaceTagOperation=NULL;if(cod1FaceTagBusy==c->actor.epoch)cod1FaceTagBusy=0;}free(c);}
 return ok;
}
int G_Cod1ActorFaceTagWorld(const GscEntityRef *actor,const char *name,int now,float origin[3],float angles[3],int(*current)(void*),void *context){
 float matrix[16],a[3];size_t n;
 if(!actor||!name||!origin||!angles||!C1DT_String(name))return 0;n=strlen(name)+1;
 if(!ActorGoalBuffers(actor,sizeof(*actor),origin,12,NULL,0)||!ActorGoalBuffers(actor,sizeof(*actor),angles,12,NULL,0)||
 !ActorGoalBuffers(name,n,origin,12,NULL,0)||!ActorGoalBuffers(name,n,angles,12,NULL,0)||ActorOverlap(origin,12,angles,12)||
 ActorFaceTagPrivate(origin,12)||ActorFaceTagPrivate(angles,12))return 0;
 if(!G_Cod1ActorFaceTagMatrix(actor,name,now,matrix,current,context)||!C1AT14_MatrixAngles(matrix,a)||
 !ActorGoalBuffers(actor,sizeof(*actor),origin,12,NULL,0)||!ActorGoalBuffers(actor,sizeof(*actor),angles,12,NULL,0)||
 !ActorGoalBuffers(name,n,origin,12,NULL,0)||!ActorGoalBuffers(name,n,angles,12,NULL,0)||
 ActorFaceTagPrivate(origin,12)||ActorFaceTagPrivate(angles,12))return 0;
 memcpy(origin,matrix+12,12);memcpy(angles,a,12);return 1;
}
