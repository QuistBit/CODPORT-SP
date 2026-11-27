# OPT R47E — opt-in map-owned GSC configuration

Base R47D 95900e9e559bbdbd35c6af2c432d93bb05b0187b. The new native configuration lane is now connected
to GAME. This is an experimental integration, not accepted CFG-free
gameplay, not an FPS result, and not default activation.

## Entry and lifetime
With fresh SP BSP59 Training/us_intro and default-only legacy selection,
+set g_cod1CampaignProfile 1 activates the already admitted R47D
candidate. The default (unset/0) retains R47D/legacy behavior. Any
explicit legacy feature/root selection keeps priority; use a clean
profile without +exec training_player.cfg for the experimental run.
No cvars are synthesized or assigned native feature values. Native
root selection uses the existing map/root table. No Pathfinder profile.
The selector is sampled at map admission only; it is not a mid-game
enable/disable switch. The native latch survives retirement, so reads
fail closed rather than fall through to diagnostic values. Only a
strictly newer entity-map epoch can clear the latch. Ordinary Session
setup Drop does not retire the profile; shutdown/save/MapEnd still do.

## Configuration and receipt
The existing typed integer gateway returns the selected immutable value
without a cvar trap in the native lane. The string gateway returns
canonical feature strings and the original map root, bounded by the
caller's capacity; mapname, savegame_loading and developer_script
remain live reads. Legacy lane still invokes the exact original
reader with unmodified input/result. Retired native feature reads
return zero, root/feature strings empty, live keys still live.
Native init/frame admission rechecks actual live map/save and epoch;
failure retires only that epoch. Early owner checks still bracket all
24 receipt fields and the callback, with additional pure lane checks.
Of the 24 string reads in a full early check, 21 now use the native
descriptor rather than engine cvar queries; the original callbacks
and comparison count are NOT reduced. No measured time gain claimed.
Native changelevel no longer writes the next root to a diagnostic
cvar; the next map derives its own configuration. Legacy writes and
native map command sequence retained. Training's existing finale
transition is unchanged. Unsupported next maps still lack admission.

## Additional actual consumers
- src/game/g_client.c: 1 reads
- src/game/g_cod1_training_goal_placement.inc: 1 reads
- src/game/g_cod1_use_select.inc: 2 reads
- src/game/g_cod1_node_registry.inc: 3 reads
- src/game/g_cod1_vehicle_nodes.inc: 2 reads
These cover player sentient initialization, USE, training goals,
authored node and vehicle intake. The explicit R200/R216 diagnostic
spawn fixtures intentionally retain raw diagnostic cvar/root checks;
the default-only native lane must not select these fixture positions.
Sampled CGAME, client, server files have no literal g_cod1Gsc feature
reads. This is not a repository-wide proof: GitHub code search
returned no results and a broad fetch batch reached the tool call cap.
Additional raw/dynamic consumers and isolated fixture compatibility
must be audited before default activation. Legacy registrations and
diagnostic flags are retained. The non-GSC renderer/game options from
training_player.cfg and packaged launcher still require separate review.

## Verification actually performed
Exact owner header 9b77a960fd9859c4cd3f7a6eb14ecbc5beaaf7d3,
lane header baa75a442a2cc8ab9b844921b16fe59278542fa7,
fixture 8c2d433899f656391a3e2f574fe770751aa390cd.
R47A entry, R47B feature and R47C config headers locally reconstructed
and Git hashes match the pinned blobs. GCC C99 -O2 -Wall -Wextra
-Werror -Wno-unused-function and combined ASan/UBSan -O1 -g
-fno-omit-frame-pointer -no-pie both executed 1774 cases PASS:
two profiles x (23 feature cases + 27 keys x 32 capacities).
Asserts exact legacy callback count, zero callbacks for owned keys,
live key forwarding, byte-exact bounded strings, invalid/stale owner,
retirement, fresh epoch and refusal of repeated activation.
Reproduce:
    cc -std=c99 -O2 -Wall -Wextra -Werror -Wno-unused-function -Isrc/game/gsc_host tools/tests/opt_r47e_campaign_lane_test.c -o /tmp/r47e && /tmp/r47e
    cc -std=c99 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -fsanitize=address,undefined -fno-omit-frame-pointer -no-pie -Isrc/game/gsc_host tools/tests/opt_r47e_campaign_lane_test.c -o /tmp/r47e-san && /tmp/r47e-san
The actual GAME wrappers, transition, teardown and engine traps have
not been executed by this fixture. No Windows DLL build or game-level
test. Hunk, assets, animation, VM execution and renderer unchanged.

## Next
R47F: finish consumer/launcher audit, compare clean native vs accepted
legacy startup, check full lifecycle including deferred cleanup and
transition. Retain the opt-in until integration is demonstrated.
For a first experimental run use clean profile, original assets,
native DLL parameters, omit +exec training_player.cfg, add
+set g_cod1CampaignProfile 1 and spmap training. Keep existing accepted
non-GSC settings for controlled comparison. Confirm [Campaign R47E]
native map profile=training, original VFS script, early construction,
graph, player start, full Training and us_intro entry. No assertion
that these succeed before user/native build testing. R51/R52 remain
the full integration and frame-time gate.
