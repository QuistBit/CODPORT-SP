/* Private reliable command consumer. Arrival cg.time is a declared port
 * clock policy; original snapshot/PVS delivery is outside this binding. */
#include "cg_cod1_script_quake.h"

static cod1ScriptQuakeQueue_t cgCod1ScriptQuakeQueue;
static uint64_t cgCod1ScriptQuakeEpoch;
static uint64_t cgCod1ScriptQuakeEpochFloor;
static uint64_t cgCod1ScriptQuakeLastSequence;
static uint64_t cgCod1ScriptQuakeIncarnation;
static int cgCod1ScriptQuakeLastTime;
static int cgCod1ScriptQuakeHaveTime;
static uint32_t cgCod1ScriptQuakeRandomState;

void CG_Cod1ScriptQuakeReset(qboolean full) {
    memset(&cgCod1ScriptQuakeQueue, 0, sizeof(cgCod1ScriptQuakeQueue));
    cgCod1ScriptQuakeHaveTime = 0;
    cgCod1ScriptQuakeLastTime = 0;
    cgCod1ScriptQuakeRandomState = 1u;
    ++cgCod1ScriptQuakeIncarnation;
    if (!cgCod1ScriptQuakeIncarnation) ++cgCod1ScriptQuakeIncarnation;
    if (full) {
        cgCod1ScriptQuakeEpoch = 0;
        cgCod1ScriptQuakeEpochFloor = 0;
        cgCod1ScriptQuakeLastSequence = 0;
    }
}

void CG_Cod1ScriptQuakeConfigChanged(int number) {
    uint64_t epoch, incarnation, retired_epoch;
    const char *metadata;
    if (number != CS_COD1_SCRIPT_EFFECTS) return;
    incarnation = cgCod1ScriptQuakeIncarnation;
    metadata = CG_ConfigString(number);
    if (incarnation != cgCod1ScriptQuakeIncarnation || !metadata) return;
    if (!metadata[0]) {
        retired_epoch = cgCod1ScriptQuakeEpochFloor;
        CG_Cod1ScriptQuakeReset(qtrue);
        cgCod1ScriptQuakeEpochFloor = retired_epoch;
        return;
    }
    /* A malformed marker cannot erase a valid new scene. */
    if (!CQ_DecodeMetadata(metadata, &epoch)) return;
    if (epoch == cgCod1ScriptQuakeEpoch) return;
    if (epoch <= cgCod1ScriptQuakeEpochFloor) return;
    CG_Cod1ScriptQuakeReset(qtrue);
    cgCod1ScriptQuakeEpoch = epoch;
    cgCod1ScriptQuakeEpochFloor = epoch;
}

void CG_Cod1ScriptQuakeCommand(void) {
    char copied[10][24];
    const char *arguments[10];
    cod1ScriptQuake_t event;
    uint64_t incarnation, epoch, sequence;
    int index, count;
    if (!cgCod1ScriptQuakeEpoch) return;
    incarnation = cgCod1ScriptQuakeIncarnation;
    epoch = cgCod1ScriptQuakeEpoch;
    sequence = cgCod1ScriptQuakeLastSequence;
    count = trap_Argc();
    if (incarnation != cgCod1ScriptQuakeIncarnation || count != 10) return;
    for (index = 0; index < 10; ++index) {
        memset(copied[index], 0, sizeof(copied[index]));
        trap_Argv(index, copied[index], sizeof(copied[index]));
        if (incarnation != cgCod1ScriptQuakeIncarnation ||
            epoch != cgCod1ScriptQuakeEpoch ||
            sequence != cgCod1ScriptQuakeLastSequence) return;
        arguments[index] = copied[index];
    }
    if (!CQ_DecodeCommand(count, arguments, &event) ||
        !CQ_CommandFollows(&event, epoch, sequence)) return;
    if (cg.time < 0 || (cgCod1ScriptQuakeHaveTime &&
        cg.time < cgCod1ScriptQuakeLastTime)) {
        CG_Cod1ScriptQuakeReset(qfalse);
        return;
    }
    if (!CQS_Start(&cgCod1ScriptQuakeQueue, &event, cg.time, cg.refdef.vieworg)) return;
    cgCod1ScriptQuakeLastSequence = event.sequence;
    cgCod1ScriptQuakeLastTime = cg.time;
    cgCod1ScriptQuakeHaveTime = 1;
}

void CG_Cod1ScriptQuakeApply(void) {
    float angles[3];
    uint32_t next_random;
    if (!cgCod1ScriptQuakeEpoch) return;
    if (cg.time < 0 || (cgCod1ScriptQuakeHaveTime &&
        cg.time < cgCod1ScriptQuakeLastTime)) {
        CG_Cod1ScriptQuakeReset(qfalse);
        return;
    }
    cgCod1ScriptQuakeLastTime = cg.time;
    cgCod1ScriptQuakeHaveTime = 1;
    memcpy(angles, cg.refdefViewAngles, sizeof(angles));
    /* This private bounded phase service does not consume the legacy weapon
     * PRNG. Its state progression is port policy, not original CRT parity. */
    next_random = cgCod1ScriptQuakeRandomState * 214013u + 2531011u;
    if (CQS_Frame(&cgCod1ScriptQuakeQueue, cg.time, cg.refdef.vieworg,
                  (next_random >> 16) & 32767u, angles))
        cgCod1ScriptQuakeRandomState = next_random;
    memcpy(cg.refdefViewAngles, angles, sizeof(angles));
    AnglesToAxis(cg.refdefViewAngles, cg.refdef.viewaxis);
}
