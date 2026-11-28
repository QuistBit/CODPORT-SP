#!/usr/bin/env bash
set -euo pipefail
cd "$(git rev-parse --show-toplevel)"
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
p=src/game/cod1_gsc
base=57cdc3f1e01bfdfdaad5b25113c27dded31a9586
candidate=f9febc8386f2d545824718660e44cef926d1c160
git show "$base:$p/gsc_vm_r237_frame_buckets.inc" | awk '/^static int r237_drain_bucket_slice53\(/ {active=1} /^static int r237_drain_bucket\(GscRuntime/ {active=0} active {print}' > "$tmp/new_drain.inc"
git show "$candidate:$p/gsc_vm_r237_frame_buckets.inc" | awk '/^static int r237_drain_bucket_core54\(/ {active=1} /^#include "gsc_vm_r237_dirty_drain54.inc"/ {active=0} active {print}' > "$tmp/dirty_core.inc"
test -s "$tmp/new_drain.inc" && test -s "$tmp/dirty_core.inc"
read -r -a flags <<< "${CFLAGS:--std=c11 -O2 -Wall -Wextra -Werror}"
"${CC:-cc}" "${flags[@]}" -I"$p" -I"$tmp" tools/tests/opt_r54d2_scope_test.c -o "$tmp/test"
"$tmp/test"
