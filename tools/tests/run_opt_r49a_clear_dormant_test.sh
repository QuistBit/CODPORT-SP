#!/bin/sh
set -eu
cd "$(git rev-parse --show-toplevel)"
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT HUP INT TERM
tree=src/qcommon/cod1_actor_generic_tree14.h
clock=src/qcommon/cod1_actor_tree_clock14.h
active=src/qcommon/cod1_actor_active_paths37.h
git show cd47153be9d75fb4e65683a7a584c854d229225a:"$tree" | sed -n '/^static int C1AGT14_ClearInner(/,/^static int C1AGT14_SetAnimInner(/{ /^static int C1AGT14_SetAnimInner(/d; p; }' > "$tmp/clear_old.inc"
sed -n '/^\/\* Only a canonical, already-cleared inactive node/,/^static int C1AGT14_SetAnimInner(/{ /^static int C1AGT14_SetAnimInner(/d; p; }' "$tree" > "$tmp/clear_new.inc"
{
 grep '^static int C1ATL14_Finite(' "$clock"
 sed -n '/^static int C1AGT14_ValidScalar(/,/^\/\* Exact per-thread topology proof/{ /^\/\* Exact per-thread topology proof/d; p; }' "$tree"
 grep '^static int C1AGT14_Valid(.*return C1AGT14_ValidScalar' "$tree"
 sed -n '/^static int C1AGT14_Descendant(/,/^#include "cod1_actor_active_paths37.h"/{ /^#include "cod1_actor_active_paths37.h"/d; p; }' "$tree"
 sed -n '/^static void C1AGT37_ActivePaths(/,/^#endif/{ /^#endif/d; p; }' "$active"
 sed -n '/^static C1ATL14_NOINLINE int C1ATL14_ClearGoalInner(/,/^static int C1ATL14_ClearGoal(/{ /^static int C1ATL14_ClearGoal(/d; s/^static C1ATL14_NOINLINE int/static int/; p; }' "$clock"
} > "$tmp/deps.inc"
test "$(git hash-object "$tmp/clear_old.inc")" = 6d12d9719629fc3fb4b65100651dd51066872214
test "$(git hash-object "$tmp/clear_new.inc")" = c70bbb0043320e492387193121f8a207a7203d61
test "$(git hash-object "$tmp/deps.inc")" = 93c3be22e6a910cd7b84b39c0668528147ab3de1
cp tools/tests/opt_r49a_clear_dormant_test.c "$tmp/test.c"
cc -std=c11 -O2 -Wall -Wextra -Werror -Wno-misleading-indentation -frounding-math "$tmp/test.c" -lm -o "$tmp/test"
"$tmp/test"
