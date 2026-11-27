# OPT R47C — complete the examined dynamic configuration read seams

Base R47B 387f85dc1b3507224db83273d40e66e96a133342.

R47B forwarded literal integer reads in nine files. This step extends
the same gateway to the examined Training feature consumers, typed
dynamic configuration arrays and root string reads. No map-owned
profile is activated and no settings are changed. Legacy strings remain
exact, including empty strings and numeric spelling such as "01".

Changes:
- g_cod1_gsc_test.c: 0 integer, 5 string, 0 dynamic read sites
- g_cod1_early_program_handoff.inc: 0 integer, 0 string, 2 dynamic read sites
- g_cod1_client_startup.inc: 0 integer, 4 string, 1 dynamic read sites
- g_cod1_cvar_host.inc: 0 integer, 0 string, 1 dynamic read sites
- g_cod1_training_turret_host.inc: 10 integer, 0 string, 0 dynamic read sites
- g_cod1_training_carbine_host.inc: 10 integer, 0 string, 0 dynamic read sites
- g_cod1_training_grenade_host.inc: 2 integer, 0 string, 0 dynamic read sites
- g_cod1_training_finale_host.inc: 1 integer, 0 string, 0 dynamic read sites
- g_cod1_turret_host.inc: 1 integer, 0 string, 0 dynamic read sites
- g_cod1_author_fog.c: 1 integer, 0 string, 0 dynamic read sites

The early receipt still has exactly 24 rows, in the original order;
mapname remains index 15 and root index 16. Its string comparison,
busy/cancelled checks, owner calls after each read and capture layout
are unchanged. The startup array still reads 13 fields in original
order; setcvar admission still reads six values in original order,
including repeated enable/mode/save checks. No callback is skipped.
Authored fog remains disabled by the same enable predicate. No
change to the actual VM setcvar API or the protected-name policy.

The new configuration key classifier distinguishes the 23 features
and root (future immutable map-owned candidates) from live mapname,
savegame_loading and developer_script. All are still read live today.
Do not normalize strings in legacy mode, which would weaken its
existing receipt comparison. The separate integer helper routes
feature IDs to the existing R47B gateway and save to the original
engine query.

## Map lifetime seam for next step

G_InitGame calls G_Cod1GscTestShutdown then G_Cod1EntityMapBegin before
clearing level and spawning. EntityMapBegin increments epoch after
cleanup, EntityMapEnd retires readiness/active state. G_SpawnEntitiesFromString
currently calls legacy CampaignMapProfile BEFORE G_Cod1MapSpawnBegin;
the latter checks the actual VFS BSP header and sets g_cod1MapEntities.
Therefore normal profile admission must occur AFTER format detection
and BEFORE actor/node prepass. Do not use the old map's format flag.
The actual restart argument is available in G_InitGame; level.restarted
alone is not a fresh-map certificate. Preserve separate legacy entry
order. Map-owned selection must be invalidated at shutdown and before
save load, even if Session cleanup defers; ordinary GscTestDrop is also
called during new Session initialization, so it is not an unconditional
map-profile retirement seam. No epoch ownership object installed here.

Remaining: finish inventory outside the examined GAME files and
other dynamic reads; root writes at transition/legacy setup and
diagnostic registration remain explicit. Build a map-scoped
admission object with independent live map, save, restart and epoch
checks and a diagnostic override policy. Only then activate Training
and us_intro without CFG. Pathfinder remains experimental.

## Actual tests

Exact new config header blob 1ea80295381d0d62fc7b0f8163e29e1ba945ffaa;
test blob 54a5900aee3e3d413237d111a0af486c1b899303; R47B feature
header c38bb7ee21c4d74ab0967e59a481d93f21079448.
Executed locally with GCC C99 -O2 -Wall -Wextra -Werror and
ASan/UBSan -O1 -g -fno-omit-frame-pointer -no-pie. Both returned
2160 cases PASS: 27 keys x 8 raw values x 10 capacities, full output
byte comparison and exactly one reader call, plus invalid arguments.
Reproduce:
    cc -std=c99 -O2 -Wall -Wextra -Werror -Isrc/game/gsc_host tools/tests/opt_r47c_config_test.c -o /tmp/r47c && /tmp/r47c
    cc -std=c99 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -fno-omit-frame-pointer -no-pie -Isrc/game/gsc_host tools/tests/opt_r47c_config_test.c -o /tmp/r47c-san && /tmp/r47c-san

Tests cover the actual pure string helper, not the GAME integration,
owner callbacks, map transitions or full Windows compilation. Native
profile not yet selected; CFG remains required. No FPS gain claimed.
Hunk, assets, animations, scheduling and renderer unchanged.
