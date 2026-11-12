#include "../g_cod1_weapon_registration.h"
#ifndef COD1_REGISTRATION_EXTRA_STORAGE
#define COD1_REGISTRATION_EXTRA_STORAGE(p,n) 0
#endif
/* Persistent guard/staging survives item-scene reset. No dynamic allocation. */
static struct {int active,cancelled;uint64_t serial;} weaponRegistrationGuard;
static uint64_t weaponRegistrationSerial;
static cod1AmmoRegistryReceipt_t weaponRegistrationRegistry;
static cod1WeaponRegistrationReceipt_t weaponRegistrationStage,weaponRegistrationFresh;
typedef struct {GscTestItemScene scene;uint64_t serial;int logical_ms,server_time;} WeaponRegistrationLease;
static int WeaponRegistrationPrivate(const void *p,size_t n) {
    int i;const cwDef_t *definition;uintptr_t first=(uintptr_t)p;
    if(n && n>UINTPTR_MAX-first)return 1;
    /* An unclassifiable active client pool must refuse before diagnostics. */
    if(!level.clients || level.maxclients<=0 || level.maxclients>MAX_CLIENTS ||
       sizeof(gclient_t)*(size_t)level.maxclients>UINTPTR_MAX-(uintptr_t)level.clients)return 1;
    if(CAR_StorageProtected(p,n) || COD1_REGISTRATION_EXTRA_STORAGE(p,n) || G_Cod1EntityStorageProtected(p,n) ||
       CAR_Overlap(p,n,level.clients,sizeof(gclient_t)*(size_t)level.maxclients) ||
       CAR_Overlap(p,n,G_Cod1PlayerNativeHost(),sizeof(GscPlayerNativeHost)) ||
       CAR_Overlap(p,n,&gscTestItem,sizeof(gscTestItem)) ||
       CAR_Overlap(p,n,&gscTestItemBusy,sizeof(gscTestItemBusy)) ||
       CAR_Overlap(p,n,&gscTestItemInitialization,sizeof(gscTestItemInitialization)) ||
       CAR_Overlap(p,n,&gscTestSession,sizeof(gscTestSession)) ||
       CAR_Overlap(p,n,&gscTestId,sizeof(gscTestId)) ||
       CAR_Overlap(p,n,&gscTestMapEpoch,sizeof(gscTestMapEpoch)) ||
       CAR_Overlap(p,n,&gscTestActorPrefixEpoch,sizeof(gscTestActorPrefixEpoch)) ||
       CAR_Overlap(p,n,&gscTestBusy,sizeof(gscTestBusy)) ||
       CAR_Overlap(p,n,&gscTestStopRequested,sizeof(gscTestStopRequested)) ||
       CAR_Overlap(p,n,&gscTestCompleted,sizeof(gscTestCompleted)) ||
       CAR_Overlap(p,n,&gscTestEntityMode,sizeof(gscTestEntityMode)) ||
       CAR_Overlap(p,n,&gscTestEnabled,sizeof(gscTestEnabled)) ||
       CAR_Overlap(p,n,&gscTestScript,sizeof(gscTestScript)) ||
       CAR_Overlap(p,n,&gscTestEntities,sizeof(gscTestEntities)) ||
       CAR_Overlap(p,n,&gscTestAiArrays,sizeof(gscTestAiArrays)) ||
       CAR_Overlap(p,n,&gscTestActorPrefix,sizeof(gscTestActorPrefix)) ||
       CAR_Overlap(p,n,&gscTestDeleteTransactions,sizeof(gscTestDeleteTransactions)) ||
       CAR_Overlap(p,n,&gscTestDeleteTransactionsSelected,sizeof(gscTestDeleteTransactionsSelected)) ||
       CAR_Overlap(p,n,cod1ItemCompiled,sizeof(cod1ItemCompiled)) ||
       CAR_Overlap(p,n,&weaponRegistrationGuard,sizeof(weaponRegistrationGuard)) ||
       CAR_Overlap(p,n,&weaponRegistrationSerial,sizeof(weaponRegistrationSerial)) ||
       CAR_Overlap(p,n,&weaponRegistrationRegistry,sizeof(weaponRegistrationRegistry)) ||
       CAR_Overlap(p,n,&weaponRegistrationStage,sizeof(weaponRegistrationStage)) ||
       CAR_Overlap(p,n,&weaponRegistrationFresh,sizeof(weaponRegistrationFresh)))return 1;
    if(CW_Count()<0 || CW_Count()>CW_MAX_DEFS)return 1;
    for(i=0;i<CW_Count();++i){definition=CW_Get(i);if(!definition||CAR_Overlap(p,n,definition,sizeof(*definition)))return 1;}
    return 0;
}
static int WeaponRegistrationBuffers(const cod1AmmoRegistryReceipt_t *registry,
    cod1WeaponRegistrationReceipt_t *out,char *error,size_t size) {
    return !WeaponRegistrationPrivate(registry,sizeof(*registry)) &&
        !WeaponRegistrationPrivate(out,sizeof(*out)) && !WeaponRegistrationPrivate(error,size) &&
        !CAR_Overlap(registry,sizeof(*registry),out,sizeof(*out)) &&
        !CAR_Overlap(registry,sizeof(*registry),error,size) && !CAR_Overlap(out,sizeof(*out),error,size);
}
static int WeaponRegistrationScope(const WeaponRegistrationLease *lease) {
    GscSessionSnapshot snapshot;
    return weaponRegistrationGuard.active && !weaponRegistrationGuard.cancelled &&
        weaponRegistrationGuard.serial==lease->serial && !gscTestItemBusy &&
        /* CAR first validates current CW token ownership before the older
         * ItemCurrent fingerprint reads those strings. */
        CAR_Current(&weaponRegistrationRegistry) &&
        GscTestItemCurrent(&lease->scene) &&
        lease->scene.registry_revision && lease->scene.registry_revision==gscTestItem.registry_revision &&
        lease->scene.registry_revision==CW_RegistryRevision() &&
        lease->scene.registry_revision==weaponRegistrationRegistry.registryRevision &&
        lease->scene.operation_id==gscTestItem.operation_id &&
        !memcmp(lease->scene.registered,gscTestItem.registered,sizeof(lease->scene.registered)) &&
        !memcmp(lease->scene.complete,gscTestItem.complete,sizeof(lease->scene.complete)) &&
        !memcmp(lease->scene.published,gscTestItem.published,sizeof(lease->scene.published)) &&
        level.time==lease->server_time &&
        gsc_session_snapshot(gscTestSession,&snapshot) && snapshot.session_id==lease->scene.id &&
        snapshot.state==GSC_SESSION_RUNNING && !snapshot.error[0] && snapshot.logical_ms==lease->logical_ms;
}
static int WeaponRegistrationGet(const WeaponRegistrationLease *lease,int slot,char *text,size_t size) {
    if(!WeaponRegistrationScope(lease))return 0;
    memset(text,0,size);trap_GetConfigstring(slot,text,(int)size);
    return WeaponRegistrationScope(lease) && memchr(text,0,size)!=NULL;
}
static int WeaponRegistrationMarker(const WeaponRegistrationLease *lease) {
    static const char *names[6]={"g_cod1GscTest","g_cod1GscEntityTest","savegame_loading","g_cod1GscTest","savegame_loading","g_cod1GscEntityTest"};
    int values[6]={1,lease->scene.mode,0,1,0,lease->scene.mode},i,value;char text[MAX_STRING_CHARS];
    for(i=0;i<6;++i) {
        if(!WeaponRegistrationScope(lease))return 0;
        value=trap_Cvar_VariableIntegerValue(names[i]);
        if(!WeaponRegistrationScope(lease)||value!=values[i])return 0;
        if(i==0||i==5) {
            value=trap_Cvar_VariableIntegerValue(COD1_AMBIENT_MODULE_CVAR);
            if(!WeaponRegistrationScope(lease)||(uint32_t)value!=lease->scene.module_id)return 0;
        }
    }
    return lease->scene.marker_ready &&
        WeaponRegistrationGet(lease,CS_COD1_ITEM_PRECACHE,text,sizeof(text)) &&
        !strcmp(text,lease->scene.published);
}
static int WeaponRegistrationModel(const WeaponRegistrationLease *lease,const char *name,
    cod1WeaponRegistrationModel_t *out) {
    char text[MAX_STRING_CHARS];int slot;
    cod1ModelResourceReference_t request,result,validated;
    for(slot=1;slot<MAX_MODELS;++slot) {
        if(!WeaponRegistrationGet(lease,CS_MODELS+slot,text,sizeof(text)))return 0;
        if(!text[0])return 0;
        if(!strcmp(text,name))break;
    }
    if(slot==MAX_MODELS)return 0;
    memset(&request,0,sizeof(request));request.version=COD1_MODEL_RESOURCE_REFERENCE_VERSION;
    request.bytes=sizeof(request);request.operation=COD1_MODEL_RESOURCE_QUERY;strcpy(request.name,name);
    if(!WeaponRegistrationScope(lease) || trap_Cod1ModelResourceReference(&request,&result)!=1 ||
       !WeaponRegistrationScope(lease))return 0;
    if(result.version!=request.version || result.bytes!=sizeof(result) || result.reserved ||
       result.operation!=COD1_MODEL_RESOURCE_QUERY || !result.scene ||
       result.slot<1 || result.slot>COD1_PRECACHE_MODELS ||
       !memchr(result.name,0,sizeof(result.name)) || strcmp(result.name,name))return 0;
    request=result;request.operation=COD1_MODEL_RESOURCE_VALIDATE;
    if(!WeaponRegistrationScope(lease) || trap_Cod1ModelResourceReference(&request,&validated)!=1 ||
       !WeaponRegistrationScope(lease) || memcmp(&request,&validated,sizeof(request)))return 0;
    out->configstringSlot=slot;out->resource=validated;
    return WeaponRegistrationGet(lease,CS_MODELS+slot,text,sizeof(text)) && !strcmp(text,name);
}
static int WeaponRegistrationProject(const WeaponRegistrationLease *lease,int selected,
    cod1WeaponRegistrationReceipt_t *out) {
    cod1ItemDescriptor_t descriptor;int index,i,j;unsigned char seen[CW_MAX_DEFS+1];
    if(!WeaponRegistrationScope(lease) || !WeaponRegistrationMarker(lease) ||
       selected<1 || selected>weaponRegistrationRegistry.weaponCount)return 0;
    memset(out,0,sizeof(*out));memset(seen,0,sizeof(seen));
    out->version=COD1_WEAPON_REGISTRATION_VERSION;out->bytes=sizeof(*out);
    out->sessionOwner=(uintptr_t)lease->scene.session;out->sessionId=lease->scene.id;
    out->mapEpoch=lease->scene.map_epoch;out->itemEpoch=lease->scene.epoch;
    out->incarnation=lease->scene.incarnation;out->operationId=lease->scene.operation_id;
    out->registryRevision=lease->scene.registry_revision;out->moduleId=lease->scene.module_id;
    out->fingerprint=lease->scene.fingerprint;out->selectedCAPIndex=selected;
    out->registry=weaponRegistrationRegistry;memcpy(out->marker,lease->scene.published,sizeof(out->marker));
    index=weaponRegistrationRegistry.weapons[selected].cwIndex+1;
    for(;;) {
        cod1WeaponRegistrationRow_t *row;
        if(index<1 || index>weaponRegistrationRegistry.weaponCount || seen[index] ||
           out->count==CW_MAX_DEFS || !CI_Descriptor(index,&descriptor) || !descriptor.dynamic ||
           descriptor.type!=1 || descriptor.weapon_index!=index-1 ||
           !gscTestItem.registered[index] || !gscTestItem.complete[index])return 0;
        seen[index]=1;row=&out->rows[out->count++];row->itemIndex=index;row->cwIndex=index-1;
        row->capIndex=0;
        for(i=1;i<=weaponRegistrationRegistry.weaponCount;++i)
            if(weaponRegistrationRegistry.weapons[i].cwIndex==row->cwIndex){row->capIndex=i;break;}
        if(!row->capIndex || strcmp(weaponRegistrationRegistry.weapons[row->capIndex].name,descriptor.internal_name))return 0;
        row->descriptor=descriptor;
        if(!descriptor.linked_index || descriptor.linked_index==out->rows[0].itemIndex)break;
        index=descriptor.linked_index;
    }
    for(i=0;i<out->count;++i)for(j=0;j<2;++j)
        if(out->rows[i].descriptor.model_present[j] && out->rows[i].descriptor.models[j][0] &&
           !WeaponRegistrationModel(lease,out->rows[i].descriptor.models[j],&out->rows[i].models[j]))return 0;
    return WeaponRegistrationMarker(lease) && WeaponRegistrationScope(lease);
}
static int WeaponRegistrationBegin(WeaponRegistrationLease *lease) {
    GscSessionSnapshot snapshot;
    if(weaponRegistrationGuard.active){weaponRegistrationGuard.cancelled=1;return 0;}
    if(weaponRegistrationSerial==UINT64_MAX)return 0;
    weaponRegistrationGuard.active=1;weaponRegistrationGuard.cancelled=0;
    weaponRegistrationGuard.serial=++weaponRegistrationSerial;
    memset(lease,0,sizeof(*lease));lease->serial=weaponRegistrationSerial;
    lease->scene=gscTestItem;lease->server_time=level.time;
    if(!CAR_Current(&weaponRegistrationRegistry) || !GscTestItemCurrent(&lease->scene) ||
       !gsc_session_snapshot(gscTestSession,&snapshot))return 0;
    lease->logical_ms=snapshot.logical_ms;return WeaponRegistrationScope(lease);
}
static void WeaponRegistrationEnd(void) {weaponRegistrationGuard.active=0;weaponRegistrationGuard.cancelled=0;}
int G_Cod1ReadWeaponRegistration(const cod1AmmoRegistryReceipt_t *input,int selected,
    cod1WeaponRegistrationReceipt_t *out,char *error,size_t size,int (*current)(void *),void *context) {
    WeaponRegistrationLease lease;int phase,ok=0,entered;
    if(!WeaponRegistrationBuffers(input,out,error,size) || !input || !out || !current)return 0;
    if(weaponRegistrationGuard.active){weaponRegistrationGuard.cancelled=1;return 0;}
    weaponRegistrationRegistry=*input;
    entered=WeaponRegistrationBegin(&lease);
    if(!entered)goto done;
    if(!WeaponRegistrationProject(&lease,selected,&weaponRegistrationStage))goto done;
    for(phase=0;phase<2;++phase) {
        if(!WeaponRegistrationBuffers(input,out,error,size) || memcmp(input,&weaponRegistrationRegistry,sizeof(*input)) ||
           !WeaponRegistrationScope(&lease) || current(context)!=1 ||
           !WeaponRegistrationBuffers(input,out,error,size) || memcmp(input,&weaponRegistrationRegistry,sizeof(*input)) ||
           !WeaponRegistrationProject(&lease,selected,&weaponRegistrationFresh) ||
           memcmp(&weaponRegistrationFresh,&weaponRegistrationStage,sizeof(weaponRegistrationStage)))goto done;
    }
    if(!WeaponRegistrationBuffers(input,out,error,size) || memcmp(input,&weaponRegistrationRegistry,sizeof(*input)) ||
       !WeaponRegistrationScope(&lease))goto done;
    memcpy(out,&weaponRegistrationStage,sizeof(*out));ok=1;
done:
    WeaponRegistrationEnd();
    if(!ok && WeaponRegistrationBuffers(input,out,error,size))
        GscTestError(error,size,"completed dynamic weapon registration is unavailable or changed");
    return ok;
}
int G_Cod1WeaponRegistrationCurrent(const cod1WeaponRegistrationReceipt_t *input) {
    WeaponRegistrationLease lease;int ok=0;
    if(!input || WeaponRegistrationPrivate(input,sizeof(*input)))return 0;
    if(weaponRegistrationGuard.active){weaponRegistrationGuard.cancelled=1;return 0;}
    weaponRegistrationStage=*input;weaponRegistrationRegistry=weaponRegistrationStage.registry;
    if(!WeaponRegistrationBegin(&lease))goto done;
    if(WeaponRegistrationProject(&lease,weaponRegistrationStage.selectedCAPIndex,&weaponRegistrationFresh) &&
       !memcmp(&weaponRegistrationFresh,&weaponRegistrationStage,sizeof(weaponRegistrationStage)) &&
       !WeaponRegistrationPrivate(input,sizeof(*input)) && !memcmp(input,&weaponRegistrationStage,sizeof(*input)) &&
       WeaponRegistrationScope(&lease))ok=1;
done:WeaponRegistrationEnd();return ok;
}
