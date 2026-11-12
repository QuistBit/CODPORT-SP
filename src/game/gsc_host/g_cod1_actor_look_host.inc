/* Included after the concrete generic graph transaction owner and current-tag
 * service. No successful generic visibility/perception replacement is supplied. */
#include "../g_cod1_actor_look.h"
#include "../g_cod1_turret_actor_tags.h"
#include "../g_cod1_actor_unmounted_tags.h"
static struct {int busy,cancelled;uint64_t serial;void*stage;size_t bytes;} cod1ActorLookOperation;
static struct {uint64_t epoch,lease;int tick;} cod1ActorLookClocks[COD1_ACTOR_CAPACITY];
static GscActorLookHost cod1ActorLookFacade;
static GscSpatialNativeHost cod1ActorEyeFacade;
static int ActorLookPrivate(const void*p,size_t n){return ActorOverlap(p,n,&cod1ActorLookOperation,sizeof(cod1ActorLookOperation))||ActorOverlap(p,n,cod1ActorLookClocks,sizeof(cod1ActorLookClocks))||ActorOverlap(p,n,&cod1ActorLookFacade,sizeof(cod1ActorLookFacade))||ActorOverlap(p,n,&cod1ActorEyeFacade,sizeof(cod1ActorEyeFacade))||ActorOverlap(p,n,cod1ActorLookOperation.stage,cod1ActorLookOperation.bytes);}
static void ActorLookReset(void){if(cod1ActorLookOperation.busy)cod1ActorLookOperation.cancelled=1;if(cod1ActorLookOperation.serial!=UINT64_MAX)cod1ActorLookOperation.serial++;memset(cod1ActorLookClocks,0,sizeof(cod1ActorLookClocks));}
typedef struct {GscEntityRef entity;cod1ActorLease_t lease;uint64_t epoch,serial;int now;const void*source;size_t bytes;const void*copy;int(*current)(void*);void*ctx;} C1ALCall;
static int ActorLookScope(C1ALCall*c){cod1ActorLease_t lease;
 return c&&cod1ActorLookOperation.busy&&!cod1ActorLookOperation.cancelled&&c->serial==cod1ActorLookOperation.serial&&c->epoch==cod1EntityEpoch&&cod1EntityReady&&!level.spawning&&!level.restarted&&level.time==c->now&&ActorMapAvailable()&&TurretAnimationMapCurrent(cod1TurretAnimationReceipt)&&
  ActorGoalBuffers(c->source,c->bytes,NULL,0,NULL,0)&&!ActorLookPrivate(c->source,c->bytes)&&!memcmp(c->source,c->copy,c->bytes)&&ActorGoalLease(&c->entity,&lease)&&!memcmp(&lease,&c->lease,sizeof(lease));
}
static int ActorLookFence(void*context){C1ALCall*c=context;return ActorLookScope(c)&&c->current(c->ctx)==1&&ActorLookScope(c);}
/* The readonly unmounted sampler requires the real graph clock at this
 * actual GAME tick. A query can be the first clock consumer before Session's
 * frame hook; same-tick preparation never advances phases a second time. */
static int ActorLookPreparePose(C1ALCall*c){C1GARecord*r;
 if(!ActorLookFence(c))return 0;
 if(cod1Actors[c->lease.actorIndex].turretMount.mounted)return 1;
 r=cod1GenericActors+c->lease.actorIndex;
 if(!r->used||r->tick>c->now)return 0;
 if(r->tick<c->now&&!G_Cod1ActorGenericFrame(&c->entity,c->now,ActorLookFence,c))return 0;
 return ActorLookFence(c)&&r->used&&r->tick==c->now;
}
static int ActorLookTagWorld(C1ALCall*c,const char*name,float origin[3],float angles[3]){
 if(ActorScriptedClockSelected(c->lease.actorIndex))return G_Cod1ActorFaceTagWorld(&c->entity,name,c->now,origin,angles,ActorLookFence,c);
 return cod1Actors[c->lease.actorIndex].turretMount.mounted?
  G_Cod1TurretActorTagWorld(&c->entity,name,c->now,origin,angles,ActorLookFence,c):
  G_Cod1ActorUnmountedTagWorld(&c->entity,name,c->now,origin,angles,ActorLookFence,c);
}
/* Native324A0 returns the common32360 world matrix translation at+30h.
 * Unlike GetTagAngles, origin does not require matrix-to-angles conversion. */
static int ActorLookTagMatrix(C1ALCall*c,const char*name,float matrix[16]){
 if(ActorScriptedClockSelected(c->lease.actorIndex))return G_Cod1ActorFaceTagMatrix(&c->entity,name,c->now,matrix,ActorLookFence,c);
 return cod1Actors[c->lease.actorIndex].turretMount.mounted?
  G_Cod1TurretActorTagMatrix(&c->entity,name,c->now,matrix,ActorLookFence,c):
  G_Cod1ActorUnmountedTagMatrix(&c->entity,name,c->now,matrix,ActorLookFence,c);
}
static int ActorLookBegin(C1ALCall*c,const GscEntityRef*entity,int now,const void*source,size_t bytes,const void*copy,int(*current)(void*),void*ctx){
 if(cod1ActorLookOperation.busy){cod1ActorLookOperation.cancelled=1;return 0;}
 if(!entity||!source||!bytes||!copy||!current||now!=level.time||cod1ActorLookOperation.serial==UINT64_MAX||!ActorGoalBuffers(source,bytes,NULL,0,NULL,0)||ActorLookPrivate(source,bytes))return 0;
 memset(c,0,sizeof(*c));c->entity=*entity;c->source=source;c->bytes=bytes;c->copy=copy;c->now=now;c->epoch=cod1EntityEpoch;c->current=current;c->ctx=ctx;if(!ActorGoalLease(&c->entity,&c->lease))return 0;
 cod1ActorLookOperation.busy=1;cod1ActorLookOperation.cancelled=0;c->serial=++cod1ActorLookOperation.serial;if(!ActorLookFence(c)){cod1ActorLookOperation.busy=0;return 0;}return 1;
}
static int ActorLookValidate(void*user,const GscEntityRef*input,uint64_t runtime,int32_t now,char*error,size_t cap,int(*current)(void*),void*ctx){C1ALCall c;GscEntityRef copy;int ok;(void)user;
 if(!input||!runtime||!ActorGoalBuffers(input,sizeof(*input),NULL,0,error,cap)||ActorLookPrivate(error,cap))return 0;copy=*input;
 if(!ActorLookBegin(&c,&copy,now,input,sizeof(*input),&copy,current,ctx))return 0;ok=ActorLookFence(&c)&&ActorGoalBuffers(input,sizeof(*input),NULL,0,error,cap);cod1ActorLookOperation.busy=0;return ok;
}
static int ActorLookBinding(const GscAnimationLengthRequest*a,uint64_t runtime,uint32_t*node){const GscAnimationCatalogEntry*e;
 if(!a||!memchr(a->tree,0,sizeof(a->tree))||strcmp(a->tree,"generic_human")||a->runtime_identity!=runtime||!a->ordinal||a->ordinal>=C1GA_COUNT)return 0;e=cod1GenericNames+a->ordinal;
 if(!memchr(a->name,0,sizeof(a->name))||strcmp(a->name,e->name)||a->flags!=e->flags)return 0;*node=a->ordinal;return 1;
}
typedef struct {GscActorLookRequest q;float eye[3],yaw;int frame;} C1ALMutation;
static int ActorLookApply(C1GARecord*r,void*user){C1ALMutation*m=user;c1al14State_t state;c1al14Result_t result;uint32_t nodes[3];float scalar[3],vector[3],weights[3];unsigned i;int ok=0;
 memcpy(&state,r->lookCells,64);memcpy(scalar,m->q.scalar_bits,12);memcpy(vector,m->q.vector_bits,12);
 if(m->frame){for(i=0;i<3;i++){if(state.nodes[i]>=C1GA_COUNT)return 0;weights[i]=r->nodes[state.nodes[i]].currentWeight;}ok=C1AL14_Step(&state,m->q.now_ms,m->eye,m->yaw,weights,&result);}
 else if(m->q.method==GSC_ACTOR_LOOK_NODES){
  static const char*const names[3]={"look_straight","look_left","look_right"};
  for(i=0;i<3;i++)if(!ActorLookBinding(m->q.animation+i,m->q.runtime_identity,nodes+i)||Q_stricmp(cod1GenericNames[nodes[i]].name,names[i]))return 0;
  ok=C1AL14_Nodes(&state,nodes,m->q.now_ms,&result);
 }else if(m->q.method==GSC_ACTOR_LOOK_TARGET){
  /* Original2006744B..20067460 requires configured Look nodes. The neutral
   * arithmetic helper does not own this Actor admission precondition. */
  if(!state.ready)return 0;
  ok=C1AL14_Target(&state,vector,scalar[0],&result);
 }
 else if(m->q.method==GSC_ACTOR_LOOK_YAW_LIMITS)ok=C1AL14_Limits(&state,scalar[0],scalar[1],scalar[2],m->q.now_ms,&result);
 else if(m->q.method==GSC_ACTOR_LOOK_STOP)ok=C1AL14_Stop(&state,scalar[0],&result);
 if(!ok)return 0;
 for(i=0;i<result.count;i++){c1al14Command_t*q=result.command+i;if(q->node>=C1GA_COUNT)return 0;
  if(q->kind==97){if(!C1AGT14_Clear(cod1GenericDefinitions,r->nodes,C1GA_COUNT,q->node,q->blend))return 0;}
  else if(q->kind==109){if(!C1AGT14_SetAnim(cod1GenericDefinitions,r->nodes,C1GA_COUNT,q->node,q->weight,q->blend,q->rate,0))return 0;}else return 0;
 }
 memcpy(r->lookCells,&result.state,64);return 1;
}
static int ActorLookInvoke(void*user,const GscActorLookRequest*input,GscActorLookReply*out,char*error,size_t cap,int(*current)(void*),void*ctx){
 C1ALCall c;C1ALMutation m;GscActorLookReply reply;cod1ActorNativeData_t native;gentity_t body;float origin[3],angles[3],matrix[16];int ok=0;(void)user;
 if(!input||!out||!ActorGoalBuffers(input,sizeof(*input),out,sizeof(*out),error,cap)||ActorLookPrivate(out,sizeof(*out))||ActorLookPrivate(error,cap))return 0;
 memset(&m,0,sizeof(m));m.q=*input;memset(&reply,0,sizeof(reply));if(!m.q.runtime_identity||!m.q.catalog_identity||m.q.precision_bits!=53||m.q.rounding||m.q.method<1||m.q.method>7)return 0;
 if(!ActorLookBegin(&c,&m.q.entity,m.q.now_ms,input,sizeof(*input),&m.q,current,ctx))return 0;
 if(m.q.method==GSC_ACTOR_LOOK_TAG_ORIGIN){
  if(!memchr(m.q.tag,0,sizeof(m.q.tag))||!m.q.tag[0])goto done;
  native=cod1Actors[c.lease.actorIndex].nativeData;body=g_entities[c.entity.slot];
  if(!ActorLookPreparePose(&c)||!ActorLookTagMatrix(&c,m.q.tag,matrix)||!ActorLookFence(&c)||
     memcmp(&body,g_entities+c.entity.slot,sizeof(body))||memcmp(&native,&cod1Actors[c.lease.actorIndex].nativeData,sizeof(native)))goto done;
  reply.kind=GSC_ACTOR_LOOK_RESULT_VECTOR;memcpy(reply.vector_bits,matrix+12,12);ok=1;
 }else if(m.q.method==GSC_ACTOR_LOOK_TAG_ANGLES){
  if(!memchr(m.q.tag,0,sizeof(m.q.tag))||!m.q.tag[0])goto done;native=cod1Actors[c.lease.actorIndex].nativeData;body=g_entities[c.entity.slot];
  if(!ActorLookPreparePose(&c)||!ActorLookTagWorld(&c,m.q.tag,origin,angles)||!ActorLookFence(&c)||memcmp(&body,&g_entities[c.entity.slot],sizeof(body))||memcmp(&native,&cod1Actors[c.lease.actorIndex].nativeData,sizeof(native)))goto done;
  reply.kind=GSC_ACTOR_LOOK_RESULT_VECTOR;memcpy(reply.vector_bits,angles,12);ok=1;
 }else if(m.q.method==GSC_ACTOR_LOOK_CAN_SEE){
  /* Original676B0 returns a visibility fraction via6A680/6A2B0. An arbitrary
   * clear ray is not that perception authority; refuse until it is supplied. */
  goto done;
 }else{
  if(!ActorGenericTransaction(&m.q.entity,m.q.runtime_identity,m.q.catalog_identity,m.q.now_ms,input,sizeof(*input),error,cap,ActorLookFence,&c,ActorLookApply,&m))goto done;
  if(m.q.method==GSC_ACTOR_LOOK_NODES){cod1ActorLookClocks[c.lease.actorIndex].epoch=c.entity.epoch;cod1ActorLookClocks[c.lease.actorIndex].lease=c.lease.serial;cod1ActorLookClocks[c.lease.actorIndex].tick=m.q.now_ms;}
  reply.kind=GSC_ACTOR_LOOK_RESULT_VOID;ok=1;
 }
done:if(ok&&(!ActorLookFence(&c)||!ActorGoalBuffers(input,sizeof(*input),out,sizeof(*out),error,cap)||ActorLookPrivate(out,sizeof(*out))||ActorLookPrivate(error,cap)))ok=0;
 if(ok)*out=reply;cod1ActorLookOperation.busy=0;return ok;
}
const GscActorLookHost*G_Cod1ActorLookHost(void){cod1ActorLookFacade.user=NULL;cod1ActorLookFacade.validate=ActorLookValidate;cod1ActorLookFacade.invoke=ActorLookInvoke;return &cod1ActorLookFacade;}
/* Actor and player share the existing typed GetEye descriptor. This adapter
 * changes no old spatial policy and never returns a bind-eye fallback. */
typedef struct {cod1ActorRecord_t actor;gentity_t body;} C1ALEyeSnapshot;
static int ActorEyeSnapshots(const C1ALCall*c,const C1ALEyeSnapshot*s){return ActorLookScope((C1ALCall*)c)&&!memcmp(&s->actor,cod1Actors+c->lease.actorIndex,sizeof(s->actor))&&!memcmp(&s->body,g_entities+c->entity.slot,sizeof(s->body));}
static int ActorEyeInvoke(void*unused,const GscSpatialNativeRequest*input,GscSpatialNativeResult*out,char*error,size_t cap,int(*current)(void*),void*ctx){
 GscSpatialNativeRequest q;GscSpatialNativeResult reply;cod1ActorLease_t lease;C1ALCall c;C1ALEyeSnapshot*s=NULL;float angles[3];int ok=0;(void)unused;
 if(cod1ActorLookOperation.busy){cod1ActorLookOperation.cancelled=1;return 0;}
 if(!input||!out||!current||!ActorGoalBuffers(input,sizeof(*input),out,sizeof(*out),error,cap)||ActorLookPrivate(input,sizeof(*input))||ActorLookPrivate(out,sizeof(*out))||ActorLookPrivate(error,cap))return 0;
 q=*input;
 if(q.kind!=GSC_SPATIAL_GET_EYE||!ActorGoalLease(&q.receiver,&lease)){
  const GscSpatialNativeHost*host=G_Cod1SpatialNativeHost();
  return host&&host->invoke&&host->invoke(host->user,input,out,error,cap,current,ctx)==1;
 }
 if(q.other.epoch||q.other.slot||q.other.generation||q.now_ms!=level.time)return 0;
 s=malloc(sizeof(*s));if(!s)return 0;memcpy(&s->actor,cod1Actors+lease.actorIndex,sizeof(s->actor));memcpy(&s->body,g_entities+q.receiver.slot,sizeof(s->body));
 cod1ActorLookOperation.stage=s;cod1ActorLookOperation.bytes=sizeof(*s);
 if(!ActorLookBegin(&c,&q.receiver,q.now_ms,input,sizeof(*input),&q,current,ctx))goto done;
 memset(&reply,0,sizeof(reply));
 if(!ActorEyeSnapshots(&c,s)||!ActorLookPreparePose(&c)||!ActorLookTagWorld(&c,"TAG_EYE",reply.vector,angles)||!ActorLookFence(&c)||!ActorEyeSnapshots(&c,s))goto done;
 if(!isfinite(reply.vector[0])||!isfinite(reply.vector[1])||!isfinite(reply.vector[2]))goto done;
 reply.kind=GSC_SPATIAL_RESULT_VECTOR;
 if(!ActorGoalBuffers(input,sizeof(*input),out,sizeof(*out),error,cap)||ActorLookPrivate(out,sizeof(*out))||ActorLookPrivate(error,cap)||!ActorEyeSnapshots(&c,s))goto done;
 *out=reply;ok=1;
done:cod1ActorLookOperation.stage=NULL;cod1ActorLookOperation.bytes=0;cod1ActorLookOperation.busy=0;free(s);return ok;
}
const GscSpatialNativeHost*G_Cod1ActorSpatialNativeHost(void){cod1ActorEyeFacade.user=NULL;cod1ActorEyeFacade.invoke=ActorEyeInvoke;return &cod1ActorEyeFacade;}
int G_Cod1ActorLookFrame(const GscEntityRef*input,int now,int(*current)(void*),void*ctx){
 C1ALCall c;GscEntityRef ref;C1ALMutation m;C1GARecord saved;C1GARecord*row;c1al14State_t state;float angles[3];uint32_t yaw;int index,ok=0;
 if(!input||!ActorGoalBuffers(input,sizeof(*input),NULL,0,NULL,0))return 0;ref=*input;if(!ActorLookBegin(&c,&ref,now,input,sizeof(*input),&ref,current,ctx))return 0;index=c.lease.actorIndex;row=cod1GenericActors+index;
 if(!row->used){ok=1;goto done;}saved=*row;memcpy(&state,row->lookCells,64);if(!state.ready){ok=1;goto done;}
 if(cod1ActorLookClocks[index].epoch!=ref.epoch||cod1ActorLookClocks[index].lease!=c.lease.serial||now<cod1ActorLookClocks[index].tick)goto done;
 if(now==cod1ActorLookClocks[index].tick){ok=1;goto done;}memcpy(&yaw,&state.yaw,4);
 if(!state.active&&!(yaw&0x7fffffffu)){cod1ActorLookClocks[index].tick=now;ok=1;goto done;}
 if(now-cod1ActorLookClocks[index].tick!=50)goto done;
 if(!ActorLookPreparePose(&c))goto done;saved=*row;
 memset(&m,0,sizeof(m));m.frame=1;m.q.entity=ref;m.q.now_ms=now;m.q.runtime_identity=row->runtime;m.q.catalog_identity=row->catalog;
 if(!ActorLookTagWorld(&c,"TAG_EYE",m.eye,angles)||!ActorLookFence(&c)||memcmp(row,&saved,sizeof(saved)))goto done;m.yaw=g_entities[ref.slot].r.currentAngles[1];
 if(!ActorGenericTransaction(&ref,m.q.runtime_identity,m.q.catalog_identity,now,&m.q,sizeof(m.q),NULL,0,ActorLookFence,&c,ActorLookApply,&m))goto done;
 cod1ActorLookClocks[index].tick=now;ok=1;
done:if(ok&&!ActorLookFence(&c))ok=0;cod1ActorLookOperation.busy=0;return ok;
}
