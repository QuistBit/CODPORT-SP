#ifndef G_COD1_ACTOR_BOOTSTRAP_H
#define G_COD1_ACTOR_BOOTSTRAP_H
#if defined(COD1_GSC_TEST_WIP) && COD1_GSC_TEST_WIP
#include "g_cod1_actor.h"
#include "cod1_gsc/gsc_session.h"
typedef struct {
    GscEntityRef entity;
    cod1ActorLease_t lease;
    uint32_t constructorOrdinal;
    /* Exact authored record -> actual allocation -> constructor consumption.
     * These DATA identities never authorize VM execution. */
    uint32_t bspOrdinal,actorRecord;
    uint64_t allocationReceipt,programIdentity;
    int stage,startupConsumed,entryStatus,accounted,retired;
    int placedInitAttempted,placedInitAccepted;GscPlacedInitRun placedInit;
} cod1ActorBootstrapRow_t;
typedef struct {
    uint64_t epoch,receipt;
    uint32_t placedRows,resourceCount,hookCount,accountedRows,returnedRows;
    int enabled,declarationsReady,sealed,selected,runAttempted,runComplete,closed,invalid;
    char error[256];
    cod1ActorBootstrapRow_t rows[COD1_ACTOR_CAPACITY];
    int constructionComplete,entryAttempted;
    int prepassReady;uint32_t prepassRows,prepassModuleCount;
} cod1ActorBootstrapSnapshot_t;
/* Internal synchronous per-row handoff, unavailable before entry return or
 * after the loop advances. It does not widen ordinary Actor method leases. */
int G_Cod1ActorBootstrapConstructedCurrent(const GscEntityRef *);
int G_Cod1ActorBootstrapEntryCurrent(const cod1ActorLease_t *);
int G_Cod1ActorBootstrapPostinitCurrent(const cod1ActorLease_t *);
int G_Cod1ActorPostinitPosition(const cod1ActorLease_t *);
int G_Cod1ActorPlacedPostinitTarget(const GscActorBootstrapServices *,const cod1ActorLease_t *,char *,size_t);
int G_Cod1ActorPlacedCurrentResourcesLoad(void);
int G_Cod1ActorPlacedInitReturn(void *,const GscPlacedInitReturnRequest *,char *,size_t,int(*)(void*),void*);
int G_Cod1ActorPlacedInitInitial(const GscActorBootstrapServices *,const cod1ActorLease_t *,GscPlacedInitRun *,char *,size_t);
int G_Cod1ActorPlacedInitCohortCurrent(const cod1ActorLease_t *,const GscPlacedInitRun *);
int G_Cod1ActorBootstrapRead(cod1ActorBootstrapSnapshot_t *,char *,size_t);
const GscSessionActorBootstrapHost *G_Cod1ActorBootstrapHost(void);
const GscSessionActorBootstrapHost *G_Cod1ActorBspBootstrapHost(void);
int G_Cod1ActorAdoptBspPlaced(const GscActorBootstrapServices *,const GscEntityRef *,char *,size_t);
const GscSessionActorProgramPlan *G_Cod1ActorProgramPlan(void);
/* Map-owned declarations only. No entity references, VM handles or execution
 * certificate. Duplicate BSP requests remain in the separate replay records. */
#define COD1_EARLY_TURRET_CAPACITY 32u
typedef struct {char weapon[64],module[128];} cod1EarlyTurretDeclaration_t;
typedef struct {
 uint64_t epoch,receipt;GscNodeScriptNames negotiation;
 uint32_t negotiationRequests,turretRequests,turretCount;
 cod1EarlyTurretDeclaration_t turrets[COD1_EARLY_TURRET_CAPACITY];
} cod1EarlyProgramDeclarations_t;
int G_Cod1EarlyTurretProgramPlan(GscSessionTurretProgramPlan *);
int G_Cod1EarlyNodeProgramPlan(GscSessionNodeProgramPlan *);
int G_Cod1EarlyProgramRead(cod1EarlyProgramDeclarations_t *,char *,size_t);
int G_Cod1EarlyTurretModule(const char *,char *,size_t);
typedef struct {
 GscEntityRef entity;uint64_t epoch,receipt,allocation,program;uint32_t ordinal;
 void *user;int (*current)(void *,const GscEntityRef *);
} cod1ActorBspConstructionOwner_t;
int G_Cod1ActorConstructBspPlaced(GscSession *,uint64_t,const cod1ActorBspConstructionOwner_t *,cod1ActorPlacedEntryResult_t *,char *,size_t);
int G_Cod1ActorBootstrapRetire(uint64_t epoch,char *,size_t);
#endif
#endif
