#!/usr/bin/env bash
set -euo pipefail
cd "$(git rev-parse --show-toplevel)"
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
read -r -a flags <<< "${CFLAGS:--std=c11 -O2 -Wall -Wextra -Werror}"
for test in nested scope; do
  "${CC:-cc}" "${flags[@]}" -Isrc/game/cod1_gsc "tools/tests/opt_r55b_${test}_test.c" -o "$tmp/$test"
  "$tmp/$test"
done
