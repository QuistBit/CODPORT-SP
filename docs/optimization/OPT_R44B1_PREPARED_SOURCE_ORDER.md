# OPT R44B1 — publish catalog-owned immutable native source order

Base 611467af256c1be7157315069c150577a168bede (R44A), executable baseline R40
8f92680c078b717f61aa762ef9a5c93e10eb99cb. Reviewed R37–R40, R43A/B1/B2A
and R44A handoffs and actual generic host, root and locomotion consumers.
This is the first cache-publication substep, not completion of R44.

## Implementation and invariants

- src/game/gsc_host/g_cod1_actor_generic_host.inc at base :83–87,
  :158–167, :191–228: definitions and names have map lifetime;
  admitted definitions are published only after C1AGT14_Valid and the
  epoch/ATR receipt/cancellation fence. Prepare source order on the
  same staged definitions immediately before copying them into live
  storage. The new helper is pure integer work without callbacks or
  allocation; no intervening external observation of partial publication.
  Failure leaves cache unavailable but DOES NOT change map admission.
- New src/qcommon/cod1_actor_prepared_order14.h, capacity set by the
  host to C1GA_COUNT=2078. Owns first/next/native ordinal arrays, count,
  frontier, explicit ready, epoch and receipt. No new resource pointers,
  no actor-specific data, no additional dynamic allocation. Arrays take
  24936 bytes at 32-bit int/unsigned, plus metadata/alignment. This is
  static GAME storage, not a change to the 256 MiB Hunk setting.
- Explicit retirement is first operation in ActorGenericReset, before
  locomotion/face teardown. Map end can retain its epoch; epoch alone
  cannot establish readiness. Failed prepare retires any old publication.
  ActorGenericRetire remains per-actor and does not retire the catalog.
- ActorGenericPrivate (base :126–149) now includes the entire cache
  struct, including metadata and arrays. TurretStoragePrivate routes to
  it; root caller buffers continue through the existing umbrella.
  Prepare writes directly into this protected storage; no new separately
  allocated stage to track. No raw pointer exported.
- src/game/gsc_host/g_cod1_actor_current_root.inc :16–18: wrapper uses
  copy only with active map, matching live generic and entity epochs,
  matching generic and ATR receipts, plus cache's ready/count/exact
  identity. On any miss execute the original R44A builder, retaining
  its success/refusal and partial scratch semantics. Copy on hit writes
  exactly the same three arrays and frontier as the reference builder.
  Root caller still checks C1AGT14_Valid, both current callbacks and
  ActorCurrentRootFence, resource leaf identities, TurretAnimationBanks,
  leases and mutable rows. No caller order, timing, floats, resource
  validation or event behavior changed.
- src/game/gsc_host/g_cod1_actor_locomotion_resources.inc :137–170
  also calls the wrapper before C1AGF14_Build and before resource
  callbacks. The bank's own topology, loading and lifetime fences
  remain unchanged. No use of cached data across callbacks by reference:
  current consumers still own scratch copies. No replacement of the
  general interval's different child order.

This removes repeated native-order construction on cache hits, not
the existing per-call scratch allocations, full mutable-tree validation,
root sampling, or the larger FaceFrame/controller clone/fence cost.
No hit frequency, duration or FPS measured on target hardware.
Cache reuse assumes the admitted catalog's documented private immutable
definition ownership; it is not a new general mutable-tree certificate.

## Isolated validation

Actual new header and test compiled/run under strict GCC and combined
ASan/UBSan. Builder and definition dependency hashes match pinned R44A
(eced81fdde56da35dd6604cc8db5f393aec9f829 and
e94882143eca76588a0676e0480e841a1f1ed0f1). Results both modes:
2401 exact copies, 2706 misses with untouched scratch. 300 generated
star/balanced/random parent-first catalogs up to 2078 nodes; full scratch
comparison against production R44A builder, including untouched tails;
consumer scratch mutation does not alter cache. Tests include zero
identity, mismatched epoch/receipt/count, retirement without epoch
change, republish with new receipt, failed malformed rebuild, invalid
capacity/null source and max uint64 identities. Local Git blob hashes
of tested header and fixture match repository blobs:
29124046a3765795734a8db27291db347692b67f,
29d4462c53c584bf617441837cd3fd782492e06f.

    cc -std=c99 -O2 -Wall -Wextra -Werror tools/tests/opt_r44b_prepared_order_test.c -o /tmp/r44b_test && /tmp/r44b_test
    cc -std=c99 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -fno-omit-frame-pointer -no-pie tools/tests/opt_r44b_prepared_order_test.c -o /tmp/r44b_san && /tmp/r44b_san

Host integration reviewed as exact anchored edits to pinned sources,
not compiled as a linked GAME DLL. No native MSVC x86, actual ATR replay,
callback-driven full reset, live alias rejection, actor generation
reuse or full Training test executed. These remain integration gates;
isolated tests simulate identity retirement, not an actual map reset.
No gameplay request to user at this stage.

## Next bounded step

R44B2: validate host reset/callback/alias boundaries with extracted actual
host paths, then reduce redundant scratch storage/copies by borrowing
immutable order only inside callback-free intervals (no pointers across
external calls) or use a smaller topology-only scratch for locomotion.
Measure isolated kernel costs if useful, keep actual runtime profiling
for integration. R44 not closed; R45 all-mutation/transaction proof
still required before changing mutable actor state representation.
R47 CFG-independent normal campaign acceptance remains mandatory.
