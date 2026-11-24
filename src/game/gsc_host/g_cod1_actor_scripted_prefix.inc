/* Explicit selected fresh-AI native prefix. No activation/profile defaults.
 * Include after scripted_resources/scripted_frame inside turret_host; this is
 * already after ActorInheritedCertificate definition in entity_host.
 * Require proposed member a->scriptedAux and final parent request typedef.
 * Only public invoke entry is new; every provider called below actually exists.
 * It commits queue ONLY. VM executes original init after host callback closes. */
#include "../g_cod1_actor_bootstrap.h"
typedef struct {
 GscActorScriptedPrefixRequest request;cod1ActorLease_t lease;
 cod1ActorRecord_t actor;gentity_t entity;C1GARecord generic;
 const c1atl14Node_t*nodeOwner;const C1GANotifyState*notifyOwner;const C1GANameEntry*nameOwner;
 uint64_t epoch,receipt,lookRevision,faceRevision,scriptedRevision,jumperRevision;int dynamic;
 const void *previousRecord;
 uint64_t previousRevision;
 unsigned previousClip;
} FreshPrefixCall;
static int MoodyPreviousRecord(const FreshPrefixCall *,const void **,uint64_t *,unsigned *);
static int FreshRecordCurrent(const FreshPrefixCall *c){
 const void *record=NULL;uint64_t revision=0;unsigned clip=0;
 return MoodyPreviousRecord(c,&record,&revision,&clip) && record==c->previousRecord &&
        revision==c->previousRevision && clip==c->previousClip;
}
static struct {int busy,cancelled;FreshPrefixCall*stage;} freshOperation;
static const GscActorScriptedPrefixHost freshPrefixHost;
static int FreshStoragePrivate(const void*p,size_t n){
 FreshPrefixCall*c=freshOperation.stage;
 return ActorOverlap(p,n,&freshPrefixHost,sizeof(freshPrefixHost))||ActorOverlap(p,n,&freshOperation,sizeof(freshOperation))||
  (c&&(ActorOverlap(p,n,c,sizeof(*c))||
   ActorOverlap(p,n,c->generic.nodes,c->generic.nodes?C1GA_COUNT*sizeof(*c->generic.nodes):0)||
   ActorGenericNamesPrivate(p,n,&c->generic)||ActorOverlap(p,n,c->generic.notify,c->generic.notify?sizeof(*c->generic.notify):0)));
}
static void FreshOperationCancelOwner(uint32_t i,const GscEntityRef*o){
 FreshPrefixCall*c=freshOperation.stage;
 if(freshOperation.busy&&c&&c->lease.actorIndex==i&&ActorRefEqual(&c->lease.entity,o))freshOperation.cancelled=1;
}
static void FreshOperationReset(void){if(freshOperation.busy)freshOperation.cancelled=1;}
static int FreshExternal(const void*p,size_t n){
 return !FreshStoragePrivate(p,n)&&!ActorGenericPrivate(p,n)&&
  !TurretAnimationPrivate(p,n)&&!TurretBankPrivate(p,n)&&!ActorGoalPrivate(p,n);
}
static int FreshBootstrapTarget(const cod1ActorLease_t*l){
 cod1ActorBootstrapSnapshot_t*b;uint32_t i;int count=0,ok=0;
 b=malloc(sizeof(*b));if(!b)return 0;
 /* Actual public read, NOT G_Cod1ActorBootstrapHost getter: that getter refuses
  * runAttempted after startup. No invented bootstrap-current getter used. */
 if(!G_Cod1ActorBootstrapRead(b,NULL,0)||b->epoch!=l->entity.epoch||!b->receipt||
    !b->enabled||!b->declarationsReady||!b->sealed||!b->selected||!b->runAttempted||
    !b->runComplete||b->closed||b->invalid||b->placedRows>COD1_ACTOR_CAPACITY)goto done;
 for(i=0;i<b->placedRows;i++)if(ActorRefEqual(&b->rows[i].entity,&l->entity)){
  const cod1ActorBootstrapRow_t*r=b->rows+i;++count;
  if(memcmp(&r->lease,l,sizeof(*l))||r->retired||!r->startupConsumed||
     r->stage!=COD1_ACTOR_ENTRY_ENTERED||r->entryStatus!=GSC_ENTITY_ENTRY_RETURNED)goto done;
 }
 ok=count==1;
 done:free(b);return ok;
}
static int FreshBindings(const GscActorScriptedPrefixRequest*q){
 const c1ar14Root_t*r;const c1ab14Clip_t*b;unsigned i;
 if(!q->runtime_identity||!q->catalog_identity||q->now_ms<0||q->now_ms!=level.time||
    q->argc<4||q->argc>6||q->root_present!=(q->argc==6)||
    !memchr(q->notify,0,sizeof(q->notify))||
    !ActorGenericBinding(&q->animation,q->runtime_identity)||
    !q->animation.ordinal||(q->animation.flags&GSC_ANIMATION_NODE_BRANCH)||
    (q->root_present&&!ActorGenericBinding(&q->root,q->runtime_identity)))return 0;
 for(i=0;i<3;i++)if(!isfinite(ActorFloatFromBits(q->origin_bits[i]))||
    !isfinite(ActorFloatFromBits(q->angles_bits[i])))return 0;
 /* Explicit selected resource domain, NOT arbitrary catalog-loaded readiness.
  * Exact real Foley scripted resources currently comprise two leaves. No
  * positive goal/presence sample is required for the newly supplied clip. */
 r=ActorScriptedRoot(q->animation.ordinal);b=ActorScriptedBody(q->animation.ordinal);
 return ActorScriptedReady(cod1EntityEpoch)&&r&&b&&r->magic==C1AR14_MAGIC&&
  b->magic==C1AB14_MAGIC&&r->frames==b->frames&&r->fps==b->fps&&
  r->intervals==b->intervals&&r->flags==b->flags;
}
/* Distinct dynamic domain: a real healthy factory completion and its
 * independently loaded exact jumper resource. Placed/Foley admission stays
 * unchanged; the normal-record/MAIN consumer remains separately held. */
static int FreshDynamicBindings(const GscActorScriptedPrefixRequest*q){unsigned i;
 if(!q||!q->runtime_identity||!q->catalog_identity||q->now_ms<0||q->now_ms!=level.time||
    q->argc<4||q->argc>6||q->root_present!=(q->argc==6)||
    !memchr(q->notify,0,sizeof(q->notify))||!ActorJumperBinding(q)||
    (q->root_present&&!ActorGenericBinding(&q->root,q->runtime_identity)))return 0;
 for(i=0;i<3;i++)if(!isfinite(ActorFloatFromBits(q->origin_bits[i]))||
  !isfinite(ActorFloatFromBits(q->angles_bits[i])))return 0;
 return 1;
}
static int FreshTarget(const FreshPrefixCall*c){return c->dynamic?
 ActorFactoryCompletedTarget(&c->lease,c->request.runtime_identity,c->request.catalog_identity):
 FreshBootstrapTarget(&c->lease);}
static int FreshRequestCurrent(const FreshPrefixCall*c){return c->dynamic?
 c->jumperRevision==cod1JumperResources.revision&&FreshDynamicBindings(&c->request):FreshBindings(&c->request);}
static int FreshOwnerCurrent(const FreshPrefixCall*c){
 cod1ActorLease_t after;const cod1ActorScriptedAux_t*a;
 if(!c||freshOperation.cancelled||c->epoch!=cod1EntityEpoch||!cod1EntityReady||
    level.spawning||level.restarted||level.time!=c->request.now_ms||
    cod1ActorCriticalBusy||cod1GenericOperation.busy||cod1TurretAnimationOperation.busy||
    !ActorGoalLease(&c->request.entity,&after)||memcmp(&after,&c->lease,sizeof(after))||
    !ActorInheritedCertificate(&after.entity,after.serial)||
    g_entities[after.entity.slot].health<=0||cod1Actors[after.actorIndex].turretMount.mounted||
    !ActorCriticalReady(&after)||G_Cod1ActorGenericPresent(&after.entity)!=1||
    c->receipt!=cod1GenericReceipt||c->receipt!=cod1TurretAnimationReceipt||
    c->lookRevision!=cod1GenericLookRevision||c->faceRevision!=cod1FaceResources.revision||
    c->scriptedRevision!=cod1ScriptedResources.revision||
    !TurretAnimationMapCurrent(c->receipt)||!FreshRequestCurrent(c)||!FreshTarget(c))return 0;
 a=&cod1Actors[after.actorIndex].scriptedAux;
 return a->actorSerial==after.serial&&a->revision&&a->revision!=UINT64_MAX&&
  a->record==(c->previousRecord?FRESH_AUX_OWNED:FRESH_AUX_EMPTY)&&
  a->link==FRESH_LINK_UNATTACHED&&a->prefix==FRESH_PREFIX_UNUSED&&FreshRecordCurrent(c)&&
  !memcmp(&c->actor,cod1Actors+after.actorIndex,sizeof(c->actor))&&
  !memcmp(&c->entity,g_entities+after.entity.slot,sizeof(c->entity))&&
  c->generic.used&&c->generic.runtime==c->request.runtime_identity&&
  c->generic.catalog==c->request.catalog_identity&&
  !memcmp(&c->generic.lease,&after,sizeof(after))&&
  /* Owned clone has different nodes/notify pointer values, compare metadata
   * explicitly, then full real arrays: no shallow struct memcmp shortcut. */
  !memcmp(&cod1GenericActors[after.actorIndex].lease,&c->lease,sizeof(c->lease))&&
  cod1GenericActors[after.actorIndex].nodes==c->nodeOwner&&
  cod1GenericActors[after.actorIndex].notify==c->notifyOwner&&cod1GenericActors[after.actorIndex].names==c->nameOwner&&
  cod1GenericActors[after.actorIndex].used==c->generic.used&&
  cod1GenericActors[after.actorIndex].tick==c->generic.tick&&
  cod1GenericActors[after.actorIndex].runtime==c->generic.runtime&&
  cod1GenericActors[after.actorIndex].catalog==c->generic.catalog&&
  cod1GenericActors[after.actorIndex].revision==c->generic.revision&&
  !memcmp(cod1GenericActors[after.actorIndex].lookCells,c->generic.lookCells,sizeof(c->generic.lookCells))&&
  cod1GenericActors[after.actorIndex].nodes&&cod1GenericActors[after.actorIndex].notify&&
  !memcmp(c->generic.nodes,cod1GenericActors[after.actorIndex].nodes,C1GA_COUNT*sizeof(*c->generic.nodes))&&
  !memcmp(c->generic.notify,cod1GenericActors[after.actorIndex].notify,sizeof(*c->generic.notify))&&ActorGenericNamesEqual(&c->generic,cod1GenericActors+after.actorIndex);
}
static int TrainingRigStart(int kind,const GscActorScriptedPrefixRequest *input,
 GscEntityExecutionCertificate *out,int *admitted,int(*current)(void*),void *context){
 GscActorScriptedPrefixRequest q=*input;TrainingRigTree *tree=trainingRigTrees+kind;
 TrainingRigPlayback *p=trainingRigPlayback+kind;
 c1asr14Record_t record={0};c1ar14Sample_t sample;float org[3],ang[3];gentity_t *e=g_entities+q.entity.slot;
 if(!current(context)||memcmp(&q,input,sizeof(q))||TrainingRigKind(&q.entity)!=kind||
    !ActorRefEqual(&q.entity,&tree->owner)||q.catalog_identity!=tree->catalog||
    q.runtime_identity!=tree->runtime||q.animation.runtime_identity!=tree->runtime||
    strcmp(q.animation.tree,trainingRigNames[kind])||strcmp(q.animation.name,trainingRigClips[kind])||
    q.animation.flags||q.animation.ordinal!=1||q.root_present||q.argc!=4||
    !q.notify[0]||!memchr(q.notify,0,sizeof(q.notify))||strlen(q.notify)>=sizeof(p->event)||
    p->epoch!=q.entity.epoch||!p->root||p->running||tree->modelIndex!=e->s.modelindex)return 0;
 memcpy(org,q.origin_bits,12);memcpy(ang,q.angles_bits,12);
 if(!C1ASR14_NormalBasis(&record,1,0,org,ang)||!C1AR14_SampleRaw(p->root,0,&sample)||
    !C1ASR14_StartCorrection(&record,sample.q2,sample.translation,e->r.currentOrigin,e->r.currentAngles))return 0;
 /* A repeat teleports the moving gear back to the platform. */
 if(kind==0&&p->startTick>0)e->s.eFlags^=EF_TELEPORT_BIT;
 if(g_debugMove.integer>1)G_Printf("[tower rig entry] tick=%d owner=%u/%u model=%d elapsed=0 xyz=%.4f %.4f %.4f yaw=%.3f teleport=%d\n",
    level.time,q.entity.slot,q.entity.generation,kind,e->r.currentOrigin[0],e->r.currentOrigin[1],e->r.currentOrigin[2],
    e->r.currentAngles[YAW],!!(e->s.eFlags&EF_TELEPORT_BIT));
 e->s.effect1Time=0x4a5200+kind;
 e->s.effect2Time=q.entity.generation;e->s.effect3Time=(int)q.entity.epoch;
 /* At map start the newly constructed script_models take their current
  * simulation step after GSC setup. An existing rig restarts at phase zero. */
 p->initialStep=p->startTick?0:level.time-level.previousTime;
 e->s.time=level.time-p->initialStep;
 p->record=record;p->startTick=level.time;p->lastTick=e->s.time;p->running=1;strcpy(p->event,q.notify);
 memset(out,0,sizeof(*out));*admitted=2;
 G_Printf("[Training tower] start model=%s owner=%u/%u host=%d requestTick=%d\n",trainingRigClips[kind],q.entity.slot,q.entity.generation,level.time,q.now_ms);
 return 1;
}
int TrainingRigFrame(int now,int(*note)(const GscEntityRef*,const char*,const char*)){
 int i;for(i=0;i<2;i++){
  TrainingRigPlayback*p=trainingRigPlayback+i;TrainingRigTree*t=trainingRigTrees+i;
  c1ar14Sample_t sample;c1asr14Pose_t pose;float phase;gentity_t *e;
  if(!p->running)continue;
  if(!p->root||p->epoch!=cod1EntityEpoch||TrainingRigKind(&t->owner)!=i||now<p->lastTick)return 0;
  if(now==p->lastTick)continue;
  phase=(float)((double)(now-p->startTick+p->initialStep)*p->root->fps/(p->root->intervals*1000.0));
  if(phase>1)phase=1;
  if(!C1AR14_SampleRaw(p->root,phase,&sample)||!C1ASR14_UpdatePose(&p->record,sample.q2,sample.translation,540.f,&pose))return 0;
  e=g_entities+t->owner.slot;G_SetOrigin(e,pose.origin);G_SetAngle(e,pose.angles);
  /* Both samples describe the same server tick; interpolate the moving
   * model's world transform between those snapshots, alongside its bones. */
  e->s.pos.trType=TR_INTERPOLATE;e->s.apos.trType=TR_INTERPOLATE;
  if(e->r.linked)trap_LinkEntity(e);
  if(g_debugMove.integer>1)G_Printf("[tower rig pose] tick=%d owner=%u/%u model=%d elapsed=%d xyz=%.4f %.4f %.4f yaw=%.3f teleport=%d\n",
    now,t->owner.slot,t->owner.generation,i,now-p->startTick,pose.origin[0],pose.origin[1],pose.origin[2],
    pose.angles[YAW],!!(e->s.eFlags&EF_TELEPORT_BIT));
  p->lastTick=now;
  if(g_debugMove.integer&&(now-p->startTick)%500==0)G_Printf("[Training tower] model=%d host=%d phase=%.5f xyz=%.3f %.3f %.3f\n",i,now,phase,pose.origin[0],pose.origin[1],pose.origin[2]);
  if(phase==1){p->running=0;if(!note(&t->owner,p->event,"end"))return 0;}
 }
 return 1;
}
static int FreshPrefixInvoke(void*user,const GscActorScriptedPrefixRequest*input,
 GscEntityExecutionCertificate*out,int*outAdmitted,char*error,size_t cap,
 int(*current)(void*),void*context){
 FreshPrefixCall*c=NULL;GscEntityExecutionCertificate result;cod1ActorLease_t lease;
 int admitted=0,ok=0;char why[256];(void)user;
 if(freshOperation.busy){freshOperation.cancelled=1;return 0;}
 if(!input||!out||!outAdmitted||!current||!FreshExternal(input,sizeof(*input))||
    !FreshExternal(out,sizeof(*out))||!FreshExternal(outAdmitted,sizeof(*outAdmitted))||
    !FreshExternal(error,cap)||!ActorGoalBuffers(input,sizeof(*input),out,sizeof(*out),error,cap)||
    !ActorGoalBuffers(input,sizeof(*input),outAdmitted,sizeof(*outAdmitted),error,cap)||
    ActorOverlap(out,sizeof(*out),outAdmitted,sizeof(*outAdmitted)))return 0;
 {int kind=TrainingRigKind(&input->entity);
  if(kind>=0)return TrainingRigStart(kind,input,out,outAdmitted,current,context);}
 c=calloc(1,sizeof(*c));if(!c)return 0;c->request=*input;
 if(!ActorGoalLease(&c->request.entity,&lease)||G_Cod1ActorGenericPresent(&lease.entity)!=1)goto done;
 c->lease=lease;c->dynamic=ActorFactoryCompletedTarget(&lease,c->request.runtime_identity,c->request.catalog_identity);
 c->jumperRevision=cod1JumperResources.revision;
 if(!FreshRequestCurrent(c)||!FreshTarget(c))goto done;
 c->epoch=cod1EntityEpoch;c->receipt=cod1GenericReceipt;
 c->lookRevision=cod1GenericLookRevision;c->faceRevision=cod1FaceResources.revision;
 c->scriptedRevision=cod1ScriptedResources.revision;
 c->actor=cod1Actors[lease.actorIndex];c->entity=g_entities[lease.entity.slot];
 c->nodeOwner=cod1GenericActors[lease.actorIndex].nodes;
 c->notifyOwner=cod1GenericActors[lease.actorIndex].notify;c->nameOwner=cod1GenericActors[lease.actorIndex].names;
 if(!MoodyPreviousRecord(c,&c->previousRecord,&c->previousRevision,&c->previousClip))goto done;
 if(!ActorGenericClone(&c->generic,cod1GenericActors+lease.actorIndex))goto done;
 freshOperation.busy=1;freshOperation.cancelled=0;freshOperation.stage=c;
 /* Existing Session guard owns current/context and its captured real map cert.
  * No caller-provided numeric receipt or successful fake current is a provider. */
 if(!FreshOwnerCurrent(c)||current(context)!=1||!FreshOwnerCurrent(c)||
    memcmp(input,&c->request,sizeof(*input))||!FreshTarget(c))goto done;
 /* No callbacks from here through queue commit and result copy. Public queue
  * sees ordinary LOCAL buffers, not this protected staged snapshot. */
 /* GAME 36E05 queues row 9, then 36E14..36E38 invokes 109 on the
  * previous record's clip with weight 1, blend .2, rate 1, no name or
  * restart. Prepare the private tree before changing the real queue. */
 if(c->previousClip){
  unsigned n=c->previousClip;
  if(c->generic.revision==UINT64_MAX || !ActorFaceFrameCanMutate(&c->generic,0,0) ||
     !C1AGT14_SetAnim(cod1GenericDefinitions,c->generic.nodes,C1GA_COUNT,n,1,.2f,1,0))goto done;
  ActorGenericNamedCancelSubtree(&c->generic,n);
  if(!ActorFaceFrameCanMutate(&c->generic,0,0))goto done;
 }
 why[0]=0;
 if(!G_Cod1ActorScriptedQueuePush(&lease,&admitted,why,sizeof(why)))goto done;
 if(admitted!=0&&admitted!=1)goto done; /* old producer promises0/1; no fake cast */
 if(c->previousClip){
  C1GARecord *live=cod1GenericActors+lease.actorIndex;
  memcpy(live->nodes,c->generic.nodes,C1GA_COUNT*sizeof(*live->nodes));
  memcpy(live->notify,c->generic.notify,sizeof(*live->notify));memcpy(live->names,c->generic.names,(size_t)c->generic.notify->nameCount*sizeof(*live->names));++live->revision;
 }
 if(c->previousRecord && g_debugMove.integer)
  G_Printf("[Training gesture replacement] owner=%u/%u now=%d old=%u next=%u op=%d state=%d\n",
    lease.entity.slot,lease.entity.generation,level.time,c->previousClip,c->request.animation.ordinal,
    cod1Actors[lease.actorIndex].critical.commands[0].operation,cod1Actors[lease.actorIndex].critical.commands[0].state);
 cod1Actors[lease.actorIndex].scriptedAux.prefix=FRESH_PREFIX_QUEUE_COMMITTED;
 cod1Actors[lease.actorIndex].scriptedAux.admitted=admitted;
 ++cod1Actors[lease.actorIndex].scriptedAux.revision;
 memset(&result,0,sizeof(result));result.owner=lease.entity;result.serial=lease.serial;
 result.current=ActorInheritedCertificate;*out=result;*outAdmitted=admitted;ok=1;
 /* Deliberately NO finish callback, init fields, pose goal, record construction,
  * controller pump, task/global readiness, implicit successful end or rollback. */
 done:
 if(c){ActorGenericFreeOwned(&c->generic);free(c);}
 freshOperation.stage=NULL;freshOperation.busy=0;freshOperation.cancelled=0;
 if(!ok)EntityError(error,cap,"fresh scripted prefix requires current exclusive normal/unattached Actor owner and selected resources (HELD)");
 return ok;
}

static const GscActorScriptedPrefixHost freshPrefixHost={NULL,FreshPrefixInvoke};
const GscActorScriptedPrefixHost *G_Cod1ActorScriptedPrefixHost(void){return &freshPrefixHost;}
