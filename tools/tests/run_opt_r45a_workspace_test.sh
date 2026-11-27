#!/bin/sh
set -eu
cd "$(git rev-parse --show-toplevel)"
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT HUP INT TERM
src=src/game/gsc_host/g_cod1_actor_generic_host.inc
test=tools/tests/opt_r45a_workspace_test.c
extract() {
 awk '
 /^\/\* Main-thread GAME transactions/ {on=1}
 on && /^typedef struct \{$/ {copy=1}
 copy && /^static int ActorGenericNamedActive/ {exit}
 copy {print}
 ' "$1"
}
extract "$src" > "$tmp/clone.inc"
test -s "$tmp/clone.inc"
git show 8f92680c078b717f61aa762ef9a5c93e10eb99cb:"$src" > "$tmp/r40.inc"
extract "$tmp/r40.inc" > "$tmp/reference.inc"
cmp "$tmp/clone.inc" "$tmp/reference.inc"
${CC:-cc} -std=c99 -O2 -Wall -Wextra -Werror -I"$tmp" "$test" -o "$tmp/strict"
"$tmp/strict"
${CC:-cc} -std=c99 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -fno-omit-frame-pointer -no-pie -I"$tmp" "$test" -o "$tmp/sanitized"
"$tmp/sanitized"
