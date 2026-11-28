#!/bin/sh
set -eu
cd "$(git rev-parse --show-toplevel)"
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT HUP INT TERM
src=src/game/cod1_gsc/gsc_r232_function_admission.inc
git show 569e40e324cf0b43bb3f686583c5e049db186a06:"$src" | sed -n '/^static int admission_target_push(/,/^\/\* Parser-wide OR flags/{ /^\/\* Parser-wide OR flags/d; p; }' > "$tmp/original.inc"
sed -n '/^static int admission_target_push(/,/^\/\* Parser-wide OR flags/{ /^\/\* Parser-wide OR flags/d; p; }' "$src" > "$tmp/candidate.inc"
test "$(git hash-object "$tmp/original.inc")" = 68d32c0d1e8f1c2a3017e76b39f6c2539f7b580e
test "$(git hash-object "$tmp/candidate.inc")" = 9d8cbd6b27836e9d9e5f2bb74ac9f4bc28a1551d
cp tools/tests/opt_r48c_function_closure_test.c "$tmp/test.c"
cc -std=c11 -O2 -Wall -Wextra -Werror -Wno-misleading-indentation "$tmp/test.c" -o "$tmp/test"
"$tmp/test"
