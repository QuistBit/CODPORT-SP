/* R101: bounded BSP59 actor-spawner intake, not an actor constructor.
 * The registry is map-scoped and is not serialized by SAVE24. It supplies no
 * GSC capability, native actor, sentient, spawned event or wire entity type. */
#ifndef G_COD1_SPAWNER_H
#define G_COD1_SPAWNER_H

#if defined(COD1_GSC_TEST_WIP) && COD1_GSC_TEST_WIP
#include "cod1_gsc/gsc_fixture.h"
struct gentity_s;

enum {
    COD1_SPAWNER_TEAM_AXIS = 1,
    COD1_SPAWNER_TEAM_ALLIES = 2,
    COD1_SPAWNER_TEAM_NEUTRAL = 3,
    COD1_SPAWNER_MASK_AXIS = 2,
    COD1_SPAWNER_MASK_ALLIES = 4,
    COD1_SPAWNER_MASK_NEUTRAL = 8
};

typedef struct {
    GscEntityRef entity;
    float origin[3], angles[3];
    int count, team, lastSpawnTime;
    int rawFieldCount, rawBytes;
} cod1SpawnerSnapshot_t;

/* Called with the BSP bootstrap. Old references fail both the map epoch and
 * the registry reset; no earlier map/raw-save record can regain membership. */
void G_Cod1SpawnerMapBegin( void );

/* Only the native actor_ spawnflags&1 BSP branch calls intake. It copies the
 * complete ordered spawn variable list, including duplicates, before returning.
 * No borrowed pointer is retained, and no entity is linked or actor allocated.
 * R175 also attempts initial native-static extras publication once per fresh
 * allocation, using the proved spawner model0/defaults and a narrow parser.
 * Unsupported extra text/numeric forms retain membership but grant no static
 * source authority. Reservation/model history prevents stale republication.
 * The native spawner key reset is applied after the ordinary key parser. */
int G_Cod1SpawnerIntake( struct gentity_s *ent );

/* Pure checked reads. Membership requires the original complete allocation
 * identity; a classname, health, marker or reused slot cannot establish it.
 * Snapshot count is the canonical intake count, not a running AI spawn count.
 * Typed team mutation and the separate explicitly selected R210 origin-only
 * prototype provider may update current snapshot cells. Ordered raw BSP fields
 * and the initial extras authority remain intake records.
 * Ref/out alias is safe. Failure leaves output unchanged. */
int G_Cod1SpawnerCapture( const struct gentity_s *ent, GscEntityRef *out );
int G_Cod1SpawnerRead( const GscEntityRef *entity,
    cod1SpawnerSnapshot_t *out, char *error, size_t errorSize );
int G_Cod1SpawnerRawField( const GscEntityRef *entity, size_t index,
    char *key, size_t keyCapacity, char *value, size_t valueCapacity,
    char *error, size_t errorSize );

/* Complete copied ascending collections, no truncation or callbacks. All
 * includes initial team0 and count0. Teams requires a nonzero subset of 2/4/8;
 * native getspawnerteamarray does not accept mask0 as 'all'. Failure leaves
 * outputs unchanged. Output/count/error byte ranges must not overlap. */
int G_Cod1SpawnerCollectAll( GscEntityRef *out, size_t capacity,
    size_t *count, char *error, size_t errorSize );
int G_Cod1SpawnerCollectTeams( unsigned int teamMask,
    GscEntityRef *out, size_t capacity, size_t *count,
    char *error, size_t errorSize );

/* Native setspawnerteam's admitted named values, case-insensitive ASCII:
 * axis/allies/neutral -> 1/2/3. Initial team0 has no named setter. This is a
 * checked internal primitive, not a registered or automatically enabled VM API. */
int G_Cod1SpawnerSetTeam( const GscEntityRef *entity, const char *team,
    char *error, size_t errorSize );
/* Nonreviving typed-intake identity, independent of mutable team and entity
 * allocation generation. Zero means unavailable/exhausted; old intake remains
 * usable. Reset/reintake cannot revive an earlier selected hook certificate. */
uint64_t G_Cod1SpawnerIntakeIdentity(const GscEntityRef *);
/* Selected script composition updates the proved shared native key/team cell
 * as well as the typed registry. Old SetTeam remains snapshot-only. */
int G_Cod1SpawnerSetScriptTeam(const GscEntityRef *,const char *,char *,size_t);
/* Retail entity field14 (count +1f8) is a live signed integer. These selected
 * ready-map accesses leave the intake snapshot/raw keys unchanged. They do
 * not spawn, decrement, publish an event or advance lastSpawnTime. Numeric
 * values use the already selected GetInt domain; strings are not admitted. */
/* Private factory commit requires the actual active GAME finishing owner;
 * callers cannot supply a successful callback or choose a stored clock. */
int G_Cod1SpawnerCommitFactory(const GscActorSpawnTicket *,uint64_t);
int G_Cod1SpawnerCountRead(const GscEntityRef *,GscStringCastValue *,char *,size_t);
int G_Cod1SpawnerCountWrite(const GscEntityRef *,const GscStringCastValue *,char *,size_t);
#else
#define G_Cod1SpawnerMapBegin() ((void)0)
#endif
#endif
