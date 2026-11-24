#include "../bg_cod1_direct_fog.h"
/* GAME675A0/6A970, retained 261 wrapper oracle. Cache is per Actor lease
 * and server tick; even a range failure refreshes the muzzle, as in retail.
 * Raw CoD CM mask and real sentient teams; no RTCW mask translation. */
static int ActorMuzzle(C1ALCall*c,C1AMuzzle*out,char*error,size_t cap){
 C1AMuzzle next=cod1ActorMuzzles[c->lease.actorIndex];float matrix[16];
 if(!ActorLookFence(c))return 0;
 if(!next.valid||next.epoch!=c->entity.epoch||next.lease!=c->lease.serial||next.tick!=c->now){
  if(!ActorLookPreparePose(c)||!ActorLookTagMatrix(c,"tag_flash",matrix)||!ActorLookFence(c)){
   if(error&&cap)snprintf(error,cap,"canShoot: current Actor tag_flash unavailable");return 0;
  }
  memset(&next,0,sizeof(next));next.valid=1;next.epoch=c->entity.epoch;next.lease=c->lease.serial;next.tick=c->now;
  memcpy(next.origin,matrix+12,12);memcpy(next.forward,matrix,12);
  cod1ActorMuzzles[c->lease.actorIndex]=next;
 }
 *out=next;return 1;
}
/* GAME6A970 keeps all three subtractions on the PC53 x87 stack.
 * In particular, do not round each delta to binary32 before squaring. */
static int ActorCanShootRange(const float muzzle[3],const float offset[3],const float target[3],float limit,float start[3]){
 double d[3],squared;int i,result=0;
#if defined(_MSC_VER)&&defined(_M_IX86)
 unsigned char outer[108];uint32_t savedMx,nearestMx=0x1f80u;const unsigned short nearest=0x027f;
 __asm fnsave outer
 __asm stmxcsr savedMx
 __asm fldcw nearest
 __asm ldmxcsr nearestMx
#endif
 for(i=0;i<3;i++){start[i]=muzzle[i]+offset[i];d[i]=(double)start[i]-(double)target[i];}
 squared=(d[2]*d[2]+d[1]*d[1])+d[0]*d[0];result=squared<=(double)limit;
#if defined(_MSC_VER)&&defined(_M_IX86)
 __asm ldmxcsr savedMx
 __asm frstor outer
#endif
 return result;
}
static int ActorCanShoot(C1ALCall*c,const GscActorLookRequest*q,GscActorLookReply*out,char*error,size_t cap){
 C1AMuzzle muzzle;cod1SpatialTraceQuery_t query;trace_t tr;
 cod1DirectFogVisibility_t fog,afterFog;cod1ActorLease_t hitLease;GscEntityRef hit={0};
 float target[3],offset[3],start[3];int i,team,hitTeam=0,hitSentient=0,result=0;
 cod1ActorNativeData_t before=cod1Actors[c->lease.actorIndex].nativeData;gentity_t body=g_entities[c->entity.slot];
 team=cod1Sentients[c->lease.sentientIndex].defaults.team;
 if(!ActorMuzzle(c,&muzzle,error,cap)||!G_Cod1DirectFogVisibility(&fog)||!ActorLookFence(c))return 0;
 memcpy(target,q->vector_bits,12);memcpy(offset,q->scalar_bits,12);
 if(ActorCanShootRange(muzzle.origin,offset,target,fog.squared,start)){
  for(i=0;i<3;i++)if(!isfinite(start[i]))return 0;
  memset(&query,0,sizeof(query));query.version=COD1_SPATIAL_VERSION;query.bytes=sizeof(query);
  query.contentsMask=0x0200c061;query.passEntity[0]=c->entity.slot;query.passEntity[1]=ENTITYNUM_NONE;
  memcpy(query.start,start,12);memcpy(query.end,target,12);memset(&tr,0,sizeof(tr));
  if(trap_Cod1SpatialPointTrace(&query,&tr)!=COD1_SPATIAL_OK||!ActorLookFence(c)||!isfinite(tr.fraction)||tr.fraction<0||tr.fraction>1)return 0;
  if(tr.fraction==1)result=1;
  else if(!tr.startsolid&&tr.entityNum>=0&&tr.entityNum<ENTITYNUM_WORLD){
   if(!G_Cod1EntityCapture(g_entities+tr.entityNum,&hit))return 0;
   if(cod1SentientByEntity[hit.slot]){if(!TurretShotSentient(&hit,&hitLease,&hitTeam))return 0;hitSentient=1;result=hitTeam!=team;}
  }
 }
 if(!ActorLookFence(c)||memcmp(&before,&cod1Actors[c->lease.actorIndex].nativeData,sizeof(before))||
    memcmp(&body,g_entities+c->entity.slot,sizeof(body))||cod1Sentients[c->lease.sentientIndex].defaults.team!=(unsigned)team||
    !G_Cod1DirectFogVisibility(&afterFog)||memcmp(&fog,&afterFog,sizeof(fog))||
    (hit.epoch&&!G_Cod1EntitySameAllocation(&hit))||
    (hitSentient&&(!ActorLeaseValid(&hitLease)||cod1Sentients[hitLease.sentientIndex].defaults.team!=(unsigned)hitTeam)))return 0;
 out->kind=GSC_ACTOR_LOOK_RESULT_BOOL;out->boolean=result;return 1;
}
