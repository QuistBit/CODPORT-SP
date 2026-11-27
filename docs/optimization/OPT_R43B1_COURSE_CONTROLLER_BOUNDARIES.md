# OPT R43B1 — course scheduling, controller authority and topology consumers

Base R43A: `0d9a658828fcd5e503ab0f66a4532b5e15c352e2`; executable reference remains R40
`8f92680c078b717f61aa762ef9a5c93e10eb99cb`.
GitHub compare R40..R43A: one forward commit, solely the R43A design
document, no production changes. Read R43A and R37–R40 handoffs.
This is the bounded course/controller part of R43B, not completion of
the all-writer inventory, R43, or any runtime optimization.

## 1. Scheduling is an ordered continuation, not a parallel actor loop

All paths below relative to src/; line ranges are at R43A/R40.

game/gsc_host/g_cod1_native_thread_frame.inc:11–24,66–122:
GscTestNativeFrameAdvance opens one R237 frame under Session identity,
physical/logical time and early configuration gates; a same-time
decision skips duplicate phases. Cursor order is:
GOALS -> RIG -> JUMPER_BEGIN -> MOODY_BEGIN -> COURSE -> MOODY_END ->
JUMPER_END -> CURRENT -> EXCLUSIVE. COURSE return 2 retains its own
coursePaused state. Current and exclusive phases have their own slice
continuations. On result 1 the stage advances before checking expiry;
result 0 is refusal. C1FP35 records each active part; COURSE includes
nested callback/VM work, is not exclusive AnimTree math. Do not sum
nested inclusive durations or convert a server tick into an FPS sample.

game/gsc_host/g_cod1_turret_host.inc:607–694:
CourseClockCursor holds next/count/stage, instructor ref and lease and
speaking leases. Stage 1 scans selected scripted face frames in ascending
actor index, checks allocation, and dispatches eligible non-speaking
runners immediately. Named-active runners pass the closure/owner checks
and enter speaking list in that order. Stage 2 Begins all speaking frames
in list order, then dispatches instructor. Stage 3 finishes speaking
frames; retired leases can be skipped only with the explicit deletion
receipt. This makes Elder pose current BEFORE a note can wake a script
that reads the actors' tags. A committed instructor deletion can occur
during dispatch; resumption may proceed after it only in stage 3 with
FoleySameBucketRetirement. Final completion wins over deadline at the
last speaking actor. A separate unsliced path uses a stack cursor, the
sliced one a static cursor reset at non-resume. Its continuity relies on
the enclosing frame cursor and current callbacks: do not expose this as
a reusable global work queue. No new map-specific routing or actor cap.
A generic scheduler replacement must preserve this partial order and
the original expiry checkpoints, not merely final node values.

turret_host.inc:399–469:
FoleyDispatchFrame checks current, lease and time; unselected clocks
delegate to legacy GenericFrame. Already-completed matching tick/frame
still invokes G_Cod1ActorScriptedSampleFull and then current, even with
g_debugMove off. This is a candidate for measuring read/validation cost,
NOT permission to discard the sample: its failure is observable.
Same-tick first entry is a distinct early success path. Otherwise Begin
once (unless prepared); Step, Drain if open, then if resume run the
current VM bucket and revalidate lease, owner, runtime/catalog, flags
and frame before another Step. Committed actor retirement ends only
actor work, not the VM frame. Never advance another 50 ms just because
a script wakes, and never cache a live node pointer across the bucket.

game/gsc_host/g_cod1_training_turret_host.inc:299–308,375–437:
GscTestCourseActorFrame has a separate closed-frame admission path.
FoleySessionQueueFrameNote validates exact packet and current owner,
Session state, time, open frame and generation, queues entity notify,
then repeats currentness. FoleyPrivateFrameNote binds the exact borrowed
note pointer to foleyDelivering; each FoleyFrameNoteCurrent invokes
ActorFaceFrameFence (turret_host.inc:399–418). This explains why one
delivered note may trigger several full comparisons, without proving
any duration. Delivery and bucket execution are different effects.
FoleyRunNoteBucket checks identity both sides, then recognizes deletion
only by a committed matching receipt. Accepted notifications cannot be
rolled back or repeated merely because subsequent validation refuses.

R46 counters: native stage entries/resumes; course actors scanned,
ordinary dispatches, speaking Begins, instructor and speaking finish;
per-actor Begin/Step/Drain counts; delivered notes, bucket invocations,
retirements; full fences/compared bytes by caller. Time separately the
pure work and nested bucket; compare identical replay with same frame
and host times. Do not instrument every event with a print.

## 2. Two distinct controller scopes and an independent expected model

game/gsc_host/g_cod1_actor_scripted_controller.inc:41–121:
ActorControllerProviderArm establishes a one-shot, exact kind/owner/
time/current-function/context permit for direct CLOCK_SELECT or
RECORD_UPDATE. Enter consumes it before callbacks; frame child needs
its separately armed permit; Finish validates parent and expected frame
count and clears gate. Foreign mutation during a busy controller cancels
it. A broad "busy owner is trusted" flag is not an equivalent replacement.

Same file:229–304,409–472,686–731:
ActorControllerBegin allocates before/stage/entity snapshots and deep
generic baseline, remembers face frame, normal-record pointer/value and
native static record. Current compares whole scope before and after the
foreign callback, plus operation header integrity. ExpectedBase allocates
an additional expected object and clones generic baseline. Expected
record update computes pose, completion, optional SetAnim/cancel-subtree,
revision and native/entity changes. Actual provider executes separately,
then ExpectedMatches compares exact post-state, including node/notify/
name bytes, pointer identity where meaningful, frame, native-static,
actor and entity. ExpectedRebase transfers expected owned arrays into
the next baseline, not into live state. Previous baseline freed, expected
pointers cleared; already-replaced live storage must never be freed by
the rebase. Expected calculation is an independent effect certificate,
not just a duplicate transaction to delete. Keep it until a new
publication API proves exactly the same allowed write set and
observable failure behavior, including zero-frame deletion of an empty
normal record versus a one-frame committed update. Important distinction:
provider may have committed; later verification failure is not a
license to replay that provider.

game/gsc_host/g_cod1_actor_controller_animation.inc:15–110:
Separate AnimationStep scope owns borrowed service table, request,
controller revision, DObj and lease. It cannot coexist with busy
record controller. PreparedInvoke reopens the SAME closed prepared
frame, not another weight/oldPhase advance. Single-use framePermit
authorizes each Step/Drain with exact callback/context. Resume invokes
run_current with vm_running set, then verifies scope again; bounded
512 iterations and mid-yield suppression remain. Notify binds packet
identity, copies it across callback, verifies unchanged bytes.
g_cod1_actor_source_dispatch.inc:31–79 binds an exact source receipt/
request within the permitted VM-running path, forbids concurrent source/
look-source checking and consumes its own scope. Do not make arbitrary
run_current or current callback dynamic extent into write authority.
These two controller paths need separate explicit capability types
even if they eventually share workspace storage.

R45 implementation prerequisites: include expected storage, protected
idle pool and nested stage in alias exclusions; track cancel/retire
while on stack; do not recycle workspace before its owning unwind.
Instrument allocations/clones and actual/expected computations
separately. R43A byte estimate is a lower bound for FaceFrame, not a
count of all controller or VM work.

## 3. Additional generic access paths: cannot replace storage in one call

game/gsc_host/g_cod1_actor_generic_host.inc:
- 644–663 ActorGenericTurretCommit is a distinct writer, cloning and
  advancing all weights, SetAnim then overlays 2x25 mounted tree nodes,
  validates, advances revision and transfers ownership. It is not
  ActorGenericTransactionCore. Keep it in mutation coverage.
- 665–714 GenericPresent is a lease query. GenericSample creates a
  projected copy and actor/entity snapshots, advances copied weights,
  overlays mounted/detached state, checks covered leaves and outputs
  98 weights only after second current/identity fence. It must not
  increment generic revision. It has different fences than FaceFrame.
- 718–754 UnmountedSample reads already advanced tree at exact tick,
  copies it and snapshots, checks supported leaves, returns weights/
  phases; final fence also compares copied nodes to live bytes.
- 767–822 RootInterval takes original and working copies, overlays
  already advanced MG owner rather than advancing it again, checks
  projected leaf coverage, revalidates owner and publishes result.
- 823–829 GenericFrame routes selected/unselected/present cases
  differently; it is not an alias of FaceFrame.
- 832–887 loaded length query uses copied request, resource admission
  and metadata resolution on both sides of callback. It deliberately
  never keeps a resource pointer across that callback.
g_cod1_actor_scripted_prefix.inc:101–127,202–231 stores original
node/notify/name pointer identities AND cloned data for fresh-prefix
handoff. g_cod1_actor_scripted_record.inc:148–150,198–241 has further
FaceFrame provider entries/commits. g_cod1_actor_unmounted_publisher.inc:
27,117 stores generic header plus actor/entity state; publication is
not just a tree read. These are additional consumers to retain when
migrating representation; a full cross-include audit remains open.
Do not globally weaken fences to the least strict read path.

## 4. Prepared topology: concrete first migration candidate

game/gsc_host/g_cod1_actor_current_root.inc:2–40:
ActorCurrentRootOrder constructs first/next by inserting source-order
siblings at the head, then reserves consecutive immediate-child native
indices before recursing. Source ordinal remains canonical for storage
and GSC lookup. It requires parent < source index (the actual parsed
catalog), root parent zero, child count agreement, bounded depth, and
complete frontier. This is stricter than generic C1AGF14_Build, which
accepts forward-parent definitions and takes an explicit native-order
permutation. Do not silently reject valid generic forward-parent trees
by extending this particular source catalog contract to the public API.

ActorCurrentRoot.inc:104–149 (file name lowercase as above):
Current root query allocates a large context with two dense node arrays,
snapshots actor/entity/MG, overlays mounted tree on private working
copy, calls current and Fence, validates nodes, reconstructs native
order, traverses positive-goal children, samples, calls current and
Fence again, and re-resolves resources before returning result.
Its collection order and native ordinals affect numeric accumulation.
g_cod1_actor_scripted_record.inc:54 also rebuilds order for native
ordinal mapping. g_cod1_actor_locomotion_resources.inc:137–170 already
constructs this same order at resource load and feeds C1AGF14_Build.
qcommon/cod1_actor_generic_frequency14.h:9–38 stores first/next/order
and validates native-order permutation, parent chains, child counts;
topology contains no mutable clock. Do NOT add another independent
mutable topology cache to every actor.

R44 proposed first safe substep:
1. Extract/define a pure compact prepared source-order view (first,
   next, native ordinal, count) and differential-test exact output and
   failure conditions against ActorCurrentRootOrder. No change to
   generic public contracts or phase/weight storage yet.
2. Keep publication at the generic catalog lifetime, not at a particular
   resource bank lifetime. Prove catalog writer/reset and nonalias
   coverage first. Prefer reuse of already-built equivalent data
   where lifetime and validation are equal. Existing locomotion bank
   may be absent on other paths, so do not make all consumers depend
   on it. Do not add a failure to previously admitted map load merely
   because optional optimization allocation fails; use reference
   path until prepared view can be safely established.
3. Switch one read consumer under matching map epoch/receipt and
   protected storage; retain original order validation/reference
   fallback until equivalence is established. A generation alone
   cannot establish immutability. Measure count of order builds,
   allocation bytes and active cost before further migrations.
4. Differential tests: actual source-preorder trees, multiple sibling
   orders and mixed depths, single root, max count/depth, invalid
   parent/child count, cycles, duplicate/out-of-range supplied
   native order for generic API, allocation failure, stale map and
   callback retirement. Test exact child traversal and floating-point
   results (including ties), not merely permutation equality.
No production edits in this audit.

## 5. R47 dependency and acceptance

Retain the user-approved requirement: replace experimental CFG-based
campaign admission with штатный CoD1 SP session context, not hidden exec
or copying set commands into code. R43B2 should inventory map/bootstrap
and configuration capture together with direct mutation/retirement
coverage; establish how the new context is owned before constructors,
before dropping any repeated live config check. Diagnostics remain
explicit opt-in; RTCW/MP/default and unsupported maps must not gain
unproven capabilities. R51 compares clean-config normal startup with
R40 + exec training_player.cfg (including real mission behavior and
training -> us_intro). No need for user gameplay testing now.

## Status / verification / next

R43B1 complete as a source audit of course scheduling, controller
authority and selected read/write/topology consumers. R43 remains open.
All references above read from pinned GitHub source; no native build,
harness run, sanitizer run, performance measurement or change to
production code. Static findings are candidates, not proven causes of
the R40 measured peaks. R40 baseline and 256 MiB Hunk unchanged.

Next R43B2: enumerate remaining direct writes, publication/retirement
and pointer escape paths across the turret-host include graph (including
look, face, normal-record, locomotion selection, source pose/tags and
mounted variants); trace VM-side controller receipt/resume and Session
time-root ownership. Record a complete writer/reader matrix for the
prepared topology lifetime boundary and early GSC config/map lifecycle.
If unresolved, keep R43 open rather than assume a safe mutation census.
Then R44 pure prepared topology extraction with strict actual-source
differential tests; no intermediate user gameplay test.
