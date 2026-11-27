# OPT R37 — exact active paths during AnimTree Clear

Base: R36 `af829f0a971167e8fa23485b89ec0278ea37e166`. R36 user acceptance on Training: Hunk peak
98.39 MiB of 256 MiB, zero nonfinal frees. Keep Hunk size unchanged.
The new frame capture puts mean GAME/course during ladder descent at
24.39/15.37 ms, maxima 49.56/42.83 ms. These are server tick inclusive
active-slice durations, not presentation frame durations. Spikes also occur
outside the ladder. Do not treat the whole course stage as leaf evaluation.

## Source audit and change

G_Cod1ActorFaceFrameBegin still uses C1AFW601_Private; Step enters a
fresh actor transaction, then ActorLocomotionFrameAdvance (general clock)
or ActorScriptedFrameAdvance. Acquire/Commit verify the full owner,
baseline, closure and callback fences. None of those checks are removed.
The generic clock and weight advance still validate all mutable nodes.
Their separate contributions to course have not been measured yet.

One concrete algorithmic issue is in C1AGT14_ClearInner. Locomotion
animation mutations call C1AGT14_Clear from ActorLocomotionMutate;
the generic animation mutation path also uses it. The old implementation
queries C1AGT14_Active for each descendant, repeatedly scanning the full
catalog of up to 4096 nodes (2078 in generic_human) and walking ancestry.
This can be quadratic in the tree size, even before ClearGoal itself.

Compute exact active-path membership once on the validated private stage
for branch clears. Mark the ancestor chain of every positive-current leaf
only if it has no zero-current ancestor. A proven ancestor can terminate
a subsequent walk. Leaf-only clears retain their old query. ClearGoal is
called on every original selected node at the exact same depth/ordinal
position, with the same fade, validation, FP wrapper and state arithmetic.
No topology/validation or ownership changes, no cross-call cache, no
rate/phase/notification shortcuts, no map-specific logic. The temporary
mask costs 4096 bytes of stack, no extra allocations.

Proof: a node is active exactly when it lies on a nonzero-current path
from the root to a positive-current leaf. ClearGoal changes currentWeight
only on an inactive path, setting it to zero. Such a node is not on any
active path; its change cannot destroy or create one. Therefore the
initial mask remains exact during that specific clear transaction.
This does not hold for general animation mutations or advancement.

## Validation

The new production helper and the exact changed ClearInner, scalar
Valid, Active, Descendant and ClearGoalInner function bodies were tested
in a local extracted C harness; the production function excerpts were
checked against the fetched/modified repository text by length and FNV-1a.
The public test includes the real generic-tree header when built in
the repository. 160 deterministic generated trees, 484 successful
differential clear cases (1 to 4096 nodes), forward parents, deep trees,
zero and signed-zero weights, subnormals, mixed fades, sparse and dense
activity, and invalid input/unchanged-state checks. All compared output
node bytes match the R36 reference. Strict GCC and ASan/UBSan pass.
Local harness used scalar validation and non-Windows wrappers; native
MSVC SSE/FP integration and actual game build remain untested.

Reproduce in the repository (existing compact headers use one-line
statements and have unused static functions in standalone tests):

    cc -std=c99 -O2 -Wall -Wextra -Werror -Wno-misleading-indentation -Wno-unused-function tools/tests/opt_r37_active_paths_test.c -o /tmp/r37_test
    /tmp/r37_test
    cc -std=c99 -O1 -g -Wall -Wextra -Werror -Wno-misleading-indentation -Wno-unused-function -fsanitize=address,undefined -fno-omit-frame-pointer -no-pie tools/tests/opt_r37_active_paths_test.c -o /tmp/r37_san
    ASAN_OPTIONS=detect_leaks=0 /tmp/r37_san

This is an algorithmic reduction in tree scans for branch clears, not
a measured FPS improvement or evidence that Clear is the main course
bottleneck. A small or mostly inactive subtree may see little benefit;
the new branch mask still scans the catalog. Keep the existing frame/anim
profilers. Next R37 follow-up/R38: audit FaceFrame acquire/clone/fence,
closure, weight advance, general and scripted phase steps and GSC callback
delivery independently; preserve admission/receipt boundaries. Continue
through all Training phases, not just ladder, before native acceptance.
