/* Allocation-owned lifecycle; include after cod1ActorRecord_t and registry helpers.
 * Forward
 * declarations before registry callers. Proposed member: cod1ActorScriptedAux_t scriptedAux.
 * FreshCall global is defined by the later host include; cancel helper is forward.
 * To keep all existing globals private, root must add FreshStoragePrivate to
 * G_Cod1EntityStorageProtected (pure, no recursive ActorGoalBuffers call). */
static void ActorNormalRetire(uint32_t,const GscEntityRef*);
static void ActorNormalReset(void);
static void FreshOperationCancelOwner(uint32_t,const GscEntityRef*);
static void FreshOperationReset(void);
static int FreshAuxConstruct(cod1ActorRecord_t*a,const cod1ActorLease_t*l){
 cod1ActorScriptedAux_t n;
 if(!a||!l||!l->serial||l->kind!=COD1_ACTOR_LEASE_ACTOR||
    a->serial!=l->serial||!ActorRefEqual(&a->owner,&l->entity))return 0;
 memset(&n,0,sizeof(n));n.actorSerial=l->serial;n.revision=1;
 /* All current portable allocations construct this exclusive representation.
  * Native raw birth-zero equivalence is not claimed. A future pre-Actor link
  * producer/restore must transfer ownership here or explicitly retain UNKNOWN. */
 n.record=FRESH_AUX_EMPTY;n.link=FRESH_LINK_UNATTACHED;
 a->scriptedAux=n;return 1;
}
static void FreshAuxRetire(uint32_t i,const GscEntityRef*owner){
 cod1ActorScriptedAux_t*a;
 if(!owner||i>=COD1_ACTOR_CAPACITY||!ActorRefEqual(owner,&cod1Actors[i].owner))return;
 FreshOperationCancelOwner(i,owner);ActorNormalRetire(i,owner);a=&cod1Actors[i].scriptedAux;
 if(a->revision!=UINT64_MAX)++a->revision;
 a->prefix=FRESH_PREFIX_INVALID;a->record=FRESH_AUX_UNKNOWN;a->link=FRESH_LINK_UNKNOWN;
 /* ActorNormalRetire above releases the actual selected heap before aux
  * invalidation. Native root-link/restore producers remain separately held. */
}
static void FreshAuxReset(void){
 /* ActorPoolReset then clears real Actor array. Poison in-flight operation
  * FIRST and preserve its busy/stage pointer lease until unwind. */
 FreshOperationReset();ActorNormalReset();
}
