#!/usr/bin/env bash
set -euo pipefail
cd "$(git rev-parse --show-toplevel)"
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
p=src/game/cod1_gsc
base=57cdc3f1e01bfdfdaad5b25113c27dded31a9586
candidate=1e0e9038f05a218cdacc5b873716fd51b9db18f1
git show "$base:$p/gsc_vm_r237_frame_buckets.inc" | awk '/^static int r237_drain_bucket_slice53\(/ {active=1} /^static int r237_drain_bucket\(GscRuntime/ {active=0} active {print}' > "$tmp/new_drain.inc"
git show "$candidate:$p/gsc_vm_r237_frame_buckets.inc" | awk '/^static int r237_drain_bucket_core54\(/ {active=1} /^#include "gsc_vm_r237_dirty_drain54.inc"/ {active=0} active {print}' > "$tmp/dirty_core.inc"
test -s "$tmp/new_drain.inc" && test -s "$tmp/dirty_core.inc"
for name in dirty_scope dirty_audit dirty_drain verified_choice queue_shadow; do
  git show "$candidate:$p/gsc_vm_r237_${name}54.inc" > "$tmp/gsc_vm_r237_${name}54.inc"
done
read -r -a flags <<< "${CFLAGS:--std=c11 -O2 -Wall -Wextra -Werror}"
"${CC:-cc}" "${flags[@]}" -I"$tmp" -I"$p" tools/tests/opt_r54d3b_verified_test.c -o "$tmp/test"
"$tmp/test"
