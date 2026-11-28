#!/usr/bin/env bash
set -euo pipefail
cd "$(git rev-parse --show-toplevel)"
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
p=src/game/gsc_host
rev=1bfc6b12659e98eb4ff2a7796aeaa9b098eaddcc
git show "$rev:$p/g_cod1_script_sound_notify_host.inc" | awk '/^static int GscSoundRead\(/ {active=1} /^static int GscSoundExists\(/ {active=0} active {print}' > "$tmp/reader.inc"
git show "$rev:$p/bg_cod1_sound_read56.inc" > "$tmp/bg_cod1_sound_read56.inc"
test -s "$tmp/reader.inc" && test -s "$tmp/bg_cod1_sound_read56.inc"
read -r -a flags <<< "${CFLAGS:--std=c11 -O2 -Wall -Wextra -Werror}"
"${CC:-cc}" "${flags[@]}" -I"$tmp" tools/tests/opt_r56a3a_sound_read_test.c -o "$tmp/test"
"$tmp/test"
