# OPT R34 — quiet ordinary play

Base R33: `51da1afb96ffe99978d00c77080522e905b40cb4`. Keep the actual game logic, event delivery,
animation transactions and all OPT_ANIM/OPT_ANIM_CG measurements intact.

The Steam Deck R32 console capture contains 1428 occurrences of
[479 general interval], 1391 [Foley note], 1059 [Foley note bucket]
(including one retirement), 160 [542 nearest unavailable], 35 each
[478 graph pair]/[478 physical pair], 61 [Foley pose], and 735 [R194].
These are counts of literal tags (some old lines contain literal backslash-n).
No original private log is added to the repository.

- g_debugMove (default 0): legacy successful animation-notification and
  bucket chatter, selected init/frame/pose/start traces, root interval
  probe and graph-pair diagnostic. Failed deliveries, negative wake counts,
  nonempty errors, controller refusals and committed retirement remain.
  The root interval is computed only to print it, not to move the actor;
  the call itself is now skipped. The completed-frame
  G_Cod1ActorScriptedSampleFull validation remains mandatory even when its
  old Foley pose print is off. Its return and final current fence are intact.
- g_cod1NavigationProbe 1/2: preserve the existing optional navigation
  experiment, including its unavailable result. The ordinary caller no
  longer invokes it in mode 0 or other unsupported modes just to print a
  false return. Assignment/node diagnostic tracking is still updated when
  disabled, so enabling the probes does not reset actor state. The navigation
  implementation already admits only 1/2 and describes itself as read-only.
- developer 1: server R194 successful sound send messages use existing
  G_DPrintf, which returns before formatting when developer is zero.
- cg_debugevents 1 (existing cheat-protected client cvar): successful R194
  sound-play message. Sound sequence admission, registration, actual playback
  and voice completion are not changed.

Milestone logs, error/refusal diagnostics, normal script output and sound
protocol are preserved. No new cvar, no change to default logging level,
no suppression of R31–R33 profiles. This is a targeted cleanup, not a global
filter of all log lines. Existing verbose log-based tests should explicitly
enable the relevant cvars. In particular use g_debugMove 1 for the old
Foley/479/478 evidence. Use the same profile settings for the next comparison;
for ordinary play leave developer, g_debugMove and g_cod1NavigationProbe 0.

The number of root interval log occurrences in the supplied capture is 1428,
not a measured CPU or FPS gain. Native Windows compilation and a new
controlled game capture are required. No gameplay or frame-rate claim.
