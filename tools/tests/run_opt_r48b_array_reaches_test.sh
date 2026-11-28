#!/bin/sh
set -eu
cd "$(git rev-parse --show-toplevel)"
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT HUP INT TERM
src=src/game/cod1_gsc/gsc_vm_array_cow_r238.inc
git show 28c036e7771546111e8a3f951c08a40d6ae34a01:"$src" | sed -n '/^static int r238_stage_reaches(/,/^static int r238_array_cow_commit(/{ /^static int r238_array_cow_commit(/d; p; }' > "$tmp/original.inc"
sed -n '/^\/\* One callback-free walk for the ordered ancestor checks/,/^static int r238_array_cow_commit(/{ /^static int r238_array_cow_commit(/d; p; }' "$src" > "$tmp/candidate.inc"
test "$(git hash-object "$tmp/original.inc")" = d010396ebdda7425cb6581edc12f0e1df67175ef
test "$(git hash-object "$tmp/candidate.inc")" = f0acdcb902b251aac7a93abcbab9bf078cac31c1
cp tools/tests/opt_r48b_array_reaches_test.c "$tmp/test.c"
cc -std=c11 -O2 -Wall -Wextra -Werror "$tmp/test.c" -o "$tmp/test"
"$tmp/test"
