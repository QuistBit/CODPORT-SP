# OPT R47G — fix historical-header compile failure

Base R47F 3dda7b9ad0ad970668ec1fe57c7aa9950900a24f.

The user's Windows compile-0.log reports 7 C2065 errors and 4
C4013 warnings: the R47 feature enum identifiers and G_Cod1GscFeature47
are missing in g_client.c, g_cmds.c (use selector), g_cod1_author_fog.c
and g_trigger.c (damage/look-at). The first GAME compile group in
tools/windows-x86.json uses headers 609A. Both builders overlay
src/game/g_cod1_gsc_test.h from tools/source-revisions/609A/game/.
That exact historical header predates R47. The next group restores
the current header; changing historical ABI header sets is not needed.

Move the current declarations and disabled-build lifecycle no-op
macros to g_cod1_campaign_api47.h, with its own include guard.
Include it from the current GSC header and directly from the four
affected translation units. Keep the 609A overlay, recipe and all
historic headers unchanged. No algorithms, settings or resource
changes. Other R47 users in the second and third GAME groups already
receive the current header.

## Tests actually run
Compile-only fixture uses the actual interface against both the
609A overlay and the patched current GSC header, in both inclusion
orders and with COD1_GSC_TEST_WIP 0 and 1. GCC and Clang C11
-Wall -Wextra -Werror: 16 compilations PASS. Exercises all five
feature identifiers reported missing and three lifecycle declarations.
Historical header hash 6fa34e51bd5b62970bf81e34a4effce646aab215,
current patched header 75627353ac842dbed282633a186e1cfc2ca534a4,
feature descriptor c38bb7ee21c4d74ab0967e59a481d93f21079448,
API eeb7ca2b0655bb7ef481342dab4ea770442697dc,
fixture 85bc9a41eb1373c08226b40854c8e1e6f6e6bd10.
Local files reconstructed and hashes checked. Reproduce from checkout:

    for cc in gcc clang; do
      for overlay in tools/source-revisions/609A/game src/game; do
        for mode in 0 1; do
          for order in '' '-DR47G_API_FIRST=1'; do
            "$cc" -std=c11 -Wall -Wextra -Werror -DCOD1_GSC_TEST_WIP=$mode $order -I"$overlay" -Isrc/game -c tools/tests/opt_r47g_campaign_api_test.c -o /tmp/r47g.o || exit 1
          done
        done
      done
    done

Not a full MSVC/GAME compile or gameplay run. Rebuild from clean output
on Windows; inspect later groups/link for additional errors.
R47F experimental native lane and legacy control remain as before.
