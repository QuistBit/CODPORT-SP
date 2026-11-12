/* Optional source-owned Training jumper DATA bank, independent of Foley.
 * Loading it never starts an animation, clock, MAIN or wire publication. */
typedef struct {uint64_t epoch,revision,busy;const void*stage;
 c1ar14Root_t*root;c1ab14Clip_t*body;c1anm14Metadata_t metadata;} C1JumperResources;
static C1JumperResources cod1JumperResources;
static int ActorJumperReady(uint64_t);
static cod1ActorLease_t trainingJumperOwner;
int TrainingJumperRead(GscEntityRef *ref){
 const cod1ActorLease_t*l=&trainingJumperOwner;const cod1ActorRecord_t*a;
 if(!l->serial||!ActorLeaseValid(l)||!ActorJumperReady(cod1EntityEpoch))return 0;
 a=cod1Actors+l->actorIndex;
 if(!ActorFactoryCompletedTarget(l,a->spawnCompletion.runtime,a->spawnCompletion.catalog))return 0;
 *ref=l->entity;return 1;
}
static int TrainingJumperAllocation(const GscEntityRef *ref){
 GscEntityRef current;return TrainingJumperRead(&current)&&ActorRefEqual(&current,ref);
}
static int ActorJumperDataPrivate(const void*p,size_t n,const C1JumperResources*r){
 return TurretBodyPrivate(p,n,r->body)||ActorOverlap(p,n,r->root,r->root?sizeof(*r->root):0);
}
static int ActorJumperPrivate(const void*p,size_t n){C1JumperResources*r=&cod1JumperResources;
 return ActorOverlap(p,n,r,sizeof(*r))||ActorOverlap(p,n,r->stage,r->stage?sizeof(*r):0)||
  ActorJumperDataPrivate(p,n,r)||(r->stage&&ActorJumperDataPrivate(p,n,r->stage));
}
static void ActorJumperReset(void){C1JumperResources*r=&cod1JumperResources;
 TrainingRigPlaybackReset();if(r->revision!=UINT64_MAX)++r->revision;C1AR14_Free(r->root);C1AB14_Free(r->body);
 r->root=NULL;r->body=NULL;r->epoch=0;memset(&r->metadata,0,sizeof(r->metadata));
 /* The loader retains busy/stage through callback unwind. */
}
static int ActorJumperValid(const c1ar14Root_t*r,const c1ab14Clip_t*b){
 return r&&b&&r->magic==C1AR14_MAGIC&&b->magic==C1AB14_MAGIC&&
  r->frames==176&&b->frames==176&&r->serializedFrames==176&&b->serializedFrames==176&&
  r->intervals==175&&b->intervals==175&&r->fps==30&&b->fps==30&&r->flags==2&&b->flags==2&&
  r->bones==52&&b->bones==52&&r->rotationKeys==1&&r->translationKeys==116&&
  r->prefixBytes==1523&&r->noteCount==2&&!strcmp(r->notes[0].name,"attach")&&
  r->notes[0].frame==2&&!strcmp(r->notes[1].name,"detach")&&r->notes[1].frame==173;
}
static int ActorJumperReady(uint64_t epoch){C1JumperResources*r=&cod1JumperResources;
 return epoch&&r->epoch==epoch&&!r->busy&&r->revision&&r->revision!=UINT64_MAX&&
  ActorFaceReady(epoch)&&ActorJumperValid(r->root,r->body)&&C1ANM14_Valid(&r->metadata)&&!r->metadata.loop;
}
static int ActorJumperBinding(const GscActorScriptedPrefixRequest*q){
 return q&&ActorJumperReady(cod1EntityEpoch)&&q->animation.ordinal==892&&
  cod1GenericDefinitions[892].parent==879&&!cod1GenericDefinitions[892].children&&
  !strcmp(cod1GenericNames[892].name,"training_guy_practicejump")&&
  !strcmp(q->animation.tree,"generic_human")&&ActorGenericBinding(&q->animation,q->runtime_identity);
}
int G_Cod1ActorJumperScriptedResourcesLoad(void){
 C1JumperResources*r=&cod1JumperResources,stage;uint64_t epoch=cod1EntityEpoch,revision=r->revision;int ok=0;
 if(ActorJumperReady(epoch))return 1;
 if(!epoch||!level.spawning||!cod1EntityActive||r->busy||r->epoch||r->root||r->body||
    revision==UINT64_MAX||cod1GenericOperation.busy||!ActorFaceReady(epoch))return 0;
 memset(&stage,0,sizeof(stage));r->busy=epoch;r->stage=&stage;
 if(!TurretBankRead("xanim/training_guy_practicejump",&stage.root,&stage.body,epoch)||
    r->revision!=revision||cod1EntityEpoch!=epoch||!level.spawning||!cod1EntityActive||
    !ActorFaceReady(epoch)||!ActorJumperValid(stage.root,stage.body)||
    !C1ANM14_Build(stage.root,&stage.metadata)||stage.metadata.loop)goto done;
 if(r->revision!=revision||r->busy!=epoch||r->epoch||cod1EntityEpoch!=epoch||
    !level.spawning||!cod1EntityActive||!ActorFaceReady(epoch))goto done;
 r->root=stage.root;r->body=stage.body;r->metadata=stage.metadata;r->epoch=epoch;
 r->revision=revision+1;stage.root=NULL;stage.body=NULL;ok=1;
 done:C1AR14_Free(stage.root);C1AB14_Free(stage.body);r->stage=NULL;if(r->busy==epoch)r->busy=0;return ok;
}

int TrainingJumperOwnerIs(const GscEntityRef*ref){return TrainingJumperAllocation(ref);}
