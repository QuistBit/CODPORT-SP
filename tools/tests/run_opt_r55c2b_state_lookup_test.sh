#!/usr/bin/env bash
set -euo pipefail
cd "$(git rev-parse --show-toplevel)"
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
p=src/game/cod1_gsc
base=51fffdce7d22ce0a0407405ffc0bfbfc9acc2388
candidate=dd20a3bce2f52139b7e0ac7d5416ee95afae64de
for pair in "old:$base" "new:$candidate"; do
  name=${pair%%:*}; rev=${pair#*:}
  git show "$rev:$p/gsc_vm_state.inc" | awk '/^static int state_field_index\(/ {active=1} /^static int state_field_read\(/ {active=0} active {print}' > "$tmp/$name.inc"
  test -s "$tmp/$name.inc"
done
git show "$candidate:$p/gsc_vm_state_bound55.inc" > "$tmp/bound.inc"
git show "$candidate:$p/gsc_vm_state_arena.inc" | awk '/^static uint32_t state_fields_limit\(/ {active=1} /^\/\* Allocator hooks/ {active=0} active {print}' > "$tmp/reserve_helpers.inc"
git show "$candidate:$p/gsc_vm_state_arena.inc" | awk '/^static int state_fields_reserve_free\(/ {active=1} /^int gsc_runtime_select_owned_state_arena\(/ {active=0} active {print}' > "$tmp/reserve.inc"
test -s "$tmp/bound.inc" && test -s "$tmp/reserve_helpers.inc" && test -s "$tmp/reserve.inc"
read -r -a flags <<< "${CFLAGS:--std=c11 -O2 -Wall -Wextra -Werror -Wno-misleading-indentation}"
for test in state_lookup state_bound_lifecycle; do
  "${CC:-cc}" "${flags[@]}" -I"$tmp" "tools/tests/opt_r55c2b_${test}_test.c" -o "$tmp/$test"
  "$tmp/$test"
done
