#!/usr/bin/env bash
set -euo pipefail
cd "$(git rev-parse --show-toplevel)"
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
p=src/game/cod1_gsc/gsc_vm_state_arena.inc
base=13ea4741e9e47ea11c2402f3a8ae88333fcc9aaa
candidate=76cd4e732943eea52b4f701899dd97c909485273
for pair in "old:$base" "new:$candidate"; do
  name=${pair%%:*}; rev=${pair#*:}
  git show "$rev:$p" | awk '/^static uint32_t state_fields_limit\(/ {active=1} /^\/\* Allocator hooks/ {active=0} active {print}' > "$tmp/$name.inc"
  git show "$rev:$p" | awk '/^static int state_fields_reserve_free\(/ {active=1} /^static void state_fields_release\(/ {active=0} active {print}' > "$tmp/${name}_reserve.inc"
  test -s "$tmp/$name.inc" && test -s "$tmp/${name}_reserve.inc"
done
read -r -a flags <<< "${CFLAGS:--std=c11 -O2 -Wall -Wextra -Werror -Wno-misleading-indentation}"
"${CC:-cc}" "${flags[@]}" -I"$tmp" tools/tests/opt_r55c2a_state_reserve_test.c -o "$tmp/test"
"$tmp/test"
