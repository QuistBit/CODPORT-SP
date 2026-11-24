#ifndef TR_COD1_TURRET_ACTOR_H
#define TR_COD1_TURRET_ACTOR_H
#include "cod1_turret_actor_state.h"
#include "cod1_actor_sealed14.h"
#include "cod1_actor_bounds575.h"
#include "../game/bg_cod1_turret_pose.h"
#include "../qcommon/cod1_native_start_origin.h"
#define COD1_TURRET_ACTOR_SURFACES (C1AA14_PARTS*XM14_SURFS)
extern cnsArena_t cod1ActorSkinScratch;
struct cod1TurretActorModel_s {
 cod1TurretActorResourceState_t definition;
 c1aa14Resource_t *resource;
 c1aa14Seal_t sealed;
 shader_t *shader[C1AA14_PARTS][XM14_SURFS];
 float bounds[2][3]; /* registration bind pose only; scene uses capture bounds */
 struct cod1TurretActorModel_s *next;
 int handle;
 unsigned pendingFrames;int retired;
 c1aaBounds575_t poseBounds;
};
struct cod1TurretActorSubmission_s {
 cod1TurretActorRenderState_t state;
 cod1TurretActorModel_t *pendingModel;
 struct cod1TurretActorSubmission_s *nextPending;
 float bounds[2][3];
 unsigned surfaces;
 srfTriangles_t *geometry;
 shader_t **shader;
};
int R_CaptureCod1TurretActor(const cod1TurretActorRenderState_t*,trRefEntity_t*);
void R_ClearActorCapturePins(void);
void R_ReleaseActorCapture(cod1TurretActorModel_t*);
#endif
