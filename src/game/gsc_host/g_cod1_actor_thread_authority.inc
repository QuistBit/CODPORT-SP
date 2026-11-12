#include "../g_cod1_actor_thread_authority.h"
#include "../g_cod1_spawner.h"
/* Native spawner is a real class0 entity object. Its own intake identity,
 * not an Actor lease or a classname, owns a source-thread receiver. */
static int SpawnerThreadCurrent(const GscEntityRef*ref,uint64_t intake){
 return ref&&ref->slot<MAX_GENTITIES&&intake&&cod1EntityReady&&
  !level.spawning&&!level.restarted&&ActorMapAvailable()&&G_Cod1EntitySameAllocation(ref)&&
  !cod1ActorByEntity[ref->slot]&&!cod1SentientByEntity[ref->slot]&&
  G_Cod1SpawnerIntakeIdentity(ref)==intake;
}
static int ActorThreadResolve(void*user,const GscEntityRef*input,
 GscEntityExecutionCertificate*out,char*error,size_t size){
 GscEntityRef ref;GscEntityExecutionCertificate certificate;unsigned index;uint64_t serial;
 (void)user;
 if(!input||!out||!ActorGoalBuffers(input,sizeof(*input),out,sizeof(*out),error,size)||
    ActorOverlap(input,sizeof(*input),G_Cod1ActorThreadAuthorityHost(),sizeof(GscThreadEntityAuthorityHost))||
    ActorOverlap(out,sizeof(*out),G_Cod1ActorThreadAuthorityHost(),sizeof(GscThreadEntityAuthorityHost))||
    ActorOverlap(error,size,G_Cod1ActorThreadAuthorityHost(),sizeof(GscThreadEntityAuthorityHost)))return 0;
 ref=*input;
 if(!cod1EntityReady||level.spawning||level.restarted||!ActorMapAvailable()||
    !G_Cod1EntitySameAllocation(&ref)||ref.slot>=MAX_GENTITIES)return 0;
 if(!cod1ActorByEntity[ref.slot]){
  serial=G_Cod1SpawnerIntakeIdentity(&ref);if(!SpawnerThreadCurrent(&ref,serial))return 0;
  memset(&certificate,0,sizeof(certificate));certificate.owner=ref;certificate.serial=serial;
  certificate.current=SpawnerThreadCurrent;*out=certificate;return 1;
 }
 index=cod1ActorByEntity[ref.slot]-1;if(index>=COD1_ACTOR_CAPACITY)return 0;
 serial=cod1Actors[index].serial;if(!serial||!ActorInheritedCertificate(&ref,serial))return 0;
 memset(&certificate,0,sizeof(certificate));certificate.owner=ref;
 certificate.serial=serial;certificate.current=ActorInheritedCertificate;
 /* No host callback, mutation, aitype replay or search through VM tasks. */
 *out=certificate;return 1;
}
void G_Cod1BindReadOnlyActor(GscReadOnlyContext *context){
 if(context)context->actor_current=ActorInheritedCertificate;
}
const GscThreadEntityAuthorityHost *G_Cod1ActorThreadAuthorityHost(void){
 static const GscThreadEntityAuthorityHost host={NULL,ActorThreadResolve};return &host;
}
