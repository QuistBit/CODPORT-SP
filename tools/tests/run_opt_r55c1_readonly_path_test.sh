#!/usr/bin/env bash
set -euo pipefail
cd "$(git rev-parse --show-toplevel)"
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
p=src/game/cod1_gsc/gsc_vm_results.inc
base=b39adf0c2561a5eb0dfde35f2dc4b47a6cb84d1d
candidate=f47ad9b9b1f3e6017aea2123069c8ae64962e6d0
for pair in "old:$base" "new:$candidate"; do
  name=${pair%%:*}; rev=${pair#*:}
  git show "$rev:$p" | awk '/^static int results_readonly_path\(/ {active=1} /^static int results_readonly_guards\(/ {active=0} active {print}' > "$tmp/$name.inc"
  test -s "$tmp/$name.inc"
done
read -r -a flags <<< "${CFLAGS:--std=c11 -O2 -Wall -Wextra -Werror}"
"${CC:-cc}" "${flags[@]}" -I"$tmp" tools/tests/opt_r55c1_readonly_path_test.c -o "$tmp/test"
"$tmp/test"
