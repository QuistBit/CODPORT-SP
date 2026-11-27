/* Authored-record ownership for the actual BSP allocation. This receipt is
 * DATA, not an Actor lease or execution certificate. Construction still uses
 * the existing complete-cohort service and its fresh native certificate. */
enum { COD1_RECORD_ALLOCATED=1,COD1_RECORD_BOUND,COD1_RECORD_PLACED,
       COD1_RECORD_SPAWNER,COD1_RECORD_FILTERED,COD1_RECORD_SPAWNER_BOUND,
       COD1_RECORD_CONSUMED,COD1_RECORD_BSP_CONSTRUCTED };
typedef struct {
 uint64_t epoch,receipt,allocation,program;
 uint32_t ordinal,index;GscEntityRef entity;int state;
} Cod1ActorRecordOwner;
static Cod1ActorRecordOwner cod1ActorRecordOwners[COD1_ACTOR_RECORD_CAPACITY];
static uint64_t cod1ActorRecordSerial;
static uint32_t cod1ActorRecordActive;
static int cod1ActorRecordBusy;
static int Cod1ActorRecordPrivate(const void*p,size_t n){
 return Cod1AiOverlap(p,n,cod1ActorRecordOwners,sizeof(cod1ActorRecordOwners))||
  Cod1AiOverlap(p,n,&cod1ActorRecordSerial,sizeof(cod1ActorRecordSerial))||
  Cod1AiOverlap(p,n,&cod1ActorRecordActive,sizeof(cod1ActorRecordActive))||
  Cod1AiOverlap(p,n,&cod1ActorRecordBusy,sizeof(cod1ActorRecordBusy));
}
static void Cod1ActorRecordReset(void){
 memset(cod1ActorRecordOwners,0,sizeof(cod1ActorRecordOwners));cod1ActorRecordActive=0;
 /* Serial is process lifetime. An interrupted callback must still fail its
  * captured epoch/receipt check after a new map resets these records. */
}
static int Cod1ActorRecordRefuse(const char*text){
 cod1ActorPrefix.invalid=1;return Cod1ActorPrefixFail(NULL,0,text);
}
static int Cod1ActorRecordCompleted(uint32_t count){uint32_t i;
 if(cod1ActorRecordActive||cod1ActorRecordBusy)return 0;
 if(count>COD1_ACTOR_RECORD_CAPACITY)return 0;
 for(i=0;i<count;i++){
  int state=cod1ActorRecordOwners[i].state;
  if(state!=COD1_RECORD_PLACED&&state!=COD1_RECORD_SPAWNER&&state!=COD1_RECORD_FILTERED&&state!=COD1_RECORD_BSP_CONSTRUCTED)return 0;
 }
 return 1;
}
static int Cod1ActorRecordPreviousComplete(void){return Cod1ActorRecordCompleted(cod1ActorPrepassRecords.actors);}
static int Cod1ActorRecordBytesCurrent(uint32_t i){const Cod1ActorPrepassRecord*r;
 if(i>=cod1ActorPrefix.prepassRows)return 0;r=&cod1ActorPrepassRecords.rows[i];
 return r->ordinal==cod1ActorPrepassRecords.seen&&r->pairs==level.numSpawnVars&&
  r->bytes==level.numSpawnVarChars&&r->bytes>0&&r->bytes<=MAX_SPAWN_VARS_CHARS&&
  !memcmp(r->text,level.spawnVarChars,(size_t)r->bytes);
}
static int Cod1ActorRecordAllocationBegin(gentity_t*ent){
 GscEntityRef ref;uint64_t epoch,receipt,program;uint32_t index;char*name;Cod1ActorRecordOwner*r;
 if(!cod1ActorPrefix.enabled)return 1;
 if(!Cod1ActorPrefixOwner()||cod1ActorPrefix.invalid||cod1ActorPrefix.closed||
    !level.spawning||level.restarted||!cod1ActorPrefix.prepassReady||cod1ActorPrefix.sealed||
    cod1ActorPrepassRecords.complete||cod1ActorRecordActive||cod1ActorRecordBusy)
  return Cod1ActorRecordRefuse("Actor allocation is outside its fresh admitted BSP record");
 G_SpawnString("classname","",&name);
 if(Q_stricmpn(name,"actor_",6))return Cod1ActorRecordPreviousComplete()?1:
  Cod1ActorRecordRefuse("non-Actor allocation followed an incomplete Actor record");
 if(!cod1ActorPrepassRecords.actors||cod1ActorPrepassRecords.actors>COD1_ACTOR_RECORD_CAPACITY||
    !Cod1ActorRecordCompleted(cod1ActorPrepassRecords.actors-1)||cod1ActorRecordSerial==UINT64_MAX)
  return Cod1ActorRecordRefuse("Actor allocation has no admitted record or receipt capacity");
 index=cod1ActorPrepassRecords.actors-1;r=&cod1ActorRecordOwners[index];
 if(r->state||!Cod1ActorRecordBytesCurrent(index)||!G_Cod1EntityCapture(ent,&ref))
  return Cod1ActorRecordRefuse("Actor allocation differs from its exact admitted authored record");
 epoch=cod1ActorPrefix.epoch;receipt=cod1ActorPrefix.receipt;cod1ActorRecordBusy=1;
 program=G_Cod1GscTestEarlyProgramIdentity(epoch,receipt);cod1ActorRecordBusy=0;
 if(!program||epoch!=cod1ActorPrefix.epoch||receipt!=cod1ActorPrefix.receipt||
    !Cod1ActorPrefixOwner()||cod1ActorPrefix.invalid||cod1ActorPrefix.closed||
    !level.spawning||level.restarted||!Cod1ActorRecordBytesCurrent(index)||
    !G_Cod1EntitySameAllocation(&ref)||r->state||cod1ActorRecordActive)
  return Cod1ActorRecordRefuse("Actor allocation lost its early compiled program or map owner");
 r->epoch=epoch;r->receipt=receipt;r->allocation=++cod1ActorRecordSerial;r->program=program;
 r->index=index;r->ordinal=cod1ActorPrepassRecords.seen;r->entity=ref;r->state=COD1_RECORD_ALLOCATED;
 cod1ActorRecordActive=index+1;return 1;
}
static int Cod1ActorRecordIntakeCurrent(gentity_t*ent){GscEntityRef ref;Cod1ActorRecordOwner*r;
 if(!cod1ActorRecordActive||cod1ActorRecordActive>COD1_ACTOR_RECORD_CAPACITY||cod1ActorRecordBusy||
    !Cod1ActorPrefixOwner()||cod1ActorPrefix.invalid||cod1ActorPrefix.closed||!level.spawning||
    level.restarted||!G_Cod1EntityCapture(ent,&ref))
  return Cod1ActorRecordRefuse("Actor intake has no current one-record allocation owner");
 r=&cod1ActorRecordOwners[cod1ActorRecordActive-1];
 if(r->state!=COD1_RECORD_ALLOCATED||r->epoch!=cod1ActorPrefix.epoch||r->receipt!=cod1ActorPrefix.receipt||
    !r->allocation||!r->program||!Cod1AiRefEqual(&ref,&r->entity)||!Cod1ActorRecordBytesCurrent(r->index))
  return Cod1ActorRecordRefuse("Actor intake changed allocation, generation or authored BSP content");
 return 1;
}
static int Cod1ActorRecordBind(gentity_t*ent,cod1ActorBootstrapRow_t*row){Cod1ActorRecordOwner*r;
 if(!row||!Cod1ActorRecordIntakeCurrent(ent))return 0;r=&cod1ActorRecordOwners[cod1ActorRecordActive-1];
 if(!Cod1AiRefEqual(&row->entity,&r->entity)||(ent->spawnflags&1))
  return Cod1ActorRecordRefuse("placed Actor binding changed family or original allocation");
 row->bspOrdinal=r->ordinal;row->actorRecord=r->index;row->allocationReceipt=r->allocation;
 row->programIdentity=r->program;r->state=COD1_RECORD_BOUND;return 1;
}
static int Cod1ActorRecordBindSpawner(gentity_t*ent){
 if(!cod1ActorPrefix.enabled)return 1;
 if(!Cod1ActorRecordIntakeCurrent(ent)||!(ent->spawnflags&1))return 0;
 cod1ActorRecordOwners[cod1ActorRecordActive-1].state=COD1_RECORD_SPAWNER_BOUND;return 1;
}
/* outcome0 is reached only by actual authored exclusion branches;1 by a
 * successful class dispatch. Failure never becomes a reusable receipt. */
static int Cod1ActorRecordConstructBsp(Cod1ActorRecordOwner*);
static int Cod1ActorRecordAllocationEnd(gentity_t*ent,int outcome){
 GscEntityRef ref;Cod1ActorRecordOwner*r;
 if(!cod1ActorPrefix.enabled)return 1;
 if(!cod1ActorRecordActive){char*name;G_SpawnString("classname","",&name);
  return Q_stricmpn(name,"actor_",6)&&!cod1ActorPrefix.invalid?1:
   Cod1ActorRecordRefuse("Actor record has no active allocation outcome or was already closed");}
 if(cod1ActorRecordActive>COD1_ACTOR_RECORD_CAPACITY||cod1ActorRecordBusy||
    !Cod1ActorPrefixOwner()||cod1ActorPrefix.invalid||cod1ActorPrefix.closed||!level.spawning||
    level.restarted||!G_Cod1EntityCapture(ent,&ref))
  return Cod1ActorRecordRefuse("Actor record terminal outcome lost its actual allocation");
 r=&cod1ActorRecordOwners[cod1ActorRecordActive-1];
 if(r->epoch!=cod1ActorPrefix.epoch||r->receipt!=cod1ActorPrefix.receipt||
    !Cod1AiRefEqual(&ref,&r->entity)||!Cod1ActorRecordBytesCurrent(r->index))
  return Cod1ActorRecordRefuse("Actor record terminal outcome changed owner or authored data");
 if(outcome==0&&r->state==COD1_RECORD_ALLOCATED)r->state=COD1_RECORD_FILTERED;
 else if(outcome==1&&r->state==COD1_RECORD_BOUND&&!(ent->spawnflags&1))r->state=COD1_RECORD_PLACED;
 else if(outcome==1&&r->state==COD1_RECORD_SPAWNER_BOUND&&(ent->spawnflags&1))r->state=COD1_RECORD_SPAWNER;
 else return Cod1ActorRecordRefuse("Actor BSP record has no genuine terminal intake outcome");
 cod1ActorRecordActive=0;
 if(r->state==COD1_RECORD_PLACED&&G_Cod1GscFeature47(C1CP47_F_BSP_CONSTRUCT)==1)
  return Cod1ActorRecordConstructBsp(r);
 return 1;
}
static int Cod1ActorRecordConsume(cod1ActorBootstrapRow_t*row,const GscActorBootstrapServices*api,char*error,size_t size){
 Cod1ActorRecordOwner*r;uint64_t epoch,receipt,allocation,program;
 if(!row||!api||!api->program_identity||cod1ActorRecordBusy||cod1ActorRecordActive||
    row->actorRecord>=cod1ActorPrefix.prepassRows)goto refuse;
 r=&cod1ActorRecordOwners[row->actorRecord];epoch=cod1ActorPrefix.epoch;receipt=cod1ActorPrefix.receipt;allocation=r->allocation;
 if(r->state!=COD1_RECORD_PLACED||r->epoch!=epoch||r->receipt!=receipt||!allocation||
    r->allocation!=row->allocationReceipt||r->ordinal!=row->bspOrdinal||r->program!=row->programIdentity||
    !Cod1AiRefEqual(&r->entity,&row->entity)||!G_Cod1EntitySameAllocation(&row->entity))goto refuse;
 cod1ActorRecordBusy=1;program=api->program_identity(api->context);cod1ActorRecordBusy=0;
 if(!program||epoch!=cod1ActorPrefix.epoch||receipt!=cod1ActorPrefix.receipt||
    cod1ActorPrefix.invalid||r->state!=COD1_RECORD_PLACED||r->allocation!=allocation||
    program!=r->program||!Cod1AiRefEqual(&r->entity,&row->entity)||!G_Cod1EntitySameAllocation(&row->entity))goto refuse;
 /* One-shot before any native fields/reservation/constructor callbacks. A
  * failed transaction cannot retry against this record. */
 r->state=COD1_RECORD_CONSUMED;return 1;
refuse:
 Cod1ActorRecordRefuse("constructor refused stale, substituted or consumed BSP allocation receipt");
 return Cod1ActorPrefixFail(error,size,cod1ActorPrefix.error);
}

static int Cod1ActorRecordBspCurrent(void*u,const GscEntityRef*ref){
 const cod1ActorBspConstructionOwner_t*c=u;const Cod1ActorRecordOwner*r;
 if(!c||!ref||!level.spawning||level.restarted||!Cod1ActorPrefixOwner()||cod1ActorPrefix.invalid||cod1ActorPrefix.closed||
    c->epoch!=cod1ActorPrefix.epoch||c->receipt!=cod1ActorPrefix.receipt||!c->ordinal||!Cod1AiRefEqual(ref,&c->entity)||
    !G_Cod1EntitySameAllocation(ref))return 0;
 if(!cod1ActorPrepassRecords.actors)return 0;r=&cod1ActorRecordOwners[cod1ActorPrepassRecords.actors-1];
 return r->state==COD1_RECORD_CONSUMED&&r->epoch==c->epoch&&r->receipt==c->receipt&&r->allocation==c->allocation&&
  r->program==c->program&&r->ordinal==c->ordinal&&Cod1AiRefEqual(&r->entity,ref)&&Cod1ActorRecordBytesCurrent(r->index);
}
static int Cod1ActorRecordConstructBsp(Cod1ActorRecordOwner*r){
 cod1ActorBspConstructionOwner_t owner;cod1ActorPlacedEntryResult_t result;cod1ActorBootstrapRow_t*row;
 cod1ActorLeaseSnapshot_t actor;uint32_t index,i;int ok;char error[256];
 if(!r||r->state!=COD1_RECORD_PLACED||cod1ActorPrefixBusy||!level.spawning)return Cod1ActorRecordRefuse("BSP construction is outside its one-record terminal intake");
 index=r->index;for(i=0;i<cod1ActorPrefix.placedRows;i++)if(cod1ActorPrefix.rows[i].allocationReceipt==r->allocation)break;
 if(i==cod1ActorPrefix.placedRows)return Cod1ActorRecordRefuse("BSP constructor has no exact placed row");
 memset(&owner,0,sizeof(owner));owner.entity=r->entity;owner.epoch=r->epoch;owner.receipt=r->receipt;
 owner.allocation=r->allocation;owner.program=r->program;owner.ordinal=r->ordinal;owner.current=Cod1ActorRecordBspCurrent;owner.user=&owner;
 r->state=COD1_RECORD_CONSUMED;cod1ActorPrefixBusy=1;memset(&result,0,sizeof(result));memset(error,0,sizeof(error));
 ok=G_Cod1GscTestConstructBspActor(&owner,&result,error,sizeof(error));cod1ActorPrefixBusy=0;
 if(owner.epoch!=cod1ActorPrefix.epoch||owner.receipt!=cod1ActorPrefix.receipt)return 0;
 r=&cod1ActorRecordOwners[index];row=&cod1ActorPrefix.rows[i];
 if(error[0])Cod1ActorPrefixFail(NULL,0,error);
 row->stage=result.stage;row->startupConsumed=result.startupConsumed;row->entryStatus=result.entry.status;
 if(result.lease.serial)row->lease=result.lease;
 if(!row->accounted&&result.leaseRetained&&G_Cod1ActorReadLease(&result.lease,&actor,NULL,0)&&Cod1AiRefEqual(&actor.lease.entity,&row->entity)){
  row->accounted=1;cod1ActorPrefix.accountedRows++;
  if(!cod1AiDomain.unaccountedRows)return Cod1ActorRecordRefuse("BSP constructor accounting has no original unaccounted row");
  cod1AiDomain.unaccountedRows--;cod1AiDomainAccounted++;
 }
 if(!ok||!Cod1ActorRecordBspCurrent(&owner,&owner.entity)||result.stage!=COD1_ACTOR_ENTRY_LINKED||!row->accounted)
  return Cod1ActorRecordRefuse("BSP constructor failed; actual committed prefix retained for exact cleanup");
 r->state=COD1_RECORD_BSP_CONSTRUCTED;
 G_Printf("[GSC BSP] constructor before next record ordinal=%u entity=%u generation=%u fields=%u lease=%llu stage=%d; no script executed\n",owner.ordinal,owner.entity.slot,owner.entity.generation,result.initialFields,(unsigned long long)result.lease.serial,result.stage);
 return 1;
}
