# OPT R43A — server animation ownership and migration design

Reference: R40 `8f92680c078b717f61aa762ef9a5c93e10eb99cb`, tree `7957bc0d945c6485106213a7c6a2bf15cc7e9517`.
This is the first bounded R43 audit, not a completed R43–R52 migration.
R36..R40 compare: four forward commits, no divergence. Read R35 and
R37–R40 handoffs. R37 active paths, R38 sparse-name cleanup, R39 queue
admission and R40 recyclable resource pool remain the reference behavior.
No production changes in R43A.

## Motivation and measurement boundary

User-supplied R40 DroidDeck report: descent GAME mean/max 39.02/101.83 ms,
course 29.82/87.05 ms; resource frame peaks up to 23.76 ms; initial GSC
call 6681 ms; Hunk peak 118.53 of 256 MiB, zero warnings. Executable
SHA and native Steam Deck repeat are not established. Do not equate
DroidDeck and native x86, or tick active-slice duration and presentation
frame latency. R35 timings are inclusive, exclude pauses between slices,
and include preemption. Do not sum parent and child timings. No claim
that any particular line explains the measured peak, or of FPS gain.
Hunk capacity stays 256 MiB.

## Source inventory and lifetime domains (all paths at R40)

Paths below are relative to src/; line ranges are for the reference only.

- game/gsc_host/g_cod1_actor_generic_host.inc:14–21,43–88:
  C1GA_COUNT=2078, C1GARecord owns nodes, notify and names allocations.
  Catalog definitions and names already shared, not per actor.
  ActorClonePool retains at most two complete released triples. Checkout
  overwrites nodes and notify in full and initialized names prefix only.
  Pooling does not remove copies. Do not introduce a second cache of
  mutable state without tracking its unique owner and retirement.
- Same file:126–167: ActorGenericPrivate checks catalog, all actor-owned
  allocations, full name capacity, pool entries, active operation storage,
  resource extents and delegated private ranges. Retire/reset cancel the
  active operation before releasing owned state; reset clears the pool
  and catalog, advances resource revision. A leased temporary is not a
  pool entry and must remain owned until its unwinding path.
- Same file:194–241: ActorGenericBuild stages parsed ATR definitions,
  verifies source ordinal completeness, parent chains, branch flags and
  sync-mode consistency, validates default nodes, then publishes under
  epoch/receipt/cancellation checks. Fresh actor has rate=1, cursor=-1,
  independent notify owner identity. Shared definition data is currently
  immutable between publication/reset by convention plus overlap guards,
  not a capability that arbitrary raw-pointer consumers can assume.
- game/gsc_host/g_cod1_actor_notify_names.inc:1–33: sorted initialized
  name prefix, empty-name reads return shared 64 zero bytes; equality
  compares prefix only, alias protection covers entire reserved allocation.
  NameCount and the original 64-byte text representation are semantic.
- game/gsc_host/g_cod1_actor_face_frame.inc:251–301:
  FaceFrameCall owns saved, stage and baseline record views, frame copy,
  actor/entity snapshots, borrowed owner pointer, epoch and revisions.
  Acquire checks provider, private/overlap, owner, mount, closure and
  selected-frame identity; clones stage (if writable) and baseline;
  sets busy/private ranges; calls current(ctx); then checks Fence.
  Fence verifies cancellation, time, map/generic epochs, face/scripted
  resource revisions, borrowed owner, current lease, full saved record,
  all live node and notify bytes, initialized name prefix, frame,
  actor and entity bytes. Commit calls current again, repeats Fence and
  checks stage closure before moving stage ownership into live record.
  Old live state is released; stage pointers are nulled before Leave.
  Leave disposes stage/baseline and clears active ranges/busy.
- game/gsc_host/g_cod1_face_frame_storage35.h:3–18: one malloc for call
  and actor/entity snapshots (R35 already reduced three to one).
- game/gsc_host/g_cod1_actor_face_frame.inc:307–362: Begin checks actual
  frame cadence and advances weights once; Step opens a new transaction,
  dispatches general/scripted/face clocks; returns resume after commit.
  A resumption uses the original full 50 ms dt, not an invented remaining
  dt. Map settling cadence may have 100/150 ms wall game-time gaps.
- Same file:364–386: Drain is an external-effect boundary, NOT an
  ordinary rollbackable batch. Before delivery current and Fence run;
  afterward packet integrity and Fence are checked. After accepted
  delivery, live nextNote and cleared note slot are updated immediately,
  and baseline frame is updated to match. Never redeliver accepted
  receipts after later refusal. Poison only matching owner/frame identity
  on ambiguous accepted effect or invalid callback result. Final Commit
  still swaps the staged record; do not assume Drain is read-only.
- game/gsc_host/g_cod1_actor_generic_host.inc:340–475: distinct generic
  mutation path: copy borrowed request and actor/entity; current callback
  before clone, stage existing or fresh state, advance legacy clock if
  selected-clock admission permits, mutate, validate active resource
  contracts, second callback/fence; turret MG commit and face admission
  precede final ownership transfer. Unlike FaceFrame, final old-tree
  check here is revision-based; do not silently substitute this weaker
  fence for FaceFrame. Include legacy/mounted and fresh/retired cases
  in migration. Other direct mutation and readout paths are not yet
  fully inventoried (R43B).
- game/gsc_host/g_cod1_actor_locomotion_resources.inc:1–44,137–170:
  C1LocoResources owns topology, frequency and metadata pointer table,
  root/body/metadata allocations; load stages and validates, publishes
  under epoch/revision/busy fence. Reset frees live resources, advances
  revision, preserves busy so staging loader unwinds itself. R44 topology
  plan must account for both generic catalog and resource bank lifetime.
- Same file:185–266 and face_frame.inc:185–250: closure selects general
  or specialized clock, validates all nodes and pending/token/resource
  constraints. Four-dormant SIMD paths still inspect mutable bytes, with
  exceptional FP encodings falling back. LocomotionFrameAdvance checks
  closure again, allocates a step event buffer, calls AdvanceDisposable,
  checks produced events and fills ordered frame notes before commit.
- qcommon/cod1_actor_generic_tree14.h:9–74,85–159: topology proof cache
  uses exact bytes (MSVC x86 SSE2); scalar path validates all parent
  chains, child counts, node fields. Clear and SetAnim each allocate and
  copy the whole node array, then commit or discard. Clear uses R37
  transaction-local exact activity mask, not a general persistent index.
- qcommon/cod1_actor_frame_weights14.h:9–34,52–87: private weight path
  already avoids nested clone for parent-first catalogs and retains
  atomic fallback for forward parents. Inactive-parent pruning snaps
  weights to goals but does not update old phase/cycle. Preserve this
  distinction, even on an inactive leaf.
- qcommon/cod1_actor_generic_flagged14.h:31–76 and
  qcommon/cod1_actor_ancestor_knob14.h:10–13: generic flagged mutation
  allocates/copies nodes and flags; ancestor selection may invoke further
  transactional knob operations. Count/ownership proof needed for each
  nested helper, not a global switch to in-place writes.
- qcommon/cod1_actor_generic_clock14.h:118–177: public atomic and
  disposable paths already share math. Storage validates disjoint
  ranges including each non-null metadata object, then Inner validates
  tree, frequency, flag/cursor/descendant, live-leaf and named metadata
  contracts, searches earliest event, advances in native order. Only
  disposable entry may partially write on refusal. A full table scan
  exists even when few leaves are active.
- qcommon/cod1_actor_generic_frequency14.h:9–17: topology stores
  first/next/order in native definition order; not equivalent to sorting
  children by source ordinal. Preserve tie order in flagged selection
  (generic_flagged14.h:5–15) and ordered event traversal.
- cgame/cg_cod1_face_unmounted_actor.inc and cg_cod1_face_resource40.inc:
  snapshot lease, revision, owner prefix, composition and renderer
  registration remain separate from server tree ownership. R40 bounded
  FIFO retains decoded resources, not ownership of current actor.
  Resource design audit deferred to later steps.

## Structural traffic, not a benchmark

Source layouts: qcommon/cod1_actor_tree_clock14.h:9 (32-byte node),
qcommon/cod1_actor_flagged_leaf14.h:7 (8-byte flag); generic_host.inc:15–16
(64-bit generation/pending, 32-bit pendingTick, 68-byte name entry).
For a writable FaceFrame acquire:
- 2078*32 = 66,496 node bytes per clone;
- 2078*(8+8+8+4) = 58,184 bytes of per-node notify arrays per clone,
  plus 20 bytes scalar fields and ABI padding;
- two clones therefore copy at least 249,360 bytes (about 243.5 KiB)
  excluding notify scalar fields, initialized name prefix and snapshots;
- each successful fence compares another complete live/baseline nodes
  and notify arrays (at least 124,680 logical bytes, each side read).
  Acquire and commit both fence; each delivered note adds fences.
- name capacity is 2078*68=141,304 bytes per triple under the stated
  32-bit int layout, not all initialized nor all copied.
These are source-derived byte lower bounds, not cache misses, measured
memory traffic, CPU time, or a recommendation to change Hunk.

## Proposed architecture and migration gates

A. Map-owned immutable catalog view: checked parent/child order, depth,
   subtree membership/range index, static classification and resource
   metadata references. Build and validate once at actual publication;
   canonical ordinals unchanged, support forward-parent trees. Keep
   native ordering separately from ordinal ordering. A pointer + epoch
   alone is not proof: catalog publication, reset, all writers and
   private-range exclusions must establish lifetime and non-aliasing
   first. Use existing dense reference API at unproven boundaries.
   R44 first implementation should be an additive prepared topology
   with differential membership/order tests; do not start with a
   wholesale sparse rewrite of nodes.

B. Per-actor mutable storage with explicit live/staged/baseline roles:
   dense reference projection initially, separate hot phase/weight/flags
   from cold name/identity data only after every reader/writer mapped.
   A future sparse index or SoA must represent *all* legal inactive
   states (including phase/cycle, signed zero, nondefault rate, fades,
   token/descendant/pending and ancestor prune behavior), not merely
   currentWeight>0. Preserve full public ordinal semantics. Active lists
   are derived and invalidate with every writer, not a proof of validity.
   No hash-only replacement for exact fences.

C. Transaction context with bounded reusable buffers: acquire private
   workspace under existing busy/provider rules; keep immutable borrowed
   input snapshots and live baseline proof; produce unpublished stage;
   validate and publish with original callback/fence order. Retire/reset
   cancels operations but must not return their workspace to pool while
   on stack. Reject overlaps including idle pool, stage, baseline and
   prepared topology; retain all current protected ranges. Start by
   sharing scratch for proven no-callback private operations, then route
   nested mutators to disposable variants only where entire outer stage
   is abandoned on *every* error. Public atomic entry points unchanged.
   Full live comparisons remain until mutation coverage and callback
   isolation are proven; a dirty flag or revision alone is insufficient.

D. Clock runner: reuse prepared topology and metadata under proven
   map lifetime. Separate local numeric work from commit and external
   callback dispatch. Preserve old phase/cycle rules, exact FP wrappers,
   sync search/tie order, event packets, owner and generation, full-dt
   resumption, accepted receipt consumption and poisoning. Do not merge
   Begin/Step/Drain across callback boundaries as a shortcut.

E. VM/resource/client/renderer: audit dispatch and time-root claims,
   staged asset loading and snapshot lease fences, and renderer backend
   lifetimes before changes. No worker thread may publish state, invoke
   VM or call arbitrary engine callbacks; only disjoint pure computations
   on stable inputs after lifetime/ordering proof. Keep serial fallback.

## Verification contract for implementation stages

For each stage pin R40 and new SHA; exercise actual production functions
with test fixtures, not a second hand-written algorithm alone. Compare
return values, every defined node/flag/notify/name/packet byte, tick,
revision, lease identity, event order and delivery count. Compare output
and live state on refusal; on disposable paths compare only successful
results plus proof that caller discards the entire partial stage. Include
allocation failures and callback refusal/mutation/reset/retire/reentry,
owner reuse and revision exhaustion, stale borrowed inputs, aliasing of
all storage classes, zero/negative-zero/subnormal/NaN/out-of-range values,
forward parents, invalid/cyclic topology, branch order/ties, inactive
ancestor and sync/notify transitions, partial drain with accepted effects,
mounted/legacy and ordinary actors. Native MSVC x86 FP environment checks
are separate from scalar GCC/ASan/UBSan tests. Preserve same FP operation
order wherever it affects observable behavior.

Use per-operation counters for clones, bytes, validations, metadata
walks and event resumes plus opt-in accumulated active durations, not
per-call log spam. Compare stage costs before/after under identical
workload. Native full Training and presentation P95/P99/1% low remain
integration gates, not required from user during R43–R50.

## Status / next bounded step

R43A complete: server ownership/transaction source audit and design.
Documentation-only, no native build, test harness, sanitizer or FPS
measurement executed in this step. Previous fixture results are historical
from R35/R37–R40, not rerun here. Source paths and reference revision
checked against GitHub; arithmetic is a static estimate.
R43 is still open. R43B: inventory all mutable accessors/callback callers
and course runner/Foley call chain, especially ActorControllerProviderEnter,
ActorGeneric read/direct writes, root publication, and VM resumption;
identify any raw-pointer escapes and exact native child-order inputs.
Document lifetime proof gaps and the first prepared-topology migration
boundary before R44. R43C if needed: verify CGame/renderer/VM interface
ownership and reset map. No intermediate user gameplay test required.
