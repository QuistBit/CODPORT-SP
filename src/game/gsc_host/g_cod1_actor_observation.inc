/* Private storage bodies. They prove actual storage identities, never the
 * authority to execute Think4/Think9 or a source/current transition. A future
 * native caller needs its own exact owner/current fences before invoking the
 * write body. No source-ready/visibility/target selection result is supplied.
 */
static int ActorPerceptionMemberLease(const cod1SentientRecord_t*candidate,
 cod1ActorLease_t*out){
 unsigned i;cod1ActorLease_t q;
 if(!candidate||!out)return 0;
 for(i=0;i<COD1_SENTIENT_CAPACITY;i++)if(candidate==cod1Sentients+i)break;
 if(i==COD1_SENTIENT_CAPACITY)return 0;
 /* Only now read the physically established member. */
 memset(&q,0,sizeof(q));q.entity=candidate->owner;q.serial=candidate->serial;
 q.sentientIndex=i;q.actorIndex=candidate->actorIndex;q.kind=candidate->kind;
 if(!ActorLeaseValid(&q))return 0;*out=q;return 1;
}
static int ActorPerceptionPoolRead(c1per14ScanCell_t*out){
 unsigned i;cod1ActorLease_t q;c1per14ScanCell_t cells[C1PER14_SLOTS];
 if(!out||COD1_SENTIENT_CAPACITY!=C1PER14_SLOTS||!ActorMapAvailable())return 0;
 for(i=0;i<C1PER14_SLOTS;i++){
  const cod1SentientRecord_t*s=cod1Sentients+i;
  if(s->activation>=0&&!ActorPerceptionMemberLease(s,&q))return 0;
  cells[i].activation=s->activation;cells[i].team=s->defaults.team;
 }
 memcpy(out,cells,sizeof(cells));return 1;
}
/* -2 invalid physical owner/pool, -1 genuine exhausted scan, >=0 actual
 * member index. Actor and player sentients are both represented. No filters
 * based on health, enemyKnown, busy or task count are introduced. */
static int ActorPerceptionFirstBody(uint32_t mask,cod1ActorLease_t*out){
 c1per14ScanCell_t cells[C1PER14_SLOTS];int index;cod1ActorLease_t q;
 if(!out||!ActorPerceptionPoolRead(cells))return C1PER14_INVALID;
 index=C1PER14_Scan(cells,0,mask);
 if(index>=0){if(!ActorPerceptionMemberLease(cod1Sentients+index,&q))return C1PER14_INVALID;*out=q;}
 return index;
}
static int ActorPerceptionNextBody(const cod1SentientRecord_t*prior,
 const cod1ActorLease_t*priorIdentity,uint32_t mask,cod1ActorLease_t*out){
 c1per14ScanCell_t cells[C1PER14_SLOTS];cod1ActorLease_t q;int index;
 if(!priorIdentity||!out||!ActorPerceptionMemberLease(prior,&q)||
    memcmp(&q,priorIdentity,sizeof(q))||!ActorPerceptionPoolRead(cells))return C1PER14_INVALID;
 index=C1PER14_Scan(cells,q.sentientIndex+1,mask);
 if(index>=0){if(!ActorPerceptionMemberLease(cod1Sentients+index,&q))return C1PER14_INVALID;*out=q;}
 return index;
}
/* Existing13590 publishes native entity E0 into r.currentOrigin; the public
 * EntityGetPose reads the same current pose, and player publication shares it.
 * No Actor-only NativeData projection or trajectory is used here. */
static int ActorObservationCurrentPose(const GscEntityRef*ref,uint32_t bits[3]){
 if(!ref||!bits||!G_Cod1EntitySameAllocation(ref))return 0;
 memcpy(bits,g_entities[ref->slot].r.currentOrigin,12);return 1;
}
/* Native row provenance is separate from the 0x40 bytes. If a target slot is
 * reused, preserve its historical row and refuse until native4D9D0/reuse
 * clearing is implemented. Neither zeroing nor silently rebinding stale
 * history is justified by the current source contract. */
static int ActorObservationWriteBody(const cod1ActorLease_t*observer,
 const cod1SentientRecord_t*target,const cod1ActorLease_t*targetIdentity){
 cod1ActorLease_t observed;c1per14Observation_t input;cod1ActorRecord_t*a;
 Cod1ObservationTarget*binding;
 if(!observer||!targetIdentity||!ActorLeaseValid(observer)||
    observer->kind!=COD1_ACTOR_LEASE_ACTOR||level.time<0||
    !ActorPerceptionMemberLease(target,&observed)||
    memcmp(&observed,targetIdentity,sizeof(observed)))return 0;
 a=cod1Actors+observer->actorIndex;binding=a->observationTargets+observed.sentientIndex;
 if(binding->serial&&(binding->serial!=observed.serial||!ActorRefEqual(&binding->owner,&observed.entity)))return 0;
 memset(&input,0,sizeof(input));input.time=(uint32_t)level.time;
 /* Copy actual entity E0 bits, including unchanged bit patterns. */
 if(!ActorObservationCurrentPose(&observed.entity,input.targetOrigin)||
    !ActorObservationCurrentPose(&observer->entity,input.observerOrigin))return 0;
 input.sentient54=target->observation54;input.sentient58=target->observation58;
 /* All checks precede actual stores; no callback or failure below. */
 C1PER14_Observe(&a->observations.row[observed.sentientIndex],&input);
 binding->serial=observed.serial;binding->owner=observed.entity;
 return 1; /* Port storage-operation result, not native Think return. */
}

/* Native6AD50 initial4FD10 cohort, BEFORE4D7E0/aitype. Only the private
 * projected Session prepare caller below may select this fresh-owner phase.
 * Stage all rows first: invalid late pool/identity cannot partially seed it. */
static int ActorObservationSeedInitial(const cod1ActorLease_t*observer){
 cod1ActorRecord_t*a;c1per14ScanCell_t cells[C1PER14_SLOTS];
 cod1ActorLease_t leases[C1PER14_SLOTS];c1per14Observation_t rows[C1PER14_SLOTS];
 uint32_t origin[3];unsigned i,count=0,indices[C1PER14_SLOTS];int now=level.time;
 if(!observer||!ActorLeaseValid(observer)||observer->kind!=COD1_ACTOR_LEASE_ACTOR||
    now<0||level.spawning||level.restarted||!cod1EntityReady||!ActorMapAvailable())return 0;
 a=cod1Actors+observer->actorIndex;
 if(!a->alive||a->initialized||!a->dataInitialized||!a->nativeDataInitialized||
    a->positionPostinit||a->numericTargetApplied||!a->spatial.linked||a->spatial.busy||
    !ActorPerceptionPoolRead(cells)||!ActorObservationCurrentPose(&observer->entity,origin))return 0;
 for(i=0;i<C1PER14_SLOTS;i++)if(cells[i].activation>=0){
  cod1ActorLease_t q;c1per14Observation_t in;Cod1ObservationTarget*binding;
  if(!ActorPerceptionMemberLease(cod1Sentients+i,&q))return 0;
  binding=a->observationTargets+i;
  if(binding->serial&&(binding->serial!=q.serial||!ActorRefEqual(&binding->owner,&q.entity)))return 0;
  memset(&in,0,sizeof(in));in.time=(uint32_t)now;memcpy(in.observerOrigin,origin,12);
  if(!ActorObservationCurrentPose(&q.entity,in.targetOrigin))return 0;
  in.sentient54=cod1Sentients[i].observation54;in.sentient58=cod1Sentients[i].observation58;
  indices[count]=i;leases[count]=q;rows[count]=in;++count;
 }
 /* No external calls or remaining refusals: all active physical slots,
  * ascending, including the new observer itself. No health/team filters. */
 for(i=0;i<count;i++){
  unsigned index=indices[i];C1PER14_Observe(a->observations.row+index,rows+i);
  a->observationTargets[index].serial=leases[i].serial;
  a->observationTargets[index].owner=leases[i].entity;
 }
 return 1; /* Port storage commit, no native AI/Think success is fabricated. */
}

/* 6AF28..6AFC5: actual Actor pool32, after original postinit, not the
 * Sentient First/Next pool used by the earlier initial observations.
 * Only the private dynamic returned-INIT owner selects this body. */
static int ActorObservationInheritPeers(const cod1ActorLease_t*observer){
 cod1ActorRecord_t*a;c1per14Memory_t staged;Cod1ObservationTarget bindings[C1PER14_SLOTS];
 uint32_t mask;unsigned i,j;
 if(!observer||!ActorLeaseValid(observer)||observer->kind!=COD1_ACTOR_LEASE_ACTOR||
    !ActorMapAvailable()||!cod1EntityReady||level.spawning||level.restarted||level.time<0)return 0;
 a=cod1Actors+observer->actorIndex;
 mask=UINT32_C(1)<<(cod1Sentients[observer->sentientIndex].defaults.team&31u);
 staged=a->observations;memcpy(bindings,a->observationTargets,sizeof(bindings));
 for(i=0;i<COD1_ACTOR_CAPACITY;i++)if(cod1Actors[i].activation>=0){
  const cod1ActorRecord_t*peer=cod1Actors+i;cod1ActorLease_t identity;
  if(peer->sentientIndex>=COD1_SENTIENT_CAPACITY||
     !ActorPerceptionMemberLease(cod1Sentients+peer->sentientIndex,&identity)||
     identity.kind!=COD1_ACTOR_LEASE_ACTOR||identity.actorIndex!=i||
     identity.serial!=peer->serial||!ActorRefEqual(&identity.entity,&peer->owner))return 0;
  if(i==observer->actorIndex)continue;
  if(!(mask&(UINT32_C(1)<<(cod1Sentients[identity.sentientIndex].defaults.team&31u))))continue;
  for(j=0;j<C1PER14_SLOTS;j++){
   int32_t before,after;const Cod1ObservationTarget*binding=peer->observationTargets+j;
   memcpy(&before,staged.row[j].word+0x18/4,4);memcpy(&after,peer->observations.row[j].word+0x18/4,4);
   if(before<after){cod1ActorLease_t target;
    /* Never invent a current binding from row index. General inactive/reused
     * target reconciliation4D9D0 and unowned restore/history remain OPEN. */
    if(!binding->serial||!ActorPerceptionMemberLease(cod1Sentients+j,&target)||
       target.serial!=binding->serial||!ActorRefEqual(&target.entity,&binding->owner))return 0;
    C1PER14_InheritRow(staged.row+j,peer->observations.row+j);bindings[j]=*binding;
   }
  }
 }
 /* No external calls/failures after staging the entire ordered cohort. */
 a->observations=staged;memcpy(a->observationTargets,bindings,sizeof(bindings));return 1;
}

/* Original6AA70/6BA60 supported branches, selected ONLY by the private
 * returned-INIT caller. Completion here means these native paths returned;
 * it does not authorize Think, spawned, count or a full combat AI result. */
extern vmCvar_t g_cod1AIDebugThreat;
static int ActorPerceptionTailTeam(const cod1ActorLease_t*l,unsigned*team){
 if(!l||!team||!ActorLeaseValid(l)||l->kind!=COD1_ACTOR_LEASE_ACTOR||
    !ActorMapAvailable()||!cod1EntityReady||level.spawning||level.restarted||level.time<0)return 0;
 *team=cod1Sentients[l->sentientIndex].defaults.team;return *team<=4;
}
static int ActorSpawnNative6AA70(const cod1ActorLease_t*l,char*error,size_t cap){
 static const unsigned opponent[5]={0,2,1,0,0};unsigned team;cod1ActorLease_t target;int index;
 if(!ActorPerceptionTailTeam(l,&team))return 0;
 if(!opponent[team])return 1; /* Original direct exit, before any pool/F0 read. */
 index=ActorPerceptionFirstBody(UINT32_C(1)<<opponent[team],&target);
 if(index!=C1PER14_NONE){EntityError(error,cap,index==C1PER14_INVALID?
  "spawn native6AA70 requires its complete current Sentient pool":
  "spawn native6AA70 hostile perception6A2B0 remains unsupported");return 0;}
 {volatile uint32_t count=cod1Actors[l->actorIndex].nativeVisibilityQueriesF0;(void)count;}
 return 1; /* Genuine exhausted native scan; no persistent store or callback. */
}
static int ActorSpawnNative6BA60(const cod1ActorLease_t*l,char*error,size_t cap){
 static const unsigned opponent[5]={0,2,1,0,0};unsigned team;cod1ActorLease_t target;
 const cod1SentientRecord_t*s;int index;
 if(!ActorPerceptionTailTeam(l,&team))return 0;
 if(!opponent[team])return 1; /* Original direct exit precedes debug/enemy/timer. */
 /* Native WORD entity number is represented by the actual allocation slot.
  * This is the real GAME cached vmCvar, not a fabricated debug sentinel or
  * an immediate query that could differ from the per-frame native snapshot.
  * Matching debug-log branch remains explicit HOLD (including developer0). */
 if(g_cod1AIDebugThreat.integer==(int)l->entity.slot){
  EntityError(error,cap,"spawn native6BA60 selected debug log remains unsupported");return 0;}
 index=ActorPerceptionFirstBody(UINT32_C(1)<<opponent[team],&target);
 if(index!=C1PER14_NONE){EntityError(error,cap,index==C1PER14_INVALID?
  "spawn native6BA60 requires its complete current Sentient pool":
  "spawn native6BA60 hostile ranking6B6B0 remains unsupported");return 0;}
 s=cod1Sentients+l->sentientIndex;
 if(!s->enemyKnown||s->enemy.epoch||s->enemy.slot||s->enemy.generation){
  EntityError(error,cap,"spawn native6BA60 existing enemy transition remains unsupported");return 0;}
 if(s->nativeEnemyDeadline38&&level.time>=s->nativeEnemyDeadline38){
  EntityError(error,cap,"spawn native6BA60 expired enemy deadline event4 remains unsupported");return 0;}
 /* Candidate=NULL equals canonical enemyNULL; native7C8 is unchanged.
  * Zero/future deadline has no event4 or deadline clear; native returns. */
 return 1;
}
