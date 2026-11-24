/* Active native prone state shares the actor allocation, owned tree and
 * existing checked CoD capsule/box world. No synthetic script completion. */
#include "../bg_cod1_active_prone14.h"
typedef struct {c1apr14State_t state;GscActorAnimationRequest q;int weightsChanged,update;float weights[3];} C1ProneMutation;
static void ActorProneRead(const cod1ActorRecord_t*a,c1apr14State_t*s){
 const uint32_t*z=a->nativeData.actor.zeroCells;
 memset(s,0,sizeof(*s));memcpy(s->nodes,a->nativeData.actor.proneNodes16c,12);memcpy(s->inverse,z,8);memcpy(&s->angle,z+2,4);
 s->ground=a->nativeData.actor.proneGround188;memcpy(&s->disabled,z+3,24);
}
static void ActorProneWrite(cod1ActorRecord_t*a,const c1apr14State_t*s,uint64_t runtime,uint64_t catalog,int tick){
 uint32_t*z=a->nativeData.actor.zeroCells;
 memcpy(a->nativeData.actor.proneNodes16c,s->nodes,12);memcpy(z,s->inverse,8);memcpy(z+2,&s->angle,4);
 a->nativeData.actor.proneGround188=s->ground;memcpy(z+3,&s->disabled,24);a->nativeData.actor.proneRuntime=runtime;a->nativeData.actor.proneCatalog=catalog;a->nativeData.actor.proneTick=tick;
}
static int ActorProneFence(void*ctx){C1CheckProneCall*m=ctx;return !m->failed&&ActorMayMoveCurrent(&m->owner);}
static int ActorProneApply(C1GARecord*r,void*ctx){
 C1ProneMutation*m=ctx;unsigned i,p,n;float f[3];
 if(!ActorLocomotionClosure(r))return 0;
 /* These original 109/110 calls have notify0. Retain the proven empty
  * notification-path domain; unrelated facial/body notes remain owned. */
 for(i=0;i<(m->update?5u:3u);i++){
  n=i<3?m->state.nodes[i]:m->q.animation[i-3].ordinal;
  if(!n||n>=C1GA_COUNT)return 0;
  for(p=n;;p=cod1GenericDefinitions[p].parent){
   if(r->notify->flags[p].token||r->notify->flags[p].clientType||r->notify->flags[p].cursor!=-1||r->notify->flags[p].descendant||
      r->notify->generation[p]||r->notify->pending[p]||ActorGenericName(r,p)[0])return 0;
   if(!p)break;
  }
 }
 memcpy(f,m->q.scalar_bits,12);
 if(m->update)for(i=0;i<2;i++)if(!C1AGT14_SetAnim(cod1GenericDefinitions,r->nodes,C1GA_COUNT,m->q.animation[i].ordinal,f[0],f[1],f[2],0))return 0;
 if(m->weightsChanged)for(i=0;i<3;i++)if(!C1AAW14_SetWeight(cod1GenericDefinitions,r->nodes,C1GA_COUNT,m->state.nodes[i],m->weights[i],.05f,1))return 0;
 return ActorLocomotionClosure(r);
}
static __declspec(noinline) int ActorProneMath(C1ProneMutation*m,int frame,int*query){
 unsigned char saved[108];uint32_t mx,nearMx=0x1f80;const unsigned short cw=0x027f;float f[3];unsigned nodes[3],i;int ok=1;
 __asm fnsave saved
 __asm stmxcsr mx
 __asm fldcw cw
 __asm ldmxcsr nearMx
 memcpy(f,m->q.scalar_bits,12);*query=0;
 if(frame){if(C1APR14_Active(&m->state,m->q.now_ms))*query=2;}
 else if(m->q.method==GSC_ACTOR_ANIMATION_PRONE_NODES){for(i=0;i<3;i++)nodes[i]=m->q.animation[i].ordinal;ok=C1APR14_Nodes(&m->state,nodes,f[0],f[1]);}
 else if(m->q.method==GSC_ACTOR_ANIMATION_ENTER_PRONE)*query=C1APR14_Enter(&m->state,m->q.now_ms,(int)((double)f[0]*1000));
 else if(m->q.method==GSC_ACTOR_ANIMATION_EXIT_PRONE)C1APR14_Exit(&m->state,m->q.now_ms,(int)((double)f[0]*1000));
 else if(m->q.method==GSC_ACTOR_ANIMATION_UPDATE_PRONE){m->update=C1APR14_Active(&m->state,m->q.now_ms);if(m->update)m->weightsChanged=C1APR14_Weights(&m->state,1,m->weights);}
 else ok=0;
 __asm ldmxcsr mx
 __asm frstor saved
 return ok;
}
static __declspec(noinline) int ActorProneGround(C1CheckProneCall*m,C1ProneMutation*v,int already){
 unsigned char saved[108];uint32_t mx,nearMx=0x1f80;const unsigned short cw=0x027f;int ok;
 const float*origin=m->owner.entity.r.currentOrigin;float yaw=m->owner.entity.r.currentAngles[1];
 if(!C1MMP14_Vec(origin)||!isfinite(yaw)||fabsf(yaw)>3600||!ActorProneFence(m))return 0;
 __asm fnsave saved
 __asm stmxcsr mx
 __asm fldcw cw
 __asm ldmxcsr nearMx
 v->state.ground=C1AP14_QueryRaw(m->owner.call->entity.slot,origin,15,24,yaw,&v->state.height,&v->state.bodyPitch,&v->state.feetPitch,already,1,NULL,ActorCheckProneCapsule,ActorCheckProneBox,1);
 if(already)v->weightsChanged=C1APR14_Weights(&v->state,0,v->weights);
 ok=isfinite(v->state.height)&&isfinite(v->state.bodyPitch)&&isfinite(v->state.feetPitch);
 __asm ldmxcsr mx
 __asm frstor saved
 return ok&&ActorProneFence(m);
}
static int ActorProneRun(const GscActorAnimationRequest*input,const GscEntityRef*frameRef,int now,char*error,size_t cap,int(*current)(void*),void*ctx){
 C1ALCall call;C1CheckProneCall*m=NULL;C1ProneMutation v;cod1ActorLease_t lease;GscEntityRef ref;const void*borrowed;size_t bytes;unsigned i;int ok=0,query=0,frame=frameRef!=NULL;
 memset(&v,0,sizeof(v));
 if(frame){
  if(!ActorGoalBuffers(frameRef,sizeof(*frameRef),NULL,0,error,cap)||!ActorGoalLease(frameRef,&lease))return 0;ref=*frameRef;
  if(!cod1Actors[lease.actorIndex].nativeData.actor.zeroCells[4]||cod1Actors[lease.actorIndex].nativeData.actor.proneTick==now)return 1;
  v.q.entity=ref;v.q.now_ms=now;v.q.runtime_identity=cod1Actors[lease.actorIndex].nativeData.actor.proneRuntime;v.q.catalog_identity=cod1Actors[lease.actorIndex].nativeData.actor.proneCatalog;
  borrowed=frameRef;bytes=sizeof(*frameRef);
 }else{
  if(!input||!ActorGoalBuffers(input,sizeof(*input),NULL,0,error,cap)||ActorLookPrivate(input,sizeof(*input)))return 0;
  v.q=*input;ref=v.q.entity;now=v.q.now_ms;borrowed=input;bytes=sizeof(*input);
  if(v.q.precision_bits!=53||v.q.rounding||v.q.notify[0]||!ActorGoalLease(&ref,&lease))return 0;
  if(v.q.method==GSC_ACTOR_ANIMATION_PRONE_NODES){for(i=0;i<3;i++)if(!ActorGenericBinding(v.q.animation+i,v.q.runtime_identity))return 0;}
  else if(v.q.method==GSC_ACTOR_ANIMATION_UPDATE_PRONE){for(i=0;i<2;i++)if(!ActorGenericBinding(v.q.animation+i,v.q.runtime_identity))return 0;}
  for(i=0;i<(v.q.method==GSC_ACTOR_ANIMATION_PRONE_NODES?2u:v.q.method==GSC_ACTOR_ANIMATION_UPDATE_PRONE?3u:1u);i++){
   float f;memcpy(&f,v.q.scalar_bits+i,4);if(!isfinite(f)||fabsf(f)>600||((v.q.method!=GSC_ACTOR_ANIMATION_PRONE_NODES)&&f<0))return 0;
  }
 }
 if(!v.q.runtime_identity||!v.q.catalog_identity||!ActorLookBegin(&call,&ref,now,borrowed,bytes,frame?(const void*)&ref:(const void*)&v.q,current,ctx))return 0;
 m=calloc(1,sizeof(*m));if(!m)goto done;
 m->owner.call=&call;m->owner.actor=cod1Actors[lease.actorIndex];m->owner.entity=g_entities[ref.slot];m->owner.graph=cod1GenericActors[lease.actorIndex];memcpy(m->owner.sentients,cod1Sentients,sizeof(m->owner.sentients));
 cod1ActorLookOperation.stage=m;cod1ActorLookOperation.bytes=sizeof(*m);
 if(!m->owner.graph.used||m->owner.graph.runtime!=v.q.runtime_identity||m->owner.graph.catalog!=v.q.catalog_identity||memcmp(&m->owner.graph.lease,&lease,sizeof(lease))||!ActorProneFence(m))goto done;
 ActorProneRead(&m->owner.actor,&v.state);
 if(v.q.method!=GSC_ACTOR_ANIMATION_PRONE_NODES&&(frame||v.q.method==GSC_ACTOR_ANIMATION_ENTER_PRONE||v.state.start)){
  if(m->owner.actor.nativeData.actor.proneRuntime!=v.q.runtime_identity||m->owner.actor.nativeData.actor.proneCatalog!=v.q.catalog_identity||!v.state.nodes[0]||!v.state.nodes[1]||!v.state.nodes[2])goto done;
 }
 if(!ActorProneMath(&v,frame,&query)||(query&&!ActorProneGround(m,&v,query==2))||!ActorProneFence(m))goto done;
 if(v.update||v.weightsChanged){
  if(!ActorGenericTransactionCore(&ref,v.q.runtime_identity,v.q.catalog_identity,now,borrowed,bytes,error,cap,ActorProneFence,m,ActorProneApply,&v))goto done;
 }else if(!ActorProneFence(m))goto done;
 /* GenericCore closes all external callbacks before committing its tree.
  * Publish the paired numeric state immediately, without a new callback. */
 {c1apr14State_t before;ActorProneRead(&m->owner.actor,&before);
  if(frame||v.q.method==GSC_ACTOR_ANIMATION_PRONE_NODES||memcmp(&before,&v.state,sizeof(before)))
   ActorProneWrite(cod1Actors+lease.actorIndex,&v.state,v.q.runtime_identity,v.q.catalog_identity,now);
 }ok=1;
 if(!frame&&g_debugMove.integer>1)G_Printf("[417 prone] owner=%u tick=%d method=%u start=%d duration=%d fit=%d terrain=%.7g,%.7g,%.7g traces=%d\n",ref.slot,now,v.q.method,v.state.start,v.state.duration,v.state.ground,v.state.height,v.state.bodyPitch,v.state.feetPitch,m->owner.queryCount);
done:
 if(!ok&&error&&cap&&!error[0])snprintf(error,cap,"active prone owner/tree/ground refused method=%u",v.q.method);
 cod1ActorLookOperation.stage=NULL;cod1ActorLookOperation.bytes=0;cod1ActorLookOperation.busy=0;free(m);return ok;
}
static int ActorProneInvoke(const GscActorAnimationRequest*input,char*error,size_t cap,int(*current)(void*),void*ctx){
 GscActorAnimationRequest q;int owned,ok;
 if(!input||!ActorGoalBuffers(input,sizeof(*input),NULL,0,error,cap)||ActorLookPrivate(input,sizeof(*input))||ActorLookPrivate(error,cap))return 0;q=*input;
 owned=ActorControllerGenericSourceEnter(&input->entity,q.runtime_identity,q.catalog_identity,q.now_ms,input,sizeof(*input),current,ctx,&q,7);
 if(owned<0||(!owned&&!ActorControllerForeignMutationAllowed()))return 0;
 ok=ActorProneRun(input,NULL,q.now_ms,error,cap,owned?ActorControllerGenericSourceCurrent:current,owned?ActorControllerGenericSourceContext():ctx);
 return ActorControllerGenericSourceFinish(owned,ok);
}
static int ActorProneFrame(const GscEntityRef*input,int now,int(*current)(void*),void*ctx){
 if(!ActorControllerForeignMutationAllowed())return 0;
 return ActorProneRun(NULL,input,now,NULL,0,current,ctx);
}
