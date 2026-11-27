# OPT R46A — prepared metadata range admission for the general clock

Base R45D3 87c42845b121fd38e4c96f61ebe64e698df30ce3.
Control R40 8f92680c078b717f61aa762ef9a5c93e10eb99cb.
R45D3 leaves independent FaceFrame baselines in place; the
unproved sharing candidate is deferred. R46A starts the server
clock migration at the immutable resource/working-state boundary.
This is not completion of R46 or a measured FPS improvement.

## Source evidence and representation

g_cod1_actor_locomotion_resources.inc (base :1–44,137–170,249–265):
C1LocoResources owns the topology, frequency array and metadata
pointer table. Loader builds a private stage, validates each
resource, and publishes the whole bank only after the final epoch,
revision and busy checks. Reset frees resources and clears the
bank, retaining the loading lease. ActorLocomotionPrivate protects
the entire live and staged bank plus separately allocated resources.
ActorLocomotionFrameAdvance invokes the disposable clock kernel
on a private actor transaction, then materializes ordered notes.
No external callback occurs inside AdvanceStorage or Inner.

qcommon/cod1_actor_generic_clock14.h AdvanceStorage checks the
three mutable ranges against each other and definitions, topology,
pointer table, frequencies and every non-null metadata object.
For 2078 table entries, the last step performs a complete pointer
scan and up to three range comparisons per populated entry every
invocation. The resource metadata object size is constant.
qcommon/cod1_actor_named_clock14.h C1ANC14_Overlap uses unsigned
subtractions, not endpoint addition. Preserve its exact semantics.

Add a map-owned immutable index (cod1_actor_clock_ranges46.h):
copy the pointer table, sort the non-null start addresses once.
For any query starting at x, only the greatest metadata start
strictly less than x and the least start greater than or equal to
x can overlap, because all metadata objects have the same size.
This also handles duplicates and overlapping input ranges. Use the
same unsigned-subtraction comparisons, including zero-length
queries and the address-space boundaries. No metadata content or
clock state is cached by this index.

AdvanceStorageIndexed retains all existing preflight checks.
The prepared entry compares the entire current pointer table to
the captured table, bounds count/used, and checks that none of the
three mutable ranges overlaps the index itself. If not admitted
to the indexed path, the original exact per-object scan runs.
Legacy AdvanceStorage, Advance and AdvanceDisposable keep their
signatures and reference scan behavior. Only the internal GAME
locomotion clock calls AdvanceDisposablePrepared. Index preparation
is optional; it adds no explicit resource admission predicate.
It is performed on the protected staged bank before publication;
reset clears it with the bank. The table's content is still
validated by the original Inner every time, as are mutable fields.
Neither the search/advance arithmetic, x87/MXCSR setup, event
ordering, allocation/rollback logic of the clock, nor note delivery
is changed. All public call paths outside this owner use the
reference admission. The sorted derived index itself relies on the
protected synchronous map-owned lifetime, as does the existing
topology. This is not a general untrusted-proof API.

Cost tradeoff: one full pointer-table comparison plus at most three
binary searches replaces the per-entry metadata range scan when
the proof matches. Still O(count) for the table comparison; do not
describe the entire invocation as O(log n). The index reserves
4096 pointers and 4096 uintptr_t entries plus two unsigned counters
(32776 bytes on the expected 32-bit layout; native build not yet
verified). Live and staged banks are larger; an allocation under
memory pressure may differ, and sorting has a one-time load cost.
Hunk 256 MiB unchanged. No changes to actor counts, physics,
graphics, timing, callbacks, or mission/CFG admission.

## Actual local checks

Pinned R40 and base clock header both have blob
e25e8781db81061edd4d2e10feff12b6f1094f51.
Locally reconstructed reference storage suffix 2496 bytes,
FNV1a 3fba9573, blob 65a742677949fefab6cba9b24055b9601fa743af.
Tested exact candidate suffix bfa1880d891acfc168473d1c58d28f633f993704
and header 77be817203ff7852decf91f5e4ec2305dccfc8c2.
Fixture blobs 3b915fdae4219682774a56428b845030eb6f98df and
6d1cabc569ed1c8300d0886e19fdc98ba0d4f95e.

GCC C99 strict -O2 -Wall -Wextra -Werror and combined ASan/UBSan
(-O1 -g -fno-omit-frame-pointer -no-pie):
- 12929 comparisons per execution against the exact subtractive
  reference predicate. Null and duplicate entries, overlapping
  entries, 2078/4096 slots, stale pointer tables, proof alias,
  zero length, SIZE_MAX and UINTPTR_MAX boundary cases.
  Strict run: 12843 indexed, 86 fallback; sanitized: 12842 indexed,
  87 fallback (the actual proof address differs between builds).
  Initial test exposed equality-at-zero-length discrepancy; changed
  search to strict lower bound and reran both modes successfully.
- 17 actual extracted old/new storage-entry cases per execution,
  with synthetic clock context and a stub Inner. Verified refusal
  on metadata alias of each writable range, stale-proof fallback,
  legacy API, success/refusal and exact disposable/transactional
  output behavior for the stub. Both modes passed.

Driver syntax and extraction checked locally. Reproduce on checkout
containing R40 history:
    sh tools/tests/run_opt_r46a_clock_ranges_test.sh

The tests prove the address predicate and exercise the storage
entry, not the real full numerical clock or resource-loader
integration. The GAME bank wiring is reviewed by source only;
no complete checkout driver run, native MSVC x86 GAME DLL build,
real resource publication/reset, performance measurement or
gameplay replay. Do not infer FPS improvement from scan complexity.
Retain reference fallback and test real ownership/lifetime and
native ABI during R51. The index adds no per-actor state.

## Next

R46B: separate mutable clock admission from immutable resource
validation using the actual C1AGC14_Inner and ActorLocomotionClosure
call graph, preserving all node/flag checks and metadata validity.
Audit repeated full definition and metadata checks within Begin/
Step/Drain and the difference between course inclusive time and
nested VM time. Do not skip a validator merely because another
caller previously validated an object or because epoch matches.
The actual clock arithmetic remains untouched in R46A.
R47 normal CoD1 SP without training_player.cfg remains mandatory;
do not remove legacy CFG before R51/R52 differential coverage.
