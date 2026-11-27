# OPT R46C — observe ordered server clock phases separately

Base R46B 687dc551200268327b94773f1c2f5bfcc8e6fe0b.
Control R40 8f92680c078b717f61aa762ef9a5c93e10eb99cb.
Read R46B and R43B1 course boundaries; inspected the pinned
FaceFrame, course, Session notification and locomotion sources.
This is a bounded observability/ownership step, not a frame-time
optimization or completion of R46.

## Why retain the existing transaction boundaries

g_cod1_actor_face_frame.inc :267–303 acquires private stage and
independent baseline, invokes current, checks all fenced state,
commits with revision increment and owned pointer replacement.
:323–329 Begin prepares weight/old-phase state, sets current tick
and publishes it. :342–364 Step publishes phase advancement and
resume. :371–387 Drain may consume accepted receipts immediately,
then publishes cleared notes through the same Commit even if there
are no new notes. Therefore an empty Drain is not a semantic no-op;
skipping it changes revision, callbacks and refusal behavior.

g_cod1_turret_host.inc :422–465 dispatches completed-frame sample,
ordinary Begin, Step, Drain and the resumed VM bucket. The sample
has observable refusal and current checks even with debug off.
The VM bucket can modify tree state or retire an actor; the next
step cannot borrow a stale stage. :613–694 orders runner scan,
speaking-runner Begins, instructor and speaking completion, with
retirement receipt and slice deadline boundaries. R40 and R46B
dispatch (3887 bytes) and course scheduling range (5301 bytes)
are byte-identical, verified from both connector revisions.
Do not fuse phases, cache callbacks or discard the completed-frame
sample without preserving these observable boundaries.

## Production change

Add a shared internal phase-observation scope and forwarding
wrappers. Use the existing g_cod1AnimProfile opt-in and the
existing ActorProfileStart31/Finish31 window aggregation in
g_cod1_anim_profile31.inc. No new cvar or setting changes.
Append seven stage IDs/names without renumbering existing stages.

- course_begin: actual Begin at all four existing course-related
  call sites (dispatch, jumper, Moody and speaking preparation).
- course_step: one actual FaceFrameStep.
- course_drain: actual Drain including all callbacks and receipts.
- course_sample: completed-frame sample including fences.
- course_bucket: FoleyRunNoteBucket including its VM and guards.
- course_note: FoleySessionQueueFrameNote including its guards
  and GSC notify enqueue; nested within course_drain.
- locomotion_kernel: actual C1AGC14_AdvanceDisposablePrepared
  including admission, search and advance, but not the surrounding
  ActorLocomotionClosure or face-note materialization. Nested
  inside Step; may also be called outside the course.

Source integration in turret_host.inc is nine call substitutions
and one header/prototype insertion. Reversing these substitutions
and restoring the declaration location reproduces the complete
original file byte-for-byte. Cursor/expiry/lease control flow,
callback arguments and return checks are untouched.
ActorLocomotionFrameAdvance invokes the original kernel through a
single forwarding wrapper, with identical inputs, return checks,
allocation, note materialization and cleanup.
No production changes to FaceFrame, Session or clock math.

The profile scope captures labels at entry and discards samples
if the epoch, game tick or frame changes before return; owner
label is captured before callbacks. The return value of the real
operation is forwarded without change. On non-Windows-x86 builds
course forwarding names expand to the original provider names;
the kernel wrapper remains a simple forwarding function.
On native Windows x86 with profiling off there is still the
existing cached enable check at each added measurement site and
small wrapper overhead; timing/overhead not measured.
Enabled profiling adds QPC calls and at most one existing-format
aggregate line per active stage per completed 250ms game-time
window, not a print for every event. This is diagnostic overhead,
not a new default logging stream. Existing observer aggregation
and partial-window behavior are unchanged.

Do not add nested inclusive timings together. course_note is
part of course_drain; locomotion_kernel is part of a Step where
used. course_bucket includes VM execution, not only animation.
Some course_begin calls belong to neighboring native jumper/Moody
stages rather than NATIVE_COURSE itself; compare by call and
time-window context. Existing OPT_FRAME course/game measurements
remain the top-level totals; wait between paused slices is not
part of these new synchronous call durations.

## Verification actually performed

Local exact wrapper blobs:
g_cod1_phase_profile46.h fa828eea8217b44db7f89a2668ec071948e9ef81,
g_cod1_course_profile46.h d0c3ad0307738b11ce45d4c203a17fca49064cf9,
extracted kernel wrapper 438f23655d041ec384fe83ea28cc18dad38f23fb.
Fixture 8e60299f01336193fbb5bf144f9c7194c96e0d0d.
Verified all local hashes; the candidate kernel wrapper extracted
from the actual Git blob matches the tested fragment. Reversal of
call substitutions verified against the pinned source.

Four local runs of the exact wrappers with controlled provider
and profiling stubs: portable and simulated Windows preprocessor
branch, each GCC C99 -O2 -Wall -Wextra -Werror and combined
ASan/UBSan (-O1 -g -fno-omit-frame-pointer -no-pie).
Each run passed 56 cases: seven phases x enabled/disabled x
success/refusal, plus seven phases x four mutations of epoch,
time, frame and owner label. Checks cover exact callback/context
and output arguments, provider call count, callback/delivery
order, return forwarding, unchanged outputs on modeled refusal,
profile labels and dropping cross-epoch/tick/frame observations.
All four executions printed 56 cases PASS.

Reproduce:
    sh tools/tests/run_opt_r46c_course_profile_test.sh

Driver syntax checked locally. Local compiler invocations used
the exact wrapper files and extracted fragment, not a full
checkout. Windows preprocessor branch is GCC with stubs, not
MSVC or QPC execution. Actual provider algorithms, scheduler,
Session retirement and nested VM callbacks were reviewed by
source, not executed by this fixture. Existing R45D1/D2 and
R46A/B regression coverage remains separate. No complete GAME
DLL build, real profiling capture, performance or FPS claim.
Hunk remains 256 MiB; no settings, graphics, actor or clock
changes. No intermediate gameplay check requested.

## Next

R46D: establish whether an actual larger reduction in repeated
transaction or completed-frame sample work is equivalent at the
observed boundaries. If not provable, retain the original path
and document the limitation; proceed to R47 rather than repeat
small validator changes without evidence. R51/R52 same-stand
integration/profiling should enable the existing opt-in only
for measured runs and compare R40 to the integrated version.
R47 CFG-free normal CoD1 SP remains mandatory.
