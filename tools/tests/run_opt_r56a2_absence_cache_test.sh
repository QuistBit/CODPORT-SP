#!/usr/bin/env bash
set -euo pipefail
cd "$(git rev-parse --show-toplevel)"
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
base=0ca75247a13b0efb84ec8b388f2088332bcff220
git show "$base:src/game/bg_cod1_script_sound.h" | awk '/^static Cod1SSResourceStatus Cod1ScriptSoundResolveMode\(/ {active=1;sub(/Cod1ScriptSoundResolveMode\(/,"ReferenceResolve56(")} /^static Cod1SSResourceStatus Cod1ScriptSoundResolveSelected\(/ {active=0} active {print}' > "$tmp/reference.inc"
test -s "$tmp/reference.inc"
read -r -a flags <<< "${CFLAGS:--std=c11 -O2 -Wall -Wextra -Werror -Wno-unused-function -Wno-misleading-indentation}"
"${CC:-cc}" "${flags[@]}" -Isrc/game -I"$tmp" tools/tests/opt_r56a2_absence_cache_test.c -o "$tmp/test"
"$tmp/test"
