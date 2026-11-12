#ifndef TR_COD1_TURRET_ACTOR_H
#define TR_COD1_TURRET_ACTOR_H
#include "cod1_turret_actor_state.h"
#include "cod1_actor_assembly14.h"
#include "../game/bg_cod1_turret_pose.h"
#include "../qcommon/cod1_native_start_origin.h"
#define COD1_TURRET_ACTOR_SURFACES (C1AA14_PARTS*XM14_SURFS)
struct cod1TurretActorModel_s {
 cod1TurretActorResourceState_t definition;
 c1aa14Resource_t *resource;
 shader_t *shader[C1AA14_PARTS][XM14_SURFS];
 float bounds[2][3]; /* registration bind pose only; scene uses capture bounds */
 struct cod1TurretActorModel_s *next;
 int handle;
};
struct cod1TurretActorSubmission_s {
 cod1TurretActorRenderState_t state;
 c1aa14Frame_t *capture;
 unsigned surfaces;
 srfTriangles_t *geometry;
 shader_t **shader;
};
#endif
