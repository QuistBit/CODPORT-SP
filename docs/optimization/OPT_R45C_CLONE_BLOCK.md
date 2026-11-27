# OPT R45C — indivisible AnimTree allocation blocks

Base R45B efaba1d12237dfd625cb6795ddb584ae8975bda7.
Control R40 8f92680c078b717f61aa762ef9a5c93e10eb99cb.
Read R45A/R45B ownership handoffs and current HEAD. This changes
the representation of owned generic records, not their logical contents
or the number of independent snapshots. R45 remains open.

## Ownership audit (base source)

- g_cod1_actor_generic_host.inc :48–81 owns the two-entry idle triple
  pool, allocation and release of clones. :239–251 creates fresh
  records from three separate allocations. These are the two producers
  of owned triples found in the inspected actor/turret host sources.
  :130–148 protects all three arrays including unused name capacity;
  :156–173 retires live owners, cancels busy work, and clears idle
  capacity. :198–237 uses separate catalog-building scratch (unchanged).
  :361–475 and :654–673 publish a complete stage and null every donor
  pointer. Other temporary node arrays (:561–592, :697–831) are
  separately allocated projections; they are not owned triples.
- g_cod1_actor_face_frame.inc :284–302 clones stage/baseline, releases
  them on rejection/leave, and publishes all three stage pointers on
  commit. The R45B shell owns neither committed actor memory nor idle
  clone blocks. Reset/callback/receipt handling unchanged.
- g_cod1_actor_scripted_controller.inc :207, :277–295, :418–455 owns
  baseline and expected clones. ExpectedMatches substitutes all three
  pointer values in its temporary header before comparing bytes.
  ExpectedRebase releases its old independent clone, transfers all
  three pointers from expected and nulls the donor. The record layout
  and pointer substitution remain unchanged.
- g_cod1_actor_scripted_prefix.inc :209,247 clones and releases its own
  record. g_cod1_actor_face_tags.inc :215 frees nodesBefore, a different
  standalone array. Do not route that memory through the triple owner.
- g_cod1_actor_notify_names.inc :27–33 compares only the defined name
  prefix and guards the full capacity. Do not copy uninitialized names.
  Existing pointer-based identity checks remain as written.

Inspected the relevant generic/face/controller/prefix, actor resource,
lifecycle, query, locomotion and turret host files using the pinned
connector source, including the inclusion list in turret_host.inc.
No additional separate allocator or free of an owned triple member
was found in that inspected set. This is a source audit, not a full
repository-wide static analyzer or integrated native execution.

## Implementation

Move the owner functions into g_cod1_actor_clone_storage45.h.
Allocate a single typed ActorCloneBlock45 with nodes first, then notify,
then names; retain the three existing C1GARecord pointers and two idle
slots. All owning producers are migrated together. Nodes is the
allocation base; release frees only the base when not pooled.
No independent member is ever freed. Each new checkout on a cold
miss either supplies all three pointers or fails with an empty triple.
The previous partial-allocation failure states are no longer produced;
there is no attempt to reinterpret a legacy independently allocated
triple as a new block. Fresh still does not take an idle slot, clears
the full node and notify arrays, and leaves the name tail unspecified;
the original rate/cursor/identity initialization follows unchanged.
Clone still copies all node/notify bytes and only the defined name
prefix. Preserve all input and prefix-count checks. Existing pool
ordering, release/transfer paths, publication, and reset semantics
unchanged. No new cap on simultaneous owners. Checked-out blocks
survive idle clear, and late unwind can repopulate idle capacity.

Compile-time layout check requires exact adjacency and no external
padding; existing range checks thus cover the entire block. No
C1GARecord layout/ABI change, no borrowed live arrays across callbacks,
no revision-only checks, no physics, clock, event or feature changes.
The diagnostic __LINE__ values in the generic host shift after the
extraction; refusal conditions and return values are unchanged.
A single larger allocation may succeed/fail differently under real
allocator exhaustion or fragmentation than three smaller ones.
No additional retained idle slots. Hunk is not changed.

## Reproducible validation

Old allocator fragment from R45B and R40 is byte-identical (1766 bytes,
FNV1a 50469be1, git blob 34a88b8984e4e94338374f5ecb88f46a66cb9762).
Exact new header git blob b6c3aca06d6134f9272035e857b597dccb17df8b;
fixture blob d1ff7d609721cfcfba0f3899e5281dad6ad0d3af.

Local GCC C99 -O2 -Wall -Wextra -Werror and ASan/UBSan (-O1 -no-pie)
both ran successfully against the verified extracted reference.
Synthetic record/lease types use the node/notify/name layouts from
R45A; 128 nested four-clone transfer/reset cycles, defined byte
comparisons (including fresh zero initialization), full and partial
name prefixes, independent live/stage/baseline/expected allocations,
donor clearing, simulated commit/rebase, idle clear with active blocks,
pool hits with pending allocation failure, invalid admissions, three
old partial allocation failures and new cold/fresh allocation failures,
repeated clear and final zero live allocations. Four cold clones:
12 malloc calls in the old reference, 4 in the new header. Block size
266008 bytes in this host fixture, not measured MSVC x86 ABI. This
is an allocation count, not evidence of FPS or timing improvements.

On a checkout containing R40 history:
    sh tools/tests/run_opt_r45c_clone_block_test.sh

The R45A historical driver is pinned to R45A and R40 so its allocator
baseline test still runs after the representation change. Local test
invocations used the extracted reference, not a full checkout or
the full driver. Native GAME DLL, actual FaceFrame fences/commit/drain,
controller callbacks, cancellation/reentry, allocator fragmentation,
and real gameplay remain untested. Do not mark R45 as complete.

## Next

R45D: examine independent controller expected-state computation versus
FaceFrame stage ownership and prepare actual callback/fence/receipt
differential harness. Do not replace or remove independent prediction
or full byte checks without an equivalent trusted publication design.
R46 course clock costs, R47 CFG-free normal startup and later stages
remain mandatory. No intermediate user game test requested.
