# OPT R29 — ladder state and client prediction timing

Parent: OPT R28 0eff58161bbb38a3302d591a61a6b3f2f39cf795.
Accepted master: R26 8cc16d1cd051bc6d33103ed2d801a5ad18b6d5d9.

## Source audit (not a measured root cause)

- src/game/bg_public.h defines PMF_LADDER = 4 in pm_flags; src/qcommon/msg.c
  serializes pm_flags with 16 bits. The R241 pred_flags and snap_flags are
  eFlags and must not be interpreted as ladder state.
- src/game/bg_pmove.c dispatches PM_CheckLadderMove and PM_LadderMove to
  the CoD-specific path for WBUTTON_COD1_STANCE and non-AI players.
  src/game/bg_cod1_ladder.h uses one reduced-bounds trace for attachment,
  then PM_StepSlideMove(qfalse) during ladder movement. The legacy RTCW
  branch can use two ladder checks; do not attribute that branch to CoD.
  The timer early return may retain PMF_LADDER.
- CG_PredictPlayerState in src/cgame/cg_predict.c starts from a current
  or next snapshot and replays unacknowledged commands using Pmove;
  Pmove subdivides commands into fixed or bounded steps. CG_TraceCapsule
  checks world collision and solid entities, whose cost may depend on
  visible/solid entities and location. The server also invokes Pmove.
- src/qcommon/cod1_ladders.h converts supported six-sided ladder brushes
  to collision triangles. No evidence yet that the triangle path dominates.
  Weapon transition, camera/view, rendering and server workload remain
  alternative explanations. The ladder flag is a state sample, not a
  statement that every command or collision within the frame was on a ladder.

## Diagnostic changes

Extend the existing R241 CG record, append five fields without changing
existing column names/order: pred_pm_flags, snap_pm_flags, predict_mask,
predict_begin, predict_end. Flags are sampled by existing CG241_State
after prediction (at world submission or fallback end); snapshot is the
current cg.snap, not necessarily the snapshot used as the prediction base.
Missing snapshot is -1. predict_mask=3 marks a completed bracket around
CG_PredictPlayerState, including any interpolation/early return inside it.
Times use the existing capture-relative QPC clock. No new syscall, cvar,
gameplay or network field, allocations or extra per-frame logging.
When R241 is inactive the hooks return before QPC; two small hook calls
remain. Active capture adds two clock samples per prediction call.
Old R28 Python usage is retained; exact-schema external consumers may
need updating because the CG CSV gains a suffix.

The optional --ladder on|off|mixed filter in tools/opt_r28_frame_profile.py
uses matching engine/CG rows and completed, ordered prediction timing:
on = both flags set; off = neither; mixed = disagree. Missing flags,
unpaired or malformed rows are excluded, not silently categorized.
Cadence does not bridge excluded rows. cg_prediction is inclusive wall time
of the entire client prediction function, not exclusive ladder/trace time.
Other engine metrics remain inclusive; initial_server excludes deferred
server work. These groups are correlation, not causal attribution.
Empty groups are reported as no eligible paired gameplay frames.
Existing capture validation and non-SMP requirement still apply.

## Required user test before further optimization

Use matching Windows modules from this branch, not just a new executable.
From an x86 Native Tools Command Prompt:
    python tools\build_windows.py --output build_opt_r29

At main menu, with the same settings/resource paths as R26:
    set com_cod1LoadProfile 1
    spmap training

Save the complete rtcwconsole.log (including OPT_LOAD); then turn off
load profiling:
    set com_cod1LoadProfile 0

At the loaded map, use actual non-SMP mode and unchanged FPS cap/VSync.
Do not enable com_speeds concurrently. For each separate 5–10 second
capture: r241_trace start, close console, perform the action, reopen
console, r241_trace stop. Export before map change. Keep both original
engine and cgame CSVs from r241/. Repeat each action with same viewpoint
and settings, note capture labels:
1. stationary on ground facing ladder;
2. ordinary ground movement;
3. approach and climb;
4. pause on ladder if possible; then descend/dismount separately.

From repository root (substitute actual paired filenames):
    python tools/opt_r28_frame_profile.py engine.csv cgame.csv
    python tools/opt_r28_frame_profile.py engine.csv cgame.csv --ladder on
    python tools/opt_r28_frame_profile.py engine.csv cgame.csv --ladder off
    python tools/opt_r28_frame_profile.py engine.csv cgame.csv --ladder mixed

Some captures may not contain all groups. Send raw CSVs, settings and
rtcwconsole.log. Also verify normal Training completion and us_intro
transition with tracing off. Do not infer improvement from trace-on FPS.

## Checks / stop condition

Eight synthetic Python unittest cases passed, including all six R28 cases,
new group and cadence checks, missing/invalid prediction state/timing
and legacy CSV rejection in ladder mode. Tested exact Git blobs:
analyzer 684962f644fed68375ec1b13c57bb6bc213f8009,
tests 6fe88709243bce76cdcb3c59d3be08c0821d3e1c.
Run (set PYTHONPATH=tools on Windows first):
    python -m unittest discover -s tools/tests -p test_opt_r28_frame_profile.py

Static C inspection checked append-only CSV schema (no duplicate columns),
field count, hook declarations/call order and inactive guards. No native
MSVC build or real engine capture was executed. No FPS or loading gain
is claimed. Next: obtain actual R27/R29 captures and analyze them;
do not add speculative optimizations or repeat diagnostic commits in
absence of new data. If prediction is expensive, separate command replay
count and world/entity collision cost in a later measured step. If not,
follow the largest verified interval instead.
