# OPT R47F — finish examined trigger/startup consumers and prepare A/B launch

Base R47E 20a41ea17faf1e8da16ce25a6823bedec5dfe37c. No default native activation and no performance claim.

## Actual integration blockers
R47E left raw diagnostic reads in SP_trigger_damage, SP_trigger_lookat
and G_Cod1LookAtRunFrame. The native lane supplies the admitted values
through the typed gateway, but raw values are zero in a clean profile:
this could retire the genuine grenade/look-at brushes or suppress their
events. Replace these three reads with the existing gateway.
Three more host admission loops (spawner array, localized string precache,
saveGame) read the six-entry enable/mode/save sequence directly. Move
their six-entry arrays to typed keys in the same order and call the
existing GscConfigInteger47. Preserve all predicates, short circuits,
owner fences and event/trace logic. Reverse substitutions reproduce
the pinned original source exactly. The save-loading key remains live.
No save/restore implementation or new script capability is claimed.

Examined 36 additional GAME source files in targeted batches; the five
affected files above contained the remaining feature reads in those
batches. This is not a full-repository proof. Legacy spawn fixtures are
still deliberately explicit. Native selection reports the first legacy
conflicting key (name only), or the rejected map/format/SP/Session or
live map/save/dedicated gate if explicitly requested. Default behavior
and diagnostics priority are unchanged. No per-frame messages added.

## Launch
Existing default/Single/Multi launchers still use legacy CFG. New
Play-Training-Native.cmd launches single with CampaignMode native.
Both source builders seed a separate profile-native/main/wolfconfig.cfg
from the same supplied template. Do not copy training_player.cfg there.
Native fs_basepath and fs_homepath both use profile-native; original
fs_cdpath remains the selected retail installation. Retain the final
non-GSC settings from Training control as command arguments, not a GSC
profile or synthetic script. The source launcher supports PrintArguments
for examining its resolved argument array without starting the EXE.
No automatic spmap: menu launch remains the normal test path.
The old release packager remains pinned to 724A binaries and is unchanged.

## Validation actually performed
Repeated the unchanged exact R47E pure lane fixture, 1774 cases PASS in
strict GCC and ASan/UBSan. Local blobs previously matched the pinned
Git hashes, rechecked before execution. In the actual candidate script
literal +set pairs, all 18 distinct non-GSC settings from
training_player.cfg match their final values. Native argument branch
has no +exec and no g_cod1Gsc* assignments. Legacy argument sequence
retains +exec training_player.cfg. This is source-level validation of
literal arguments, not actual PowerShell parsing or engine startup.
Updated existing PowerShell builder fixture to check separate profile
seed and absent CFG; PowerShell is unavailable locally, so this test
has NOT been run. No Windows DLL compilation, actual GAME callbacks
or map completion tested. Hunk remains unchanged.

## Integration gate
First compare legacy and native on the same fresh build and thread
mode. Native log must show [Campaign R47E] native map profile=training,
then the original early GSC program, pre-player graph, actual client
startup and entire course including grenade/look-at triggers, and
finally native map profile=us_intro after the original transition.
Check no unexpected refusals, and compare [OPT_FRAME] and [OPT_ANIM].
A clean profile may still inherit default.cfg/autoexec.cfg from mounted
retail assets; the refusal message helps detect conflicting legacy
settings. Do not claim CFG-free acceptance or switch the default until
real game validation. R48 resource work can proceed independently;
R51/R52 still gate full integration and performance.
