# OPT R28 — frame timing without duplicate runtime instrumentation

Parent: OPT R27 0aed72d66941d751ad0782c94108c16d073c5895.
Accepted master remains R26 8cc16d1cd051bc6d33103ed2d801a5ad18b6d5d9.

## Source audit and design decision

The engine already provides opt-in native Windows high-resolution paired R241
capture (r241_trace start|stop|status). The engine hooks surround the cap/event
loop, initial SV_FrameWithBudget, cgame VM call, renderer EndFrame, SwapBuffers
and the frame envelope. CG hooks surround view setup, world submission and HUD.
The shared QPC clock, identity, stage masks and bounded buffers already exist.
The capture does not run by default and requires actual non-SMP rendering.
Do not force r_smp or other presentation/simulation settings to enable it.

com_speeds is a separate millisecond console diagnostic. It reports every
frame and mixes inclusive/exclusive intervals; its output perturbs timing.
In particular, Com_Frame can drain deferred server work inside CL_Frame or
after drawing. The R241 initial server interval is not total server work.
Do not interpret post-EndFrame as exclusively server or swap wall time as
GPU execution time. The envelope excludes work before CL_R241TraceFrameBegin
(including worker configuration) and after CL_R241TraceFrameEnd.

R28 reuses this runtime diagnostic rather than introducing competing timers,
new VM syscalls or changing any engine source. It adds an offline Python
summary with count/median/nearest-rank p95/max, in milliseconds. It validates
capture identity, headers, footer/count, invalid/overflow markers and CG
acknowledgement, filters partial/non-gameplay/non-single-draw frames, checks
timestamp order and pairs CG rows by containment on QPC plus input time/stereo.
The engine interval population may exceed the paired CG population; counts
are explicit. No GPU timings, no exclusive cost claim, and no timing columns
should be summed. Swap cadence is consecutive eligible swap-exit spacing,
not engine simulation msec; gaps/filtered rows break the series. CG view/world
and HUD intervals are nested within the cgame VM call. World-to-HUD includes
RenderScene and tile clear, not just rendering. Capture overhead remains.
This is a conservative first-pass analyzer, not a general R241 forensic validator.

## Reproduction (R27/R28 build or matching R26 R241 modules)

Keep the same build/modules, resource paths, settings, resolution, VSync,
FPS cap and viewpoint route. Do not run com_speeds during R241 captures.
At a loaded Training map, with non-SMP already active:

    r241_trace start

Close the console, perform a short repeatable action, reopen console, then:

    r241_trace stop

Export before changing maps. Record separate captures (5–10 seconds, shorter
at high FPS) for stationary ground facing the ladder, ground movement,
approach/climb and, if practical, stationary on the ladder facing the same
direction. Repeat each; note exact route, viewpoint, camera and settings.
The engine capture is bounded to 2048 rows and 20 seconds; CG to 4096 rows.
Auto-stop freezes, but manual stop exports. Overflow/invalid captures are
rejected by the summary. Keep original paired CSV files in the game's r241/
output directory, with names printed by the export path conventions.
Do not send captures across map resets. If refused due to SMP, report that
and current settings rather than silently changing them.

For each pair (replace paths with actual filenames):

    python tools/opt_r28_frame_profile.py path/to/trace-...-engine.csv path/to/trace-...-cgame.csv

Also collect full rtcwconsole.log from the R27 load profiling pass separately.
Provide both CSVs, not only the derived report; include capture labels.
No runtime timings have been obtained here.

## Verification

Local Python syntax check and six synthetic unittest cases passed.
The tests cover interval conversion, cadence, partial/console filtering,
unmatched coverage, mismatched capture identity, overflow/invalid markers,
ambiguous joins, malformed timing, truncation, footer counts and duplicate
headers. Run from repository root:

    python -m unittest discover -s tools/tests -p test_opt_r28_frame_profile.py

The test imports the analyzer from tools; use the following cross-platform
command instead if tools is not already on PYTHONPATH:

    python -c "import sys,unittest; sys.path.insert(0,'tools'); r=unittest.TextTestRunner(verbosity=2).run(unittest.defaultTestLoader.discover('tools/tests',pattern='test_opt_r28_frame_profile.py')); sys.exit(not r.wasSuccessful())"

Tested source blobs: analyzer 76a421bfddf03f625ed9629c4d6bf04d50b72fee,
test 71e15cd3cedbad873b962fa5fa8c79133bc2487d.
No MSVC build, no actual Windows/R241 capture or gameplay test was available.
No performance improvement is claimed.

## R29 next step

Audit ladder flag propagation and prediction/collision paths before adding
narrower diagnostics. Existing R241 pred_flags/snap_flags are eFlags, not
pm_flags; PMF_LADDER (bg_public.h) is a pm_flags bit. Do not classify ladder
frames using existing eFlags columns. The manually labelled captures can
already narrow down expensive wall-time intervals. A precise ladder-state
instrumentation and/or separate deferred-server timing should be considered
only as a small diagnostic step if warranted; real captures are necessary
before claiming a cause or altering behavior.
