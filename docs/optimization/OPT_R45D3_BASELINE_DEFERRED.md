# OPT R45D3 — independent fence retained

Base R45D2 07294124b91978b63d1a59221710c462121d8ff9.
Control R40 8f92680c078b717f61aa762ef9a5c93e10eb99cb.
Read R45D2 and R43B1 handoffs and actual FaceFrame/controller,
record-update, animation-owner and course scheduling sources.

## Decision

Do not replace FaceFrame's independent baseline with the parent's
genericDeepBefore on current evidence. At face_frame.inc :267–303 the
child compares live node/notify/name bytes to a child-owned capture.
scripted_controller.inc :189–204 protects the parent deep ranges;
:229–274 compares parent snapshots before and after its current
callback, and :418–455 uses independently computed expected output.
:218–225 poisons on reset/retire rather than freeing active scopes.
The two parent kinds have distinct callback/context paths; the
separate AnimationStep owner has no genericDeepBefore.

An independent child snapshot also detects a correlated change to
live and parent deep bytes that a shared baseline cannot distinguish.
The existing private-range guards are important; the counterexample
deliberately writes private memory directly in a synthetic fixture.
It does not establish that any public GAME or VM API can perform
such a write, or that a shipped gameplay bug exists. However the
stronger independent-fence guarantee is not proved for a borrow.
The prior isolated tests do not establish integrated callback,
private alias and lifetime equivalence. Keep all original fences,
independent expected prediction and allocations. No production
change in this commit. Mark this optimization deferred, not R45
full integration complete. Revisit only with a proven immutable
ownership boundary, not a weaker revision check.

## Actual local verification

Reused the exact existing R45D1 extracted functions and allocator
headers, with Git hashes checked:
face core ca9944da0a41e954ad7599062a799125e9d6b4bc,
drain bee28e3d168aadf8cff3f3fa62250633ceb22345,
fixture a23dc77a39df91d5fac30e65a85853ec893a41a3,
R40 pool 34a88b8984e4e94338374f5ecb88f46a66cb9762,
R40 shell 359c497c7466bb55d93f1e619030120a4d35b24f,
R45B shell 24c64f8148effcf7003a2a64a9a21c7a08a23505,
R45C pool b6c3aca06d6134f9272035e857b597dccb17df8b.
New fixture fba3ae940f3a2fb537af1d34aedcacfbbb6376f9.

Six counterexamples per implementation: read/write acquisition times
node/notify/name mutations. With live-only mutation both comparisons
refuse; with matching parent mutation and unchanged revision the
actual independent Fence refuses but a hypothetical shallow copy
of the parent's record passes. The fixture restores ownership
before Leave. This is a predicate experiment, not an integrated
parent/child borrow implementation or actual GSC execution.
All four local executions passed (R40 and R45C, GCC -O2 strict
and ASan/UBSan). Zero tracked live allocations after cleanup.

Reproduce on checkout with R40 history:
    sh tools/tests/run_opt_r45d3_baseline_counterexample.sh

The driver is adapted from the R45D1 driver; local executions used
the exact extracted functions rather than a full checkout driver.
No GAME DLL build or FPS measurement.

## Next: R46

Do not spend more iterations on unproved baseline sharing. The
main course-clock path is distinct from the record-controller
path. In turret_host.inc :422–465 and :607 onward preserve ordered
runner/instructor dispatch, Begin/Step/Drain, VM bucket, retirement
receipts and slice expiry boundaries. In native_thread_frame.inc
:92–120 the existing course timing is inclusive, not exclusive
kernel time. In locomotion_resources.inc :249–265 the general
clock invokes C1AGC14_AdvanceDisposable on the private stage. In
qcommon/cod1_actor_generic_clock14.h AdvanceStorage checks all
metadata object ranges every step, even though the resource table
is published with the immutable map bank (:137–170 loader).
R46A: prepare an exact address index once at resource publication;
validate pointer-table identity, retain original scan fallback
and all mutable/metadata-content checks and original math.
Do not confuse this admission work with the large measured
course deficit or claim a measured gain without same-stand data.
R47 CFG-free campaign requirement and R51/R52 remain unchanged.
Hunk 256 MiB unchanged. No intermediate game test requested.
