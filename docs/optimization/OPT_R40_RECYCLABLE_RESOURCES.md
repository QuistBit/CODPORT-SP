# OPT R40 — reuse exact compositions of recyclable actor resources

Base: R39 `fd89b5ace09adf172fec5a03fc18b4c62b4e6833`. R37–R39 still require integrated game acceptance.
R40 fixes a specific repeated-load path, not all course/renderer spikes.

## Evidence and source audit

DroidDeck capture (402 completed CGame windows, 5412 observed frames; executable SHA absent):
eight consecutive repeated tower cycles at game times 13.00, 24.95, 36.55,
49.25, 60.65, 72.55, 84.55 and 96.00 s coincide with the eight largest
CGame resource windows. Seven times lie within the reported window, one
10 ms before it. Each cycle includes a fresh scripted human actor (ordinal
892) and two rig starts. Peak resource frame times respectively 16.7188,
10.5111, 16.7813, 19.0793, 19.7563, 14.4392, 11.2270 and 14.2736 ms.
Corresponding maximum single resource calls: 16.6451, 10.4468, 16.7391,
19.0354, 19.7114, 14.3885, 11.1623 and 14.2246 ms.
The ninth-highest resource window is 0.6618 ms. The maximum measured
actor frame is 20.4144 ms. All windows have revision 2. QPC frequency
10 MHz. No [OPT_FRAME] in this capture; no full engine/GPU frame statistics.

The resource timer wraps CGFA4_Resource in cg_cod1_face_unmounted_actor.inc,
not the rig loader. cg_cod1_training_rig.inc retains the two rig models and
clips until reset. Do not conclude that the rig animations are reloaded.
The server g_cod1_actor_unmounted_publisher.inc binds the recyclable
CS_COD1_JUMPER configstring to the exact epoch/owner/generation. The old
CGFA4_Resource validates that prefix, parses the remaining exact model/tag
composition, and uses a single cgJumperResource slot. A different
composition or normalized bank version discards the old decoded resource
after successful staging. Returning to the earlier composition rebuilds
it from xmodel/xmodelparts, constructs the DObj, builds the bank, registers
the renderer resource, and loses the earlier per-clip mapping cache.
Renderer RE_RegisterCod1TurretActor already deduplicates canonical full
definitions; renderer handles are not the missing CGame cache. Source pose
clips are separately map-revision-owned. The log lacks actual composition
keys/hit counters: it supports this candidate but does not prove which
composition changes or how much the fix removes on the device.

## Change and ownership

Separate the existing recyclable configstring's owner admission from the
map-revision-owned decoded resource cache. Replace the single cache slot
with a bounded four-entry exact-key FIFO replacement pool, independent of
the ordinary 16 permanent config slots. Keep every prefix, parser, bank
version, snapshot, current-lease, model, tag, staged registration and
post-callback validity check. Use the same canonical composition bytes and
normalized source/scripted bank version as before. No asset names, actor
IDs, map coordinates, or script timing are added to the cache policy.
No change to the wire protocol, endpoint pose cache, actor count, animation
rate, renderer registration semantics, GSC events or Hunk capacity.

Only publish a new slot after the original loader's successful final
fence; failed loads do not evict prior entries or advance the cursor.
Full cache replaces an entry and does not refuse additional compositions.
Reset frees every retained DObj and clears the cursor before the original
clip cleanup. Map revision remains the lifetime boundary. The actor pose
cache still includes the full wire state, renderer handle and revision.
The extra three possible decoded resources consume bounded CGame heap
and static cache metadata; actual memory cost is unmeasured. More than
four rotating compositions may still miss; unique first-use loads still
cost time. Four entries is an engineering choice, not a measured optimum.

## Validation

The original loader is moved to cg_cod1_face_resource40.inc with the
minimal changes above, to test the actual loader and selector together.
Fixture uses a copied original composition parser, minimal resource/lease
types and deterministic FS, DObj, bank, renderer and current-lease stubs.
After two cold variants, 1000 alternating accesses with changed owner
and generation (and normalized bank version 25/29) cause no further
file reads, DObj builds or renderer registration calls. This is a
synthetic workload, not a claim about actual variant frequency.
Exact distinct version, stale prefix, stale revision, six injected
failure boundaries, transactional full-cache replacement, callback
reset and ordinary namespace tested. Final 11 creates/11 frees.
Strict GCC with the inherited compact-code indentation warning disabled,
and ASan/UBSan with leak detection enabled, pass.

    cc -std=c99 -O2 -Wall -Wextra -Werror -Wno-misleading-indentation -Isrc/cgame tools/tests/opt_r40_resource_cache_test.c -o /tmp/r40 && /tmp/r40
    cc -std=c99 -O1 -g -Wall -Wextra -Werror -Wno-misleading-indentation -fsanitize=address,undefined -fno-omit-frame-pointer -no-pie -Isrc/cgame tools/tests/opt_r40_resource_cache_test.c -o /tmp/r40san && /tmp/r40san

Tested production loader blob 080894f50ed0dfaad744156cfa69e8ccc595fa18;
selector blob f6b75f7710d465ffa898c5cc95d8dda05a98cf21;
fixture blob e6eba24532c90e6f6d81bd3d4efd243583ff8128.
No actual Windows build, full engine integration, renderer/GPU trace or FPS
gain verified. Do not claim removal of real spikes until repeat capture.

## Next

Repeat identical native Steam Deck Training run from first actor appearance
through us_intro (and DroidDeck separately), retain at least eight repeat
jumper cycles. Use +set g_cod1FrameProfile 1 +set g_cod1AnimProfile 1
+set cg_cod1AnimProfile 1 +set logfile 1; keep graphics/thread settings
fixed. Record executable source SHA and wall-clock load time. If resource
peaks remain, instrument exact canonical composition/version, cache hits,
misses, eviction, read, DObj creation, bank build and renderer registration
on misses. Measure separately the remaining true first-use clip loads
(e.g. course_load peak 4.5198 ms in 72.399–72.637 s window).
Do not bulk-preload unbounded resources or remove validity checks.
R41: audit actual renderer frontend/skin workload and independent tasks;
R42: presentation frame traces with P95/P99/1% low and server active slices.
FaceFrame clone/fence cost and native course peaks still require attention.
