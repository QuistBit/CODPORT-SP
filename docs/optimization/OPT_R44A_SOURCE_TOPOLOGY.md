# OPT R43B2B / R44A — catalog lifetime boundary and pure source topology

Base 36d827a0cf137e125cb222423d3c929b7bdc75e2 (R43B2A).
Compare R40 8f92680c078b717f61aa762ef9a5c93e10eb99cb..base:
3 forward / 0 behind, solely R43A/B1/B2A docs. Read these and R37–R40
handoffs. R40 remains executable control. This closes the narrow pure
topology extraction gate, not a proof of all mutable writers or a
complete R44. No cache publication or hot-state representation change yet.

## Lifetime and ownership (paths relative to src/, ranges at base)

- game/gsc_host/g_cod1_entity_host.c:213–244 and :251–275:
  both map begin and end run cleanup (including callbacks), compare
  previous epoch/active, then ActorPoolReset and TurretReset. The old
  map epoch and active flag are still present during reset; begin
  increments epoch only afterward and never wraps. End clears ready
  and active without increment. Therefore epoch equality alone is NOT
  a sufficient catalog certificate during teardown.
- game/gsc_host/g_cod1_turret_host.inc:30–42: private storage umbrella
  includes ActorGenericPrivate, TurretAnimationPrivate and resource
  banks. Reset orders ActorGenericReset before TurretAnimationReset.
  generic_host.inc:126–167 covers live definitions/names, epoch/receipt,
  active stage and pool, delegates locomotion and face checks; resets
  locomotion/face, cancels in-flight generic work, clears records/pool
  and definitions/names/epoch/receipt. A future derived cache must be
  invalidated explicitly at reset, not just wait for epoch increment.
- game/gsc_host/g_cod1_actor_goal.inc:4–15 -> entity_host.c:923–940 ->
  turret_host.inc:30–35 -> generic_host.inc:126–149:
  caller-owned input/output/error ranges are checked through common
  private-storage umbrella. A future static prepared topology and any
  staged allocation must join this protection; a derived representation
  does not automatically inherit coverage of the original arrays.
- game/gsc_host/g_cod1_turret_animation_host.inc:18–70,85–138:
  ATR source and reference allocations, sealed/watch/exact comparison
  fallback, distinct bank readiness and monotonic receipt. Reset
  cancels loader, frees ATR, clears receipt/epoch but not serial.
  Load refuses exhausted serial, publishes only after staged parse
  and epoch/spawning checks. Do not remove TurretAnimationBanks or
  treat source-byte integrity as implied by the derived topology.
- generic_host.inc:191–228: current catalog fast path checks epoch,
  receipt and banks; build parses both definitions/names on stage,
  validates nodes/topology, then publishes arrays and receipt.
  Only selected writer/reset paths establish this boundary; no raw
  public definition pointer is introduced by this change.
- game/gsc_host/g_cod1_actor_registry.inc:306–337,387–433:
  ActorRetireActor retires controller/fresh/generic/mount before
  clearing actor/sentient binding; ActorRetireEntity checks owner;
  provisional reservation failure also invokes retirement. Generic
  retirement (generic_host.inc:151–157) cancels active generic op
  and frees only matching actor, not map catalog. A map cache must not
  be made per-actor or use actor index as catalog identity.
- game/gsc_host/g_cod1_actor_locomotion_resources.inc:16–44,137–170:
  separate resource bank reset advances revision and preserves busy
  loader ownership, private-range guards include live and stage
  topology/leaf allocations. Load uses ActorCurrentRootOrder then
  C1AGF14_Build before clip-loading callbacks, checks resource epoch
  and revision and publishes bank. Catalog topology cannot authorize
  metadata dereferences after resource bank reset.
- game/gsc_host/g_cod1_actor_current_root.inc:15–40,89–149:
  old root builder reads static defs, writes only per-call first/next/
  order/frontier. Current callback is before Fence/Valid/Order/Collect,
  and again before final Fence/leaf-resource revalidation/output.
  Keep all these checks and order. The pure interval is the only
  initially approved reuse boundary; no saved pointer across callback.
- game/gsc_host/g_cod1_actor_general_root_interval.inc:28–35 and
  qcommon/cod1_actor_general_root_interval14.h:13–30:
  distinct general projection uses an ascending source loop for
  children (despite header comment about native descending order).
  Do NOT replace that loop with descending sibling traversal in the
  name of unification; this could alter actual output and FP order.
- qcommon/cod1_actor_generic_frequency14.h:13–35: separate topology
  builder supports forward parents, validates permutation, child count,
  bounded ancestry, sorts siblings by native order. Its permutation
  check is quadratic; this is a startup candidate, not evidence that
  it causes ongoing course peaks. Public validation stays unchanged.

## R44A: first production separation, no algorithmic shortcut

Move exact c1agt14Definition_t declaration to
qcommon/cod1_actor_definition14.h (same fields/order/ABI), include it
from the existing generic tree header. Extract ActorCurrentRootOrder
and recursive visit into qcommon/cod1_actor_source_order14.h, a
callback-free builder with explicit immutable definitions/count and
caller-owned disjoint scratch arrays, no actor/resource/global state.
Root wrapper invokes this on the same arrays; locomotion load continues
through the wrapper. Preserve head insertion, contiguous immediate
child native index blocks before recursion, depth bound 64,
parent-before-child restriction, child count and frontier checks,
including partial scratch writes on refusal. Added defensive null/count
guards for the new private helper (1..4096); production always passes
2078 and disjoint non-null arrays. No floating point operations changed.
No allocations added; no persistent cache yet; no live mutable state
representation change. Existing output fences and failure paths remain.

For later R44B: build derived source topology on actual validated
catalog stage and publish only with live map epoch AND ATR receipt;
explicit validity flag must retire before reset. If preparation fails
retain reference builder/failure behavior; no new map admission failure.
First integrate only callback-free consumption into root and bank
builder. Retain all generic tree validations, separate general interval
order, mutable row and metadata guards. Add tests for reset within
callback, map end without epoch increment, wrong receipt, same actor
index new generation, missing cache, overlap with live/stage cache.
Do not collapse metadata or VM ownership into a catalog certificate.
Then reduce per-call topology scratch allocation, and consider SoA only
after R45 mutation coverage. The complete all-writer proof and
renderer/VM integration audits remain open at their respective stages.

## Validation

Local strict GCC and ASan/UBSan runs of the actual new two headers
and standalone differential fixture passed:
424 success + 1605 refusal = 2029 old/new comparisons; 7 separate
argument guards. Generated and fixed trees of 1..4096 nodes including
2078, star, chain, balanced, random parent-first, depth 64/65,
forward/self parents, malformed root and child count, UINT_MAX
children, exact native child block example. Compare full scratch
bytes (including unused tail and partial failure output), input
immutability, return code; independently check permutation,
descending sibling order, consecutive child blocks and child counts.
Reference functions copied exactly from pinned root include: length
1257, FNV-1a fd40bd9f in source and local fixture. Fixture replaces
unrelated host structure fields and count with testCount.
New definition blob e94882143eca76588a0676e0480e841a1f1ed0f1,
builder blob eced81fdde56da35dd6604cc8db5f393aec9f829,
fixture blob 0c7dd85ba21380dbcfbcaed26c9e4911138b02dd.
Reproduce from repository root:

    cc -std=c99 -O2 -Wall -Wextra -Werror tools/tests/opt_r44a_source_order_test.c -o /tmp/r44a && /tmp/r44a
    cc -std=c99 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -fno-omit-frame-pointer -no-pie tools/tests/opt_r44a_source_order_test.c -o /tmp/r44asan && /tmp/r44asan

This is an isolated integer kernel test, not a linked GAME DLL, native
MSVC x86 build, game/renderer integration, actual ATR replay, or
performance measurement. No FPS or tick improvement claimed. Hunk
256 MiB unchanged. No user gameplay test required at this step.
R47 CFG-free normal campaign requirement remains mandatory per B2A
and user plan; no startup changes here. Next R44B prepared immutable
catalog topology publication and reset tests; do not close R44 yet.
