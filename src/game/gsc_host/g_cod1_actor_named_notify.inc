/* R233 owned name/state foundation. No script command is admitted here and
 * no completion event is fabricated. The enclosing record owns lease, runtime,
 * catalog, revision and staged commit; this sidecar follows that ownership. */
static int ActorGenericNamedActive(const C1GARecord*r){unsigned i;if(!r->notify)return 0;for(i=0;i<C1GA_COUNT;i++)if(r->notify->flags[i].token)return 1;return 0;}
static void ActorGenericNamedCancelSubtree(C1GARecord*r,unsigned root){unsigned i;if(!r->notify)return;for(i=0;i<C1GA_COUNT;i++)if(C1AGT14_Descendant(cod1GenericDefinitions,i,root)){
 memset(r->notify->flags+i,0,sizeof(r->notify->flags[i]));r->notify->flags[i].cursor=-1;(void)ActorGenericNameSet(r,i,NULL,0);r->notify->generation[i]=0;r->notify->pending[i]=0;r->notify->pendingTick[i]=0;
}}
static int ActorGenericNamedLeafStage(C1GARecord*stage,void*user){
 const GscActorAnimationRequest*q=user;unsigned i,n=q->animation[0].ordinal;const char*end;float f[3];
 if(C1AL14_Index(q->animation[0].name)<0&&ActorGenericShuffleIndex(q->animation[0].name)<0)return 0;
 if(!stage->notify||stage->notify->serial==UINT64_MAX||!ActorGenericBinding(q->animation,q->runtime_identity)||
    !ActorGenericBinding(q->animation+1,q->runtime_identity)||strcmp(q->animation[0].tree,q->animation[1].tree)||
    !(q->animation[1].flags&GSC_ANIMATION_NODE_BRANCH)||
    !(end=memchr(q->notify,0,sizeof(q->notify)))||end==q->notify)return 0;
 memcpy(f,q->scalar_bits,sizeof(f));
 if(!C1AFL14_Select(cod1GenericDefinitions,stage->nodes,stage->notify->flags,C1GA_COUNT,n,q->animation[1].ordinal,n+1,f[0],f[1],f[2],q->method==GSC_ACTOR_ANIMATION_FLAGGED_ALL_RESTART))return 0;
 for(i=0;i<C1GA_COUNT;i++)if(!stage->notify->flags[i].token){(void)ActorGenericNameSet(stage,i,NULL,0);stage->notify->generation[i]=0;stage->notify->pending[i]=0;stage->notify->pendingTick[i]=0;}
 if(!ActorGenericNameSet(stage,n,q->notify,(size_t)(end-q->notify)))return 0;
 stage->notify->pending[n]=0;stage->notify->pendingTick[n]=0;stage->notify->generation[n]=++stage->notify->serial;return 1;
}

static int ActorGenericNamedPending(const C1GARecord*r){unsigned i;if(!r->notify)return 0;for(i=0;i<C1GA_COUNT;i++)if(r->notify->pending[i])return 1;return 0;}
static int ActorGenericNamedAdvance(C1GARecord*r,unsigned ordinal,const c1ar14Root_t*root,float dt,int now){
 c1atl14Node_t next;unsigned event=0;int ok;float frequency;
 if(!r->notify||ordinal>=C1GA_COUNT||!root||root->noteCount||root->intervals>4095||!root->fps||root->fps>120||r->notify->pending[ordinal]||!r->notify->generation[ordinal]||!ActorGenericName(r,ordinal)[0])return 0;
 next=r->nodes[ordinal];
#if defined(_MSC_VER)&&defined(_M_IX86)
 {unsigned char saved[108];uint32_t mx,nearest=0x1f80;const unsigned short cw=0x027f;
 __asm fnsave saved
 __asm stmxcsr mx
 __asm fldcw cw
 __asm ldmxcsr nearest
 frequency=root->intervals?(float)((double)root->fps/root->intervals):0;
 ok=C1AFC14_Advance(&next,r->notify->flags+ordinal,frequency,root->flags&1,dt,1,1,&event);
 __asm ldmxcsr mx
 __asm frstor saved
 }
#else
 frequency=root->intervals?(float)((double)root->fps/root->intervals):0;
 ok=C1AFC14_Advance(&next,r->notify->flags+ordinal,frequency,root->flags&1,dt,1,1,&event);
#endif
 if(!ok)return 0;r->nodes[ordinal]=next;if(event){r->notify->pending[ordinal]=r->notify->generation[ordinal];r->notify->pendingTick[ordinal]=now;}return 1;
}
static int ActorGenericNotifyOwnerCurrent(C1GARecord*r,const GscEntityRef*owner,const cod1ActorLease_t*lease,uint64_t epoch,uint64_t identity,uint64_t revision,int now){
 cod1ActorLease_t after;return !cod1GenericOperation.cancelled&&now==level.time&&epoch==cod1GenericEpoch&&epoch==cod1EntityEpoch&&!level.spawning&&!level.restarted&&cod1EntityReady&&ActorGoalLease(owner,&after)&&!memcmp(lease,&after,sizeof(after))&&r->used&&r->notify&&r->notify->ownerIdentity==identity&&r->revision==revision&&cod1GenericReceipt==cod1TurretAnimationReceipt&&cod1GenericLookEpoch==epoch&&C1AUT14_Ready(&cod1GenericUnmountedBank);
}
int G_Cod1ActorGenericNotifyDrain(const GscEntityRef*input,int now,int(*current)(void*),cod1ActorAnimationNotifyCallback deliver,void*ctx){
 GscEntityRef owner;cod1ActorLease_t lease;C1GARecord*r=NULL;cod1ActorAnimationNotify_t packet,copy;uint64_t epoch,identity,revision;unsigned i;int accepted,ok=0;
 if(!input||!current||!deliver||now!=level.time||cod1GenericOperation.busy||!ActorGoalBuffers(input,sizeof(*input),NULL,0,NULL,0)||ActorGenericPrivate(input,sizeof(*input)))return 0;
 owner=*input;if(!ActorGoalLease(&owner,&lease))return 0;r=cod1GenericActors+lease.actorIndex;
 if(!r->used)return 1;if(!r->notify||r->tick>now||memcmp(&r->lease,&lease,sizeof(lease)))return 0;
 epoch=cod1GenericEpoch;identity=r->notify->ownerIdentity;revision=r->revision;cod1GenericOperation.busy=1;cod1GenericOperation.cancelled=0;
 if(current(ctx)!=1||memcmp(input,&owner,sizeof(owner))||!ActorGenericNotifyOwnerCurrent(r,&owner,&lease,epoch,identity,revision,now))goto done;
 for(i=1;i<C1GA_COUNT;i++)if(r->notify->pending[i]){
  if(r->notify->pending[i]!=r->notify->generation[i]||r->notify->flags[i].token!=i+1||!ActorGenericName(r,i)[0]||r->notify->pendingTick[i]>now)goto done;
  memset(&packet,0,sizeof(packet));packet.entity=owner;packet.runtime=r->runtime;packet.catalog=r->catalog;packet.owner_identity=identity;packet.generation=r->notify->pending[i];packet.ordinal=i;packet.produced_ms=r->notify->pendingTick[i];memcpy(packet.event,ActorGenericName(r,i),64);strcpy(packet.payload,"end");copy=packet;
  if(current(ctx)!=1||memcmp(input,&owner,sizeof(owner))||!ActorGenericNotifyOwnerCurrent(r,&owner,&lease,epoch,identity,revision,now))goto done;
  accepted=deliver(ctx,&packet);
  if(!ActorGenericNotifyOwnerCurrent(r,&owner,&lease,epoch,identity,revision,now)||memcmp(&packet,&copy,sizeof(packet))||memcmp(input,&owner,sizeof(owner)))goto done;
  if(accepted!=1)goto done;
  r->notify->pending[i]=0;r->notify->pendingTick[i]=0;
  if(current(ctx)!=1||memcmp(input,&owner,sizeof(owner))||!ActorGenericNotifyOwnerCurrent(r,&owner,&lease,epoch,identity,revision,now))goto done;
 }
 ok=1;
 done:cod1GenericOperation.busy=0;return ok;
}
