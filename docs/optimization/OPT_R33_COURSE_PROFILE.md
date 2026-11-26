# OPT R33 — actual course and publication profiling

Parent R32: `0920f483f19929a2ca90ccd80118412f43bda38e`. This is a follow-up to the user's Steam Deck
capture, not a measured FPS fix. The source-pose ladder mask can be zero
while the retained Training mover owns the ladder pose. The publisher builds
and validates a source packet before deciding to use the prior scripted
profile. The checks are admission fences: do not skip them blindly.

R33 extends the existing flags, without changing game state, route selection,
trace geometry, animation weights, cache keys, model quality or notifications.
There are no new cvars. Use the same R31/R32 capture commands. New GAME stages:

- publish: full admitted unmounted publication attempt, including both pose
  paths and final wire validation; ladder mask/actors now refer to successfully
  published scripted ladder wire frames (1 down, 2 on, 4 off, 8 up).
- discard_pose: successful source sampling + encoding followed by selection of
  the prior traversal profile; measures work whose serialized pose is discarded,
  not proof that its validation can be removed.
- prior_pose: scripted pose sampling after prior-profile selection.
- motion: full active actor motion frame after admission, including callback.
- ladder_up / ladder_down: retained mover's entire climb call (including
  transition, based on the route direction, not the exact animation phase).
- sweep / floor: motion capsule sweep and floor projection (nested).
- ladder_trace: direct body capsule trace in the ladder mover, measured
  separately from sweep. The void syscall is counted as completed; it does not
  imply clear passage.

For motion/sweep/floor/ladder a nonpositive/refused result can include normal
blocked movement; failed is not necessarily a game fault. Timings are
inclusive, and cannot be summed as exclusive frame time. Empty windows are
not flushed; a later call to the same stage is needed for output. Continue
through the course and normal gameplay. Source pose and prior pose may be
nested in publish, and a floor call may include multiple sweeps.

New CGame stages: course (non-ladder course overlay), course_ladder (overlay
for a ladder wire frame on a pose miss), course_load and course_map (first-use
course clip load and mapping). The two course blend samplers also contribute
to tree_sample. The old decoded_ladder_actors field remains a source-decode
measure only; use GAME publish for ladder occupancy. Cache hits do not
execute course overlays.

Baseline user observation: minimum 44 FPS on Steam Deck, previously 25 FPS;
not a controlled A/B comparison. R31/R32/R33 are measurement builds, not a
claim of speedup. Old high-rate log output still needs a separate audit.
Native Windows build and in-game acceptance are pending.

## Validation

Exact observer/aggregation/test Git blobs verified with git hash-object.
Both extended test programs passed GCC -std=c99 -O2 -Wall -Wextra -Werror
and GCC -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer -no-pie
with ASAN_OPTIONS=detect_leaks=0. The Win32 clock and client math calls are
stubbed; this is not a native full-engine integration test. Reproduce:

    for t in tools/tests/opt_r31_anim_runtime_test.c tools/tests/opt_r32_anim_profile_test.c; do
      cc -std=c99 -O2 -Wall -Wextra -Werror -Itools/tests/r31_include "$t" -o /tmp/r33test && /tmp/r33test || exit 1
    done

The ladder wrapper observes the direction already calculated by the original
mover, not a second route lookup. No source packet checks were removed.
