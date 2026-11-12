/* Two executable semantic entries; native spawner slot is not synthesized. */
typedef struct {
    char name[121],module[128];uint64_t receipt;int consumed;
    GscResourceBootstrapRun run;
} ActorAitypeResource;
static ActorAitypeResource cod1AitypeResources[COD1_AITYPE_RESOURCES_MAX];
static uint32_t cod1AitypeCount;
static uint64_t cod1AitypeEpoch,cod1AitypeProgram,cod1AitypeSerial;
static int cod1AitypeBusy,cod1AitypePhaseInvoked,cod1AitypeFullHooks;
static void ActorEntityHooksReset(void);
static void ActorAitypeResourcesReset(void) {
    memset(cod1AitypeResources,0,sizeof(cod1AitypeResources));
    cod1AitypeCount=0;cod1AitypeEpoch=cod1AitypeProgram=0;cod1AitypePhaseInvoked=cod1AitypeFullHooks=0;
    ActorEntityHooksReset();
    /* Busy scopes and monotonic serials do not revive across callback reset. */
}
static int ActorAitypeResourceCurrent(uint64_t receipt) {
    uint32_t i;
    if(!receipt||!cod1AitypeEpoch||cod1AitypeEpoch!=cod1EntityEpoch||!ActorMapAvailable())return 0;
    for(i=0;i<cod1AitypeCount;++i)if(cod1AitypeResources[i].receipt==receipt)return 1;
    return 0;
}
static int ActorSelectAitypeResources(const GscActorBootstrapServices *services,const cod1AitypeResourceSelection_t *input,int full,char *error,size_t size) {
    cod1AitypeResourceSelection_t copy;ActorAitypeResource staged[COD1_AITYPE_RESOURCES_MAX];uint64_t program;uint32_t i,j;
    if(!ActorServicesValid(services)||!input||!services->storage_separate(services->context,input,sizeof(*input))||
       !services->storage_separate(services->context,error,size)||!ActorArguments(input,sizeof(*input),NULL,0,error,size))return 0;
    if(cod1AitypeBusy||cod1ActorInheritedEntryBusy||cod1PlacedBusy||cod1AitypeEpoch||!ActorMapAvailable()||
       !services->available(services->context,GSC_ACTOR_BOOTSTRAP_AVAILABLE_RESOURCES)||(full&&!services->available(services->context,GSC_ACTOR_BOOTSTRAP_AVAILABLE_SPAWNER_TEAM)))goto refused;
    copy=*input;program=services->program_identity(services->context);
    if(!program||!copy.count||copy.count>COD1_AITYPE_RESOURCES_MAX||UINT64_MAX-cod1AitypeSerial<copy.count)goto refused;
    memset(staged,0,sizeof(staged));
    for(i=0;i<copy.count;++i){size_t n;
        if(!memchr(copy.names[i],0,sizeof(copy.names[i]))||!copy.names[i][0])goto refused;
        for(n=0;copy.names[i][n];++n)if(!((copy.names[i][n]>='a'&&copy.names[i][n]<='z')||(copy.names[i][n]>='0'&&copy.names[i][n]<='9')||copy.names[i][n]=='_'))goto refused;
        for(j=0;j<i;++j)if(!strcmp(copy.names[i],copy.names[j]))goto refused;
        strcpy(staged[i].name,copy.names[i]);strcpy(staged[i].module,"aitype/");strcat(staged[i].module,copy.names[i]);
        if(!services->script_function_available(services->context,staged[i].module,"main")||
           !services->script_function_available(services->context,staged[i].module,"precache")||
           (full&&!services->script_function_available(services->context,staged[i].module,"spawner")))goto refused;
    }
    for(i=0;i<copy.count;++i)staged[i].receipt=++cod1AitypeSerial;
    memcpy(cod1AitypeResources,staged,sizeof(staged));cod1AitypeCount=copy.count;
    cod1AitypeProgram=program;cod1AitypeEpoch=cod1EntityEpoch;cod1AitypeFullHooks=full;return 1;
refused:
    EntityError(error,size,"aitype resources require an unbound map and available main/precache entries");return 0;
}
int G_Cod1ActorSelectAitypeResources(const GscRuntime *runtime,const cod1AitypeResourceSelection_t *input,char *error,size_t size) {
    GscActorBootstrapServices services=ActorLegacyServices(runtime);return ActorSelectAitypeResources(&services,input,0,error,size);
}
int G_Cod1ActorSelectAitypeResourcesWithSpawnerHooks(const GscRuntime *runtime,const cod1AitypeResourceSelection_t *input,char *error,size_t size) {
    GscActorBootstrapServices services=ActorLegacyServices(runtime);return ActorSelectAitypeResources(&services,input,1,error,size);
}
int G_Cod1ActorRunAitypeResourcePhase(GscRuntime *runtime,cod1AitypeResourcePhaseResult_t *out,char *error,size_t size) {
    GscResourceBootstrapEntry entries[COD1_AITYPE_RESOURCES_MAX];GscResourceBootstrapResult run;
    cod1AitypeResourcePhaseResult_t result;uint64_t epoch,program;uint32_t i;int ok;
    if(!runtime||!out||!gsc_runtime_storage_separate(runtime,out,sizeof(*out))||
       !gsc_runtime_storage_separate(runtime,error,size)||!ActorArguments(NULL,0,out,sizeof(*out),error,size))return 0;
    memset(&result,0,sizeof(result));*out=result;epoch=cod1AitypeEpoch;program=gsc_runtime_program_identity(runtime);
    if(cod1AitypeBusy||cod1ActorInheritedEntryBusy||cod1PlacedBusy||!epoch||epoch!=cod1EntityEpoch||
       !ActorMapAvailable()||program!=cod1AitypeProgram||cod1AitypePhaseInvoked||cod1AitypeFullHooks)goto refused;
    for(i=0;i<cod1AitypeCount;++i)if(cod1AitypeResources[i].consumed)goto refused;
    memset(entries,0,sizeof(entries));memset(&run,0,sizeof(run));result.count=cod1AitypeCount;
    for(i=0;i<cod1AitypeCount;++i){strcpy(entries[i].module,cod1AitypeResources[i].module);strcpy(entries[i].function,"precache");
        entries[i].certificate.receipt=cod1AitypeResources[i].receipt;entries[i].certificate.current=ActorAitypeResourceCurrent;}
    cod1AitypeBusy=1;ok=gsc_start_resource_hooks(runtime,entries,cod1AitypeCount,&run,error,size);cod1AitypeBusy=0;
    result.attempted=run.attempted;result.allReturned=ok;
    for(i=0;i<run.attempted;++i){result.hooks[i]=run.runs[i];if(run.runs[i].status!=GSC_ENTITY_ENTRY_RETURNED)result.allReturned=0;}
    result.phaseInvoked=ok&&run.attempted==result.count;
    if(epoch==cod1EntityEpoch&&epoch==cod1AitypeEpoch&&program==cod1AitypeProgram){
        for(i=0;i<run.attempted;++i){cod1AitypeResources[i].consumed=1;cod1AitypeResources[i].run=run.runs[i];}
        cod1AitypePhaseInvoked=result.phaseInvoked;
    } else {ok=0;result.phaseInvoked=result.allReturned=0;EntityError(error,size,"aitype resource map retired during phase");}
    *out=result;return ok;
refused:
    EntityError(error,size,"aitype resource phase is unavailable or already attempted");return 0;
}
int G_Cod1ActorRunPlacedAitypeEntryStageViaServices(const GscActorBootstrapServices *services,const cod1ActorPlacedAitypeEntryRequest_t *input,
    cod1ActorPlacedEntryResult_t *out,char *error,size_t size) {
    cod1ActorPlacedAitypeEntryRequest_t copy;cod1ActorPlacedEntryRequest_t request;cod1NativeStaticText_t classname;uint32_t i;
    if(!ActorServicesValid(services)||!input||!out||!services->storage_separate(services->context,input,sizeof(*input))||
       !services->storage_separate(services->context,out,sizeof(*out))||!services->storage_separate(services->context,error,size)||
       !ActorArguments(input,sizeof(*input),out,sizeof(*out),error,size)||ActorOverlap(input,sizeof(*input),out,sizeof(*out)))return 0;
    copy=*input;memset(out,0,sizeof(*out));
    if(cod1AitypeBusy||!cod1AitypePhaseInvoked||!cod1AitypeEpoch||cod1AitypeEpoch!=cod1EntityEpoch||
       !ActorMapAvailable()||services->program_identity(services->context)!=cod1AitypeProgram||
       !G_Cod1EntitySameAllocation(&copy.entity)||!NativeStaticTextCapture(g_entities[copy.entity.slot].classname,&classname)||
       !classname.present||strncmp(classname.text,"actor_",6))goto refused;
    for(i=0;i<cod1AitypeCount;++i)if(!strcmp(classname.text+6,cod1AitypeResources[i].name))break;
    if(i==cod1AitypeCount||!cod1AitypeResources[i].consumed)goto refused;
    memset(&request,0,sizeof(request));request.entity=copy.entity;request.nativeServerFlags=copy.nativeServerFlags;request.nativeEntityFlags=copy.nativeEntityFlags;
    strcpy(request.module,cod1AitypeResources[i].module);strcpy(request.function,"main");
    return G_Cod1ActorRunPlacedEntryStageViaServices(services,&request,out,error,size);
refused:
    EntityError(error,size,"placed aitype main requires its current program and all initial shared invocations");return 0;
}

int G_Cod1ActorSelectAitypeResourcesViaServices(const GscActorBootstrapServices *services,
    const cod1AitypeResourceSelection_t *input,char *error,size_t size) {
    return ActorSelectAitypeResources(services,input,1,error,size);
}
int G_Cod1ActorRunPlacedAitypeEntryStage(GscRuntime *runtime,const cod1ActorPlacedAitypeEntryRequest_t *input,
    cod1ActorPlacedEntryResult_t *out,char *error,size_t size) {
    GscActorBootstrapServices services=ActorLegacyServices(runtime);
    return G_Cod1ActorRunPlacedAitypeEntryStageViaServices(&services,input,out,error,size);
}

#include "../g_cod1_actor_bootstrap.h"
int G_Cod1ActorConstructPlacedAitype(const GscActorBootstrapServices*s,const cod1ActorPlacedAitypeEntryRequest_t*input,cod1ActorPlacedEntryResult_t*out,char*error,size_t cap){
 cod1ActorPlacedEntryRequest_t q;cod1ActorPlacedAitypeEntryRequest_t copy;cod1NativeStaticText_t name;uint32_t i;
 if(!ActorServicesValid(s)||!s->construct_entity||!input||!out||!s->storage_separate(s->context,input,sizeof(*input))||!s->storage_separate(s->context,out,sizeof(*out))||
    !s->storage_separate(s->context,error,cap)||!ActorArguments(input,sizeof(*input),out,sizeof(*out),error,cap)||ActorOverlap(input,sizeof(*input),out,sizeof(*out)))return 0;
 copy=*input;memset(out,0,sizeof(*out));
 if(cod1AitypeBusy||cod1AitypePhaseInvoked||!cod1AitypeEpoch||cod1AitypeEpoch!=cod1EntityEpoch||!ActorMapAvailable()||s->program_identity(s->context)!=cod1AitypeProgram||
    !G_Cod1EntitySameAllocation(&copy.entity)||!NativeStaticTextCapture(g_entities[copy.entity.slot].classname,&name)||!name.present||strncmp(name.text,"actor_",6))goto refused;
 for(i=0;i<cod1AitypeCount;i++)if(!strcmp(name.text+6,cod1AitypeResources[i].name))break;
 if(i==cod1AitypeCount||cod1AitypeResources[i].consumed)goto refused;
 memset(&q,0,sizeof(q));q.entity=copy.entity;q.nativeServerFlags=copy.nativeServerFlags;q.nativeEntityFlags=copy.nativeEntityFlags;
 strcpy(q.module,cod1AitypeResources[i].module);strcpy(q.function,"main");return ActorPlacedEntryPhase(s,&q,out,error,cap,1);
refused:EntityError(error,cap,"construction requires current selected uninvoked aitype resources");return 0;
}
int G_Cod1ActorEnterConstructedAitype(const GscActorBootstrapServices*s,const cod1ActorLease_t*input,GscEntityScriptRun*out,char*error,size_t cap){
 cod1ActorLease_t lease;GscEntityScriptEntry entry;GscEntityExecutionCertificate certificate;Cod1PlacedActorRecord*record;cod1NativeStaticText_t name;uint64_t declaration;uint32_t i;int ok;
 if(!ActorServicesValid(s)||!s->enter_constructed_entity||!input||!out||!s->storage_separate(s->context,input,sizeof(*input))||!s->storage_separate(s->context,out,sizeof(*out))||
    !s->storage_separate(s->context,error,cap)||!ActorArguments(input,sizeof(*input),out,sizeof(*out),error,cap)||ActorOverlap(input,sizeof(*input),out,sizeof(*out)))return 0;
 lease=*input;memset(out,0,sizeof(*out));record=PlacedFind(&lease.entity);
 if(cod1PlacedBusy||cod1ActorInheritedEntryBusy||cod1AitypeBusy||!cod1AitypePhaseInvoked||!ActorLeaseValid(&lease)||!G_Cod1ActorBootstrapEntryCurrent(&lease)||!record||!record->snapshot.startupConsumed||
    !ActorMapAvailable()||s->program_identity(s->context)!=cod1AitypeProgram||!ActorConstructedOriginalCurrent(&lease.entity)||
    !NativeStaticTextCapture(g_entities[lease.entity.slot].classname,&name)||!name.present||strncmp(name.text,"actor_",6))goto refused;
 for(i=0;i<cod1AitypeCount;i++)if(!strcmp(name.text+6,cod1AitypeResources[i].name))break;
 if(i==cod1AitypeCount||!cod1AitypeResources[i].consumed)goto refused;
 declaration=record->declarationReceipt;cod1PlacedBusy=1;ok=PlacedDeclarationsCurrent(declaration,lease.entity.epoch);cod1PlacedBusy=0;
 record=PlacedFind(&lease.entity);
 if(!ok||ActorServicesFailed(s)||!record||record->declarationReceipt!=declaration||!record->snapshot.startupConsumed||!ActorLeaseValid(&lease)||
    !ActorInheritedCertificate(&lease.entity,lease.serial)||!ActorConstructedOriginalCurrent(&lease.entity)||!G_Cod1ActorBootstrapEntryCurrent(&lease))goto refused;
 memset(&entry,0,sizeof(entry));entry.entity=lease.entity;strcpy(entry.module,cod1AitypeResources[i].module);strcpy(entry.function,"main");
 memset(&certificate,0,sizeof(certificate));certificate.owner=lease.entity;certificate.serial=lease.serial;certificate.current=ActorInheritedCertificate;
 cod1ActorInheritedEntryBusy=1;ok=s->enter_constructed_entity(s->context,&entry,&certificate,out,error,cap);cod1ActorInheritedEntryBusy=0;return ok;
refused:EntityError(error,cap,"constructed aitype requires current original allocation, declaration and component");return 0;
}

int G_Cod1ActorResolveSpawnAitypeSession(GscSession *session,uint64_t id,
 const GscEntityRef *input,cod1ActorSpawnScript_t *out,char *error,size_t cap){
 GscEntityRef source;cod1NativeStaticText_t name;uint64_t program;uint32_t i;
 if(!session||!input||!out||!gsc_session_spawn_entry_storage_separate(session,input,sizeof(*input))||
    !gsc_session_spawn_entry_storage_separate(session,out,sizeof(*out))||!gsc_session_spawn_entry_storage_separate(session,error,cap)||
    !ActorArguments(input,sizeof(*input),out,sizeof(*out),error,cap)||ActorOverlap(input,sizeof(*input),out,sizeof(*out)))return 0;
 source=*input;memset(out,0,sizeof(*out));
 program=gsc_session_spawn_entry_program(session,id,&source);
 if(!program||program!=cod1AitypeProgram||cod1AitypeBusy||cod1ActorInheritedEntryBusy||
    !cod1AitypeEpoch||cod1AitypeEpoch!=cod1EntityEpoch||!cod1AitypePhaseInvoked||
    !ActorMapAvailable()||!EntityValidate(NULL,&source)||!G_Cod1SpawnerIntakeIdentity(&source)||
    !NativeStaticTextCapture(g_entities[source.slot].classname,&name)||!name.present||strncmp(name.text,"actor_",6))goto refused;
 for(i=0;i<cod1AitypeCount;i++)if(!strcmp(name.text+6,cod1AitypeResources[i].name))break;
 if(i==cod1AitypeCount||!cod1AitypeResources[i].consumed||!ActorAitypeResourceCurrent(cod1AitypeResources[i].receipt))goto refused;
 strcpy(out->module,cod1AitypeResources[i].module);strcpy(out->function,"main");
 out->resourceReceipt=cod1AitypeResources[i].receipt;return 1;
refused:EntityError(error,cap,"spawn aitype requires current factory, program, typed source and invoked resource");return 0;
}
int G_Cod1ActorAllocateAitypePrefixSession(GscSession *session,uint64_t id,
 const cod1ActorDynamicPrefixRequest_t *input,cod1ActorSpawnPrefixResult_t *out,char *error,size_t cap){
 cod1ActorDynamicPrefixRequest_t copy;cod1ActorSpawnScript_t script;cod1ActorSpawnPrefixRequest_t request;
 if(!session||!input||!out||!gsc_session_spawn_entry_storage_separate(session,input,sizeof(*input))||
    !gsc_session_spawn_entry_storage_separate(session,out,sizeof(*out))||!gsc_session_spawn_entry_storage_separate(session,error,cap)||
    !ActorArguments(input,sizeof(*input),out,sizeof(*out),error,cap)||ActorOverlap(input,sizeof(*input),out,sizeof(*out)))return 0;
 copy=*input;memset(out,0,sizeof(*out));
 if(!G_Cod1ActorResolveSpawnAitypeSession(session,id,&copy.spawner,&script,error,cap))return 0;
 memset(&request,0,sizeof(request));request.spawner=copy.spawner;request.targetname=copy.targetname;
 request.nativeServerFlags=copy.nativeServerFlags;request.nativeEntityFlags=copy.nativeEntityFlags;
 strcpy(request.module,script.module);strcpy(request.function,script.function);
 return G_Cod1ActorAllocateSpawnPrefixSession(session,id,&request,out,error,cap);
}
