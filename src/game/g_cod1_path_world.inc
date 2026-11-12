/* Sole owner g_spawn.c. Native6DBB0 saves entityC4 for ALL active118 entries,
 * zeroes flags134&1000, and restores ALL still-active entries. Port ownership
 * strengthens slot identity with epoch/generation and native actor lease.
 * No graph construction/native binding calls this until the other world
 * producer contracts are complete. No Actor/Session capability is granted. */
typedef struct {
    GscEntityRef ref;
    cod1ActorLease_t actor;
    uint32_t contents,nativeContents,flags;
    int active,isActor,projected;
} C1PathWorldRow;
static C1PathWorldRow cod1PathWorldRows[MAX_GENTITIES];
static uint64_t cod1PathWorldSerial,cod1PathWorldReceipt,cod1PathWorldEpoch;
static int cod1PathWorldPhase,cod1PathWorldCount,cod1PathWorldUsable;
static int Cod1PathProjectionActive(const GscEntityRef*ref,uint32_t contents){
 C1PathWorldRow*r;
 if(cod1PathWorldPhase!=2||!cod1PathWorldUsable||ref->slot>=(uint32_t)cod1PathWorldCount||
    ref->epoch!=cod1PathWorldEpoch||!cod1PathWorldReceipt)return 0;
 r=&cod1PathWorldRows[ref->slot];
 return r->active&&r->projected&&r->ref.epoch==ref->epoch&&r->ref.generation==ref->generation&&
  r->ref.slot==ref->slot&&contents==((r->flags&0x1000u)?0:r->nativeContents);
}
static int Cod1PathWorldOverlap(const void*p,size_t n,const void*q,size_t m){
 uintptr_t a=(uintptr_t)p,b=(uintptr_t)q;return p&&q&&n&&m&&(a<=b?b-a<n:a-b<m);
}
static int Cod1PathWorldSameActor(const cod1ActorLease_t*a,const cod1ActorLease_t*b){
 return a->entity.epoch==b->entity.epoch&&a->entity.generation==b->entity.generation&&
  a->entity.slot==b->entity.slot&&a->serial==b->serial&&a->actorIndex==b->actorIndex&&
  a->sentientIndex==b->sentientIndex&&a->kind==b->kind;
}
static int Cod1PathWorldCohortCurrent(void){
 int i;if(G_Cod1EntityMapEpoch()!=cod1PathWorldEpoch||level.num_entities!=cod1PathWorldCount)return 0;
 for(i=0;i<cod1PathWorldCount;i++){
  C1PathWorldRow*r=cod1PathWorldRows+i;
  if(!!g_entities[i].inuse!=r->active)return 0;
  if(r->active&&!G_Cod1EntitySameAllocation(&r->ref))return 0;
 }return 1;
}
int G_Cod1PathWorldCurrent(uint64_t receipt){
 int i;uint32_t flags;cod1ActorPathView_t actor;
 if(!receipt||receipt!=cod1PathWorldReceipt||cod1PathWorldPhase!=2||
    !cod1PathWorldUsable||!Cod1PathWorldCohortCurrent())return 0;
 for(i=0;i<cod1PathWorldCount;i++){
  C1PathWorldRow*r=cod1PathWorldRows+i;uint32_t expected;
  if(!r->active)continue;
  if(!G_Cod1PathEntityRead(&r->ref,&flags)||flags!=r->flags)return 0;
  expected=(r->flags&0x1000u)?0:r->nativeContents;
  if(r->isActor){
   if(!G_Cod1ActorPathRead(&r->ref,&actor)||!Cod1PathWorldSameActor(&r->actor,&actor.lease)||actor.contents!=expected)return 0;
  }else if((uint32_t)g_entities[i].r.contents!=expected)return 0;
 }return 1;
}
int G_Cod1PathWorldBegin(uint64_t*out){
 int i;uint64_t epoch;cod1ActorPathView_t actor;
 if(!out||cod1PathWorldPhase||cod1PathWorldSerial==UINT64_MAX||
    Cod1PathEntityOverlap(out,sizeof(*out)))return 0;
 /* Prevent writes into the transaction or game-owned input tables. */
 if(Cod1PathWorldOverlap(out,sizeof(*out),cod1PathWorldRows,sizeof(cod1PathWorldRows))||
    Cod1PathWorldOverlap(out,sizeof(*out),g_entities,sizeof(g_entities))||
    Cod1PathWorldOverlap(out,sizeof(*out),&cod1PathWorldReceipt,sizeof(cod1PathWorldReceipt))||
    Cod1PathWorldOverlap(out,sizeof(*out),&cod1PathWorldSerial,sizeof(cod1PathWorldSerial))||
    Cod1PathWorldOverlap(out,sizeof(*out),&cod1PathWorldEpoch,sizeof(cod1PathWorldEpoch))||
    Cod1PathWorldOverlap(out,sizeof(*out),&cod1PathWorldPhase,sizeof(cod1PathWorldPhase))||
    Cod1PathWorldOverlap(out,sizeof(*out),&cod1PathWorldCount,sizeof(cod1PathWorldCount))||
    Cod1PathWorldOverlap(out,sizeof(*out),&cod1PathWorldUsable,sizeof(cod1PathWorldUsable)))return 0;
 epoch=G_Cod1EntityMapEpoch();
 if(!epoch||!G_Cod1MapEntities()||level.num_entities<0||level.num_entities>MAX_GENTITIES)return 0;
 memset(cod1PathWorldRows,0,sizeof(cod1PathWorldRows));
 cod1PathWorldCount=level.num_entities;cod1PathWorldEpoch=epoch;
 for(i=0;i<cod1PathWorldCount;i++)if(g_entities[i].inuse){
  C1PathWorldRow*r=cod1PathWorldRows+i;r->active=1;
  if(!G_Cod1EntityCapture(g_entities+i,&r->ref)||!G_Cod1PathEntityRead(&r->ref,&r->flags))return 0;
  if(G_Cod1ActorPathRead(&r->ref,&actor)){
   r->isActor=1;r->actor=actor.lease;r->contents=actor.contents;
  }else {
   /* No query mask contract: filter-only models are not geometry owners.
    * A stale model row must not block a replacement native actor above. */
   if(cod1PathEntities[i].kind==7||cod1PathEntities[i].kind==8||cod1PathEntities[i].kind==9)return 0;
   r->contents=(uint32_t)g_entities[i].r.contents;
  }
  r->nativeContents=r->contents;
  if(!r->isActor&&cod1PathEntities[i].projected){
   C1PathEntityRecord*p=Cod1PathProjectionRecord(&r->ref);
   if(!p||r->contents!=(uint32_t)p->shape.contents)return 0;
   r->projected=1;r->nativeContents=p->nativeContents;
  }
 }
 if(!Cod1PathWorldCohortCurrent())return 0;
 cod1PathWorldReceipt=++cod1PathWorldSerial;cod1PathWorldUsable=0;cod1PathWorldPhase=1;*out=cod1PathWorldReceipt;
 for(i=0;i<cod1PathWorldCount;i++){
  C1PathWorldRow*r=cod1PathWorldRows+i;
  if(!r->active||(!(r->flags&0x1000u)&&!r->projected))continue;
  if(r->isActor){
   if(!G_Cod1ActorPathContents(&r->actor,r->contents,0)||!Cod1PathWorldCohortCurrent()){
    cod1PathWorldPhase=2;return -1;
   }
  }else g_entities[i].r.contents=(r->flags&0x1000u)?0:(int)r->nativeContents;
 }
 cod1PathWorldPhase=2;cod1PathWorldUsable=1;return 1;
}
int G_Cod1PathWorldEnd(uint64_t receipt){
 int i,failed=0;cod1ActorPathView_t actor;
 if(!receipt||receipt!=cod1PathWorldReceipt||cod1PathWorldPhase!=2)return 0;
 cod1PathWorldUsable=0;cod1PathWorldPhase=3;
 for(i=0;i<cod1PathWorldCount;i++){
  C1PathWorldRow*r=cod1PathWorldRows+i;
  if(!r->active)continue;
  if(!G_Cod1EntitySameAllocation(&r->ref)){r->active=0;continue;}
  if(r->isActor){
   if(G_Cod1ActorPathRetired(&r->actor)){r->active=0;continue;}
   if(!G_Cod1ActorPathRead(&r->ref,&actor)){failed=1;continue;}
   if(!Cod1PathWorldSameActor(&r->actor,&actor.lease)){r->active=0;continue;}
   if(!G_Cod1ActorPathContents(&r->actor,actor.contents,r->contents)){
    if(G_Cod1EntitySameAllocation(&r->ref))failed=1;else r->active=0;
    continue;
   }
  }else {
   /* A new native owner on the same entity allocation is not the captured
    * brush contents owner. Do not write through an ambiguous representation. */
   if(G_Cod1ActorPathRead(&r->ref,&actor)){failed=1;continue;}
   /* A changed typed shape/handler cannot authorize a write through the old
    * projection. Retain cleanup for retry; replaced allocations are handled
    * above without touching their new contents. */
   if(r->projected&&!Cod1PathProjectionRecord(&r->ref)){failed=1;continue;}
   g_entities[i].r.contents=(int)r->contents;
  }
  r->active=0;
 }
 if(failed){cod1PathWorldPhase=2;return 0;}
 memset(cod1PathWorldRows,0,sizeof(cod1PathWorldRows));cod1PathWorldPhase=0;
 cod1PathWorldReceipt=cod1PathWorldEpoch=0;cod1PathWorldCount=0;return 1;
}
