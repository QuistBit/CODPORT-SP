# OPT R47B — typed campaign feature gateway

Base R47A c2dc7785af86b1822eddeb19d38e0ae5158d056c. This is a bounded migration, not CFG-free
acceptance or a performance improvement. No native mode is activated.

## Configuration inventory and descriptor

The 23 numeric g_cod1Gsc features in main/training_player.cfg are
represented by a typed feature ID, exact legacy cvar name and candidate
immutable Training/us_intro values. The GSC root is separate (R47A).
Program selection, script path and capability selection must remain
distinct. The table is NOT used to synthesize cvars or grant a Session.
Pathfinder remains experimental and has no native descriptor.

The Training values were compared against the actual pinned CFG and
all 23 match. The intro values were compared by applying the existing
16-feature off list and four enable writes in CampaignMapProfile to
that CFG; all 23 match. Captions remain 1 after that explicit transition,
but their actual use also requires the existing streamed profile 2.
This is a description of the accepted explicit configuration, not
proof that all those features are necessary or sufficient in isolation.

## Production migration

Add G_Cod1GscFeature47, an internal GAME function taking a typed
feature ID. For now it invokes exactly one original cvar read, returns
the integer unchanged (including diagnostic values 2 and 44–47),
and does not cache or clamp results. Unknown IDs are not admitted.
The descriptor is not yet used in this function. Migrate 90
literal integer reads across these existing files:

- src/game/gsc_host/g_cod1_gsc_test.c: 64
- src/game/gsc_host/g_cod1_early_program_handoff.inc: 3
- src/game/gsc_host/g_cod1_native_thread_frame.inc: 1
- src/game/gsc_host/g_cod1_client_startup.inc: 1
- src/game/gsc_host/g_cod1_script_sound_notify_host.inc: 1
- src/game/gsc_host/g_cod1_entity_host.c: 3
- src/game/gsc_host/g_cod1_actor_bootstrap_host.inc: 13
- src/game/gsc_host/g_cod1_early_turret_resources.inc: 3
- src/game/gsc_host/g_cod1_actor_placed_init.inc: 1

The declaration is in g_cod1_gsc_test.h inside the existing feature
build guard. Entity host includes it explicitly. No new translation
unit, ABI, game/save/protocol struct, cvar write or launcher change.
All replaced read sites were reversed back to their exact original
text and compared byte-for-byte before adding the declaration and
function. The replacements retain original short circuit/order and
checks; the production wrapper may add small call overhead, not
measured. No timing benefit claimed.

## Remaining consumers and lifetime work

The map root string reads/writes, the dynamic names[] and
gscEarlyNames[] reads (24-string early receipt), registrations,
other GAME hosts and any engine/CGAME consumers are not migrated.
The sampled CGAME loaded-caption, script-sound and voice files have
no literal reads of these 23 cvars; GAME script_sound_notify_host
does. This is a partial inventory, not an exhaustive repository audit.
Save/load, map identity, developer_script, runtime diagnostics,
owner/epoch/Session fences and live callback checks must stay live.
R47C must complete the consumer inventory and profile lifetime
admission before switching the normal path. No silent automatic
fallback to a diagnostic mode or unsupported map.

## Verification

Local exact pure header and fixture tested with GCC C99 strict
-O2 -Wall -Wextra -Werror and ASan/UBSan -O1 -g
-fno-omit-frame-pointer -no-pie. Each run: 161 live read cases,
46 descriptor values PASS, plus name uniqueness, invalid input
and unchanged output on invalid profile. Each valid feature read
calls the original reader exactly once; repeated values change.
Header Git blob c38bb7ee21c4d74ab0967e59a481d93f21079448,
fixture ded9bab27da5ec85ec0f51391ebced0e4ce8e71c.
Reproduce:
    cc -std=c99 -O2 -Wall -Wextra -Werror tools/tests/opt_r47b_campaign_features_test.c -o /tmp/r47b && /tmp/r47b
    cc -std=c99 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -fno-omit-frame-pointer -no-pie tools/tests/opt_r47b_campaign_features_test.c -o /tmp/r47b-san && /tmp/r47b-san

No actual GAME DLL linkage, native build, original scripts, transition
or frame-time comparison executed. Existing CFG remains mandatory.
No Hunk/graphics/animation/VM behavior change intended.
