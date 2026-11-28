#!/usr/bin/env bash
set -euo pipefail
cd "$(git rev-parse --show-toplevel)"
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
base=37c40a272ae5e3251abf826deb3c518fcfbd6ee3
git show "$base:src/game/bg_cod1_script_sound.h" | awk '/^static Cod1SSResourceStatus Cod1ScriptSoundParseTableMode\(/ {active=1;sub(/Cod1ScriptSoundParseTableMode\(/,"ReferenceParse56(")} /^static Cod1SSResourceStatus Cod1ScriptSoundParseTableSelected\(/ {active=0} active {print}' > "$tmp/reference_parser.inc"
old=0ca75247a13b0efb84ec8b388f2088332bcff220
git show "$old:src/game/bg_cod1_script_sound.h" | awk '/^static Cod1SSResourceStatus Cod1ScriptSoundResolveMode\(/ {active=1;sub(/Cod1ScriptSoundResolveMode\(/,"ReferenceResolve56(")} /^static Cod1SSResourceStatus Cod1ScriptSoundResolveSelected\(/ {active=0} active {gsub(/Cod1ScriptSoundParseTableMode\(/,"ReferenceParse56(");print}' > "$tmp/reference_resolver.inc"
test -s "$tmp/reference_parser.inc" && test -s "$tmp/reference_resolver.inc"
sed 's/#include "reference.inc"/#include "reference_parser.inc"\n#include "reference_resolver.inc"/' tools/tests/opt_r56a2_absence_cache_test.c > "$tmp/resolver_test.c"
read -r -a flags <<< "${CFLAGS:--std=c11 -O2 -Wall -Wextra -Werror -Wno-unused-function -Wno-misleading-indentation}"
"${CC:-cc}" "${flags[@]}" -Isrc/game -I"$tmp" tools/tests/opt_r56a2b_rows_test.c -lm -o "$tmp/rows"
"$tmp/rows"
"${CC:-cc}" "${flags[@]}" -Isrc/game -I"$tmp" "$tmp/resolver_test.c" -o "$tmp/resolver"
"$tmp/resolver"
