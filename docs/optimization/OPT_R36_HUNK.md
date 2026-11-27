# OPT R36 — Hunk usage and LIFO cleanup

Base R35: `d5b03dce8e5c25ce32b84d0e3af08fb320630179`. The actual allocator default is 256 MiB;
minimum non-dedicated is 54 MiB. Config or command line may override the
default. The R34 Steam Deck capture contains 294 non-final-free warnings,
no Hunk allocation failure or bad-magic report, and no meminfo output.
The preceding R32 capture contains 351 non-final warnings. Neither log
establishes actual Hunk capacity or peak consumption. R34 clusters include
four at MG42 registration and 191 between initial actor arrivals, with
later groups of 14/17 around further registrations.

## Fix
Both renderer resource owners acquired multiple FS_ReadFile buffers in
ascending order and freed in ascending order. The MG42 loader has five
buffers; the actor loader has three per model part. Their cleanup now walks
the initialized arrays in reverse order, skipping NULL on partial failure.
No changes to read order, model construction, shader registration, resource
lifetime or Hunk allocation semantics. Other nested readers may still
free out of order. Existing warnings and errors remain enabled.
This is a demonstrated source-level ordering fix, not proof every warning
in the real log came from these two paths.

## Measurement
Use +set com_cod1HunkProfile 1 in launch arguments, with the normal Hunk
size (do not change capacity during the first comparison).
The observer tracks successful rounded permanent and temporary allocations,
frees, nonfinal bytes, and peaks from actual allocator offsets. No extra
heap allocation or per-operation logging. Counter collection is always on
and follows the original allocator owner's serialization; no new parallel
allocation support is introduced. Use meminfo for explicit snapshots.
With the cvar enabled, [OPT_HUNK] summaries print at mark, before reset,
shutdown and about every five seconds when SMP renderer is inactive.
In SMP avoid periodic reads; capture meminfo at a settled frame or use
the map/reset checkpoints. Each clear or clear-to-mark begins a new
counter epoch; clear-to-mark retains the original permanent baseline.
The pre-reset summary includes the outgoing epoch; initial epoch has no
prior data. A final incomplete interval can be captured with meminfo.

All sizes are bytes including allocator alignment and headers.
- total: actual allocated capacity, not merely the requested cvar.
- used: current two bank offsets combined, including retained temp space.
- permanent: permanent allocation offsets combined.
- free: current capacity less used.
- peak_used: actual maximum combined occupancy in this epoch, not the
  sum of independently measured peaks.
- peak_permanent, peak_temp: maximum permanent/temporary occupancy.
- retained, peak_retained: non-final freed bytes still occupying the
  temporary stack, and their maximum.
- nonfinal_bytes: cumulative freed-out-of-order bytes, not current waste.
- cleared_bytes: total temporary space reclaimed by bulk clear calls.
- cleared_live_bytes: bytes cleared that had no individual free recorded;
  this can be an intentional bulk clear, not a leak assertion.
- fs_stack: filesystem outstanding buffer count at report time.
- largest_temp: largest rounded temporary allocation.
Counters exclude allocations outside Hunk (malloc, Zone, GPU/driver).

The exact counter header/test pass strict GCC and ASan/UBSan. The fixture
models both low/high stack directions for all prefixes of 0..21 buffers,
forward and reverse, plus reset, mixed allocation and forced bulk clear.
It is not a full native renderer or allocator integration test. Windows
build and in-game logs are pending. No measured memory saving or FPS gain
is claimed; reverse release does not reduce the initial peak of a batch
that needs all files simultaneously. It reduces delayed reclamation and
warning output.
