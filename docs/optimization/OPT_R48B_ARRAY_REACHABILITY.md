# OPT R48B — batch staged-array ancestor reachability

Base R48A 28c036e7771546111e8a3f951c08a40d6ae34a01; gameplay control R47L 2febee854737531f55e70d95cbd0103c4edefa4d.

Audit: the live arena array_reaches already has an operation-local directory. The private staged arena r238_stage_reaches still scans every member for every expanded node and the COW commit repeats this search for each of up to 48 ancestors. R48B only replaces the final staged ancestor loop. All original live-arena prechecks, COW capture/ownership, allocation, and publication remain unchanged.

On valid bounded stage inputs use a single ordered LIFO walk, with a temporary reverse-built edge directory on second expansion. The first target retains the original early return: if an invalid reachable edge would be encountered before that target, the first old search would fail regardless of any later targets; otherwise a fully successful walk can return whether any target is reachable. All edge enumeration preserves ascending source order. Invalid stage domains and invalid starts use the original ordered loop, with original diagnostics. No change to source array clones, script order, or persistent data; no heap allocation. Scratch is about 18.5 KiB stack on x86; no MSVC stack measurement.

Complexity for a fully explored valid graph changes from up to T*V*E member inspections to O(T+V+E) with the local index, where T<=48, V<=256, E<=4096. This is a source-level bound, not measured speedup. Single-node or early-hit cases may gain little; no claim that this is the main VM bottleneck.

Validation actually performed: old function 744 bytes FNV1a c4acc79a / git d010396ebdda7425cb6581edc12f0e1df67175ef; new helper 2099 bytes FNV1a f39c1031 / git f0acdcb902b251aac7a93abcbab9bf078cac31c1. Connector replacement matched local lengths/FNV and reversing insertion/call change reproduced source. Extracted functions compiled with GCC and Clang C11 -O2 -Wall -Wextra -Werror, plus Clang -O1 -g ASan/UBSan: each printed 10030 exact result/diagnostic comparisons PASS. Fixture has minimal stand-in structs and rfail; compares against the old ordered per-ancestor loop, including explicit LIFO error precedence, maximum capacities, cycles, invalid starts/edges, duplicates and randomized target lists. No complete game build or live mission run. Driver included; local tests used extracted functions, not full checkout driver.

Reproduce with repository history: sh tools/tests/run_opt_r48b_array_reaches_test.sh

Next R48C: audit repeated VM function/host admission work; retain all externally observable checks at callback boundaries. No campaign or Hunk changes. Accumulate changes before integrated native R52 acceptance.
