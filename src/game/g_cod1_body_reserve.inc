/* Sole owner g_client.c. These are RTCW adapter reservations, NOT native
 * CoD entities or a source of native flags/contents. Only the normal trace
 * cohort may account for an exact, still-unused unlinked reservation.
 * Native path/world/actor APIs continue to refuse it. */
#if defined(COD1_GSC_TEST_WIP) && COD1_GSC_TEST_WIP
#include "g_cod1_entity_host.h"
#include "g_cod1_actor.h"
typedef struct {
 GscEntityRef ref;gentity_t initial;int present,consumed;
} C1BodyReserve;
static C1BodyReserve cod1BodyReserves[BODY_QUEUE_SIZE];
static void Cod1BodyReserveBorn(int index,gentity_t*e){
 C1BodyReserve*r;GscEntityRef ref;
 if(index<0||index>=BODY_QUEUE_SIZE||!e||level.spawning||
    level.bodyQue[index]!=e||!G_Cod1EntityCapture(e,&ref))return;
 r=cod1BodyReserves+index;
 /* Same-map reinvocation is not a new startup authority. */
 if(r->ref.epoch==ref.epoch){r->present=0;r->consumed=1;return;}
 memset(r,0,sizeof(*r));r->ref=ref;
 if(e->client||e->r.linked||e->r.contents||e->s.eType!=ET_GENERAL||
    e->s.modelindex||!e->neverFree||!e->classname||strcmp(e->classname,"bodyque")||
    e->think||e->touch||e->use||e->die||e->nextthink||e->takedamage)return;
 r->initial=*e;r->present=1;
}
static void Cod1BodyReserveConsume(gentity_t*e){
 int i;GscEntityRef ref;if(!G_Cod1EntityCapture(e,&ref))return;
 for(i=0;i<BODY_QUEUE_SIZE;i++){
  C1BodyReserve*r=cod1BodyReserves+i;
  if(r->ref.epoch==ref.epoch&&r->ref.generation==ref.generation&&r->ref.slot==ref.slot){
   r->present=0;r->consumed=1;
  }
 }
}
int G_Cod1BodyReserveCurrent(const GscEntityRef*ref){
 int i;gentity_t*e;uint64_t actor;char error[128];
 if(!ref||!G_Cod1EntitySameAllocation(ref))return 0;e=g_entities+ref->slot;
 for(i=0;i<BODY_QUEUE_SIZE;i++){
  C1BodyReserve*r=cod1BodyReserves+i;
  if(r->ref.epoch!=ref->epoch||r->ref.generation!=ref->generation||r->ref.slot!=ref->slot)continue;
  if(!r->present||r->consumed||level.bodyQue[i]!=e||memcmp(e,&r->initial,sizeof(*e)))return 0;
  if(!G_Cod1ActorPresenceRead(ref,&actor,error,sizeof(error))||actor)return 0;
  return 1;
 }
 return 0;
}
#endif
