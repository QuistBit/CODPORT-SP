#include "../../qcommon/cod1_actor_aim_math14.h"
#include "../../qcommon/cod1_actor_aim_weight14.h"
typedef struct {
 C1ALCall *call;GscActorLookRequest request;
 cod1ActorRecord_t actor;gentity_t body;C1GARecord graph;
 float eye[3],weights[3];
} C1AimPosition;
static int ActorAimScope(const C1AimPosition*m){
 const C1ALCall*c=m->call;
 return ActorLookScope((C1ALCall*)c)&&
  !memcmp(&m->actor,cod1Actors+c->lease.actorIndex,sizeof(m->actor))&&
  !memcmp(&m->body,g_entities+c->entity.slot,sizeof(m->body))&&
  !memcmp(&m->graph,cod1GenericActors+c->lease.actorIndex,sizeof(m->graph));
}
static int ActorAimFence(void*ctx){C1AimPosition*m=ctx;return ActorAimScope(m)&&ActorLookFence(m->call)&&ActorAimScope(m);}
static int ActorAimApply(C1GARecord*r,void*ctx){
 C1AimPosition*m=ctx;unsigned i,n,p;
 if(r->runtime!=m->request.runtime_identity||r->catalog!=m->request.catalog_identity||
    memcmp(&r->lease,&m->call->lease,sizeof(r->lease))||!ActorLocomotionClosure(r))return 0;
 /* The retained109/110 numeric proof has empty notify ownership on each
  * affected path. Refuse a named shoot/aim path until its string ownership
  * and cursor transfer are qualified; unrelated facial channels may remain. */
 for(i=0;i<6;i++){
  n=m->actor.aimNodes154[i];if(!ActorAimLeaf(n))return 0;
  for(p=n;;p=cod1GenericDefinitions[p].parent){
   if(r->notify->flags[p].token||r->notify->flags[p].clientType||r->notify->flags[p].cursor!=-1||r->notify->flags[p].descendant||
      r->notify->generation[p]||r->notify->pending[p]||ActorGenericName(r,p)[0])return 0;
   if(!p)break;
  }
 }
 for(i=0;i<6;i++){
  n=m->actor.aimNodes154[i];
  if(!ActorAimLeaf(n)||!ActorLocomotionRoot(n)||!ActorLocomotionBody(n))return 0;
  /* Native six calls in their authored order: 109 raises aim ancestors;
   * 110 changes shoot leaves without activating their parents. */
  if(i<3){if(!C1AGT14_SetAnim(cod1GenericDefinitions,r->nodes,C1GA_COUNT,n,m->weights[i],.1f,1,0))return 0;}
  else if(!C1AAW14_SetWeight(cod1GenericDefinitions,r->nodes,C1GA_COUNT,n,m->weights[i-3],.1f,1))return 0;
 }
 return ActorLocomotionClosure(r);
}
/* Consume a typed Look call before selecting the common transaction core. */
static int ActorAimTransaction(C1AimPosition*m,char*error,size_t cap){
 C1ALCall*c=m->call;GscActorLookRequest local=m->request;const GscActorLookRequest*q=&local;
 /* The public transaction rejects pointers into another private operation.
  * Retain the sealed request, but pass its local copy across this boundary. */
 if(c->sourceOwned){
  if(!ActorControllerLookSourceMatch(c->source,c->bytes,c->current,c->ctx,q))return 0;
  return ActorGenericTransactionCore(&q->entity,q->runtime_identity,q->catalog_identity,q->now_ms,
   c->source,c->bytes,error,cap,ActorAimFence,m,ActorAimApply,m);
 }
 return ActorGenericTransaction(&q->entity,q->runtime_identity,q->catalog_identity,q->now_ms,
   c->source,c->bytes,error,cap,ActorAimFence,m,ActorAimApply,m);
}
static int ActorAimAtPos(C1ALCall*c,const GscActorLookRequest*q,GscActorLookReply*reply,char*error,size_t cap){
 static const GscAnimationLengthRequest emptyAnimation[3]={{0}};
 static const GscEntityRef emptyEntity={0};static const char emptyTag[64]={0};
 C1AimPosition*m=NULL;float target[3],angles[3],eye[3];unsigned i;int ok=0,phase=1;
 if(q->method!=GSC_ACTOR_LOOK_AIM_AT_POS||q->scalar_bits[0]||q->scalar_bits[1]||q->scalar_bits[2]||
    memcmp(&q->argument,&emptyEntity,sizeof(emptyEntity))||memcmp(q->animation,emptyAnimation,sizeof(emptyAnimation))||memcmp(q->tag,emptyTag,sizeof(emptyTag))||
    !ActorScriptedClockSelected(c->lease.actorIndex)||cod1Actors[c->lease.actorIndex].turretMount.mounted){if(error&&cap)snprintf(error,cap,"aim initial selected=%d mounted=%d",ActorScriptedClockSelected(c->lease.actorIndex),cod1Actors[c->lease.actorIndex].turretMount.mounted);return 0;}
 m=malloc(sizeof(*m));if(!m)return 0;memset(m,0,sizeof(*m));m->call=c;m->request=*q;
 m->actor=cod1Actors[c->lease.actorIndex];m->body=g_entities[c->entity.slot];m->graph=cod1GenericActors[c->lease.actorIndex];
 cod1ActorLookOperation.stage=m;cod1ActorLookOperation.bytes=sizeof(*m);
 phase=2;
 if(!m->actor.aimNodesSet||m->actor.aimNodesRuntime!=q->runtime_identity||m->actor.aimNodesCatalog!=q->catalog_identity||
    !m->graph.used||!m->graph.nodes||!m->graph.notify||!m->graph.names||m->graph.runtime!=q->runtime_identity||m->graph.catalog!=q->catalog_identity||
    memcmp(&m->graph.lease,&c->lease,sizeof(c->lease))||m->graph.tick!=c->now||!ActorAimFence(m))goto done;
 phase=3;for(i=0;i<6;i++)if(!ActorAimLeaf(m->actor.aimNodes154[i]))goto done;
 phase=4;if(!ActorLookPreparePose(c)||!ActorAimFence(m))goto done;
 /* The public tag provider cannot write into another operation's private
  * snapshot. Receive on this stack, revalidate, then copy into our owner. */
 phase=5;if(!ActorLookTagWorld(c,"TAG_EYE",eye,angles)||!ActorAimFence(m))goto done;
 memcpy(m->eye,eye,sizeof(eye));
 memcpy(target,q->vector_bits,12);
 phase=6;if(!C1AAM14_Weights(target,m->eye,m->weights)||!ActorAimFence(m))goto done;
 phase=7;if(!ActorAimTransaction(m,error,cap))goto done;
 reply->kind=GSC_ACTOR_LOOK_RESULT_FLOAT;reply->vector_bits[0]=0x3dcccccdu;
 if(g_debugMove.integer>1)G_Printf("[398 aim position] owner=%u tick=%d eye=%.8g,%.8g,%.8g weights=%.8g,%.8g,%.8g six_effects=1\n",q->entity.slot,q->now_ms,m->eye[0],m->eye[1],m->eye[2],m->weights[0],m->weights[1],m->weights[2]);
 ok=1;
done:if(!ok&&error&&cap&& !error[0])snprintf(error,cap,"aim phase=%d graphTick=%d now=%d aim=%d runtime=%llu/%llu catalog=%llu/%llu scope=%d",phase,m->graph.tick,c->now,m->actor.aimNodesSet,(unsigned long long)m->actor.aimNodesRuntime,(unsigned long long)q->runtime_identity,(unsigned long long)m->actor.aimNodesCatalog,(unsigned long long)q->catalog_identity,ActorAimScope(m));cod1ActorLookOperation.stage=NULL;cod1ActorLookOperation.bytes=0;free(m);return ok;
}
