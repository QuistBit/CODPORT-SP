#include "../cod1_gsc/actor-float-profile.h"
/* Pure bounded N0019 path lowWORD0 or N0087 complete physical36/no opponent.
 * Path reads only its actual own Actor/Sentient/entity allocation snapshot.
 * Pure bounded N0087 lane: actual complete physical36, no opposing member.
 * Constructor census is additional coverage, never a replacement for36 slots.
 * Opposing members and native no-AddFloat domains remain terminal HOLD. */
typedef struct {
 R243ActorFloatRequest q,seal;const R243ActorFloatRequest*source;
 cod1ActorLease_t lease;cod1ActorRecord_t actor;gentity_t body;
 cod1SentientRecord_t ownSentient;unsigned char ownActorRegistry,ownSentientRegistry;
 cod1SentientRecord_t sentients[COD1_SENTIENT_CAPACITY];
 unsigned char actorByEntity[MAX_GENTITIES],sentientByEntity[MAX_GENTITIES];
 cod1PlayerLifetime_t players[MAX_CLIENTS];int connected[MAX_CLIENTS],maxClients;
 cod1AiDomainSnapshot_t domain;GscAiArrayRequest censusRequest;GscAiArraySnapshot census;
 uint64_t epoch,nonce,allocationSerial;int hostNow;
 int(*current)(void*);void*context;R243ActorFloatReply reply;
} R243ActorFloatGameFrame;
static R243ActorFloatGameFrame*r243ActorFloatGameActive;
static uint64_t r243ActorFloatGameNonce;
static const R243ActorFloatHost r243ActorFloatGameHost;
static int ActorFloatPrivate(const void*p,size_t n){
 return ActorOverlap(p,n,&r243ActorFloatGameActive,sizeof(r243ActorFloatGameActive))||
  ActorOverlap(p,n,&r243ActorFloatGameNonce,sizeof(r243ActorFloatGameNonce))||
  ActorOverlap(p,n,&r243ActorFloatGameHost,sizeof(r243ActorFloatGameHost))||
  (r243ActorFloatGameActive&&ActorOverlap(p,n,r243ActorFloatGameActive,sizeof(*r243ActorFloatGameActive)));
}
static int ActorFloatLease(const GscEntityRef*ref,cod1ActorLease_t*out){
 unsigned i;cod1ActorRecord_t*a;
 if(!ref||!out||ref->slot>=MAX_GENTITIES||!G_Cod1EntitySameAllocation(ref)||!cod1ActorByEntity[ref->slot])return 0;
 i=cod1ActorByEntity[ref->slot]-1;if(i>=COD1_ACTOR_CAPACITY)return 0;a=cod1Actors+i;
 memset(out,0,sizeof(*out));out->entity=*ref;out->actorIndex=i;out->sentientIndex=a->sentientIndex;out->serial=a->serial;out->kind=COD1_ACTOR_LEASE_ACTOR;
 return ActorLeaseValid(out)&&a->alive&&a->nativeDataInitialized&&!a->spatial.busy&&!a->dobj.busy&&!a->modelBusy&&!a->modelMethodBusy;
}
static int r243_actor_float_game_validate(void*u,const GscEntityRef*ref){cod1ActorLease_t lease;(void)u;
 return ref&&R243ActorFloatValidationRefCurrent(ref)&&!ActorFloatPrivate(ref,sizeof(*ref))&&ActorFloatLease(ref,&lease);
}
static int ActorFloatPlayers(R243ActorFloatGameFrame*f,int capture){
 unsigned i;GscEntityRef ref;cod1PlayerLifetime_t life;
 if(level.maxclients<0||level.maxclients>MAX_CLIENTS)return 0;
 if(capture)f->maxClients=level.maxclients;else if(f->maxClients!=level.maxclients)return 0;
 for(i=0;i<MAX_CLIENTS;i++){
  int connected=g_clients[i].pers.connected;
  if(capture)f->connected[i]=connected;else if(f->connected[i]!=connected)return 0;
  if(connected==CON_DISCONNECTED)continue;
  if(i>=(unsigned)level.maxclients||!g_entities[i].inuse||g_entities[i].client!=g_clients+i||
     !G_Cod1EntityCapture(g_entities+i,&ref)||!cod1SentientByEntity[i]||cod1SentientByEntity[i]>COD1_SENTIENT_CAPACITY)return 0;
  /* Existing completed ClientBegin lifetime, NOT native118/path134/C4. */
  if(capture){if(!G_Cod1PlayerLifetimeRead(&ref,&life))return 0;f->players[i]=life;}
  else{life=f->players[i];if(!ActorRefEqual(&life.entity,&ref)||!G_Cod1PlayerLifetimeSameCompleted(&life))return 0;}
  if(cod1Sentients[cod1SentientByEntity[i]-1].kind!=COD1_ACTOR_LEASE_PLAYER)return 0;
 }
 return 1;
}
static int ActorFloatRosterCurrent(R243ActorFloatGameFrame*f){
 unsigned i;cod1ActorLease_t lease;cod1AiDomainSnapshot_t domain;
 if(f->allocationSerial!=cod1ActorSerial||
    !R242ExactATRBytesEqual(f->sentients,cod1Sentients,sizeof(f->sentients))||
    !R242ExactATRBytesEqual(f->actorByEntity,cod1ActorByEntity,sizeof(f->actorByEntity))||
    !R242ExactATRBytesEqual(f->sentientByEntity,cod1SentientByEntity,sizeof(f->sentientByEntity))||
    !G_Cod1AiDomainRead(&domain,NULL,0)||memcmp(&domain,&f->domain,sizeof(domain))||
    !domain.sealed||!domain.complete||domain.invalid||domain.unaccountedRows||!ActorFloatPlayers(f,0))return 0;
 for(i=0;i<COD1_SENTIENT_CAPACITY;i++){
  const cod1SentientRecord_t*s=f->sentients+i;if(s->activation<0)continue;
  if(s->defaults.team<1||s->defaults.team>3)return 0;
  memset(&lease,0,sizeof(lease));lease.entity=s->owner;lease.serial=s->serial;
  lease.actorIndex=s->actorIndex;lease.sentientIndex=i;lease.kind=s->kind;
  if(!ActorLeaseValid(&lease))return 0;
  /* Every physical PLAYER row must belong to a captured connected, completed
   * ClientBegin owner. The connected-client scan alone cannot qualify rows
   * left behind after disconnect/retirement. Never synthesize that owner. */
  if(s->kind==COD1_ACTOR_LEASE_PLAYER){
   unsigned slot=s->owner.slot;
   if(slot>=MAX_CLIENTS||slot>=(unsigned)f->maxClients||
      f->connected[slot]==CON_DISCONNECTED||
      !ActorRefEqual(&f->players[slot].entity,&s->owner)||
      cod1SentientByEntity[slot]!=i+1)return 0;
  }
 }
 return 1;
}
/* No external callback below. This is rechecked after the last census/current
 * callback, before deriving the result from the current complete snapshot. */
static int r243_actor_float_game_scope(R243ActorFloatGameFrame*f){
 cod1ActorLease_t after;
 return f&&r243ActorFloatGameActive==f&&f->nonce==r243ActorFloatGameNonce&&
  !memcmp(&f->q,&f->seal,sizeof(f->q))&&f->epoch==cod1EntityEpoch&&
  ActorMapAvailable()&&cod1EntityReady&&!level.spawning&&!level.restarted&&f->hostNow==level.time&&
  !ActorFloatPrivate(f->source,sizeof(*f->source))&&!memcmp(f->source,&f->seal,sizeof(f->seal))&&
  ActorFloatLease(&f->seal.entity,&after)&&!memcmp(&after,&f->lease,sizeof(after))&&
  R242ExactATRBytesEqual(cod1Actors+f->lease.actorIndex,&f->actor,sizeof(f->actor))&&
  R242ExactATRBytesEqual(g_entities+f->seal.entity.slot,&f->body,sizeof(f->body))&&
  (f->seal.method!=R243_ACTOR_FLOAT_ENEMY?
   (f->actor.dataInitialized&&f->ownActorRegistry==cod1ActorByEntity[f->seal.entity.slot]&&
    f->ownSentientRegistry==cod1SentientByEntity[f->seal.entity.slot]&&
    !memcmp(cod1Sentients+f->lease.sentientIndex,&f->ownSentient,sizeof(f->ownSentient))):
   f->seal.method==R243_ACTOR_FLOAT_ENEMY&&ActorFloatRosterCurrent(f));
}
static int r243_actor_float_game_fence(void*opaque){R243ActorFloatGameFrame*f=opaque;
 return r243_actor_float_game_scope(f)&&R243ActorFloatSessionHostTimeCurrent(&f->seal,f->hostNow)&&
  R243ActorFloatExactCallCurrent(&f->seal,f->current,f->context)&&r243_actor_float_game_scope(f);
}
/* GAME 20068F40. In this profile the Training mover owns both the
 * current path segment and the velocity after collision. */
static int ActorCourseMotionAngle(const R243ActorFloatGameFrame *f,float *out){
 const cod1ActorMotion_t *m=&f->actor.motion;
 const float *direction;
 float heading;
 /* A factory-owned jumper has a real mounted-graph path. Read its
  * calculated lookahead, never the seed vector used while building it. */
 if(TrainingJumperAllocation(&f->seal.entity)){
  const cod1TowerPath_t*p=&m->route.tower;
  if(!m->active||m->busy||m->model||m->arrived||!f->actor.goal.assigned||
     m->assignment!=f->actor.goal.assignment||m->route.phase!=2||
     p->length<1||p->length>6||
     !TrainingJumperExitNode(&f->seal.entity,m->assignment,&f->actor.goal.node))return 0;
  direction=p->aim;
  if(!isfinite(direction[0])||!isfinite(direction[1])||
     !isfinite(f->body.r.currentAngles[YAW])||fabsf(f->body.r.currentAngles[YAW])>360.f)return 0;
  /* Native vectoyaw stores FLOAT after double atan2 *180 /pi. */
  heading=(direction[0]==0&&direction[1]==0)?0:
   (float)((atan2((double)direction[1],(double)direction[0])*180.0)/M_PI);
  if(heading<0)heading+=360;
  heading-=f->body.r.currentAngles[YAW];
  while(heading>180)heading-=360;
  while(heading<-180)heading+=360;
  *out=heading;return 1;
 }
 if(m->busy || m->model || !f->body.targetname ||
    strcmp(f->body.targetname,"runner") || !TrainingCurrentRoute(&f->seal.entity))return 0;
 /* A placed runner is already a live Actor before the mover owns a path.
  * Read zero only for its untouched, stationary pre-movement state. */
 if(!m->active){
    if(m->assignment || m->moving || m->route.headingValid ||
       m->velocity[0]!=0 || m->velocity[1]!=0 || m->velocity[2]!=0)return 0;
    *out=0;return 1;
 }
 if(m->route.headingValid && !m->arrived && f->actor.goal.assigned && m->assignment==f->actor.goal.assignment)
    direction=m->route.heading;
 else if(VectorLengthSquared(m->velocity)>1.0f)direction=m->velocity;
 else{*out=0;return 1;}
 if(!isfinite(direction[0]) || !isfinite(direction[1]) || !isfinite(f->body.r.currentAngles[YAW]))return 0;
 heading=(direction[0]==0 && direction[1]==0)?0:atan2f(direction[1],direction[0])*(180.0f/(float)M_PI);
 if(heading<0)heading+=360;
 heading-=f->body.r.currentAngles[YAW];
 while(heading>180)heading-=360;
 while(heading<-180)heading+=360;
 *out=heading;return 1;
}
static int r243_actor_float_game_query(void*u,const R243ActorFloatRequest*input,R243ActorFloatReply*out,char*error,size_t cap,int(*current)(void*),void*context){
 R243ActorFloatGameFrame*f=NULL;R243ActorFloatRequest q;cod1ActorLease_t lease;
 const GscAiArrayHost*census=NULL;unsigned i,ownTeam,opposing,count=0,active=0;int ok=0;(void)u;
 if(!input||!out||!current||r243ActorFloatGameActive||r243ActorFloatGameNonce==UINT64_MAX||
    !R243ActorFloatNativeBuffersCurrent(input,out,error,cap,current,context)||
    ActorFloatPrivate(input,sizeof(*input))||ActorFloatPrivate(out,sizeof(*out))||ActorFloatPrivate(error,cap))return 0;
 q=*input;
 if(!R243ActorFloatMask(q.method)||!q.program||!q.runtime||!q.catalog||!q.call||
    !R243ActorFloatSessionHostTimeCurrent(&q,level.time)||
    !R243ActorFloatExactCallCurrent(&q,current,context)||!ActorFloatLease(&q.entity,&lease))return 0;
 if(q.method==R243_ACTOR_FLOAT_ENEMY){census=G_Cod1AiArrayHost();if(!census||!census->query||!census->validate)return 0;}
 f=malloc(sizeof(*f));if(!f)return 0;memset(f,0,sizeof(*f));f->q=f->seal=q;f->source=input;f->lease=lease;
 f->actor=cod1Actors[lease.actorIndex];f->body=g_entities[q.entity.slot];
 if(q.method==R243_ACTOR_FLOAT_ENEMY){memcpy(f->sentients,cod1Sentients,sizeof(f->sentients));memcpy(f->actorByEntity,cod1ActorByEntity,sizeof(f->actorByEntity));memcpy(f->sentientByEntity,cod1SentientByEntity,sizeof(f->sentientByEntity));}
 else{f->ownSentient=cod1Sentients[lease.sentientIndex];f->ownActorRegistry=cod1ActorByEntity[q.entity.slot];f->ownSentientRegistry=cod1SentientByEntity[q.entity.slot];}
 f->epoch=cod1EntityEpoch;f->allocationSerial=cod1ActorSerial;f->hostNow=level.time;
 f->nonce=++r243ActorFloatGameNonce;f->current=current;f->context=context;
 f->reply.bits=0x7fc00000u;f->censusRequest.now_ms=q.now;f->censusRequest.team_mask=14;
 if(q.method==R243_ACTOR_FLOAT_ENEMY&&(!G_Cod1AiDomainRead(&f->domain,NULL,0)||!ActorFloatPlayers(f,1)))goto done;
 r243ActorFloatGameActive=f;
 if(q.method==COD1_ACTOR_MOTION_ANGLE){
  float angle;
  if(!r243_actor_float_game_fence(f)||!R243ActorFloatNativeBuffersCurrent(input,out,error,cap,current,context)||
     !r243_actor_float_game_scope(f)||!ActorCourseMotionAngle(f,&angle))goto done;
  memcpy(&f->reply.bits,&angle,sizeof(angle));*out=f->reply;ok=1;
  goto done;
 }
 if(q.method==R243_ACTOR_FLOAT_PATH){
  if(!r243_actor_float_game_fence(f)||!R243ActorFloatNativeBuffersCurrent(input,out,error,cap,current,context)||!r243_actor_float_game_scope(f))goto done;
  /* Original63010 tests only lowWORD actor6F4. No goal/path repair, no
   * cursor/segment reads, no enemy-domain or player readiness authority. */
  if((f->actor.defaults.pathWords[(0x6f4-0x374)/4]&UINT32_C(0xffff))!=0){if(error&&cap)snprintf(error,cap,"approxPathDist nonempty path storage/order owner HOLD");goto done;}
  f->reply.bits=UINT32_C(0);*out=f->reply;ok=1;goto done;
 }
 if(!r243_actor_float_game_fence(f)||
    !census->query(census->user,&f->censusRequest,&f->census,error,cap,r243_actor_float_game_fence,f)||
    !r243_actor_float_game_fence(f)||
    !R243ActorFloatNativeBuffersCurrent(input,out,error,cap,current,context)||
    !census->validate(census->user,&f->censusRequest,&f->census,error,cap,r243_actor_float_game_fence,f)||
    !r243_actor_float_game_scope(f))goto done;
 /* No callback after this final whole36 scan. Never filter an opponent by
  * missing/zero observation data, and never borrow an earlier pool epoch. */
 ownTeam=f->sentients[lease.sentientIndex].defaults.team;
 if(ownTeam!=1&&ownTeam!=2){if(error&&cap)snprintf(error,cap,"GetClosestEnemySqDist native no-AddFloat domain HOLD");goto done;}
 opposing=ownTeam==1?2u:1u;
 for(i=0;i<COD1_SENTIENT_CAPACITY;i++)if(f->sentients[i].activation>=0){active++;if(f->sentients[i].defaults.team==opposing)count++;}
 if(count){if(error&&cap)snprintf(error,cap,"GetClosestEnemySqDist opposing Sentient observation owner HOLD");goto done;}
 f->reply.bits=UINT32_C(0x4cbebc20);*out=f->reply;ok=1;
done:r243ActorFloatGameActive=NULL;free(f);return ok;
}
static const R243ActorFloatHost r243ActorFloatGameHost={NULL,r243_actor_float_game_validate,r243_actor_float_game_query};
const R243ActorFloatHost*R243GameActorFloatHost(void){return &r243ActorFloatGameHost;}
