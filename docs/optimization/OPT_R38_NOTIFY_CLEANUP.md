# OPT R38 — sparse notification cleanup in locomotion mutations

Base R37: `56faeac9a7df493b4b4e3788533dd8ab978da2b4`. R37 branch-clear active path change remains intact.
The R36 Training capture still sets the baseline; no native R37/R38 capture
has been supplied. Hunk capacity remains unchanged.

## Transaction audit

ActorFaceFrameAcquire validates the owner and closure, clones mutable nodes
and notify data (both stage and baseline), copies the actor/entity snapshots,
and invokes current(ctx) before a full fence. Commit repeats the callback,
fence and stage closure. R35 already pools clone arrays and consolidates the
face call allocation. R242/R325 already provide exact SIMD byte comparisons.
Those fences and callback boundaries must not be skipped; a revision alone
cannot prove that arbitrary callbacks did not modify the owner. Begin already
uses disposable weight advancement and Step disposable general-clock advance.
The cost of these components has not yet been individually measured.

One concrete avoidable cost remains in ActorLocomotionMutate (used by the
general tree set/knob/flagged/clear paths). For each tokenless ordinal in the
2078-node catalog, the old loop calls ActorGenericNameSet(NULL,0), including
a binary search and a name buffer initialization, even when no name exists.
The sparse name prefix is already sorted by the insertion method.

The replacement validates the prefix order and bounds before walking the
ordinals with a forward cursor. For tokenless ordinals without a name no
binary search is necessary. For each actual deletion use the same memmove,
in the same ascending order, preserving even bytes outside the semantic
prefix. Every tokenless generation, pending identity and pending tick is
still cleared. Token-bearing ordinals and their names are untouched.
On malformed prefix, missing storage or out-of-range count use the original
per-ordinal name-set path, including its partial writes and failure order.
No change to the checked transaction/ownership, notify delivery, new-name
insertion, phase, FP or physics. No persistent cache and no allocations.
The dense removal case still has the same memmove cost. The valid sparse
case eliminates per-ordinal binary searches, not the ordinal scan itself.
Scope is only ActorLocomotionMutate; other name cleanup callers stay unchanged.

## Tests

The original local notify-names include matched the fetched Git blob
80f8d958861ed0dd9642dd211b70f0447bd3dc8a. The modified actual include and
test pass 489 deterministic differential cases, comparing return values,
all notify bytes and full name storage (including tail), with empty/full/
sparse names, all-token/all-tokenless patterns, unsorted/duplicate/out-of-
range ordinals, invalid count and missing name storage. Host types are
minimal fixtures, not a linked GAME DLL. Strict GCC and ASan/UBSan pass.

    cc -std=c99 -O2 -Wall -Wextra -Werror -Wno-unused-function -Wno-misleading-indentation tools/tests/opt_r38_notify_cleanup_test.c -o /tmp/r38test
    /tmp/r38test
    cc -std=c99 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-misleading-indentation -fsanitize=address,undefined -fno-omit-frame-pointer -no-pie tools/tests/opt_r38_notify_cleanup_test.c -o /tmp/r38san
    ASAN_OPTIONS=detect_leaks=0 /tmp/r38san

This is a bounded transaction mutation optimization. It does not show
that the largest course peaks are solved. No native build or FPS
measurement. Next: isolate costs of FaceFrame acquire/clone/fence,
closure, weight advance, general phase advance and callback delivery
using opt-in accumulated active durations, avoiding per-call logging;
audit other repeated GSC/actor checks only with source-backed evidence.
Continue to R39 without altering cadence or ownership.
