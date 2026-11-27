# OPT R44B2 — callback-free root topology borrow; compact load scratch

Base R44B1 1aca17b54f17359218939874d94b541fa497cdb0; executable control R40
8f92680c078b717f61aa762ef9a5c93e10eb99cb. Reviewed R43A/B1/B2A,
R44A and R44B1 handoffs and the R44B1 diff. This is a bounded topology
consumer migration, not all of R44 or a FaceFrame rewrite.

## Source boundaries and implementation

- src/game/gsc_host/g_cod1_actor_current_root.inc (base :16–22,
  :40–70, :86–132): the old projection copied all three prepared
  arrays and frontier into the large actor transaction even on a hit.
  Select the same ready/count/epoch/ATR receipt identity in
  ActorCurrentRootPrepared. Pass const first/next/order to the same
  recursive projection. The new ActorCurrentRootProject obtains this
  borrow after the first current(context), complete
  ActorCurrentRootFence and C1AGT14_Valid. It returns before
  C1ARC14_Current and the second current(context); no topology view
  is stored in the stage, returned to the VM, or used across callbacks.
  Native child order, exact active cuts, depth/count bounds, leaf
  ordering, resource identity and all floating operations are unchanged.
  Fallback uses the existing scratch builder and same refusal path.
- Call graph audit: ActorCurrentRootResource (:23–39 at base) uses
  C1AL14_Index, ActorGenericShuffleIndex (generic_host.inc :256),
  ActorFaceLeafIndex/Root/Body (face_frame.inc :116–118),
  ActorFaceReady (face_host.inc :27–30), ActorScriptedIndex/Root/Body/
  Ready (scripted_resources.inc :36–39), ActorJumperReady/Valid
  (jumper_resources.inc :30–41), ActorLocomotionRoot/Body/Ready
  (locomotion_resources.inc :44,180–185) and direct MG resources.
  These are synchronous lookups and validation of existing memory,
  not GSC resumes, engine traps, resource loads or mutation callbacks.
  No borrow around ActorCurrentRootFence, which may validate other
  resource owners, or around caller current(). Keep second resource
  lookup, exact actor/lease/row comparison, and output publication
  as before. No new concurrency contract: GAME main-thread only.
- g_cod1_actor_generic_host.inc :130–174 still protects the entire
  prepared catalog and live root stage through ActorGenericPrivate.
  Per-actor retirement cancels generic operation without retiring
  catalog. ActorGenericReset retires prepared order first, then
  frees/clears banks and definitions. TurretReset (turret_host.inc
  :30–42) reaches this before ATR reset. EntityMapBegin/End
  (g_cod1_entity_host.c :213–244,251–275) can clean up before changing
  epoch; explicit ready retirement remains essential. A reset during
  either external current callback must be rejected by the existing
  cancellation/owner fence. No reset path is changed. The new view
  cannot outlive the callback-free recursive projection.
- qcommon/cod1_actor_prepared_order14.h factors the previous Copy
  predicate into C1AGO14_Matches; keep Copy's untouched-on-miss
  behavior and all publication/reset checks. No new allocations.
- g_cod1_actor_locomotion_resources.inc :137–170 used a complete
  C1ActorCurrentRoot allocation only to supply native order to
  C1AGF14_Build. Use C1ActorRootOrderScratch instead, same calloc
  timing, same two allocation refusals, same ownership, cleanup and
  independent frequency-topology builder. No change to loading
  callbacks, r->busy, cod1LocoStage, revision/epoch checks or output.
  Locomotion still copies the prepared order and builds its own
  separate validated topology before any clip callbacks.

At 32-bit int/unsigned, the root cache hit eliminates 24,940 bytes
of explicit writes (three arrays plus frontier) per projection.
This is structural traffic, NOT a measured frame-time improvement.
Root still retains and zeroes the old-size scratch in its stage to
preserve allocation timing and avoid adding an allocation/stack
failure on a cache miss. The locomotion loading scratch is now
24,940 bytes on the test ABI, instead of an entire actor snapshot
with two full node arrays and resource/pose records. Exact native
size delta not measured. No new persistent allocation, Hunk change,
actor limit, map special-case or mutable state shortcut.

## Validation

Local strict GCC -Werror and combined ASan/UBSan ran the actual
extracted old and changed projection/selection functions, compiled
with real prepared/source-order/definition headers; both gave:
2640 comparisons, 1920 hits, 720 misses, 1174 success, 1466 refusal.
120 generated 2078-node catalogs (star, balanced, random parent-first,
deep chain), sparse/full goal weights, selected leaf/composite/invalid
ordinal, missing resource stub, projection depth/count/leaf limits,
inactive map, epoch/receipt/count mismatch, retirement with same epoch,
malformed rebuild. Compare exact projection bytes, source indices,
leaf pointers/indices, return code, resource lookup order, input and
cache bytes. Assert scratch untouched on hit and exact old scratch on
miss including partial refusal. The old Copy predicate is separately
checked via the fixture's independent ready/identity expression.
The old functions are R44B1 (not direct R40); R44A/B1 documented
earlier R40 builder parity.

Local extracted old fragment 2436 ASCII bytes/FNV1a de8dc4c7, new
3344/180f08c3, verified equal to pinned GitHub input and candidate
source. Exact tested header blob 4444df5adb7ea677ca1c37b7f75f0dd8f3d1b31b;
fixture 9abe1e1261370db8065809e30f7555e3e1017650; dependency builder
eced81fdde56da35dd6604cc8db5f393aec9f829; definition
e94882143eca76588a0676e0480e841a1f1ed0f1. Shell driver syntax/extraction
tested locally on reconstructed fragments; complete checkout command
(to run on checkout with R44B1 history):

    sh tools/tests/run_opt_r44b2_root_projection_test.sh

The resource resolver is a deterministic stub, the node/host fixture
uses only fields read by the extracted functions. Actual GAME DLL,
native x86 MSVC, actual ATR/animation sampling, whole-map callback
reset and output alias integration, real allocator failures and
locomotion load integration are not exercised by this fixture.
Source audit of those boundaries is not a dynamic test. No gameplay
test or performance claim. R44 remains open.

## Next bounded step

R44C: separate cold fallback scratch from hot root stage only if
allocation/failure/cancellation boundaries can be preserved; examine
shared topology admission for generic frequency/clock without replacing
its distinct native sibling order or integrity checks. Then R45
transaction ownership and reusable buffers, requiring full direct
mutation/notify and controller expected-model proof. R47 clean
CFG-free normal campaign startup remains mandatory; no startup changes.
No intermediate user gameplay test required.
