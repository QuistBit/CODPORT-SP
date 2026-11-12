#ifndef G_COD1_PATH_ENTITY_HOST_H
#define G_COD1_PATH_ENTITY_HOST_H
#if defined(COD1_GSC_TEST_WIP) && COD1_GSC_TEST_WIP
#include "cod1_gsc/gsc_fixture.h"
/* Native entity.flags134 path bits, NOT r.svFlags or native server flags.
 * Read only succeeds for an allocation registered by its real spawn path.
 * Unknown is not interpreted as zero. No live graph capability is granted. */
/* Completed original BSP point constructor; deferred placement is separate.
 * Readiness here proves only that normal spatial queries see native contents.
 * It never zeroes DYNAMICPATH objects as the graph-building bracket does. */
int G_Cod1PathSpawnPointBorn(gentity_t *,const GscEntityRef *);
void G_Cod1SpawnPointThink(gentity_t *);
int G_Cod1SpawnPointCurrent(const GscEntityRef *);
int G_Cod1PathSpawnWorldReady(void);
/* Actual static-health BSP constructor/finish phases, filter-only ownership. */
int G_Cod1PathHealthStart(gentity_t*,gitem_t*,GscEntityRef*);
void G_Cod1PathHealthPending(gentity_t*,const GscEntityRef*);
int G_Cod1PathHealthFinishStart(gentity_t*,GscEntityRef*);
void G_Cod1PathHealthFinished(gentity_t*,const GscEntityRef*);
/* Exact unused RTCW adapter reserve; supplies NO native entity flags. */
int G_Cod1BodyReserveCurrent(const GscEntityRef*);
int G_Cod1ScriptHelperCurrent(const GscEntityRef*);
/* Actual initial idle turret only; pure read, consumed at control handoff. */
int G_Cod1TurretInitialMaskCurrent(const GscEntityRef*);
/* Fixed0281C011 only: mask-excluded native model/trigger representations
 * may coexist without a geometry grant or physical contents mutation. */
int G_Cod1PathSpawnTrace(const cod1SpatialTraceQuery_t *,trace_t *);
/* Native6D9F0 trace33/00820011, two exact hulls, no dynamic1000 owners.
 * Query result only; does not authorize graph publication or native main. */
int G_Cod1PathNodePlaceTrace(const cod1SpatialTraceQuery_t *,trace_t *);
/* Exact no1000 cohort snapshot for one synchronous preparation. End releases
 * only the matching receipt. Current failure permanently retires it. */
int G_Cod1PathNodeWorldBegin(uint64_t *);
int G_Cod1PathNodeWorldCurrent(uint64_t);
void G_Cod1PathNodeWorldEnd(uint64_t);
/* Exact current no1000 node-world intake. Adapter-only reservations/helpers
 * are explicitly excluded from native active owners, never flags-zero owners.
 * This copied table is DATA; only the still-current world receipt authorizes
 * its use during the synchronous stage. No geometry or native VM permit. */
typedef struct {GscEntityRef ref;uint32_t flags;unsigned active,adapter;} cod1PathNodeOwner_t;
typedef struct {unsigned count;int32_t time;cod1PathNodeOwner_t rows[MAX_GENTITIES];} cod1PathNodeOwners_t;
int G_Cod1PathNodeOwners(uint64_t,cod1PathNodeOwners_t *);
/* Original6FAD0 active/flags snapshot only, zero candidate transitions.
 * No spatial/contents grant. Unknown player or both3000 refuses. */
int G_Cod1LateDisconnectPrivate(const void*,size_t);
int G_Cod1LateDisconnectInputsPrivate(const void*,size_t);
int G_Cod1LateDisconnectGraphPrivate(const void*,size_t);
int G_Cod1LateDisconnectBegin(uint64_t*);
int G_Cod1LateDisconnectCurrent(uint64_t);
int G_Cod1LateDisconnectHeld(uint64_t);
void G_Cod1LateDisconnectEnd(uint64_t);
int G_Cod1LateDisconnectOwners(uint64_t,cod1PathNodeOwners_t*);
int G_Cod1PathEntityRead(const GscEntityRef *,uint32_t *);
/* Completion of the admitted BSP TOUCH constructor only; requires the
 * allocation/dispatch receipt held privately by g_spawn.c. */
void G_Cod1PathTriggerMultipleBorn(gentity_t *,const GscEntityRef *);
/* Actual completed BSP constructor only: 3=USE,4=lookat,5=damage.
 * Native contents are projected only inside PathWorld Begin/End; the
 * existing interaction representation remains unchanged outside it. */
void G_Cod1PathTriggerProjectedBorn(gentity_t *,const GscEntityRef *,int);
/* Scoped native6DBB0 contents dependency, not graph publication. Begin
 * validates the complete current inuse cohort before any contents change.
 * 1=active, 0=refused unchanged, -1=interrupted after starting. For 1/-1 a
 * nonzero receipt is returned and MUST be ended even if graph work fails.
 * End restores every surviving exact owner, including unflagged owners.
 * End0 retains pending cleanup for retry; End1 consumes the receipt. The
 * caller must independently reject a graph across any map/cohort change.
 * Unknown native flag producers refuse; player/item coverage is not implied. */
int G_Cod1PathWorldBegin(uint64_t *receipt);
/* Consume a typed initial weapon receipt before selected lifecycle adoption.
 * No external calls; failed adoption cannot revive the initial snapshot. */
void G_Cod1PathHealthHandoff(const GscEntityRef *);
int G_Cod1PathHealthPreparedItem(const GscEntityRef *);
/* Current GAME cohort and captured contents/flags only, not a graph or
 * SERVER geometry certificate. False after an interrupted begin/end. */
int G_Cod1PathWorldCurrent(uint64_t receipt);
int G_Cod1TowerDamageBrushCurrent(const GscEntityRef *);
int G_Cod1PathWorldEnd(uint64_t receipt);
/* Target-only native capsule sight, output0/-1 only on success. Unsupported
 * geometry/refused ownership leaves output unchanged. Not a graph permit. */
int G_Cod1PathSight(const GscEntityRef*,const float*,const float*,int*);
int G_Cod1PathCandidateTrace(void*,unsigned,unsigned,const float*,const float*,const float*,const float*,unsigned,unsigned);
#endif
#endif
