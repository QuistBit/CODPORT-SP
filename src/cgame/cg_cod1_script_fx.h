/* R115 private cgame script FX interfaces. No public cg_t/entity layout. */
#ifndef CG_COD1_SCRIPT_FX_H
#define CG_COD1_SCRIPT_FX_H
#include <stdint.h>
#include "../game/bg_cod1_script_fx.h"
#include "../game/bg_cod1_tag_fx.h"
#include "../game/bg_cod1_loop_fx.h"
void CG_Cod1LoopFxCommand(void);
void CG_Cod1LoopFxFrame(void);

typedef int (*cod1ScriptFxAdmission_t)(void *user);
uint64_t CG_Cod1EfxGeneration(void);
int CG_Cod1EfxScriptRegister(const char *path,
    cod1ScriptFxAdmission_t admission, void *user);
int CG_Cod1EfxScriptPlay(int cache_id, const float origin[3], int directed,
    const float direction[3], cod1ScriptFxAdmission_t admission, void *user);
/* R128 private port entry for a complete world frame. The caller supplies a
 * current pose and its own identity admission; this API does not resolve an
 * actor, infer a model revision, or certify original CoD FX renderer parity.
 * Finite unit, orthogonal, right-handed axes are copied without rebuilding
 * their roll. Admission and cache/scene/clock leases remain current at commit.
 * Disabled FX consumes the request; successful scheduling is not GPU draw. */
int CG_Cod1EfxScriptPlayBasis(int cache_id, const float origin[3],
    const float basis[3][3], cod1ScriptFxAdmission_t admission, void *user);
void CG_Cod1EfxScriptClear(void);
/* Separate current world attachment channel. Never a frozen PlayBasis frame. */
int CG_Cod1EfxAttachedRegister(const char*,cod1ScriptFxAdmission_t,void*);
int CG_Cod1EfxAttachedPlay(int,int,uint64_t,uint32_t,cod1ScriptFxAdmission_t,void*);
int CG_Cod1TurretFlashFrame(int,uint64_t,uint32_t,float[16]);
void CG_Cod1EfxAttachedResolver(int(*)(int,uint64_t,uint32_t,float[16]));
void CG_Cod1EfxAttachedRetire(void);

#endif
