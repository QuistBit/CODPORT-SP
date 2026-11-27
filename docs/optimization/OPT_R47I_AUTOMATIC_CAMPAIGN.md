# OPT R47I — automatic campaign profile on fresh SP maps

Base R47H 2620811c4902706d4e01258770e0a0ecb6c51e79.

The user's rtcwconsole(8).log is a CFG-enabled control: it executes
training_player.cfg, prepares maps/training.gsc, constructs the graph
and starts the original GSC at 750 game ms. The reported failure without
the CFG is consistent with the R47F source still requiring the optional
g_cod1CampaignProfile integer selector to be 1. Engine Cvar reads
return 0 for an absent integer and an empty string for an absent string.
No CFG-free runtime trace was supplied.

Select the already implemented immutable campaign profile automatically
when the optional selector is absent/empty or explicitly 1. An explicit
0 or invalid spelling opts out. Preserve all original fresh SP, IBSP59,
map identity, Session, epoch, dedicated, restart, save and legacy test
configuration fences. Do not write or synthesize the 23 old test cvars.
No new maps are admitted. Normal Training and us_intro use their own
map roots, including the next epoch after transition. The native source
launcher no longer passes the optional selector either. Existing legacy
launchers remain available while real GAME acceptance is pending.

Tests actually executed: opt_r47i_campaign_default_test.c, 3584 cases
each under GCC and Clang C11 (-O2 -Wall -Wextra -Werror
-Wno-unused-function) and Clang ASan/UBSan. The fixture exercises the
real lane helpers, not the full GAME callback or Windows DLL.
The 5 original headers were reconstructed and all original Git blob
hashes matched the pinned base before editing. Tested new lane header
blob 324d7f70b3e40dbb65bb8c8a969768dbd83f5a13; test blob
90677f67c58475e81f92911d430abf53ebf69c35.

Actual acceptance: use fresh process with no +exec training_player.cfg
or +set g_cod1CampaignProfile. Start through menu or spmap training.
Expect [Campaign R47E] native map profile=training, original GSC
preparation, pre-player graph, then started. If legacy configuration
is inherited, the R47I message names its conflicting key. Verify
us_intro transition separately. Full MSVC build and game test pending.
