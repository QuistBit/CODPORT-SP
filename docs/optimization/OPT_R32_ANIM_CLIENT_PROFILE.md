# OPT R32 — CGame AnimTree observation

Parent R31: `ed0ad613f5941d0e234f5bed9bdc31b195f71706`. Native Windows x86 only. This is diagnostic work,
not a performance fix. The CGame actor path is instrumented on its existing
owner thread, independently of R241 and the renderer SMP setting. No changes
to tree clocks, pose-cache keys or replacement, bone math, interpolation,
resource order, event handling, graphics quality or AI.

## Capture

Build the current optimization/animtree-r31 branch, which now contains R31/R32:

    Build-Windows.cmd -Output build_opt_r32

Use the usual identical single/multi settings; save each log before restarting.
For the first detailed capture prefer single, so the existing R241 trace can
also run if needed. The R31 and R32 observers themselves do not require single.

    set logfile 1
    set g_cod1AnimProfile 1
    set cg_cod1AnimProfile 1

Stand on the ground watching the recruits go through the first and second
ladder, including the lowest FPS during descent. Continue at least one second
afterwards with actors still being processed so completed windows can print.
Then disable both flags, exit normally and save profile/main/rtcwconsole.log.

    set g_cod1AnimProfile 0
    set cg_cod1AnimProfile 0

Note mode, device, FPS cap, VSync, approximate FPS/timing at the trough and
whether the camera faced the actors. A separate unprofiled repeat controls
for logging overhead. Avoid cg_debuganim and com_speeds during this capture.
If using R241, retain both CSV files and report when it was enabled; its
own non-SMP limitation still applies.

## Reading the client rows

[OPT_ANIM_CG] has a summary and one row per nonempty stage in the same 250 ms
CGame-time window. Compare against R31 [OPT_ANIM] by the game-time intervals
(first/last), not by matching revision numbers or assuming identical ticks.
The windows are emitted at the next observed frame. A final incomplete
window is not flushed. Revision changes, backwards time and disabled mode
discard pending observations.

| Stage | Inclusive scope |
| --- | --- |
| actor | Admitted CGFA4 entity call, after initial snapshot/lease checks, including resource lookup, pose, blending, submission and existing post-submit code |
| resource | CGFA4_Resource, including initial resource creation/registration if needed |
| pose_hit | Endpoint pose lookup returned the existing exact cached result |
| pose_miss | Endpoint pose lookup did not hit, includes successful rebuild or failure |
| source | Source-wire read/decode, source resource preparation, sampling and controller application |
| clip_load | First-use source XAnim read/decode and lease check |
| bone_map | First-use source clip-to-skeleton mapping and lease check |
| tree_sample | Actual C1AST14_Sample for this actor path |
| skeleton | Actual C1AD14_Build |
| blend | Interpolation of two endpoint poses |
| submit | Synchronous trap_R_AddCod1TurretActorToScene call; not GPU completion time |

Convert ticks to milliseconds as ticks * 1000 / hz from the summary.
max_frame_ticks is peak sum within one observed cg.clientFrame, not wall
time of the whole frame. Frames sharing cg.time are still separate.
Nested intervals are inclusive and must not be added as exclusive costs.
For cache hits/misses use pose_hit/pose_miss calls, including both endpoints
and failed misses; hit rate = hit/(hit+miss). actor calls count attempts
after admission; actors counts unique entity slots observed within a window.
frames counts only frames in which this path was observed, not all rendered
frames. decoded_ladder_actors is a lower bound from source decode samples:
cached poses are not decoded again. max_nodes/max_leaves are maxima for those
decodes; ladder_mask uses the same bits as R31 (1 down, 2 on, 4 off, 8 up).
The cvar is read once per observed client frame; no QPC timing or per-actor
log messages when disabled. This profiles CGFA4 only, not all actor formats,
all CGame or the renderer backend. Diagnostic print and nested timer overhead
can perturb FPS; measure normal performance without them.

## Validation

The exact new observer and aggregation header were compiled and exercised
under strict GCC and ASan/UBSan, using the existing test-only Windows clock
stub plus controlled CGame/sampler/skeleton stand-ins. Checks cover disabled
timing, one cvar poll per frame, window rollover, same-time separate frames,
unique slots, ladder bits, stage failure propagation, revision/time/clock
rejection and log fields.

    cc -std=c99 -O2 -Wall -Wextra -Werror -Itools/tests/r31_include tools/tests/opt_r32_anim_profile_test.c -o /tmp/r32_test
    /tmp/r32_test
    cc -std=c99 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -fno-omit-frame-pointer -Itools/tests/r31_include tools/tests/opt_r32_anim_profile_test.c -o /tmp/r32_test_san
    ASAN_OPTIONS=detect_leaks=0 /tmp/r32_test_san

Native MSVC build, full CGame integration execution, and real game captures
are still pending. No new FPS measurements. No assets or binaries added.
