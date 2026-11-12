#include "../g_cod1_spawner.h"
#include "../g_cod1_actor_bootstrap.h"
static int ActorHookPlacedPhaseCurrent(const Cod1PlacedActorRecord*r){return r&&(r->snapshot.startupConsumed?(G_Cod1ActorBootstrapConstructedCurrent(&r->snapshot.entity)&&ActorConstructedOriginalCurrent(&r->snapshot.entity)):(PlacedOriginalRaw(&r->snapshot.entity)&&ActorInheritedRaw(&r->snapshot.entity)));}
typedef struct {
    GscEntityRef owner;uint64_t serial,intake,resource;int attempted;
} ActorSpawnerHookHistory;
static ActorSpawnerHookHistory cod1SpawnerHooks[MAX_GENTITIES];
static uint64_t cod1SpawnerHookSerial;
static void ActorEntityHooksReset(void) {memset(cod1SpawnerHooks,0,sizeof(cod1SpawnerHooks));}
static int ActorSpawnerMethodValidate(void *user,const GscEntityRef *ref,char *error,size_t size) {
    (void)user;
    if(G_Cod1SpawnerIntakeIdentity(ref))return 1;
    EntityError(error,size,"setspawnerteam requires a current typed spawner");return 0;
}
static int ActorSpawnerMethodSet(void *user,const GscEntityRef *ref,const char *team,char *error,size_t size) {
    (void)user;return G_Cod1SpawnerSetScriptTeam(ref,team,error,size);
}
const GscSpawnerTeamMethodHost *G_Cod1SpawnerTeamMethodHost(void) {
    static const GscSpawnerTeamMethodHost host={NULL,ActorSpawnerMethodValidate,ActorSpawnerMethodSet};return &host;
}
static int ActorHookResource(const GscEntityRef *ref) {
    cod1NativeStaticText_t classname;uint32_t i;
    if(!ref||!G_Cod1EntitySameAllocation(ref)||!NativeStaticTextCapture(g_entities[ref->slot].classname,&classname)||
       !classname.present||strncmp(classname.text,"actor_",6))return -1;
    for(i=0;i<cod1AitypeCount;++i)if(!strcmp(classname.text+6,cod1AitypeResources[i].name))return (int)i;
    return -1;
}
static int ActorSpawnerHookCurrent(const GscEntityRef *ref,uint64_t serial) {
    ActorSpawnerHookHistory *history;int resource;
    if(!ref||ref->slot>=MAX_GENTITIES||!serial)return 0;
    history=&cod1SpawnerHooks[ref->slot];resource=ActorHookResource(ref);
    return resource>=0&&ActorRefEqual(ref,&history->owner)&&history->serial==serial&&
        history->intake&&G_Cod1SpawnerIntakeIdentity(ref)==history->intake&&
        history->resource==cod1AitypeResources[resource].receipt&&ActorAitypeResourceCurrent(history->resource);
}
typedef struct {
    cod1ActorEntityHookPhaseRequest_t request;
    uint64_t intake[COD1_ACTOR_ENTITY_HOOKS_MAX];
    int resource[COD1_ACTOR_ENTITY_HOOKS_MAX];
} ActorEntityPhaseContext;
static int ActorEntityPhaseCurrent(void *user,size_t attempted) {
    ActorEntityPhaseContext *context=user;GscEntityRef spawners[COD1_ACTOR_ENTITY_HOOKS_MAX];
    size_t count=0,total=0;uint32_t i;int slot;
    (void)attempted;
    if(!cod1AitypeFullHooks||cod1AitypeEpoch!=cod1EntityEpoch||!ActorMapAvailable()||
       !G_Cod1SpawnerCollectAll(spawners,COD1_ACTOR_ENTITY_HOOKS_MAX,&count,NULL,0))return 0;
    total=count;
    for(slot=MAX_CLIENTS;slot<ENTITYNUM_MAX_NORMAL;++slot) {
        Cod1PlacedActorRecord *record=&cod1PlacedActors[slot];
        if(PlacedFind(&record->snapshot.entity)){++total;
            if(!ActorHookPlacedPhaseCurrent(record))return 0;}
    }
    if(total!=context->request.count)return 0;
    for(i=0;i<context->request.count;++i) {
        GscEntityRef *ref=&context->request.entities[i];Cod1PlacedActorRecord *placed=PlacedFind(ref);
        if(ActorHookResource(ref)!=context->resource[i])return 0;
        if(context->intake[i]) {
            if(placed||G_Cod1SpawnerIntakeIdentity(ref)!=context->intake[i])return 0;
        } else if(!ActorHookPlacedPhaseCurrent(placed))return 0;
    }
    return 1;
}
int G_Cod1ActorRunEntityHookPhaseViaServices(const GscActorBootstrapServices *services,const cod1ActorEntityHookPhaseRequest_t *input,
    cod1ActorEntityHookPhaseResult_t *out,char *error,size_t size) {
    ActorEntityPhaseContext context;cod1ActorEntityHookPhaseResult_t result;
    GscEntityHookBootstrapEntry plan[GSC_ENTITY_HOOK_BOOTSTRAP_MAX];GscEntityHookBootstrapResult run;
    GscEntityHookBootstrapHost host;unsigned char seen[COD1_AITYPE_RESOURCES_MAX];
    uint64_t epoch,program;uint32_t i,n=0,j;int resource,ok;
    if(!ActorServicesValid(services)||!input||!out||!services->storage_separate(services->context,input,sizeof(*input))||
       !services->storage_separate(services->context,out,sizeof(*out))||!services->storage_separate(services->context,error,size)||
       !ActorArguments(input,sizeof(*input),out,sizeof(*out),error,size)||ActorOverlap(input,sizeof(*input),out,sizeof(*out)))return 0;
    memset(&context,0,sizeof(context));context.request=*input;memset(&result,0,sizeof(result));*out=result;
    epoch=cod1AitypeEpoch;program=services->program_identity(services->context);
    if(cod1AitypeBusy||cod1ActorInheritedEntryBusy||cod1PlacedBusy||!cod1AitypeFullHooks||
       !epoch||epoch!=cod1EntityEpoch||!ActorMapAvailable()||program!=cod1AitypeProgram||
       cod1AitypePhaseInvoked||!context.request.count||context.request.count>COD1_ACTOR_ENTITY_HOOKS_MAX||
       !services->available(services->context,GSC_ACTOR_BOOTSTRAP_AVAILABLE_RESOURCES))goto refused;
    for(i=0;i<cod1AitypeCount;++i)if(cod1AitypeResources[i].consumed)goto refused;
    memset(plan,0,sizeof(plan));memset(&run,0,sizeof(run));memset(seen,0,sizeof(seen));
    for(i=0;i<context.request.count;++i) {
        GscEntityRef *ref=&context.request.entities[i];uint64_t intake=G_Cod1SpawnerIntakeIdentity(ref);
        if(ref->epoch!=epoch||(i&&ref->slot<=context.request.entities[i-1].slot)||(resource=ActorHookResource(ref))<0)goto refused;
        context.resource[i]=resource;context.intake[i]=intake;
        if(intake) {
            ActorSpawnerHookHistory *history=&cod1SpawnerHooks[ref->slot];
            if(ActorRefEqual(ref,&history->owner)&&history->attempted)goto refused;
            if(cod1SpawnerHookSerial==UINT64_MAX)goto refused;
            history->owner=*ref;history->serial=++cod1SpawnerHookSerial;history->intake=intake;history->resource=cod1AitypeResources[resource].receipt;history->attempted=0;
            plan[n].kind=GSC_HOOK_ROOT_ENTITY;plan[n].entity_certificate.owner=*ref;
            plan[n].entity_certificate.serial=history->serial;plan[n].entity_certificate.current=ActorSpawnerHookCurrent;
            result.hooks[n].kind=COD1_ACTOR_HOOK_SPAWNER;strcpy(plan[n].function,"spawner");
            result.hooks[n].entity=*ref;result.hooks[n].resourceIndex=(uint32_t)resource;
            strcpy(plan[n].module,cod1AitypeResources[resource].module);plan[n].map_certificate.receipt=cod1AitypeResources[resource].receipt;
            plan[n].map_certificate.current=ActorAitypeResourceCurrent;++n;
        }
        if(!seen[resource]) {
            plan[n].kind=GSC_HOOK_ROOT_GLOBAL;strcpy(plan[n].function,"precache");strcpy(plan[n].module,cod1AitypeResources[resource].module);
            plan[n].map_certificate.receipt=cod1AitypeResources[resource].receipt;plan[n].map_certificate.current=ActorAitypeResourceCurrent;
            result.hooks[n].kind=COD1_ACTOR_HOOK_PRECACHE;result.hooks[n].entity=*ref;result.hooks[n].resourceIndex=(uint32_t)resource;
            seen[resource]=1;++n;
        }
    }
    for(j=0;j<cod1AitypeCount;++j)if(!seen[j])goto refused;
    if(!ActorEntityPhaseCurrent(&context,0))goto refused;
    result.entityCount=context.request.count;result.hookCount=n;host.user=&context;host.after_initial=ActorEntityPhaseCurrent;
    cod1AitypeBusy=1;ok=services->start_entity_hooks(services->context,plan,n,&host,&run,error,size);cod1AitypeBusy=0;
    result.attempted=run.attempted;result.allReturned=ok;
    for(i=0;i<run.attempted;++i){result.hooks[i].run=run.runs[i];if(run.runs[i].status!=GSC_ENTITY_ENTRY_RETURNED)result.allReturned=0;}
    result.phaseInvoked=ok&&run.attempted==n;
    if(epoch==cod1EntityEpoch&&epoch==cod1AitypeEpoch&&program==cod1AitypeProgram) {
        for(i=0;i<run.attempted;++i) {
            if(result.hooks[i].kind==COD1_ACTOR_HOOK_PRECACHE){resource=(int)result.hooks[i].resourceIndex;
                cod1AitypeResources[resource].consumed=1;cod1AitypeResources[resource].run=run.runs[i];}
            else {ActorSpawnerHookHistory *history=&cod1SpawnerHooks[result.hooks[i].entity.slot];
                if(ActorRefEqual(&history->owner,&result.hooks[i].entity)&&history->serial==plan[i].entity_certificate.serial)history->attempted=1;}
        }
        cod1AitypePhaseInvoked=result.phaseInvoked;
    } else {ok=0;result.phaseInvoked=result.allReturned=0;EntityError(error,size,"entity hook map retired during phase");}
    *out=result;return ok;
refused:
    EntityError(error,size,"entity hook phase requires the complete fresh typed collection and three-label catalog");return 0;
}

int G_Cod1ActorRunEntityHookPhase(GscRuntime *runtime,const cod1ActorEntityHookPhaseRequest_t *input,
    cod1ActorEntityHookPhaseResult_t *out,char *error,size_t size) {
    GscActorBootstrapServices services=ActorLegacyServices(runtime);
    return G_Cod1ActorRunEntityHookPhaseViaServices(&services,input,out,error,size);
}
