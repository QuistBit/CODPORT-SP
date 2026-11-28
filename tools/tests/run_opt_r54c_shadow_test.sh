#!/usr/bin/env bash
set -euo pipefail
cd "$(git rev-parse --show-toplevel)"
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
p=src/game/cod1_gsc
base=78f86c5981856b22459084fa9d3209c531eee145
candidate=5e5e69e27a22160a5560879b318076ddcf5234cb
for pair in "old:$base" "new:$candidate"; do
  name=${pair%%:*}; rev=${pair#*:}
  git show "$rev:$p/gsc_vm_r237_frame_buckets.inc" | awk '/^static int r237_drain_bucket_slice53\(/ {active=1} /^static int r237_drain_bucket\(GscRuntime/ {active=0} active {print}' > "$tmp/${name}_drain.inc"
  test -s "$tmp/${name}_drain.inc"
done
read -r -a flags <<< "${CFLAGS:--std=c11 -O2 -Wall -Wextra -Werror}"
for test in queue_shadow drain; do
  "${CC:-cc}" "${flags[@]}" -I"$p" -I"$tmp" "tools/tests/opt_r54c_${test}_test.c" -o "$tmp/$test"
  "$tmp/$test"
done
