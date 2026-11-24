#include "../../qcommon/cod1_actor_path_stance14.h"
#include "../cod1_gsc/stance-profile.h"
#include "../cod1_gsc/stance-data.h"
typedef struct {
 R243StanceRequest q,seal;const R243StanceRequest*source;
 cod1ActorLease_t lease;cod1ActorRecord_t actor;cod1SentientRecord_t sentient;gentity_t body;
 uint64_t epoch,nonce;int hostNow;int(*current)(void*);void*context;R243StanceReply reply;
} R243StanceGameFrame;
static R243StanceGameFrame*r243StanceGameActive;
static uint64_t r243StanceGameNonce;
static const R243StanceHost r243StanceGameHost;
static int ActorStancePrivate(const void*p,size_t n){
 return ActorOverlap(p,n,&r243StanceGameActive,sizeof(r243StanceGameActive))||
  ActorOverlap(p,n,&r243StanceGameNonce,sizeof(r243StanceGameNonce))||
  ActorOverlap(p,n,&r243StanceGameHost,sizeof(r243StanceGameHost))||
  (r243StanceGameActive&&ActorOverlap(p,n,r243StanceGameActive,sizeof(*r243StanceGameActive)));
}
/* Method-specific read scope. The original predicate needs an actual Actor;
 * a Goal mutator's separate !initialized restriction is not this contract.
 * NativeData ownership and busy-state bounds stay explicit. No global lease
 * validator or Goal API is changed. */
static int ActorStanceLease(const GscEntityRef*ref,cod1ActorLease_t*out){
 unsigned i;cod1ActorRecord_t*a;
 if(!ref||!G_Cod1EntitySameAllocation(ref)||ref->slot>=MAX_GENTITIES||!cod1ActorByEntity[ref->slot])return 0;
 i=cod1ActorByEntity[ref->slot]-1;if(i>=COD1_ACTOR_CAPACITY)return 0;a=cod1Actors+i;
 memset(out,0,sizeof(*out));out->entity=*ref;out->actorIndex=i;out->sentientIndex=a->sentientIndex;out->serial=a->serial;out->kind=COD1_ACTOR_LEASE_ACTOR;
 return ActorLeaseValid(out)&&a->alive&&a->nativeDataInitialized&&!a->spatial.busy&&!a->dobj.busy&&!a->modelBusy&&!a->modelMethodBusy;
}
static int r243_stance_game_validate(void*u,const GscEntityRef*input){
 cod1ActorLease_t lease;GscEntityRef entity;(void)u;
 if(!input||!R243StanceValidationRefCurrent(input)||ActorStancePrivate(input,sizeof(*input)))return 0;
 entity=*input;return ActorStanceLease(&entity,&lease);
}
static int r243_stance_game_scope(R243StanceGameFrame*f){
 cod1ActorLease_t after;
 if(!f||r243StanceGameActive!=f||f->nonce!=r243StanceGameNonce||memcmp(&f->q,&f->seal,sizeof(f->q))||
    f->epoch!=cod1EntityEpoch||!ActorMapAvailable()||!cod1EntityReady||level.spawning||level.restarted||f->hostNow!=level.time||!R243StanceSessionHostTimeCurrent(&f->seal,level.time)||
    ActorStancePrivate(f->source,sizeof(*f->source))||memcmp(f->source,&f->seal,sizeof(f->seal))||
    !ActorStanceLease(&f->seal.entity,&after)||memcmp(&after,&f->lease,sizeof(after)))return 0;
 return R242ExactATRBytesEqual(&cod1Actors[f->lease.actorIndex],&f->actor,sizeof(f->actor))&&
  !memcmp(&cod1Sentients[f->lease.sentientIndex],&f->sentient,sizeof(f->sentient))&&
  R242ExactATRBytesEqual(g_entities+f->seal.entity.slot,&f->body,sizeof(f->body));
}
static int r243_stance_game_fence(R243StanceGameFrame*f){
 return r243_stance_game_scope(f)&&R243StanceExactCallCurrent(&f->seal,f->current,f->context)&&r243_stance_game_scope(f);
}

int G_Cod1TrainingJumperPathFlags(const GscEntityRef*,const GscNodeRef*,const float[6][3],uint16_t[6],uint64_t*);
static int ActorStancePathMask(R243StanceGameFrame*f,unsigned*out){
 const cod1ActorMotion_t*m=&f->actor.motion;const cod1LocalRoute_t*r=&m->route;
 const int32_t indices[6]={0,1,2,3,4,5};uint16_t flags[6],after[6];uint64_t receipt,again;int32_t selected;
 if(!f->actor.goal.assigned||!m->active||m->busy||m->model||m->assignment!=f->actor.goal.assignment||
    (r->phase!=1&&r->phase!=2)||r->tower.length<1||r->tower.length>6||r->cursor<0||r->cursor>=r->tower.length||
    !TrainingJumperAllocation(&f->lease.entity)||!r243_stance_game_fence(f))return 0;
 if(!G_Cod1TrainingJumperPathFlags(&f->lease.entity,&f->actor.goal.node,r->points,flags,&receipt)||
    !C1APS14_SelectNode(r->tower.length,r->cursor,r->tower.base,r->points,indices,6,&selected)||
    !r243_stance_game_fence(f)||!G_Cod1TrainingJumperPathFlags(&f->lease.entity,&f->actor.goal.node,r->points,after,&again)||
    receipt!=again||memcmp(flags,after,sizeof(flags))||!r243_stance_game_scope(f))return 0;
 *out=selected<0?7:C1APS14_NodeMask(flags[selected]);
 if(g_debugMove.integer>1)G_Printf("[384 path stance] owner=%u/%u tick=%d count=%d cursor=%d selected=%d mask=%u\n",f->lease.entity.slot,f->lease.entity.generation,level.time,r->tower.length,r->cursor,selected,*out);
 return 1;
}
static int r243_stance_game_query(void*u,const R243StanceRequest*input,R243StanceReply*out,char*error,size_t cap,int(*current)(void*),void*context){
 R243StanceRequest q;R243StanceGameFrame*f=NULL;cod1ActorLease_t lease;int ok=0;uint32_t countWord;(void)u;
 if(!input||!out||!current||r243StanceGameActive||r243StanceGameNonce==UINT64_MAX||
    !R243StanceNativeBuffersCurrent(input,out,error,cap,current,context)||
    ActorStancePrivate(input,sizeof(*input))||ActorStancePrivate(out,sizeof(*out))||ActorStancePrivate(error,cap))return 0;
 q=*input;
 if(!memchr(q.stance,0,sizeof(q.stance))||q.bit!=R243StanceBit(q.stance)||!q.bit||!R243StanceSessionHostTimeCurrent(&q,level.time)||
    !q.program||!q.runtime||!q.catalog||!q.call||!R243StanceExactCallCurrent(&q,current,context)||!ActorStanceLease(&q.entity,&lease))return 0;
 f=malloc(sizeof(*f));if(!f)return 0;memset(f,0,sizeof(*f));f->q=q;f->seal=q;f->source=input;f->lease=lease;
 f->actor=cod1Actors[lease.actorIndex];f->sentient=cod1Sentients[lease.sentientIndex];f->body=g_entities[q.entity.slot];
 f->hostNow=level.time;f->epoch=cod1EntityEpoch;f->nonce=++r243StanceGameNonce;f->current=current;f->context=context;f->reply.allowed=2;r243StanceGameActive=f;
 if(!r243_stance_game_fence(f))goto done;
 if(q.writeMask){
  /* GAME 200682B0 replaces actor+234 with the union of all requested bits. */
  if((q.writeMask & ~7u) || !(q.writeMask&q.bit) ||
     !R243StanceNativeBuffersCurrent(input,out,error,cap,current,context) || !r243_stance_game_scope(f))goto done;
  cod1Actors[lease.actorIndex].nativeData.actor.cell0234=q.writeMask;
  if(g_debugMove.integer && f->actor.goal.assignment)
   G_Printf("[allowed stances] owner=%u/%u tick=%d assignment=%llu previous=%u mask=%u\n",
    lease.entity.slot,lease.entity.generation,level.time,(unsigned long long)f->actor.goal.assignment,
    f->actor.nativeData.actor.cell0234,q.writeMask);
  out->allowed=1;
  ok=1;
  goto done;
 }
 /* The read predicate inspects path state as well as the stance mask.
  * Its existing no-goal query domain is independent of the native setter. */
 /* A replaced node retains its assignment serial. The active bit identifies
  * a live node goal. No-path and null sentient node are checked below. */
 if(!Cod1StanceIndependentOfPath(f->actor.nativeData.actor.cell0234,q.bit,&f->reply.allowed)){
  if(f->actor.goal.assigned){
   unsigned pathMask;
   if(!ActorStancePathMask(f,&pathMask)||!C1APS14_Allowed(f->actor.nativeData.actor.cell0234,pathMask,q.bit,&f->reply.allowed)){
    if(error&&cap)snprintf(error,cap,"stance requires owned active path node flags");goto done;
   }
  }else{
  countWord=f->actor.defaults.pathWords[(0x6f4-0x374)/4];
  if(!R243StanceNoPathData(f->actor.nativeData.actor.cell0234,(int16_t)(uint16_t)countWord,
                          f->sentient.rawGoal.node0040,q.bit,&f->reply.allowed)){
   if(error&&cap)snprintf(error,cap,"isStanceAllowed active path/goal physical owner HOLD");
   goto done;
  }
  }
 }
 if(!r243_stance_game_fence(f)||!R243StanceNativeBuffersCurrent(input,out,error,cap,current,context)||ActorStancePrivate(out,sizeof(*out))||ActorStancePrivate(error,cap))goto done;
 /* Pure read: publish only after exact whole Actor/Sentient/entity and caller
  * revalidation. No animation, pose, path, goal, ready or field mutation. */
 *out=f->reply;ok=1;
done:
 r243StanceGameActive=NULL;free(f);return ok;
}
static const R243StanceHost r243StanceGameHost={NULL,r243_stance_game_validate,r243_stance_game_query,1};
const R243StanceHost*R243GameStanceHost(void){return &r243StanceGameHost;}
