#include "../../qcommon/cod1_actor_may_move14.h"
/* Read-only hit projection: no callbacks and no borrowed actor pointers.
 * Compare again after Current, which may retire/reuse or mutate a hit owner. */
typedef struct {GscEntityRef entity;cod1ActorLease_t actor,chain;Cod1ObservationTarget avoidance;unsigned sentientIndex;int sentient,category,pass;} C1MayMoveHit;
static int ActorMayMoveHit(int slot,int pass,C1MayMoveHit*out){
 C1MayMoveHit v;unsigned si;GscEntityRef empty={0};
 memset(&v,0,sizeof(v));
 if(slot<0||slot>=ENTITYNUM_WORLD||!G_Cod1EntityCapture(g_entities+slot,&v.entity))return 0;
 si=cod1SentientByEntity[slot];
 if(si){const cod1SentientRecord_t*sent;
  if(si>COD1_SENTIENT_CAPACITY)return 0;sent=cod1Sentients+si-1;
  if(sent->activation<0||memcmp(&sent->owner,&v.entity,sizeof(v.entity)))return 0;
  v.sentient=1;v.sentientIndex=si;
  if(cod1ActorByEntity[slot]){const cod1ActorRecord_t*hit;
   if(!ActorGoalLease(&v.entity,&v.actor))return 0;hit=cod1Actors+v.actor.actorIndex;
   v.category=hit->scriptedController.currentCategory;v.avoidance=hit->avoidance778;
   if(v.category==1)v.pass=1;
   else if(v.avoidance.serial||memcmp(&v.avoidance.owner,&empty,sizeof(empty))){
    if(!v.avoidance.serial||!ActorGoalLease(&v.avoidance.owner,&v.chain)||v.chain.serial!=v.avoidance.serial)return 0;
    v.pass=v.avoidance.owner.slot==(unsigned)pass;
   }
  }
 }
 *out=v;return 1;
}
typedef struct {
 C1ALCall*call;cod1ActorRecord_t actor;gentity_t entity;
 cod1SentientRecord_t sentients[COD1_SENTIENT_CAPACITY];C1GARecord graph;
 int queryCount;unsigned hitCount;C1MayMoveHit hits[COD1_ACTOR_CAPACITY+1];
} C1MayMoveCall;
static int ActorMayMoveCurrent(C1MayMoveCall*m){
 C1ALCall*c=m->call;unsigned i;C1MayMoveHit view;
 if(!(ActorLookFence(c)&&!memcmp(&m->actor,cod1Actors+c->lease.actorIndex,sizeof(m->actor))&&
  !memcmp(&m->entity,g_entities+c->entity.slot,sizeof(m->entity))&&
  !memcmp(m->sentients,cod1Sentients,sizeof(m->sentients))&&
  !memcmp(&m->graph,cod1GenericActors+c->lease.actorIndex,sizeof(m->graph))))return 0;
 for(i=0;i<m->hitCount;i++)if(!ActorMayMoveHit((int)m->hits[i].entity.slot,(int)c->entity.slot,&view)||memcmp(&view,m->hits+i,sizeof(view)))return 0;
 return 1;
}
static int ActorMayMoveQuery(void*ctx,int capsule,const float*start,const float*end,const float*mins,const float*maxs,int pass,uint32_t mask,c1mmp14Trace_t*out){
 C1MayMoveCall*m=ctx;cod1SpatialTraceQuery_t q;trace_t tr;int status,hit=0;c1mmp14Trace_t v;C1MayMoveHit before,after;
 if(pass!=(int)m->call->entity.slot||!ActorMayMoveCurrent(m))return 0;
 memset(&q,0,sizeof(q));q.version=COD1_SPATIAL_VERSION;q.bytes=sizeof(q);
 memcpy(q.start,start,12);memcpy(q.end,end,12);memcpy(q.mins,mins,12);memcpy(q.maxs,maxs,12);
 q.passEntity[0]=pass;q.passEntity[1]=ENTITYNUM_NONE;q.contentsMask=mask;memset(&tr,0,sizeof(tr));
 status=capsule?trap_Cod1SpatialCapsuleTrace(&q,&tr):trap_Cod1SpatialBoxTrace(&q,&tr);
 ++m->queryCount;
 if(status!=COD1_SPATIAL_OK)G_Printf("[417 maymove query] owner=%u tick=%d status=%d capsule=%d mask=%08x start=%.9g,%.9g,%.9g end=%.9g,%.9g,%.9g\n",m->call->entity.slot,m->call->now,status,capsule,mask,start[0],start[1],start[2],end[0],end[1],end[2]);
 if(status!=COD1_SPATIAL_OK)return 0;
 hit=(tr.allsolid||tr.fraction<1)&&tr.entityNum>=0&&tr.entityNum<ENTITYNUM_WORLD;
 if(hit&&!ActorMayMoveHit(tr.entityNum,pass,&before))return 0;
 if(!ActorMayMoveCurrent(m))return 0;
 if(hit&&(!ActorMayMoveHit(tr.entityNum,pass,&after)||memcmp(&before,&after,sizeof(before))))return 0;
 if(hit&&before.sentient){unsigned i;for(i=0;i<m->hitCount;i++)if(m->hits[i].entity.slot==before.entity.slot)break;
  if(i==m->hitCount){if(i>=COD1_ACTOR_CAPACITY+1)return 0;m->hits[m->hitCount++]=before;}
  else if(memcmp(m->hits+i,&before,sizeof(before)))return 0;
 }
 memset(&v,0,sizeof(v));v.fraction=tr.fraction;memcpy(v.end,tr.endpos,12);memcpy(v.normal,tr.plane.normal,12);
 v.allsolid=tr.allsolid;v.startsolid=tr.startsolid;
 if(hit){v.sentient=before.sentient;v.sentientPass=before.pass;
  if(v.sentient&&g_debugMove.integer>1)G_Printf("[418 maymove sentient] owner=%u tick=%d hit=%d category=%d pass=%d fraction=%.7g\n",m->call->entity.slot,m->call->now,tr.entityNum,before.category,before.pass,tr.fraction);
 }
 *out=v;return 1;
}
static int ActorMayMoveToPoint(C1ALCall*c,const GscActorLookRequest*q,GscActorLookReply*out,char*error,size_t cap){
 C1MayMoveCall*m;GscActorLookRequest canonical;const cod1SentientRecord_t*self;GscEntityRef empty={0};
 unsigned i;int ok=0,value=-1,phase=1;float target[3];uint32_t mask;
 /* Exact one-vector command lane; no residual animation/tag/argument data. */
 memset(&canonical,0,sizeof(canonical));canonical.entity=q->entity;canonical.method=GSC_ACTOR_LOOK_MAY_MOVE;
 canonical.runtime_identity=q->runtime_identity;canonical.catalog_identity=q->catalog_identity;
 canonical.now_ms=q->now_ms;canonical.precision_bits=53;memcpy(canonical.vector_bits,q->vector_bits,12);
 if(memcmp(q,&canonical,sizeof(canonical)))return 0;
 m=calloc(1,sizeof(*m));if(!m)return 0;m->call=c;m->actor=cod1Actors[c->lease.actorIndex];m->entity=g_entities[c->entity.slot];
 memcpy(m->sentients,cod1Sentients,sizeof(m->sentients));m->graph=cod1GenericActors[c->lease.actorIndex];
 cod1ActorLookOperation.stage=m;cod1ActorLookOperation.bytes=sizeof(*m);self=m->sentients+c->lease.sentientIndex;
 if(!m->graph.used||m->graph.runtime!=q->runtime_identity||m->graph.catalog!=q->catalog_identity||
    memcmp(&m->graph.lease,&c->lease,sizeof(c->lease))||memcmp(m->actor.nativeData.entity.origin,m->entity.r.currentOrigin,12)||
    !ActorMayMoveCurrent(m))goto done;
 phase=2;
 /* Native73D90/73DD0 scan all36 active same-team sentients, excluding self.
  * Their enemy absence must be known from the real allocation/lifetime owner.
  * Present-enemy crossing geometry is not silently omitted. */
 for(i=0;i<COD1_SENTIENT_CAPACITY;i++){
  const cod1SentientRecord_t*s=m->sentients+i;
  if(i==c->lease.sentientIndex||s->activation<0||((s->defaults.team^self->defaults.team)&31u))continue;
  if(!s->enemyKnown||memcmp(&s->enemy,&empty,sizeof(empty))||!G_Cod1EntitySameAllocation(&s->owner)){
   EntityError(error,cap,"mayMoveToPoint same-team enemy segment not yet qualified");goto done;
  }
 }
 phase=3;memcpy(target,q->vector_bits,12);mask=m->actor.defaults.lifeWords[(0x2d0-0x26c)/4];
 if(!C1MMP14_Apply(m->entity.r.currentOrigin,self->rawGoal.position,self->defaults.goalradiusSquared,target,
    (int)c->entity.slot,mask,ActorMayMoveQuery,m,&value)||!ActorMayMoveCurrent(m))goto done;
 out->kind=GSC_ACTOR_LOOK_RESULT_BOOL;out->boolean=value;ok=1;
 if(g_debugMove.integer>1)G_Printf("[412 mayMoveToPoint] owner=%u tick=%d result=%d traces=%d target=%.9g %.9g %.9g\n",c->entity.slot,c->now,value,m->queryCount,target[0],target[1],target[2]);
done:if(!ok&&error&&cap&&!error[0])snprintf(error,cap,"mayMoveToPoint phase=%d queries=%d origin_same=%d owner=%u",phase,m->queryCount,!memcmp(m->actor.nativeData.entity.origin,m->entity.r.currentOrigin,12),c->entity.slot);cod1ActorLookOperation.stage=NULL;cod1ActorLookOperation.bytes=0;free(m);return ok;
}
