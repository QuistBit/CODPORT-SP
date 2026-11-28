#!/bin/sh
set -eu
cd "$(git rev-parse --show-toplevel)"
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT HUP INT TERM
src=src/game/cod1_gsc/gsc_vm_array_cow_r238.inc
git show 2febee854737531f55e70d95cbd0103c4edefa4d:"$src" | sed -n '/^static int r238_stage_clone(/,/^static int r238_stage_reaches(/{ /^static int r238_stage_reaches(/d; p; }' > "$tmp/original.inc"
sed -n '/^static int r238_stage_clone(/,/^static int r238_stage_reaches(/{ /^static int r238_stage_reaches(/d; p; }' "$src" > "$tmp/candidate.inc"
test "$(git hash-object "$tmp/original.inc")" = d145718a5855fd37f8c884a917c39f28d9447eac
test "$(git hash-object "$tmp/candidate.inc")" = 8e2823458b66af00dc4eb996b65349fa06df119f
cp tools/tests/opt_r48a_array_clone_test.c "$tmp/test.c"
cc -std=c11 -O2 -Wall -Wextra -Werror "$tmp/test.c" -o "$tmp/test"
"$tmp/test"
