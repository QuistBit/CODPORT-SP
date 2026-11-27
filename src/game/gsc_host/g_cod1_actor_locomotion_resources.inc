#include "../../qcommon/cod1_actor_generic_clock14.h"
/* Resource ownership for a real ATR subtree. Admission to the frame clock,
 * notetrack sender and pose publisher is deliberately separate from loading. */
typedef struct {
 c1ar14Root_t *root;c1ab14Clip_t *body;c1anm14Metadata_t *metadata;
 TurretClipExtent extent;
} C1LocoLeaf;
typedef struct {
 uint64_t epoch,revision,busy;unsigned root,transitionRoot,count,authored;
 C1LocoLeaf leaf[C1GA_COUNT];float frequency[C1GA_COUNT];unsigned ordinals[C1GA_COUNT];
 const c1anm14Metadata_t*metadata[C1GA_COUNT];
 TurretClipExtent extent;
 c1agf14Topology_t topology;
} C1LocoResources;
static C1LocoResources cod1LocoResources,*cod1LocoStage;
static void ActorLocomotionFree(C1LocoResources*r){unsigned i;if(!r)return;
 for(i=0;i<C1GA_COUNT;i++){C1AR14_Free(r->leaf[i].root);C1AB14_Free(r->leaf[i].body);free(r->leaf[i].metadata);}
 memset(r,0,sizeof(*r));
}
static void ActorLocomotionReset(void){
 uint64_t revision=cod1LocoResources.revision,busy=cod1LocoResources.busy;
 ActorLocomotionFree(&cod1LocoResources);
 cod1LocoResources.revision=revision==UINT64_MAX?revision:revision+1;
 /* An in-flight loader owns its staging allocation until it unwinds. */
 cod1LocoResources.busy=busy;
}
static int ActorLocomotionPrivate(const void*p,size_t n){
 C1LocoResources*r;unsigned stage,i,k;uintptr_t address=(uintptr_t)p;
 if(!p||!n)return 0;
 if(ActorOverlap(p,n,&cod1LocoResources,sizeof(cod1LocoResources))||ActorOverlap(p,n,&cod1LocoStage,sizeof(cod1LocoStage)))return 1;
 for(stage=0;stage<2;stage++){
  r=stage?cod1LocoStage:&cod1LocoResources;if(!r)continue;
  if(ActorOverlap(p,n,r,sizeof(*r)))return 1;
  if(!stage&&r->epoch&&(address>r->extent.hi||(address<r->extent.lo&&n<=r->extent.lo-address)))continue;
  for(k=0;k<r->count;k++){i=r->ordinals[k];if(r->leaf[i].root){
   C1LocoLeaf*l=r->leaf+i;
   if(!stage&&r->epoch&&(address>l->extent.hi||(address<l->extent.lo&&n<=l->extent.lo-address)))continue;
   if(TurretBodyPrivate(p,n,l->body)||ActorOverlap(p,n,l->root,sizeof(*l->root))||
      (l->metadata&&ActorOverlap(p,n,l->metadata,sizeof(*l->metadata))))return 1;
  }}
 }
 return 0;
}
static int ActorLocomotionReady(uint64_t epoch){return epoch&&cod1LocoResources.epoch==epoch&&!cod1LocoResources.busy&&cod1GenericEpoch==epoch;}
/* Original aim.gsc transition family and stop.gsc non-cover rest palette.
 * These are actual generic_human source leaves, owned by the same prepared
 * resource bank/clock/notify/pose transaction as the run tree. */
static const struct {unsigned ordinal;const char*name;} cod1RestLeaves[]={
{76u,"crouch_alert2aim_A"},
{77u,"crouch_alert_A_idle"},
{78u,"crouch_alert_A_twitch"},
{79u,"crouch_aim2alert_A"},
{80u,"crouch_alertB2aim"},
{81u,"crouch_alert_B_idle1"},
{82u,"crouch_alert_B_twitch1"},
{83u,"crouch_aim2alert_B"},
{100u,"wounded_alert2aim"},
{101u,"wounded_woundedalert_twitch"},
{102u,"wounded_aim2woundedalert"},
{103u,"wounded_woundedalert_idle"},
{104u,"stand_aim2alert_1"},
{105u,"stand_aim2alert_2"},
{106u,"stand_alert2aim"},
{116u,"stand_aim2alertb"},
{119u,"stand_alertb2aim"},
{120u,"stand_aim_straight_loop1"},
{121u,"stand_aim_straight_loop2"},
{122u,"stand_aim_straight_loop3"},
{123u,"stand_aim_straight_loop4"},
};
static int ActorRestLeaf(unsigned n){unsigned i;for(i=0;i<sizeof(cod1RestLeaves)/sizeof(cod1RestLeaves[0]);i++)if(cod1RestLeaves[i].ordinal==n)return 1;return 0;}
static int ActorRestCatalog(void){unsigned i,n;for(i=0;i<sizeof(cod1RestLeaves)/sizeof(cod1RestLeaves[0]);i++){n=cod1RestLeaves[i].ordinal;if(n>=C1GA_COUNT||cod1GenericDefinitions[n].children||strcmp(cod1GenericNames[n].name,cod1RestLeaves[i].name))return 0;}return 1;}
static int ActorRestActive(const C1GARecord*r){unsigned i,n;
 if(!r||!r->nodes||!r->notify||!ActorLocomotionReady(cod1EntityEpoch)||!ActorScriptedClockSelected(r->lease.actorIndex)||cod1Actors[r->lease.actorIndex].turretMount.mounted)return 0;
 for(i=0;i<sizeof(cod1RestLeaves)/sizeof(cod1RestLeaves[0]);i++){n=cod1RestLeaves[i].ordinal;if(r->nodes[n].currentWeight||r->nodes[n].goalWeight||r->notify->flags[n].token)return 1;}return 0;
}
/* Original ladder_up/down and wall_dive traversal source leaves.
 * These are actual generic_human source leaves, owned by the same prepared
 * resource bank/clock/notify/pose transaction as the run tree. */
static const struct {unsigned ordinal;const char*name;} cod1TraverseLeaves[]={
{311u,"jump_over_low_wall"},
{318u,"ladder_climbdown"},
{319u,"ladder_climbon"},
{320u,"ladder_climboff"},
{321u,"ladder_climbup"},
};
static int ActorTraverseLeaf(unsigned n){unsigned i;for(i=0;i<sizeof(cod1TraverseLeaves)/sizeof(cod1TraverseLeaves[0]);i++)if(cod1TraverseLeaves[i].ordinal==n)return 1;return 0;}
static int ActorTraverseCatalog(void){unsigned i,n;for(i=0;i<sizeof(cod1TraverseLeaves)/sizeof(cod1TraverseLeaves[0]);i++){n=cod1TraverseLeaves[i].ordinal;if(n>=C1GA_COUNT||cod1GenericDefinitions[n].children||strcmp(cod1GenericNames[n].name,cod1TraverseLeaves[i].name))return 0;}return 1;}
static int ActorTraverseActive(const C1GARecord*r){unsigned i,n;
 if(!r||!r->nodes||!r->notify||!ActorLocomotionReady(cod1EntityEpoch)||!ActorScriptedClockSelected(r->lease.actorIndex)||cod1Actors[r->lease.actorIndex].turretMount.mounted)return 0;
 for(i=0;i<sizeof(cod1TraverseLeaves)/sizeof(cod1TraverseLeaves[0]);i++){n=cod1TraverseLeaves[i].ordinal;if(r->nodes[n].currentWeight||r->nodes[n].goalWeight||r->notify->flags[n].token)return 1;}return 0;
}
/* Remaining original setposemovement transition source leaves.
 * These are actual generic_human source leaves, owned by the same prepared
 * resource bank/clock/notify/pose transaction as the run tree. */
static const struct {unsigned ordinal;const char*name;} cod1MovementLeaves[]={
{63u,"pistol_crouchaimstraight2stand"},
{84u,"crouch2stand"},
{136u,"stand_walk_start_straight"},
{137u,"stand2quickstep_straight"},
{140u,"crouch2crouchwalk_straight"},
{225u,"stand_walk_stop_lff"},
{226u,"stand_walk_stop_rff"},
{228u,"crouchwalk_stop_rff"},
};
static int ActorMovementLeaf(unsigned n){unsigned i;for(i=0;i<sizeof(cod1MovementLeaves)/sizeof(cod1MovementLeaves[0]);i++)if(cod1MovementLeaves[i].ordinal==n)return 1;return 0;}
static int ActorMovementCatalog(void){unsigned i,n;for(i=0;i<sizeof(cod1MovementLeaves)/sizeof(cod1MovementLeaves[0]);i++){n=cod1MovementLeaves[i].ordinal;if(n>=C1GA_COUNT||cod1GenericDefinitions[n].children||strcmp(cod1GenericNames[n].name,cod1MovementLeaves[i].name))return 0;}return 1;}
static int ActorMovementActive(const C1GARecord*r){unsigned i,n;
 if(!r||!r->nodes||!r->notify||!ActorLocomotionReady(cod1EntityEpoch)||!ActorScriptedClockSelected(r->lease.actorIndex)||cod1Actors[r->lease.actorIndex].turretMount.mounted)return 0;
 for(i=0;i<sizeof(cod1MovementLeaves)/sizeof(cod1MovementLeaves[0]);i++){n=cod1MovementLeaves[i].ordinal;if(r->nodes[n].currentWeight||r->nodes[n].goalWeight||r->notify->flags[n].token)return 1;}return 0;
}
static int ActorProneTarget(unsigned n){return (n>=254&&n<=284)||(n>=229&&n<=231);}
static int ActorProneTreeActive(const C1GARecord*r){unsigned i;if(!r||!r->nodes)return 0;for(i=229;i<=284;i++)if(ActorProneTarget(i)&&(r->nodes[i].currentWeight||r->nodes[i].goalWeight))return 1;return 0;}
/* Original weaponList auto and rifle aim/shoot sets, standing and crouched. */
static int ActorAimLeaf(unsigned n){return (n>=15&&n<=17)||(n>=22&&n<=24)||(n>=26&&n<=28)||(n>=36&&n<=38)||(n>=40&&n<=42)||(n>=51&&n<=53);}
static int ActorAimTarget(unsigned n){return ActorAimLeaf(n)||n==13||n==14||n==21||n==25||n==35||n==39||n==50;}
static int ActorAimActive(const C1GARecord*r){
 unsigned i;if(!r||!r->nodes||!r->notify||!ActorLocomotionReady(cod1EntityEpoch)||!ActorScriptedClockSelected(r->lease.actorIndex)||cod1Actors[r->lease.actorIndex].turretMount.mounted)return 0;
 for(i=13;i<=53;i++)if(ActorAimTarget(i)&&(r->nodes[i].currentWeight||r->nodes[i].goalWeight))return 1;
 return 0;
}
/* Original face.gsc aimface and autofireface palettes, including angst02/03.
 * Keep the actual GSC random selection, named animation and completion. */
static int ActorAimFaceLeaf(unsigned n){return n>=1618&&n<=1622;}
static int ActorAimFaceActive(const C1GARecord*r){
 static const unsigned nodes[]={1618,1619,1620,1621,1622};unsigned i,n;
 if(!r||!r->nodes||!r->notify||!ActorLocomotionReady(cod1EntityEpoch)||!ActorScriptedClockSelected(r->lease.actorIndex)||cod1Actors[r->lease.actorIndex].turretMount.mounted)return 0;
 for(i=0;i<5;i++){n=nodes[i];if(r->nodes[n].currentWeight||r->nodes[n].goalWeight||r->notify->flags[n].token)return 1;}
 return 0;
}
/* MAIN starts its face before its first body command. Once native selection
 * has published the ordinary source, tree ownership cannot be inferred from
 * nonzero body weights. Keep the canonical lease and selected frame; all
 * resource, token, descendant and pending checks remain in the full closure. */
static int ActorOrdinaryClockOwner(const C1GARecord*);
static int ActorGeneralClockActive(const C1GARecord*r){return ActorOrdinaryClockOwner(r)||ActorLocomotionActive(r)||ActorAimActive(r)||ActorAimFaceActive(r)||ActorProneTreeActive(r)||ActorRestActive(r)||ActorTraverseActive(r)||ActorMovementActive(r);}
static int ActorLocomotionLoad(void){
 C1LocoResources*r=&cod1LocoResources,*v=NULL;C1ActorRootOrderScratch*order=NULL;
 uint64_t epoch=cod1EntityEpoch,revision=r->revision;unsigned root,i;int ok=0;char path[128]="topology";
 if(ActorLocomotionReady(epoch))return 1;
 if(!epoch||!level.spawning||level.restarted||!cod1EntityActive||r->epoch||r->busy||cod1LocoStage||revision==UINT64_MAX||!ActorGenericBuild()||!ActorRestCatalog()||!ActorTraverseCatalog()||!ActorMovementCatalog())return 0;
 root=ActorGenericOrdinal("walk_and_run_loops");if(!root||(cod1GenericDefinitions[root].flags&7)!=5)return 0;
 v=calloc(1,sizeof(*v));order=calloc(1,sizeof(*order));if(!v||!order)goto done;
 r->busy=epoch;cod1LocoStage=v;v->root=root;v->transitionRoot=ActorGenericOrdinal("crouch2crouchrun");v->extent.lo=UINTPTR_MAX;
 if(!v->transitionRoot||(cod1GenericDefinitions[v->transitionRoot].flags&7)!=6)goto done;
 if(!ActorCurrentRootOrder(order)||!C1AGF14_Build(cod1GenericDefinitions,C1GA_COUNT,order->order,&v->topology))goto done;
 for(i=1;i<C1GA_COUNT;i++)if(!cod1GenericDefinitions[i].children&&
   (C1AGT14_Descendant(cod1GenericDefinitions,i,root)||C1AGT14_Descendant(cod1GenericDefinitions,i,v->transitionRoot)||C1AL14_Index(cod1GenericNames[i].name)>=0||
    ActorGenericShuffleIndex(cod1GenericNames[i].name)>=0||ActorFaceLeafIndex(i)>=0||ActorScriptedIndex(i)>=0||ActorAimLeaf(i)||ActorAimFaceLeaf(i)||ActorProneTarget(i)||ActorRestLeaf(i)||ActorTraverseLeaf(i)||ActorMovementLeaf(i)||i==892)){
  C1LocoLeaf*l=v->leaf+i;v->ordinals[v->count++]=i;
  snprintf(path,sizeof(path),"xanim/%s",cod1GenericNames[i].name);
  if(!TurretBankRead(path,&l->root,&l->body,epoch)||r->revision!=revision||cod1EntityEpoch!=epoch||!level.spawning||!cod1EntityActive||
     (C1AGT14_Descendant(cod1GenericDefinitions,i,root)&&!(l->root->flags&1u))||l->root->bones!=l->body->bones)goto done;
  if(i==892&&!ActorJumperValid(l->root,l->body))goto done;
  l->metadata=malloc(sizeof(*l->metadata));
  if(!l->metadata||!C1AGM14_Build(l->root,l->metadata))goto done;
  v->frequency[i]=l->metadata->frequency;v->metadata[i]=l->metadata;v->authored+=l->metadata->authoredCount;
  l->extent.lo=UINTPTR_MAX;l->extent.hi=0;
  TurretBodyRanges(NULL,0,l->body,&l->extent);TurretClipRange(NULL,0,l->root,sizeof(*l->root),&l->extent);
  TurretClipRange(NULL,0,l->metadata,sizeof(*l->metadata),&l->extent);
  if(l->extent.lo<v->extent.lo)v->extent.lo=l->extent.lo;
  if(l->extent.hi>v->extent.hi)v->extent.hi=l->extent.hi;
  G_Printf("[377 locomotion resource] ordinal=%u name=%s frames=%u intervals=%u fps=%u notes=%u\n",i,cod1GenericNames[i].name,l->root->frames,l->root->intervals,l->root->fps,l->root->noteCount);
 }
 if(!v->count||r->revision!=revision||cod1EntityEpoch!=epoch||!level.spawning||level.restarted||!cod1EntityActive||r->busy!=epoch||r->epoch)goto done;
 v->epoch=epoch;v->revision=revision;*r=*v;memset(v,0,sizeof(*v));ok=1;
 G_Printf("[377 general clock bank] root=%u clips=%u authored_notes=%u epoch=%llu ready=1\n",r->root,r->count,r->authored,(unsigned long long)epoch);
done:
 if(!ok)G_Printf("[377 locomotion bank] refused root=%u path=%s\n",root,v?path:"allocation");
 ActorLocomotionFree(v);free(v);free(order);cod1LocoStage=NULL;if(r->busy==epoch)r->busy=0;return ok;
}
static int ActorLocomotionTarget(unsigned n){
 return ActorLocomotionReady(cod1EntityEpoch)&&n<C1GA_COUNT&&(ActorProneTarget(n)||ActorMovementLeaf(n)||ActorTraverseLeaf(n)||ActorRestLeaf(n)||C1AGT14_Descendant(cod1GenericDefinitions,n,cod1LocoResources.root)||C1AGT14_Descendant(cod1GenericDefinitions,n,cod1LocoResources.transitionRoot));
}
static int ActorLocomotionActive(const C1GARecord*r){
 unsigned root=cod1LocoResources.root;
 return r&&r->nodes&&r->notify&&ActorLocomotionReady(cod1EntityEpoch)&&root&&
  (r->nodes[root].currentWeight>0||r->nodes[root].goalWeight>0||r->nodes[cod1LocoResources.transitionRoot].currentWeight>0||r->nodes[cod1LocoResources.transitionRoot].goalWeight>0);
}
static const c1ar14Root_t*ActorLocomotionRoot(unsigned n){
 return n<C1GA_COUNT&&ActorLocomotionReady(cod1EntityEpoch)?cod1LocoResources.leaf[n].root:NULL;
}
static const c1ab14Clip_t*ActorLocomotionBody(unsigned n){
 return n<C1GA_COUNT&&ActorLocomotionReady(cod1EntityEpoch)?cod1LocoResources.leaf[n].body:NULL;
}
/* Four inactive rows can be proved without following leaf metadata. Read
 * every pending bit, token and both live weights. Other fields are still
 * checked by ActorFaceTreeValid above. No remembered state or skipped tick. */
static int ActorLocomotionFourInactive(const C1GARecord*r,unsigned i){
#if defined(_MSC_VER) && defined(_M_IX86) && _M_IX86_FP >= 2
 const __m128i zero=_mm_setzero_si128();
 __m128i weight=_mm_or_si128(_mm_loadu_si128((const __m128i*)&r->nodes[i].remaining),_mm_loadu_si128((const __m128i*)&r->nodes[i+1].remaining));
 __m128i tail=_mm_or_si128(_mm_loadu_si128((const __m128i*)&r->nodes[i+2].remaining),_mm_loadu_si128((const __m128i*)&r->nodes[i+3].remaining));
 __m128i token=_mm_or_si128(_mm_loadu_si128((const __m128i*)(r->notify->flags+i)),_mm_loadu_si128((const __m128i*)(r->notify->flags+i+2)));
 __m128i pending=_mm_or_si128(_mm_loadu_si128((const __m128i*)(r->notify->pending+i)),_mm_loadu_si128((const __m128i*)(r->notify->pending+i+2)));
 weight=_mm_and_si128(_mm_or_si128(weight,tail),_mm_setr_epi32(0,-1,-1,0));
 token=_mm_and_si128(token,_mm_setr_epi16(0,0,-1,0,0,0,-1,0));
 return _mm_movemask_epi8(_mm_cmpeq_epi8(_mm_or_si128(pending,_mm_or_si128(weight,token)),zero))==65535;
#else
 (void)r;(void)i;return 0;
#endif
}
static int ActorLocomotionClosure(const C1GARecord*r){
 unsigned i;
 if(!r||!r->notify||!r->nodes||!ActorLocomotionReady(cod1EntityEpoch)||!ActorScriptedClockSelected(r->lease.actorIndex)||
    cod1Actors[r->lease.actorIndex].turretMount.mounted||!ActorFaceTreeValid(r->nodes))return 0;
 for(i=0;i<C1GA_COUNT;i++){
  if(i+3<C1GA_COUNT&&ActorLocomotionFourInactive(r,i)){i+=3;continue;}
  unsigned leaf=cod1GenericDefinitions[i].children?r->notify->flags[i].descendant:i;
  if(r->notify->pending[i])return 0;
  if(i==892&&(r->nodes[i].currentWeight||r->nodes[i].goalWeight||r->notify->flags[i].token)&&!ActorJumperTreeLeaf(r,i))return 0;
  if(!cod1GenericDefinitions[i].children&&(r->nodes[i].currentWeight||r->nodes[i].goalWeight)&&!cod1LocoResources.metadata[i])return 0;
  if(r->notify->flags[i].token){
   if(r->notify->flags[i].token!=i+1||!r->notify->generation[i]||!ActorGenericName(r,i)[0]||
      leaf>=C1GA_COUNT||!leaf||!cod1LocoResources.metadata[leaf]||
      r->notify->flags[i].cursor<-1||r->notify->flags[i].cursor>=(int)cod1LocoResources.metadata[leaf]->count||
      (cod1GenericDefinitions[i].children&&(cod1GenericDefinitions[leaf].children||!C1AGT14_Descendant(cod1GenericDefinitions,leaf,i))))return 0;
  }
 }
 return 1;
}
static int ActorLocomotionMutate(C1GARecord*r,const GscActorAnimationRequest*q){
 unsigned n=q->animation[0].ordinal,i,token=0,stop=0;float f[3];int named=0,ok=0;const char*end=NULL;
 if(!ActorLocomotionReady(cod1EntityEpoch)||!ActorScriptedClockSelected(r->lease.actorIndex)||!ActorGenericBinding(q->animation,q->runtime_identity)||
    !r->notify||ActorGenericNamedPending(r)||r->notify->serial==UINT64_MAX)return 0;
 memcpy(f,q->scalar_bits,12);
 named=q->method==GSC_ACTOR_ANIMATION_FLAGGED_ALL||q->method==GSC_ACTOR_ANIMATION_FLAGGED_ALL_RESTART||
       q->method==GSC_ACTOR_ANIMATION_FLAGGED_KNOB||q->method==GSC_ACTOR_ANIMATION_FLAGGED_RESTART;
 if(named){end=memchr(q->notify,0,sizeof(q->notify));if(!end||end==q->notify)return 0;token=n+1;}
 else if(q->notify[0])return 0;
 if(q->method==GSC_ACTOR_ANIMATION_KNOB_ALL||q->method==GSC_ACTOR_ANIMATION_KNOB_ALL_RESTART||
    q->method==GSC_ACTOR_ANIMATION_FLAGGED_ALL||q->method==GSC_ACTOR_ANIMATION_FLAGGED_ALL_RESTART){
  if(!ActorGenericBinding(q->animation+1,q->runtime_identity))return 0;stop=q->animation[1].ordinal;
  ok=C1AGFL14_Select(cod1GenericDefinitions,r->nodes,r->notify->flags,&cod1LocoResources.topology,cod1LocoResources.metadata,n,stop,token,f[0],f[1],f[2],
       q->method==GSC_ACTOR_ANIMATION_KNOB_ALL_RESTART||q->method==GSC_ACTOR_ANIMATION_FLAGGED_ALL_RESTART);
 }else if(q->method==GSC_ACTOR_ANIMATION_SET||q->method==GSC_ACTOR_ANIMATION_KNOB||
          q->method==GSC_ACTOR_ANIMATION_FLAGGED_KNOB||q->method==GSC_ACTOR_ANIMATION_FLAGGED_RESTART){
  ok=C1AGFL14_Set(cod1GenericDefinitions,r->nodes,r->notify->flags,&cod1LocoResources.topology,cod1LocoResources.metadata,n,token,f[0],f[1],f[2],
      q->method==GSC_ACTOR_ANIMATION_FLAGGED_RESTART,q->method!=GSC_ACTOR_ANIMATION_SET);
 }else if(q->method==GSC_ACTOR_ANIMATION_CLEAR){
  ok=C1AGT14_Clear(cod1GenericDefinitions,r->nodes,C1GA_COUNT,n,f[0]);
  if(ok)for(i=0;i<C1GA_COUNT;i++)if(C1AGT14_Descendant(cod1GenericDefinitions,i,n)){r->notify->flags[i].token=0;r->notify->flags[i].cursor=-1;}
 }
 if(!ok)return 0;
 if(!ActorGenericClearUnnamed(r))return 0;
 if(named){if(!ActorGenericNameSet(r,n,q->notify,(size_t)(end-q->notify)))return 0;r->notify->generation[n]=++r->notify->serial;r->notify->pending[n]=0;r->notify->pendingTick[n]=0;}
 return ActorLocomotionClosure(r);
}
static int ActorLocomotionFrameAdvance(C1GARecord*r,C1FaceFrame*f,float dt){
 c1agc14Step_t*step;unsigned i;int ok=0;
 if(!ActorLocomotionClosure(r))return 0;step=malloc(sizeof(*step));if(!step)return 0;
 if(!C1AGC14_AdvanceDisposable(cod1GenericDefinitions,r->nodes,r->notify->flags,&cod1LocoResources.topology,cod1LocoResources.metadata,
                    cod1LocoResources.frequency,dt,1,1,step)||step->count>C1ANC14_MAX_NOTES)goto done;
 for(i=0;i<step->count;i++){
  const c1agc14Event_t*e=step->event+i;cod1ActorFaceNote_t*q=f->notes+i;const c1anm14Metadata_t*m;
  if(e->node>=C1GA_COUNT||e->notify!=e->node+1||!r->notify->generation[e->node]||!ActorGenericName(r,e->node)[0])goto done;
  memset(q,0,sizeof(*q));q->entity=r->lease.entity;q->runtime=r->runtime;q->catalog=r->catalog;q->owner_identity=r->notify->ownerIdentity;
  q->generation=r->notify->generation[e->node];q->frame_generation=f->frame;q->ordinal=e->node;q->produced_ms=f->tick;
  memcpy(q->event,ActorGenericName(r,e->node),64);
  if(!e->leaf)strcpy(q->payload,"end");
  else{if(e->leaf>=C1GA_COUNT||!(m=cod1LocoResources.metadata[e->leaf])||!e->payload||e->payload>m->count)goto done;memcpy(q->payload,m->payload[e->payload-1],128);}
 }
 f->count=step->count;f->done=!step->resume;if(f->done&&!f->count)f->open=0;ok=1;
done:free(step);return ok;
}

#include "../../qcommon/cod1_actor_general_root_interval14.h"

/* ActorLocomotionLoad published the immutable child lists and source root
 * bank together. Only children with positive current weight contribute to
 * ENGINE112; a retired child can still retain an earlier phase interval. */
static int MotionActiveRootNodes(const c1atl14Node_t *nodes,unsigned ordinal,
                                 int parent,unsigned depth,c1agr14Projection_t *out){
    const c1agf14Topology_t *tree=&cod1LocoResources.topology;
    const c1atl14Node_t *live=nodes+ordinal;
    unsigned child,here;
    c1ari14Node_t *copy;
    if(depth>32 || out->count>=sizeof(out->nodes)/sizeof(out->nodes[0]))return 0;
    here=out->count++;
    copy=out->nodes+here;
    copy->ordinal=ordinal;
    copy->parent=parent;
    copy->leaf=-1;
    copy->currentWeight=live->currentWeight;
    copy->goalWeight=live->goalWeight;
    copy->oldPhase=live->oldPhase;
    copy->phase=live->phase;
    if(tree->first[ordinal]==C1AGF14_NONE){
        const c1ar14Root_t *root=cod1LocoResources.leaf[ordinal].root;
        if(!root || out->leaves>=sizeof(out->roots)/sizeof(out->roots[0]))return 0;
        copy->leaf=(int)out->leaves;
        out->roots[out->leaves++]=root;
    }else{
        for(child=tree->first[ordinal];child!=C1AGF14_NONE;child=tree->next[child]){
            if(nodes[child].currentWeight>0 &&
               !MotionActiveRootNodes(nodes,child,(int)here,depth+1,out))return 0;
        }
    }
    return 1;
}

static int MotionWireExitDistance(const cod1ActorLease_t *lease,int now,float *step){
    const C1GARecord *actor=cod1GenericActors+lease->actorIndex;
    c1agr14Projection_t active;
    c1ari14Result_t root;
    if(!actor->used || !actor->nodes || actor->tick!=now ||
       memcmp(&actor->lease,lease,sizeof(*lease)) || !ActorLocomotionReady(cod1EntityEpoch))return 0;
    active.count=active.leaves=0;
    if(!MotionActiveRootNodes(actor->nodes,0,-1,0,&active) ||
       !C1ARI14_Interval(active.roots,active.leaves,active.nodes,active.count,0,&root))return 0;
    *step=hypotf(root.translation[0],root.translation[1]);
    return 1;
}
