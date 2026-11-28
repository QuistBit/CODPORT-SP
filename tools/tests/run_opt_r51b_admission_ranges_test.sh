#!/bin/sh
set -eu
cd "$(git rev-parse --show-toplevel)"
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT HUP INT TERM
src=src/game/cod1_gsc/gsc_r232_function_admission.inc
git show c29f28b08cb1a5cd421daaf7aea7c02eb6e84ea3:"$src" | sed -n '/^static void function_admission_requirements(/,/^static void function_admission_hide_body(/{ /^static void function_admission_hide_body(/d; p; }' > "$tmp/old.inc"
sed -n '/^\/\* Only masked single-function views/,/^static void function_admission_hide_body(/{ /^static void function_admission_hide_body(/d; p; }' "$src" > "$tmp/new.inc"
test "$(git hash-object "$tmp/old.inc")" = 8d9442a84df0a68cef014de31803c6d4a178f2a9
test "$(git hash-object "$tmp/new.inc")" = c6b56bf1fa8bacdd5688bc4819c804ff638f8aa5
cp tools/tests/opt_r51b_admission_ranges_test.c "$tmp/test.c"
test "$(git hash-object "$tmp/test.c")" = 8422b38571107a2f6e04c4261a95ca7e4c746b70
cc -std=c11 -O2 -Wall -Wextra -Werror "$tmp/test.c" -o "$tmp/test"
"$tmp/test"
