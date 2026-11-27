# OPT R44C — ordinary immutable root snapshot; reserved MG overlay

Base R44B2 693c1e49ea3ef68ab8c18afdd9c61de44367a298.
Executable reference R40 8f92680c078b717f61aa762ef9a5c93e10eb99cb.
Reviewed R43A/B1/B2A and R44A/B1/B2 handoffs and R44B2 diff.
This is a bounded change of root-query representation, not a general
FaceFrame transaction rewrite, not completion of all R44.

## Ownership and change

- src/game/gsc_host/g_cod1_actor_current_root.inc (base lines 10–18,
  48–88, 89–103, 104–150): the root query previously copied the
  authoritative generic node snapshot to a second full writable array
  for every call. Only the mounted actor path actually modified it,
  overlaying two 25-node machine-gun trees. Keep the immutable original
  snapshot as the ordinary read-only projection, materialize the private
  working overlay only for a mounted actor. ActorCurrentRootPrepareNodes
  retains all three mounted refusal conditions and the same overlay
  assignment order. ActorCurrentRootNodes returns a const view of the
  chosen stage-owned array; use it for the original selected leaf gate,
  C1AGT14_Valid and the recursive collection. No pointer to live actor
  storage is borrowed across current callbacks. The original snapshot
  remains unchanged and is still compared byte-for-byte to live nodes.
- In the unchanged initial capture, ActorGenericMGMap
  (generic_host.inc :257–276) validates the actual authored aim/fire and
  parent hierarchy; ActorGenericRootMGCurrent (:770–776) compares the
  saved turret/animation record and verifies time and live ownership.
  This check still precedes overlay preparation and the first callback.
  Keep actor snapshot, lease, entity, resources and MG fences, busy and
  cancelled transitions, both current callbacks, second leaf resource
  resolution and final output publication in their original order.
  No native node identity or new event timing is inferred.
- Existing whole-stage overlap guard in ActorGenericPrivate
  (generic_host.inc :130–148) still covers both arrays and the fallback
  scratch through cod1GenericOperation.stage/bytes. Reset/retire
  cancellation and cleanup unchanged. The change only delays private
  overlay writes until after mount checks; no observable output is
  published by a failed call. Stage is still calloc'd once at the same
  point and freed on all original paths. No added allocation failure,
  persistent buffer or changed Hunk requirement.
- Source-tree collection and snapshot/overlay preparation in R40 and
  R44B1 compared directly: old collect and capture through selected-leaf
  gate are byte-identical. R44B2 had already introduced prepared topology
  with old builder fallback. Preserve cache identity and resource-read
  order. The generic frequency topology in
  qcommon/cod1_actor_generic_frequency14.h :9–35 is a distinct validated
  ascending-native-order structure, built once in locomotion_resources
  :137–146. Do not equate its validation or order with source traversal.
  It is not a repeated hot builder and is left unchanged.

The actual node layout in cod1_actor_tree_clock14.h :9 is 32 bytes on
the target ABI. For 2078 nodes this removes one explicit 66,496-byte
copy (64.94 KiB) on each ordinary root query. Mounted queries still
copy and overlay exactly as before. R44B2 already eliminated the
24,940-byte source-order copy on cache hits. These are structural
copy sizes, not measured bandwidth, allocation reduction or FPS gains.
The stage still reserves and zero-initializes the second array and
fallback scratch: do not introduce a second allocator refusal or change
stage alias protection to reduce nominal size. A later workspace
migration must prove ownership and failure/reentry behavior before
changing this capacity or reusing storage across transactions.

## Validation

Extended the existing reproducible R44B2 fixture and driver. Tested
actual extracted old order/collect and new order/prepare/view/project/
collect functions, with the production prepared/source-order/definition
headers. Direct baselines R44B1 AND R40; R40 uses its actual recursive
source-order builder. Synthetic actor/turret structures and deterministic
resource resolver; full 32-byte node layout and all fields filled.
120 generated 2078-node catalogs, 2640 ordinary + 2640 mounted
comparisons per baseline, 3840 cache hits, 1440 misses, 2354 successful
projections and 2926 matching refusals, plus three rejected mounted
admissions (missing gun, unused animation, no selected animation).
Compare full projected arrays, source order, resource access trace,
return status, original snapshot bytes, selected node bytes, cache
bytes, scratch-on-miss and untouched scratch-on-hit. Check the
unmounted overlay stays byte-for-byte untouched and mounted substitutions
match all 50 fixture indices. No assertion that synthetic mount data
represents actual MG resources.

Both baselines passed GCC C99 -O2 -Wall -Wextra -Werror and combined
AddressSanitizer/UndefinedBehaviorSanitizer (-O1, -no-pie).
Local extracted fragments checked against pinned connector/candidate
source: R44B1 renamed 2390 ASCII bytes/FNV1a 32d051e4; R40 renamed
3170/59797be3; candidate 4123/fb842ba1. Exact tested fixture blob
0853d4e9c2b5b1f912bdc46f385c906f142f24d1. Headers match pinned
4444df5adb7ea677ca1c37b7f75f0dd8f3d1b31b,
eced81fdde56da35dd6604cc8db5f393aec9f829,
e94882143eca76588a0676e0480e841a1f1ed0f1.
Driver shell syntax checked; same compile invocations executed locally
with extracted fragments, not a complete checkout. On checkout:

    sh tools/tests/run_opt_r44b2_root_projection_test.sh

Native GAME DLL build, MSVC x86, real MG binding/resource sampling,
callback/reset/reentry and real allocator failure integration have not
been dynamically tested. Whole-query behavior beyond the extracted
kernel has been reviewed by source only. No timing or FPS claim.

## Migration gate / next

R44C separates the common read-only state from the conditional overlay.
Cold scratch still reserved; do not move it to per-actor storage or
global shared scratch just to reduce allocations. Generic frequency
topology is already prepared, not interchangeable with source order.
Next R44/R45 boundary: design transaction workspace admission using
actual clone pool and FaceFrame storage (generic_host.inc :48–81,
130–174; face_frame.inc :251–300), all externally visible ranges,
retirement, controller baseline and stage ownership, accepted notify
commit/poison semantics. Do not replace byte comparisons by a revision
number until all direct mutations and pending-event paths are covered.
Defer broader hot/cold mutable state representation until these
invariants and native parity are established. R47 CFG-free startup
and R51/R52 integration remain mandatory, no user gameplay check now.
