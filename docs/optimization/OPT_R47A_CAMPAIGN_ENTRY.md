# OPT R46D / R47A — preserve clock boundaries; separate map/root identity

Base R46C f5c6ce61385fdc69d268a0689d62cfe711ca1fa2. Control R40
8f92680c078b717f61aa762ef9a5c93e10eb99cb.

## R46D decision: no unsafe transaction shortcut

Re-read exact R46C face_frame.inc:252–303,323–388,
scripted_frame.inc:110–154, turret_host.inc:399–467,
g_cod1_actor_clone_storage45.h and g_cod1_face_frame_workspace45.h,
and R45D3/R46B/C handoffs. R45 already pools independent snapshots
and coallocates node/notify/name arrays. A completed-frame sample
in FoleyDispatchFrame is not only debug output: its Acquire invokes
provider admission and current; it checks live closure, owner and
catalog, validates C1ASB14_NodesFull, calls current again and fences.
Removing it or substituting a previous success would remove refusal
and callback boundaries. Begin and Step commit distinct revisions;
Drain can consume receipts before another callback and commits
even for an empty batch. No new evidence justifies fusion, revision-
only guards or borrowing the parent's baseline. Keep these operations.
Defer larger changes until R46C stage capture and actual owner
equivalence. Do not mark the full performance target achieved.

## R47A — source-backed entry selection foundation

User acceptance for R47: normal menu launch/spmap training, clean
installation with original assets, no training_player.cfg or manual
g_cod1Gsc* configuration, map-derived original GSC, automatic transitions.
Do not implement by transferring the CFG commands into engine startup.

Observed R46C boundaries:

- server/sv_init.c:782–799 loads collision, sets actual mapname, then
  initializes GAME. g_spawn.c:1141–1175 calls
  G_Cod1GscCampaignMapProfile before map spawn/prepass, then
  PrepareBeforeBsp and PreparePlacedResources. This is the earliest
  current campaign selection seam in GAME, not a UI-only solution.
- g_cod1_gsc_test.c:1886–1904 currently requires explicit enabled mode
  43 even to choose a campaign map; us_intro requires previous training
  root and rewrites configuration. Pathfinder requires its explicit
  root and remains incomplete/experimental. No automatic activation
  for training exists in this function.
- g_cod1_early_program_handoff.inc captures 24 string cvars, including
  actual mapname, developer_script, savegame_loading and selected root.
  GscTestEarlyCurrent repeats up to 24 trap reads plus owner validation
  after each read. This can contribute to inclusive GSC/course cost;
  no timing or call count for this operation is yet available.
- g_cod1_client_startup.inc, g_cod1_actor_bootstrap_host.inc,
  g_cod1_entity_host.c, g_cod1_native_thread_frame.inc and
  g_cod1_early_turret_resources.inc have direct feature checks.
  g_cod1_gsc_test.c owns the Session construction and phase gates.
  This is a multi-TU migration, not simply enabling defaults.
- g_cod1_changelevel_host.inc:55 currently writes NEXT root to a cvar
  before the queued map command. A native entry must derive its own
  profile from the new map, not mutate live Session configuration.
- packaging/windows/Play-Training.ps1 still executes training_player.cfg.
  Do not remove until actual native selection and all consumers work.

Production step: g_cod1_campaign_entry47.h classifies known canonical
map/root pairs and the existing explicit-mode transition predicates.
Integrate in GscTestOriginalTrainingPath and G_Cod1GscCampaignMapProfile,
preserving exact case-sensitive matches and existing cvar writes, gates
and their order. No new activation, no unsupported map claim, no script
replacement or synthetic CFG. Training/us_intro/pathfinder are known
entry identities only, not proof of complete gameplay. The legacy
transition semantics remain unchanged including mismatched root refusal.

## Verification

Exact header 9178c2d54b0ff5353ee647b72e9e45574937cc31, test
ea1fc18c8ffb5d4bd3a1d66ff09d05f54db60193. Strict GCC C99
-O2 -Wall -Wextra -Werror and GCC ASan/UBSan (-O1 -g
-fno-omit-frame-pointer -no-pie) both passed 208 map/root pairs
against original predicates, plus identity checks. Covers null/empty,
case differences, unsupported roots, test paths, traversal-like strings,
known entries and mismatches. Test is pure selector only; not full GAME
or menu/mission integration. Reproduce:
    cc -std=c99 -O2 -Wall -Wextra -Werror tools/tests/opt_r47a_campaign_entry_test.c -o /tmp/r47a && /tmp/r47a
    cc -std=c99 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -fno-omit-frame-pointer -no-pie tools/tests/opt_r47a_campaign_entry_test.c -o /tmp/r47a-san && /tmp/r47a-san

## Next R47B

Inventory every selected feature consumer (including dynamic string
arrays, GAME/CGAME and engine boundaries). Define immutable, map-owned
native capability selection with distinct explicit diagnostic lane.
Bind to actual fresh SP map/format and epoch before actor prepass.
Preserve independent live map/save/restart/session/receipt checks.
Migrate consumers in bounded steps before activating normal launch.
Do not silently admit unsupported maps; Pathfinder remains experimental.
Transition selects new root at the new map boundary, not by changing
current Session. Leave launcher/CFG compatibility until native path is
integrated. R51/R52 still require Windows same-stand comparison,
full Training and us_intro handoff, frame-time peaks, and regression
tests. No native build, performance improvement or CFG-free acceptance
claimed in R47A. Hunk unchanged.
