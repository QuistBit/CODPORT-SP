# OPT R53B — R237 scheduler audit and callback-free root view

R53A base ef49b3abe3c1b99169356cde9a0aa1c00609848f; production change a8689fc5c2a1479c1739366cba404a2b2bbba6cc. No changes to master.

## Evidence and bounds

R52 log SHA256 fb8103b5b7b4a9aafd8877f403b797f10ea78951f75fc5fed65d423c1e355def. Completed Training windows after 800 ms and before us_intro initialization: vm_current 855 windows/4275 calls, 89 windows above 16.67 ms, max 109.5084 ms; vm_exclusive 855/4275, 8 above, max 32.4464; course_bucket 325/1125, 167 above, max 90.4067; course_note 370/1642, max 0.7746; course_drain 370/1493, max 2.6202. These are inclusive game-time window maxima, not presentation frame time. Do not equate window maxima with individual events or sum nested measurements.

Source audit: gsc_session_r237_frame_phase_slice validates Session identity, resource certificates before dispatch and after successful unpaused dispatch, then session_after_vm checks animation resources again and captures stats. Resource checks call host certificates, which may change state or reject reentry; do not cache them across callbacks. Runtime storage separation includes private allocation validation; do not bypass output alias checks. r237_drain_bucket_slice scans the entire task directory after each continuation, O(N*(K+1)) for N directory entries and K dispatched continuations on an uninterrupted drain. It chooses the greatest insertion identity at the current deadline and records the earliest future deadline. Dispatch can modify queue ownership, status, and directory; no persistent snapshot is valid across dispatch. r237_finish_operation closes precache and runs endon retirement and array collection at the original boundary. Array collection already has capacity and pin guards and a graph index. Do not assume it runs on every phase. This audit identifies candidates, not attribution of the 90–109 ms spikes.

## Implemented

Reuse existing R237QueueView39 inside each callback-free scan and frame-valid check. Original ownership, incarnation, cursor and error paths remain; refresh view after every dispatch, never across callbacks. No new allocation, queue, scheduling change, notification delay, gameplay config or map-specific fix. Two registry searches in scan become one; up to three in frame-valid become one. Full directory scan remains for R54 research. This is a bounded optimization, not a claimed FPS gain.

## Test

100000 deterministic cases with extracted old/new production functions and mocked root membership and continuation dispatch: 67420 completed, 25126 refused, 7454 paused, 93450 mock dispatches. GCC and Clang C11 O2 strict and Clang ASan/UBSan all passed; total 300000 cases. Test instrumentation measured 716857 -> 381793 root lookups per run. Includes legacy mode, missing/retired/replaced roots, insertion errors, requeue, future deadline, pause and failure. Exact extracted header matched connector content by length 4269 and FNV-1a fe0e6436. No full checkout/native MSVC build or game test was performed; shell runner is supplied for reproducing extraction from Git history, not executed against a full checkout here. Run bash tools/tests/run_opt_r53b_root_view_test.sh. This does not measure real CPU time or validate all engine callbacks.

Next: instrument task selection/dispatch/cleanup and count directory entries examined. Only after proof of the dominant work, implement a validated deadline/index structure with stable newest-first ordering, cancellation and reentrant mutation handling. Compare with current scan in differential tests. No user build requested yet.
