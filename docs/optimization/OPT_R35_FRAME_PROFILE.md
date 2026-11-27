# OPT R35 — GAME/GSC frame profile and FaceFrame storage

Base R34: `a508de88aa3426ad8ef6f9fb157352611b81cfb0`. R34 gameplay observation: obstacle course smoother,
ladder minimum broadly unchanged. R33/R34 profile showed that measured
publication, movement and CGame pose work alone cannot explain the entire
frame. The regular course clock uses FoleyDispatchFrame and FaceFrame
operations, not just ActorControllerAnimationStep (R31's sparse samples).

## Allocation change

ActorFaceFrameAcquire formerly allocated a zeroed call and two separate
actor/entity snapshots. One private allocation now owns those three records;
only the call header is zeroed, and the snapshots are filled by the same
full memcpy as before. Both rejection and normal leave free the single
allocation. The entire allocation is covered by the private-overlap check.
Baseline and writable tree clones still use the original pool, and all
leases, callback order, byte comparisons, closure checks, revision writes
and notification handling are preserved. No allocation survives a call.
This reduces three allocation/free pairs to one per successful acquire;
failure allocation behavior may differ under actual memory exhaustion.
No measured FPS gain is claimed.

## Capture

Use the usual identical graphics/thread settings, with developer,
g_debugMove and g_cod1NavigationProbe at zero. Add to launch arguments:

    +set g_cod1FrameProfile 1 +set g_cod1AnimProfile 1 +set cg_cod1AnimProfile 1 +set logfile 1

The new switch defaults to off when absent. Existing profiles are unchanged.
Native Windows x86 only. [OPT_FRAME] rows use QPC ticks, convert to ms by
multiplying by 1000/hz. Separate 250 ms game-time windows for GAME main and
GSC host; epoch or backwards time drops unfinished windows. Stage peaks
sum all active slices with the same levelTime; waiting between slices
is excluded. A new tick flushes the old window, not the last partial one.

- game: entire active G_RunFrameSlice call, not the complete engine frame.
- gsc: entire G_Cod1GscTestRunFrameSlice invocation.
- entities: loop sound retirement and ordinary entity processing.
- ai: AICast_StartServerFrame.
- game_post: client finalization and remaining GAME rules.
- native: GscTestNativeFrameAdvance, including preparation.
- course: pre-VM course clocks (runner and instructor).
- vm_current / vm_exclusive: selected native VM bucket phases.
- native_other: remaining native switch stages combined.
- actors: post-VM turret/course actor service.

GAME and GSC windows can differ; compare by epoch and tick ranges, not
row order. All durations are inclusive; never add parent and child. The
GAME/GSC outer wrappers label result 0 as paused because those public slice
APIs return 0 when incomplete; they do not prove successful script execution.
Native stage 0 is refused, 2 is paused. The tick count is observed distinct
game times for the local observer, not necessarily calls to every stage.
Timing includes scheduling/preemption inside calls, not exclusive CPU time.
Profiling print and QPC overhead can perturb results; repeat without flags.

This does not yet measure server networking, renderer backend/GPU or Hunk
allocation lifetime. No source pose admission was removed on the basis of
its discarded result. Further optimization should follow the new measurements.

## Validation

The exact new headers and test were compiled with strict GCC and with
ASan/UBSan using deterministic Win32 clock stubs and dummy snapshot types.
Checks: disabled timers, polling, same-tick slice accumulation, wait
exclusion, rollover, epoch/time/clock refusal, independent allocations,
header initialization, snapshot ownership, failure and single free.
Reproduce:

    cc -std=c99 -O2 -Wall -Wextra -Werror -Itools/tests/r31_include tools/tests/opt_r35_test.c -o /tmp/r35_test
    /tmp/r35_test
    cc -std=c99 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -fno-omit-frame-pointer -no-pie -Itools/tests/r31_include tools/tests/opt_r35_test.c -o /tmp/r35_san
    ASAN_OPTIONS=detect_leaks=0 /tmp/r35_san

Actual GAME module integration and Windows build/gameplay remain pending.
