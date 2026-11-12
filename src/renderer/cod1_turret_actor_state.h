#ifndef COD1_TURRET_ACTOR_STATE_H
#define COD1_TURRET_ACTOR_STATE_H
/* Public fixed-size boundary. Bone transforms are normalized current BODY
 * modelspace values, not animation goals or renderer-time inferred samples. */
#define COD1_TURRET_ACTOR_RESOURCE_VERSION 1
#define COD1_TURRET_ACTOR_RENDER_VERSION 1
#define COD1_TURRET_ACTOR_RENDER_GENERIC_VERSION 2
#define COD1_TURRET_ACTOR_PARTS 8
#define COD1_TURRET_ACTOR_BONES 128
typedef struct {
 int version,size,partCount;
 char model[COD1_TURRET_ACTOR_PARTS][64];
 char tag[COD1_TURRET_ACTOR_PARTS][64];
} cod1TurretActorResourceState_t;
typedef struct {
 int version,size,hModel,ownerNum,epochLo,epochHi,generation,sampleTick,boneCount;
 float origin[3],baseAngles[3];
 float q[COD1_TURRET_ACTOR_BONES][4];
 float p[COD1_TURRET_ACTOR_BONES][3];
} cod1TurretActorRenderState_t;
#define COD1_TURRET_ACTOR_RESOURCE_BYTES 1036
#define COD1_TURRET_ACTOR_RENDER_BYTES 3644
typedef char cod1TurretActorResourceSizeCheck[(sizeof(cod1TurretActorResourceState_t)==COD1_TURRET_ACTOR_RESOURCE_BYTES)?1:-1];
typedef char cod1TurretActorRenderSizeCheck[(sizeof(cod1TurretActorRenderState_t)==COD1_TURRET_ACTOR_RENDER_BYTES)?1:-1];
#endif
