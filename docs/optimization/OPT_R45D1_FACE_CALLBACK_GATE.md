# OPT R45D1 — FaceFrame callback and accepted-receipt regression gate

Base R45C 12c80c5f20918f6cb8faf6602c33533e5d85bc76.
Control R40 8f92680c078b717f61aa762ef9a5c93e10eb99cb.
Reviewed R45A and R45C handoffs, actual controller, record update,
scripted frame and FaceFrame source at the pinned base. This is one
bounded regression-test step before reducing nested snapshot duplication.
No production logic change. R45 is not complete.

## Why not eliminate the expected snapshot yet

g_cod1_actor_scripted_controller.inc :229–303 verifies owner/resource
identity, independent baseline and actor/entity/frame/record/static
snapshots both before and after current callbacks. The checking flag
and saved operation header also detect reentry/mutation.
:418–455 independently computes expected output, compares all defined
result bytes and rebases the independent controller baseline only
after successful provider return. Pointer replacement is allowed at
this seam, but content is not accepted by address or revision alone.

:457–471 InitClock predicts a frame/clock selection, independently
executes the real provider, and retains the staged controller actor
changes until its own commit. :687–731 PrepareRecordUpdate predicts
a record update, including a possible completed leaf cancellation,
actor/entity pose writes and native static provenance. If the clip
is absent the provider releases the record without a child frame
transaction. With a clip it enters a child frame transaction.
g_cod1_actor_scripted_record.inc :188–251 uses ActorNormalUpdateCurrent
as that child callback, not the direct outer callback/context.
g_cod1_actor_scripted_frame.inc :28–34 arms a distinct clock-select
child grant. g_cod1_actor_scripted_controller.inc :41–120 enforces
single-use exact parent/child admission; controller animation has a
separate owner in g_cod1_actor_controller_animation.inc.

g_cod1_actor_face_frame.inc :267–303 independently admits a lease,
captures state, calls current and compares the full snapshot before
publication. :366–388 consumes accepted notification receipts in
the actual live frame before the next callback. A failed final commit
cannot roll back those receipts, and poison must not touch a replacement
owner. A shared live pointer or a revision-only comparison would not
preserve these guarantees. Controller expected and child mutable stage
cannot simply become the same writable object.

Next candidate for R45D2: test the actual parent/child provider gate,
controller scope/current and expected-match/rebase seams with controlled
mutations. Then consider a scoped immutable parent baseline borrow
only if the child captured owner and all defined baseline bytes match,
the parent remains protected and alive until child leave, and cleanup
distinguishes owned from borrowed storage. Keep independent expected
result and all callbacks until equivalence is demonstrated. No such
borrow is implemented here. Do not treat this as a performance gain.

## Actual extracted-function test

Current and R40 bodies of ActorFaceFrameFence, Acquire, Enter, Leave,
Commit and Poison/Drain are byte-identical in the selected ranges.
The fixture runs those functions, not a reimplemented transaction loop,
against old and new storage implementations separately. It extracts
the core between Fence and FaceClockSelect and the suffix from Poison
to EOF. The shell/clone implementations are R40 storage35 and original
three-array pool versus R45B workspace45 and R45C clone_storage45.
Hash and FNV1a checks on locally reconstructed, connector-matched text:

- core: 4454 ASCII bytes, FNV1a b81541f3, Git blob
  ca9944da0a41e954ad7599062a799125e9d6b4bc.
- drain: 1910 ASCII bytes, FNV1a 82976da6, Git blob
  bee28e3d168aadf8cff3f3fa62250633ceb22345.
- R40 pool: 34a88b8984e4e94338374f5ecb88f46a66cb9762.
- R40 shell: 359c497c7466bb55d93f1e619030120a4d35b24f.
- R45B shell: 24c64f8148effcf7003a2a64a9a21c7a08a23505.
- R45C clone header: b6c3aca06d6134f9272035e857b597dccb17df8b.
- exact executed fixture: a23dc77a39df91d5fac30e65a85853ec893a41a3.

76 asserted scenarios per implementation, each run in strict GCC and
combined ASan/UBSan. Entry (read/write) and commit callback refusal,
mutations to live node/notification/name/header/frame/actor/entity,
borrowed owner, lease, epochs, resource revisions/readiness, timing,
spawn/restart, cancellation and modeled reset/replacement. Also
successful commit, busy reentry (including provider entrance count),
provider refusal, cold shell allocation failure, readonly acquisition,
and closure failure. Actual Drain loop: refusal after first accepted
receipt, refusal before second delivery, refusal at final commit with
both receipts consumed, packet mutation with accept/refuse, external
live mutation, replacement owner not poisoned, incorrect generation.
All four executions printed 76 scenarios PASS; tracked allocations
were zero after each scenario cleanup. Same behavioral assertions
passed in both implementations, not an exhaustive byte-for-byte replay
of the entire game.

On a checkout containing R40 history:

    sh tools/tests/run_opt_r45d_face_callback_test.sh

Driver syntax checked locally; full checkout driver not executed.
Local compiler runs used the exact verified extracted fragments and
headers with GCC C99 -O2 -Wall -Wextra -Werror
-Wno-misleading-indentation (existing compact source), then -O1
-g -fsanitize=address,undefined -fno-omit-frame-pointer -no-pie.

## Limits

Synthetic actor, entity, note and lease types; 2078-node array and
actual storage headers. Provider admission, lease, readiness,
closure, overlap, names equality, external VM and reset effects are
controlled fixtures or simplified predicates; actual controller gate,
GSC runtime, native reset and full map lifecycle are not executed.
This does not prove real MSVC x86 ABI or callback behavior, or cover
all failure allocations. Full GAME DLL not compiled. No FPS timing.
Hunk 256 MiB unchanged. R46 course clocks, R47 CFG-free campaign
startup and R48–R52 still required. No intermediate gameplay requested.
