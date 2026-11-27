/* R203 opt-in original-allocation constructor prefix. Included only by
 * g_spawn.c after the actor-domain query/census. No controller, initialized,
 * spawned notification or network actor publication is supplied here. */
#if defined(COD1_GSC_TEST_WIP) && COD1_GSC_TEST_WIP
#include "../g_cod1_actor_bootstrap.h"
#include "../g_cod1_actor_weapon_fields_cw.h"
#include <stdlib.h>
#include "../g_cod1_node_host.h"
static cod1ActorBootstrapSnapshot_t cod1ActorPrefix;
static GscSessionActorBootstrapHost cod1ActorPrefixHost;
static GscSessionActorProgramPlan cod1ActorProgramPlan;
static cod1EarlyProgramDeclarations_t cod1EarlyProgram;
#include "../../qcommon/cod1_exact_bytes_r242.h"
#include "g_cod1_atr_seal.h"
#include "g_cod1_bsp_pages592.h"
#define COD1_EARLY_ATR_MAX 131072u
static struct {uint64_t epoch,receipt,prepass;int ready,developer,busy,cancelled;size_t sizes[2];
 unsigned char bytes[2][COD1_EARLY_ATR_MAX],reference[2][COD1_EARLY_ATR_MAX];
} cod1EarlyTurretResources;
static Cod1ATRSeal cod1EarlyTurretSeals[2];
static uint64_t cod1EarlyTurretSerial;
static int Cod1EarlyTurretLoad(void);
static void Cod1EarlyTurretReset(void){
 unsigned i;for(i=0;i<2;i++)Cod1ATRSealFree(cod1EarlyTurretSeals+i);
 cod1EarlyTurretResources.cancelled=cod1EarlyTurretResources.busy?1:0;
 cod1EarlyTurretResources.ready=0;cod1EarlyTurretResources.epoch=cod1EarlyTurretResources.receipt=cod1EarlyTurretResources.prepass=0;
 memset(cod1EarlyTurretResources.sizes,0,sizeof(cod1EarlyTurretResources.sizes));
}
static uint64_t cod1ActorPrefixSerial;
static int cod1ActorPrefixBusy;
#include "../cod1_gsc/gsc_postcohort_private.h"
static struct {int armed,invoked,body,finished,completed,checking,cancelled;const void*service;GscSession*session;uint64_t epoch,prefix,program,id,graph;} cod1ActorLate;
/* Tokenized authored records, not entity handles or execution certificates.
 * The parser already bounds each key/value record; retain exact bytes including
 * duplicate keys/order and the BSP ordinal, without lossy hashes. */
/* BSP declarations include deferred spawners; they do not consume the32 live Actor slots. */
#define COD1_ACTOR_RECORD_CAPACITY MAX_GENTITIES
typedef struct {C1BSP592_ALIGN uint32_t ordinal;int pairs,bytes;unsigned short keyOffset[MAX_SPAWN_VARS], valueOffset[MAX_SPAWN_VARS];char text[MAX_SPAWN_VARS_CHARS];} Cod1ActorPrepassRecord;
static struct {uint32_t records,seen,actors;int complete;Cod1ActorPrepassRecord rows[COD1_ACTOR_RECORD_CAPACITY];} cod1ActorPrepassRecords;

static unsigned char cod1PrepassFieldSealed[COD1_ACTOR_RECORD_CAPACITY];
typedef char Cod1ActorPrepassPageLayoutCheck[(sizeof(Cod1ActorPrepassRecord)%4096==0)?1:-1];
int G_Cod1Bsp592SealPrivate(const void*p,size_t n){return Cod1AiOverlap(p,n,cod1PrepassFieldSealed,sizeof(cod1PrepassFieldSealed));}
static void Cod1ClearCohortPlan(void);
static int Cod1CohortPlanPrivate(const void *,size_t);
static int Cod1PrepassFieldsUnsealAll(void){unsigned i;
 Cod1ClearCohortPlan();
 for(i=0;i<COD1_ACTOR_RECORD_CAPACITY;i++)if(cod1PrepassFieldSealed[i]){
  if(!C1Bsp592Pages(&cod1ActorPrepassRecords.rows[i],sizeof(Cod1ActorPrepassRecord),1))return 0;
  cod1PrepassFieldSealed[i]=0;
 }
 return 1;
}
#define COD1_EARLY_RECORD_CAPACITY 256u
static struct {uint32_t count,seen;Cod1ActorPrepassRecord rows[COD1_EARLY_RECORD_CAPACITY];} cod1EarlyRecords;
typedef struct { cod1ActorDeleteReceipt_t receipt;int state,notified; } Cod1ActorDeleteRecord;
static Cod1ActorDeleteRecord cod1ActorDeletes[COD1_ACTOR_CAPACITY];
static uint64_t cod1ActorDeleteSerial;
static int Cod1ActorDeleteAuthorized(uint32_t i) {
    const Cod1ActorDeleteRecord *d=&cod1ActorDeletes[i];
    const cod1ActorBootstrapRow_t *r=&cod1ActorPrefix.rows[i];
    return d->state==COD1_ACTOR_DELETE_RETIRED&&d->notified&&r->retired&&
        d->receipt.transaction&&d->receipt.row==i&&d->receipt.epoch==cod1ActorPrefix.epoch&&
        d->receipt.receipt==cod1ActorPrefix.receipt&&
        Cod1AiRefEqual(&d->receipt.entity,&r->entity)&&
        !memcmp(&d->receipt.lease,&r->lease,sizeof(r->lease));
}
static int Cod1ActorRecordPrivate(const void *,size_t);
static int Cod1ActorPrefixPrivate(const void *p,size_t n){
    unsigned i;
    if(Cod1AiOverlap(p,n,cod1EarlyTurretSeals,sizeof(cod1EarlyTurretSeals)))return 1;
    for(i=0;i<2;i++)if(cod1EarlyTurretSeals[i].base && Cod1AiOverlap(p,n,cod1EarlyTurretSeals[i].base,2*cod1EarlyTurretSeals[i].length))return 1;
    return Cod1CohortPlanPrivate(p,n)||G_Cod1Bsp592SealPrivate(p,n)||G_Cod1NodeGraphPostCohortPrivate(p,n)||Cod1AiOverlap(p,n,&cod1ActorLate,sizeof(cod1ActorLate))||Cod1ActorRecordPrivate(p,n)||Cod1AiOverlap(p,n,&cod1EarlyTurretResources,sizeof(cod1EarlyTurretResources))||Cod1AiOverlap(p,n,&cod1EarlyTurretSerial,sizeof(cod1EarlyTurretSerial))||Cod1AiOverlap(p,n,&cod1EarlyProgram,sizeof(cod1EarlyProgram))||Cod1AiOverlap(p,n,&cod1EarlyRecords,sizeof(cod1EarlyRecords))||Cod1AiOverlap(p,n,&cod1ActorProgramPlan,sizeof(cod1ActorProgramPlan))||Cod1AiOverlap(p,n,&cod1ActorPrepassRecords,sizeof(cod1ActorPrepassRecords))||Cod1AiPrivate(p,n)||Cod1AiOverlap(p,n,&cod1ActorPrefix,sizeof(cod1ActorPrefix))||
        Cod1AiOverlap(p,n,&cod1ActorPrefixHost,sizeof(cod1ActorPrefixHost))||
        Cod1AiOverlap(p,n,&cod1ActorPrefixSerial,sizeof(cod1ActorPrefixSerial))||
        Cod1AiOverlap(p,n,&cod1ActorPrefixBusy,sizeof(cod1ActorPrefixBusy))||
        Cod1AiOverlap(p,n,cod1ActorDeletes,sizeof(cod1ActorDeletes))||
        Cod1AiOverlap(p,n,&cod1ActorDeleteSerial,sizeof(cod1ActorDeleteSerial));
}

static int Cod1ActorPrefixOwner(void){return cod1ActorPrefix.epoch&&cod1ActorPrefix.receipt&&
    cod1ActorPrefix.epoch==G_Cod1EntityMapEpoch()&&G_Cod1MapEntities()&&g_gametype.integer==GT_SINGLE_PLAYER;}
static int Cod1ActorPrefixHeaderCurrent(uint64_t receipt){
    if(!Cod1ActorPrefixOwner()||receipt!=cod1ActorPrefix.receipt||!cod1ActorPrefix.enabled||
       !cod1ActorPrefix.declarationsReady||!cod1ActorPrefix.sealed||cod1ActorPrefix.closed||
       cod1ActorPrefix.invalid||!cod1ActorPrepassRecords.complete||level.spawning||level.restarted)return 0;
    return 1;
}
static int Cod1ActorPrefixCurrent(uint64_t receipt){uint32_t i;
    if(!Cod1ActorPrefixHeaderCurrent(receipt))return 0;
    for(i=0;i<cod1ActorPrefix.placedRows;++i){cod1ActorBootstrapRow_t *row=&cod1ActorPrefix.rows[i];
        if(Cod1ActorDeleteAuthorized(i))continue;
        if(cod1ActorDeletes[i].state==COD1_ACTOR_DELETE_FAILED)return 0;
        if(!G_Cod1EntitySameAllocation(&row->entity)||row->retired)return 0;
        if(row->lease.serial&&(!G_Cod1ActorLeaseCurrent(&row->lease)||
           !Cod1AiRefEqual(&row->lease.entity,&row->entity)))return 0;
    }
    return 1;
}
static int Cod1ActorPrefixFail(char *error,size_t size,const char *text){
    if(!cod1ActorPrefix.error[0])Cod1AiError(cod1ActorPrefix.error,sizeof(cod1ActorPrefix.error),text);
    return Cod1AiError(error,size,text);
}
#include "g_cod1_actor_postcohort.inc"
#include "g_cod1_actor_record_owner.inc"
static void Cod1ActorPrefixBegin(void){
    fileHandle_t file=0;char *text=NULL;int bytes,enabled,mode,feature;uint64_t epoch=G_Cod1EntityMapEpoch();
    static const cod1ActorDeleteOwnerHooks_t deleteOwner={G_Cod1ActorBootstrapDeleteCandidate,
        G_Cod1ActorBootstrapDeleteAdmit,G_Cod1ActorBootstrapDeleteNotified,
        G_Cod1ActorBootstrapDeleteCurrent,G_Cod1ActorBootstrapDeleteCommit,G_Cod1ActorBootstrapDeleteFail};
    Cod1EarlyTurretReset();
    Cod1ActorRecordReset();
    memset(&cod1ActorPrefix,0,sizeof(cod1ActorPrefix));memset(&cod1ActorPrefixHost,0,sizeof(cod1ActorPrefixHost));
    memset(cod1ActorDeletes,0,sizeof(cod1ActorDeletes));
    if(!Cod1PrepassFieldsUnsealAll()){G_Error("BSP prepass protection could not be released");return;}
    memset(&cod1ActorPrepassRecords,0,sizeof(cod1ActorPrepassRecords));
    memset(&cod1ActorProgramPlan,0,sizeof(cod1ActorProgramPlan));
    memset(&cod1EarlyProgram,0,sizeof(cod1EarlyProgram));memset(&cod1EarlyRecords,0,sizeof(cod1EarlyRecords));
    if(cod1ActorPrefixSerial==UINT64_MAX||!epoch||!G_Cod1MapEntities()||g_gametype.integer!=GT_SINGLE_PLAYER)return;
    cod1ActorPrefix.epoch=epoch;cod1ActorPrefix.receipt=++cod1ActorPrefixSerial;
    if(!G_Cod1EntitySelectActorDeleteOwner(epoch,&deleteOwner)){cod1ActorPrefix.invalid=1;return;}
    enabled=G_Cod1GscFeature47(C1CP47_F_ENABLE);
    if(epoch!=G_Cod1EntityMapEpoch()||enabled!=1)return;
    mode=G_Cod1GscFeature47(C1CP47_F_MODE);
    if(epoch!=G_Cod1EntityMapEpoch()||mode!=43)return;
    feature=G_Cod1GscFeature47(C1CP47_F_ACTOR_PREFIX);
    if(epoch!=G_Cod1EntityMapEpoch()||feature!=1)return;
    cod1ActorPrefix.enabled=1;bytes=trap_FS_FOpenFile("radiant/keys.txt",&file,FS_READ);
    if(bytes>0&&bytes<=COD1_BSP_DECLARATION_TEXT_MAX&&file&&epoch==G_Cod1EntityMapEpoch()) {
        text=malloc((size_t)bytes+1);
        if(text){trap_FS_Read(text,bytes,file);text[bytes]=0;}
    }
    if(file)trap_FS_FCloseFile(file);
    if(text&&epoch==G_Cod1EntityMapEpoch()&&G_Cod1MapEntities()&&g_gametype.integer==GT_SINGLE_PLAYER&&
       G_Cod1EntitySelectBspFieldDeclarationText(text,(size_t)bytes,cod1ActorPrefix.error,sizeof(cod1ActorPrefix.error)))
        cod1ActorPrefix.declarationsReady=1;
    else if(epoch==cod1ActorPrefix.epoch)Cod1ActorPrefixFail(NULL,0,"actor prefix requires genuine mounted radiant/keys.txt before BSP intake");
    if(text){memset(text,0,(size_t)bytes+1);free(text);}
}
static int Cod1ActorPrefixIntake(gentity_t *ent){
    GscEntityRef ref;cod1PlacedActorSnapshot_t placed;cod1ActorBootstrapRow_t *row;
    if(!Cod1ActorPrefixOwner()||!cod1ActorPrefix.enabled||cod1ActorPrefix.closed||!level.spawning)return 0;
    if(cod1ActorPrefix.placedRows>=COD1_ACTOR_CAPACITY||!G_Cod1EntityCapture(ent,&ref)){
        cod1ActorPrefix.invalid=1;Cod1ActorPrefixFail(NULL,0,"actor prefix original-allocation catalog capacity/identity refused");return 0;}
    if(!Cod1ActorRecordIntakeCurrent(ent))return 0;
    if(!G_Cod1PlacedActorIntake(ent,cod1ActorPrefix.error,sizeof(cod1ActorPrefix.error)))return 0;
    if(!Cod1ActorRecordIntakeCurrent(ent))return 0;
    row=&cod1ActorPrefix.rows[cod1ActorPrefix.placedRows++];row->entity=ref;
    row->constructorOrdinal=cod1AiDomain.constructorRows;
    if(!Cod1ActorRecordBind(ent,row)){cod1ActorPrefix.invalid=1;return 0;}
    Cod1AiDomainObserve(ent,0);
    if(!G_Cod1PlacedActorRead(&ref,&placed,NULL,0)||!placed.privateFieldsAvailable||!placed.nativeStaticAvailable)
        Cod1ActorPrefixFail(NULL,0,"actor prefix retained raw BSP allocation but declared private/native startup authority is unavailable");
    return 1;
}
static void Cod1ActorPrefixFinish(void){
    if(!Cod1ActorPrefixOwner()||!cod1ActorPrefix.enabled)return;
    if(cod1ActorPrefix.sealed||level.spawning||!cod1ActorPrepassRecords.complete)cod1ActorPrefix.invalid=1;
    cod1ActorPrefix.sealed=1;
    G_Printf("[GSC R203] prefix intake: placed=%u declarations=%d; hidden pending originals, controller/publication held\n",
        cod1ActorPrefix.placedRows,cod1ActorPrefix.declarationsReady);
}
static int Cod1ActorPrefixAccounting(uint64_t epoch){
    uint32_t i;cod1ActorLeaseSnapshot_t actor;
    if(!Cod1ActorPrefixCurrent(cod1ActorPrefix.receipt)||epoch!=cod1ActorPrefix.epoch||
       cod1ActorPrefix.accountedRows!=cod1ActorPrefix.placedRows)return 0;
    for(i=0;i<cod1ActorPrefix.placedRows;++i){const cod1ActorBootstrapRow_t *row=&cod1ActorPrefix.rows[i];
        if(Cod1ActorDeleteAuthorized(i))continue;
        if(!row->accounted||!G_Cod1ActorReadLease(&row->lease,&actor,NULL,0)||
           !Cod1AiRefEqual(&row->entity,&actor.lease.entity))return 0;}
    return 1;
}
static int Cod1ActorPrefixCoverage(uint64_t epoch){
    return cod1ActorPrefix.runComplete&&cod1ActorPrefix.returnedRows==cod1ActorPrefix.placedRows&&
        Cod1ActorPrefixAccounting(epoch);
}

int G_Cod1ActorBootstrapConstructedCurrent(const GscEntityRef*ref){
 uint32_t i;cod1ActorLeaseSnapshot_t snapshot;
 if(!ref||!cod1ActorPrefixBusy||!cod1ActorPrefix.runAttempted||cod1ActorPrefix.runComplete||cod1ActorPrefix.returnedRows||
    !Cod1ActorPrefixCurrent(cod1ActorPrefix.receipt)||cod1ActorPrefix.accountedRows!=cod1ActorPrefix.placedRows)return 0;
 for(i=0;i<cod1ActorPrefix.placedRows;i++){
  const cod1ActorBootstrapRow_t*r=&cod1ActorPrefix.rows[i];
  if(Cod1AiRefEqual(ref,&r->entity))return r->accounted&&!r->retired&&r->startupConsumed&&r->stage==COD1_ACTOR_ENTRY_LINKED&&
   G_Cod1ActorReadLease(&r->lease,&snapshot,NULL,0)&&Cod1AiRefEqual(ref,&snapshot.lease.entity);
 }return 0;
}

int G_Cod1ActorBootstrapEntryCurrent(const cod1ActorLease_t *lease){
 const cod1ActorBootstrapRow_t*r;
 if(!lease||!cod1ActorPrefixBusy||!cod1ActorPrefix.runAttempted||cod1ActorPrefix.runComplete||!cod1ActorPrefix.hookCount||
    cod1ActorPrefix.accountedRows!=cod1ActorPrefix.placedRows||cod1ActorPrefix.returnedRows>=cod1ActorPrefix.placedRows||
    !Cod1ActorPrefixCurrent(cod1ActorPrefix.receipt))return 0;
 r=&cod1ActorPrefix.rows[cod1ActorPrefix.returnedRows];
 return r->accounted&&!r->retired&&r->startupConsumed&&r->stage==COD1_ACTOR_ENTRY_LINKED&&!memcmp(lease,&r->lease,sizeof(*lease));
}
int G_Cod1ActorBootstrapPostinitCurrent(const cod1ActorLease_t *lease){
 const cod1ActorBootstrapRow_t*r;
 if(!lease||!cod1ActorPrefixBusy||!cod1ActorPrefix.runAttempted||cod1ActorPrefix.runComplete||
    cod1ActorPrefix.returnedRows>=cod1ActorPrefix.placedRows||
    !Cod1ActorPrefixCurrent(cod1ActorPrefix.receipt))return 0;
 r=&cod1ActorPrefix.rows[cod1ActorPrefix.returnedRows];
 return r->accounted&&!r->retired&&r->startupConsumed&&r->stage==COD1_ACTOR_ENTRY_ENTERED&&
  r->entryStatus==GSC_ENTITY_ENTRY_RETURNED&&!memcmp(lease,&r->lease,sizeof(*lease));
}
static int Cod1ActorPrefixConstruct(void *user,const GscActorBootstrapServices *services,char *error,size_t size){
    cod1AitypeResourceSelection_t selection;cod1ActorEntityHookPhaseRequest_t entities;
    cod1ActorEntityHookPhaseResult_t hooks;cod1ActorPlacedAitypeEntryRequest_t request;
    cod1ActorPlacedEntryResult_t result;cod1ActorLeaseSnapshot_t actor;
    GscActorBootstrapServices api;uint64_t epoch,receipt;uint32_t i,j;int ok=0;
    (void)user;
    if(Cod1ActorPrefixPrivate(error,size)||Cod1AiOverlap(services,services?sizeof(*services):0,error,size))return 0;
    if(!services||!services->current||services->current(services->context)!=1||cod1ActorPrefixBusy||
       !Cod1ActorPrefixCurrent(cod1ActorPrefix.receipt)||!cod1ActorPrefix.selected||cod1ActorPrefix.runAttempted)
        return Cod1ActorPrefixFail(error,size,"actor prefix requires its fresh selected map catalog and Session service lease");
    api=*services;epoch=cod1ActorPrefix.epoch;receipt=cod1ActorPrefix.receipt;
    memset(&selection,0,sizeof(selection));selection.count=cod1ActorPrefixHost.module_count;
    for(i=0;i<selection.count;++i)strcpy(selection.names[i],cod1ActorPrefixHost.module_roots[i]+7);
    memset(&entities,0,sizeof(entities));
    for(i=0;i<cod1ActorPrefix.placedRows;++i)entities.entities[entities.count++]=cod1ActorPrefix.rows[i].entity;
    if(entities.count+cod1AiDomain.spawnerRows>COD1_ACTOR_ENTITY_HOOKS_MAX)
        return Cod1ActorPrefixFail(error,size,"actor prefix hook collection exceeds complete bounded capacity");
    for(i=0;i<cod1AiDomain.spawnerRows;++i)entities.entities[entities.count++]=cod1AiDomainSpawners[i].entity;
    for(i=1;i<entities.count;++i){GscEntityRef ref=entities.entities[i];j=i;
        while(j&&entities.entities[j-1].slot>ref.slot){entities.entities[j]=entities.entities[j-1];--j;}entities.entities[j]=ref;}
    cod1ActorPrefixBusy=1;cod1ActorPrefix.runAttempted=1;
    if(!G_Cod1ActorSelectCWWeaponFields(error,size)||
       !G_Cod1ActorSelectAitypeResourcesViaServices(&api,&selection,error,size))goto done;
    for(i=0;i<cod1ActorPrefix.placedRows;++i){cod1ActorBootstrapRow_t *row=&cod1ActorPrefix.rows[i];
        if(!Cod1ActorPrefixCurrent(receipt)||api.current(api.context)!=1){ok=0;Cod1ActorPrefixFail(error,size,"actor prefix map/Session changed before original entry");goto done;}
        if(!Cod1ActorRecordConsume(row,&api,error,size)){ok=0;goto done;}
        memset(&request,0,sizeof(request));request.entity=row->entity;
        /* Genuine fresh BSP semantic seeds are0/0: native entity clear and
         * spawn/field tables do not write A0/134 before constructor ORs.
         * These are not copied from the port r.svFlags/flags namespace. */
        memset(&result,0,sizeof(result));ok=G_Cod1ActorConstructPlacedAitype(&api,&request,&result,error,size);
        if(epoch!=cod1ActorPrefix.epoch||receipt!=cod1ActorPrefix.receipt){ok=0;goto done;}
        row=&cod1ActorPrefix.rows[i];row->stage=result.stage;row->startupConsumed=result.startupConsumed;
        row->entryStatus=result.entry.status;if(result.lease.serial)row->lease=result.lease;
        if(!row->accounted&&result.leaseRetained&&G_Cod1ActorReadLease(&result.lease,&actor,NULL,0)&&
           Cod1AiRefEqual(&actor.lease.entity,&row->entity)) {
            row->accounted=1;++cod1ActorPrefix.accountedRows;
            if(!cod1AiDomain.unaccountedRows){cod1AiDomain.invalid=1;ok=0;goto done;}
            --cod1AiDomain.unaccountedRows;++cod1AiDomainAccounted;
        }
        G_Printf("[GSC R203] actor prefix ordinal=%u entity=%u generation=%u stage=%d entry=%d lease=%llu; initialized/controller/publication held\n",
            row->constructorOrdinal,row->entity.slot,row->entity.generation,row->stage,row->entryStatus,(unsigned long long)row->lease.serial);
        if(epoch!=cod1ActorPrefix.epoch||receipt!=cod1ActorPrefix.receipt){ok=0;goto done;}
        row=&cod1ActorPrefix.rows[i];
        if(!Cod1ActorPrefixCurrent(receipt)||api.current(api.context)!=1){ok=0;goto done;}
        if(!ok||row->stage!=COD1_ACTOR_ENTRY_LINKED||!row->accounted){ok=0;if(!error||!size||!error[0])Cod1ActorPrefixFail(error,size,"actor construction failed before hooks");goto done;}
    }
    ok=Cod1ActorPrefixAccounting(epoch)&&api.current(api.context)==1;
    if(ok)cod1ActorPrefix.constructionComplete=1;
done:
    if(epoch==cod1ActorPrefix.epoch&&receipt==cod1ActorPrefix.receipt&&!ok){
        cod1ActorPrefix.runComplete=0;cod1AiDomain.complete=0;
        if(error&&size&&error[0])Cod1ActorPrefixFail(NULL,0,error);
    }
    cod1ActorPrefixBusy=0;return ok;
}
static int Cod1ActorPrefixNodeTarget(void*user,GscSession*s,uint64_t id,uint64_t receipt,const GscEntityRef*row,const char*name,GscActorBootstrapNodeTarget*out){
 cod1ActorLease_t lease;int ok;(void)user;
 if(!row||cod1ActorPrefix.returnedRows>=cod1ActorPrefix.placedRows)return 0;
 lease=cod1ActorPrefix.rows[cod1ActorPrefix.returnedRows].lease;
 if(!Cod1AiRefEqual(row,&lease.entity)||!G_Cod1ActorBootstrapPostinitCurrent(&lease))return 0;
 ok=G_Cod1NodeGraphActorTarget(s,id,receipt,row,name,out);
 return ok&&G_Cod1ActorBootstrapPostinitCurrent(&lease);
}
static int Cod1ActorPrefixNodeTargetEnd(void*user,GscSession*s,uint64_t id,uint64_t receipt,const GscEntityRef*row,int keep){
 cod1ActorLease_t lease;int current=0;(void)user;
 if(row&&cod1ActorPrefix.returnedRows<cod1ActorPrefix.placedRows){lease=cod1ActorPrefix.rows[cod1ActorPrefix.returnedRows].lease;
  current=Cod1AiRefEqual(row,&lease.entity)&&G_Cod1ActorBootstrapPostinitCurrent(&lease);}
 return G_Cod1NodeGraphActorTargetEnd(s,id,receipt,row,keep&&current);
}
static int Cod1ActorPrefixRun(void *user,const GscActorBootstrapServices *services,char *error,size_t size){
    cod1AitypeResourceSelection_t selection;cod1ActorEntityHookPhaseRequest_t entities;
    cod1ActorEntityHookPhaseResult_t hooks;cod1ActorPlacedAitypeEntryRequest_t request;
    cod1ActorPlacedEntryResult_t result;cod1ActorLeaseSnapshot_t actor;
    GscActorBootstrapServices api;uint64_t epoch,receipt;uint32_t i,j;int ok=0;
    (void)user;
    if(Cod1ActorPrefixPrivate(error,size)||Cod1AiOverlap(services,services?sizeof(*services):0,error,size))return 0;
    if(!services||!services->current||services->current(services->context)!=1||cod1ActorPrefixBusy||
       !Cod1ActorPrefixCurrent(cod1ActorPrefix.receipt)||!cod1ActorPrefix.selected||!cod1ActorPrefix.runAttempted||!cod1ActorPrefix.constructionComplete||cod1ActorPrefix.entryAttempted)
        return Cod1ActorPrefixFail(error,size,"actor prefix requires its fresh selected map catalog and Session service lease");
    api=*services;epoch=cod1ActorPrefix.epoch;receipt=cod1ActorPrefix.receipt;
    memset(&selection,0,sizeof(selection));selection.count=cod1ActorPrefixHost.module_count;
    for(i=0;i<selection.count;++i)strcpy(selection.names[i],cod1ActorPrefixHost.module_roots[i]+7);
    memset(&entities,0,sizeof(entities));
    for(i=0;i<cod1ActorPrefix.placedRows;++i)entities.entities[entities.count++]=cod1ActorPrefix.rows[i].entity;
    if(entities.count+cod1AiDomain.spawnerRows>COD1_ACTOR_ENTITY_HOOKS_MAX)
        return Cod1ActorPrefixFail(error,size,"actor prefix hook collection exceeds complete bounded capacity");
    for(i=0;i<cod1AiDomain.spawnerRows;++i)entities.entities[entities.count++]=cod1AiDomainSpawners[i].entity;
    for(i=1;i<entities.count;++i){GscEntityRef ref=entities.entities[i];j=i;
        while(j&&entities.entities[j-1].slot>ref.slot){entities.entities[j]=entities.entities[j-1];--j;}entities.entities[j]=ref;}
    cod1ActorPrefixBusy=1;cod1ActorPrefix.entryAttempted=1;
    /* Original initial construction precedes export2 preparation and export7
     * entity hooks. The graph phase itself remains a separate integration gate. */
    if(!Cod1ActorPrefixAccounting(epoch)||api.current(api.context)!=1){ok=0;goto done;}
    if(!G_Cod1ActorRunEntityHookPhaseViaServices(&api,&entities,&hooks,error,size)){ok=0;goto done;}
    cod1ActorPrefix.hookCount=hooks.hookCount;
    if(!hooks.phaseInvoked||!hooks.allReturned){ok=0;Cod1ActorPrefixFail(error,size,"constructed-cohort hooks did not return");goto done;}
    for(i=0;i<cod1ActorPrefix.placedRows;i++){
        cod1ActorBootstrapRow_t*row=&cod1ActorPrefix.rows[i];GscEntityScriptRun entry;cod1ActorLease_t lease=row->lease;
        if(!Cod1ActorPrefixCurrent(receipt)||api.current(api.context)!=1){ok=0;goto done;}
        memset(&entry,0,sizeof(entry));ok=G_Cod1ActorEnterConstructedAitype(&api,&lease,&entry,error,size);
        if(epoch!=cod1ActorPrefix.epoch||receipt!=cod1ActorPrefix.receipt){ok=0;goto done;}
        row=&cod1ActorPrefix.rows[i];row->entryStatus=entry.status;
        if(!ok||entry.status!=GSC_ENTITY_ENTRY_RETURNED||!Cod1ActorPrefixCurrent(receipt)||api.current(api.context)!=1){ok=0;Cod1ActorPrefixFail(error,size,"constructed aitype did not return");goto done;}
        row->stage=COD1_ACTOR_ENTRY_ENTERED;
        /* Partial native4D7E0 continuation on THIS returned row. Optional
         * target, original INIT/state/tail and complete ordering remain held. */
        {cod1ActorLease_t lease=row->lease;
         if(!G_Cod1ActorPostinitPosition(&lease)){
          ok=0;Cod1ActorPrefixFail(error,size,"actor postinit position/yaw admission refused");goto done;}}
        if(api.node_target&&!G_Cod1ActorPlacedPostinitTarget(&api,&lease,error,size)){
         ok=0;Cod1ActorPrefixFail(error,size,"actor optional target postinit refused");goto done;}
        if(api.placed_init_initial){GscPlacedInitRun init;int accepted;
         if(row->placedInitAttempted){ok=0;Cod1ActorPrefixFail(error,size,"placed INIT row already attempted");goto done;}
         row->placedInitAttempted=1;memset(&init,0,sizeof(init));
         accepted=G_Cod1ActorPlacedInitInitial(&api,&lease,&init,error,size);
         if(epoch!=cod1ActorPrefix.epoch||receipt!=cod1ActorPrefix.receipt){ok=0;goto done;}
         row=&cod1ActorPrefix.rows[i];row->placedInit=init;row->placedInitAccepted=accepted;
         if(!accepted){ok=0;Cod1ActorPrefixFail(error,size,"placed original INIT initial invocation refused");goto done;}
         /* Real original INIT may have returned with live children. Do not
          * borrow whole-prefix/factory authority, forge a graph, or advance
          * another Actor before the actual current-state dispatch is owned. */
         if(!api.placed_cohort){ok=0;Cod1ActorPrefixFail(error,size,init.currentstate_completed?"placed native currentstate completed; exact retained task-set before next Actor remains open":"placed INIT initial invocation completed; native current-state/animation-graph owner remains open");goto done;}
         if(!init.currentstate_completed||api.current(api.context)!=1){ok=0;goto done;}
        }
        ++cod1ActorPrefix.returnedRows;
    }
    if(api.placed_cohort){int completed;
     if(!api.postcohort_node_owners||cod1ActorLate.armed){ok=0;Cod1ActorPrefixFail(error,size,"post-cohort requires its unused native pending frame");goto done;}
     memset(&cod1ActorLate,0,sizeof(cod1ActorLate));cod1ActorLate.armed=1;cod1ActorLate.service=api.context;
     cod1ActorLate.epoch=epoch;cod1ActorLate.prefix=receipt;cod1ActorLate.program=api.program_identity(api.context);
     ok=cod1ActorLate.program&&api.postcohort_node_owners(api.context,error,size)==1;
     completed=cod1ActorLate.service==api.context&&cod1ActorLate.finished&&cod1ActorLate.completed;
     if(cod1ActorLate.service==api.context)memset(&cod1ActorLate,0,sizeof(cod1ActorLate));
     if(epoch!=cod1ActorPrefix.epoch||receipt!=cod1ActorPrefix.receipt){ok=0;goto done;}
     if(!ok||!completed){ok=0;Cod1ActorPrefixFail(error,size,"post-cohort actual late native owner phase refused");goto done;}
     G_Printf("[Training startup] completed late path scan after %u placed actors\n",cod1ActorPrefix.returnedRows);
    }
    ok=Cod1ActorPrefixAccounting(epoch)&&api.current(api.context)==1&&
        cod1ActorPrefix.returnedRows==cod1ActorPrefix.placedRows;
    if(ok){cod1ActorPrefix.runComplete=1;cod1AiCoverage=Cod1ActorPrefixCoverage;
        cod1AiDomain.complete=!cod1AiDomain.invalid&&!cod1AiDomain.unaccountedRows&&
            cod1AiDomain.constructorRows==cod1AiDomain.spawnerRows+cod1AiDomainAccounted;
        G_Printf("[GSC R203] ACTOR_PREFIX_COMPLETE placed=%u returned=%u resources=%u hooks=%u; controller/postinit/publication held\n",
        cod1ActorPrefix.placedRows,cod1ActorPrefix.returnedRows,cod1ActorPrefix.resourceCount,cod1ActorPrefix.hookCount);
        if(epoch!=cod1ActorPrefix.epoch||receipt!=cod1ActorPrefix.receipt||
           !Cod1ActorPrefixCurrent(receipt)||api.current(api.context)!=1)ok=0;}
done:
    if(epoch==cod1ActorPrefix.epoch&&receipt==cod1ActorPrefix.receipt&&!ok){
        cod1ActorPrefix.runComplete=0;cod1AiDomain.complete=0;
        if(error&&size&&error[0])Cod1ActorPrefixFail(NULL,0,error);
    }
    cod1ActorPrefixBusy=0;return ok;
}
static int Cod1ActorPrefixAddModuleName(GscSessionActorBootstrapHost *host,const char *name){
    size_t n;uint32_t i;
    if(!name||strncmp(name,"actor_",6))return 0;
    name+=6;for(n=0;name[n];++n)if(n>=120||!((name[n]>='a'&&name[n]<='z')||(name[n]>='0'&&name[n]<='9')||name[n]=='_'))return 0;
    if(!n)return 0;for(i=0;i<host->module_count;++i)if(!strcmp(host->module_roots[i]+7,name))return 1;
    if(host->module_count>=GSC_ACTOR_BOOTSTRAP_MODULE_CAPACITY)return 0;
    strcpy(host->module_roots[host->module_count],"aitype/");strcat(host->module_roots[host->module_count++],name);return 1;
}
static int Cod1ActorPrefixAddModule(GscSessionActorBootstrapHost *host,const GscEntityRef *ref){
    return G_Cod1EntitySameAllocation(ref)&&Cod1ActorPrefixAddModuleName(host,g_entities[ref->slot].classname);
}
/* No entity/Actor allocation exists in this pass. These roots are declarations,
 * not execution authority. The actual allocated cohort must later match them. */
static int Cod1ActorPrepassRecordSave(void);
static int Cod1EarlyDeclaration(const char *name);
static int Cod1ActorPrefixPrepassClassname(const char *name){
 if(!Cod1ActorPrefixOwner()||!cod1ActorPrefix.enabled||cod1ActorPrefix.invalid||cod1ActorPrefix.closed||
    cod1ActorPrefix.prepassReady||cod1ActorPrefix.placedRows||cod1ActorPrefix.selected||cod1ActorPrefix.runAttempted||!level.spawning)return 0;
 if(!name||Q_stricmpn(name,"actor_",6))return Cod1EarlyDeclaration(name);
 if(!Cod1ActorPrefixAddModuleName(&cod1ActorPrefixHost,name)){
  cod1ActorPrefix.invalid=1;return Cod1ActorPrefixFail(NULL,0,"early actor module declaration is outside supported canonical domain");}
 if(!Cod1ActorPrepassRecordSave())return 0;
 ++cod1ActorPrefix.prepassRows;cod1ActorPrefix.prepassModuleCount=cod1ActorPrefixHost.module_count;return 1;
}
static int Cod1ActorPrepassRefuse(const char *text){
 cod1ActorPrefix.invalid=1;return Cod1ActorPrefixFail(NULL,0,text);
}
/* Original 2FA30 families: Begin.animscript and misc_mg42/misc_turret.
 * 22EE0 requires class6 and reads WeaponDef.script. This is an owned metadata
 * snapshot, not the original VM function/string cache or live WeaponDef lease. */
static int Cod1EarlyFamily(const char *name){
 if(!name)return 0;if(!Q_stricmp(name,"node_negotiation_begin"))return 1;
 return (!Q_stricmp(name,"misc_mg42")||!Q_stricmp(name,"misc_turret"))?2:0;
}
static int Cod1EarlyPath(const char *s,size_t max,int slash){size_t n;
 if(!s||!s[0])return 0;
 for(n=0;s[n];++n){char c=s[n];if(n>=max||!((c>='a'&&c<='z')||(c>='0'&&c<='9')||c=='_'||(slash&&c=='/')))return 0;
  if(c=='/'&&(!n||s[n-1]=='/'||!s[n+1]))return 0;}
 return 1;
}
static int Cod1EarlyWeapon(const char *name,char module[128]){
 fileHandle_t file=0;char path[96],bytes[CW_TEXT_SIZE];cwDef_t def;
 const char *script;int length,ok;uint64_t epoch=cod1ActorPrefix.epoch,receipt=cod1ActorPrefix.receipt;
 module[0]=0;if(!Cod1EarlyPath(name,63,0))return 0;
 Com_sprintf(path,sizeof(path),"weapons/sp/%s",name);
 length=trap_FS_FOpenFile(path,&file,FS_READ);
 ok=file&&length>0&&length<(int)sizeof(bytes);
 if(ok){memset(bytes,0,sizeof(bytes));trap_FS_Read(bytes,length,file);}
 if(file)trap_FS_FCloseFile(file);
 if(!Cod1ActorPrefixOwner()||epoch!=cod1ActorPrefix.epoch||receipt!=cod1ActorPrefix.receipt||
    cod1ActorPrefix.closed||cod1ActorPrefix.invalid||!level.spawning)return 0;
 if(!ok||memchr(bytes,0,(size_t)length)||!CW_Parse(&def,name,bytes,length)||strcmp(CW_String(&def,"weaponClass"),"turret"))return 0;
 script=CW_String(&def,"script");if(!script[0])return 1;
 if(!Cod1EarlyPath(script,116,1))return 0;
 strcpy(module,"animscripts/");strcat(module,script);return 1;
}
static int Cod1EarlyDeclaration(const char *name){
 int family=Cod1EarlyFamily(name);char *value,module[128];uint32_t i;Cod1ActorPrepassRecord *r;
 if(!family)return 1;
 if(cod1EarlyRecords.count>=COD1_EARLY_RECORD_CAPACITY||level.numSpawnVars<1||
    level.numSpawnVarChars<1||level.numSpawnVarChars>MAX_SPAWN_VARS_CHARS)
  return Cod1ActorPrepassRefuse("non-Actor declaration exceeds selected parser capacity");
 r=&cod1EarlyRecords.rows[cod1EarlyRecords.count];r->ordinal=cod1ActorPrepassRecords.records;
 r->pairs=level.numSpawnVars;r->bytes=level.numSpawnVarChars;memcpy(r->text,level.spawnVarChars,(size_t)r->bytes);
 if(family==1){
  G_SpawnString("animscript","",&value);
  if(value[0]){
   if(!gsc_node_script_name_valid(value))return Cod1ActorPrepassRefuse("early negotiation name outside selected canonical domain");
   for(i=0;i<cod1EarlyProgram.negotiation.count;i++)if(!strcmp(cod1EarlyProgram.negotiation.names[i],value))break;
   if(i==cod1EarlyProgram.negotiation.count){if(i==GSC_NODE_SCRIPT_CAPACITY)return Cod1ActorPrepassRefuse("early negotiation declarations full");
    strcpy(cod1EarlyProgram.negotiation.names[cod1EarlyProgram.negotiation.count++],value);}
   ++cod1EarlyProgram.negotiationRequests;
  }
 }else{
  G_SpawnString("weaponinfo","",&value);
  if(!Cod1EarlyWeapon(value,module))return Cod1ActorPrepassRefuse("early turret weapon declaration unavailable or invalid");
  for(i=0;i<cod1EarlyProgram.turretCount;i++)if(!strcmp(cod1EarlyProgram.turrets[i].weapon,value))break;
  if(i<cod1EarlyProgram.turretCount){if(strcmp(cod1EarlyProgram.turrets[i].module,module))return Cod1ActorPrepassRefuse("early turret declaration changed within pass");}
  else {if(i==COD1_EARLY_TURRET_CAPACITY)return Cod1ActorPrepassRefuse("early turret declarations full");
   strcpy(cod1EarlyProgram.turrets[i].weapon,value);strcpy(cod1EarlyProgram.turrets[i].module,module);++cod1EarlyProgram.turretCount;}
  ++cod1EarlyProgram.turretRequests;
 }
 ++cod1EarlyRecords.count;return 1;
}
static int Cod1EarlyRecordAdmit(const char *name){
 const Cod1ActorPrepassRecord *r=cod1EarlyRecords.seen<cod1EarlyRecords.count?&cod1EarlyRecords.rows[cod1EarlyRecords.seen]:NULL;
 /* Check the expected ordinal even if classname was changed to another family. */
 if(!Cod1EarlyFamily(name)&&(!r||r->ordinal>cod1ActorPrepassRecords.seen))return 1;
 if(!r||r->ordinal!=cod1ActorPrepassRecords.seen||r->pairs!=level.numSpawnVars||
    r->bytes!=level.numSpawnVarChars||r->bytes<1||r->bytes>MAX_SPAWN_VARS_CHARS||
    memcmp(r->text,level.spawnVarChars,(size_t)r->bytes))return Cod1ActorPrepassRefuse("non-Actor BSP record differs from early declaration");
 ++cod1EarlyRecords.seen;return 1;
}
static int Cod1ActorPrepassRecordSave(void){
 Cod1ActorPrepassRecord *r;
 if(cod1ActorPrefix.prepassRows>=COD1_ACTOR_RECORD_CAPACITY||level.numSpawnVars<1||
    level.numSpawnVarChars<1||level.numSpawnVarChars>MAX_SPAWN_VARS_CHARS)
  return Cod1ActorPrepassRefuse("early Actor record exceeds selected parser capacity");
 r=&cod1ActorPrepassRecords.rows[cod1ActorPrefix.prepassRows];
 r->ordinal=cod1ActorPrepassRecords.records;r->pairs=level.numSpawnVars;r->bytes=level.numSpawnVarChars;
 /* Parse the independently retained prepass once. Keep exact boundaries
  * alongside its bytes; intake builds its own offsets from the live spawn. */
 if(r->pairs>MAX_SPAWN_VARS)return Cod1ActorPrepassRefuse("too many Actor fields");
 {
  size_t cursor=0;int p;
  for(p=0;p<r->pairs;p++){
   const char *end;
   r->keyOffset[p]=(unsigned short)cursor;
   end=memchr(level.spawnVarChars+cursor,0,(size_t)r->bytes-cursor);
   if(!end)return Cod1ActorPrepassRefuse("unterminated Actor field key");
   cursor=(size_t)(end-level.spawnVarChars)+1;
   r->valueOffset[p]=(unsigned short)cursor;
   end=memchr(level.spawnVarChars+cursor,0,(size_t)r->bytes-cursor);
   if(!end)return Cod1ActorPrepassRefuse("unterminated Actor field value");
   cursor=(size_t)(end-level.spawnVarChars)+1;
  }
  if(cursor!=(size_t)r->bytes)return Cod1ActorPrepassRefuse("trailing Actor fields");
 }
 memcpy(r->text,level.spawnVarChars,(size_t)r->bytes);
 cod1PrepassFieldSealed[cod1ActorPrefix.prepassRows]=(unsigned char)C1Bsp592Pages(r,sizeof(*r),0);return 1;
}
/* Called by the actual spawn loop BEFORE G_SpawnGEntityFromSpawnVars/G_Spawn. */
static int Cod1ActorPrepassRecordAdmit(void){
 char *name;const Cod1ActorPrepassRecord *r;
 if(!cod1ActorPrefix.enabled)return 1;
 if(!Cod1ActorPrefixOwner()||cod1ActorPrefix.invalid||cod1ActorPrefix.closed||!level.spawning||
    !cod1ActorPrefix.prepassReady||cod1ActorPrefix.sealed||cod1ActorPrefix.selected||
    cod1ActorPrepassRecords.complete||cod1ActorPrepassRecords.seen>=cod1ActorPrepassRecords.records)
  return Cod1ActorPrepassRefuse("Actor BSP record replay outside current prepass");
 if(!Cod1ActorRecordPreviousComplete())return Cod1ActorPrepassRefuse("previous Actor record has no terminal allocation outcome");
 ++cod1ActorPrepassRecords.seen;G_SpawnString("classname","",&name);
 if(!Cod1EarlyRecordAdmit(name))return 0;
 if(Q_stricmpn(name,"actor_",6))return 1;
 if(cod1ActorPrepassRecords.actors>=cod1ActorPrefix.prepassRows)
  return Cod1ActorPrepassRefuse("Actor BSP record replay has extra Actor");
 r=&cod1ActorPrepassRecords.rows[cod1ActorPrepassRecords.actors];
 if(r->ordinal!=cod1ActorPrepassRecords.seen||r->pairs!=level.numSpawnVars||
    r->bytes!=level.numSpawnVarChars||r->bytes<1||r->bytes>MAX_SPAWN_VARS_CHARS||
    memcmp(r->text,level.spawnVarChars,(size_t)r->bytes))
  return Cod1ActorPrepassRefuse("Actor BSP record differs from early authored declaration");
 ++cod1ActorPrepassRecords.actors;return 1;
}
static int Cod1ActorPrepassRecordFinish(void){
 if(!cod1ActorPrefix.enabled)return 1;
 if(!Cod1ActorPrefixOwner()||cod1ActorPrefix.invalid||cod1ActorPrefix.closed||!level.spawning||
    !cod1ActorPrefix.prepassReady||cod1ActorPrepassRecords.complete||
    cod1ActorPrepassRecords.seen!=cod1ActorPrepassRecords.records||
    cod1ActorPrepassRecords.actors!=cod1ActorPrefix.prepassRows||cod1EarlyRecords.seen!=cod1EarlyRecords.count||
    !Cod1ActorRecordPreviousComplete())
  return Cod1ActorPrepassRefuse("Actor BSP record replay incomplete or repeated");
 cod1ActorPrepassRecords.complete=1;return 1;
}
qboolean G_ParseSpawnVars(void);
static void Cod1ActorPrefixPrepass(void){
 char *name;
 if(!cod1ActorPrefix.enabled)return;
 if(cod1ActorPrefix.invalid||cod1ActorPrefix.prepassReady||!G_ParseSpawnVars()){
  G_Error("CoD1 early actor program prepass has no fresh world record");return;}
 while(G_ParseSpawnVars()){
  if(cod1ActorPrepassRecords.records==UINT32_MAX){Cod1ActorPrepassRefuse("BSP record ordinal exhausted");G_Error("CoD1 prepass record ordinal exhausted");return;}
  ++cod1ActorPrepassRecords.records;
  G_SpawnString("classname","",&name);
  if(!Cod1ActorPrefixPrepassClassname(name)){G_Error("CoD1 early actor program prepass refused: %s",cod1ActorPrefix.error);return;}
 }
 if(!trap_Cod1EntityParseRewind()){cod1ActorPrefix.invalid=1;Cod1ActorPrefixFail(NULL,0,"ENGINE refused exhausted init cursor rewind");G_Error("CoD1 early actor program prepass rewind refused");return;}
 cod1EarlyProgram.epoch=cod1ActorPrefix.epoch;cod1EarlyProgram.receipt=cod1ActorPrefix.receipt;
 cod1ActorPrefix.prepassReady=1;level.numSpawnVars=0;level.numSpawnVarChars=0;
 if(cod1EarlyProgram.turretCount&&!Cod1EarlyTurretLoad()){
  Cod1ActorPrepassRefuse("early turret ATR/settings resource acquisition refused");G_Error("CoD1 early turret resources refused");return;
 }
}
static int Cod1ActorProgramCurrent(void*unused,uint64_t epoch,uint64_t receipt){
 (void)unused;return Cod1ActorPrefixOwner()&&epoch==cod1ActorPrefix.epoch&&receipt==cod1ActorPrefix.receipt&&
 cod1ActorPrefix.enabled&&cod1ActorPrefix.declarationsReady&&cod1ActorPrefix.prepassReady&&!cod1ActorPrefix.closed&&!cod1ActorPrefix.invalid;
}
#include "g_cod1_early_turret_resources.inc"
int G_Cod1EarlyNodeProgramPlan(GscSessionNodeProgramPlan *out){
 GscSessionNodeProgramPlan plan;
 if(!out||Cod1ActorPrefixPrivate(out,sizeof(*out))||!Cod1ActorProgramCurrent(NULL,cod1EarlyProgram.epoch,cod1EarlyProgram.receipt))return 0;
 memset(&plan,0,sizeof(plan));plan.epoch=cod1EarlyProgram.epoch;plan.receipt=cod1EarlyProgram.receipt;
 plan.current=Cod1ActorProgramCurrent;plan.scripts=cod1EarlyProgram.negotiation;*out=plan;return 1;
}
int G_Cod1EarlyProgramRead(cod1EarlyProgramDeclarations_t *out,char *error,size_t size){
 if(!out||Cod1ActorPrefixPrivate(out,sizeof(*out))||Cod1ActorPrefixPrivate(error,size)||Cod1AiOverlap(out,sizeof(*out),error,size))return 0;
 if(!Cod1ActorProgramCurrent(NULL,cod1EarlyProgram.epoch,cod1EarlyProgram.receipt))return Cod1AiError(error,size,"early program declarations have no current owner");
 *out=cod1EarlyProgram;return 1;
}
int G_Cod1EarlyTurretModule(const char *weapon,char *out,size_t size){uint32_t i;
 if(!weapon||!out||size<1||Cod1ActorPrefixPrivate(out,size)||
    !Cod1ActorProgramCurrent(NULL,cod1EarlyProgram.epoch,cod1EarlyProgram.receipt))return 0;
 for(i=0;i<cod1EarlyProgram.turretCount;i++)if(!strcmp(weapon,cod1EarlyProgram.turrets[i].weapon)){
  const char *s=cod1EarlyProgram.turrets[i].module;size_t n=strlen(s);if(!n||n>=size)return 0;memmove(out,s,n+1);return 1;}
 return 0;
}
const GscSessionActorProgramPlan *G_Cod1ActorProgramPlan(void){
 if(!Cod1ActorProgramCurrent(NULL,cod1ActorPrefix.epoch,cod1ActorPrefix.receipt)||!cod1ActorPrefix.prepassModuleCount)return NULL;
 memset(&cod1ActorProgramPlan,0,sizeof(cod1ActorProgramPlan));cod1ActorProgramPlan.epoch=cod1ActorPrefix.epoch;
 cod1ActorProgramPlan.receipt=cod1ActorPrefix.receipt;cod1ActorProgramPlan.current=Cod1ActorProgramCurrent;
 cod1ActorProgramPlan.module_count=cod1ActorPrefix.prepassModuleCount;
 memcpy(cod1ActorProgramPlan.module_roots,cod1ActorPrefixHost.module_roots,sizeof(cod1ActorProgramPlan.module_roots));
 return &cod1ActorProgramPlan;
}
/* Separate already-constructed cohort lane. Ordinary late intake keeps its
 * unaccounted/unconsumed admission rules. This receipt authorizes adoption,
 * while later script entries still obtain fresh native certificates. */
/* Only actual protected prepass storage can authorize comparison reuse.
 * Foreign buffers, wrong bounds and a replaced map retain the full scan. */
uint64_t G_Cod1Bsp592FieldsSealed(const char *bytes,size_t size,size_t pairs,const unsigned short *keys,const unsigned short *values){
 uintptr_t first=(uintptr_t)cod1ActorPrepassRecords.rows[0].text,address=(uintptr_t)bytes,offset;
 size_t index;const Cod1ActorPrepassRecord *row;
 if(!bytes||address<first||!Cod1ActorPrefixOwner()||!cod1ActorPrefix.receipt)return 0;
 offset=address-first;if(offset%sizeof(Cod1ActorPrepassRecord))return 0;index=offset/sizeof(Cod1ActorPrepassRecord);
 if(index>=COD1_ACTOR_RECORD_CAPACITY||index>=cod1ActorPrepassRecords.actors||!cod1PrepassFieldSealed[index])return 0;
 row=cod1ActorPrepassRecords.rows+index;
 return row->bytes>0&&size==(size_t)row->bytes&&row->pairs>0&&pairs==(size_t)row->pairs&&keys==row->keyOffset&&values==row->valueOffset?cod1ActorPrefix.receipt:0;
}
static int Cod1ActorBspRecordRetained(const Cod1ActorRecordOwner*r,const cod1ActorBootstrapRow_t*row){
 const Cod1ActorPrepassRecord *saved;
 if(!r||!row||r->index!=row->actorRecord||r->ordinal!=row->bspOrdinal||r->index>=cod1ActorPrepassRecords.actors)return 0;
 saved=&cod1ActorPrepassRecords.rows[r->index];
 return saved->ordinal==r->ordinal && saved->bytes>0 && saved->bytes<=MAX_SPAWN_VARS_CHARS &&
    saved->pairs>0 && G_Cod1PlacedActorMatchesFields(&row->entity,saved->text,saved->bytes,saved->pairs,saved->keyOffset,saved->valueOffset);
}
/* The original proof establishes the relationships among bootstrap rows
 * and construction records. Reuse them only on exact equality of both spans.
 * Every call still examines the live world, leases, deletion status and both
 * seals. Unprotecting prepass memory invalidates the plan before any write. */
typedef struct {
    uint64_t epoch,receipt;
    unsigned count,first,span,prepassCount;
    cod1ActorBootstrapRow_t rows[COD1_ACTOR_CAPACITY];
    Cod1ActorRecordOwner owners[COD1_ACTOR_CAPACITY];
    cod1BspLiveRow_t live[COD1_ACTOR_CAPACITY];
} Cod1CohortPlan;
static Cod1CohortPlan cod1CohortPlan;
static void Cod1ClearCohortPlan(void){cod1CohortPlan.receipt=0;}
static int Cod1CohortPlanPrivate(const void *p,size_t n){
    return Cod1AiOverlap(p,n,&cod1CohortPlan,sizeof(cod1CohortPlan));
}
static int Cod1CohortHeader(uint64_t receipt){
 if(!Cod1ActorPrefixHeaderCurrent(receipt)||!cod1ActorPrefix.prepassReady||!cod1ActorPrefix.placedRows||
    cod1AiDomain.invalid||!cod1AiDomain.sealed||cod1AiDomain.unaccountedRows||
    cod1ActorPrefix.accountedRows!=cod1ActorPrefix.placedRows||
    cod1ActorPrefix.prepassRows!=cod1AiDomain.constructorRows)return 0;
 return 1;
}
static int Cod1CohortRowsFull(uint64_t receipt){uint32_t i;uint64_t program=0;
 for(i=0;i<cod1ActorPrefix.placedRows;i++){
  const cod1ActorBootstrapRow_t*row=&cod1ActorPrefix.rows[i];const Cod1ActorRecordOwner*r;
  if(row->actorRecord>=cod1ActorPrepassRecords.actors)return 0;r=&cod1ActorRecordOwners[row->actorRecord];
  if(r->state!=COD1_RECORD_BSP_CONSTRUCTED||r->index!=row->actorRecord||r->ordinal!=row->bspOrdinal||
     r->epoch!=cod1ActorPrefix.epoch||r->receipt!=receipt||!r->program||!r->allocation||
     r->allocation!=row->allocationReceipt||!Cod1AiRefEqual(&r->entity,&row->entity)||
     !row->accounted||!row->startupConsumed||row->stage<COD1_ACTOR_ENTRY_LINKED||
     row->stage>COD1_ACTOR_ENTRY_ENTERED||(i&&r->program!=program))return 0;
  program=r->program;
  /* A script delete commits the bootstrap row only after proving native
   * free of the original generation. The constructor record remains part
   * of this map, including across subsequent reuse of its entity slot. */
  if(Cod1ActorDeleteAuthorized(i))continue;
  /* The retained BSP comparison resolves row->entity through PlacedFind.
     * The linked/alive check verifies the complete lease through ActorLeaseValid.
     * These pure reads subsume the prefix's separate live-row scan. */
  if(cod1ActorDeletes[i].state==COD1_ACTOR_DELETE_FAILED||row->retired||!Cod1ActorBspRecordRetained(r,row)||
     !Cod1AiRefEqual(&row->lease.entity,&row->entity)||!G_Cod1ActorLinkedAndAlive(&row->lease))return 0;
 }return 1;
}
static void Cod1RememberCohort(uint64_t receipt){
    Cod1CohortPlan *plan=&cod1CohortPlan;
    unsigned i,first=COD1_ACTOR_RECORD_CAPACITY,last=0,count=cod1ActorPrefix.placedRows;
    Cod1ClearCohortPlan();
    if(!count || count>COD1_ACTOR_CAPACITY)return;
    for(i=0;i<count;i++){
        const cod1ActorBootstrapRow_t *row=cod1ActorPrefix.rows+i;
        unsigned index=row->actorRecord;
        const Cod1ActorPrepassRecord *saved;
        cod1BspLiveRow_t *live=plan->live+i;
        if(row->retired || index>=COD1_ACTOR_RECORD_CAPACITY || !cod1PrepassFieldSealed[index])return;
        if(index<first)first=index;
        if(index>last)last=index;
        saved=cod1ActorPrepassRecords.rows+index;
        live->lease=row->lease;live->bytes=saved->text;live->size=saved->bytes;live->pairs=saved->pairs;
        live->keys=saved->keyOffset;live->values=saved->valueOffset;
    }
    if(last-first>=COD1_ACTOR_CAPACITY || !G_Cod1BspLiveRows(plan->live,count,receipt))return;
    plan->count=count;plan->first=first;plan->span=last-first+1;
    plan->prepassCount=cod1ActorPrepassRecords.actors;plan->epoch=cod1ActorPrefix.epoch;
    memcpy(plan->rows,cod1ActorPrefix.rows,count*sizeof(plan->rows[0]));
    memcpy(plan->owners,cod1ActorRecordOwners+first,plan->span*sizeof(plan->owners[0]));
    plan->receipt=receipt;
}
static int Cod1CohortPlanCurrent(uint64_t receipt){
    const Cod1CohortPlan *plan=&cod1CohortPlan;
    unsigned i;
    if(plan->receipt!=receipt || plan->epoch!=cod1ActorPrefix.epoch ||
       plan->count!=cod1ActorPrefix.placedRows || plan->prepassCount!=cod1ActorPrepassRecords.actors ||
       !R242ExactATRBytesEqual(plan->rows,cod1ActorPrefix.rows,plan->count*sizeof(plan->rows[0])) ||
       !R242ExactATRBytesEqual(plan->owners,cod1ActorRecordOwners+plan->first,plan->span*sizeof(plan->owners[0])))return 0;
    for(i=0;i<plan->count;i++)
        if(cod1ActorDeletes[i].state==COD1_ACTOR_DELETE_FAILED || !cod1PrepassFieldSealed[plan->rows[i].actorRecord])return 0;
    return G_Cod1BspLiveRows(plan->live,plan->count,receipt);
}
static int Cod1ActorBspCohortCurrent(uint64_t receipt){
    if(!Cod1CohortHeader(receipt))return 0;
    if(Cod1CohortPlanCurrent(receipt))return 1;
    if(!Cod1CohortRowsFull(receipt))return 0;
    Cod1RememberCohort(receipt);
    return 1;
}
static int Cod1ActorPrefixAdoptBsp(void*user,const GscActorBootstrapServices*services,char*error,size_t size){
 GscActorBootstrapServices api;cod1AitypeResourceSelection_t selection;uint32_t i,j;uint64_t epoch,receipt,program;int ok=0;
 (void)user;
 if(!services||Cod1ActorPrefixPrivate(error,size)||Cod1AiOverlap(services,sizeof(*services),error,size)||
    !services->adopt_bsp_entity||!services->current||!services->program_identity||
    services->current(services->context)!=1||cod1ActorPrefixBusy||!cod1ActorPrefix.selected||cod1ActorPrefix.runAttempted||
    !Cod1ActorBspCohortCurrent(cod1ActorPrefix.receipt))return Cod1ActorPrefixFail(error,size,"BSP adoption requires its complete retained cohort");
 api=*services;epoch=cod1ActorPrefix.epoch;receipt=cod1ActorPrefix.receipt;
 program=api.program_identity(api.context);if(!program)return 0;
 memset(&selection,0,sizeof(selection));selection.count=cod1ActorPrefixHost.module_count;
 for(i=0;i<selection.count;i++)strcpy(selection.names[i],cod1ActorPrefixHost.module_roots[i]+7);
 cod1ActorPrefixBusy=1;cod1ActorPrefix.runAttempted=1;
 if(!G_Cod1ActorSelectCWWeaponFields(error,size)||!G_Cod1ActorSelectAitypeResourcesViaServices(&api,&selection,error,size))goto done;
 for(i=0;i<cod1ActorPrefix.placedRows;i++){
  const cod1ActorBootstrapRow_t*row=&cod1ActorPrefix.rows[i];const Cod1ActorRecordOwner*record=&cod1ActorRecordOwners[row->actorRecord];
  if(record->program!=program||!Cod1ActorBspCohortCurrent(receipt)||api.current(api.context)!=1)goto done;
  if(!G_Cod1ActorAdoptBspPlaced(&api,&row->entity,error,size))goto done;
 }
 ok=Cod1ActorBspCohortCurrent(receipt)&&api.current(api.context)==1;
 if(ok)cod1ActorPrefix.constructionComplete=1;
done:
 if(epoch==cod1ActorPrefix.epoch&&receipt==cod1ActorPrefix.receipt&&!ok){cod1ActorPrefix.runComplete=0;cod1AiDomain.complete=0;if(error&&size&&error[0])Cod1ActorPrefixFail(NULL,0,error);}
 cod1ActorPrefixBusy=0;return ok;
}
const GscSessionActorBootstrapHost *G_Cod1ActorBspBootstrapHost(void){
 GscSessionActorBootstrapHost host;uint32_t route,i;int found;
 if(!Cod1ActorBspCohortCurrent(cod1ActorPrefix.receipt)||cod1ActorPrefix.runAttempted)return NULL;
 if(cod1ActorPrefix.selected)return cod1ActorPrefixHost.construct==Cod1ActorPrefixAdoptBsp?&cod1ActorPrefixHost:NULL;
 memset(&host,0,sizeof(host));
 for(route=0;route<cod1AiDomain.constructorRows;route++){found=0;
  for(i=0;i<cod1ActorPrefix.placedRows;i++)if(cod1ActorPrefix.rows[i].constructorOrdinal==route){
   GscEntityRef*ref=&cod1ActorPrefix.rows[i].entity;if(!Cod1ActorPrefixAddModule(&host,ref))return NULL;
   host.placed[host.placed_count++]=*ref;found=1;break;}
  for(i=0;i<cod1AiDomain.spawnerRows&&!found;i++)if(cod1AiDomainSpawners[i].ordinal==route){if(!Cod1ActorPrefixAddModule(&host,&cod1AiDomainSpawners[i].entity))return NULL;found=1;}
  if(!found)return NULL;
 }
 if(host.module_count!=cod1ActorPrefix.prepassModuleCount||memcmp(host.module_roots,cod1ActorPrefixHost.module_roots,sizeof(host.module_roots)))return NULL;
 host.map_certificate.receipt=cod1ActorPrefix.receipt;host.map_certificate.current=Cod1ActorBspCohortCurrent;
 host.access=*G_Cod1ActorConstructorAccessHost();host.fields=*G_Cod1ActorTrainingFieldHost();host.methods=*G_Cod1ActorConstructorMethodHost();
 host.spawner_team=*G_Cod1SpawnerTeamMethodHost();host.attachments=*G_Cod1ActorAttachmentQueryHost();host.run=Cod1ActorPrefixRun;host.construct=Cod1ActorPrefixAdoptBsp;
 if(G_Cod1GscFeature47(C1CP47_F_GRAPH_STARTUP)==1||G_Cod1GscFeature47(C1CP47_F_GRAPH_STARTUP)==2){host.node_target=Cod1ActorPrefixNodeTarget;host.node_target_end=Cod1ActorPrefixNodeTargetEnd;}
  host.placed_init_initial=host.node_target&&G_Cod1GscFeature47(C1CP47_F_PLACED_INIT)==1;
  host.placed_init_return=host.placed_init_initial&&G_Cod1GscFeature47(C1CP47_F_PLACED_STATE)==1?G_Cod1ActorPlacedInitReturn:NULL;
  host.placed_cohort=host.placed_init_return&&G_Cod1GscFeature47(C1CP47_F_PLACED_COHORT)==1;
 cod1ActorPrefix.resourceCount=host.module_count;cod1ActorPrefixHost=host;cod1ActorPrefix.selected=1;return &cod1ActorPrefixHost;
}
const GscSessionActorBootstrapHost *G_Cod1ActorBootstrapHost(void){
    GscSessionActorBootstrapHost host;cod1PlacedActorSnapshot_t placed;uint32_t route,i;int found;
    if(!Cod1ActorPrefixCurrent(cod1ActorPrefix.receipt)||!cod1ActorPrefix.prepassReady||cod1ActorPrefix.runAttempted||!cod1ActorPrefix.placedRows||
       cod1AiDomain.invalid||!cod1AiDomain.sealed||cod1AiDomain.unaccountedRows!=cod1ActorPrefix.placedRows)return NULL;
    if(cod1ActorPrefix.selected)return &cod1ActorPrefixHost;
    memset(&host,0,sizeof(host));
    for(route=0;route<cod1AiDomain.constructorRows;++route){found=0;
        for(i=0;i<cod1ActorPrefix.placedRows;++i)if(cod1ActorPrefix.rows[i].constructorOrdinal==route){
            GscEntityRef *ref=&cod1ActorPrefix.rows[i].entity;
            if(!G_Cod1PlacedActorRead(ref,&placed,NULL,0)||!placed.privateFieldsAvailable||!placed.nativeStaticAvailable||
               placed.startupConsumed||!Cod1ActorPrefixAddModule(&host,ref))return NULL;
            host.placed[host.placed_count++]=*ref;found=1;break;}
        for(i=0;i<cod1AiDomain.spawnerRows&&!found;++i)if(cod1AiDomainSpawners[i].ordinal==route){
            if(!Cod1ActorPrefixAddModule(&host,&cod1AiDomainSpawners[i].entity))return NULL;found=1;}
        if(!found)return NULL;
    }
    if(host.module_count!=cod1ActorPrefix.prepassModuleCount||
       memcmp(host.module_roots,cod1ActorPrefixHost.module_roots,sizeof(host.module_roots))||
       cod1ActorPrefix.prepassRows!=cod1AiDomain.constructorRows)return NULL;
    host.map_certificate.receipt=cod1ActorPrefix.receipt;host.map_certificate.current=Cod1ActorPrefixCurrent;
    host.access=*G_Cod1ActorConstructorAccessHost();host.fields=*G_Cod1ActorTrainingFieldHost();
    host.methods=*G_Cod1ActorConstructorMethodHost();host.spawner_team=*G_Cod1SpawnerTeamMethodHost();
    host.attachments=*G_Cod1ActorAttachmentQueryHost();host.run=Cod1ActorPrefixRun;host.construct=Cod1ActorPrefixConstruct;
    if(G_Cod1GscFeature47(C1CP47_F_GRAPH_STARTUP)==1||G_Cod1GscFeature47(C1CP47_F_GRAPH_STARTUP)==2){host.node_target=Cod1ActorPrefixNodeTarget;host.node_target_end=Cod1ActorPrefixNodeTargetEnd;}
  host.placed_init_initial=host.node_target&&G_Cod1GscFeature47(C1CP47_F_PLACED_INIT)==1;
  host.placed_init_return=host.placed_init_initial&&G_Cod1GscFeature47(C1CP47_F_PLACED_STATE)==1?G_Cod1ActorPlacedInitReturn:NULL;
  host.placed_cohort=host.placed_init_return&&G_Cod1GscFeature47(C1CP47_F_PLACED_COHORT)==1;
    cod1ActorPrefix.resourceCount=host.module_count;cod1ActorPrefixHost=host;cod1ActorPrefix.selected=1;return &cod1ActorPrefixHost;
}
int G_Cod1ActorBootstrapRead(cod1ActorBootstrapSnapshot_t *out,char *error,size_t size){
    if(!out||Cod1AiOverlap(out,sizeof(*out),error,size)||
       Cod1ActorPrefixPrivate(out,sizeof(*out))||Cod1ActorPrefixPrivate(error,size))return 0;
    if(!Cod1ActorPrefixOwner())return Cod1AiError(error,size,"actor prefix has no current map catalog");
    *out=cod1ActorPrefix;return 1;
}
int G_Cod1ActorBootstrapRetire(uint64_t epoch,char *error,size_t size){
    uint32_t i;cod1ActorReservationView_t actors[COD1_ACTOR_CAPACITY];size_t count,j;uint64_t receipt;int released;
    if(Cod1ActorPrefixPrivate(error,size))return 0;
    if(!epoch||epoch!=cod1ActorPrefix.epoch||epoch!=G_Cod1EntityMapEpoch())return 1;
    receipt=cod1ActorPrefix.receipt;
    cod1ActorPrefix.closed=1;cod1AiDomain.complete=0;cod1AiDomain.invalid=1;
    Cod1ActorRecordReset();
    if(cod1ActorPrefixBusy)return Cod1AiError(error,size,"actor prefix cleanup deferred while its constructor callback is running");
    for(i=0;i<cod1ActorPrefix.placedRows;++i){cod1ActorBootstrapRow_t *row=&cod1ActorPrefix.rows[i];
        if(row->retired)continue;
        if(!G_Cod1EntitySameAllocation(&row->entity)){row->retired=1;continue;}
        if(!G_Cod1ActorCollectReservations(actors,COD1_ACTOR_CAPACITY,&count,error,size))return 0;
        for(j=0;j<count;++j)if(Cod1AiRefEqual(&actors[j].lease.entity,&row->entity))break;
        if(j<count){
            if(!row->lease.serial||actors[j].lease.serial!=row->lease.serial)
                return Cod1AiError(error,size,"actor prefix cleanup refuses replacement component on original allocation");
            released=G_Cod1ActorRelease(&row->lease,error,size);
            if(epoch!=G_Cod1EntityMapEpoch()||epoch!=cod1ActorPrefix.epoch||receipt!=cod1ActorPrefix.receipt)return 1;
            row=&cod1ActorPrefix.rows[i];
            if(!G_Cod1EntitySameAllocation(&row->entity)){row->retired=1;continue;}
            if(!released)return 0;
        }
        if(!G_Cod1EntitySameAllocation(&row->entity)){row->retired=1;continue;}
        if(!G_Cod1ActorCollectReservations(actors,COD1_ACTOR_CAPACITY,&count,error,size))return 0;
        for(j=0;j<count;++j)if(Cod1AiRefEqual(&actors[j].lease.entity,&row->entity))
            return Cod1AiError(error,size,"actor prefix cleanup refuses a component installed during retirement");
        if(g_entities[row->entity.slot].neverFree||g_entities[row->entity.slot].client||
           (g_entities[row->entity.slot].r.svFlags&(SVF_CASTAI|SVF_BOT)))
            return Cod1AiError(error,size,"actor prefix cleanup refuses protected or foreign original entity");
        G_FreeEntity(&g_entities[row->entity.slot]);
        if(epoch!=G_Cod1EntityMapEpoch()||epoch!=cod1ActorPrefix.epoch||receipt!=cod1ActorPrefix.receipt)return 1;
        row=&cod1ActorPrefix.rows[i];
        if(G_Cod1EntitySameAllocation(&row->entity))return Cod1AiError(error,size,"actor prefix entity cleanup retained exact ownership for retry");
        row->retired=1;
    }
    return 1;
}
#include "g_cod1_actor_delete_receipts.inc"
#endif
