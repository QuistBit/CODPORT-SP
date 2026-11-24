/* Immutable authored CoD path-node catalog, separate from GAME entities. */
#ifndef G_COD1_NODE_HOST_H
#define G_COD1_NODE_HOST_H
#if defined(COD1_GSC_TEST_WIP) && COD1_GSC_TEST_WIP
#include "cod1_gsc/gsc_fixture.h"
typedef struct {float position[3],yaw;int status;} cod1PlacedGoal_t;
void G_Cod1PrepareTrainingGoals(void);
int G_Cod1LocalWaypoints(const GscEntityRef *owner,uint64_t assignment,const GscNodeRef *goal,const float start[3],const float end[3],
                         float points[][3],int capacity);
/* 1 placed and current, 0 outside selected scope, -1 stale or unavailable. */
cod1PlacedGoal_t G_Cod1TrainingPlacedGoal(const GscEntityRef *,uint64_t,const GscNodeRef *);
static inline int G_Cod1ElderStandingRun(uint64_t assignment){
    return assignment==18 || assignment==20 || assignment==22;
}
/* The bounded Training course has a fixed verified sequence of movement
 * handlers. Node identities and links come from the current BSP catalog. */
#define COD1_TRAINING_ROUTE_LENGTH 22
typedef struct {
    uint64_t epoch;
    unsigned index[COD1_TRAINING_ROUTE_LENGTH];
    unsigned char kind[COD1_TRAINING_ROUTE_LENGTH];
} cod1TrainingRoute_t;
int G_Cod1BindTrainingRoute(const char *first, cod1TrainingRoute_t *route);
unsigned G_Cod1RunnerStep(const GscEntityRef *,const GscNodeRef *);
int G_Cod1RunnerCourseGoal(const GscEntityRef *,uint64_t,const GscNodeRef *);
int G_Cod1RunnerSharesNode(const GscEntityRef *actor,unsigned assignment);
int TrainingElderCurrent(const GscEntityRef *);
int G_Cod1StandingNode(const float *,const GscNodeRef *,GscNodeRef *);
int G_Cod1WaitingRunnerContact(const GscEntityRef *actor,const float at[3]);
const char *G_Cod1RunnerName(const GscEntityRef *);
int G_Cod1MoodyCourseGoal(uint64_t,const GscNodeRef *);
int G_Cod1FoleyCourseGoal(uint64_t assignment, const GscNodeRef *node);
void G_Cod1NodeRegistryClear(void);
/* ENGINE-owned CM retirement, no GSC notification or ready permission. */
void G_Cod1NodeResourceRetire(uint64_t);
const GscNodeQueryHost *G_Cod1NodeQueryHost(void);
/* Explicit original cleared-node default; authored radius remains refused. */
const GscNodeQueryHost *G_Cod1NodeRadiusQueryHost(void);
/* Copy actual authored Begin module names plus an independent registry/map
 * certificate. This is dependency acquisition, not finalized graph authority. */
int G_Cod1NodeScriptDependencies(GscNodeScriptNames *,GscMapExecutionCertificate *);
#include "g_cod1_node_prepare.h"
#include "cod1_gsc/gsc_session.h"
int G_Cod1NodeScriptAdmission(GscSessionNodeScriptAdmission *);
C1NPPrepared *G_Cod1NodePreparedBuild(GscSession *,uint64_t);
int G_Cod1NodePreparedCurrent(C1NPPrepared *,GscSession *,uint64_t);
void G_Cod1NodePreparedFree(C1NPPrepared *);
/* Private staged graph only. Caller lump bytes are copied DATA, not proof of
 * the currently loaded map resource. No VM/pathfinding publication is made.
 * Session pointer is never retained; Current requires a live caller Session.
 * Map registry clear discards or cancels the stage; stale handles are inert. */
/* DATA-only acquisition retained for bounded component work. */
int G_Cod1NodeGraphStage(GscSession *,uint64_t,const void *,size_t,uint64_t *);
/* Actual current engine resource, still an unpublished stage. */
int G_Cod1NodeGraphStageMap(GscSession *,uint64_t,uint64_t *);
int G_Cod1NodeGraphStageCurrent(GscSession *,uint64_t,uint64_t);
/* One-shot transfer of placed/sorted graph DATA to current map/registry/
 * program lifetime; no pathfinding/VM mutation grant. Stage APIs stay scoped. */
int G_Cod1NodeGraphDisconnectPaths(GscSession *,uint64_t,const GscEntityRef *,int,char *,size_t,int (*)(void *),void *);
int G_Cod1NodeGraphConnectPaths(GscSession *,uint64_t,const GscEntityRef *,char *,size_t,int (*)(void *),void *);
int G_Cod1NodeGraphPublish(GscSession *,uint64_t,uint64_t);
int G_Cod1NodeGraphPublishedCurrent(GscSession *,uint64_t,uint64_t);
int G_Cod1NodeGraphPublishedInitialOwners(GscSession *,uint64_t,uint64_t);
int G_Cod1NodeGraphPostCohortInitialOwners(GscSession *,uint64_t,uint64_t,const void*);
int G_Cod1NodeGraphPostCohortCurrent(GscSession *,uint64_t,uint64_t,const void*);
int G_Cod1NodeGraphPostCohortEnd(GscSession *,uint64_t,uint64_t,const void*,int);
int G_Cod1NodeGraphPostCohortPrivate(const void*,size_t);
/* Copied identity of one current, active Begin->End graph edge. This is
 * graph data only; it neither creates a path nor queues an AI state. */
typedef struct {
 uint64_t receipt,sessionId;GscNodeRef begin,end;GscNodeScriptRef script;
 uint32_t beginPhysical,endPhysical;float start[3],finish[3],yaw;
} cod1NegotiationPair_t;
int G_Cod1NodeGraphNegotiationPair(GscSession*,uint64_t,const GscNodeRef*,const GscNodeRef*,cod1NegotiationPair_t*);
/* Pure allocation/graph check inside an enclosing live map/resource lease. */
int G_Cod1NodeGraphNegotiationCurrent(const cod1NegotiationPair_t*);
typedef struct {uint64_t receipt;uint32_t nodes,links,beginRefs,owners;} cod1NodeGraphPublication_t;
cod1NodeGraphPublication_t G_Cod1NodeGraphPublishedInfo(GscSession *,uint64_t,uint64_t);


/* Copy the node-only target lookup from the placed/sorted private graph.
 * status: 0 refused/stale, 1 current but absent, 2 first physical match.
 * Returned by value; no arena pointer or native graph authority escapes.
 * This does not perform 736A0's earlier entity/numeric target branches,
 * apply 73910 actor goal stores, or certify postinit/late-owner scheduling. */
typedef GscActorBootstrapNodeTarget cod1NodeStageTarget_t;
cod1NodeStageTarget_t G_Cod1NodeGraphPublishedTarget(GscSession *,uint64_t,uint64_t,const char *);
cod1NodeStageTarget_t G_Cod1NodeGraphStageTarget(GscSession *,uint64_t,uint64_t,const char *);
/* Paired private GAME transaction; successful begin holds graph through all
 * Session postchecks. End is pure and consumes this exact operation. */
int G_Cod1NodeGraphActorTarget(GscSession *,uint64_t,uint64_t,const GscEntityRef *,const char *,GscActorBootstrapNodeTarget *);
int G_Cod1NodeGraphActorTargetEnd(GscSession *,uint64_t,uint64_t,const GscEntityRef *,int);
void G_Cod1NodeGraphStageDiscard(uint64_t);
void G_Cod1NodeGraphStageClear(void);
#endif
#endif

#if defined(COD1_GSC_TEST_WIP) && COD1_GSC_TEST_WIP
/* Separate later initial-owner phase for an unpublished stage. Caller must
 * prove original scheduling; failure discards the whole stage. Duplicate or
 * wrong-owner calls refuse without granting a native/VM graph capability. */
int G_Cod1NodeGraphStageInitialOwners(GscSession *,uint64_t,uint64_t);
#endif
