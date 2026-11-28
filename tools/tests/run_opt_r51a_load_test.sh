#!/bin/sh
set -eu
cd "$(git rev-parse --show-toplevel)"
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT HUP INT TERM
base=64199fba6448f3b7924f0c18560dbb1822e195ca
mkdir -p "$tmp/src/qcommon" "$tmp/src/cgame"
git show "$base:src/qcommon/cod1_load_profile.h" > "$tmp/old.h"
cp src/qcommon/cod1_load_profile.h "$tmp/src/qcommon/"
cp src/cgame/cg_cod1_load_profile51.h "$tmp/src/cgame/"
git show "$base:src/renderer/cod1_material_stype.h" | sed -n '/^static int Cod1ST_PrepareAuth(/,/^static int Cod1ST_Prepare(/{ /^static int Cod1ST_Prepare(/d; p; }' > "$tmp/stype.old"
sed -n '/^static int Cod1ST_PrepareAuth(/,/^static int Cod1ST_Prepare(/{ /^static int Cod1ST_Prepare(/d; p; }' src/renderer/cod1_material_stype.h > "$tmp/stype.new"
cp tools/tests/opt_r51a_load_profile_test.c "$tmp/test_profile.c"
cp tools/tests/opt_r51a_stype_scratch_test.c "$tmp/test_stype.c"
test "$(git hash-object "$tmp/old.h")" = becf91c1b03df28f7466f7c2292260e07a49962c
test "$(git hash-object "$tmp/src/qcommon/cod1_load_profile.h")" = 931a4ec62936b25a571f6b058ef17cb23875cb76
test "$(git hash-object "$tmp/src/cgame/cg_cod1_load_profile51.h")" = 3db2e1dc9b3459665cd9a5f5d6f13446d09c53be
test "$(git hash-object "$tmp/stype.old")" = 6756f8251c00b1e3e837d68543c22b3fb5de4cf3
test "$(git hash-object "$tmp/stype.new")" = 05b7ce56b6bf4af33ad16b08add93d3b3fdb1559
test "$(git hash-object "$tmp/test_profile.c")" = 9370ba9d6ede1189bff62496aeab0f539ec4620b
test "$(git hash-object "$tmp/test_stype.c")" = 2c428dccf732eeffb56ac211fc84a55adf4cb2d6
cc -std=c11 -O2 -Wall -Wextra -Werror "$tmp/test_profile.c" -o "$tmp/profile"
cc -std=c11 -O2 -Wall -Wextra -Werror "$tmp/test_stype.c" -o "$tmp/stype"
"$tmp/profile"
"$tmp/stype"
