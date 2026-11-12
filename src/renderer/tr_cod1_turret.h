#ifndef TR_COD1_TURRET_H
#define TR_COD1_TURRET_H
#include "cod1_turret_animated_mesh.h"
/* Registration-owned decoded source; no current GAME identity is inferred. */
struct cod1TurretModel_s {
 c1tmResource_t *resource;
 c1aw14Skeleton_t *animatedSkeleton;
 c1ab14Clip_t *animatedClip[2];
 shader_t *shader[XM14_SURFS];
 vec3_t bounds[2];
 struct cod1TurretModel_s *next;
};
/* Everything consumed by the backend is copied into the current SMP arena.
 * No resource/input pointers are retained in surface geometry. */
struct cod1TurretSubmission_s {
 cod1TurretRenderState_t state;
 c1tmFrame_t *capture;
 c1tpWorldFlash_t flash;
 int surfaces;
 srfTriangles_t geometry[XM14_SURFS];
 shader_t *shader[XM14_SURFS];
};
#endif
