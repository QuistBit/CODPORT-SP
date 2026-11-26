# OPT R31 — GAME AnimTree observation

Base: merged master `9f224da9f2a5e8568f22520cb2e24ebb366717ec`. This is a diagnostic step, not a measured
speed improvement or a Training-specific workaround.

Observed in source: the generic_human catalogue contains 2078 nodes.
ActorGenericClone already reuses two allocations, but still copies full nodes
and notify state. ActorFaceFrameAcquire creates additional actor/entity copies
and performs closure and lifetime checks. ActorControllerAnimationStep may
repeat the tree step, drain notifications and resume GSC within one GAME tick.
The client already caches endpoint poses (32 buckets, two slots per bucket);
a client-side profile is a separate next step. The user's observation of low
FPS while standing on the ground watching recruits descend makes actor
processing a priority, not proof of a particular hot function.

## Capture (Windows x86)

Build this branch using Build-Windows.cmd -Output build_opt_r31.
Run a new Training session; use the same settings as before. Start on the
ground with a view of the wire and both ladders. In the console:

    set logfile 1
    set g_cod1AnimProfile 1

Watch the actors pass the course, including the second ladder, and note the
FPS and approximate game time at the trough. Continue for another second
after the last actor descends, then:

    set g_cod1AnimProfile 0

Save rtcwconsole.log. Repeat without diagnostics to check overhead. R31 works
independently of r_smp (GAME thread only); R241 still requires non-SMP.
No new cvar is archived or required: an absent value is zero. Only value 1
enables it; the server polls it once per observed GAME frame. On other
platforms the diagnostic is compiled out.

Each [OPT_ANIM] row summarizes an independent 250 ms game-time window.
It is emitted on the next observed call after the window ends; the last
partial window is intentionally not flushed. Map epoch changes discard
incomplete windows. Counted owners are unique entity slots within the window,
not all actors on the map or necessarily simultaneous actors.

- controller: entire ActorControllerAnimationStep after input validation,
  including preparation, step/drain loops and GSC resumes. iterations is
  the loop attempt count; max_iter is the maximum for a single call.
- pose: ActorSourcePoseSample including acquire, checks, projection, resource
  validation and release. Nodes/leaves are maxima for successful samples.
  ladder_mask: 1=climbdown (318), 2=climbon (319), 4=climboff (320),
  8=climbup (321). ladder_actors counts slots with at least one reachable
  positive-weight ladder leaf sampled successfully; it is not a transition
  counter or an exact count of actors physically on a ladder.
- sum_ticks, max_tick_ticks (sum within one observed server game tick),
  max_call_ticks are QPC ticks. Divide by hz and multiply by 1000 for ms.
  Controller and pose may be nested; do not sum them as exclusive times.
  Rows exclude the diagnostic formatting and output itself; that output
  can still affect measured FPS. No per-actor log spam or timers when off.

No changes to tree weights, animation events, movement, collision, renderer
or AI. No validation fences removed. Keep current CGame and runtime
modules matched. A later client-side capture should distinguish endpoint
cache hits, blend, skeleton build and first-use clip decoding.

## Validation

Portable C accumulator tests: strict GCC and ASan/UBSan, both passed.
Covers rollover, same-tick sums, counts, maxima, unique slots, ladder bits,
epoch reset, backwards time and owner bounds. Reproduce:

    cc -std=c99 -O2 -Wall -Wextra -Werror -Isrc/qcommon tools/tests/opt_r31_anim_profile_test.c -o /tmp/r31_test
    /tmp/r31_test

The actual observer also passed strict GCC and ASan/UBSan with deterministic
Win32 clock stubs: disabled-mode timer avoidance, one poll per observed frame,
window output, enabling/disabling, epoch and clock rejection. Reproduce:

    cc -std=c99 -O2 -Wall -Wextra -Werror -Itools/tests/r31_include tools/tests/opt_r31_anim_runtime_test.c -o /tmp/r31_runtime
    /tmp/r31_runtime

Source integration reviewed against exact base files. Native MSVC/QPC
integration and gameplay have not been built or run here. The tests validate
portable aggregation and observer control flow, not the full GAME module or FPS.
