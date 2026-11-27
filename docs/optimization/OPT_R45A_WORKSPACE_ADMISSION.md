# OPT R45A — transaction workspace admission and baseline fixture

Base R44C 0e99dfddb5fa8560db00f9efe3ddc217dd80d1d8; executable control
R40 8f92680c078b717f61aa762ef9a5c93e10eb99cb. Reviewed R43A/B1/B2A
and R44A/B1/B2/C handoffs and current HEAD diff. This is the first
R45 ownership/test gate, not a production allocator migration.

## Actual ownership, pinned source ranges

- src/game/gsc_host/g_cod1_actor_generic_host.inc:48–81: a clone owns
  three allocations (nodes, notify, names), not one buffer. Two idle
  triples are retained; checkout detaches all three pointers before copy.
  Cold checkout attempts all three malloc calls even on partial failure.
  FreeOwned accepts only a complete triple into an empty slot; partial
  storage is freed and all pointers nulled. Copy is full node and notify
  arrays, only initialized name prefix. The name allocation is still
  full capacity, and ActorGenericNamesPrivate in actor_notify_names.inc
  :31–33 protects that entire capacity, not just initialized bytes.
- generic_host.inc:130–148,156–173: private-range admission covers idle
  triples, active stage ranges and live actor arrays. Reset cancels
  active generic operations, frees live owners and clears idle pool;
  checked-out stage/baseline/controller clones must not be returned by
  reset. They remain owned until the outstanding call unwinds. Retire
  also cancels a busy generic operation. A release after reset can
  repopulate the idle pool: idle storage is capacity, not epoch content.
- src/game/gsc_host/g_cod1_actor_face_frame.inc:251–300 and
  g_cod1_face_frame_storage35.h:3–18: outer allocation contains call
  header plus independent actor/entity snapshots. Header zeroed;
  actor/entity overwritten before callbacks. Writable Acquire clones
  stage AND baseline, readonly Acquire clones baseline only. Before
  setting operation busy, controller provider entrance, argument and
  lease/closure checks, allocation and clone admissions all occur in
  their original order. Do not equate allocation ownership with the
  published busy flag: private work exists before publication. Keep the
  provider entrance even for a rejected reentrant request.
  After publication current(ctx) and full Fence revalidate borrowed
  input, map and time, resource revisions, actor lease, saved record
  header, all node/notify bytes, semantic name prefix, frame, actor and
  entity bytes. Commit calls current again before fence/closure;
  increments revision and transfers stage allocations into live owner.
  Leave frees only remaining owned stage/baseline; operation metadata
  is cleared on original unwind. Never recycle committed storage as
  scratch just because the call ended. Full allocation plus baseline
  arrays and stage ranges are protected separately.
- face_frame.inc:340–385: step publishes resume only after commit;
  Drain consumes accepted note immediately into LIVE frame and captured
  frame before the next callback. A later refusal cannot undo the
  accepted receipt; poisoned owner is matched by lease/runtime/catalog/
  identity/frame. Generic notify drain has a different legacy contract
  documented in R43B2A, including mutation without revision increment.
  Do not fold these into a universal rollback of live state.
- src/game/gsc_host/g_cod1_actor_scripted_controller.inc:12–39,
  64–86,189–225,260–303,418–455: outer controller retains independent
  before, stage, entity, deep generic baseline and expected result.
  The expected record owns another clone; a trusted nested provider
  gets its own FaceFrame stage and baseline. Four distinct generic
  clones can therefore coexist, in addition to the live actor. Two
  idle triples do NOT imply only two concurrently owned clones.
  Provider gate admits exactly one armed child with exact owner,
  time, callback/context; it consumes pending grant before ordinary
  provider validation. A denied entrance can poison controller.
  ExpectedMatches permits pointer replacement but compares actual
  result bytes to independently computed expected bytes. Rebase
  transfers expected clone into controller baseline, nulls donor
  pointers and frees the previous baseline, not the live actor.
  ControllerReset/Retire poison instead of freeing the active owner;
  ControllerEnd performs actual cleanup. Keep these nested scopes.
- src/game/gsc_host/g_cod1_actor_source_pose.inc:5–22 and
  g_cod1_turret_host.inc:478–482 use readonly Acquire, not a free
  license to borrow live mutable arrays across current callbacks.

## Design gate for R45B

1. Explicit roles: idle capacity; checked-out unpublished scratch;
   active immutable baseline; active mutable stage; controller
   expected; published live owner. Transfer changes role, never
   duplicates ownership. All private ranges include full allocated
   extent; preserve semantic name-prefix equality and never compare
   uninitialized tail as data. No callback during role transfer.
2. First migrate only FaceFrame call/actor/entity allocation shell
   to a bounded reusable workspace, if admission proves equivalent.
   A checkout must reserve its slot independently of published busy;
   fallback must support any pre-publication reentry admitted by
   existing checks. Protect idle and checked-out ranges in the
   actual ActorGoalBuffers/ActorGenericPrivate call graph. Reset
   invalidates idle storage, but cancels active work and leaves the
   checked-out allocation alive until its original unwind. No
   unbounded cache. Account for actual allocation failures and test
   the miss path; cache hits avoid allocation but cannot waive checks.
   Before implementation, prove prepublication lease/closure callbacks,
   output alias handling and reset ordering. Do not simply replace
   malloc/free with a global object. The R35 fixture expects independent
   simultaneous allocations; preserve independent owners.
3. Next migrate clone storage (potential single contiguous triple)
   only with a proven ownership/deallocation model for every live,
   pool, partial-failure and controller transfer path. Separate
   snapshot retention from mutable calculation before reducing copies.
   An immutable baseline cannot point to live arrays through callbacks.
   Revision-only fencing is unsound until all direct mutations are
   covered. Keep independent expected computation as reference until
   an equivalent trusted publication protocol is demonstrated.
4. Integration tests: actual acquire/leave/commit with current refusal,
   mutation of each compared range, callback reentry/reset/retire,
   lease reuse, partial drain accepted receipt then refusal, poisoned
   stale/new owner, controller nested provider permit and exact rebase;
   caller-visible outputs unchanged on refusal where contract requires.
   Compare against R40 including callback order and floating point
   behavior. Native GAME/MSVC x86 and real map run remain separate.

## R45A reproducible baseline

The actual clone pool fragment (1766 ASCII bytes, FNV1a 50469be1)
is byte-identical in R40 lines 45–78 and R44C lines 48–81.
Local reconstructed fragment hash
34a88b8984e4e94338374f5ecb88f46a66cb9762; FNV/length checked against
pinned source. Test fixture blob ab4b00fe1b9f57ebf21b75e924faa4cb63465d9d.
Strict GCC C99 -O2 -Wall -Wextra -Werror and combined ASan/UBSan
(-O1 -no-pie) both passed: 64 nested four-clone/release/clear cycles,
3 separately injected malloc faults, 2 invalid admissions, exact
defined node/notify/name-prefix comparisons, independent snapshots,
cache hit despite injected next-allocation failure, checked-out
storage survives idle clear, bounded idle storage, final zero live
allocation count. Synthetic lease/header and no actual game callbacks.
The test exercises production clone functions, not full FaceFrame.
The driver extracts current and R40 fragments and compares them before
running both test modes. Local runs used verified extracted fragment,
not a complete checkout; run the driver on checkout with R40 history:

    sh tools/tests/run_opt_r45a_workspace_test.sh

No production change or FPS improvement. No GAME DLL compilation,
native x86, real callback/reset, FaceFrame effect or controller
transaction replay. Those remain explicitly open. Hunk 256 MiB and
master unchanged. Next R45B shell workspace migration and dynamic
admission tests; R47 CFG-free normal campaign requirement preserved.
