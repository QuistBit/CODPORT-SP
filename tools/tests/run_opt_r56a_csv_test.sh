#!/usr/bin/env bash
set -euo pipefail
cd "$(git rev-parse --show-toplevel)"
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
p=src/game/bg_cod1_script_sound.h
base=9d73377adbd3660201714494e46fcab997c821dc
git show "$base:$p" | awk '/^static const char \*Cod1SS_Delimiter\(/ {active=1} /^static int Cod1SS_Number\(/ {active=0} active {print}' > "$tmp/cell.inc"
cp src/game/bg_cod1_sound_skip56.inc "$tmp/skip.inc"
test -s "$tmp/cell.inc" && test -s "$tmp/skip.inc"
read -r -a flags <<< "${CFLAGS:--std=c11 -O2 -Wall -Wextra -Werror -Wno-misleading-indentation}"
"${CC:-cc}" "${flags[@]}" -I"$tmp" tools/tests/opt_r56a_csv_test.c -o "$tmp/test"
"$tmp/test"
