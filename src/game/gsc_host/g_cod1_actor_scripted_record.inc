/* Selected fresh normal/unattached Actor record. Actual heap ownership is
 * indexed by the SAME Actor allocation and canonical scriptedAux state.
 * Source ordinals in numeric data are PORT semantic indices, not fabricated
 * packed native handles. Runtime/catalog/resource epochs accompany them. */
#include "../g_cod1_actor_scripted_record.h"
typedef struct {
 cod1ActorLease_t lease;uint64_t runtime,catalog,resources,revision;
 unsigned clipNative;int created,updated,present;
 c1asr14Record_t value;
} C1NormalRecord;
static C1NormalRecord*cod1NormalRecords[COD1_ACTOR_CAPACITY];
typedef struct {
 GscActorScriptedPrefixRequest q;C1NormalRecord*record;C1FaceFrameCall*frame;
 const GscActorScriptedPrefixRequest*input;
 int busy,cancelled,checking;int(*current)(void*);void*context;
} C1NormalOperation;
static C1NormalOperation cod1NormalOperation;
static int ActorNormalStart(void*,const GscActorScriptedPrefixRequest*,char*,size_t,int(*)(void*),void*);
static const GscActorScriptedStartHost cod1NormalHost={NULL,ActorNormalStart};
static int ActorNormalUpdatePrivate(const void*,size_t);
static void ActorNormalUpdateCancelOwner(const GscEntityRef*);
static void ActorNormalUpdateCancelAll(void);
static int ActorNormalPrivate(const void*p,size_t n){
 unsigned i;if(ActorNormalUpdatePrivate(p,n)||ActorOverlap(p,n,&cod1NormalHost,sizeof(cod1NormalHost))||
  ActorOverlap(p,n,&cod1NormalOperation,sizeof(cod1NormalOperation))||
  ActorOverlap(p,n,cod1NormalRecords,sizeof(cod1NormalRecords))||
  ActorOverlap(p,n,cod1NormalOperation.record,cod1NormalOperation.record?sizeof(C1NormalRecord):0))return 1;
 for(i=0;i<COD1_ACTOR_CAPACITY;i++)if(ActorOverlap(p,n,cod1NormalRecords[i],cod1NormalRecords[i]?sizeof(C1NormalRecord):0))return 1;
 return 0;
}
static void ActorNormalRetire(uint32_t i,const GscEntityRef*owner){
 C1NormalRecord*r;if(i>=COD1_ACTOR_CAPACITY||!owner)return;ActorNormalUpdateCancelOwner(owner);
 if(cod1NormalOperation.busy&&cod1NormalOperation.record&&
    ActorRefEqual(&cod1NormalOperation.record->lease.entity,owner))cod1NormalOperation.cancelled=1;
 r=cod1NormalRecords[i];if(!r||!ActorRefEqual(&r->lease.entity,owner))return;
 cod1NormalRecords[i]=NULL;memset(r,0,sizeof(*r));free(r);
}
static void ActorNormalReset(void){
 unsigned i;ActorNormalUpdateCancelAll();if(cod1NormalOperation.busy)cod1NormalOperation.cancelled=1;
 for(i=0;i<COD1_ACTOR_CAPACITY;i++)if(cod1NormalRecords[i]){
  memset(cod1NormalRecords[i],0,sizeof(C1NormalRecord));free(cod1NormalRecords[i]);cod1NormalRecords[i]=NULL;
 }
}
static int ActorNormalLeafSample(unsigned source,unsigned native,const c1atl14Node_t*clock,c1arc14Result_t*out){
 const c1ar14Root_t*bank[1];c1arc14Node_t node;
 bank[0]=ActorScriptedRoot(source);if(!bank[0]||!clock||!out)return 0;
 memset(&node,0,sizeof(node));node.ordinal=native;node.parent=-1;node.leaf=0;
 node.runtime_present=1;node.goalWeight=clock->goalWeight;node.currentWeight=clock->currentWeight;node.phase=clock->phase;
 return C1ARC14_Current(bank,1,&node,1,0,out);
}
static int ActorNormalNativeOrdinal(unsigned source,unsigned*out){
 C1ActorCurrentRoot*map;int ok=0;if(!out||source>=C1GA_COUNT)return 0;
 map=calloc(1,sizeof(*map));if(!map)return 0;
 if(ActorCurrentRootOrder(map)){*out=map->order[source];ok=1;}free(map);return ok;
}
static int ActorNormalBinding(const GscActorScriptedPrefixRequest *q,const cod1ActorLease_t *l){
 if(q->animation.ordinal==892)return TrainingJumperAllocation(&q->entity)&&
    ActorFactoryCompletedTarget(l,q->runtime_identity,q->catalog_identity)&&FreshDynamicBindings(q);
 return FreshBootstrapTarget(l)&&FreshBindings(q);
}
static int ActorNormalScope(const C1NormalOperation*o){
 cod1ActorLease_t l;const cod1ActorScriptedAux_t*a;const C1GARecord*r;
 if(!o||o!=&cod1NormalOperation||!o->busy||o->cancelled||!o->record||!o->input||
    memcmp(o->input,&o->q,sizeof(o->q))||
    !ActorGoalLease(&o->q.entity,&l)||memcmp(&l,&o->record->lease,sizeof(l))||
    cod1NormalRecords[l.actorIndex]||o->q.now_ms!=level.time||
    o->record->resources!=cod1ScriptedResources.revision||!ActorNormalBinding(&o->q,&l))return 0;
 r=cod1GenericActors+l.actorIndex;
 if(!r->used||r->runtime!=o->q.runtime_identity||r->catalog!=o->q.catalog_identity||
    r->tick!=o->q.now_ms||memcmp(&r->lease,&l,sizeof(l)))return 0;
 a=&cod1Actors[l.actorIndex].scriptedAux;
 return a->actorSerial==l.serial&&a->revision&&a->revision!=UINT64_MAX&&
  a->record==FRESH_AUX_EMPTY&&a->link==FRESH_LINK_UNATTACHED&&a->prefix==FRESH_PREFIX_QUEUE_COMMITTED;
}
static int ActorNormalCurrent(void*opaque){
 C1NormalOperation*o=opaque;int ok;
 if(!ActorNormalScope(o))return 0;
 if(o->checking){o->cancelled=1;return 0;}
 o->checking=1;ok=o->current(o->context)==1&&ActorNormalScope(o);o->checking=0;return ok;
}
/* Native104 preclamps BEFORE computing sibling fade.99 clears children,
 * not the supplied root itself. No legacy101/clearanim helper is redefined. */
static int ActorNormalLook104(C1GARecord*r,unsigned selected,float weight){
 const uint32_t epsBits=0x3a83126fu;float epsilon;memcpy(&epsilon,&epsBits,4);
 if(weight<epsilon)weight=0;
 return C1AKA14_Select(cod1GenericDefinitions,r->nodes,C1GA_COUNT,selected,
        cod1GenericDefinitions[selected].parent,weight,.2f,1,0);
}
static int ActorNormalConstruct(C1NormalOperation*o){
 C1FaceFrameCall*c=o->frame;C1GARecord*r=&c->stage;C1NormalRecord*record=o->record;
 unsigned clip=o->q.animation.ordinal,root=o->q.root.ordinal,i;float origin[3],angles[3],straight,left,right; c1al14State_t look;
 GscActorAnimationRequest request;c1arc14Result_t sample;
 if(memcmp(&record->lease,&c->lease,sizeof(c->lease))||r->runtime!=o->q.runtime_identity||r->catalog!=o->q.catalog_identity||
    !ActorScriptedClockSelected(c->lease.actorIndex)||!c->frame.scripted||c->frame.count||
    c->frame.tick!=o->q.now_ms||r->tick!=o->q.now_ms||!o->q.root_present||!root||
    !(o->q.root.flags&GSC_ANIMATION_NODE_BRANCH)||(clip!=892&&!C1ASB14_Body(ActorScriptedIndex(clip)))||
    (clip!=892 && cod1ScriptedResources.bank.metadata[ActorScriptedIndex(clip)].loop && !C1ASB14_TowerLoop(clip))||!(c->actor->nativeData.entity.nativeEntityFlags&0x20000u)||
    c->actor->nativeData.entity.nativeKind!=0x0bu||c->entity->health<=0||
    c->actor->turretMount.mounted||!ActorNormalNativeOrdinal(clip,&record->clipNative)){G_Printf("[Foley start] rejected at record.inc line 94\n");return 0;}
 memcpy(origin,o->q.origin_bits,12);memcpy(angles,o->q.angles_bits,12);
 if(!C1ASR14_NormalBasis(&record->value,clip,root,origin,angles)){G_Printf("[Foley start] rejected at record.inc line 96\n");return 0;}
 memcpy(&look,r->lookCells,sizeof(look));
 if(!isfinite(look.yaw)){G_Printf("[Foley start] rejected at record.inc line 98\n");return 0;}
 if(look.yaw!=0&&look.ready){
  /* Actual three Look bindings are taken from the live owner. A different
   * tree/role binding remains held; no initial-default yaw is substituted. */
  unsigned s=look.nodes[0],l=look.nodes[1],rr=look.nodes[2];
  G_Printf("[Foley start] owned Look bindings %u %u %u\n",s,l,rr);
  if(s!=4||l!=2040||rr!=2059||strcmp(cod1GenericNames[s].name,"look_straight")||
     strcmp(cod1GenericNames[l].name,"look_left")||strcmp(cod1GenericNames[rr].name,"look_right")){G_Printf("[Foley start] rejected at record.inc line 104\n");return 0;}
  straight=r->nodes[s].currentWeight;left=r->nodes[l].currentWeight;right=r->nodes[rr].currentWeight;
  if(left>0){if(!ActorNormalLook104(r,l,left)){G_Printf("[Foley start] rejected at record.inc line 106\n");return 0;}}
  else if(right>0&&!ActorNormalLook104(r,rr,right)){G_Printf("[Foley start] rejected at record.inc line 107\n");return 0;}
  if(!C1AGT14_SetAnim(cod1GenericDefinitions,r->nodes,C1GA_COUNT,s,straight,.2f,1,0)){G_Printf("[Foley start] rejected at record.inc line 108\n");return 0;}
 }
 /* Actual selected registered definition lookup, not a successful dummy95. */
 if(!ActorGenericBinding(&o->q.animation,o->q.runtime_identity)||
    !ActorGenericBinding(&o->q.root,o->q.runtime_identity)||!cod1GenericDefinitions[root].children){G_Printf("[Foley start] rejected at record.inc line 112\n");return 0;}
 for(i=1;i<C1GA_COUNT;i++)if(cod1GenericDefinitions[i].parent==root){
  if(!C1AGT14_Clear(cod1GenericDefinitions,r->nodes,C1GA_COUNT,i,.2f)){G_Printf("[Foley start] rejected at record.inc line 114\n");return 0;}
  ActorGenericNamedCancelSubtree(r,i);
 }
 memset(&request,0,sizeof(request));request.entity=o->q.entity;request.runtime_identity=o->q.runtime_identity;
 request.catalog_identity=o->q.catalog_identity;request.now_ms=o->q.now_ms;request.method=GSC_ACTOR_ANIMATION_SET;
 request.animation[0]=o->q.animation;request.scalar_bits[0]=0x3f800000u;request.scalar_bits[1]=0x3e4ccccdu;
 request.scalar_bits[2]=0x3f800000u;memcpy(request.notify,o->q.notify,sizeof(request.notify));
 if(!request.notify[0]||!ActorScriptedMutate(r,&request)||
    !ActorNormalLeafSample(clip,record->clipNative,r->nodes+clip,&sample)||
    !C1ASR14_StartCorrection(&record->value,sample.q2,sample.translation,
       c->entity->r.currentOrigin,c->entity->r.currentAngles)){G_Printf("[Foley start] rejected at record.inc line 124\n");return 0;}
 record->present=1;record->created=o->q.now_ms;record->updated=-1;record->revision=1;return 1;
}
static int ActorNormalStart(void*u,const GscActorScriptedPrefixRequest*input,char*error,size_t cap,
 int(*current)(void*),void*ctx){
 C1NormalOperation*o=&cod1NormalOperation;cod1ActorLease_t l;C1NormalRecord*record=NULL;GscEntityRef owner;int ok=0;(void)u;
 G_Printf("[Foley start] entered GAME at tick=%d\n",level.time);
 if(!ActorControllerAnimationMutationAllowed()||!ActorControllerForeignMutationAllowed()){G_Printf("[Foley start] rejected at record.inc line 130\n");return 0;}
 if(o->busy){o->cancelled=1;{G_Printf("[Foley start] rejected at record.inc line 131\n");return 0;}}
 if(!input||!current||!ActorGoalBuffers(input,sizeof(*input),NULL,0,error,cap)||
    !FreshExternal(input,sizeof(*input))||!FreshExternal(error,cap)){G_Printf("[Foley start] rejected at record.inc line 133\n");return 0;}
 memset(o,0,sizeof(*o));o->q=*input;o->input=input;o->current=current;o->context=ctx;
 if(!ActorGoalLease(&o->q.entity,&l)||!ActorNormalBinding(&o->q,&l)||
    cod1NormalRecords[l.actorIndex]||!ActorScriptedClockSelected(l.actorIndex)){G_Printf("[Foley start] rejected at record.inc line 136\n");goto done;}
 owner=o->q.entity;record=calloc(1,sizeof(*record));if(!record){G_Printf("[Foley start] rejected at record.inc line 137\n");goto done;}record->lease=l;
 record->runtime=o->q.runtime_identity;record->catalog=o->q.catalog_identity;
 record->resources=cod1ScriptedResources.revision;o->record=record;o->busy=1;
 if(!ActorNormalCurrent(o)||memcmp(input,&o->q,sizeof(o->q))){G_Printf("[Foley start] rejected at record.inc line 140\n");goto done;}
 o->frame=ActorFaceFrameEnter(&owner,o->q.now_ms,ActorNormalCurrent,o);if(!o->frame){G_Printf("[Foley start] rejected at record.inc line 141\n");goto done;}
 if(!ActorNormalConstruct(o)||memcmp(input,&o->q,sizeof(o->q))||
    !ActorFaceFrameCommit(o->frame,&o->frame->frame)){G_Printf("[Foley start] rejected at record.inc line 143\n");goto done;}
 /* Tree and real record commit with no callback between. The existing Actor
   * allocation remains exact; the same exclusive aux owner now owns the heap. */
 cod1NormalRecords[l.actorIndex]=record;record=NULL;
 cod1Actors[l.actorIndex].scriptedAux.record=FRESH_AUX_OWNED;
 G_Printf("[Foley start] committed normal body actor=%u clip=%u tick=%d goal=%g token=%u\n",owner.slot,o->q.animation.ordinal,o->q.now_ms,cod1GenericActors[l.actorIndex].nodes[o->q.animation.ordinal].goalWeight,cod1GenericActors[l.actorIndex].notify->flags[o->q.animation.ordinal].token);
 cod1Actors[l.actorIndex].scriptedAux.prefix=FRESH_PREFIX_UNUSED;
 ++cod1Actors[l.actorIndex].scriptedAux.revision;ok=1;
done:
 if(o->frame)ActorFaceFrameLeave(o->frame);if(record){memset(record,0,sizeof(*record));free(record);}
 memset(o,0,sizeof(*o));return ok;
}
const GscActorScriptedStartHost*G_Cod1ActorScriptedStartHost(void){return &cod1NormalHost;}

typedef struct {
 int busy,cancelled,checking,now;GscEntityRef owner;const GscEntityRef*input;
 C1NormalRecord*live,before;uint32_t rateBits;
 int(*current)(void*);void*context;
} C1NormalUpdate;
static C1NormalUpdate cod1NormalUpdate;
static int ActorNormalUpdatePrivate(const void*p,size_t n){return ActorOverlap(p,n,&cod1NormalUpdate,sizeof(cod1NormalUpdate));}
static void ActorNormalUpdateCancelOwner(const GscEntityRef*owner){
 if(cod1NormalUpdate.busy&&owner&&ActorRefEqual(owner,&cod1NormalUpdate.owner))cod1NormalUpdate.cancelled=1;
}
static void ActorNormalUpdateCancelAll(void){if(cod1NormalUpdate.busy)cod1NormalUpdate.cancelled=1;}
static int ActorNormalUpdateScope(const C1NormalUpdate*u){
 cod1ActorLease_t lease;const cod1ActorScriptedAux_t*a;cod1EntityNativeStaticRecord_t*stored;
 if(!u||u!=&cod1NormalUpdate||!u->busy||u->cancelled||u->now!=level.time||
    !u->input||memcmp(u->input,&u->owner,sizeof(u->owner))||!ActorGoalLease(&u->owner,&lease)||
    memcmp(&lease,&u->before.lease,sizeof(lease))||cod1NormalRecords[lease.actorIndex]!=u->live||
    !u->live||memcmp(u->live,&u->before,sizeof(u->before))||
    u->before.resources!=cod1ScriptedResources.revision||!ActorScriptedReady(cod1EntityEpoch)||
    !ActorScriptedClockSelected(lease.actorIndex))return 0;
 a=&cod1Actors[lease.actorIndex].scriptedAux;stored=NativeStaticFind(&u->owner);
 return a->actorSerial==lease.serial&&a->revision&&a->revision!=UINT64_MAX&&
  a->record==FRESH_AUX_OWNED&&a->link==FRESH_LINK_UNATTACHED&&stored&&stored->extrasPresent&&
  stored->stored.anglelerprateBits==u->rateBits;
}
static int ActorNormalUpdateCurrent(void*context){
 C1NormalUpdate*u=context;int ok;if(!ActorNormalUpdateScope(u))return 0;
 if(u->checking){u->cancelled=1;return 0;}u->checking=1;
 ok=u->current(u->context)==1&&ActorNormalUpdateScope(u);u->checking=0;return ok;
}
int G_Cod1ActorScriptedRecordUpdate(const GscEntityRef*input,int now,int*outPresent,
 char*error,size_t cap,int(*current)(void*),void*context){
 C1NormalUpdate*u=&cod1NormalUpdate;cod1ActorLease_t lease;GscEntityRef owner;
 C1FaceFrameCall*c=NULL;C1NormalRecord next;c1arc14Result_t sample;c1asr14Pose_t pose;
 cod1EntityNativeStaticRecord_t*stored;float rate;unsigned clip;int finished,ok=0;
 if(!ActorControllerProviderEnter(C1CP_RECORD_UPDATE,input,now,current,context))return 0;
 if(u->busy){u->cancelled=1;return 0;}
 if(!input||!outPresent||!current||!ActorGoalBuffers(input,sizeof(*input),outPresent,sizeof(*outPresent),error,cap)||
    now!=level.time||!ActorGoalLease(input,&lease))return 0;
 owner=*input;if(!cod1NormalRecords[lease.actorIndex])return 0;
 memset(u,0,sizeof(*u));u->owner=owner;u->input=input;u->now=now;u->live=cod1NormalRecords[lease.actorIndex];
 u->before=*u->live;u->current=current;u->context=context;stored=NativeStaticFind(&owner);
 if(!stored||!stored->extrasPresent)goto done;u->rateBits=stored->stored.anglelerprateBits;u->busy=1;
 if(now<u->before.created||now<u->before.updated||!ActorNormalUpdateCurrent(u))goto done;
 /* Native clip0 is a distinct subsequent invocation: actual free/null,
  * with no fabricated note, task cancellation or completion timer. */
 if(!u->before.value.clip){
  if(!ActorNormalUpdateCurrent(u))goto done;
  cod1NormalRecords[lease.actorIndex]=NULL;memset(u->live,0,sizeof(*u->live));free(u->live);
  cod1Actors[lease.actorIndex].scriptedAux.record=FRESH_AUX_EMPTY;
  ++cod1Actors[lease.actorIndex].scriptedAux.revision;*outPresent=0;ok=1;goto done;
 }
 if(!ActorControllerProviderArmFrame(C1CP_RECORD_UPDATE,&owner,now,ActorNormalUpdateCurrent,u))goto done;
 c=ActorFaceFrameEnter(&owner,now,ActorNormalUpdateCurrent,u);if(!c)goto done;
 next=u->before;clip=next.value.clip;
 if(clip>=C1GA_COUNT||next.value.mode||!next.present||c->frame.count||
    c->saved.tick!=now||c->saved.runtime!=next.runtime||c->saved.catalog!=next.catalog||
    !(c->stage.nodes[clip].goalWeight>0)||
    !ActorNormalLeafSample(clip,next.clipNative,c->stage.nodes+clip,&sample))goto done;
 memcpy(&rate,&u->rateBits,4);
 if(!C1ASR14_UpdatePose(&next.value,sample.q2,sample.translation,rate,&pose))goto done;
 if(!next.value.firstUpdate)next.value.firstUpdate=1;
 else{
  const c1atl14Node_t*n=c->stage.nodes+clip;
  /* Constructor109 is the actual allocating writer in this selected complete
   * preallocated tree; retained lease + positive GOAL preserve known slot.
   * A cleared/unknown slot is refused above, never inferred from record heap
   * presence or passed as missing-slot success to native86. General pruning
   * and arbitrary slot deletion require their own real owner. */
  finished=C1ASR14_Finished(1,n->phase,n->oldPhase,(uint16_t)n->cycle,(uint16_t)n->oldCycle);
  if(finished<0)goto done;
  if(finished){
   /*431B0: real109 retains selected leaf goal1/.2/rate1, removes notify,
    * does not force restart; native record packed clip becomes zero. */
   if(!C1AGT14_SetAnim(cod1GenericDefinitions,c->stage.nodes,C1GA_COUNT,clip,1,.2f,1,0))goto done;
   ActorGenericNamedCancelSubtree(&c->stage,clip);next.value.clip=0;
  }
 }
 if(next.revision==UINT64_MAX||!ActorFaceFrameCommit(c,&c->frame))goto done;
 ++next.revision;next.updated=now;*u->live=next;
 /* Native13590 writes entity E0/EC only. Trajectories, velocity, spatial
  * links and publication belong to their later real pipeline, not this step. */
 memcpy(g_entities[owner.slot].r.currentOrigin,pose.origin,12);
 memcpy(g_entities[owner.slot].r.currentAngles,pose.angles,12);
 memcpy(cod1Actors[lease.actorIndex].nativeData.entity.origin,pose.origin,12);
 memcpy(cod1Actors[lease.actorIndex].nativeData.entity.angles,pose.angles,12);
 ActorNativeStaticInvalidateSource(&owner);*outPresent=1;ok=1;
done:
 if(c)ActorFaceFrameLeave(c);memset(u,0,sizeof(*u));return ok;
}

static int MoodyPreviousRecord(const FreshPrefixCall *c,const void **out,uint64_t *revision,unsigned *clip){
 const cod1ActorLease_t *l=&c->lease;
 const cod1ActorRecord_t *a=cod1Actors+l->actorIndex;
 const cod1ActorScriptedController_t *ctl=&a->scriptedController;
 const C1NormalRecord *n=cod1NormalRecords[l->actorIndex];
 const gentity_t *e=g_entities+l->entity.slot;
 *out=NULL;*revision=0;*clip=0;
 if(!n)return a->scriptedAux.record==FRESH_AUX_EMPTY;
 if(c->dynamic || cod1NormalOperation.busy || cod1NormalUpdate.busy || !ActorLeaseValid(l) ||
    !e->targetname || (strcmp(e->targetname,"moody") && strcmp(e->targetname,"foley")) || !e->classname ||
    strcmp(e->classname,"actor_ally_airborne_officer_thompson") || e->health<=0 ||
    !n->present || !n->revision || n->resources!=cod1ScriptedResources.revision ||
    n->runtime!=c->request.runtime_identity || n->catalog!=c->request.catalog_identity ||
    memcmp(&n->lease,l,sizeof(*l)) || a->scriptedAux.record!=FRESH_AUX_OWNED ||
    !ctl->entered || ctl->invalid || ctl->resumed4 || ctl->actorSerial!=l->serial ||
    !ctl->selectionSerial || ctl->force250 || ctl->currentSource!=4 ||
    a->turretMount.mounted || a->turretMount.currentTop!=1 || a->turretMount.currentStack[0]!=4 ||
    a->turretMount.currentStack[1]!=9 || a->critical.top!=1 || a->critical.stack[0]!=4 ||
    a->critical.stack[1]!=9 || a->critical.command_count ||
    !ActorScriptedClockSelected(l->actorIndex) || !C1ASB14_Body(ActorScriptedIndex(c->request.animation.ordinal)))return 0;
 if(n->value.clip && (n->value.clip>=C1GA_COUNT || ActorScriptedIndex(n->value.clip)<0 ||
    !C1ASB14_Body(ActorScriptedIndex(n->value.clip))))return 0;
 *out=n;*revision=n->revision;*clip=n->value.clip;return 1;
}
