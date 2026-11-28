#!/bin/sh
set -eu
cd "$(git rev-parse --show-toplevel)"
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT HUP INT TERM
q=src/qcommon
tree=$q/cod1_actor_generic_tree14.h
clock=$q/cod1_actor_tree_clock14.h
active=$q/cod1_actor_active_paths37.h
git show 1312d9afd3fca91846887a44f75f0c210434e294:"$q/cod1_actor_knob_all14.h" > "$tmp/knob.old"
git show 1312d9afd3fca91846887a44f75f0c210434e294:"$q/cod1_actor_ancestor_knob14.h" > "$tmp/ancestor.old"
cp "$q/cod1_actor_knob_all14.h" "$tmp/knob.new"
cp "$q/cod1_actor_ancestor_knob14.h" "$tmp/ancestor.new"
{
 grep '^static int C1ATL14_Finite(' "$clock"
 sed -n '/^static int C1AGT14_Descendant(/,/^#include "cod1_actor_active_paths37.h"/{ /^#include "cod1_actor_active_paths37.h"/d; p; }' "$tree"
 sed -n '/^static void C1AGT37_ActivePaths(/,/^#endif/{ /^#endif/d; p; }' "$active"
 sed -n '/^static C1ATL14_NOINLINE int C1ATL14_ClearGoalInner(/,/^static int C1ATL14_ClearGoal(/{ /^static int C1ATL14_ClearGoal(/d; s/^static C1ATL14_NOINLINE int/static int/; p; }' "$clock"
} > "$tmp/deps.inc"
sed -n '/^\/\* The sibling clears cannot change active-path membership/,/^\/\* Only a canonical, already-cleared inactive node/{ /^\/\* Only a canonical, already-cleared inactive node/d; p; }' "$tree" > "$tmp/helper.inc"
sed -n '/^static int C1AGT14_ValidScalar(/,/^\/\* Exact per-thread topology proof/{ /^\/\* Exact per-thread topology proof/d; p; }' "$tree" > "$tmp/valid.inc"
sed -n '/^static C1ATL14_NOINLINE int C1ATL14_SetGoalInner(/,/^static C1ATL14_NOINLINE int C1ATL14_AdvanceWeightInner(/{ /^static C1ATL14_NOINLINE int C1ATL14_AdvanceWeightInner(/d; p; }' "$clock" > "$tmp/goal.inc"
test "$(git hash-object "$tmp/knob.old")" = f52b471553f4843f2e07fd658ab4639656171f7a
test "$(git hash-object "$tmp/ancestor.old")" = 602bf278aca52bd7ab825cb9ecbbb8acce9273cf
test "$(git hash-object "$tmp/knob.new")" = 55c362b02e8550f02034a9650be459b37fa107cd
test "$(git hash-object "$tmp/ancestor.new")" = 26ac556f1b894c7478bf51a30e04881f3c91a685
test "$(git hash-object "$tmp/deps.inc")" = 50ff7cd26b964c37ddf3ad7b6e6917a7b3fce3cb
test "$(git hash-object "$tmp/helper.inc")" = 11f7a8ff5c920713c269a578e4866a8913b62789
test "$(git hash-object "$tmp/valid.inc")" = a5cf2de33e1e258a339ee37d676bbf1e10f5c235
test "$(git hash-object "$tmp/goal.inc")" = be4294a25441d703d052db237d64a023907c724e
touch "$tmp/cod1_actor_generic_tree14.h" "$tmp/cod1_actor_knob_all14.h"
cp tools/tests/opt_r49c_disposable_knob_test.c "$tmp/test.c"
cc -std=c11 -O2 -Wall -Wextra -Werror -Wno-misleading-indentation -frounding-math "$tmp/test.c" -lm -o "$tmp/test"
"$tmp/test"
