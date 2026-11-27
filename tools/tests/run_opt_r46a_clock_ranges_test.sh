#!/bin/sh
set -eu
cd "$(git rev-parse --show-toplevel)"
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT HUP INT TERM
src=src/qcommon/cod1_actor_generic_clock14.h
ref=8f92680c078b717f61aa762ef9a5c93e10eb99cb
git show "$ref:$src" > "$tmp/reference.h"
awk '/^static int C1AGC14_AdvanceStorage\(/ {on=1} on {print}' "$tmp/reference.h" > "$tmp/reference.inc"
test "$(git hash-object "$tmp/reference.inc")" = 65a742677949fefab6cba9b24055b9601fa743af
awk '/^static int C1AGC14_AdvanceStorageIndexed\(/ {on=1} on {print}' "$src" > "$tmp/candidate.inc"
sed '$d' "$tmp/reference.inc" > "$tmp/old.inc"
sed '$d' "$tmp/candidate.inc" > "$tmp/new.inc"
for mode in strict san; do
 flags=-O2
 if [ "$mode" = san ]; then flags='-O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer -no-pie'; fi
 ${CC:-cc} -std=c99 -Wall -Wextra -Werror $flags -Isrc/qcommon tools/tests/opt_r46a_clock_ranges_test.c -o "$tmp/ranges-$mode"
 "$tmp/ranges-$mode"
 ${CC:-cc} -std=c99 -Wall -Wextra -Werror $flags -I"$tmp" -Isrc/qcommon tools/tests/opt_r46a_clock_storage_test.c -o "$tmp/storage-$mode"
 "$tmp/storage-$mode"
done
