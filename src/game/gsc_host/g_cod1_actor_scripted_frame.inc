/* Explicit two-original-clip owner layered on the retained staged frame
 * transactions. Native definition order is captured from the authenticated
 * full catalog, never confused with the source ordinal namespace. */
#include "../g_cod1_actor_scripted_frame.h"
#include "../../qcommon/cod1_actor_multi_named_clock14.h"
static int ActorNormalNativeOrdinal(unsigned,unsigned*);
static int ActorScriptedMap(C1FaceFrame*f){
 C1ActorCurrentRoot*map;unsigned marks[C1GA_COUNT]={0},i,k,n=0;int parent;
 if(!ActorScriptedReady(cod1EntityEpoch))return 0;
 map=calloc(1,sizeof(*map));if(!map)return 0;
 if(!ActorCurrentRootOrder(map)){free(map);return 0;}
 for(k=0;k<2;k++)for(i=c1asb14Additional[2+k].ordinal;;i=cod1GenericDefinitions[i].parent){marks[i]=1;if(!i)break;}
 for(i=0;i<C1GA_COUNT;i++)if(marks[i]){
  if(n==32){free(map);return 0;}parent=-1;
  if(i){for(k=0;k<n;k++)if(f->scriptedSource[k]==cod1GenericDefinitions[i].parent)parent=(int)k;if(parent<0){free(map);return 0;}}
  f->scriptedSource[n]=i;f->scriptedNative[n]=map->order[i];f->scriptedParent[n++]=parent;
 }
 for(k=0;k<C1ASB14_COUNT;k++){unsigned n=c1asb14Additional[2+k].ordinal;if(ActorScriptedIndex(n)!=(int)k||cod1GenericDefinitions[n].parent!=(C1ASB14_Standing(n)?75:cod1GenericDefinitions[c1asb14Additional[C1ASB14_Body((int)k)?2:3].ordinal].parent)){free(map);return 0;}f->pairNative[k]=map->order[n];}
 for(k=0;k<5;k++){
  const unsigned idle=c1afu14Definitions[51+k].ordinal,face=c1asb14Additional[3].ordinal;
  if(ActorFaceLeafIndex(idle)!=(int)k||cod1GenericDefinitions[idle].parent!=cod1GenericDefinitions[face].parent){free(map);return 0;}
  f->faceNative[k]=map->order[idle];
 }
 f->standNative=map->order[75];f->scriptedCount=n;free(map);return n>2;
}
int G_Cod1ActorScriptedClockSelect(const GscEntityRef*input,int now,int(*current)(void*),void*ctx){
 C1FaceFrameCall*c;C1FaceFrame next;int ok=0;
 if(!ActorControllerProviderEnter(C1CP_CLOCK_SELECT,input,now,current,ctx)||
    !ActorControllerProviderArmFrame(C1CP_CLOCK_SELECT,input,now,current,ctx))return 0;
 c=ActorFaceFrameEnter(input,now,current,ctx);if(!c)return 0;
 if(c->frame.selected||c->saved.tick!=now||!ActorScriptedReady(c->epoch)||ActorGenericNamedActive(&c->stage))goto done;
 memset(&next,0,sizeof(next));next.selected=next.scripted=1;next.tick=now;next.gameFrame=level.framenum;next.runtime=c->saved.runtime;next.catalog=c->saved.catalog;next.identity=c->saved.notify->ownerIdentity;next.lease=c->lease;
 if(!ActorScriptedMap(&next))goto done;ok=ActorFaceFrameCommit(c,&next);
 done:ActorFaceFrameLeave(c);return ok;
}
static int ActorScriptedMutate(C1GARecord*r,void*user){
 const GscActorAnimationRequest*q=user;unsigned n=q->animation[0].ordinal,i,p;int leaf=ActorScriptedIndex(n);const char*end;const c1anm14Metadata_t*m;float f[3];
 if((leaf<0&&!(n==892&&TrainingJumperAllocation(&q->entity)))||!ActorScriptedClockSelected(r->lease.actorIndex)||!ActorScriptedReady(cod1EntityEpoch)||!r->notify||r->notify->serial==UINT64_MAX||!ActorGenericBinding(q->animation,q->runtime_identity)||!(end=memchr(q->notify,0,64))||end==q->notify)return 0;
 /* The ordinary MAIN owns the complete prepared tree. Its preceding clip
  * may retain authored-note cursors which the old implicit-end-only selector
  * cannot represent. Use the same general native command and resource closure
  * as the rest of this tree; retain the outer command receipt/transaction. */
 if(ActorOrdinaryClockOwner(r))return ActorLocomotionMutate(r,q);
 m=n==892?&cod1JumperResources.metadata:cod1ScriptedResources.bank.metadata+leaf;memcpy(f,q->scalar_bits,12);
 if(C1ASB14_Standing(n)){
  /* The original knob replaces the token and resets the note cursor on
   * the target and direct siblings. A previous clip can have consumed several
   * authored marks by its end. Preserve every other named branch. */
  for(i=0;i<C1ASB14_COUNT;i++){
   unsigned old=c1asb14Additional[2+i].ordinal;
   if(C1ASB14_Standing(old)&&r->notify->flags[old].token&&r->notify->flags[old].cursor>0){
    if(cod1GenericDefinitions[old].parent!=cod1GenericDefinitions[n].parent ||
       r->notify->flags[old].token!=old+1 || r->notify->flags[old].descendant ||
       r->notify->flags[old].cursor>(int)cod1ScriptedResources.bank.metadata[i].count)return 0;
    r->notify->flags[old].cursor=-1;
   }
  }
  int restart=n!=131;
  if(q->method!=(restart?GSC_ACTOR_ANIMATION_FLAGGED_ALL_RESTART:GSC_ACTOR_ANIMATION_FLAGGED_ALL) ||
     strcmp(q->notify,restart?"animdone":"transAnimDone2") ||
     !ActorGenericBinding(q->animation+1,q->runtime_identity) || q->animation[1].ordinal!=5 ||
     !C1AFL14_Select(cod1GenericDefinitions,r->nodes,r->notify->flags,C1GA_COUNT,n,5,n+1,f[0],f[1],f[2],restart))return 0;
  r->notify->flags[n].cursor=(int16_t)C1ANC14_Cursor(m->notes,m->count,r->nodes[n].phase);
 }else if(n!=892&&!C1ASB14_Body(leaf)){
  if(q->method!=GSC_ACTOR_ANIMATION_FLAGGED_RESTART||!C1AFK14_Select(cod1GenericDefinitions,r->nodes,r->notify->flags,C1GA_COUNT,n,n+1,f[0],f[1],f[2],1,m->notes,m->count))return 0;
 }else if(n==889 && q->method==GSC_ACTOR_ANIMATION_FLAGGED_ALL_RESTART){
  /* A queued row9 does not yet replace the current row4 tree. The native
   * knob may restart this still-owned idle while the old stop resumes.
   * Admit only the existing exact pending receipt; do not consume it or
   * change MAIN ownership. Tree/lifetime transaction fences remain intact. */
  if((!MoodyStopped(r)&&!MoodyQueuedIdle(r)) || strcmp(q->notify,"idleanim") ||
     !ActorGenericBinding(q->animation+1,q->runtime_identity) || strcmp(q->animation[1].name,"body") ||
     q->scalar_bits[0]!=0x3f800000u || q->scalar_bits[1]!=0x3dcccccdu || q->scalar_bits[2]!=0x3f800000u ||
     !C1AFL14_Select(cod1GenericDefinitions,r->nodes,r->notify->flags,C1GA_COUNT,n,q->animation[1].ordinal,
                     n+1,1,.1f,1,1))return 0;
 }else{
  /* Normal-record109 operation, independently of the ctor's preceding99 or
   * Look-preservation104. This is not ENGINE100 sibling cancellation. */
  if(q->method!=GSC_ACTOR_ANIMATION_SET||q->scalar_bits[0]!=0x3f800000u||q->scalar_bits[1]!=0x3e4ccccdu||q->scalar_bits[2]!=0x3f800000u||(m->loop&&!C1ASB14_TowerLoop(n)))return 0;
  for(p=n;;p=cod1GenericDefinitions[p].parent){if(cod1GenericDefinitions[p].flags&3)return 0;if(!p)break;}
  for(p=cod1GenericDefinitions[n].parent;;p=cod1GenericDefinitions[p].parent){if(r->nodes[p].goalWeight==0){r->notify->flags[p].token=0;r->notify->flags[p].cursor=-1;r->notify->flags[p].descendant=0;}if(!p)break;}
  if(!C1AGT14_SetAnim(cod1GenericDefinitions,r->nodes,C1GA_COUNT,n,1,.2f,1,1))return 0;
  r->notify->flags[n].token=(uint16_t)(n+1);r->notify->flags[n].descendant=0;r->notify->flags[n].cursor=(int16_t)C1ANC14_Cursor(m->notes,m->count,r->nodes[n].phase);
 }
 for(i=0;i<C1GA_COUNT;i++)if(!r->notify->flags[i].token){(void)ActorGenericNameSet(r,i,NULL,0);r->notify->generation[i]=r->notify->pending[i]=0;r->notify->pendingTick[i]=0;}
 if(!ActorGenericNameSet(r,n,q->notify,(size_t)(end-q->notify)))return 0;r->notify->pending[n]=0;r->notify->pendingTick[n]=0;r->notify->generation[n]=++r->notify->serial;return 1;
}
static int ActorScriptedInvoke(void*unused,const GscActorAnimationRequest*input,char*error,size_t cap,int(*current)(void*),void*ctx){
 GscActorAnimationRequest q;(void)unused;
 if(!input||!ActorGoalBuffers(input,sizeof(*input),NULL,0,error,cap)||ActorGenericPrivate(input,sizeof(*input)))return 0;
 q=*input;
 if(ActorScriptedIndex(q.animation[0].ordinal)<0&&q.animation[0].ordinal!=892)return ActorFaceInvoke(NULL,input,error,cap,current,ctx);
 if(q.precision_bits!=53||q.rounding)return 0;
 if(q.method==GSC_ACTOR_ANIMATION_FLAGGED_ALL && !R243CommandSessionTake(input,current,ctx))return 0;
 int accepted=ActorGenericTransaction(&input->entity,q.runtime_identity,q.catalog_identity,q.now_ms,input,sizeof(*input),error,cap,current,ctx,ActorScriptedMutate,&q);
 if(accepted && q.method==GSC_ACTOR_ANIMATION_FLAGGED_ALL && q.animation[0].ordinal==131)
  G_Printf("[stance transition] tick=%d owner=%u clip=%s event=%s\n",q.now_ms,q.entity.slot,q.animation[0].name,q.notify);
 if(!accepted && q.entity.slot<MAX_GENTITIES && cod1ActorByEntity[q.entity.slot]){
   unsigned i=cod1ActorByEntity[q.entity.slot]-1;
   if(i<COD1_ACTOR_CAPACITY){
    const C1GARecord *g=cod1GenericActors+i;const C1FaceFrame *f=cod1FaceFrames+i;
    G_Printf("[Foley private] refused snapshot now=%d genericTick=%d selected=%d scripted=%d frameTick=%d open=%d done=%d poisoned=%d resources=%llu catalogMatch=%d runtimeMatch=%d\n",
      q.now_ms,g->tick,f->selected,f->scripted,f->tick,f->open,f->done,f->poisoned,(unsigned long long)cod1ScriptedResources.epoch,
      q.catalog_identity==g->catalog,q.runtime_identity==g->runtime);
   }
 }
 return accepted;
}
const GscActorAnimationHost*G_Cod1ActorScriptedAnimationHost(void){static const GscActorAnimationHost h={NULL,ActorGenericValidate,ActorScriptedInvoke};return &h;}
int G_Cod1ActorScriptedSampleFull(const GscEntityRef*input,int now,float output[98],float phases[2][21],
 float facial[2][25],int(*current)(void*),void*ctx){
 C1FaceFrameCall*c=NULL;float weights[98]={0},times[2][21]={{0}},face[2][25]={{0}};c1ast14Node_t checked[C1ASB14_NODES];unsigned i;int ok=0;
 if(!input||!output||!phases||!current||!ActorGoalBuffers(input,sizeof(*input),output,sizeof(weights),NULL,0)||!ActorGoalBuffers(input,sizeof(*input),phases,sizeof(times),NULL,0)||ActorOverlap(output,sizeof(weights),phases,sizeof(times))||ActorGenericPrivate(output,sizeof(weights))||ActorGenericPrivate(phases,sizeof(times)))return 0;
 if(facial && (!ActorGoalBuffers(input,sizeof(*input),facial,sizeof(face),NULL,0) ||
    ActorOverlap(facial,sizeof(face),output,sizeof(weights)) || ActorOverlap(facial,sizeof(face),phases,sizeof(times)) ||
    ActorGenericPrivate(facial,sizeof(face))))return 0;
 /* The baseline is private until leave. Sampling never commits a staged tree. */
 c=ActorFaceFrameAcquire(input,now,current,ctx,0);if(!c)return 0;
 if(!c->frame.scripted||c->saved.tick!=now||!ActorScriptedReady(c->epoch))goto done;
 for(i=0;i<C1ASB14_NODES;i++){
  const c1afu14Definition_t*d=C1ASB14_Definition(i);unsigned source=d->ordinal;const char*name;
  if(source>=C1GA_COUNT||(i&&cod1GenericDefinitions[source].parent!=C1ASB14_Definition((unsigned)d->parent)->ordinal))goto done;
  if(d->leaf>=0){name=d->leaf<47?C1AFU14_Name((unsigned)d->leaf):c1asb14Names[d->leaf-47];if(strcmp(name,cod1GenericNames[source].name))goto done;}
  if(source==131){weights[97]=c->baseline.nodes[source].currentWeight;
   if(weights[97])times[1][16]=c->baseline.nodes[source].phase;
   continue;
  }
  if(i>=C1ASB14_STAND_FIRST){
   unsigned wire=C1ASB14_BASE_NODES+i-C1ASB14_STAND_FIRST;
   weights[wire]=c->baseline.nodes[source].currentWeight;
   if(weights[wire])weights[wire+C1ASB14_STANDING]=c->baseline.nodes[source].phase;
   continue;
  }
  if(i>=C1ASB14_BASE_NODES){
   unsigned slot=i-C1ASB14_BASE_NODES;
   face[0][slot]=c->baseline.nodes[source].currentWeight;
   if(face[0][slot]){
    if(!facial)goto done;
    face[1][slot]=c->baseline.nodes[source].phase;
   }
   continue;
  }
  weights[i]=c->baseline.nodes[source].currentWeight;if(d->phase>=0){
   if(d->phase>=C1ASB14_PHASES)goto done;
   /* Extended gesture phases share the route part of the snapshot.
    * A fully faded leaf no longer contributes to the pose. Keep its live
    * tree clock for callbacks and export a phase only when it contributes. */
   if(d->phase<32 || weights[i]>0)
       times[d->phase/21][d->phase%21]=c->baseline.nodes[source].phase;
  }
 }
 if(!C1ASB14_NodesFull(weights,times,face,checked)||current(ctx)!=1||!ActorFaceFrameFence(c))goto done;
 memcpy(output,weights,sizeof(weights));memcpy(phases,times,sizeof(times));if(facial)memcpy(facial,face,sizeof(face));ok=1;
 done:ActorFaceFrameLeave(c);return ok;
}
int G_Cod1ActorScriptedSample(const GscEntityRef*owner,int now,float weights[98],float phases[2][21],int(*current)(void*),void*ctx){
 return G_Cod1ActorScriptedSampleFull(owner,now,weights,phases,NULL,current,ctx);
}
/* A pair may fade while a new authored pair becomes current. Only named leaves
 * participate in earliest-event selection. The rest advance on that same dt. */
static int ActorScriptedFrameAdvance(C1GARecord*r,C1FaceFrame*f,float dt){
 c1amn14Node_t nodes[32];
 c1afl14State_t flags[2];
 c1amn14Step_t step;
 const c1anm14Metadata_t *metadata[2];
 unsigned selected[2]={0,0},native[2]={0,0},source[32],i,k,p;
 int active,idle=0;
 c1anc14Step_t other;
 if(!f->scripted||!f->scriptedCount||f->scriptedCount>32||!ActorScriptedReady(cod1EntityEpoch))return 0;
 for(i=0;i<C1ASB14_COUNT;i++){
  unsigned n=c1asb14Additional[2+i].ordinal, lane=(C1ASB14_Body((int)i)||C1ASB14_Standing(n))?0:1;
  if(!r->notify->flags[n].token)continue;
  if(selected[lane])return 0;
  selected[lane]=n;metadata[lane]=cod1ScriptedResources.bank.metadata+i;native[lane]=f->pairNative[i];
 }
 if(r->notify->flags[892].token){
  if(selected[0]||!TrainingJumperAllocation(&r->lease.entity)||!ActorNormalNativeOrdinal(892,&native[0]))return 0;
  selected[0]=892;metadata[0]=&cod1JumperResources.metadata;
 }
 for(i=0;i<5;i++){
  unsigned n=c1afu14Definitions[51+i].ordinal;
  if(!r->notify->flags[n].token)continue;
  if(selected[1]||ActorFaceLeafIndex(n)!=(int)i)return 0;
  idle=i>=2;selected[1]=n;metadata[1]=cod1FaceResources.metadata+i;native[1]=f->faceNative[i];
 }
 for(i=0;i<2;i++){
  if(!selected[i]){selected[i]=c1asb14Additional[2+i].ordinal;metadata[i]=cod1ScriptedResources.bank.metadata+i;native[i]=f->pairNative[i];}
  flags[i]=r->notify->flags[selected[i]];
 }
 unsigned used=0;
 int remap[32];
 for(i=0;i<f->scriptedCount;i++){
  unsigned n=f->scriptedSource[i];int lane=ActorScriptedIndex(n);
  nodes[i].nativeOrdinal=f->scriptedNative[i];nodes[i].parent=f->scriptedParent[i];nodes[i].leaf=lane;nodes[i].present=1;
  if(lane>=0){if(lane>=2)return 0;n=selected[lane];nodes[i].nativeOrdinal=native[lane];}
  /* The standing leaves are direct children of stand(75), below body(5).
   * Replace scripted(801) with this actual branch and omit level_training(879). */
  if(C1ASB14_Standing(selected[0]) && n==879){remap[i]=remap[f->scriptedParent[i]];continue;}
  if(C1ASB14_Standing(selected[0]) && n==801){n=75;nodes[i].nativeOrdinal=f->standNative;}
  if(nodes[i].parent>=0)nodes[i].parent=remap[nodes[i].parent];
  nodes[i].clock=r->nodes[n];source[used]=n;remap[i]=(int)used;nodes[used++]=nodes[i];
 }
 const c1ar14Root_t *bodyRoot=ActorScriptedRoot(selected[0]);
 if(!bodyRoot||!C1AMN14_AdvanceBodyPair(nodes,used,flags,metadata[0],metadata[1],dt,1,1,idle,bodyRoot->noteCount,&step))return 0;
 for(i=0;i<used;i++)r->nodes[source[i]]=nodes[i].clock;
 for(i=0;i<2;i++)r->notify->flags[selected[i]]=flags[i];
 for(i=1;i<C1GA_COUNT;i++){
  int index;
  if(cod1GenericDefinitions[i].children||i==selected[0]||i==selected[1]||
     (!r->nodes[i].currentWeight&&!r->nodes[i].goalWeight))continue;
  if(r->notify->flags[i].token)return 0;
  active=1;
  for(p=i;;p=cod1GenericDefinitions[p].parent){if(!r->nodes[p].currentWeight){active=0;break;}if(!p)break;}
  if(!active)continue;
  index=ActorScriptedIndex(i);
  if(index>=0||(i==892&&TrainingJumperAllocation(&r->lease.entity))){
   const c1anm14Metadata_t*m=i==892?&cod1JumperResources.metadata:cod1ScriptedResources.bank.metadata+index;
   if(m->loop){
    if((!C1ASB14_TowerLoop(i) && !C1ASB14_Standing(i)) || !C1ANC14_AdvanceLoopEnd(r->nodes+i,r->notify->flags+i,m->notes,m->frequency,step.usedDt,1,0,&other))return 0;
   }else if(!C1ANC14_Advance(r->nodes+i,r->notify->flags+i,m->notes,m->count,m->frequency,0,step.usedDt,1,0,&other))return 0;
  }else if(!ActorFaceFrameAdvanceNode(r,i,step.usedDt,0,&other))return 0;
 }
 if(step.count>C1ANC14_MAX_NOTES)return 0;
 for(k=0;k<step.count;k++){
  cod1ActorFaceNote_t*note=f->notes+k;
  unsigned lane=step.events[k].leaf,n=selected[lane];
  memset(note,0,sizeof(*note));note->entity=r->lease.entity;note->runtime=r->runtime;note->catalog=r->catalog;
  note->owner_identity=r->notify->ownerIdentity;note->generation=r->notify->generation[n];
  note->frame_generation=f->frame;note->ordinal=n;note->produced_ms=r->tick;
  memcpy(note->event,ActorGenericName(r,n),sizeof(note->event));
  if(!note->generation||!note->event[0]||!C1ANM14_Resolve(metadata[lane],step.events[k].payload,note->payload))return 0;
 }
 f->count=step.count;f->done=!step.resume;if(f->done&&!f->count)f->open=0;return 1;
}


#include "../../qcommon/cod1_actor_source_pose14.h"
#include "g_cod1_actor_prone_pose.inc"
#include "g_cod1_actor_source_pose.inc"

#include "g_cod1_actor_source_tags.inc"
