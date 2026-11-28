# OPT R54D3B — verified handoff and Steam Deck native gate
Base 720eaa286c044aece2d88cd0875cfafbf8c84c24. Candidate core 1e0e9038f05a218cdacc5b873716fd51b9db18f1. Default reference selector remains unchanged. Master untouched.

## Native evidence (user supplied, 2026-11-28)
rtcwconsole(20261128-161002).log SHA256 4fcb88d260c57b52a7493f9e240efef4f73a12b7e564ba81752a73160908685a. Last cumulative counters for epoch 1: current 7324, exclusive 8382, note 4565 checks; all matched, zero skipped/mismatched/mark_misses; 33647 mark lookups / 36071 lookup probes, 1060205 map rows, 10418 reseeds, 2265129 feeds / 4349596 audit rows. Last sampled totals are lower bounds (unsampled tail); no epoch 2 dirty coverage. 909 OPT_VM53 samples, total raw QPC ticks 72564975, source 70832003 (97.61%), scan 169996 (0.234%). Largest sampled source 33.9195 ms; scan max 0.1116 ms. These are selected phase samples, not all frames or exclusive engine CPU attribution; source includes interpreter plus native/nested execution. Given evidence, unverified fast dispatch is deferred: no plausible major benefit for observed spikes, and incomplete mutation coverage is still a risk.

## Experimental verified mode
+set g_cod1VmProfile 1 +set g_cod1QueueDirty 1 +set g_cod1QueueVerified 1. After original successful scan and full independent membership/selection audit, resolve candidate slot and verify serial/insertion/deadline or empty/future deadline; use only if identical. If refused, preserve reference result. Adds indexed_choices and indexed_fallbacks to sampled cumulative OPT_DIRTY54 logs. No new fast path; all original scans/errors/guards remain. Does not reduce CPU cost and not intended for FPS tests. Default off.

## Verification
GCC and Clang C11 O2 strict plus Clang ASan/UBSan O1 all executed locally. Each: 100000 mocked drain cases, 274524 verified choices, 199397 dispatches, 75127 complete, 6502 paused, 18371 reference failures. Deliberate missed non-head mutation forces one fallback while preserving original execution. 300000 randomized cases across compilers, 823572 matched boundaries. Exact locally tested verified helper ff8f9794e0357163688522245a416a7fd834bf20 and fixture d84857d47c195362e0652a55858256c86bde1e84. Runner pins helper revisions; no full native compilation or new gameplay test. Container cannot resolve github.com. R54D3A native evidence applies to observer, not this new mode.

## Next
Prioritize R55 source/native attribution; retain reference scheduler as shipping default. A faster unchecked queue is deferred pending stronger mutation coverage and measurable need. No mandatory new build for this diagnostic step alone.
