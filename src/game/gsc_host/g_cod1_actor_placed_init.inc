/* Actual immutable resource objects, loaded only at real map spawning.
 * Training has the authored generic bank owner. Missing banks on other maps
 * refuse here; no mission name fallback or fabricated resource epoch. */
int G_Cod1ActorPlacedCurrentResourcesLoad(void){uint64_t epoch=cod1EntityEpoch;
 if(!epoch||!level.spawning||level.restarted||!cod1EntityActive||cod1GenericLookEpoch!=epoch||!C1AUT14_Ready(&cod1GenericUnmountedBank))return 0;
 return G_Cod1ActorFaceResourcesLoad()&&G_Cod1ActorScriptedResourcesLoad()&&
  (trap_Cvar_VariableIntegerValue("g_cod1GscTrainingRigs")!=1||(G_Cod1ActorJumperScriptedResourcesLoad()&&TrainingRigPlaybackLoad()))&&
  epoch==cod1EntityEpoch&&level.spawning&&!level.restarted&&ActorFaceReady(epoch)&&ActorScriptedReady(epoch);
}
int G_Cod1ActorPlacedInitReturn(void*unused,const GscPlacedInitReturnRequest*input,char*error,size_t cap,int(*current)(void*),void*context){
 C1PlacedControllerScope*f=&cod1PlacedControllerScope;GscActorInitReturnRequest q;int ok=0;(void)unused;
 if(f->popActive){f->cancelled=1;return 0;}
 if(f->completed)return 0;
 if(!input||!current||!ActorGoalBuffers(input,sizeof(*input),NULL,0,error,cap)||!ActorPlacedControllerPure(f)||
    !f->api.placed_init_return_current||!f->api.storage_separate(f->api.context,error,cap)||
    input->host_now_ms!=f->now||input->init.now_ms!=0||!input->session_id||!input->program_identity||
    !ActorRefEqual(&input->init.entity,&f->lease.entity))return 0;
 q=input->init;q.now_ms=input->host_now_ms;f->borrowed=input;f->seal=*input;f->controllerBorrowed=&q;f->controllerSeal=q;f->vmCurrent=current;f->vmContext=context;f->popActive=1;
 if(ActorPlacedControllerCurrent(f)&&ActorControllerInitReturnSelected(NULL,&q,error,cap,ActorPlacedControllerCurrent,f,2)&&ActorPlacedControllerCurrent(f)){
  /* Native4D89E/4D8A4 stores after actual row4/clock/timers. No ready/init/notify. */
  cod1Actors[f->lease.actorIndex].nativeData.entity.cell01a4=(uint32_t)f->now;
  cod1Actors[f->lease.actorIndex].nativeData.entity.cell01a8=2;f->completed=1;ok=1;
 }
 f->popActive=0;f->borrowed=NULL;f->controllerBorrowed=NULL;f->vmCurrent=NULL;f->vmContext=NULL;
 memset(&f->seal,0,sizeof(f->seal));memset(&f->controllerSeal,0,sizeof(f->controllerSeal));return ok&&!f->cancelled;
}
/* GAME owns admission to the exact current placed row. This is separate from
 * factory and completed-prefix authorities, neither of which is relaxed. */
static int ActorPlacedInitInitialSelected(const GscActorBootstrapServices*input,const cod1ActorLease_t*inputLease,
 GscPlacedInitRun*out,char*error,size_t cap){
 GscActorBootstrapServices api;cod1ActorLease_t lease;cod1ActorRecord_t*a;
 GscEntityExecutionCertificate certificate;GscPlacedInitRun run;int ok;uint64_t program;
 if(!input||!inputLease||!out||ActorGoalPrivate(input,sizeof(*input))||
    !ActorGoalBuffers(inputLease,sizeof(*inputLease),out,sizeof(*out),error,cap)||
    ActorOverlap(input,sizeof(*input),out,sizeof(*out))||ActorOverlap(inputLease,sizeof(*inputLease),out,sizeof(*out))||
    ActorOverlap(input,sizeof(*input),error,cap))return 0;
 api=*input;lease=*inputLease;
 if(!api.current||!api.program_identity||!api.placed_init_initial||!api.storage_separate||
    !api.storage_separate(api.context,input,sizeof(*input))||!api.storage_separate(api.context,inputLease,sizeof(*inputLease))||
    !api.storage_separate(api.context,out,sizeof(*out))||!api.storage_separate(api.context,error,cap)||
    api.current(api.context)!=1||!ActorLeaseValid(&lease)||!G_Cod1ActorBootstrapPostinitCurrent(&lease)||
    !ActorInheritedCertificate(&lease.entity,lease.serial)||cod1PlacedInitCancelled||
    memcmp(input,&api,sizeof(api))||memcmp(inputLease,&lease,sizeof(lease)))return 0;
 a=cod1Actors+lease.actorIndex;
 if(lease.kind!=COD1_ACTOR_LEASE_ACTOR||!a->positionPostinit||!a->placedTargetApplied||a->placedInitAttempted||
    !a->alive||a->initialized||!a->nativeDataInitialized||a->spatial.busy||!a->spatial.linked||
    a->dobj.busy||a->modelBusy||a->modelMethodBusy||a->motion.active||a->goal.assigned||a->criticalBusy||a->scriptStateBusy)
  {EntityError(error,cap,"placed INIT requires unused completed position/target on its current Actor");return 0;}
 a->placedInitAttempted=1;memset(&certificate,0,sizeof(certificate));certificate.owner=lease.entity;
 certificate.serial=lease.serial;certificate.current=ActorInheritedCertificate;memset(&run,0,sizeof(run));
 if(api.placed_init_return_current){C1PlacedControllerScope*f=&cod1PlacedControllerScope;
  if(f->active)return 0;memset(f,0,sizeof(*f));f->active=1;f->now=level.time;f->input=input;f->api=api;f->inputLease=inputLease;f->lease=lease;
 }
 ok=api.placed_init_initial(api.context,&lease.entity,&certificate,&run,error,cap);*out=run;
 if(api.placed_init_return_current){C1PlacedControllerScope*f=&cod1PlacedControllerScope;
  ok=ok&&f->completed&&!f->cancelled&&run.currentstate_completed;memset(f,0,sizeof(*f));
 }
 if(!ok)return 0;
 program=api.program_identity(api.context);
 if(api.current(api.context)!=1||cod1PlacedInitCancelled||!ActorLeaseValid(&lease)||!G_Cod1ActorBootstrapPostinitCurrent(&lease)||
    !ActorInheritedCertificate(&lease.entity,lease.serial)||memcmp(input,&api,sizeof(api))||memcmp(inputLease,&lease,sizeof(lease))||
    !cod1Actors[lease.actorIndex].placedInitAttempted||!ActorRefEqual(&run.entry.entity,&lease.entity)||!run.entry.task_serial||
    !run.runtime_identity||!run.catalog_identity||program!=run.program_identity)
  {EntityError(error,cap,"placed INIT owner retired after actual initial invocation");return 0;}
 return 1;
}

int G_Cod1ActorPlacedInitInitial(const GscActorBootstrapServices*api,const cod1ActorLease_t*lease,GscPlacedInitRun*out,char*error,size_t cap){
 int ok;
 if(cod1PlacedInitBusy){cod1PlacedInitCancelled=1;return 0;}
 if(ActorOverlap(api,api?sizeof(*api):0,&cod1PlacedInitBusy,sizeof(cod1PlacedInitBusy))||
    ActorOverlap(lease,lease?sizeof(*lease):0,&cod1PlacedInitBusy,sizeof(cod1PlacedInitBusy))||
    ActorOverlap(out,out?sizeof(*out):0,&cod1PlacedInitBusy,sizeof(cod1PlacedInitBusy))||
    ActorOverlap(error,cap,&cod1PlacedInitBusy,sizeof(cod1PlacedInitBusy))||
    ActorOverlap(api,api?sizeof(*api):0,&cod1PlacedInitCancelled,sizeof(cod1PlacedInitCancelled))||
    ActorOverlap(lease,lease?sizeof(*lease):0,&cod1PlacedInitCancelled,sizeof(cod1PlacedInitCancelled))||
    ActorOverlap(out,out?sizeof(*out):0,&cod1PlacedInitCancelled,sizeof(cod1PlacedInitCancelled))||
    ActorOverlap(error,cap,&cod1PlacedInitCancelled,sizeof(cod1PlacedInitCancelled)))return 0;
 cod1PlacedInitBusy=1;cod1PlacedInitCancelled=0;
 ok=ActorPlacedInitInitialSelected(api,lease,out,error,cap)&&!cod1PlacedInitCancelled;
 cod1PlacedInitBusy=0;return ok;
}
/* Durable post-cohort read. The old per-row borrowed return scope is gone. */
int G_Cod1ActorPlacedInitCohortCurrent(const cod1ActorLease_t*lease,const GscPlacedInitRun*run){
 const cod1ActorRecord_t*a;const cod1ActorScriptedController_t*m;
 if(!lease||!run||!ActorArguments(lease,sizeof(*lease),NULL,0,NULL,0)||!ActorArguments(run,sizeof(*run),NULL,0,NULL,0)||
  lease->kind!=COD1_ACTOR_LEASE_ACTOR||!ActorLeaseValid(lease)||!ActorInheritedCertificate(&lease->entity,lease->serial)||
  !run->root_pop_observed||!run->entry_scope_released||!run->currentstate_completed||run->entry.status!=GSC_ENTITY_ENTRY_RETURNED||
  !run->entry.task_serial||!ActorRefEqual(&run->entry.entity,&lease->entity)||level.time<0||level.time>INT_MAX-2000)return 0;
 a=cod1Actors+lease->actorIndex;m=&a->scriptedController;
 return a->placedInitAttempted&&!a->initialized&&m->entered&&m->initTask==run->entry.task_serial&&
  m->entered_ms==level.time&&m->timer28==500&&m->timer2c==level.time+50&&m->timer34==level.time+2000&&
  a->nativeData.entity.cell01a4==(uint32_t)level.time&&a->nativeData.entity.cell01a8==2;
}
