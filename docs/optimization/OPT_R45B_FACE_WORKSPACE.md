# OPT R45B — reusable FaceFrame allocation shell

Base: R45A 0487557ca3d97b1a3e1887d0bd4fb2c1cbf22c24.
Control: R40 8f92680c078b717f61aa762ef9a5c93e10eb99cb.
Read R45A ownership/test gate and R44C handoff. This migrates only the
FaceFrame allocation shell; the three-array clone pool and independent
controller/baseline/expected computation remain unchanged.

## Admission and lifetime

At base g_cod1_actor_face_frame.inc :251–300, the R35 allocation holds
the call and separate actor/entity snapshots. Acquire allocates after
provider entrance, argument and busy checks, but before ActorGoalLease,
closure, clones and publication. Leave and the rejected path free both
owned clones before freeing the shell. No shell ownership is transferred
by Commit (only stage node/notify/name allocations are transferred).
Retain this order. Do not use cod1GenericOperation.busy to decide
whether a shell is available: there is an unpublished admission interval.
Do not make existing provider rejection/reentry checks weaker.

The new header retains at most one idle shell on the main GAME thread.
Checkout detaches idle storage or allocates an independent block; links
it in the active list before returning. The header is fully zeroed,
snapshot pointers restored and actual Acquire still copies both entire
snapshots before publication. No previous semantic state is reused.
Release removes only its exact active shell and retains it if the idle
slot is empty, otherwise frees it. No allocation or callback during
ownership transfer. Existing allocation refusal on a cold miss remains;
a cache hit can succeed under allocator pressure where R40 would fail.
No added recursion cap. Active heap storage scales only with outstanding
admitted calls, not completed calls.

Base g_cod1_actor_goal.inc :4–23 connects ActorGoalBuffers to
G_Cod1EntityStorageProtected (entity_host.c :923–941); that predicate
uses TurretStoragePrivate (turret_host.inc :31–35), then
ActorGenericPrivate (generic_host.inc :130–148), ActorFacePrivate
(face_host.inc :12–17) and ActorFaceFramePrivate (face_frame.inc :258–264).
Direct ActorGenericPrivate checks also reach the same predicate.
Cover the two workspace metadata pointers, full idle allocation and all
active shells (including before publication). Preserve existing baseline,
stage, live array, name and frame guards. ActorGoalLease uses the
same-allocation predicate (entity_host.c :344–353), not an external
buffer admission, so its internal borrowed owner is not rejected by
this new shell range guard. No pointer to a private snapshot is
intentionally exported. Full native provider/alias replay is still
required; this is source review, not a proof of all external callers.

ActorGenericReset invokes ActorFaceReset then cancels busy generic work
(generic_host.inc :156–173). ActorFaceReset calls ActorFaceFrameReset
(face_host.inc :19–23). Add idle-shell clear to the existing frame reset;
active allocations must remain live until original unwind, including
prepublication owners. A later release can repopulate idle capacity;
the next checkout zeros the header and Acquire recopies the snapshots.
Do not change frame/resource revision, cancelled flags or reset order.
Accepted note receipt and poison semantics untouched.

## Validation

The unchanged R35 reference header is byte-identical in R40 and R45A
(blob 359c497c7466bb55d93f1e619030120a4d35b24f).
Local production workspace header blob
24c64f8148effcf7003a2a64a9a21c7a08a23505;
fixture blob b29f1c6d33332851068dd330df2a17ce97f13d3e.
Exact headers tested with synthetic snapshot types, tracked malloc/free
and a range-overlap stub. 1000 sequential reference requests allocate
1000 shells; 1000 new requests allocate 1 shell. Additionally 1000
old/new defined snapshot/header comparisons after dirty reuse; three
simultaneously checked out shells with independent snapshots, non-LIFO
unlink, two injected malloc failures, idle hit with allocation failure
pending, metadata/idle/active range protection, reset with active shells,
repeat clear, release after clear and zero outstanding allocations.
Both strict GCC and ASan/UBSan passed locally.

    cc -std=c99 -O2 -Wall -Wextra -Werror -Isrc/game/gsc_host tools/tests/opt_r45b_face_workspace_test.c -o /tmp/r45b
    /tmp/r45b
    cc -std=c99 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -fno-omit-frame-pointer -no-pie -Isrc/game/gsc_host tools/tests/opt_r45b_face_workspace_test.c -o /tmp/r45bsan
    /tmp/r45bsan

Tests exercise actual allocation headers, not integrated Acquire/Fence/
Commit/Drain, GAME callbacks or native MSVC x86. Only the headers and
fixture were compiled, not a complete checkout. One extra retained shell
and one link pointer per active allocation are the memory tradeoff;
actual target ABI allocation size and FPS effect are not measured.
The added range checks have a cost. Hunk 256 MiB unchanged. No physics,
script timing, rendering, actor count or feature selection changes.

## Next gate

R45C: inventory every producer/consumer/free of nodes, notify and names,
including live owners, partial malloc failures, controller rebase and
reset. Establish a common allocation ownership model before coallocating
the triple or reducing deep copies. Compare actual callbacks and receipt
semantics to R40; no revision-only fences. R45 overall remains open.
R46 course clocks and R47 CFG-free startup remain mandatory. No
intermediate gameplay test requested.
