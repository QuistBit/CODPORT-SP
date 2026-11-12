/* Original200678B0 ->2001BCA0 ->2001BB30 ->2001B980. Not a VM binding.
 * Initial link precedes ammo/tag/final throw. No extra link is invented. */
#include "../bg_cod1_drop_place.h"
#include "../bg_cod1_drop_throw.h"
#include "../g_cod1_actor_unmounted_tags.h"
#include "../g_cod1_turret_actor_tags.h"
static struct {
 int(*callerCurrent)(void*);void*callerUser;
 int active,cancelled,checking,now;uint64_t epoch,key;int(*current)(void*);void*user;
 const GscEntityRef*borrowed;GscEntityRef actor,item;cod1ActorLease_t lease;
 cod1ActorRecord_t record;gentity_t body;
 const char*nameSource,*handSource;char name[64],hand[64];size_t nameBytes,handBytes;
 unsigned definitionHash;int cwIndex;
} cod1ActorDrop;
static int DropActorStorage(const void*p,size_t n){return ActorOverlap(p,n,&cod1ActorDrop,sizeof(cod1ActorDrop));}
static int DropActorBusy(void){if(cod1ActorDrop.active){cod1ActorDrop.cancelled=1;return 1;}return 0;}
static void DropActorReset(void){if(cod1ActorDrop.active)cod1ActorDrop.cancelled=1;else memset(&cod1ActorDrop,0,sizeof(cod1ActorDrop));}
/* The native32 queue is complete only when every live dropped weapon is
 * owned by this producer. Legacy player drops outside that domain refuse;
 * they are not silently omitted from a supposedly complete native queue. */
static int DropActorQueueCovered(void){int i;for(i=MAX_CLIENTS;i<level.num_entities;i++){gentity_t*e=g_entities+i;GscEntityRef ref;if(!e->inuse||e->s.eType!=ET_ITEM||!e->item||e->item->giType!=IT_WEAPON||(!e->cod1DroppedAmmo&&!(e->flags&FL_DROPPED_ITEM)))continue;if(!G_Cod1EntityCapture(e,&ref)||!LiveItemRefEqual(&ref,&cod1Drops.rows[i].ref)||!DropBody(cod1Drops.rows+i))return 0;}return 1;}
static int DropActorScope(void){cod1ActorLease_t lease;const cwDef_t*d;
 if(!cod1ActorDrop.active)return 1;
 if(cod1ActorDrop.cancelled||cod1ActorDrop.epoch!=cod1EntityEpoch||level.time!=cod1ActorDrop.now||!DropMap()||cod1ActorDrop.key!=cod1Drops.key||cod1ActorDrop.current!=cod1Drops.current||cod1ActorDrop.user!=cod1Drops.user||!ActorGoalLease(&cod1ActorDrop.actor,&lease)||memcmp(&lease,&cod1ActorDrop.lease,sizeof(lease))||memcmp(&cod1ActorDrop.record,cod1Actors+lease.actorIndex,sizeof(cod1ActorDrop.record))||memcmp(&cod1ActorDrop.body,g_entities+lease.entity.slot,sizeof(gentity_t)))return 0;
 if(!ActorGoalBuffers(cod1ActorDrop.borrowed,sizeof(GscEntityRef),NULL,0,NULL,0)||memcmp(cod1ActorDrop.borrowed,&cod1ActorDrop.actor,sizeof(GscEntityRef))||!ActorGoalBuffers(cod1ActorDrop.nameSource,cod1ActorDrop.nameBytes,NULL,0,NULL,0)||memcmp(cod1ActorDrop.nameSource,cod1ActorDrop.name,cod1ActorDrop.nameBytes))return 0;
 if(cod1ActorDrop.handBytes&&(!ActorGoalBuffers(cod1ActorDrop.handSource,cod1ActorDrop.handBytes,NULL,0,NULL,0)||memcmp(cod1ActorDrop.handSource,cod1ActorDrop.hand,cod1ActorDrop.handBytes)))return 0;
 d=cod1ActorDrop.cwIndex<0?NULL:CW_Get(cod1ActorDrop.cwIndex);return cod1ActorDrop.cwIndex<0||(d&&d->hash==cod1ActorDrop.definitionHash&&DropActorQueueCovered());
}
static int DropActorFence(void*u){int ok;(void)u;if(!cod1ActorDrop.active||!DropActorScope()||cod1ActorDrop.checking){cod1ActorDrop.cancelled=1;return 0;}cod1ActorDrop.checking=1;ok=cod1ActorDrop.current(cod1ActorDrop.user)==1&&DropActorScope();if(ok&&cod1ActorDrop.callerCurrent)ok=cod1ActorDrop.callerCurrent(cod1ActorDrop.callerUser)==1&&DropActorScope();cod1ActorDrop.checking=0;return ok&&DropActorScope();}
static int DropActorProfileCurrent(void){return cod1ActorDrop.active?DropActorFence(NULL):cod1Drops.current&&cod1Drops.current(cod1Drops.user)==1;}
static int DropActorDraw(void*u,int*out){int value;(void)u;if(!out||!DropActorFence(NULL))return 0;value=rand();if(value<0||value>32767||!DropActorFence(NULL))return 0;*out=value;return 1;}
static int DropActorString(const char*input,char copy[64],size_t*bytes,GscEntityRef*out,char*error,size_t cap){size_t n;if(!input)return 0;for(n=0;n<64&&input[n];n++){}if(n==64||!n||!DropBuffers(input,n+1,out,sizeof(*out),error,cap)||!ActorGoalBuffers(input,n+1,out,sizeof(*out),error,cap))return 0;memcpy(copy,input,n+1);*bytes=n+1;return 1;}
/* Original DropWeapon calls engine83 on a real Actor after collision is
 * enabled. The old R125 public query intentionally admits only provisional
 * hidden/no-contents entities; do not extend that unrelated API silently. */
static int DropActorBone(const char*tag,cod1DObjQueryResult_t*out){cod1DObjQuery_t q,before;cod1DObjQueryResult_t result;const cod1DObjSummary_t*summary=&cod1ActorDrop.record.dobj.summary;int status;
 if(!tag||!out||!DropActorFence(NULL)||!cod1ActorDrop.record.dobj.present||cod1ActorDrop.record.dobj.busy||cod1ActorDrop.record.modelBusy||cod1ActorDrop.record.modelMethodBusy||!ActorDObjSummaryValid(summary,summary->modelCount+summary->skippedCount)||strlen(tag)>=sizeof(q.tag))return 0;
 memset(&q,0,sizeof(q));memset(&result,0,sizeof(result));q.version=COD1_DOBJ_QUERY_VERSION;q.bytes=sizeof(q);q.key=ActorSpatialKey(&cod1ActorDrop.lease);q.topologySerial=summary->topologySerial;q.phase=COD1_DOBJ_QUERY_BONE;strcpy(q.tag,tag);before=q;status=trap_Cod1DObjQuery(&q,&result);
 if(status!=COD1_DOBJ_OK||memcmp(&q,&before,sizeof(q))||!DropActorFence(NULL)||result.version!=COD1_DOBJ_QUERY_VERSION||result.bytes!=sizeof(result)||result.topologySerial!=summary->topologySerial||result.modelCount!=summary->modelCount||result.boneCount!=summary->boneCount||!memchr(result.model,0,sizeof(result.model))||Q_stricmpn(result.model,"xmodel/",7)||!result.model[7]||result.boneIndex< -1||result.boneIndex>=(int32_t)summary->boneCount)return 0;*out=result;return 1;
}
static int DropActorFinalize(const cod1DroppedItemRequest_t*q){C1DroppedRow*r;gentity_t*e;GscEntityRef ref=cod1ActorDrop.item;int i,ok=0;const cwDef_t*d=CW_Get(q->cwIndex);
 if(!d||!DropActorFence(NULL)||ref.slot>=MAX_GENTITIES||q->cwIndex!=cod1ActorDrop.cwIndex||q->clip< -1||q->reserve< -1||q->clip>(int)CW_Number(d,"clipSize")||q->reserve>(int)CW_Number(d,"maxAmmo"))return 0;
 r=cod1Drops.rows+ref.slot;if(!LiveItemRefEqual(&ref,&r->ref)||!DropBody(r)||!DropBegin(&ref))return 0;e=g_entities+ref.slot;
 if(!DropModelCurrent(r)||!DropFence(NULL)||!DropBody(r))goto done;
 r->reserve=q->reserve;r->clip=q->clip;e->count=q->reserve;e->cod1DropClip=q->clip<0?0:q->clip;e->cod1DropReserve=q->reserve<0?0:q->reserve;
 for(i=0;i<3;i++){r->state.origin[i]=r->state.base[i]=q->origin[i];r->state.delta[i]=q->velocity[i];r->state.angles[i]=r->state.angularBase[i]=q->angles[i];r->state.angularDelta[i]=q->angular?q->angularDelta[i]:0;}
 r->state.time=level.time;r->state.angularKind=q->angular?2:0;r->state.angularTime=q->angular?level.time:0;DropApply(r,&r->state);ok=DropBody(r)&&DropFence(NULL);
 done:DropEnd();return ok;
}
static int DropActorInvoke(const GscEntityRef*input,const char*name,const char*hand,float speed,GscEntityRef*out,char*error,size_t cap,int(*callerCurrent)(void*),void*callerUser){
 GscEntityRef actor,ref={0};cod1ActorLease_t lease;cod1DroppedItemRequest_t q;c1daPlan ammo;c1dtPlan thrown;cod1DObjQueryResult_t bone;cod1SpatialTraceQuery_t traceQuery,traceBefore;trace_t trace;const cwDef_t*def;const char*tag=NULL;char tagName[COD1_DOBJ_NAME_MAX];float yaw,center[3],matrix[16];int draw,i,ok=0;const char*stage="arguments";C1GARecord*graph;double lo,hi,clip;
 if(cod1ActorDrop.active){cod1ActorDrop.cancelled=1;return 0;}
 if(!input||!out||!DropMap()||cod1DropCall.busy||!DropBuffers(input,sizeof(*input),out,sizeof(*out),error,cap)||!ActorGoalBuffers(input,sizeof(*input),out,sizeof(*out),error,cap))return 0;
 actor=*input;if(!ActorGoalLease(&actor,&lease))return 0;memset(&cod1ActorDrop,0,sizeof(cod1ActorDrop));cod1ActorDrop.cwIndex=-1;
 if(!DropActorString(name,cod1ActorDrop.name,&cod1ActorDrop.nameBytes,out,error,cap))goto done;
 cod1ActorDrop.callerCurrent=callerCurrent;cod1ActorDrop.callerUser=callerUser;cod1ActorDrop.active=1;cod1ActorDrop.actor=actor;cod1ActorDrop.borrowed=input;cod1ActorDrop.lease=lease;cod1ActorDrop.record=cod1Actors[lease.actorIndex];cod1ActorDrop.body=g_entities[actor.slot];cod1ActorDrop.epoch=cod1EntityEpoch;cod1ActorDrop.key=cod1Drops.key;cod1ActorDrop.current=cod1Drops.current;cod1ActorDrop.user=cod1Drops.user;cod1ActorDrop.now=level.time;cod1ActorDrop.nameSource=name;
 if(!DropActorFence(NULL))goto done;
 if(!Q_stricmp(cod1ActorDrop.name,"none")){*out=ref;ok=1;goto done;}
 if(!DropActorString(hand,cod1ActorDrop.hand,&cod1ActorDrop.handBytes,out,error,cap)||!isfinite(speed)||fabsf(speed)>1000000)goto done;cod1ActorDrop.handSource=hand;
 stage="existing weapon lookup";i=CW_Find(cod1ActorDrop.name);def=CW_Get(i);if(i<0||!def)goto done;stage="dropped weapon queue coverage";cod1ActorDrop.cwIndex=i;cod1ActorDrop.definitionHash=def->hash;if(!DropActorFence(NULL))goto done;
 memset(&q,0,sizeof(q));q.cwIndex=i;memcpy(q.origin,cod1ActorDrop.body.r.currentOrigin,12);q.origin[2]=(float)(((double)cod1ActorDrop.body.r.maxs[2]-cod1ActorDrop.body.r.mins[2])*0.5+q.origin[2]);
 yaw=(float)((double)cod1ActorDrop.body.r.currentAngles[1]*0.01745329238474369f);q.velocity[0]=(float)((double)(float)cos(yaw)*150.0);q.velocity[1]=(float)((double)(float)sin(yaw)*150.0);
 if(!DropActorDraw(NULL,&draw))goto done;q.velocity[2]=(float)(((double)draw/32768.0*2.0-1.0)*50.0+200.0);
 stage="registered item creation";if(!G_Cod1DroppedItemSpawn(&q,&ref,error,cap))goto done;cod1ActorDrop.item=ref;
 stage="original ammo distribution";lo=CW_Number(def,"dropAmmoMin");hi=CW_Number(def,"dropAmmoMax");clip=CW_Number(def,"clipSize");if(!isfinite(lo)||!isfinite(hi)||!isfinite(clip)||lo<INT_MIN||lo>INT_MAX||hi<INT_MIN||hi>INT_MAX||clip<0||clip>1024||floor(lo)!=lo||floor(hi)!=hi||floor(clip)!=clip||!C1DA_Plan((int)lo,(int)hi,(int)clip,DropActorDraw,NULL,&ammo))goto done;q.reserve=ammo.reserve;q.clip=ammo.clip;
 if(!strcmp(cod1ActorDrop.hand,"left"))tag="tag_weapon_left";else if(!strcmp(cod1ActorDrop.hand,"right"))tag="tag_weapon_right";
 if(tag){
  stage="current Actor DObj tag query";memset(tagName,0,sizeof(tagName));strcpy(tagName,tag);if(!DropActorBone(tagName,&bone)||!DropActorFence(NULL))goto done;
  if(bone.boneIndex>=0){stage="current Actor animation pose";
   if(cod1ActorDrop.record.turretMount.mounted){if(!G_Cod1TurretActorTagMatrix(&actor,tag,level.time,matrix,DropActorFence,NULL))goto done;}
   else{graph=cod1GenericActors+lease.actorIndex;if(!graph->used||graph->tick>level.time)goto done;if(graph->tick<level.time&&!G_Cod1ActorGenericFrame(&actor,level.time,DropActorFence,NULL))goto done;if(!G_Cod1ActorUnmountedTagMatrix(&actor,tag,level.time,matrix,DropActorFence,NULL))goto done;}
   if(!DropActorFence(NULL)||!C1DP_Center(cod1ActorDrop.body.r.currentOrigin,cod1ActorDrop.body.r.mins,cod1ActorDrop.body.r.maxs,center))goto done;
   memset(&traceQuery,0,sizeof(traceQuery));traceQuery.version=COD1_SPATIAL_VERSION;traceQuery.bytes=sizeof(traceQuery);memcpy(traceQuery.start,center,12);memcpy(traceQuery.end,matrix+12,12);for(i=0;i<3;i++){traceQuery.mins[i]=-1;traceQuery.maxs[i]=1;}traceQuery.passEntity[0]=actor.slot;traceQuery.passEntity[1]=ENTITYNUM_NONE;traceQuery.contentsMask=0x411;
   stage="tag collision trace";traceBefore=traceQuery;if(trap_Cod1SpatialCapsuleTrace(&traceQuery,&trace)!=COD1_SPATIAL_OK||!DropActorFence(NULL)||memcmp(&traceQuery,&traceBefore,sizeof(traceQuery))||!isfinite(trace.fraction)||trace.fraction<0||trace.fraction>1||!LiveItemFinite(trace.endpos))goto done;memcpy(q.origin,trace.endpos,12);
  }
  stage="angular randomization";memcpy(q.angles,cod1ActorDrop.body.r.currentAngles,12);q.angular=1;if(!C1DP_Angular(DropActorDraw,NULL,q.angularDelta))goto done;
 }
 if(!DropActorFence(NULL)||!C1DT_Plan(cod1ActorDrop.body.r.currentOrigin,cod1ActorDrop.body.r.mins[2],cod1ActorDrop.body.r.maxs[2],q.origin,speed,level.time,&thrown))goto done;memcpy(q.velocity,thrown.velocity,12);
 stage="final throw publication";if(!DropActorFinalize(&q)||!DropActorFence(NULL)||!DropBuffers(input,sizeof(*input),out,sizeof(*out),error,cap)||memcmp(input,&actor,sizeof(actor)))goto done;*out=ref;ok=1;
 done:if(!ok&&ref.epoch&&G_Cod1EntitySameAllocation(&ref)&&LiveItemRefEqual(&ref,&cod1Drops.rows[ref.slot].ref)){gentity_t*e=g_entities+ref.slot;if(!e->client&&!e->r.bmodel&&!e->neverFree&&e->s.eType==ET_ITEM&&e->classname==cod1Drops.rows[ref.slot].classname){G_FreeEntity(e);if(!G_Cod1EntitySameAllocation(&ref))memset(cod1Drops.rows+ref.slot,0,sizeof(C1DroppedRow));}}
 if(!ok&&error&&cap)snprintf(error,cap,"Actor DropWeapon refused: %s",stage);memset(&cod1ActorDrop,0,sizeof(cod1ActorDrop));return ok;
}

int G_Cod1ActorDropWeaponChecked(const GscEntityRef*a,const char*n,const char*h,float speed,GscEntityRef*out,char*error,size_t cap,int(*current)(void*),void*user){if(!current)return 0;return DropActorInvoke(a,n,h,speed,out,error,cap,current,user);}
int G_Cod1ActorDropWeapon(const GscEntityRef*a,const char*n,const char*h,float speed,GscEntityRef*out,char*error,size_t cap){return DropActorInvoke(a,n,h,speed,out,error,cap,NULL,NULL);}
