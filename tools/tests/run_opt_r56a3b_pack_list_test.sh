#!/usr/bin/env bash
set -euo pipefail
cd "$(git rev-parse --show-toplevel)"
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
base=c2c1f12bf00cb548f90932353b3c7a18cac45189
git show "$base:src/game/q_shared.c" | awk '/^int Q_(islower|stricmpn|stricmp)\(/ {active=1;sub(/^int /,"static int ")} active {print} /^}/ {active=0}' > "$tmp/reference.inc"
git show "$base:src/qcommon/files.c" | awk '/^static int FS_ReturnPath\(/ {active=1} active {print} /^}/ {active=0}' >> "$tmp/reference.inc"
test -s "$tmp/reference.inc"
read -r -a flags <<< "${CFLAGS:--std=c11 -O2 -Wall -Wextra -Werror}"
"${CC:-cc}" "${flags[@]}" -Isrc/qcommon -I"$tmp" tools/tests/opt_r56a3b_pack_list_test.c -o "$tmp/test"
"$tmp/test"
