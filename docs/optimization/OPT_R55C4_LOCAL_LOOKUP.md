# OPT R55C4 — exact prefix-filtered local lookup

Base R55C3B 4ccb184f88baf164fec6708c30f5350e3676fb01. Production integration e47c2d2d15f62d45022abbc96803f6dd05047bf4.

## Changes
Introduce frame_local_find55 and frame_local_match55 for committed, NUL-terminated local names. The first two bytes of the query are loaded once, and candidates with differing prefixes avoid strcmp; empty and one-byte names terminate safely. The remaining suffix uses the original exact, case-sensitive strcmp. Descending scan preserves newest matching slot for reads, ascending scan preserves oldest matching slot for writes and waittill output registration. Six source sites: eval_inner, assignment, speculative and ordered Results captured path guards, array lvalue commit and payload_register. The original staging/commit of waittill outputs is preserved; only the lookup of already-published locals changes. No metadata, added frame memory, allocation or callback. No new cvar, no change to special local names, VM budget, field validation or original first-match semantics. Still worst-case O(MAX_LOCALS), not an index.

## Actual verification
Local GCC C11 O2 strict, Clang C11 O2 strict and Clang ASan/UBSan O1 with leak detection all compiled and executed successfully. Each run 200000 deterministic modeled frames with two searches per frame (400000 searches; 228190 hits, 171810 misses). Exact original linear reference and new helper agree in both directions and do not modify frame state. Empty names, one/two-byte names, case, non-ASCII bytes, 63-byte names, duplicate first/last and misses tested. Instrumented strcmp calls 4052163 -> 365113, 90.99% fewer calls; excludes the new prefix comparisons and is not elapsed time or FPS. Three repeated runs total 1200000 searches. Helper blob 404b881a1b32ad3527d6b038e740236c9faa4f6b and fixture e7440d1d885c0c5ec2e29c071f2bbb5be824c027 verified against locally compiled files. All six production anchors re-read after integration. The fixtures are isolated modeled data, not full native execution of eval/assignment or payload registration. No MSVC build yet. Master unchanged.

Runner: bash tools/tests/run_opt_r55c4_local_lookup_test.sh.
