# OPT R45D2 — controller gate, callback scope and rebase regression

Base R45D1 cb856d5f495655f495dac9048ab0a40aa33101c0.
Control R40 8f92680c078b717f61aa762ef9a5c93e10eb99cb.
Read R45D1 handoff and current controller, record update and animation
source. The entire scripted_controller.inc blob b6f6db138875fee16f7bcef27a634fb179a6ccd9
is identical at both revisions. This is a bounded validation step,
not a production optimization.

## Actual ownership and candidate

g_cod1_actor_scripted_controller.inc :41–120: Arm binds owner,
time, parent kind, callback and context. Enter invokes the independent
animation-owner admission first, even before the outer busy check.
Parent grants are consumed; ArmFrame explicitly binds the child's own
callback/context; Enter consumes a single frame grant. Finish clears
the gate, and denial poisons the active owner. A parent name or actor
serial alone cannot identify a permitted child. The generic gate can
accept zero child frames if explicitly expected; real clock select
requires one, and record update requires zero only on the clip0 path.

:229–274: Identity checks the request, lease, epoch and both resource
revisions. Scope compares the actor/entity and full generic/header,
frame, record pointer/content and native-static presence/content.
Current invokes Scope before and after the foreign callback, checks
the saved operation bytes, and refuses reentry. Revision numbers do
not replace these comparisons; direct notify mutations exist.

:418–455: ExpectedBase owns a separate clone; ExpectedMatches checks
actual output against the independently prepared bytes after provider
exit. It normalizes the three allocation pointers in the expected
header, not the actual node/notify/name content. Rebase transfers the
expected allocation to the parent's baseline; actorStage=0 preserves
unpublished controller actor changes, actorStage=1 rebases them.
No extra allocation or callback follows matching before transfer.

:457–471 and :687–731: parent stays synchronous until provider returns.
g_cod1_actor_scripted_frame.inc :28–34 enters FaceFrame directly with
the parent current. g_cod1_actor_scripted_record.inc :188–251 binds
ActorNormalUpdateCurrent and its distinct context; clip0 releases
record without FaceFrame. The nonzero path commits child tree first,
then writes normal record, actor/entity pose and invalidates static
source. The child must leave before ProviderFinish/ExpectedMatches.
g_cod1_actor_controller_animation.inc :16–32,57–66,76 onward is
a separate animation owner with a one-shot frame permit and no
independent genericDeepBefore. Do not apply parent sharing to this
owner or claim coverage of the main course clock by this candidate.

R45D3 candidate: borrow only a verified immutable parent baseline
at the explicitly admitted child entry, with fallback to independent
clone for every other call. Prove exact header and defined tree
equality at the acquisition boundary, retain full child Fence and
parent Scope, track owned versus borrowed storage, protect the
borrowed ranges and ensure release cannot recycle the parent block.
Verify actual parent+child integration including nested callback
chains, reentry, reset/retire and parent unwind. In particular, a
shared baseline cannot silently weaken detection of modifications
to private snapshots themselves. Keep expected prediction independent.
Do not merge writable stages or change receipt consumption. If
equivalence cannot be proven, retain the clone and move to a
better-supported R46 hot-clock candidate rather than weakening
integrity checks.

## Tests performed

Actual extracted functions:
- gate (ProviderDeny through ProviderReset): 3995 ASCII bytes,
  FNV1a 4eef46b4, Git blob de86ec558ac8e5efd77a0be14fd0284271748772.
- scope (Identity through Current): 2993 bytes, FNV1a e17456f6,
  Git blob f361404026927cea27e5f83f1c3e35f1e3069b82.
- expected (ExpectedBase through ExpectedRebase): 2716 bytes,
  FNV1a 574165bb, Git blob 10f933244e8dd54bc261c3aa570bf7180f8223d8.

The connector ranges at both revisions match these length/FNV
fingerprints. Local reconstructed fragments were checked against them.
The two allocators are the R40 reference fragment
34a88b8984e4e94338374f5ecb88f46a66cb9762 and R45C production header
b6c3aca06d6134f9272035e857b597dccb17df8b.
Exact fixture blob 50a484fe210397d81ae9b268fe202e2f399b51e7.
Both strict GCC C99 -O2 -Wall -Wextra -Werror and combined ASan/UBSan
(-O1 -g -fno-omit-frame-pointer -no-pie) ran successfully for each
allocator. All four local executions printed 69 scenarios PASS.
Cases include both parent kinds, zero/one child grant and count
refusal, wrong owner/time/callback/context, duplicate grant use,
checking/reentry, animation admission refusal, no-parent legacy
behavior, unarmed child, foreign mutation, before/after current
mutation across all major snapshot categories, cancellation, input
and resource invalidation, correct and incorrect expected output,
simulated replacement and rebase with actorStage=0/1, busy child/
record/generic exclusion, and two allocation failure boundaries.
Every scenario verifies zero tracked allocations after cleanup.

Reproduce on checkout with R40 history:

    sh tools/tests/run_opt_r45d2_controller_test.sh

Driver shell syntax checked; local executions used extracted
fragments, not the complete checkout driver. Exact production
controller functions tested, but actual FaceFrame and controller
are not linked together in this fixture; the provider publication,
lease/readiness, animation-owner admission, private checks, names
equality, actor and record types and external callback effects are
modeled. ExpectedFree mirrors the original. Actual independent
prediction math, native reset and VM callback stack are not
executed. No real Windows GAME build, timing or FPS claim.

R45 remains open; Hunk 256 MiB unchanged. R46 clock cost and
R47 normal campaign without training_player.cfg remain mandatory.
No intermediate user gameplay test requested.
