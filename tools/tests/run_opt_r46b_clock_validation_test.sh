#!/bin/sh
set -eu
cd "$(git rev-parse --show-toplevel)"
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT HUP INT TERM
src=src/qcommon
ref=8f92680c078b717f61aa762ef9a5c93e10eb99cb
for file in cod1_actor_generic_clock14.h cod1_actor_generic_metadata14.h cod1_actor_generic_tree14.h cod1_actor_generic_frequency14.h cod1_actor_named_clock14.h; do
 git show "$ref:$src/$file" > "$tmp/ref-$file"
done
awk '/^static int C1AGC14_Cycles\(/ {on=1} /^static int C1AGC14_Inner\(/ {exit} on {print}' "$tmp/ref-cod1_actor_generic_clock14.h" > "$tmp/kernel.inc"
awk '/^static int C1AGC14_Inner\(/ {on=1} /^static int C1AGC14_AdvanceStorage\(/ {exit} on {print}' "$tmp/ref-cod1_actor_generic_clock14.h" > "$tmp/old_inner.inc"
awk '/^\/\* A validation ledger belongs to one synchronous admission only/ {on=1} /^static int C1AGC14_AdvanceStorageIndexed\(/ {exit} on {print}' "$src/cod1_actor_generic_clock14.h" > "$tmp/new_inner.inc"
awk '/^static int C1AGC14_Cycles\(/ {on=1} /^\/\* A validation ledger belongs to one synchronous admission only/ {exit} on {print}' "$src/cod1_actor_generic_clock14.h" > "$tmp/current_kernel.inc"
# The arithmetic is deliberately shared by the two extracted admissions.
# Fail if future changes invalidate that assumption, rather than silently
# test old arithmetic for a changed implementation.
cmp "$tmp/kernel.inc" "$tmp/current_kernel.inc"
awk '/^static int C1AGM14_Valid\(/ {on=1} /^static int C1AGM14_BuildInner\(/ {exit} on {print}' "$tmp/ref-cod1_actor_generic_metadata14.h" > "$tmp/metadata.inc"
awk '/^static int C1AGT14_ValidScalar\(/ {on=1} /^\/\* Exact per-thread topology proof/ {exit} on {print}' "$tmp/ref-cod1_actor_generic_tree14.h" > "$tmp/tree.inc"
{
 awk '/^static double C1AGF14_Recurse\(/ {on=1} /^static int C1AGF14_Frequency\(/ {exit} on {print}' "$tmp/ref-cod1_actor_generic_frequency14.h"
 awk '/^static int C1AGT14_Descendant\(/ {on=1} /^static int C1AGT14_Active\(/ {exit} on {print}' "$tmp/ref-cod1_actor_generic_tree14.h"
 awk '/^static unsigned C1ANC14_Cursor\(/ {on=1} /^static int C1ANC14_Inner\(/ {exit} on {print}' "$tmp/ref-cod1_actor_named_clock14.h"
} > "$tmp/deps.inc"
# Confirm dependencies shared by both revisions (scalar validator path).
awk '/^static int C1AGM14_Valid\(/ {on=1} /^static int C1AGM14_BuildInner\(/ {exit} on {print}' "$src/cod1_actor_generic_metadata14.h" > "$tmp/current_metadata.inc"
cmp "$tmp/metadata.inc" "$tmp/current_metadata.inc"
awk '/^static int C1AGT14_ValidScalar\(/ {on=1} /^\/\* Exact per-thread topology proof/ {exit} on {print}' "$src/cod1_actor_generic_tree14.h" > "$tmp/current_tree.inc"
cmp "$tmp/tree.inc" "$tmp/current_tree.inc"
{
 awk '/^static double C1AGF14_Recurse\(/ {on=1} /^static int C1AGF14_Frequency\(/ {exit} on {print}' "$src/cod1_actor_generic_frequency14.h"
 awk '/^static int C1AGT14_Descendant\(/ {on=1} /^static int C1AGT14_Active\(/ {exit} on {print}' "$src/cod1_actor_generic_tree14.h"
 awk '/^static unsigned C1ANC14_Cursor\(/ {on=1} /^static int C1ANC14_Inner\(/ {exit} on {print}' "$src/cod1_actor_named_clock14.h"
} > "$tmp/current_deps.inc"
cmp "$tmp/deps.inc" "$tmp/current_deps.inc"
for mode in strict san; do
 flags=-O2
 if [ "$mode" = san ]; then flags='-O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer -no-pie'; fi
 ${CC:-cc} -std=c99 -Wall -Wextra -Werror -Wno-misleading-indentation -frounding-math $flags -I"$tmp" tools/tests/opt_r46b_clock_validation_test.c -lm -o "$tmp/$mode"
 "$tmp/$mode"
done
