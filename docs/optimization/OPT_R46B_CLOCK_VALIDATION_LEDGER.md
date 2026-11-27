# OPT R46B — clock admission separated from arithmetic

Base R46A d4f30558232b4232a953fd65e78be96411ff98f2.
Control R40 8f92680c078b717f61aa762ef9a5c93e10eb99cb.
Read R46A and the actual clock, metadata, tree and locomotion owner
sources. This is a bounded server clock migration, not completion
of R46 or proof of a frame-time gain.

## Ownership and validation

At the base src/qcommon/cod1_actor_generic_clock14.h :119–138,
Inner combines admission and advancement. Every row checks the live
node rate, resource frequency, descendant, cursor and ancestry.
A current weighted leaf validates its metadata; a notify token
validates the selected leaf metadata again. Multiple ancestors
can name the same leaf. C1AGM14_Valid in
cod1_actor_generic_metadata14.h validates header, count, frequency,
all defined note tokens, sorted phases, payload strings and the
implicit end record. It has no callbacks or writes. C1ATL14_Finite
is the bit-based finite predicate in cod1_actor_tree_clock14.h.
AdvanceStorageIndexed (:139–172) performs range separation before
Inner and enters its existing floating-point environment. No VM
or other owner callback occurs during Inner's admission loop.

Extract C1AGC46_ValidateClock and its invocation-local metadata
ledger. The 4096-bit (512-byte) stack ledger is keyed by source
ordinal. First use of each ordinal still performs the original
metadata validation, in original row/predicate order; only a
successful result sets its bit. Null pointers and each token's
own cursor check remain explicit. Distinct ordinals that happen
to alias a metadata address are still checked separately. All
mutable row predicates, full tree validation, ancestry checks,
frequency checks and input bounds are unchanged. No output or
actor state is written during this admission. Arithmetic resumes
with the original output initialization, Search, fraction
rounding and AdvanceNode; their function bodies are unchanged.

The ledger is discarded before clock advancement, and is never
reused on another invocation, another actor or across a VM bucket.
Changes to resource content at the same address are rechecked on
the next call. This is not a load-time validity certificate and
does not use an epoch as a substitute for byte validation.
No new allocation or retained bank memory. Preserve the original
synchronous const input and separated mutable storage contract.
The metadata validator's repeated comparisons have no observable
callback or state effects under this contract. The real x86
floating-point environment wrapper is not edited; the fixture
also compares standard floating-point exception flags on GCC.
Unmasked native x87/SSE exception behavior remains an integration
test, not a claim proven by this scalar fixture.

The same Inner serves the existing prepared and reference storage
entries; no change to their APIs, alias checks, rollback or
partial-disposable-write contract. GAME locomotion resource
publication/reset and owner checks are unchanged
(g_cod1_actor_locomotion_resources.inc :137–170,206–224,252–268).
Do not remove ActorLocomotionClosure or FaceFrame fences on the
strength of this ledger: those have additional predicates and
different lifetime boundaries.

## Verification actually performed

Connector ranges at R40 and R46A have matching lengths and FNV1a:
- arithmetic + original Inner: 6915 bytes, a4e25f5c;
- arithmetic alone: 5725 bytes, 21e54726;
- original Inner: 1190 bytes, b968538f;
- metadata validator: 660 bytes, 3ad6c72f;
- scalar tree validator: 1234 bytes, 2cdb8cab;
- frequency recursion: 546 bytes, c6e8c8ff;
- descendant: 162 bytes, 6211c0e4;
- cursor: 168 bytes, b31ad407.

Local extracted fragment Git hashes:
old Inner 0a435a04a1f1c3f7dac58ae846bf60ca2a033dca,
new admission/Inner af786c96555f879099e0b89cb9778ce16412388b,
kernel 1db277444b42e4d45945716ccbf394940f8e50e4,
deps 29d197d3f195be0bbe706507db55a83630582a6a,
metadata 6f0020ddc995180ba3de04bcd2f3c61b16884acc,
tree a5cf2de33e1e258a339ee37d676bbf1e10f5c235.
Exact executed fixture 4bf2de2ec755b1fa806504509f4bd24616301cf1.

GCC C99 -O2 -Wall -Wextra -Werror -Wno-misleading-indentation
-frounding-math and the combined ASan/UBSan build (-O1 -g
-fno-omit-frame-pointer -no-pie) both passed:
516 cases per build, 481 success, 35 refusal, 582 emitted event
records and 193 resume results among successes. The test compares
return status, context ok, complete node and flag arrays, full
step bytes (including event order, usedDt and resume) and standard
floating-point exception flags. Four rounding modes, 5/2078/4096
nodes, up to 256 notes, sync and ordinary branches, inactive rows,
loop/cycle wrap cases, invalid metadata and mutable fields,
same-address metadata mutation between invocations, null resources,
subnormal and NaN values, shared resource references.
Counted calls to the actual metadata validator across the synthetic
suite: 2101 before, 934 after; first selected case 5 -> 2.
These counts are not a timing benchmark or representative of the
actual Training workload.

Reproduce on checkout with R40 history:
    sh tools/tests/run_opt_r46b_clock_validation_test.sh

The driver compares shared kernel/dependency fragments to R40
and stops if they differ; local driver syntax checked.
Local executions used reconstructed fragments verified against
the pinned connector ranges, not a complete checkout. The tests
execute the real scalar admission and arithmetic functions;
tree layouts and resource inputs are synthetic, not native assets.
No full GAME build, MSVC x86 SIMD/fenv validation, real GSC event
delivery, loader integration, or game-level frame profiling.
Hunk unchanged; no claims about FPS. Keep R40 as control.

## Next

R46C: investigate the actual CourseClockSteps/Runner and
FoleyDispatchFrame/Begin/Step/Drain boundaries for coarse-grained
ownership and unnecessary work with preserved ordered callback,
retirement and resume semantics. Distinguish inclusive course
time from kernel and VM time. Do not continue a series of minor
validator edits without evidence of architectural benefit.
R47 CFG-free normal campaign and R51/R52 integration remain
mandatory. No intermediate gameplay test requested.
