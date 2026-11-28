#!/usr/bin/env bash
set -euo pipefail
cd "$(git rev-parse --show-toplevel)"
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
read -r -a flags <<< "${CFLAGS:--std=c11 -O2 -Wall -Wextra -Werror}"
"${CC:-cc}" "${flags[@]}" -Isrc/game/cod1_gsc tools/tests/opt_r55c3b_array_lookup_test.c -o "$tmp/test"
"$tmp/test"
