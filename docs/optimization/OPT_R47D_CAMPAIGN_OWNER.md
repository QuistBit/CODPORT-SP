# OPT R47D — map profile owner and prospective normal admission

Base R47C b182be888849b966f7d3994bfc2f625c98d08787.

This revision installs an epoch-bound prospective profile owner. It does
NOT activate the new profile or replace any gameplay cvar read yet.
It prepares the boundary for R47E. Legacy/diagnostic map selection,
registration and script dispatch remain unchanged.

## Actual lifetime seam

G_InitGame receives the actual restart argument from GAME_INIT. After the
existing entity MapBegin and before G_SpawnEntitiesFromString it arms one
prospective owner for the current epoch. It does not infer a restart from
the zeroed level structure. G_Cod1MapSpawnBegin returns the exact mapname
used for the existing BSP probe through its caller-provided buffer (same
capacity and original trap). After the probe and before any actor/node
prepass the observer admits only exact Training/us_intro names and BSP59
fresh SP with no save load, dedicated mode, active/busy Session or explicit
legacy profile. The old CampaignMapProfile call remains before the BSP
probe. No change to actual file reads, script roots or entity ordering.

Default-only prospective admission treats all 23 raw feature strings as
default only if absent/empty or exactly "0"; root may be absent/empty or
the existing default diagnostic script. A noncanonical or nonzero value
is preserved as a legacy selection. This conservative choice does not
distinguish an explicitly typed 0 from the default 0; future activation
needs a deliberate diagnostic/baseline override policy. Pathfinder and
other maps are not admitted. The observer reads the legacy values once
at map admission only, not on each frame. It rechecks epoch and pending
phase after each trap, samples save/dedicated and rechecks mapname against
the name actually probed. It cannot atomically sample independent engine
cvars. The prospective owner never supplies gameplay configuration.

Owner phases: empty, armed, candidate, refused, retired. An admission is
one-shot per epoch; retirement retains the epoch high-water mark and
clears the profile. A stale observation cannot publish into a later epoch.
Shutdown, before-load, EntityMapBegin and EntityMapEnd retire the owner
at entry, including cases where cleanup later refuses. Ordinary GscTestDrop
does NOT retire the map owner because it also runs during Session setup.
No new allocations or retained asset pointers. A successful prospective
admission can emit a developer-only line; no default per-frame logging.

## Verification actually performed

Exact header 56305d18a1bcef874a78419e034f1a9a7ac8a710 and fixture
bd43b9659af7ea5c68f94bb193dcf51a36573ec1, with unchanged R47A/B/C
entry, feature and config headers checked by Git hash. GCC C99 strict
-O2 -Wall -Wextra -Werror -Wno-unused-function and ASan/UBSan
-O1 -g -fno-omit-frame-pointer -no-pie both ran the exact fixture:
10368 combinations PASS (9 probed names x 9 live names x 128 gate
combinations). Further assertions cover stale epoch, same-epoch refusal,
retirement, restart/save/format mismatch and strict legacy string policy.
Unused-function suppression is for existing static header functions not
called by this fixture. Reproduce:

    cc -std=c99 -O2 -Wall -Wextra -Werror -Wno-unused-function -Isrc/game/gsc_host tools/tests/opt_r47d_campaign_owner_test.c -o /tmp/r47d && /tmp/r47d
    cc -std=c99 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -fsanitize=address,undefined -fno-omit-frame-pointer -no-pie -Isrc/game/gsc_host tools/tests/opt_r47d_campaign_owner_test.c -o /tmp/r47d-san && /tmp/r47d-san

The fixture exercises the pure owner, not actual engine callbacks or
GAME linkage. Native Windows build/gameplay are pending. The integration
hooks are reviewed by source diff. No FPS improvement or CFG-free
acceptance is claimed. Existing launcher/CFG remain unchanged.

## Next

Finish auditing remaining GAME/engine/CGAME raw feature consumers and
the legacy transition writes. In R47E bind feature/root gateways and
early-program receipt to the owner only after explicit native admission,
keep live map/save/restart/epoch/Session fences and preserve cleanup.
An active native lane must fail closed on retirement, not fall back to
stale diagnostic values. Avoid changing mode during an existing Session.
Keep original script resources and no synthetic CFG. Do not claim the
full performance target achieved until R51/R52 native integration and
frame-time capture. Hunk remains unchanged.
