#!/bin/sh
set -eu
cd "$(git rev-parse --show-toplevel)"
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT HUP INT TERM
tree=src/qcommon/cod1_actor_generic_tree14.h
clock=src/qcommon/cod1_actor_tree_clock14.h
active=src/qcommon/cod1_actor_active_paths37.h
{
 grep '^static int C1ATL14_Finite(' "$clock"
 sed -n '/^static int C1AGT14_Descendant(/,/^#include "cod1_actor_active_paths37.h"/{ /^#include "cod1_actor_active_paths37.h"/d; p; }' "$tree"
 sed -n '/^static void C1AGT37_ActivePaths(/,/^#endif/{ /^#endif/d; p; }' "$active"
 sed -n '/^static C1ATL14_NOINLINE int C1ATL14_ClearGoalInner(/,/^static int C1ATL14_ClearGoal(/{ /^static int C1ATL14_ClearGoal(/d; s/^static C1ATL14_NOINLINE int/static int/; p; }' "$clock"
} > "$tmp/deps.inc"
sed -n '/^\/\* The sibling clears cannot change active-path membership/,/^\/\* Only a canonical, already-cleared inactive node/{ /^\/\* Only a canonical, already-cleared inactive node/d; p; }' "$tree" > "$tmp/helper.inc"
test "$(git hash-object "$tmp/deps.inc")" = 50ff7cd26b964c37ddf3ad7b6e6917a7b3fce3cb
test "$(git hash-object "$tmp/helper.inc")" = 11f7a8ff5c920713c269a578e4866a8913b62789
cp tools/tests/opt_r49b_sibling_paths_test.c "$tmp/test.c"
cc -std=c11 -O2 -Wall -Wextra -Werror -Wno-misleading-indentation -frounding-math "$tmp/test.c" -lm -o "$tmp/test"
"$tmp/test"
