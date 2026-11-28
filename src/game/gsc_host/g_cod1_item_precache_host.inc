/* R121 separate startup item registration. Server registered bits precede
 * model dependencies; strict failures retain committed model prefixes and
 * refuse the operation. Client publication follows complete dependencies and
 * is not a renderer acknowledgement. R108 resource admission is unchanged. */
#include "../bg_cod1_item_precache.h"
#include "../cod1_gsc/gsc_session_clock_r50.h"
typedef struct {
    GscSession *session;
    uint64_t id,map_epoch,epoch,incarnation,operation_id;
    uint32_t module_id,fingerprint,registry_revision;
    int owned,marker_ready,failed,mode,module_ready;
    unsigned char registered[COD1_ITEM_COUNT],complete[COD1_ITEM_COUNT];
    char published[COD1_ITEM_STATE_CAPACITY];
} GscTestItemScene;
static GscTestItemScene gscTestItem;
static int gscTestItemBusy,gscTestItemInitialization;
/* A caller with an immediately proved CAR receipt can reuse its descriptor
 * fingerprint. Other callers retain the original independent full hash. */
static int GscTestItemIdentityChecked(const GscTestItemScene *lease,int validated,uint32_t fingerprint) {
    return lease && lease->owned && gscTestItem.owned && CI_Generation(lease->epoch) &&
        CI_Generation(lease->epoch)!=UINT32_MAX &&
        (lease->module_ready?CI_Epoch(lease->epoch):!CI_Module(lease->epoch)) &&
        lease->epoch==gscTestItem.epoch && lease->incarnation && lease->incarnation!=UINT64_MAX &&
        lease->incarnation==gscTestItem.incarnation && lease->module_ready==gscTestItem.module_ready &&
        (!lease->module_ready || (lease->module_id==gscTestItem.module_id && lease->module_id==CI_Module(lease->epoch))) &&
        lease->map_epoch && lease->map_epoch==gscTestItem.map_epoch && lease->map_epoch==G_Cod1EntityMapEpoch() &&
        lease->session && lease->session==gscTestItem.session && lease->session==gscTestSession &&
        lease->id && lease->id==gscTestItem.id && lease->id==gscTestId &&
        lease->fingerprint==gscTestItem.fingerprint && lease->fingerprint==(validated?fingerprint:CI_Fingerprint());
}
static int GscTestItemIdentity(const GscTestItemScene *lease) {
    return GscTestItemIdentityChecked(lease,0,0);
}
static int GscTestItemCurrentClockChecked(const GscTestItemScene *lease,int validated,uint32_t fingerprint,GscSessionClockR50 *out) {
    GscSessionClockR50 clock;
    if(!(GscTestItemIdentityChecked(lease,validated,fingerprint) && !gscTestItem.failed && (lease->mode==28 || (lease->mode==29 || (lease->mode==30 || (lease->mode==31 || (lease->mode==32 || (lease->mode==33 || (lease->mode==34 || lease->mode==35 || lease->mode==36 || lease->mode==37 || lease->mode==38 || lease->mode==39 || (lease->mode==40||(lease->mode==41||(lease->mode==42||lease->mode==43)))))))))) &&
        gscTestEntityMode==lease->mode && !gscTestCompleted && !gscTestStopRequested &&
        !level.restarted && G_Cod1MapEntities() && g_gametype.integer==GT_SINGLE_PLAYER))return 0;
    clock=gsc_session_clock_r50(lease->session,lease->id);
    if(!clock.valid || (clock.state!=GSC_SESSION_PREPARED && clock.state!=GSC_SESSION_RUNNING))return 0;
    if(out)*out=clock;
    return 1;
}
static int GscTestItemCurrentChecked(const GscTestItemScene *lease,int validated,uint32_t fingerprint) {
    return GscTestItemCurrentClockChecked(lease,validated,fingerprint,NULL);
}
static int GscTestItemCurrent(const GscTestItemScene *lease) {
    return GscTestItemCurrentChecked(lease,0,0);
}
static int GscTestItemTime(const GscTestItemScene *lease,int logical_ms) {
    GscSessionClockR50 clock;
    return logical_ms>=0 && gscTestItemInitialization && GscTestItemCurrentClockChecked(lease,0,0,&clock) &&
        clock.state==GSC_SESSION_RUNNING && clock.logical_ms==logical_ms;
}
static int GscTestItemAdmission(const GscTestItemScene *lease,int logical_ms,int running) {
    static const unsigned keys[6]={C1CP47_F_ENABLE,C1CP47_F_MODE,C1CP47_K_SAVE,
        C1CP47_F_ENABLE,C1CP47_K_SAVE,C1CP47_F_MODE};
    int values[6]={1,lease->mode,0,1,0,lease->mode};int i,value;
    for(i=0;i<6;++i) {
        if(!(running?GscTestItemTime(lease,logical_ms):GscTestItemCurrent(lease)))return 0;
        value=GscConfigInteger47(keys[i]);
        if(!(running?GscTestItemTime(lease,logical_ms):GscTestItemCurrent(lease)) || value!=values[i])return 0;
        if(lease->module_ready && (i==0 || i==5)) {
            value=trap_Cvar_VariableIntegerValue(COD1_AMBIENT_MODULE_CVAR);
            if(!(running?GscTestItemTime(lease,logical_ms):GscTestItemCurrent(lease)) || (uint32_t)value!=lease->module_id)return 0;
        }
    }
    return 1;
}
static int GscTestItemMarker(const GscTestItemScene *lease,int logical_ms,int running) {
    char actual[MAX_STRING_CHARS],expected[COD1_ITEM_STATE_CAPACITY];int i;
    if(!lease->marker_ready)return 0;
    memcpy(expected,gscTestItem.published,sizeof(expected));
    for(i=0;i<2;++i) {
        if(!GscTestItemAdmission(lease,logical_ms,running))return 0;
        memset(actual,0,sizeof(actual));trap_GetConfigstring(CS_COD1_ITEM_PRECACHE,actual,sizeof(actual));
        if(!GscTestItemAdmission(lease,logical_ms,running) || !memchr(actual,0,sizeof(actual)) || strcmp(actual,expected) ||
           strcmp(gscTestItem.published,expected))return 0;
    }
    return 1;
}
static int GscTestItemResetCurrent(uint64_t epoch,uint64_t incarnation,uint64_t map_epoch) {
    return !gscTestItem.owned && gscTestItem.epoch==epoch && gscTestItem.incarnation==incarnation &&
        map_epoch && map_epoch==G_Cod1EntityMapEpoch();
}
static void GscTestItemReset(void) {
    GscTestItemScene old=gscTestItem;uint32_t generation=CI_Generation(old.epoch);
    uint64_t epoch=generation==UINT32_MAX?UINT32_MAX:(uint64_t)generation+1;
    uint64_t incarnation=old.incarnation==UINT64_MAX?UINT64_MAX:old.incarnation+1;
    char actual[MAX_STRING_CHARS];int i;
    memset(&gscTestItem,0,sizeof(gscTestItem));gscTestItem.epoch=epoch;gscTestItem.incarnation=incarnation;
    gscTestItemInitialization=0;
    if(!old.owned || !old.marker_ready || !old.published[0])return;
    for(i=0;i<2;++i) {
        if(!GscTestItemResetCurrent(epoch,incarnation,old.map_epoch))return;
        memset(actual,0,sizeof(actual));trap_GetConfigstring(CS_COD1_ITEM_PRECACHE,actual,sizeof(actual));
        if(!GscTestItemResetCurrent(epoch,incarnation,old.map_epoch) || !memchr(actual,0,sizeof(actual)) || strcmp(actual,old.published))return;
    }
    trap_SetConfigstring(CS_COD1_ITEM_PRECACHE,"");
}
static int GscTestItemBegin(char *error,size_t capacity) {
    GscTestItemScene lease;cod1ItemState_t state;char actual[MAX_STRING_CHARS],publication[COD1_ITEM_STATE_CAPACITY];int i,module_id;
    if(gscTestItemBusy || gscTestItem.owned || !CI_Generation(gscTestItem.epoch) ||
       CI_Generation(gscTestItem.epoch)==UINT32_MAX || !gscTestItem.incarnation || gscTestItem.incarnation==UINT64_MAX)goto refuse;
    gscTestItem.registry_revision=CW_RegistryRevision();
    gscTestItem.session=gscTestSession;gscTestItem.id=gscTestId;gscTestItem.map_epoch=G_Cod1EntityMapEpoch();
    gscTestItem.mode=gscTestEntityMode;gscTestItem.fingerprint=CI_Fingerprint();gscTestItem.owned=1;lease=gscTestItem;
    if(!GscTestItemAdmission(&lease,0,0))goto refuse;
    module_id=trap_Cvar_VariableIntegerValue(COD1_AMBIENT_MODULE_CVAR);
    if(!GscTestItemCurrent(&lease) || module_id<=0)goto refuse;
    gscTestItem.module_id=(uint32_t)module_id;gscTestItem.module_ready=1;
    gscTestItem.epoch=((uint64_t)(uint32_t)module_id<<32)|CI_Generation(lease.epoch);lease=gscTestItem;
    memset(&state,0,sizeof(state));state.epoch=lease.epoch;state.fingerprint=lease.fingerprint;
    if(!CI_EncodeState(gscTestItem.published,sizeof(gscTestItem.published),&state))goto refuse;
    for(i=0;i<2;++i) {
        if(!GscTestItemAdmission(&lease,0,0))goto refuse;
        memset(actual,0,sizeof(actual));trap_GetConfigstring(CS_COD1_ITEM_PRECACHE,actual,sizeof(actual));
        if(!GscTestItemAdmission(&lease,0,0) || !memchr(actual,0,sizeof(actual)) || actual[0])goto refuse;
    }
    gscTestItem.marker_ready=1;lease=gscTestItem;
    memcpy(publication,gscTestItem.published,sizeof(publication));
    trap_SetConfigstring(CS_COD1_ITEM_PRECACHE,publication);
    if(!GscTestItemMarker(&lease,0,0))goto refuse;
    return 1;
refuse:
    GscTestError(error,capacity,"item scene unavailable, occupied or map/session/provider changed");return 0;
}
typedef struct {
    GscTestItemScene lease;
    int logical_ms,server_time,count;
    uint64_t operation_id;
    cod1ItemDescriptor_t descriptors[COD1_ITEM_COUNT];
} GscTestItemTransaction;
static int GscTestItemTransactionCurrent(const GscTestItemTransaction *transaction) {
    cod1ItemDescriptor_t current;int i;
    if(!transaction || !GscTestItemMarker(&transaction->lease,transaction->logical_ms,1) ||
       level.time!=transaction->server_time || gscTestItem.operation_id!=transaction->operation_id)return 0;
    for(i=0;i<transaction->count;++i) {
        if(!CI_Descriptor(transaction->descriptors[i].index,&current) ||
           memcmp(&current,&transaction->descriptors[i],sizeof(current)))return 0;
    }
    return 1;
}
static int GscTestItemModel(const GscTestItemTransaction *transaction,const char *name) {
    cod1ModelPrecache_t request;char registered[MAX_STRING_CHARS];int i,slot=-1,existing=0;
    if(!*name)return GscTestItemTransactionCurrent(transaction); /* proved empty-model no-op */
    if(!Cod1PrecacheName(name,1))return 0;
    for(i=1;i<MAX_MODELS;++i) {
        if(!GscTestItemTransactionCurrent(transaction))return 0;
        memset(registered,0,sizeof(registered));trap_GetConfigstring(CS_MODELS+i,registered,sizeof(registered));
        if(!GscTestItemTransactionCurrent(transaction) || !memchr(registered,0,sizeof(registered)))return 0;
        if(!registered[0]) {slot=i;break;}
        if(!strcmp(registered,name)) {slot=i;existing=1;break;}
    }
    if(slot<0)return 0;
    memset(&request,0,sizeof(request));request.version=COD1_PRECACHE_VERSION;request.bytes=sizeof(request);strcpy(request.name,name);
    if(!GscTestItemTransactionCurrent(transaction) || trap_Cod1ModelPrecache(&request)<=0 ||
       !GscTestItemTransactionCurrent(transaction))return 0;
    memset(registered,0,sizeof(registered));trap_GetConfigstring(CS_MODELS+slot,registered,sizeof(registered));
    if(!GscTestItemTransactionCurrent(transaction) || !memchr(registered,0,sizeof(registered)) ||
       (existing?strcmp(registered,name):registered[0]))return 0;
    if(existing)return 1;
    trap_SetConfigstring(CS_MODELS+slot,name);
    if(!GscTestItemTransactionCurrent(transaction))return 0;
    memset(registered,0,sizeof(registered));trap_GetConfigstring(CS_MODELS+slot,registered,sizeof(registered));
    return GscTestItemTransactionCurrent(transaction) && memchr(registered,0,sizeof(registered)) && !strcmp(registered,name);
}
static int GscTestItemPublish(void *user,uint64_t id,int logical_ms,
    const GscPrecacheItemEvent *event,char *error,size_t capacity) {
    GscPrecacheItemEvent copy;GscTestItemTransaction transaction;cod1ItemDescriptor_t descriptor;
    cod1ItemState_t state;unsigned char seen[COD1_ITEM_COUNT];char publication[COD1_ITEM_STATE_CAPACITY];int original,i,j,accepted=0;
    (void)user;
    if(gscTestItemBusy) {gscTestStopRequested=1;gscTestItem.failed=1;goto refuse;}
    if(!event || !gscTestItemInitialization)goto refuse;
    copy=*event;memset(&transaction,0,sizeof(transaction));transaction.lease=gscTestItem;
    transaction.logical_ms=logical_ms;transaction.server_time=level.time;
    if(!memchr(copy.name,0,sizeof(copy.name)) || !copy.operation_id || copy.operation_id==UINT64_MAX ||
       copy.operation_id<=transaction.lease.operation_id || id!=transaction.lease.id ||
       !GscTestItemTime(&transaction.lease,logical_ms) || !CI_Resolve(copy.name,&descriptor))goto refuse;
    original=descriptor.index;memset(seen,0,sizeof(seen));
    for(;;) {
        if(descriptor.index<1 || descriptor.index>=COD1_ITEM_COUNT || seen[descriptor.index])goto refuse;
        seen[descriptor.index]=1;transaction.descriptors[transaction.count++]=descriptor;
        if(descriptor.type!=1 || !descriptor.linked_index || descriptor.linked_index==original)break;
        if(transaction.count==COD1_ITEM_COUNT || !CI_Descriptor(descriptor.linked_index,&descriptor))goto refuse;
    }
    gscTestItemBusy=1;transaction.operation_id=transaction.lease.operation_id;
    if(!GscTestItemTransactionCurrent(&transaction))goto end;
    gscTestItem.operation_id=copy.operation_id;transaction.operation_id=copy.operation_id;
    if(gscTestItem.registered[original]) {
        if(gscTestItem.complete[original] && GscTestItemTransactionCurrent(&transaction))accepted=1;
        goto end;
    }
    /* Preserve the native state-before-model order. A failed strict resource
     * prefix is retained and poisons this scene rather than falsely succeeding
     * on a duplicate registered bit with incomplete resource dependencies. */
    for(i=0;i<transaction.count;++i) {
        descriptor=transaction.descriptors[i];gscTestItem.registered[descriptor.index]=1;
        for(j=0;j<2;++j) if(descriptor.model_present[j] && !GscTestItemModel(&transaction,descriptor.models[j]))goto end;
    }
    if(!GscTestItemTransactionCurrent(&transaction))goto end;
    memset(&state,0,sizeof(state));state.epoch=transaction.lease.epoch;state.operation_id=copy.operation_id;
    state.fingerprint=transaction.lease.fingerprint;memcpy(state.registered,gscTestItem.registered,sizeof(state.registered));
    memset(publication,0,sizeof(publication));
    if(!CI_EncodeState(publication,sizeof(publication),&state))goto end;
    memcpy(gscTestItem.published,publication,sizeof(publication));
    trap_SetConfigstring(CS_COD1_ITEM_PRECACHE,publication);
    if(!GscTestItemTransactionCurrent(&transaction))goto end;
    for(i=0;i<transaction.count;++i)gscTestItem.complete[transaction.descriptors[i].index]=1;
    accepted=1;
end:
    if(!accepted && GscTestItemIdentity(&transaction.lease))gscTestItem.failed=1;
    gscTestItemBusy=0;if(accepted)return 1;
refuse:
    GscTestError(error,capacity,"item unknown/unavailable, phase closed or map/session/provider changed; committed prefixes may remain");return 0;
}

/* Map items register during level construction even if no script calls
 * precacheItem for their weapon. Merge their definitions after script
 * initialization, before closing the shared registration window. */
static int GscTestRegisterPlacedWeapons(char *error,size_t capacity) {
    GscSessionSnapshot snapshot;
    int i;
    if(!gsc_session_snapshot(gscTestSession,&snapshot))return 0;
    for(i=1;i<level.num_entities;++i) {
        const gentity_t *ent=&g_entities[i];
        cod1ItemDescriptor_t descriptor;
        GscEntityRef owner;
        GscPrecacheItemEvent registration;
        if(!ent->inuse || !ent->item || ent->item->giType!=IT_WEAPON ||
           !ent->classname || strncmp(ent->classname,"weapon_",7) ||
           (ent->flags&FL_DROPPED_ITEM))continue;
        if(!CI_Resolve(ent->classname+7,&descriptor) || !descriptor.dynamic ||
           strcmp(descriptor.classname,ent->classname) || descriptor.type!=1)continue;
        if(gscTestItem.complete[descriptor.index])continue;
        memset(&registration,0,sizeof(registration));
        Q_strncpyz(registration.name,descriptor.internal_name,sizeof(registration.name));
        registration.operation_id=gscTestItem.operation_id+1;
        if(!G_Cod1EntityCapture(ent,&owner) ||
           !GscTestItemPublish(NULL,gscTestId,snapshot.logical_ms,&registration,error,capacity) ||
           !G_Cod1EntitySameAllocation(&owner))return 0;
        G_Printf("[CoD1 items] registered placed weapon %s at entity %d\n",descriptor.classname,i);
    }
    return 1;
}
