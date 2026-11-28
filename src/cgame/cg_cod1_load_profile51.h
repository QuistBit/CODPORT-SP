/* CGAME adapter for the existing R27 load profiler. No new cvar or ABI. */
#ifndef CG_COD1_LOAD_PROFILE51_H
#define CG_COD1_LOAD_PROFILE51_H
static int CG_Cod1LoadProfileEnabled51(void) {
    char value[64];
    trap_Cvar_VariableStringBuffer("com_cod1LoadProfile",value,sizeof(value));
    return atoi(value)!=0;
}
#define C1LP_ENABLED() CG_Cod1LoadProfileEnabled51()
#define C1LP_NOW() trap_Milliseconds()
#define C1LP_PRINT CG_Printf
#include "../qcommon/cod1_load_profile.h"
#undef C1LP_ENABLED
#undef C1LP_NOW
#undef C1LP_PRINT
#endif
