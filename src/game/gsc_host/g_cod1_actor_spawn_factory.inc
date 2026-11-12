/* Genuine current force1 factory. Full hostile AI/deferred INIT/named target
 * remain unsupported; no public caller can assert native-tail completion. */
#include "../g_cod1_spawner.h"
extern vmCvar_t g_cod1AIDisableSpawn,g_developer;
typedef struct {
 int phase,busy,cancelled;GscSession*session;uint64_t id,program,intake,operation;
 GscActorSpawnRequest request;cod1ActorLease_t lease;GscEntityRef allocation;
} C1ActorFactory;
static C1ActorFactory cod1ActorFactory;
static uint64_t cod1ActorFactorySerial;
static int ActorFactoryPrivate(const void*p,size_t n){return
 ActorOverlap(p,n,&cod1ActorFactory,sizeof(cod1ActorFactory))||
 ActorOverlap(p,n,&cod1ActorFactorySerial,sizeof(cod1ActorFactorySerial))||
 ActorOverlap(p,n,&g_cod1AIDisableSpawn,sizeof(g_cod1AIDisableSpawn))||
 ActorOverlap(p,n,&g_developer,sizeof(g_developer));}
static void ActorFactoryReset(void){
 if(cod1ActorFactory.busy)cod1ActorFactory.cancelled=1;
 else memset(&cod1ActorFactory,0,sizeof(cod1ActorFactory));
 /* Serial is process-monotonic; map reset cannot revive a consumed ticket. */
}
static int ActorFactoryPrepareCurrent(C1ActorFactory*f){return f==&cod1ActorFactory&&
 f->busy&&f->phase==1&&!f->cancelled&&ActorMapAvailable()&&cod1EntityReady&&
 !level.spawning&&!level.restarted&&level.time>=0&&
 G_Cod1SpawnerIntakeIdentity(&f->request.spawner)==f->intake&&
 gsc_session_spawn_prepare_program(f->session,f->id,&f->request)==f->program&&
 (f->lease.serial?
  gsc_session_spawn_issued_owner_program(f->session,f->id,&f->request.spawner,&f->lease.entity,f->lease.serial):
  gsc_session_spawn_entry_program(f->session,f->id,&f->request.spawner))==f->program;}
static void ActorFactoryTicket(const C1ActorFactory*f,GscActorSpawnTicket*t){
 memset(t,0,sizeof(*t));t->operation=f->operation;t->spawner=f->request.spawner;t->actor=f->allocation;
}
int G_Cod1ActorFactoryPrepareSession(GscSession*s,uint64_t id,const GscActorSpawnRequest*input,
 GscActorSpawnTicket*out,int*created,char*error,size_t cap){
 C1ActorFactory*f=&cod1ActorFactory;GscActorSpawnRequest q;cod1SpawnerSnapshot_t source;
 GscStringCastValue count;cod1ActorDynamicPrefixRequest_t request;cod1ActorSpawnPrefixResult_t born;
 GscEntityScriptRun run;cod1ActorLease_t lease;uint64_t program,intake;int ok=0,now;
 if(!s||!input||!out||!created||!ActorGoalBuffers(input,sizeof(*input),out,sizeof(*out),error,cap)||
    ActorGoalPrivate(created,sizeof(*created))||ActorOverlap(created,sizeof(*created),input,sizeof(*input))||
    ActorOverlap(created,sizeof(*created),out,sizeof(*out))||ActorOverlap(created,sizeof(*created),error,cap)||
    !gsc_session_spawn_entry_storage_separate(s,input,sizeof(*input))||
    !gsc_session_spawn_entry_storage_separate(s,out,sizeof(*out))||
    !gsc_session_spawn_entry_storage_separate(s,created,sizeof(*created))||
    !gsc_session_spawn_entry_storage_separate(s,error,cap))return 0;
 q=*input;memset(out,0,sizeof(*out));*created=0;memset(&born,0,sizeof(born));
 if(f->busy||f->phase||!ActorMapAvailable()||!cod1EntityReady||level.spawning||level.restarted||level.time<0||
    q.wrapper<GSC_ACTOR_SPAWN_DOSPAWN||q.wrapper>GSC_ACTOR_SPAWN_STALINGRAD||q.arg_count<0||q.arg_count>16||
    !(program=gsc_session_spawn_prepare_program(s,id,&q))||
    gsc_session_spawn_entry_program(s,id,&q.spawner)!=program||
    !(intake=G_Cod1SpawnerIntakeIdentity(&q.spawner))||!G_Cod1SpawnerRead(&q.spawner,&source,error,cap))goto refused;
 /* Actual wrapper time gate PRECEDES optional getter/type validation. */
 if(source.lastSpawnTime>=level.time)return 1;
 memset(&request,0,sizeof(request));request.spawner=q.spawner;
 if(q.arg_count==1){
  if(q.single_kind!=GSC_ACTOR_SPAWN_STRING||!memchr(q.single_string,0,sizeof(q.single_string))){
   EntityError(error,cap,"spawn optional getter requires its one string argument");return 0;}
  request.targetname.present=1;memcpy(request.targetname.text,q.single_string,sizeof(q.single_string));
 }
 if(g_cod1AIDisableSpawn.integer){
  /* Original6AD50, before live count and force. Native1E640 reads the
   * genuine cached developer cvar; even a callback changing disableSpawn
   * cannot resume this selected branch into construction. */
  memset(f,0,sizeof(*f));f->busy=1;f->phase=1;f->session=s;f->id=id;
  f->program=program;f->intake=intake;f->request=q;
  now=level.time;G_DPrintf("Attempted spawn prevented by ai_disableSpawn.\n");
  ok=level.time==now&&ActorFactoryPrepareCurrent(f);
  /* No allocation or ticket exists on this lane. A reset during print marks
   * cancellation; nested public prepare refuses while busy. Always unwind. */
  memset(f,0,sizeof(*f));if(!ok)goto refused;
  return 1; /* Genuine native NULL/undefined, no ticket/count/clock effects. */
 }
 if(!G_Cod1SpawnerCountRead(&q.spawner,&count,error,cap)||count.type!=5)return 0;
 if(!count.integer)return 1; /* Native count==0; negative count remains eligible. */
 if(q.wrapper!=GSC_ACTOR_SPAWN_STALINGRAD){EntityError(error,cap,"dospawn force0 visibility gate remains unsupported");return 0;}
 if(cod1ActorFactorySerial==UINT64_MAX)goto refused;
 memset(f,0,sizeof(*f));f->busy=1;f->phase=1;f->session=s;f->id=id;f->program=program;
 f->intake=intake;f->request=q;f->operation=++cod1ActorFactorySerial;
 /* Fresh native destination A0/134 are zero from real birth; native411F0
  * static rows exclude them and current private-only VM inheritance cannot
  * change them. Do not copy source flags or portable r.svFlags encoding. */
 if(!ActorFactoryPrepareCurrent(f)||!G_Cod1ActorAllocateAitypePrefixSession(s,id,&request,&born,error,cap))goto failed;
 f->allocation=born.allocation;f->lease=born.prefix.inherited.lease;lease=f->lease;
 if(!ActorFactoryPrepareCurrent(f)||born.prefix.inherited.entry.status!=GSC_ENTITY_ENTRY_RETURNED||
    !ActorLeaseValid(&f->lease)||!G_Cod1ActorSpawnPostinitPositionSession(s,id,&q.spawner,&lease)||
    !ActorFactoryPrepareCurrent(f)||!G_Cod1ActorSpawnPostinitSession(s,id,&q.spawner,&lease,&run,error,cap)||
    run.status!=GSC_ENTITY_ENTRY_RETURNED||!ActorFactoryPrepareCurrent(f))goto failed;
 /* The real source97 native tail returned. This receipt is internal and
  * tied to this operation; it never marks Actor initialized or emits notify. */
 f->phase=2;f->busy=0;ActorFactoryTicket(f,out);*created=1;return 1;
 failed:
 if(born.allocated&&G_Cod1EntitySameAllocation(&born.allocation))G_FreeEntity(g_entities+born.allocation.slot);
 if(born.allocated&&G_Cod1EntitySameAllocation(&born.allocation)){
  f->allocation=born.allocation;f->lease=born.prefix.inherited.lease;f->phase=3;f->busy=0;ActorFactoryTicket(f,out);
 }else memset(f,0,sizeof(*f));
 return 0;
 refused:EntityError(error,cap,"spawn requires a current unused own factory operation/source/Session");return ok;
}
int G_Cod1ActorFactoryCommitCurrent(const GscActorSpawnTicket*t,uint64_t intake){
 C1ActorFactory*f=&cod1ActorFactory;GscActorSpawnTicket expected;
 if(!t||f->phase!=4||!f->busy||f->cancelled||!cod1EntityReady||!ActorMapAvailable()||
    level.spawning||level.restarted||level.time<0||f->intake!=intake||!ActorLeaseValid(&f->lease)||
    !ActorRefEqual(&f->lease.entity,&f->allocation)||G_Cod1SpawnerIntakeIdentity(&f->request.spawner)!=intake)return 0;
 ActorFactoryTicket(f,&expected);
 return !memcmp(t,&expected,sizeof(expected))&&
  gsc_session_spawn_commit_program(f->session,f->id,t)==f->program;
}
int G_Cod1ActorFactoryFinishSession(GscSession*s,uint64_t id,const GscActorSpawnTicket*input,
 int published,char*error,size_t cap){
 C1ActorFactory*f=&cod1ActorFactory;GscActorSpawnTicket t,expected;int ok=0;
 if(!s||!input||!ActorGoalBuffers(input,sizeof(*input),NULL,0,error,cap)||
    !gsc_session_spawn_entry_storage_separate(s,input,sizeof(*input))||
    !gsc_session_spawn_entry_storage_separate(s,error,cap))return 0;t=*input;
 if(f->busy||(f->phase!=2&&f->phase!=3)||f->session!=s||f->id!=id||
    (published!=0&&published!=1)||gsc_session_spawn_finish_program(s,id,&t,published)!=f->program)goto refused;
 ActorFactoryTicket(f,&expected);if(memcmp(&t,&expected,sizeof(t))||(published&&f->phase!=2))goto refused;
 f->busy=1;f->phase=4;
 if(published){
  if(G_Cod1ActorFactoryCommitCurrent(&t,f->intake))ok=G_Cod1SpawnerCommitFactory(&t,f->intake);
  if(ok){
   cod1ActorRecord_t*a=cod1Actors+f->lease.actorIndex;
   C1GARecord*r=cod1GenericActors+f->lease.actorIndex;
   a->spawnCompletion.operation=f->operation;a->spawnCompletion.program=f->program;
   a->spawnCompletion.runtime=r->runtime;a->spawnCompletion.catalog=r->catalog;
   a->spawnCompletion.serial=f->lease.serial;
   if(ActorJumperReady(cod1EntityEpoch)&&g_entities[f->request.spawner.slot].targetname&&
      !strcmp(g_entities[f->request.spawner.slot].targetname,"jumper")){
      trainingJumperOwner=f->lease;
   }
  }
  /* The event is already a committed prefix. Even failure consumes this
   * attempt: no retry can publish/decrement twice. Actor remains lease-owned. */
  memset(f,0,sizeof(*f));if(!ok)EntityError(error,cap,"spawn published prefix retained; one-shot count-clock commit refused");return ok;
 }
 if(G_Cod1EntitySameAllocation(&f->allocation))G_FreeEntity(g_entities+f->allocation.slot);
 ok=!G_Cod1EntitySameAllocation(&f->allocation);
 if(ok)memset(f,0,sizeof(*f));else{f->phase=3;f->busy=0;}
 return ok;
 refused:EntityError(error,cap,"spawn finish requires its exact private VM phase and one-shot completed GAME ticket");return 0;
}

/* Historical source/factory receipt belongs to the newborn allocation.
 * Source lifetime after return is not borrowed as ongoing Actor authority. */
static int ActorFactoryCompletedTarget(const cod1ActorLease_t*l,uint64_t runtime,uint64_t catalog){
 const cod1ActorRecord_t*a;const C1GARecord*r;
 if(!l||!runtime||!catalog||!ActorLeaseValid(l)||!ActorMapAvailable()||!cod1EntityReady||
    level.spawning||level.restarted||l->kind!=COD1_ACTOR_LEASE_ACTOR)return 0;
 a=cod1Actors+l->actorIndex;r=cod1GenericActors+l->actorIndex;
 return a->spawnCompletion.operation&&a->spawnCompletion.program&&
  a->spawnCompletion.serial==l->serial&&a->spawnCompletion.runtime==runtime&&
  a->spawnCompletion.catalog==catalog&&r->used&&r->runtime==runtime&&r->catalog==catalog&&
  !memcmp(&r->lease,l,sizeof(*l))&&a->scriptedController.entered&&a->scriptedController.initTask;
}
